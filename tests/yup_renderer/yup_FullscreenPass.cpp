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

class FullscreenPassTests : public ::testing::Test
{
};

TEST_F (FullscreenPassTests, IsNotPreparedUntilCompiled)
{
    FullscreenPass pass;
    EXPECT_FALSE (pass.isPrepared());

    const uint8 garbage[] = { 1, 2, 3, 4 };
    EXPECT_TRUE (pass.prepare (nullptr, garbage, sizeof (garbage), GpuTextureFormat::rgba8unorm).failed());
    EXPECT_FALSE (pass.isPrepared());
}

#if YUP_APPLE && YUP_ENABLE_SHADER_TRANSPILER
class FullscreenPassMetalTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr || ! device->isGpuAvailable())
            GTEST_SKIP() << "No Metal device available";

        context = GraphicsContext::createContext (GpuPlatform::Metal, {}, device);
        if (context == nullptr || ! context->isGpuAvailable())
            GTEST_SKIP() << "No Metal graphics context available";
    }

    /** Bundles shaders/yup_Fullscreen.vert with @p fragment, as the module build does. */
    static MemoryBlock compileBundle (const String& fragment)
    {
        const auto vertex = File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory()
                                .getChildFile ("modules/yup_renderer/shaders/yup_Fullscreen.vert")
                                .loadFileAsString();
        EXPECT_FALSE (vertex.isEmpty());

        ShaderBundleCompileRequest request;
        request.sourceLanguage = ShaderLanguage::glsl;

        for (const auto& [stage, source] : { std::pair { ShaderStage::vertex, vertex }, std::pair { ShaderStage::fragment, fragment } })
        {
            ShaderBundleEntry entry;
            entry.stage = stage;
            entry.source = source;
            entry.targetLanguages = { ShaderLanguage::msl };
            request.entries.push_back (entry);
        }

        ShaderBundleCompiler compiler;
        auto bundle = compiler.compile (request);
        EXPECT_TRUE (bundle.wasOk()) << bundle.getErrorMessage();

        MemoryBlock block;
        if (bundle.wasOk())
            EXPECT_TRUE (bundle.getReference().saveToMemoryBlock (block).wasOk());

        return block;
    }

    /** A canvas-backed render target, like the one SceneRenderer returns. */
    GpuTexture::Ptr createTarget()
    {
        targets.push_back (GpuTarget::create (device, size, size));
        return targets.back() != nullptr ? targets.back()->asTexture() : nullptr;
    }

    /** Draws @p texture through a canvas and reads it back, the way SceneRenderer tests read frames:
        row 0 of the image is the top of the frame. */
    Image readBack (const GpuTexture::Ptr& texture)
    {
        auto canvas = GpuCanvas::create (*context, texture->getWidth(), texture->getHeight());
        if (canvas == nullptr)
            return {};

        auto& g = canvas->beginDraw();
        g.drawTexture (texture, { 0.0f, 0.0f, static_cast<float> (texture->getWidth()), static_cast<float> (texture->getHeight()) });
        return canvas->asImage();
    }

    static constexpr int size = 8;

    GpuDevice::Ptr device;
    std::unique_ptr<GraphicsContext> context;
    std::vector<GpuTarget::Ptr> targets;
};

TEST_F (FullscreenPassMetalTests, UvStartsAtTheTopLeftTexel)
{
    const auto bundle = compileBundle (R"(#version 450
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main() { color = vec4(uv, 0.0, 1.0); })");

    FullscreenPass pass;
    const auto prepared = pass.prepare (device, static_cast<const uint8*> (bundle.getData()), bundle.getSize(), GpuTextureFormat::rgba8unorm);
    ASSERT_TRUE (prepared.wasOk()) << prepared.getErrorMessage();
    EXPECT_TRUE (pass.isPrepared());

    auto target = createTarget();
    ASSERT_NE (target, nullptr);

    auto frame = GpuFrame::begin (device);
    EXPECT_TRUE (pass.draw (frame, target, {}, nullptr, GpuColor::transparentBlack()));
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    const auto image = readBack (target);
    ASSERT_TRUE (image.isValid());

    // Texel centers: uv = (i + 0.5) / 8, so 16 at the first texel and 239 at the last
    EXPECT_NEAR (image.getPixelColor (0, 0).getRed(), 16, 2);
    EXPECT_NEAR (image.getPixelColor (0, 0).getGreen(), 16, 2);
    EXPECT_NEAR (image.getPixelColor (size - 1, 0).getRed(), 239, 2);
    EXPECT_NEAR (image.getPixelColor (0, size - 1).getGreen(), 239, 2);
}

