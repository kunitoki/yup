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

/** The nodes of a `Scene3D` that are controls: their `value` moves them along their `travel` and
    turns them by their `turn`, and dragging them changes it.

    The scenes of sceneOf() are seen through the camera of the fixture, 50 pixels for each unit
    around 100, 100, x to the right and y up. The handle of a control is a square 0.4 wide, so
    20 pixels.
*/
class SceneDraggablePartTests : public SceneDocumentTests
{
protected:
    /** A Mesh3D drawing a square 0.4 wide around where it is, with an id and the properties written for it. */
    static String handle (const String& id, const String& properties)
    {
        return "Mesh3D { id: " + id + "; scale: 0.2; " + properties + " " + quadGeometry() + " }";
    }

    static Quaternion aroundX (float degrees)
    {
        return Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, degrees * MathConstants<float>::pi / 180.0f);
    }

    static Quaternion aroundY (float degrees)
    {
        return Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, degrees * MathConstants<float>::pi / 180.0f);
    }

    static Quaternion aroundZ (float degrees)
    {
        return Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, degrees * MathConstants<float>::pi / 180.0f);
    }

    /** Where the origin of the entity of a node is in its scene. */
    static Vector3<float> placeInSceneOf (const SceneNodeComponent& node)
    {
        return node.getEntity().getWorldMatrix().transformPoint ({});
    }
};

TEST_F (SceneDraggablePartTests, TheValueMovesANodeAlongItsTravel)
{
    YuiDocument document;
    load (document, R"yui(
        Scene3D {
            Node3D { id: plain; travel: [2, 0, 0]; value: 0.25; }
            Node3D { id: placed; position: [1, 2, 3]; travel: [0, -2, 0]; value: 0.5; }
            Node3D { id: still; position: [1, 2, 3]; value: 0.5; }
            Node3D { id: group; value: 0.5; }
        })yui");

    auto* plain = document.findById<SceneNodeComponent> ("plain");
    auto* placed = document.findById<SceneNodeComponent> ("placed");
    auto* still = document.findById<SceneNodeComponent> ("still");
    auto* group = document.findById<SceneNodeComponent> ("group");

    ASSERT_NE (plain, nullptr);
    ASSERT_NE (placed, nullptr);
    ASSERT_NE (still, nullptr);
    ASSERT_NE (group, nullptr);

    const Identifier position ("position"), travel ("travel"), turn ("turn"), value ("value");

    // From where it is without its travel: the origin of its parent, or its position
    expectVector (positionOf (*plain), 0.5f, 0.0f, 0.0f);
    expectVector (positionOf (*placed), 1.0f, 1.0f, 3.0f);

    // A node that does not travel is not moved by its value, and one that sets nothing else stays a plain group
    expectVector (positionOf (*still), 1.0f, 2.0f, 3.0f);
    EXPECT_EQ (group->getEntity().get<TransformNode>(), nullptr);
    EXPECT_DOUBLE_EQ (group->getValue(), 0.5);

    // What was set reads back as it was set, the position without the travel, and what was not as nothing
    expectNumbers (placed->getProperty (position), { 1.0, 2.0, 3.0 });
    expectNumbers (placed->getProperty (travel), { 0.0, -2.0, 0.0 });
    EXPECT_DOUBLE_EQ (static_cast<double> (placed->getProperty (value)), 0.5);
    EXPECT_TRUE (still->getProperty (travel).isVoid());
    EXPECT_TRUE (placed->getProperty (turn).isVoid());

    // Nothing else of the node is touched
    expectRotation (rotationOf (*placed), Quaternion());
    expectVector (scaleOf (*placed), 1.0f, 1.0f, 1.0f);

    // The value moves it between the two ends of its travel, and no further
    placed->setProperty (value, 1.0);
    expectVector (positionOf (*placed), 1.0f, 0.0f, 3.0f);

    placed->setProperty (value, 3.0);
    EXPECT_DOUBLE_EQ (placed->getValue(), 1.0);
    expectVector (positionOf (*placed), 1.0f, 0.0f, 3.0f);

    placed->setValue (-2.0);
    EXPECT_DOUBLE_EQ (placed->getValue(), 0.0);
    expectVector (positionOf (*placed), 1.0f, 2.0f, 3.0f);

    // Another travel, another position, take it where its value is along them
    placed->setValue (0.5);
    placed->setProperty (travel, numbers ({ 4.0, 0.0, 0.0 }));
    expectVector (positionOf (*placed), 3.0f, 2.0f, 3.0f);

    placed->setProperty (position, numbers ({ 0.0, 0.0, 0.0 }));
    expectVector (positionOf (*placed), 2.0f, 0.0f, 0.0f);

    // Without a travel the node is where its position puts it, and one that sets nothing anymore is a plain group again
    placed->setProperty (travel, var());
    expectVector (positionOf (*placed), 0.0f, 0.0f, 0.0f);

    plain->setProperty (travel, var());
    EXPECT_EQ (plain->getEntity().get<TransformNode>(), nullptr);
}

