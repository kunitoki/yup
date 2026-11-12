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

#include "SynthNeonEffect.h"

//==============================================================================
/** How one instance shows itself: its theme, editor size, frame rate, landscape
    displays and post effect.

    Each plugin instance keeps its own with the session (see RetraceProcessor::getViewOptions()),
    so instances in one host can look different. New instances start with the defaults kept in
    SynthSettings, which "Save as Default" in the settings menu replaces. Presets never carry them.
*/
struct SynthViewOptions
{
    yup::String theme; /**< The name of a theme in SynthThemes, or empty for the default. */
    yup::Size<int> editorSize; /**< The size the editor was left at, or empty for its design size. */
    int frameRate = 60; /**< Frames per second, or 0 to follow the display refresh. */
    bool landscapes = true; /**< Draws the filter, envelope and LFO curves as shader landscapes. */
    bool postEffect = true;
    SynthNeonEffect::Options neon;

    bool operator== (const SynthViewOptions&) const = default;

    /** Returns the index of the theme in SynthThemes::getThemes(), the default one when the
        name is empty or unknown. */
    int getThemeIndex() const;

    /** Returns the options as a tree, see treeType. */
    yup::DataTree toDataTree() const;

    /** Reads options from a tree written by toDataTree(), or any tree holding its properties.

        @param tree       The tree to read, which may be invalid.
        @param defaults   The values kept for anything the tree does not hold.
    */
    static SynthViewOptions fromDataTree (const yup::DataTree& tree, const SynthViewOptions& defaults);

    /** The type of the tree the options are written to. */
    static const yup::Identifier treeType;
};
