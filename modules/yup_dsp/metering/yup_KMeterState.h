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
    Real-time safe K-Meter state management following Bob Katz specifications.

    KMeterState handles thread-safe audio level metering with support for multiple
    measurement standards:
    - RMS (Root Mean Square) with flat frequency response
    - ITU BS.1770-4 LUFS (K-weighted loudness)
    - EBU R128 (integrated, short-term, momentary loudness)

    The class implements Bob Katz's K-System with three scale types:
    - K-20: 20dB headroom, for wide dynamic range (film, classical)
    - K-14: 14dB headroom, for pop/rock production
    - K-12: 12dB headroom, for broadcast/streaming

    Threading Model:
    - Audio thread: pushSamples() writes to lock-free FIFO (real-time safe)
    - Processing: processPendingAudio() reads FIFO and computes levels
    - UI thread: Getter methods read Atomic variables (wait-free)

    All level values are thread-safe using Atomic variables. The class uses AbstractFifo
    for lock-free communication between threads, ensuring no blocking operations occur
    in the audio path.

    Reference: Bob Katz "Mastering Audio" Chapter 13, "Level Practices (Part 2)"

    @see KMeterComponent, LevelProcessor, LoudnessFilter

    @tags{DSP, Metering}
*/
class YUP_API KMeterState
{
public:
    //==============================================================================
    /** Metering standard enumeration. */
    enum class MeteringStandard
    {
        rmsFlat,     /**< RMS with flat frequency response (600ms integration) */
        ituBS1770_4, /**< ITU BS.1770-4 K-weighted loudness (LUFS) */
        ebuR128      /**< EBU R128 with gating and program loudness */
    };

    /** K-System scale enumeration. */
    enum class Scale
    {
        k20, /**< K-20: 0dB @ -20dBFS, range -70 to +20 dB, 20dB headroom */
        k14, /**< K-14: 0dB @ -14dBFS, range -64 to +26 dB, 14dB headroom */
        k12  /**< K-12: 0dB @ -12dBFS, range -62 to +28 dB, 12dB headroom */
    };

    /** OVER counter mode enumeration. */
    enum class OverCounterMode
    {
        contiguous, /**< Counts contiguous samples at/above threshold (K-System recommendation). */
        total       /**< Counts total samples at/above threshold since reset. */
    };

    //==============================================================================
    /** Creates a KMeterState with default settings. */
    KMeterState();

    /** Creates a KMeterState with specified sample rate and channel count.

        @param sampleRate   the sample rate in Hz
        @param maxChannels  maximum number of channels to support
    */
    explicit KMeterState (double sampleRate, int maxChannels = 2);

    /** Destructor. */
    ~KMeterState();

    //==============================================================================
    /** Prepares the meter for processing.

        Allocates buffers and configures processors based on sample rate and channel count.
        Must be called before pushSamples().

        @param sampleRate   the sample rate in Hz
        @param maxChannels  maximum number of channels to support
    */
    void prepare (double sampleRate, int maxChannels);

    /** Resets all internal state.

        Clears buffers, resets levels, and clears peak hold values.
    */
    void reset() noexcept;

    //==============================================================================
    /** Pushes audio samples to the meter for processing (real-time safe).

        This method is designed to be called from the audio thread. It writes samples to an
        internal lock-free FIFO buffer without blocking or allocating memory.

        @param channelData  array of pointers to audio samples for each channel
        @param numChannels  number of channels (must match or be less than maxChannels from prepare())
        @param numSamples   number of samples per channel
    */
    void pushSamples (const float* const* channelData, int numChannels, int numSamples) noexcept;

    /** Pushes mono samples to the meter for processing (real-time safe).

        Convenience method for single-channel audio.

        @param samples      pointer to the audio samples
        @param numSamples   number of samples
    */
    void pushMonoSamples (const float* samples, int numSamples) noexcept;

