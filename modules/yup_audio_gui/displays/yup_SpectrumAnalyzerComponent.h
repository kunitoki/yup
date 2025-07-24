/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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
    A component that displays a real-time spectrum analyzer.

    This component performs FFT processing on audio data collected by a
    SpectrumAnalyzerState and renders the frequency spectrum as a visual display.
    The FFT processing is performed on the UI thread using a timer, following
    the pattern from the JUCE spectrum analyzer tutorial.

    The component can be configured with different window functions, display
    types, frequency ranges, and update rates. It automatically handles
    logarithmic frequency scaling for natural spectrum visualization.

    Example usage:
    @code
    SpectrumAnalyzerState analyzerState;
    SpectrumAnalyzerComponent analyzerComponent(analyzerState);

    // Configure the display
    analyzerComponent.setWindowType(WindowType::hann);
    analyzerComponent.setFrequencyRange(20.0f, 20000.0f);
    analyzerComponent.setDecibelRange(-100.0f, 0.0f);
    analyzerComponent.setUpdateRate(30);

    // In audio callback:
    analyzerState.pushSamples(audioData, numSamples);
    @endcode

    @see SpectrumAnalyzerState, FFTProcessor, WindowFunctions

    @tags{AudioGUI}
*/
class YUP_API SpectrumAnalyzerComponent
    : public Component
    , public Timer
{
public:
    //==============================================================================
    /** Display type for the spectrum visualization. */
    enum class DisplayType
    {
        lines,       ///< Draw spectrum as smooth connected lines
        filled       ///< Draw spectrum as smooth filled area
    };

    //==============================================================================
    /** FFT constants (must match SpectrumAnalyzerState) */
    enum
    {
        fftOrder = 11,                    ///< 2^11 = 2048 samples
        fftSize = 1 << fftOrder,          ///< 2048
        scopeSize = 512                   ///< Number of display points
    };

    //==============================================================================
    /** Creates a SpectrumAnalyzerComponent.

        @param state    the SpectrumAnalyzerState that provides audio data
    */
    explicit SpectrumAnalyzerComponent (SpectrumAnalyzerState& state);

    /** Destructor. */
    ~SpectrumAnalyzerComponent() override;

    //==============================================================================
    /** Sets the window function used for FFT processing.

        @param type    the window function type to use
    */
    void setWindowType (WindowType type);

    /** Returns the current window function type. */
    WindowType getWindowType() const noexcept { return currentWindowType; }

    //==============================================================================
    /** Sets the display update rate in Hz.

        @param hz    update rate (typical values: 15-60 Hz)
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
    /** Sets the decibel range for the display.

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
    /** Sets the display type.

        @param type    the display type to use
    */
    void setDisplayType (DisplayType type);

    /** Returns the current display type. */
    DisplayType getDisplayType() const noexcept { return displayType; }

    //==============================================================================
    /** Sets the smoothing factor for spectrum falloff.

        @param factor    smoothing factor between 0.0 (no smoothing) and 1.0 (maximum smoothing)
    */
    void setSmoothingFactor (float factor);

    /** Returns the current smoothing factor. */
    float getSmoothingFactor() const noexcept { return smoothingFactor; }

    //==============================================================================
    /** Returns the frequency for a given bin index.

        @param binIndex    the FFT bin index
        @returns          the frequency in Hz
    */
    float getFrequencyForBin (int binIndex) const noexcept;

    /** Returns the bin index for a given frequency.

        @param frequency    the frequency in Hz
        @returns           the FFT bin index
    */
    int getBinForFrequency (float frequency) const noexcept;

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
    void updateDisplay();
    void generateWindow();
    void computeSpectrumPath (Path spectrumPath, const Rectangle<float>& bounds, bool closePath);
    void drawLinesSpectrum (Graphics& g, const Rectangle<float>& bounds);
    void drawFilledSpectrum (Graphics& g, const Rectangle<float>& bounds);
    void drawFrequencyGrid (Graphics& g, const Rectangle<float>& bounds);
    void drawDecibelGrid (Graphics& g, const Rectangle<float>& bounds);

    float frequencyToX (float frequency, const Rectangle<float>& bounds) const noexcept;
    float decibelToY (float decibel, const Rectangle<float>& bounds) const noexcept;
    float binToY (int binIndex, float height) const noexcept;

    //==============================================================================
    SpectrumAnalyzerState& analyzerState;

    // FFT processing (performed on UI thread)
    FFTProcessor fftProcessor;
    std::vector<float> fftInputBuffer;      // Real input samples
    std::vector<float> fftOutputBuffer;     // Complex FFT output
    std::vector<float> windowBuffer;        // Window function

    // Display data
    std::vector<float> scopeData;
    Path spectrumPath;

    // Configuration
    WindowType currentWindowType = WindowType::hann;
    DisplayType displayType = DisplayType::filled;
    float minFrequency = 20.0f;
    float maxFrequency = 20000.0f;
    float logMinFrequency = std::log10 (minFrequency);
    float logMaxFrequency = std::log10 (maxFrequency);
    float minDecibels = -100.0f;
    float maxDecibels = 0.0f;
    double sampleRate = 44100.0;
    float smoothingFactor = 0.8f;

    // State
    bool needsWindowUpdate = true;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalyzerComponent)
};

} // namespace yup
