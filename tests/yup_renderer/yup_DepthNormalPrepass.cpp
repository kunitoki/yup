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

#if YUP_APPLE
#if YUP_ENABLE_SHADER_TRANSPILER
/** DepthNormalPrepass on its own frame resources, next to the GBufferPass of the same items. */
class DepthNormalPrepassMetalTests : public GBufferPassMetalTests
{
protected:
    void SetUp() override
    {
        GBufferPassMetalTests::SetUp();
        if (IsSkipped())
            return;

        prepass = std::make_unique<DepthNormalPrepass> (device);
        prepassResources = std::make_unique<FrameResources> (device);
    }

    /** Draws the prepass of the items and view of the last GBufferPassMetalTests::render(). Its own
        frame resources keep it from reusing the G-buffer targets it is compared with. */
    bool renderPrepass()
    {
        prepassResources->beginFrame (size, size);

        auto frame = GpuFrame::begin (device);
        const auto drawn = prepass->render (frame, view, drawList, *prepassResources, size, size);
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        prepassResources->endFrame();
        return drawn;
    }

    /** Depth as three 8 bit levels of increasing precision, so equal images mean equal depths to 2^-24. */
    Image readDepth (const GpuTexture::Ptr& depth)
    {
        return mapTexels (depth, "vec4(t.r, fract(t.r * 255.0), fract(t.r * 65025.0), 1.0)");
    }

    std::unique_ptr<DepthNormalPrepass> prepass;
    std::unique_ptr<FrameResources> prepassResources;
};

TEST_F (DepthNormalPrepassMetalTests, IsSupportedOnMetal)
{
    EXPECT_TRUE (DepthNormalPrepass::isSupported (*device));
    EXPECT_TRUE (RenderCapabilities::queryDevice (*device).depthNormalPrepass);
}

TEST_F (DepthNormalPrepassMetalTests, MatchesTheGBufferTexelForTexel)
{
    // A turned quad with its own roughness and metalness, so the normals are not +Z
    auto material = Material::Ptr (new Material());
    material->metallicFactor = 0.7f;
    material->roughnessFactor = 0.5f;

    auto quad = makeQuad (0.0f, 0.6f, material);
    // Turned by 0.45 rad, the normal encodes to 169.03 levels in red, away from a rounding boundary
    quad->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.45f));

    Scene scene;
    scene.getRoot()->addChild (quad);

    const auto gbuffer = render (scene);
    ASSERT_TRUE (renderPrepass());

    const auto normalRoughness = prepass->getNormalRoughness();
    const auto depth = prepass->getDepth();
    ASSERT_NE (normalRoughness, nullptr);
    ASSERT_NE (depth, nullptr);
    ASSERT_NE (normalRoughness, gbuffer.normalRoughness);
    ASSERT_NE (depth, gbuffer.depth);

    EXPECT_EQ (normalRoughness->getFormat(), GpuTextureFormat::rgba16float);
    EXPECT_EQ (depth->getFormat(), GpuTextureFormat::depth32float);
    for (const auto& texture : { normalRoughness, depth })
    {
        EXPECT_EQ (texture->getWidth(), static_cast<uint32_t> (size));
        EXPECT_EQ (texture->getHeight(), static_cast<uint32_t> (size));
        EXPECT_EQ (texture->getSampleCount(), 1u);
    }

    const auto encoded = [&] (const GpuTexture::Ptr& texture)
    {
        return std::pair { mapTexels (texture, "vec4(t.xy * 0.5 + 0.5, t.z, 1.0)"), mapTexels (texture, "vec4(t.w, 0.0, 0.0, 1.0)") };
    };

    const auto [expectedNormal, expectedMetallic] = encoded (gbuffer.normalRoughness);
    const auto [actualNormal, actualMetallic] = encoded (normalRoughness);
    const auto expectedDepth = readDepth (gbuffer.depth);
    const auto actualDepth = readDepth (depth);
    for (const auto* image : { &expectedNormal, &expectedMetallic, &actualNormal, &actualMetallic, &expectedDepth, &actualDepth })
        ASSERT_TRUE (image->isValid());

    // Not vacuous: the quad's normal is far from +Z, and it has a depth
    const auto center = actualNormal.getPixelColor (32, 32);
    EXPECT_GT (std::abs (center.getRed() - 128) + std::abs (center.getGreen() - 128), 32);
    EXPECT_LT (actualDepth.getPixelColor (32, 32).getRed(), 255);
    EXPECT_GT (actualMetallic.getPixelColor (32, 32).getRed(), 128);

    // One level of margin: two 8 bit readouts of the same texture can already differ by one level
    const auto normalLevels = maxChannelDifference (actualNormal, expectedNormal);
    RecordProperty ("normalLevelsApart", normalLevels);
    EXPECT_LE (normalLevels, 1);
    EXPECT_EQ (maxChannelDifference (actualMetallic, expectedMetallic), 0);
    EXPECT_EQ (maxChannelDifference (actualDepth, expectedDepth), 0);
}

TEST_F (DepthNormalPrepassMetalTests, MaskedTexelsAreNotWritten)
{
    // Below and above the default 0.5 cutoff, centered at about x = 23 and x = 41
    Scene scene;
    scene.getRoot()->addChild (makeQuad (-0.6f, 0.4f, makeMasked (0.2f)));
    scene.getRoot()->addChild (makeQuad (0.6f, 0.4f, makeMasked (0.8f)));

    render (scene);
    ASSERT_TRUE (renderPrepass());

    const auto roughness = mapTexels (prepass->getNormalRoughness(), "vec4(t.z, 0.0, 0.0, 1.0)");
    const auto cleared = mapTexels (prepass->getDepth(), "vec4(t.r >= 1.0 ? 1.0 : 0.0, 0.0, 0.0, 1.0)");
    ASSERT_TRUE (roughness.isValid());
    ASSERT_TRUE (cleared.isValid());

    EXPECT_GT (roughness.getPixelColor (41, 32).getRed(), 64);
    EXPECT_EQ (cleared.getPixelColor (41, 32).getRed(), 0);

    EXPECT_EQ (roughness.getPixelColor (23, 32).getRed(), 0);
    EXPECT_EQ (cleared.getPixelColor (23, 32).getRed(), 255);
}

TEST_F (DepthNormalPrepassMetalTests, TargetsThatCannotBeCreatedLeaveTheFrameWithoutIt)
{
    Scene scene;
    scene.getRoot()->addChild (makeQuad (0.0f, 0.5f, Material::Ptr (new Material())));
    render (scene);

    // Not an error for the frame: the pass draws nothing and has no targets, as long as they fail
    prepass->failTargetsForTesting();
    for (int frame = 0; frame < 2; ++frame)
    {
        EXPECT_TRUE (renderPrepass()) << frame;
        EXPECT_EQ (prepass->getNormalRoughness(), nullptr) << frame;
        EXPECT_EQ (prepass->getDepth(), nullptr) << frame;
    }
}
#endif
#endif
