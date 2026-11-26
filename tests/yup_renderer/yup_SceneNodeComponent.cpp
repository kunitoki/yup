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

#include <yup_renderer/yup_renderer.h>

#include <gtest/gtest.h>

using namespace yup;

/** The `Node3D` of a `.yui` document: a component whose entity follows it in the tree of its
    scene, placed by properties that bindings, style rules and transitions drive.
*/
class SceneNodeComponentTests : public SceneDocumentTests
{
};

TEST_F (SceneNodeComponentTests, PlacesItsEntityWithPositionRotationAndScale)
{
    YuiDocument document;
    load (document, R"yui(
        Scene3D {
            Node3D { id: group; }
            Node3D { id: placed; position: [1, 2, 3]; }
            Node3D { id: turned; rotation: [90deg, 0, 0]; }
            Node3D { id: aimed; rotation: [30deg, 60deg, 0]; }
            Node3D { id: rolled; rotation: [30deg, 0, 45deg]; }
            Node3D { id: big; scale: 2; }
            Node3D { id: wide; scale: [1, 2, 3]; }
        })yui");

    auto* group = document.findById<SceneNodeComponent> ("group");
    auto* placed = document.findById<SceneNodeComponent> ("placed");
    auto* turned = document.findById<SceneNodeComponent> ("turned");
    auto* aimed = document.findById<SceneNodeComponent> ("aimed");
    auto* rolled = document.findById<SceneNodeComponent> ("rolled");
    auto* big = document.findById<SceneNodeComponent> ("big");
    auto* wide = document.findById<SceneNodeComponent> ("wide");

    ASSERT_NE (group, nullptr);
    ASSERT_NE (placed, nullptr);
    ASSERT_NE (turned, nullptr);
    ASSERT_NE (aimed, nullptr);
    ASSERT_NE (rolled, nullptr);
    ASSERT_NE (big, nullptr);
    ASSERT_NE (wide, nullptr);

    // A node that sets nothing is a plain group
    ASSERT_TRUE (group->getEntity().isValid());
    EXPECT_EQ (group->getEntity().get<TransformNode>(), nullptr);

    expectVector (positionOf (*placed), 1.0f, 2.0f, 3.0f);
    expectRotation (rotationOf (*placed), Quaternion());
    expectVector (scaleOf (*placed), 1.0f, 1.0f, 1.0f);

    const auto degrees = MathConstants<float>::pi / 180.0f;
    const auto aroundX = [] (float angle) { return Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, angle); };
    const auto aroundY = [] (float angle) { return Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, angle); };
    const auto aroundZ = [] (float angle) { return Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, angle); };

    expectRotation (rotationOf (*turned), aroundX (90.0f * degrees));

    // The turn around z comes first, then the one around x, then the one around y
    expectRotation (rotationOf (*aimed), aroundY (60.0f * degrees) * aroundX (30.0f * degrees));
    expectRotation (rotationOf (*rolled), aroundX (30.0f * degrees) * aroundZ (45.0f * degrees));

    expectVector (scaleOf (*big), 2.0f, 2.0f, 2.0f);
    expectVector (scaleOf (*wide), 1.0f, 2.0f, 3.0f);
}

TEST_F (SceneNodeComponentTests, ReadsBackWhatWasSetAndTheIdentityForWhatWasNot)
{
    YuiDocument document;
    load (document, R"yui(
        Scene3D {
            Node3D { id: placed; position: [1, 2, 3]; rotation: [0, 2, 0]; scale: [1, 2, 3]; lookAt: [0, 1, 0]; }
            Node3D { id: big; scale: 2; }
        })yui");

    auto* placed = document.findById<SceneNodeComponent> ("placed");
    auto* big = document.findById<SceneNodeComponent> ("big");

    ASSERT_NE (placed, nullptr);
    ASSERT_NE (big, nullptr);

    const Identifier position ("position"), rotation ("rotation"), scale ("scale"), lookAt ("lookAt");

    expectNumbers (placed->getProperty (position), { 1.0, 2.0, 3.0 });
    expectNumbers (placed->getProperty (rotation), { 0.0, 2.0, 0.0 });
    expectNumbers (placed->getProperty (scale), { 1.0, 2.0, 3.0 });
    expectNumbers (placed->getProperty (lookAt), { 0.0, 1.0, 0.0 });

    EXPECT_DOUBLE_EQ (static_cast<double> (big->getProperty (scale)), 2.0);
    expectNumbers (big->getProperty (position), { 0.0, 0.0, 0.0 });
    expectNumbers (big->getProperty (rotation), { 0.0, 0.0, 0.0 });
    EXPECT_TRUE (big->getProperty (lookAt).isVoid());

    // The angles are kept as they are given: more than a turn stays more than a turn
    placed->setProperty (rotation, numbers ({ 0.0, 7.0, 0.0 }));
    expectNumbers (placed->getProperty (rotation), { 0.0, 7.0, 0.0 });

    // Setting nothing takes the value away, and the last one takes the TransformNode with it
    big->setProperty (scale, var());
    EXPECT_EQ (big->getEntity().get<TransformNode>(), nullptr);
    EXPECT_DOUBLE_EQ (static_cast<double> (big->getProperty (scale)), 1.0);

    // Outside a scene there is nothing to read
    SceneNodeComponent alone;
    EXPECT_FALSE (alone.getEntity().isValid());
    EXPECT_TRUE (alone.getProperty (position).isVoid());
}

