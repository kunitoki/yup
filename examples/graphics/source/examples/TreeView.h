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

//==============================================================================
/** Three trees side by side.

    - Files: the home folder, loaded lazily one folder at a time as it opens.
    - Tasks: plain items that can be reordered and regrouped by drag and drop, with a backlog of
      thousands of tasks to scroll through.
    - DataTree: a DataTree document mirrored by DataTreeViewItems. Drag and drop moves the nodes,
      and the buttons add, delete, undo and redo.

    Open and close with the disclosure buttons, a double-click, Left and Right, or Alt for a whole
    branch.
*/
class TreeViewDemo : public yup::Component
{
public:
    TreeViewDemo()
        : Component ("TreeViewDemo")
    {
        setupTree (fileTree, fileTitle, "Files");
        fileTree.setRootItem (std::make_unique<FileItem> (yup::File::getSpecialLocation (yup::File::userHomeDirectory)));
        fileTree.getRootItem()->setOpen (true);

        setupTree (taskTree, taskTitle, "Tasks");
        taskTree.setRootItemVisible (false);
        taskTree.setRootItem (createTasks());

        setupTree (dataTree, dataTitle, "DataTree");
        dataTree.setRootItemVisible (false);
        dataTree.setRootItem (std::make_unique<NodeItem> (createDocument(), undoManager));
        dataTree.onDeleteKeyPressed = [this] (std::vector<yup::TreeViewItem*>)
        {
            deleteSelectedNodes();
        };

        for (auto [button, text] : { std::pair { &addButton, "Add" },
                                     std::pair { &deleteButton, "Delete" },
                                     std::pair { &undoButton, "Undo" },
                                     std::pair { &redoButton, "Redo" } })
        {
            button->setButtonText (text);
            addAndMakeVisible (*button);
        }

        addButton.onClick = [this] { addNode(); };
        deleteButton.onClick = [this] { deleteSelectedNodes(); };
        undoButton.onClick = [this] { undoManager->undo(); };
        redoButton.onClick = [this] { undoManager->redo(); };

        addAndMakeVisible (status);
        status.setText ("Drag tasks and nodes to reorder them", yup::dontSendNotification);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (4.0f);

        status.setBounds (bounds.removeFromBottom (24.0f));
        bounds.removeFromBottom (4.0f);

        const auto panelWidth = bounds.getWidth() / 3.0f;

        for (auto [title, tree] : { std::pair { &fileTitle, &fileTree }, std::pair { &taskTitle, &taskTree }, std::pair { &dataTitle, &dataTree } })
        {
            auto panel = bounds.removeFromLeft (panelWidth).reduced (4.0f);
            title->setBounds (panel.removeFromTop (24.0f));

            if (tree == &dataTree)
            {
                auto buttons = panel.removeFromBottom (32.0f);
                const auto buttonWidth = buttons.getWidth() / 4.0f;

                for (auto* button : { &addButton, &deleteButton, &undoButton, &redoButton })
                    button->setBounds (buttons.removeFromLeft (buttonWidth).reduced (2.0f));

                panel.removeFromBottom (4.0f);
            }

            tree->setBounds (panel);
        }
    }

private:
    //==============================================================================
    /** A file or folder; a folder lists its content the first time it opens. */
    class FileItem : public yup::TreeViewItem
    {
    public:
        explicit FileItem (yup::File file)
            : file (std::move (file))
        {
        }

        yup::String getItemText() const override
        {
            const auto name = file.getFileName();
            return name.isNotEmpty() ? name : file.getFullPathName();
        }

        yup::String getUniqueName() const override
        {
            return file.getFullPathName();
        }

        bool mightContainSubItems() const override
        {
            return file.isDirectory();
        }

        yup::var getDragSourceDescription() const override
        {
            return file.getFullPathName();
        }

        void itemOpennessChanged (bool isNowOpen) override
        {
            if (! isNowOpen || getNumSubItems() > 0)
                return;

            auto children = file.findChildFiles (yup::File::findFilesAndDirectories | yup::File::ignoreHiddenFiles, false);

            std::sort (children.begin(), children.end(), [] (const yup::File& a, const yup::File& b)
            {
                if (a.isDirectory() != b.isDirectory())
                    return a.isDirectory();

                return a.getFileName().compareIgnoreCase (b.getFileName()) < 0;
            });

            for (const auto& child : children)
                addSubItem (std::make_unique<FileItem> (child));
        }

    private:
        yup::File file;
    };

    //==============================================================================
    /** A task or a group of tasks. Tasks dragged from this tree can be dropped among the sub-items of any item. */
    class TaskItem : public yup::TreeViewItem
    {
    public:
        explicit TaskItem (yup::String name, bool isGroup = false)
            : name (std::move (name))
            , group (isGroup)
        {
        }

        yup::String getItemText() const override
        {
            return name;
        }

        bool mightContainSubItems() const override
        {
            return group || getNumSubItems() > 0;
        }

        yup::var getDragSourceDescription() const override
        {
            return name;
        }

        bool isInterestedInDragSource (const yup::DragAndDropSourceDetails& details) const override
        {
            const auto dragged = yup::TreeView::getDraggedItems (details);

            return ! dragged.empty() && std::all_of (dragged.begin(), dragged.end(), [] (yup::TreeViewItem* item)
            {
                return dynamic_cast<TaskItem*> (item) != nullptr;
            });
        }

