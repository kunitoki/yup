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
#include <gmock/gmock.h>

#include "../mocks/rive_gpu.h"

using namespace yup;

//==============================================================================
// ArtboardFile::AssetInfo
//==============================================================================

TEST (ArtboardFileAssetInfoTests, DefaultConstruction)
{
    ArtboardFile::AssetInfo info;

    EXPECT_TRUE (info.uniqueName.isEmpty());
    EXPECT_TRUE (info.extension.isEmpty());
}

TEST (ArtboardFileAssetInfoTests, FieldAssignment)
{
    ArtboardFile::AssetInfo info;
    info.uniqueName = "test_name";
    info.extension = "png";

    EXPECT_EQ (String ("test_name"), info.uniqueName);
    EXPECT_EQ (String ("png"), info.extension);
}

//==============================================================================
// Artboard
//==============================================================================

class ArtboardTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        artboard = std::make_unique<Artboard> ("testArtboard");
    }

    void TearDown() override
    {
        artboard.reset();
    }

    std::unique_ptr<Artboard> artboard;
};

TEST_F (ArtboardTests, DefaultConstruction)
{
    EXPECT_FALSE (artboard->isPaused());
    EXPECT_TRUE (artboard->isPausingWhenHidden());
    EXPECT_EQ (Artboard::Layout::contain, artboard->getLayout());
    EXPECT_EQ (Artboard::Alignment::center, artboard->getAlignment());
}

TEST_F (ArtboardTests, ConstructWithComponentId)
{
    Artboard a ("myCustomId");
    EXPECT_EQ (String ("myCustomId"), a.getComponentID());
}

TEST_F (ArtboardTests, SetAndGetLayout)
{
    artboard->setLayout (Artboard::Layout::fill);
    EXPECT_EQ (Artboard::Layout::fill, artboard->getLayout());

    artboard->setLayout (Artboard::Layout::cover);
    EXPECT_EQ (Artboard::Layout::cover, artboard->getLayout());

    artboard->setLayout (Artboard::Layout::fitWidth);
    EXPECT_EQ (Artboard::Layout::fitWidth, artboard->getLayout());

    artboard->setLayout (Artboard::Layout::fitHeight);
    EXPECT_EQ (Artboard::Layout::fitHeight, artboard->getLayout());

    artboard->setLayout (Artboard::Layout::none);
    EXPECT_EQ (Artboard::Layout::none, artboard->getLayout());

    artboard->setLayout (Artboard::Layout::scaleDown);
    EXPECT_EQ (Artboard::Layout::scaleDown, artboard->getLayout());

    artboard->setLayout (Artboard::Layout::layout);
    EXPECT_EQ (Artboard::Layout::layout, artboard->getLayout());
}

TEST_F (ArtboardTests, SetAndGetAlignment)
{
    artboard->setAlignment (Artboard::Alignment::topLeft);
    EXPECT_EQ (Artboard::Alignment::topLeft, artboard->getAlignment());

    artboard->setAlignment (Artboard::Alignment::topRight);
    EXPECT_EQ (Artboard::Alignment::topRight, artboard->getAlignment());

    artboard->setAlignment (Artboard::Alignment::centerLeft);
    EXPECT_EQ (Artboard::Alignment::centerLeft, artboard->getAlignment());

    artboard->setAlignment (Artboard::Alignment::centerRight);
    EXPECT_EQ (Artboard::Alignment::centerRight, artboard->getAlignment());

    artboard->setAlignment (Artboard::Alignment::bottomLeft);
    EXPECT_EQ (Artboard::Alignment::bottomLeft, artboard->getAlignment());

    artboard->setAlignment (Artboard::Alignment::bottomCenter);
    EXPECT_EQ (Artboard::Alignment::bottomCenter, artboard->getAlignment());

    artboard->setAlignment (Artboard::Alignment::bottomRight);
    EXPECT_EQ (Artboard::Alignment::bottomRight, artboard->getAlignment());
}

TEST_F (ArtboardTests, SetAndGetPaused)
{
    EXPECT_FALSE (artboard->isPaused());

    artboard->setPaused (true);
    EXPECT_TRUE (artboard->isPaused());

    artboard->setPaused (false);
    EXPECT_FALSE (artboard->isPaused());
}

TEST_F (ArtboardTests, ShouldPauseWhenHidden)
{
    EXPECT_TRUE (artboard->isPausingWhenHidden());

    artboard->shouldPauseWhenHidden (false);
    EXPECT_FALSE (artboard->isPausingWhenHidden());

    artboard->shouldPauseWhenHidden (true);
    EXPECT_TRUE (artboard->isPausingWhenHidden());
}

TEST_F (ArtboardTests, DurationSecondsReturnsZeroWithoutFile)
{
    EXPECT_FLOAT_EQ (0.0f, artboard->durationSeconds());
}

TEST_F (ArtboardTests, AdvanceAndApplyDoesNotCrashWithoutFile)
{
    EXPECT_NO_THROW (artboard->advanceAndApply (0.016f));
    EXPECT_NO_THROW (artboard->advanceAndApply (0.0f));
    EXPECT_NO_THROW (artboard->advanceAndApply (-1.0f));
}

TEST_F (ArtboardTests, HasBoolInputReturnsFalseWithoutFile)
{
    EXPECT_FALSE (artboard->hasBoolInput ("anyInput"));
    EXPECT_FALSE (artboard->hasBoolInput (String()));
}

TEST_F (ArtboardTests, HasNumberInputReturnsFalseWithoutFile)
{
    EXPECT_FALSE (artboard->hasNumberInput ("anyInput"));
    EXPECT_FALSE (artboard->hasNumberInput (String()));
}

TEST_F (ArtboardTests, HasTriggerInputReturnsFalseWithoutFile)
{
    EXPECT_FALSE (artboard->hasTriggerInput ("anyInput"));
    EXPECT_FALSE (artboard->hasTriggerInput (String()));
}

TEST_F (ArtboardTests, SetBoolInputDoesNotCrashWithoutFile)
{
    EXPECT_NO_THROW (artboard->setBoolInput ("test", true));
    EXPECT_NO_THROW (artboard->setBoolInput ("test", false));
}

TEST_F (ArtboardTests, SetNumberInputDoesNotCrashWithoutFile)
{
    EXPECT_NO_THROW (artboard->setNumberInput ("test", 42.0));
    EXPECT_NO_THROW (artboard->setNumberInput ("test", -1.0));
}

TEST_F (ArtboardTests, TriggerInputDoesNotCrashWithoutFile)
{
    EXPECT_NO_THROW (artboard->triggerInput ("test"));
    EXPECT_NO_THROW (artboard->triggerInput (String()));
}

TEST_F (ArtboardTests, GetAllInputsReturnsEmptyWithoutFile)
{
    EXPECT_NO_THROW (artboard->getAllInputs());
}

TEST_F (ArtboardTests, SetAllInputsDoesNotCrash)
{
    EXPECT_NO_THROW (artboard->setAllInputs (var()));
    EXPECT_NO_THROW (artboard->setAllInputs (var()));
}

TEST_F (ArtboardTests, SetInputDoesNotCrashWithoutFile)
{
    EXPECT_NO_THROW (artboard->setInput ("testInput", var (true)));
    EXPECT_NO_THROW (artboard->setInput ("testInput", var (42.0)));
    EXPECT_NO_THROW (artboard->setInput ("testInput", var()));
}

TEST_F (ArtboardTests, OnPropertyChangedCallbackCanBeSet)
{
    bool called = false;
    artboard->onPropertyChanged = [&] (Artboard&, const String&, const String&, const var&, const var&)
    {
        called = true;
    };

    EXPECT_FALSE (called);
}

TEST_F (ArtboardTests, PropertyChangedVirtualMethodCanBeCalled)
{
    EXPECT_NO_THROW (artboard->propertyChanged ("event", "property", var(), var (42)));
}

TEST_F (ArtboardTests, ClearDoesNotCrash)
{
    EXPECT_NO_THROW (artboard->clear());

    EXPECT_FALSE (artboard->isPaused());
    EXPECT_TRUE (artboard->isPausingWhenHidden());
    EXPECT_EQ (Artboard::Layout::contain, artboard->getLayout());
    EXPECT_EQ (Artboard::Alignment::center, artboard->getAlignment());
    EXPECT_FLOAT_EQ (0.0f, artboard->durationSeconds());
}

TEST_F (ArtboardTests, RefreshDisplayDoesNotCrashWithoutFile)
{
    EXPECT_NO_THROW (artboard->refreshDisplay (0.016));
    EXPECT_NO_THROW (artboard->refreshDisplay (0.0));
}

TEST_F (ArtboardTests, ClearResetsState)
{
    artboard->setPaused (true);
    artboard->shouldPauseWhenHidden (false);
    artboard->setLayout (Artboard::Layout::cover);
    artboard->setAlignment (Artboard::Alignment::topLeft);

    artboard->clear();

    EXPECT_FLOAT_EQ (0.0f, artboard->durationSeconds());
    EXPECT_FALSE (artboard->hasBoolInput ("test"));
}

TEST_F (ArtboardTests, LayoutEnumValuesAreDistinct)
{
    EXPECT_EQ (static_cast<int> (Artboard::Layout::fill), 0);
    EXPECT_NE (Artboard::Layout::fill, Artboard::Layout::contain);
    EXPECT_NE (Artboard::Layout::contain, Artboard::Layout::cover);
    EXPECT_NE (Artboard::Layout::cover, Artboard::Layout::fitWidth);
    EXPECT_NE (Artboard::Layout::fitWidth, Artboard::Layout::fitHeight);
    EXPECT_NE (Artboard::Layout::fitHeight, Artboard::Layout::none);
    EXPECT_NE (Artboard::Layout::none, Artboard::Layout::scaleDown);
    EXPECT_NE (Artboard::Layout::scaleDown, Artboard::Layout::layout);
}

