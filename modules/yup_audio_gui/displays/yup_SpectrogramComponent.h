/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

//==============================================================================
/**
    A color map that converts normalized magnitude values (0.0 to 1.0) into
    ARGB colors for spectrogram visualization.

    The default color map produces a professional heatmap gradient:
    black → dark blue → blue → cyan → green → yellow → red → white.

    @tags{Audio}
*/
class YUP_API SpectrogramColorMap
{
public:
    //==============================================================================
    /** Predefined color map types. */
    enum class Type
    {
        heatmap,   ///< Black → blue → cyan → green → yellow → red → white
        grayscale, ///< Black → gray → white
        cool,      ///< Black → blue → cyan → white
        warm,      ///< Black → red → orange → yellow → white
        viridis    ///< Perceptually uniform blue → green → yellow
    };

    //==============================================================================
    /** Creates a color map of the given type with a specified number of color stops.

        @param type             The predefined color map type to use.
        @param numColorStops    Number of entries in the lookup table (default: 256).
    */
    SpectrogramColorMap (Type type = Type::heatmap, int numColorStops = 256);

    /** Destructor. */
    ~SpectrogramColorMap() = default;

    //==============================================================================
    /** Maps a normalized magnitude value to an ARGB color.

        @param normalizedMagnitude    Value in [0.0, 1.0] where 0.0 is minimum and 1.0 is maximum.
        @return                       The corresponding ARGB color (0xAARRGGBB).
    */
    uint32 map (float normalizedMagnitude) const noexcept;

    /** Returns the number of color stops in the lookup table. */
    int getNumColorStops() const noexcept { return numColorStops; }

private:
    //==============================================================================
    void buildHeatmap();
    void buildGrayscale();
    void buildCool();
    void buildWarm();
    void buildViridis();
    void buildFromGradient (ColorGradient gradient);

    //==============================================================================
    std::vector<uint32> colorTable;
    int numColorStops;
};

