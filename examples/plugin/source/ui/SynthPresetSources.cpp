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

#include "SynthPresetSources.h"
#include "SynthPresetLibrary.h"
#include "SynthTheme.h"
#include "../browser/PresetBrowseState.h"

#include <map>

namespace
{
constexpr float addFolderHeight = 24.0f;
constexpr int revealId = 1;
constexpr int removeId = 2;
constexpr int saveHereId = 3;
} // namespace

//==============================================================================
class SynthPresetSources::SourceItem : public yup::TreeViewItem
{
public:
    SourceItem (SynthPresetSources& ownerToNotify, PresetQuery::Source sourceToList, yup::String textToShow, const char* iconGlyph, yup::File folderToList = {}, bool isAddedFolder = false)
        : owner (ownerToNotify)
        , source (sourceToList)
        , folder (std::move (folderToList))
        , extraRoot (isAddedFolder)
        , text (std::move (textToShow))
        , icon (yup::String::fromUTF8 (iconGlyph))
    {
    }

    bool matches (const PresetQuery& query) const
    {
        return query.source == source && (source != PresetQuery::Source::folder || query.folder == folder);
    }

    yup::String getItemText() const override { return text; }

    yup::String getUniqueName() const override { return yup::String (static_cast<int> (source)) + ":" + folder.getFullPathName(); }

    bool hasItemIcon() const override { return true; }

    void paintItemIcon (yup::Graphics& g, yup::Rectangle<float> area, bool isSelected) const override
    {
        g.setFillColor (isSelected ? SynthTheme::accent (owner) : SynthTheme::textSecondary (owner));
        g.fillFittedText (icon, yup::ApplicationTheme::getGlobalTheme()->getDefaultIconFont().withHeight (area.getHeight() * 0.7f), area, yup::Justification::center);
    }

    void itemClicked (const yup::MouseEvent& event) override
    {
        owner.itemClicked (*this, event);
    }

    SynthPresetSources& owner;
    const PresetQuery::Source source;
    const yup::File folder;
    const bool extraRoot;

private:
    yup::String text;
    yup::String icon;
};

//==============================================================================
SynthPresetSources::SynthPresetSources (RetraceProcessor& processorToControl, SynthPresetLibrary& libraryToShow)
    : library (libraryToShow)
    , browser (processorToControl.getPresetBrowser())
{
    setOpaque (false);

    tree.setRootItemVisible (false);
    tree.setIndentSize (12.0f);
    tree.setDefaultItemHeight (22.0f);
    addAndMakeVisible (tree);

    addFolderButton.setButtonText ("+ Add folder...");
    SynthTheme::styleRaisedButton (addFolderButton);
    addFolderButton.onClick = [this] { addFolder(); };
    addAndMakeVisible (addFolderButton);

    library.getLibrary().addChangeListener (this);
    rebuildTree();
}

SynthPresetSources::~SynthPresetSources()
{
    library.getLibrary().removeChangeListener (this);
}

yup::String SynthPresetSources::getRevealCommandName()
{
#if YUP_MAC
    return "Show in Finder";
#elif YUP_WINDOWS
    return "Show in Explorer";
#else
    return "Show in File Manager";
#endif
}

void SynthPresetSources::resized()
{
    auto bounds = getLocalBounds();
    addFolderButton.setBounds (bounds.removeFromBottom (addFolderHeight));
    bounds.removeFromBottom (SynthTheme::controlGap);
    treeArea = bounds;
    tree.setBounds (treeArea.reduced (2.0f));
}

void SynthPresetSources::paint (yup::Graphics& g)
{
    SynthTheme::paintWell (g, *this, treeArea, SynthTheme::displayCorner);
}

void SynthPresetSources::paintOverChildren (yup::Graphics& g)
{
    SynthTheme::paintDisplayBorder (g, *this, treeArea);
}

void SynthPresetSources::changeListenerCallback (yup::ChangeBroadcaster*)
{
    rebuildTree();
}

