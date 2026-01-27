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
    Handles level measurement calculations including RMS, peak detection, and ballistics.

    LevelProcessor provides efficient real-time safe algorithms for measuring audio levels
    using various techniques:
    - RMS (Root Mean Square) with configurable integration time
    - 1-sample peak detection
    - Peak fall with exponential decay
    - Ballistics for smooth transitions

    All processing methods are marked noexcept and designed to be real-time safe with
    no memory allocation or blocking operations.

    @see KMeterState, Decibels

    @tags{DSP, Metering}
*/
class YUP_API LevelProcessor
{
public:
    //==============================================================================
    /** Creates a LevelProcessor with default settings. */
    LevelProcessor();

    /** Destructor. */
    ~LevelProcessor();

    //==============================================================================
    /** Sets the sample rate for ballistics calculations.

        @param sampleRate  the sample rate in Hz
    */
    void setSampleRate (double sampleRate);

    /** Sets the integration time for RMS averaging.

        The integration time determines the window size for RMS calculation.
        Longer times provide smoother but less responsive measurements.

        @param seconds  integration time in seconds (default: 0.6s / 600ms)
    */
    void setIntegrationTime (double seconds);

    /** Sets the fall time for peak decay.

        The fall time controls how quickly the peak level falls back down after
        a transient. Longer times hold peaks longer.

        @param seconds  fall time in seconds (default: 3.0s)
    */
    void setFallTime (double seconds);

    /** Returns the current sample rate. */
    double getSampleRate() const noexcept { return sampleRate; }

    /** Returns the current integration time in seconds. */
    double getIntegrationTime() const noexcept { return integrationTime; }

    /** Returns the current fall time in seconds. */
    double getFallTime() const noexcept { return fallTime; }

    //==============================================================================
    /** Resets all internal state.

        Clears the RMS buffer and resets all accumulated values.
    */
    void reset() noexcept;

    //==============================================================================
    /** Processes a block of samples to find the peak value (real-time safe).

        Finds the maximum absolute value in the sample block.

        @param samples     pointer to the audio samples
        @param numSamples  number of samples to process
        @param peakOut     on output, contains the peak linear amplitude (0.0 to 1.0+)
    */
    void processPeak (const float* samples, int numSamples, float& peakOut) noexcept;

    /** Processes a block of samples to calculate RMS level (real-time safe).

        Calculates the Root Mean Square level using a moving window average.
        The window size is determined by integrationTime * sampleRate.

        @param samples     pointer to the audio samples
        @param numSamples  number of samples to process
        @param rmsOut      on output, contains the RMS linear amplitude (0.0 to 1.0+)
    */
    void processRMS (const float* samples, int numSamples, float& rmsOut) noexcept;

    /** Applies peak fall ballistics (real-time safe).

        Uses exponential decay to smoothly reduce the peak level over time.

        @param currentPeak  the current peak value to apply fall to
        @param timeDelta    time elapsed since last update in seconds
        @param peakOut      on output, contains the peak after fall applied
    */
    void processPeakWithFall (float currentPeak, double timeDelta, float& peakOut) noexcept;

    //==============================================================================
    /** Calculates exponential ballistics for smooth parameter transitions (static, real-time safe).

        Uses first-order exponential smoothing:
        output = current + alpha * (target - current)
        where alpha = 1 - exp(-timeDelta / timeConstant)

        @param current        the current value
        @param target         the target value to approach
        @param timeConstant   time constant for the exponential curve (seconds)
        @param timeDelta      time elapsed since last update (seconds)

        @returns              the smoothed output value
    */
    static float calculateBallistics (float current, float target, double timeConstant, double timeDelta) noexcept;

private:
    //==============================================================================
    void updateRMSBufferSize();

    //==============================================================================
    double sampleRate = 48000.0;
    double integrationTime = 0.6; // 600ms
    double fallTime = 3.0;        // 3 seconds

    // RMS state (circular buffer for moving average)
    std::vector<float> rmsBuffer;
    int rmsBufferPos = 0;
    double rmsSumSquares = 0.0;
    int rmsBufferSize = 0;

    //==============================================================================
    YUP_LEAK_DETECTOR (LevelProcessor)
};

} // namespace yup
