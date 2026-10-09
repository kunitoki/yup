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

class LtcTablesTests : public ::testing::Test
{
protected:
    struct Direction
    {
        double x = 0.0, y = 0.0, z = 0.0;

        double dot (const Direction& other) const { return x * other.x + y * other.y + z * other.z; }

        Direction normalized() const
        {
            const auto length = std::sqrt (dot (*this));
            return { x / length, y / length, z / length };
        }
    };

    /** A spherical cap: the directions within an angle of an axis. */
    struct Cap
    {
        Direction axis;
        double cosAngle = 1.0;

        bool contains (const Direction& l) const { return l.dot (axis) >= cosAngle; }
    };

    /** The directions between two elevations over the horizon. */
    struct ElevationBand
    {
        double lowDegrees = 0.0, highDegrees = 90.0;

        bool contains (const Direction& l) const
        {
            const auto elevation = radiansToDegrees (std::asin (jlimit (-1.0, 1.0, l.z)));
            return elevation >= lowDegrees && elevation < highDegrees;
        }
    };

    /** A view and roughness, the parameters of a table cell or of a point between cells. */
    struct Lobe
    {
        double alpha = 1.0;
        Direction view;
    };

    /** A cell of the tables, or the bilinear blend of four at a point between cells. */
    struct Fit
    {
        std::array<double, 4> inverse {};
        std::array<double, 2> amplitude {};
    };

    /** GGX integrated by midpoint quadrature over a grid of the normal distribution's inverse CDF. */
    struct Reference
    {
        double magnitude = 0.0;
        double fresnel = 0.0;
        Direction mean;
    };

    static constexpr double pi = MathConstants<double>::pi;
    static constexpr int quadratureSize = 256;

    //==============================================================================
    /** The cell parameters LtcTables documents: rows of sqrt (alpha) floored at 0.04, columns of
        sqrt (1 - cos theta) with cos theta floored at 1.0e-4.
    */
    static Lobe getLobe (double roughnessIndex, double angleIndex)
    {
        const auto roughness = std::max (roughnessIndex / (LtcTables::size - 1), 0.04);
        const auto u = angleIndex / (LtcTables::size - 1);
        const auto cosTheta = std::max (1.0 - u * u, 1.0e-4);

        return { roughness * roughness, { std::sqrt (1.0 - cosTheta * cosTheta), 0.0, cosTheta } };
    }

    static Fit getFit (int roughnessIndex, int angleIndex)
    {
        const auto inverse = LtcTables::getInverseMatrix (roughnessIndex, angleIndex);
        const auto amplitude = LtcTables::getAmplitude (roughnessIndex, angleIndex);

        return { { inverse[0], inverse[1], inverse[2], inverse[3] }, { amplitude[0], amplitude[1] } };
    }

    /** What a shader reads with bilinear filtering halfway between four cells. */
    static Fit getFitBetweenCells (int roughnessIndex, int angleIndex)
    {
        Fit result;

        for (int dr = 0; dr < 2; ++dr)
        {
            for (int da = 0; da < 2; ++da)
            {
                const auto cell = getFit (roughnessIndex + dr, angleIndex + da);

                for (size_t i = 0; i < 4; ++i)
                    result.inverse[i] += 0.25 * cell.inverse[i];

                for (size_t i = 0; i < 2; ++i)
                    result.amplitude[i] += 0.25 * cell.amplitude[i];
            }
        }

        return result;
    }

    //==============================================================================
    static double ggxLambda (double alpha, double cosTheta)
    {
        const auto cos2 = cosTheta * cosTheta;
        return 0.5 * (std::sqrt (1.0 + alpha * alpha * (1.0 - cos2) / cos2) - 1.0);
    }

