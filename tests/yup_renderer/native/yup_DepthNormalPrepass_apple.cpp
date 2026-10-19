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
