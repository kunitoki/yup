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

/** The nodes of a `Scene3D` under the pointer: the one whose triangle is the nearest answers for
    a position, and gets the mouse events, the states and the clicks of any component.
*/
class SceneInteractionTests : public SceneDocumentTests
{
protected:
    /** A square 1 wide in front of one 2 wide, written before it: at 100 to 150 by 75 to 125, and at 50 to 150. */
    static String nearAndFar()
    {
        const auto nearPlate = "Mesh3D { id: near; position: [0.5, 0, 0]; scale: 0.5; " + quadGeometry() + " }";
        const auto farPlate = "Mesh3D { id: far; position: [0, 0, -1]; " + quadGeometry() + " }";

        return sceneOf (nearPlate + " " + farPlate);
    }
};

TEST_F (SceneInteractionTests, TheNodeUnderAPositionIsTheOneWhoseTriangleIsNearest)
{
    YuiDocument document;
    load (document, nearAndFar());

    auto* view = showSceneOf (document);
    auto* nearPlate = document.findById<SceneMeshComponent> ("near");
    auto* farPlate = document.findById<SceneMeshComponent> ("far");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (nearPlate, nullptr);
    ASSERT_NE (farPlate, nullptr);

    // The nearest one where both are, whatever their order among the components
    EXPECT_EQ (view->findNodeAt ({ 125.0f, 100.0f }), nearPlate);
    EXPECT_EQ (view->findNodeAt ({ 75.0f, 100.0f }), farPlate);
    EXPECT_EQ (view->findNodeAt ({ 125.0f, 60.0f }), farPlate);

    // Beside every mesh, and outside the scene
    EXPECT_EQ (view->findNodeAt ({ 20.0f, 20.0f }), nullptr);
    EXPECT_EQ (view->findNodeAt ({ -5.0f, 100.0f }), nullptr);

    // One node answers for a position, the others don't
    EXPECT_TRUE (nearPlate->hitTest (125.0f, 100.0f));
    EXPECT_FALSE (farPlate->hitTest (125.0f, 100.0f));
    EXPECT_FALSE (nearPlate->hitTest (75.0f, 100.0f));
    EXPECT_TRUE (farPlate->hitTest (75.0f, 100.0f));
    EXPECT_FALSE (nearPlate->hitTest (20.0f, 20.0f));
    EXPECT_FALSE (farPlate->hitTest (20.0f, 20.0f));

    // So the pointer is over that node, and over the scene where there is none
    PointerSimulator pointer (*view);

    EXPECT_EQ (pointer.moveTo ({ 125.0f, 100.0f }), nearPlate);
    EXPECT_EQ (pointer.moveTo ({ 75.0f, 100.0f }), farPlate);
    EXPECT_EQ (pointer.moveTo ({ 20.0f, 20.0f }), view);
}

