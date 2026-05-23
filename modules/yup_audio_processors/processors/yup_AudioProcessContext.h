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
    All inputs available to an AudioProcessor for a single processing block.

    AudioProcessContext<FloatType> is passed to AudioProcessor::processBlock() and bundles:
      - the audio I/O buffer (in-place processing model, single or double precision),
      - sample-accurate MIDI events,
      - sample-accurate parameter automation events, and
      - the global transport sample position.

    Use AudioProcessContext<float> for the primary single-precision processing path.
    Use AudioProcessContext<double> for double-precision processing in processors that
    override processBlock(AudioProcessContext<double>&) and return true from
    supportsDoublePrecisionProcessing().

    Processors that only need audio and MIDI can ignore the @c params and
    @c samplePosition fields. Processors that implement sample-accurate
    automation should use AudioParameterHandle::prepareBlock() and
    AudioParameterHandle::advanceToSample() together with the @c params buffer.

    @see AudioProcessor, ParameterChangeBuffer, AudioParameterHandle, MidiBuffer
*/
template <typename FloatType>
struct AudioProcessContext
{
    /** Audio I/O buffer. Process in-place: read and write the same channels. */
    AudioBuffer<FloatType>& audio;

    /** MIDI events for this block, sorted by samplePosition in [0, blockSize). */
    MidiBuffer& midi;

    /** Parameter automation events for this block, sorted by sampleOffset in [0, blockSize). */
    ParameterChangeBuffer& params;

    /** Global sample position at the start of this block (from the transport). */
    int64_t samplePosition = 0;
};

} // namespace yup