TEST_F (FullscreenPassMetalTests, CopyKeepsTheOrientationOfItsInput)
{
    const auto bundle = compileBundle (R"(#version 450
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main() { color = texture(sampler2D(source, sourceSampler), uv); })");

    FullscreenPass pass;
    ASSERT_TRUE (pass.prepare (device, static_cast<const uint8*> (bundle.getData()), bundle.getSize(), GpuTextureFormat::rgba8unorm).wasOk());

    // The first uploaded row red, the others green; one column blue
    std::vector<uint8> texels (size * size * 4);
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            auto* texel = texels.data() + (y * size + x) * 4;
            texel[0] = y == 0 ? 255 : 0;
            texel[1] = y == 0 ? 0 : 255;
            texel[2] = x == 0 ? 255 : 0;
            texel[3] = 255;
        }
    }

    auto source = GpuTexture::create (device, GpuTextureDesc (size, size, GpuTextureFormat::rgba8unorm));
    ASSERT_NE (source, nullptr);

    GpuTextureDataDesc data;
    data.data = texels.data();
    ASSERT_TRUE (source->upload (data));

    auto copy = createTarget();
    ASSERT_NE (copy, nullptr);

    auto sampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::nearest, GpuWrapMode::clampToEdge));

    auto frame = GpuFrame::begin (device);
    EXPECT_TRUE (pass.draw (frame, copy, {}, [&] (GpuRenderPass& renderPass)
    {
        renderPass.setTexture (0, 0, source);
        renderPass.setSampler (0, 1, sampler);
    }));
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    // The first uploaded row is v = 0, which uv maps to the top of the frame
    const auto image = readBack (copy);
    ASSERT_TRUE (image.isValid());

    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            const auto color = image.getPixelColor (x, y);
            EXPECT_EQ (color.getRed(), y == 0 ? 255 : 0) << x << ", " << y;
            EXPECT_EQ (color.getGreen(), y == 0 ? 0 : 255) << x << ", " << y;
            EXPECT_EQ (color.getBlue(), x == 0 ? 255 : 0) << x << ", " << y;
        }
    }
}

TEST_F (FullscreenPassMetalTests, WritesEveryColorTarget)
{
    const auto bundle = compileBundle (R"(#version 450
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 first;
layout(location = 1) out vec4 second;
void main() { first = vec4(1.0, 0.0, 0.0, 1.0); second = vec4(0.0, 1.0, 0.0, 1.0); })");

    FullscreenPass pass;
    const auto prepared = pass.prepare (device, static_cast<const uint8*> (bundle.getData()), bundle.getSize(), GpuTextureFormat::rgba8unorm, FullscreenPass::Blend::replace, 2);
    ASSERT_TRUE (prepared.wasOk()) << prepared.getErrorMessage();

    auto first = createTarget();
    auto second = createTarget();
    ASSERT_NE (first, nullptr);
    ASSERT_NE (second, nullptr);

    auto frame = GpuFrame::begin (device);
    EXPECT_TRUE (pass.draw (frame, first, {}, [&] (GpuRenderPass& renderPass)
    {
        renderPass.setColorAttachment (1, second, { true, GpuColor::transparentBlack() });
    }, GpuColor::transparentBlack()));
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    const auto firstImage = readBack (first);
    const auto secondImage = readBack (second);
    ASSERT_TRUE (firstImage.isValid());
    ASSERT_TRUE (secondImage.isValid());

    EXPECT_EQ (firstImage.getPixelColor (3, 3).getRed(), 255);
    EXPECT_EQ (firstImage.getPixelColor (3, 3).getGreen(), 0);
    EXPECT_EQ (secondImage.getPixelColor (3, 3).getRed(), 0);
    EXPECT_EQ (secondImage.getPixelColor (3, 3).getGreen(), 255);
}

