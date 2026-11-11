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

#include "SynthViewOptions.h"
#include "SynthThemes.h"

#include <utility>
#include <vector>

namespace
{
const yup::Identifier themeProperty ("theme");
const yup::Identifier widthProperty ("width");
const yup::Identifier heightProperty ("height");
const yup::Identifier frameRateProperty ("frameRate");

/** The switches by name, so the trees stay readable and survive new switches. */
std::vector<std::pair<yup::Identifier, bool*>> getSwitches (SynthViewOptions& options)
{
    auto& neon = options.neon;

    return { { "landscapes", &options.landscapes }, { "postEffect", &options.postEffect }, { "glow", &neon.glow },
             { "aberration", &neon.aberration }, { "grade", &neon.grade }, { "scanlines", &neon.scanlines },
             { "vignette", &neon.vignette }, { "grain", &neon.grain }, { "curvature", &neon.curvature },
             { "phosphor", &neon.phosphor }, { "reflection", &neon.reflection } };
}
} // namespace

const yup::Identifier SynthViewOptions::treeType ("View");

int SynthViewOptions::getThemeIndex() const
{
    return yup::jmax (0, SynthThemes::findTheme (theme));
}

yup::DataTree SynthViewOptions::toDataTree() const
{
    yup::DataTree tree (treeType);
    auto transaction = tree.beginTransaction();
    transaction.setProperty (themeProperty, theme);
    transaction.setProperty (widthProperty, editorSize.getWidth());
    transaction.setProperty (heightProperty, editorSize.getHeight());
    transaction.setProperty (frameRateProperty, frameRate);

    auto copy = *this;

    for (const auto& [name, flag] : getSwitches (copy))
        transaction.setProperty (name, *flag);

    return tree;
}

SynthViewOptions SynthViewOptions::fromDataTree (const yup::DataTree& tree, const SynthViewOptions& defaults)
{
    auto options = defaults;

    if (! tree.isValid())
        return options;

    options.theme = tree.getProperty (themeProperty, options.theme).toString();
    options.frameRate = yup::jlimit (0, 240, static_cast<int> (tree.getProperty (frameRateProperty, options.frameRate)));

    const auto width = static_cast<int> (tree.getProperty (widthProperty, 0));
    const auto height = static_cast<int> (tree.getProperty (heightProperty, 0));

    if (width > 0 && height > 0)
        options.editorSize = { width, height };

    for (const auto& [name, flag] : getSwitches (options))
        *flag = static_cast<bool> (tree.getProperty (name, *flag));

    return options;
}
