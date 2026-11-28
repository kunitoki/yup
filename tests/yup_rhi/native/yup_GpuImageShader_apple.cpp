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

#include <yup_rhi/yup_rhi.h>

using namespace yup;

#if YUP_ENABLE_SHADER_TRANSPILER

//==============================================================================
// Metal - image shaders drawn by a real device, read back through a canvas-backed target
//==============================================================================

class GpuImageShaderMetalTests : public GpuReadbackFixture
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr || ! device->isGpuAvailable())
            GTEST_SKIP() << "No Metal device available";
    }

    /** Writes the premultiplied color of its uniform block: no input. */
    static constexpr const char* fillFragment = R"(#version 450
layout(set = 0, binding = 0) uniform Parameters { vec4 tint; } params;
layout(location = 0) out vec4 color;
void main() { color = params.tint; })";

    /** Copies its input. */
    static constexpr const char* copyFragment = R"(#version 450
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main() { color = texture (sampler2D (source, sourceSampler), uv); })";

    /** Writes the red of the texture at binding 1 and the green of the one at binding 3, declared the other way around. */
    static constexpr const char* twoInputsFragment = R"(#version 450
layout(set = 0, binding = 3) uniform texture2D overlay;
layout(set = 0, binding = 1) uniform texture2D base;
layout(set = 0, binding = 2) uniform sampler linearClamp;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    color = vec4 (texture (sampler2D (base, linearClamp), uv).r, texture (sampler2D (overlay, linearClamp), uv).g, 0.0, 1.0);
})";

    static GpuImageShader::Ptr createShader (const char* fragment)
    {
        const auto created = GpuImageShader::createFromGlsl (fragment);
        EXPECT_TRUE (created.wasOk()) << created.getErrorMessage();
        return created.wasOk() ? created.getValue() : nullptr;
    }

    /** Uploads rows of RGBA8 texels, the top one first. */
    GpuTexture::Ptr makeTexture (int width, int height, const std::vector<uint8>& texels)
    {
        auto texture = GpuTexture::create (device, GpuTextureDesc (static_cast<uint32_t> (width), static_cast<uint32_t> (height), GpuTextureFormat::rgba8unorm));
        if (texture == nullptr)
            return nullptr;

        GpuTextureDataDesc data;
        data.data = texels.data();
        return texture->upload (data) ? texture : nullptr;
    }

    /** Draws a shader into @p target, submits the frame and waits for the GPU. */
    bool drawInto (const GpuImageShader& shader, GpuTarget& target, const ShaderUniforms& uniforms, GpuTexture::Ptr first = nullptr, GpuTexture::Ptr second = nullptr)
    {
        auto frame = GpuFrame::begin (device);

        const auto drawn = shader.draw (device, frame, target, uniforms, first, second);
        EXPECT_TRUE (drawn.wasOk()) << drawn.getErrorMessage();

        EXPECT_TRUE (frame.submit());
        EXPECT_TRUE (frame.waitForGPU().wasOk());
        return drawn.wasOk();
    }

    /** Draws a shader into a canvas-backed size x size target and returns its RGBA8 pixels, the top
        row first, or nothing when the draw or the readback fails. */
    std::vector<uint8> drawAndRead (const GpuImageShader& shader, int size, const ShaderUniforms& uniforms, GpuTexture::Ptr first = nullptr, GpuTexture::Ptr second = nullptr)
    {
        auto target = GpuTarget::create (device, size, size);
        EXPECT_NE (target, nullptr);
        if (target == nullptr || ! drawInto (shader, *target, uniforms, first, second))
            return {};

        std::vector<uint8> pixels (static_cast<size_t> (size * size * 4));
        if (! target->readPixels (pixels.data(), pixels.size()))
            return {};

        return pixels;
    }

    static void expectPixel (const std::vector<uint8>& pixels, size_t index, int red, int green, int blue, int alpha)
    {
        const auto* pixel = pixels.data() + index * 4;
        EXPECT_NEAR (pixel[0], red, 2) << "pixel " << index << " red";
        EXPECT_NEAR (pixel[1], green, 2) << "pixel " << index << " green";
        EXPECT_NEAR (pixel[2], blue, 2) << "pixel " << index << " blue";
        EXPECT_NEAR (pixel[3], alpha, 2) << "pixel " << index << " alpha";
    }
};

