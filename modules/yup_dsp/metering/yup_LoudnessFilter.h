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
    ITU-R BS.1770-4 K-weighting filter implementation.

    This filter implements the two-stage K-weighting filter specified in ITU-R BS.1770-4
    for loudness measurement. The filter approximates the frequency response of the human
    head-related transfer function and removes low-frequency content.

    The K-weighting consists of two cascaded biquad filters:
    1. **Pre-filter (High-shelf)**: +4dB gain above 1681 Hz
       - Approximates head-related transfer function
       - Emphasizes high frequencies

    2. **Highpass filter**: 38 Hz cutoff, 2nd order Butterworth
       - Removes DC offset and low-frequency rumble
       - Q = 0.5 (Butterworth characteristic)

    After K-weighting is applied, the mean square of the filtered signal is computed
    and converted to LUFS (Loudness Units Full Scale) using:
    LUFS = -0.691 + 10 * log10(meanSquare)

    Reference: ITU-R BS.1770-4 Section 5.1 "K-weighting filter"

    @see KMeterState, LevelProcessor

    @tags{DSP, Metering}
*/
class YUP_API LoudnessFilter
{
public:
    //==============================================================================
    /** Creates a LoudnessFilter with default settings. */
    LoudnessFilter();

    /** Destructor. */
    ~LoudnessFilter();

    //==============================================================================
    /** Prepares the filter for processing.

        This must be called before processing audio. It calculates the filter coefficients
        based on the specified sample rate.

        @param sampleRate     the sample rate in Hz
        @param maxBlockSize   maximum expected block size (for optimization)
    */
    void prepare (double sampleRate, int maxBlockSize);

    /** Resets the filter state.

        Clears all internal delay lines to zero.
    */
    void reset() noexcept;

    //==============================================================================
    /** Processes a single sample through the K-weighting filter (real-time safe).

        @param sample  the input sample

        @returns       the K-weighted output sample
    */
    float processSample (float sample) noexcept;

    /** Processes a block of samples in-place through the K-weighting filter (real-time safe).

        The samples are filtered in-place - the input buffer is modified.

        @param samples      pointer to the audio samples (modified in-place)
        @param numSamples   number of samples to process
    */
    void processBlock (float* samples, int numSamples) noexcept;

    /** Returns the current sample rate. */
    double getSampleRate() const noexcept { return sampleRate; }

    //==============================================================================
    /** Calculates the pre-filter (high-shelf) coefficients for ITU BS.1770-4.

        The pre-filter is a high-shelf filter with +4dB gain above 1681 Hz.
        Uses bilinear transform to convert from analog to digital domain.

        @param sampleRate  sample rate in Hz
        @param b0          on output, numerator coefficient b0
        @param b1          on output, numerator coefficient b1
        @param b2          on output, numerator coefficient b2
        @param a0          on output, denominator coefficient a0
        @param a1          on output, denominator coefficient a1
        @param a2          on output, denominator coefficient a2
    */
    static void calculatePreFilterCoefficients (double sampleRate,
                                                double& b0,
                                                double& b1,
                                                double& b2,
                                                double& a0,
                                                double& a1,
                                                double& a2);

    /** Calculates the highpass filter coefficients for ITU BS.1770-4.

        The highpass filter is a 2nd order Butterworth with 38 Hz cutoff and Q = 0.5.
        Uses bilinear transform to convert from analog to digital domain.

        @param sampleRate  sample rate in Hz
        @param b0          on output, numerator coefficient b0
        @param b1          on output, numerator coefficient b1
        @param b2          on output, numerator coefficient b2
        @param a0          on output, denominator coefficient a0
        @param a1          on output, denominator coefficient a1
        @param a2          on output, denominator coefficient a2
    */
    static void calculateHighpassCoefficients (double sampleRate,
                                               double& b0,
                                               double& b1,
                                               double& b2,
                                               double& a0,
                                               double& a1,
                                               double& a2);

private:
    //==============================================================================
    void updateCoefficients();

    //==============================================================================
    double sampleRate = 48000.0;

    // Two-stage filter cascade
    Biquad<float, double> preFilter;      // Stage 1: High-shelf (+4dB above 1681 Hz)
    Biquad<float, double> highpassFilter; // Stage 2: Highpass (38 Hz, Q=0.5)

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoudnessFilter)
};

} // namespace yup
