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

// Deliberately named differently from the target tests' helper: the test sources are compiled
// together, so two anonymous-namespace classes of the same name would clash.
class ManagerTestTarget : public Component
    , public DragAndDropTarget
{
public:
    using Component::Component;

    bool isInterestedInDragSource (const DragAndDropSourceDetails& details) override
    {
        ++interestQueryCount;

        if (wantsFilesOnly)
            return details.data.hasFiles();

        return interested;
    }

    bool itemDropped (const DragAndDropSourceDetails& details) override
    {
        ++dropCount;
        lastDropData = details.data;
        return handlesDrop;
    }

    void itemDragEnter (const DragAndDropSourceDetails&) override { ++dragEnterCount; }
    void itemDragMove (const DragAndDropSourceDetails&) override { ++dragMoveCount; }
    void itemDragExit (const DragAndDropSourceDetails&) override { ++dragExitCount; }

    bool interested = true;

    /** True for a target that only accepts files, which is what makes the empty hover payload visible. */
    bool wantsFilesOnly = false;

    bool handlesDrop = true;
    int interestQueryCount = 0;
    int dropCount = 0;
    int dragEnterCount = 0;
    int dragMoveCount = 0;
    int dragExitCount = 0;
    DragAndDropData lastDropData;
};

} // namespace

/** Exercises the path an OS-originated drag takes through the manager.

    These entry points are what the platform backend calls, so driving them directly covers the
    target resolution, the enter/move/exit bookkeeping and the drop without needing a window, a real
    mouse gesture or the ghost. */
class DragAndDropManagerTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        root.setBounds (0.0f, 0.0f, 200.0f, 200.0f);
        root.setVisible (true);

        target.setBounds (0.0f, 0.0f, 100.0f, 100.0f);
        target.setVisible (true);

        other.setBounds (100.0f, 0.0f, 100.0f, 100.0f);
        other.setVisible (true);

        root.addAndMakeVisible (target);
        root.addAndMakeVisible (other);
    }

    DragAndDropManager& manager() const
    {
        return *DragAndDropManager::getInstance();
    }

    Component root;
    ManagerTestTarget target;
    ManagerTestTarget other;
};

TEST_F (DragAndDropManagerTests, ExternalMoveEntersOnceThenMoves)
{
    Array<File> files;
    files.add (File ("/tmp/one.txt"));

    const auto data = DragAndDropData{}.withFiles (files);

    manager().handleExternalDragPosition (root, Point<float> (50.0f, 50.0f), data);
    EXPECT_EQ (1, target.dragEnterCount);

    // Still the same component under the cursor, so this is a move rather than another enter.
    manager().handleExternalDragPosition (root, Point<float> (60.0f, 60.0f), data);
    EXPECT_EQ (1, target.dragEnterCount);
    EXPECT_EQ (1, target.dragMoveCount);

    manager().handleExternalDragExit();
    EXPECT_EQ (1, target.dragExitCount);
}

TEST_F (DragAndDropManagerTests, MovingBetweenComponentsExitsTheFirstAndEntersTheSecond)
{
    Array<File> files;
    files.add (File ("/tmp/one.txt"));

    const auto data = DragAndDropData{}.withFiles (files);

    manager().handleExternalDragPosition (root, Point<float> (50.0f, 50.0f), data);
    EXPECT_EQ (1, target.dragEnterCount);

    manager().handleExternalDragPosition (root, Point<float> (150.0f, 50.0f), data);
    EXPECT_EQ (1, target.dragExitCount);
    EXPECT_EQ (1, other.dragEnterCount);
}

TEST_F (DragAndDropManagerTests, AnEmptyHoverPayloadIsNotDeliveredToAPickyTarget)
{
    // The OS-drag case: the operating system reports nothing about the payload until it is dropped,
    // so a target that only accepts files never sees the hover at all.
    target.wantsFilesOnly = true;

    manager().handleExternalDragPosition (root, Point<float> (50.0f, 50.0f), DragAndDropData{});

    EXPECT_EQ (1, target.interestQueryCount);
    EXPECT_EQ (0, target.dragEnterCount);
    EXPECT_EQ (0, target.dragMoveCount);
}

TEST_F (DragAndDropManagerTests, ExternalDropDeliversThePayloadAndReportsHandled)
{
    manager().handleExternalDrop (root, Point<float> (50.0f, 50.0f), DragAndDropData{}.withText ("hello"));

    EXPECT_EQ (1, target.dropCount);
    EXPECT_EQ ("hello", target.lastDropData.getText());
}

TEST_F (DragAndDropManagerTests, ExternalDropReportsFalseWhenNoTargetHandlesIt)
{
    target.handlesDrop = false;

    EXPECT_FALSE (manager().handleExternalDrop (root, Point<float> (50.0f, 50.0f), DragAndDropData{}.withText ("hello")));
    EXPECT_EQ (1, target.dropCount);

    // Dropped where there is nothing at all: no target is offered the payload, and nothing crashes.
    EXPECT_FALSE (manager().handleExternalDrop (root, Point<float> (500.0f, 500.0f), DragAndDropData{}.withText ("hello")));
    EXPECT_EQ (1, target.dropCount);
    EXPECT_EQ (0, other.dropCount);
}

TEST_F (DragAndDropManagerTests, AnExternalDropLeavesNoDragInFlight)
{
    EXPECT_FALSE (manager().isDragging());

    manager().handleExternalDragPosition (root, Point<float> (50.0f, 50.0f), DragAndDropData{}.withText ("hello"));
    manager().handleExternalDrop (root, Point<float> (50.0f, 50.0f), DragAndDropData{}.withText ("hello"));

    // An inbound drag is not a session of ours, so nothing is left running for the next one.
    EXPECT_FALSE (manager().isDragging());
    EXPECT_EQ (1, target.dragExitCount);
}
