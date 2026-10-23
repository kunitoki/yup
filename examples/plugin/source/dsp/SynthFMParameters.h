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

#include "SynthParameterFactory.h"
#include "SynthTypes.h"

//==============================================================================
/** A plain snapshot of the phase modulation matrix.

    amounts[i][j] is how much operator j modulates operator i: in periods per unit of
    output off the diagonal, and as a normalized feedback amount on it.
*/
struct SynthFMValues
{
    using Row = std::array<float, SynthExample::oscillatorCount>;

    std::array<Row, SynthExample::oscillatorCount> amounts {};

    bool operator== (const SynthFMValues&) const = default;

    /** True when no operator modulates any, itself included. */
    bool isZero() const noexcept
    {
        for (const auto& row : amounts)
            for (const auto amount : row)
                if (amount != 0.0f)
                    return false;

        return true;
    }
};

//==============================================================================
/** The sixteen amounts of the phase modulation matrix, IDs "fm.11" to "fm.44".

    "fm.ij" is how much operator j modulates operator i; the diagonal "fm.ii" is the
    feedback of operator i.

    @see SynthFMValues, yup::PhaseModulationMatrix
*/
class SynthFMParameters
{
public:
    /** The largest off-diagonal amount, in periods of phase: the TX81Z's full scale. */
    static constexpr float maximumAmount = 4.0f;

    /** Creates every amount at zero. */
    SynthFMParameters();

    /** Registers every parameter with the processor. */
    void addTo (yup::AudioProcessor& processor) const;

    /** Takes a snapshot. */
    SynthFMValues read (SynthParameterReader reader = {}) const noexcept;

    /** Returns the parameter of operator modulator's amount into operator carrier. */
    const yup::AudioParameter::Ptr& get (int carrier, int modulator) const noexcept
    {
        return amounts[static_cast<std::size_t> (carrier)][static_cast<std::size_t> (modulator)];
    }

    std::array<std::array<yup::AudioParameter::Ptr, SynthExample::oscillatorCount>, SynthExample::oscillatorCount> amounts;
};
