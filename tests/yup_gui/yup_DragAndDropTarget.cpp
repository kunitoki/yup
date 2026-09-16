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

namespace
{

// A component that also opts in to drag-and-drop by deriving from the mixin.
class DragDropTargetComponent : public Component
    , public DragAndDropTarget
{
public:
    using Component::Component;

    bool isInterestedInDragSource (const DragAndDropSourceDetails&) override
    {
        ++interestQueryCount;
        return interested;
    }

    bool itemDropped (const DragAndDropSourceDetails& details) override
    {
        ++dropCount;
        lastDropPosition = details.localPosition;
        lastDropData = details.data;
        lastDropSourceComponent = details.sourceComponent.get();
        lastDropAllowedActions = details.allowedActions;
        lastDropSuggestedAction = details.suggestedAction;
        return handlesDrop;
    }

    void itemDragEnter (const DragAndDropSourceDetails& details) override
    {
        ++dragEnterCount;
        lastDragEnterPosition = details.localPosition;
        lastDragEnterData = details.data;
    }

    void itemDragMove (const DragAndDropSourceDetails& details) override
    {
        ++dragMoveCount;
        lastDragMovePosition = details.localPosition;
        lastDragMoveData = details.data;
    }

    void itemDragExit (const DragAndDropSourceDetails& details) override
    {
        ++dragExitCount;
        lastDragExitData = details.data;
    }

    bool interested = false;
    bool handlesDrop = false;
    int interestQueryCount = 0;
    int dropCount = 0;
    int dragEnterCount = 0;
    int dragMoveCount = 0;
    int dragExitCount = 0;
    Point<float> lastDropPosition;
    DragAndDropData lastDropData;
    Component* lastDropSourceComponent = nullptr;
    DragAndDropActions lastDropAllowedActions;
    DragAndDropAction lastDropSuggestedAction = DragAndDropAction::none;
    Point<float> lastDragEnterPosition;
    DragAndDropData lastDragEnterData;
    Point<float> lastDragMovePosition;
    DragAndDropData lastDragMoveData;
    DragAndDropData lastDragExitData;
};

} // namespace

class DragAndDropTargetTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        root = std::make_unique<DragDropTargetComponent> ("root");
        parent = std::make_unique<DragDropTargetComponent> ("parent");
        child = std::make_unique<DragDropTargetComponent> ("child");

        root->setBounds (0, 0, 400, 300);
        parent->setBounds (50, 50, 200, 150);
        child->setBounds (25, 25, 100, 75);

        root->addChildComponent (*parent);
        parent->addChildComponent (*child);

        root->setVisible (true);
        parent->setVisible (true);
        child->setVisible (true);
    }

    std::unique_ptr<DragDropTargetComponent> root;
    std::unique_ptr<DragDropTargetComponent> parent;
    std::unique_ptr<DragDropTargetComponent> child;
};

// =============================================================================
// Opt-in nature of the mixin
// =============================================================================

TEST_F (DragAndDropTargetTests, DefaultTargetDoesNotHandlePayload)
{
    DragAndDropData data = DragAndDropData().withText ("hello");

    EXPECT_FALSE (child->isInterestedInDragSource ({}));
    EXPECT_FALSE (child->itemDropped ({}));
    EXPECT_FALSE (DragAndDropTarget::dispatchItemDrop (*child, data, { 10.0f, 20.0f }));
    EXPECT_FALSE (DragAndDropTarget::dispatchItemDrop (*root, data, { 10.0f, 20.0f }));
}

TEST_F (DragAndDropTargetTests, GetTargetComponentReturnsSameComponent)
{
    EXPECT_EQ (child->getTargetComponent(), static_cast<Component*> (child.get()));
}

TEST (DragAndDropTargetDispatchTests, NonTargetComponentInChainIsSkipped)
{
    // The middle component is a plain Component, so it is not a target and must be skipped.
    DragDropTargetComponent root ("root");
    Component middle ("middle");
    DragDropTargetComponent leaf ("leaf");

    root.setBounds (0, 0, 400, 300);
    middle.setBounds (50, 50, 200, 150);
    leaf.setBounds (25, 25, 100, 75);

    root.addChildComponent (middle);
    middle.addChildComponent (leaf);

    root.setVisible (true);
    middle.setVisible (true);
    leaf.setVisible (true);

    leaf.interested = false;
    root.interested = true;
    root.handlesDrop = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    EXPECT_TRUE (DragAndDropTarget::dispatchItemDrop (leaf, data, { 85.0f, 85.0f }));
    EXPECT_EQ (leaf.dropCount, 0);
    EXPECT_EQ (root.dropCount, 1);
}

// =============================================================================
// Dropping
// =============================================================================

