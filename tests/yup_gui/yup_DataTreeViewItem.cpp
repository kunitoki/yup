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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

//==============================================================================
class DataTreeViewItemTests : public ::testing::Test
{
protected:
    /** Shows the "name" property, and creates items of its own type. */
    class NamedNodeItem : public DataTreeViewItem
    {
    public:
        using DataTreeViewItem::DataTreeViewItem;

        String getItemText() const override
        {
            return getDataTree().getProperty ("name").toString();
        }

        std::unique_ptr<TreeViewItem> createSubItem (const DataTree& child) override
        {
            return std::make_unique<NamedNodeItem> (child, getUndoManager());
        }
    };

    void SetUp() override
    {
        undoManager = UndoManager::Ptr (new UndoManager());

        document = DataTree ("Root");

        {
            auto transaction = document.beginTransaction();
            transaction.addChild (makeNode ("A", { "A0", "A1" }));
            transaction.addChild (makeNode ("B"));
            transaction.addChild (makeNode ("C", { "C0" }));
        }

        tree = std::make_unique<TreeView>();
        tree->setBounds (0.0f, 0.0f, 300.0f, 240.0f);
        tree->setVisible (true);
        tree->setExpandAnimationTime (0.0);
        tree->setRootItemVisible (false);
        tree->setRootItem (std::make_unique<NamedNodeItem> (document, undoManager));
    }

    static DataTree makeNode (const String& name, std::initializer_list<const char*> children = {})
    {
        DataTree node ("Node");

        {
            auto transaction = node.beginTransaction();
            transaction.setProperty ("name", name);

            for (const auto* child : children)
                transaction.addChild (makeNode (child));
        }

        return node;
    }

    DataTree nodeNamed (const String& name) const
    {
        return document.findDescendant ([&name] (const DataTree& node)
        {
            return node.getProperty ("name").toString() == name;
        });
    }

    TreeViewItem& itemNamed (const String& name) const
    {
        for (int row = 0; row < tree->getNumRowsInTree(); ++row)
        {
            if (tree->getItemOnRow (row)->getItemText() == name)
                return *tree->getItemOnRow (row);
        }

        jassertfalse;
        return *tree->getRootItem();
    }

    String rowNames() const
    {
        StringArray names;

        for (int row = 0; row < tree->getNumRowsInTree(); ++row)
            names.add (tree->getItemOnRow (row)->getItemText());

        return names.joinIntoString (",");
    }

    String childNamesOf (const DataTree& node) const
    {
        StringArray names;

        for (int index = 0; index < node.getNumChildren(); ++index)
            names.add (node.getChild (index).getProperty ("name").toString());

        return names.joinIntoString (",");
    }

    DragAndDropSourceDetails dragFromTreeAt (float y) const
    {
        DragAndDropSourceDetails details;
        details.localPosition = { 150.0f, y };
        details.sourceComponent = tree->getChildComponent (0);
        return details;
    }

    float rowY (int row, float fraction) const
    {
        return (static_cast<float> (row) + fraction) * tree->getDefaultItemHeight();
    }

    UndoManager::Ptr undoManager;
    DataTree document;
    std::unique_ptr<TreeView> tree;
};

//==============================================================================
TEST_F (DataTreeViewItemTests, SubItemsAreCreatedOnFirstOpen)
{
    EXPECT_EQ ("A,B,C", rowNames());

    auto& a = itemNamed ("A");
    EXPECT_TRUE (a.mightContainSubItems());
    EXPECT_EQ (0, a.getNumSubItems());
    EXPECT_FALSE (itemNamed ("B").mightContainSubItems());

    a.setOpen (true);

    EXPECT_EQ ("A,A0,A1,B,C", rowNames());
}

TEST_F (DataTreeViewItemTests, DefaultTextIsTheNodeType)
{
    DataTreeViewItem item (DataTree ("Thing"));

    EXPECT_EQ ("Thing", item.getItemText());
    EXPECT_EQ (var ("Thing"), item.getDragSourceDescription());
    EXPECT_EQ (nullptr, item.getUndoManager().get());
}

TEST_F (DataTreeViewItemTests, AddedChildrenAreMirrored)
{
    {
        auto transaction = document.beginTransaction();
        transaction.addChild (makeNode ("D"), 1);
    }

    EXPECT_EQ ("A,D,B,C", rowNames());

    // Not opened yet: nothing to mirror, the node gains the child all the same.
    {
        auto transaction = nodeNamed ("A").beginTransaction();
        transaction.addChild (makeNode ("A2"));
    }

    EXPECT_EQ (0, itemNamed ("A").getNumSubItems());

    itemNamed ("A").setOpen (true);
    EXPECT_EQ ("A,A0,A1,A2,D,B,C", rowNames());
}

TEST_F (DataTreeViewItemTests, RemovedChildrenAreMirrored)
{
    {
        auto transaction = document.beginTransaction();
        transaction.removeChild (nodeNamed ("B"));
    }

    EXPECT_EQ ("A,C", rowNames());
}

TEST_F (DataTreeViewItemTests, MovedChildrenAreMirrored)
{
    itemNamed ("A").setOpen (true);

    {
        auto transaction = document.beginTransaction();
        transaction.moveChild (0, 2);
    }

    EXPECT_EQ ("B,C,A,A0,A1", rowNames());
}