    /** Visits the quadrature points of the GGX lobe: each reflected direction over the horizon
        with its share of the integral of f (v, l) cos (theta_l), and its v.h.
    */
    template <typename Visitor>
    static void forEachGgxSample (const Lobe& lobe, Visitor&& visit)
    {
        const auto& v = lobe.view;
        const auto lambdaView = ggxLambda (lobe.alpha, v.z);
        constexpr auto area = 1.0 / (quadratureSize * quadratureSize);

        for (int i = 0; i < quadratureSize; ++i)
        {
            // tan^2 (theta_h) = alpha^2 u / (1 - u) samples D (h) cos (theta_h). The weight grows as
            // 1 / sqrt (1 - u) towards the horizon, which u = 1 - (1 - s)^2 cancels
            const auto s = (i + 0.5) / quadratureSize;
            const auto u = 1.0 - (1.0 - s) * (1.0 - s);
            const auto du = 2.0 * (1.0 - s);
            const auto cosH = 1.0 / std::sqrt (1.0 + lobe.alpha * lobe.alpha * u / (1.0 - u));
            const auto sinH = std::sqrt (1.0 - cosH * cosH);

            for (int j = 0; j < quadratureSize; ++j)
            {
                const auto phi = 2.0 * pi * (j + 0.5) / quadratureSize;
                const Direction h { sinH * std::cos (phi), sinH * std::sin (phi), cosH };
                const auto vDotH = v.dot (h);

                const Direction l { 2.0 * vDotH * h.x - v.x, 2.0 * vDotH * h.y - v.y, 2.0 * vDotH * h.z - v.z };
                if (vDotH <= 0.0 || l.z <= 0.0)
                    continue;

                // f cos (theta_l) = D G2 / (4 cos (theta_v)), over the density D cos (theta_h) / (4 v.h)
                const auto g2 = 1.0 / (1.0 + lambdaView + ggxLambda (lobe.alpha, l.z));
                visit (l, area * du * g2 * vDotH / (v.z * cosH), vDotH);
            }
        }
    }

    static Reference integrateGgx (const Lobe& lobe)
    {
        Reference result;

        forEachGgxSample (lobe, [&] (const Direction& l, double weight, double vDotH)
        {
            result.magnitude += weight;
            result.fresnel += weight * std::pow (1.0 - vDotH, 5.0);
            result.mean = { result.mean.x + weight * l.x, result.mean.y + weight * l.y, result.mean.z + weight * l.z };
        });

        result.mean = { result.mean.x / result.magnitude, result.mean.y / result.magnitude, result.mean.z / result.magnitude };
        return result;
    }

    /** The share of the normalized GGX lobe inside a region. */
    template <typename Region>
    static double getGgxMass (const Lobe& lobe, const Reference& reference, const Region& region)
    {
        auto mass = 0.0;

        forEachGgxSample (lobe, [&] (const Direction& l, double weight, double)
        {
            if (region.contains (l))
                mass += weight;
        });

        return mass / reference.magnitude;
    }

    /** The share of the fitted distribution inside a region and over the horizon, which is what
        a shader integrates after clipping a polygon: the cosine integrated over the directions
        that M (the inverse of the stored M^-1) maps into it.
    */
    template <typename Region>
    static double getLtcMass (const Fit& fit, const Region& region)
    {
        const auto [m00, m02, m11, m20] = fit.inverse;
        const auto determinant = m00 - m02 * m20; // of the xz block, m22 being 1

        auto inside = 0;

        for (int i = 0; i < quadratureSize; ++i)
        {
            const auto u = (i + 0.5) / quadratureSize;
            const auto radius = std::sqrt (u);

            for (int j = 0; j < quadratureSize; ++j)
            {
                const auto phi = 2.0 * pi * (j + 0.5) / quadratureSize;
                const Direction w { radius * std::cos (phi), radius * std::sin (phi), std::sqrt (1.0 - u) };

                const auto l = Direction { (w.x - m02 * w.z) / determinant, w.y / m11, (m00 * w.z - m20 * w.x) / determinant }.normalized();
                if (l.z > 0.0 && region.contains (l))
                    ++inside;
            }
        }

        return static_cast<double> (inside) / (quadratureSize * quadratureSize);
    }

    /** Caps around the lobe's mean direction and beside it, sized by the lobe's spread. */
    static std::vector<Cap> getCapsAround (const Reference& reference)
    {
        const auto axis = reference.mean.normalized();
        const auto spread = jlimit (0.002, 1.0, std::acos (std::sqrt (reference.mean.dot (reference.mean))));

        const auto turned = [&axis] (double angle, bool aroundY)
        {
            if (aroundY)
                return Direction { axis.x * std::cos (angle) + axis.z * std::sin (angle), 0.0, axis.z * std::cos (angle) - axis.x * std::sin (angle) };

            return Direction { axis.x * std::cos (angle), std::sin (angle), axis.z * std::cos (angle) };
        };

        return {
            { axis, std::cos (0.5 * spread) },
            { axis, std::cos (spread) },
            { axis, std::cos (2.0 * spread) },
            { turned (spread, true), std::cos (spread) },
            { turned (-spread, true), std::cos (spread) },
            { turned (spread, false), std::cos (spread) },
        };
    }

