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

/** The `Mesh3D` of a `.yui` document: a node drawing the triangles its arrays of numbers describe. */
class SceneMeshComponentTests : public SceneDocumentTests
{
protected:
    /** The mesh on the entity of a Mesh3D, or nullptr. */
    static Mesh::Ptr meshOf (const SceneMeshComponent& component)
    {
        const auto* node = component.getEntity().get<MeshNode>();
        return node != nullptr ? node->mesh : Mesh::Ptr();
    }
};

TEST_F (SceneMeshComponentTests, TakesTheVerticesThreeByThreeWithoutIndices)
{
    YuiDocument document;
    load (document, "Scene3D { Mesh3D { id: triangle; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; } }");

    auto* triangle = document.findById<SceneMeshComponent> ("triangle");
    ASSERT_NE (triangle, nullptr);

    const auto mesh = meshOf (*triangle);
    ASSERT_NE (mesh, nullptr);
    EXPECT_EQ (triangle->getMesh(), mesh);
    ASSERT_EQ (mesh->getNumPrimitives(), 1);

    const auto& primitive = mesh->getPrimitive (0);
    ASSERT_EQ (primitive.vertices.size(), 3u);
    ASSERT_EQ (primitive.indices.size(), 3u);

    expectVector (primitive.vertices[1].position, 1.0f, 0.0f, 0.0f);

    // Without normals the triangle gets flat ones, facing the side its counter-clockwise winding faces
    for (const auto& vertex : primitive.vertices)
        expectVector (vertex.normal, 0.0f, 0.0f, 1.0f);

    EXPECT_EQ (primitive.material, nullptr);
}

TEST_F (SceneMeshComponentTests, KeepsTheNormalsUvsColorsAndIndicesItIsGiven)
{
    YuiDocument document;
    load (document, R"yui(
        Scene3D {
            Mesh3D {
                id: quad;
                vertices: [-1, -1, 0,  1, -1, 0,  1, 1, 0,  -1, 1, 0];
                normals: [0, 0, 1,  0, 0, 1,  0, 0, 1,  0, 1, 0];
                uvs: [0, 1,  1, 1,  1, 0,  0, 0];
                colors: [1, 0, 0, 1,  0, 1, 0, 1,  0, 0, 1, 1,  1, 1, 1, 0.5];
                indices: [0, 1, 2,  0, 2, 3];
            }
        })yui");

    auto* quad = document.findById<SceneMeshComponent> ("quad");
    ASSERT_NE (quad, nullptr);

    const auto mesh = meshOf (*quad);
    ASSERT_NE (mesh, nullptr);
    ASSERT_EQ (mesh->getNumPrimitives(), 1);

    // The vertices stay shared by the triangles, as they come with their normals
    const auto& primitive = mesh->getPrimitive (0);
    ASSERT_EQ (primitive.vertices.size(), 4u);
    EXPECT_EQ (primitive.indices, (std::vector<uint32> { 0, 1, 2, 0, 2, 3 }));

    expectVector (primitive.vertices[2].position, 1.0f, 1.0f, 0.0f);
    expectVector (primitive.vertices[2].normal, 0.0f, 0.0f, 1.0f);
    expectVector (primitive.vertices[3].normal, 0.0f, 1.0f, 0.0f);

    EXPECT_FLOAT_EQ (primitive.vertices[1].uv.getX(), 1.0f);
    EXPECT_FLOAT_EQ (primitive.vertices[1].uv.getY(), 1.0f);
    EXPECT_FLOAT_EQ (primitive.vertices[2].uv.getY(), 0.0f);

    EXPECT_EQ (primitive.vertices[1].color, (std::array<float, 4> { 0.0f, 1.0f, 0.0f, 1.0f }));
    EXPECT_EQ (primitive.vertices[3].color, (std::array<float, 4> { 1.0f, 1.0f, 1.0f, 0.5f }));

    // The arrays read back as they were given
    expectNumbers (quad->getProperty (Identifier ("uvs")), { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0 });
    expectNumbers (quad->getProperty (Identifier ("indices")), { 0.0, 1.0, 2.0, 0.0, 2.0, 3.0 });
}

