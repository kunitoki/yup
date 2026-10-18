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

TEST (TemporalAABlendWeightTests, StillFramesTakeTheTrueRunningMean)
{
    // Frame n of a still accumulation weighs 1 / (n + 1): once sampleCount frames are in, each of
    // them holds the same share of the history, so rotating noise averages out evenly
    for (int sampleCount : { 4, 16, 64 })
    {
        for (int index = 0; index < sampleCount; ++index)
            EXPECT_FLOAT_EQ (TemporalAA::getBlendWeight (index, sampleCount, false), 1.0f / static_cast<float> (index + 1)) << index << " of " << sampleCount;

        EXPECT_EQ (TemporalAA::getBlendWeight (sampleCount, sampleCount, false), 0.0f);
        EXPECT_EQ (TemporalAA::getBlendWeight (sampleCount + 5, sampleCount, false), 0.0f);
    }
}

TEST (TemporalAABlendWeightTests, ChangingFramesKeepAtLeastATenth)
{
    // While the scene changes, the current frame keeps at least a tenth, so the history fades
    // within a few frames behind moving content; the start of the ramp is the same running mean
    for (int index = 0; index < 16; ++index)
        EXPECT_FLOAT_EQ (TemporalAA::getBlendWeight (index, 16, true), jmax (1.0f / static_cast<float> (index + 1), 0.1f)) << index;

    EXPECT_EQ (TemporalAA::getBlendWeight (16, 16, true), 0.0f);
}

#if YUP_APPLE && YUP_ENABLE_SHADER_TRANSPILER
// Reuses the Metal fixture of yup_SceneRenderer.cpp, included before this file by module.cpp
class TemporalAATests : public SceneRendererMetalTests
{
protected:
    static constexpr int size = 4;

    /** A size x size rgba16float texture of one half-float value in rgb and another in alpha. */
    GpuTexture::Ptr createHalfTexture (uint16 rgb, uint16 alpha)
    {
        return createCheckerTexture (rgb, alpha, rgb, alpha);
    }

    /** A size x size rgba16float checkerboard of half-float gray levels and alphas: the even
        texels, where x + y is even, then the odd ones. */
    GpuTexture::Ptr createCheckerTexture (uint16 evenRgb, uint16 evenAlpha, uint16 oddRgb, uint16 oddAlpha)
    {
        std::vector<uint16> texels;
        for (int i = 0; i < size * size; ++i)
        {
            const auto even = (i % size + i / size) % 2 == 0;
            const auto rgb = even ? evenRgb : oddRgb;
            texels.insert (texels.end(), { rgb, rgb, rgb, even ? evenAlpha : oddAlpha });
        }

        auto texture = GpuTexture::create (device, GpuTextureDesc (size, size, GpuTextureFormat::rgba16float));
        if (texture == nullptr)
            return nullptr;

        GpuTextureDataDesc data;
        data.data = texels.data();
        return texture->upload (data) ? texture : nullptr;
    }
};

TEST_F (TemporalAATests, RejectsNonFiniteHistory)
{
    const GpuTextureDesc desc (size, size, GpuTextureFormat::rgba16float, true);

    // A NaN history, valid as far as FrameResources knows
    FrameResources resources (device);
    resources.beginFrame (size, size);
    const auto history = resources.getHistory (TemporalAA::historyName, desc);
    ASSERT_TRUE (drawFullscreen (R"(#version 450
layout(location = 0) out vec4 color;
void main() { color = vec4(uintBitsToFloat(0x7fc00000u)); })",
                                 nullptr, history.write));
    resources.endFrame();
    ASSERT_FALSE (isFiniteTexture (history.write));

    // A finite 0.5 gray frame, opaque, with no depth and no motion
    const auto current = createHalfTexture (0x3800, 0x3c00);
    const auto zeros = createHalfTexture (0, 0);
    const auto output = GpuTexture::create (device, desc);
    ASSERT_NE (current, nullptr);
    ASSERT_NE (zeros, nullptr);
    ASSERT_NE (output, nullptr);

    Scene scene;
    ViewData view;
    view.update (scene, size, size, false, 1, 0);

    TemporalAA temporalAA;
    resources.beginFrame (size, size);
    ASSERT_TRUE (resources.getHistory (TemporalAA::historyName, desc).valid);

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (temporalAA.render (device, frame, view, resources, current, zeros, zeros, 1, 16, output));
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());
    resources.endFrame();

    // The history is ignored: the output is the current frame, not a blend with a laundered NaN
    EXPECT_TRUE (isFiniteTexture (output));

    const auto image = runFullscreen (R"(#version 450
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(location = 0) out vec4 color;
void main() { color = vec4(texelFetch(sampler2D(source, sourceSampler), ivec2(gl_FragCoord.xy), 0).rgb, 1.0); })",
                                      output);
    ASSERT_TRUE (image.isValid());
    EXPECT_NEAR (image.getPixelColor (1, 1).getRed(), 128, 2);
}

