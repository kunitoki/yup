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

TEST_F (LookupTablesTests, CreatesANewHalfFloatArrayForADevice)
{
    auto metal = GpuDevice::create (GpuPlatform::Metal, {});
    if (metal == nullptr)
        GTEST_SKIP() << "No Metal device available";

    const auto texture = LookupTables::createTexture (metal);
    ASSERT_NE (texture, nullptr);
    EXPECT_EQ (texture->getType(), GpuTextureType::array2D);
    EXPECT_EQ (texture->getWidth(), static_cast<uint32_t> (LookupTables::size));
    EXPECT_EQ (texture->getHeight(), static_cast<uint32_t> (LookupTables::size));
    EXPECT_EQ (texture->getDepthOrArrayLayers(), static_cast<uint32_t> (LookupTables::numLayers));
    EXPECT_EQ (texture->getFormat(), GpuTextureFormat::rgba16float);

    // No cache: the caller owns every texture, and releases it with its device
    EXPECT_NE (LookupTables::createTexture (metal), texture);

    const auto black = LookupTables::createBlackTexture (metal);
    ASSERT_NE (black, nullptr);
    EXPECT_EQ (black->getType(), GpuTextureType::array2D);
    EXPECT_EQ (black->getDepthOrArrayLayers(), static_cast<uint32_t> (LookupTables::numLayers));
}

#if YUP_ENABLE_SHADER_TRANSPILER

class LookupTablesMetalTests : public LookupTablesTests
{
protected:
    /** Creates a Metal device and its lookup array, false without a GPU. */
    bool prepareLookup()
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr || ! device->isGpuAvailable())
            return false;

        lookup = LookupTables::createTexture (device);
        EXPECT_NE (lookup, nullptr);
        return lookup != nullptr;
    }

    /** Draws one pixel with @p body, GLSL that sets vec4 color from the lookup array, bound as
        table and read through tableSampler (nearest), and returns its rgba8 levels.
    */
    std::vector<uint8_t> drawPixel (const String& body)
    {
        const auto vertex = String (R"(#version 450
layout(location = 0) out vec2 uv;
void main()
{
    uv = vec2 ((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4 (uv * 2.0 - 1.0, 0.0, 1.0);
})");

        const auto fragment = String (R"(#version 450
layout(set = 0, binding = 0) uniform texture2DArray table;
layout(set = 0, binding = 1) uniform sampler tableSampler;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
vec4 cell (int layer, int roughness, int angle)
{
    return texelFetch (sampler2DArray (table, tableSampler), ivec3 (angle, roughness, layer), 0);
}
void main()
{
)") + body + "\n}\n";

        GpuPipelineOptions options;
        auto& colorTarget = options.colorTargets.emplace_back();
        colorTarget.format = GpuTextureFormat::rgba8unorm;
        colorTarget.blendEnabled = false; // The alpha channel holds a value too

        auto compiled = GpuPipeline::compileFromGlsl (device, vertex, fragment, options);
        EXPECT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

        auto sampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::nearest, GpuWrapMode::clampToEdge));
        auto target = GpuTarget::create (device, 1, 1);
        std::vector<uint8_t> pixel (4);

        if (! compiled.wasOk() || sampler == nullptr || target == nullptr)
            return {};

        auto frame = GpuFrame::begin (device);
        auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
        pass.setPipeline (compiled.getValue());
        pass.setTexture (0, 0, lookup);
        pass.setSampler (0, 1, sampler);
        EXPECT_TRUE (pass.draw (3));
        pass.finish();
        EXPECT_TRUE (frame.submit());
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        return target->readPixels (pixel.data(), pixel.size()) ? pixel : std::vector<uint8_t>();
    }

    GpuDevice::Ptr device;
    GpuTexture::Ptr lookup;
};

