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
/** Four trees side by side.

    - Files: the home folder, loaded lazily one folder at a time as it opens, with file and folder
      icons drawn by the items themselves (paintItemIcon).
    - Tasks: plain items that can be reordered and regrouped by drag and drop, with a backlog of
      thousands of tasks to scroll through.
    - DataTree: a DataTree document mirrored by DataTreeViewItems. Drag and drop moves the nodes,
      and the buttons add, delete, undo and redo.
    - Widgets: custom row components (refreshItemComponent) with a type icon on the left and
      buttons on the right that add and remove widgets.

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

        setupTree (widgetTree, widgetTitle, "Widgets");
        widgetTree.setRootItemVisible (false);
        widgetTree.setDefaultItemHeight (30.0f);
        widgetTree.setRootItem (createWidgets());

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

        const auto panelWidth = bounds.getWidth() / 4.0f;

        for (auto [title, tree] : { std::pair { &fileTitle, &fileTree },
                                    std::pair { &taskTitle, &taskTree },
                                    std::pair { &dataTitle, &dataTree },
                                    std::pair { &widgetTitle, &widgetTree } })
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
            , directory (this->file.isDirectory())
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
            return directory;
        }

        bool hasItemIcon() const override
        {
            return true;
        }

        void paintItemIcon (yup::Graphics& g, yup::Rectangle<float> area, bool isSelected) const override
        {
            const auto r = area.reduced (area.getWidth() * 0.1f);
            yup::Path shape;

            if (directory)
            {
                // A tab over a body.
                shape.addRoundedRectangle (r.getX(), r.getY() + r.getHeight() * 0.1f, r.getWidth() * 0.45f, r.getHeight() * 0.3f, 1.5f);
                shape.addRoundedRectangle (r.getX(), r.getY() + r.getHeight() * 0.22f, r.getWidth(), r.getHeight() * 0.68f, 2.0f);

                g.setFillColor (isSelected ? yup::Colors::white : yup::Color (0xffe8b04a));
                g.fillPath (shape);
                return;
            }

            // A page with a folded corner.
            const auto width = r.getHeight() * 0.75f;
            const auto left = r.getCenterX() - width * 0.5f;
            const auto fold = width * 0.35f;

            shape.moveTo (left, r.getY());
            shape.lineTo (left + width - fold, r.getY());
            shape.lineTo (left + width, r.getY() + fold);
            shape.lineTo (left + width, r.getBottom());
            shape.lineTo (left, r.getBottom());
            shape.close();

            g.setFillColor (isSelected ? yup::Colors::white : yup::Color (0xff8aa4c8));
            g.fillPath (shape);
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
        bool directory = false;
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
    enum class WidgetKind
    {
        scaffold,
        column,
        row,
        container,
        text,
        image,
        button
    };

    /** Draws the outline glyph for a kind of widget. */
    static void paintWidgetIcon (yup::Graphics& g, WidgetKind kind, yup::Rectangle<float> area, yup::Color color)
    {
        const auto r = area.reduced (area.getWidth() * 0.15f);
        yup::Path path;

        switch (kind)
        {
            case WidgetKind::scaffold:
                path.addRoundedRectangle (r.reduced (r.getWidth() * 0.18f, 0.0f), 2.0f);
                break;

            case WidgetKind::column:
                for (int index = 0; index < 3; ++index)
                    path.addRoundedRectangle (r.getX(), r.getY() + r.getHeight() * (0.05f + index * 0.33f), r.getWidth(), r.getHeight() * 0.24f, 1.0f);
                break;

            case WidgetKind::row:
                for (int index = 0; index < 3; ++index)
                    path.addRoundedRectangle (r.getX() + r.getWidth() * (0.05f + index * 0.33f), r.getY(), r.getWidth() * 0.24f, r.getHeight(), 1.0f);
                break;

            case WidgetKind::container:
            case WidgetKind::button:
                path.addRoundedRectangle (kind == WidgetKind::button ? r.reduced (0.0f, r.getHeight() * 0.2f) : r, 2.0f);
                break;

            case WidgetKind::image:
                path.addRoundedRectangle (r, 2.0f);
                path.moveTo (r.getX() + r.getWidth() * 0.15f, r.getBottom() - r.getHeight() * 0.2f);
                path.lineTo (r.getX() + r.getWidth() * 0.45f, r.getY() + r.getHeight() * 0.45f);
                path.lineTo (r.getRight() - r.getWidth() * 0.15f, r.getBottom() - r.getHeight() * 0.2f);
                break;

            case WidgetKind::text:
                path.moveTo (r.getX() + r.getWidth() * 0.1f, r.getY() + r.getHeight() * 0.1f);
                path.lineTo (r.getRight() - r.getWidth() * 0.1f, r.getY() + r.getHeight() * 0.1f);
                path.moveTo (r.getCenterX(), r.getY() + r.getHeight() * 0.1f);
                path.lineTo (r.getCenterX(), r.getBottom());
                break;
        }

        g.setStrokeColor (color);
        g.setStrokeWidth (1.5f);
        g.setStrokeCap (yup::StrokeCap::Round);
        g.setStrokeJoin (yup::StrokeJoin::Round);
        g.strokePath (path);
    }

    /** A small borderless button drawing a plus or a cross, highlighted under the mouse. */
    class IconButton final : public yup::Button
    {
    public:
        explicit IconButton (bool isPlus)
            : Button ("IconButton")
            , plus (isPlus)
        {
        }

        void paintButton (yup::Graphics& g) override
        {
            const auto bounds = getLocalBounds().reduced (3.0f);

            if (isButtonOver() || isButtonDown())
            {
                g.setFillColor (yup::Color (isButtonDown() ? 0x40000000 : 0x20000000));
                g.fillRoundedRect (bounds, 4.0f);
            }

            const auto glyph = bounds.reduced (bounds.getWidth() * 0.3f);
            yup::Path path;

            if (plus)
            {
                path.moveTo (glyph.getCenterX(), glyph.getY());
                path.lineTo (glyph.getCenterX(), glyph.getBottom());
                path.moveTo (glyph.getX(), glyph.getCenterY());
                path.lineTo (glyph.getRight(), glyph.getCenterY());
            }
            else
            {
                path.moveTo (glyph.getX(), glyph.getY());
                path.lineTo (glyph.getRight(), glyph.getBottom());
                path.moveTo (glyph.getRight(), glyph.getY());
                path.lineTo (glyph.getX(), glyph.getBottom());
            }

            g.setStrokeColor (color);
            g.setStrokeWidth (1.5f);
            g.setStrokeCap (yup::StrokeCap::Round);
            g.strokePath (path);
        }

        yup::Color color;

    private:
        bool plus = false;
    };

    class WidgetItem;

    /** The custom content of a widget row: type icon and name on the left, actions on the right. */
    class WidgetRowContent final : public yup::Component
    {
    public:
        WidgetRowContent()
        {
            // Presses on the empty areas still select the row; the buttons take their own.
            setWantsMouseEvents (false, true);

            addButton.onClick = [this] { addChild(); };
            removeButton.onClick = [this] { removeItem(); };

            addAndMakeVisible (addButton);
            addAndMakeVisible (removeButton);
        }

        void setup (WidgetItem& newItem);

        void resized() override
        {
            auto bounds = getLocalBounds().reduced (0.0f, 2.0f);
            const auto buttonSize = bounds.getHeight();

            removeButton.setBounds (bounds.removeFromRight (buttonSize));
            addButton.setBounds (bounds.removeFromRight (buttonSize));
        }

        void paint (yup::Graphics& g) override;

    private:
        void addChild();
        void removeItem();

        WidgetItem* item = nullptr;
        IconButton addButton { true };
        IconButton removeButton { false };
    };

    /** A widget of a UI hierarchy, shown by a WidgetRowContent. */
    class WidgetItem : public yup::TreeViewItem
    {
    public:
        WidgetItem (yup::String name, WidgetKind kind)
            : name (std::move (name))
            , kind (kind)
        {
        }

        yup::String getItemText() const override { return name; }

        bool canHaveChildren() const noexcept
        {
            return kind != WidgetKind::text && kind != WidgetKind::image && kind != WidgetKind::button;
        }

        bool mightContainSubItems() const override { return getNumSubItems() > 0; }

        void refreshItemComponent (std::unique_ptr<yup::Component>& component) override
        {
            reuseOrCreate<WidgetRowContent> (component).setup (*this);
        }

        WidgetItem& add (const yup::String& childName, WidgetKind childKind)
        {
            auto& child = static_cast<WidgetItem&> (addSubItem (std::make_unique<WidgetItem> (childName, childKind)));
            setOpen (true);
            return child;
        }

        yup::String name;
        WidgetKind kind;
    };

    static std::unique_ptr<yup::TreeViewItem> createWidgets()
    {
        auto root = std::make_unique<WidgetItem> ("Root", WidgetKind::container);

        auto& scaffold = root->add ("Scaffold", WidgetKind::scaffold);
        auto& page = scaffold.add ("Column", WidgetKind::column);

        auto& sideNav = page.add ("SideNavTablet", WidgetKind::container);
        sideNav.add ("Logo", WidgetKind::image);
        auto& navColumn = sideNav.add ("Column", WidgetKind::column);
        navColumn.add ("Nav Item", WidgetKind::row).add ("Text", WidgetKind::text);
        navColumn.add ("Nav Item", WidgetKind::row).add ("Text", WidgetKind::text);

        auto& content = page.add ("Column", WidgetKind::column);
        content.add ("Location Map", WidgetKind::image);
        auto& details = content.add ("Row", WidgetKind::row);
        details.add ("Text", WidgetKind::text);
        details.add ("Text", WidgetKind::text);

        auto& card = page.add ("Container", WidgetKind::container);
        card.add ("CircleImage", WidgetKind::image);
        auto& header = card.add ("Row", WidgetKind::row);
        header.add ("ComponentName", WidgetKind::button);
        header.add ("SubHeader", WidgetKind::text);

        return root;
    }

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

    yup::Label fileTitle, taskTitle, dataTitle, widgetTitle, status;
    yup::TreeView fileTree, taskTree, dataTree, widgetTree;
    yup::TextButton addButton, deleteButton, undoButton, redoButton;
};