TEST_F (FullscreenPassMetalTests, WritesColorTargetsOfDifferentFormats)
{
    // One rgba8 target and one rgba16float target holding a value an rgba8 target would clamp
    const auto bundle = compileBundle (R"(#version 450
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 first;
layout(location = 1) out vec4 second;
void main() { first = vec4(1.0, 0.0, 0.0, 1.0); second = vec4(0.0, 3.0, 0.0, 1.0); })");

    const std::array formats { GpuTextureFormat::rgba8unorm, GpuTextureFormat::rgba16float };
    FullscreenPass pass;
    const auto prepared = pass.prepare (device, static_cast<const uint8*> (bundle.getData()), bundle.getSize(), formats);
    ASSERT_TRUE (prepared.wasOk()) << prepared.getErrorMessage();

    auto first = createTarget();
    auto second = GpuTexture::create (device, GpuTextureDesc (size, size, GpuTextureFormat::rgba16float, true));
    ASSERT_NE (first, nullptr);
    ASSERT_NE (second, nullptr);

    // The half float target read back through a quarter of its green, into an rgba8 target
    const auto quarter = compileBundle (R"(#version 450
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(location = 0) out vec4 color;
void main() { color = vec4(0.0, texelFetch(sampler2D(source, sourceSampler), ivec2(gl_FragCoord.xy), 0).g * 0.25, 0.0, 1.0); })");

    FullscreenPass reader;
    ASSERT_TRUE (reader.prepare (device, static_cast<const uint8*> (quarter.getData()), quarter.getSize(), GpuTextureFormat::rgba8unorm).wasOk());

    auto readable = createTarget();
    ASSERT_NE (readable, nullptr);
    auto sampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::nearest, GpuWrapMode::clampToEdge));

    auto frame = GpuFrame::begin (device);
    EXPECT_TRUE (pass.draw (frame, first, {}, [&] (GpuRenderPass& renderPass)
    {
        renderPass.setColorAttachment (1, second, { true, GpuColor::transparentBlack() });
    }, GpuColor::transparentBlack()));
    EXPECT_TRUE (reader.draw (frame, readable, {}, [&] (GpuRenderPass& renderPass)
    {
        renderPass.setTexture (0, 0, second);
        renderPass.setSampler (0, 1, sampler);
    }, GpuColor::transparentBlack()));
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    const auto firstImage = readBack (first);
    const auto secondImage = readBack (readable);
    ASSERT_TRUE (firstImage.isValid());
    ASSERT_TRUE (secondImage.isValid());

    EXPECT_EQ (firstImage.getPixelColor (3, 3).getRed(), 255);
    EXPECT_NEAR (secondImage.getPixelColor (3, 3).getGreen(), 191, 1); // 3 / 4, not 1 / 4 from a clamped 1

    // No format, nothing to compile
    FullscreenPass none;
    EXPECT_TRUE (none.prepare (device, static_cast<const uint8*> (bundle.getData()), bundle.getSize(), Span<const GpuTextureFormat>()).failed());
    EXPECT_FALSE (none.isPrepared());
}

