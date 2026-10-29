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

#include "SynthEffectChain.h"

#include <algorithm>
#include <vector>

namespace
{
const yup::Identifier effectType ("Effect");
const yup::Identifier nameProperty ("name");
} // namespace

const yup::Identifier SynthEffectChain::treeType ("EffectChain");

SynthEffectChain::SynthEffectChain()
    : packed (pack (getDefaultOrder()))
{
}

SynthEffectOrder SynthEffectChain::getDefaultOrder() noexcept
{
    using Type = SynthEffectType;
    return { Type::distortion, Type::bitcrusher, Type::equalizer, Type::compressor, Type::chorus,
             Type::phaser, Type::flanger, Type::delay, Type::reverb, Type::surround };
}

void SynthEffectChain::setOrder (const SynthEffectOrder& order) noexcept
{
    std::array<bool, SynthExample::effectCount> seen {};

    for (const auto type : order)
    {
        const auto index = static_cast<std::size_t> (type);

        if (index >= seen.size() || seen[index])
            return;

        seen[index] = true;
    }

    packed.store (pack (order), std::memory_order_relaxed);
}

yup::DataTree SynthEffectChain::createTree() const
{
    const auto names = getSynthEffectNames();

    yup::DataTree tree (treeType);
    auto transaction = tree.beginTransaction();

    for (const auto type : getOrder())
    {
        yup::DataTree child (effectType);

        {
            auto childTransaction = child.beginTransaction();
            childTransaction.setProperty (nameProperty, names[static_cast<int> (type)]);
        }

        transaction.addChild (child);
    }

    return tree;
}

void SynthEffectChain::readFrom (const yup::DataTree& tree)
{
    const auto names = getSynthEffectNames();

    std::vector<SynthEffectType> chain;
    std::array<bool, SynthExample::effectCount> placed {};

    const auto place = [&] (int index)
    {
        if (! yup::isPositiveAndBelow (index, SynthExample::effectCount) || placed[static_cast<std::size_t> (index)])
            return;

        placed[static_cast<std::size_t> (index)] = true;
        chain.push_back (static_cast<SynthEffectType> (index));
    };

    if (tree.isValid() && tree.getType() == treeType)
    {
        for (int index = 0; index < tree.getNumChildren(); ++index)
        {
            const auto child = tree.getChild (index);

            if (child.getType() == effectType)
                place (names.indexOf (child.getProperty (nameProperty, {}).toString(), true));
        }
    }

    for (const auto type : getDefaultOrder())
        place (static_cast<int> (type));

    SynthEffectOrder order {};
    std::copy (chain.begin(), chain.end(), order.begin());
    setOrder (order);
}

yup::uint64 SynthEffectChain::pack (const SynthEffectOrder& order) noexcept
{
    yup::uint64 bits = 0;

    for (std::size_t position = 0; position < order.size(); ++position)
        bits |= static_cast<yup::uint64> (order[position]) << (position * bitsPerEffect);

    return bits;
}

SynthEffectOrder SynthEffectChain::unpack (yup::uint64 bits) noexcept
{
    constexpr yup::uint64 mask = (1u << bitsPerEffect) - 1u;
    SynthEffectOrder order {};

    for (std::size_t position = 0; position < order.size(); ++position)
        order[position] = static_cast<SynthEffectType> ((bits >> (position * bitsPerEffect)) & mask);

    return order;
}
