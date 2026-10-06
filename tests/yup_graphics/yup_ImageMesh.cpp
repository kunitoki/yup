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


#include <yup_graphics/yup_graphics.h>

#include <gtest/gtest.h>

using namespace yup;

TEST (ImageMeshTests, DefaultMeshIsEmptyAndInvalid)
{
    const ImageMesh mesh;

    EXPECT_TRUE (mesh.getVertices().empty());
    EXPECT_TRUE (mesh.getIndices().empty());
    EXPECT_FALSE (mesh.isValid());
}

TEST (ImageMeshTests, GridHasOneVertexPerCornerAndTwoTrianglesPerCell)
{
    const auto mesh = ImageMesh::createGrid ({ 10.0f, 20.0f, 100.0f, 50.0f }, 4, 2);

    ASSERT_EQ (mesh.getVertices().size(), 15u);
    EXPECT_EQ (mesh.getTextureCoordinates().size(), 15u);
    EXPECT_EQ (mesh.getIndices().size(), 48u);
    EXPECT_TRUE (mesh.isValid());

    EXPECT_EQ (mesh.getVertices()[0], Point<float> (10.0f, 20.0f));
    EXPECT_EQ (mesh.getVertices()[14], Point<float> (110.0f, 70.0f));
    EXPECT_EQ (mesh.getTextureCoordinates()[0], Point<float> (0.0f, 0.0f));
    EXPECT_EQ (mesh.getTextureCoordinates()[14], Point<float> (1.0f, 1.0f));
}

TEST (ImageMeshTests, GridHasAtLeastOneCell)
{
    const auto mesh = ImageMesh::createGrid ({ 0.0f, 0.0f, 10.0f, 10.0f }, 0, -3);

    EXPECT_EQ (mesh.getVertices().size(), 4u);
    EXPECT_EQ (mesh.getIndices().size(), 6u);
}

TEST (ImageMeshTests, SetVertexMovesOnlyThatVertex)
{
    auto mesh = ImageMesh::createGrid ({ 0.0f, 0.0f, 10.0f, 10.0f }, 1, 1);
    const std::vector<Point<float>> before (mesh.getVertices().begin(), mesh.getVertices().end());

    mesh.setVertex (3, { 20.0f, 30.0f });
    mesh.setVertex (99, { 1.0f, 1.0f });

    EXPECT_EQ (mesh.getVertices()[3], Point<float> (20.0f, 30.0f));
    for (int i = 0; i < 3; ++i)
        EXPECT_EQ (mesh.getVertices()[static_cast<std::size_t> (i)], before[static_cast<std::size_t> (i)]);
}

TEST (ImageMeshTests, SetVerticesNeedsTheSameCount)
{
    auto mesh = ImageMesh::createGrid ({ 0.0f, 0.0f, 10.0f, 10.0f }, 1, 1);
    const std::vector<Point<float>> moved { { 1.0f, 1.0f }, { 2.0f, 2.0f }, { 3.0f, 3.0f }, { 4.0f, 4.0f } };
    const std::vector<Point<float>> tooFew { { 1.0f, 1.0f } };

    EXPECT_TRUE (mesh.setVertices (moved));
    EXPECT_EQ (mesh.getVertices()[2], Point<float> (3.0f, 3.0f));

    EXPECT_FALSE (mesh.setVertices (tooFew));
    EXPECT_EQ (mesh.getVertices().size(), 4u);
}

TEST (ImageMeshTests, MismatchedOrOutOfRangeDataIsInvalid)
{
    const std::vector<Point<float>> triangle { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f } };

    EXPECT_TRUE (ImageMesh (triangle, triangle, { 0, 1, 2 }).isValid());
    EXPECT_FALSE (ImageMesh (triangle, { { 0.0f, 0.0f } }, { 0, 1, 2 }).isValid());
    EXPECT_FALSE (ImageMesh (triangle, triangle, { 0, 1 }).isValid());
    EXPECT_FALSE (ImageMesh (triangle, triangle, { 0, 1, 3 }).isValid());
}

TEST (ImageMeshTests, GridIsLimitedToSixteenBitIndices)
{
    const auto mesh = ImageMesh::createGrid ({ 0.0f, 0.0f, 10.0f, 10.0f }, 1000, 1000);

    EXPECT_LE (mesh.getVertices().size(), 65536u);
    EXPECT_TRUE (mesh.isValid());
}

TEST (ImageMeshTests, SetVerticesWithItsOwnVerticesKeepsThem)
{
    auto mesh = ImageMesh::createGrid ({ 0.0f, 0.0f, 10.0f, 10.0f }, 1, 1);
    const auto expected = std::vector<Point<float>> (mesh.getVertices().begin(), mesh.getVertices().end());

    EXPECT_TRUE (mesh.setVertices (mesh.getVertices()));
    EXPECT_EQ (expected, std::vector<Point<float>> (mesh.getVertices().begin(), mesh.getVertices().end()));
}

TEST (ImageMeshTests, CopiesAndMovesKeepTheData)
{
    auto mesh = ImageMesh::createGrid ({ 0.0f, 0.0f, 10.0f, 10.0f }, 2, 2);

    ImageMesh copy;
    copy = mesh;
    EXPECT_EQ (copy.getVertices().size(), mesh.getVertices().size());

    ImageMesh moved;
    moved = std::move (copy);
    EXPECT_EQ (moved.getVertices().size(), 9u);
    EXPECT_TRUE (moved.isValid());
}
