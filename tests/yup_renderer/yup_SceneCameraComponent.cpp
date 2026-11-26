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

/** The `Camera3D` of a `.yui` document: a node giving its entity a CameraNode, the first active
    one being the camera of the scene.
*/
class SceneCameraComponentTests : public SceneDocumentTests
{
protected:
    /** Where the camera of a node looks, which is down the -z axis of its entity. */
    static Vector3<float> directionOf (const SceneNodeComponent& node)
    {
        return rotationOf (node).rotate ({ 0.0f, 0.0f, -1.0f });
    }
};

TEST_F (SceneCameraComponentTests, PropertiesReachTheCameraNode)
{
    YuiDocument document;
    load (document, R"yui(
        Scene3D {
            id: view;

            Camera3D { id: camera; fov: 60deg; near: 0.5; far: 50; projection: orthographic; orthographicSize: 3; }
            Camera3D { id: plain; }
        })yui");

    auto* view = document.findById<SceneComponent> ("view");
    auto* camera = document.findById<SceneCameraComponent> ("camera");
    auto* plain = document.findById<SceneCameraComponent> ("plain");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (camera, nullptr);
    ASSERT_NE (plain, nullptr);

    const auto* node = camera->getEntity().get<CameraNode>();
    ASSERT_NE (node, nullptr);

    EXPECT_EQ (node->projection, CameraNode::Projection::orthographic);
    EXPECT_NEAR (node->yFov, MathConstants<float>::pi / 3.0f, 1.0e-5f);
    EXPECT_FLOAT_EQ (node->zNear, 0.5f);
    EXPECT_FLOAT_EQ (node->zFar, 50.0f);
    EXPECT_FLOAT_EQ (node->yMag, 3.0f);

    // An orthographic view is as wide as the shape of the scene makes it
    view->setBounds (0.0f, 0.0f, 200.0f, 100.0f);
    EXPECT_FLOAT_EQ (node->xMag, 6.0f);

    // A camera that sets nothing is the one a CameraNode is by default
    const auto* plainNode = plain->getEntity().get<CameraNode>();
    const CameraNode defaults {};

    ASSERT_NE (plainNode, nullptr);
    EXPECT_EQ (plainNode->projection, defaults.projection);
    EXPECT_FLOAT_EQ (plainNode->yFov, defaults.yFov);
    EXPECT_FLOAT_EQ (plainNode->zNear, defaults.zNear);
    EXPECT_FLOAT_EQ (plainNode->zFar, defaults.zFar);
    EXPECT_FLOAT_EQ (plain->getFieldOfView(), defaults.yFov);
    EXPECT_TRUE (plain->isActive());

    camera->setFieldOfView (1.0f);
    EXPECT_FLOAT_EQ (node->yFov, 1.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (camera->getProperty (Identifier ("fov"))), 1.0f);
}

TEST_F (SceneCameraComponentTests, TheFirstActiveCameraIsTheOneOfTheScene)
{
    YuiDocument document;
    load (document, R"yui(
        Scene3D {
            id: view;

            Node3D { Camera3D { id: first; } }
            Camera3D { id: second; }
        })yui");

    auto* view = document.findById<SceneComponent> ("view");
    auto* first = document.findById<SceneCameraComponent> ("first");
    auto* second = document.findById<SceneCameraComponent> ("second");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (first, nullptr);
    ASSERT_NE (second, nullptr);

    const auto scene = view->getScene();
    ASSERT_NE (scene, nullptr);

    // In the order they are written, whatever their depth
    ASSERT_NE (scene->getActiveCamera(), nullptr);
    EXPECT_EQ (scene->getActiveCamera(), first->getEntity().get<CameraNode>());

    // Turned off, it hands the view over to the next one and can't be fallen back on
    first->setProperty (Identifier ("active"), false);
    EXPECT_EQ (first->getEntity().get<CameraNode>(), nullptr);
    ASSERT_NE (scene->getActiveCamera(), nullptr);
    EXPECT_EQ (scene->getActiveCamera(), second->getEntity().get<CameraNode>());

    first->setActive (true);
    EXPECT_EQ (scene->getActiveCamera(), first->getEntity().get<CameraNode>());

    // A camera that goes hands it over too
    load (document, "Scene3D { id: view; Node3D { } Camera3D { id: second; } }");
    second = document.findById<SceneCameraComponent> ("second");
    ASSERT_NE (second, nullptr);
    ASSERT_EQ (document.findById<SceneComponent> ("view"), view);
    ASSERT_EQ (view->getScene(), scene);
    ASSERT_NE (scene->getActiveCamera(), nullptr);
    EXPECT_EQ (scene->getActiveCamera(), second->getEntity().get<CameraNode>());

    // Without any camera the scene has none, and the renderer uses its default one
    load (document, "Scene3D { id: view; Node3D { } }");
    EXPECT_EQ (scene->getActiveCamera(), nullptr);
}

