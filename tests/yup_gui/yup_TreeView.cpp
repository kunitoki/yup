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
class TreeViewTests : public ::testing::Test
{
protected:
    class TestItem : public TreeViewItem
    {
    public:
        explicit TestItem (String name, bool isContainer = false)
            : name (std::move (name))
            , container (isContainer)
        {
        }

        String getItemText() const override { return name; }

        bool mightContainSubItems() const override { return container || TreeViewItem::mightContainSubItems(); }

        void itemOpennessChanged (bool isNowOpen) override
        {
            ++opennessChangeCount;

            if (! isNowOpen || lazyChildren <= 0 || getNumSubItems() > 0)
                return;

            for (int index = 0; index < lazyChildren; ++index)
            {
                auto child = std::make_unique<TestItem> (name + String (index), lazyChildren > 1);
                child->lazyChildren = lazyChildren - 1;
                addSubItem (std::move (child));
            }

            if (openFirstChild)
                getSubItem (0)->setOpen (true);
        }

        void itemClicked (const MouseEvent& event) override
        {
            ignoreUnused (event);
            ++clickCount;
        }

        void itemSelectionChanged (bool isNowSelected) override
        {
            selectionChanges.push_back (isNowSelected);
        }

        var getDragSourceDescription() const override
        {
            return draggable ? var (name) : var();
        }

        bool isInterestedInDragSource (const DragAndDropSourceDetails& details) const override
        {
            ignoreUnused (details);
            return acceptsDrops;
        }

        void itemDropped (const DragAndDropSourceDetails& details, int insertIndex) override
        {
            ignoreUnused (details);
            droppedAt.push_back (insertIndex);
        }

        String name;
        bool container = false;
        int lazyChildren = 0;
        bool openFirstChild = false;
        int opennessChangeCount = 0;
        int clickCount = 0;
        std::vector<bool> selectionChanges;
        bool draggable = true;
        bool acceptsDrops = true;
        std::vector<int> droppedAt;
    };

    void SetUp() override
    {
        tree = std::make_unique<TreeView>();
        tree->setBounds (0.0f, 0.0f, 300.0f, 240.0f);
        tree->setVisible (true);
        tree->setExpandAnimationTime (0.0);
    }

    static TestItem& add (TreeViewItem& parent, const String& name, bool isContainer = false)
    {
        return static_cast<TestItem&> (parent.addSubItem (std::make_unique<TestItem> (name, isContainer)));
    }

    /** Builds root > [a > [a0, a1], b, c > [c0]] and returns the root. */
    TestItem& buildTree()
    {
        auto root = std::make_unique<TestItem> ("root");
        auto& a = add (*root, "a");
        add (a, "a0");
        add (a, "a1");
        add (*root, "b");
        auto& c = add (*root, "c");
        add (c, "c0");

        auto& result = *root;
        tree->setRootItem (std::move (root));
        return result;
    }

    /** Builds a hidden root holding a number of leaves, named "0", "1"... */
    TestItem& buildFlatTree (int numItems)
    {
        auto root = std::make_unique<TestItem> ("root");

        for (int index = 0; index < numItems; ++index)
            add (*root, String (index));

        auto& result = *root;
        tree->setRootItemVisible (false);
        tree->setRootItem (std::move (root));
        return result;
    }

    TestItem& find (const String& name) const
    {
        std::function<TestItem* (TreeViewItem&)> search = [&] (TreeViewItem& item) -> TestItem*
        {
            if (item.getItemText() == name)
                return static_cast<TestItem*> (&item);

            for (int index = 0; index < item.getNumSubItems(); ++index)
            {
                if (auto* found = search (*item.getSubItem (index)))
                    return found;
            }

            return nullptr;
        };

        auto* found = search (*tree->getRootItem());
        jassert (found != nullptr);
        return *found;
    }

    String rowNames() const
    {
        StringArray names;

        for (int row = 0; row < tree->getNumRowsInTree(); ++row)
            names.add (tree->getItemOnRow (row)->getItemText());

        return names.joinIntoString (",");
    }

    std::vector<TreeViewItem*> itemsNamed (std::initializer_list<const char*> names) const
    {
        std::vector<TreeViewItem*> items;

        for (const auto* name : names)
            items.push_back (&find (name));

        return items;
    }