//==============================================================================
inline void TreeViewDemo::WidgetRowContent::setup (WidgetItem& newItem)
{
    item = &newItem;
    addButton.setVisible (item->canHaveChildren());
    repaint();
}

inline void TreeViewDemo::WidgetRowContent::paint (yup::Graphics& g)
{
    if (item == nullptr)
        return;

    const auto selected = item->isSelected();
    const auto textColor = yup::ApplicationTheme::findComponentColor (*this, selected ? yup::TreeView::Style::itemTextSelectedColorId
                                                                                      : yup::TreeView::Style::itemTextColorId)
                               .value_or (selected ? yup::Colors::white : yup::Colors::black);

    addButton.color = textColor;
    removeButton.color = textColor;

    auto bounds = getLocalBounds();
    const auto iconArea = bounds.removeFromLeft (bounds.getHeight()).reduced (4.0f);

    paintWidgetIcon (g, item->kind, iconArea, item->kind == WidgetKind::button ? yup::Color (0xff8f6bff) : textColor);

    const auto textArea = bounds.withTrimmedLeft (4.0f).withTrimmedRight (bounds.getHeight() * 2.0f + 4.0f);

    g.setFillColor (item->kind == WidgetKind::button ? yup::Color (0xff8f6bff) : textColor);
    g.fillFittedText (item->name,
                      yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (14.0f),
                      textArea,
                      yup::Justification::centerLeft);
}

inline void TreeViewDemo::WidgetRowContent::addChild()
{
    if (item != nullptr)
        item->add ("Text", WidgetKind::text);
}

inline void TreeViewDemo::WidgetRowContent::removeItem()
{
    if (item == nullptr)
        return;

    // The row is recycled as its item goes, so forget the item before it is deleted.
    auto* parent = item->getParentItem();
    const auto index = item->getIndexInParent();
    item = nullptr;

    if (parent != nullptr)
        parent->removeSubItem (index);
}