    /** Processes pending audio from the FIFO (real-time safe).

        Reads samples from the FIFO and computes peak and average levels.
        Can be called from the audio thread, a background thread, or a timer callback.
    */
    void processPendingAudio() noexcept;

    /** Returns the number of samples currently in the FIFO waiting to be processed.

        Useful for testing and monitoring buffer fill levels.

        @returns  number of samples ready to be processed
    */
    int getNumSamplesInFifo() const noexcept;

    //==============================================================================
    /** Sets the metering standard.

        @param standard  the metering standard to use
    */
    void setMeteringStandard (MeteringStandard standard);

    /** Sets the K-System scale.

        @param scale  the K-System scale (K-20, K-14, or K-12)
    */
    void setScale (Scale scale);

    /** Sets the integration time for RMS averaging.

        @param seconds  integration time in seconds (default: 0.6s / 600ms)
    */
    void setIntegrationTime (double seconds);

    /** Sets the fall time for peak decay.

        @param seconds  fall time in seconds (default: 3.0s)
    */
    void setPeakFallTime (double seconds);

    /** Sets the fall time for average level decay.

        @param seconds  fall time in seconds (default: 0.6s)
    */
    void setAverageFallTime (double seconds);

    /** Sets the peak hold time.

        @param seconds  hold time in seconds (10.0 = 10s auto-release, -1.0 = infinite)
    */
    void setPeakHoldTime (double seconds);

    /** Sets the threshold for OVER counter.

        @param threshold  linear amplitude threshold (default: 0.999 = -0.001dBFS)
    */
    void setOverThreshold (float threshold);

    /** Sets how the OVER counter is computed.

        @param mode  counter mode (contiguous or total)
    */
    void setOverCounterMode (OverCounterMode mode);

    //==============================================================================
    /** Returns the peak level in calibrated dB (thread-safe).

        @param channel  channel index, or -1 for maximum across all channels

        @returns        peak level in calibrated dB
    */
    float getPeakLevel (int channel = -1) const noexcept;

    /** Returns the average level in calibrated dB (thread-safe).

        The returned value is in K-System calibrated dB, where 0dB represents the
        reference loudness (83dB SPL).

        @param channel  channel index, or -1 for maximum across all channels

        @returns        average level in calibrated dB
    */
    float getAverageLevel (int channel = -1) const noexcept;

    /** Returns the peak hold level in calibrated dB (thread-safe).

        @param channel  channel index, or -1 for maximum across all channels

        @returns        peak hold level in calibrated dB
    */
    float getPeakHoldLevel (int channel = -1) const noexcept;

    /** Returns the OVER counter value (thread-safe).

        The meaning depends on the current mode (contiguous or total).

        @returns  number of samples at/above threshold based on mode
    */
    int getOverCount() const noexcept;

    /** Returns whether any overflows have occurred since the last reset (thread-safe).

        @returns  true if any overflows have been detected
    */
    bool isClipping() const noexcept;

    //==============================================================================
    /** Returns the integrated loudness in LUFS (thread-safe, EBU R128 mode only).

        @returns  integrated loudness for the entire program
    */
    float getIntegratedLoudness() const noexcept;

    /** Returns the short-term loudness in LUFS (thread-safe, EBU R128 mode only).

        @returns  loudness over the last 3 seconds
    */
    float getShortTermLoudness() const noexcept;

    /** Returns the momentary loudness in LUFS (thread-safe, EBU R128 mode only).

        @returns  loudness over the last 400ms
    */
    float getMomentaryLoudness() const noexcept;

    /** Returns the loudness range in LU (thread-safe, EBU R128 mode only).

        @returns  statistical loudness range (10th to 95th percentile)
    */
    float getLoudnessRange() const noexcept;

    //==============================================================================
    /** Returns the current metering standard. */
    MeteringStandard getMeteringStandard() const noexcept { return meteringStandard; }

    /** Returns the current K-System scale. */
    Scale getScale() const noexcept { return scale; }

    /** Returns the number of channels. */
    int getNumChannels() const noexcept { return numChannels; }

