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

    The FIFO is allocated once, when the state is created, for the largest FFT size the state is to take: the
    one given to its constructor, see getMaxFftSize(). Nothing is allocated, freed or locked after that: changing
    the FFT size or the overlap only changes how the samples already going through the FIFO are cut into frames,
    so both can be changed while the audio is running.

    The FIFO holds four frames of the maximum, 16 bytes for each sample of it. A state made without a maximum
    takes every size up to maxFftSize, which costs about 1 MB: give the maximum the state needs to hold less, for
    example 32 KB for 2048.

    @code
        // A state starting at 1024 that goes up to 4096, as the largest size its user can choose
        SpectrumAnalyzerState state (1024, 4096);
    @endcode

    Threading:

    - pushSample() and pushSamples() belong to one writing thread, the audio thread. They never allocate, lock
      or wait: the samples that do not fit are dropped.
    - isFFTDataReady(), getFFTData(), skipSamples() and reset() belong to the reading side, one thread at a time:
      the message thread, or a thread holding the message manager lock as the function of a FrameAttachment
      does. They can run while the audio thread writes.
    - setFftSize(), setOverlapFactor() and the getters can be called from any thread at any time, also while
      the audio thread writes and the reading side reads. A reader that does not set the FFT size itself reads
      with getFFTData (Span<float>), which checks the size of its buffer against the FFT size of the frame.

    This class follows the same pattern as MidiKeyboardState - it handles the real-time safe data collection while
    leaving the processing and visualization to companion classes.

    @see SpectrumAnalyzerComponent, FFTProcessor
*/
class YUP_API SpectrumAnalyzerState
{
public:
    //==============================================================================
    /** The smallest FFT size a state takes, and the smallest maximum a state can be made for. */
    static constexpr int minFftSize = 64;

    /** The largest FFT size a state takes, which is the maximum of a state made without one. */
    static constexpr int maxFftSize = 65536;

    //==============================================================================
    /** Creates a SpectrumAnalyzerState with default settings (2048 FFT size), taking every size up to maxFftSize. */
    SpectrumAnalyzerState();

    /** Creates a SpectrumAnalyzerState with specified FFT size, taking every size up to maxFftSize.

        @param fftSize    FFT size, a power of 2 between minFftSize and maxFftSize. Any other size is ignored
                          and leaves the default one of 2048.
    */
    explicit SpectrumAnalyzerState (int fftSize);

    /** Creates a SpectrumAnalyzerState with specified FFT size, taking no size above a maximum.

        The FIFO is allocated for the maximum, which does not change afterwards: give the largest size the
        state will be set to, so that it holds no more than it needs.

        @param fftSize              FFT size, a power of 2 between minFftSize and the maximum. Any other size is
                                    ignored and leaves the default one: 2048, or the maximum when that is smaller.
        @param maxFftSizeToAllow    the largest FFT size the state takes, a power of 2 between minFftSize and
                                    maxFftSize. Any other value is a mistake of the caller, and falls back to
                                    maxFftSize so that no size is refused because of it.
    */
    SpectrumAnalyzerState (int fftSize, int maxFftSizeToAllow);

    /** Destructor. */
    ~SpectrumAnalyzerState();

    //==============================================================================
    /** Pushes a single sample into the analyzer (real-time safe).

        This method is designed to be called from the audio thread: it never allocates, locks or waits.

        @param sample   the audio sample to add
    */
    void pushSample (float sample) noexcept;

    /** Pushes multiple samples into the analyzer (real-time safe).

        This method is designed to be called from the audio thread: it never allocates, locks or waits. The
        FIFO keeps four frames of the current FFT size, and the samples that do not fit are dropped.

        @param samples      pointer to the audio samples
        @param numSamples   number of samples to add
    */
    void pushSamples (const float* samples, int numSamples) noexcept;

    //==============================================================================
    /** Checks if enough samples are available for FFT processing.

        This should be called from the reading side, the UI thread.

        @returns true if fftSize samples are ready for processing
    */
    bool isFFTDataReady() const noexcept;

    /** Retrieves samples for FFT processing.

        This should be called from the reading side, the UI thread, when isFFTDataReady() returns true.
        The method will copy fftSize samples into the provided buffer and advance
        the read position by the hop size.

        The buffer is trusted to hold getFftSize() samples: only use this when the FFT size is set by the
        thread that reads, otherwise use getFFTData (Span<float>).

        @param destBuffer   buffer to copy samples into (must be at least fftSize elements)

        @returns           true if data was successfully retrieved
    */
    bool getFFTData (float* destBuffer) noexcept;

    /** Retrieves samples for FFT processing into a buffer of a known size.

        This should be called from the reading side, the UI thread. It does what getFFTData (float*) does, but
        only when the buffer holds exactly the FFT size, so that it stays safe when another thread changes the
        FFT size: the reader then gets no frame, and resizes its buffer to getFftSize() before trying again.

        @param destBuffer   buffer to copy samples into, of exactly fftSize elements

        @returns           true if data was successfully retrieved
    */
    bool getFFTData (Span<float> destBuffer) noexcept;

    /** Drops the oldest buffered samples, without copying them.

        This should be called from the reading side, the UI thread, which skips this way what it has no time
        to show.

        @param numSamples   number of samples to drop, limited to the ones available
    */
    void skipSamples (int numSamples) noexcept;

    //==============================================================================
    /** Drops all the buffered samples.

        This should be called from the reading side, the UI thread, or while nothing reads: it only moves the
        read position, so it can run while the audio thread writes.
    */
    void reset() noexcept;

    //==============================================================================
    /** Returns the FFT size used by this analyzer. */
    int getFftSize() const noexcept { return fftSize.load(); }

    /** Returns the largest FFT size this state takes: the one it was created for, which never changes. */
    int getMaxFftSize() const noexcept { return largestFftSize; }

    /** Sets a new FFT size for the analyzer.

        This can be called from any thread, also while the audio thread writes: nothing is allocated, and the
        samples already buffered are kept and read in frames of the new size.

        @param newSize    FFT size, a power of 2 between minFftSize and getMaxFftSize(). Any other size is ignored.
    */
    void setFftSize (int newSize) noexcept;

    /** Returns the number of samples currently available in the FIFO. */
    int getNumAvailableSamples() const noexcept;

    /** Returns the amount of free space in the FIFO, which keeps four frames of the current FFT size. */
    int getFreeSpace() const noexcept;

    //==============================================================================
    /** Sets the overlap factor for more responsive spectrum analysis.

        This can be called from any thread, also while the audio thread writes.

        @param overlapFactor    overlap factor (0.0 = no overlap, 0.75 = 75% overlap), limited to 0.95
    */
    void setOverlapFactor (float overlapFactor) noexcept;

    /** Returns the current overlap factor. */
    float getOverlapFactor() const noexcept { return overlapFactor.load(); }

    /** Returns the hop size (samples between FFT frames). */
    int getHopSize() const noexcept;

private:
    //==============================================================================
    bool readFrame (float* destBuffer, int frameSize) noexcept;

    static bool isFftSizeUpTo (int size, int largestSize) noexcept;

    static constexpr int defaultFftSize = 2048;
    static constexpr int fifoSizeInFrames = 4;

    // Set by the constructor and never changed, so that any thread can read it
    const int largestFftSize;

    std::atomic<int> fftSize;
    std::atomic<float> overlapFactor { 0.75f };

    AbstractFifo audioFifo;
    std::vector<float> sampleBuffer;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalyzerState)
};

} // namespace yup