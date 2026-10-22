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
// Metal - a graphics pipeline whose bind groups reuse binding numbers
//==============================================================================

class GpuPipelineMultiSetMetalTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr || ! device->isGpuAvailable())
            GTEST_SKIP() << "No Metal device available";
    }

    /** Reads ViewBlock and DrawBlock, which share binding 0 in sets 0 and 2, into a flat varying. */
    static constexpr const char* vertexSource = R"(#version 450
layout(set = 0, binding = 0) uniform ViewBlock { vec4 value; } view;
layout(set = 2, binding = 0) uniform DrawBlock { vec4 value; } draw;
layout(location = 0) out vec2 uv;
layout(location = 1) flat out vec2 vertexValues;
void main()
{
    uv = vec2 ((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    vertexValues = vec2 (view.value.x, draw.value.x);
    gl_Position = vec4 (uv * 2.0 - 1.0, 0.0, 1.0);
})";

    /** Writes four vertical stripes: the vertex values with ViewBlock, then LightingBlock with
        DrawBlock, then the set 1 texture, then the set 2 texture. The three blocks all sit at
        binding 0, and both textures at binding 3. */
    static constexpr const char* fragmentSource = R"(#version 450
layout(set = 0, binding = 0) uniform ViewBlock { vec4 value; } view;
layout(set = 0, binding = 1) uniform sampler pointClamp;
layout(set = 1, binding = 0) uniform LightingBlock { vec4 value; } lighting;
layout(set = 1, binding = 3) uniform texture2D u_shadow;
layout(set = 2, binding = 0) uniform DrawBlock { vec4 value; } draw;
layout(set = 2, binding = 3) uniform texture2D u_material;
layout(location = 0) in vec2 uv;
layout(location = 1) flat in vec2 vertexValues;
layout(location = 0) out vec4 color;
void main()
{
    if (uv.x < 0.25)
        color = vec4 (vertexValues, view.value.x, 1.0);
    else if (uv.x < 0.5)
        color = vec4 (lighting.value.x, draw.value.x, 0.0, 1.0);
    else if (uv.x < 0.75)
        color = vec4 (texture (sampler2D (u_shadow, pointClamp), vec2 (0.5)).rgb, 1.0);
    else
        color = vec4 (texture (sampler2D (u_material, pointClamp), vec2 (0.5)).rgb, 1.0);
})";

    /** A fullscreen triangle declaring no resources, so the fragment shader alone decides the native slots. */
    static constexpr const char* plainVertexSource = R"(#version 450
layout(location = 0) out vec2 uv;
void main()
{
    uv = vec2 ((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4 (uv * 2.0 - 1.0, 0.0, 1.0);
})";

    GpuPipeline::Ptr compileWithPlainVertex (const char* fragment)
    {
        GpuPipelineOptions options;
        options.colorTargets.emplace_back().format = GpuTextureFormat::rgba8unorm;

        auto pipeline = GpuPipeline::compileFromGlsl (device, plainVertexSource, fragment, options);
        EXPECT_TRUE (pipeline.wasOk()) << pipeline.getErrorMessage();
        return pipeline.wasOk() ? pipeline.getValue() : nullptr;
    }

    GpuTexture::Ptr makeSolidTexture (uint8 red, uint8 green, uint8 blue)
    {
        auto texture = GpuTexture::create (device, GpuTextureDesc (2, 2, GpuTextureFormat::rgba8unorm));
        if (texture == nullptr)
            return nullptr;

        std::vector<uint8> texels;
        for (int i = 0; i < 4; ++i)
            texels.insert (texels.end(), { red, green, blue, 255 });

        GpuTextureDataDesc data;
        data.data = texels.data();
        return texture->upload (data) ? texture : nullptr;
    }

    static void expectPixel (const std::vector<uint8>& pixels, int column, int red, int green, int blue)
    {
        const auto* pixel = pixels.data() + column * 4;
        EXPECT_NEAR (pixel[0], red, 2) << "stripe " << column << " red";
        EXPECT_NEAR (pixel[1], green, 2) << "stripe " << column << " green";
        EXPECT_NEAR (pixel[2], blue, 2) << "stripe " << column << " blue";
    }

    GpuDevice::Ptr device;
};