TEST_F (SceneNodeComponentTests, AValueThatDoesNotPlaceANodeIsReported)
{
    YuiDocument document;
    const auto result = document.loadFromString ("Scene3D {\n"
                                                 "    Node3D {\n"
                                                 "        id: pair;\n"
                                                 "        position: [1, 2];\n"
                                                 "    }\n"
                                                 "    Node3D { id: odd; scale: [1, 2]; }\n"
                                                 "}");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    expectDiagnostic (document, "Node3D.position: expected 3 numbers");
    expectDiagnostic (document, "Node3D.scale: expected a number");

    // Where the value is written
    const auto* diagnostic = findDiagnostic (document, "Node3D.position");
    ASSERT_NE (diagnostic, nullptr);
    EXPECT_EQ (diagnostic->line, 4);

    // What can't be used places nothing
    auto* pair = document.findById<SceneNodeComponent> ("pair");
    ASSERT_NE (pair, nullptr);
    expectVector (positionOf (*pair), 0.0f, 0.0f, 0.0f);
}

TEST_F (SceneNodeComponentTests, ANumberThatIsNotFiniteDoesNotPlaceANode)
{
    YuiDocument document;
    load (document, "Scene3D { Node3D { id: placed; position: [1, 2, 3]; scale: 2; } }");

    auto* placed = document.findById<SceneNodeComponent> ("placed");
    ASSERT_NE (placed, nullptr);

    // A node that is nowhere would take everything under it along: the value is reported, and places nothing
    placed->setProperty (Identifier ("position"), numbers ({ 1.0, std::numeric_limits<double>::quiet_NaN(), 3.0 }));

    expectDiagnostic (document, "Node3D.position: expected 3 numbers");
    expectVector (positionOf (*placed), 0.0f, 0.0f, 0.0f);

    placed->setProperty (Identifier ("scale"), std::numeric_limits<double>::infinity());

    expectDiagnostic (document, "Node3D.scale: expected a number");
    expectVector (scaleOf (*placed), 1.0f, 1.0f, 1.0f);
}

TEST_F (SceneNodeComponentTests, TheEntitiesFollowTheTreeOfTheComponents)
{
    YuiDocument document;
    load (document, R"yui(
        Scene3D {
            id: view;

            Node3D {
                id: arm;

                Node3D { id: hand; Node3D { id: finger; } }
                Node3D { id: sleeve; }
            }

            Node3D { id: other; }
        })yui");

    auto* view = document.findById<SceneComponent> ("view");
    auto* arm = document.findById<SceneNodeComponent> ("arm");
    auto* hand = document.findById<SceneNodeComponent> ("hand");
    auto* finger = document.findById<SceneNodeComponent> ("finger");
    auto* sleeve = document.findById<SceneNodeComponent> ("sleeve");
    auto* other = document.findById<SceneNodeComponent> ("other");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (arm, nullptr);
    ASSERT_NE (hand, nullptr);
    ASSERT_NE (finger, nullptr);
    ASSERT_NE (sleeve, nullptr);
    ASSERT_NE (other, nullptr);

    const auto root = view->getScene()->getRoot();

    ASSERT_EQ (root.getNumChildren(), 2);
    EXPECT_EQ (root.getChild (0), arm->getEntity());
    EXPECT_EQ (root.getChild (1), other->getEntity());

    ASSERT_EQ (arm->getEntity().getNumChildren(), 2);
    EXPECT_EQ (arm->getEntity().getChild (0), hand->getEntity());
    EXPECT_EQ (arm->getEntity().getChild (1), sleeve->getEntity());

    ASSERT_EQ (hand->getEntity().getNumChildren(), 1);
    EXPECT_EQ (hand->getEntity().getChild (0), finger->getEntity());
    EXPECT_EQ (SceneComponent::findFor (*finger), view);
}

TEST_F (SceneNodeComponentTests, ASubtreeBuiltBeforeItsParentIsInASceneGetsItsEntitiesOnceItIs)
{
    SceneComponent view;
    SceneNodeComponent parent, child, grandchild;

    // Bottom up: nothing is in a scene yet
    child.addAndMakeVisible (grandchild);
    parent.addAndMakeVisible (child);

    EXPECT_FALSE (parent.getEntity().isValid());
    EXPECT_FALSE (child.getEntity().isValid());
    EXPECT_FALSE (grandchild.getEntity().isValid());
    EXPECT_EQ (view.getScene(), nullptr);

    view.addAndMakeVisible (parent);

    ASSERT_NE (view.getScene(), nullptr);
    ASSERT_TRUE (parent.getEntity().isValid());
    ASSERT_TRUE (child.getEntity().isValid());
    ASSERT_TRUE (grandchild.getEntity().isValid());

    EXPECT_EQ (parent.getEntity().getParent(), view.getScene()->getRoot());
    EXPECT_EQ (child.getEntity().getParent(), parent.getEntity());
    EXPECT_EQ (grandchild.getEntity().getParent(), child.getEntity());

    // Top down
    SceneNodeComponent late;
    child.addAndMakeVisible (late);

    ASSERT_TRUE (late.getEntity().isValid());
    EXPECT_EQ (late.getEntity().getParent(), child.getEntity());
    ASSERT_EQ (child.getEntity().getNumChildren(), 2);
    EXPECT_EQ (child.getEntity().getChild (1), late.getEntity());
}