TEST_F (SceneDraggablePartTests, TheValueTurnsANodeInItsOwnSpace)
{
    YuiDocument document;
    load (document, R"yui(
        Scene3D {
            Node3D { id: knob; turn: [0, 0, 180deg]; value: 0.5; }
            Node3D { id: tilted; rotation: [90deg, 0, 0]; turn: [0, 90deg, 0]; value: 1; }
            Node3D { id: aimed; lookAt: [1, 0, 0]; turn: [0, 0, 90deg]; value: 1; }
            Node3D { id: looking; lookAt: [1, 0, 0]; }
        })yui");

    auto* knob = document.findById<SceneNodeComponent> ("knob");
    auto* tilted = document.findById<SceneNodeComponent> ("tilted");
    auto* aimed = document.findById<SceneNodeComponent> ("aimed");
    auto* looking = document.findById<SceneNodeComponent> ("looking");

    ASSERT_NE (knob, nullptr);
    ASSERT_NE (tilted, nullptr);
    ASSERT_NE (aimed, nullptr);
    ASSERT_NE (looking, nullptr);

    // Half of its turn, and it stays where it is
    expectRotation (rotationOf (*knob), aroundZ (90.0f));
    expectVector (positionOf (*knob), 0.0f, 0.0f, 0.0f);

    // After the rotation that orients it, around its own axes: its y, which that rotation made the z of its parent
    expectRotation (rotationOf (*tilted), aroundX (90.0f) * aroundY (90.0f));
    expectRotation (rotationOf (*tilted), aroundZ (90.0f) * aroundX (90.0f));
    expectNumbers (tilted->getProperty (Identifier ("rotation")), { MathConstants<double>::halfPi, 0.0, 0.0 });

    // A node that looks at a point is turned to it, whatever its turn
    expectRotation (rotationOf (*aimed), rotationOf (*looking));

    // The angles go with the value
    knob->setValue (0.25);
    expectRotation (rotationOf (*knob), aroundZ (45.0f));

    knob->setValue (0.0);
    expectRotation (rotationOf (*knob), Quaternion());

    // Without a turn the node has the rotation it gave itself
    tilted->setProperty (Identifier ("turn"), var());
    expectRotation (rotationOf (*tilted), aroundX (90.0f));
}