    ListBox& list() const
    {
        return *dynamic_cast<ListBox*> (tree->getChildComponent (0));
    }

    void runFrames (int count)
    {
        for (int index = 0; index < count; ++index)
            tree->refreshDisplay (1.0 / 60.0);
    }

    float heightOf (const String& name) const
    {
        return tree->getItemBounds (find (name)).getHeight();
    }

    MouseEvent eventAt (Point<float> position, KeyModifiers modifiers = {}) const
    {
        return MouseEvent (MouseEvent::leftButton, modifiers, position);
    }

    Point<float> centerOf (const String& name) const
    {
        return tree->getItemBounds (find (name)).getCenter();
    }

    void press (int keyCode, KeyModifiers modifiers = {})
    {
        tree->keyDown (KeyPress (keyCode, modifiers), Point<float>());
    }

    DragAndDropSourceDetails dragAt (float y, bool fromThisTree = false) const
    {
        DragAndDropSourceDetails details;
        details.localPosition = { 150.0f, y };

        if (fromThisTree)
            details.sourceComponent = &list();

        return details;
    }

    /** The y of a point at a fraction of a row's height. */
    float rowY (int row, float fraction) const
    {
        return (static_cast<float> (row) + fraction) * tree->getDefaultItemHeight();
    }

    std::unique_ptr<TreeView> tree;
};

//==============================================================================
TEST_F (TreeViewTests, IsNotOpaqueBecauseTheListPaints)
{
    EXPECT_FALSE (tree->isOpaque());
}

TEST_F (TreeViewTests, EmptyTreeHasNoRows)
{
    EXPECT_EQ (0, tree->getNumRowsInTree());
    EXPECT_EQ (nullptr, tree->getItemOnRow (0));
    EXPECT_EQ (nullptr, tree->getRootItem());
}

TEST_F (TreeViewTests, ClosedRootIsTheOnlyRow)
{
    auto& root = buildTree();

    EXPECT_EQ ("root", rowNames());
    EXPECT_EQ (tree.get(), root.getOwnerView());
    EXPECT_EQ (tree.get(), find ("c0").getOwnerView());
    EXPECT_EQ (0, tree->getRowOf (root));
    EXPECT_EQ (-1, tree->getRowOf (find ("a")));
}

TEST_F (TreeViewTests, OpeningShowsSubItemsDepthFirst)
{
    auto& root = buildTree();

    root.setOpen (true);
    EXPECT_EQ ("root,a,b,c", rowNames());

    find ("a").setOpen (true);
    EXPECT_EQ ("root,a,a0,a1,b,c", rowNames());
    EXPECT_EQ (4, tree->getRowOf (find ("b")));
    EXPECT_EQ (-1, tree->getRowOf (find ("c0")));
}

TEST_F (TreeViewTests, ClosingRemovesTheWholeRun)
{
    auto& root = buildTree();
    root.setOpen (true);
    find ("a").setOpen (true);

    root.setOpen (false);

    EXPECT_EQ ("root", rowNames());
    EXPECT_TRUE (find ("a").isOpen());

    root.setOpen (true);
    EXPECT_EQ ("root,a,a0,a1,b,c", rowNames());
}

TEST_F (TreeViewTests, HiddenRootShowsItsSubItemsAtTheTop)
{
    auto& root = buildTree();

    tree->setRootItemVisible (false);

    EXPECT_TRUE (root.isOpen());
    EXPECT_EQ ("a,b,c", rowNames());
    EXPECT_EQ (-1, tree->getRowOf (root));

    auto* row = dynamic_cast<TreeViewRow*> (list().getComponentForRow (0));
    ASSERT_NE (nullptr, row);
    EXPECT_EQ (0, row->getDepth());
    EXPECT_EQ ("a", row->getItemText());

    tree->setRootItemVisible (true);
    EXPECT_EQ ("root,a,b,c", rowNames());
}

TEST_F (TreeViewTests, HiddenRootIsOpenedWhenSet)
{
    tree->setRootItemVisible (false);

    auto root = std::make_unique<TestItem> ("r", true);
    root->lazyChildren = 2;
    tree->setRootItem (std::move (root));

    EXPECT_EQ ("r0,r1", rowNames());
}

