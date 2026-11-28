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

    This component performs FFT processing on audio data collected by a SpectrumAnalyzerState and renders
    the frequency spectrum as a visual display. The FFT processing is performed on the UI thread, once per displayed
    frame by default or at a fixed rate set with setUpdateRate(). The release is timed from the elapsed time between
    updates, so the spectrum falls off at the same speed at any frame rate.

    The component can be configured with different window functions, display types, frequency ranges, and update
    rates. It automatically handles logarithmic frequency scaling for natural spectrum visualization. Levels are
    interpolated across the FFT bin domain and the outline is sampled once per pixel column, so the rendered curve
    stays continuous at any component width.

    The analyzer works only while it has something to show. It takes the frames of its window while samples come
    and while its curve is falling, and asks to be painted when a level moved. Once the audio has stopped and the
    curve is at rest it leaves the frames alone, so that a window with nothing else to do stops running them, and
    looks at its state ten times a second instead: it is back within a tenth of a second of new samples. A hidden
    analyzer, or one without a state, does nothing until it shows or is given one.

    Example usage:

    @code
        SpectrumAnalyzerState analyzerState;
        SpectrumAnalyzerComponent analyzerComponent(analyzerState);

        // Configure the display
        analyzerComponent.setWindowType(WindowType::hann);
        analyzerComponent.setFrequencyRange(20.0f, 20000.0f);
        analyzerComponent.setDecibelRange(-100.0f, 0.0f);

        // In audio callback:
        analyzerState.pushSamples(audioData, numSamples);
    @endcode

    The state can be given after construction with setState(), which is how an analyzer created by a `.yui` document
    as `SpectrumAnalyzer` gets it (see YuiDocument::setInitializer). Without a state the analyzer draws its grid alone.

    The FFT size and the overlap are the ones of the state and are set there, see SpectrumAnalyzerState::setFftSize
    and SpectrumAnalyzerState::setOverlapFactor. This can be done at any time, also while the audio is running and
    the analyzer is showing: the analyzer follows the state at its next update. It never sets the FFT size itself,
    so it never goes beyond the largest one its state was made for, see SpectrumAnalyzerState::getMaxFftSize.

    @see SpectrumAnalyzerState, SpectrumAnalysis, FFTProcessor, WindowFunctions
*/
class YUP_API SpectrumAnalyzerComponent : public Component
{
public:
    //==============================================================================
    /** Style identifiers for theme customization. */
    struct Style
    {
        /** Background color at the top of the vertical gradient. */
        static const Identifier backgroundTopColorId;

        /** Background color at the bottom of the vertical gradient. */
        static const Identifier backgroundBottomColorId;

        /** Spectrum outline color, also used for the line display type. */
        static const Identifier outlineColorId;

        /** Spectrum fill color at the top, fading out towards the bottom (filled display type). */
        static const Identifier fillColorId;

        /** Color of the major grid lines, the minor lines are drawn with a reduced alpha. */
        static const Identifier gridColorId;

        /** Color of the frequency and decibel labels. */
        static const Identifier textColorId;
    };

    //==============================================================================
    /** Display type for the spectrum visualization. */
    enum class DisplayType
    {
        lines, ///< Draw spectrum as smooth connected lines
        filled ///< Draw spectrum as smooth filled area
    };

    /** Level calculation mode for spectrum magnitudes. */
    enum class LevelMode
    {
        peakDecibels,        ///< Peak amplitude in dBFS, where a full-scale sine peak is 0 dBFS
        rmsDecibels,         ///< RMS amplitude in dBFS, where a full-scale sine is approximately -3.01 dBFS
        powerDecibels,       ///< RMS power in dBFS, summed across each displayed frequency band
        powerSpectralDensity ///< RMS power spectral density in dBFS/Hz, corrected for window ENBW
    };

    //==============================================================================
    /** Display constants */
    enum
    {
        scopeSize = 512 ///< Number of display points
    };

    //==============================================================================
    /** Creates a SpectrumAnalyzerComponent with no state, to be given one with setState(). */
    SpectrumAnalyzerComponent();

    /** Creates a SpectrumAnalyzerComponent.

        @param state    the SpectrumAnalyzerState that provides audio data
    */
    explicit SpectrumAnalyzerComponent (SpectrumAnalyzerState& state);

    /** Destructor. */
    ~SpectrumAnalyzerComponent() override;