TEST_F (SceneDraggablePartTests, ABoundNodeTravelsFromWhereTheModelPutItAndGivesItBack)
{
    files["device.gltf"] = modelFile();

    YuiDocument document;
    serve (document);
    load (document, R"yui(
        Scene3D {
            Model3D {
                id: model; source: "device.gltf";

                Node3D { id: fader; node: "Knob"; travel: [0, -1, 0]; turn: [0, 0, 90deg]; value: 0.5; }
            }
        })yui");

    auto* model = document.findById<SceneModelComponent> ("model");
    auto* fader = document.findById<SceneNodeComponent> ("fader");

    ASSERT_NE (model, nullptr);
    ASSERT_NE (fader, nullptr);

    const Identifier position ("position"), travel ("travel"), turn ("turn");

    const auto part = fader->getEntity();
    ASSERT_TRUE (part.isValid());
    ASSERT_NE (part.get<TransformNode>(), nullptr);

    // The model put the part at 1, 2, 3 and did not turn it: the node moves and turns it from there
    expectVector (positionOf (*fader), 1.0f, 1.5f, 3.0f);
    expectRotation (rotationOf (*fader), aroundZ (45.0f));
    expectVector (scaleOf (*fader), 1.0f, 1.0f, 1.0f);

    // It does not set the position of the part, which reads as nothing
    EXPECT_TRUE (fader->getProperty (position).isVoid());

    fader->setValue (1.0);
    expectVector (positionOf (*fader), 1.0f, 1.0f, 3.0f);
    expectRotation (rotationOf (*fader), aroundZ (90.0f));

    // A position of its own is where it travels from, for as long as it sets one
    fader->setProperty (position, numbers ({ 0.0, 0.0, 0.0 }));
    expectVector (positionOf (*fader), 0.0f, -1.0f, 0.0f);

    fader->setProperty (position, var());
    expectVector (positionOf (*fader), 1.0f, 1.0f, 3.0f);

    // Without a travel the part is back where the model put it, still turned. Without a turn, as the model turned it
    fader->setProperty (travel, var());
    expectVector (positionOf (*fader), 1.0f, 2.0f, 3.0f);
    expectRotation (rotationOf (*fader), aroundZ (90.0f));

    fader->setProperty (turn, var());
    expectRotation (rotationOf (*fader), Quaternion());

    // And so it is once a node that moved and turned it is gone
    fader->setProperty (travel, numbers ({ 0.0, -1.0, 0.0 }));
    fader->setProperty (turn, numbers ({ 0.0, 0.0, 1.0 }));
    expectVector (part.get<TransformNode>()->getPosition(), 1.0f, 1.0f, 3.0f);

    model->removeChildComponent (fader);

    ASSERT_TRUE (part.isValid());
    ASSERT_NE (part.get<TransformNode>(), nullptr);
    expectVector (part.get<TransformNode>()->getPosition(), 1.0f, 2.0f, 3.0f);
    expectRotation (part.get<TransformNode>()->getRotation(), Quaternion());
}

TEST_F (SceneDraggablePartTests, ANodeTravelsInTheSpaceOfItsParent)
{
    // The parent is 1 to the right and turned a quarter of a turn: what goes along its x goes up in the scene
    YuiDocument document;
    load (document, sceneOf ("Node3D { id: arm; position: [1, 0, 0]; rotation: [0, 0, 90deg]; " + handle ("fader", "travel: [1, 0, 0]; value: 0.5;") + " }"));

    auto* view = showSceneOf (document);
    auto* fader = document.findById<SceneMeshComponent> ("fader");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (fader, nullptr);

    expectVector (positionOf (*fader), 0.5f, 0.0f, 0.0f);
    expectVector (placeInSceneOf (*fader), 1.0f, 0.5f, 0.0f);

    // The scene shows the travel going up from 150, 100 to 150, 50, and the node halfway: it follows the pointer along that line
    PointerSimulator pointer (*view);

    EXPECT_EQ (pointer.moveTo ({ 150.0f, 75.0f }), fader);
    EXPECT_EQ (pointer.press(), fader);

    pointer.moveTo ({ 150.0f, 65.0f });
    EXPECT_NEAR (fader->getValue(), 0.7, 1.0e-4);
    expectVector (placeInSceneOf (*fader), 1.0f, 0.7f, 0.0f);

    // And not across it
    pointer.moveTo ({ 190.0f, 65.0f });
    EXPECT_NEAR (fader->getValue(), 0.7, 1.0e-4);

    pointer.moveTo ({ 190.0f, 90.0f });
    EXPECT_NEAR (fader->getValue(), 0.2, 1.0e-4);
}