TEST_F (GpuPipelineMultiSetMetalTests, KeepsEqualBindingNumbersOfDifferentSetsApart)
{
    constexpr int size = 4;
    auto target = GpuTarget::create (device, size, size);
    ASSERT_NE (target, nullptr);

    GpuPipelineOptions options;
    options.colorTargets.emplace_back().format = GpuTextureFormat::rgba8unorm;

    auto pipeline = GpuPipeline::compileFromGlsl (device, vertexSource, fragmentSource, options);
    ASSERT_TRUE (pipeline.wasOk()) << pipeline.getErrorMessage();

    auto shadow = makeSolidTexture (64, 128, 192);
    auto material = makeSolidTexture (192, 32, 96);
    ASSERT_NE (shadow, nullptr);
    ASSERT_NE (material, nullptr);

    const float viewValue[4] = { 0.2f, 0.2f, 0.2f, 0.2f };
    const float lightingValue[4] = { 0.4f, 0.4f, 0.4f, 0.4f };
    const float drawValue[4] = { 0.6f, 0.6f, 0.6f, 0.6f };

    auto frame = GpuFrame::begin (device);
    auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
    pass.setPipeline (pipeline.getValue());
    pass.setUniformBuffer (0, 0, viewValue, sizeof (viewValue));
    pass.setUniformBuffer (1, 0, lightingValue, sizeof (lightingValue));
    pass.setUniformBuffer (2, 0, drawValue, sizeof (drawValue));
    pass.setTexture (1, 3, shadow);
    pass.setTexture (2, 3, material);
    pass.draw (3);
    pass.finish();
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    std::vector<uint8> pixels (size * size * 4);
    ASSERT_TRUE (target->readPixels (pixels.data(), pixels.size()));

    expectPixel (pixels, 0, 51, 153, 51);
    expectPixel (pixels, 1, 102, 153, 0);
    expectPixel (pixels, 2, 64, 128, 192);
    expectPixel (pixels, 3, 192, 32, 96);
}

TEST_F (GpuPipelineMultiSetMetalTests, PipelinesSharingSetZeroDrawFromOneBindGroup)
{
    constexpr int size = 4;
    auto target = GpuTarget::create (device, size, size);
    ASSERT_NE (target, nullptr);

    // Set 0, including its default sampler, is the same in both; set 2 differs
    auto first = compileWithPlainVertex (R"(#version 450
layout(set = 0, binding = 0) uniform ViewBlock { vec4 value; } view;
layout(set = 0, binding = 1) uniform texture2D u_view;
layout(set = 0, binding = 2) uniform sampler linearClamp;
layout(set = 2, binding = 0) uniform DrawBlock { vec4 value; } draw;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    color = vec4 (view.value.x, texture (sampler2D (u_view, linearClamp), vec2 (0.5)).g, draw.value.x, 1.0);
})");

    auto second = compileWithPlainVertex (R"(#version 450
layout(set = 0, binding = 0) uniform ViewBlock { vec4 value; } view;
layout(set = 0, binding = 1) uniform texture2D u_view;
layout(set = 0, binding = 2) uniform sampler linearClamp;
layout(set = 2, binding = 0) uniform DrawBlock { vec4 value; } draw;
layout(set = 2, binding = 1) uniform texture2D u_material;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    color = vec4 (view.value.x,
                  texture (sampler2D (u_view, linearClamp), vec2 (0.5)).g,
                  texture (sampler2D (u_material, linearClamp), vec2 (0.5)).b * draw.value.y,
                  1.0);
})");

    ASSERT_NE (first, nullptr);
    ASSERT_NE (second, nullptr);

    auto viewTexture = makeSolidTexture (0, 128, 0);
    auto material = makeSolidTexture (0, 0, 200);
    ASSERT_NE (viewTexture, nullptr);
    ASSERT_NE (material, nullptr);

    const float viewValue[4] = { 0.2f, 0.2f, 0.2f, 0.2f };
    const float drawValue[4] = { 0.6f, 1.0f, 0.0f, 0.0f };

    auto frame = GpuFrame::begin (device);
    auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
    pass.setPipeline (first);
    pass.setUniformBuffer (0, 0, viewValue, sizeof (viewValue));
    pass.setTexture (0, 1, viewTexture);
    pass.setUniformBuffer (2, 0, drawValue, sizeof (drawValue));
    pass.setViewport (0.0f, 0.0f, size / 2.0f, (float) size);
    pass.draw (3);

    pass.setPipeline (second);
    pass.setTexture (2, 1, material);
    pass.setViewport (size / 2.0f, 0.0f, size / 2.0f, (float) size);
    pass.draw (3);
    pass.finish();
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    std::vector<uint8> pixels (size * size * 4);
    ASSERT_TRUE (target->readPixels (pixels.data(), pixels.size()));

    expectPixel (pixels, 0, 51, 128, 153);
    expectPixel (pixels, 3, 51, 128, 200);
}