    //==============================================================================
    /** Changes the state providing the audio data.

        The analyzer takes the FFT size of the state, follows it when it changes, and starts from an empty spectrum.
        Call it on the message thread. The state must outlive the analyzer, or be removed first by setting a null one.

        @param newState    the state to read, or nullptr for none
    */
    void setState (SpectrumAnalyzerState* newState);

    /** Returns the state providing the audio data, or nullptr if the analyzer has none. */
    SpectrumAnalyzerState* getState() const noexcept { return analysis.getState(); }

    //==============================================================================
    /** Returns the static type info describing the SpectrumAnalyzerComponent type, `SpectrumAnalyzer` in `.yui`. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

    //==============================================================================
    /** Returns the FFT size: the one of the analyzer state when there is one, which is where it is set. */
    int getFFTSize() const noexcept { return analysis.getFftSize(); }

    //==============================================================================
    /** Sets the window function used for FFT processing.

        @param type    the window function type to use
    */
    void setWindowType (WindowType type);

    /** Returns the current window function type. */
    WindowType getWindowType() const noexcept { return analysis.getWindowType(); }

    //==============================================================================
    /** Sets the display update rate in Hz.

        By default the spectrum updates once per displayed frame, following the display refresh rate. A fixed
        rate takes the frames that come when an update is due and leaves the others out, so it goes no faster
        than the display.

        @param hz    update rate (typical values: 15-60 Hz), or 0 to follow the display refresh rate
    */
    void setUpdateRate (int hz);

    /** Returns the current update rate in Hz, or 0 when following the display refresh rate. */
    int getUpdateRate() const noexcept { return updateRate; }

    //==============================================================================
    /** Sets the frequency range for the display.

        The range starts at 1 Hz or above and spans at least 1 Hz.

        @param minFreq    minimum frequency in Hz
        @param maxFreq    maximum frequency in Hz
    */
    void setFrequencyRange (float minFreq, float maxFreq);

    /** Returns the current minimum frequency. */
    float getMinFrequency() const noexcept { return analysis.getMinFrequency(); }

    /** Returns the current maximum frequency. */
    float getMaxFrequency() const noexcept { return analysis.getMaxFrequency(); }

    //==============================================================================
    /** Sets the decibel range for the display.

        The range is kept between SpectrumAnalysis::lowestDecibels and SpectrumAnalysis::highestDecibels and
        spans at least 1 dB.

        @param minDb    minimum decibel level
        @param maxDb    maximum decibel level
    */
    void setDecibelRange (float minDb, float maxDb);

    /** Returns the current minimum decibel level. */
    float getMinDecibels() const noexcept { return analysis.getMinDecibels(); }

    /** Returns the current maximum decibel level. */
    float getMaxDecibels() const noexcept { return analysis.getMaxDecibels(); }

    //==============================================================================
    /** Sets the sample rate for frequency calculations.

        @param sampleRate    the sample rate in Hz
    */
    void setSampleRate (double sampleRate);

    /** Returns the current sample rate. */
    double getSampleRate() const noexcept { return analysis.getSampleRate(); }

    //==============================================================================
    /** Sets the display type.

        @param type    the display type to use
    */
    void setDisplayType (DisplayType type);

    /** Returns the current display type. */
    DisplayType getDisplayType() const noexcept { return displayType; }

    //==============================================================================
    /** Sets the level calculation mode.

        @param mode    the level mode to use when converting FFT bins to decibels
    */
    void setLevelMode (LevelMode mode);

    /** Returns the current level calculation mode. */
    LevelMode getLevelMode() const noexcept { return levelMode; }

    /** Returns the coherent gain of the current window. */
    float getWindowCoherentGain() const noexcept { return analysis.getWindowCoherentGain(); }

    /** Returns the equivalent noise bandwidth of the current window in FFT bins. */
    float getEquivalentNoiseBandwidthBins() const noexcept { return analysis.getEquivalentNoiseBandwidthBins(); }

    /** Returns the equivalent noise bandwidth of the current window in Hz. */
    float getEquivalentNoiseBandwidthHz() const noexcept;

    //==============================================================================
    /** Sets the release time for spectrum falloff.

        @param timeSeconds    release time in seconds, at least 0.1 (5.0 = 5 second falloff)
    */
    void setReleaseTimeSeconds (float timeSeconds);

    /** Returns the current release time in seconds. */
    float getReleaseTimeSeconds() const noexcept { return releaseTimeSeconds; }

