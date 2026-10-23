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

#include "SynthTypes.h"

#include <vector>

//==============================================================================
/** One of the eight algorithms of Yamaha's four operator synthesisers.

    The DX21, DX27, DX100 and TX81Z share the DX9's eight algorithms, which are the four
    operator parts of DX7 algorithms 1, 14, 8, 7, 5, 22, 31 and 32, renumbered so that
    operator 4 is the one with feedback. Operators are numbered 1 to 4 here, as on the
    hardware; the FM matrix indexes them from 0.
*/
struct SynthAlgorithm
{
    /** A modulation link: modulator feeds carrier, both numbered 1 to 4. */
    struct Link
    {
        int modulator;
        int carrier;
    };

    std::vector<Link> links;
    std::vector<int> outputs; /**< The operators that are heard. */
};

/** Returns the eight algorithms, index 0 being algorithm 1. */
inline const std::vector<SynthAlgorithm>& getSynthAlgorithms()
{
    static const std::vector<SynthAlgorithm> algorithms {
        { { { 4, 3 }, { 3, 2 }, { 2, 1 } }, { 1 } },
        { { { 3, 2 }, { 4, 2 }, { 2, 1 } }, { 1 } },
        { { { 3, 2 }, { 2, 1 }, { 4, 1 } }, { 1 } },
        { { { 4, 3 }, { 3, 1 }, { 2, 1 } }, { 1 } },
        { { { 2, 1 }, { 4, 3 } }, { 1, 3 } },
        { { { 4, 1 }, { 4, 2 }, { 4, 3 } }, { 1, 2, 3 } },
        { { { 4, 3 } }, { 1, 2, 3 } },
        { {}, { 1, 2, 3, 4 } }
    };

    return algorithms;
}
