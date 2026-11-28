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

#include <algorithm>
#include <map>
#include <set>
#include <utility>

using namespace yup;

/** CPU only: the reflection of the precompiled prepass bundle against the G-buffer one it mirrors.
    The GPU tests live with the SceneRenderer ones (DepthNormalPrepassMetalTests). */
class DepthNormalPrepassTests : public ::testing::Test
{
protected:
    static ShaderBundle load (const uint8* data, size_t size)
    {
        auto bundle = ShaderBundle::loadFromData (data, size);
        EXPECT_TRUE (bundle.wasOk()) << bundle.getErrorMessage();
        return bundle.wasOk() ? std::move (bundle.getReference()) : ShaderBundle();
    }

    static ShaderBundle prepass()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_DepthNormalPrepass.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle gbufferPass()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_GBufferPass_tier_full.inc"
        };
        return load (data, sizeof (data));
    }

    static const ShaderReflection* reflectionOf (const ShaderBundle& bundle, ShaderStage stage)
    {
        const auto* shader = bundle.findShader (stage, ShaderLanguage::spirv);
        return shader != nullptr ? &shader->reflection : nullptr;
    }

    static const ShaderReflection::ResourceBinding* findBlock (const ShaderReflection& reflection, uint32_t set, uint32_t binding)
    {
        const auto& blocks = reflection.uniformBuffers;
        const auto block = std::find_if (blocks.begin(), blocks.end(), [&] (const auto& b) { return b.set == set && b.binding == binding; });
        return block != blocks.end() ? &*block : nullptr;
    }

    static std::map<String, uint32_t> memberOffsets (const ShaderReflection::ResourceBinding& block)
    {
        std::map<String, uint32_t> offsets;
        for (const auto& member : block.members)
            offsets[member.name] = member.offset;

        return offsets;
    }

    /** The (set, binding) of every texture and sampler. */
    static std::set<std::pair<uint32_t, uint32_t>> textureAndSamplerBindings (const ShaderReflection& reflection)
    {
        std::set<std::pair<uint32_t, uint32_t>> bindings;
        for (const auto* resources : { &reflection.separateImages, &reflection.separateSamplers })
            for (const auto& resource : *resources)
                bindings.insert ({ resource.set, resource.binding });

        return bindings;
    }
};

TEST_F (DepthNormalPrepassTests, WritesOnlyTheNormalTarget)
{
    const auto bundle = prepass();
    const auto* fragment = reflectionOf (bundle, ShaderStage::fragment);
    ASSERT_NE (fragment, nullptr);

    std::set<uint32_t> locations;
    for (const auto& output : fragment->stageOutputs)
        locations.insert (output.location);

    EXPECT_EQ (locations, (std::set<uint32_t> { 0 }));
}

TEST_F (DepthNormalPrepassTests, SharesTheGeometryAndMaterialBindingsOfTheGBufferPass)
{
    const auto prepassBundle = prepass();
    const auto gbufferBundle = gbufferPass();

    // The material textures it reads are the G-buffer's: base color (mask), metallic-roughness and
    // normal at set 2 bindings 2-4, with their samplers at 7-9. Unread ones may be compiled out
    for (const auto stage : { ShaderStage::vertex, ShaderStage::fragment })
    {
        const auto* prepassReflection = reflectionOf (prepassBundle, stage);
        const auto* gbufferReflection = reflectionOf (gbufferBundle, stage);
        ASSERT_NE (prepassReflection, nullptr) << toString (stage);
        ASSERT_NE (gbufferReflection, nullptr) << toString (stage);

        const auto read = textureAndSamplerBindings (*prepassReflection);
        const auto gbuffer = textureAndSamplerBindings (*gbufferReflection);
        EXPECT_TRUE (std::includes (gbuffer.begin(), gbuffer.end(), read.begin(), read.end())) << toString (stage);

        if (stage == ShaderStage::fragment)
            for (const uint32_t binding : { 2u, 3u, 4u, 7u, 8u, 9u })
                EXPECT_EQ (read.count ({ 2u, binding }), 1u) << binding;
    }

    const auto* draw = findBlock (*reflectionOf (prepassBundle, ShaderStage::vertex), 2, 0);
    ASSERT_NE (draw, nullptr);
    EXPECT_EQ (draw->blockSize, static_cast<uint32_t> (sizeof (GeometryBindings::DrawBlock)));

    const auto* material = findBlock (*reflectionOf (prepassBundle, ShaderStage::fragment), 2, 1);
    const auto* gbufferMaterial = findBlock (*reflectionOf (gbufferBundle, ShaderStage::fragment), 2, 1);
    ASSERT_NE (material, nullptr);
    ASSERT_NE (gbufferMaterial, nullptr);
    // Without the clearcoat and sheen rows, which only the shaders compiling the layers read
    EXPECT_EQ (material->blockSize, static_cast<uint32_t> (offsetof (GeometryBindings::MaterialBlock, extension) + offsetof (GpuMaterial, clearcoat)));
    EXPECT_EQ (memberOffsets (*material), memberOffsets (*gbufferMaterial));
}