TEST_F (DragAndDropTargetTests, InterestedTopmostHandlesDrop)
{
    child->interested = true;
    child->handlesDrop = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    // Window position (85,85) is inside child (child origin = 0,0 + 50,50 + 25,25 = 75,75).
    EXPECT_TRUE (DragAndDropTarget::dispatchItemDrop (*child, data, { 85.0f, 85.0f }));
    EXPECT_EQ (child->dropCount, 1);
    EXPECT_EQ (parent->dropCount, 0);
    EXPECT_EQ (root->dropCount, 0);
}

TEST_F (DragAndDropTargetTests, InterestedButReturnsFalseBubblesToParent)
{
    child->interested = true;
    child->handlesDrop = false;
    parent->interested = true;
    parent->handlesDrop = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    EXPECT_TRUE (DragAndDropTarget::dispatchItemDrop (*child, data, { 85.0f, 85.0f }));
    EXPECT_EQ (child->dropCount, 1);
    EXPECT_EQ (parent->dropCount, 1);
    EXPECT_EQ (root->dropCount, 0);
}

TEST_F (DragAndDropTargetTests, UninterestedComponentSkippedEvenIfItOverridesDrop)
{
    child->interested = false;
    child->handlesDrop = true;
    parent->interested = true;
    parent->handlesDrop = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    EXPECT_TRUE (DragAndDropTarget::dispatchItemDrop (*child, data, { 85.0f, 85.0f }));
    EXPECT_EQ (child->dropCount, 0);
    EXPECT_EQ (parent->dropCount, 1);
}

TEST_F (DragAndDropTargetTests, DropPositionIsComponentLocal)
{
    child->interested = true;
    child->handlesDrop = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    DragAndDropTarget::dispatchItemDrop (*child, data, { 85.0f, 85.0f });
    EXPECT_FLOAT_EQ (child->lastDropPosition.getX(), 10.0f);
    EXPECT_FLOAT_EQ (child->lastDropPosition.getY(), 10.0f);
}

TEST_F (DragAndDropTargetTests, DropPositionRecomputedPerAncestor)
{
    child->interested = true;
    child->handlesDrop = false;
    parent->interested = true;
    parent->handlesDrop = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    DragAndDropTarget::dispatchItemDrop (*child, data, { 85.0f, 85.0f });
    EXPECT_FLOAT_EQ (parent->lastDropPosition.getX(), 35.0f);
    EXPECT_FLOAT_EQ (parent->lastDropPosition.getY(), 35.0f);
}

TEST_F (DragAndDropTargetTests, InvisibleComponentSkipped)
{
    child->interested = true;
    child->handlesDrop = true;
    child->setVisible (false);
    parent->interested = true;
    parent->handlesDrop = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    EXPECT_TRUE (DragAndDropTarget::dispatchItemDrop (*child, data, { 85.0f, 85.0f }));
    EXPECT_EQ (child->dropCount, 0);
    EXPECT_EQ (parent->dropCount, 1);
}

TEST_F (DragAndDropTargetTests, DisabledComponentSkipped)
{
    child->interested = true;
    child->handlesDrop = true;
    child->setEnabled (false);
    parent->interested = true;
    parent->handlesDrop = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    EXPECT_TRUE (DragAndDropTarget::dispatchItemDrop (*child, data, { 85.0f, 85.0f }));
    EXPECT_EQ (child->dropCount, 0);
    EXPECT_EQ (parent->dropCount, 1);
}

TEST_F (DragAndDropTargetTests, NobodyHandlesReturnsFalse)
{
    DragAndDropData data = DragAndDropData().withText ("hello");

    EXPECT_FALSE (DragAndDropTarget::dispatchItemDrop (*child, data, { 85.0f, 85.0f }));
    EXPECT_EQ (child->dropCount, 0);
    EXPECT_EQ (parent->dropCount, 0);
    EXPECT_EQ (root->dropCount, 0);
}

TEST_F (DragAndDropTargetTests, FilesOnlyPayloadDelivered)
{
    child->interested = true;
    child->handlesDrop = true;

    Array<File> files;
    files.add (File ("/tmp/one.txt"));
    files.add (File ("/tmp/two.txt"));
    DragAndDropData data = DragAndDropData().withFiles (files);

    DragAndDropTarget::dispatchItemDrop (*child, data, { 85.0f, 85.0f });
    EXPECT_TRUE (child->lastDropData.hasFiles());
    EXPECT_FALSE (child->lastDropData.hasText());
    EXPECT_EQ (child->lastDropData.getFiles().size(), 2);
}

