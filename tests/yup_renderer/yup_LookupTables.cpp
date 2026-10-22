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

    /** The directional albedo of the sheen lobe at a cell of the tables, integrated independently
        of LookupTables: over the light directions, by the midpoint rule on 1024 x 1024 cells of the
        hemisphere, with the Charlie distribution (Estevez and Kulla 2017) and the visibility of
        Neubelt and Pettineo 2013. The row is the perceptual sheen roughness, floored at
        LookupTables::minSheenRoughness, and the column sqrt (1 - n.v), n.v floored at 1e-4 as the
        shaders do. Not limited to 1.
    */
    static double referenceSheenAlbedo (int roughness, int angle)
    {
        const auto pi = MathConstants<double>::pi;
        const auto perceptual = std::max (roughness / 63.0, static_cast<double> (LookupTables::minSheenRoughness));
        const auto alpha = perceptual * perceptual;
        const auto u = angle / 63.0;
        const auto nDotV = std::max (1.0 - u * u, 1.0e-4);
        const auto sinV = std::sqrt (1.0 - nDotV * nDotV);

        constexpr int steps = 1024;
        const auto dTheta = pi * 0.5 / steps;
        const auto dPhi = 2.0 * pi / steps;
        auto sum = 0.0;

        for (int i = 0; i < steps; ++i)
        {
            const auto theta = (i + 0.5) * dTheta;
            const auto nDotL = std::cos (theta);
            const auto sinL = std::sin (theta);
            const auto visibility = 1.0 / (4.0 * (nDotL + nDotV - nDotL * nDotV));

            for (int j = 0; j < steps; ++j)
            {
                const auto phi = (j + 0.5) * dPhi;
                const auto hx = sinL * std::cos (phi) + sinV;
                const auto hy = sinL * std::sin (phi);
                const auto hz = nDotL + nDotV;
                const auto cos2 = hz * hz / (hx * hx + hy * hy + hz * hz);
                const auto distribution = (2.0 + 1.0 / alpha) * std::pow (std::max (1.0 - cos2, 0.0), 0.5 / alpha) / (2.0 * pi);
                sum += distribution * visibility * nDotL * sinL;
            }
        }

        return sum * dTheta * dPhi;
    }
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

TEST_F (LookupTablesTests, SheenAlbedoMatchesAReferenceIntegration)
{
    const auto start = Time::getMillisecondCounterHiRes();
    const auto table = LookupTables::computeSheenAlbedo();
    RecordProperty ("sheenAlbedoMilliseconds", String (Time::getMillisecondCounterHiRes() - start, 2).toStdString());

    ASSERT_EQ (table.size(), static_cast<size_t> (LookupTables::size * LookupTables::size));

    // Rough to smooth, head-on to grazing, and the floored rows below the minimum roughness. Within
    // 1e-3, above the integration error of the table's 256 x 32 quadrature (6.6e-4 at most, at n.v = 1e-4)
    for (const auto& [r, a] : { std::pair { 63, 0 }, std::pair { 63, 40 }, std::pair { 63, 63 }, std::pair { 32, 32 },
                                std::pair { 16, 56 }, std::pair { 8, 60 }, std::pair { 0, 56 }, std::pair { 4, 30 } })
    {
        const auto expected = std::min (referenceSheenAlbedo (r, a), 1.0);
        EXPECT_NEAR (table[static_cast<size_t> (r * LookupTables::size + a)], expected, 1.0e-3) << "cell " << r << ", " << a;
    }

    // Not vacuous: the albedo of a rough sheen seen head-on, and its rise towards grazing
    EXPECT_NEAR (table[63 * LookupTables::size], 0.1414, 1.0e-3);
    EXPECT_GT (table[63 * LookupTables::size + 63], 0.6f);
}

TEST_F (LookupTablesTests, MinimumSheenRoughnessIsARowCenter)
{
    // The shaders floor the sheen roughness there and read the row it maps to: at a row center the
    // read is that row alone, not a blend with the next one, which is computed at a rougher sheen
    const auto row = LookupTables::minSheenRoughness * static_cast<float> (LookupTables::size - 1);
    EXPECT_NEAR (row, std::round (row), 1.0e-4f);
    EXPECT_GT (std::round (row), 0.0f);
}

TEST_F (LookupTablesTests, SheenAlbedoIsLimitedToOne)
{
    // The visibility of Neubelt and Pettineo is not energy conserving at grazing angles: a smooth
    // sheen seen edge-on reflects several times what it receives. The table holds at most 1, so
    // the layers under the sheen are dimmed at most to black
    EXPECT_GT (referenceSheenAlbedo (8, 63), 1.5);

    const auto table = LookupTables::computeSheenAlbedo();
    auto limited = 0;
    for (const auto value : table)
    {
        EXPECT_TRUE (value >= 0.0f && value <= 1.0f) << value;
        limited += value == 1.0f ? 1 : 0;
    }

    EXPECT_GT (limited, 0);
    EXPECT_EQ (table[8 * LookupTables::size + 63], 1.0f);
}