TEST_F (SceneNodeComponentTests, AnEntityKeepsThePlaceOfItsComponentAmongItsSiblings)
{
    SceneComponent view;
    SceneNodeComponent first, second, third;

    view.addAndMakeVisible (first);
    view.addAndMakeVisible (second);

    // Added before the others
    view.addAndMakeVisible (third, 0);

    const auto root = view.getScene()->getRoot();

    ASSERT_EQ (root.getNumChildren(), 3);
    EXPECT_EQ (root.getChild (0), third.getEntity());
    EXPECT_EQ (root.getChild (1), first.getEntity());
    EXPECT_EQ (root.getChild (2), second.getEntity());

    // Moved last, with the same entity
    const auto entity = third.getEntity();
    view.addChildComponent (third);

    EXPECT_EQ (third.getEntity(), entity);
    EXPECT_EQ (root.getChild (0), first.getEntity());
    EXPECT_EQ (root.getChild (1), second.getEntity());
    EXPECT_EQ (root.getChild (2), third.getEntity());
}

TEST_F (SceneNodeComponentTests, AnEntityMovesWithItsComponentToAnotherParentOfItsScene)
{
    SceneComponent view;
    SceneNodeComponent left, right, item, inside;

    view.addAndMakeVisible (left);
    view.addAndMakeVisible (right);
    left.addAndMakeVisible (item);
    item.addAndMakeVisible (inside);

    item.setProperty (Identifier ("position"), numbers ({ 1.0, 0.0, 0.0 }));

    const auto scene = view.getScene();
    const auto before = item.getEntity();
    const auto beforeInside = inside.getEntity();
    ASSERT_TRUE (before.isValid());
    ASSERT_TRUE (beforeInside.isValid());
    ASSERT_EQ (left.getEntity().getNumChildren(), 1);

    // What code gives the entity, which no component describes
    item.getEntity().attach<LightNode>().intensity = 7.0f;

    // To another parent: the entity moves under the entity of that parent, with the ones under it and all they hold
    right.addAndMakeVisible (item);

    EXPECT_EQ (item.getEntity(), before);
    EXPECT_EQ (inside.getEntity(), beforeInside);
    EXPECT_EQ (left.getEntity().getNumChildren(), 0);
    ASSERT_EQ (right.getEntity().getNumChildren(), 1);
    EXPECT_EQ (right.getEntity().getChild (0), before);
    EXPECT_EQ (beforeInside.getParent(), before);
    EXPECT_TRUE (beforeInside.isActive());
    expectVector (positionOf (item), 1.0f, 0.0f, 0.0f);

    ASSERT_NE (before.get<LightNode>(), nullptr);
    EXPECT_FLOAT_EQ (before.get<LightNode>()->intensity, 7.0f);

    // The frame of the scene finds the node in it: there is nothing to let go of
    view.refreshDisplay (0.0);

    EXPECT_EQ (item.getEntity(), before);
    EXPECT_EQ (scene->getNumEntities(), 5);
}

TEST_F (SceneNodeComponentTests, ANodeTakenOutOfItsSceneKeepsItsEntityOutOfTheTreeUntilTheNextFrame)
{
    SceneComponent view;
    SceneNodeComponent parent, item, inside;

    view.addAndMakeVisible (parent);
    parent.addAndMakeVisible (item);
    item.addAndMakeVisible (inside);

    const auto scene = view.getScene();
    const auto entity = item.getEntity();
    const auto insideEntity = inside.getEntity();
    ASSERT_TRUE (entity.isValid());
    ASSERT_TRUE (insideEntity.isValid());

    parent.removeChildComponent (item);

    // Out of the tree of the scene, so neither drawn nor updated, with the entities under it: the node may only
    // be on its way to another parent, and still belongs to the scene
    EXPECT_EQ (item.getEntity(), entity);
    EXPECT_EQ (inside.getEntity(), insideEntity);
    EXPECT_FALSE (entity.getParent().isValid());
    EXPECT_FALSE (entity.isActive());
    EXPECT_FALSE (insideEntity.isActive());
    EXPECT_EQ (parent.getEntity().getNumChildren(), 0);
    EXPECT_EQ (SceneComponent::findFor (item), &view);
    EXPECT_EQ (SceneComponent::findFor (inside), &view);

    // The next frame of the scene finds the node still out of it: its entity goes, with the ones under it
    view.refreshDisplay (0.0);

    EXPECT_FALSE (entity.isValid());
    EXPECT_FALSE (insideEntity.isValid());
    EXPECT_FALSE (item.getEntity().isValid());
    EXPECT_FALSE (inside.getEntity().isValid());
    EXPECT_EQ (SceneComponent::findFor (item), nullptr);
    EXPECT_EQ (SceneComponent::findFor (inside), nullptr);
    EXPECT_EQ (scene->getNumEntities(), 2);

    // Back in the scene after that, the entities are made again
    parent.addAndMakeVisible (item);

    ASSERT_TRUE (item.getEntity().isValid());
    EXPECT_NE (item.getEntity(), entity);
    EXPECT_EQ (item.getEntity().getParent(), parent.getEntity());
    EXPECT_EQ (inside.getEntity().getParent(), item.getEntity());
}