TEST_F (TreeViewTests, AddingUnderAnOpenParentInsertsRows)
{
    auto& root = buildTree();
    root.setOpen (true);
    auto& a = find ("a");
    a.setOpen (true);

    add (a, "a2");
    EXPECT_EQ ("root,a,a0,a1,a2,b,c", rowNames());

    root.addSubItem (std::make_unique<TestItem> ("first"), 0);
    EXPECT_EQ ("root,first,a,a0,a1,a2,b,c", rowNames());

    auto branch = std::make_unique<TestItem> ("d");
    add (*branch, "d0");
    branch->setOpen (true);
    root.addSubItem (std::move (branch));
    EXPECT_EQ ("root,first,a,a0,a1,a2,b,c,d,d0", rowNames());
}

TEST_F (TreeViewTests, AddingUnderAClosedParentAddsNoRows)
{
    auto& root = buildTree();
    root.setOpen (true);

    add (find ("c"), "c1");

    EXPECT_EQ ("root,a,b,c", rowNames());
    EXPECT_EQ (2, find ("c").getNumSubItems());
}

TEST_F (TreeViewTests, RemovingRemovesTheItemAndItsRun)
{
    auto& root = buildTree();
    root.setOpen (true);
    find ("a").setOpen (true);

    auto removed = root.removeSubItem (0);

    EXPECT_EQ ("root,b,c", rowNames());
    EXPECT_EQ (nullptr, removed->getOwnerView());
    EXPECT_EQ (-1, tree->getRowOf (*removed));
}

TEST_F (TreeViewTests, ClearingSubItemsRemovesTheirRows)
{
    auto& root = buildTree();
    root.setOpen (true);
    find ("a").setOpen (true);

    root.clearSubItems();

    EXPECT_EQ ("root", rowNames());
}

TEST_F (TreeViewTests, MovingALeafKeepsItsSelection)
{
    auto& root = buildTree();
    root.setOpen (true);
    auto& b = find ("b");
    b.setSelected (true);

    root.moveSubItem (1, 0);

    EXPECT_EQ ("root,b,a,c", rowNames());
    EXPECT_TRUE (b.isSelected());
    EXPECT_EQ (1, tree->getNumSelectedItems());
}

TEST_F (TreeViewTests, MovingABranchMovesItsRunAndSelection)
{
    auto& root = buildTree();
    root.setOpen (true);
    find ("a").setOpen (true);
    auto& a0 = find ("a0");
    a0.setSelected (true);

    int selectionChanges = 0;
    tree->onSelectionChanged = [&] { ++selectionChanges; };

    root.moveSubItem (0, 2);

    EXPECT_EQ ("root,b,c,a,a0,a1", rowNames());
    EXPECT_TRUE (a0.isSelected());
    EXPECT_EQ (1, tree->getNumSelectedItems());
    EXPECT_EQ (0, selectionChanges);
}

TEST_F (TreeViewTests, LazySubItemsAreInsertedOnce)
{
    auto root = std::make_unique<TestItem> ("r", true);
    root->lazyChildren = 3;
    auto& rootRef = *root;
    tree->setRootItem (std::move (root));

    rootRef.setOpen (true);

    EXPECT_EQ ("r,r0,r1,r2", rowNames());
    EXPECT_EQ (1, rootRef.opennessChangeCount);

    find ("r1").setOpen (true);
    EXPECT_EQ ("r,r0,r1,r10,r11,r2", rowNames());
}

TEST_F (TreeViewTests, LazyItemsCanOpenTheirOwnSubItems)
{
    auto root = std::make_unique<TestItem> ("r", true);
    root->lazyChildren = 2;
    root->openFirstChild = true;
    auto& rootRef = *root;
    tree->setRootItem (std::move (root));

    rootRef.setOpen (true);

    EXPECT_EQ ("r,r0,r00,r1", rowNames());
}

TEST_F (TreeViewTests, ReplacingTheRootClearsTheSelection)
{
    auto& root = buildTree();
    root.setSelected (true);

    int selectionChanges = 0;
    tree->onSelectionChanged = [&] { ++selectionChanges; };

    tree->setRootItem (std::make_unique<TestItem> ("other"));

    EXPECT_EQ ("other", rowNames());
    EXPECT_EQ (0, tree->getNumSelectedItems());
    EXPECT_EQ (1, selectionChanges);
}