TEST_F (SceneInteractionTests, AKnobOnABodyIsPickedWhereItCoversIt)
{
    YuiDocument document;
    files["device.gltf"] = deviceFile();
    serve (document);

    load (document, sceneOf (R"yui(
        Node3D {
            id: group;

            Model3D {
                id: device; source: "device.gltf";

                Node3D { id: knob; node: "Knob"; }
            }
        })yui"));

    auto* view = showSceneOf (document);
    auto* group = document.findById<SceneNodeComponent> ("group");
    auto* device = document.findById<SceneModelComponent> ("device");
    auto* knob = document.findById<SceneNodeComponent> ("knob");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (group, nullptr);
    ASSERT_NE (device, nullptr);
    ASSERT_NE (knob, nullptr);
    ASSERT_TRUE (knob->getEntity().isValid());

    const Point<float> overKnob (125.0f, 75.0f);
    const Point<float> overScreen (75.0f, 75.0f);
    const Point<float> overBody (100.0f, 100.0f);
    const Point<float> overCorner (56.25f, 143.75f);

    // The bounds of the body, which its raised corner makes as deep as the knob stands, are entered before the knob
    const auto ray = view->getScene()->getActiveCamera()->getRay (overKnob, view->getLocalBounds());
    ASSERT_TRUE (ray.has_value());

    const auto boundsHit = view->getScene()->raycast (*ray);
    ASSERT_TRUE (boundsHit.has_value());
    EXPECT_EQ (boundsHit->entity.getName(), "Body");

    // Its triangles are behind it: the node bound to the knob answers there
    EXPECT_EQ (view->findNodeAt (overKnob), knob);

    // The model answers for the rest of it: its body, and the part no node is bound to
    EXPECT_EQ (view->findNodeAt (overBody), device);
    EXPECT_EQ (view->findNodeAt (overCorner), device);
    EXPECT_EQ (view->findNodeAt (overScreen), device);
    EXPECT_EQ (view->findNodeAt ({ 20.0f, 20.0f }), nullptr);

    // The nodes a node is in answer with it, for the pointer to reach it through them
    EXPECT_TRUE (group->hitTest (overKnob.getX(), overKnob.getY()));
    EXPECT_TRUE (device->hitTest (overKnob.getX(), overKnob.getY()));
    EXPECT_TRUE (knob->hitTest (overKnob.getX(), overKnob.getY()));

    EXPECT_TRUE (group->hitTest (overBody.getX(), overBody.getY()));
    EXPECT_TRUE (device->hitTest (overBody.getX(), overBody.getY()));
    EXPECT_FALSE (knob->hitTest (overBody.getX(), overBody.getY()));

    EXPECT_FALSE (group->hitTest (20.0f, 20.0f));

    PointerSimulator pointer (*view);

    EXPECT_EQ (pointer.moveTo (overKnob), knob);
    EXPECT_EQ (pointer.moveTo (overBody), device);
    EXPECT_EQ (pointer.moveTo ({ 20.0f, 20.0f }), view);

    // Behind the body, the knob is not under the pointer anymore
    knob->setProperty (Identifier ("position"), numbers ({ 0.5, 0.5, -0.5 }));
    EXPECT_EQ (view->findNodeAt (overKnob), device);
}

TEST_F (SceneInteractionTests, ARotatedScaledAndNestedNodeIsPickedWhereItIsShown)
{
    // A square made 2 by 1 and moved 1 to the right, then halved, turned a quarter of a turn and moved by the
    // node around them: from 112.5 to 137.5 by 50 to 100
    const auto plateText = "Mesh3D { id: plate; scale: [1, 0.5, 1]; " + quadGeometry() + " }";
    const auto innerText = "Node3D { id: inner; position: [1, 0, 0]; " + plateText + " }";

    YuiDocument document;
    load (document, sceneOf ("Node3D { id: group; position: [0.5, 0, 0]; rotation: [0, 0, 90deg]; scale: 0.5; " + innerText + " }"));

    auto* view = showSceneOf (document);
    auto* group = document.findById<SceneNodeComponent> ("group");
    auto* inner = document.findById<SceneNodeComponent> ("inner");
    auto* plate = document.findById<SceneMeshComponent> ("plate");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (group, nullptr);
    ASSERT_NE (inner, nullptr);
    ASSERT_NE (plate, nullptr);

    EXPECT_EQ (view->findNodeAt ({ 125.0f, 55.0f }), plate);
    EXPECT_EQ (view->findNodeAt ({ 125.0f, 95.0f }), plate);
    EXPECT_EQ (view->findNodeAt ({ 115.0f, 75.0f }), plate);

    // Where it would be if it were not turned, and beside where it is
    EXPECT_EQ (view->findNodeAt ({ 160.0f, 100.0f }), nullptr);
    EXPECT_EQ (view->findNodeAt ({ 105.0f, 75.0f }), nullptr);
    EXPECT_EQ (view->findNodeAt ({ 125.0f, 110.0f }), nullptr);

    // A group shows nothing itself: it answers for the nodes inside it
    EXPECT_TRUE (group->hitTest (125.0f, 55.0f));
    EXPECT_TRUE (inner->hitTest (125.0f, 55.0f));
    EXPECT_TRUE (plate->hitTest (125.0f, 55.0f));
    EXPECT_FALSE (group->hitTest (160.0f, 100.0f));

    PointerSimulator pointer (*view);
    EXPECT_EQ (pointer.moveTo ({ 125.0f, 55.0f }), plate);
}

TEST_F (SceneInteractionTests, APerspectiveCameraPicksAlongItsRaysAndBetweenItsPlanes)
{
    // A field of view whose half has a tangent of 0.5, so a view 4 high at 4 from the camera: the large
    // square from 50 to 150, and the small one halfway from 75 to 125
    const auto smallText = "Mesh3D { id: small; position: [0, 0, 2]; scale: 0.25; " + quadGeometry() + " }";
    const auto largeText = "Mesh3D { id: large; " + quadGeometry() + " }";

    YuiDocument document;
    load (document, "Scene3D { id: view; Camera3D { id: camera; position: [0, 0, 4]; fov: 0.9272952; } " + smallText + " " + largeText + " }");

    auto* view = showSceneOf (document);
    auto* camera = document.findById<SceneCameraComponent> ("camera");
    auto* smallPlate = document.findById<SceneMeshComponent> ("small");
    auto* largePlate = document.findById<SceneMeshComponent> ("large");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (camera, nullptr);
    ASSERT_NE (smallPlate, nullptr);
    ASSERT_NE (largePlate, nullptr);

    // Nearer, the small one is shown larger than it is
    EXPECT_EQ (view->findNodeAt ({ 120.0f, 100.0f }), smallPlate);
    EXPECT_EQ (view->findNodeAt ({ 130.0f, 100.0f }), largePlate);
    EXPECT_EQ (view->findNodeAt ({ 40.0f, 100.0f }), nullptr);

    // A camera that sees infinitely far picks the same
    camera->setFarPlane (0.0f);
    EXPECT_EQ (view->findNodeAt ({ 120.0f, 100.0f }), smallPlate);
    EXPECT_EQ (view->findNodeAt ({ 130.0f, 100.0f }), largePlate);

    // What the camera clips is not drawn, and not picked: before its near plane, and beyond its far one
    camera->setNearPlane (2.5f);
    EXPECT_EQ (view->findNodeAt ({ 120.0f, 100.0f }), largePlate);

    camera->setNearPlane (0.1f);
    camera->setFarPlane (3.0f);
    EXPECT_EQ (view->findNodeAt ({ 120.0f, 100.0f }), smallPlate);
    EXPECT_EQ (view->findNodeAt ({ 130.0f, 100.0f }), nullptr);
}

TEST_F (SceneInteractionTests, ASceneWithoutACameraIsPickedThroughTheOneTheRendererDrawsItWith)
{
    // From 5 away with a field of view of 0.8: the square 2 wide is shown from 52.7 to 147.3
    YuiDocument document;
    load (document, "Scene3D { id: view; Mesh3D { id: plate; " + quadGeometry() + " } }");

    auto* view = showSceneOf (document);
    auto* plate = document.findById<SceneMeshComponent> ("plate");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (plate, nullptr);
    ASSERT_EQ (view->getScene()->getActiveCamera(), nullptr);

    EXPECT_EQ (view->findNodeAt ({ 100.0f, 100.0f }), plate);
    EXPECT_EQ (view->findNodeAt ({ 55.0f, 145.0f }), plate);
    EXPECT_EQ (view->findNodeAt ({ 50.0f, 100.0f }), nullptr);
    EXPECT_EQ (view->findNodeAt ({ 100.0f, 150.0f }), nullptr);
}

TEST_F (SceneInteractionTests, AnInvisibleNodeIsNotThereAndADisabledOneTakesThePointerForNothing)
{
    YuiDocument document;
    load (document, nearAndFar());

    auto* view = showSceneOf (document);
    auto* nearPlate = document.findById<SceneMeshComponent> ("near");
    auto* farPlate = document.findById<SceneMeshComponent> ("far");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (nearPlate, nullptr);
    ASSERT_NE (farPlate, nullptr);

    int nearClicks = 0;
    int farClicks = 0;
    nearPlate->onClick = [&nearClicks] { ++nearClicks; };
    farPlate->onClick = [&farClicks] { ++farClicks; };

    // Not visible, it is not drawn: what is behind it is under the pointer
    nearPlate->setVisible (false);
    EXPECT_EQ (view->findNodeAt ({ 125.0f, 100.0f }), farPlate);

    PointerSimulator pointer (*view);
    pointer.click ({ 125.0f, 100.0f });
    EXPECT_EQ (nearClicks, 0);
    EXPECT_EQ (farClicks, 1);

    // Not enabled, it is drawn: it is still the one under the pointer, which does nothing to it
    nearPlate->setVisible (true);
    nearPlate->setEnabled (false);
    EXPECT_EQ (view->findNodeAt ({ 125.0f, 100.0f }), nearPlate);

    EXPECT_EQ (pointer.click ({ 125.0f, 100.0f }), nearPlate);
    EXPECT_EQ (nearClicks, 0);
    EXPECT_EQ (farClicks, 1);
    EXPECT_FALSE (nearPlate->isPressed());
    EXPECT_FALSE (farPlate->isHovered());

    nearPlate->setEnabled (true);
    pointer.click ({ 125.0f, 100.0f });
    EXPECT_EQ (nearClicks, 1);
    EXPECT_EQ (farClicks, 1);
}

TEST_F (SceneInteractionTests, HoverAndPressedStatesReachTheStyleRules)
{
    YuiDocument document;
    load (document, R"yui(
        style {
            Mesh3D:hover { scale: 1.5; }
            Mesh3D:pressed { position: [0, 0, 0.5]; }
        }
        )yui" + sceneOf ("Mesh3D { id: plate; " + quadGeometry() + " }"));

    auto* view = showSceneOf (document);
    auto* plate = document.findById<SceneMeshComponent> ("plate");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (plate, nullptr);

    auto& context = *document.getRoot()->getStyleContext();
    PointerSimulator pointer (*view);

    pointer.moveTo ({ 100.0f, 100.0f });
    context.restyleNow();

    EXPECT_TRUE (plate->isHovered());
    EXPECT_TRUE (view->isHovered());
    expectVector (scaleOf (*plate), 1.5f, 1.5f, 1.5f);

    pointer.press();
    context.restyleNow();

    EXPECT_TRUE (plate->isPressed());
    expectVector (positionOf (*plate), 0.0f, 0.0f, 0.5f);
    expectVector (scaleOf (*plate), 1.5f, 1.5f, 1.5f);

    pointer.release();
    context.restyleNow();

    EXPECT_FALSE (plate->isPressed());
    expectVector (positionOf (*plate), 0.0f, 0.0f, 0.0f);

    // Out of the square, grown to 25 to 175
    pointer.moveTo ({ 10.0f, 10.0f });
    context.restyleNow();

    EXPECT_FALSE (plate->isHovered());
    expectVector (scaleOf (*plate), 1.0f, 1.0f, 1.0f);
}

TEST_F (SceneInteractionTests, OnClickFiresOnceForAClickAndNotForADragReleasedElsewhere)
{
    int picked = 0;

    // A square 1 wide, from 75 to 125
    YuiDocument document;
    document.setAction ("picked", [&picked] (const Array<var>&)
    {
        ++picked;
    });

    load (document, sceneOf ("Mesh3D { id: plate; scale: 0.5; onClick: action (\"picked\"); " + quadGeometry() + " }"));

    auto* view = showSceneOf (document);
    auto* plate = document.findById<SceneMeshComponent> ("plate");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (plate, nullptr);

    PointerSimulator pointer (*view);

    EXPECT_EQ (pointer.click ({ 100.0f, 100.0f }), plate);
    EXPECT_EQ (picked, 1);

    // Pressed over it and released beside it, then the other way round
    EXPECT_EQ (pointer.drag ({ 100.0f, 100.0f }, { 20.0f, 20.0f }), plate);
    EXPECT_EQ (picked, 1);

    EXPECT_EQ (pointer.drag ({ 20.0f, 20.0f }, { 100.0f, 100.0f }), view);
    EXPECT_EQ (picked, 1);

    // Pressed and released over it, wherever
    pointer.drag ({ 80.0f, 80.0f }, { 120.0f, 120.0f });
    EXPECT_EQ (picked, 2);

    // Code connects to it as to the one of a button
    int clicked = 0;
    plate->onClick = [&clicked] { ++clicked; };

    pointer.click ({ 100.0f, 100.0f });
    EXPECT_EQ (picked, 2);
    EXPECT_EQ (clicked, 1);
}

TEST_F (SceneInteractionTests, AComponentWrittenOverTheSceneWinsOverTheMeshUnderIt)
{
    // The mesh comes after the button among the components: only the scene tells that the button is over it
    YuiDocument document;
    load (document, sceneOf ("TextButton { id: hud; x: 80; y: 80; width: 40; height: 40; } Mesh3D { id: plate; " + quadGeometry() + " }"));

    auto* view = showSceneOf (document);
    auto* hud = document.findById<TextButton> ("hud");
    auto* plate = document.findById<SceneMeshComponent> ("plate");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (hud, nullptr);
    ASSERT_NE (plate, nullptr);
    ASSERT_LT (view->getIndexOfChildComponent (hud), view->getIndexOfChildComponent (plate));

    EXPECT_FALSE (hud->isManuallyComposited());

    EXPECT_EQ (view->findNodeAt ({ 100.0f, 100.0f }), nullptr);
    EXPECT_FALSE (plate->hitTest (100.0f, 100.0f));
    EXPECT_EQ (view->findNodeAt ({ 60.0f, 60.0f }), plate);

    PointerSimulator pointer (*view);

    EXPECT_EQ (pointer.moveTo ({ 100.0f, 100.0f }), hud);
    EXPECT_EQ (pointer.moveTo ({ 60.0f, 60.0f }), plate);
    EXPECT_EQ (pointer.moveTo ({ 20.0f, 20.0f }), view);

    // A component that takes no mouse event lets the mesh have the pointer, from the next frame of the scene
    hud->setWantsMouseEvents (false, false);
    view->refreshDisplay (0.0);

    EXPECT_EQ (view->findNodeAt ({ 100.0f, 100.0f }), plate);
    EXPECT_EQ (pointer.moveTo ({ 100.0f, 100.0f }), plate);
}

TEST_F (SceneInteractionTests, ThePickFollowsWhatChanges)
{
    YuiDocument document;
    load (document, nearAndFar());

    auto* view = showSceneOf (document);
    auto* camera = dynamic_cast<SceneCameraComponent*> (view != nullptr ? view->getChildComponent (0) : nullptr);
    auto* nearPlate = document.findById<SceneMeshComponent> ("near");
    auto* farPlate = document.findById<SceneMeshComponent> ("far");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (camera, nullptr);
    ASSERT_NE (nearPlate, nullptr);
    ASSERT_NE (farPlate, nullptr);

    EXPECT_EQ (view->findNodeAt ({ 125.0f, 100.0f }), nearPlate);
    EXPECT_EQ (view->findNodeAt ({ 60.0f, 100.0f }), farPlate);

    // A node that moves: the near square is now from 50 to 100
    nearPlate->setProperty (Identifier ("position"), numbers ({ -0.5, 0.0, 0.0 }));
    EXPECT_EQ (view->findNodeAt ({ 125.0f, 100.0f }), farPlate);
    EXPECT_EQ (view->findNodeAt ({ 60.0f, 100.0f }), nearPlate);

    // A scene that is resized: twice as large, it shows the near square from 100 to 200 by 150 to 250
    view->setBounds (0.0f, 0.0f, 400.0f, 400.0f);
    EXPECT_EQ (view->findNodeAt ({ 60.0f, 100.0f }), nullptr);
    EXPECT_EQ (view->findNodeAt ({ 160.0f, 200.0f }), nearPlate);

    // A camera that moves: 1 to the right, what it shows goes 100 to the left
    camera->setProperty (Identifier ("position"), numbers ({ 1.0, 0.0, 4.0 }));
    EXPECT_EQ (view->findNodeAt ({ 60.0f, 200.0f }), nearPlate);
    EXPECT_EQ (view->findNodeAt ({ 160.0f, 200.0f }), farPlate);

    // Nothing tells the scene of what code does to its entities: the answer it gave for the position is
    // kept until the next display frame
    farPlate->getEntity().getOrAttach<TransformNode>().setPosition ({ 9.0f, 0.0f, -1.0f });
    EXPECT_EQ (view->findNodeAt ({ 160.0f, 200.0f }), farPlate);

    view->refreshDisplay (0.0);
    EXPECT_EQ (view->findNodeAt ({ 160.0f, 200.0f }), nullptr);
}

TEST_F (SceneInteractionTests, ADeformedMeshIsPickedWhereItIsDrawn)
{
    YuiDocument document;
    load (document, sceneOf ("Node3D { id: shape; }"));

    auto* view = showSceneOf (document);
    auto* shape = document.findById<SceneNodeComponent> ("shape");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (shape, nullptr);

    // A square 1 wide around the origin, which a morph target takes 1.5 to the right
    std::vector<Mesh::Vertex> vertices (4);
    vertices[0].position = { -0.5f, -0.5f, 0.0f };
    vertices[1].position = { 0.5f, -0.5f, 0.0f };
    vertices[2].position = { 0.5f, 0.5f, 0.0f };
    vertices[3].position = { -0.5f, 0.5f, 0.0f };

    for (auto& vertex : vertices)
        vertex.normal = { 0.0f, 0.0f, 1.0f };

    Mesh::MorphTarget target;
    target.positions.assign (vertices.size(), Vector3<float> (1.5f, 0.0f, 0.0f));

    auto mesh = Mesh::Ptr (new Mesh ("shape"));
    mesh->addPrimitive (vertices, { 0, 1, 2, 0, 2, 3 }, nullptr, {}, { target });

    // A mesh that code gives the entity of a group is the one of that node
    auto entity = shape->getEntity();
    entity.getOrAttach<MeshNode>().mesh = mesh;

    EXPECT_EQ (view->findNodeAt ({ 100.0f, 100.0f }), shape);
    EXPECT_EQ (view->findNodeAt ({ 175.0f, 100.0f }), nullptr);

    // Deformed, the entity draws a copy of the mesh: that one is under the pointer, from 150 to 200
    auto& deform = entity.attach<DeformNode> (mesh);
    deform.setWeights (std::array<float, 1> { 1.0f });
    deform.update (0.0);
    view->refreshDisplay (0.0);

    ASSERT_NE (entity.get<MeshNode>()->mesh, mesh);
    EXPECT_EQ (view->findNodeAt ({ 100.0f, 100.0f }), nullptr);
    EXPECT_EQ (view->findNodeAt ({ 175.0f, 100.0f }), shape);
}