TEST_F (SceneDraggablePartTests, DraggingANodeMovesItsValueAlongTheTravelTheSceneShows)
{
    // A handle that travels 2 to the right from -1, 0: the scene shows it going from 50, 100 to 150, 100
    YuiDocument document;
    load (document, sceneOf (handle ("fader", "position: [-1, 0, 0]; travel: [2, 0, 0];")));

    auto* view = showSceneOf (document);
    auto* fader = document.findById<SceneMeshComponent> ("fader");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (fader, nullptr);

    PointerSimulator pointer (*view);

    EXPECT_EQ (pointer.moveTo ({ 50.0f, 100.0f }), fader);
    EXPECT_EQ (pointer.press(), fader);
    EXPECT_DOUBLE_EQ (fader->getValue(), 0.0);

    // By the part of that line the pointer went along since the press, one way and the other: the node stays under it
    EXPECT_EQ (pointer.moveTo ({ 75.0f, 100.0f }), fader);
    EXPECT_NEAR (fader->getValue(), 0.25, 1.0e-4);
    expectVector (positionOf (*fader), -0.5f, 0.0f, 0.0f);

    pointer.moveTo ({ 100.0f, 100.0f });
    EXPECT_NEAR (fader->getValue(), 0.5, 1.0e-4);

    pointer.moveTo ({ 60.0f, 100.0f });
    EXPECT_NEAR (fader->getValue(), 0.1, 1.0e-4);

    // The pointer keeps the node when it leaves what the node shows
    EXPECT_EQ (pointer.moveTo ({ 100.0f, 180.0f }), fader);
    EXPECT_NEAR (fader->getValue(), 0.5, 1.0e-4);
    EXPECT_EQ (view->findNodeAt ({ 100.0f, 180.0f }), nullptr);

    // And takes it to the ends of its travel, no further
    pointer.moveTo ({ 400.0f, 100.0f });
    EXPECT_DOUBLE_EQ (fader->getValue(), 1.0);
    expectVector (positionOf (*fader), 1.0f, 0.0f, 0.0f);

    pointer.moveTo ({ -300.0f, 100.0f });
    EXPECT_DOUBLE_EQ (fader->getValue(), 0.0);
    expectVector (positionOf (*fader), -1.0f, 0.0f, 0.0f);

    pointer.moveTo ({ 90.0f, 100.0f });
    EXPECT_NEAR (fader->getValue(), 0.4, 1.0e-4);
    EXPECT_EQ (pointer.release(), fader);

    // The next press drags it from the value it has: the node is now shown at 90, 100
    EXPECT_EQ (pointer.moveTo ({ 90.0f, 100.0f }), fader);
    EXPECT_EQ (pointer.press(), fader);

    pointer.moveTo ({ 65.0f, 100.0f });
    EXPECT_NEAR (fader->getValue(), 0.15, 1.0e-4);
}

TEST_F (SceneDraggablePartTests, ATravelGoingIntoTheDepthOfAPerspectiveIsDraggedAsItIsShown)
{
    // A field of view whose half has a tangent of 0.5: 50 pixels for each unit at 4 from the camera, 25 at 8. The
    // handle is 1 to the right and travels 4 away from the camera, which shows that going from 150, 100 to 125, 100
    YuiDocument document;
    load (document, "Scene3D { id: view; Camera3D { position: [0, 0, 4]; fov: 0.9272952; } " + handle ("fader", "position: [1, 0, 0]; travel: [0, 0, -4];") + " }");

    auto* view = showSceneOf (document);
    auto* fader = document.findById<SceneMeshComponent> ("fader");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (fader, nullptr);

    PointerSimulator pointer (*view);

    EXPECT_EQ (pointer.moveTo ({ 150.0f, 100.0f }), fader);
    EXPECT_EQ (pointer.press(), fader);

    // 10 of the 25 pixels of that line, towards its far end
    pointer.moveTo ({ 140.0f, 100.0f });
    EXPECT_NEAR (fader->getValue(), 0.4, 1.0e-3);
    expectVector (positionOf (*fader), 1.0f, 0.0f, -1.6f, 1.0e-2f);

    // And back towards the camera
    pointer.moveTo ({ 145.0f, 100.0f });
    EXPECT_NEAR (fader->getValue(), 0.2, 1.0e-3);
}

TEST_F (SceneDraggablePartTests, ATravelPointingAtTheCameraIsNotDragged)
{
    YuiDocument document;
    load (document, sceneOf (handle ("fader", "travel: [0, 0, 1];")));

    auto* view = showSceneOf (document);
    auto* fader = document.findById<SceneMeshComponent> ("fader");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (fader, nullptr);

    const Identifier travel ("travel");
    PointerSimulator pointer (*view);

    EXPECT_EQ (pointer.moveTo ({ 100.0f, 100.0f }), fader);
    EXPECT_EQ (pointer.press(), fader);

    // The scene shows the travel as a point: the pointer has no line to follow
    pointer.moveTo ({ 150.0f, 60.0f });
    EXPECT_DOUBLE_EQ (fader->getValue(), 0.0);

    // Nor has it along 2 pixels
    fader->setProperty (travel, numbers ({ 0.04, 0.0, 1.0 }));
    pointer.moveTo ({ 160.0f, 60.0f });
    EXPECT_DOUBLE_EQ (fader->getValue(), 0.0);

    // Along 10 pixels the node is steered again: 5 of them since the press are half of its range
    fader->setProperty (travel, numbers ({ 0.2, 0.0, 1.0 }));
    pointer.moveTo ({ 105.0f, 60.0f });
    EXPECT_NEAR (fader->getValue(), 0.5, 1.0e-3);
}