TEST_F (SceneCameraComponentTests, ACameraAddedBeforeTheOthersTakesTheView)
{
    SceneComponent view;
    SceneCameraComponent first, second;

    view.addAndMakeVisible (first);
    ASSERT_NE (view.getScene(), nullptr);
    EXPECT_EQ (view.getScene()->getActiveCamera(), first.getEntity().get<CameraNode>());

    view.addAndMakeVisible (second, 0);
    EXPECT_EQ (view.getScene()->getActiveCamera(), second.getEntity().get<CameraNode>());

    view.removeChildComponent (second);
    EXPECT_EQ (view.getScene()->getActiveCamera(), first.getEntity().get<CameraNode>());
}

TEST_F (SceneCameraComponentTests, ACamera3DWinsOverACameraTheSceneFindsBeforeIt)
{
    SceneComponent view;

    // An entity made by code and carrying a camera, before anything a component makes: the one the scene
    // falls back on while it is given none, as it is the first it finds
    auto& scene = view.getOrCreateScene();
    auto other = scene.createEntity ("other");
    other.attach<CameraNode>();
    ASSERT_EQ (scene.getActiveCamera(), other.get<CameraNode>());

    SceneCameraComponent camera;
    view.addAndMakeVisible (camera);

    ASSERT_TRUE (camera.getEntity().isValid());
    ASSERT_EQ (scene.getRoot().getChild (0), other);
    EXPECT_EQ (scene.getActiveCamera(), camera.getEntity().get<CameraNode>());

    // Turned off, the scene falls back on the other one, and it is given the Camera3D again once that one is on
    camera.setActive (false);
    EXPECT_EQ (scene.getActiveCamera(), other.get<CameraNode>());

    camera.setActive (true);
    EXPECT_EQ (scene.getActiveCamera(), camera.getEntity().get<CameraNode>());
}

TEST_F (SceneCameraComponentTests, LookAtTurnsTheCameraAndFollowsItsPosition)
{
    YuiDocument document;
    load (document, R"yui(
        Scene3D {
            Camera3D { id: camera; position: [4, 0, 0]; rotation: [0, 1, 0]; lookAt: [0, 0, 0]; }
        })yui");

    auto* camera = document.findById<SceneCameraComponent> ("camera");
    ASSERT_NE (camera, nullptr);

    // The point wins over the rotation
    expectVector (directionOf (*camera), -1.0f, 0.0f, 0.0f);

    camera->setProperty (Identifier ("position"), numbers ({ 0.0, 0.0, -4.0 }));
    expectVector (directionOf (*camera), 0.0f, 0.0f, 1.0f);

    camera->setProperty (Identifier ("lookAt"), numbers ({ 0.0, 4.0, -4.0 }));
    expectVector (directionOf (*camera), 0.0f, 1.0f, 0.0f);

    // Without a point the rotation rules again
    camera->setProperty (Identifier ("lookAt"), var());
    expectRotation (rotationOf (*camera), Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 1.0f));
}

