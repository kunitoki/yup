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

/** The components written inside a node of a `Scene3D`: shown on the mesh of the node, where the
    pointer reaches them. Nothing is rendered without a GPU: the tests look at which components
    are taken out of the painting of their parent, at where the points of the scene and theirs
    are mapped, and at the material the node is given to show them.

    The scenes are seen through the camera of the fixture, 50 pixels for each unit around
    100, 100: the quad, 2 wide, is shown from 50 to 150, with the texture coordinates 0, 0 at its
    top left and 1, 1 at its bottom right.
*/
class SceneSurfaceTests : public SceneDocumentTests
{
protected:
    /** Records the pointer events it is given, and where the last one was in its own coordinates. */
    class Recorder final : public Component
    {
    public:
        void mouseMove (const MouseEvent& event) override { record ("move", event); }

        void mouseDown (const MouseEvent& event) override { record ("down", event); }

        void mouseDrag (const MouseEvent& event) override { record ("drag", event); }

        void mouseUp (const MouseEvent& event) override { record ("up", event); }

        StringArray events;
        Point<float> lastPosition;

    private:
        void record (const char* name, const MouseEvent& event)
        {
            events.add (name);
            lastPosition = event.getPosition();
        }
    };

    /** A text where `$QUAD` stands for the arrays of the textured quad. */
    static String withQuad (const String& text)
    {
        return text.replace ("$QUAD", texturedQuadGeometry());
    }

    /** Expects a point of a scene to be the one where a point of a component is shown, both ways. */
    static void expectShownAt (SceneComponent& view, Component& component, Point<float> scenePoint, Point<float> componentPoint)
    {
        SCOPED_TRACE ("the point " + scenePoint.toString().toStdString() + " of the scene and " + componentPoint.toString().toStdString() + " of the component");

        expectPoint (component.getLocalPoint (&view, scenePoint), componentPoint.getX(), componentPoint.getY(), 0.05f);
        expectPoint (view.getLocalPoint (&component, componentPoint), scenePoint.getX(), scenePoint.getY(), 0.05f);
    }

    /** The material the entity of a node is drawn with, or nullptr when it has none of its own. */
    static Material::Ptr materialOf (const SceneNodeComponent& node)
    {
        const auto entity = node.getEntity();
        const auto* materialNode = entity.isValid() ? entity.get<MaterialNode>() : nullptr;

        return materialNode != nullptr ? materialNode->material : Material::Ptr();
    }

    /** A glTF model holding everything as data URIs: the scene Main with Panel, whose mesh is made of two
        primitives, a triangle each over the same texture coordinates.
    */
    static MemoryBlock twoPrimitiveFile()
    {
        return bytesOf (String (R"({
            "asset": { "version": "2.0" },
            "scene": 0,
            "scenes": [ { "name": "Main", "nodes": [ 0 ] } ],
            "nodes": [ { "name": "Panel", "mesh": 0 } ],
            "meshes": [ { "name": "PanelMesh", "primitives": [ { "attributes": { "POSITION": 0, "TEXCOORD_0": 2 } },
                                                               { "attributes": { "POSITION": 1, "TEXCOORD_0": 2 } } ] } ],
            "buffers": [ $FRONT, $BACK, $UVS ],
            "bufferViews": [ { "buffer": 0, "byteLength": 36 }, { "buffer": 1, "byteLength": 36 }, { "buffer": 2, "byteLength": 24 } ],
            "accessors": [ { "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3" },
                           { "bufferView": 1, "componentType": 5126, "count": 3, "type": "VEC3" },
                           { "bufferView": 2, "componentType": 5126, "count": 3, "type": "VEC2" } ]
        })")
                            .replace ("$FRONT", bufferOf<float> ({ -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f }))
                            .replace ("$BACK", bufferOf<float> ({ -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f }))
                            .replace ("$UVS", bufferOf<float> ({ 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f })));
    }
};

