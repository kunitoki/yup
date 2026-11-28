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

#include <gtest/gtest.h>

#include <yup_renderer/yup_renderer.h>

using namespace yup;

class SceneComponentTests : public ::testing::Test
{
};

TEST_F (SceneComponentTests, IsOpaqueAndTakesFocusWhenClicked)
{
    SceneComponent component;

    EXPECT_TRUE (component.isOpaque());
    EXPECT_TRUE (component.getWantsKeyboardFocus());
}

TEST_F (SceneComponentTests, KeepsSceneAndUpdateMode)
{
    SceneComponent component;
    auto scene = Scene::Ptr (new Scene());

    EXPECT_EQ (component.getScene(), nullptr);
    EXPECT_FALSE (component.isUpdatingContinuously());

    component.setScene (scene);
    component.setContinuousUpdates (true);

    EXPECT_EQ (component.getScene(), scene);
    EXPECT_TRUE (component.isUpdatingContinuously());
}

TEST_F (SceneComponentTests, SetSceneMakesTheComponentOpaqueOrNotAfterItsBackground)
{
    SceneComponent component;

    // The default background is opaque
    component.setScene (Scene::Ptr (new Scene()));
    EXPECT_TRUE (component.isOpaque());

    auto seeThrough = Scene::Ptr (new Scene());
    seeThrough->getRoot().attach<BackgroundNode> (Colors::transparentBlack);
    component.setScene (seeThrough);
    EXPECT_FALSE (component.isOpaque());

    // Without a scene the component fills itself
    component.setScene (nullptr);
    EXPECT_TRUE (component.isOpaque());
}

TEST_F (SceneComponentTests, OpaqueStateFollowsTheBackgroundOnEveryDisplayFrame)
{
    SceneComponent component;
    auto scene = Scene::Ptr (new Scene());
    component.setScene (scene);
    ASSERT_TRUE (component.isOpaque());

    // Nothing tells the component that the scene changed: its next display frame finds out
    auto& background = scene->getRoot().attach<BackgroundNode> (Colors::black.withAlpha (0.5f));
    EXPECT_TRUE (component.isOpaque());

    component.refreshDisplay (0.0);
    EXPECT_FALSE (component.isOpaque());

    // So does a color edited in place
    background.color = Colors::black;
    component.refreshDisplay (0.0);
    EXPECT_TRUE (component.isOpaque());

    background.color = Colors::transparentBlack;
    component.refreshDisplay (0.0);
    EXPECT_FALSE (component.isOpaque());

    // Without its node the background is the default one again
    EXPECT_TRUE (scene->getRoot().detach<BackgroundNode>());
    component.refreshDisplay (0.0);
    EXPECT_TRUE (component.isOpaque());
}

/** The display frames a scene takes of the window it is shown in: only the ones it has something to do
    in, so that a window whose scene stands still is free to skip the frames it has nothing to paint in.
*/
class SceneComponentFrameTests : public SceneDocumentTests
{
protected:
    /** Counts the updates of its scene that time elapsed for: the ones of its display frames. */
    struct UpdateCounter final : Node
    {
        void update (double deltaSeconds) override
        {
            if (deltaSeconds > 0.0)
                ++numUpdates;
        }

        int numUpdates = 0;
    };

    void SetUp() override
    {
        SceneDocumentTests::SetUp();

        window.addAndMakeVisible (view);
        view.setBounds (0.0f, 0.0f, 200.0f, 200.0f);
    }

    static int numUpdatesOf (const Entity& entity)
    {
        const auto* counter = entity.get<UpdateCounter>();
        return counter != nullptr ? counter->numUpdates : -1;
    }

    Component window { "window" };
    WindowNative& native = makeWindow (window);
    SceneComponent view;
};

TEST_F (SceneComponentFrameTests, ASceneThatStandsStillLeavesTheFramesOfItsWindow)
{
    SceneNodeComponent node;
    view.addAndMakeVisible (node);

    // It takes the frames that find out that nothing is left for them to do
    EXPECT_TRUE (native.hasFrameListeners());

    sendFrames (native, 3);
    EXPECT_FALSE (native.hasFrameListeners());
}

TEST_F (SceneComponentFrameTests, ContinuousUpdatesTakeEveryFrame)
{
    auto scene = Scene::Ptr (new Scene());
    auto counter = scene->createEntity ("counter");
    counter.attach<UpdateCounter>();

    view.setScene (scene);

    sendFrames (native, 3);
    ASSERT_FALSE (native.hasFrameListeners());
    EXPECT_EQ (numUpdatesOf (counter), 0);

    view.setContinuousUpdates (true);
    EXPECT_TRUE (native.hasFrameListeners());

    sendFrames (native, 3);
    EXPECT_TRUE (native.hasFrameListeners());
    EXPECT_EQ (numUpdatesOf (counter), 3);

    // Not asked anymore, the next frame finds nothing left to do
    view.setContinuousUpdates (false);
    sendFrames (native, 3);

    EXPECT_FALSE (native.hasFrameListeners());
    EXPECT_EQ (numUpdatesOf (counter), 3);
}

TEST_F (SceneComponentFrameTests, ANodeThatChangesTakesTheFramesThatFollowIt)
{
    SceneNodeComponent node;
    view.addAndMakeVisible (node);

    sendFrames (native, 3);
    ASSERT_FALSE (native.hasFrameListeners());

    node.setProperty (Identifier ("position"), numbers ({ 1.0, 2.0, 3.0 }));
    EXPECT_TRUE (native.hasFrameListeners());

    sendFrames (native, 3);
    EXPECT_FALSE (native.hasFrameListeners());
}