TEST_F (SceneMeshComponentTests, AMeshWithoutVerticesDrawsNothingAndReportsNothing)
{
    YuiDocument document;
    load (document, "Scene3D { Mesh3D { id: empty; } }");

    auto* empty = document.findById<SceneMeshComponent> ("empty");
    ASSERT_NE (empty, nullptr);

    EXPECT_EQ (empty->getMesh(), nullptr);
    EXPECT_EQ (meshOf (*empty), nullptr);
    expectNumbers (empty->getProperty (Identifier ("vertices")), {});
}

TEST_F (SceneMeshComponentTests, ArraysThatDoNotFitTogetherDrawNothingAndAreReported)
{
    struct Case
    {
        const char* arrays;
        const char* diagnostic;
    };

    const Case cases[] = {
        { "vertices: [0, 0, 0,  1, 0];", "Mesh3D.vertices: expected an x, y and z for each vertex" },
        { "vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0,  1, 1, 0];", "Mesh3D.vertices: without indices the vertices are taken three by three" },
        { "vertices: [0, 0, 0,  1, 0, 0]; indices: [0, 1, 0];", "Mesh3D.vertices: a triangle takes 3 vertices" },
        { "vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; normals: [0, 0, 1,  0, 0, 1];", "Mesh3D.normals: expected 9 numbers" },
        { "vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; uvs: [0, 0,  1, 0,  0, 1,  1, 1];", "Mesh3D.uvs: expected 6 numbers" },
        { "vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; colors: [1, 1, 1];", "Mesh3D.colors: expected 12 numbers" },
        { "vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; indices: [0, 1];", "Mesh3D.indices: expected three indices for each triangle" },
        { "vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; indices: [0, 1, 3];", "Mesh3D.indices: the index 3 is not the one of a vertex" },
        { "vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; indices: [0, -1, 2];", "Mesh3D.indices: the index -1 is not the one of a vertex" },
        { "indices: [0, 1, 2];", "Mesh3D.indices: the index 0 is not the one of a vertex" },
    };

    for (const auto& test : cases)
    {
        SCOPED_TRACE (test.arrays);

        YuiDocument document;
        const auto result = document.loadFromString ("Scene3D { Mesh3D { id: broken; " + String (test.arrays) + " } }");
        ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

        expectDiagnostic (document, test.diagnostic);
        EXPECT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());

        auto* broken = document.findById<SceneMeshComponent> ("broken");
        ASSERT_NE (broken, nullptr);
        EXPECT_EQ (broken->getMesh(), nullptr);
        EXPECT_EQ (meshOf (*broken), nullptr);
    }
}

TEST_F (SceneMeshComponentTests, AProblemIsReportedWhereItsArrayIsWritten)
{
    YuiDocument document;
    const auto result = document.loadFromString ("Scene3D {\n"
                                                 "    Mesh3D {\n"
                                                 "        vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0];\n"
                                                 "        indices: [0, 1, 7];\n"
                                                 "    }\n"
                                                 "}");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());

    const auto& diagnostic = document.getDiagnostics().front();
    EXPECT_EQ (diagnostic.message, "Mesh3D.indices: the index 7 is not the one of a vertex, there are 3");
    EXPECT_EQ (diagnostic.line, 4);
    EXPECT_TRUE (diagnostic.isError());

    // It is told again by the loads that keep it, and no more once the array is right
    ASSERT_TRUE (document.loadFromString ("Scene3D {\n"
                                          "    Mesh3D {\n"
                                          "        vertices: [0, 0, 0,  1, 0, 0,  0, 2, 0];\n"
                                          "        indices: [0, 1, 7];\n"
                                          "    }\n"
                                          "}")
                     .wasOk());

    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (document.getDiagnostics().front().line, 4);

    load (document, "Scene3D {\n"
                    "    Mesh3D {\n"
                    "        vertices: [0, 0, 0,  1, 0, 0,  0, 2, 0];\n"
                    "        indices: [0, 1, 2];\n"
                    "    }\n"
                    "}");
}

