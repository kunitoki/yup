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

#pragma once

namespace yup
{

//==============================================================================
/**
    The analysis the spectrum displays share: it reads the frames of a SpectrumAnalyzerState, runs their FFT and
    tells the calibrated amplitude of every bin, along with the ranges the levels are displayed in.

    A display owns one and gives it its window, its ranges and its sample rate. At every update it calls
    syncWithState(), skipStaleFrames() and then processNextFrame() for as long as it has time, reading
    getBinAmplitudes() after each frame:

    @code
        SpectrumAnalyzerState state;

        SpectrumAnalysis analysis (512);
        analysis.setState (&state);

        // At every update, on the message thread
        analysis.syncWithState();
        analysis.skipStaleFrames (4);

        for (int frame = 0; frame < 4 && analysis.processNextFrame(); ++frame)
        {
            const auto& mapping = analysis.getBinMapping();

            for (int point = 0; point < mapping.getNumDisplayPoints(); ++point)
                levels[point] = mapping.getBandLevel (analysis.getBinAmplitudes(), point, SpectrumBinMapping::BandAggregation::peak);
        }
    @endcode

    The amplitudes are one-sided and compensated for the window, so a full-scale sine reads 1.0, which is 0 dBFS,
    with any window and at any FFT size.

    The FFT size is the one of the state, which is where it is set: the analysis takes it in setState() and again
    in syncWithState(), so a state resized while it is displayed is followed at the next update. The analysis never
    sets the size of its state, and its FFT, its window and its buffers are the size of the state: they never go
    beyond SpectrumAnalyzerState::getMaxFftSize().

    Threading: an analysis is the reading side of its state. Use it from the message thread, or from a thread
    holding the message manager lock as the function of a FrameAttachment does.

    @see SpectrumAnalyzerState, SpectrumBinMapping, SpectrumAnalyzerComponent, SpectrogramComponent

    @tags{Audio}
*/
class YUP_API SpectrumAnalysis
{
public:
    //==============================================================================
    /** The lowest level a decibel range can start from, in dB. */
    static constexpr float lowestDecibels = -200.0f;

    /** The highest level a decibel range can end at, in dB. */
    static constexpr float highestDecibels = 60.0f;

    //==============================================================================
    /** Creates an analysis with no state, to be given one with setState().

        @param numDisplayPoints   the number of points of the logarithmic frequency axis resolved by getBinMapping()
    */
    explicit SpectrumAnalysis (int numDisplayPoints);

    /** Destructor. */
    ~SpectrumAnalysis() = default;

    //==============================================================================
    /** Changes the state providing the audio data, and takes its FFT size.

        The state must outlive the analysis, or be removed first by setting a null one.

        @param newState    the state to read, or nullptr for none

        @returns true if the state changed
    */
    bool setState (SpectrumAnalyzerState* newState);

    /** Returns the state providing the audio data, or nullptr if the analysis has none. */
    SpectrumAnalyzerState* getState() const noexcept { return state; }

    //==============================================================================
    /** Returns the FFT size: the one of the state when there is one, otherwise the last one taken from a state
        in setState() or syncWithState(), 4096 before any.
    */
    int getFftSize() const noexcept;

    /** Takes the FFT size of the state when it changed, resizing the FFT, the window and the bin amplitudes.

        Call it before reading the frames of an update: processNextFrame() gives no frame while the sizes differ.

        @returns true if the FFT size changed, in which case the bin amplitudes are back to zero
    */
    bool syncWithState();

    /** Drops the oldest frames of the state, so that the display keeps up with the audio instead of falling
        behind it when frames come faster than they are shown.

        @param numFramesToKeep    the number of the newest frames left to process
    */
    void skipStaleFrames (int numFramesToKeep) noexcept;

    /** Reads the next frame of the state, when one is ready, and updates the bin amplitudes from its FFT.

        @returns true if a frame was processed
    */
    bool processNextFrame();