TEST_F (LookupTablesMetalTests, LayersHoldTheAreaLightTablesAtTheirCells)
{
    if (! prepareLookup())
        GTEST_SKIP() << "No Metal device available";

    const auto vec4Literal = [] (const std::array<double, 4>& values)
    {
        String literal ("vec4 (");
        for (size_t i = 0; i < values.size(); ++i)
            literal << (i > 0 ? ", " : "") << String::formatted ("%.9e", values[i]);
        return literal + ")";
    };

    const auto sheenAlbedo = LookupTables::computeSheenAlbedo();

    // Cells (0, 63) and (63, 0) swap under a transposed layout, and their values differ widely
    for (const auto& [r, a] : { std::pair { 0, 63 }, std::pair { 63, 0 }, std::pair { 32, 40 } })
    {
        SCOPED_TRACE (testing::Message() << "cell " << r << ", " << a);

        const auto m = LtcTables::getInverseMatrix (r, a);
        const auto amplitude = LtcTables::getAmplitude (r, a);
        const auto sheen = sheenAlbedo[static_cast<size_t> (r * LookupTables::size + a)];

        for (const auto& [layer, expected] : { std::pair { 0, std::array<double, 4> { m[0], m[1], m[2], m[3] } },
                                               std::pair { 1, std::array<double, 4> { amplitude[0], amplitude[1], sheen, 1.0 } } })
        {
            // 0.5 plus twenty times the relative error of the texel, which an rgba8 target can hold
            std::array<double, 4> scales {};
            for (size_t i = 0; i < scales.size(); ++i)
                scales[i] = 20.0 / std::max (std::abs (expected[i]), 1.0);

            const auto pixel = drawPixel ("color = vec4 (0.5) + (cell (" + String (layer) + ", " + String (r) + ", " + String (a) + ") - "
                                          + vec4Literal (expected) + ") * " + vec4Literal (scales) + ";");
            ASSERT_EQ (pixel.size(), 4u) << "layer " << layer;

            // 8 levels is 1.6e-3 of relative error, three times a half float's rounding
            for (const auto channel : pixel)
                EXPECT_NEAR (static_cast<int> (channel), 128, 8) << "layer " << layer;
        }
    }
}

TEST_F (LookupTablesMetalTests, SmoothSurfaceReflectsEverythingAtNormalIncidence)
{
    if (! prepareLookup())
        GTEST_SKIP() << "No Metal device available";

    // Roughness 0 at n.v = 1: scale plus bias, the lobe magnitude, is 1 within 0.02
    const auto pixel = drawPixel ("vec4 t = cell (1, 0, 0);\n    color = vec4 (0.5 + ((t.x - t.y) + t.y - 1.0) * 10.0, t.y, 0.0, 1.0);");
    ASSERT_EQ (pixel.size(), 4u);
    EXPECT_NEAR (static_cast<int> (pixel[0]), 128, 51);
    EXPECT_LT (static_cast<int> (pixel[1]), 3);
}

TEST_F (LookupTablesMetalTests, GrazingAngleRaisesTheBias)
{
    if (! prepareLookup())
        GTEST_SKIP() << "No Metal device available";

    // The Fresnel bias grows from normal incidence (column 0) to grazing (column 63), at every roughness
    const auto pixel = drawPixel (R"(float rows = 0.0;
    for (int r = 0; r < 64; ++r)
        rows += cell (1, r, 63).y > cell (1, r, 0).y ? 1.0 : 0.0;
    color = vec4 (rows / 64.0, 0.0, 0.0, 1.0);)");
    ASSERT_EQ (pixel.size(), 4u);
    EXPECT_EQ (static_cast<int> (pixel[0]), 255);
}

TEST_F (LookupTablesMetalTests, ValuesStayInTheUnitRange)
{
    if (! prepareLookup())
        GTEST_SKIP() << "No Metal device available";

    // Scale, bias, their sum and the sheen albedo lie within [0, 1] in every cell
    const auto pixel = drawPixel (R"(float outside = 0.0;
    for (int r = 0; r < 64; ++r)
    {
        for (int a = 0; a < 64; ++a)
        {
            vec4 t = cell (1, r, a);
            vec3 values = vec3 (t.x - t.y, t.y, t.z);
            bool inside = all (greaterThanEqual (values, vec3 (0.0))) && all (lessThanEqual (values, vec3 (1.001))) && t.x <= 1.001 && t.w == 1.0;
            // An all-ones exponent is NaN or infinity. Testing the bits keeps fast math from folding it away
            bool bad = ! inside || any (equal (floatBitsToUint (t) & uvec4 (0x7f800000u), uvec4 (0x7f800000u)));
            outside += bad ? 1.0 : 0.0;
        }
    }
    color = vec4 (min (outside, 255.0) / 255.0, 0.0, 0.0, 1.0);)");
    ASSERT_EQ (pixel.size(), 4u);
    EXPECT_EQ (static_cast<int> (pixel[0]), 0);
}

#endif // YUP_ENABLE_SHADER_TRANSPILER
