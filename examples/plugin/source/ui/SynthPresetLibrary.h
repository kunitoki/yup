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

#include "../browser/PresetLibrary.h"
#include "SynthPresetUserData.h"

#include <memory>

//==============================================================================
/** Retrace's preset library, shared by every instance in the process so a big folder is
    scanned once: the factory presets, the user folder and the folders the user added,
    with the favorites and ratings from the user's settings.

    All of the methods are for the message thread.

    @see PresetLibrary, SynthPresetFormat, SynthPresetUserData
*/
class SynthPresetLibrary
{
public:
    /** Returns the library every instance shares. When no instance holds it, it is created,
        filled with the factory presets and a scan of the folders starts.
    */
    static std::shared_ptr<SynthPresetLibrary> acquire();

    /** Returns the presets. */
    PresetLibrary& getLibrary() noexcept { return library; }

    /** Returns the favorites and ratings. */
    PresetUserData& getUserData() noexcept { return userData; }

    /** Returns the folder user presets are saved in by default, creating it when missing. */
    static yup::File getUserFolder();

    /** Returns the folders the user added, in the order they were added. */
    yup::Array<yup::File> getExtraFolders() const;

    /** Adds a folder to scan, saves it in the settings and rescans. Ignores the user folder
        and folders already added.
    */
    void addFolder (const yup::File& folder);

    /** Removes a folder the user added, saves the settings and rescans. */
    void removeFolder (const yup::File& folder);

private:
    SynthPresetLibrary();
    void applyFolders();

    SynthPresetUserData userData;
    PresetLibrary library;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthPresetLibrary)
};