TEST_F (DragAndDropTargetTests, TextOnlyPayloadDelivered)
{
    child->interested = true;
    child->handlesDrop = true;

    DragAndDropData data = DragAndDropData().withText ("dropped");

    DragAndDropTarget::dispatchItemDrop (*child, data, { 85.0f, 85.0f });
    EXPECT_FALSE (child->lastDropData.hasFiles());
    EXPECT_TRUE (child->lastDropData.hasText());
    EXPECT_EQ (child->lastDropData.getText(), String ("dropped"));
}

TEST_F (DragAndDropTargetTests, MixedPayloadDelivered)
{
    child->interested = true;
    child->handlesDrop = true;

    Array<File> files;
    files.add (File ("/tmp/one.txt"));
    DragAndDropData data = DragAndDropData().withFiles (files).withText ("dropped");

    DragAndDropTarget::dispatchItemDrop (*child, data, { 85.0f, 85.0f });
    EXPECT_TRUE (child->lastDropData.hasFiles());
    EXPECT_TRUE (child->lastDropData.hasText());
}

TEST_F (DragAndDropTargetTests, DragAndDropSourceDetailsCarrySourceAndActions)
{
    child->interested = true;
    child->handlesDrop = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    DragAndDropTarget::dispatchItemDrop (*child, data, { 85.0f, 85.0f });

    // An OS-originated drop has no source component and defaults to a copy suggestion.
    EXPECT_EQ (child->lastDropSourceComponent, nullptr);
    EXPECT_EQ (child->lastDropSuggestedAction, DragAndDropAction::copy);
    EXPECT_TRUE (child->lastDropAllowedActions.test (dragAndDropActionCopy));
}

// =============================================================================
// Enter / move / exit
// =============================================================================

TEST_F (DragAndDropTargetTests, DragEnterCalledWhenInterested)
{
    child->interested = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    DragAndDropTarget::dispatchItemDragEnter (*child, data, { 85.0f, 85.0f });
    EXPECT_EQ (child->dragEnterCount, 1);
    EXPECT_EQ (child->dragMoveCount, 0);
    EXPECT_EQ (child->dragExitCount, 0);
}

TEST_F (DragAndDropTargetTests, DragEnterPositionIsLocalToComponent)
{
    child->interested = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    DragAndDropTarget::dispatchItemDragEnter (*child, data, { 85.0f, 85.0f });
    EXPECT_FLOAT_EQ (child->lastDragEnterPosition.getX(), 10.0f);
    EXPECT_FLOAT_EQ (child->lastDragEnterPosition.getY(), 10.0f);
}

TEST_F (DragAndDropTargetTests, DragEnterBubblesToParentIfInterested)
{
    child->interested = true;
    parent->interested = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    DragAndDropTarget::dispatchItemDragEnter (*child, data, { 85.0f, 85.0f });
    EXPECT_EQ (child->dragEnterCount, 1);
    EXPECT_EQ (parent->dragEnterCount, 1);
    EXPECT_EQ (root->dragEnterCount, 0);
}

TEST_F (DragAndDropTargetTests, DragEnterNotCalledWhenNotInterested)
{
    child->interested = false;
    parent->interested = false;

    DragAndDropData data = DragAndDropData().withText ("hello");

    DragAndDropTarget::dispatchItemDragEnter (*child, data, { 85.0f, 85.0f });
    EXPECT_EQ (child->dragEnterCount, 0);
    EXPECT_EQ (parent->dragEnterCount, 0);
}

TEST_F (DragAndDropTargetTests, DragMoveCalledForSameComponent)
{
    child->interested = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    DragAndDropTarget::dispatchItemDragMove (*child, data, { 85.0f, 85.0f });
    EXPECT_EQ (child->dragMoveCount, 1);
    EXPECT_EQ (child->dragEnterCount, 0);
    EXPECT_EQ (child->dragExitCount, 0);
}

TEST_F (DragAndDropTargetTests, DragMoveBubblesToParentIfInterested)
{
    child->interested = true;
    parent->interested = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    DragAndDropTarget::dispatchItemDragMove (*child, data, { 85.0f, 85.0f });
    EXPECT_EQ (child->dragMoveCount, 1);
    EXPECT_EQ (parent->dragMoveCount, 1);
}

TEST_F (DragAndDropTargetTests, DragExitCalledWhenInterested)
{
    child->interested = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    DragAndDropTarget::dispatchItemDragExit (*child, data);
    EXPECT_EQ (child->dragExitCount, 1);
    EXPECT_EQ (child->dragEnterCount, 0);
    EXPECT_EQ (child->dragMoveCount, 0);
}

TEST_F (DragAndDropTargetTests, DragExitBubblesToParentIfInterested)
{
    child->interested = true;
    parent->interested = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    DragAndDropTarget::dispatchItemDragExit (*child, data);
    EXPECT_EQ (child->dragExitCount, 1);
    EXPECT_EQ (parent->dragExitCount, 1);
}

