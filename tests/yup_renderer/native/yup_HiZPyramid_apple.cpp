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

#include <gtest/gtest.h>

#include <yup_renderer/yup_renderer.h>

using namespace yup;

#if YUP_ENABLE_SHADER_TRANSPILER
/** HiZPyramid over the G-buffer depth of the compact scene, checked by fullscreen passes writing
    255 where a texel holds and 0 where it does not. */
class HiZPyramidMetalTests : public SceneRendererMetalTests
{
protected:
    void SetUp() override
    {
        SceneRendererMetalTests::SetUp();
        if (IsSkipped())
            return;

        frameResources = std::make_unique<FrameResources> (device);
        pyramid = std::make_unique<HiZPyramid> (device);
    }

    struct Built
    {
        GpuTexture::Ptr depth;
        GpuTexture::Ptr pyramid;
    };

    /** Draws the compact scene, seen from above and in front, into the depth of a G-buffer of
        @p width x @p height, and builds the pyramid of that depth in the same frame. The ground
        fills the bottom rows, nearest at the bottom, and the background the top ones. */
    Built renderAndBuild (int width, int height)
    {
        Scene scene;
        addCompactScene (scene);

        auto camera = scene.createEntity ("camera");
        camera.attach<CameraNode>();
        aimCamera (camera, { 0.0f, 1.2f, 1.8f }, { 0.0f, 0.2f, 0.0f });
        scene.setActiveCamera (camera);

        frameResources->beginFrame (width, height);

        const auto acquire = [&] (GpuTextureFormat format)
        {
            return frameResources->acquire (GpuTextureDesc (static_cast<uint32_t> (width), static_cast<uint32_t> (height), format, true));
        };

        const auto targets = GBufferPass::acquireTargets (*frameResources, acquire (GpuTextureFormat::rg16float), acquire (GpuTextureFormat::depth32float));

        view.update (scene, SceneSettings::resolve (scene), width, height, false, 1, 0);
        drawList.build (scene.getRoot(), view.getView());
        EXPECT_TRUE (gbufferPass.prepare (device).wasOk());

        Built built;
        auto frame = GpuFrame::begin (device);
        EXPECT_TRUE (gbufferPass.render (frame, view, drawList.getOpaqueItems(), materials, targets));
        built.depth = targets.depth;
        built.pyramid = pyramid->build (frame, targets.depth, *frameResources);
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        pyramid->trimUnusedTargets();
        frameResources->endFrame();
        drawList.clear();
        return built;
    }

    /** Draws @p fragment into an rgba8 target of @p width x @p height, with a nearest sampler at
        set 0 binding 1 and the textures @p bind sets, and returns its rgba texels. */
    std::vector<uint8> runChecker (const String& fragment, int width, int height, const std::function<void (GpuRenderPass&)>& bind)
    {
        const auto vertex = File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory().getParentDirectory()
                                .getChildFile ("modules/yup_renderer/shaders/yup_Fullscreen.vert")
                                .loadFileAsString();

        ShaderBundleCompileRequest request;
        request.sourceLanguage = ShaderLanguage::glsl;

        for (const auto& [stage, code] : { std::pair { ShaderStage::vertex, vertex }, std::pair { ShaderStage::fragment, fragment } })
        {
            ShaderBundleEntry entry;
            entry.stage = stage;
            entry.source = code;
            entry.targetLanguages = { ShaderLanguage::msl };
            request.entries.push_back (entry);
        }

        ShaderBundleCompiler compiler;
        auto bundle = compiler.compile (request);
        MemoryBlock block;
        if (! bundle.wasOk() || ! bundle.getReference().saveToMemoryBlock (block).wasOk())
        {
            ADD_FAILURE() << bundle.getErrorMessage();
            return {};
        }

        auto target = GpuTarget::create (device, width, height);
        auto sampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::nearest, GpuWrapMode::clampToEdge));
        if (target == nullptr || sampler == nullptr)
            return {};

        FullscreenPass pass;
        if (! pass.prepare (device, static_cast<const uint8*> (block.getData()), block.getSize(), target->asTexture()->getFormat()).wasOk())
            return {};

        auto frame = GpuFrame::begin (device);
        EXPECT_TRUE (pass.draw (frame, target->asTexture(), {}, [&] (GpuRenderPass& renderPass)
        {
            renderPass.setSampler (0, 1, sampler);
            bind (renderPass);
        }, GpuColor::transparentBlack()));
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        std::vector<uint8> texels (static_cast<size_t> (width * height * 4));
        if (! target->readPixels (texels.data(), texels.size()))
            return {};

        return texels;
    }

    /** How many texels of @p texels have @p channel not at 255. */
    static int countFailing (const std::vector<uint8>& texels, int channel)
    {
        int failing = 0;
        for (size_t i = static_cast<size_t> (channel); i < texels.size(); i += 4)
            failing += texels[i] != 255 ? 1 : 0;

        return failing;
    }

    std::unique_ptr<FrameResources> frameResources;
    std::unique_ptr<HiZPyramid> pyramid;
    ViewData view;
    SceneDrawList drawList;
    GBufferPass gbufferPass;
    MaterialTable materials;
};

