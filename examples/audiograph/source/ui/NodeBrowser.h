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

#include <functional>
#include <memory>

#include "../nodes/NodeRegistry.h"
#include "../nodes/SubgraphNode.h"

//==============================================================================
/**
    The inline browser of everything that can be added to a graph.

    Shows internal nodes, subgraphs and the discovered plugins grouped by format and
    vendor. Items are added by double-clicking them or by dragging them onto a graph,
    both of which carry a key understood by AudioGraphEditorPanel::addBrowserItem().
*/
class NodeBrowser final : public yup::Component
{
public:
    explicit NodeBrowser (NodeRegistry& registryToUse)
        : registry (registryToUse)
    {
        filterEditor.onTextChange = [this]
        {
            refresh();
        };
        addAndMakeVisible (filterEditor);

        tree.setRootItemVisible (false);
        tree.onReturnKeyPressed = [this] (yup::TreeViewItem& item)
        {
            if (auto* entry = dynamic_cast<EntryItem*> (&item))
                choose (entry->key);
        };
        addAndMakeVisible (tree);

        refresh();
    }

    //==============================================================================
    static yup::String makeNodeKey (const yup::String& identifier) { return "node:" + identifier; }

    static yup::String makeSubgraphKey (int presetID) { return "subgraph:" + yup::String (presetID); }

    static yup::String makePluginKey (int pluginIndex) { return "plugin:" + yup::String (pluginIndex); }

    //==============================================================================
    /** Rebuilds the tree from the registry, keeping the open folders. */
    void refresh()
    {
        const auto filter = filterEditor.getText().trim();
        const bool isFiltering = filter.isNotEmpty();

        if (! isFiltering && tree.getNumRowsInTree() > 0 && ! wasFiltering)
            savedOpenness = tree.getOpennessState (true);

        auto root = std::make_unique<FolderItem> ("Browser");

        auto matches = [&filter] (const yup::String& text)
        {
            return filter.isEmpty() || text.containsIgnoreCase (filter);
        };

        auto internals = std::make_unique<FolderItem> ("Internal");
        auto identifiers = registry.getInternalNodeIdentifiers();
        std::sort (identifiers.begin(), identifiers.end(), [] (const auto& a, const auto& b)
        {
            return NodeRegistry::identifierToDisplayName (a) < NodeRegistry::identifierToDisplayName (b);
        });

        for (const auto& identifier : identifiers)
        {
            if (identifier == NodeRegistry::subgraphIdentifier)
                continue;

            if (const auto name = NodeRegistry::identifierToDisplayName (identifier); matches (name))
                internals->addSubItem (std::make_unique<EntryItem> (*this, name, makeNodeKey (identifier)));
        }

        auto subgraphs = std::make_unique<FolderItem> ("Subgraphs");
        for (const auto preset : { SubgraphConfig::mono, SubgraphConfig::stereo, SubgraphConfig::monoMidi, SubgraphConfig::stereoMidi })
        {
            if (const auto name = SubgraphConfig::fromPreset (preset).getDisplayName(); matches (name))
                subgraphs->addSubItem (std::make_unique<EntryItem> (*this, name, makeSubgraphKey (preset)));
        }

        addFolderIfNotEmpty (*root, std::move (internals));
        addFolderIfNotEmpty (*root, std::move (subgraphs));

#if YUP_DESKTOP
        addPluginFolders (*root, matches);
#endif

        tree.setRootItem (std::move (root));

        if (isFiltering)
            tree.getRootItem()->setOpenRecursively (true);
        else if (savedOpenness.isValid())
            tree.restoreOpennessState (savedOpenness, false);
        else
            openTopLevelFolders();

        wasFiltering = isFiltering;
    }

    /** Called with the key of an item that was double-clicked or activated with return. */
    std::function<void (const yup::String& key)> onItemChosen;

#if YUP_DESKTOP
    /** Returns whether a plugin runs in a separate process. */
    std::function<bool (const yup::AudioPluginDescription&)> isPluginSandboxed;

    /** Changes whether a plugin runs in a separate process. */
    std::function<void (const yup::AudioPluginDescription&, bool)> setPluginSandboxed;
#endif

    //==============================================================================
    void resized() override
    {
        auto bounds = getLocalBounds().reduced (6.0f);
        filterEditor.setBounds (bounds.removeFromTop (28.0f));
        bounds.removeFromTop (6.0f);
        tree.setBounds (bounds);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::Color (0xff161b22));
        g.fillAll();
    }