TEST_F (SceneDraggablePartTests, ANodeThatOnlyTurnsFollowsThePointerUpAndDown)
{
    YuiDocument document;
    load (document, sceneOf (handle ("knob", "turn: [0, 0, 270deg];")));

    auto* view = showSceneOf (document);
    auto* knob = document.findById<SceneMeshComponent> ("knob");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (knob, nullptr);

    PointerSimulator pointer (*view);

    EXPECT_EQ (pointer.moveTo ({ 100.0f, 100.0f }), knob);
    EXPECT_EQ (pointer.press(), knob);

    // Up for more, 200 units of the pointer for the whole range: it turns where it is
    pointer.moveTo ({ 100.0f, 50.0f });
    EXPECT_NEAR (knob->getValue(), 0.25, 1.0e-4);
    expectRotation (rotationOf (*knob), aroundZ (67.5f));
    expectVector (positionOf (*knob), 0.0f, 0.0f, 0.0f);

    // Whatever the pointer does sideways
    pointer.moveTo ({ 170.0f, 50.0f });
    EXPECT_NEAR (knob->getValue(), 0.25, 1.0e-4);

    // To the ends of its turn, no further
    pointer.moveTo ({ 100.0f, 130.0f });
    EXPECT_DOUBLE_EQ (knob->getValue(), 0.0);

    pointer.moveTo ({ 100.0f, -200.0f });
    EXPECT_DOUBLE_EQ (knob->getValue(), 1.0);

    pointer.moveTo ({ 100.0f, 0.0f });
    EXPECT_NEAR (knob->getValue(), 0.5, 1.0e-4);
    EXPECT_EQ (pointer.release(), knob);

    // A node that travels too is dragged along its travel. Half of this one puts the knob at 125, 100, where
    // 10 pixels to the right are a fifth of the 50 it travels
    knob->setProperty (Identifier ("travel"), numbers ({ 1.0, 0.0, 0.0 }));
    expectVector (positionOf (*knob), 0.5f, 0.0f, 0.0f);

    EXPECT_EQ (pointer.moveTo ({ 125.0f, 100.0f }), knob);
    EXPECT_EQ (pointer.press(), knob);

    pointer.moveTo ({ 125.0f, 40.0f });
    EXPECT_NEAR (knob->getValue(), 0.5, 1.0e-4);

    pointer.moveTo ({ 135.0f, 40.0f });
    EXPECT_NEAR (knob->getValue(), 0.7, 1.0e-4);
    expectVector (positionOf (*knob), 0.7f, 0.0f, 0.0f);
    expectRotation (rotationOf (*knob), aroundZ (189.0f));
}