TEST_F (SceneSurfaceTests, TheComponentsInsideANodeAreShownOnItsMeshAndTheOnesInsideTheSceneOverIt)
{
    YuiDocument document;
    load (document, sceneOf (withQuad (R"yui(
        Label { id: hud; text: "Over the scene"; }

        Node3D { id: group;  Node3D { id: inner; } }

        Mesh3D {
            id: quad; $QUAD

            Component { id: panel; width: 200; height: 100;  Label { id: caption; text: "On the mesh"; } }
            Material3D { id: paint; }
            Light3D { id: lamp; type: point; }
        })yui")));

    auto* view = showSceneOf (document);
    auto* hud = document.findById<Label> ("hud");
    auto* inner = document.findById<SceneNodeComponent> ("inner");
    auto* quad = document.findById<SceneMeshComponent> ("quad");
    auto* panel = document.findById<Component> ("panel");
    auto* caption = document.findById<Label> ("caption");
    auto* paint = document.findById<SceneMaterialComponent> ("paint");
    auto* lamp = document.findById<SceneLightComponent> ("lamp");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (hud, nullptr);
    ASSERT_NE (inner, nullptr);
    ASSERT_NE (quad, nullptr);
    ASSERT_NE (panel, nullptr);
    ASSERT_NE (caption, nullptr);
    ASSERT_NE (paint, nullptr);
    ASSERT_NE (lamp, nullptr);

    // Directly inside the scene a component is painted over it, by the scene
    EXPECT_EQ (hud->getParentComponent(), view);
    EXPECT_FALSE (hud->isManuallyComposited());

    // Inside a node it is not painted by the node: it is rendered for the mesh, with what it holds
    EXPECT_EQ (panel->getParentComponent(), quad);
    EXPECT_TRUE (panel->isManuallyComposited());
    EXPECT_FALSE (caption->isManuallyComposited());

    // The 3D types are of the scene wherever they are: a node, and the material of a node
    EXPECT_FALSE (inner->isManuallyComposited());
    EXPECT_FALSE (lamp->isManuallyComposited());
    EXPECT_FALSE (paint->isManuallyComposited());

    // The entities are those of the nodes alone
    EXPECT_EQ (quad->getEntity().getNumChildren(), 1);
    EXPECT_EQ (quad->getEntity().getChild (0), lamp->getEntity());
}

TEST_F (SceneSurfaceTests, TheSurfaceLiesOverTheTextureCoordinatesOfTheMesh)
{
    YuiDocument document;
    load (document, sceneOf (withQuad ("Mesh3D { id: quad; $QUAD Component { id: panel; width: 200; height: 100; } }")));

    auto* view = showSceneOf (document);
    auto* panel = document.findById<Component> ("panel");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (panel, nullptr);

    // The component is the whole surface, which goes from a corner of the quad to the other: 2 of its units
    // for each pixel across, and 1 down
    expectShownAt (*view, *panel, { 50.0f, 50.0f }, { 0.0f, 0.0f });
    expectShownAt (*view, *panel, { 150.0f, 50.0f }, { 200.0f, 0.0f });
    expectShownAt (*view, *panel, { 50.0f, 150.0f }, { 0.0f, 100.0f });
    expectShownAt (*view, *panel, { 150.0f, 150.0f }, { 200.0f, 100.0f });
    expectShownAt (*view, *panel, { 100.0f, 100.0f }, { 100.0f, 50.0f });
    expectShownAt (*view, *panel, { 75.0f, 75.0f }, { 50.0f, 25.0f });

    // With another component the surface goes to the far corner of both, 300 by 150, each one where it is in it
    load (document, sceneOf (withQuad (R"yui(
        Mesh3D {
            id: quad; $QUAD

            Component { id: panel; width: 200; height: 100; }
            Component { id: badge; x: 200; y: 100; width: 100; height: 50; }
        })yui")));

    view = showSceneOf (document);
    panel = document.findById<Component> ("panel");
    auto* badge = document.findById<Component> ("badge");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (panel, nullptr);
    ASSERT_NE (badge, nullptr);

    expectShownAt (*view, *panel, { 50.0f, 50.0f }, { 0.0f, 0.0f });
    expectShownAt (*view, *panel, { 100.0f, 100.0f }, { 150.0f, 75.0f });
    expectShownAt (*view, *badge, { 150.0f, 150.0f }, { 100.0f, 50.0f });
    expectShownAt (*view, *badge, { 125.0f, 125.0f }, { 25.0f, 12.5f });

    // And follows them when they are moved or resized: 400 by 200
    badge->setBounds (200.0f, 100.0f, 200.0f, 100.0f);

    expectShownAt (*view, *panel, { 100.0f, 100.0f }, { 200.0f, 100.0f });
    expectShownAt (*view, *badge, { 150.0f, 150.0f }, { 200.0f, 100.0f });
}

TEST_F (SceneSurfaceTests, TheMappingFollowsTheNodeTheMeshAndTheCamera)
{
    YuiDocument document;
    load (document, sceneOf (withQuad ("Mesh3D { id: quad; rotation: [0, 0, 90deg]; $QUAD Component { id: panel; width: 200; height: 100; } }")));

    auto* view = showSceneOf (document);
    auto* camera = dynamic_cast<SceneCameraComponent*> (view != nullptr ? view->getChildComponent (0) : nullptr);
    auto* quad = document.findById<SceneMeshComponent> ("quad");
    auto* panel = document.findById<Component> ("panel");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (camera, nullptr);
    ASSERT_NE (quad, nullptr);
    ASSERT_NE (panel, nullptr);

    // Turned a quarter of a turn, the top left corner of the quad is shown at the bottom left, and its top edge goes up
    expectShownAt (*view, *panel, { 50.0f, 150.0f }, { 0.0f, 0.0f });
    expectShownAt (*view, *panel, { 50.0f, 50.0f }, { 200.0f, 0.0f });
    expectShownAt (*view, *panel, { 150.0f, 50.0f }, { 200.0f, 100.0f });
    expectShownAt (*view, *panel, { 100.0f, 100.0f }, { 100.0f, 50.0f });
    expectShownAt (*view, *panel, { 75.0f, 125.0f }, { 50.0f, 25.0f });

    // Upright again, then half as large: shown from 75 to 125
    quad->setProperty (Identifier ("rotation"), numbers ({ 0.0, 0.0, 0.0 }));
    expectShownAt (*view, *panel, { 75.0f, 75.0f }, { 50.0f, 25.0f });

    quad->setProperty (Identifier ("scale"), 0.5);
    expectShownAt (*view, *panel, { 75.0f, 75.0f }, { 0.0f, 0.0f });
    expectShownAt (*view, *panel, { 112.5f, 112.5f }, { 150.0f, 75.0f });

    quad->setProperty (Identifier ("scale"), 1.0);

    // The camera 1 to the right shows it 50 to the left
    camera->setProperty (Identifier ("position"), numbers ({ 1.0, 0.0, 4.0 }));
    expectShownAt (*view, *panel, { 25.0f, 75.0f }, { 50.0f, 25.0f });

    camera->setProperty (Identifier ("position"), numbers ({ 0.0, 0.0, 4.0 }));

    // A scene twice as large shows it from 100 to 300
    view->setBounds (0.0f, 0.0f, 400.0f, 400.0f);
    expectShownAt (*view, *panel, { 150.0f, 150.0f }, { 50.0f, 25.0f });

    showScene (*view);

    // Texture coordinates going from right to left mirror it: built by the next frame of the scene
    quad->setProperty (Identifier ("uvs"), numbers ({ 1.0, 1.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0 }));
    view->refreshDisplay (0.0);

    expectShownAt (*view, *panel, { 150.0f, 50.0f }, { 0.0f, 0.0f });
    expectShownAt (*view, *panel, { 50.0f, 150.0f }, { 200.0f, 100.0f });
    expectShownAt (*view, *panel, { 125.0f, 75.0f }, { 50.0f, 25.0f });
}

TEST_F (SceneSurfaceTests, APerspectiveCameraMapsTheSurfaceWhereItShowsIt)
{
    // A field of view whose half has a tangent of 0.5: 4 units high at 4 from the camera, so the quad from 50 to 150
    YuiDocument document;
    load (document, withQuad (R"yui(
        Scene3D {
            id: view;

            Camera3D { id: camera; position: [0, 0, 4]; fov: 0.9272952; }
            Mesh3D { id: quad; $QUAD Component { id: panel; width: 200; height: 100; } }
        })yui"));

    auto* view = showSceneOf (document);
    auto* camera = document.findById<SceneCameraComponent> ("camera");
    auto* quad = document.findById<SceneMeshComponent> ("quad");
    auto* panel = document.findById<Component> ("panel");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (camera, nullptr);
    ASSERT_NE (quad, nullptr);
    ASSERT_NE (panel, nullptr);

    expectShownAt (*view, *panel, { 50.0f, 50.0f }, { 0.0f, 0.0f });
    expectShownAt (*view, *panel, { 125.0f, 125.0f }, { 150.0f, 75.0f });

    // Halfway to the camera it is shown twice as large, all over the scene
    quad->setProperty (Identifier ("position"), numbers ({ 0.0, 0.0, 2.0 }));

    expectShownAt (*view, *panel, { 50.0f, 50.0f }, { 50.0f, 25.0f });
    expectShownAt (*view, *panel, { 150.0f, 100.0f }, { 150.0f, 50.0f });

    // A camera that sees infinitely far maps it the same
    camera->setFarPlane (0.0f);

    expectShownAt (*view, *panel, { 50.0f, 50.0f }, { 50.0f, 25.0f });
    expectShownAt (*view, *panel, { 150.0f, 100.0f }, { 150.0f, 50.0f });
}

TEST_F (SceneSurfaceTests, ThePointerReachesAComponentWhereTheMeshShowsIt)
{
    YuiDocument document;
    load (document, sceneOf (withQuad ("Mesh3D { id: quad; $QUAD }")));

    auto* view = showSceneOf (document);
    auto* quad = document.findById<SceneMeshComponent> ("quad");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (quad, nullptr);

    Recorder panel;
    panel.setBounds (0.0f, 0.0f, 200.0f, 100.0f);
    quad->addAndMakeVisible (panel);

    PointerSimulator pointer (*view);

    EXPECT_EQ (pointer.moveTo ({ 75.0f, 75.0f }), &panel);
    expectPoint (panel.lastPosition, 50.0f, 25.0f, 0.05f);
    EXPECT_TRUE (panel.isHovered());
    EXPECT_TRUE (quad->isHovered());

    EXPECT_EQ (pointer.press(), &panel);
    expectPoint (panel.lastPosition, 50.0f, 25.0f, 0.05f);

    EXPECT_EQ (pointer.moveTo ({ 125.0f, 125.0f }), &panel);
    expectPoint (panel.lastPosition, 150.0f, 75.0f, 0.05f);

    // Dragged off the mesh, the pointer goes on over its plane, as one that leaves the component it drags
    EXPECT_EQ (pointer.moveTo ({ 175.0f, 100.0f }), &panel);
    expectPoint (panel.lastPosition, 250.0f, 50.0f, 0.05f);

    EXPECT_EQ (pointer.moveTo ({ 100.0f, 20.0f }), &panel);
    expectPoint (panel.lastPosition, 100.0f, -30.0f, 0.05f);

    EXPECT_EQ (pointer.release(), &panel);
    EXPECT_EQ (panel.events, StringArray ({ "move", "down", "drag", "drag", "drag", "up" }));

    // Released beside the mesh, it is over the scene
    EXPECT_EQ (pointer.getComponentUnderPointer(), view);
    EXPECT_FALSE (panel.isHovered());
    EXPECT_FALSE (quad->isHovered());

    // A component that is not visible is not shown, and leaves the mesh to its node
    panel.setVisible (false);
    EXPECT_EQ (pointer.moveTo ({ 75.0f, 75.0f }), quad);
}

TEST_F (SceneSurfaceTests, AClickOnTheMeshReachesTheButtonShownThereAndTheNodeAroundIt)
{
    int pressed = 0;
    int picked = 0;

    YuiDocument document;

    document.setAction ("pressed", [&pressed] (const Array<var>&)
    {
        ++pressed;
    });

    document.setAction ("picked", [&picked] (const Array<var>&)
    {
        ++picked;
    });

    // The button takes the bottom right quarter of the surface, so of the quad: from 100 to 150
    load (document, sceneOf (withQuad (R"yui(
        Mesh3D {
            id: quad; onClick: action ("picked"); $QUAD

            TextButton { id: button; x: 100; y: 50; width: 100; height: 50; onClick: action ("pressed"); }
        })yui")));

    auto* view = showSceneOf (document);
    auto* quad = document.findById<SceneMeshComponent> ("quad");
    auto* button = document.findById<TextButton> ("button");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (quad, nullptr);
    ASSERT_NE (button, nullptr);

    PointerSimulator pointer (*view);

    // Where the mesh shows the button the click is the one of the button, as a component wins over its parent
    EXPECT_EQ (pointer.click ({ 125.0f, 125.0f }), button);
    EXPECT_EQ (pressed, 1);
    EXPECT_EQ (picked, 0);

    // Around it the mesh is the node
    EXPECT_EQ (pointer.click ({ 75.0f, 75.0f }), quad);
    EXPECT_EQ (pressed, 1);
    EXPECT_EQ (picked, 1);

    // The button has its own states, and the node it is in is hovered with it as any parent is
    EXPECT_EQ (pointer.moveTo ({ 125.0f, 125.0f }), button);
    EXPECT_TRUE (button->isHovered());
    EXPECT_TRUE (quad->isHovered());

    EXPECT_EQ (pointer.press(), button);
    EXPECT_TRUE (button->isPressed());

    // Released over the mesh but beside the button, then beside the mesh: neither is a click, of the button or of the node
    pointer.moveTo ({ 75.0f, 75.0f });
    pointer.release();
    EXPECT_FALSE (button->isPressed());
    EXPECT_EQ (pressed, 1);
    EXPECT_EQ (picked, 1);

    EXPECT_EQ (pointer.drag ({ 125.0f, 125.0f }, { 20.0f, 20.0f }), button);
    EXPECT_EQ (pressed, 1);
    EXPECT_EQ (picked, 1);
}

TEST_F (SceneSurfaceTests, ASliderIsDraggedAlongTheMesh)
{
    // The surface, 200 by 40, lies over the quad: 2 of its units for each pixel across, the middle of the slider at 100
    YuiDocument document;
    load (document, sceneOf (withQuad ("Mesh3D { id: quad; $QUAD Slider { id: cutoff; width: 200; height: 40; } }")));

    auto* view = showSceneOf (document);
    auto* cutoff = document.findById<Slider> ("cutoff");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (cutoff, nullptr);

    // The same slider in an ordinary component, dragged through the same points of its own
    Component flat;
    Slider expected (Slider::LinearHorizontal);

    flat.setVisible (true);
    flat.setBounds (0.0f, 0.0f, 200.0f, 40.0f);
    expected.setBounds (0.0f, 0.0f, 200.0f, 40.0f);
    flat.addAndMakeVisible (expected);

    PointerSimulator pointer (*view);
    PointerSimulator flatPointer (flat);

    EXPECT_EQ (pointer.drag ({ 60.0f, 100.0f }, { 120.0f, 100.0f }), cutoff);
    EXPECT_EQ (flatPointer.drag ({ 20.0f, 20.0f }, { 140.0f, 20.0f }), &expected);

    const auto dragged = cutoff->getValue();
    EXPECT_GT (dragged, 0.0);
    EXPECT_NEAR (dragged, expected.getValue(), 1.0e-3);

    // Dragged off the mesh, it follows the pointer as a slider whose component the pointer leaves does
    pointer.drag ({ 100.0f, 100.0f }, { 190.0f, 100.0f });
    flatPointer.drag ({ 100.0f, 20.0f }, { 280.0f, 20.0f });

    EXPECT_GT (cutoff->getValue(), dragged);
    EXPECT_NEAR (cutoff->getValue(), expected.getValue(), 1.0e-3);
}

TEST_F (SceneSurfaceTests, ANodeInFrontOfTheMeshTakesThePointerBeforeTheComponentsShownBehindIt)
{
    int clicks = 0;

    YuiDocument document;

    document.setAction ("knob", [&clicks] (const Array<var>&)
    {
        ++clicks;
    });

    // A knob standing on the quad, in front of it: shown from 120 to 130 by 70 to 80
    load (document, sceneOf (withQuad ("Mesh3D { id: quad; $QUAD Mesh3D { id: knob; position: [0.5, 0.5, 0.2]; scale: 0.1; onClick: action (\"knob\"); " + quadGeometry() + " } }")));

    auto* view = showSceneOf (document);
    auto* quad = document.findById<SceneMeshComponent> ("quad");
    auto* knob = document.findById<SceneMeshComponent> ("knob");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (quad, nullptr);
    ASSERT_NE (knob, nullptr);

    // The component comes after the knob among those of the quad, so it is the first one asked for the pointer
    Recorder panel;
    panel.setBounds (0.0f, 0.0f, 200.0f, 100.0f);
    quad->addAndMakeVisible (panel);

    ASSERT_LT (quad->getIndexOfChildComponent (knob), quad->getIndexOfChildComponent (&panel));

    PointerSimulator pointer (*view);

    // Over the knob the pointer is the knob's: the quad shows the component behind it
    EXPECT_EQ (pointer.moveTo ({ 125.0f, 75.0f }), knob);
    EXPECT_TRUE (knob->isHovered());
    EXPECT_FALSE (panel.isHovered());

    EXPECT_EQ (pointer.click ({ 125.0f, 75.0f }), knob);
    EXPECT_EQ (clicks, 1);
    EXPECT_TRUE (panel.events.isEmpty());

    // Beside it the pointer is over the component
    EXPECT_EQ (pointer.moveTo ({ 100.0f, 120.0f }), &panel);
    expectPoint (panel.lastPosition, 100.0f, 70.0f, 0.05f);

    // Which keeps the pointer it holds when it goes over the knob, and is told where the quad shows it there
    EXPECT_EQ (pointer.press(), &panel);
    EXPECT_EQ (pointer.moveTo ({ 125.0f, 75.0f }), &panel);
    expectPoint (panel.lastPosition, 150.0f, 25.0f, 0.05f);

    EXPECT_EQ (pointer.release(), &panel);
    expectPoint (panel.lastPosition, 150.0f, 25.0f, 0.05f);
    EXPECT_EQ (panel.events, StringArray ({ "move", "down", "drag", "up" }));
    EXPECT_EQ (clicks, 1);

    // Released, the pointer is over the knob again
    EXPECT_EQ (pointer.getComponentUnderPointer(), knob);
    EXPECT_FALSE (panel.isHovered());
}

TEST_F (SceneSurfaceTests, AComponentHoldingThePointerStaysWhereTheMeshLastShowedItOnceNoTriangleFacesThePointer)
{
    {
        // A quad that turns its back on the camera while a component it shows holds the pointer
        YuiDocument document;
        load (document, sceneOf (withQuad ("Mesh3D { id: quad; $QUAD }")));

        auto* view = showSceneOf (document);
        auto* quad = document.findById<SceneMeshComponent> ("quad");

        ASSERT_NE (view, nullptr);
        ASSERT_NE (quad, nullptr);

        Recorder panel;
        panel.setBounds (0.0f, 0.0f, 200.0f, 100.0f);
        quad->addAndMakeVisible (panel);

        PointerSimulator pointer (*view);

        EXPECT_EQ (pointer.moveTo ({ 75.0f, 75.0f }), &panel);
        EXPECT_EQ (pointer.press(), &panel);

        pointer.moveTo ({ 125.0f, 125.0f });
        expectPoint (panel.lastPosition, 150.0f, 75.0f, 0.05f);

        // The pointer has no point of the component to be at: it stays at the last one, and is not given one of the scene
        quad->setProperty (Identifier ("rotation"), numbers ({ 0.0, MathConstants<double>::pi, 0.0 }));

        EXPECT_EQ (pointer.moveTo ({ 60.0f, 90.0f }), &panel);
        expectPoint (panel.lastPosition, 150.0f, 75.0f, 0.05f);

        // And follows the quad again once it faces the camera
        quad->setProperty (Identifier ("rotation"), numbers ({ 0.0, 0.0, 0.0 }));

        pointer.moveTo ({ 60.0f, 90.0f });
        expectPoint (panel.lastPosition, 20.0f, 40.0f, 0.05f);
    }

    {
        // A quad leaning back by an eighth of a turn, seen in perspective: a field of view whose half has a tangent of 0.5.
        // Its plane goes up to the horizon the camera sees 100 above the scene, and no further
        YuiDocument document;
        load (document, withQuad ("Scene3D { id: view; Camera3D { position: [0, 0, 4]; fov: 0.9272952; } Mesh3D { id: quad; rotation: [-45deg, 0, 0]; $QUAD } }"));

        auto* view = showSceneOf (document);
        auto* quad = document.findById<SceneMeshComponent> ("quad");

        ASSERT_NE (view, nullptr);
        ASSERT_NE (quad, nullptr);

        Recorder panel;
        panel.setBounds (0.0f, 0.0f, 200.0f, 100.0f);
        quad->addAndMakeVisible (panel);

        PointerSimulator pointer (*view);

        // Beside the middle of the quad: the middle lies on the edge its two triangles share, where a ray can miss both
        EXPECT_EQ (pointer.moveTo ({ 110.0f, 100.0f }), &panel);
        expectPoint (panel.lastPosition, 120.0f, 50.0f, 0.05f);
        EXPECT_EQ (pointer.press(), &panel);

        // Past the top of the quad, on its plane
        pointer.moveTo ({ 120.0f, 60.0f });
        expectPoint (panel.lastPosition, 150.0f, -20.71f, 0.5f);

        // And past the horizon of that plane, where the pointer looks at the sky above it
        EXPECT_EQ (pointer.moveTo ({ 120.0f, -150.0f }), &panel);
        expectPoint (panel.lastPosition, 150.0f, -20.71f, 0.5f);

        pointer.moveTo ({ 100.0f, 100.0f });
        expectPoint (panel.lastPosition, 100.0f, 50.0f, 0.05f);
    }
}

TEST_F (SceneSurfaceTests, ANodeThatDrawsNoMeshOfItsOwnSaysItCanNotShowItsComponents)
{
    const String problem ("the components inside a node are shown on the mesh its own entity draws, and this one draws none: "
                          "write them in a Mesh3D, or in a Node3D bound to the part of a model that shows them");

    struct Case
    {
        const char* text;
        const char* type;
        int line;
    };

    // A group draws nothing, and a model draws the meshes of its parts, not one of its own
    const Case cases[] = {
        { "Scene3D {\n"
          "    Node3D {\n"
          "        Label { id: lost; text: \"Lost\"; width: 100; height: 20; }\n"
          "    }\n"
          "}",
          "Node3D",
          2 },
        { "Scene3D {\n"
          "    Camera3D { }\n"
          "    Model3D {\n"
          "        source: \"device.gltf\";\n"
          "        Label { id: lost; text: \"Lost\"; width: 100; height: 20; }\n"
          "    }\n"
          "}",
          "Model3D",
          3 },
    };

    files["device.gltf"] = deviceFile();

    for (const auto& test : cases)
    {
        YuiDocument document;
        serve (document);

        // Told when the document has loaded, and again by the loads that keep it
        for (const auto* pass : { "the first load", "the load that keeps it" })
        {
            SCOPED_TRACE (String (test.type) + ", " + pass);

            const auto result = document.loadFromString (test.text);
            ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
            ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());

            const auto& diagnostic = document.getDiagnostics().front();
            EXPECT_EQ (diagnostic.message, String (test.type) + ": " + problem);
            EXPECT_EQ (diagnostic.line, test.line);
            EXPECT_TRUE (diagnostic.isError());
        }

        // The components are not painted over the scene for all that: they are shown nowhere
        auto* lost = document.findById<Label> ("lost");
        ASSERT_NE (lost, nullptr);
        EXPECT_TRUE (lost->isManuallyComposited());
    }

    // Written where a mesh is drawn, they have nothing to say
    YuiDocument document;
    serve (document);

    load (document, withQuad (R"yui(
        Scene3D {
            Mesh3D { $QUAD  Label { text: "Found"; width: 100; height: 20; } }

            Model3D {
                source: "device.gltf";

                Node3D { node: "Screen";  Label { text: "Found too"; width: 100; height: 20; } }
            }
        })yui"));
}

TEST_F (SceneSurfaceTests, TheProblemStandsWhileTheNodeDrawsNoMesh)
{
    YuiDocument document;
    const auto result = document.loadFromString ("Scene3D { id: view; Mesh3D { id: quad; Label { text: \"Waiting\"; width: 100; height: 20; } } }");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    expectDiagnostic (document, "Mesh3D: the components inside a node are shown on the mesh its own entity draws, and this one draws none");

    auto* view = showSceneOf (document);
    auto* quad = document.findById<SceneMeshComponent> ("quad");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (quad, nullptr);
    EXPECT_EQ (Internals::getNumProblems (*view), 1u);

    // Told once, whatever the number of frames
    view->refreshDisplay (0.0);
    view->refreshDisplay (0.0);
    EXPECT_EQ (Internals::getNumProblems (*view), 1u);

    // The mesh its arrays make is built by the next frame, which withdraws the problem
    quad->setProperty (Identifier ("vertices"), numbers ({ -1.0, -1.0, 0.0, 1.0, -1.0, 0.0, 1.0, 1.0, 0.0 }));
    quad->setProperty (Identifier ("uvs"), numbers ({ 0.0, 1.0, 1.0, 1.0, 1.0, 0.0 }));
    view->refreshDisplay (0.0);
    EXPECT_EQ (Internals::getNumProblems (*view), 0u);
    EXPECT_NE (materialOf (*quad), nullptr);

    // And it is back with a mesh that draws nothing again
    quad->setProperty (Identifier ("vertices"), var());
    quad->setProperty (Identifier ("uvs"), var());
    view->refreshDisplay (0.0);
    EXPECT_EQ (Internals::getNumProblems (*view), 1u);
    EXPECT_EQ (materialOf (*quad), nullptr);
}

TEST_F (SceneSurfaceTests, AMeshWhoseTextureCoordinatesSpanNoAreaShowsNoComponentAndSaysSo)
{
    const String problem ("the components inside a node are shown over the texture coordinates of its mesh, and the ones of this mesh span no area: "
                          "give the Mesh3D its uvs, or the part of the model its texture coordinates");

    int pressed = 0;
    int picked = 0;

    YuiDocument document;

    document.setAction ("pressed", [&pressed] (const Array<var>&)
    {
        ++pressed;
    });

    document.setAction ("picked", [&picked] (const Array<var>&)
    {
        ++picked;
    });

    // A quad that was given no texture coordinates has them all at 0, 0: every point of it would show the same one of the button
    const auto result = document.loadFromString ("Scene3D {\n"
                                                 "    id: view; " + orthographicCamera() + "\n"
                                                 "    Mesh3D {\n"
                                                 "        id: quad; onClick: action (\"picked\"); " + quadGeometry() + "\n"
                                                 "        TextButton { id: button; width: 200; height: 200; onClick: action (\"pressed\"); }\n"
                                                 "    }\n"
                                                 "}");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());

    const auto& diagnostic = document.getDiagnostics().front();
    EXPECT_EQ (diagnostic.message, "Mesh3D: " + problem);
    EXPECT_EQ (diagnostic.line, 3);
    EXPECT_TRUE (diagnostic.isError());

    auto* view = showSceneOf (document);
    auto* quad = document.findById<SceneMeshComponent> ("quad");
    auto* button = document.findById<TextButton> ("button");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (quad, nullptr);
    ASSERT_NE (button, nullptr);

    // The button is shown nowhere: not over the scene, and the quad is given no material to show it with
    EXPECT_TRUE (button->isManuallyComposited());
    EXPECT_EQ (materialOf (*quad), nullptr);

    {
        // The pointer is the node's all over its mesh, where the button has no place
        PointerSimulator pointer (*view);

        EXPECT_EQ (pointer.click ({ 100.0f, 100.0f }), quad);
        EXPECT_EQ (pointer.click ({ 60.0f, 140.0f }), quad);
        EXPECT_EQ (picked, 2);
        EXPECT_EQ (pressed, 0);

        // Told once, whatever the number of frames
        view->refreshDisplay (0.0);
        EXPECT_EQ (Internals::getNumProblems (*view), 1u);

        // The quad its arrays make with texture coordinates is built by the next frame, which withdraws the problem
        quad->setProperty (Identifier ("uvs"), numbers ({ 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0 }));
        view->refreshDisplay (0.0);

        EXPECT_EQ (Internals::getNumProblems (*view), 0u);
        EXPECT_NE (materialOf (*quad), nullptr);

        // The button is then where the quad shows it
        EXPECT_EQ (pointer.click ({ 100.0f, 100.0f }), button);
        EXPECT_EQ (pressed, 1);
        EXPECT_EQ (picked, 2);
    }

    // The part of a model that has no texture coordinates says the same
    files["device.gltf"] = deviceFile();

    YuiDocument device;
    serve (device);

    const auto loaded = device.loadFromString ("Scene3D {\n"
                                               "    Model3D {\n"
                                               "        source: \"device.gltf\";\n"
                                               "        Node3D { node: \"Body\"; Label { text: \"Lost\"; width: 100; height: 20; } }\n"
                                               "    }\n"
                                               "}");

    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    ASSERT_EQ (device.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (device.getDiagnostics());
    EXPECT_EQ (device.getDiagnostics().front().message, "Node3D: " + problem);
    EXPECT_EQ (device.getDiagnostics().front().line, 4);
}

TEST_F (SceneSurfaceTests, ANodeWithoutAMaterialIsGivenOneThatLooksAsItsMeshWould)
{
    YuiDocument document;
    load (document, sceneOf (withQuad (R"yui(
        Node3D {
            Material3D { id: paint; baseColor: #ff0000; metallic: 0.25; roughness: 0.75; }

            Mesh3D { id: painted; $QUAD Component { id: panel; width: 200; height: 100; } }
        }

        Mesh3D { id: bare; $QUAD Component { width: 200; height: 100; } }
        Mesh3D { id: plain; $QUAD }
        )yui")));

    auto* view = showSceneOf (document);
    auto* paint = document.findById<SceneMaterialComponent> ("paint");
    auto* painted = document.findById<SceneMeshComponent> ("painted");
    auto* panel = document.findById<Component> ("panel");
    auto* bare = document.findById<SceneMeshComponent> ("bare");
    auto* plain = document.findById<SceneMeshComponent> ("plain");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (paint, nullptr);
    ASSERT_NE (painted, nullptr);
    ASSERT_NE (panel, nullptr);
    ASSERT_NE (bare, nullptr);
    ASSERT_NE (plain, nullptr);

    // A node that shows no component is left without a material of its own
    EXPECT_EQ (materialOf (*plain), nullptr);

    // The one of the surface looks as the material the renderer draws a mesh that has none with
    const auto bareMaterial = materialOf (*bare);
    ASSERT_NE (bareMaterial, nullptr);
    EXPECT_FLOAT_EQ (bareMaterial->metallicFactor, 0.0f);
    EXPECT_FLOAT_EQ (bareMaterial->roughnessFactor, 0.5f);
    EXPECT_FLOAT_EQ (bareMaterial->baseColorFactor[1], 1.0f);

    // Or as the material above the node, whose place it takes for this mesh
    const auto paintedMaterial = materialOf (*painted);
    ASSERT_NE (paintedMaterial, nullptr);
    EXPECT_NE (paintedMaterial, paint->getMaterial());
    EXPECT_FLOAT_EQ (paintedMaterial->metallicFactor, 0.25f);
    EXPECT_FLOAT_EQ (paintedMaterial->roughnessFactor, 0.75f);
    EXPECT_FLOAT_EQ (paintedMaterial->baseColorFactor[0], 1.0f);
    EXPECT_FLOAT_EQ (paintedMaterial->baseColorFactor[1], 0.0f);

    // Which it follows at each frame of the scene
    paint->setProperty (Identifier ("roughness"), 0.5);
    view->refreshDisplay (0.0);

    EXPECT_EQ (materialOf (*painted), paintedMaterial);
    EXPECT_FLOAT_EQ (paintedMaterial->roughnessFactor, 0.5f);

    // It goes with the components, which their next parent paints again
    painted->removeChildComponent (panel);

    EXPECT_EQ (materialOf (*painted), nullptr);
    EXPECT_FALSE (panel->isManuallyComposited());
}

TEST_F (SceneSurfaceTests, AMeshOfSeveralPrimitivesShownByTheMaterialOfTheSurfaceIsWarnedAbout)
{
    files["panel.gltf"] = twoPrimitiveFile();

    const auto text = [] (const String& material)
    {
        return "Scene3D {\n"
               "    id: view;\n"
               "    Model3D {\n"
               "        source: \"panel.gltf\";\n"
               "        Node3D {\n"
               "            id: panel; node: \"Panel\"; "
             + material
             + "\n"
               "            Label { text: \"Shown\"; width: 100; height: 20; }\n"
               "        }\n"
               "    }\n"
               "}";
    };

    YuiDocument document;
    serve (document);

    const auto result = document.loadFromString (text (""));
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    auto* view = showSceneOf (document);
    auto* panel = document.findById<SceneNodeComponent> ("panel");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (panel, nullptr);
    ASSERT_NE (panel->getEntity().get<MeshNode>(), nullptr);
    ASSERT_NE (panel->getEntity().get<MeshNode>()->mesh, nullptr);
    ASSERT_EQ (panel->getEntity().get<MeshNode>()->mesh->getNumPrimitives(), 2);

    // The components are shown all the same, by the material the surface gives the part, which is then the one of
    // both primitives: a warning says so, where the node is written
    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());

    const auto& diagnostic = document.getDiagnostics().front();
    EXPECT_EQ (diagnostic.message, "Node3D: the components inside a node are shown by one material, which takes the place of the ones of the 2 primitives of its mesh: "
                                   "give the node a Material3D to choose that material, or bind it to a part drawn with one");
    EXPECT_EQ (diagnostic.line, 5);
    EXPECT_EQ (diagnostic.severity, DataTreeDiagnostic::Severity::warning);
    EXPECT_NE (materialOf (*panel), nullptr);

    // Told once, whatever the number of frames
    view->refreshDisplay (0.0);
    view->refreshDisplay (0.0);
    EXPECT_EQ (Internals::getNumProblems (*view), 1u);
    EXPECT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());

    // A Material3D of the node is the material its author chose for the whole part: nothing to tell
    load (document, text ("Material3D { }"));

    view = document.findById<SceneComponent> ("view");
    ASSERT_NE (view, nullptr);
    EXPECT_EQ (Internals::getNumProblems (*view), 0u);
}

TEST_F (SceneSurfaceTests, TheMaterial3DOfANodeIsTheOneThatShowsItsComponents)
{
    YuiDocument document;
    load (document, sceneOf (withQuad (R"yui(
        Mesh3D {
            id: quad; surface: baseColor; $QUAD

            Material3D { id: paint; baseColor: #808080; }
            Component { id: panel; width: 200; height: 100; }
        }

        Mesh3D { id: other; $QUAD }
        )yui")));

    auto* view = showSceneOf (document);
    auto* quad = document.findById<SceneMeshComponent> ("quad");
    auto* other = document.findById<SceneMeshComponent> ("other");
    auto* paint = document.findById<SceneMaterialComponent> ("paint");
    auto* panel = document.findById<Component> ("panel");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (quad, nullptr);
    ASSERT_NE (other, nullptr);
    ASSERT_NE (paint, nullptr);
    ASSERT_NE (panel, nullptr);

    // The slot of the material that takes the components: as emitted light unless the node says otherwise
    EXPECT_EQ (quad->getSurfaceSlot(), SceneNodeComponent::SurfaceSlot::baseColor);
    EXPECT_EQ (quad->getProperty (Identifier ("surface")).toString(), "baseColor");
    EXPECT_EQ (other->getSurfaceSlot(), SceneNodeComponent::SurfaceSlot::emissive);
    EXPECT_EQ (other->getProperty (Identifier ("surface")).toString(), "emissive");

    // The node keeps the material of its Material3D, which is as it was written while nothing is rendered to give it
    const auto material = paint->getMaterial();
    ASSERT_NE (material, nullptr);
    EXPECT_EQ (materialOf (*quad), material);
    EXPECT_EQ (material->baseColorTexture, nullptr);
    EXPECT_NEAR (material->baseColorFactor[0], toLinear (128.0f / 255.0f), 1.0e-4f);

    // Once the Material3D is gone, the surface gives the node a material of its own by the next frame of the scene
    quad->removeChildComponent (paint);
    EXPECT_EQ (materialOf (*quad), nullptr);

    view->refreshDisplay (0.0);

    const auto own = materialOf (*quad);
    ASSERT_NE (own, nullptr);
    EXPECT_NE (own, material);

    // The Material3D takes its place when it comes back, and stays when the components go
    quad->addChildComponent (paint);
    view->refreshDisplay (0.0);
    EXPECT_EQ (materialOf (*quad), material);

    quad->removeChildComponent (panel);
    EXPECT_EQ (materialOf (*quad), material);
}

TEST_F (SceneSurfaceTests, ComponentsThatComeAndGoWhileTheSceneIsShownBecomeAndStopBeingItsSurface)
{
    SceneComponent view;
    SceneCameraComponent camera;
    SceneMeshComponent quad;
    Component panel;
    Component badge;

    camera.setProjection (CameraNode::Projection::orthographic);
    camera.setOrthographicSize (2.0f);
    camera.setProperty (Identifier ("position"), numbers ({ 0.0, 0.0, 4.0 }));

    quad.setProperty (Identifier ("vertices"), numbers ({ -1.0, -1.0, 0.0, 1.0, -1.0, 0.0, 1.0, 1.0, 0.0, -1.0, 1.0, 0.0 }));
    quad.setProperty (Identifier ("uvs"), numbers ({ 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0 }));
    quad.setProperty (Identifier ("indices"), numbers ({ 0.0, 1.0, 2.0, 0.0, 2.0, 3.0 }));

    panel.setBounds (0.0f, 0.0f, 200.0f, 100.0f);
    badge.setBounds (200.0f, 100.0f, 100.0f, 50.0f);

    view.addAndMakeVisible (camera);
    view.addAndMakeVisible (quad);
    showScene (view);
    view.refreshDisplay (0.0);

    // Over the scene a component is where its bounds put it
    view.addAndMakeVisible (panel);
    EXPECT_FALSE (panel.isManuallyComposited());
    expectPoint (panel.getLocalPoint (&view, { 75.0f, 75.0f }), 75.0f, 75.0f);

    // Moved into a node it is its surface at once, and the node has the material that shows it by the next frame
    quad.addAndMakeVisible (panel);
    EXPECT_TRUE (panel.isManuallyComposited());
    expectShownAt (view, panel, { 75.0f, 75.0f }, { 50.0f, 25.0f });
    EXPECT_EQ (materialOf (quad), nullptr);

    view.refreshDisplay (0.0);
    EXPECT_NE (materialOf (quad), nullptr);

    // Another one makes the surface larger: 300 by 150
    quad.addAndMakeVisible (badge);
    EXPECT_TRUE (badge.isManuallyComposited());
    expectShownAt (view, badge, { 150.0f, 150.0f }, { 100.0f, 50.0f });
    expectShownAt (view, panel, { 100.0f, 100.0f }, { 150.0f, 75.0f });

    // One that leaves is painted where it goes, and the surface is the one of those that stay
    view.addAndMakeVisible (badge);
    EXPECT_FALSE (badge.isManuallyComposited());
    expectShownAt (view, panel, { 100.0f, 100.0f }, { 100.0f, 50.0f });
    EXPECT_NE (materialOf (quad), nullptr);

    // With the last one the material of the surface goes
    quad.removeChildComponent (panel);
    EXPECT_FALSE (panel.isManuallyComposited());
    EXPECT_EQ (materialOf (quad), nullptr);

    view.refreshDisplay (0.0);
    EXPECT_EQ (materialOf (quad), nullptr);
}

TEST_F (SceneSurfaceTests, TheComponentsAreRenderedAgainWhenWhatIsAboveThemIsShownOrEnabled)
{
    YuiDocument document;
    load (document, sceneOf (withQuad ("Node3D { id: group; Mesh3D { id: quad; $QUAD Component { id: panel; width: 200; height: 100; } } }")));

    auto* view = showSceneOf (document);
    auto* group = document.findById<SceneNodeComponent> ("group");
    auto* quad = document.findById<SceneMeshComponent> ("quad");
    auto* panel = document.findById<Component> ("panel");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (group, nullptr);
    ASSERT_NE (quad, nullptr);
    ASSERT_NE (panel, nullptr);

    // A component is rendered for its node when it asked to be painted. What is above it does not ask it to
    Component* const above[] = { quad, group, view };

    for (auto* parent : above)
    {
        SCOPED_TRACE (parent == quad ? "its node" : parent == group ? "a node around its node" : "the scene");

        // What it asks while it is not showing is dropped: it is asked again once it shows
        parent->setVisible (false);
        Internals::takeRepaintRequest (*panel);

        panel->repaint();
        EXPECT_FALSE (Internals::takeRepaintRequest (*panel));

        parent->setVisible (true);
        EXPECT_TRUE (Internals::takeRepaintRequest (*panel));

        // It paints itself enabled or not, as what is above it is
        parent->setEnabled (false);
        EXPECT_FALSE (panel->isEnabled());
        EXPECT_TRUE (Internals::takeRepaintRequest (*panel));

        parent->setEnabled (true);
        EXPECT_TRUE (panel->isEnabled());
        EXPECT_TRUE (Internals::takeRepaintRequest (*panel));
    }
}

TEST_F (SceneSurfaceTests, TheSurfaceFollowsTheLoadsOfItsDocument)
{
    const auto onTheMesh = sceneOf (withQuad ("Mesh3D { id: quad; $QUAD Component { id: panel; width: 200; height: 100; } }"));

    YuiDocument document;
    load (document, onTheMesh);

    auto* view = showSceneOf (document);
    auto* quad = document.findById<SceneMeshComponent> ("quad");
    auto* panel = document.findById<Component> ("panel");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (quad, nullptr);
    ASSERT_NE (panel, nullptr);

    EXPECT_TRUE (panel->isManuallyComposited());
    EXPECT_NE (materialOf (*quad), nullptr);
    expectShownAt (*view, *panel, { 150.0f, 150.0f }, { 200.0f, 100.0f });

    // A component more: the surface is the one of both
    load (document, sceneOf (withQuad (R"yui(
        Mesh3D {
            id: quad; $QUAD

            Component { id: panel; width: 200; height: 100; }
            Component { id: badge; x: 200; y: 100; width: 100; height: 50; }
        })yui")));

    view = showSceneOf (document);
    quad = document.findById<SceneMeshComponent> ("quad");
    panel = document.findById<Component> ("panel");
    auto* badge = document.findById<Component> ("badge");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (quad, nullptr);
    ASSERT_NE (panel, nullptr);
    ASSERT_NE (badge, nullptr);

    EXPECT_TRUE (panel->isManuallyComposited());
    EXPECT_TRUE (badge->isManuallyComposited());
    EXPECT_NE (materialOf (*quad), nullptr);
    expectShownAt (*view, *badge, { 150.0f, 150.0f }, { 100.0f, 50.0f });

    // The component written over the scene, and the mesh left with none
    load (document, sceneOf (withQuad ("Component { id: panel; width: 200; height: 100; } Mesh3D { id: quad; $QUAD }")));

    view = showSceneOf (document);
    quad = document.findById<SceneMeshComponent> ("quad");
    panel = document.findById<Component> ("panel");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (quad, nullptr);
    ASSERT_NE (panel, nullptr);

    EXPECT_EQ (panel->getParentComponent(), view);
    EXPECT_FALSE (panel->isManuallyComposited());
    EXPECT_EQ (materialOf (*quad), nullptr);

    // And on the mesh again
    load (document, onTheMesh);

    view = showSceneOf (document);
    quad = document.findById<SceneMeshComponent> ("quad");
    panel = document.findById<Component> ("panel");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (quad, nullptr);
    ASSERT_NE (panel, nullptr);

    EXPECT_EQ (panel->getParentComponent(), quad);
    EXPECT_TRUE (panel->isManuallyComposited());
    EXPECT_NE (materialOf (*quad), nullptr);
    expectShownAt (*view, *panel, { 75.0f, 75.0f }, { 50.0f, 25.0f });
}

TEST_F (SceneSurfaceTests, TheComponentsOfABoundNodeAreShownOnThatPartOfTheModel)
{
    int pressed = 0;

    YuiDocument document;
    files["device.gltf"] = deviceFile();
    serve (document);

    document.setAction ("pressed", [&pressed] (const Array<var>&)
    {
        ++pressed;
    });

    // The screen of the device is a square shown from 62.5 to 87.5, in front of its body
    load (document, sceneOf (R"yui(
        Model3D {
            id: device; source: "device.gltf";

            Node3D {
                id: screen; node: "Screen";

                TextButton { id: button; width: 100; height: 100; onClick: action ("pressed"); }
            }
        })yui"));

    auto* view = showSceneOf (document);
    auto* device = document.findById<SceneModelComponent> ("device");
    auto* screen = document.findById<SceneNodeComponent> ("screen");
    auto* button = document.findById<TextButton> ("button");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (device, nullptr);
    ASSERT_NE (screen, nullptr);
    ASSERT_NE (button, nullptr);

    EXPECT_TRUE (button->isManuallyComposited());

    // Over the texture coordinates the model gives the part
    expectShownAt (*view, *button, { 62.5f, 62.5f }, { 0.0f, 0.0f });
    expectShownAt (*view, *button, { 75.0f, 75.0f }, { 50.0f, 50.0f });
    expectShownAt (*view, *button, { 87.5f, 87.5f }, { 100.0f, 100.0f });

    {
        PointerSimulator pointer (*view);

        EXPECT_EQ (pointer.click ({ 75.0f, 75.0f }), button);
        EXPECT_EQ (pressed, 1);

        // The rest of the model is not the screen
        EXPECT_EQ (pointer.click ({ 100.0f, 100.0f }), device);
        EXPECT_EQ (pressed, 1);
    }

    // The part is given the material that shows the button: the one sharing its mesh is left alone
    const auto part = screen->getEntity();
    ASSERT_TRUE (part.isValid());
    EXPECT_NE (part.get<MaterialNode>(), nullptr);

    const auto knob = device->getInstance().findChild ("Knob");
    ASSERT_TRUE (knob.isValid());
    ASSERT_EQ (knob.get<MeshNode>()->mesh, part.get<MeshNode>()->mesh);
    EXPECT_EQ (knob.get<MaterialNode>(), nullptr);

    // And is as the model made it once the node is gone
    load (document, sceneOf ("Model3D { id: device; source: \"device.gltf\"; }"));

    device = document.findById<SceneModelComponent> ("device");
    ASSERT_NE (device, nullptr);

    const auto left = device->getInstance().findChild ("Screen");
    ASSERT_TRUE (left.isValid());
    EXPECT_EQ (left.get<MaterialNode>(), nullptr);
}