private:
    //==============================================================================
    class FolderItem final : public yup::TreeViewItem
    {
    public:
        explicit FolderItem (yup::String nameToShow)
            : name (std::move (nameToShow))
        {
        }

        yup::String getItemText() const override { return name + "  (" + yup::String (countEntries (*this)) + ")"; }

        yup::String getUniqueName() const override { return name; }

        bool mightContainSubItems() const override { return true; }

    private:
        static int countEntries (const yup::TreeViewItem& item)
        {
            int count = 0;
            for (int i = 0; i < item.getNumSubItems(); ++i)
            {
                auto* subItem = item.getSubItem (i);
                count += dynamic_cast<const FolderItem*> (subItem) != nullptr ? countEntries (*subItem) : 1;
            }

            return count;
        }

        yup::String name;
    };

    class EntryItem final : public yup::TreeViewItem
    {
    public:
        EntryItem (NodeBrowser& ownerToUse, yup::String textToShow, yup::String keyToUse, int pluginIndexToUse = -1)
            : key (std::move (keyToUse))
            , owner (ownerToUse)
            , text (std::move (textToShow))
            , pluginIndex (pluginIndexToUse)
        {
        }

        yup::String getItemText() const override { return text; }

        yup::var getDragSourceDescription() const override { return key; }

        void itemDoubleClicked (const yup::MouseEvent&) override
        {
            owner.choose (key);
        }

        void itemClicked (const yup::MouseEvent& event) override
        {
#if YUP_DESKTOP
            if (event.isRightButtonDown() && pluginIndex >= 0)
                owner.showPluginMenu (pluginIndex, event.getScreenPosition());
#else
            yup::ignoreUnused (event);
#endif
        }

        const yup::String key;

    private:
        NodeBrowser& owner;
        yup::String text;
        int pluginIndex = -1;
    };

    //==============================================================================
    static void addFolderIfNotEmpty (yup::TreeViewItem& parent, std::unique_ptr<yup::TreeViewItem> folder)
    {
        if (folder->getNumSubItems() > 0)
            parent.addSubItem (std::move (folder));
    }

    void openTopLevelFolders()
    {
        auto* root = tree.getRootItem();
        for (int i = 0; i < root->getNumSubItems(); ++i)
            root->getSubItem (i)->setOpen (true);
    }

    void choose (const yup::String& key)
    {
        if (onItemChosen != nullptr)
            onItemChosen (key);
    }

#if YUP_DESKTOP
    template <class Matcher>
    void addPluginFolders (yup::TreeViewItem& root, Matcher&& matches)
    {
        // Discovered plugins are sorted by format, vendor and name
        const auto& plugins = registry.getDiscoveredPlugins();

        std::unique_ptr<yup::TreeViewItem> formatFolder, vendorFolder;
        yup::String formatName, vendorName;

        auto flushVendor = [&]
        {
            if (vendorFolder != nullptr)
                addFolderIfNotEmpty (*formatFolder, std::move (vendorFolder));
        };

        auto flushFormat = [&]
        {
            flushVendor();
            if (formatFolder != nullptr)
                addFolderIfNotEmpty (root, std::move (formatFolder));
        };

        for (int i = 0; i < static_cast<int> (plugins.size()); ++i)
        {
            const auto& desc = plugins[static_cast<size_t> (i)];
            const auto thisFormat = NodeRegistry::formatTypeToDisplayName (desc.formatType);
            const auto thisVendor = NodeRegistry::getVendorName (desc);

            if (formatFolder == nullptr || thisFormat != formatName)
            {
                flushFormat();
                formatFolder = std::make_unique<FolderItem> (thisFormat);
                formatName = thisFormat;
                vendorName = {};
            }

            if (vendorFolder == nullptr || thisVendor.compareIgnoreCase (vendorName) != 0)
            {
                flushVendor();
                vendorFolder = std::make_unique<FolderItem> (thisVendor);
                vendorName = thisVendor;
            }

            if (! matches (desc.name) && ! matches (thisVendor))
                continue;

            const bool sandboxed = isPluginSandboxed != nullptr && isPluginSandboxed (desc);
            vendorFolder->addSubItem (std::make_unique<EntryItem> (*this,
                                                                   sandboxed ? desc.name + "  [sandboxed]" : desc.name,
                                                                   makePluginKey (i),
                                                                   i));
        }

        flushFormat();
    }

    void showPluginMenu (int pluginIndex, yup::Point<float> screenPosition)
    {
        const auto& plugins = registry.getDiscoveredPlugins();
        if (pluginIndex >= static_cast<int> (plugins.size()))
            return;

        const auto desc = plugins[static_cast<size_t> (pluginIndex)];
        const bool sandboxed = isPluginSandboxed != nullptr && isPluginSandboxed (desc);

        pluginMenu = yup::PopupMenu::create (yup::PopupMenu::Options {}
                                                 .withFocusComponent (&tree)
                                                 .withPosition (screenPosition, yup::Justification::topLeft));
        pluginMenu->addItem ("Add to Graph", 1);
        pluginMenu->addItem ("Run Sandboxed", 2, setPluginSandboxed != nullptr, sandboxed);

        yup::WeakReference<yup::Component> weakThis (this);
        pluginMenu->show ([this, weakThis, desc, sandboxed] (int selectedID)
        {
            if (weakThis == nullptr)
                return;

            if (selectedID == 1)
            {
                const auto& current = registry.getDiscoveredPlugins();
                if (const auto it = std::find (current.begin(), current.end(), desc); it != current.end())
                    choose (makePluginKey (static_cast<int> (std::distance (current.begin(), it))));
            }
            else if (selectedID == 2)
                setPluginSandboxed (desc, ! sandboxed);
        });
    }

    yup::PopupMenu::Ptr pluginMenu;
#endif

    NodeRegistry& registry;
    yup::TextEditor filterEditor { "browserFilter" };
    yup::TreeView tree { "browserTree" };
    yup::DataTree savedOpenness;
    bool wasFiltering = false;
};