    //==============================================================================
    /** Returns the levels the analyzer shows: one for each of its scopeSize display points.

        The points are evenly spaced along the frequency axis as it is displayed, from the lowest
        frequency to the highest on a logarithmic scale, and each level goes from 0 at the lowest
        decibel of the display range to 1 at the highest. They are the levels the analyzer draws,
        falling as its release time tells.

        This is what the `levels` property gives a `.yui` drawing or a shader painting the
        analyzer in place of its own paint (see Component::setStyle): they read the levels where
        they are, with no copy. The values change in place at every update, on the message
        thread.

        @see getLevelsRevision
    */
    Span<const float> getLevels() const noexcept { return scopeData; }

    /** Returns a number that changes whenever the levels do, and is never 0. */
    uint32 getLevelsRevision() const noexcept { return levelsRevision; }

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
    void visibilityChanged() override;
    /** @internal */
    void parentHierarchyChanged() override;
    /** @internal */
    void refreshDisplay (double lastFrameTimeSeconds);

private:
    //==============================================================================
    struct GridLabel
    {
        StyledText text;
        Rectangle<float> bounds;
    };

    struct GridLine
    {
        float position = 0.0f; // The x of a frequency line, the y of a decibel line
        float alpha = 1.0f;    // Multiplies the alpha of the grid color
        float width = 1.0f;
        GridLabel label;
    };

    //==============================================================================
    void updateSpectrum (double elapsedSeconds);
    void updateBinLevels();
    bool updateTargets();
    void levelsChanged() noexcept;
    void updateRefreshing (bool hasWorkToDo);
    void updateSpectrumPaths (const Rectangle<float>& bounds);
    void updateGrid (const Rectangle<float>& bounds, const Font& defaultFont);
    void drawLinesSpectrum (Graphics& g);
    void drawFilledSpectrum (Graphics& g, const Rectangle<float>& bounds);
    void drawGrid (Graphics& g, const Rectangle<float>& bounds);

    float getBinLinearLevel (float peakAmplitude, bool isRealBin) const noexcept;
    float linearLevelToDecibels (float level) const noexcept;
    SpectrumBinMapping::BandAggregation getBandAggregation() const noexcept;
    bool isPowerMode() const noexcept;

    float frequencyToX (float frequency, const Rectangle<float>& bounds) const noexcept;
    float decibelToY (float decibel, const Rectangle<float>& bounds) const noexcept;
    float levelToY (float level, const Rectangle<float>& bounds) const noexcept;
    float getDisplayLevelForPosition (float displayPoint) const noexcept;

    //==============================================================================
    // The state, its FFT (performed on the UI thread), the window and the ranges
    SpectrumAnalysis analysis { scopeSize };

    // Display data
    std::vector<float> scopeData;
    uint32 levelsRevision = 1;
    std::vector<float> targetData;     // Latest FFT level of every display point, the release target
    std::vector<float> binLevelBuffer; // Linear level of every FFT bin, in the level mode

    // What is painted, kept from one paint to the next: the curve until the levels or the size change, the grid
    // until the size, a range or the font do
    Path spectrumPath;
    Path spectrumFillPath;
    Rectangle<float> pathBounds;
    uint32 pathRevision = 0;
    bool pathIsFilled = false;

    std::vector<GridLine> frequencyLines;
    std::vector<GridLine> decibelLines;
    GridLabel hertzLabel;
    GridLabel decibelLabel;
    Rectangle<float> gridBounds;
    Font gridFont;
    bool gridNeedsUpdate = true;

    // Configuration
    DisplayType displayType = DisplayType::filled;
    LevelMode levelMode = LevelMode::peakDecibels;
    float releaseTimeSeconds = 1.0f;
    int updateRate = 0;

    // Timing, in the seconds its window tells between frames
    double secondsSinceUpdate = 0.0;
    double secondsUntilUpdate = 0.0;
    double secondsSinceSpectrum = 0.0;
    bool hasRecentSpectrum = false; // A spectrum was analyzed a moment ago: the audio is running

    // The window calls refreshDisplay() at every frame while there is something to do, the timer at a low rate
    // while there is not: the audio thread can't tell that it pushed samples
    static constexpr int restingRefreshRate = 10;

    FrameAttachment frameAttachment { *this, bindFront (&SpectrumAnalyzerComponent::refreshDisplay, this), false };
    TimedCallback restingTimer { [this] { refreshDisplay (1.0 / restingRefreshRate); } };

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalyzerComponent)
};

} // namespace yup