TEST_F (ArtboardTests, AlignmentEnumValuesAreDistinct)
{
    EXPECT_EQ (static_cast<int> (Artboard::Alignment::topLeft), 0);
    EXPECT_NE (Artboard::Alignment::topLeft, Artboard::Alignment::topCenter);
    EXPECT_NE (Artboard::Alignment::topCenter, Artboard::Alignment::topRight);
    EXPECT_NE (Artboard::Alignment::topRight, Artboard::Alignment::centerLeft);
    EXPECT_NE (Artboard::Alignment::centerLeft, Artboard::Alignment::center);
    EXPECT_NE (Artboard::Alignment::center, Artboard::Alignment::centerRight);
    EXPECT_NE (Artboard::Alignment::centerRight, Artboard::Alignment::bottomLeft);
    EXPECT_NE (Artboard::Alignment::bottomLeft, Artboard::Alignment::bottomCenter);
    EXPECT_NE (Artboard::Alignment::bottomCenter, Artboard::Alignment::bottomRight);
}

//==============================================================================
// Artboard layout tests (require tests/data/rive/layout_test.riv)
//==============================================================================

namespace
{

// Names of the layout nodes expected in tests/data/rive/layout_test.riv
constexpr const char* kHeaderNodeName = "header";
constexpr const char* kPanelNodeName = "knob-panel";

const File getTestDataRiveDirectory()
{
    // Try source-relative path first (works on desktop builds)
    auto dir = File (__FILE__)
                   .getParentDirectory()
                   .getParentDirectory()
                   .getChildFile ("data")
                   .getChildFile ("rive");

    if (dir.exists())
        return dir;

    dir = File::getCurrentWorkingDirectory()
              .getParentDirectory()
              .getParentDirectory()
              .getParentDirectory()
              .getChildFile ("tests")
              .getChildFile ("data")
              .getChildFile ("rive");

    if (dir.exists())
        return dir;

    return File ("/data/rive");
}

} // namespace

class ArtboardLayoutTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto file = getTestDataRiveDirectory().getChildFile ("layout_test.riv");
        if (! file.existsAsFile())
        {
            GTEST_SKIP() << "Missing test asset: tests/data/rive/layout_test.riv";
            return;
        }

        auto result = ArtboardFile::load (file, factory);
        if (result.failed())
        {
            GTEST_SKIP() << "Failed to load test asset: " << result.getErrorMessage();
            return;
        }

        artboardFile = result.getValue();
        artboard = std::make_unique<Artboard> ("testArtboard", artboardFile);
        artboard->setLayout (Artboard::Layout::layout);
        artboard->setBounds (0.0f, 0.0f, 400.0f, 300.0f);

        // Layout bounds are animated over frames, so advance until they settle.
        for (int i = 0; i < 10; ++i)
            artboard->advanceAndApply (0.0f);
    }

    ::testing::NiceMock<MockRiveFactory> factory;
    std::shared_ptr<ArtboardFile> artboardFile;
    std::unique_ptr<Artboard> artboard;
};

//==============================================================================
// Model A — node bounds listeners
//==============================================================================

TEST_F (ArtboardLayoutTests, GetNodeBoundsReturnsEmptyForUnknownName)
{
    EXPECT_TRUE (artboard->getNodeBounds ("nonexistent").isEmpty());
}

TEST_F (ArtboardLayoutTests, GetNodeBoundsReturnsNonEmptyForKnownNode)
{
    const auto bounds = artboard->getNodeBounds (kHeaderNodeName);
    EXPECT_FALSE (bounds.isEmpty());
}