//==============================================================================
TEST_F (TreeViewTests, CollapsingMovesTheSelectionToTheItem)
{
    auto& root = buildTree();
    root.setOpen (true);
    auto& a = find ("a");
    a.setOpen (true);
    auto& a1 = find ("a1");
    a1.setSelected (true);

    int selectionChanges = 0;
    tree->onSelectionChanged = [&] { ++selectionChanges; };

    a.setOpen (false);

    EXPECT_EQ ("root,a,b,c", rowNames());
    EXPECT_TRUE (a.isSelected());
    EXPECT_FALSE (a1.isSelected());
    EXPECT_EQ (1, list().getCurrentRow());
    EXPECT_EQ (1, selectionChanges);
    EXPECT_EQ ((std::vector<bool> { true, false }), a1.selectionChanges);
    EXPECT_EQ ((std::vector<bool> { true }), a.selectionChanges);
}

TEST_F (TreeViewTests, MultipleSelectionSurvivesExpanding)
{
    tree->setSelectionMode (ListBox::SelectionMode::multiple);

    auto& root = buildTree();
    root.setOpen (true);
    find ("b").setSelected (true);
    find ("c").setSelected (true, false);

    find ("a").setOpen (true);

    EXPECT_EQ (itemsNamed ({ "b", "c" }), tree->getSelectedItems());
}

TEST_F (TreeViewTests, CollapsingKeepsSelectionOutsideTheRun)
{
    tree->setSelectionMode (ListBox::SelectionMode::multiple);

    auto& root = buildTree();
    root.setOpen (true);
    find ("a").setOpen (true);
    find ("a0").setSelected (true);
    find ("c").setSelected (true, false);

    find ("a").setOpen (false);

    EXPECT_EQ (itemsNamed ({ "a", "c" }), tree->getSelectedItems());
}

TEST_F (TreeViewTests, SelectionCallbacksReportTheItems)
{
    auto& root = buildTree();
    root.setOpen (true);
    auto& b = find ("b");
    auto& c = find ("c");

    int selectionChanges = 0;
    tree->onSelectionChanged = [&] { ++selectionChanges; };

    b.setSelected (true);
    c.setSelected (true);
    tree->clearSelectedItems();

    EXPECT_EQ ((std::vector<bool> { true, false }), b.selectionChanges);
    EXPECT_EQ ((std::vector<bool> { true, false }), c.selectionChanges);
    EXPECT_EQ (3, selectionChanges);
}

TEST_F (TreeViewTests, TurningSelectionOffReportsTheDeselection)
{
    auto& root = buildTree();
    root.setOpen (true);
    auto& b = find ("b");
    b.setSelected (true);

    int selectionChanges = 0;
    tree->onSelectionChanged = [&] { ++selectionChanges; };

    tree->setSelectionMode (ListBox::SelectionMode::none);

    EXPECT_EQ (0, tree->getNumSelectedItems());
    EXPECT_EQ ((std::vector<bool> { true, false }), b.selectionChanges);
    EXPECT_EQ (1, selectionChanges);
}

TEST_F (TreeViewTests, HiddenItemsCannotBeSelected)
{
    buildTree();

    find ("a0").setSelected (true);

    EXPECT_EQ (0, tree->getNumSelectedItems());
}

//==============================================================================
TEST_F (TreeViewTests, RightOpensThenMovesToTheFirstSubItem)
{
    auto& root = buildTree();
    root.setOpen (true);
    auto& a = find ("a");
    a.setSelected (true);

    press (KeyPress::rightKey);
    EXPECT_TRUE (a.isOpen());
    EXPECT_TRUE (a.isSelected());

    press (KeyPress::rightKey);
    EXPECT_TRUE (find ("a0").isSelected());

    // A leaf ignores Right.
    press (KeyPress::rightKey);
    EXPECT_TRUE (find ("a0").isSelected());
}

TEST_F (TreeViewTests, LeftMovesToTheParentThenCloses)
{
    auto& root = buildTree();
    root.setOpen (true);
    auto& a = find ("a");
    a.setOpen (true);
    find ("a0").setSelected (true);

    press (KeyPress::leftKey);
    EXPECT_TRUE (a.isSelected());
    EXPECT_TRUE (a.isOpen());

    press (KeyPress::leftKey);
    EXPECT_FALSE (a.isOpen());
    EXPECT_TRUE (a.isSelected());

    press (KeyPress::leftKey);
    EXPECT_TRUE (root.isSelected());
}

