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

#include "../RetraceProcessor.h"

#include <yup_gui/yup_gui.h>

#include <functional>

class PresetBrowseState;
class SynthPresetLibrary;

//==============================================================================
/** The sources of the preset browser: Favorites, All presets, Factory, then the user
    folder and every folder the user added, each with its subfolders, and below them
    "+ Add folder...".

    Clicking a source lists its presets; a folder lists the presets in it and in every
    subfolder. The subfolders come from the library's last scan, so the tree never reads
    the disk. A right click on a folder shows it in the file manager, removes a folder the
    user added, and saves the current patch into it.

    @see SynthPresetBrowser, PresetLibrary
*/
class SynthPresetSources
    : public yup::Component
    , private yup::ChangeListener
{
public:
    SynthPresetSources (RetraceProcessor& processorToControl, SynthPresetLibrary& libraryToShow);
    ~SynthPresetSources() override;

    /** Called with a folder chosen for "Save current patch here..." in its right click menu,
        which offers the item only when this is set.
    */
    std::function<void (const yup::File& folder)> onSaveIntoFolder;

    /** Returns the name of the command that reveals a file in the platform's file manager. */
    static yup::String getRevealCommandName();

    void resized() override;
    void paint (yup::Graphics& g) override;
    void paintOverChildren (yup::Graphics& g) override;

private:
    class SourceItem;

    void changeListenerCallback (yup::ChangeBroadcaster* source) override;
    void rebuildTree();
    void selectQueriedItem (yup::TreeViewItem& item);
    void itemClicked (SourceItem& item, const yup::MouseEvent& event);
    void showFolderMenu (const yup::File& folder, bool isExtraRoot, yup::Point<float> position);
    void addFolder();

    SynthPresetLibrary& library;
    PresetBrowseState& browser;
    yup::TreeView tree;
    yup::TextButton addFolderButton;
    yup::Rectangle<float> treeArea;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthPresetSources)
};