TEST_F (SceneDraggablePartTests, TheValueIsLinkedBothWaysWithASlider)
{
    // The slider is written over the scene, under where the handle travels
    YuiDocument document;
    load (document, sceneOf (handle ("fader", "position: [-1, 0, 0]; travel: [2, 0, 0]; value <=> cutoff.value;")
                             + " Slider { id: cutoff; value: 0.5; x: 0; y: 170; width: 200; height: 30; }"));

    auto* view = showSceneOf (document);
    auto* fader = document.findById<SceneMeshComponent> ("fader");
    auto* cutoff = document.findById<Slider> ("cutoff");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (fader, nullptr);
    ASSERT_NE (cutoff, nullptr);

    // The node takes the value of the slider, and is where that puts it
    EXPECT_DOUBLE_EQ (fader->getValue(), 0.5);
    expectVector (positionOf (*fader), 0.0f, 0.0f, 0.0f);

    // The slider moves the node
    cutoff->setValue (0.25);
    EXPECT_DOUBLE_EQ (fader->getValue(), 0.25);
    expectVector (positionOf (*fader), -0.5f, 0.0f, 0.0f);

    // And the node the slider: set by code, then dragged from 125, 100 where the scene shows it
    fader->setValue (0.75);
    EXPECT_DOUBLE_EQ (cutoff->getValue(), 0.75);

    PointerSimulator pointer (*view);

    EXPECT_EQ (pointer.moveTo ({ 125.0f, 100.0f }), fader);
    EXPECT_EQ (pointer.press(), fader);
    pointer.moveTo ({ 100.0f, 100.0f });
    EXPECT_EQ (pointer.release(), fader);

    EXPECT_NEAR (fader->getValue(), 0.5, 1.0e-4);
    EXPECT_DOUBLE_EQ (cutoff->getValue(), fader->getValue());

    // Dragging the slider itself takes the node along
    const auto before = cutoff->getValue();

    EXPECT_EQ (pointer.drag ({ 40.0f, 185.0f }, { 160.0f, 185.0f }), cutoff);
    EXPECT_NE (cutoff->getValue(), before);
    EXPECT_DOUBLE_EQ (fader->getValue(), cutoff->getValue());
    expectVector (positionOf (*fader), static_cast<float> (cutoff->getValue()) * 2.0f - 1.0f, 0.0f, 0.0f);
}

TEST_F (SceneDraggablePartTests, ADragKeepsTheNodePressedAndIsNotAClick)
{
    int clicks = 0;

    YuiDocument document;

    document.setAction ("clicked", [&clicks] (const Array<var>&)
    {
        ++clicks;
    });

    load (document, "style { Mesh3D:pressed { rotation: [0, 0, 45deg]; } }\n"
                        + sceneOf (handle ("fader", "position: [-1, 0, 0]; travel: [2, 0, 0]; onClick: action (\"clicked\");")));

    auto* view = showSceneOf (document);
    auto* fader = document.findById<SceneMeshComponent> ("fader");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (fader, nullptr);

    auto& context = *document.getRoot()->getStyleContext();
    PointerSimulator pointer (*view);

    // Pressed for as long as it is dragged, whether the pointer is over what it shows or not
    EXPECT_EQ (pointer.moveTo ({ 50.0f, 100.0f }), fader);
    EXPECT_EQ (pointer.press(), fader);
    context.restyleNow();

    EXPECT_TRUE (fader->isPressed());
    expectRotation (rotationOf (*fader), aroundZ (45.0f));

    pointer.moveTo ({ 100.0f, 160.0f });
    context.restyleNow();

    EXPECT_NEAR (fader->getValue(), 0.5, 1.0e-4);
    EXPECT_TRUE (fader->isPressed());
    expectRotation (rotationOf (*fader), aroundZ (45.0f));

    // Released over itself: a press that moved the value is not a click
    pointer.moveTo ({ 100.0f, 100.0f });
    EXPECT_EQ (pointer.release(), fader);
    context.restyleNow();

    EXPECT_FALSE (fader->isPressed());
    expectRotation (rotationOf (*fader), Quaternion());
    EXPECT_NEAR (fader->getValue(), 0.5, 1.0e-4);
    EXPECT_EQ (clicks, 0);

    // A press that did not move it is one: with the pointer still, or moving across the travel
    EXPECT_EQ (pointer.click ({ 100.0f, 100.0f }), fader);
    EXPECT_EQ (clicks, 1);

    EXPECT_EQ (pointer.drag ({ 100.0f, 96.0f }, { 100.0f, 104.0f }), fader);
    EXPECT_NEAR (fader->getValue(), 0.5, 1.0e-4);
    EXPECT_EQ (clicks, 2);
}

