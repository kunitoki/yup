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

#include "SynthPatchMigration.h"

#include <array>

namespace SynthPatchMigration
{
namespace
{
/** Version 3 to 4: the 1x quality was added in front of 2x, so every quality index moves up by one. */
void addHostRateQuality (yup::DataTree& state)
{
    const yup::Identifier quality ("global.quality");

    if (! state.hasProperty (quality))
        return;

    auto transaction = state.beginTransaction();
    transaction.setProperty (quality, static_cast<double> (state.getProperty (quality)) + 1.0);
}

using Step = void (*) (yup::DataTree&);

// steps[i] rewrites a patch from version oldestVersion + i to the next one.
constexpr std::array<Step, currentVersion - oldestVersion> steps { addHostRateQuality };
} // namespace

const yup::Identifier& getVersionProperty()
{
    static const yup::Identifier property ("version");
    return property;
}

int getVersion (const yup::DataTree& state)
{
    return static_cast<int> (state.getProperty (getVersionProperty(), 0));
}

yup::Result migrate (yup::DataTree& state)
{
    const auto version = getVersion (state);

    if (version > currentVersion)
        return yup::Result::fail ("The patch was saved by a newer version of Retrace");

    if (version < oldestVersion)
        return yup::Result::fail ("The patch was saved by an older version of Retrace, which this version cannot load");

    for (auto from = version; from < currentVersion; ++from)
    {
        steps[static_cast<std::size_t> (from - oldestVersion)](state);

        auto transaction = state.beginTransaction();
        transaction.setProperty (getVersionProperty(), from + 1);
    }

    return yup::Result::ok();
}
} // namespace SynthPatchMigration