TEST_F (SceneMeshComponentTests, ArraysChangedTogetherBuildTheMeshOnce)
{
    YuiDocument document;
    load (document, "Scene3D { id: view; Mesh3D { id: quad; " + quadGeometry() + " } }");

    auto* view = document.findById<SceneComponent> ("view");
    auto* quad = document.findById<SceneMeshComponent> ("quad");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (quad, nullptr);

    const auto first = meshOf (*quad);
    ASSERT_NE (first, nullptr);

    // Nothing is built while the arrays change, one after the other
    quad->setProperty (Identifier ("vertices"), numbers ({ 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 2.0, 0.0 }));
    quad->setProperty (Identifier ("indices"), numbers ({ 0.0, 1.0, 2.0 }));
    quad->setProperty (Identifier ("normals"), numbers ({ 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0 }));

    EXPECT_EQ (meshOf (*quad), first);

    // The next frame of the scene builds the mesh they make
    view->refreshDisplay (0.0);

    const auto second = meshOf (*quad);
    ASSERT_NE (second, nullptr);
    EXPECT_NE (second, first);
    ASSERT_EQ (second->getPrimitive (0).vertices.size(), 3u);
    expectVector (second->getPrimitive (0).vertices[1].position, 2.0f, 0.0f, 0.0f);

    // And it stays until an array changes again
    view->refreshDisplay (0.0);
    EXPECT_EQ (meshOf (*quad), second);
    EXPECT_EQ (quad->getMesh(), second);

    quad->setProperty (Identifier ("vertices"), numbers ({ 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 2.0, 0.0 }));
    EXPECT_EQ (quad->getMesh(), second);

    // Asking for the mesh builds it too
    quad->setProperty (Identifier ("vertices"), numbers ({ 0.0, 0.0, 0.0, 3.0, 0.0, 0.0, 0.0, 3.0, 0.0 }));
    const auto third = quad->getMesh();

    ASSERT_NE (third, nullptr);
    EXPECT_NE (third, second);
    EXPECT_EQ (meshOf (*quad), third);
}

TEST_F (SceneMeshComponentTests, AMeshEnteringASceneIsBuiltThere)
{
    SceneComponent view;
    SceneMeshComponent mesh;

    mesh.setProperty (Identifier ("vertices"), numbers ({ 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0 }));

    // Outside a scene there is no entity to draw it, the mesh is still made when asked for
    const auto alone = mesh.getMesh();
    ASSERT_NE (alone, nullptr);
    EXPECT_EQ (alone->getPrimitive (0).vertices.size(), 3u);

    view.addAndMakeVisible (mesh);
    EXPECT_EQ (meshOf (mesh), nullptr);

    view.refreshDisplay (0.0);
    ASSERT_NE (meshOf (mesh), nullptr);
    EXPECT_EQ (meshOf (mesh), mesh.getMesh());
}

TEST_F (SceneMeshComponentTests, AMeshBuiltByAFrameOfTheSceneIsPaintedAfterIt)
{
    YuiDocument document;
    load (document, "Scene3D { id: view; Mesh3D { id: quad; " + quadGeometry() + " } }");

    auto* view = showSceneOf (document);
    auto* quad = document.findById<SceneMeshComponent> ("quad");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (quad, nullptr);

    // The frame of the scene comes first, then what changes the arrays in the same display frame: a style rule or
    // a binding of a document whose root is the scene. The paint of that frame draws the mesh as it was
    view->refreshDisplay (0.0);
    quad->setProperty (Identifier ("vertices"), numbers ({ -2.0, -2.0, 0.0, 2.0, -2.0, 0.0, 2.0, 2.0, 0.0, -2.0, 2.0, 0.0 }));

    const auto drawn = meshOf (*quad);
    EXPECT_TRUE (Internals::takeRepaintRequest (*view));

    // The next frame builds the mesh the arrays make, and has it painted
    view->refreshDisplay (0.0);

    EXPECT_NE (meshOf (*quad), drawn);
    EXPECT_TRUE (Internals::takeRepaintRequest (*view));

    // A scene that stands still is not painted again
    view->refreshDisplay (0.0);
    EXPECT_FALSE (Internals::takeRepaintRequest (*view));
}