TEST_F (SceneDraggablePartTests, ADisabledNodeIsNotDragged)
{
    YuiDocument document;
    load (document, sceneOf (handle ("fader", "position: [-1, 0, 0]; travel: [2, 0, 0]; enabled: false;")));

    auto* view = showSceneOf (document);
    auto* fader = document.findById<SceneMeshComponent> ("fader");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (fader, nullptr);

    PointerSimulator pointer (*view);

    // It is there for the pointer, which does nothing to it
    EXPECT_EQ (pointer.moveTo ({ 50.0f, 100.0f }), fader);
    pointer.press();
    pointer.moveTo ({ 100.0f, 100.0f });

    EXPECT_FALSE (fader->isPressed());
    EXPECT_DOUBLE_EQ (fader->getValue(), 0.0);

    pointer.release();
    EXPECT_DOUBLE_EQ (fader->getValue(), 0.0);

    // Its value still places it
    fader->setValue (0.5);
    expectVector (positionOf (*fader), 0.0f, 0.0f, 0.0f);

    // Enabled, it is dragged from there
    fader->setEnabled (true);

    EXPECT_EQ (pointer.moveTo ({ 100.0f, 100.0f }), fader);
    EXPECT_EQ (pointer.press(), fader);
    pointer.moveTo ({ 125.0f, 100.0f });

    EXPECT_TRUE (fader->isPressed());
    EXPECT_NEAR (fader->getValue(), 0.75, 1.0e-4);
}

TEST_F (SceneDraggablePartTests, ATransitionMovesTheNodeThroughTheValuesInBetween)
{
    YuiDocument document;
    load (document, R"yui(
        style {
            .fader { value: 0; transition: value 100ms; }
            .open { value: 1; }
        }

        Scene3D { Node3D { id: fader; class: "fader"; travel: [2, 0, 4]; } })yui");

    auto* fader = document.findById<SceneNodeComponent> ("fader");
    ASSERT_NE (fader, nullptr);

    auto& context = *document.getRoot()->getStyleContext();
    expectVector (positionOf (*fader), 0.0f, 0.0f, 0.0f);

    fader->addStyleClass (Identifier ("open"));
    context.refresh (0.0);
    expectVector (positionOf (*fader), 0.0f, 0.0f, 0.0f);

    context.refresh (0.05);
    EXPECT_NEAR (fader->getValue(), 0.5, 1.0e-3);
    expectVector (positionOf (*fader), 1.0f, 0.0f, 2.0f, 1.0e-2f);

    context.refresh (0.06);
    EXPECT_DOUBLE_EQ (fader->getValue(), 1.0);
    expectVector (positionOf (*fader), 2.0f, 0.0f, 4.0f);
}

TEST_F (SceneDraggablePartTests, ATravelOrATurnThatIsNotThreeNumbersIsReported)
{
    YuiDocument document;
    const auto result = document.loadFromString ("Scene3D {\n"
                                                 "    id: view;\n"
                                                 "    Node3D {\n"
                                                 "        id: pair;\n"
                                                 "        position: [1, 2, 3];\n"
                                                 "        travel: [1, 2];\n"
                                                 "        value: 1;\n"
                                                 "    }\n"
                                                 "    Node3D { id: four; turn: [1, 2, 3, 4]; value: 1; }\n"
                                                 "}");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    ASSERT_EQ (document.getDiagnostics().size(), 2u) << DataTreeDiagnostic::join (document.getDiagnostics());

    expectDiagnostic (document, "Node3D.travel: expected 3 numbers");
    expectDiagnostic (document, "Node3D.turn: expected 3 numbers");

    // Where the value is written
    const auto* diagnostic = findDiagnostic (document, "Node3D.travel");
    ASSERT_NE (diagnostic, nullptr);
    EXPECT_EQ (diagnostic->line, 6);

    // What can't be used neither moves nor turns the node
    auto* view = document.findById<SceneComponent> ("view");
    auto* pair = document.findById<SceneNodeComponent> ("pair");
    auto* four = document.findById<SceneNodeComponent> ("four");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (pair, nullptr);
    ASSERT_NE (four, nullptr);

    expectVector (positionOf (*pair), 1.0f, 2.0f, 3.0f);
    expectRotation (rotationOf (*four), Quaternion());
    EXPECT_EQ (Internals::getNumProblems (*view), 2u);

    // Once it can, the problem is withdrawn and the node goes where its value is
    pair->setProperty (Identifier ("travel"), numbers ({ 1.0, 0.0, 0.0 }));

    expectVector (positionOf (*pair), 2.0f, 2.0f, 3.0f);
    EXPECT_EQ (Internals::getNumProblems (*view), 1u);
}
