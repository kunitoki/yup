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

#include <yup_3d/yup_3d.h>

#include <gtest/gtest.h>

using namespace yup;

class GeometryBindingsTests : public ::testing::Test
{
protected:
    static SceneDrawList::DrawItem makeItem (const Material& material, bool mirrored = false)
    {
        SceneDrawList::DrawItem item;
        item.material = &material;
        item.mirrored = mirrored;
        item.world = Matrix4::translation ({ 1.0f, 2.0f, 3.0f });
        return item;
    }
};

TEST_F (GeometryBindingsTests, RasterStateCombinesDoubleSidedAndMirrored)
{
    Material singleSided, doubleSided;
    doubleSided.doubleSided = true;

    EXPECT_EQ (GeometryBindings::getRasterState (makeItem (singleSided)), 0u);
    EXPECT_EQ (GeometryBindings::getRasterState (makeItem (singleSided, true)), 1u);
    EXPECT_EQ (GeometryBindings::getRasterState (makeItem (doubleSided)), 2u);
    EXPECT_EQ (GeometryBindings::getRasterState (makeItem (doubleSided, true)), 3u);
    EXPECT_EQ (GeometryBindings::numRasterStates, 4u);
}

TEST_F (GeometryBindingsTests, PipelineGeometryCullsAndWindsPerState)
{
    for (size_t state = 0; state < GeometryBindings::numRasterStates; ++state)
    {
        GpuPipelineOptions options;
        GeometryBindings::setPipelineGeometry (options, state);

        ASSERT_EQ (options.vertexBuffers.size(), 1u);
        EXPECT_EQ (options.vertexBuffers[0].stride, static_cast<uint32_t> (sizeof (Mesh::Vertex)));
        EXPECT_EQ (options.vertexBuffers[0].attributes.size(), 5u);
        EXPECT_EQ (options.indexFormat, GpuIndexFormat::uint32);
        EXPECT_EQ (options.cullMode, (state & 2) != 0 ? GpuCullMode::none : GpuCullMode::back);
        EXPECT_EQ (options.winding, (state & 1) != 0 ? GpuFaceWinding::clockwise : GpuFaceWinding::counterClockwise);
    }
}

TEST_F (GeometryBindingsTests, DrawBlockCarriesTheMaterialIdAndThePreviousTransform)
{
    Material material;
    auto item = makeItem (material);

    // Without a node there is no previous transform: no motion
    auto block = GeometryBindings::makeDrawBlock (item, 7);
    EXPECT_FLOAT_EQ (block.ids[0], 7.0f);
    EXPECT_FLOAT_EQ (block.model[12], 1.0f);
    EXPECT_FLOAT_EQ (block.previousModel[12], 1.0f);

    MeshNode node;
    node.previousWorld = Matrix4::translation ({ 4.0f, 5.0f, 6.0f });
    item.node = &node;

    block = GeometryBindings::makeDrawBlock (item, 0);
    EXPECT_FLOAT_EQ (block.ids[0], 0.0f);
    EXPECT_FLOAT_EQ (block.model[12], 1.0f);
    EXPECT_FLOAT_EQ (block.previousModel[12], 4.0f);
    EXPECT_FLOAT_EQ (block.previousModel[13], 5.0f);
    EXPECT_FLOAT_EQ (block.previousModel[14], 6.0f);
}

TEST_F (GeometryBindingsTests, MaterialBlockPacksTheMaterial)
{
    Material material;
    material.alphaMode = Material::AlphaMode::mask;
    material.alphaCutoff = 0.25f;
    material.roughnessFactor = 0.3f;
    material.specularFactor = 0.5f;
    material.ior = 1.33f;

    const auto block = GeometryBindings::makeMaterialBlock (material, true);
    EXPECT_FLOAT_EQ (block.flags[0], 1.0f);
    EXPECT_FLOAT_EQ (block.flags[3], 1.0f);
    EXPECT_FLOAT_EQ (block.emissiveFactor[3], 0.25f);
    EXPECT_FLOAT_EQ (block.params[1], 0.3f);

    const auto expected = GpuSceneData::packMaterial (material);
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_FLOAT_EQ (block.extension.specular[i], expected.specular[i]);
        EXPECT_FLOAT_EQ (block.extension.extended[i], expected.extended[i]);
    }

    EXPECT_FLOAT_EQ (GeometryBindings::makeMaterialBlock (material, false).flags[3], 0.0f);
}