void SynthPresetSources::rebuildTree()
{
    using Source = PresetQuery::Source;

    const auto openness = tree.getOpennessState (true);
    const auto extraFolders = library.getExtraFolders();
    auto root = std::make_unique<SourceItem> (*this, Source::all, yup::String(), "");

    root->addSubItem (std::make_unique<SourceItem> (*this, Source::favorites, "Favorites", YUP_ICON_STAR));
    root->addSubItem (std::make_unique<SourceItem> (*this, Source::all, "All presets", YUP_ICON_LIST));
    root->addSubItem (std::make_unique<SourceItem> (*this, Source::factory, "Factory", YUP_ICON_CUBE));

    for (const auto& folder : library.getLibrary().getFolders())
    {
        auto& folderItem = root->addSubItem (std::make_unique<SourceItem> (*this, Source::folder, folder.displayName, YUP_ICON_FOLDER, folder.folder, extraFolders.contains (folder.folder)));
        std::map<yup::String, yup::TreeViewItem*> itemsByPath { { folder.folder.getFullPathName(), &folderItem } };

        for (const auto& subfolder : library.getLibrary().getSubfolders (folder.folder))
        {
            const auto parent = itemsByPath.find (subfolder.getParentDirectory().getFullPathName());
            auto* parentItem = parent != itemsByPath.end() ? parent->second : &folderItem;
            itemsByPath[subfolder.getFullPathName()] = &parentItem->addSubItem (std::make_unique<SourceItem> (*this, Source::folder, subfolder.getFileName(), YUP_ICON_FOLDER, subfolder));
        }
    }

    tree.setRootItem (std::move (root));
    tree.restoreOpennessState (openness, false);
    selectQueriedItem (*tree.getRootItem());
}

/** Selects the item the query lists, which never changes the query back. */
void SynthPresetSources::selectQueriedItem (yup::TreeViewItem& item)
{
    for (int index = 0; index < item.getNumSubItems(); ++index)
    {
        auto* subItem = item.getSubItem (index);

        if (auto* source = dynamic_cast<SourceItem*> (subItem); source != nullptr && source->matches (browser.getQuery()))
        {
            source->setSelected (true);
            return;
        }

        selectQueriedItem (*subItem);
    }
}

void SynthPresetSources::itemClicked (SourceItem& item, const yup::MouseEvent& event)
{
    if (event.isRightButtonDown())
    {
        if (item.source == PresetQuery::Source::folder)
            showFolderMenu (item.folder, item.extraRoot, event.withRelativePositionTo (this).getPosition());
        else
            selectQueriedItem (*tree.getRootItem());

        return;
    }

    auto query = browser.getQuery();
    query.source = item.source;
    query.folder = item.folder;
    browser.setQuery (std::move (query));
}

void SynthPresetSources::showFolderMenu (const yup::File& folder, bool isExtraRoot, yup::Point<float> position)
{
    auto* parent = getPopupParentComponent();
    auto menu = yup::PopupMenu::create (yup::PopupMenu::Options {}
                                            .withParentComponent (parent)
                                            .withPosition (parent != nullptr ? parent->getLocalPoint (this, position) : position));

    menu->addItem (getRevealCommandName(), revealId);

    if (isExtraRoot)
        menu->addItem ("Remove folder", removeId);

    if (onSaveIntoFolder != nullptr)
    {
        menu->addSeparator();
        menu->addItem ("Save current patch here...", saveHereId);
    }

    menu->show ([safeThis = yup::WeakReference<yup::Component> (this), folder] (int selectedId)
    {
        auto* sources = dynamic_cast<SynthPresetSources*> (safeThis.get());

        if (sources == nullptr)
            return;

        if (selectedId == revealId)
        {
            folder.revealToUser();
        }
        else if (selectedId == removeId)
        {
            if (auto query = sources->browser.getQuery(); query.source == PresetQuery::Source::folder && (query.folder == folder || query.folder.isAChildOf (folder)))
            {
                query.source = PresetQuery::Source::all;
                query.folder = yup::File();
                sources->browser.setQuery (std::move (query));
            }

            sources->library.removeFolder (folder);
        }
        else if (selectedId == saveHereId && sources->onSaveIntoFolder != nullptr)
        {
            sources->onSaveIntoFolder (folder);
        }

        sources->selectQueriedItem (*sources->tree.getRootItem());
    });
}

void SynthPresetSources::addFolder()
{
    auto chooser = yup::FileChooser::create ("Add a preset folder", yup::File::getSpecialLocation (yup::File::userHomeDirectory));

    chooser->browseForDirectory ([safeThis = yup::WeakReference<yup::Component> (this)] (bool success, const yup::Array<yup::File>& results)
    {
        auto* sources = dynamic_cast<SynthPresetSources*> (safeThis.get());

        if (sources == nullptr || ! success || results.isEmpty())
            return;

        sources->library.addFolder (results[0]);
    });
}