TEST_F (GpuPipelineMultiSetMetalTests, SwitchingToAPipelineWithOtherSetLayoutsRebuildsThem)
{
    constexpr int size = 4;
    auto target = GpuTarget::create (device, size, size);
    ASSERT_NE (target, nullptr);

    auto first = compileWithPlainVertex (R"(#version 450
layout(set = 0, binding = 0) uniform ViewBlock { vec4 value; } view;
layout(set = 1, binding = 0) uniform LightingBlock { vec4 value; } lighting;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    color = vec4 (view.value.x, lighting.value.x, 0.0, 1.0);
})");

    // Set 0 gains a block, which also moves the native slot of the otherwise equal set 1
    auto second = compileWithPlainVertex (R"(#version 450
layout(set = 0, binding = 0) uniform ViewBlock { vec4 value; } view;
layout(set = 0, binding = 1) uniform TintBlock { vec4 value; } tint;
layout(set = 1, binding = 0) uniform LightingBlock { vec4 value; } lighting;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    color = vec4 (view.value.x, lighting.value.x, tint.value.x, 1.0);
})");

    ASSERT_NE (first, nullptr);
    ASSERT_NE (second, nullptr);

    const float viewValue[4] = { 0.2f, 0.2f, 0.2f, 0.2f };
    const float lightingValue[4] = { 0.4f, 0.4f, 0.4f, 0.4f };
    const float tintValue[4] = { 0.8f, 0.8f, 0.8f, 0.8f };

    // Every binding is set up front: only the pipeline switch can bring the tint into a bind group
    auto frame = GpuFrame::begin (device);
    auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
    pass.setPipeline (first);
    pass.setUniformBuffer (0, 0, viewValue, sizeof (viewValue));
    pass.setUniformBuffer (0, 1, tintValue, sizeof (tintValue));
    pass.setUniformBuffer (1, 0, lightingValue, sizeof (lightingValue));
    pass.setViewport (0.0f, 0.0f, size / 2.0f, (float) size);
    pass.draw (3);

    pass.setPipeline (second);
    pass.setViewport (size / 2.0f, 0.0f, size / 2.0f, (float) size);
    pass.draw (3);
    pass.finish();
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    std::vector<uint8> pixels (size * size * 4);
    ASSERT_TRUE (target->readPixels (pixels.data(), pixels.size()));

    expectPixel (pixels, 0, 51, 102, 0);
    expectPixel (pixels, 3, 51, 102, 204);
}

#endif // YUP_ENABLE_SHADER_TRANSPILER
