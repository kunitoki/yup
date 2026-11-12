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

#include <yup_data_model/yup_data_model.h>

//==============================================================================
/** Brings patches saved by earlier versions of Retrace up to the current format.

    Every format change bumps currentVersion and adds one step that rewrites a patch
    from the previous version to it. A patch is migrated by running every step from its
    own version up, in order, so each step only knows about its two neighbouring formats.

    @see SynthPatch::loadState
*/
namespace SynthPatchMigration
{
/** The version patches are saved with. */
inline constexpr int currentVersion = 4;

/** The oldest version that can still be migrated: the first one Retrace shipped with. */
inline constexpr int oldestVersion = 3;

/** The property holding the version a patch was saved with. */
const yup::Identifier& getVersionProperty();

/** Returns the version a patch was saved with, 0 when it has none. */
int getVersion (const yup::DataTree& state);

/** Rewrites a patch in place to currentVersion, stamping the new version on it.

    A patch already at currentVersion is left untouched. Fails, leaving the patch as it
    is, when it is newer than currentVersion or older than oldestVersion.
*/
yup::Result migrate (yup::DataTree& state);
} // namespace SynthPatchMigration