TEST_F (TemporalAATests, BlendWeighsColorsByLumaAndKeepsCoverageLinear)
{
    const GpuTextureDesc desc (size, size, GpuTextureFormat::rgba16float, true);

    // Blends a uniform history into a checkerboard current frame with weight 1/2 (sample index 1),
    // and reads texel (1, 1), an even one: red holds rgb / 8, green the alpha
    const auto resolve = [&] (const char* history, const GpuTexture::Ptr& current)
    {
        FrameResources resources (device);
        resources.beginFrame (size, size);
        const auto pair = resources.getHistory (TemporalAA::historyName, desc);
        EXPECT_TRUE (drawFullscreen (String ("#version 450\nlayout(location = 0) out vec4 color;\nvoid main() { color = ") + history + "; }", nullptr, pair.write));
        resources.endFrame();

        const auto zeros = createHalfTexture (0, 0);
        const auto output = GpuTexture::create (device, desc);

        Scene scene;
        ViewData view;
        view.update (scene, size, size, false, 1, 0);

        TemporalAA temporalAA;
        resources.beginFrame (size, size);
        auto frame = GpuFrame::begin (device);
        EXPECT_TRUE (temporalAA.render (device, frame, view, resources, current, zeros, zeros, 1, 16, output));
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());
        resources.endFrame();

        const auto image = runFullscreen (R"(#version 450
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(location = 0) out vec4 color;
void main()
{
    vec4 c = texelFetch(sampler2D(source, sourceSampler), ivec2(gl_FragCoord.xy), 0);
    color = vec4(c.r / 8.0, c.a, 0.0, 1.0);
})",
                                          output);
        return image.isValid() ? image.getPixelColor (1, 1) : Color();
    };

    // Opaque 0.2 at even texels and 10 at odd ones, over an opaque history of 10. Weighting each
    // color by 1 / (1 + luma) gives (0.5 * 10 / 11 + 0.5 * 0.2 / 1.2) / (0.5 / 11 + 0.5 / 1.2) = 1.164,
    // 37 once divided by 8, where a plain average would give 5.1, 163
    const auto weighted = resolve ("vec4(10.0, 10.0, 10.0, 1.0)", createCheckerTexture (0x3266, 0x3c00, 0x4900, 0x3c00));
    RecordProperty ("weighted", static_cast<int> (weighted.getRed()));
    EXPECT_NEAR (weighted.getRed(), 37, 3);
    EXPECT_NEAR (weighted.getGreen(), 255, 1);

    // 10 covering the even texels and nothing the odd ones, over an uncovered history: the same
    // color at half the coverage, 5 at alpha 1/2, like a plain average of the premultiplied colors
    const auto covered = resolve ("vec4(0.0)", createCheckerTexture (0x4900, 0x3c00, 0, 0));
    RecordProperty ("coveredRed", static_cast<int> (covered.getRed()));
    RecordProperty ("coveredAlpha", static_cast<int> (covered.getGreen()));
    EXPECT_NEAR (covered.getRed(), 159, 3);
    EXPECT_NEAR (covered.getGreen(), 128, 2);
}
#endif
