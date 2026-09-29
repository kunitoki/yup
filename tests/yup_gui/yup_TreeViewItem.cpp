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
class TreeViewItemTests : public ::testing::Test
{
protected:
    class NamedItem : public TreeViewItem
    {
    public:
        explicit NamedItem (String name, bool isContainer = false)
            : name (std::move (name))
            , container (isContainer)
        {
        }

        String getItemText() const override { return name; }

        bool mightContainSubItems() const override { return container || TreeViewItem::mightContainSubItems(); }

        void itemOpennessChanged (bool isNowOpen) override
        {
            opennessChanges.push_back (isNowOpen);

            if (isNowOpen && lazyChildren > 0 && getNumSubItems() == 0)
            {
                for (int index = 0; index < lazyChildren; ++index)
                    addSubItem (std::make_unique<NamedItem> (name + "." + String (index), true));
            }
        }

        String name;
        bool container = false;
        int lazyChildren = 0;
        std::vector<bool> opennessChanges;
    };

    static NamedItem& add (TreeViewItem& parent, const String& name)
    {
        return static_cast<NamedItem&> (parent.addSubItem (std::make_unique<NamedItem> (name)));
    }

    static String namesOf (const TreeViewItem& parent)
    {
        StringArray names;

        for (int index = 0; index < parent.getNumSubItems(); ++index)
            names.add (parent.getSubItem (index)->getItemText());

        return names.joinIntoString (",");
    }

    NamedItem root { "root" };
};

//==============================================================================
TEST_F (TreeViewItemTests, StartsEmptyClosedAndDetached)
{
    EXPECT_EQ (0, root.getNumSubItems());
    EXPECT_FALSE (root.isOpen());
    EXPECT_EQ (nullptr, root.getParentItem());
    EXPECT_EQ (nullptr, root.getOwnerView());
    EXPECT_EQ (-1, root.getIndexInParent());
    EXPECT_EQ (0, root.getDepth());
    EXPECT_FALSE (root.mightContainSubItems());
}

TEST_F (TreeViewItemTests, AddSubItemAppendsAndSetsParent)
{
    auto& a = add (root, "a");
    auto& b = add (root, "b");

    EXPECT_EQ ("a,b", namesOf (root));
    EXPECT_EQ (&root, a.getParentItem());
    EXPECT_EQ (1, b.getIndexInParent());
    EXPECT_EQ (1, b.getDepth());
    EXPECT_TRUE (root.mightContainSubItems());
}

TEST_F (TreeViewItemTests, AddSubItemAtIndexInserts)
{
    add (root, "a");
    add (root, "c");
    root.addSubItem (std::make_unique<NamedItem> ("b"), 1);
    root.addSubItem (std::make_unique<NamedItem> ("z"), 42);

    EXPECT_EQ ("a,b,c,z", namesOf (root));
}

TEST_F (TreeViewItemTests, RemoveSubItemHandsOwnershipBack)
{
    add (root, "a");
    auto& b = add (root, "b");
    add (b, "b0");

    auto removed = root.removeSubItem (1);

    ASSERT_NE (nullptr, removed);
    EXPECT_EQ (&b, removed.get());
    EXPECT_EQ (nullptr, removed->getParentItem());
    EXPECT_EQ (1, removed->getNumSubItems());
    EXPECT_EQ ("a", namesOf (root));
}

TEST_F (TreeViewItemTests, RemoveSubItemOutOfRangeReturnsNull)
{
    add (root, "a");

    EXPECT_EQ (nullptr, root.removeSubItem (-1));
    EXPECT_EQ (nullptr, root.removeSubItem (1));
    EXPECT_EQ (1, root.getNumSubItems());
}

TEST_F (TreeViewItemTests, MoveSubItemPutsItAtTheFinalIndex)
{
    add (root, "a");
    add (root, "b");
    add (root, "c");

    root.moveSubItem (0, 2);
    EXPECT_EQ ("b,c,a", namesOf (root));

    root.moveSubItem (2, 0);
    EXPECT_EQ ("a,b,c", namesOf (root));

    root.moveSubItem (0, 3);
    EXPECT_EQ ("a,b,c", namesOf (root));
}