    /** How far the fitted share of a cap may be from GGX's.

        Measured over the tested cells, the worst errors are 0.04 down to cos theta = 0.4 and 0.07
        down to 0.2. Towards grazing GGX's lobe is cut by the horizon and strongly asymmetric, which
        no transformed cosine follows: the worst error is 0.27, on the smoothest rows.
    */
    static double getMassTolerance (const Lobe& lobe)
    {
        if (lobe.view.z >= 0.4)
            return 0.05;

        return lobe.view.z >= 0.2 ? 0.1 : 0.3;
    }

    /** Checks a fit against the GGX lobe it approximates, cap by cap.

        The magnitudes come from 32k quasi Monte Carlo samples of a weight in [0, 1], and agree
        with this quadrature within 4e-4.
    */
    static void expectFitMatchesGgx (const Fit& fit, const Lobe& lobe)
    {
        const auto reference = integrateGgx (lobe);
        const auto massTolerance = getMassTolerance (lobe);

        EXPECT_NEAR (fit.amplitude[0], reference.magnitude, 2.0e-3);
        EXPECT_NEAR (fit.amplitude[1], reference.fresnel, 2.0e-3);

        for (const auto& cap : getCapsAround (reference))
            EXPECT_NEAR (getLtcMass (fit, cap), getGgxMass (lobe, reference, cap), massTolerance);

        const Cap upperHemisphere { { 0.0, 0.0, 1.0 }, 0.0 };
        EXPECT_GT (getLtcMass (fit, upperHemisphere), 1.0 - massTolerance);
    }

    static std::vector<int> getTestedIndices()
    {
        return { 0, 3, 8, 16, 32, 48, 56, 62, 63 };
    }
};

//==============================================================================
TEST_F (LtcTablesTests, RoughSurfaceAtNormalIncidenceIsNearlyCosine)
{
    // At alpha = 1 and theta = 0 the cosine-weighted GGX lobe is close to a clamped cosine:
    // the inverse matrix is close to a scaled identity
    const auto m = LtcTables::getInverseMatrix (LtcTables::size - 1, 0);
    EXPECT_NEAR (m[1], 0.0f, 0.05f);       // m02
    EXPECT_NEAR (m[3], 0.0f, 0.05f);       // m20
    EXPECT_NEAR (m[0] / m[2], 1.0f, 0.1f); // isotropic in x and y
}

TEST_F (LtcTablesTests, AmplitudeIsAnAlbedo)
{
    for (int r = 0; r < LtcTables::size; ++r)
    {
        for (int a = 0; a < LtcTables::size; ++a)
        {
            const auto amplitude = LtcTables::getAmplitude (r, a);
            EXPECT_GE (amplitude[0], 0.0f);
            EXPECT_LE (amplitude[0], 1.0f + 1.0e-3f); // directional albedo of GGX without Fresnel
            EXPECT_GE (amplitude[1], 0.0f);
            EXPECT_LE (amplitude[1], amplitude[0] + 1.0e-3f); // the Fresnel part is a fraction of it
        }
    }
}

TEST_F (LtcTablesTests, TablesAreFinite)
{
    for (int r = 0; r < LtcTables::size; ++r)
        for (int a = 0; a < LtcTables::size; ++a)
            for (auto value : LtcTables::getInverseMatrix (r, a))
                EXPECT_TRUE (std::isfinite (value));
}

TEST_F (LtcTablesTests, EntriesFitInHalfFloats)
{
    constexpr auto maxHalf = 65504.0f;

    for (int r = 0; r < LtcTables::size; ++r)
    {
        for (int a = 0; a < LtcTables::size; ++a)
        {
            for (auto value : LtcTables::getInverseMatrix (r, a))
                EXPECT_LT (std::abs (value), maxHalf);

            // The texture divides by m00 m22 - m02 m20, which must stay clear of zero
            const auto m = LtcTables::getInverseMatrix (r, a);
            EXPECT_GT (m[0] - m[1] * m[3], 0.0f);
            EXPECT_GT (m[2], 0.0f);
        }
    }
}