TEST_F (TreeViewTests, AltRightOpensTheWholeBranch)
{
    auto& root = buildTree();
    root.setSelected (true);

    press (KeyPress::rightKey, KeyModifiers (KeyModifiers::altMask));

    EXPECT_EQ ("root,a,a0,a1,b,c,c0", rowNames());
    EXPECT_TRUE (root.isSelected());

    press (KeyPress::leftKey, KeyModifiers (KeyModifiers::altMask));

    EXPECT_EQ ("root", rowNames());
    EXPECT_FALSE (find ("a").isOpen());
}

TEST_F (TreeViewTests, OtherKeysReachTheList)
{
    auto& root = buildTree();
    root.setOpen (true);
    find ("a").setSelected (true);

    press (KeyPress::downKey);

    EXPECT_TRUE (find ("b").isSelected());
}

TEST_F (TreeViewTests, ReturnAndDeleteReportItems)
{
    tree->setSelectionMode (ListBox::SelectionMode::multiple);

    auto& root = buildTree();
    root.setOpen (true);
    find ("a").setSelected (true);
    find ("c").setSelected (true, false);

    TreeViewItem* returned = nullptr;
    std::vector<TreeViewItem*> deleted;
    tree->onReturnKeyPressed = [&] (TreeViewItem& item) { returned = &item; };
    tree->onDeleteKeyPressed = [&] (std::vector<TreeViewItem*> items) { deleted = items; };

    press (KeyPress::enterKey);
    press (KeyPress::deleteKey);

    EXPECT_EQ (&find ("c"), returned);
    EXPECT_EQ (itemsNamed ({ "a", "c" }), deleted);
}

//==============================================================================
TEST_F (TreeViewTests, ClickSelectsAndReportsTheItem)
{
    auto& root = buildTree();
    root.setOpen (true);
    auto& b = find ("b");

    TreeViewItem* clicked = nullptr;
    tree->onItemClicked = [&] (TreeViewItem& item) { clicked = &item; };

    list().mouseDown (eventAt (centerOf ("b")));

    EXPECT_EQ (1, b.clickCount);
    EXPECT_EQ (&b, clicked);
    EXPECT_TRUE (b.isSelected());
}

TEST_F (TreeViewTests, DoubleClickTogglesTheItem)
{
    auto& root = buildTree();
    root.setOpen (true);
    auto& a = find ("a");

    TreeViewItem* doubleClicked = nullptr;
    tree->onItemDoubleClicked = [&] (TreeViewItem& item) { doubleClicked = &item; };

    list().mouseDoubleClick (eventAt (centerOf ("a")));
    EXPECT_TRUE (a.isOpen());
    EXPECT_EQ (&a, doubleClicked);

    list().mouseDoubleClick (eventAt (centerOf ("a")));
    EXPECT_FALSE (a.isOpen());
}

TEST_F (TreeViewTests, DisclosureClickTogglesWithoutSelecting)
{
    auto& root = buildTree();
    root.setOpen (true);
    auto& a = find ("a");

    // Row "a" is at depth 1, so its disclosure button spans the second indent column.
    const Point<float> disclosure { tree->getIndentSize() * 1.5f, centerOf ("a").getY() };
    auto* hit = tree->findComponentAtForMouseEvent (disclosure);

    ASSERT_NE (nullptr, hit);
    EXPECT_NE (&list(), hit);

    hit->mouseDown (eventAt (disclosure));

    EXPECT_TRUE (a.isOpen());
    EXPECT_FALSE (a.isSelected());

    // A press on the text falls through to the list.
    EXPECT_EQ (&list(), tree->findComponentAtForMouseEvent (centerOf ("a")));
}

TEST_F (TreeViewTests, LeavesHaveNoDisclosureButton)
{
    auto& root = buildTree();
    root.setOpen (true);

    const Point<float> disclosure { tree->getIndentSize() * 1.5f, centerOf ("b").getY() };

    EXPECT_EQ (&list(), tree->findComponentAtForMouseEvent (disclosure));
}