TEST_F (GpuImageShaderMetalTests, ProceduralShaderFillsTheTargetWithItsPremultipliedColor)
{
    const auto shader = createShader (fillFragment);
    ASSERT_NE (shader, nullptr);
    EXPECT_EQ (shader->getNumInputTextures(), 0);

    auto uniforms = shader->createUniforms();
    ASSERT_TRUE (uniforms.set ("tint", { 0.2f, 0.4f, 0.1f, 0.5f }));

    constexpr int size = 4;
    const auto pixels = drawAndRead (*shader, size, uniforms);
    ASSERT_EQ (pixels.size(), static_cast<size_t> (size * size * 4));

    for (size_t index = 0; index < static_cast<size_t> (size * size); ++index)
        expectPixel (pixels, index, 51, 102, 26, 128);
}

TEST_F (GpuImageShaderMetalTests, ShaderWithOneInputCopiesATextureWhereItIs)
{
    const auto shader = createShader (copyFragment);
    ASSERT_NE (shader, nullptr);
    EXPECT_EQ (shader->getNumInputTextures(), 1);

    // Red and green on the top row, blue and white on the bottom one
    const std::vector<uint8> texels { 255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255 };
    auto source = makeTexture (2, 2, texels);
    ASSERT_NE (source, nullptr);

    const auto pixels = drawAndRead (*shader, 2, shader->createUniforms(), source);
    ASSERT_EQ (pixels.size(), texels.size());

    for (size_t index = 0; index < 4; ++index)
        expectPixel (pixels, index, texels[index * 4], texels[index * 4 + 1], texels[index * 4 + 2], texels[index * 4 + 3]);
}

TEST_F (GpuImageShaderMetalTests, InputsAreTheTexturesInBindingOrder)
{
    const auto shader = createShader (twoInputsFragment);
    ASSERT_NE (shader, nullptr);
    EXPECT_EQ (shader->getNumInputTextures(), 2);

    auto first = makeTexture (1, 1, { 200, 0, 0, 255 });
    auto second = makeTexture (1, 1, { 0, 100, 0, 255 });
    ASSERT_NE (first, nullptr);
    ASSERT_NE (second, nullptr);

    const auto pixels = drawAndRead (*shader, 2, shader->createUniforms(), first, second);
    ASSERT_EQ (pixels.size(), 16u);

    for (size_t index = 0; index < 4; ++index)
        expectPixel (pixels, index, 200, 100, 0, 255);
}

TEST_F (GpuImageShaderMetalTests, DrawsIntoTargetsOfDifferentFormats)
{
    const auto fill = createShader (fillFragment);
    const auto copy = createShader (copyFragment);
    ASSERT_NE (fill, nullptr);
    ASSERT_NE (copy, nullptr);

    auto uniforms = fill->createUniforms();
    ASSERT_TRUE (uniforms.set ("tint", { 0.2f, 0.4f, 0.1f, 0.5f }));

    // The same shader into a rgba8 canvas, then into a float texture, each with a pipeline of its own
    const auto direct = drawAndRead (*fill, 2, uniforms);
    ASSERT_EQ (direct.size(), 16u);
    expectPixel (direct, 0, 51, 102, 26, 128);

    auto floatTarget = GpuTarget::create (device, GpuTextureDesc (2, 2, GpuTextureFormat::rgba16float));
    ASSERT_NE (floatTarget, nullptr);
    ASSERT_TRUE (drawInto (*fill, *floatTarget, uniforms));

    const auto copied = drawAndRead (*copy, 2, copy->createUniforms(), floatTarget->asTexture());
    ASSERT_EQ (copied.size(), 16u);

    for (size_t index = 0; index < 4; ++index)
        expectPixel (copied, index, 51, 102, 26, 128);
}

