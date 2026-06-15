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
    The audio domain specification for configuring an audio processor, including sample
    rate, maximum block size, and number of channels.

    @see AudioProcessor, DomainProcessor
*/
class AudioSpec
{
public:
    /** Creates an AudioSpec with the given sample rate, maximum block size, and number of channels.

        When configuring an AudioProcessor, the sample rate and maximum block size should be set
        to the values provided by the host or audio engine. The number of channels should be set
        to the maximum number of channels that the processor will handle, which may be determined by
        the bus layout or other factors.

        @param sampleRate The sample rate in Hz (e.g., 44100.0f).
        @param maxBlockSize The maximum block size in samples (e.g., 1024).
        @param numChannels The number of channels (e.g., 2 for stereo).
    */
    AudioSpec (float sampleRate, int maxBlockSize, int numChannels = 2)
        : sampleRate (sampleRate)
        , maxBlockSize (maxBlockSize)
        , numChannels (numChannels)
    {
    }

    /** Returns the sample rate in Hz. */
    float sampleRate = 44100.0f;

    /** Returns the maximum block size in samples. */
    int maxBlockSize = 1024;

    /** Returns the number of channels. */
    int numChannels = 2;
};

} // namespace yup