TEST_F (FullscreenPassMetalTests, PremultipliedOverCoversTheTargetByItsAlpha)
{
    // (S, 1 - T) over the target gives dst * T + S, alpha included: here S = (0.25, 0, 0) and T = 0.5
    const auto bundle = compileBundle (R"(#version 450
layout(location = 0) out vec4 color;
void main() { color = vec4(0.25, 0.0, 0.0, 0.5); })");

    FullscreenPass pass;
    const auto prepared = pass.prepare (device, static_cast<const uint8*> (bundle.getData()), bundle.getSize(), GpuTextureFormat::rgba16float, FullscreenPass::Blend::premultipliedOver);
    ASSERT_TRUE (prepared.wasOk()) << prepared.getErrorMessage();

    // The half float target read back into an rgba8 one, its alpha into blue
    const auto copy = compileBundle (R"(#version 450
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(location = 0) out vec4 color;
void main() { vec4 t = texelFetch(sampler2D(source, sourceSampler), ivec2(gl_FragCoord.xy), 0); color = vec4(t.rg, t.a, 1.0); })");

    FullscreenPass reader;
    ASSERT_TRUE (reader.prepare (device, static_cast<const uint8*> (copy.getData()), copy.getSize(), GpuTextureFormat::rgba8unorm).wasOk());

    auto target = GpuTexture::create (device, GpuTextureDesc (size, size, GpuTextureFormat::rgba16float, true));
    auto readable = createTarget();
    auto sampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::nearest, GpuWrapMode::clampToEdge));
    ASSERT_NE (target, nullptr);
    ASSERT_NE (readable, nullptr);

    auto frame = GpuFrame::begin (device);
    EXPECT_TRUE (pass.draw (frame, target, {}, nullptr, GpuColor (0.0f, 0.8f, 0.0f, 0.4f)));
    EXPECT_TRUE (reader.draw (frame, readable, {}, [&] (GpuRenderPass& renderPass)
    {
        renderPass.setTexture (0, 0, target);
        renderPass.setSampler (0, 1, sampler);
    }, GpuColor::transparentBlack()));
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    const auto image = readBack (readable);
    ASSERT_TRUE (image.isValid());

    const auto pixel = image.getPixelColor (3, 3);
    EXPECT_NEAR (pixel.getRed(), 64, 1);    // 0.25 + 0 * 0.5
    EXPECT_NEAR (pixel.getGreen(), 102, 1); // 0 + 0.8 * 0.5
    EXPECT_NEAR (pixel.getBlue(), 179, 1);  // alpha 0.5 + 0.4 * 0.5
}

TEST_F (FullscreenPassMetalTests, DrawWithoutPrepareFails)
{
    FullscreenPass pass;
    auto target = createTarget();

    auto frame = GpuFrame::begin (device);
    EXPECT_FALSE (pass.draw (frame, target, {}, nullptr));
}

TEST_F (FullscreenPassMetalTests, KeepsOneTargetPerTextureAndView)
{
    const auto bundle = compileBundle (R"(#version 450
layout(location = 0) out vec4 color;
void main() { color = vec4(1.0); })");

    FullscreenPass pass;
    const auto prepared = pass.prepare (device, static_cast<const uint8*> (bundle.getData()), bundle.getSize(), GpuTextureFormat::rgba8unorm);
    ASSERT_TRUE (prepared.wasOk()) << prepared.getErrorMessage();

    const auto draw = [&] (const GpuTexture::Ptr& texture, const GpuTextureViewDesc& view)
    {
        auto frame = GpuFrame::begin (device);
        EXPECT_TRUE (pass.draw (frame, texture, view, nullptr, GpuColor::transparentBlack()));
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());
    };

    GpuTextureDesc desc (size, size, GpuTextureFormat::rgba8unorm, true);
    desc.mipLevels = 2;

    auto first = GpuTexture::create (device, desc);
    auto second = GpuTexture::create (device, desc);
    ASSERT_NE (first, nullptr);
    ASSERT_NE (second, nullptr);

    // The cached target holds the texture, once however many times it is drawn into
    draw (first, {});
    const auto referencesWithTarget = first->getReferenceCount();
    EXPECT_GT (referencesWithTarget, 1);

    draw (first, {});
    draw (first, {});
    EXPECT_EQ (first->getReferenceCount(), referencesWithTarget);
    EXPECT_EQ (pass.getNumCachedTargets(), 1u);

    // Another view, another texture: one target each
    draw (first, GpuTextureViewDesc (1, 0));
    draw (second, {});
    EXPECT_EQ (pass.getNumCachedTargets(), 3u);

    // Every target was drawn into since the last trim, so all stay
    pass.trimUnusedTargets();
    EXPECT_EQ (pass.getNumCachedTargets(), 3u);

    // A frame drawing only into the second texture: the first one's targets go, and with them its last reference
    draw (second, {});
    pass.trimUnusedTargets();
    EXPECT_EQ (pass.getNumCachedTargets(), 1u);
    EXPECT_EQ (first->getReferenceCount(), 1);

    pass.releaseTargets();
    EXPECT_EQ (pass.getNumCachedTargets(), 0u);
    EXPECT_EQ (second->getReferenceCount(), 1);
}
#endif
