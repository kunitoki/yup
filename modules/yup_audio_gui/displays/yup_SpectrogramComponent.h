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
    black > dark blue > blue > cyan > green > yellow > red > white.

    @tags{Audio}
*/
class YUP_API SpectrogramColorMap
{
public:
    //==============================================================================
    /** Predefined color map types. */
    enum class Type
    {
        heatmap,   ///< Black > blue > cyan > green > yellow > red > white
        grayscale, ///< Black > gray > white
        cool,      ///< Black > blue > cyan > white
        warm,      ///< Black > red > orange > yellow > white
        viridis    ///< Perceptually uniform blue > green > yellow
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

    /** Returns the raw ARGB color lookup table (0xAARRGGBB per entry).

        The table holds exactly getNumColorStops() entries sampled from the
        gradient, which is what map() interpolates between. Uploading this table
        to the GPU lets a shader perform the same color mapping.

        @return a span over the color table.
    */
    Span<const uint32> getColorTable() const noexcept { return colorTable; }

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

    Example usage:

    @code
        SpectrumAnalyzerState analyzerState;
        SpectrogramComponent spectrogram (analyzerState);

        // Configure the display
        spectrogram.setFrequencyRange (20.0f, 20000.0f);
        spectrogram.setDecibelRange (-100.0f, 0.0f);
        spectrogram.setColorMap (SpectrogramColorMap::Type::heatmap);

        // In audio callback:
        analyzerState.pushSamples (audioData, numSamples);
    @endcode

    @see SpectrumAnalyzerState, SpectrumAnalyzerComponent, SpectrogramColorMap
*/
class YUP_API SpectrogramComponent : public Component
{
public:
    //==============================================================================
    /** Display constants. */
    enum
    {
        defaultSpectrogramWidth = 1024,      ///< Default number of horizontal frequency bins.
        defaultSpectrogramMagnitudes = 2048, ///< Default number of magnitudes per FFT (must be >= defaultSpectrogramWidth).
        defaultColorLutSize = 256            ///< Default number of color stops in the color lookup table.
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
    /** Sets the waterfall scroll speed, as a multiplier of the realtime FFT
        row rate (sampleRate / hopSize).

        A multiplier of 1.0 (the default) keeps the waterfall locked to the
        audio: new rows slide in exactly as fast as the FFT produces them.
        Values above 1.0 scroll faster than realtime (rows are dropped at the
        bottom sooner), values below 1.0 scroll slower. 0.0 pauses the scroll.

        @param scrollSpeedMultiplier   the scroll speed multiplier (clamped to >= 0)
    */
    void setScrollSpeed (float scrollSpeedMultiplier);

    /** Returns the current scroll speed multiplier (1.0 = realtime). */
    float getScrollSpeed() const noexcept { return scrollSpeedMultiplier; }

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
        The history canvases are resized to match this value.

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
    /** Returns the current spectrogram image.

        The returned image contains the waterfall history held in the current
        history canvas. This performs a GPU readback, which is slower than
        rendering from the component's cached GPU texture (via paint()); prefer
        painting the component directly when possible. The image is invalid
        until the component has a live GPU render context and its history
        canvases have been created (e.g. after the first paint).

        @returns the current spectrogram image.
    */
    Image getSpectrogramImage();

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void refreshDisplay (double lastFrameTimeSeconds) override;

private:
    //==============================================================================
    void processFFT();
    void updateFrequencyMapping();
    bool ensureGpuTargets (GraphicsContext& context);
    void ensureWaterfallPipeline();
    void applyPendingRows();
    void advanceScroll();
    float getRowRate() const noexcept;
    void initializeFFTBuffers();
    void generateWindow();

    float frequencyToX (float frequency, float width) const noexcept;
    void drawFrequencyGrid (Graphics& g, const Rectangle<float>& bounds);
    void drawFrequencyGridCached (Graphics& g, const Rectangle<float>& bounds);

    //==============================================================================
    SpectrumAnalyzerState& analyzerState;

    std::unique_ptr<FFTProcessor> fftProcessor;
    std::vector<float> fftInputBuffer;
    std::vector<float> fftOutputBuffer;
    std::vector<float> windowBuffer;
    std::vector<float> magnitudeBuffer;
    std::vector<float> displayMagnitudes;

    // Spectrogram color map
    SpectrogramColorMap colorMap;

    // Precomputed log-frequency > FFT-bin mapping
    struct DisplayBinMapping
    {
        float startBin = 0.0f;
        float endBin = 0.0f;
        float exactBin = 0.0f;
    };

    std::vector<DisplayBinMapping> displayBinMapping;

    // GPU waterfall state
    GpuDevice::Ptr gpuDevice;
    GpuPipeline::Ptr waterfallPipeline;
    GpuTarget::Ptr gpuTargets[2];
    GpuTexture::Ptr displayTexture;
    std::vector<std::vector<float>> pendingRows;
    std::vector<float> rowDataCache;  // Reused uniform buffer (shader mags[defaultSpectrogramMagnitudes])
    std::vector<uint32> lutDataCache; // Reused color LUT (shader lut[defaultColorLutSize])
    bool lutNeedsRefresh = true;      // Re-fill lutDataCache on the next apply
    int pingPongIndex = 0;
    float scrollOffset = 0.0f;
    uint32 lastPaintTimeMs = 0;

    // Cached frequency grid (frequency lines + labels).
    GpuCanvas::Ptr gridCanvas;
    Rectangle<float> gridCacheBounds;
    bool gridNeedsRedraw = true;

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
    float scrollSpeedMultiplier = 1.0f;

    // Window compensation
    float windowGain = 1.0f;
    bool needsWindowUpdate = true;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrogramComponent)
};

} // namespace yup
