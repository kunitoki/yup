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

#include <algorithm>
#include <array>
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

/** Returns the index of the algorithm a matrix is wired as, or -1 when it is wired as none.

    Only which links are on counts, not their amounts, and feedback is ignored; an
    operator is heard when its level is above zero.

    @param amounts amounts[carrier][modulator], both indexed from 0
    @param levels  The output level of every operator
*/
inline int findSynthAlgorithm (const std::array<std::array<float, SynthExample::oscillatorCount>, SynthExample::oscillatorCount>& amounts,
                               const std::array<float, SynthExample::oscillatorCount>& levels)
{
    const auto& algorithms = getSynthAlgorithms();

    for (std::size_t index = 0; index < algorithms.size(); ++index)
    {
        const auto& algorithm = algorithms[index];
        auto matches = true;

        for (int carrier = 0; carrier < SynthExample::oscillatorCount && matches; ++carrier)
        {
            const auto isOutput = std::find (algorithm.outputs.begin(), algorithm.outputs.end(), carrier + 1) != algorithm.outputs.end();
            matches = isOutput == (levels[static_cast<std::size_t> (carrier)] > 0.0f);

            for (int modulator = 0; modulator < SynthExample::oscillatorCount && matches; ++modulator)
            {
                if (modulator == carrier)
                    continue;

                const auto isLink = std::any_of (algorithm.links.begin(), algorithm.links.end(), [&] (const SynthAlgorithm::Link& link)
                {
                    return link.modulator == modulator + 1 && link.carrier == carrier + 1;
                });

                matches = isLink == (amounts[static_cast<std::size_t> (carrier)][static_cast<std::size_t> (modulator)] > 0.0f);
            }
        }

        if (matches)
            return static_cast<int> (index);
    }

    return -1;
}