TEST_F (LtcTablesTests, LobeLeansAwayFromTheView)
{
    // The view is towards +x, so the reflection and most of the fitted lobe are towards -x
    const auto fit = getFit (16, 40);
    const Cap negativeX { { -1.0, 0.0, 0.0 }, 0.0 };

    EXPECT_GT (getLtcMass (fit, negativeX), 0.9);
    EXPECT_LT (integrateGgx (getLobe (16, 40)).mean.x, 0.0);
}

TEST_F (LtcTablesTests, AmplitudeMatchesGgxAlbedo)
{
    for (int r = 0; r < LtcTables::size; r += 7)
    {
        for (int a : getTestedIndices())
        {
            SCOPED_TRACE (testing::Message() << "cell " << r << ", " << a);

            const auto reference = integrateGgx (getLobe (r, a));
            const auto amplitude = LtcTables::getAmplitude (r, a);

            EXPECT_NEAR (amplitude[0], reference.magnitude, 2.0e-3);
            EXPECT_NEAR (amplitude[1], reference.fresnel, 2.0e-3);
        }
    }
}

TEST_F (LtcTablesTests, CellsMatchGgxLobes)
{
    for (int r : getTestedIndices())
    {
        for (int a : getTestedIndices())
        {
            SCOPED_TRACE (testing::Message() << "cell " << r << ", " << a);
            expectFitMatchesGgx (getFit (r, a), getLobe (r, a));
        }
    }
}

TEST_F (LtcTablesTests, GrazingMassPerElevationMatchesGgx)
{
    // Towards grazing the caps allow 0.3, so the elevation profile of the lobe gets its own check
    const std::vector<ElevationBand> bands { { 0.0, 1.0 }, { 1.0, 3.0 }, { 3.0, 10.0 }, { 10.0, 30.0 }, { 30.0, 90.0 } };

    for (int r : getTestedIndices())
    {
        for (int a : { 56, 60, 62, 63 })
        {
            SCOPED_TRACE (testing::Message() << "cell " << r << ", " << a);

            const auto lobe = getLobe (r, a);
            const auto reference = integrateGgx (lobe);
            const auto fit = getFit (r, a);

            for (const auto& band : bands)
            {
                EXPECT_NEAR (getLtcMass (fit, band), getGgxMass (lobe, reference, band), 0.1);
            }
        }
    }
}

TEST_F (LtcTablesTests, BilinearFitsBetweenCellsMatchGgxLobes)
{
    // Halfway between cells, where filtering blends four fits. Lobes narrower than the change of
    // the reflection between two columns cannot be resolved by the table: below roughness 0.12
    // (row 8) and towards grazing, where the columns are degrees apart, that is not a fit error
    for (int r : { 8, 16, 32, 48, 56, 62 })
    {
        for (int a : { 0, 16, 32, 48, 55 })
        {
            SCOPED_TRACE (testing::Message() << "between cells " << r << ", " << a);
            expectFitMatchesGgx (getFitBetweenCells (r, a), getLobe (r + 0.5, a + 0.5));
        }
    }
}

TEST_F (LtcTablesTests, NullDeviceCreatesNoTextures)
{
    EXPECT_EQ (LtcTables::createMatrixTexture (nullptr), nullptr);
    EXPECT_EQ (LtcTables::createAmplitudeTexture (nullptr), nullptr);
}

#if YUP_APPLE
TEST_F (LtcTablesTests, CreatesNewHalfFloatTexturesForADevice)
{
    auto device = GpuDevice::create (GpuPlatform::Metal, {});
    if (device == nullptr)
        GTEST_SKIP() << "No Metal device available";

    for (const auto& create : { LtcTables::createMatrixTexture, LtcTables::createAmplitudeTexture })
    {
        const auto texture = create (device);
        ASSERT_NE (texture, nullptr);
        EXPECT_EQ (texture->getWidth(), LtcTables::size);
        EXPECT_EQ (texture->getHeight(), LtcTables::size);
        EXPECT_EQ (texture->getFormat(), GpuTextureFormat::rgba16float);

        // No cache: the caller owns every texture, and releases it with its device
        EXPECT_NE (create (device), texture);
    }
}
#endif