TEST_F (SceneNodeComponentTests, ANodeDeletedOnItsWayOutOfItsSceneTakesItsEntitiesWithIt)
{
    SceneComponent view;
    SceneNodeComponent child;
    auto node = std::make_unique<SceneNodeComponent>();

    view.addAndMakeVisible (*node);
    node->addAndMakeVisible (child);

    const auto scene = view.getScene();
    const auto entity = node->getEntity();
    const auto childEntity = child.getEntity();
    ASSERT_TRUE (entity.isValid());
    ASSERT_TRUE (childEntity.isValid());

    // Taken out, then deleted before any frame of the scene: its entities do not wait for one
    view.removeChildComponent (*node);
    ASSERT_TRUE (entity.isValid());

    node.reset();

    EXPECT_FALSE (entity.isValid());
    EXPECT_FALSE (childEntity.isValid());
    EXPECT_FALSE (child.getEntity().isValid());
    EXPECT_EQ (SceneComponent::findFor (child), nullptr);
    EXPECT_EQ (scene->getNumEntities(), 1);

    // And the frame has nothing left to release
    view.refreshDisplay (0.0);
    EXPECT_EQ (scene->getNumEntities(), 1);
}

TEST_F (SceneNodeComponentTests, ANodeMovedToAnotherSceneMakesItsEntitiesAgainThere)
{
    SceneComponent first, second;
    SceneNodeComponent item, inside;

    first.addAndMakeVisible (item);
    item.addAndMakeVisible (inside);

    const auto firstScene = first.getScene();
    const auto entity = item.getEntity();
    const auto insideEntity = inside.getEntity();
    ASSERT_TRUE (entity.isValid());
    ASSERT_TRUE (insideEntity.isValid());

    second.addAndMakeVisible (item);

    // An entity can't go to another scene: the ones the node had are gone at once, and made again where it is now
    const auto secondScene = second.getScene();
    ASSERT_NE (secondScene, nullptr);
    EXPECT_NE (secondScene, firstScene);

    EXPECT_FALSE (entity.isValid());
    EXPECT_FALSE (insideEntity.isValid());
    EXPECT_EQ (firstScene->getNumEntities(), 1);

    const auto moved = item.getEntity();
    ASSERT_TRUE (moved.isValid());
    EXPECT_EQ (moved.getScene(), secondScene.get());
    EXPECT_EQ (moved.getParent(), secondScene->getRoot());
    EXPECT_EQ (inside.getEntity().getParent(), moved);
    EXPECT_EQ (SceneComponent::findFor (item), &second);

    // The scene it left has nothing to release, and leaves alone the entity the node has in the other one
    first.refreshDisplay (0.0);

    EXPECT_EQ (item.getEntity(), moved);
    EXPECT_TRUE (moved.isValid());
    EXPECT_EQ (secondScene->getNumEntities(), 3);
}

TEST_F (SceneNodeComponentTests, AnEntityKeptForANodeThatLeftGoesWhenItsSceneShowsAnotherOne)
{
    SceneComponent view;
    SceneNodeComponent stay, gone;

    view.addAndMakeVisible (stay);
    view.addAndMakeVisible (gone);

    const auto first = view.getScene();
    const auto entity = gone.getEntity();
    ASSERT_TRUE (entity.isValid());

    view.removeChildComponent (gone);
    ASSERT_TRUE (entity.isValid());

    // The scene the node could come back to is not shown anymore: nothing of the components is left in it
    const auto second = Scene::Ptr (new Scene());
    view.setScene (second);

    EXPECT_FALSE (entity.isValid());
    EXPECT_FALSE (gone.getEntity().isValid());
    EXPECT_EQ (SceneComponent::findFor (gone), nullptr);
    EXPECT_EQ (first->getNumEntities(), 1);
    EXPECT_EQ (stay.getEntity().getScene(), second.get());

    // Coming back, the node makes its entity in the scene shown by then
    view.addAndMakeVisible (gone);

    ASSERT_TRUE (gone.getEntity().isValid());
    EXPECT_EQ (gone.getEntity().getScene(), second.get());
    EXPECT_EQ (second->getRoot().getNumChildren(), 2);
}