TEST_F (SceneMeshComponentTests, AnArrayReadFromAMeshIsNotTheOneItKeeps)
{
    YuiDocument document;
    load (document, "Scene3D { Mesh3D { id: triangle; vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; } }");

    auto* triangle = document.findById<SceneMeshComponent> ("triangle");
    ASSERT_NE (triangle, nullptr);

    const Identifier vertices ("vertices");
    const auto first = triangle->getMesh();
    ASSERT_NE (first, nullptr);

    // Changed in place, the array that was read leaves the mesh its own numbers
    auto read = triangle->getProperty (vertices);
    ASSERT_NE (read.getArray(), nullptr);
    read.getArray()->set (7, 2.0);

    expectNumbers (triangle->getProperty (vertices), { 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0 });
    EXPECT_EQ (triangle->getMesh(), first);

    // Given back, it is another value: the mesh is built again
    triangle->setProperty (vertices, read);

    const auto second = triangle->getMesh();
    ASSERT_NE (second, nullptr);
    EXPECT_NE (second, first);
    expectVector (second->getPrimitive (0).vertices[2].position, 0.0f, 2.0f, 0.0f);

    // And the mesh does not follow the array it was given either
    read.getArray()->set (7, 3.0);

    expectNumbers (triangle->getProperty (vertices), { 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 2.0, 0.0 });
    EXPECT_EQ (triangle->getMesh(), second);
}

TEST_F (SceneMeshComponentTests, WhatIsWrongWithTheArraysIsToldOnceTheMeshIsTheOneTheyMake)
{
    YuiDocument document;
    load (document, "Scene3D { id: view; Mesh3D { id: quad; " + quadGeometry() + " } }");

    auto* view = document.findById<SceneComponent> ("view");
    auto* quad = document.findById<SceneMeshComponent> ("quad");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (quad, nullptr);
    ASSERT_NE (meshOf (*quad), nullptr);

    // Who hears of the problem may reload the document, which destroys the mesh: by then nothing is left to do with it
    auto drawnWhenTold = meshOf (*quad);
    StringArray told;

    const DiagnosticListener listener (document, [&] (const DataTreeDiagnostic& diagnostic)
    {
        told.add (diagnostic.message);
        drawnWhenTold = meshOf (*quad);
    });

    quad->setProperty (Identifier ("indices"), numbers ({ 0.0, 1.0, 9.0 }));
    view->refreshDisplay (0.0);

    ASSERT_EQ (told.size(), 1);
    EXPECT_EQ (told[0], "Mesh3D.indices: the index 9 is not the one of a vertex, there are 4");
    EXPECT_EQ (drawnWhenTold, nullptr);
}

TEST_F (SceneMeshComponentTests, AMeshThatKeepsChangingIsBuiltOnceByTheNextFrame)
{
    SceneComponent view;
    SceneMeshComponent mesh;

    view.addAndMakeVisible (mesh);

    // What an offscreen loop does: the arrays change and the mesh is asked for, with no frame of the scene in between
    for (int size = 1; size <= 4; ++size)
    {
        mesh.setProperty (Identifier ("vertices"), numbers ({ 0.0, 0.0, 0.0, static_cast<double> (size), 0.0, 0.0, 0.0, 1.0, 0.0 }));
        ASSERT_NE (mesh.getMesh(), nullptr);

        EXPECT_EQ (Internals::getNumChangedMeshes (view), 1);
    }

    // Nor does the scene keep the meshes that are gone before it has a frame
    for (int pass = 0; pass < 4; ++pass)
    {
        SceneMeshComponent other;
        view.addAndMakeVisible (other);

        EXPECT_EQ (Internals::getNumChangedMeshes (view), 2);
    }
}

TEST_F (SceneMeshComponentTests, NumbersThatAreNotFiniteAndIndicesThatAreNotWholeDrawNothingAndAreReported)
{
    YuiDocument document;
    load (document, "Scene3D { id: view; Mesh3D { id: quad; " + quadGeometry() + " } }");

    auto* quad = document.findById<SceneMeshComponent> ("quad");
    ASSERT_NE (quad, nullptr);
    ASSERT_NE (meshOf (*quad), nullptr);

    const Identifier vertices ("vertices"), indices ("indices");

    // A vertex that is nowhere would make the bounds of the mesh, and all that is fitted to them, meaningless
    for (const auto nowhere : { std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity() })
    {
        quad->setProperty (vertices, numbers ({ -1.0, -1.0, 0.0, 1.0, nowhere, 0.0, 1.0, 1.0, 0.0, -1.0, 1.0, 0.0 }));

        EXPECT_EQ (quad->getMesh(), nullptr);
        EXPECT_EQ (meshOf (*quad), nullptr);
    }

    expectDiagnostic (document, "Mesh3D.vertices: expected numbers");

    // An index between two vertices is not taken for one of them
    quad->setProperty (vertices, numbers ({ -1.0, -1.0, 0.0, 1.0, -1.0, 0.0, 1.0, 1.0, 0.0, -1.0, 1.0, 0.0 }));
    quad->setProperty (indices, numbers ({ 0.0, 1.5, 2.0, 0.0, 2.0, 3.0 }));

    EXPECT_EQ (quad->getMesh(), nullptr);
    EXPECT_EQ (meshOf (*quad), nullptr);
    expectDiagnostic (document, "Mesh3D.indices: the index 1.5 is not a whole number");

    quad->setProperty (indices, numbers ({ 0.0, 1.0, 2.0, 0.0, 2.0, 3.0 }));
    EXPECT_NE (quad->getMesh(), nullptr);
}