#if YUP_APPLE && YUP_ENABLE_SHADER_TRANSPILER
TEST_F (LtcTablesTests, TexturesHoldTheTablesAtTheirCells)
{
    auto device = GpuDevice::create (GpuPlatform::Metal, {});
    if (device == nullptr || ! device->isGpuAvailable())
        GTEST_SKIP() << "No Metal device available";

    const auto matrices = LtcTables::createMatrixTexture (device);
    const auto amplitudes = LtcTables::createAmplitudeTexture (device);
    ASSERT_NE (matrices, nullptr);
    ASSERT_NE (amplitudes, nullptr);

    const auto sampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::nearest, GpuWrapMode::clampToEdge));
    ASSERT_NE (sampler, nullptr);

    const auto vec4Literal = [] (const std::array<double, 4>& values)
    {
        String literal ("vec4 (");
        for (size_t i = 0; i < values.size(); ++i)
            literal << (i > 0 ? ", " : "") << String::formatted ("%.9e", values[i]);
        return literal + ")";
    };

    // Draws 0.5 plus twenty times the relative error of one texel, which an rgba8 target can hold
    const auto readError = [&] (const GpuTexture::Ptr& texture, int r, int a, const std::array<double, 4>& expected)
    {
        std::array<double, 4> scales {};
        for (size_t i = 0; i < scales.size(); ++i)
            scales[i] = 20.0 / std::max (std::abs (expected[i]), 1.0);

        const auto vertex = String (R"(#version 450
layout(location = 0) out vec2 uv;
void main()
{
    uv = vec2 ((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4 (uv * 2.0 - 1.0, 0.0, 1.0);
})");

        const auto fragment = String (R"(#version 450
layout(set = 0, binding = 0) uniform texture2D table;
layout(set = 0, binding = 1) uniform sampler tableSampler;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    vec4 texel = texelFetch (sampler2D (table, tableSampler), ivec2 ()")
                            + String (a) + ", " + String (r) + "), 0);\n    color = vec4 (0.5) + (texel - " + vec4Literal (expected) + ") * " + vec4Literal (scales) + ";\n}\n";

        GpuPipelineOptions options;
        auto& colorTarget = options.colorTargets.emplace_back();
        colorTarget.format = GpuTextureFormat::rgba8unorm;
        colorTarget.blendEnabled = false; // The alpha channel holds an error too

        auto compiled = GpuPipeline::compileFromGlsl (device, vertex, fragment, options);
        EXPECT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

        auto target = GpuTarget::create (device, 1, 1);
        std::vector<uint8_t> pixel (4);

        if (! compiled.wasOk() || target == nullptr)
            return std::vector<uint8_t>();

        auto frame = GpuFrame::begin (device);
        auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
        pass.setPipeline (compiled.getValue());
        pass.setTexture (0, 0, texture);
        pass.setSampler (0, 1, sampler);
        EXPECT_TRUE (pass.draw (3));
        pass.finish();
        EXPECT_TRUE (frame.submit());
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        return target->readPixels (pixel.data(), pixel.size()) ? pixel : std::vector<uint8_t>();
    };

    // Cells (0, 63) and (63, 0) swap under a transposed layout, and their values differ widely
    for (const auto& [r, a] : { std::pair { 0, 63 }, std::pair { 63, 0 }, std::pair { 32, 40 } })
    {
        SCOPED_TRACE (testing::Message() << "cell " << r << ", " << a);

        const auto m = LtcTables::getInverseMatrix (r, a);
        const auto amplitude = LtcTables::getAmplitude (r, a);

        for (const auto& [texture, expected] : { std::pair { matrices, std::array<double, 4> { m[0], m[1], m[2], m[3] } },
                                                 std::pair { amplitudes, std::array<double, 4> { amplitude[0], amplitude[1], 0.0, 1.0 } } })
        {
            const auto pixel = readError (texture, r, a, expected);
            ASSERT_EQ (pixel.size(), 4u);

            // 8 levels is 1.6e-3 of relative error, three times a half float's rounding
            for (const auto channel : pixel)
                EXPECT_NEAR (static_cast<int> (channel), 128, 8);
        }
    }
}
#endif
