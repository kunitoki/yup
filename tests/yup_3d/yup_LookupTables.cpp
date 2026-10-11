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

class LookupTablesTests : public ::testing::Test
{
protected:
    /** The split-sum scale and bias of the analytic fit the lookup tables replaced (Karis, mobile),
        for a perceptual roughness and n.v.
    */
    static std::array<double, 2> analyticFit (double roughness, double nDotV)
    {
        const double c0[4] = { -1.0, -0.0275, -0.572, 0.022 };
        const double c1[4] = { 1.0, 0.0425, 1.04, -0.04 };

        double r[4];
        for (int i = 0; i < 4; ++i)
            r[i] = roughness * c0[i] + c1[i];

        const auto a004 = std::min (r[0] * r[0], std::exp2 (-9.28 * nDotV)) * r[0] + r[1];
        return { -1.04 * a004 + r[2], 1.04 * a004 + r[3] };
    }

    /** The Fresnel part of the lobe of a rough surface (roughness 1) at normal incidence. */
    static double amplitudeBias() { return LtcTables::getAmplitude (LtcTables::size - 1, 0)[1]; }

#if YUP_APPLE && YUP_ENABLE_SHADER_TRANSPILER
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
#endif
};

TEST_F (LookupTablesTests, NullDeviceCreatesNoTextures)
{
    EXPECT_EQ (LookupTables::createTexture (nullptr), nullptr);
    EXPECT_EQ (LookupTables::createBlackTexture (nullptr), nullptr);
}

TEST_F (LookupTablesTests, RecordsTheAnalyticFitItReplaces)
{
    // The split sum is the lobe magnitude x and its Fresnel part y of the area light tables: scale
    // x - y, bias y. The fit it replaced (until Phase 4) is recorded, not asserted: it overestimates
    // rough surfaces, 0.45 instead of 1 - ln 2 at roughness 1 and normal incidence
    double largestScaleDifference = 0.0, largestBiasDifference = 0.0;
    for (int r = 6; r < LtcTables::size; r += 19)
    {
        for (int a = 0; a < LtcTables::size; a += 21)
        {
            const auto roughness = r / static_cast<double> (LtcTables::size - 1);
            const auto u = a / static_cast<double> (LtcTables::size - 1);
            const auto amplitude = LtcTables::getAmplitude (r, a);
            const auto fit = analyticFit (roughness, std::max (1.0 - u * u, 1.0e-4));

            largestScaleDifference = std::max (largestScaleDifference, std::abs (amplitude[0] - amplitude[1] - fit[0]));
            largestBiasDifference = std::max (largestBiasDifference, std::abs (amplitude[1] - fit[1]));
        }
    }

    RecordProperty ("largestScaleDifferenceFromFit", String (largestScaleDifference, 4).toStdString());
    RecordProperty ("largestBiasDifferenceFromFit", String (largestBiasDifference, 4).toStdString());

    // White rough metal (f0 = f90 = 1) at normal incidence, under a uniform environment of radiance
    // 1: the fit, the tables alone, and the tables with the multiple-scattering compensation
    // (Fdez-Aguera 2019: Fms Ems = Ess Favg (1 - Ess) / (1 - Favg (1 - Ess)), Favg = 1)
    const auto fit = analyticFit (1.0, 1.0);
    const auto singleScattering = static_cast<double> (LtcTables::getAmplitude (LtcTables::size - 1, 0)[0]);
    const auto multipleScattering = singleScattering * (1.0 - singleScattering) / (1.0 - (1.0 - singleScattering));

    RecordProperty ("roughMetalFit", String (fit[0] + fit[1], 4).toStdString());
    RecordProperty ("roughMetalLookupTables", String (singleScattering, 4).toStdString());
    RecordProperty ("roughMetalCompensated", String (singleScattering + multipleScattering, 4).toStdString());

    EXPECT_NEAR (singleScattering, 1.0 - std::log (2.0), 1.0e-3);

    // A gray metal, f0 = 0.5: the average Fresnel 0.5 + 0.5 / 21 now bounds the regained energy
    const auto grayFresnel = 0.5 + 0.5 / 21.0;
    const auto graySingle = 0.5 * (singleScattering - amplitudeBias()) + amplitudeBias();
    const auto missing = 1.0 - singleScattering;
    const auto gray = graySingle + graySingle * grayFresnel * missing / (1.0 - grayFresnel * missing);
    RecordProperty ("grayMetalCompensated", String (gray, 4).toStdString());
    EXPECT_NEAR (gray, 0.2409, 1.0e-3);
}

#if YUP_APPLE
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
#endif

#if YUP_APPLE && YUP_ENABLE_SHADER_TRANSPILER
TEST_F (LookupTablesTests, LayersHoldTheAreaLightTablesAtTheirCells)
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

    // Cells (0, 63) and (63, 0) swap under a transposed layout, and their values differ widely
    for (const auto& [r, a] : { std::pair { 0, 63 }, std::pair { 63, 0 }, std::pair { 32, 40 } })
    {
        SCOPED_TRACE (testing::Message() << "cell " << r << ", " << a);

        const auto m = LtcTables::getInverseMatrix (r, a);
        const auto amplitude = LtcTables::getAmplitude (r, a);

        for (const auto& [layer, expected] : { std::pair { 0, std::array<double, 4> { m[0], m[1], m[2], m[3] } },
                                               std::pair { 1, std::array<double, 4> { amplitude[0], amplitude[1], 0.0, 1.0 } } })
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

TEST_F (LookupTablesTests, SmoothSurfaceReflectsEverythingAtNormalIncidence)
{
    if (! prepareLookup())
        GTEST_SKIP() << "No Metal device available";

    // Roughness 0 at n.v = 1: scale plus bias, the lobe magnitude, is 1 within 0.02
    const auto pixel = drawPixel ("vec4 t = cell (1, 0, 0);\n    color = vec4 (0.5 + ((t.x - t.y) + t.y - 1.0) * 10.0, t.y, 0.0, 1.0);");
    ASSERT_EQ (pixel.size(), 4u);
    EXPECT_NEAR (static_cast<int> (pixel[0]), 128, 51);
    EXPECT_LT (static_cast<int> (pixel[1]), 3);
}

TEST_F (LookupTablesTests, GrazingAngleRaisesTheBias)
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

TEST_F (LookupTablesTests, ValuesStayInTheUnitRange)
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
#endif