TEST_F (SceneCameraComponentTests, AValueACameraCanNotHaveIsReportedAndNotKept)
{
    YuiDocument document;
    const auto result = document.loadFromString ("Scene3D {\n"
                                                 "    id: view;\n"
                                                 "    Camera3D {\n"
                                                 "        id: camera;\n"
                                                 "        fov: 0;\n"
                                                 "        near: -1;\n"
                                                 "        far: -5;\n"
                                                 "        orthographicSize: 0;\n"
                                                 "    }\n"
                                                 "}");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    expectDiagnostic (document, "Camera3D.fov: expected an angle above 0 and below 180deg");
    expectDiagnostic (document, "Camera3D.near: expected a distance above 0");
    expectDiagnostic (document, "Camera3D.far: expected a distance, or 0 for a perspective camera to see infinitely far");
    expectDiagnostic (document, "Camera3D.orthographicSize: expected a size above 0");
    EXPECT_EQ (document.getDiagnostics().size(), 4u) << DataTreeDiagnostic::join (document.getDiagnostics());

    // Where the value is written
    const auto* diagnostic = findDiagnostic (document, "Camera3D.fov");
    ASSERT_NE (diagnostic, nullptr);
    EXPECT_EQ (diagnostic->line, 5);

    auto* view = document.findById<SceneComponent> ("view");
    auto* camera = document.findById<SceneCameraComponent> ("camera");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (camera, nullptr);

    // The camera keeps the values it had: the ones a CameraNode has by default
    const auto* node = camera->getEntity().get<CameraNode>();
    const CameraNode defaults {};

    ASSERT_NE (node, nullptr);
    EXPECT_FLOAT_EQ (node->yFov, defaults.yFov);
    EXPECT_FLOAT_EQ (node->zNear, defaults.zNear);
    EXPECT_FLOAT_EQ (node->zFar, defaults.zFar);
    EXPECT_FLOAT_EQ (node->yMag, defaults.yMag);
    EXPECT_FLOAT_EQ (camera->getFieldOfView(), defaults.yFov);

    // So it does with a value that is not a number, or not a finite one, which does not ask for the scene to be painted
    view->refreshDisplay (0.0);
    Internals::takeRepaintRequest (*view);

    camera->setFieldOfView (std::numeric_limits<float>::quiet_NaN());
    camera->setNearPlane (std::numeric_limits<float>::infinity());
    camera->setFieldOfView (MathConstants<float>::pi);

    EXPECT_FLOAT_EQ (node->yFov, defaults.yFov);
    EXPECT_FLOAT_EQ (node->zNear, defaults.zNear);
    EXPECT_FALSE (Internals::takeRepaintRequest (*view));
    EXPECT_EQ (Internals::getNumProblems (*view), 4u);

    // A value it can have is kept, and withdraws what was reported about the property
    camera->setFieldOfView (1.0f);

    EXPECT_FLOAT_EQ (node->yFov, 1.0f);
    EXPECT_EQ (Internals::getNumProblems (*view), 3u);
}

TEST_F (SceneCameraComponentTests, AnOrthographicCameraWhoseFarPlaneIsNotBeyondItsNearOneIsReported)
{
    YuiDocument document;
    const auto result = document.loadFromString ("Scene3D {\n"
                                                 "    id: view;\n"
                                                 "    Camera3D {\n"
                                                 "        id: camera;\n"
                                                 "        projection: orthographic;\n"
                                                 "        far: 0;\n"
                                                 "    }\n"
                                                 "}");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());

    // An infinite far plane is the one of a perspective camera
    EXPECT_EQ (document.getDiagnostics().front().message, "Camera3D.projection: an orthographic camera sees up to its far plane, which has to be beyond its near one");
    EXPECT_EQ (document.getDiagnostics().front().line, 5);

    auto* view = document.findById<SceneComponent> ("view");
    auto* camera = document.findById<SceneCameraComponent> ("camera");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (camera, nullptr);
    EXPECT_EQ (Internals::getNumProblems (*view), 1u);

    // It stands for as long as the two planes and the projection do not fit together, whichever of them changes
    camera->setProjection (CameraNode::Projection::perspective);
    EXPECT_EQ (Internals::getNumProblems (*view), 0u);

    camera->setProjection (CameraNode::Projection::orthographic);
    EXPECT_EQ (Internals::getNumProblems (*view), 1u);

    camera->setFarPlane (10.0f);
    EXPECT_EQ (Internals::getNumProblems (*view), 0u);

    camera->setNearPlane (20.0f);
    EXPECT_EQ (Internals::getNumProblems (*view), 1u);
}

TEST_F (SceneCameraComponentTests, WhatCodeSetsOnTheCameraNodeIsKeptWhenTheCameraMovesToAnotherParent)
{
    SceneComponent view;
    SceneNodeComponent left, right;
    SceneCameraComponent camera;

    view.addAndMakeVisible (left);
    view.addAndMakeVisible (right);
    left.addAndMakeVisible (camera);

    auto* lens = camera.getEntity().get<CameraNode>();
    ASSERT_NE (lens, nullptr);
    ASSERT_EQ (view.getScene()->getActiveCamera(), lens);

    // The depth of field has no property
    lens->fStop = 1.4f;
    lens->focusDistance = 3.0f;

    right.addAndMakeVisible (camera);

    // The camera moved with its entity, which carries the same CameraNode, and is still the one of the scene
    ASSERT_EQ (camera.getEntity().get<CameraNode>(), lens);
    EXPECT_EQ (camera.getEntity().getParent(), right.getEntity());
    EXPECT_FLOAT_EQ (lens->fStop, 1.4f);
    EXPECT_FLOAT_EQ (lens->focusDistance, 3.0f);
    EXPECT_EQ (view.getScene()->getActiveCamera(), lens);

    // Taken out of the scene, it is not the camera the scene is seen through, before any frame releases it
    right.removeChildComponent (camera);

    EXPECT_TRUE (camera.getEntity().isValid());
    EXPECT_EQ (view.getScene()->getActiveCamera(), nullptr);
}