TEST_F (DataTreeViewItemTests, PropertyChangesRefreshTheRow)
{
    {
        auto transaction = nodeNamed ("B").beginTransaction();
        transaction.setProperty ("name", "Bee");
    }

    auto* list = dynamic_cast<ListBox*> (tree->getChildComponent (0));
    auto* row = dynamic_cast<TreeViewRow*> (list->getComponentForRow (1));

    ASSERT_NE (nullptr, row);
    EXPECT_EQ ("Bee", row->getItemText());
}

TEST_F (DataTreeViewItemTests, FirstChildGivesADisclosureButton)
{
    auto* list = dynamic_cast<ListBox*> (tree->getChildComponent (0));
    auto* row = dynamic_cast<TreeViewRow*> (list->getComponentForRow (1));
    ASSERT_NE (nullptr, row);
    EXPECT_TRUE (row->getDisclosureBounds().isEmpty());

    {
        auto transaction = nodeNamed ("B").beginTransaction();
        transaction.addChild (makeNode ("B0"));
    }

    row = dynamic_cast<TreeViewRow*> (list->getComponentForRow (1));
    ASSERT_NE (nullptr, row);
    EXPECT_FALSE (row->getDisclosureBounds().isEmpty());
}

TEST_F (DataTreeViewItemTests, UndoAndRedoAreMirrored)
{
    {
        auto transaction = document.beginTransaction (undoManager.get());
        transaction.removeChild (nodeNamed ("A"));
    }

    EXPECT_EQ ("B,C", rowNames());

    undoManager->undo();
    EXPECT_EQ ("A,B,C", rowNames());

    undoManager->redo();
    EXPECT_EQ ("B,C", rowNames());
}

TEST_F (DataTreeViewItemTests, DraggingWithinAParentIsOneMove)
{
    itemNamed ("A").setSelected (true);

    // Below "C", which is closed: after it.
    EXPECT_TRUE (tree->itemDropped (dragFromTreeAt (rowY (2, 0.9f))));

    EXPECT_EQ ("B,C,A", childNamesOf (document));
    EXPECT_EQ ("B,C,A", rowNames());
    EXPECT_TRUE (itemNamed ("A").isSelected());

    undoManager->undo();

    EXPECT_EQ ("A,B,C", childNamesOf (document));
    EXPECT_EQ ("A,B,C", rowNames());
    EXPECT_FALSE (undoManager->canUndo());
}

TEST_F (DataTreeViewItemTests, DraggingAcrossParentsUndoesAsOneStep)
{
    itemNamed ("A").setOpen (true);
    itemNamed ("A0").setSelected (true);

    // Rows: A, A0, A1, B, C - into the middle of "C", which appends.
    EXPECT_TRUE (tree->itemDropped (dragFromTreeAt (rowY (4, 0.5f))));

    EXPECT_EQ ("A1", childNamesOf (nodeNamed ("A")));
    EXPECT_EQ ("C0,A0", childNamesOf (nodeNamed ("C")));
    EXPECT_EQ ("A,A1,B,C", rowNames());

    undoManager->undo();

    EXPECT_EQ ("A0,A1", childNamesOf (nodeNamed ("A")));
    EXPECT_EQ ("C0", childNamesOf (nodeNamed ("C")));
    EXPECT_EQ ("A,A0,A1,B,C", rowNames());
    EXPECT_FALSE (undoManager->canUndo());
}

TEST_F (DataTreeViewItemTests, SeveralNodesMoveInOrder)
{
    tree->setSelectionMode (ListBox::SelectionMode::multiple);
    itemNamed ("A").setSelected (true);
    itemNamed ("B").setSelected (true, false);

    EXPECT_TRUE (tree->itemDropped (dragFromTreeAt (rowY (2, 0.9f))));

    EXPECT_EQ ("C,A,B", childNamesOf (document));
    EXPECT_EQ (2, tree->getNumSelectedItems());
    EXPECT_TRUE (itemNamed ("A").isSelected());
    EXPECT_TRUE (itemNamed ("B").isSelected());
}

TEST_F (DataTreeViewItemTests, NodesCannotBeDroppedIntoThemselves)
{
    itemNamed ("A").setOpen (true);
    itemNamed ("A").setSelected (true);

    const auto details = dragFromTreeAt (0.0f);

    EXPECT_FALSE (itemNamed ("A").isInterestedInDragSource (details));
    EXPECT_FALSE (itemNamed ("A0").isInterestedInDragSource (details));
    EXPECT_TRUE (itemNamed ("B").isInterestedInDragSource (details));

    // Into "A".
    EXPECT_FALSE (tree->itemDropped (dragFromTreeAt (rowY (0, 0.5f))));
    EXPECT_EQ ("A,B,C", childNamesOf (document));
}

TEST_F (DataTreeViewItemTests, DropsFromElsewhereAreIgnored)
{
    DragAndDropSourceDetails details;
    details.localPosition = { 150.0f, rowY (1, 0.1f) };

    EXPECT_FALSE (itemNamed ("B").isInterestedInDragSource (details));
    EXPECT_FALSE (tree->itemDropped (details));
}

TEST_F (DataTreeViewItemTests, DeletedItemsStopListening)
{
    tree->setRootItem (nullptr);

    {
        auto transaction = document.beginTransaction();
        transaction.addChild (makeNode ("D"));
        transaction.setProperty ("name", "Changed");
    }

    EXPECT_EQ (0, tree->getNumRowsInTree());
    EXPECT_EQ (4, document.getNumChildren());
}