TEST_F (DepthNormalPrepassTests, ReadsOnlyTheViewAndTheDraw)
{
    const auto bundle = prepass();

    for (const auto stage : { ShaderStage::vertex, ShaderStage::fragment })
    {
        const auto* reflection = reflectionOf (bundle, stage);
        ASSERT_NE (reflection, nullptr) << toString (stage);

        for (const auto& block : reflection->uniformBuffers)
            EXPECT_TRUE (block.set == 0 || block.set == 2) << toString (stage) << " set " << (int) block.set;
    }
}

/** DepthNormalPrepass recording on a mocked device. What it draws is tested on Metal, in
    native/yup_DepthNormalPrepass_apple.cpp. */
class DepthNormalPrepassMockTests : public RendererMockTests
{
protected:
    void SetUp() override
    {
        RendererMockTests::SetUp();
        resources = std::make_unique<FrameResources> (device);
        mesh->addPrimitive ({ { { 0.0f, 1.0f, 0.0f } }, { { -1.0f, -1.0f, 0.0f } }, { { 1.0f, -1.0f, 0.0f } } }, { 0, 1, 2 });
    }

    /** Adds the triangle at @p z on the axis of a camera standing at the origin and looking down -Z. */
    void addTriangle (float z, Material::Ptr material = nullptr)
    {
        auto entity = scene.createEntity();
        entity.getOrAttach<TransformNode>().setPosition ({ 0.0f, 0.0f, z });
        entity.attach<MeshNode> (mesh);

        if (material != nullptr)
            entity.attach<MaterialNode> (std::move (material));
    }

    bool renderFrame (DepthNormalPrepass& pass)
    {
        resources->beginFrame (64, 64);

        auto frame = GpuFrame::begin (device);
        const auto rendered = pass.render (frame, view, drawList, *resources, 64, 64);
        frame.submit();

        resources->endFrame();
        return rendered;
    }

    Scene scene;
    SceneDrawList drawList;
    ViewData view;
    std::unique_ptr<FrameResources> resources;
    Mesh::Ptr mesh { new Mesh ("triangle") };
};

TEST_F (DepthNormalPrepassMockTests, DrawsTheOpaqueItemsInView)
{
    auto glass = Material::Ptr (new Material());
    glass->alphaMode = Material::AlphaMode::blend;

    addTriangle (-5.0f);
    addTriangle (-6.0f);
    addTriangle (5.0f);
    addTriangle (-7.0f, glass);

    DepthNormalPrepass pass (device);

    // Without the projection of the camera every opaque item is in view, the one behind it too
    drawList.build (scene.getRoot(), {});
    ASSERT_EQ (drawList.getOpaqueItems().size(), 3u);

    ASSERT_TRUE (renderFrame (pass));
    ASSERT_EQ (passes.size(), 1u);
    EXPECT_EQ (passes[0].draws, 3);
    EXPECT_NE (pass.getDepth(), nullptr);

    SceneDrawList::BuildOptions options;
    options.projection = Matrix4::perspective (MathConstants<float>::halfPi, 1.0f, 0.1f, 100.0f);
    drawList.build (scene.getRoot(), {}, options);
    ASSERT_EQ (drawList.getOpaqueItems().size(), 3u);

    ASSERT_TRUE (renderFrame (pass));
    ASSERT_EQ (passes.size(), 2u);
    EXPECT_EQ (passes[1].draws, 2);
    EXPECT_NE (pass.getDepth(), nullptr);
}
