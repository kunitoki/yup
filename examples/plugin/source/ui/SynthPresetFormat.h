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

#include "../browser/PresetFormat.h"
#include "../dsp/SynthPatch.h"

//==============================================================================
/** Retrace's presets as the browser sees them: `.retrace` files and the factory presets.

    @see PresetFormat, SynthPatch, SynthPresetLibrary
*/
namespace SynthPresetFormat
{
/** Returns the `.retrace` format. Files that are not patches are skipped, and writing
    metadata keeps the sound and the macro names.
*/
PresetFormat create();

/** Returns the browser entry of a factory preset. */
PresetEntry makeFactoryEntry (int index);

/** Returns an entry holding a patch's id and description, without a key, file or location. */
PresetEntry fromInfo (const SynthPatch::PresetInfo& info);
} // namespace SynthPresetFormat