TEST_F (SceneNodeComponentTests, AnEntityKeptForANodeThatLeftGoesWhenItsDocumentLoadsAgain)
{
    const String text ("Scene3D { id: view; Node3D { id: group; } }");

    YuiDocument document;
    load (document, text);

    auto* view = document.findById<SceneComponent> ("view");
    auto* group = document.findById<SceneNodeComponent> ("group");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (group, nullptr);

    // A node made by code, which the document does not describe
    SceneNodeComponent extra;
    group->addAndMakeVisible (extra);

    const auto scene = view->getScene();
    const auto entity = extra.getEntity();
    ASSERT_TRUE (entity.isValid());
    ASSERT_EQ (scene->getNumEntities(), 3);

    group->removeChildComponent (extra);
    ASSERT_TRUE (entity.isValid());

    // A load is a frame for what the document keeps: the node is not back in the scene by then
    load (document, text);

    ASSERT_EQ (document.findById<SceneComponent> ("view"), view);
    EXPECT_FALSE (entity.isValid());
    EXPECT_FALSE (extra.getEntity().isValid());
    EXPECT_EQ (scene->getNumEntities(), 2);
}

TEST_F (SceneNodeComponentTests, ASceneThatGoesFirstLeavesNoEntityToTheNodesThatWereInIt)
{
    // The nodes outlive the scene they are in, and the scene it shows outlives both
    SceneNodeComponent stay, child, gone;
    Scene::Ptr scene;
    Entity stayEntity, childEntity, goneEntity;

    {
        Component window;
        SceneComponent view;

        window.addAndMakeVisible (view);
        view.addAndMakeVisible (stay);
        view.addAndMakeVisible (gone);
        stay.addAndMakeVisible (child);

        scene = view.getScene();
        stayEntity = stay.getEntity();
        childEntity = child.getEntity();
        goneEntity = gone.getEntity();

        ASSERT_TRUE (stayEntity.isValid());
        ASSERT_TRUE (childEntity.isValid());
        ASSERT_TRUE (goneEntity.isValid());

        // One of them is on its way out when the scene goes, with no frame left to release its entity
        view.removeChildComponent (gone);
        ASSERT_TRUE (goneEntity.isValid());
    }

    EXPECT_FALSE (goneEntity.isValid());
    EXPECT_FALSE (gone.getEntity().isValid());
    EXPECT_EQ (SceneComponent::findFor (gone), nullptr);

    EXPECT_FALSE (stayEntity.isValid());
    EXPECT_FALSE (childEntity.isValid());
    EXPECT_FALSE (stay.getEntity().isValid());
    EXPECT_FALSE (child.getEntity().isValid());
    EXPECT_EQ (stay.getParentComponent(), nullptr);

    ASSERT_NE (scene, nullptr);
    EXPECT_EQ (scene->getNumEntities(), 1);
}

TEST_F (SceneNodeComponentTests, ANodeInsideAnOrdinaryComponentIsNotInTheScene)
{
    SceneComponent view;
    Component panel;
    SceneNodeComponent node;

    view.addAndMakeVisible (panel);
    panel.addAndMakeVisible (node);

    EXPECT_EQ (SceneComponent::findFor (node), nullptr);
    EXPECT_FALSE (node.getEntity().isValid());
    EXPECT_EQ (view.getScene(), nullptr);
}

TEST_F (SceneNodeComponentTests, DestroyingTheComponentDestroysItsEntity)
{
    SceneComponent view;
    auto node = std::make_unique<SceneNodeComponent>();
    SceneNodeComponent child;

    view.addAndMakeVisible (*node);
    node->addAndMakeVisible (child);

    const auto scene = view.getScene();
    const auto entity = node->getEntity();
    const auto childEntity = child.getEntity();
    ASSERT_TRUE (entity.isValid());
    ASSERT_TRUE (childEntity.isValid());

    node.reset();

    EXPECT_FALSE (entity.isValid());
    EXPECT_FALSE (childEntity.isValid());
    EXPECT_FALSE (child.getEntity().isValid());
    EXPECT_EQ (scene->getRoot().getNumChildren(), 0);
}

TEST_F (SceneNodeComponentTests, FollowsABindingToAnotherComponent)
{
    YuiDocument document;
    load (document, R"yui(
        Component {
            Slider { id: slider; value: 0.25; }

            Scene3D {
                Node3D { id: mover; position: [slider.value, 0, 0]; rotation: [0, slider.value * 180deg, 0]; }
            }
        })yui");

    auto* slider = document.findById<Slider> ("slider");
    auto* mover = document.findById<SceneNodeComponent> ("mover");

    ASSERT_NE (slider, nullptr);
    ASSERT_NE (mover, nullptr);

    expectVector (positionOf (*mover), 0.25f, 0.0f, 0.0f);

    slider->setValue (0.75);

    expectVector (positionOf (*mover), 0.75f, 0.0f, 0.0f);
    expectRotation (rotationOf (*mover), Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.75f * MathConstants<float>::pi));
}

