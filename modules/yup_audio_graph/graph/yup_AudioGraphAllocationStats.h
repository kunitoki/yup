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
    Lightweight diagnostics describing the most recently compiled graph plan.
*/
struct AudioGraphAllocationStats
{
    /** Number of preallocated node scratch audio buffers. */
    int scratchAudioBuffers = 0;

    /** Number of preallocated node MIDI buffers. */
    int midiBuffers = 0;

    /** Number of per-connection delay lines. */
    int delayLines = 0;

    /** Maximum latency compensation inserted on any graph output path. */
    int totalCompensationSamples = 0;

    /** Maximum preallocated audio channel count used by any node or graph endpoint. */
    int maxPreallocatedChannels = 0;

    /** Maximum preallocated block size for the compiled plan. */
    int maxPreallocatedBlockSize = 0;
};

} // namespace yup