TEST_F (ArtboardLayoutTests, NodeBoundsListenerFiresOnReflow)
{
    int callCount = 0;
    Rectangle<float> lastBounds;

    artboard->setNodeBoundsListener (kHeaderNodeName,
                                     [&] (Artboard&, const String&, const ArtboardNode::Ptr& node)
                                     {
                                         ++callCount;
                                         lastBounds = node->getBounds();
                                     });

    // Resize the artboard, which triggers a layout reflow that moves the node.
    artboard->setBounds (0.0f, 0.0f, 600.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_GT (callCount, 0);
    EXPECT_FALSE (lastBounds.isEmpty());
}

TEST_F (ArtboardLayoutTests, NodeBoundsListenerDoesNotFireWhenUnchanged)
{
    int callCount = 0;

    artboard->setNodeBoundsListener (kHeaderNodeName,
                                     [&] (Artboard&, const String&, const ArtboardNode::Ptr&)
                                     {
                                         ++callCount;
                                     });

    for (int i = 0; i < 5; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_EQ (0, callCount);
}

TEST_F (ArtboardLayoutTests, EmptyCallbackRemovesListener)
{
    int callCount = 0;

    artboard->setNodeBoundsListener (kHeaderNodeName,
                                     [&] (Artboard&, const String&, const ArtboardNode::Ptr&)
                                     {
                                         ++callCount;
                                     });

    artboard->setNodeBoundsListener (kHeaderNodeName, {});

    artboard->setBounds (0.0f, 0.0f, 600.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_EQ (0, callCount);
}

TEST_F (ArtboardLayoutTests, ClearNodeBoundsListenerStopsNotifications)
{
    int callCount = 0;

    artboard->setNodeBoundsListener (kHeaderNodeName,
                                     [&] (Artboard&, const String&, const ArtboardNode::Ptr&)
                                     {
                                         ++callCount;
                                     });

    artboard->clearNodeBoundsListener (kHeaderNodeName);

    artboard->setBounds (0.0f, 0.0f, 600.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_EQ (0, callCount);
}

TEST_F (ArtboardLayoutTests, ClearAllNodeBoundsListenersRemovesAll)
{
    int callCount = 0;

    artboard->setNodeBoundsListener (kHeaderNodeName,
                                     [&] (Artboard&, const String&, const ArtboardNode::Ptr&)
                                     {
                                         ++callCount;
                                     });

    artboard->setNodeBoundsListener (kPanelNodeName,
                                     [&] (Artboard&, const String&, const ArtboardNode::Ptr&)
                                     {
                                         ++callCount;
                                     });

    artboard->clearAllNodeBoundsListeners();

    artboard->setBounds (0.0f, 0.0f, 600.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_EQ (0, callCount);
}

//==============================================================================
// Model B — attached components
//==============================================================================

TEST_F (ArtboardLayoutTests, AttachComponentToNodeReturnsFalseForUnknownNode)
{
    Component component ("test");

    EXPECT_FALSE (artboard->attachComponentToNode ("nonexistent", &component));
}

TEST_F (ArtboardLayoutTests, AttachComponentToNodeReturnsFalseForNullComponent)
{
    EXPECT_FALSE (artboard->attachComponentToNode (kHeaderNodeName, nullptr));
}

TEST_F (ArtboardLayoutTests, AttachComponentToNodePositionsComponentImmediately)
{
    Component component ("test");

    EXPECT_TRUE (artboard->attachComponentToNode (kPanelNodeName, &component));

    const auto expected = artboard->getNodeBounds (kPanelNodeName);
    const auto actual = component.getBounds();

    EXPECT_FLOAT_EQ (expected.getX(), actual.getX());
    EXPECT_FLOAT_EQ (expected.getY(), actual.getY());
    EXPECT_FLOAT_EQ (expected.getWidth(), actual.getWidth());
    EXPECT_FLOAT_EQ (expected.getHeight(), actual.getHeight());
}

TEST_F (ArtboardLayoutTests, AttachedComponentFollowsNodeOnReflow)
{
    Component component ("test");

    ASSERT_TRUE (artboard->attachComponentToNode (kPanelNodeName, &component));

    artboard->setBounds (0.0f, 0.0f, 600.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    const auto nodeBounds = artboard->getNodeBounds (kPanelNodeName);
    const auto componentBounds = component.getBounds();

    EXPECT_FLOAT_EQ (nodeBounds.getX(), componentBounds.getX());
    EXPECT_FLOAT_EQ (nodeBounds.getY(), componentBounds.getY());
    EXPECT_FLOAT_EQ (nodeBounds.getWidth(), componentBounds.getWidth());
    EXPECT_FLOAT_EQ (nodeBounds.getHeight(), componentBounds.getHeight());
}

TEST_F (ArtboardLayoutTests, DetachComponentFromNodeStopsUpdates)
{
    Component component ("test");

    ASSERT_TRUE (artboard->attachComponentToNode (kPanelNodeName, &component));

    Component other ("other");
    EXPECT_FALSE (artboard->detachComponentFromNode (kPanelNodeName, &other));

    EXPECT_TRUE (artboard->detachComponentFromNode (kPanelNodeName, &component));

    const auto boundsBefore = component.getBounds();

    artboard->setBounds (0.0f, 0.0f, 600.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_TRUE (component.getBounds() == boundsBefore);
}

TEST_F (ArtboardLayoutTests, DetachAllComponentsStopsUpdates)
{
    Component header ("headerComponent");
    Component panel ("panelComponent");

    ASSERT_TRUE (artboard->attachComponentToNode (kHeaderNodeName, &header));
    ASSERT_TRUE (artboard->attachComponentToNode (kPanelNodeName, &panel));

    artboard->detachAllComponents();

    const auto headerBoundsBefore = header.getBounds();
    const auto panelBoundsBefore = panel.getBounds();

    artboard->setBounds (0.0f, 0.0f, 600.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_TRUE (header.getBounds() == headerBoundsBefore);
    EXPECT_TRUE (panel.getBounds() == panelBoundsBefore);
}

TEST_F (ArtboardLayoutTests, AttachedComponentTracksPositionWithoutResize)
{
    Component component ("test");

    yup::Artboard::NodeAttachmentOptions options;
    options.mode = yup::Artboard::NodeAttachmentOptions::Mode::trackPosition;

    ASSERT_TRUE (artboard->attachComponentToNode (kPanelNodeName, &component, options));

    // Give the component an explicit size; only its position should follow the node.
    component.setBounds (0.0f, 0.0f, 50.0f, 30.0f);

    artboard->setBounds (0.0f, 0.0f, 600.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    // The size must never be touched by the attachment...
    EXPECT_FLOAT_EQ (50.0f, component.getWidth());
    EXPECT_FLOAT_EQ (30.0f, component.getHeight());

    // ...and the top-left must follow the node's new bounds origin.
    const auto nodeBounds = artboard->getNodeBounds (kPanelNodeName);
    EXPECT_FLOAT_EQ (nodeBounds.getX(), component.getX());
    EXPECT_FLOAT_EQ (nodeBounds.getY(), component.getY());
}

TEST_F (ArtboardLayoutTests, TrackPositionWithCenteredPivot)
{
    Component component ("test");

    yup::Artboard::NodeAttachmentOptions options;
    options.mode = yup::Artboard::NodeAttachmentOptions::Mode::trackPosition;
    options.pivot = yup::Justification::center;

    ASSERT_TRUE (artboard->attachComponentToNode (kPanelNodeName, &component, options));

    // Give the component an explicit size; its center should follow the node's
    // top-left (the default anchor).
    component.setBounds (0.0f, 0.0f, 50.0f, 30.0f);

    artboard->setBounds (0.0f, 0.0f, 600.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_FLOAT_EQ (50.0f, component.getWidth());
    EXPECT_FLOAT_EQ (30.0f, component.getHeight());

    const auto nodeBounds = artboard->getNodeBounds (kPanelNodeName);
    EXPECT_FLOAT_EQ (nodeBounds.getX(), component.getBounds().getCenterX());
    EXPECT_FLOAT_EQ (nodeBounds.getY(), component.getBounds().getCenterY());
}

TEST_F (ArtboardLayoutTests, TrackPositionWithCenteredAnchor)
{
    Component component ("test");

    yup::Artboard::NodeAttachmentOptions options;
    options.mode = yup::Artboard::NodeAttachmentOptions::Mode::trackPosition;
    options.anchor = yup::Justification::center;

    ASSERT_TRUE (artboard->attachComponentToNode (kPanelNodeName, &component, options));

    // With the default pivot (top-left) the component's top-left corner should
    // sit on the center of the node's bounds.
    component.setBounds (0.0f, 0.0f, 50.0f, 30.0f);

    artboard->setBounds (0.0f, 0.0f, 600.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_FLOAT_EQ (50.0f, component.getWidth());
    EXPECT_FLOAT_EQ (30.0f, component.getHeight());

    const auto nodeBounds = artboard->getNodeBounds (kPanelNodeName);
    EXPECT_FLOAT_EQ (nodeBounds.getCenterX(), component.getX());
    EXPECT_FLOAT_EQ (nodeBounds.getCenterY(), component.getY());
}

TEST_F (ArtboardLayoutTests, TrackPositionWithPivotCenterAndBottomRightAnchor)
{
    Component component ("test");

    yup::Artboard::NodeAttachmentOptions options;
    options.mode = yup::Artboard::NodeAttachmentOptions::Mode::trackPosition;
    options.pivot = yup::Justification::center;
    options.anchor = yup::Justification::bottomRight;

    ASSERT_TRUE (artboard->attachComponentToNode (kPanelNodeName, &component, options));

    component.setBounds (0.0f, 0.0f, 50.0f, 30.0f);

    artboard->setBounds (0.0f, 0.0f, 600.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    // The component's center (pivot) should sit on the node bounds' bottom-right.
    const auto nodeBounds = artboard->getNodeBounds (kPanelNodeName);
    EXPECT_FLOAT_EQ (nodeBounds.getRight(), component.getBounds().getCenterX());
    EXPECT_FLOAT_EQ (nodeBounds.getBottom(), component.getBounds().getCenterY());
}

TEST_F (ArtboardLayoutTests, TrackPositionAnchorSupportsAllJustifications)
{
    struct Case
    {
        yup::Justification anchor;
        float xFactor;
        float yFactor;
    };

    const Case cases[] = {
        { yup::Justification::topLeft,       0.0f, 0.0f },
        { yup::Justification::centerTop,     0.5f, 0.0f },
        { yup::Justification::topRight,      1.0f, 0.0f },
        { yup::Justification::centerLeft,    0.0f, 0.5f },
        { yup::Justification::center,        0.5f, 0.5f },
        { yup::Justification::centerRight,   1.0f, 0.5f },
        { yup::Justification::bottomLeft,    0.0f, 1.0f },
        { yup::Justification::centerBottom,  0.5f, 1.0f },
        { yup::Justification::bottomRight,   1.0f, 1.0f },
    };

    for (const auto& testCase : cases)
    {
        Component component ("test");

        yup::Artboard::NodeAttachmentOptions options;
        options.mode = yup::Artboard::NodeAttachmentOptions::Mode::trackPosition;
        options.anchor = testCase.anchor;

        ASSERT_TRUE (artboard->attachComponentToNode (kPanelNodeName, &component, options));

        // With the default pivot (top-left) the component's top-left corner
        // should land on the requested point of the node bounds.
        component.setBounds (0.0f, 0.0f, 50.0f, 30.0f);

        artboard->setBounds (0.0f, 0.0f, 600.0f, 400.0f);

        for (int i = 0; i < 10; ++i)
            artboard->advanceAndApply (0.0f);

        const auto nodeBounds = artboard->getNodeBounds (kPanelNodeName);
        const auto expectedX = nodeBounds.getX() + nodeBounds.getWidth() * testCase.xFactor;
        const auto expectedY = nodeBounds.getY() + nodeBounds.getHeight() * testCase.yFactor;

        EXPECT_FLOAT_EQ (expectedX, component.getX()) << "anchor x factor " << testCase.xFactor;
        EXPECT_FLOAT_EQ (expectedY, component.getY()) << "anchor y factor " << testCase.yFactor;
    }
}

TEST_F (ArtboardLayoutTests, ApplyTransformKeepsBoundsForUnrotatedNode)
{
    Component component ("test");

    yup::Artboard::NodeAttachmentOptions options;
    options.applyTransform = true;

    ASSERT_TRUE (artboard->attachComponentToNode (kPanelNodeName, &component, options));

    artboard->setBounds (0.0f, 0.0f, 600.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    // For an unrotated node the applied transform is the identity, so the bounds
    // must still track the node exactly and the transform must be identity.
    EXPECT_TRUE (component.getTransform().isIdentity());

    const auto nodeBounds = artboard->getNodeBounds (kPanelNodeName);
    EXPECT_FLOAT_EQ (nodeBounds.getX(), component.getX());
    EXPECT_FLOAT_EQ (nodeBounds.getY(), component.getY());
    EXPECT_FLOAT_EQ (nodeBounds.getWidth(), component.getWidth());
    EXPECT_FLOAT_EQ (nodeBounds.getHeight(), component.getHeight());
}

TEST_F (ArtboardLayoutTests, ModelAAndModelBOnSameNodeBothUpdate)
{
    Component component ("test");
    int callCount = 0;

    ASSERT_TRUE (artboard->attachComponentToNode (kHeaderNodeName, &component));

    artboard->setNodeBoundsListener (kHeaderNodeName,
                                     [&] (Artboard&, const String&, const ArtboardNode::Ptr&)
                                     {
                                         ++callCount;
                                     });

    artboard->setBounds (0.0f, 0.0f, 600.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_GT (callCount, 0);

    const auto nodeBounds = artboard->getNodeBounds (kHeaderNodeName);
    const auto componentBounds = component.getBounds();

    EXPECT_FLOAT_EQ (nodeBounds.getX(), componentBounds.getX());
    EXPECT_FLOAT_EQ (nodeBounds.getY(), componentBounds.getY());
    EXPECT_FLOAT_EQ (nodeBounds.getWidth(), componentBounds.getWidth());
    EXPECT_FLOAT_EQ (nodeBounds.getHeight(), componentBounds.getHeight());
}

TEST_F (ArtboardLayoutTests, NodeBoundsListenerReceivesStableCachedHandle)
{
    int callCount = 0;
    ArtboardNode::Ptr firstHandle;

    artboard->setNodeBoundsListener (kHeaderNodeName,
                                     [&] (Artboard&, const String&, const ArtboardNode::Ptr& node)
                                     {
                                         ++callCount;
                                         if (firstHandle == nullptr)
                                             firstHandle = node;
                                         else
                                             EXPECT_EQ (firstHandle.get(), node.get());
                                     });

    artboard->setBounds (0.0f, 0.0f, 600.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_GT (callCount, 0);

    ASSERT_NE (nullptr, firstHandle.get());
    EXPECT_TRUE (firstHandle->isValid());
    EXPECT_FALSE (firstHandle->getBounds().isEmpty());

    // The callback receives the same cached handle that findNode hands out.
    EXPECT_EQ (firstHandle.get(), artboard->findNode (kHeaderNodeName).get());
}

TEST_F (ArtboardLayoutTests, ClearInvalidatesNodeLookup)
{
    Component component ("test");

    artboard->clear();

    EXPECT_TRUE (artboard->getNodeBounds (kHeaderNodeName).isEmpty());
    EXPECT_EQ (nullptr, artboard->findNode (kHeaderNodeName).get());
    EXPECT_FALSE (artboard->attachComponentToNode (kHeaderNodeName, &component));
}

//==============================================================================
// Artboard::setFile named artboard selection (requires tests/data/rive/responsive-sliders.riv)
//==============================================================================

class ArtboardSetFileNamedTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto file = getTestDataRiveDirectory().getChildFile ("responsive-sliders.riv");
        if (! file.existsAsFile())
        {
            GTEST_SKIP() << "Missing test asset: tests/data/rive/responsive-sliders.riv";
            return;
        }

        auto result = ArtboardFile::load (file, factory);
        if (result.failed())
        {
            GTEST_SKIP() << "Failed to load test asset: " << result.getErrorMessage();
            return;
        }

        artboardFile = result.getValue();
        artboard = std::make_unique<Artboard> ("testArtboard");
    }

    ::testing::NiceMock<MockRiveFactory> factory;
    std::shared_ptr<ArtboardFile> artboardFile;
    std::unique_ptr<Artboard> artboard;
};

TEST_F (ArtboardSetFileNamedTests, SetFileWithoutNameLoadsDefaultArtboard)
{
    artboard->setFile (artboardFile);

    EXPECT_EQ (String ("Main"), artboard->getViewModelName());
}

TEST_F (ArtboardSetFileNamedTests, SetFileWithNameLoadsNamedArtboard)
{
    artboard->setFile (artboardFile, "Slider_instance");

    EXPECT_EQ (String ("Slider_instance"), artboard->getViewModelName());
}

TEST_F (ArtboardSetFileNamedTests, SetFileWithUnknownNameLeavesNothingLoaded)
{
    artboard->setFile (artboardFile, "DoesNotExist");

    EXPECT_FLOAT_EQ (0.0f, artboard->durationSeconds());
}

//==============================================================================
// ArtboardNode
//==============================================================================

TEST_F (ArtboardTests, FindNodeReturnsNullWithoutFile)
{
    EXPECT_EQ (nullptr, artboard->findNode (kHeaderNodeName).get());
}

TEST_F (ArtboardLayoutTests, FindNodeReturnsNullForUnknownName)
{
    EXPECT_EQ (nullptr, artboard->findNode ("nonexistent").get());
}

TEST_F (ArtboardLayoutTests, FindNodeReturnsValidHandleForKnownNode)
{
    auto node = artboard->findNode (kHeaderNodeName);
    ASSERT_NE (nullptr, node.get());

    EXPECT_TRUE (node->isValid());
    EXPECT_EQ (String (kHeaderNodeName), node->getName());
    EXPECT_NE (0, node->getTypeKey());
    EXPECT_FALSE (node->getTypeName().isEmpty());
}

TEST_F (ArtboardLayoutTests, FindNodeWorksOnConstArtboard)
{
    const auto& constArtboard = *artboard;

    auto node = constArtboard.findNode (kHeaderNodeName);
    ASSERT_NE (nullptr, node.get());
    EXPECT_TRUE (node->isValid());
}

TEST_F (ArtboardLayoutTests, NodeBoundsMatchGetNodeBounds)
{
    auto node = artboard->findNode (kHeaderNodeName);
    ASSERT_NE (nullptr, node.get());

    const auto nodeBounds = node->getBounds();
    const auto artboardBounds = artboard->getNodeBounds (kHeaderNodeName);

    EXPECT_FALSE (nodeBounds.isEmpty());
    EXPECT_FLOAT_EQ (artboardBounds.getX(), nodeBounds.getX());
    EXPECT_FLOAT_EQ (artboardBounds.getY(), nodeBounds.getY());
    EXPECT_FLOAT_EQ (artboardBounds.getWidth(), nodeBounds.getWidth());
    EXPECT_FLOAT_EQ (artboardBounds.getHeight(), nodeBounds.getHeight());
}

TEST_F (ArtboardLayoutTests, IsLayoutMatchesLayoutTypeKey)
{
    auto node = artboard->findNode (kHeaderNodeName);
    ASSERT_NE (nullptr, node.get());

    EXPECT_EQ (node->getTypeKey() == rive::LayoutComponentBase::typeKey, node->isLayout());
}

TEST_F (ArtboardLayoutTests, ParentAndChildrenTraversal)
{
    auto node = artboard->findNode (kHeaderNodeName);
    ASSERT_NE (nullptr, node.get());

    auto parent = node->getParent();
    ASSERT_NE (nullptr, parent.get());
    EXPECT_TRUE (parent->isValid());
    EXPECT_EQ (String ("Artboard"), parent->getTypeName());

    auto siblings = parent->getChildren();
    bool found = false;
    for (const auto& sibling : siblings)
    {
        if (sibling != nullptr && sibling->getName() == String (kHeaderNodeName))
        {
            found = true;
            break;
        }
    }

    EXPECT_TRUE (found);
}

TEST_F (ArtboardLayoutTests, HandleRemainsUsableAfterCopy)
{
    auto node = artboard->findNode (kHeaderNodeName);
    ASSERT_NE (nullptr, node.get());

    auto copy = node;
    EXPECT_EQ (node.get(), copy.get());
    EXPECT_TRUE (copy->isValid());
    EXPECT_EQ (String (kHeaderNodeName), copy->getName());
}

TEST_F (ArtboardLayoutTests, ClearInvalidatesHandles)
{
    auto node = artboard->findNode (kHeaderNodeName);
    ASSERT_NE (nullptr, node.get());
    EXPECT_TRUE (node->isValid());

    artboard->clear();

    EXPECT_FALSE (node->isValid());
    EXPECT_TRUE (node->getName().isEmpty());
    EXPECT_EQ (0, node->getTypeKey());
    EXPECT_TRUE (node->getTypeName().isEmpty());
    EXPECT_FALSE (node->isLayout());
    EXPECT_TRUE (node->getBounds().isEmpty());
    EXPECT_TRUE (node->getLocalTransform().isIdentity());
    EXPECT_TRUE (node->getWorldTransform().isIdentity());
    EXPECT_EQ (nullptr, node->getParent().get());
    EXPECT_TRUE (node->getChildren().isEmpty());
}

TEST_F (ArtboardLayoutTests, NodeTransformsAreFinite)
{
    auto node = artboard->findNode (kHeaderNodeName);
    ASSERT_NE (nullptr, node.get());

    const auto local = node->getLocalTransform();
    EXPECT_TRUE (std::isfinite (local.getScaleX()));
    EXPECT_TRUE (std::isfinite (local.getShearX()));
    EXPECT_TRUE (std::isfinite (local.getTranslateX()));
    EXPECT_TRUE (std::isfinite (local.getShearY()));
    EXPECT_TRUE (std::isfinite (local.getScaleY()));
    EXPECT_TRUE (std::isfinite (local.getTranslateY()));

    const auto world = node->getWorldTransform();
    EXPECT_TRUE (std::isfinite (world.getScaleX()));
    EXPECT_TRUE (std::isfinite (world.getShearX()));
    EXPECT_TRUE (std::isfinite (world.getTranslateX()));
    EXPECT_TRUE (std::isfinite (world.getShearY()));
    EXPECT_TRUE (std::isfinite (world.getScaleY()));
    EXPECT_TRUE (std::isfinite (world.getTranslateY()));
}

//==============================================================================

//==============================================================================
// Artboard and ArtboardNode against tests/data/rive/game-animation.riv
//
// game-animation.riv's default artboard ("New Artboard") contains a single
// named nested artboard node ("7ZRGY5S 5"); the animated character parts live
// inside that nested instance and are not reachable through the outer
// artboard's name-based APIs. These tests drive Artboard/ArtboardNode through
// the artboard root and the nested node: identity, bounds/transform
// consistency, handle caching, epoch invalidation (clear / setFile / destroy),
// bounds listeners and attached components. Resizing changes the view (fit)
// transform, which deterministically moves both the root and the nested node.
//==============================================================================

namespace
{
constexpr const char* kGameRootArtboardNodeName = "New Artboard";
constexpr const char* kGameNestedArtboardNodeName = "7ZRGY5S 5";
} // namespace

class ArtboardGameAnimationTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto file = getTestDataRiveDirectory().getChildFile ("game-animation.riv");
        if (! file.existsAsFile())
        {
            GTEST_SKIP() << "Missing test asset: tests/data/rive/game-animation.riv";
            return;
        }

        auto result = ArtboardFile::load (file, factory);
        if (result.failed())
        {
            GTEST_SKIP() << "Failed to load test asset: " << result.getErrorMessage();
            return;
        }

        artboardFile = result.getValue();
        artboard = std::make_unique<Artboard> ("testArtboard", artboardFile);
        artboard->setLayout (Artboard::Layout::contain);
        artboard->setBounds (0.0f, 0.0f, 500.0f, 500.0f);

        for (int i = 0; i < 5; ++i)
            artboard->advanceAndApply (0.0f);
    }

    ::testing::NiceMock<MockRiveFactory> factory;
    std::shared_ptr<ArtboardFile> artboardFile;
    std::unique_ptr<Artboard> artboard;
};

//==============================================================================
// Artboard lifecycle with a loaded file
//==============================================================================

TEST_F (ArtboardGameAnimationTests, LoadingFileProvidesNodeQueries)
{
    EXPECT_FALSE (artboard->getNodeBounds (kGameRootArtboardNodeName).isEmpty());
    EXPECT_FALSE (artboard->getNodeBounds (kGameNestedArtboardNodeName).isEmpty());

    auto root = artboard->findNode (kGameRootArtboardNodeName);
    ASSERT_NE (nullptr, root.get());
    EXPECT_TRUE (root->isValid());

    auto nested = artboard->findNode (kGameNestedArtboardNodeName);
    ASSERT_NE (nullptr, nested.get());
    EXPECT_TRUE (nested->isValid());
}

TEST_F (ArtboardGameAnimationTests, UnknownNodesAreNotResolved)
{
    Component component ("test");

    EXPECT_TRUE (artboard->getNodeBounds ("nonexistent").isEmpty());
    EXPECT_EQ (nullptr, artboard->findNode ("nonexistent").get());
    EXPECT_FALSE (artboard->attachComponentToNode ("nonexistent", &component));
}

TEST_F (ArtboardGameAnimationTests, InputQueriesWithLoadedFileDoNotThrow)
{
    // game-animation.riv's state machine exposes no direct bool/number/trigger
    // inputs, so lookups must be false and writes must be safe no-ops.
    EXPECT_FALSE (artboard->hasBoolInput ("nonexistentInput"));
    EXPECT_FALSE (artboard->hasNumberInput ("nonexistentInput"));
    EXPECT_FALSE (artboard->hasTriggerInput ("nonexistentInput"));

    EXPECT_NO_THROW (artboard->setBoolInput ("nonexistentInput", true));
    EXPECT_NO_THROW (artboard->setNumberInput ("nonexistentInput", 42.0));
    EXPECT_NO_THROW (artboard->triggerInput ("nonexistentInput"));
    EXPECT_NO_THROW (artboard->setInput ("nonexistentInput", var (true)));
    EXPECT_NO_THROW (artboard->setAllInputs (var()));
    EXPECT_NO_THROW (artboard->getAllInputs());
}

TEST_F (ArtboardGameAnimationTests, AdvanceAndApplyRunsTheLoadedScene)
{
    for (int frame = 0; frame < 60; ++frame)
    {
        EXPECT_NO_THROW (artboard->advanceAndApply (0.016f));
        EXPECT_NO_THROW (artboard->refreshDisplay (0.016));
    }
}

TEST_F (ArtboardGameAnimationTests, ClearResetsTheLoadedFile)
{
    artboard->clear();

    EXPECT_TRUE (artboard->getNodeBounds (kGameRootArtboardNodeName).isEmpty());
    EXPECT_TRUE (artboard->getNodeBounds (kGameNestedArtboardNodeName).isEmpty());
    EXPECT_EQ (nullptr, artboard->findNode (kGameRootArtboardNodeName).get());
    EXPECT_EQ (nullptr, artboard->findNode (kGameNestedArtboardNodeName).get());
    EXPECT_EQ (Artboard::Layout::contain, artboard->getLayout()); // layout survives clear
}

TEST_F (ArtboardGameAnimationTests, ReloadingTheFileRestoresTheScene)
{
    artboard->setFile (artboardFile);

    auto node = artboard->findNode (kGameNestedArtboardNodeName);
    ASSERT_NE (nullptr, node.get());
    EXPECT_TRUE (node->isValid());
    EXPECT_FALSE (artboard->getNodeBounds (kGameNestedArtboardNodeName).isEmpty());
}

//==============================================================================
// ArtboardNode handles - root artboard and nested artboard nodes
//==============================================================================

TEST_F (ArtboardGameAnimationTests, NodeHandlesReportTheirIdentity)
{
    auto root = artboard->findNode (kGameRootArtboardNodeName);
    ASSERT_NE (nullptr, root.get());
    EXPECT_TRUE (root->isValid());
    EXPECT_EQ (String (kGameRootArtboardNodeName), root->getName());
    EXPECT_EQ (static_cast<int> (rive::ArtboardBase::typeKey), static_cast<int> (root->getTypeKey()));
    EXPECT_EQ (String ("Artboard"), root->getTypeName());
    EXPECT_TRUE (root->isLayout());

    auto nested = artboard->findNode (kGameNestedArtboardNodeName);
    ASSERT_NE (nullptr, nested.get());
    EXPECT_TRUE (nested->isValid());
    EXPECT_EQ (String (kGameNestedArtboardNodeName), nested->getName());
    EXPECT_NE (0, nested->getTypeKey());
    EXPECT_EQ (String ("NestedArtboard"), nested->getTypeName());
    EXPECT_FALSE (nested->isLayout());
}

TEST_F (ArtboardGameAnimationTests, NodeBoundsAreConsistentWithGetNodeBounds)
{
    for (const auto* nodeName : { kGameRootArtboardNodeName, kGameNestedArtboardNodeName })
    {
        auto node = artboard->findNode (nodeName);
        ASSERT_NE (nullptr, node.get()) << nodeName;

        const auto nodeBounds = node->getBounds();
        const auto artboardBounds = artboard->getNodeBounds (nodeName);

        EXPECT_FALSE (nodeBounds.isEmpty()) << nodeName;
        EXPECT_FLOAT_EQ (artboardBounds.getX(), nodeBounds.getX()) << nodeName;
        EXPECT_FLOAT_EQ (artboardBounds.getY(), nodeBounds.getY()) << nodeName;
        EXPECT_FLOAT_EQ (artboardBounds.getWidth(), nodeBounds.getWidth()) << nodeName;
        EXPECT_FLOAT_EQ (artboardBounds.getHeight(), nodeBounds.getHeight()) << nodeName;
    }
}

TEST_F (ArtboardGameAnimationTests, NodeTransformsAreFinite)
{
    const auto checkFinite = [] (const AffineTransform& transform, const char* label)
    {
        EXPECT_TRUE (std::isfinite (transform.getScaleX())) << label;
        EXPECT_TRUE (std::isfinite (transform.getShearX())) << label;
        EXPECT_TRUE (std::isfinite (transform.getTranslateX())) << label;
        EXPECT_TRUE (std::isfinite (transform.getShearY())) << label;
        EXPECT_TRUE (std::isfinite (transform.getScaleY())) << label;
        EXPECT_TRUE (std::isfinite (transform.getTranslateY())) << label;
    };

    for (const auto* nodeName : { kGameRootArtboardNodeName, kGameNestedArtboardNodeName })
    {
        auto node = artboard->findNode (nodeName);
        ASSERT_NE (nullptr, node.get()) << nodeName;

        checkFinite (node->getLocalTransform(), nodeName);
        checkFinite (node->getWorldTransform(), nodeName);
        checkFinite (node->getViewTransform(), nodeName);
    }
}

TEST_F (ArtboardGameAnimationTests, NestedNodeHierarchyRelatesToRoot)
{
    auto nested = artboard->findNode (kGameNestedArtboardNodeName);
    ASSERT_NE (nullptr, nested.get());

    auto parent = nested->getParent();
    ASSERT_NE (nullptr, parent.get());
    EXPECT_TRUE (parent->isValid());
    EXPECT_EQ (String (kGameRootArtboardNodeName), parent->getName());

    // Whatever children the nested artboard node has (e.g. nested inputs) must
    // be valid handles whose parent resolves back to the nested node.
    for (const auto& child : nested->getChildren())
    {
        ASSERT_NE (nullptr, child.get());
        EXPECT_TRUE (child->isValid());
        ASSERT_NE (nullptr, child->getParent().get());
        EXPECT_EQ (String (kGameNestedArtboardNodeName), child->getParent()->getName());
    }

    // ...and the root's children include the nested node.
    auto root = artboard->findNode (kGameRootArtboardNodeName);
    ASSERT_NE (nullptr, root.get());

    bool foundNested = false;
    for (const auto& child : root->getChildren())
    {
        ASSERT_NE (nullptr, child.get());
        if (child->getName() == String (kGameNestedArtboardNodeName))
        {
            foundNested = true;
            break;
        }
    }

    EXPECT_TRUE (foundNested);
}

TEST_F (ArtboardGameAnimationTests, FindNodeReturnsTheCachedHandle)
{
    for (const auto* nodeName : { kGameRootArtboardNodeName, kGameNestedArtboardNodeName })
    {
        auto first = artboard->findNode (nodeName);
        auto second = artboard->findNode (nodeName);

        ASSERT_NE (nullptr, first.get()) << nodeName;
        EXPECT_EQ (first.get(), second.get()) << nodeName;
    }
}

TEST_F (ArtboardGameAnimationTests, HandleIsRefcounted)
{
    auto node = artboard->findNode (kGameNestedArtboardNodeName);
    ASSERT_NE (nullptr, node.get());

    ArtboardNode::Ptr copy = node;
    EXPECT_EQ (node.get(), copy.get());
    EXPECT_TRUE (copy->isValid());

    copy = nullptr;
    EXPECT_TRUE (node->isValid());
}

TEST_F (ArtboardGameAnimationTests, FindNodeWorksOnConstArtboard)
{
    const auto& constArtboard = *artboard;
    auto node = constArtboard.findNode (kGameNestedArtboardNodeName);
    ASSERT_NE (nullptr, node.get());
    EXPECT_TRUE (node->isValid());
}

TEST_F (ArtboardGameAnimationTests, ClearInvalidatesOutstandingHandles)
{
    ArtboardNode::Ptr rootHandle = artboard->findNode (kGameRootArtboardNodeName);
    ArtboardNode::Ptr nestedHandle = artboard->findNode (kGameNestedArtboardNodeName);

    ASSERT_NE (nullptr, rootHandle.get());
    ASSERT_NE (nullptr, nestedHandle.get());
    EXPECT_TRUE (rootHandle->isValid());
    EXPECT_TRUE (nestedHandle->isValid());

    artboard->clear();

    for (const auto& node : { rootHandle, nestedHandle })
    {
        EXPECT_FALSE (node->isValid());
        EXPECT_TRUE (node->getName().isEmpty());
        EXPECT_EQ (0, node->getTypeKey());
        EXPECT_TRUE (node->getTypeName().isEmpty());
        EXPECT_FALSE (node->isLayout());
        EXPECT_TRUE (node->getBounds().isEmpty());
        EXPECT_TRUE (node->getLocalTransform().isIdentity());
        EXPECT_TRUE (node->getWorldTransform().isIdentity());
        EXPECT_TRUE (node->getViewTransform().isIdentity());
        EXPECT_EQ (nullptr, node->getParent().get());
        EXPECT_TRUE (node->getChildren().isEmpty());
    }
}

TEST_F (ArtboardGameAnimationTests, SetFileInvalidatesHandlesAndRefreshesCache)
{
    auto oldHandle = artboard->findNode (kGameNestedArtboardNodeName);
    ASSERT_NE (nullptr, oldHandle.get());
    EXPECT_TRUE (oldHandle->isValid());

    artboard->setFile (artboardFile);

    EXPECT_FALSE (oldHandle->isValid());

    auto newHandle = artboard->findNode (kGameNestedArtboardNodeName);
    ASSERT_NE (nullptr, newHandle.get());
    EXPECT_TRUE (newHandle->isValid());
    EXPECT_NE (oldHandle.get(), newHandle.get());
}

TEST_F (ArtboardGameAnimationTests, DestroyingTheArtboardInvalidatesHandles)
{
    ArtboardNode::Ptr handle;

    {
        Artboard temp ("temp", artboardFile);
        temp.setLayout (Artboard::Layout::contain);
        temp.setBounds (0.0f, 0.0f, 500.0f, 500.0f);

        for (int i = 0; i < 5; ++i)
            temp.advanceAndApply (0.0f);

        handle = temp.findNode (kGameNestedArtboardNodeName);
        ASSERT_NE (nullptr, handle.get());
        EXPECT_TRUE (handle->isValid());
    }

    EXPECT_FALSE (handle->isValid());
    EXPECT_TRUE (handle->getName().isEmpty());
    EXPECT_EQ (0, handle->getTypeKey());
    EXPECT_TRUE (handle->getBounds().isEmpty());
    EXPECT_TRUE (handle->getLocalTransform().isIdentity());
    EXPECT_TRUE (handle->getWorldTransform().isIdentity());
    EXPECT_TRUE (handle->getViewTransform().isIdentity());
    EXPECT_EQ (nullptr, handle->getParent().get());
    EXPECT_TRUE (handle->getChildren().isEmpty());
}

//==============================================================================
// Node bounds listeners - driven by resize (view transform changes)
//==============================================================================

TEST_F (ArtboardGameAnimationTests, ListenerFiresOnResizeWithCachedHandle)
{
    for (const auto* nodeName : { kGameRootArtboardNodeName, kGameNestedArtboardNodeName })
    {
        int callCount = 0;
        ArtboardNode::Ptr firstHandle;

        artboard->setNodeBoundsListener (nodeName,
                                         [&] (Artboard&, const String&, const ArtboardNode::Ptr& node)
                                         {
                                             ++callCount;
                                             if (firstHandle == nullptr)
                                                 firstHandle = node;
                                             else
                                                 EXPECT_EQ (firstHandle.get(), node.get());
                                         });

        // Restore the base size so the resize below is a real change for both nodes.
        artboard->setBounds (0.0f, 0.0f, 500.0f, 500.0f);

        for (int i = 0; i < 5; ++i)
            artboard->advanceAndApply (0.0f);

        artboard->setBounds (0.0f, 0.0f, 700.0f, 400.0f);

        for (int i = 0; i < 10; ++i)
            artboard->advanceAndApply (0.0f);

        EXPECT_GT (callCount, 0) << nodeName;

        ASSERT_NE (nullptr, firstHandle.get()) << nodeName;
        EXPECT_TRUE (firstHandle->isValid()) << nodeName;
        EXPECT_FALSE (firstHandle->getBounds().isEmpty()) << nodeName;

        // The callback receives the same cached handle that findNode hands out.
        EXPECT_EQ (firstHandle.get(), artboard->findNode (nodeName).get()) << nodeName;

        artboard->clearNodeBoundsListener (nodeName);
    }
}

TEST_F (ArtboardGameAnimationTests, ListenerDoesNotFireWhenNothingAdvances)
{
    for (const auto* nodeName : { kGameRootArtboardNodeName, kGameNestedArtboardNodeName })
    {
        int callCount = 0;

        artboard->setNodeBoundsListener (nodeName,
                                         [&] (Artboard&, const String&, const ArtboardNode::Ptr&)
                                         {
                                             ++callCount;
                                         });

        // Zero-delta advances leave the scene at frame zero, so nothing moves.
        for (int i = 0; i < 5; ++i)
            artboard->advanceAndApply (0.0f);

        EXPECT_EQ (0, callCount) << nodeName;

        artboard->clearNodeBoundsListener (nodeName);
    }
}

TEST_F (ArtboardGameAnimationTests, EmptyCallbackRemovesTheListener)
{
    int callCount = 0;

    artboard->setNodeBoundsListener (kGameNestedArtboardNodeName,
                                     [&] (Artboard&, const String&, const ArtboardNode::Ptr&)
                                     {
                                         ++callCount;
                                     });

    artboard->setNodeBoundsListener (kGameNestedArtboardNodeName, {});

    artboard->setBounds (0.0f, 0.0f, 700.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_EQ (0, callCount);
}

TEST_F (ArtboardGameAnimationTests, ClearingListenersStopsNotifications)
{
    int callCount = 0;

    artboard->setNodeBoundsListener (kGameNestedArtboardNodeName,
                                     [&] (Artboard&, const String&, const ArtboardNode::Ptr&)
                                     {
                                         ++callCount;
                                     });

    artboard->clearNodeBoundsListener (kGameNestedArtboardNodeName);

    artboard->setBounds (0.0f, 0.0f, 700.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_EQ (0, callCount);
}

TEST_F (ArtboardGameAnimationTests, ListenerFiresAgainAfterAnotherResize)
{
    int callCount = 0;
    Rectangle<float> lastBounds;

    artboard->setNodeBoundsListener (kGameNestedArtboardNodeName,
                                     [&] (Artboard&, const String&, const ArtboardNode::Ptr& node)
                                     {
                                         ++callCount;
                                         lastBounds = node->getBounds();
                                     });

    artboard->setBounds (0.0f, 0.0f, 700.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_GT (callCount, 0);
    EXPECT_FALSE (lastBounds.isEmpty());

    const auto boundsAfterFirstResize = lastBounds;

    artboard->setBounds (0.0f, 0.0f, 300.0f, 600.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_GT (callCount, 1);
    EXPECT_FALSE (lastBounds == boundsAfterFirstResize);
}

//==============================================================================
// Attached components on the root and nested nodes
//==============================================================================

TEST_F (ArtboardGameAnimationTests, FillNodeAttachmentMatchesNodeBounds)
{
    for (const auto* nodeName : { kGameRootArtboardNodeName, kGameNestedArtboardNodeName })
    {
        Component component ("test");

        ASSERT_TRUE (artboard->attachComponentToNode (nodeName, &component)) << nodeName;

        const auto expected = artboard->getNodeBounds (nodeName);
        const auto actual = component.getBounds();

        EXPECT_FALSE (expected.isEmpty()) << nodeName;
        EXPECT_FLOAT_EQ (expected.getX(), actual.getX()) << nodeName;
        EXPECT_FLOAT_EQ (expected.getY(), actual.getY()) << nodeName;
        EXPECT_FLOAT_EQ (expected.getWidth(), actual.getWidth()) << nodeName;
        EXPECT_FLOAT_EQ (expected.getHeight(), actual.getHeight()) << nodeName;

        artboard->detachComponentFromNode (nodeName, &component);
    }
}

TEST_F (ArtboardGameAnimationTests, FillNodeAttachmentFollowsResize)
{
    for (const auto* nodeName : { kGameRootArtboardNodeName, kGameNestedArtboardNodeName })
    {
        Component component ("test");

        ASSERT_TRUE (artboard->attachComponentToNode (nodeName, &component)) << nodeName;

        // Restore the base size so the resize below is a real change for both nodes.
        artboard->setBounds (0.0f, 0.0f, 500.0f, 500.0f);

        for (int i = 0; i < 5; ++i)
            artboard->advanceAndApply (0.0f);

        artboard->setBounds (0.0f, 0.0f, 700.0f, 400.0f);

        for (int i = 0; i < 10; ++i)
            artboard->advanceAndApply (0.0f);

        const auto nodeBounds = artboard->getNodeBounds (nodeName);
        const auto componentBounds = component.getBounds();

        EXPECT_FALSE (nodeBounds.isEmpty()) << nodeName;
        EXPECT_FLOAT_EQ (nodeBounds.getX(), componentBounds.getX()) << nodeName;
        EXPECT_FLOAT_EQ (nodeBounds.getY(), componentBounds.getY()) << nodeName;
        EXPECT_FLOAT_EQ (nodeBounds.getWidth(), componentBounds.getWidth()) << nodeName;
        EXPECT_FLOAT_EQ (nodeBounds.getHeight(), componentBounds.getHeight()) << nodeName;

        artboard->detachComponentFromNode (nodeName, &component);
    }
}

TEST_F (ArtboardGameAnimationTests, TrackPositionKeepsSizeAndTracksOrigin)
{
    for (const auto* nodeName : { kGameRootArtboardNodeName, kGameNestedArtboardNodeName })
    {
        Component component ("test");

        Artboard::NodeAttachmentOptions options;
        options.mode = Artboard::NodeAttachmentOptions::Mode::trackPosition;

        ASSERT_TRUE (artboard->attachComponentToNode (nodeName, &component, options)) << nodeName;

        component.setBounds (0.0f, 0.0f, 40.0f, 20.0f);

        // Restore the base size so the resize below is a real change for both nodes.
        artboard->setBounds (0.0f, 0.0f, 500.0f, 500.0f);

        for (int i = 0; i < 5; ++i)
            artboard->advanceAndApply (0.0f);

        artboard->setBounds (0.0f, 0.0f, 700.0f, 400.0f);

        for (int i = 0; i < 10; ++i)
            artboard->advanceAndApply (0.0f);

        EXPECT_FLOAT_EQ (40.0f, component.getWidth()) << nodeName;
        EXPECT_FLOAT_EQ (20.0f, component.getHeight()) << nodeName;

        const auto nodeBounds = artboard->getNodeBounds (nodeName);
        EXPECT_FLOAT_EQ (nodeBounds.getX(), component.getX()) << nodeName;
        EXPECT_FLOAT_EQ (nodeBounds.getY(), component.getY()) << nodeName;

        artboard->detachComponentFromNode (nodeName, &component);
    }
}

TEST_F (ArtboardGameAnimationTests, TrackPositionPivotAndAnchor)
{
    Component component ("test");

    Artboard::NodeAttachmentOptions options;
    options.mode = Artboard::NodeAttachmentOptions::Mode::trackPosition;
    options.pivot = Justification::center;
    options.anchor = Justification::center;

    ASSERT_TRUE (artboard->attachComponentToNode (kGameNestedArtboardNodeName, &component, options));

    component.setBounds (0.0f, 0.0f, 40.0f, 20.0f);

    artboard->setBounds (0.0f, 0.0f, 700.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    const auto nodeBounds = artboard->getNodeBounds (kGameNestedArtboardNodeName);
    EXPECT_FLOAT_EQ (nodeBounds.getCenterX(), component.getBounds().getCenterX());
    EXPECT_FLOAT_EQ (nodeBounds.getCenterY(), component.getBounds().getCenterY());
}

TEST_F (ArtboardGameAnimationTests, ApplyTransformIsIdentityForUnrotatedRoot)
{
    Component component ("test");

    Artboard::NodeAttachmentOptions options;
    options.applyTransform = true;

    ASSERT_TRUE (artboard->attachComponentToNode (kGameRootArtboardNodeName, &component, options));

    artboard->setBounds (0.0f, 0.0f, 700.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_TRUE (component.getTransform().isIdentity());
}

TEST_F (ArtboardGameAnimationTests, DetachStopsFollowingAfterResize)
{
    Component component ("test");

    ASSERT_TRUE (artboard->attachComponentToNode (kGameNestedArtboardNodeName, &component));

    ASSERT_TRUE (artboard->detachComponentFromNode (kGameNestedArtboardNodeName, &component));

    const auto boundsBefore = component.getBounds();

    artboard->setBounds (0.0f, 0.0f, 700.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_TRUE (component.getBounds() == boundsBefore);
}

TEST_F (ArtboardGameAnimationTests, ReattachingReplacesThePreviousAttachment)
{
    Component first ("first");
    Component second ("second");

    ASSERT_TRUE (artboard->attachComponentToNode (kGameNestedArtboardNodeName, &first));

    const auto boundsBeforeSecond = first.getBounds();

    ASSERT_TRUE (artboard->attachComponentToNode (kGameNestedArtboardNodeName, &second));

    artboard->setBounds (0.0f, 0.0f, 700.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    // Only the latest attachment follows the node after a reflow.
    EXPECT_TRUE (first.getBounds() == boundsBeforeSecond);

    const auto nodeBounds = artboard->getNodeBounds (kGameNestedArtboardNodeName);
    EXPECT_FLOAT_EQ (nodeBounds.getX(), second.getX());
    EXPECT_FLOAT_EQ (nodeBounds.getY(), second.getY());
}

//==============================================================================
// Artboard and ArtboardNode against tests/data/rive/data-binding.riv
//
// data-binding.riv names only its artboard ("Artboard"); the Text nodes are
// unnamed, so the name-based APIs are exercised through the artboard root.
// The file also contains an in-band FontAsset, a text-bound ViewModel and a
// (keyframe-less) animation, all of which must load and advance safely.
//==============================================================================

namespace
{
constexpr const char* kDataBindingArtboardRootName = "Artboard";
} // namespace

class ArtboardDataBindingTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto file = getTestDataRiveDirectory().getChildFile ("data-binding.riv");
        if (! file.existsAsFile())
        {
            GTEST_SKIP() << "Missing test asset: tests/data/rive/data-binding.riv";
            return;
        }

        auto result = ArtboardFile::load (file, factory);
        if (result.failed())
        {
            GTEST_SKIP() << "Failed to load test asset: " << result.getErrorMessage();
            return;
        }

        artboardFile = result.getValue();
        artboard = std::make_unique<Artboard> ("testArtboard", artboardFile);
        artboard->setLayout (Artboard::Layout::contain);
        artboard->setBounds (0.0f, 0.0f, 500.0f, 500.0f);

        for (int i = 0; i < 5; ++i)
            artboard->advanceAndApply (0.0f);
    }

    ::testing::NiceMock<MockRiveFactory> factory;
    std::shared_ptr<ArtboardFile> artboardFile;
    std::unique_ptr<Artboard> artboard;
};

TEST_F (ArtboardDataBindingTests, LoadingFileProvidesRootQueries)
{
    EXPECT_FALSE (artboard->getNodeBounds (kDataBindingArtboardRootName).isEmpty());

    auto node = artboard->findNode (kDataBindingArtboardRootName);
    ASSERT_NE (nullptr, node.get());
    EXPECT_TRUE (node->isValid());
}

TEST_F (ArtboardDataBindingTests, RootHandleReportsItsIdentity)
{
    auto node = artboard->findNode (kDataBindingArtboardRootName);
    ASSERT_NE (nullptr, node.get());

    EXPECT_EQ (String (kDataBindingArtboardRootName), node->getName());
    EXPECT_EQ (static_cast<int> (rive::ArtboardBase::typeKey), static_cast<int> (node->getTypeKey()));
    EXPECT_EQ (String ("Artboard"), node->getTypeName());
    EXPECT_TRUE (node->isLayout());
}

TEST_F (ArtboardDataBindingTests, RootBoundsAreConsistentWithGetNodeBounds)
{
    auto node = artboard->findNode (kDataBindingArtboardRootName);
    ASSERT_NE (nullptr, node.get());

    const auto nodeBounds = node->getBounds();
    const auto artboardBounds = artboard->getNodeBounds (kDataBindingArtboardRootName);

    EXPECT_FALSE (nodeBounds.isEmpty());
    EXPECT_GT (nodeBounds.getWidth(), 0.0f);
    EXPECT_GT (nodeBounds.getHeight(), 0.0f);

    EXPECT_FLOAT_EQ (artboardBounds.getX(), nodeBounds.getX());
    EXPECT_FLOAT_EQ (artboardBounds.getY(), nodeBounds.getY());
    EXPECT_FLOAT_EQ (artboardBounds.getWidth(), nodeBounds.getWidth());
    EXPECT_FLOAT_EQ (artboardBounds.getHeight(), nodeBounds.getHeight());
}

TEST_F (ArtboardDataBindingTests, RootTransformsAreFinite)
{
    auto node = artboard->findNode (kDataBindingArtboardRootName);
    ASSERT_NE (nullptr, node.get());

    const auto checkFinite = [] (const AffineTransform& transform, const char* label)
    {
        EXPECT_TRUE (std::isfinite (transform.getScaleX())) << label;
        EXPECT_TRUE (std::isfinite (transform.getShearX())) << label;
        EXPECT_TRUE (std::isfinite (transform.getTranslateX())) << label;
        EXPECT_TRUE (std::isfinite (transform.getShearY())) << label;
        EXPECT_TRUE (std::isfinite (transform.getScaleY())) << label;
        EXPECT_TRUE (std::isfinite (transform.getTranslateY())) << label;
    };

    checkFinite (node->getLocalTransform(), "local");
    checkFinite (node->getWorldTransform(), "world");
    checkFinite (node->getViewTransform(), "view");
}

TEST_F (ArtboardDataBindingTests, RootChildrenIncludeTextNodes)
{
    auto node = artboard->findNode (kDataBindingArtboardRootName);
    ASSERT_NE (nullptr, node.get());

    const auto children = node->getChildren();
    ASSERT_FALSE (children.isEmpty());

    bool foundText = false;
    for (const auto& child : children)
    {
        ASSERT_NE (nullptr, child.get());
        EXPECT_TRUE (child->isValid());
        ASSERT_NE (nullptr, child->getParent().get());
        EXPECT_EQ (String (kDataBindingArtboardRootName), child->getParent()->getName());

        if (child->getTypeName() == String ("Text"))
            foundText = true;
    }

    EXPECT_TRUE (foundText);
}

TEST_F (ArtboardDataBindingTests, FindNodeReturnsTheCachedHandle)
{
    auto first = artboard->findNode (kDataBindingArtboardRootName);
    auto second = artboard->findNode (kDataBindingArtboardRootName);

    ASSERT_NE (nullptr, first.get());
    EXPECT_EQ (first.get(), second.get());
}

TEST_F (ArtboardDataBindingTests, HandleIsRefcounted)
{
    auto node = artboard->findNode (kDataBindingArtboardRootName);
    ASSERT_NE (nullptr, node.get());

    ArtboardNode::Ptr copy = node;
    EXPECT_EQ (node.get(), copy.get());
    EXPECT_TRUE (copy->isValid());

    copy = nullptr;
    EXPECT_TRUE (node->isValid());
}

TEST_F (ArtboardDataBindingTests, FindNodeWorksOnConstArtboard)
{
    const auto& constArtboard = *artboard;
    auto node = constArtboard.findNode (kDataBindingArtboardRootName);
    ASSERT_NE (nullptr, node.get());
    EXPECT_TRUE (node->isValid());
}

TEST_F (ArtboardDataBindingTests, UnknownNodesAreNotResolved)
{
    Component component ("test");

    EXPECT_TRUE (artboard->getNodeBounds ("nonexistent").isEmpty());
    EXPECT_EQ (nullptr, artboard->findNode ("nonexistent").get());
    EXPECT_FALSE (artboard->attachComponentToNode ("nonexistent", &component));
}

TEST_F (ArtboardDataBindingTests, InputQueriesWithLoadedFileDoNotThrow)
{
    EXPECT_FALSE (artboard->hasBoolInput ("nonexistentInput"));
    EXPECT_FALSE (artboard->hasNumberInput ("nonexistentInput"));
    EXPECT_FALSE (artboard->hasTriggerInput ("nonexistentInput"));

    EXPECT_NO_THROW (artboard->setBoolInput ("nonexistentInput", true));
    EXPECT_NO_THROW (artboard->setNumberInput ("nonexistentInput", 42.0));
    EXPECT_NO_THROW (artboard->triggerInput ("nonexistentInput"));
    EXPECT_NO_THROW (artboard->setInput ("nonexistentInput", var (true)));
    EXPECT_NO_THROW (artboard->setAllInputs (var()));
    EXPECT_NO_THROW (artboard->getAllInputs());
}

TEST_F (ArtboardDataBindingTests, AdvanceAndApplyRunsTheLoadedScene)
{
    for (int frame = 0; frame < 60; ++frame)
    {
        EXPECT_NO_THROW (artboard->advanceAndApply (0.016f));
        EXPECT_NO_THROW (artboard->refreshDisplay (0.016));
    }
}

TEST_F (ArtboardDataBindingTests, ClearResetsTheLoadedFile)
{
    artboard->clear();

    EXPECT_TRUE (artboard->getNodeBounds (kDataBindingArtboardRootName).isEmpty());
    EXPECT_EQ (nullptr, artboard->findNode (kDataBindingArtboardRootName).get());
    EXPECT_EQ (Artboard::Layout::contain, artboard->getLayout()); // layout survives clear
}

TEST_F (ArtboardDataBindingTests, ReloadingTheFileRestoresTheScene)
{
    artboard->setFile (artboardFile);

    auto node = artboard->findNode (kDataBindingArtboardRootName);
    ASSERT_NE (nullptr, node.get());
    EXPECT_TRUE (node->isValid());
    EXPECT_FALSE (artboard->getNodeBounds (kDataBindingArtboardRootName).isEmpty());
}

TEST_F (ArtboardDataBindingTests, ClearInvalidatesOutstandingHandle)
{
    auto node = artboard->findNode (kDataBindingArtboardRootName);
    ASSERT_NE (nullptr, node.get());
    EXPECT_TRUE (node->isValid());

    artboard->clear();

    EXPECT_FALSE (node->isValid());
    EXPECT_TRUE (node->getName().isEmpty());
    EXPECT_EQ (0, node->getTypeKey());
    EXPECT_TRUE (node->getTypeName().isEmpty());
    EXPECT_FALSE (node->isLayout());
    EXPECT_TRUE (node->getBounds().isEmpty());
    EXPECT_TRUE (node->getLocalTransform().isIdentity());
    EXPECT_TRUE (node->getWorldTransform().isIdentity());
    EXPECT_TRUE (node->getViewTransform().isIdentity());
    EXPECT_EQ (nullptr, node->getParent().get());
    EXPECT_TRUE (node->getChildren().isEmpty());
}

TEST_F (ArtboardDataBindingTests, SetFileInvalidatesHandlesAndRefreshesCache)
{
    auto oldHandle = artboard->findNode (kDataBindingArtboardRootName);
    ASSERT_NE (nullptr, oldHandle.get());
    EXPECT_TRUE (oldHandle->isValid());

    artboard->setFile (artboardFile);

    EXPECT_FALSE (oldHandle->isValid());

    auto newHandle = artboard->findNode (kDataBindingArtboardRootName);
    ASSERT_NE (nullptr, newHandle.get());
    EXPECT_TRUE (newHandle->isValid());
    EXPECT_NE (oldHandle.get(), newHandle.get());
}

TEST_F (ArtboardDataBindingTests, DestroyingTheArtboardInvalidatesHandles)
{
    ArtboardNode::Ptr handle;

    {
        Artboard temp ("temp", artboardFile);
        temp.setLayout (Artboard::Layout::contain);
        temp.setBounds (0.0f, 0.0f, 500.0f, 500.0f);

        for (int i = 0; i < 5; ++i)
            temp.advanceAndApply (0.0f);

        handle = temp.findNode (kDataBindingArtboardRootName);
        ASSERT_NE (nullptr, handle.get());
        EXPECT_TRUE (handle->isValid());
    }

    EXPECT_FALSE (handle->isValid());
    EXPECT_TRUE (handle->getBounds().isEmpty());
    EXPECT_TRUE (handle->getLocalTransform().isIdentity());
    EXPECT_TRUE (handle->getWorldTransform().isIdentity());
    EXPECT_TRUE (handle->getViewTransform().isIdentity());
    EXPECT_EQ (nullptr, handle->getParent().get());
    EXPECT_TRUE (handle->getChildren().isEmpty());
}

TEST_F (ArtboardDataBindingTests, ListenerFiresOnResizeWithCachedHandle)
{
    int callCount = 0;
    ArtboardNode::Ptr firstHandle;

    artboard->setNodeBoundsListener (kDataBindingArtboardRootName,
                                     [&] (Artboard&, const String&, const ArtboardNode::Ptr& node)
                                     {
                                         ++callCount;
                                         if (firstHandle == nullptr)
                                             firstHandle = node;
                                         else
                                             EXPECT_EQ (firstHandle.get(), node.get());
                                     });

    artboard->setBounds (0.0f, 0.0f, 700.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_GT (callCount, 0);

    ASSERT_NE (nullptr, firstHandle.get());
    EXPECT_TRUE (firstHandle->isValid());
    EXPECT_FALSE (firstHandle->getBounds().isEmpty());

    EXPECT_EQ (firstHandle.get(), artboard->findNode (kDataBindingArtboardRootName).get());
}

TEST_F (ArtboardDataBindingTests, ListenerDoesNotFireWhenNothingAdvances)
{
    int callCount = 0;

    artboard->setNodeBoundsListener (kDataBindingArtboardRootName,
                                     [&] (Artboard&, const String&, const ArtboardNode::Ptr&)
                                     {
                                         ++callCount;
                                     });

    for (int i = 0; i < 5; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_EQ (0, callCount);
}

TEST_F (ArtboardDataBindingTests, EmptyCallbackRemovesTheListener)
{
    int callCount = 0;

    artboard->setNodeBoundsListener (kDataBindingArtboardRootName,
                                     [&] (Artboard&, const String&, const ArtboardNode::Ptr&)
                                     {
                                         ++callCount;
                                     });

    artboard->setNodeBoundsListener (kDataBindingArtboardRootName, {});

    artboard->setBounds (0.0f, 0.0f, 700.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_EQ (0, callCount);
}

TEST_F (ArtboardDataBindingTests, ClearingListenersStopsNotifications)
{
    int callCount = 0;

    artboard->setNodeBoundsListener (kDataBindingArtboardRootName,
                                     [&] (Artboard&, const String&, const ArtboardNode::Ptr&)
                                     {
                                         ++callCount;
                                     });

    artboard->clearNodeBoundsListener (kDataBindingArtboardRootName);

    artboard->setBounds (0.0f, 0.0f, 700.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_EQ (0, callCount);
}

TEST_F (ArtboardDataBindingTests, FillNodeAttachmentMatchesNodeBounds)
{
    Component component ("test");

    ASSERT_TRUE (artboard->attachComponentToNode (kDataBindingArtboardRootName, &component));

    const auto expected = artboard->getNodeBounds (kDataBindingArtboardRootName);
    const auto actual = component.getBounds();

    EXPECT_FALSE (expected.isEmpty());
    EXPECT_FLOAT_EQ (expected.getX(), actual.getX());
    EXPECT_FLOAT_EQ (expected.getY(), actual.getY());
    EXPECT_FLOAT_EQ (expected.getWidth(), actual.getWidth());
    EXPECT_FLOAT_EQ (expected.getHeight(), actual.getHeight());
}

TEST_F (ArtboardDataBindingTests, FillNodeAttachmentFollowsResize)
{
    Component component ("test");

    ASSERT_TRUE (artboard->attachComponentToNode (kDataBindingArtboardRootName, &component));

    artboard->setBounds (0.0f, 0.0f, 700.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    const auto nodeBounds = artboard->getNodeBounds (kDataBindingArtboardRootName);
    const auto componentBounds = component.getBounds();

    EXPECT_FALSE (nodeBounds.isEmpty());
    EXPECT_FLOAT_EQ (nodeBounds.getX(), componentBounds.getX());
    EXPECT_FLOAT_EQ (nodeBounds.getY(), componentBounds.getY());
    EXPECT_FLOAT_EQ (nodeBounds.getWidth(), componentBounds.getWidth());
    EXPECT_FLOAT_EQ (nodeBounds.getHeight(), componentBounds.getHeight());
}

TEST_F (ArtboardDataBindingTests, TrackPositionTracksOriginAndKeepsSize)
{
    Component component ("test");

    Artboard::NodeAttachmentOptions options;
    options.mode = Artboard::NodeAttachmentOptions::Mode::trackPosition;

    ASSERT_TRUE (artboard->attachComponentToNode (kDataBindingArtboardRootName, &component, options));

    component.setBounds (0.0f, 0.0f, 40.0f, 20.0f);

    artboard->setBounds (0.0f, 0.0f, 700.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_FLOAT_EQ (40.0f, component.getWidth());
    EXPECT_FLOAT_EQ (20.0f, component.getHeight());

    const auto nodeBounds = artboard->getNodeBounds (kDataBindingArtboardRootName);
    EXPECT_FLOAT_EQ (nodeBounds.getX(), component.getX());
    EXPECT_FLOAT_EQ (nodeBounds.getY(), component.getY());
}

TEST_F (ArtboardDataBindingTests, TrackPositionPivotAndAnchor)
{
    Component component ("test");

    Artboard::NodeAttachmentOptions options;
    options.mode = Artboard::NodeAttachmentOptions::Mode::trackPosition;
    options.pivot = Justification::center;
    options.anchor = Justification::center;

    ASSERT_TRUE (artboard->attachComponentToNode (kDataBindingArtboardRootName, &component, options));

    component.setBounds (0.0f, 0.0f, 40.0f, 20.0f);

    artboard->setBounds (0.0f, 0.0f, 700.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    const auto nodeBounds = artboard->getNodeBounds (kDataBindingArtboardRootName);
    EXPECT_FLOAT_EQ (nodeBounds.getCenterX(), component.getBounds().getCenterX());
    EXPECT_FLOAT_EQ (nodeBounds.getCenterY(), component.getBounds().getCenterY());
}

TEST_F (ArtboardDataBindingTests, ApplyTransformIsIdentityForUnrotatedRoot)
{
    Component component ("test");

    Artboard::NodeAttachmentOptions options;
    options.applyTransform = true;

    ASSERT_TRUE (artboard->attachComponentToNode (kDataBindingArtboardRootName, &component, options));

    artboard->setBounds (0.0f, 0.0f, 700.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_TRUE (component.getTransform().isIdentity());
}

TEST_F (ArtboardDataBindingTests, DetachStopsFollowingAfterResize)
{
    Component component ("test");

    ASSERT_TRUE (artboard->attachComponentToNode (kDataBindingArtboardRootName, &component));

    ASSERT_TRUE (artboard->detachComponentFromNode (kDataBindingArtboardRootName, &component));

    const auto boundsBefore = component.getBounds();

    artboard->setBounds (0.0f, 0.0f, 700.0f, 400.0f);

    for (int i = 0; i < 10; ++i)
        artboard->advanceAndApply (0.0f);

    EXPECT_TRUE (component.getBounds() == boundsBefore);
}