TEST_F (GpuImageShaderMetalTests, DrawsWithUniformsSetByHandleAndWithTheOnesOfTheSameBlockMadeApart)
{
    const auto shader = createShader (fillFragment);
    const auto twin = createShader (fillFragment);
    ASSERT_NE (shader, nullptr);
    ASSERT_NE (twin, nullptr);

    // A member found once in the shader is set with no search in the uniforms the shader creates
    const auto tint = shader->getUniformLayout().findMember ("tint");
    ASSERT_TRUE (tint.isValid());

    auto uniforms = shader->createUniforms();
    ASSERT_TRUE (uniforms.set (tint, { 0.2f, 0.4f, 0.1f, 0.5f }));

    constexpr int size = 2;
    auto pixels = drawAndRead (*shader, size, uniforms);
    ASSERT_EQ (pixels.size(), static_cast<size_t> (size * size * 4));

    for (size_t index = 0; index < static_cast<size_t> (size * size); ++index)
        expectPixel (pixels, index, 51, 102, 26, 128);

    // The uniforms of a shader made apart from the same source are laid out the same: the draw takes them as its own
    auto fromTwin = twin->createUniforms();
    ASSERT_NE (fromTwin.getMembers().data(), uniforms.getMembers().data());
    ASSERT_TRUE (fromTwin.set (tint, { 0.4f, 0.2f, 0.1f, 0.5f }));

    pixels = drawAndRead (*shader, size, fromTwin);
    ASSERT_EQ (pixels.size(), static_cast<size_t> (size * size * 4));

    for (size_t index = 0; index < static_cast<size_t> (size * size); ++index)
        expectPixel (pixels, index, 102, 51, 26, 128);
}

TEST_F (GpuImageShaderMetalTests, ShaderCompiledForMetalAloneDrawsWithTheMembersTheSourceNames)
{
    /** `level` and `bias` are names of Metal's own, and so is `kernel`: its variant declares them under other names. */
    static constexpr const char* fragment = R"(#version 450
layout(set = 0, binding = 0) uniform Parameters
{
    float level;
    float bias;
} params;
layout(set = 0, binding = 1) uniform texture2D kernel;
layout(set = 0, binding = 2) uniform sampler kernelSampler;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main() { color = texture (sampler2D (kernel, kernelSampler), uv) * params.level + vec4 (params.bias); })";

    const auto created = GpuImageShader::createFromGlsl (fragment, GpuPlatform::Metal);
    ASSERT_TRUE (created.wasOk()) << created.getErrorMessage();

    const auto shader = created.getValue();
    EXPECT_TRUE (shader->canDrawWith (GpuPlatform::Metal));
    EXPECT_FALSE (shader->canDrawWith (GpuPlatform::OpenGL));
    ASSERT_EQ (shader->getNumInputTextures(), 1);
    EXPECT_EQ (shader->getInputTextureNames()[0], String ("kernel"));

    auto uniforms = shader->createUniforms();
    ASSERT_TRUE (uniforms.set ("level", 0.5f));
    ASSERT_TRUE (uniforms.set ("bias", 0.2f));

    auto source = makeTexture (1, 1, { 200, 100, 0, 255 });
    ASSERT_NE (source, nullptr);

    const auto pixels = drawAndRead (*shader, 2, uniforms, source);
    ASSERT_EQ (pixels.size(), 16u);

    for (size_t index = 0; index < 4; ++index)
        expectPixel (pixels, index, 151, 101, 51, 178);

    // The shader compiled for every API draws the same picture, with the same uniforms
    const auto everywhere = createShader (fragment);
    ASSERT_NE (everywhere, nullptr);
    ASSERT_TRUE (everywhere->getUniformLayout().hasSameLayoutAs (shader->getUniformLayout()));
    EXPECT_EQ (drawAndRead (*everywhere, 2, uniforms, source), pixels);
}

TEST_F (GpuImageShaderMetalTests, ShaderCompiledForAnotherApiIsNotDrawn)
{
    const auto created = GpuImageShader::createFromGlsl (fillFragment, GpuPlatform::OpenGL);
    ASSERT_TRUE (created.wasOk()) << created.getErrorMessage();

    auto target = GpuTarget::create (device, 2, 2);
    ASSERT_NE (target, nullptr);

    auto frame = GpuFrame::begin (device);
    const auto drawn = created.getReference()->draw (device, frame, *target, created.getReference()->createUniforms());
    frame.submit();

    ASSERT_TRUE (drawn.failed());
    EXPECT_TRUE (drawn.getErrorMessage().contains ("variant")) << drawn.getErrorMessage();
}

TEST_F (GpuImageShaderMetalTests, DrawFailsWithoutTheInputTheShaderDeclares)
{
    const auto shader = createShader (copyFragment);
    ASSERT_NE (shader, nullptr);

    auto target = GpuTarget::create (device, 2, 2);
    ASSERT_NE (target, nullptr);

    auto frame = GpuFrame::begin (device);
    const auto drawn = shader->draw (device, frame, *target, shader->createUniforms());
    frame.submit();

    ASSERT_TRUE (drawn.failed());
    EXPECT_TRUE (drawn.getErrorMessage().isNotEmpty());
}

#endif // YUP_ENABLE_SHADER_TRANSPILER