//==============================================================================
/**
    A component that displays a real-time scrolling spectrogram (waterfall display).

    This component performs FFT processing on audio data collected by a
    SpectrumAnalyzerState and renders the frequency spectrum over time as a
    scrolling waterfall display. Each new row of FFT data is rendered at the
    top of the display and previous rows scroll downward.

    The spectrogram image is stored in a GPU texture which is updated each frame
    by invalidating the existing texture and letting the renderer recreate it.
    Frequency and decibel grid lines are drawn as vector overlays.

    Example usage:

    @code
        SpectrumAnalyzerState analyzerState;
        SpectrogramComponent spectrogram (analyzerState);

        // Configure the display
        spectrogram.setFrequencyRange (20.0f, 20000.0f);
        spectrogram.setDecibelRange (-100.0f, 0.0f);
        spectrogram.setColorMap (SpectrogramColorMap::Type::heatmap);
        spectrogram.setUpdateRate (25);

        // In audio callback:
        analyzerState.pushSamples (audioData, numSamples);
    @endcode

    @see SpectrumAnalyzerState, SpectrumAnalyzerComponent, SpectrogramColorMap
*/
class YUP_API SpectrogramComponent
    : public Component
    , public Timer
{
public:
    //==============================================================================
    /** Display constants. */
    enum
    {
        defaultSpectrogramWidth = 512 ///< Default number of horizontal frequency bins.
    };

    //==============================================================================
    /** Creates a SpectrogramComponent.

        @param state    the SpectrumAnalyzerState that provides audio data
    */
    explicit SpectrogramComponent (SpectrumAnalyzerState& state);

    /** Destructor. */
    ~SpectrogramComponent() override;

    //==============================================================================
    /** Sets the FFT size for analysis.

        @param size    FFT size (must be a power of 2)
    */
    void setFFTSize (int size);

    /** Returns the current FFT size. */
    int getFFTSize() const noexcept { return analyzerState.getFftSize(); }

    //==============================================================================
    /** Sets the window function used for FFT processing.

        @param type    the window function type to use
    */
    void setWindowType (WindowType type);

    /** Returns the current window function type. */
    WindowType getWindowType() const noexcept { return currentWindowType; }

    //==============================================================================
    /** Sets the display update rate in Hz.

        @param hz    update rate (typical values: 10-30 Hz)
    */
    void setUpdateRate (int hz);

    /** Returns the current update rate in Hz. */
    int getUpdateRate() const noexcept;

    //==============================================================================
    /** Sets the frequency range for the display.

        @param minFreq    minimum frequency in Hz
        @param maxFreq    maximum frequency in Hz
    */
    void setFrequencyRange (float minFreq, float maxFreq);

    /** Returns the current minimum frequency. */
    float getMinFrequency() const noexcept { return minFrequency; }

    /** Returns the current maximum frequency. */
    float getMaxFrequency() const noexcept { return maxFrequency; }

    //==============================================================================
    /** Sets the decibel range used to normalize FFT magnitudes.

        @param minDb    minimum decibel level
        @param maxDb    maximum decibel level
    */
    void setDecibelRange (float minDb, float maxDb);

    /** Returns the current minimum decibel level. */
    float getMinDecibels() const noexcept { return minDecibels; }

    /** Returns the current maximum decibel level. */
    float getMaxDecibels() const noexcept { return maxDecibels; }

    //==============================================================================
    /** Sets the sample rate for frequency calculations.

        @param sampleRate    the sample rate in Hz
    */
    void setSampleRate (double sampleRate);

    /** Returns the current sample rate. */
    double getSampleRate() const noexcept { return sampleRate; }

    //==============================================================================
    /** Sets the color map used for the spectrogram.

        @param type    the color map type to use
    */
    void setColorMap (SpectrogramColorMap::Type type);

    /** Returns a reference to the current color map. */
    const SpectrogramColorMap& getColorMap() const noexcept { return colorMap; }

    //==============================================================================
    /** Sets the number of FFT frames kept in the scrolling history.

        This determines how many past FFT frames are visible in the display.
        The spectrogram image height is resized to match this value.

        @param numFrames    number of history frames (minimum: 4, default: matches component height)
    */
    void setNumHistoryFrames (int numFrames);

    /** Returns the number of history frames. */
    int getNumHistoryFrames() const noexcept { return numHistoryFrames; }

    //==============================================================================
    /** Sets the overlap factor for more responsive spectrum analysis.

        @param overlapFactor    overlap factor (0.0 = no overlap, 0.75 = 75% overlap)
    */
    void setOverlapFactor (float overlapFactor);

    /** Returns the current overlap factor. */
    float getOverlapFactor() const noexcept;

    //==============================================================================
    /** Clears the spectrogram history, resetting the display. */
    void clearHistory();

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void timerCallback() override;

private:
    //==============================================================================
    void processFFT();
    void updateSpectrogramImage();
    void scrollSpectrogram();
    void writeMagnitudeRow();
    void initializeFFTBuffers();
    void generateWindow();
    void ensureImageSize();

    float frequencyToX (float frequency, float width) const noexcept;
    void drawFrequencyGrid (Graphics& g, const Rectangle<float>& bounds);

    //==============================================================================
    SpectrumAnalyzerState& analyzerState;

    // FFT processing (performed on UI thread)
    std::unique_ptr<FFTProcessor> fftProcessor;
    std::vector<float> fftInputBuffer;
    std::vector<float> fftOutputBuffer;
    std::vector<float> windowBuffer;
    std::vector<float> magnitudeBuffer;   // Per-bin magnitudes from FFT
    std::vector<float> displayMagnitudes; // Magnitudes mapped to display bins

    // Spectrogram image
    Image spectrogramImage;
    SpectrogramColorMap colorMap;

    // Configuration
    WindowType currentWindowType = WindowType::hann;
    int fftSize = 4096;
    int spectrogramWidth = defaultSpectrogramWidth;
    int numHistoryFrames = 256;
    float minFrequency = 20.0f;
    float maxFrequency = 20000.0f;
    float logMinFrequency = std::log10 (20.0f);
    float logMaxFrequency = std::log10 (20000.0f);
    float minDecibels = -100.0f;
    float maxDecibels = 0.0f;
    double sampleRate = 44100.0;

    // Window compensation
    float windowGain = 1.0f;
    bool needsWindowUpdate = true;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrogramComponent)
};

} // namespace yup