        void itemDropped (const yup::DragAndDropSourceDetails& details, int insertIndex) override
        {
            const auto dragged = yup::TreeView::getDraggedItems (details);

            for (auto* item : dragged)
            {
                auto* oldParent = item->getParentItem();
                const auto oldIndex = item->getIndexInParent();

                if (oldParent == this)
                {
                    // The insertion index counts the item itself when it sits before it.
                    const auto newIndex = yup::jmin (getNumSubItems() - 1, insertIndex > oldIndex ? insertIndex - 1 : insertIndex);
                    moveSubItem (oldIndex, newIndex);
                    insertIndex = newIndex + 1;
                }
                else
                {
                    addSubItem (oldParent->removeSubItem (oldIndex), insertIndex++);
                }
            }

            for (size_t index = 0; index < dragged.size(); ++index)
                dragged[index]->setSelected (true, index == 0);
        }

    private:
        yup::String name;
        bool group = false;
    };

    //==============================================================================
    /** Shows the "name" property of a node, and creates the same kind of item for its children. */
    class NodeItem : public yup::DataTreeViewItem
    {
    public:
        using DataTreeViewItem::DataTreeViewItem;

        yup::String getItemText() const override
        {
            return getDataTree().getProperty ("name").toString();
        }

        std::unique_ptr<yup::TreeViewItem> createSubItem (const yup::DataTree& child) override
        {
            return std::make_unique<NodeItem> (child, getUndoManager());
        }
    };

    //==============================================================================
    void setupTree (yup::TreeView& tree, yup::Label& title, const yup::String& name)
    {
        title.setText (name, yup::dontSendNotification);
        addAndMakeVisible (title);

        tree.setSelectionMode (yup::ListBox::SelectionMode::multiple);
        tree.onSelectionChanged = [this, &tree, name]
        {
            status.setText (name + ": " + yup::String (tree.getNumSelectedItems()) + " selected", yup::dontSendNotification);
        };
        addAndMakeVisible (tree);
    }

    static std::unique_ptr<yup::TreeViewItem> createTasks()
    {
        auto root = std::make_unique<TaskItem> ("Tasks", true);

        const auto addGroup = [&root] (const char* name, std::initializer_list<const char*> tasks) -> yup::TreeViewItem&
        {
            auto& group = root->addSubItem (std::make_unique<TaskItem> (name, true));

            for (const auto* task : tasks)
                group.addSubItem (std::make_unique<TaskItem> (task));

            group.setOpen (true);
            return group;
        };

        addGroup ("Today", { "Write the release notes", "Review the tree view", "Fix the flaky test" });
        addGroup ("This week", { "Plan the sprint", "Update the dependencies" });
        addGroup ("Someday", {});

        auto& backlog = addGroup ("Backlog", {});
        backlog.setOpen (false);

        for (int index = 1; index <= 5000; ++index)
            backlog.addSubItem (std::make_unique<TaskItem> ("Backlog task " + yup::String (index)));

        return root;
    }

    static yup::DataTree createNode (const yup::String& name, std::initializer_list<yup::DataTree> children = {})
    {
        yup::DataTree node ("Node");

        {
            auto transaction = node.beginTransaction();
            transaction.setProperty ("name", name);

            for (const auto& child : children)
                transaction.addChild (child);
        }

        return node;
    }

    static yup::DataTree createDocument()
    {
        return createNode ("Project", {
                                          createNode ("Sources", { createNode ("main.cpp"), createNode ("app.cpp"), createNode ("app.h") }),
                                          createNode ("Resources", { createNode ("icon.svg"), createNode ("strings.json") }),
                                          createNode ("README.md"),
                                      });
    }

    void addNode()
    {
        const auto selected = dataTree.getSelectedItems();
        auto* selectedNode = selected.empty() ? nullptr : dynamic_cast<NodeItem*> (selected.front());
        auto parent = selectedNode != nullptr ? selectedNode->getDataTree() : dynamic_cast<NodeItem*> (dataTree.getRootItem())->getDataTree();

        yup::UndoManager::ScopedTransaction undoTransaction (*undoManager, "Add node");
        auto transaction = parent.beginTransaction (undoManager.get());
        transaction.addChild (createNode ("Node " + yup::String (++nodeCounter)));
    }

    void deleteSelectedNodes()
    {
        std::vector<yup::DataTree> nodes;

        for (auto* item : dataTree.getSelectedItems())
        {
            if (auto* nodeItem = dynamic_cast<NodeItem*> (item))
                nodes.push_back (nodeItem->getDataTree());
        }

        yup::UndoManager::ScopedTransaction undoTransaction (*undoManager, "Delete nodes");

        for (const auto& node : nodes)
        {
            auto parent = node.getParent();
            auto transaction = parent.beginTransaction (undoManager.get());
            transaction.removeChild (node);
        }
    }

    //==============================================================================
    yup::UndoManager::Ptr undoManager { new yup::UndoManager() };
    int nodeCounter = 0;

    yup::Label fileTitle, taskTitle, dataTitle, status;
    yup::TreeView fileTree, taskTree, dataTree;
    yup::TextButton addButton, deleteButton, undoButton, redoButton;
};
