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

#include "SynthEffectsParameters.h"

#include <atomic>

//==============================================================================
/** The order the master effects run in: patch state, not a parameter.

    The order cannot be automated, so it is saved with the patch as a child of its own
    holding one "Effect" child per effect, by name, from first to last, and the editor undoes its changes through its history.
    It is packed into one atomic word, so the audio thread reads it without locking.

    @see SynthEffectsRack, SynthEffectsParameters
*/
class SynthEffectChain
{
public:
    /** Starts with the default order. */
    SynthEffectChain();

    /** Returns the order, from the first effect to process to the last. Any thread. */
    SynthEffectOrder getOrder() const noexcept { return unpack (packed.load (std::memory_order_relaxed)); }

    /** Replaces the order; one that is not a permutation of every effect is ignored. Message thread. */
    void setOrder (const SynthEffectOrder& order) noexcept;

    /** Writes the order into an "EffectChain" tree, one "Effect" child per effect in chain order. */
    yup::DataTree createTree() const;

    /** Replaces the order from an "EffectChain" tree. Unknown names are skipped and
        effects it does not list follow in their default order, so a missing tree loads
        the default. Message thread.
    */
    void readFrom (const yup::DataTree& tree);

    /** The type of the tree createTree() returns. */
    static const yup::Identifier treeType;

    /** Returns the order a new patch starts with. */
    static SynthEffectOrder getDefaultOrder() noexcept;

private:
    static constexpr int bitsPerEffect = 4;
    static_assert (SynthExample::effectCount <= (1 << bitsPerEffect) && SynthExample::effectCount * bitsPerEffect <= 64);

    static yup::uint64 pack (const SynthEffectOrder& order) noexcept;
    static SynthEffectOrder unpack (yup::uint64 bits) noexcept;

    std::atomic<yup::uint64> packed;

    YUP_DECLARE_NON_COPYABLE (SynthEffectChain)
};