TEST_F (SceneComponentFrameTests, ANodeThatLeftIsLetGoOfByAFrameItsSceneTakesForIt)
{
    SceneNodeComponent node;
    view.addAndMakeVisible (node);

    sendFrames (native, 3);
    ASSERT_FALSE (native.hasFrameListeners());

    const auto entity = node.getEntity();
    view.removeChildComponent (node);

    // Kept until the next frame of the scene, which a scene that stood still takes for it
    ASSERT_TRUE (entity.isValid());
    EXPECT_TRUE (native.hasFrameListeners());

    sendFrames (native, 1);
    EXPECT_FALSE (entity.isValid());

    sendFrames (native, 2);
    EXPECT_FALSE (native.hasFrameListeners());
}

TEST_F (SceneComponentFrameTests, ANodeShowingComponentsHasItsSceneTakeEveryFrame)
{
    SceneNodeComponent node;
    Component label;

    view.addAndMakeVisible (node);
    node.addAndMakeVisible (label);

    // The mesh and the material that show them are looked for on every frame: nothing tells when code changes them
    sendFrames (native, 3);
    EXPECT_TRUE (native.hasFrameListeners());

    node.removeChildComponent (label);
    sendFrames (native, 3);

    EXPECT_FALSE (native.hasFrameListeners());
}

TEST_F (SceneComponentFrameTests, AHiddenSceneTakesNoFrameAndCarriesOnOnceItShows)
{
    auto scene = Scene::Ptr (new Scene());
    auto counter = scene->createEntity ("counter");
    counter.attach<UpdateCounter>();

    view.setScene (scene);
    view.setContinuousUpdates (true);

    SceneNodeComponent node;
    view.addAndMakeVisible (node);

    sendFrames (native, 2);
    ASSERT_TRUE (native.hasFrameListeners());
    ASSERT_EQ (numUpdatesOf (counter), 2);

    // Hidden with what it is in, its scene stands still and the window is free to skip its frames
    window.setVisible (false);
    EXPECT_FALSE (native.hasFrameListeners());

    const auto entity = node.getEntity();
    view.removeChildComponent (node);

    sendFrames (native, 3);

    EXPECT_FALSE (native.hasFrameListeners());
    EXPECT_EQ (numUpdatesOf (counter), 2);
    EXPECT_TRUE (entity.isValid());

    // Showing again, it carries on with its next frame, which lets go of what left it meanwhile
    window.setVisible (true);
    EXPECT_TRUE (native.hasFrameListeners());

    sendFrames (native, 1);

    EXPECT_EQ (numUpdatesOf (counter), 3);
    EXPECT_FALSE (entity.isValid());
}

TEST_F (SceneComponentFrameTests, ASceneMovedUnderSomethingHiddenTakesNoFrameUntilItIsBackWhereItShows)
{
    Component hidden;
    window.addChildComponent (hidden);

    view.setContinuousUpdates (true);
    sendFrames (native, 2);
    ASSERT_TRUE (native.hasFrameListeners());

    hidden.addChildComponent (view);
    EXPECT_FALSE (native.hasFrameListeners());

    window.addChildComponent (view);
    EXPECT_TRUE (native.hasFrameListeners());
}

TEST_F (SceneComponentFrameTests, ASceneChangedFromCodeIsLookedAtByTheFrameThatFollowsItsPaint)
{
    auto scene = Scene::Ptr (new Scene());
    view.setScene (scene);

    sendFrames (native, 3);
    ASSERT_FALSE (native.hasFrameListeners());
    ASSERT_TRUE (view.isOpaque());

    // Code holding the scene changes it and asks for it to be painted, as it always had to: nothing else tells
    scene->getRoot().attach<BackgroundNode> (Colors::black.withAlpha (0.5f));
    view.repaint();

    // Without a theme nothing is written where no GPU draws the scene
    ApplicationTheme::setGlobalTheme (nullptr);
    ASSERT_FALSE (paintWithoutGpu (view).empty());

    // The paint that answers has the scene take the frame that follows, which finds what changed: here that the scene
    // is to be painted again, over what lies behind it
    EXPECT_TRUE (native.hasFrameListeners());

    Internals::takeRepaintRequest (view);
    sendFrames (native, 1);

    EXPECT_FALSE (view.isOpaque());
    EXPECT_TRUE (Internals::takeRepaintRequest (view));

    // Painted as it is by then, it leaves the frames again
    ASSERT_FALSE (paintWithoutGpu (view).empty());
    sendFrames (native, 3);

    EXPECT_FALSE (native.hasFrameListeners());
    EXPECT_FALSE (Internals::takeRepaintRequest (view));
}

TEST_F (SceneComponentFrameTests, ASceneUpdatedContinuouslyFollowsWhatCodeChangesWithoutBeingTold)
{
    auto scene = Scene::Ptr (new Scene());
    view.setScene (scene);
    view.setContinuousUpdates (true);

    sendFrames (native, 2);
    ASSERT_TRUE (view.isOpaque());

    // Neither a repaint nor a paint: every frame is one of the scene, and asks for the paint itself
    scene->getRoot().attach<BackgroundNode> (Colors::black.withAlpha (0.5f));
    Internals::takeRepaintRequest (view);

    sendFrames (native, 1);

    EXPECT_FALSE (view.isOpaque());
    EXPECT_TRUE (Internals::takeRepaintRequest (view));
}