TEST_F (SceneNodeComponentTests, ReloadingKeepsTheEntitiesOfTheElementsThatStay)
{
    const auto text = [] (const String& position, const String& more)
    {
        return "Scene3D { id: view; Node3D { id: mover; position: " + position + "; } " + more + " }";
    };

    YuiDocument document;
    load (document, text ("[1, 0, 0]", "Node3D { id: gone; }"));

    auto* view = document.findById<SceneComponent> ("view");
    auto* mover = document.findById<SceneNodeComponent> ("mover");
    auto* gone = document.findById<SceneNodeComponent> ("gone");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (mover, nullptr);
    ASSERT_NE (gone, nullptr);

    const auto scene = view->getScene();
    const auto moverEntity = mover->getEntity();
    const auto goneEntity = gone->getEntity();

    ASSERT_TRUE (moverEntity.isValid());
    ASSERT_TRUE (goneEntity.isValid());

    load (document, text ("[2, 0, 0]", ""));

    // A value that changed is given to the same component, which keeps its entity
    ASSERT_EQ (document.findById<SceneNodeComponent> ("mover"), mover);
    EXPECT_EQ (mover->getEntity(), moverEntity);
    expectVector (positionOf (*mover), 2.0f, 0.0f, 0.0f);

    // An element that went takes its entity with it
    EXPECT_EQ (document.findById ("gone"), nullptr);
    EXPECT_FALSE (goneEntity.isValid());
    EXPECT_EQ (scene->getRoot().getNumChildren(), 1);

    // A value that went too is not set anymore
    load (document, "Scene3D { id: view; Node3D { id: mover; } }");
    ASSERT_EQ (document.findById<SceneNodeComponent> ("mover"), mover);
    EXPECT_EQ (mover->getEntity(), moverEntity);
    EXPECT_EQ (moverEntity.get<TransformNode>(), nullptr);
}

TEST_F (SceneNodeComponentTests, ReloadingMovesTheEntityOfAnElementWrittenInAnotherParent)
{
    YuiDocument document;
    load (document, "Scene3D { id: view; Node3D { id: left; Node3D { id: item; scale: 3; } } Node3D { id: right; } }");

    auto* left = document.findById<SceneNodeComponent> ("left");
    auto* right = document.findById<SceneNodeComponent> ("right");

    ASSERT_NE (left, nullptr);
    ASSERT_NE (right, nullptr);
    ASSERT_EQ (left->getEntity().getNumChildren(), 1);
    ASSERT_EQ (right->getEntity().getNumChildren(), 0);

    load (document, "Scene3D { id: view; Node3D { id: left; } Node3D { id: right; Node3D { id: item; scale: 3; } } }");

    auto* item = document.findById<SceneNodeComponent> ("item");
    left = document.findById<SceneNodeComponent> ("left");
    right = document.findById<SceneNodeComponent> ("right");

    ASSERT_NE (item, nullptr);
    ASSERT_NE (left, nullptr);
    ASSERT_NE (right, nullptr);

    EXPECT_EQ (left->getEntity().getNumChildren(), 0);
    ASSERT_EQ (right->getEntity().getNumChildren(), 1);
    EXPECT_EQ (right->getEntity().getChild (0), item->getEntity());
    expectVector (scaleOf (*item), 3.0f, 3.0f, 3.0f);
}

TEST_F (SceneNodeComponentTests, AStyleRulePlacesTheNodesItMatches)
{
    YuiDocument document;
    load (document, R"yui(
        style {
            Node3D.big { scale: 2; }
        }

        Scene3D { Node3D { id: item; } })yui");

    auto* item = document.findById<SceneNodeComponent> ("item");
    ASSERT_NE (item, nullptr);

    auto& context = *document.getRoot()->getStyleContext();
    expectVector (scaleOf (*item), 1.0f, 1.0f, 1.0f);

    item->addStyleClass (Identifier ("big"));
    context.restyleNow();
    expectVector (scaleOf (*item), 2.0f, 2.0f, 2.0f);

    item->removeStyleClass (Identifier ("big"));
    context.restyleNow();
    expectVector (scaleOf (*item), 1.0f, 1.0f, 1.0f);
}

TEST_F (SceneNodeComponentTests, ATransitionMovesTheEntityThroughThePositionsInBetween)
{
    YuiDocument document;
    load (document, R"yui(
        style {
            .mover { position: [0, 1, 0]; transition: position 100ms; }
            .moved { position: [2, 1, 4]; }
        }

        Scene3D { Node3D { id: item; class: "mover"; } })yui");

    auto* item = document.findById<SceneNodeComponent> ("item");
    ASSERT_NE (item, nullptr);

    auto& context = *document.getRoot()->getStyleContext();
    expectVector (positionOf (*item), 0.0f, 1.0f, 0.0f);

    item->addStyleClass (Identifier ("moved"));
    context.refresh (0.0);
    expectVector (positionOf (*item), 0.0f, 1.0f, 0.0f);

    context.refresh (0.05);
    expectVector (positionOf (*item), 1.0f, 1.0f, 2.0f, 1.0e-3f);

    context.refresh (0.06);
    expectVector (positionOf (*item), 2.0f, 1.0f, 4.0f);
}