TEST_F (DragAndDropTargetTests, DragEnterRespectsDisabledComponent)
{
    child->interested = true;
    child->setEnabled (false);

    DragAndDropData data = DragAndDropData().withText ("hello");

    DragAndDropTarget::dispatchItemDragEnter (*child, data, { 85.0f, 85.0f });
    EXPECT_EQ (child->dragEnterCount, 0);
}

TEST_F (DragAndDropTargetTests, DragEnterRespectsHiddenComponent)
{
    child->interested = true;
    child->setVisible (false);

    DragAndDropData data = DragAndDropData().withText ("hello");

    DragAndDropTarget::dispatchItemDragEnter (*child, data, { 85.0f, 85.0f });
    EXPECT_EQ (child->dragEnterCount, 0);
}

TEST_F (DragAndDropTargetTests, PayloadDataDeliveredToDragEnter)
{
    child->interested = true;

    DragAndDropData data = DragAndDropData().withText ("hello").withFiles ({ File ("/tmp/a.txt") });

    DragAndDropTarget::dispatchItemDragEnter (*child, data, { 85.0f, 85.0f });
    EXPECT_TRUE (child->lastDragEnterData.hasText());
    EXPECT_EQ (child->lastDragEnterData.getText(), String ("hello"));
    EXPECT_TRUE (child->lastDragEnterData.hasFiles());
    EXPECT_EQ (child->lastDragEnterData.getFiles().size(), 1);
}

TEST_F (DragAndDropTargetTests, PayloadDataDeliveredToDragMove)
{
    child->interested = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    DragAndDropTarget::dispatchItemDragMove (*child, data, { 85.0f, 85.0f });
    EXPECT_TRUE (child->lastDragMoveData.hasText());
    EXPECT_EQ (child->lastDragMoveData.getText(), String ("hello"));
}

TEST_F (DragAndDropTargetTests, PayloadDataDeliveredToDragExit)
{
    child->interested = true;

    DragAndDropData data = DragAndDropData().withFiles ({ File ("/tmp/a.txt") });

    DragAndDropTarget::dispatchItemDragExit (*child, data);
    EXPECT_TRUE (child->lastDragExitData.hasFiles());
    EXPECT_EQ (child->lastDragExitData.getFiles().size(), 1);
}

// =============================================================================
// std::function callbacks
// =============================================================================

TEST_F (DragAndDropTargetTests, StdFunctionCallbacksAreInvoked)
{
    int enterCount = 0, moveCount = 0, exitCount = 0, dropCount = 0;

    child->onIsInterestedInDragSource = [] (const DragAndDropSourceDetails&) { return true; };
    child->onItemDragEnter = [&enterCount] (const DragAndDropSourceDetails&) { ++enterCount; };
    child->onItemDragMove = [&moveCount] (const DragAndDropSourceDetails&) { ++moveCount; };
    child->onItemDragExit = [&exitCount] (const DragAndDropSourceDetails&) { ++exitCount; };
    child->onItemDropped = [&dropCount] (const DragAndDropSourceDetails&) { ++dropCount; return true; };

    DragAndDropData data = DragAndDropData().withText ("hello");

    DragAndDropTarget::dispatchItemDragEnter (*child, data, { 85.0f, 85.0f });
    DragAndDropTarget::dispatchItemDragMove (*child, data, { 86.0f, 86.0f });
    EXPECT_TRUE (DragAndDropTarget::dispatchItemDrop (*child, data, { 85.0f, 85.0f }));
    DragAndDropTarget::dispatchItemDragExit (*child, data);

    EXPECT_EQ (enterCount, 1);
    EXPECT_EQ (moveCount, 1);
    EXPECT_EQ (dropCount, 1);
    EXPECT_EQ (exitCount, 1);
}

// =============================================================================
// The defaults a target inherits
// =============================================================================

TEST_F (DragAndDropTargetTests, TheInheritedDefaultImplementationsArePassive)
{
    // DragAndDropTargetComponent is the nameable "a Component that can receive drops" type used by
    // factories, containers and the language bindings, with none of the callbacks overridden.
    DragAndDropTargetComponent plain;

    DragAndDropSourceDetails details;
    details.data = DragAndDropData().withText ("hello");

    // The inherited defaults: no interest, nothing handled, and the notifications are safe no-ops.
    EXPECT_FALSE (plain.isInterestedInDragSource (details));
    EXPECT_FALSE (plain.itemDropped (details));

    plain.itemDragEnter (details);
    plain.itemDragMove (details);
    plain.itemDragExit (details);

    EXPECT_EQ (static_cast<Component*> (&plain), plain.getTargetComponent());
}