    /** Returns the amplitude of every bin of the last frame processed, from bin 0 to Nyquist.

        The amplitudes are linear, one-sided and compensated for the coherent gain of the window: a sine of
        amplitude 1.0 centered on a bin reads 1.0. A bin that is not finite, as after samples that are not, reads 0.
    */
    Span<const float> getBinAmplitudes() const noexcept { return binAmplitudes; }

    /** Returns the mapping of the display points onto the bins, kept up to date with the FFT size, the sample
        rate and the frequency range.
    */
    const SpectrumBinMapping& getBinMapping() const noexcept { return binMapping; }

    //==============================================================================
    /** Sets the window function applied to the frames before their FFT.

        @param type    the window function type to use

        @returns true if the window changed
    */
    bool setWindowType (WindowType type);

    /** Returns the current window function type. */
    WindowType getWindowType() const noexcept { return windowType; }

    /** Returns the coherent gain of the current window. */
    float getWindowCoherentGain() const noexcept { return windowCoherentGain; }

    /** Returns the equivalent noise bandwidth of the current window in FFT bins. */
    float getEquivalentNoiseBandwidthBins() const noexcept { return equivalentNoiseBandwidthBins; }

    //==============================================================================
    /** Sets the frequency range displayed.

        The range starts at 1 Hz or above and spans at least 1 Hz: values outside of that are moved.

        @param minFreq    minimum frequency in Hz
        @param maxFreq    maximum frequency in Hz

        @returns true if the range changed
    */
    bool setFrequencyRange (float minFreq, float maxFreq);

    /** Returns the current minimum frequency. */
    float getMinFrequency() const noexcept { return minFrequency; }

    /** Returns the current maximum frequency. */
    float getMaxFrequency() const noexcept { return maxFrequency; }

    /** Returns where a frequency sits on the logarithmic axis: 0 at the minimum frequency, 1 at the maximum.

        @param frequency    the frequency in Hz, above 0
    */
    float frequencyToProportion (float frequency) const noexcept;

    //==============================================================================
    /** Sets the decibel range displayed.

        The range is kept between lowestDecibels and highestDecibels and spans at least 1 dB: values outside of
        that are moved. Values that are not numbers are ignored.

        @param minDb    minimum decibel level
        @param maxDb    maximum decibel level, above the minimum

        @returns true if the range changed
    */
    bool setDecibelRange (float minDb, float maxDb);

    /** Returns the current minimum decibel level. */
    float getMinDecibels() const noexcept { return minDecibels; }

    /** Returns the current maximum decibel level. */
    float getMaxDecibels() const noexcept { return maxDecibels; }

    /** Returns where a level sits in the decibel range: 0 at the minimum level or below, 1 at the maximum or above.

        @param decibels    the level in dB
    */
    float decibelsToProportion (float decibels) const noexcept;

    //==============================================================================
    /** Sets the sample rate of the audio the state is given, which places the frequencies.

        @param newSampleRate    the sample rate in Hz, at least 1

        @returns true if the sample rate changed
    */
    bool setSampleRate (double newSampleRate);

    /** Returns the current sample rate. */
    double getSampleRate() const noexcept { return sampleRate; }

private:
    //==============================================================================
    void allocate();
    void generateWindow();

    //==============================================================================
    static constexpr int defaultFftSize = 4096;

    SpectrumAnalyzerState* state = nullptr;

    FFTProcessor<float> fftProcessor { defaultFftSize };
    std::vector<float> fftInputBuffer;
    std::vector<float> fftOutputBuffer;
    std::vector<float> windowBuffer;
    std::vector<float> binAmplitudes;
    SpectrumBinMapping binMapping;

    WindowType windowType = WindowType::hann;
    int fftSize = defaultFftSize;
    float minFrequency = 20.0f;
    float maxFrequency = 20000.0f;
    float minDecibels = -100.0f;
    float maxDecibels = 0.0f;
    double sampleRate = 44100.0;
    float windowCoherentGain = 1.0f;
    float equivalentNoiseBandwidthBins = 1.0f;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalysis)
};

} // namespace yup