TEST_F (SceneNodeComponentTests, HidingANodeHidesItsEntityAndWhatIsUnderIt)
{
    YuiDocument document;
    load (document, R"yui(
        Scene3D {
            Node3D { id: group; visible: false; Node3D { id: inner; } }
            Node3D { id: shown; }
        })yui");

    auto* group = document.findById<SceneNodeComponent> ("group");
    auto* inner = document.findById<SceneNodeComponent> ("inner");
    auto* shown = document.findById<SceneNodeComponent> ("shown");

    ASSERT_NE (group, nullptr);
    ASSERT_NE (inner, nullptr);
    ASSERT_NE (shown, nullptr);

    EXPECT_FALSE (group->getEntity().isVisible());
    EXPECT_TRUE (inner->getEntity().isVisible());
    EXPECT_FALSE (inner->getEntity().isActive());
    EXPECT_TRUE (shown->getEntity().isActive());

    group->setVisible (true);

    EXPECT_TRUE (group->getEntity().isVisible());
    EXPECT_TRUE (inner->getEntity().isActive());
}

TEST_F (SceneNodeComponentTests, FillsItsSceneAndTakesNoMouseEventWhereItShowsNothing)
{
    YuiDocument document;
    load (document, "Scene3D { id: view; Node3D { id: outer; x: 10; width: 20; Node3D { id: inner; } } }");

    auto* view = document.findById<SceneComponent> ("view");
    auto* outer = document.findById<SceneNodeComponent> ("outer");
    auto* inner = document.findById<SceneNodeComponent> ("inner");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (outer, nullptr);
    ASSERT_NE (inner, nullptr);

    view->setBounds (5.0f, 5.0f, 200.0f, 100.0f);

    // The 2D bounds a node is given have no effect: it has the area of its scene
    EXPECT_EQ (outer->getBounds(), Rectangle<float> (0.0f, 0.0f, 200.0f, 100.0f));
    EXPECT_EQ (inner->getBounds(), Rectangle<float> (0.0f, 0.0f, 200.0f, 100.0f));

    outer->setBounds (10.0f, 10.0f, 20.0f, 20.0f);
    EXPECT_EQ (outer->getBounds(), Rectangle<float> (0.0f, 0.0f, 200.0f, 100.0f));

    // It paints nothing, and takes the pointer only where it shows something: these draw no mesh
    EXPECT_FALSE (outer->isOpaque());
    EXPECT_FALSE (outer->hitTest (50.0f, 50.0f));
    EXPECT_FALSE (inner->hitTest (50.0f, 50.0f));
}

TEST_F (SceneNodeComponentTests, AnArrayReadFromANodeIsNotTheOneItKeeps)
{
    YuiDocument document;
    load (document, "Scene3D { Node3D { id: placed; position: [1, 0, 0]; } Node3D { id: group; } Node3D { id: other; } }");

    auto* placed = document.findById<SceneNodeComponent> ("placed");
    auto* group = document.findById<SceneNodeComponent> ("group");
    auto* other = document.findById<SceneNodeComponent> ("other");

    ASSERT_NE (placed, nullptr);
    ASSERT_NE (group, nullptr);
    ASSERT_NE (other, nullptr);

    const Identifier position ("position");

    // Changed in place, the array that was read leaves the node where it is
    auto read = placed->getProperty (position);
    ASSERT_NE (read.getArray(), nullptr);
    read.getArray()->set (1, 2.0);

    expectNumbers (placed->getProperty (position), { 1.0, 0.0, 0.0 });
    expectVector (positionOf (*placed), 1.0f, 0.0f, 0.0f);

    // Given back, it is another value: the node moves
    placed->setProperty (position, read);
    expectVector (positionOf (*placed), 1.0f, 2.0f, 0.0f);

    // And the node does not follow the array it was given either
    read.getArray()->set (2, 3.0);

    expectNumbers (placed->getProperty (position), { 1.0, 2.0, 0.0 });
    expectVector (positionOf (*placed), 1.0f, 2.0f, 0.0f);

    // What a node that sets nothing reads is its own too: the other nodes still read the identity
    auto identity = group->getProperty (position);
    ASSERT_NE (identity.getArray(), nullptr);
    identity.getArray()->set (1, 2.0);

    expectNumbers (group->getProperty (position), { 0.0, 0.0, 0.0 });
    expectNumbers (other->getProperty (position), { 0.0, 0.0, 0.0 });

    group->setProperty (position, identity);

    expectVector (positionOf (*group), 0.0f, 2.0f, 0.0f);
    expectNumbers (other->getProperty (position), { 0.0, 0.0, 0.0 });
    EXPECT_EQ (other->getEntity().get<TransformNode>(), nullptr);
}