TEST_F (TreeViewItemTests, ClearSubItemsDeletesThem)
{
    add (root, "a");
    add (root, "b");

    root.clearSubItems();

    EXPECT_EQ (0, root.getNumSubItems());
    EXPECT_EQ (nullptr, root.getSubItem (0));
}

TEST_F (TreeViewItemTests, DescendantCheckWalksParents)
{
    auto& a = add (root, "a");
    auto& a0 = add (a, "a0");
    auto& b = add (root, "b");

    EXPECT_TRUE (a0.isEqualToOrDescendantOf (root));
    EXPECT_TRUE (a0.isEqualToOrDescendantOf (a));
    EXPECT_TRUE (a0.isEqualToOrDescendantOf (a0));
    EXPECT_FALSE (a0.isEqualToOrDescendantOf (b));
    EXPECT_FALSE (a.isEqualToOrDescendantOf (a0));
    EXPECT_EQ (2, a0.getDepth());
}

TEST_F (TreeViewItemTests, SetOpenNotifiesOnlyOnChange)
{
    root.setOpen (true);
    root.setOpen (true);
    root.setOpen (false);

    EXPECT_FALSE (root.isOpen());
    EXPECT_EQ ((std::vector<bool> { true, false }), root.opennessChanges);
}

TEST_F (TreeViewItemTests, OpeningCreatesLazySubItems)
{
    root.container = true;
    root.lazyChildren = 3;

    EXPECT_TRUE (root.mightContainSubItems());
    EXPECT_EQ (0, root.getNumSubItems());

    root.setOpen (true);

    EXPECT_EQ ("root.0,root.1,root.2", namesOf (root));
}

TEST_F (TreeViewItemTests, SetOpenRecursivelyOpensContainersAndLazyDescendants)
{
    root.container = true;
    root.lazyChildren = 2;

    root.setOpenRecursively (true);

    ASSERT_EQ (2, root.getNumSubItems());
    auto* child = static_cast<NamedItem*> (root.getSubItem (0));
    EXPECT_TRUE (root.isOpen());
    EXPECT_TRUE (child->isOpen());

    auto& leaf = add (*child, "leaf");
    root.setOpenRecursively (false);

    EXPECT_FALSE (root.isOpen());
    EXPECT_FALSE (child->isOpen());
    EXPECT_FALSE (leaf.isOpen());
}

TEST_F (TreeViewItemTests, SetOpenRecursivelySkipsLeaves)
{
    auto& leaf = add (root, "leaf");

    root.setOpenRecursively (true);

    EXPECT_TRUE (root.isOpen());
    EXPECT_FALSE (leaf.isOpen());
}

TEST_F (TreeViewItemTests, DoubleClickTogglesContainers)
{
    auto& leaf = add (root, "leaf");
    const MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float>());

    root.itemDoubleClicked (event);
    EXPECT_TRUE (root.isOpen());

    root.itemDoubleClicked (event);
    EXPECT_FALSE (root.isOpen());

    leaf.itemDoubleClicked (event);
    EXPECT_FALSE (leaf.isOpen());
}

TEST_F (TreeViewItemTests, DetachedItemCannotBeSelected)
{
    root.setSelected (true);

    EXPECT_FALSE (root.isSelected());
}

TEST_F (TreeViewItemTests, UniqueNameDefaultsToText)
{
    EXPECT_EQ ("root", root.getUniqueName());
}

TEST_F (TreeViewItemTests, DefaultsForViewHooks)
{
    DragAndDropSourceDetails details;

    EXPECT_TRUE (root.getDragSourceDescription().isVoid());
    EXPECT_FALSE (root.isInterestedInDragSource (details));
    EXPECT_FALSE (root.getItemIcon().isValid());
    EXPECT_LE (root.getItemHeight(), 0.0f);

    auto component = std::unique_ptr<Component> (new Component());
    root.refreshItemComponent (component);
    EXPECT_EQ (nullptr, component);
}