TEST_F (TreeViewTests, ItemsCanBeFoundAndScrolledTo)
{
    auto& root = buildFlatTree (50);

    EXPECT_EQ (root.getSubItem (2), tree->getItemAt ({ 50.0f, rowY (2, 0.5f) }));

    tree->scrollToItem (*root.getSubItem (40));

    EXPECT_GT (tree->getScrollPosition(), 0.0f);
    EXPECT_TRUE (tree->getLocalBounds().contains (tree->getItemBounds (*root.getSubItem (40))));
}

//==============================================================================
TEST_F (TreeViewTests, ExpandingRowsGrowFromNothing)
{
    auto& root = buildTree();
    root.setOpen (true);
    tree->setExpandAnimationTime (0.2);

    find ("a").setOpen (true);

    EXPECT_EQ ("root,a,a0,a1,b,c", rowNames());
    EXPECT_LT (heightOf ("a0"), 1.0f);

    runFrames (3);
    EXPECT_GT (heightOf ("a0"), 1.0f);
    EXPECT_LT (heightOf ("a0"), 24.0f);

    runFrames (20);
    EXPECT_FLOAT_EQ (24.0f, heightOf ("a0"));
    EXPECT_FLOAT_EQ (24.0f, heightOf ("b"));
}

TEST_F (TreeViewTests, CollapsingRemovesTheRowsAtTheEnd)
{
    auto& root = buildTree();
    root.setOpen (true);
    find ("a").setOpen (true);
    tree->setExpandAnimationTime (0.2);

    find ("a").setOpen (false);

    EXPECT_EQ ("root,a,a0,a1,b,c", rowNames());

    runFrames (3);
    EXPECT_EQ ("root,a,a0,a1,b,c", rowNames());
    EXPECT_LT (heightOf ("a0"), 24.0f);

    runFrames (20);
    EXPECT_EQ ("root,a,b,c", rowNames());
}

TEST_F (TreeViewTests, ReversingMidAnimationFlipsDirection)
{
    auto& root = buildTree();
    root.setOpen (true);
    tree->setExpandAnimationTime (0.2);
    auto& a = find ("a");

    a.setOpen (true);
    runFrames (4);
    const auto grown = heightOf ("a0");

    a.setOpen (false);
    EXPECT_NEAR (grown, heightOf ("a0"), 0.01f);

    runFrames (1);
    EXPECT_LT (heightOf ("a0"), grown);
    EXPECT_EQ ("root,a,a0,a1,b,c", rowNames());

    runFrames (30);
    EXPECT_EQ ("root,a,b,c", rowNames());
}

TEST_F (TreeViewTests, PressDuringAnAnimationHitsTheRowUnderThePointer)
{
    auto& root = buildTree();
    root.setOpen (true);
    tree->setExpandAnimationTime (0.2);

    find ("a").setOpen (true);
    runFrames (2);

    list().mouseDown (eventAt (centerOf ("b")));

    EXPECT_TRUE (find ("b").isSelected());
    EXPECT_FLOAT_EQ (24.0f, heightOf ("a0"));
}

TEST_F (TreeViewTests, LargeRunsOpenInstantly)
{
    auto& root = buildTree();
    root.setOpen (true);
    auto& b = find ("b");

    for (int index = 0; index < 30; ++index)
        add (b, "b" + String (index));

    tree->setExpandAnimationTime (0.2);
    b.setOpen (true);

    EXPECT_FLOAT_EQ (24.0f, heightOf ("b0"));
}

TEST_F (TreeViewTests, StructuralChangesFinishTheAnimation)
{
    auto& root = buildTree();
    root.setOpen (true);
    tree->setExpandAnimationTime (0.2);

    find ("a").setOpen (true);
    add (root, "d");

    EXPECT_FLOAT_EQ (24.0f, heightOf ("a0"));
    EXPECT_EQ ("root,a,a0,a1,b,c,d", rowNames());
}

//==============================================================================
TEST_F (TreeViewTests, DropInTheTopQuarterInsertsBefore)
{
    auto& root = buildTree();
    root.setOpen (true);
    find ("a").setOpen (true);

    // Rows: root, a, a0, a1, b, c
    EXPECT_TRUE (tree->itemDropped (dragAt (rowY (4, 0.1f))));
    EXPECT_EQ ((std::vector<int> { 1 }), root.droppedAt);
}