TEST_F (SceneNodeComponentTests, ANodeThatCanNotBeBoundSaysSoOnceItLetGoOfTheEntityItHad)
{
    files["device.gltf"] = modelFile();

    YuiDocument document;
    serve (document);
    load (document, R"yui(
        Scene3D {
            Model3D {
                source: "device.gltf";

                Node3D { id: knob; node: "Knob"; position: [0, 1, 0]; }
            }
        })yui");

    auto* knob = document.findById<SceneNodeComponent> ("knob");
    ASSERT_NE (knob, nullptr);

    const auto bound = knob->getEntity();
    ASSERT_TRUE (bound.isValid());
    ASSERT_NE (bound.get<TransformNode>(), nullptr);
    expectVector (bound.get<TransformNode>()->getPosition(), 0.0f, 1.0f, 0.0f);

    // Who hears of the problem may destroy the node: by then the node has nothing left to do, and the
    // entity of the model has what the model gave it
    StringArray told;
    bool hadEntityWhenTold = true;
    Vector3<float> positionWhenTold;

    const DiagnosticListener listener (document, [&] (const DataTreeDiagnostic& diagnostic)
    {
        told.add (diagnostic.message);
        hadEntityWhenTold = knob->getEntity().isValid();
        positionWhenTold = bound.get<TransformNode>()->getPosition();
    });

    knob->setNodeName ("Dial");

    ASSERT_EQ (told.size(), 1);
    EXPECT_EQ (told[0], "Node3D.node: the model has no entity 'Dial'");
    EXPECT_FALSE (hadEntityWhenTold);
    expectVector (positionWhenTold, 1.0f, 2.0f, 3.0f);
}

TEST_F (SceneNodeComponentTests, AnEntityTwoNodesAreBoundToGetsBackWhatTheModelGaveItWhicheverLetsGoFirst)
{
    files["device.gltf"] = modelFile();

    for (const auto firstGoesFirst : { true, false })
    {
        SCOPED_TRACE (firstGoesFirst ? "the first one lets go first" : "the second one lets go first");

        YuiDocument document;
        serve (document);
        load (document, R"yui(
            Scene3D {
                Model3D {
                    id: device; source: "device.gltf";

                    Node3D { id: first; node: "Knob"; position: [0, 1, 0]; }
                    Node3D { id: second; node: "Knob"; position: [0, 0, 5]; visible: false; }
                }
            })yui");

        auto* device = document.findById<SceneModelComponent> ("device");
        auto* first = document.findById<SceneNodeComponent> ("first");
        auto* second = document.findById<SceneNodeComponent> ("second");

        ASSERT_NE (device, nullptr);
        ASSERT_NE (first, nullptr);
        ASSERT_NE (second, nullptr);

        const auto knob = first->getEntity();
        ASSERT_TRUE (knob.isValid());
        ASSERT_EQ (second->getEntity(), knob);
        ASSERT_NE (knob.get<TransformNode>(), nullptr);

        // Both drive the entity, where the last one to write shows. What the model gave it is what both took when
        // they were bound, not what the first one had made of it by then
        device->removeChildComponent (firstGoesFirst ? first : second);
        device->removeChildComponent (firstGoesFirst ? second : first);

        ASSERT_TRUE (knob.isValid());
        expectVector (knob.get<TransformNode>()->getPosition(), 1.0f, 2.0f, 3.0f);
        EXPECT_TRUE (knob.isVisible());
    }
}

TEST_F (SceneNodeComponentTests, ABoundNodeTurningAJointBendsTheSkinBeforeTheSceneIsPainted)
{
    files["arm.gltf"] = skinnedFile();

    YuiDocument document;
    serve (document);
    load (document, R"yui(
        Scene3D {
            id: view;

            Model3D {
                id: arm; source: "arm.gltf";

                Node3D { id: tip; node: "Tip"; rotation: [0, 0, 90deg]; }
            }
        })yui");

    auto* view = document.findById<SceneComponent> ("view");
    auto* arm = document.findById<SceneModelComponent> ("arm");
    auto* tip = document.findById<SceneNodeComponent> ("tip");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (arm, nullptr);
    ASSERT_NE (tip, nullptr);
    ASSERT_TRUE (tip->getEntity().isValid());

    const auto strip = arm->getInstance().findChild ("Strip");
    ASSERT_TRUE (strip.isValid());
    ASSERT_NE (strip.get<MeshNode>(), nullptr);
    ASSERT_NE (strip.get<MeshNode>()->mesh, nullptr);

    const auto vertexAt = [&strip] (size_t index)
    {
        return strip.get<MeshNode>()->mesh->getPrimitive (0).vertices[index].position;
    };

    // What the scene does before it is drawn, when a node changed since: the strip follows the joint the node
    // turned around 0, 1, so its top, 1 above the joint, is 1 to the left of it
    view->updateScene();

    expectVector (vertexAt (0), 0.0f, 0.0f, 0.0f);
    expectVector (vertexAt (1), 1.0f, 0.0f, 0.0f);
    expectVector (vertexAt (2), -1.0f, 1.0f, 0.0f);
    expectVector (vertexAt (3), -1.0f, 2.0f, 0.0f);

    // And again after each change of a node
    tip->setProperty (Identifier ("rotation"), numbers ({ 0.0, 0.0, 0.0 }));
    view->updateScene();

    expectVector (vertexAt (2), 0.0f, 2.0f, 0.0f);
    expectVector (vertexAt (3), 1.0f, 2.0f, 0.0f);
}
