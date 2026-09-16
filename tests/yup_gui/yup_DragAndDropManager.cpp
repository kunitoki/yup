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

//==============================================================================

namespace
{

/** A source that is also a Component, the shape a real draggable row takes. */
class ManagerTestSource : public Component
    , public DragAndDropSource
{
public:
    using Component::Component;

    void dragOperationStarted (const DragAndDropData& data) override
    {
        ++startedCount;
        lastStarted = data;
    }

    void dragOperationEnded (const DragAndDropData& data, DragAndDropAction performed) override
    {
        ++endedCount;
        lastEnded = data;
        lastPerformed = performed;
    }

    int startedCount = 0;
    int endedCount = 0;
    DragAndDropData lastStarted;
    DragAndDropData lastEnded;
    DragAndDropAction lastPerformed = DragAndDropAction::none;
};

/** An event carrying a position already in screen space; with no source component the position comes
    through verbatim, which is what a global mouse listener sees. */
MouseEvent screenEventAt (const Point<float>& screenPosition, KeyModifiers modifiers = {})
{
    return MouseEvent (MouseEvent::noButtons, modifiers, screenPosition, nullptr);
}

DragAndDropSource::DragOptions payloadOptions (const String& text)
{
    DragAndDropSource::DragOptions options;
    options.withData (DragAndDropData{}.withText (text));
    return options;
}

/** Cancels whatever is in flight and ends it, so a failure in one test cannot leave a session
    running - or a ghost window open - for the next one. */
void endAnyActiveDrag()
{
    auto* manager = DragAndDropManager::getInstanceWithoutCreating();

    if (manager == nullptr || ! manager->isDragging())
        return;

    manager->cancelDrag();
    Desktop::getInstance()->handleGlobalMouseUp (screenEventAt ({ 0.0f, 0.0f }));
}

} // namespace

/** Drives the session the manager itself owns: the ghost, the global mouse listener, the source
    notifications and the start/end bookkeeping. A drag that never crosses a component resolves to
    nothing under the cursor, so none of this needs a window. */
class DragAndDropManagerSessionTests : public ::testing::Test
{
protected:
    void TearDown() override { endAnyActiveDrag(); }

    DragAndDropManager& manager() const { return *DragAndDropManager::getInstance(); }

    void startSession (const String& text = "payload")
    {
        ASSERT_TRUE (manager().startDragging (source, source, payloadOptions (text)));
    }

    ManagerTestSource source;

    /** Kept as a fixture member so it outlives TearDown, which ends the drag that reparented it. */
    Component dragImageHost;
};

TEST_F (DragAndDropManagerSessionTests, AnEmptyPayloadDoesNotStartADrag)
{
    EXPECT_FALSE (manager().startDragging (source, source, DragAndDropSource::DragOptions{}));
    EXPECT_FALSE (manager().isDragging());
    EXPECT_EQ (0, source.startedCount);
}

TEST_F (DragAndDropManagerSessionTests, StartingADragBeginsTheSessionAndNotifiesTheSource)
{
    EXPECT_FALSE (manager().isDragging());

    ASSERT_TRUE (manager().startDragging (source, source, payloadOptions ("payload")));

    EXPECT_TRUE (manager().isDragging());
    EXPECT_EQ (1, source.startedCount);
    EXPECT_EQ ("payload", source.lastStarted.getText());
    EXPECT_EQ ("payload", manager().getCurrentDragData().getText());
    EXPECT_EQ (static_cast<Component*> (&source), manager().getCurrentDragSourceComponent());

    // Nothing has been hovered yet, so there is no target even though a drag is live.
    EXPECT_EQ (nullptr, manager().getCurrentDragTarget());
}

TEST_F (DragAndDropManagerSessionTests, ASecondDragCannotStartWhileOneIsInFlight)
{
    startSession ("first");

    EXPECT_FALSE (manager().startDragging (source, source, payloadOptions ("second")));
    EXPECT_EQ ("first", manager().getCurrentDragData().getText());
    EXPECT_EQ (1, source.startedCount);
}

TEST_F (DragAndDropManagerSessionTests, TheFunctionNotificationsRunAlongsideTheVirtualOnes)
{
    int functionStarted = 0;
    int functionEnded = 0;

    source.onDragStarted = [&] (const DragAndDropData&) { ++functionStarted; };
    source.onDragEnded = [&] (const DragAndDropData&, DragAndDropAction) { ++functionEnded; };

    startSession();

    EXPECT_EQ (1, functionStarted);

    Desktop::getInstance()->handleGlobalMouseUp (screenEventAt ({ 5.0f, 5.0f }));

    EXPECT_EQ (1, functionEnded);
}