TEST_F (HiZPyramidMetalTests, HiZMipsKeepTheClosestDepth)
{
    // Mip n (lod 1 of a view starting at mip n - 1, since a draw binds a texture through one view
    // only) against the texels of mip n - 1 it covers, found from the sizes alone: 3 per axis
    // where mip n - 1 is odd. Red: not farther than any of them. Green: exactly the closest of them.
    const String checker = R"(#version 450
layout(set = 0, binding = 0) uniform texture2D pyramid;
layout(set = 0, binding = 1) uniform sampler nearest;
layout(location = 0) out vec4 color;
void main()
{
    ivec2 texel = ivec2(gl_FragCoord.xy);
    ivec2 upperSize = textureSize(sampler2D(pyramid, nearest), 0);
    ivec2 lowerSize = textureSize(sampler2D(pyramid, nearest), 1);
    ivec2 first = (texel * upperSize) / lowerSize;
    ivec2 last = ((texel + 1) * upperSize + lowerSize - 1) / lowerSize - 1;

    float closest = 2.0;
    for (int y = first.y; y <= last.y; ++y)
        for (int x = first.x; x <= last.x; ++x)
            closest = min(closest, texelFetch(sampler2D(pyramid, nearest), ivec2(x, y), 0).r);

    float value = texelFetch(sampler2D(pyramid, nearest), texel, 1).r;
    color = vec4(value <= closest ? 1.0 : 0.0, value == closest ? 1.0 : 0.0, 0.0, 1.0);
})";

    // Odd extents at most mips of the first (37 x 23 down to 2 x 1 and 1 x 1), and of the second from mip 1
    for (const auto& [width, height] : { std::pair { 37, 23 }, std::pair { 40, 30 } })
    {
        SCOPED_TRACE (String (width) + " x " + String (height));

        const auto built = renderAndBuild (width, height);
        ASSERT_NE (built.pyramid, nullptr);
        EXPECT_EQ (built.pyramid->getFormat(), GpuTextureFormat::r32float);
        EXPECT_EQ (built.pyramid->getWidth(), width);
        EXPECT_EQ (built.pyramid->getHeight(), height);
        EXPECT_EQ (built.pyramid->getMipLevels(), 6u);

        for (uint32_t mip = 1; mip < built.pyramid->getMipLevels(); ++mip)
        {
            GpuTextureViewDesc twoMips (mip - 1, 0);
            twoMips.mipCount = 2;

            const auto texels = runChecker (checker, jmax (1, width >> mip), jmax (1, height >> mip), [&] (GpuRenderPass& pass)
            {
                pass.setTexture (0, 0, built.pyramid, twoMips);
            });
            ASSERT_FALSE (texels.empty()) << mip;
            EXPECT_EQ (countFailing (texels, 0), 0) << "farther than a covered texel at mip " << mip;
            EXPECT_EQ (countFailing (texels, 1), 0) << "not the closest covered texel at mip " << mip;
        }
    }
}

TEST_F (HiZPyramidMetalTests, MipZeroCopiesTheDepth)
{
    // Red: the pyramid is within 1e-6 of the depth. Green: something was drawn there.
    const String checker = R"(#version 450
layout(set = 0, binding = 0) uniform texture2D depth;
layout(set = 0, binding = 1) uniform sampler nearest;
layout(set = 0, binding = 2) uniform texture2D pyramid;
layout(location = 0) out vec4 color;
void main()
{
    ivec2 texel = ivec2(gl_FragCoord.xy);
    float d = texelFetch(sampler2D(depth, nearest), texel, 0).r;
    float value = texelFetch(sampler2D(pyramid, nearest), texel, 0).r;
    color = vec4(abs(value - d) <= 1e-6 ? 1.0 : 0.0, d < 1.0 ? 1.0 : 0.0, 0.0, 1.0);
})";

    const auto built = renderAndBuild (37, 23);
    ASSERT_NE (built.pyramid, nullptr);

    const auto texels = runChecker (checker, 37, 23, [&] (GpuRenderPass& pass)
    {
        pass.setTexture (0, 0, built.depth);
        pass.setTexture (0, 2, built.pyramid, GpuTextureViewDesc (0, 0));
    });
    ASSERT_EQ (texels.size(), static_cast<size_t> (37 * 23 * 4));
    EXPECT_EQ (countFailing (texels, 0), 0);

    // Neither empty nor full: the scene draws the ground and the box over the background
    const auto background = countFailing (texels, 1);
    EXPECT_GT (background, 0);
    EXPECT_LT (background, 37 * 23);
}

TEST_F (HiZPyramidMetalTests, ReturnsNullWithoutASingleSampledDepth)
{
    frameResources->beginFrame (8, 8);

    auto color = frameResources->acquire (GpuTextureDesc (8, 8, GpuTextureFormat::rgba8unorm, true));
    ASSERT_NE (color, nullptr);

    GpuTextureDesc multisampledDesc (8, 8, GpuTextureFormat::depth32float, true);
    multisampledDesc.sampleCount = 4;
    auto multisampled = frameResources->acquire (multisampledDesc);
    ASSERT_NE (multisampled, nullptr);

    auto frame = GpuFrame::begin (device);
    EXPECT_EQ (pyramid->build (frame, nullptr, *frameResources), nullptr);
    EXPECT_EQ (pyramid->build (frame, color, *frameResources), nullptr);
    EXPECT_EQ (pyramid->build (frame, multisampled, *frameResources), nullptr);
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    frameResources->endFrame();
    EXPECT_EQ (pyramid->getNumCachedTargets(), 0u);
}

TEST_F (HiZPyramidMetalTests, ReusesThePooledPyramidAndOneTargetPerMip)
{
    const auto first = renderAndBuild (37, 23).pyramid;
    const auto second = renderAndBuild (37, 23).pyramid;
    ASSERT_NE (first, nullptr);
    EXPECT_EQ (first, second);
    EXPECT_EQ (pyramid->getNumCachedTargets(), static_cast<size_t> (first->getMipLevels()));

    pyramid->releaseTargets();
    EXPECT_EQ (pyramid->getNumCachedTargets(), 0u);
}
#endif