    /** Returns the sample rate. */
    double getSampleRate() const noexcept { return sampleRate; }

    /** Returns the RMS integration time in seconds. */
    double getIntegrationTime() const noexcept { return integrationTime; }

    /** Returns the average fall time in seconds. */
    double getAverageFallTime() const noexcept { return averageFallTime; }

    /** Returns the peak hold time in seconds. */
    double getPeakHoldTime() const noexcept { return peakHoldTime; }

    /** Returns the current OVER counter mode. */
    OverCounterMode getOverCounterMode() const noexcept { return overCounterMode; }

    //==============================================================================
    /** Returns the scale offset for the given K-System scale (static).

        @param scale  the K-System scale

        @returns      offset in dBFS (-20 for K-20, -14 for K-14, -12 for K-12)
    */
    static float scaleOffsetForScale (Scale scale) noexcept;

    /** Returns the minimum display range for the given scale (static).

        @param scale  the K-System scale

        @returns      minimum dB value (-70 for K-20, -64 for K-14, -62 for K-12)
    */
    static float rangeMinForScale (Scale scale) noexcept;

    /** Returns the maximum display range for the given scale (static).

        @param scale  the K-System scale

        @returns      maximum dB value (+20 for K-20, +26 for K-14, +28 for K-12)
    */
    static float rangeMaxForScale (Scale scale) noexcept;

private:
    //==============================================================================
    struct ChannelState
    {
        float currentPeak = 0.0f;         // Current 1-sample peak
        float currentAverage = 0.0f;      // Current average level (linear)
        float currentAverageDb = -100.0f; // Current average level (dB)
        float peakHold = 0.0f;            // Peak hold value
        double peakHoldTimer = 0.0;       // Time since peak hold was set
        int contiguousOverSamples = 0;    // For contiguous OVER counter
        int totalOverflows = 0;           // For total OVER counter
    };

    //==============================================================================
    void updateProcessors();
    void processChannelLevels (int channel, const float* samples, int numSamples);

    //==============================================================================
    // Configuration
    double sampleRate = 48000.0;
    int numChannels = 2;
    MeteringStandard meteringStandard = MeteringStandard::rmsFlat;
    Scale scale = Scale::k20;

    double integrationTime = 0.6; // 600ms
    double peakFallTime = 3.0;    // 3 seconds
    double averageFallTime = 0.6; // 600ms
    double peakHoldTime = 10.0;   // 10 seconds (-1.0 = infinite)
    float overThreshold = 0.999f; // -0.001 dBFS
    OverCounterMode overCounterMode = OverCounterMode::contiguous;

    // Lock-free FIFO for audio data
    std::unique_ptr<AbstractFifo> audioFifo;
    std::vector<float> sampleBuffer; // Interleaved samples
    int fifoSize = 8192;

    // Per-channel state
    std::vector<ChannelState> channels;

    // Level processors
    std::vector<LevelProcessor> levelProcessors;    // One per channel
    std::vector<LoudnessFilter> loudnessFilters;    // One per channel (for ITU/EBU K-weighting)
    std::vector<std::vector<float>> channelBuffers; // Per-channel de-interleaving buffers
    std::vector<float> filteredBuffer;              // Temporary buffer for K-weighted samples

    // Atomic state for UI thread (wait-free reads)
    Atomic<float> atomicPeakLevelDb { -100.0f };
    Atomic<float> atomicAverageLevelDb { -100.0f };
    Atomic<float> atomicPeakHoldLevelDb { -100.0f };
    Atomic<int> atomicOverCount { 0 };
    Atomic<bool> atomicClipping { false };

    // EBU R128 extended metrics (Phase 4)
    Atomic<float> atomicIntegratedLoudness { -70.0f };
    Atomic<float> atomicShortTermLoudness { -70.0f };
    Atomic<float> atomicMomentaryLoudness { -70.0f };
    Atomic<float> atomicLoudnessRange { 0.0f };

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KMeterState)
};

} // namespace yup