TEST_F (DragAndDropManagerSessionTests, AStaticDragImageIsHandedToTheGhost)
{
    auto options = payloadOptions ("payload");
    options.withDragImage (Image (4, 4, PixelFormat::RGBA), { 1.0f, 2.0f }).withImageOpacity (0.4f);

    ASSERT_TRUE (manager().startDragging (source, source, options));

    EXPECT_TRUE (manager().isDragging());
}

TEST_F (DragAndDropManagerSessionTests, ALiveComponentDragImageIsHandedToTheGhost)
{
    dragImageHost.setSize (16.0f, 16.0f);

    auto options = payloadOptions ("payload");
    options.withDragImageComponent (&dragImageHost, {}).withImageOpacity (0.4f);

    ASSERT_TRUE (manager().startDragging (source, source, options));

    EXPECT_TRUE (manager().isDragging());
    EXPECT_EQ (&source, manager().getCurrentDragSourceComponent());
}

TEST_F (DragAndDropManagerSessionTests, AMouseDragKeepsTheSessionAlive)
{
    startSession();

    Desktop::getInstance()->handleGlobalMouseDrag (screenEventAt ({ 40.0f, 30.0f }));

    EXPECT_TRUE (manager().isDragging());
    EXPECT_EQ (0, source.endedCount);
}

TEST_F (DragAndDropManagerSessionTests, AMouseUpWithNothingUnderTheCursorPerformsNoAction)
{
    startSession();

    Desktop::getInstance()->handleGlobalMouseUp (screenEventAt ({ 40.0f, 30.0f }));

    EXPECT_FALSE (manager().isDragging());
    EXPECT_EQ (1, source.endedCount);
    EXPECT_EQ ("payload", source.lastEnded.getText());
    EXPECT_EQ (DragAndDropAction::none, source.lastPerformed);

    // The session state is fully released, so the next drag starts from a clean slate.
    EXPECT_TRUE (manager().getCurrentDragData().isEmpty());
    EXPECT_EQ (nullptr, manager().getCurrentDragSourceComponent());
    EXPECT_EQ (nullptr, manager().getCurrentDragTarget());
}

TEST_F (DragAndDropManagerSessionTests, CancellingOnlyTakesEffectOnTheNextMouseUp)
{
    startSession();

    manager().cancelDrag();

    // Cancelling is not the same as ending: the pointer is still down, so the session is still live.
    EXPECT_TRUE (manager().isDragging());
    EXPECT_EQ (0, source.endedCount);

    // Cancelling again is a no-op rather than a second notification.
    manager().cancelDrag();
    EXPECT_TRUE (manager().isDragging());

    Desktop::getInstance()->handleGlobalMouseUp (screenEventAt ({ 40.0f, 30.0f }));

    EXPECT_FALSE (manager().isDragging());
    EXPECT_EQ (1, source.endedCount);
    EXPECT_EQ (DragAndDropAction::none, source.lastPerformed);
}

TEST_F (DragAndDropManagerSessionTests, ACancelledDragIgnoresFurtherMouseDrags)
{
    startSession();
    manager().cancelDrag();

    Desktop::getInstance()->handleGlobalMouseDrag (screenEventAt ({ 40.0f, 30.0f }));

    EXPECT_TRUE (manager().isDragging());
}

TEST_F (DragAndDropManagerSessionTests, AnExternalDragIsIgnoredWhileAnInternalDragIsInFlight)
{
    startSession();

    Component root;
    root.setBounds (0.0f, 0.0f, 100.0f, 100.0f);

    const auto data = DragAndDropData{}.withText ("external");

    // An internal session owns the cursor, so the platform callbacks must not start a second,
    // competing hover underneath it.
    EXPECT_FALSE (manager().handleExternalDrop (root, Point<float> (10.0f, 10.0f), data));
    manager().handleExternalDragExit();
    manager().handleExternalDragPosition (root, Point<float> (10.0f, 10.0f), data);

    EXPECT_TRUE (manager().isDragging());
}

//==============================================================================

/** The other half of the manager: a drag that really crosses a window, so Desktop::findComponentAt
    has a window to resolve the cursor against. Creating one is what the ghost already does, so these
    run wherever the rest of the ghost tests do. */
class DragAndDropManagerDropTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        root.setBounds (0.0f, 0.0f, 200.0f, 200.0f);
        root.setVisible (true);

        target.setBounds (0.0f, 0.0f, 100.0f, 100.0f);
        source.setBounds (100.0f, 0.0f, 100.0f, 100.0f);

        root.addAndMakeVisible (target);
        root.addAndMakeVisible (source);

        root.addToDesktop (ComponentNative::Options{}
                               .withDecoration (false)
                               .withResizableWindow (false)
                               .withFocusable (false)
                               .withAllowedHighDensityDisplay (false)
                               .withTemporaryWindow (true));

        // Without a real window there is nothing for the manager to resolve the cursor against.
        auto* native = root.getNativeComponent();

        if (native == nullptr || ! native->isVisible())
            GTEST_SKIP() << "no usable native window in this environment";

        ASSERT_TRUE (manager().startDragging (source, source, payloadOptions ("payload")));
    }

    void TearDown() override
    {
        endAnyActiveDrag();
        root.removeFromDesktop();
    }

    DragAndDropManager& manager() const { return *DragAndDropManager::getInstance(); }

    /** A screen position that lands on the target however the platform placed the window.

        Desktop::findComponentAt subtracts the native window's origin before hit-testing the root
        component, so building the point from that same origin cancels it out exactly. Deriving it
        from the component's screen bounds instead disagrees whenever the window is placed somewhere
        other than where it was asked to go - which is what happens under a window manager. */
    Point<float> overTarget() const
    {
        return root.getNativeComponent()->getBounds().getPosition().to<float>() + target.getBounds().getCenter();
    }

    Component root;
    ManagerTestTarget target;
    ManagerTestSource source;
};

TEST_F (DragAndDropManagerDropTests, MovingOverATargetEntersItThenMovesWithinIt)
{
    Desktop::getInstance()->handleGlobalMouseDrag (screenEventAt (overTarget()));

    EXPECT_EQ (1, target.dragEnterCount);
    EXPECT_EQ (1, target.interestQueryCount);
    EXPECT_EQ (static_cast<DragAndDropTarget*> (&target), manager().getCurrentDragTarget());

    // Still the same component under the cursor: a move, not a second enter.
    Desktop::getInstance()->handleGlobalMouseDrag (screenEventAt (overTarget() + Point<float> (5.0f, 5.0f)));

    EXPECT_EQ (1, target.dragEnterCount);
    EXPECT_EQ (1, target.dragMoveCount);
}

TEST_F (DragAndDropManagerDropTests, MovingOffATargetExitsItAndLeavesNoHover)
{
    Desktop::getInstance()->handleGlobalMouseDrag (screenEventAt (overTarget()));
    ASSERT_EQ (1, target.dragEnterCount);

    // Well outside every window: nothing is under the cursor, so the hover has to be released.
    Desktop::getInstance()->handleGlobalMouseDrag (screenEventAt ({ -5000.0f, -5000.0f }));

    EXPECT_EQ (1, target.dragExitCount);
    EXPECT_EQ (nullptr, manager().getCurrentDragTarget());
}

TEST_F (DragAndDropManagerDropTests, DroppingOverATargetPerformsACopy)
{
    Desktop::getInstance()->handleGlobalMouseDrag (screenEventAt (overTarget()));
    Desktop::getInstance()->handleGlobalMouseUp (screenEventAt (overTarget()));

    EXPECT_EQ (1, target.dropCount);
    EXPECT_EQ ("payload", target.lastDropData.getText());
    EXPECT_EQ (DragAndDropAction::copy, source.lastPerformed);
    EXPECT_FALSE (manager().isDragging());
}

TEST_F (DragAndDropManagerDropTests, ShiftAsksForAMoveInsteadOfACopy)
{
    const auto withShift = KeyModifiers (KeyModifiers::shiftMask);

    Desktop::getInstance()->handleGlobalMouseUp (screenEventAt (overTarget(), withShift));

    EXPECT_EQ (1, target.dropCount);
    EXPECT_EQ (DragAndDropAction::move, source.lastPerformed);
}

TEST_F (DragAndDropManagerDropTests, ATargetThatIsNotInterestedLeavesTheDropUnperformed)
{
    target.interested = false;

    Desktop::getInstance()->handleGlobalMouseUp (screenEventAt (overTarget()));

    // The cursor did resolve to the target - it just declined the payload, so nothing is performed.
    EXPECT_EQ (1, target.interestQueryCount);
    EXPECT_EQ (0, target.dropCount);
    EXPECT_EQ (DragAndDropAction::none, source.lastPerformed);
    EXPECT_FALSE (manager().isDragging());
}