TEST_F (TreeViewTests, DropInTheBottomQuarterInsertsAfter)
{
    auto& root = buildTree();
    root.setOpen (true);
    find ("a").setOpen (true);

    EXPECT_TRUE (tree->itemDropped (dragAt (rowY (4, 0.9f))));
    EXPECT_EQ ((std::vector<int> { 2 }), root.droppedAt);
}

TEST_F (TreeViewTests, DropInTheMiddleOfAContainerInsertsInto)
{
    auto& root = buildTree();
    root.setOpen (true);

    EXPECT_TRUE (tree->itemDropped (dragAt (rowY (3, 0.5f))));
    EXPECT_EQ ((std::vector<int> { 1 }), find ("c").droppedAt);
    EXPECT_TRUE (root.droppedAt.empty());
}

TEST_F (TreeViewTests, DropInTheMiddleOfALeafPicksTheNearestSide)
{
    auto& root = buildTree();
    root.setOpen (true);

    tree->itemDropped (dragAt (rowY (2, 0.4f)));
    tree->itemDropped (dragAt (rowY (2, 0.6f)));

    EXPECT_EQ ((std::vector<int> { 1, 2 }), root.droppedAt);
}

TEST_F (TreeViewTests, DropBelowAnOpenItemInsertsItsFirstSubItem)
{
    auto& root = buildTree();
    root.setOpen (true);
    find ("a").setOpen (true);

    tree->itemDropped (dragAt (rowY (1, 0.9f)));

    EXPECT_EQ ((std::vector<int> { 0 }), find ("a").droppedAt);
}

TEST_F (TreeViewTests, DropOnTheRootRowOrPastTheRowsAppendsToTheRoot)
{
    auto& root = buildTree();
    root.setOpen (true);

    tree->itemDropped (dragAt (rowY (0, 0.1f)));
    tree->itemDropped (dragAt (rowY (7, 0.5f)));

    EXPECT_EQ ((std::vector<int> { 3, 3 }), root.droppedAt);
}

TEST_F (TreeViewTests, DeclinedDropIsNotHandled)
{
    auto& root = buildTree();
    root.setOpen (true);
    root.acceptsDrops = false;

    EXPECT_FALSE (tree->itemDropped (dragAt (rowY (2, 0.1f))));
    EXPECT_TRUE (root.droppedAt.empty());
}

TEST_F (TreeViewTests, ContentCannotBeDroppedIntoItself)
{
    auto& root = buildTree();
    root.setOpen (true);
    find ("a").setOpen (true);
    find ("a").setSelected (true);

    // Into "a", and before "a1" which is inside "a".
    EXPECT_FALSE (tree->itemDropped (dragAt (rowY (1, 0.5f), true)));
    EXPECT_FALSE (tree->itemDropped (dragAt (rowY (3, 0.1f), true)));

    // Before "b" is fine.
    EXPECT_TRUE (tree->itemDropped (dragAt (rowY (4, 0.1f), true)));
}

TEST_F (TreeViewTests, HoveringAClosedContainerOpensIt)
{
    auto& root = buildTree();
    root.setOpen (true);
    auto& c = find ("c");

    tree->itemDragEnter (dragAt (rowY (3, 0.5f)));
    tree->itemDragMove (dragAt (rowY (3, 0.5f)));

    runFrames (20);
    EXPECT_FALSE (c.isOpen());

    runFrames (30);
    EXPECT_TRUE (c.isOpen());
    EXPECT_EQ ("root,a,b,c,c0", rowNames());
}

TEST_F (TreeViewTests, LeavingStopsTheHoverExpansion)
{
    auto& root = buildTree();
    root.setOpen (true);

    tree->itemDragMove (dragAt (rowY (3, 0.5f)));
    runFrames (20);
    tree->itemDragExit (dragAt (rowY (3, 0.5f)));
    runFrames (60);

    EXPECT_FALSE (find ("c").isOpen());
}

TEST_F (TreeViewTests, DraggingNearTheEdgeAutoScrolls)
{
    buildFlatTree (40);

    tree->itemDragMove (dragAt (tree->getHeight() - 2.0f));
    runFrames (10);

    const auto scrolled = tree->getScrollPosition();
    EXPECT_GT (scrolled, 0.0f);

    tree->itemDragExit (dragAt (tree->getHeight() - 2.0f));
    runFrames (10);

    EXPECT_FLOAT_EQ (scrolled, tree->getScrollPosition());
}

