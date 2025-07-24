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
    Real-time safe spectrum analyzer data collection class.

    This class handles the collection of audio samples from the audio thread and provides a lock-free interface
    for UI components to retrieve FFT-ready data. It uses AbstractFifo for thread-safe communication between
    the audio and UI threads.

    The audio thread should call pushSample() or pushSamples() to feed audio data. The UI thread should check
    isFFTDataReady() and call getFFTData() to retrieve samples for FFT processing.

    This class follows the same pattern as MidiKeyboardState - it handles the real-time safe data collection while
    leaving the processing and visualization to companion classes.

    @see SpectrumAnalyzerComponent, FFTProcessor
*/
class YUP_API SpectrumAnalyzerState
{
public:
    //==============================================================================
    /** Creates a SpectrumAnalyzerState with default settings (2048 FFT size). */
    SpectrumAnalyzerState();

    /** Creates a SpectrumAnalyzerState with specified FFT size.

        @param fftSize    FFT size (must be a power of 2, between 64 and 16384)
    */
    explicit SpectrumAnalyzerState (int fftSize);

    /** Destructor. */
    ~SpectrumAnalyzerState();

    //==============================================================================
    /** Pushes a single sample into the analyzer (real-time safe).

        This method is designed to be called from the audio thread and is lock-free.

        @param sample   the audio sample to add
    */
    void pushSample (float sample) noexcept;

    /** Pushes multiple samples into the analyzer (real-time safe).

        This method is designed to be called from the audio thread and is lock-free.

        @param samples      pointer to the audio samples
        @param numSamples   number of samples to add
    */
    void pushSamples (const float* samples, int numSamples) noexcept;

    //==============================================================================
    /** Checks if enough samples are available for FFT processing.

        This should be called from the UI thread.

        @returns true if fftSize samples are ready for processing
    */
    bool isFFTDataReady() const noexcept;

    /** Retrieves samples for FFT processing.

        This should be called from the UI thread when isFFTDataReady() returns true.
        The method will copy fftSize samples into the provided buffer and advance
        the read position.

        @param destBuffer   buffer to copy samples into (must be at least fftSize elements)

        @returns           true if data was successfully retrieved
    */
    bool getFFTData (float* destBuffer) noexcept;

    //==============================================================================
    /** Resets the internal FIFO state.

        This clears all buffered samples and resets the read/write positions.
    */
    void reset() noexcept;

    //==============================================================================
    /** Returns the FFT size used by this analyzer. */
    int getFftSize() const noexcept { return fftSize; }

    /** Sets a new FFT size for the analyzer.

        @param newSize    FFT size (must be a power of 2, between 64 and 16384)
    */
    void setFftSize (int newSize);

    /** Returns the number of samples currently available in the FIFO. */
    int getNumAvailableSamples() const noexcept;

    /** Returns the amount of free space in the FIFO. */
    int getFreeSpace() const noexcept;

private:
    //==============================================================================
    void initializeFifo();
    
    int fftSize = 2048;
    int fifoSize = 8192;  // Will be updated in initializeFifo()
    
    std::unique_ptr<AbstractFifo> audioFifo;
    std::vector<float> sampleBuffer;
    std::atomic<bool> fftDataReady { false };

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalyzerState)
};

} // namespace yup