TEST_F (SceneMeshComponentTests, AScenePaintedBeforeItsNextFrameDrawsTheMeshItsArraysMake)
{
    YuiDocument document;
    load (document, "Scene3D { id: view; Mesh3D { id: quad; " + quadGeometry() + " } }");

    auto* view = showSceneOf (document);
    auto* quad = document.findById<SceneMeshComponent> ("quad");

    ASSERT_NE (view, nullptr);
    ASSERT_NE (quad, nullptr);

    const auto first = meshOf (*quad);
    ASSERT_NE (first, nullptr);

    StringArray told;

    const DiagnosticListener listener (document, [&told] (const DataTreeDiagnostic& diagnostic)
    {
        told.add (diagnostic.message);
    });

    // What a snapshot does: the scene is painted with no display frame since its arrays changed. Without a theme
    // nothing is written where no GPU draws the scene
    ApplicationTheme::setGlobalTheme (nullptr);

    quad->setProperty (Identifier ("vertices"), numbers ({ 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 2.0, 0.0 }));
    quad->setProperty (Identifier ("indices"), numbers ({ 0.0, 1.0, 2.0 }));
    EXPECT_EQ (meshOf (*quad), first);

    paintWithoutGpu (*view);

    const auto second = meshOf (*quad);
    ASSERT_NE (second, nullptr);
    EXPECT_NE (second, first);
    EXPECT_EQ (second->getPrimitive (0).vertices.size(), 3u);

    // Arrays that do not fit together draw nothing by then, and what is wrong with them is told by the frame that
    // follows: who hears of it may let go of the scene, which a scene being painted could not stand
    quad->setProperty (Identifier ("indices"), numbers ({ 0.0, 1.0, 9.0 }));
    paintWithoutGpu (*view);

    EXPECT_EQ (meshOf (*quad), nullptr);
    EXPECT_TRUE (told.isEmpty()) << told.joinIntoString ("; ");

    view->refreshDisplay (0.0);

    ASSERT_EQ (told.size(), 1);
    EXPECT_EQ (told[0], "Mesh3D.indices: the index 9 is not the one of a vertex, there are 3");
}

TEST_F (SceneMeshComponentTests, AMeshMovedToAnotherParentOfItsSceneKeepsTheMeshItBuilt)
{
    SceneComponent view;
    SceneNodeComponent left, right;
    SceneMeshComponent mesh;

    view.addAndMakeVisible (left);
    view.addAndMakeVisible (right);
    left.addAndMakeVisible (mesh);

    mesh.setProperty (Identifier ("vertices"), numbers ({ 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0 }));
    view.refreshDisplay (0.0);

    const auto built = meshOf (mesh);
    ASSERT_NE (built, nullptr);

    // The entity moves with the component: the mesh on it is not built again
    right.addAndMakeVisible (mesh);

    EXPECT_EQ (mesh.getEntity().getParent(), right.getEntity());
    EXPECT_EQ (meshOf (mesh), built);
    EXPECT_EQ (Internals::getNumChangedMeshes (view), 0);

    view.refreshDisplay (0.0);
    EXPECT_EQ (meshOf (mesh), built);

    // Arrays that change on its way are built by the scene the mesh still belongs to, where it comes back
    right.removeChildComponent (mesh);
    mesh.setProperty (Identifier ("vertices"), numbers ({ 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 2.0, 0.0 }));
    left.addAndMakeVisible (mesh);

    EXPECT_EQ (meshOf (mesh), built);

    view.refreshDisplay (0.0);

    const auto rebuilt = meshOf (mesh);
    ASSERT_NE (rebuilt, nullptr);
    EXPECT_NE (rebuilt, built);
    expectVector (rebuilt->getPrimitive (0).vertices[1].position, 2.0f, 0.0f, 0.0f);
}