TEST_F (TreeViewTests, DraggedItemsLeaveOutItemsInsideOtherDraggedItems)
{
    tree->setSelectionMode (ListBox::SelectionMode::multiple);

    auto& root = buildTree();
    root.setOpen (true);
    find ("a").setOpen (true);
    find ("a").setSelected (true);
    find ("a0").setSelected (true, false);
    find ("b").setSelected (true, false);

    EXPECT_EQ (itemsNamed ({ "a", "b" }), TreeView::getDraggedItems (dragAt (0.0f, true)));
    EXPECT_TRUE (TreeView::getDraggedItems (dragAt (0.0f, false)).empty());

    Component other;
    auto details = dragAt (0.0f);
    details.sourceComponent = &other;
    EXPECT_TRUE (TreeView::getDraggedItems (details).empty());
}

TEST_F (TreeViewTests, DragDescriptionCombinesTheItems)
{
    tree->setSelectionMode (ListBox::SelectionMode::multiple);

    auto& root = buildTree();
    root.setOpen (true);
    auto* model = list().getModel();

    EXPECT_EQ (var ("a"), model->getDragSourceDescription ({ 1 }));

    const auto both = model->getDragSourceDescription ({ 1, 2 });
    ASSERT_TRUE (both.isArray());
    EXPECT_EQ (2, both.size());

    find ("b").draggable = false;
    EXPECT_TRUE (model->getDragSourceDescription ({ 1, 2 }).isVoid());
}

//==============================================================================
TEST_F (TreeViewTests, OpennessStateRoundTrips)
{
    auto& root = buildTree();
    root.setOpen (true);
    find ("a").setOpen (true);
    find ("a1").setSelected (true);

    const auto state = tree->getOpennessState (false);

    root.setOpenRecursively (false);
    EXPECT_EQ ("root", rowNames());
    EXPECT_EQ (0, tree->getNumSelectedItems());

    tree->restoreOpennessState (state, true);

    EXPECT_EQ ("root,a,a0,a1,b,c", rowNames());
    EXPECT_TRUE (find ("a1").isSelected());
}

TEST_F (TreeViewTests, RestoringWithoutSelectionKeepsTheCurrentOne)
{
    auto& root = buildTree();
    root.setOpen (true);
    find ("c").setOpen (true);
    find ("b").setSelected (true);

    const auto state = tree->getOpennessState (false);

    find ("c0").setSelected (true);
    find ("c").setOpen (false);
    find ("c").setSelected (true);

    tree->restoreOpennessState (state, false);

    EXPECT_EQ ("root,a,b,c,c0", rowNames());
    EXPECT_TRUE (find ("c").isSelected());
    EXPECT_FALSE (find ("b").isSelected());
}

TEST_F (TreeViewTests, RestoringRecreatesLazyItems)
{
    const auto makeRoot = []
    {
        auto root = std::make_unique<TestItem> ("r", true);
        root->lazyChildren = 2;
        return root;
    };

    auto first = makeRoot();
    auto& firstRoot = *first;
    tree->setRootItem (std::move (first));
    firstRoot.setOpen (true);
    find ("r1").setOpen (true);

    const auto state = tree->getOpennessState (false);

    tree->setRootItem (makeRoot());
    EXPECT_EQ ("r", rowNames());

    tree->restoreOpennessState (state, false);

    EXPECT_EQ ("r,r0,r1,r10", rowNames());
}

TEST_F (TreeViewTests, RestoringAnotherRootDoesNothing)
{
    auto& root = buildTree();
    root.setOpen (true);
    const auto state = tree->getOpennessState (false);

    tree->setRootItem (std::make_unique<TestItem> ("other", true));
    tree->restoreOpennessState (state, false);

    EXPECT_FALSE (tree->getRootItem()->isOpen());
}

TEST_F (TreeViewTests, OpennessStateCanCarryTheScrollPosition)
{
    buildFlatTree (40);
    tree->setScrollPosition (100.0f);

    const auto state = tree->getOpennessState (true);

    tree->setScrollPosition (0.0f);
    tree->restoreOpennessState (state, false);

    EXPECT_FLOAT_EQ (100.0f, tree->getScrollPosition());
}
