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

#include <yup_3d/yup_3d.h>

#if YUP_LINUX
#include <SDL3/SDL.h>

#include <cstdlib>
#endif

#include <algorithm>
#include <array>
#include <bit>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <set>

using namespace yup;

class SceneRendererTests : public ::testing::Test
{
};

TEST_F (SceneRendererTests, UsesFourSamplesByDefault)
{
    const SceneRenderer renderer;

    EXPECT_EQ (renderer.getSampleCount(), 4);
    EXPECT_TRUE (renderer.getLastError().isEmpty());
}

TEST_F (SceneRendererTests, SampleCountIsAtLeastOne)
{
    SceneRenderer renderer;

    renderer.setSampleCount (8);
    EXPECT_EQ (renderer.getSampleCount(), 8);

    renderer.setSampleCount (0);
    EXPECT_EQ (renderer.getSampleCount(), 1);
}

TEST_F (SceneRendererTests, RenderWithoutDeviceFails)
{
    SceneRenderer renderer;
    Scene scene;

    EXPECT_EQ (renderer.render (nullptr, scene, 64, 64), nullptr);
    EXPECT_FALSE (renderer.getLastError().isEmpty());
}

/** CPU only: mirrors the AgX tone mapping of lib/color.glsl. */
class SceneRendererAgxMirrorTests : public ::testing::Test
{
public:
    /** Mirrors yupToneMapAgx and the sRGB encoding of lib/color.glsl, returning 8 bit sRGB levels. */
    static std::array<int, 3> toneMapAgx (const std::array<float, 3>& color)
    {
        static constexpr float inset[3][3] = { { 0.83947207f, 0.03456180f, 0.12596613f },
                                               { 0.02293524f, 0.94392547f, 0.03313929f },
                                               { 0.04314896f, 0.05587855f, 0.90097249f } };
        static constexpr float outset[3][3] = { { 1.12753300f, -0.00675187f, -0.12078112f },
                                                { -0.00651416f, 1.00599316f, 0.00052100f },
                                                { -0.03292859f, -0.03460768f, 1.06753627f } };
        static constexpr float polynomial[] = { -0.0046820379f, 0.66635377f, -2.4249506f, 16.346045f, -28.204515f, 19.31242f, -4.6886051f };

        std::array<float, 3> linear {};
        for (int i = 0; i < 3; ++i)
        {
            const auto shaped = inset[i][0] * color[0] + inset[i][1] * color[1] + inset[i][2] * color[2];
            const auto x = std::clamp ((std::log2 (std::max (shaped, 1.0e-10f) / 0.18f) + 5.0f) / 11.0f, 0.0f, 1.0f);

            auto y = 0.0f;
            for (int k = 6; k >= 0; --k)
                y = y * x + polynomial[k];

            y = std::clamp (y, 0.0f, 1.0f);
            linear[i] = y < 0.04045f ? y / 12.92f : std::pow ((y + 0.055f) / 1.055f, 2.4f);
        }

        std::array<int, 3> levels {};
        for (int i = 0; i < 3; ++i)
        {
            const auto c = std::clamp (outset[i][0] * linear[0] + outset[i][1] * linear[1] + outset[i][2] * linear[2], 0.0f, 1.0f);
            const auto encoded = c < 0.0031308f ? c * 12.92f : 1.055f * std::pow (c, 1.0f / 2.4f) - 0.055f;
            levels[i] = static_cast<int> (std::round (encoded * 255.0f));
        }

        return levels;
    }
};

TEST_F (SceneRendererAgxMirrorTests, MirrorMatchesFitScriptOnSaturatedRed)
{
    // Expected levels are agx() of tools/agx_fit.py for (1, 0, 0): 0.59121, 0.00821, 0.01525 display linear.
    // Grey inputs cannot catch a transposed inset or outset matrix, a saturated primary does.
    const auto levels = toneMapAgx ({ 1.0f, 0.0f, 0.0f });
    EXPECT_NEAR (levels[0], 202, 1);
    EXPECT_NEAR (levels[1], 22, 1);
    EXPECT_NEAR (levels[2], 33, 1);
}

/** CPU only: the area light references, by brute-force quadrature over sphere caps, and a mirror of
    lightsClippedCap in lib/lights.glsl checked against them. */
class SceneRendererAreaLightMathTests : public ::testing::Test
{
public:
    /** Mirror of lightsClippedCap: the clamped cosine over a cap divided by pi, from len = sin^2 of
        its angular radius and the cosine z of its axis with the normal. */
    static double clippedCap (double len, double z)
    {
        len = std::min (len, 0.9999);
        if (z * z >= len)
            return len * std::max (z, 0.0);

        const auto sinTheta = std::sqrt (1.0 - z * z);
        const auto x = std::sqrt (1.0 / len - 1.0);
        const auto y = std::clamp (-x * z / sinTheta, -1.0, 1.0);
        const auto s = sinTheta * std::sqrt (1.0 - y * y);
        return std::max ((z * std::acos (y) - x * s) * len + std::atan2 (s, x), 0.0) / MathConstants<double>::pi;
    }

    /** Integrates @p f (l) over the cap of angular radius @p sigma around the unit @p axis, with the normal along +z. */
    template <typename Function>
    static double integrateCap (std::array<double, 3> axis, double sigma, Function&& f)
    {
        // An orthonormal frame around the axis
        const std::array<double, 3> helper = std::abs (axis[2]) < 0.9 ? std::array<double, 3> { 0.0, 0.0, 1.0 } : std::array<double, 3> { 1.0, 0.0, 0.0 };
        std::array<double, 3> t1 { axis[1] * helper[2] - axis[2] * helper[1], axis[2] * helper[0] - axis[0] * helper[2], axis[0] * helper[1] - axis[1] * helper[0] };
        const auto length = std::sqrt (t1[0] * t1[0] + t1[1] * t1[1] + t1[2] * t1[2]);
        for (auto& value : t1)
            value /= length;

        const std::array<double, 3> t2 { axis[1] * t1[2] - axis[2] * t1[1], axis[2] * t1[0] - axis[0] * t1[2], axis[0] * t1[1] - axis[1] * t1[0] };

        constexpr int steps = 600;
        const auto du = sigma / steps;
        const auto dphi = MathConstants<double>::twoPi / steps;

        auto sum = 0.0;
        for (int i = 0; i < steps; ++i)
        {
            const auto u = (i + 0.5) * du;
            for (int j = 0; j < steps; ++j)
            {
                const auto phi = (j + 0.5) * dphi;
                std::array<double, 3> l;
                for (int k = 0; k < 3; ++k)
                    l[k] = std::cos (u) * axis[k] + std::sin (u) * (std::cos (phi) * t1[k] + std::sin (phi) * t2[k]);

                sum += f (l) * std::sin (u) * du * dphi;
            }
        }

        return sum;
    }

    /** The clamped cosine over a cap whose axis makes the angle @p theta with the normal, divided by pi. */
    static double capQuadrature (double sigma, double theta)
    {
        return integrateCap ({ std::sin (theta), 0.0, std::cos (theta) }, sigma, [] (const auto& l)
        {
            return std::max (l[2], 0.0);
        }) / MathConstants<double>::pi;
    }

    /** The GGX lobe of lib/bsdf.glsl without Fresnel, D V cos, integrated over a cap around @p axis,
        for the view (sqrt (1 - nDotV^2), 0, nDotV) and the GGX alpha @p alpha. */
    static double ggxCapQuadrature (double nDotV, double alpha, std::array<double, 3> axis, double sigma)
    {
        const std::array<double, 3> v { std::sqrt (1.0 - nDotV * nDotV), 0.0, nDotV };
        const auto alpha2 = alpha * alpha;

        return integrateCap (axis, sigma, [&] (const auto& l)
        {
            const auto nDotL = l[2];
            if (nDotL <= 0.0)
                return 0.0;

            std::array<double, 3> h { v[0] + l[0], v[1] + l[1], v[2] + l[2] };
            const auto nDotH = h[2] / std::sqrt (h[0] * h[0] + h[1] * h[1] + h[2] * h[2]);
            const auto d = nDotH * nDotH * (alpha2 - 1.0) + 1.0;
            const auto distribution = alpha2 / (MathConstants<double>::pi * d * d);
            const auto ggxV = nDotL * std::sqrt (nDotV * nDotV * (1.0 - alpha2) + alpha2);
            const auto ggxL = nDotV * std::sqrt (nDotL * nDotL * (1.0 - alpha2) + alpha2);
            return distribution * 0.5 / (ggxV + ggxL) * nDotL;
        });
    }
};

TEST_F (SceneRendererAreaLightMathTests, ClippedCapMatchesQuadrature)
{
    // Caps above, across and below the horizon, small to nearly a hemisphere
    for (const auto sigma : { 0.05, 0.2, 0.44, 0.8, 1.2 })
    {
        for (const auto theta : { 0.0, 0.5, 1.0, 1.3, 1.5, 1.57, 1.7, 2.0, 2.5 })
        {
            const auto len = std::sin (sigma) * std::sin (sigma);
            const auto exact = capQuadrature (sigma, theta);
            EXPECT_NEAR (clippedCap (len, std::cos (theta)), exact, 1.0e-4 * len + 1.0e-7) << "sigma " << sigma << ", theta " << theta;
        }
    }
}

/** CPU only: the reflection of the precompiled forward bundles. */
class SceneRendererBundleTests : public ::testing::Test
{
protected:
    struct Budget
    {
        int sampledImages = 0;
        int uniformBlocks = 0;
        std::map<uint32_t, int> texturesPerSet;
        std::map<uint32_t, int> uniformBlocksPerSet;
    };

    static ShaderBundle load (const uint8* data, size_t size)
    {
        auto bundle = ShaderBundle::loadFromData (data, size);
        EXPECT_TRUE (bundle.wasOk()) << bundle.getErrorMessage();
        return bundle.wasOk() ? std::move (bundle.getReference()) : ShaderBundle();
    }

    static ShaderBundle forwardPass()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ForwardPass.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassAreaLights()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ForwardPass_area_lights.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassSoftShadows()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ForwardPass_soft_shadows.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassAreaLightsSoftShadows()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ForwardPass_area_lights_soft_shadows.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ForwardPass_local_indirect.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassAreaLightsLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ForwardPass_area_lights_local_indirect.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassSoftShadowsLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ForwardPass_soft_shadows_local_indirect.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassAreaLightsSoftShadowsLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ForwardPass_area_lights_soft_shadows_local_indirect.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassFull()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ForwardPass_full.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassFullAreaLights()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ForwardPass_full_area_lights.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassFullSoftShadows()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ForwardPass_full_soft_shadows.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassFullAreaLightsSoftShadows()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ForwardPass_full_area_lights_soft_shadows.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle background()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_Background.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle shadowPass()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ShadowPass.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle shadowPassMasked()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ShadowPass_masked.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFull()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_LightingPass_full.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullAreaLights()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_LightingPass_full_area_lights.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullSoftShadows()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_LightingPass_full_soft_shadows.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullAreaLightsSoftShadows()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_LightingPass_full_area_lights_soft_shadows.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_LightingPass_full_local_indirect.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullAreaLightsLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_LightingPass_full_area_lights_local_indirect.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullSoftShadowsLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_LightingPass_full_soft_shadows_local_indirect.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullAreaLightsSoftShadowsLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_LightingPass_full_area_lights_soft_shadows_local_indirect.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ForwardPass_local_indirect_layers.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassAreaLightsLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ForwardPass_area_lights_local_indirect_layers.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassSoftShadowsLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ForwardPass_soft_shadows_local_indirect_layers.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassAreaLightsSoftShadowsLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ForwardPass_area_lights_soft_shadows_local_indirect_layers.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_LightingPass_full_local_indirect_layers.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullAreaLightsLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_LightingPass_full_area_lights_local_indirect_layers.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullSoftShadowsLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_LightingPass_full_soft_shadows_local_indirect_layers.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullAreaLightsSoftShadowsLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_LightingPass_full_area_lights_soft_shadows_local_indirect_layers.inc>
        };
        return load (data, sizeof (data));
    }

    /** True if @p resources declare a resource at @p set and @p binding. */
    static bool declares (const std::vector<ShaderReflection::ResourceBinding>& resources, uint32_t set, uint32_t binding)
    {
        return std::any_of (resources.begin(), resources.end(), [&] (const auto& resource)
        {
            return resource.set == set && resource.binding == binding;
        });
    }

    static ShaderBundle gbufferPassFull()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_GBufferPass_full.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle gtaoTrace()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_GtaoTrace.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle gtaoDenoise()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_GtaoDenoise.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle ssrTrace()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_SsrTrace.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle ssrResolve()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_SsrResolve.inc>
        };
        return load (data, sizeof (data));
    }

    static Budget countResources (const ShaderReflection& reflection)
    {
        Budget budget;

        for (const auto* images : { &reflection.sampledImages, &reflection.separateImages })
        {
            for (const auto& image : *images)
            {
                ++budget.sampledImages;
                ++budget.texturesPerSet[image.set];
            }
        }

        for (const auto& block : reflection.uniformBuffers)
        {
            ++budget.uniformBlocks;
            ++budget.uniformBlocksPerSet[block.set];
        }

        return budget;
    }

    static bool writesLocation (const ShaderReflection& reflection, uint32_t location)
    {
        return std::any_of (reflection.stageOutputs.begin(), reflection.stageOutputs.end(), [location] (const auto& output)
        {
            return output.location == location;
        });
    }
};

TEST_F (SceneRendererBundleTests, RasterBundlesStayWithinBudget)
{
    // The reflection counts what the shaders declare, so the budget check cannot pass on empty
    // lists. The shared lookups (the environment and the lookup tables) live in set 0 in every
    // variant, the local indirect variants adding the reflection, irradiance and visibility atlases
    // there, the ambient occlusion in set 1 and the probe list in set 2; set 3 holds the sun cascades and the point and spot
    // light atlas, read with the ShadowBlock. The soft shadow variants read both maps raw too
    for (const auto& [name, bundle, textures, lookups, passInputs, shadowMaps, blocks] : { std::tuple { "ForwardPass", forwardPass(), 9, 2, 0, 2, 5 },
                                                                                          std::tuple { "ForwardPass_area_lights", forwardPassAreaLights(), 9, 2, 0, 2, 5 },
                                                                                          std::tuple { "ForwardPass_soft_shadows", forwardPassSoftShadows(), 11, 2, 0, 4, 5 },
                                                                                          std::tuple { "ForwardPass_area_lights_soft_shadows", forwardPassAreaLightsSoftShadows(), 11, 2, 0, 4, 5 },
                                                                                          std::tuple { "ForwardPass_local_indirect", forwardPassLocalIndirect(), 13, 5, 1, 2, 6 },
                                                                                          std::tuple { "ForwardPass_area_lights_local_indirect", forwardPassAreaLightsLocalIndirect(), 13, 5, 1, 2, 6 },
                                                                                          std::tuple { "ForwardPass_soft_shadows_local_indirect", forwardPassSoftShadowsLocalIndirect(), 15, 5, 1, 4, 6 },
                                                                                          std::tuple { "ForwardPass_area_lights_soft_shadows_local_indirect", forwardPassAreaLightsSoftShadowsLocalIndirect(), 15, 5, 1, 4, 6 },
                                                                                          std::tuple { "ForwardPass_local_indirect_layers", forwardPassLocalIndirectLayers(), 13, 5, 1, 2, 6 },
                                                                                          std::tuple { "ForwardPass_area_lights_local_indirect_layers", forwardPassAreaLightsLocalIndirectLayers(), 13, 5, 1, 2, 6 },
                                                                                          std::tuple { "ForwardPass_soft_shadows_local_indirect_layers", forwardPassSoftShadowsLocalIndirectLayers(), 15, 5, 1, 4, 6 },
                                                                                          std::tuple { "ForwardPass_area_lights_soft_shadows_local_indirect_layers", forwardPassAreaLightsSoftShadowsLocalIndirectLayers(), 15, 5, 1, 4, 6 } })
    {
        const auto* vertex = bundle.findShader (ShaderStage::vertex, ShaderLanguage::spirv);
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (vertex, nullptr) << name;
        ASSERT_NE (fragment, nullptr) << name;

        auto budget = countResources (fragment->reflection);
        EXPECT_EQ (countResources (vertex->reflection).sampledImages, 0) << name;
        EXPECT_EQ (countResources (vertex->reflection).uniformBlocks, 2) << name;
        EXPECT_EQ (budget.sampledImages, textures) << name;
        EXPECT_EQ (budget.uniformBlocks, blocks) << name;
        EXPECT_EQ (budget.texturesPerSet[0], lookups) << name;
        EXPECT_EQ (budget.texturesPerSet[1], passInputs) << name;
        EXPECT_EQ (budget.texturesPerSet[3], shadowMaps) << name;
        EXPECT_EQ (budget.uniformBlocksPerSet[3], 1) << name;
    }

    // The full tier geometry bundles: the forward pass without the raster light list, and the G-buffer.
    // The forward pass reads the reflection, irradiance and visibility atlases in set 0 too. Its shadow filter variants read the
    // two maps raw too, but never the scene depth (set 1 binding 12): the blended items they draw
    // write none, so they march no contact shadows
    for (const auto& [name, bundle, textures, blocks, lookups] : { std::tuple { "ForwardPass_full", forwardPassFull(), 12, 4, 5 },
                                                                   std::tuple { "ForwardPass_full_area_lights", forwardPassFullAreaLights(), 12, 4, 5 },
                                                                   std::tuple { "ForwardPass_full_soft_shadows", forwardPassFullSoftShadows(), 14, 4, 5 },
                                                                   std::tuple { "ForwardPass_full_area_lights_soft_shadows", forwardPassFullAreaLightsSoftShadows(), 14, 4, 5 },
                                                                   std::tuple { "GBufferPass_full", gbufferPassFull(), 5, 1, 0 } })
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;

        auto budget = countResources (fragment->reflection);
        EXPECT_EQ (budget.sampledImages, textures) << name;
        EXPECT_EQ (budget.uniformBlocks, blocks) << name;
        EXPECT_EQ (budget.texturesPerSet[0], lookups) << name;
        EXPECT_FALSE (declares (fragment->reflection.separateImages, 1, 12)) << name;
    }

    const std::pair<const char*, ShaderBundle> bundles[] = {
        { "ForwardPass", forwardPass() },
        { "ForwardPass_area_lights", forwardPassAreaLights() },
        { "ForwardPass_soft_shadows", forwardPassSoftShadows() },
        { "ForwardPass_area_lights_soft_shadows", forwardPassAreaLightsSoftShadows() },
        { "ForwardPass_local_indirect", forwardPassLocalIndirect() },
        { "ForwardPass_local_indirect_layers", forwardPassLocalIndirectLayers() },
        { "ForwardPass_area_lights_local_indirect", forwardPassAreaLightsLocalIndirect() },
        { "ForwardPass_area_lights_local_indirect_layers", forwardPassAreaLightsLocalIndirectLayers() },
        { "ForwardPass_soft_shadows_local_indirect", forwardPassSoftShadowsLocalIndirect() },
        { "ForwardPass_soft_shadows_local_indirect_layers", forwardPassSoftShadowsLocalIndirectLayers() },
        { "ForwardPass_area_lights_soft_shadows_local_indirect", forwardPassAreaLightsSoftShadowsLocalIndirect() },
        { "ForwardPass_area_lights_soft_shadows_local_indirect_layers", forwardPassAreaLightsSoftShadowsLocalIndirectLayers() },
        { "ForwardPass_full", forwardPassFull() },
        { "ForwardPass_full_area_lights", forwardPassFullAreaLights() },
        { "ForwardPass_full_soft_shadows", forwardPassFullSoftShadows() },
        { "ForwardPass_full_area_lights_soft_shadows", forwardPassFullAreaLightsSoftShadows() },
        { "Background", background() },
        { "GBufferPass_full", gbufferPassFull() },
        { "LightingPass_full", lightingPassFull() },
        { "LightingPass_full_area_lights", lightingPassFullAreaLights() },
        { "LightingPass_full_soft_shadows", lightingPassFullSoftShadows() },
        { "LightingPass_full_area_lights_soft_shadows", lightingPassFullAreaLightsSoftShadows() },
        { "LightingPass_full_local_indirect", lightingPassFullLocalIndirect() },
        { "LightingPass_full_local_indirect_layers", lightingPassFullLocalIndirectLayers() },
        { "LightingPass_full_area_lights_local_indirect", lightingPassFullAreaLightsLocalIndirect() },
        { "LightingPass_full_area_lights_local_indirect_layers", lightingPassFullAreaLightsLocalIndirectLayers() },
        { "LightingPass_full_soft_shadows_local_indirect", lightingPassFullSoftShadowsLocalIndirect() },
        { "LightingPass_full_soft_shadows_local_indirect_layers", lightingPassFullSoftShadowsLocalIndirectLayers() },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect", lightingPassFullAreaLightsSoftShadowsLocalIndirect() },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect_layers", lightingPassFullAreaLightsSoftShadowsLocalIndirectLayers() },
        { "ShadowPass", shadowPass() },
        { "ShadowPass_masked", shadowPassMasked() },
        { "GtaoTrace", gtaoTrace() },
        { "GtaoDenoise", gtaoDenoise() },
    };

    for (const auto& [name, bundle] : bundles)
    {
        for (const auto stage : { ShaderStage::vertex, ShaderStage::fragment })
        {
            const auto* shader = bundle.findShader (stage, ShaderLanguage::spirv);
            ASSERT_NE (shader, nullptr) << name << " " << toString (stage);

            const auto budget = countResources (shader->reflection);
            EXPECT_LE (budget.sampledImages, 16) << name << " " << toString (stage);
            EXPECT_LE (budget.uniformBlocks, 12) << name << " " << toString (stage);

            for (const auto& [set, count] : budget.texturesPerSet)
                EXPECT_LE (count, 8) << name << " " << toString (stage) << " set " << (int) set;

            for (const auto& [set, count] : budget.uniformBlocksPerSet)
                EXPECT_LE (count, 8) << name << " " << toString (stage) << " set " << (int) set;
        }
    }
}

TEST_F (SceneRendererBundleTests, AmbientOcclusionIsReadByTheRasterForwardPassAndTheLightingPass)
{
    // Set 1 binding 13: on raster only the local indirect variants read it, so the default bundles
    // carry no ambient occlusion code; on full the lighting pass reads it for the opaque surfaces,
    // and the blended ones skip it
    const std::tuple<const char*, ShaderBundle, bool> bundles[] = {
        { "ForwardPass", forwardPass(), false },
        { "ForwardPass_area_lights", forwardPassAreaLights(), false },
        { "ForwardPass_soft_shadows", forwardPassSoftShadows(), false },
        { "ForwardPass_area_lights_soft_shadows", forwardPassAreaLightsSoftShadows(), false },
        { "ForwardPass_local_indirect", forwardPassLocalIndirect(), true },
        { "ForwardPass_local_indirect_layers", forwardPassLocalIndirectLayers(), true },
        { "ForwardPass_area_lights_local_indirect", forwardPassAreaLightsLocalIndirect(), true },
        { "ForwardPass_area_lights_local_indirect_layers", forwardPassAreaLightsLocalIndirectLayers(), true },
        { "ForwardPass_soft_shadows_local_indirect", forwardPassSoftShadowsLocalIndirect(), true },
        { "ForwardPass_soft_shadows_local_indirect_layers", forwardPassSoftShadowsLocalIndirectLayers(), true },
        { "ForwardPass_area_lights_soft_shadows_local_indirect", forwardPassAreaLightsSoftShadowsLocalIndirect(), true },
        { "ForwardPass_area_lights_soft_shadows_local_indirect_layers", forwardPassAreaLightsSoftShadowsLocalIndirectLayers(), true },
        { "ForwardPass_full", forwardPassFull(), false },
        { "ForwardPass_full_area_lights", forwardPassFullAreaLights(), false },
        { "ForwardPass_full_soft_shadows", forwardPassFullSoftShadows(), false },
        { "ForwardPass_full_area_lights_soft_shadows", forwardPassFullAreaLightsSoftShadows(), false },
        { "LightingPass_full", lightingPassFull(), true },
        { "LightingPass_full_area_lights", lightingPassFullAreaLights(), true },
        { "LightingPass_full_soft_shadows", lightingPassFullSoftShadows(), true },
        { "LightingPass_full_area_lights_soft_shadows", lightingPassFullAreaLightsSoftShadows(), true },
        { "LightingPass_full_local_indirect", lightingPassFullLocalIndirect(), true },
        { "LightingPass_full_local_indirect_layers", lightingPassFullLocalIndirectLayers(), true },
        { "LightingPass_full_area_lights_local_indirect", lightingPassFullAreaLightsLocalIndirect(), true },
        { "LightingPass_full_area_lights_local_indirect_layers", lightingPassFullAreaLightsLocalIndirectLayers(), true },
        { "LightingPass_full_soft_shadows_local_indirect", lightingPassFullSoftShadowsLocalIndirect(), true },
        { "LightingPass_full_soft_shadows_local_indirect_layers", lightingPassFullSoftShadowsLocalIndirectLayers(), true },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect", lightingPassFullAreaLightsSoftShadowsLocalIndirect(), true },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect_layers", lightingPassFullAreaLightsSoftShadowsLocalIndirectLayers(), true },
    };

    for (const auto& [name, bundle, reads] : bundles)
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;
        EXPECT_EQ (declares (fragment->reflection.separateImages, 1, 13), reads) << name;

        // Nor the specular occlusion of lib/probes.glsl, which every shading bundle includes, in the WGSL
        // that keeps unused functions
        const auto* wgsl = bundle.findShader (ShaderStage::fragment, ShaderLanguage::wgsl);
        ASSERT_NE (wgsl, nullptr) << name;
        EXPECT_EQ (wgsl->source.contains ("gtaoSpecularOcclusion"), reads) << name;
    }

    // The trace reads the depth and the normals (set 1 bindings 1 and 2), the denoise the trace too (3)
    for (const auto& [name, bundle, images] : { std::tuple { "GtaoTrace", gtaoTrace(), 2 }, std::tuple { "GtaoDenoise", gtaoDenoise(), 3 } })
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;

        auto budget = countResources (fragment->reflection);
        EXPECT_EQ (budget.texturesPerSet[1], images) << name;
        EXPECT_TRUE (declares (fragment->reflection.separateImages, 1, 1)) << name;
        EXPECT_TRUE (declares (fragment->reflection.separateImages, 1, 2)) << name;
        EXPECT_TRUE (declares (fragment->reflection.uniformBuffers, 1, 0)) << name;
    }
}

TEST_F (SceneRendererBundleTests, ScreenSpaceReflectionsAreReadByTheLightingPassOnly)
{
    // Set 1 binding 16, in every lighting pass variant: the full tier's opaque surfaces. The
    // blended ones (the full forward pass) and the raster tier never read it, nor any other bundle
    const std::tuple<const char*, ShaderBundle, bool> bundles[] = {
        { "ForwardPass", forwardPass(), false },
        { "ForwardPass_area_lights_soft_shadows", forwardPassAreaLightsSoftShadows(), false },
        { "ForwardPass_local_indirect", forwardPassLocalIndirect(), false },
        { "ForwardPass_area_lights_soft_shadows_local_indirect_layers", forwardPassAreaLightsSoftShadowsLocalIndirectLayers(), false },
        { "ForwardPass_full", forwardPassFull(), false },
        { "ForwardPass_full_area_lights_soft_shadows", forwardPassFullAreaLightsSoftShadows(), false },
        { "Background", background(), false },
        { "GBufferPass_full", gbufferPassFull(), false },
        { "LightingPass_full", lightingPassFull(), true },
        { "LightingPass_full_area_lights", lightingPassFullAreaLights(), true },
        { "LightingPass_full_soft_shadows", lightingPassFullSoftShadows(), true },
        { "LightingPass_full_area_lights_soft_shadows", lightingPassFullAreaLightsSoftShadows(), true },
        { "LightingPass_full_local_indirect", lightingPassFullLocalIndirect(), true },
        { "LightingPass_full_local_indirect_layers", lightingPassFullLocalIndirectLayers(), true },
        { "LightingPass_full_area_lights_local_indirect", lightingPassFullAreaLightsLocalIndirect(), true },
        { "LightingPass_full_area_lights_local_indirect_layers", lightingPassFullAreaLightsLocalIndirectLayers(), true },
        { "LightingPass_full_soft_shadows_local_indirect", lightingPassFullSoftShadowsLocalIndirect(), true },
        { "LightingPass_full_soft_shadows_local_indirect_layers", lightingPassFullSoftShadowsLocalIndirectLayers(), true },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect", lightingPassFullAreaLightsSoftShadowsLocalIndirect(), true },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect_layers", lightingPassFullAreaLightsSoftShadowsLocalIndirectLayers(), true },
    };

    for (const auto& [name, bundle, reads] : bundles)
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;
        EXPECT_EQ (declares (fragment->reflection.separateImages, 1, 16), reads) << name;
    }

    // The trace reads the Hi-Z pyramid and the normals (set 1 bindings 1 and 2), the resolve the
    // trace, the velocity and the previous scene color too (3 to 5); both read the pass block (0)
    for (const auto& [name, bundle, images] : { std::tuple { "SsrTrace", ssrTrace(), 2 }, std::tuple { "SsrResolve", ssrResolve(), 5 } })
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;

        auto budget = countResources (fragment->reflection);
        EXPECT_EQ (budget.texturesPerSet[1], images) << name;
        EXPECT_TRUE (declares (fragment->reflection.uniformBuffers, 0, 0)) << name;
        EXPECT_TRUE (declares (fragment->reflection.uniformBuffers, 1, 0)) << name;

        for (uint32_t binding = 1; binding <= static_cast<uint32_t> (images); ++binding)
            EXPECT_TRUE (declares (fragment->reflection.separateImages, 1, binding)) << name << " " << static_cast<int> (binding);
    }
}

TEST_F (SceneRendererBundleTests, ReflectionProbesAreReadWhereProbesShade)
{
    // The reflection atlas at set 0 binding 8, and the probes: on raster the per-draw ProbeListBlock
    // (set 2 binding 13, two GpuReflectionProbe, a GpuIrradianceVolume and a count, 384 bytes) in the local indirect
    // variants only, so the default bundles carry no probe code; on full a storage buffer of every
    // probe of the frame, set 1 binding 8 in the forward pass and 14 in the local indirect variants
    // of the lighting pass (preflight R27), its default variants carrying no probe code either
    enum class Reads { nothing, perDraw, storage };

    const std::tuple<const char*, ShaderBundle, Reads, uint32_t> bundles[] = {
        { "ForwardPass", forwardPass(), Reads::nothing, 0 },
        { "ForwardPass_area_lights", forwardPassAreaLights(), Reads::nothing, 0 },
        { "ForwardPass_soft_shadows", forwardPassSoftShadows(), Reads::nothing, 0 },
        { "ForwardPass_area_lights_soft_shadows", forwardPassAreaLightsSoftShadows(), Reads::nothing, 0 },
        { "Background", background(), Reads::nothing, 0 },
        { "ForwardPass_local_indirect", forwardPassLocalIndirect(), Reads::perDraw, 0 },
        { "ForwardPass_local_indirect_layers", forwardPassLocalIndirectLayers(), Reads::perDraw, 0 },
        { "ForwardPass_area_lights_local_indirect", forwardPassAreaLightsLocalIndirect(), Reads::perDraw, 0 },
        { "ForwardPass_area_lights_local_indirect_layers", forwardPassAreaLightsLocalIndirectLayers(), Reads::perDraw, 0 },
        { "ForwardPass_soft_shadows_local_indirect", forwardPassSoftShadowsLocalIndirect(), Reads::perDraw, 0 },
        { "ForwardPass_soft_shadows_local_indirect_layers", forwardPassSoftShadowsLocalIndirectLayers(), Reads::perDraw, 0 },
        { "ForwardPass_area_lights_soft_shadows_local_indirect", forwardPassAreaLightsSoftShadowsLocalIndirect(), Reads::perDraw, 0 },
        { "ForwardPass_area_lights_soft_shadows_local_indirect_layers", forwardPassAreaLightsSoftShadowsLocalIndirectLayers(), Reads::perDraw, 0 },
        { "ForwardPass_full", forwardPassFull(), Reads::storage, 8 },
        { "ForwardPass_full_area_lights", forwardPassFullAreaLights(), Reads::storage, 8 },
        { "ForwardPass_full_soft_shadows", forwardPassFullSoftShadows(), Reads::storage, 8 },
        { "ForwardPass_full_area_lights_soft_shadows", forwardPassFullAreaLightsSoftShadows(), Reads::storage, 8 },
        { "LightingPass_full", lightingPassFull(), Reads::nothing, 0 },
        { "LightingPass_full_area_lights", lightingPassFullAreaLights(), Reads::nothing, 0 },
        { "LightingPass_full_soft_shadows", lightingPassFullSoftShadows(), Reads::nothing, 0 },
        { "LightingPass_full_area_lights_soft_shadows", lightingPassFullAreaLightsSoftShadows(), Reads::nothing, 0 },
        { "LightingPass_full_local_indirect", lightingPassFullLocalIndirect(), Reads::storage, 14 },
        { "LightingPass_full_local_indirect_layers", lightingPassFullLocalIndirectLayers(), Reads::storage, 14 },
        { "LightingPass_full_area_lights_local_indirect", lightingPassFullAreaLightsLocalIndirect(), Reads::storage, 14 },
        { "LightingPass_full_area_lights_local_indirect_layers", lightingPassFullAreaLightsLocalIndirectLayers(), Reads::storage, 14 },
        { "LightingPass_full_soft_shadows_local_indirect", lightingPassFullSoftShadowsLocalIndirect(), Reads::storage, 14 },
        { "LightingPass_full_soft_shadows_local_indirect_layers", lightingPassFullSoftShadowsLocalIndirectLayers(), Reads::storage, 14 },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect", lightingPassFullAreaLightsSoftShadowsLocalIndirect(), Reads::storage, 14 },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect_layers", lightingPassFullAreaLightsSoftShadowsLocalIndirectLayers(), Reads::storage, 14 },
    };

    for (const auto& [name, bundle, reads, storageBinding] : bundles)
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;

        const auto& reflection = fragment->reflection;
        EXPECT_EQ (declares (reflection.separateImages, 0, 8), reads != Reads::nothing) << name;

        const auto& blocks = reflection.uniformBuffers;
        const auto probeList = std::find_if (blocks.begin(), blocks.end(), [] (const auto& b) { return b.set == 2 && b.binding == 13; });
        EXPECT_EQ (probeList != blocks.end(), reads == Reads::perDraw) << name;
        if (probeList != blocks.end())
            EXPECT_EQ (probeList->blockSize, static_cast<uint32_t> (2 * sizeof (GpuReflectionProbe) + sizeof (GpuIrradianceVolume) + 16)) << name;

        const auto& buffers = reflection.storageBuffers;
        const auto probes = std::find_if (buffers.begin(), buffers.end(), [binding = storageBinding] (const auto& b) { return b.set == 1 && b.binding == binding; });
        EXPECT_EQ (probes != buffers.end(), reads == Reads::storage) << name;
        if (probes != buffers.end())
            EXPECT_TRUE (probes->readOnly) << name;

        // Nor the probe functions of lib/probes.glsl, in the WGSL that keeps unused functions
        const auto* wgsl = bundle.findShader (ShaderStage::fragment, ShaderLanguage::wgsl);
        ASSERT_NE (wgsl, nullptr) << name;
        EXPECT_EQ (wgsl->source.contains ("reflectionProbeWeight"), reads != Reads::nothing) << name;
    }
}

TEST_F (SceneRendererBundleTests, IrradianceVolumesAreReadWhereProbesShade)
{
    // The irradiance and visibility atlases at set 0 bindings 9 and 10, and the volumes: on raster
    // the draw's volume in the per-draw ProbeListBlock (set 2 binding 13) of the local indirect
    // variants only, so the default bundles carry no volume code; on full a storage buffer of the
    // volumes of the frame, set 1 binding 9 in the forward pass and 15 in the local indirect variants
    // of the lighting pass (preflight R27), its default variants carrying none
    enum class Reads { nothing, perDraw, storage };

    const std::tuple<const char*, ShaderBundle, Reads, uint32_t> bundles[] = {
        { "ForwardPass", forwardPass(), Reads::nothing, 0 },
        { "ForwardPass_area_lights", forwardPassAreaLights(), Reads::nothing, 0 },
        { "ForwardPass_soft_shadows", forwardPassSoftShadows(), Reads::nothing, 0 },
        { "ForwardPass_area_lights_soft_shadows", forwardPassAreaLightsSoftShadows(), Reads::nothing, 0 },
        { "Background", background(), Reads::nothing, 0 },
        { "ForwardPass_local_indirect", forwardPassLocalIndirect(), Reads::perDraw, 0 },
        { "ForwardPass_local_indirect_layers", forwardPassLocalIndirectLayers(), Reads::perDraw, 0 },
        { "ForwardPass_area_lights_local_indirect", forwardPassAreaLightsLocalIndirect(), Reads::perDraw, 0 },
        { "ForwardPass_area_lights_local_indirect_layers", forwardPassAreaLightsLocalIndirectLayers(), Reads::perDraw, 0 },
        { "ForwardPass_soft_shadows_local_indirect", forwardPassSoftShadowsLocalIndirect(), Reads::perDraw, 0 },
        { "ForwardPass_soft_shadows_local_indirect_layers", forwardPassSoftShadowsLocalIndirectLayers(), Reads::perDraw, 0 },
        { "ForwardPass_area_lights_soft_shadows_local_indirect", forwardPassAreaLightsSoftShadowsLocalIndirect(), Reads::perDraw, 0 },
        { "ForwardPass_area_lights_soft_shadows_local_indirect_layers", forwardPassAreaLightsSoftShadowsLocalIndirectLayers(), Reads::perDraw, 0 },
        { "ForwardPass_full", forwardPassFull(), Reads::storage, 9 },
        { "ForwardPass_full_area_lights", forwardPassFullAreaLights(), Reads::storage, 9 },
        { "ForwardPass_full_soft_shadows", forwardPassFullSoftShadows(), Reads::storage, 9 },
        { "ForwardPass_full_area_lights_soft_shadows", forwardPassFullAreaLightsSoftShadows(), Reads::storage, 9 },
        { "LightingPass_full", lightingPassFull(), Reads::nothing, 0 },
        { "LightingPass_full_area_lights", lightingPassFullAreaLights(), Reads::nothing, 0 },
        { "LightingPass_full_soft_shadows", lightingPassFullSoftShadows(), Reads::nothing, 0 },
        { "LightingPass_full_area_lights_soft_shadows", lightingPassFullAreaLightsSoftShadows(), Reads::nothing, 0 },
        { "LightingPass_full_local_indirect", lightingPassFullLocalIndirect(), Reads::storage, 15 },
        { "LightingPass_full_local_indirect_layers", lightingPassFullLocalIndirectLayers(), Reads::storage, 15 },
        { "LightingPass_full_area_lights_local_indirect", lightingPassFullAreaLightsLocalIndirect(), Reads::storage, 15 },
        { "LightingPass_full_area_lights_local_indirect_layers", lightingPassFullAreaLightsLocalIndirectLayers(), Reads::storage, 15 },
        { "LightingPass_full_soft_shadows_local_indirect", lightingPassFullSoftShadowsLocalIndirect(), Reads::storage, 15 },
        { "LightingPass_full_soft_shadows_local_indirect_layers", lightingPassFullSoftShadowsLocalIndirectLayers(), Reads::storage, 15 },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect", lightingPassFullAreaLightsSoftShadowsLocalIndirect(), Reads::storage, 15 },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect_layers", lightingPassFullAreaLightsSoftShadowsLocalIndirectLayers(), Reads::storage, 15 },
    };

    for (const auto& [name, bundle, reads, storageBinding] : bundles)
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;

        const auto& reflection = fragment->reflection;
        EXPECT_EQ (declares (reflection.separateImages, 0, 9), reads != Reads::nothing) << name;
        EXPECT_EQ (declares (reflection.separateImages, 0, 10), reads != Reads::nothing) << name;

        const auto& buffers = reflection.storageBuffers;
        const auto volumes = std::find_if (buffers.begin(), buffers.end(), [binding = storageBinding] (const auto& b) { return b.set == 1 && b.binding == binding; });
        EXPECT_EQ (volumes != buffers.end(), reads == Reads::storage) << name;
        if (volumes != buffers.end())
        {
            EXPECT_TRUE (volumes->readOnly) << name;
            ASSERT_EQ (volumes->members.size(), 1u) << name;
            EXPECT_EQ (volumes->members[0].arrayStride, static_cast<uint32_t> (sizeof (GpuIrradianceVolume))) << name;
        }

        // Nor the volume functions of lib/probes.glsl, in the WGSL that keeps unused functions
        const auto* wgsl = bundle.findShader (ShaderStage::fragment, ShaderLanguage::wgsl);
        ASSERT_NE (wgsl, nullptr) << name;
        EXPECT_EQ (wgsl->source.contains ("irradianceVolumeWeight"), reads != Reads::nothing) << name;
    }
}

TEST_F (SceneRendererBundleTests, OnlyFullVariantsWriteVelocity)
{
    const auto fragmentOf = [] (const ShaderBundle& bundle) -> const ShaderReflection*
    {
        const auto* shader = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        return shader != nullptr ? &shader->reflection : nullptr;
    };

    const auto raster = forwardPass();
    const auto rasterAreaLights = forwardPassAreaLights();
    const auto rasterLocalIndirect = forwardPassLocalIndirect();
    const auto rasterAreaLightsLocalIndirect = forwardPassAreaLightsLocalIndirect();
    const auto full = forwardPassFull();
    const auto fullAreaLights = forwardPassFullAreaLights();
    const auto fullSoftShadows = forwardPassFullSoftShadows();
    const auto fullAreaLightsSoftShadows = forwardPassFullAreaLightsSoftShadows();
    const auto rasterBackground = background();

    for (const auto* reflection : { fragmentOf (raster), fragmentOf (rasterAreaLights), fragmentOf (rasterLocalIndirect), fragmentOf (rasterAreaLightsLocalIndirect), fragmentOf (rasterBackground) })
    {
        ASSERT_NE (reflection, nullptr);
        EXPECT_TRUE (writesLocation (*reflection, 0));
        EXPECT_FALSE (writesLocation (*reflection, 1));
    }

    // Blended items keep writing their motion over the deferred surfaces
    for (const auto* reflection : { fragmentOf (full), fragmentOf (fullAreaLights), fragmentOf (fullSoftShadows), fragmentOf (fullAreaLightsSoftShadows) })
    {
        ASSERT_NE (reflection, nullptr);
        EXPECT_TRUE (writesLocation (*reflection, 0));
        EXPECT_TRUE (writesLocation (*reflection, 1));
    }
}

TEST_F (SceneRendererBundleTests, MaterialBlockEndsWithTheGpuMaterial)
{
    // The bundles compiling the clearcoat and sheen layers (preflight R29: the raster local indirect
    // layers variants and the full forward pass) declare the whole block;
    // the others, which never read the layers, leave out their two rows, so their code is the same
    // as before the layers. Every draw binds the whole block
    const std::tuple<const char*, ShaderBundle, bool> bundles[] = {
        { "ForwardPass", forwardPass(), false },
        { "ForwardPass_area_lights", forwardPassAreaLights(), false },
        { "ForwardPass_soft_shadows", forwardPassSoftShadows(), false },
        { "ForwardPass_area_lights_soft_shadows", forwardPassAreaLightsSoftShadows(), false },
        { "ShadowPass_masked", shadowPassMasked(), false },
        { "GBufferPass_full", gbufferPassFull(), false },
        { "ForwardPass_local_indirect", forwardPassLocalIndirect(), false },
        { "ForwardPass_local_indirect_layers", forwardPassLocalIndirectLayers(), true },
        { "ForwardPass_area_lights_local_indirect", forwardPassAreaLightsLocalIndirect(), false },
        { "ForwardPass_area_lights_local_indirect_layers", forwardPassAreaLightsLocalIndirectLayers(), true },
        { "ForwardPass_soft_shadows_local_indirect", forwardPassSoftShadowsLocalIndirect(), false },
        { "ForwardPass_soft_shadows_local_indirect_layers", forwardPassSoftShadowsLocalIndirectLayers(), true },
        { "ForwardPass_area_lights_soft_shadows_local_indirect", forwardPassAreaLightsSoftShadowsLocalIndirect(), false },
        { "ForwardPass_area_lights_soft_shadows_local_indirect_layers", forwardPassAreaLightsSoftShadowsLocalIndirectLayers(), true },
        { "ForwardPass_full", forwardPassFull(), true },
        { "ForwardPass_full_area_lights", forwardPassFullAreaLights(), true },
        { "ForwardPass_full_soft_shadows", forwardPassFullSoftShadows(), true },
        { "ForwardPass_full_area_lights_soft_shadows", forwardPassFullAreaLightsSoftShadows(), true },
    };

    for (const auto& [name, bundle, layers] : bundles)
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;

        const auto& blocks = fragment->reflection.uniformBuffers;
        const auto block = std::find_if (blocks.begin(), blocks.end(), [] (const auto& b) { return b.set == 2 && b.binding == 1; });
        ASSERT_NE (block, blocks.end()) << name;

        const auto extension = static_cast<uint32_t> (offsetof (GeometryBindings::MaterialBlock, extension));
        EXPECT_EQ (block->blockSize, layers ? static_cast<uint32_t> (sizeof (GeometryBindings::MaterialBlock)) : extension + static_cast<uint32_t> (offsetof (GpuMaterial, clearcoat))) << name;

        std::map<String, uint32_t> offsets;
        for (const auto& member : block->members)
            offsets[member.name] = member.offset;

        EXPECT_EQ (offsets["specular"], extension) << name;
        EXPECT_EQ (offsets["extended"], extension + static_cast<uint32_t> (offsetof (GpuMaterial, extended))) << name;
        EXPECT_EQ (offsets.count ("clearcoat"), layers ? 1u : 0u) << name;
        EXPECT_EQ (offsets.count ("sheen"), layers ? 1u : 0u) << name;

        if (layers)
        {
            EXPECT_EQ (offsets["clearcoat"], extension + static_cast<uint32_t> (offsetof (GpuMaterial, clearcoat))) << name;
            EXPECT_EQ (offsets["sheen"], extension + static_cast<uint32_t> (offsetof (GpuMaterial, sheen))) << name;
        }
    }
}

TEST_F (SceneRendererBundleTests, MaterialLayersCompileOnlyWhereTheLayersShade)
{
    // Preflight R29: the clearcoat and sheen lobes compile into the local indirect layers variants of
    // the raster forward pass and of the lighting pass, and into the full forward pass; the default
    // and plain local indirect variants carry none of their code. Every lighting pass reads the
    // material table at the GpuMaterial stride
    const std::tuple<const char*, ShaderBundle, bool> bundles[] = {
        { "ForwardPass", forwardPass(), false },
        { "ForwardPass_area_lights", forwardPassAreaLights(), false },
        { "ForwardPass_soft_shadows", forwardPassSoftShadows(), false },
        { "ForwardPass_area_lights_soft_shadows", forwardPassAreaLightsSoftShadows(), false },
        { "ForwardPass_local_indirect", forwardPassLocalIndirect(), false },
        { "ForwardPass_local_indirect_layers", forwardPassLocalIndirectLayers(), true },
        { "ForwardPass_area_lights_local_indirect", forwardPassAreaLightsLocalIndirect(), false },
        { "ForwardPass_area_lights_local_indirect_layers", forwardPassAreaLightsLocalIndirectLayers(), true },
        { "ForwardPass_soft_shadows_local_indirect", forwardPassSoftShadowsLocalIndirect(), false },
        { "ForwardPass_soft_shadows_local_indirect_layers", forwardPassSoftShadowsLocalIndirectLayers(), true },
        { "ForwardPass_area_lights_soft_shadows_local_indirect", forwardPassAreaLightsSoftShadowsLocalIndirect(), false },
        { "ForwardPass_area_lights_soft_shadows_local_indirect_layers", forwardPassAreaLightsSoftShadowsLocalIndirectLayers(), true },
        { "ForwardPass_full", forwardPassFull(), true },
        { "ForwardPass_full_area_lights", forwardPassFullAreaLights(), true },
        { "ForwardPass_full_soft_shadows", forwardPassFullSoftShadows(), true },
        { "ForwardPass_full_area_lights_soft_shadows", forwardPassFullAreaLightsSoftShadows(), true },
        { "LightingPass_full", lightingPassFull(), false },
        { "LightingPass_full_area_lights", lightingPassFullAreaLights(), false },
        { "LightingPass_full_soft_shadows", lightingPassFullSoftShadows(), false },
        { "LightingPass_full_area_lights_soft_shadows", lightingPassFullAreaLightsSoftShadows(), false },
        { "LightingPass_full_local_indirect", lightingPassFullLocalIndirect(), false },
        { "LightingPass_full_local_indirect_layers", lightingPassFullLocalIndirectLayers(), true },
        { "LightingPass_full_area_lights_local_indirect", lightingPassFullAreaLightsLocalIndirect(), false },
        { "LightingPass_full_area_lights_local_indirect_layers", lightingPassFullAreaLightsLocalIndirectLayers(), true },
        { "LightingPass_full_soft_shadows_local_indirect", lightingPassFullSoftShadowsLocalIndirect(), false },
        { "LightingPass_full_soft_shadows_local_indirect_layers", lightingPassFullSoftShadowsLocalIndirectLayers(), true },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect", lightingPassFullAreaLightsSoftShadowsLocalIndirect(), false },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect_layers", lightingPassFullAreaLightsSoftShadowsLocalIndirectLayers(), true },
        { "Background", background(), false },
        { "GBufferPass_full", gbufferPassFull(), false },
    };

    for (const auto& [name, bundle, layers] : bundles)
    {
        // The WGSL keeps unused functions, so a lobe compiled anywhere shows
        const auto* wgsl = bundle.findShader (ShaderStage::fragment, ShaderLanguage::wgsl);
        ASSERT_NE (wgsl, nullptr) << name;
        EXPECT_EQ (wgsl->source.contains ("bsdfDistributionCharlie"), layers) << name;
        EXPECT_EQ (wgsl->source.contains ("bsdfEvaluateClearcoat"), layers) << name;

        if (! String (name).startsWith ("LightingPass"))
            continue;

        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;

        const auto& buffers = fragment->reflection.storageBuffers;
        const auto materials = std::find_if (buffers.begin(), buffers.end(), [] (const auto& b) { return b.set == 1 && b.binding == 8; });
        ASSERT_NE (materials, buffers.end()) << name;
        ASSERT_EQ (materials->members.size(), 1u) << name;
        EXPECT_EQ (materials->members[0].arrayStride, static_cast<uint32_t> (sizeof (GpuMaterial))) << name;
    }
}

TEST_F (SceneRendererBundleTests, LightingBlockIsSharedByEveryShadingBundle)
{
    // One declaration in lib/lighting.glsl: the background reads environmentInfo where the surfaces do
    const std::pair<const char*, ShaderBundle> bundles[] = {
        { "ForwardPass", forwardPass() },
        { "ForwardPass_area_lights", forwardPassAreaLights() },
        { "ForwardPass_soft_shadows", forwardPassSoftShadows() },
        { "ForwardPass_area_lights_soft_shadows", forwardPassAreaLightsSoftShadows() },
        { "ForwardPass_local_indirect", forwardPassLocalIndirect() },
        { "ForwardPass_local_indirect_layers", forwardPassLocalIndirectLayers() },
        { "ForwardPass_area_lights_local_indirect", forwardPassAreaLightsLocalIndirect() },
        { "ForwardPass_area_lights_local_indirect_layers", forwardPassAreaLightsLocalIndirectLayers() },
        { "ForwardPass_soft_shadows_local_indirect", forwardPassSoftShadowsLocalIndirect() },
        { "ForwardPass_soft_shadows_local_indirect_layers", forwardPassSoftShadowsLocalIndirectLayers() },
        { "ForwardPass_area_lights_soft_shadows_local_indirect", forwardPassAreaLightsSoftShadowsLocalIndirect() },
        { "ForwardPass_area_lights_soft_shadows_local_indirect_layers", forwardPassAreaLightsSoftShadowsLocalIndirectLayers() },
        { "ForwardPass_full", forwardPassFull() },
        { "ForwardPass_full_area_lights", forwardPassFullAreaLights() },
        { "ForwardPass_full_soft_shadows", forwardPassFullSoftShadows() },
        { "ForwardPass_full_area_lights_soft_shadows", forwardPassFullAreaLightsSoftShadows() },
        { "Background", background() },
        { "LightingPass_full", lightingPassFull() },
        { "LightingPass_full_area_lights", lightingPassFullAreaLights() },
        { "LightingPass_full_soft_shadows", lightingPassFullSoftShadows() },
        { "LightingPass_full_area_lights_soft_shadows", lightingPassFullAreaLightsSoftShadows() },
        { "LightingPass_full_local_indirect", lightingPassFullLocalIndirect() },
        { "LightingPass_full_local_indirect_layers", lightingPassFullLocalIndirectLayers() },
        { "LightingPass_full_area_lights_local_indirect", lightingPassFullAreaLightsLocalIndirect() },
        { "LightingPass_full_area_lights_local_indirect_layers", lightingPassFullAreaLightsLocalIndirectLayers() },
        { "LightingPass_full_soft_shadows_local_indirect", lightingPassFullSoftShadowsLocalIndirect() },
        { "LightingPass_full_soft_shadows_local_indirect_layers", lightingPassFullSoftShadowsLocalIndirectLayers() },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect", lightingPassFullAreaLightsSoftShadowsLocalIndirect() },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect_layers", lightingPassFullAreaLightsSoftShadowsLocalIndirectLayers() },
    };

    for (const auto& [name, bundle] : bundles)
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;

        const auto& blocks = fragment->reflection.uniformBuffers;
        const auto block = std::find_if (blocks.begin(), blocks.end(), [] (const auto& b) { return b.set == 1 && b.binding == 0; });
        ASSERT_NE (block, blocks.end()) << name;
        EXPECT_EQ (block->blockSize, 224u) << name;

        std::map<String, uint32_t> offsets;
        for (const auto& member : block->members)
            offsets[member.name] = member.offset;

        // The shadow data lives in the ShadowBlock of set 3 since Phase 3
        EXPECT_EQ (offsets.count ("shadowMatrix"), 0u) << name;
        EXPECT_EQ (offsets.count ("shadowInfo"), 0u) << name;
        EXPECT_EQ (offsets["environmentInfo"], 16u) << name;
        EXPECT_EQ (offsets["irradiance"], 32u) << name;
        EXPECT_EQ (offsets["clusterGrid"], 176u) << name;
        EXPECT_EQ (offsets["clusterDepth"], 192u) << name;
        EXPECT_EQ (offsets["probeInfo"], 208u) << name;
    }
}

TEST_F (SceneRendererBundleTests, ShadowBlockIsSharedByEveryShadingBundle)
{
    // One declaration in lib/shadows.glsl, mirrored by GpuShadowData: the ShadowBlock at set 3
    // binding 4, the sun cascades at set 3 binding 0 and the point and spot light atlas at set 3
    // binding 2, both compared through shadowCompare at set 0 binding 4. WebGPU needs them as depth
    // textures (an array and a 2D one) and a comparison sampler, and Metal gives every texture and
    // every sampler of a stage its own slot, whatever its set. Only the shadow filter variants
    // (YUP_3D_SOFT_SHADOWS) declare the raw depth maps
    const std::tuple<const char*, ShaderBundle, bool> bundles[] = {
        { "ForwardPass", forwardPass(), false },
        { "ForwardPass_area_lights", forwardPassAreaLights(), false },
        { "ForwardPass_soft_shadows", forwardPassSoftShadows(), true },
        { "ForwardPass_area_lights_soft_shadows", forwardPassAreaLightsSoftShadows(), true },
        { "ForwardPass_local_indirect", forwardPassLocalIndirect(), false },
        { "ForwardPass_local_indirect_layers", forwardPassLocalIndirectLayers(), false },
        { "ForwardPass_area_lights_local_indirect", forwardPassAreaLightsLocalIndirect(), false },
        { "ForwardPass_area_lights_local_indirect_layers", forwardPassAreaLightsLocalIndirectLayers(), false },
        { "ForwardPass_soft_shadows_local_indirect", forwardPassSoftShadowsLocalIndirect(), true },
        { "ForwardPass_soft_shadows_local_indirect_layers", forwardPassSoftShadowsLocalIndirectLayers(), true },
        { "ForwardPass_area_lights_soft_shadows_local_indirect", forwardPassAreaLightsSoftShadowsLocalIndirect(), true },
        { "ForwardPass_area_lights_soft_shadows_local_indirect_layers", forwardPassAreaLightsSoftShadowsLocalIndirectLayers(), true },
        { "ForwardPass_full", forwardPassFull(), false },
        { "ForwardPass_full_area_lights", forwardPassFullAreaLights(), false },
        { "LightingPass_full", lightingPassFull(), false },
        { "LightingPass_full_area_lights", lightingPassFullAreaLights(), false },
        { "ForwardPass_full_soft_shadows", forwardPassFullSoftShadows(), true },
        { "ForwardPass_full_area_lights_soft_shadows", forwardPassFullAreaLightsSoftShadows(), true },
        { "LightingPass_full_soft_shadows", lightingPassFullSoftShadows(), true },
        { "LightingPass_full_area_lights_soft_shadows", lightingPassFullAreaLightsSoftShadows(), true },
        { "LightingPass_full_local_indirect", lightingPassFullLocalIndirect(), false },
        { "LightingPass_full_local_indirect_layers", lightingPassFullLocalIndirectLayers(), false },
        { "LightingPass_full_area_lights_local_indirect", lightingPassFullAreaLightsLocalIndirect(), false },
        { "LightingPass_full_area_lights_local_indirect_layers", lightingPassFullAreaLightsLocalIndirectLayers(), false },
        { "LightingPass_full_soft_shadows_local_indirect", lightingPassFullSoftShadowsLocalIndirect(), true },
        { "LightingPass_full_soft_shadows_local_indirect_layers", lightingPassFullSoftShadowsLocalIndirectLayers(), true },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect", lightingPassFullAreaLightsSoftShadowsLocalIndirect(), true },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect_layers", lightingPassFullAreaLightsSoftShadowsLocalIndirectLayers(), true },
    };

    const auto distinctSlots = [] (const std::vector<ShaderReflection::ResourceBinding>& resources)
    {
        std::set<uint32_t> slots;
        for (const auto& resource : resources)
            slots.insert (resource.backendSlot);

        return slots.size() == resources.size();
    };

    for (const auto& [name, bundle, declaresRawDepth] : bundles)
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;

        const auto& blocks = fragment->reflection.uniformBuffers;
        const auto block = std::find_if (blocks.begin(), blocks.end(), [] (const auto& b) { return b.set == 3 && b.binding == 4; });
        ASSERT_NE (block, blocks.end()) << name;
        EXPECT_EQ (block->blockSize, static_cast<uint32_t> (sizeof (GpuShadowData))) << name;

        EXPECT_TRUE (declares (fragment->reflection.separateImages, 3, 0)) << name << ": shadowCascadesCompare";
        EXPECT_TRUE (declares (fragment->reflection.separateImages, 3, 2)) << name << ": shadowAtlasCompare";
        EXPECT_TRUE (declares (fragment->reflection.separateSamplers, 0, 4)) << name << ": shadowCompare";

        const auto* msl = bundle.findShader (ShaderStage::fragment, ShaderLanguage::msl);
        ASSERT_NE (msl, nullptr) << name;
        EXPECT_TRUE (distinctSlots (msl->reflection.separateImages)) << name;
        EXPECT_TRUE (distinctSlots (msl->reflection.separateSamplers)) << name;

        const auto* wgsl = bundle.findShader (ShaderStage::fragment, ShaderLanguage::wgsl);
        ASSERT_NE (wgsl, nullptr) << name;

        const auto layout = WgslBindingLayout::parse (wgsl->source);
        ASSERT_TRUE (layout.wasOk()) << name << ": " << layout.getErrorMessage();

        const auto find = [&layout] (uint32 group, uint32 binding) -> const WgslBindingLayout::Entry*
        {
            for (const auto& entry : layout.getReference())
                if (entry.group == group && entry.binding == binding)
                    return &entry;

            return nullptr;
        };

        const auto* cascades = find (3, 0);
        ASSERT_NE (cascades, nullptr) << name;
        EXPECT_EQ (cascades->kind, WgslBindingLayout::ResourceKind::texture) << name;
        EXPECT_EQ (cascades->sampleType, WgslBindingLayout::SampleType::depth) << name;
        EXPECT_EQ (cascades->viewDimension, WgslBindingLayout::ViewDimension::array2D) << name;

        const auto* atlas = find (3, 2);
        ASSERT_NE (atlas, nullptr) << name;
        EXPECT_EQ (atlas->kind, WgslBindingLayout::ResourceKind::texture) << name;
        EXPECT_EQ (atlas->sampleType, WgslBindingLayout::SampleType::depth) << name;
        EXPECT_EQ (atlas->viewDimension, WgslBindingLayout::ViewDimension::texture2D) << name;

        const auto* compare = find (0, 4);
        ASSERT_NE (compare, nullptr) << name;
        EXPECT_EQ (compare->kind, WgslBindingLayout::ResourceKind::sampler) << name;
        EXPECT_EQ (compare->samplerType, WgslBindingLayout::SamplerType::comparison) << name;

        // The soft shadow blocker search reads the same maps raw, with texelFetch: WGSL loads them
        // without a sampler, so WebGPU lays them out as unfilterable float textures, the one non
        // depth sample type a depth texture binds to (GpuPipeline takes it from the WGSL). The
        // default bundles of both tiers leave soft and contact shadows to their own variants and declare neither
        for (const auto& [binding, dimension] : { std::pair { 1u, WgslBindingLayout::ViewDimension::array2D },
                                                  std::pair { 3u, WgslBindingLayout::ViewDimension::texture2D } })
        {
            const auto* raw = find (3, binding);
            EXPECT_EQ (raw != nullptr, declaresRawDepth) << name << " binding " << (int) binding;
            EXPECT_EQ (declares (fragment->reflection.separateImages, 3, binding), declaresRawDepth) << name << " binding " << (int) binding;

            if (raw == nullptr)
                continue;

            EXPECT_EQ (raw->kind, WgslBindingLayout::ResourceKind::texture) << name;
            EXPECT_EQ (raw->sampleType, WgslBindingLayout::SampleType::floatUnfilterable) << name;
            EXPECT_EQ (raw->viewDimension, dimension) << name;
        }

        // The reflection the render pipeline layouts are built from (ShaderBindingMap) types them alike:
        // a depth texture and a comparison sampler, for every language
        for (const auto language : { ShaderLanguage::spirv, ShaderLanguage::wgsl, ShaderLanguage::msl, ShaderLanguage::glsl })
        {
            const auto* shader = bundle.findShader (ShaderStage::fragment, language);
            ASSERT_NE (shader, nullptr) << name;

            const auto isDepth = [] (const std::vector<ShaderReflection::ResourceBinding>& resources, uint32_t set, uint32_t binding)
            {
                return std::any_of (resources.begin(), resources.end(), [&] (const auto& r) { return r.set == set && r.binding == binding && r.imageIsDepth; });
            };

            const auto& reflection = shader->reflection;
            EXPECT_TRUE (isDepth (reflection.separateImages, 3, 0) || isDepth (reflection.sampledImages, 3, 0)) << name << " language " << (int) language;
            EXPECT_TRUE (isDepth (reflection.separateImages, 3, 2) || isDepth (reflection.sampledImages, 3, 2)) << name << " language " << (int) language;

            if (language != ShaderLanguage::glsl)
                EXPECT_TRUE (isDepth (reflection.separateSamplers, 0, 4)) << name << " language " << (int) language;
        }
    }

    // The background shades nothing: no shadow resource reaches its pipeline
    const auto rasterBackground = background();
    for (const auto stage : { ShaderStage::vertex, ShaderStage::fragment })
    {
        const auto* shader = rasterBackground.findShader (stage, ShaderLanguage::spirv);
        ASSERT_NE (shader, nullptr);

        for (const auto* resources : { &shader->reflection.uniformBuffers, &shader->reflection.separateImages, &shader->reflection.separateSamplers })
            for (const auto& resource : *resources)
                EXPECT_FALSE (resource.set == 3 || (resource.set == 0 && resource.binding == 4)) << resource.name;
    }
}

TEST_F (SceneRendererBundleTests, ShadowPassesDrawThroughTheGeometryStage)
{
    // Both shadow pipelines share the vertex stage of the mesh passes (lib/geometry.glsl): the
    // ViewBlock of the shadow view and the DrawBlock. Opaque casters write depth only; masked ones
    // read the alpha of their base color through the material bindings of set 2
    for (const auto& [name, bundle] : { std::pair { "ShadowPass", shadowPass() }, std::pair { "ShadowPass_masked", shadowPassMasked() } })
    {
        const auto* vertex = bundle.findShader (ShaderStage::vertex, ShaderLanguage::spirv);
        ASSERT_NE (vertex, nullptr) << name;
        EXPECT_TRUE (declares (vertex->reflection.uniformBuffers, 0, 0)) << name;
        EXPECT_TRUE (declares (vertex->reflection.uniformBuffers, 2, 0)) << name;
    }

    const auto opaque = shadowPass();
    const auto* opaqueFragment = opaque.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
    ASSERT_NE (opaqueFragment, nullptr);
    EXPECT_TRUE (opaqueFragment->reflection.uniformBuffers.empty());
    EXPECT_TRUE (opaqueFragment->reflection.separateImages.empty());
    EXPECT_TRUE (opaqueFragment->reflection.stageOutputs.empty());

    const auto masked = shadowPassMasked();
    const auto* maskedFragment = masked.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
    ASSERT_NE (maskedFragment, nullptr);
    EXPECT_TRUE (declares (maskedFragment->reflection.uniformBuffers, 2, 1));
    EXPECT_TRUE (declares (maskedFragment->reflection.separateImages, 2, 2));
    EXPECT_TRUE (declares (maskedFragment->reflection.separateSamplers, 2, 7));
    EXPECT_TRUE (maskedFragment->reflection.stageOutputs.empty());
}

TEST_F (SceneRendererBundleTests, RasterLightListBlockIs528Bytes)
{
    // Eight 64 byte GpuLight then the count: the array size bounds the per-draw light loop
    const auto bundle = forwardPass();
    const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
    ASSERT_NE (fragment, nullptr);

    const auto& blocks = fragment->reflection.uniformBuffers;
    const auto block = std::find_if (blocks.begin(), blocks.end(), [] (const auto& b) { return b.set == 2 && b.binding == 12; });
    ASSERT_NE (block, blocks.end());
    EXPECT_EQ (block->blockSize, 528u);
    EXPECT_EQ (block->blockSize, static_cast<uint32_t> (SceneDrawList::maxLightsPerDraw * sizeof (GpuLight) + 16));

    std::map<String, uint32_t> offsets;
    for (const auto& member : block->members)
        offsets[member.name] = member.offset;

    ASSERT_EQ (offsets.count ("count"), 1u);
    EXPECT_EQ (offsets["count"], 512u);

    EXPECT_TRUE (fragment->reflection.storageBuffers.empty());
}

TEST_F (SceneRendererBundleTests, FullForwardPassReadsTheClusteredLights)
{
    // No per-draw list on the full tier: every light in a storage buffer at set 1 binding 6, and
    // the light lists of the clusters at binding 7, both read only, in every full variant; and the
    // reflection probes of the frame at binding 8 and its irradiance volumes at binding 9
    const std::pair<const char*, ShaderBundle> bundles[] = {
        { "ForwardPass_full", forwardPassFull() },
        { "ForwardPass_full_area_lights", forwardPassFullAreaLights() },
        { "ForwardPass_full_soft_shadows", forwardPassFullSoftShadows() },
        { "ForwardPass_full_area_lights_soft_shadows", forwardPassFullAreaLightsSoftShadows() },
    };

    for (const auto& [name, bundle] : bundles)
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;

        const auto& blocks = fragment->reflection.uniformBuffers;
        EXPECT_TRUE (std::none_of (blocks.begin(), blocks.end(), [] (const auto& b) { return b.set == 2 && b.binding == 12; })) << name;

        std::vector<uint32_t> bindings;
        for (const auto& buffer : fragment->reflection.storageBuffers)
        {
            EXPECT_EQ (buffer.set, 1u) << name << ": " << buffer.name;
            EXPECT_TRUE (buffer.readOnly) << name << ": " << buffer.name;
            bindings.push_back (buffer.binding);
        }

        std::sort (bindings.begin(), bindings.end());
        EXPECT_EQ (bindings, (std::vector<uint32_t> { 6, 7, 8, 9 })) << name;
    }
}

TEST_F (SceneRendererBundleTests, RasterAreaLightsCompile)
{
    // The area light variants of the shading passes read the area light tables from the lookup
    // array, a set 0 lookup at binding 7, through the set 0 linearClamp sampler; binding 11 is spare
    const std::pair<const char*, ShaderBundle> bundles[] = {
        { "ForwardPass_area_lights", forwardPassAreaLights() },
        { "ForwardPass_area_lights_soft_shadows", forwardPassAreaLightsSoftShadows() },
        { "ForwardPass_area_lights_local_indirect", forwardPassAreaLightsLocalIndirect() },
        { "ForwardPass_area_lights_local_indirect_layers", forwardPassAreaLightsLocalIndirectLayers() },
        { "ForwardPass_area_lights_soft_shadows_local_indirect", forwardPassAreaLightsSoftShadowsLocalIndirect() },
        { "ForwardPass_area_lights_soft_shadows_local_indirect_layers", forwardPassAreaLightsSoftShadowsLocalIndirectLayers() },
        { "ForwardPass_full_area_lights", forwardPassFullAreaLights() },
        { "ForwardPass_full_area_lights_soft_shadows", forwardPassFullAreaLightsSoftShadows() },
        { "LightingPass_full_area_lights", lightingPassFullAreaLights() },
        { "LightingPass_full_area_lights_soft_shadows", lightingPassFullAreaLightsSoftShadows() },
        { "LightingPass_full_area_lights_local_indirect", lightingPassFullAreaLightsLocalIndirect() },
        { "LightingPass_full_area_lights_local_indirect_layers", lightingPassFullAreaLightsLocalIndirectLayers() },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect", lightingPassFullAreaLightsSoftShadowsLocalIndirect() },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect_layers", lightingPassFullAreaLightsSoftShadowsLocalIndirectLayers() },
    };

    for (const auto& [name, bundle] : bundles)
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;

        EXPECT_TRUE (declares (fragment->reflection.separateImages, 0, 7)) << name << ": lookupLuts";
        EXPECT_FALSE (declares (fragment->reflection.separateImages, 0, 11)) << name;
        EXPECT_TRUE (declares (fragment->reflection.separateSamplers, 0, 1)) << name << ": linearClamp";
        EXPECT_FALSE (declares (fragment->reflection.separateImages, 1, 4)) << name;
        EXPECT_FALSE (declares (fragment->reflection.separateImages, 1, 5)) << name;
    }
}

TEST_F (SceneRendererBundleTests, EveryShadingBundleReadsTheEnvironmentFromSetZero)
{
    // The environment is a set 0 lookup at binding 6, a cube read through linearClamp at binding 1
    // since its interim sampler at binding 14 is gone. Its split sum is in the lookup tables at
    // binding 7, unused by Background
    const std::pair<const char*, ShaderBundle> bundles[] = {
        { "ForwardPass", forwardPass() },
        { "ForwardPass_area_lights", forwardPassAreaLights() },
        { "ForwardPass_soft_shadows", forwardPassSoftShadows() },
        { "ForwardPass_area_lights_soft_shadows", forwardPassAreaLightsSoftShadows() },
        { "ForwardPass_local_indirect", forwardPassLocalIndirect() },
        { "ForwardPass_local_indirect_layers", forwardPassLocalIndirectLayers() },
        { "ForwardPass_area_lights_local_indirect", forwardPassAreaLightsLocalIndirect() },
        { "ForwardPass_area_lights_local_indirect_layers", forwardPassAreaLightsLocalIndirectLayers() },
        { "ForwardPass_soft_shadows_local_indirect", forwardPassSoftShadowsLocalIndirect() },
        { "ForwardPass_soft_shadows_local_indirect_layers", forwardPassSoftShadowsLocalIndirectLayers() },
        { "ForwardPass_area_lights_soft_shadows_local_indirect", forwardPassAreaLightsSoftShadowsLocalIndirect() },
        { "ForwardPass_area_lights_soft_shadows_local_indirect_layers", forwardPassAreaLightsSoftShadowsLocalIndirectLayers() },
        { "ForwardPass_full", forwardPassFull() },
        { "ForwardPass_full_area_lights", forwardPassFullAreaLights() },
        { "ForwardPass_full_soft_shadows", forwardPassFullSoftShadows() },
        { "ForwardPass_full_area_lights_soft_shadows", forwardPassFullAreaLightsSoftShadows() },
        { "Background", background() },
        { "LightingPass_full", lightingPassFull() },
        { "LightingPass_full_area_lights", lightingPassFullAreaLights() },
        { "LightingPass_full_soft_shadows", lightingPassFullSoftShadows() },
        { "LightingPass_full_area_lights_soft_shadows", lightingPassFullAreaLightsSoftShadows() },
        { "LightingPass_full_local_indirect", lightingPassFullLocalIndirect() },
        { "LightingPass_full_local_indirect_layers", lightingPassFullLocalIndirectLayers() },
        { "LightingPass_full_area_lights_local_indirect", lightingPassFullAreaLightsLocalIndirect() },
        { "LightingPass_full_area_lights_local_indirect_layers", lightingPassFullAreaLightsLocalIndirectLayers() },
        { "LightingPass_full_soft_shadows_local_indirect", lightingPassFullSoftShadowsLocalIndirect() },
        { "LightingPass_full_soft_shadows_local_indirect_layers", lightingPassFullSoftShadowsLocalIndirectLayers() },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect", lightingPassFullAreaLightsSoftShadowsLocalIndirect() },
        { "LightingPass_full_area_lights_soft_shadows_local_indirect_layers", lightingPassFullAreaLightsSoftShadowsLocalIndirectLayers() },
    };

    for (const auto& [name, bundle] : bundles)
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;

        const auto& images = fragment->reflection.separateImages;
        const auto environment = std::find_if (images.begin(), images.end(), [] (const auto& image) { return image.set == 0 && image.binding == 6; });
        ASSERT_NE (environment, images.end()) << name << ": environment";
        EXPECT_EQ (environment->imageDim, ShaderReflection::ImageDimension::cube) << name;

        EXPECT_FALSE (declares (fragment->reflection.separateSamplers, 0, 14)) << name << ": interim environment sampler";
        EXPECT_TRUE (declares (fragment->reflection.separateSamplers, 0, 1)) << name << ": linearClamp";
        EXPECT_EQ (declares (fragment->reflection.separateImages, 0, 7), String (name) != "Background") << name << ": lookupLuts";
        EXPECT_FALSE (declares (fragment->reflection.separateImages, 1, 1)) << name;
        EXPECT_FALSE (declares (fragment->reflection.separateSamplers, 1, 2)) << name;
    }
}

TEST_F (SceneRendererBundleTests, PunctualVariantsCompileNoAreaLightCode)
{
    // Without YUP_3D_AREA_LIGHTS every light is punctual: no area light code, while the lookup
    // tables and their sampler stay declared for the environment. The area variants hold the code,
    // so the check is not vacuous
    for (const auto& [name, bundle] : { std::pair { "ForwardPass_area_lights", forwardPassAreaLights() },
                                        std::pair { "LightingPass_full_area_lights", lightingPassFullAreaLights() } })
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::msl);
        ASSERT_NE (fragment, nullptr) << name;
        EXPECT_TRUE (fragment->source.contains ("lightsEllipseIntegral")) << name;
    }

    const std::pair<const char*, ShaderBundle> bundles[] = {
        { "ForwardPass", forwardPass() },
        { "ForwardPass_soft_shadows", forwardPassSoftShadows() },
        { "ForwardPass_local_indirect", forwardPassLocalIndirect() },
        { "ForwardPass_local_indirect_layers", forwardPassLocalIndirectLayers() },
        { "ForwardPass_soft_shadows_local_indirect", forwardPassSoftShadowsLocalIndirect() },
        { "ForwardPass_soft_shadows_local_indirect_layers", forwardPassSoftShadowsLocalIndirectLayers() },
        { "ForwardPass_full", forwardPassFull() },
        { "ForwardPass_full_soft_shadows", forwardPassFullSoftShadows() },
        { "LightingPass_full", lightingPassFull() },
        { "LightingPass_full_soft_shadows", lightingPassFullSoftShadows() },
        { "LightingPass_full_local_indirect", lightingPassFullLocalIndirect() },
        { "LightingPass_full_local_indirect_layers", lightingPassFullLocalIndirectLayers() },
        { "LightingPass_full_soft_shadows_local_indirect", lightingPassFullSoftShadowsLocalIndirect() },
        { "LightingPass_full_soft_shadows_local_indirect_layers", lightingPassFullSoftShadowsLocalIndirectLayers() },
    };

    for (const auto& [name, bundle] : bundles)
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;

        EXPECT_TRUE (declares (fragment->reflection.separateImages, 0, 7)) << name << ": lookupLuts";
        EXPECT_FALSE (declares (fragment->reflection.separateImages, 0, 11)) << name;
        EXPECT_TRUE (declares (fragment->reflection.separateSamplers, 0, 1)) << name << ": linearClamp";

        const auto* msl = bundle.findShader (ShaderStage::fragment, ShaderLanguage::msl);
        ASSERT_NE (msl, nullptr) << name;
        EXPECT_FALSE (msl->source.contains ("lightsEllipseIntegral")) << name;
    }
}

#if YUP_APPLE || YUP_LINUX
class SceneRendererBaselineScenes : public ::testing::Test
{
protected:
    /** A plate facing the default camera, with a smaller one floating over it to cast a shadow. */
    static EntityNode::Ptr makePlates()
    {
        const auto quad = [] (float halfSize, float z)
        {
            std::vector<Mesh::Vertex> vertices (4);
            vertices[0].position = { -halfSize, -halfSize, z };
            vertices[1].position = { halfSize, -halfSize, z };
            vertices[2].position = { halfSize, halfSize, z };
            vertices[3].position = { -halfSize, halfSize, z };

            for (auto& vertex : vertices)
                vertex.normal = { 0.0f, 0.0f, 1.0f };

            return vertices;
        };

        auto mesh = Mesh::Ptr (new Mesh ("plates"));
        mesh->addPrimitive (quad (1.0f, 0.0f), { 0, 1, 2, 0, 2, 3 });
        mesh->addPrimitive (quad (0.3f, 0.5f), { 0, 1, 2, 0, 2, 3 });

        auto entity = EntityNode::Ptr (new EntityNode ("plates"));
        entity->attach<MeshNode> (mesh);
        return entity;
    }

    /** Sun shadow over the environment: the existing plates scene with ACES. */
    static void buildShadowsScene (Scene& scene)
    {
        scene.getRoot()->addChild (makePlates());

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().castsShadows = true;
        sun->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.5f));
        scene.getRoot()->addChild (sun);

        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
        scene.setEnvironmentVisible (true);
        scene.setToneMapping (Scene::ToneMapping::aces);
    }

    /** Emissive, alpha-masked and blended quads over a white plate, lit by eight colored point lights. */
    static void buildMaterialsScene (Scene& scene)
    {
        const auto quadAt = [] (float x, float halfSize, float z, Material::Ptr material, float leftAlpha = 1.0f, float rightAlpha = 1.0f)
        {
            std::vector<Mesh::Vertex> vertices (4);
            vertices[0].position = { x - halfSize, -halfSize, z };
            vertices[1].position = { x + halfSize, -halfSize, z };
            vertices[2].position = { x + halfSize, halfSize, z };
            vertices[3].position = { x - halfSize, halfSize, z };

            for (auto& vertex : vertices)
                vertex.normal = { 0.0f, 0.0f, 1.0f };

            vertices[0].color[3] = vertices[3].color[3] = leftAlpha;
            vertices[1].color[3] = vertices[2].color[3] = rightAlpha;

            auto mesh = Mesh::Ptr (new Mesh ("quad"));
            mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);

            auto entity = EntityNode::Ptr (new EntityNode ("quad"));
            entity->attach<MeshNode> (mesh);
            return entity;
        };

        auto plate = Material::Ptr (new Material());
        plate->metallicFactor = 0.0f;
        plate->roughnessFactor = 0.4f;

        auto emissive = Material::Ptr (new Material());
        emissive->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        emissive->emissiveFactor = { 1.0f, 0.5f, 0.1f };

        // Alpha runs from 0.2 to 0.8 across the quad, so the cutoff discards its left half
        auto masked = Material::Ptr (new Material());
        masked->baseColorFactor = { 0.2f, 0.8f, 0.2f, 1.0f };
        masked->alphaMode = Material::AlphaMode::mask;

        auto blended = Material::Ptr (new Material());
        blended->baseColorFactor = { 0.2f, 0.2f, 0.9f, 0.5f };
        blended->alphaMode = Material::AlphaMode::blend;

        scene.getRoot()->addChild (quadAt (0.0f, 1.2f, -0.3f, plate));
        scene.getRoot()->addChild (quadAt (-0.7f, 0.3f, 0.0f, emissive));
        scene.getRoot()->addChild (quadAt (0.0f, 0.3f, 0.0f, masked, 0.2f, 0.8f));
        scene.getRoot()->addChild (quadAt (0.7f, 0.3f, 0.0f, blended));

        for (int i = 0; i < 8; ++i)
        {
            const auto angle = MathConstants<float>::twoPi * static_cast<float> (i) / 8.0f;

            auto light = EntityNode::Ptr (new EntityNode ("point"));
            auto& node = light->attach<LightNode>();
            node.type = LightNode::Type::point;
            node.color = { 0.5f + 0.5f * std::cos (angle), 0.5f + 0.5f * std::sin (angle), 0.5f };
            node.intensity = 4.0f;
            node.range = 1.2f;
            light->setPosition ({ 0.9f * std::cos (angle), 0.9f * std::sin (angle), 0.3f });
            scene.getRoot()->addChild (light);
        }
    }

    /** A rotated plate against a dark background, so silhouettes show the MSAA coverage. */
    static void buildEdgesScene (Scene& scene)
    {
        auto plates = makePlates();
        plates->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, 0.3f));
        scene.getRoot()->addChild (plates);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>();
        scene.getRoot()->addChild (sun);
    }

    /** Places @p entity at @p eye with its -Z axis, which cameras look along and lights shine along, towards @p target. */
    static void aimCamera (EntityNode& entity, Vector3<float> eye, Vector3<float> target, Vector3<float> up = { 0.0f, 1.0f, 0.0f })
    {
        entity.setPosition (eye);
        entity.setRotation (Quaternion::fromRotationMatrix (Matrix4::lookAt (eye, target, up).inverted()));
    }

    /** A rough light gray dielectric, for the grounds, the walls and the blockers of the shadow scenes. */
    static Material::Ptr makeWallMaterial()
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.8f, 0.8f, 0.8f, 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 1.0f;
        return material;
    }

    /** A square ground at y = 0, 2 * @p halfSize wide, drawn with makeWallMaterial(): the entity "ground". */
    static EntityNode* addGround (Scene& scene, float halfSize)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -halfSize, 0.0f, halfSize };
        vertices[1].position = { halfSize, 0.0f, halfSize };
        vertices[2].position = { halfSize, 0.0f, -halfSize };
        vertices[3].position = { -halfSize, 0.0f, -halfSize };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 1.0f, 0.0f };

        auto mesh = Mesh::Ptr (new Mesh ("ground"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, makeWallMaterial());

        auto ground = EntityNode::Ptr (new EntityNode ("ground"));
        ground->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (ground);
        return ground.get();
    }

    /** The ground of the long shadow scenes: a 200 x 200 rough dielectric quad at y = 0, lit by a
        shadow casting sun tilted 0.6 rad from vertical, so shadows fall towards +x, under an ambient
        light of 0.05 and no environment. Returns the sun.
    */
    static LightNode& addLongGround (Scene& scene)
    {
        addGround (scene, 100.0f);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        auto& light = sun->attach<LightNode>();
        light.castsShadows = true;
        aimCamera (*sun, { -10.0f * std::sin (0.6f), 10.0f * std::cos (0.6f), 0.0f }, {});
        scene.getRoot()->addChild (sun);

        scene.setAmbientColor (Color (0xff3f3f3f)); // 0.05 once linear
        return light;
    }

    /** A cube of half size @p halfSize centered on the origin, its faces wound counterclockwise seen
        from outside, or facing its center when @p inward is set (a closed room). */
    static Mesh::Ptr makeBox (float halfSize, Material::Ptr material, bool inward = false)
    {
        // Each face: its normal n and two axes u, v with u x v = n
        const std::array<std::array<Vector3<float>, 3>, 6> faces { {
            { Vector3<float> (1.0f, 0.0f, 0.0f), Vector3<float> (0.0f, 0.0f, -1.0f), Vector3<float> (0.0f, 1.0f, 0.0f) },
            { Vector3<float> (-1.0f, 0.0f, 0.0f), Vector3<float> (0.0f, 0.0f, 1.0f), Vector3<float> (0.0f, 1.0f, 0.0f) },
            { Vector3<float> (0.0f, 1.0f, 0.0f), Vector3<float> (1.0f, 0.0f, 0.0f), Vector3<float> (0.0f, 0.0f, -1.0f) },
            { Vector3<float> (0.0f, -1.0f, 0.0f), Vector3<float> (1.0f, 0.0f, 0.0f), Vector3<float> (0.0f, 0.0f, 1.0f) },
            { Vector3<float> (0.0f, 0.0f, 1.0f), Vector3<float> (1.0f, 0.0f, 0.0f), Vector3<float> (0.0f, 1.0f, 0.0f) },
            { Vector3<float> (0.0f, 0.0f, -1.0f), Vector3<float> (-1.0f, 0.0f, 0.0f), Vector3<float> (0.0f, 1.0f, 0.0f) },
        } };

        std::vector<Mesh::Vertex> vertices;
        std::vector<uint32> indices;

        for (const auto& [n, u, v] : faces)
        {
            const auto first = static_cast<uint32> (vertices.size());
            for (const auto& [a, b] : { std::pair { -1.0f, -1.0f }, std::pair { 1.0f, -1.0f }, std::pair { 1.0f, 1.0f }, std::pair { -1.0f, 1.0f } })
            {
                auto& vertex = vertices.emplace_back();
                vertex.position = (n + u * a + v * b) * halfSize;
                vertex.normal = inward ? -n : n;
            }

            for (const auto index : inward ? std::array { 0u, 2u, 1u, 0u, 3u, 2u } : std::array { 0u, 1u, 2u, 0u, 2u, 3u })
                indices.push_back (first + index);
        }

        auto mesh = Mesh::Ptr (new Mesh ("box"));
        mesh->addPrimitive (std::move (vertices), std::move (indices), std::move (material));
        return mesh;
    }

    /** A cube, the entity "caster", standing on the long ground at z = -casterDistance, seen from
        above and in front. The cube and the camera scale with the distance, so the cube and the
        shadow it casts beside it (towards +x) keep their size on screen: with k = casterDistance / 2
        the cube is 0.25 k wide and the camera looks at its foot from (0, 2 k, 3 k) away. At 256 x 256
        the shadow beside the cube covers about 14 x 11 px. A single shadow map fitted to the whole
        ground has texels of about 0.14 m, so at 2 m the shadow spans about 1 x 2 of them.
        The cube is drawn with @p casterMaterial, or a light gray dielectric. Returns the sun.
    */
    static LightNode& buildLongGroundScene (Scene& scene, float casterDistance, Material::Ptr casterMaterial = nullptr)
    {
        auto& sun = addLongGround (scene);
        const auto k = casterDistance * 0.5f;
        const auto size = 0.25f * k;

        auto material = casterMaterial != nullptr ? std::move (casterMaterial) : Material::Ptr (new Material());
        if (material->alphaMode == Material::AlphaMode::opaque)
        {
            material->baseColorFactor = { 0.8f, 0.8f, 0.8f, 1.0f };
            material->metallicFactor = 0.0f;
        }

        auto caster = EntityNode::Ptr (new EntityNode ("caster"));
        caster->attach<MeshNode> (makeBox (size * 0.5f, material));
        caster->setPosition ({ 0.0f, size * 0.5f, -casterDistance });
        scene.getRoot()->addChild (caster);

        const Vector3<float> foot (0.0f, 0.0f, -casterDistance);
        auto camera = EntityNode::Ptr (new EntityNode ("camera"));
        camera->attach<CameraNode>();
        aimCamera (*camera, foot + Vector3<float> (0.0f, 2.0f * k, 3.0f * k), foot);
        scene.getRoot()->addChild (camera);
        scene.setActiveCamera (camera.get());

        return sun;
    }

    /** Adds a 1 m cube, the entity "farCaster", to buildLongGroundScene() 60 m away beside the line
        of sight, in the last cascade only. */
    static EntityNode& addFarCaster (Scene& scene)
    {
        auto caster = EntityNode::Ptr (new EntityNode ("farCaster"));
        caster->attach<MeshNode> (makeBox (0.5f, makeWallMaterial()));
        caster->setPosition ({ 0.0f, 0.5f, -60.0f });
        scene.getRoot()->addChild (caster);
        return *caster;
    }

    /** A compact scene: a 0.6 m box, the entity "box", on a 2 m ground under a shadow casting sun,
        without a camera. Returns the box. */
    static EntityNode& addCompactScene (Scene& scene)
    {
        addGround (scene, 1.0f);

        auto box = EntityNode::Ptr (new EntityNode ("box"));
        box->attach<MeshNode> (makeBox (0.3f, makeWallMaterial()));
        box->setPosition ({ 0.0f, 0.3f, 0.0f });
        scene.getRoot()->addChild (box);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().castsShadows = true;
        aimCamera (*sun, { -3.0f, 8.0f, 2.0f }, {});
        scene.getRoot()->addChild (sun);
        return *box;
    }

    /** The shadow renderer of @p renderer, which must have rendered: a test failure otherwise,
        read as a shadow renderer that drew nothing. */
    static const ShadowRenderer& shadows (const SceneRenderer& renderer)
    {
        if (const auto* shadowRenderer = renderer.getShadowRenderer())
            return *shadowRenderer;

        ADD_FAILURE() << "The renderer has not rendered yet";
        static const ShadowRenderer none;
        return none;
    }

    /** How many frames drew each of the first @p count sun cascades. */
    static std::vector<int> getCascadeRenderCounts (const SceneRenderer& renderer, int count)
    {
        std::vector<int> counts;
        for (int c = 0; c < count; ++c)
            counts.push_back (shadows (renderer).getNumCascadeRenders (c));

        return counts;
    }

    /** The largest relative darkening of an 8x8 pixel window from @p unshadowed to @p shadowed,
        (lit - shadowed) / lit in Rec.709 luma, over the windows with a lit luma above 10 levels.
        Only the shadow tells the two images apart, so the caster itself never counts.
    */
    static double shadowContrast (const Image& unshadowed, const Image& shadowed)
    {
        auto contrast = 0.0;
        for (int y = 0; y + 8 <= unshadowed.getHeight(); y += 2)
        {
            for (int x = 0; x + 8 <= unshadowed.getWidth(); x += 2)
            {
                const Rectangle<int> window (x, y, 8, 8);
                const auto lit = meanLuminance (unshadowed, window);
                if (lit > 10.0)
                    contrast = jmax (contrast, (lit - meanLuminance (shadowed, window)) / lit);
            }
        }

        return contrast;
    }

    /** The Rec.709 luma, from 0 to 1, of every pixel of column @p x from the top, 0 where it shows
        @p background (within 2 levels in every channel). */
    static std::vector<float> luminanceColumn (const Image& image, int x, Color background)
    {
        std::vector<float> column;
        for (int y = 0; y < image.getHeight(); ++y)
        {
            const auto color = image.getPixelColor (x, y);
            const auto isBackground = std::abs (color.getRed() - background.getRed()) <= 2
                                   && std::abs (color.getGreen() - background.getGreen()) <= 2
                                   && std::abs (color.getBlue() - background.getBlue()) <= 2;

            column.push_back (isBackground ? 0.0f : static_cast<float> (meanLuminance (image, { x, y, 1, 1 }) / 255.0));
        }

        return column;
    }

    /** The median of the non-zero values of @p values, 0 without any. */
    static float median (std::vector<float> values)
    {
        values.erase (std::remove (values.begin(), values.end(), 0.0f), values.end());
        if (values.empty())
            return 0.0f;

        std::nth_element (values.begin(), values.begin() + static_cast<std::ptrdiff_t> (values.size() / 2), values.end());
        return values[values.size() / 2];
    }

    /** Pins the shadow settings of tests comparing the tiers: no soft shadows and the raster cascade count on both. */
    static void pinCrossTierShadows (Scene& scene)
    {
        auto settings = scene.getRenderSettings();
        settings.softShadows = false;
        settings.sunCascades = RenderCapabilities::defaultRasterCascades;
        scene.setRenderSettings (settings);
    }

    /** The folder holding the baseline images, tests/data/yup_3d/baseline. */
    static File getBaselineDirectory()
    {
        return File (__FILE__).getParentDirectory().getParentDirectory().getChildFile ("data/yup_3d/baseline");
    }

    /** Loads a baseline image, or returns an invalid image if it is missing. */
    static Image loadBaseline (const String& fileName)
    {
        auto stream = getBaselineDirectory().getChildFile (fileName).createInputStream();
        if (stream == nullptr)
            return {};

        PngImageFormatReader reader (stream.release());
        return reader.readImage();
    }

    /** Writes the image when YUP_3D_WRITE_BASELINE=1, into tests/data/yup_3d/baseline. */
    static void writeBaselineIfRequested (const Image& image, const String& platform, const String& sceneName)
    {
        if (SystemStats::getEnvironmentVariable ("YUP_3D_WRITE_BASELINE", {}) != "1")
            return;

        const auto file = getBaselineDirectory().getChildFile (platform + "_" + sceneName + ".png");
        file.getParentDirectory().createDirectory();
        file.deleteFile();

        auto stream = std::make_unique<FileOutputStream> (file);
        ASSERT_TRUE (stream->openedOk()) << file.getFullPathName();

        PngImageFormatWriter writer (stream.release(), PixelFormat::RGBA);
        ASSERT_TRUE (writer.writeImage (image)) << file.getFullPathName();
    }

    /** The region beside the small plate, where its shadow falls, is clearly darker than the lit big plate. */
    static void checkShadows (const Image& image)
    {
        const auto lit = static_cast<int> (image.getPixelColor (40, 60).getRed());
        const auto shadowed = static_cast<int> (image.getPixelColor (50, 64).getRed());

        RecordProperty ("shadowMargin", lit - shadowed);
        EXPECT_GE (lit - shadowed, 6) << "shadows: lit " << lit << ", shadowed " << shadowed;
    }

    /** The emissive quad is orange, the mask discards half of its quad, and the lights tint the plate with several hues. */
    static void checkMaterials (const Image& image)
    {
        const auto emissive = image.getPixelColor (42, 62);
        EXPECT_GT (emissive.getRed(), emissive.getGreen()) << "materials: emissive";
        EXPECT_GT (emissive.getGreen(), emissive.getBlue()) << "materials: emissive";

        // The left half of the masked quad shows the bluish plate behind, the right half the green quad
        const auto discarded = image.getPixelColor (56, 62);
        const auto kept = image.getPixelColor (68, 62);
        const auto plate = image.getPixelColor (53, 62); // The plate between the emissive and the masked quads
        EXPECT_NEAR (discarded.getRed(), plate.getRed(), 12) << "materials: masked left half";
        EXPECT_NEAR (discarded.getGreen(), plate.getGreen(), 12) << "materials: masked left half";
        EXPECT_NEAR (discarded.getBlue(), plate.getBlue(), 12) << "materials: masked left half";
        EXPECT_LT (kept.getBlue(), 110) << "materials: masked right half";
        EXPECT_GT (kept.getGreen(), kept.getRed() + 30) << "materials: masked right half";

        std::set<int> hues;
        for (int i = 0; i < 12; ++i)
        {
            const auto angle = MathConstants<float>::twoPi * static_cast<float> (i) / 12.0f;
            const auto x = 64 + static_cast<int> (std::round (34.0f * std::cos (angle)));
            const auto y = 64 + static_cast<int> (std::round (34.0f * std::sin (angle)));
            if (std::abs (y - 64) < 16 || y > 96)
                continue;

            const auto color = image.getPixelColor (x, y);
            if (color.getSaturation() > 0.1f)
                hues.insert (static_cast<int> (color.getHue() * 12.0f));
        }

        RecordProperty ("plateHues", static_cast<int> (hues.size()));
        EXPECT_GE (hues.size(), 3u) << "materials: distinct hues on the plate";
    }

    /** The red levels of the edges scene's pixels between the background and the lit plate, which
        only MSAA coverage produces along the silhouettes. */
    static std::vector<int> intermediateEdgeLevels (const Image& image)
    {
        std::vector<int> levels;
        for (int y = 30; y < 100; ++y)
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto red = static_cast<int> (image.getPixelColor (x, y).getRed());
                if (red > 30 && red < 147)
                    levels.push_back (red);
            }

        return levels;
    }

    /** Silhouettes have intermediate gray levels, which only MSAA coverage produces. */
    static void checkEdges (const Image& image)
    {
        const auto levels = intermediateEdgeLevels (image);
        EXPECT_GE (std::set<int> (levels.begin(), levels.end()).size(), 3u) << "edges: intermediate levels";
    }

    /** The pixels with a channel at 0 or 255 in @p image, where @p baseline has none. A raster rgba8
        frame hides non-finite values, but a channel holding one reads as either end, depending on
        where it enters the shading and on the backend (Metal returns the other operand of min (NaN, x)).
    */
    static int countNewExtremePixels (const Image& image, const Image& baseline)
    {
        const auto isExtreme = [] (Color color)
        {
            for (const auto level : { color.getRed(), color.getGreen(), color.getBlue() })
                if (level == 0 || level == 255)
                    return true;

            return false;
        };

        int extreme = 0;
        for (int y = 0; y < image.getHeight(); ++y)
            for (int x = 0; x < image.getWidth(); ++x)
                extreme += isExtreme (image.getPixelColor (x, y)) && ! isExtreme (baseline.getPixelColor (x, y)) ? 1 : 0;

        return extreme;
    }

    /** Selects the rendering tier of a scene. */
    static void setTier (Scene& scene, RenderSettings::TierPreference tier)
    {
        auto settings = scene.getRenderSettings();
        settings.tier = tier;
        scene.setRenderSettings (settings);
    }

    /** The linear radiance behind an 8 bit level that was Reinhard tone mapped and sRGB encoded. */
    static double reinhardRadiance (int level)
    {
        const auto encoded = level / 255.0;
        const auto mapped = encoded <= 0.04045 ? encoded / 12.92 : std::pow ((encoded + 0.055) / 1.055, 2.4);
        return mapped / (1.0 - mapped);
    }

    /** The mean Rec.709 luma of the 8 bit levels within @p area. */
    static double meanLuminance (const Image& image, Rectangle<int> area)
    {
        auto sum = 0.0;
        for (int y = area.getY(); y < area.getBottom(); ++y)
        {
            for (int x = area.getX(); x < area.getRight(); ++x)
            {
                const auto color = image.getPixelColor (x, y);
                sum += 0.2126 * color.getRed() + 0.7152 * color.getGreen() + 0.0722 * color.getBlue();
            }
        }

        return sum / static_cast<double> (area.getWidth() * area.getHeight());
    }

    /** A small emissive quad, (20, 20, 20) by default, about 15 px wide at 128x128, in the middle of a black background. */
    static void buildGlowScene (Scene& scene, float emission = 20.0f)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -0.25f, -0.25f, 0.0f };
        vertices[1].position = { 0.25f, -0.25f, 0.0f };
        vertices[2].position = { 0.25f, 0.25f, 0.0f };
        vertices[3].position = { -0.25f, 0.25f, 0.0f };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto emissive = Material::Ptr (new Material());
        emissive->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        emissive->emissiveFactor = { emission, emission, emission };

        auto mesh = Mesh::Ptr (new Mesh ("glow"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, emissive);

        auto entity = EntityNode::Ptr (new EntityNode ("glow"));
        entity->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (entity);
        scene.setBackgroundColor (Colors::black);
    }

    /** Turns bloom on or off in the render settings of a scene. */
    static void setBloom (Scene& scene, bool bloom)
    {
        auto settings = scene.getRenderSettings();
        settings.bloom = bloom;
        scene.setRenderSettings (settings);
    }

    /** The bounds of the pixels brighter than half the range: the emissive quad of buildGlowScene(). */
    static Rectangle<int> findBrightBounds (const Image& image)
    {
        auto left = image.getWidth(), top = image.getHeight(), right = 0, bottom = 0;
        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                if (image.getPixelColor (x, y).getRed() > 127)
                {
                    left = std::min (left, x);
                    top = std::min (top, y);
                    right = std::max (right, x + 1);
                    bottom = std::max (bottom, y + 1);
                }
            }
        }

        return right > left ? Rectangle<int> (left, top, right - left, bottom - top) : Rectangle<int>();
    }

    /** The mean luma of the ring from 4 to 12 px around @p quad. */
    static double ringLuminance (const Image& image, Rectangle<int> quad)
    {
        const auto outer = quad.enlarged (12);
        const auto inner = quad.enlarged (4);
        const auto outerArea = static_cast<double> (outer.getWidth() * outer.getHeight());
        const auto innerArea = static_cast<double> (inner.getWidth() * inner.getHeight());

        return (meanLuminance (image, outer) * outerArea - meanLuminance (image, inner) * innerArea) / (outerArea - innerArea);
    }

    /** The pixels of @p image more than 2 levels away from @p reference in any channel. */
    static int countDifferences (const Image& image, const Image& reference)
    {
        int differing = 0;
        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto a = image.getPixelColor (x, y);
                const auto b = reference.getPixelColor (x, y);
                if (std::abs (a.getRed() - b.getRed()) > 2 || std::abs (a.getGreen() - b.getGreen()) > 2 || std::abs (a.getBlue() - b.getBlue()) > 2)
                    ++differing;
            }
        }

        return differing;
    }

    /** The pixels of @p image more than 2 levels away from the Phase 0 baseline @p fileName, or -1 without the baseline. */
    static int countBaselineDifferences (const Image& image, const String& fileName)
    {
        const auto baseline = loadBaseline (fileName);
        return baseline.isValid() ? countDifferences (image, baseline) : -1;
    }

    /** True for the pixels of @p image more than 2 levels away from @p reference in any channel. */
    static std::vector<bool> differenceMask (const Image& image, const Image& reference)
    {
        std::vector<bool> mask;
        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto a = image.getPixelColor (x, y);
                const auto b = reference.getPixelColor (x, y);
                mask.push_back (std::abs (a.getRed() - b.getRed()) > 2 || std::abs (a.getGreen() - b.getGreen()) > 2 || std::abs (a.getBlue() - b.getBlue()) > 2);
            }
        }

        return mask;
    }

    /** The shadows scene against its Phase 0 baseline @p platform_shadows.png by statistics, since
        the cascades replaced the single map of Phase 0: the shadow darkens at least as much
        (checkShadows), covers about the same pixels, and nothing differs outside of the plates.
        The shadow is what tells each image apart from the scene rendered without it. The plates'
        shading under the environment changed in Phase 4 (lookup tables and multiple scattering),
        so their mean luminance is recorded for both images rather than compared.
    */
    void checkShadowsAgainstBaseline (const Image& image, const String& platform)
    {
        const auto baseline = loadBaseline (platform + "_shadows.png");
        ASSERT_TRUE (baseline.isValid()) << "capture the Phase 0 baselines first";

        Scene scene;
        buildShadowsScene (scene);
        setTier (scene, RenderSettings::TierPreference::raster);
        scene.getRoot()->findChild ("sun")->getNode<LightNode>()->castsShadows = false;

        SceneRenderer renderer;
        const auto unshadowed = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (unshadowed.isValid()) << renderer.getLastError();

        // The plates are the pixels they change over the background, in either image
        scene.getRoot()->removeChild (scene.getRoot()->findChild ("plates"));
        SceneRenderer backgroundRenderer;
        const auto background = renderToImage (backgroundRenderer, scene, 128);
        ASSERT_TRUE (background.isValid()) << backgroundRenderer.getLastError();

        checkShadows (image);

        const auto margin = [] (const Image& shadowed)
        {
            return static_cast<int> (shadowed.getPixelColor (40, 60).getRed()) - static_cast<int> (shadowed.getPixelColor (50, 64).getRed());
        };

        RecordProperty ("baselineShadowMargin", margin (baseline));
        EXPECT_GE (margin (image), margin (baseline));

        const auto shadow = differenceMask (image, unshadowed);
        const auto baselineShadow = differenceMask (baseline, unshadowed);
        const auto differing = differenceMask (image, baseline);
        const auto plates = differenceMask (image, background);
        const auto baselinePlates = differenceMask (baseline, background);

        int shadowPixels = 0, baselineShadowPixels = 0, differingOutside = 0, changedOutside = 0, platePixels = 0;
        double plateLuminance = 0.0, baselinePlateLuminance = 0.0;
        for (size_t i = 0; i < shadow.size(); ++i)
        {
            shadowPixels += shadow[i] ? 1 : 0;
            baselineShadowPixels += baselineShadow[i] ? 1 : 0;

            const auto onPlates = plates[i] || baselinePlates[i];
            differingOutside += differing[i] && ! onPlates ? 1 : 0;

            // The background stays exact, not just within 2 levels
            const auto x = static_cast<int> (i) % image.getWidth(), y = static_cast<int> (i) / image.getWidth();
            changedOutside += ! onPlates && image.getPixelColor (x, y) != baseline.getPixelColor (x, y) ? 1 : 0;

            if (onPlates)
            {
                const Rectangle<int> pixel (x, y, 1, 1);
                plateLuminance += meanLuminance (image, pixel);
                baselinePlateLuminance += meanLuminance (baseline, pixel);
                ++platePixels;
            }
        }

        RecordProperty ("shadowsDifferingFromBaseline", static_cast<int> (std::count (differing.begin(), differing.end(), true)));
        RecordProperty ("shadowPixels", shadowPixels);
        RecordProperty ("baselineShadowPixels", baselineShadowPixels);
        RecordProperty ("differingOutsidePlates", differingOutside);
        RecordProperty ("changedOutsidePlates", changedOutside);
        RecordProperty ("platePixels", platePixels);
        RecordProperty ("platesMeanLuminance", String (plateLuminance / jmax (platePixels, 1), 2).toStdString());
        RecordProperty ("baselinePlatesMeanLuminance", String (baselinePlateLuminance / jmax (platePixels, 1), 2).toStdString());
        RecordProperty ("platesLuminanceChangePercent", String (100.0 * (plateLuminance - baselinePlateLuminance) / jmax (baselinePlateLuminance, 1.0), 2).toStdString());

        EXPECT_GT (baselineShadowPixels, 0);
        EXPECT_GE (shadowPixels, baselineShadowPixels * 4 / 5);
        EXPECT_LE (shadowPixels, baselineShadowPixels * 5 / 4);
        EXPECT_GT (platePixels, 128 * 128 / 8);
        EXPECT_EQ (differingOutside, 0);
        EXPECT_EQ (changedOutside, 0);
    }

    /** How far a readVelocity() level is from zero velocity (127.5): 0 for 127 and 128, up to 127. */
    static int velocityDeviation (uint8 level)
    {
        return std::abs (2 * static_cast<int> (level) - 255) / 2;
    }

    /** The largest velocityDeviation() of the red or green level over an image. */
    static int maxVelocityDeviation (const Image& image)
    {
        int deviation = 0;
        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto color = image.getPixelColor (x, y);
                deviation = std::max ({ deviation, velocityDeviation (color.getRed()), velocityDeviation (color.getGreen()) });
            }
        }

        return deviation;
    }

    /** Renders each baseline scene at 128x128, checks it drew something and optionally writes it. */
    void captureBaselineScenes (const String& platform)
    {
        const std::pair<const char*, void (*) (Scene&)> scenes[] = {
            { "shadows", buildShadowsScene },
            { "materials", buildMaterialsScene },
            { "edges", buildEdgesScene },
        };

        for (const auto& [name, build] : scenes)
        {
            Scene scene;
            build (scene);

            // The baselines are raster frames, whichever tier the device would pick
            setTier (scene, RenderSettings::TierPreference::raster);

            SceneRenderer renderer;
            const auto image = renderToImage (renderer, scene, 128);

            ASSERT_TRUE (renderer.getLastError().isEmpty()) << name << ": " << renderer.getLastError();
            ASSERT_TRUE (image.isValid()) << name;

            // Something other than the background was drawn in the middle of the frame
            const auto background = scene.getBackgroundColor();
            bool drewSomething = false;
            for (int y = 32; y < 96 && ! drewSomething; y += 4)
                for (int x = 32; x < 96 && ! drewSomething; x += 4)
                    drewSomething = image.getPixelColor (x, y) != background;

            EXPECT_TRUE (drewSomething) << name;

            if (String (name) == "shadows")
                checkShadows (image);
            else if (String (name) == "materials")
                checkMaterials (image);
            else
                checkEdges (image);

            writeBaselineIfRequested (image, platform, name);
        }
    }

    /** The raster no-regression gate, at the default settings: the materials and edges scenes match
        the Phase 0 images of @p platform exactly (no pixel beyond 2 levels), the shadows scene is
        checked by statistics (checkShadowsAgainstBaseline()), the silhouettes of the edges scene
        keep at least the baseline's MSAA coverage, and no scene has a channel at 0 or 255 where the
        baseline has none, as a non-finite value would leave.
    */
    void checkMatchesPhaseZeroBaseline (const String& platform)
    {
        const std::pair<const char*, void (*) (Scene&)> scenes[] = {
            { "shadows", buildShadowsScene },
            { "materials", buildMaterialsScene },
            { "edges", buildEdgesScene },
        };

        for (const auto& [name, build] : scenes)
        {
            Scene scene;
            build (scene);

            RenderSettings settings;
            settings.tier = RenderSettings::TierPreference::raster;
            scene.setRenderSettings (settings);

            SceneRenderer renderer;
            const auto image = renderToImage (renderer, scene, 128);
            ASSERT_TRUE (image.isValid()) << name << ": " << renderer.getLastError();

            const auto baseline = loadBaseline (platform + "_" + name + ".png");
            ASSERT_TRUE (baseline.isValid()) << "capture the Phase 0 baselines first";

            const auto newExtreme = countNewExtremePixels (image, baseline);
            RecordProperty ((String (name) + "NewExtremePixels").toStdString(), newExtreme);
            EXPECT_EQ (newExtreme, 0) << name;

            if (String (name) == "shadows")
            {
                checkShadowsAgainstBaseline (image, platform);
                continue;
            }

            if (String (name) == "edges")
            {
                const auto coverage = static_cast<int> (intermediateEdgeLevels (image).size());
                const auto baselineCoverage = static_cast<int> (intermediateEdgeLevels (baseline).size());
                RecordProperty ("edgeCoveragePixels", coverage);
                RecordProperty ("baselineEdgeCoveragePixels", baselineCoverage);
                EXPECT_GE (coverage, baselineCoverage);
            }

            const auto differing = countDifferences (image, baseline);
            RecordProperty ((String (name) + "Differing").toStdString(), differing);
            EXPECT_EQ (differing, 0) << name;
        }
    }

    /** The baseline scenes at the default settings on @p tier, without a probe or a volume, add
        none of the opt-in passes: no depth and normal prepass, no probe baker, no probe list (only
        the local indirect variants declare its block, and the default variants shade), no
        screen-space GI and no depth of field. On raster, no ambient occlusion, screen-space reflections or Hi-Z pyramid
        either; the full tier traces its ambient occlusion and reflections, on there by default.
        No pass or allocation count is exposed, so the getters of each pass stand in for them.
    */
    void checkDefaultSettingsAddNoOptInPass (RenderSettings::TierPreference tier)
    {
        const auto full = tier == RenderSettings::TierPreference::full;

        const std::pair<const char*, void (*) (Scene&)> scenes[] = {
            { "shadows", buildShadowsScene },
            { "materials", buildMaterialsScene },
            { "edges", buildEdgesScene },
        };

        for (const auto& [name, build] : scenes)
        {
            Scene scene;
            build (scene);
            setTier (scene, tier);

            // The reflections need the history of an earlier frame
            SceneRenderer renderer;
            for (int frame = 0; frame < 2; ++frame)
                ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << name << ": " << renderer.getLastError();

            EXPECT_TRUE (renderer.waitForGPU().wasOk()) << name;

            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr) << name;
            EXPECT_EQ (renderer.getProbeBaker(), nullptr) << name;

            ASSERT_NE (renderer.getForwardPass(), nullptr) << name;
            EXPECT_FALSE (renderer.getForwardPass()->usesLocalIndirect()) << name;
            EXPECT_FALSE (renderer.getForwardPass()->usesMaterialLayers()) << name;
            EXPECT_FALSE (renderer.getLightingPass()->usesLocalIndirect()) << name;

            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion) != nullptr, full) << name;
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::reflections) != nullptr, full) << name;
            EXPECT_EQ (renderer.getReflectionPass() != nullptr, full) << name;
            EXPECT_EQ (renderer.getHiZPyramid() != nullptr, full) << name;
            EXPECT_EQ (renderer.getDepthOfField(), nullptr) << name;
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr) << name;
            EXPECT_EQ (renderer.getMotionBlur(), nullptr) << name;
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr) << name;

            const auto* ambientOcclusion = renderer.getAmbientOcclusionPass();
            EXPECT_EQ (ambientOcclusion != nullptr, full) << name;
            if (ambientOcclusion != nullptr)
            {
                EXPECT_EQ (ambientOcclusion->getIndirectDiffuse(), nullptr) << name;
                EXPECT_EQ (ambientOcclusion->getNumCachedIndirectDiffuseTargets(), 0u) << name;
            }
        }
    }

    /** Renders and reads the pixels back through a canvas. */
    Image renderToImage (SceneRenderer& renderer, Scene& scene, int size)
    {
        auto texture = renderer.render (device, scene, size, size);

        // A frame the GPU failed to execute reads back blank, without a CPU side error
        const auto gpuResult = renderer.waitForGPU();
        EXPECT_TRUE (gpuResult.wasOk()) << gpuResult.getErrorMessage();

        return texture != nullptr ? readBack (texture) : Image();
    }

    /** Draws an rgba8 texture through a canvas and reads it back: row 0 of the image is the top of the frame. */
    Image readBack (const GpuTexture::Ptr& texture)
    {
        const auto width = static_cast<int> (texture->getWidth());
        const auto height = static_cast<int> (texture->getHeight());

        auto canvas = GpuCanvas::create (*context, width, height);
        if (canvas == nullptr)
            return {};

        auto& g = canvas->beginDraw();
        g.drawTexture (texture, { 0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height) });
        return canvas->asImage();
    }

    /** Adds a square plate facing the default camera, 2 units wide, drawn with @p material, and returns its entity. */
    static EntityNode* addPlate (Scene& scene, Material::Ptr material)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -1.0f, -1.0f, 0.0f };
        vertices[1].position = { 1.0f, -1.0f, 0.0f };
        vertices[2].position = { 1.0f, 1.0f, 0.0f };
        vertices[3].position = { -1.0f, 1.0f, 0.0f };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto mesh = Mesh::Ptr (new Mesh ("plate"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, std::move (material));

        auto entity = EntityNode::Ptr (new EntityNode ("plate"));
        entity->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (entity);
        return entity.get();
    }

    /** Selects the full tier, accumulating @p taaSamples jittered frames (1 disables TAA). */
    static void setFullTierTaa (Scene& scene, int taaSamples)
    {
        auto settings = scene.getRenderSettings();
        settings.tier = RenderSettings::TierPreference::full;
        settings.taaSamples = taaSamples;
        scene.setRenderSettings (settings);
    }

    /** A 16x16 grid of white quads, 0.45 apart and 0.24 wide, each lit by its own red point light
        0.15 in front of its center with a range of 0.4: the nearest other light is 0.47 away from a
        quad center, out of range. The camera looks at the grid from 9 units, so at 256x256 the quad
        centers are the pixels of getLightGridCenter(). The quads are one primitive when @p oneDraw
        is set, and one primitive each otherwise.
    */
    static void buildLightGridScene (Scene& scene, bool oneDraw)
    {
        auto material = Material::Ptr (new Material());
        material->metallicFactor = 0.0f;

        auto mesh = Mesh::Ptr (new Mesh ("grid"));
        std::vector<Mesh::Vertex> vertices;
        std::vector<uint32> indices;

        for (int row = 0; row < 16; ++row)
        {
            for (int column = 0; column < 16; ++column)
            {
                const auto center = getLightGridPosition (column, row);
                const auto first = static_cast<uint32> (vertices.size());

                for (const auto& corner : { Vector3<float> (-0.12f, -0.12f, 0.0f), Vector3<float> (0.12f, -0.12f, 0.0f),
                                            Vector3<float> (0.12f, 0.12f, 0.0f), Vector3<float> (-0.12f, 0.12f, 0.0f) })
                {
                    auto& vertex = vertices.emplace_back();
                    vertex.position = center + corner;
                    vertex.normal = { 0.0f, 0.0f, 1.0f };
                }

                for (const auto index : { 0u, 1u, 2u, 0u, 2u, 3u })
                    indices.push_back (first + index);

                if (! oneDraw)
                {
                    mesh->addPrimitive (std::move (vertices), std::move (indices), material);
                    vertices.clear();
                    indices.clear();
                }

                auto light = EntityNode::Ptr (new EntityNode ("light"));
                auto& node = light->attach<LightNode> (LightNode::Type::point);
                node.color = { 1.0f, 0.0f, 0.0f };
                node.range = 0.4f;
                light->setPosition (center + Vector3<float> (0.0f, 0.0f, 0.15f));
                scene.getRoot()->addChild (light);
            }
        }

        if (oneDraw)
            mesh->addPrimitive (std::move (vertices), std::move (indices), material);

        auto grid = EntityNode::Ptr (new EntityNode ("grid"));
        grid->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (grid);

        auto camera = EntityNode::Ptr (new EntityNode ("camera"));
        camera->attach<CameraNode>();
        camera->setPosition ({ 0.0f, 0.0f, 9.0f });
        scene.getRoot()->addChild (camera);
        scene.setActiveCamera (camera.get());

        scene.setAmbientColor (Colors::black);
    }

    /** The world position of a quad center of buildLightGridScene(). */
    static Vector3<float> getLightGridPosition (int column, int row)
    {
        return { 0.45f * (static_cast<float> (column) - 7.5f), 0.45f * (static_cast<float> (row) - 7.5f), 0.0f };
    }

    /** The pixel of a quad center of buildLightGridScene() at 256x256: the camera sees 2 tan (0.4) * 9
        world units. The grid is symmetric, so this is the same quad center whichever way rows are read.
    */
    static Point<int> getLightGridCenter (int column, int row)
    {
        const auto pixelsPerUnit = 256.0f / (18.0f * std::tan (0.4f));
        const auto position = getLightGridPosition (column, row);
        return { static_cast<int> (128.0f + position.getX() * pixelsPerUnit), static_cast<int> (128.0f - position.getY() * pixelsPerUnit) };
    }

    /** The quad centers of buildLightGridScene() with a red level of at least 100 at 256x256. */
    static int countLitGridQuads (const Image& image)
    {
        int lit = 0;
        for (int row = 0; row < 16; ++row)
        {
            for (int column = 0; column < 16; ++column)
            {
                const auto pixel = getLightGridCenter (column, row);
                lit += image.getPixelColor (pixel.getX(), pixel.getY()).getRed() >= 100 ? 1 : 0;
            }
        }

        return lit;
    }

    /** More than eight lights shade one scene on @p tier. */
    void checkShadesMoreThanEightLightsPerScene (RenderSettings::TierPreference tier)
    {
        // A 4x4 grid of small white quads, 0.6 apart, each lit by its own red point light 0.3 in front
        // of it with a range of 0.6: a neighbour's light is 0.67 away from a quad center, out of range
        auto material = Material::Ptr (new Material());
        material->metallicFactor = 0.0f;

        Scene scene;
        scene.setAmbientColor (Colors::black);
        setTier (scene, tier);

        std::vector<Vector3<float>> centers;
        for (int row = 0; row < 4; ++row)
            for (int column = 0; column < 4; ++column)
                centers.push_back ({ 0.6f * static_cast<float> (column) - 0.9f, 0.6f * static_cast<float> (row) - 0.9f, 0.0f });

        for (const auto& center : centers)
        {
            std::vector<Mesh::Vertex> vertices (4);
            vertices[0].position = center + Vector3<float> (-0.2f, -0.2f, 0.0f);
            vertices[1].position = center + Vector3<float> (0.2f, -0.2f, 0.0f);
            vertices[2].position = center + Vector3<float> (0.2f, 0.2f, 0.0f);
            vertices[3].position = center + Vector3<float> (-0.2f, 0.2f, 0.0f);

            for (auto& vertex : vertices)
                vertex.normal = { 0.0f, 0.0f, 1.0f };

            auto mesh = Mesh::Ptr (new Mesh ("quad"));
            mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);

            auto quad = EntityNode::Ptr (new EntityNode ("quad"));
            quad->attach<MeshNode> (mesh);
            scene.getRoot()->addChild (quad);

            auto light = EntityNode::Ptr (new EntityNode ("light"));
            auto& node = light->attach<LightNode> (LightNode::Type::point);
            node.color = { 1.0f, 0.0f, 0.0f };
            node.range = 0.6f;
            light->setPosition (center + Vector3<float> (0.0f, 0.0f, 0.3f));
            scene.getRoot()->addChild (light);
        }

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        // The default camera sees 2 tan (0.4) * 5 world units over the 128 pixels; the grid is
        // symmetric, so the centers are the same pixels whichever way the rows are read back
        const auto pixelsPerUnit = 128.0f / (10.0f * std::tan (0.4f));
        int lit = 0;

        for (const auto& center : centers)
        {
            const auto x = static_cast<int> (64.0f + center.getX() * pixelsPerUnit);
            const auto y = static_cast<int> (64.0f - center.getY() * pixelsPerUnit);
            const auto red = static_cast<int> (image.getPixelColor (x, y).getRed());

            EXPECT_GE (red, 100) << "quad at " << center.getX() << ", " << center.getY();
            lit += red >= 100 ? 1 : 0;
        }

        RecordProperty ("litQuads", lit);
    }

    /** Renders @p scene at @p size x @p size, on the full tier until the accumulation converged, and reads it back. */
    Image renderConverged (SceneRenderer& renderer, Scene& scene, int size)
    {
        for (int frame = 0; frame < 64; ++frame)
            if (renderer.render (device, scene, size, size) == nullptr || renderer.isConverged())
                break;

        return renderToImage (renderer, scene, size);
    }

    /** buildLongGroundScene() on @p tier at 256 x 256, converged, with the sun casting shadows or
        not and an angular radius of @p sunRadius, and the cube drawn with @p casterMaterial. */
    Image renderLongGroundScene (RenderSettings::TierPreference tier, float casterDistance, bool castsShadows,
                                 float sunRadius = 0.0f, Material::Ptr casterMaterial = nullptr)
    {
        Scene scene;
        auto& sun = buildLongGroundScene (scene, casterDistance, std::move (casterMaterial));
        sun.castsShadows = castsShadows;
        sun.radius = sunRadius;
        setTier (scene, tier);

        SceneRenderer renderer;
        const auto image = renderConverged (renderer, scene, 256);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    }

    /** The sun shadows a cube standing near the camera and one 60 m away on @p tier: beside each
        cube the shadow darkens the ground by more than 35%. */
    void checkSunShadowReachesNearAndFarCasters (RenderSettings::TierPreference tier)
    {
        for (const auto distance : { 2.0f, 60.0f })
        {
            const auto shadowed = renderLongGroundScene (tier, distance, true);
            const auto unshadowed = renderLongGroundScene (tier, distance, false);
            ASSERT_TRUE (shadowed.isValid() && unshadowed.isValid());

            const auto contrast = shadowContrast (unshadowed, shadowed);
            RecordProperty ((String ("contrastAt") + String (static_cast<int> (distance))).toStdString(), String (contrast, 3).toStdString());
            EXPECT_GT (contrast, 0.35) << "caster at " << distance << " m";
        }
    }

    /** A sun with an angular radius, shaded through the area light variant, still casts its shadow on @p tier. */
    void checkAreaSunCastsAShadow (RenderSettings::TierPreference tier)
    {
        const auto shadowed = renderLongGroundScene (tier, 2.0f, true, 0.05f);
        const auto unshadowed = renderLongGroundScene (tier, 2.0f, false, 0.05f);
        ASSERT_TRUE (shadowed.isValid() && unshadowed.isValid());

        const auto contrast = shadowContrast (unshadowed, shadowed);
        RecordProperty ("contrast", String (contrast, 3).toStdString());
        EXPECT_GT (contrast, 0.35);
    }

    /** An alpha masked cube casts its shadow on @p tier only where its material keeps the texels:
        cut out entirely, it casts none. */
    void checkMaskedCasterShadowsOnlyWhatItKeeps (RenderSettings::TierPreference tier)
    {
        const auto contrastWithAlpha = [&] (float alpha)
        {
            const auto material = [alpha]
            {
                auto masked = Material::Ptr (new Material());
                masked->baseColorFactor = { 0.8f, 0.8f, 0.8f, alpha };
                masked->metallicFactor = 0.0f;
                masked->alphaMode = Material::AlphaMode::mask;
                return masked;
            };

            const auto shadowed = renderLongGroundScene (tier, 2.0f, true, 0.0f, material());
            const auto unshadowed = renderLongGroundScene (tier, 2.0f, false, 0.0f, material());
            return shadowed.isValid() && unshadowed.isValid() ? shadowContrast (unshadowed, shadowed) : -1.0;
        };

        const auto kept = contrastWithAlpha (0.8f);
        const auto cutOut = contrastWithAlpha (0.2f);
        RecordProperty ("keptContrast", String (kept, 3).toStdString());
        RecordProperty ("cutOutContrast", String (cutOut, 3).toStdString());

        EXPECT_GT (kept, 0.35);
        EXPECT_GE (cutOut, 0.0);
        EXPECT_LT (cutOut, 0.02);
    }

    /** The long ground seen from 1.6 m above it towards the horizon, with nothing on it, is lit
        on @p tier as if it cast no shadow: down five columns of the frame, from the far edge of the
        ground to the bottom, no cascade hands over to the next with a darker band, and no acne
        darkens the ground. The same view without shadows is the reference.
    */
    void checkCascadeTransitionHasNoSeam (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool castsShadows, Color& background)
        {
            Scene scene;
            addLongGround (scene).castsShadows = castsShadows;
            setTier (scene, tier);
            background = scene.getBackgroundColor();

            auto camera = EntityNode::Ptr (new EntityNode ("camera"));
            camera->attach<CameraNode>();
            aimCamera (*camera, { 0.0f, 1.6f, 4.0f }, { 0.0f, 0.0f, -1000.0f });
            scene.getRoot()->addChild (camera);
            scene.setActiveCamera (camera.get());

            SceneRenderer renderer;
            const auto image = renderConverged (renderer, scene, 256);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            return image;
        };

        Color background;
        const auto shadowed = render (true, background);
        const auto unshadowed = render (false, background);
        ASSERT_TRUE (shadowed.isValid() && unshadowed.isValid());

        auto worst = 1.0f;
        for (const auto x : { 64, 96, 128, 160, 192 })
        {
            // Antialiasing blends the far edge of the ground with the background: its first rows are left out
            auto lit = luminanceColumn (unshadowed, x, background);
            const auto column = luminanceColumn (shadowed, x, background);
            const auto firstGround = std::find_if (lit.begin(), lit.end(), [] (float value) { return value > 0.0f; });
            std::fill (firstGround, firstGround + std::min<std::ptrdiff_t> (4, std::distance (firstGround, lit.end())), 0.0f);

            const auto groundRows = std::count_if (lit.begin(), lit.end(), [] (float value) { return value > 0.0f; });
            EXPECT_GE (groundRows, 100) << "column " << x;

            for (size_t row = 0; row < column.size(); ++row)
            {
                if (lit[row] <= 0.0f)
                    continue;

                worst = jmin (worst, column[row] / lit[row]);

                if (column[row] < 0.95f * lit[row])
                {
                    ADD_FAILURE() << "column " << x << ", row " << row << ": " << column[row] << " against " << lit[row];
                    break;
                }
            }
        }

        RecordProperty ("darkestRelative", String (worst, 3).toStdString());
    }

    /** A 0.3 m cube, the entity "blocker", centered on @p position. */
    static void addBlocker (Scene& scene, Vector3<float> position)
    {
        auto blocker = EntityNode::Ptr (new EntityNode ("blocker"));
        blocker->attach<MeshNode> (makeBox (0.15f, makeWallMaterial()));
        blocker->setPosition (position);
        scene.getRoot()->addChild (blocker);
    }

    /** A closed 4 x 4 x 4 room around the origin, its walls facing inwards, lit by a shadow casting
        point light at the origin (intensity 20, no range), with a 0.3 m blocker cube 1 m from the
        light on each of the six axes when @p withBlockers is set: each blocker shadows the middle of
        the wall behind it, and the wall around that shadow stays lit. Returns the light.
    */
    static LightNode& buildPointLightRoom (Scene& scene, bool withBlockers = true)
    {
        auto room = EntityNode::Ptr (new EntityNode ("room"));
        room->attach<MeshNode> (makeBox (2.0f, makeWallMaterial(), true));
        scene.getRoot()->addChild (room);

        if (withBlockers)
            for (const auto& axis : getCubeAxes())
                addBlocker (scene, axis);

        auto entity = EntityNode::Ptr (new EntityNode ("point"));
        auto& light = entity->attach<LightNode> (LightNode::Type::point);
        light.intensity = 20.0f;
        light.castsShadows = true;
        scene.getRoot()->addChild (entity);
        return light;
    }

    /** The six unit axes, in the order of the faces of a point light shadow: +X, -X, +Y, -Y, +Z, -Z. */
    static std::array<Vector3<float>, 6> getCubeAxes()
    {
        return { Vector3<float> (1.0f, 0.0f, 0.0f), Vector3<float> (-1.0f, 0.0f, 0.0f), Vector3<float> (0.0f, 1.0f, 0.0f),
                 Vector3<float> (0.0f, -1.0f, 0.0f), Vector3<float> (0.0f, 0.0f, 1.0f), Vector3<float> (0.0f, 0.0f, -1.0f) };
    }

    /** Adds a shadow casting point light of intensity 1 at @p position. */
    static void addShadowedPointLight (Scene& scene, Vector3<float> position)
    {
        auto entity = EntityNode::Ptr (new EntityNode ("shadowedPoint"));
        entity->attach<LightNode> (LightNode::Type::point).castsShadows = true;
        entity->setPosition (position);
        scene.getRoot()->addChild (entity);
    }

    /** Adds a camera at @p eye looking at @p target, and makes it the active one. */
    static void addCamera (Scene& scene, Vector3<float> eye, Vector3<float> target, Vector3<float> up = { 0.0f, 1.0f, 0.0f })
    {
        auto camera = EntityNode::Ptr (new EntityNode ("camera"));
        camera->attach<CameraNode>();
        aimCamera (*camera, eye, target, up);
        scene.getRoot()->addChild (camera);
        scene.setActiveCamera (camera.get());
    }

    /** A 10 x 10 ground at y = 0 under a shadow casting spot light at (0, 3, 0) shining down
        (intensity 20, outer cone 45 degrees), with a 0.3 m blocker cube at @p blocker. The camera
        looks at the foot of the spot from (0, 3, 4), so the default blocker at (0, 1, 0) hides a
        patch of ground behind its shadow, not the shadow. Returns the spot.
    */
    static LightNode& buildSpotOverGround (Scene& scene, Vector3<float> blocker = { 0.0f, 1.0f, 0.0f })
    {
        addGround (scene, 5.0f);
        addBlocker (scene, blocker);

        auto entity = EntityNode::Ptr (new EntityNode ("spot"));
        auto& spot = entity->attach<LightNode> (LightNode::Type::spot);
        spot.intensity = 20.0f;
        spot.castsShadows = true;
        aimCamera (*entity, { 0.0f, 3.0f, 0.0f }, {}, { 0.0f, 0.0f, -1.0f });
        scene.getRoot()->addChild (entity);

        addCamera (scene, { 0.0f, 3.0f, 4.0f }, {});
        return spot;
    }

    /** The point light at the center of buildPointLightRoom() shadows the wall behind a single
        blocker on each of the six axes on @p tier: seen from beside the light, the shadow darkens the
        wall by more than half, against the same view without shadows. One blocker at a time, so the
        shadow can only come from the face of that axis.
    */
    void checkPointLightShadowsOnAllSixFaces (RenderSettings::TierPreference tier)
    {
        const auto axes = getCubeAxes();
        for (size_t face = 0; face < axes.size(); ++face)
        {
            const auto axis = axes[face];
            const auto side = axes[(face + 2) % 6] + axes[(face + 4) % 6];

            const auto render = [&] (bool castsShadows)
            {
                Scene scene;
                buildPointLightRoom (scene, false).castsShadows = castsShadows;
                addBlocker (scene, axis);
                setTier (scene, tier);
                addCamera (scene, side * 0.9f - axis * 0.5f, axis * 2.0f, axes[(face + 2) % 6]);

                SceneRenderer renderer;
                const auto image = renderConverged (renderer, scene, 128);
                EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
                return image;
            };

            const auto shadowed = render (true);
            const auto unshadowed = render (false);
            ASSERT_TRUE (shadowed.isValid() && unshadowed.isValid());

            const auto contrast = shadowContrast (unshadowed, shadowed);
            RecordProperty ((String ("contrastFace") + String (static_cast<int> (face))).toStdString(), String (contrast, 3).toStdString());
            EXPECT_GT (contrast, 0.5) << "face " << axis.getX() << "," << axis.getY() << "," << axis.getZ();
        }
    }

    /** The empty room of buildPointLightRoom(), its light moved off the center so that every face
        of its shadow sees the walls differently and the face edges cross the walls away from their
        corners, is lit on @p tier as if the light cast no shadow, seen from the light along the
        diagonal where three faces meet: no pixel darkens by more than 5% on raster, 2% on full, so
        no face edge leaves a crack and nothing acnes. The full tier filters with a smaller kernel
        than the raster tent, so a crack darkens less there: its threshold is tighter. Checked at the
        default shadow resolution, and at the smallest, whose coarse texels widen a crack on screen.
        A @p radius above 0 checks the soft shadows of a light of that size instead, whose wide
        kernels must not shadow the walls with themselves either.
    */
    void checkPointShadowHasNoFaceSeams (RenderSettings::TierPreference tier, float radius = 0.0f)
    {
        const Vector3<float> light (0.5f, -0.3f, 0.4f);
        const auto threshold = tier == RenderSettings::TierPreference::full ? 0.98 : 0.95;

        for (const auto resolution : { RenderSettings().shadowResolution, RenderCapabilities::minShadowResolution })
        {
            const auto render = [&] (bool castsShadows)
            {
                Scene scene;
                auto& pointLight = buildPointLightRoom (scene, false);
                pointLight.castsShadows = castsShadows;
                pointLight.radius = radius;
                scene.getRoot()->findChild ("point")->setPosition (light);
                addCamera (scene, light, light + Vector3<float> (1.0f, 1.0f, 1.0f), { 0.0f, 0.0f, 1.0f });

                auto settings = scene.getRenderSettings();
                settings.tier = tier;
                settings.shadowResolution = resolution;
                if (radius > 0.0f)
                    settings.softShadows = true;

                scene.setRenderSettings (settings);

                SceneRenderer renderer;
                const auto image = renderConverged (renderer, scene, 256);
                EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
                return image;
            };

            const auto shadowed = render (true);
            const auto unshadowed = render (false);
            ASSERT_TRUE (shadowed.isValid() && unshadowed.isValid());

            auto worst = 1.0;
            auto darkened = 0;
            for (int y = 0; y < shadowed.getHeight(); ++y)
            {
                for (int x = 0; x < shadowed.getWidth(); ++x)
                {
                    const auto lit = meanLuminance (unshadowed, { x, y, 1, 1 });
                    if (lit <= 10.0)
                        continue;

                    const auto relative = meanLuminance (shadowed, { x, y, 1, 1 }) / lit;
                    worst = jmin (worst, relative);
                    darkened += relative < threshold ? 1 : 0;
                }
            }

            const auto suffix = String (resolution).toStdString();
            RecordProperty ("darkestRelative" + suffix, String (worst, 3).toStdString());
            RecordProperty ("darkenedPixels" + suffix, darkened);
            EXPECT_EQ (darkened, 0) << "resolution " << resolution << ", darkest " << worst;
        }
    }

    /** The spot of buildSpotOverGround() with a radius of @p radius casts the blocker's shadow on
        the ground on @p tier: the shadow darkens the ground by more than 35%. */
    void checkSpotCastsAShadow (RenderSettings::TierPreference tier, float radius)
    {
        const auto render = [&] (bool castsShadows)
        {
            Scene scene;
            auto& spot = buildSpotOverGround (scene);
            spot.castsShadows = castsShadows;
            spot.radius = radius;
            setTier (scene, tier);

            SceneRenderer renderer;
            const auto image = renderConverged (renderer, scene, 128);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            return image;
        };

        const auto shadowed = render (true);
        const auto unshadowed = render (false);
        ASSERT_TRUE (shadowed.isValid() && unshadowed.isValid());

        const auto contrast = shadowContrast (unshadowed, shadowed);
        RecordProperty ("contrast", String (contrast, 3).toStdString());
        EXPECT_GT (contrast, 0.35);
    }

    /** The centroid on the ground y = 0 of the pixels of @p shadowed darker than 70% of
        @p unshadowed, among those brighter than 10 levels in @p unshadowed, both seen through
        @p viewProjection: each pixel is traced back to the ground and weighs the ground area it
        covers, so the perspective of the camera does not move the centroid. Returns nothing when
        no pixel is shadowed.
    */
    static std::optional<Vector3<float>> shadowCentroidOnGround (const Image& unshadowed, const Image& shadowed, const Matrix4& viewProjection)
    {
        const auto inverse = viewProjection.inverted();
        const auto width = static_cast<float> (unshadowed.getWidth());
        const auto height = static_cast<float> (unshadowed.getHeight());

        // Where the ray through the point (x, y) of the image, in pixels from the top left, meets the ground
        const auto toGround = [&] (float x, float y)
        {
            const auto ndcX = x / width * 2.0f - 1.0f;
            const auto ndcY = 1.0f - y / height * 2.0f;
            const auto nearPoint = inverse.transformPoint ({ ndcX, ndcY, 0.0f });
            const auto farPoint = inverse.transformPoint ({ ndcX, ndcY, 0.5f });
            return nearPoint + (farPoint - nearPoint) * (nearPoint.getY() / (nearPoint.getY() - farPoint.getY()));
        };

        Vector3<float> sum;
        auto area = 0.0f;

        for (int y = 0; y < unshadowed.getHeight(); ++y)
        {
            for (int x = 0; x < unshadowed.getWidth(); ++x)
            {
                const auto lit = meanLuminance (unshadowed, { x, y, 1, 1 });
                if (lit <= 10.0 || meanLuminance (shadowed, { x, y, 1, 1 }) >= 0.7 * lit)
                    continue;

                // Each pixel weighs the ground area it covers, which shrinks with the distance
                const auto corner = toGround (static_cast<float> (x), static_cast<float> (y));
                const auto right = toGround (static_cast<float> (x + 1), static_cast<float> (y)) - corner;
                const auto down = toGround (static_cast<float> (x), static_cast<float> (y + 1)) - corner;
                const auto weight = std::abs (right.getX() * down.getZ() - right.getZ() * down.getX());

                sum = sum + toGround (static_cast<float> (x) + 0.5f, static_cast<float> (y) + 0.5f) * weight;
                area += weight;
            }
        }

        if (! (area > 0.0f))
            return std::nullopt;

        return sum * (1.0f / area);
    }

    /** A spot of radius 0.3 shadows on @p tier from its center. A blocker 1 m under it and off its
        axis, on the diagonal x = z, is symmetric about the vertical plane x = z holding the spot's
        center, so its shadow centers on the ground line x = z, where the line from the center of
        the spot through the blocker meets the ground: the shadow's centroid lies within 5 cm of that
        line, and within 2 cm of the shadow of the same spot without a radius. Shadowed from a point
        a radius away from the center along x, it would center about 0.4 m off the line.
    */
    void checkAreaSpotCastsAShadowTowardsItsCenter (RenderSettings::TierPreference tier)
    {
        const Vector3<float> blocker (0.35f, 2.0f, 0.35f);
        constexpr int size = 256;

        const auto render = [&] (bool castsShadows, float radius)
        {
            Scene scene;
            auto& spot = buildSpotOverGround (scene, blocker);
            spot.castsShadows = castsShadows;
            spot.radius = radius;
            setTier (scene, tier);

            // The direction of the lookup is under test, not the filter: both radii filter alike
            auto settings = scene.getRenderSettings();
            settings.softShadows = false;
            scene.setRenderSettings (settings);

            SceneRenderer renderer;
            const auto image = renderConverged (renderer, scene, size);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            return image;
        };

        const auto unshadowed = render (false, 0.3f);
        const auto area = render (true, 0.3f);
        const auto punctual = render (true, 0.0f);
        ASSERT_TRUE (unshadowed.isValid() && area.isValid() && punctual.isValid());

        // The camera of buildSpotOverGround()
        const auto camera = Matrix4::lookAt ({ 0.0f, 3.0f, 4.0f }, {}, { 0.0f, 1.0f, 0.0f }).followedBy (Matrix4::perspective (CameraNode().yFov, 1.0f, 0.1f, 1000.0f));
        const auto areaCentroid = shadowCentroidOnGround (unshadowed, area, camera);
        const auto punctualCentroid = shadowCentroidOnGround (unshadowed, punctual, camera);
        ASSERT_TRUE (areaCentroid.has_value() && punctualCentroid.has_value());

        const auto offLine = std::abs (areaCentroid->getX() - areaCentroid->getZ()) / std::sqrt (2.0f);
        const auto toPunctual = (*areaCentroid - *punctualCentroid).length();
        RecordProperty ("areaOffTheLine", String (offLine, 3).toStdString());
        RecordProperty ("areaToPunctual", String (toPunctual, 3).toStdString());

        EXPECT_LT (offLine, 0.05f) << areaCentroid->getX() << ", " << areaCentroid->getZ();
        EXPECT_LT (toPunctual, 0.02f) << areaCentroid->getX() << ", " << areaCentroid->getZ();
    }

    /** buildPointLightRoom() with 40 more shadow casting point lights renders on @p tier without an
        error, recording whole lights within the shadow record budget of the tier, and stays finite in
        raw radiance (no temporal anti-aliasing on full, the rgba16float bloom path on raster).
    */
    void checkManyShadowedLightsRenderWithoutError (RenderSettings::TierPreference tier)
    {
        const auto full = tier == RenderSettings::TierPreference::full;

        Scene scene;
        buildPointLightRoom (scene);
        for (int i = 0; i < 40; ++i)
            addShadowedPointLight (scene, { std::sin (static_cast<float> (i)) * 1.5f, std::cos (static_cast<float> (i) * 0.7f) * 1.5f, 0.5f });

        if (full)
            setFullTierTaa (scene, 1);
        else
        {
            setTier (scene, RenderSettings::TierPreference::raster);
            setBloom (scene, true);
        }

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        ASSERT_TRUE (image.isValid());

        // 41 point lights of 6 faces each, without a sun: 2 fit the 16 local records of raster, 10 the 64 of full
        EXPECT_EQ (shadows (renderer).getNumRecords(), full ? 60 : 12);

#if YUP_ENABLE_SHADER_TRANSPILER
        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        ASSERT_NE (sceneColor, nullptr);
        EXPECT_EQ (sceneColor->getFormat(), GpuTextureFormat::rgba16float);
        EXPECT_TRUE (isFiniteTexture (sceneColor));
        EXPECT_FALSE (isSaturatedTexture (sceneColor));
#endif
    }

    /** Sets the tier of @p scene for an exact comparison of frames: raster, or full without temporal
        anti-aliasing (no jitter and a still shadow noise).
    */
    static void setTierForExactFrames (Scene& scene, RenderSettings::TierPreference tier)
    {
        if (tier == RenderSettings::TierPreference::full)
            setFullTierTaa (scene, 1);
        else
            setTier (scene, RenderSettings::TierPreference::raster);
    }

    /** Sets castsShadows on every light of @p scene. */
    static void setAllLightsCastShadows (Scene& scene, bool castsShadows)
    {
        for (const auto& child : scene.getRoot()->getChildren())
            if (auto* light = child->getNode<LightNode>())
                light->castsShadows = castsShadows;
    }

    /** A device unable to create the comparison sampler cannot read shadow maps: on @p tier it
        renders every frame, without the shadows, as if the sun cast none.
    */
    void checkRendersWithoutTheShadowCompareSampler (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool withSampler, bool castsShadows)
        {
            Scene scene;
            buildShadowsScene (scene);
            scene.getRoot()->findChild ("sun")->getNode<LightNode>()->castsShadows = castsShadows;
            setTierForExactFrames (scene, tier);

            SceneRenderer renderer;
            if (! withSampler)
                renderer.failShadowCompareSamplerForTesting();

            // The failure is remembered, so the second frame renders like the first
            Image image;
            for (int frame = 0; frame < 2; ++frame)
            {
                image = renderToImage (renderer, scene, 128);
                EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            }

            return image;
        };

        const auto shadowed = render (true, true);
        const auto withoutSampler = render (false, true);
        const auto unshadowed = render (true, false);
        ASSERT_TRUE (shadowed.isValid() && withoutSampler.isValid() && unshadowed.isValid());

        EXPECT_EQ (countDifferences (withoutSampler, unshadowed), 0);
        EXPECT_GT (countDifferences (shadowed, unshadowed), 0);
    }

    /** Shadow pipelines the device cannot compile leave every frame of @p tier unshadowed, not
        failed: the sun scene and the spot scene render as without castsShadows, frame after frame.
    */
    void checkRendersWithoutTheShadowPipelines (RenderSettings::TierPreference tier)
    {
        for (const auto spotScene : { false, true })
        {
            const auto render = [&] (bool failPipelines, bool castsShadows)
            {
                Scene scene;
                if (spotScene)
                    buildSpotOverGround (scene);
                else
                    buildShadowsScene (scene);

                setAllLightsCastShadows (scene, castsShadows);
                setTierForExactFrames (scene, tier);

                SceneRenderer renderer;
                if (failPipelines)
                    renderer.failShadowPipelinesForTesting();

                Image image;
                for (int frame = 0; frame < 2; ++frame)
                {
                    image = renderToImage (renderer, scene, 128);
                    EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
                    if (failPipelines)
                        EXPECT_EQ (shadows (renderer).getNumRecords(), 0);
                }

                return image;
            };

            const auto shadowed = render (false, true);
            const auto withoutPipelines = render (true, true);
            const auto unshadowed = render (false, false);
            ASSERT_TRUE (shadowed.isValid() && withoutPipelines.isValid() && unshadowed.isValid());

            EXPECT_EQ (countDifferences (withoutPipelines, unshadowed), 0) << (spotScene ? "spot" : "sun");
            EXPECT_GT (countDifferences (shadowed, unshadowed), 0) << (spotScene ? "spot" : "sun");
        }
    }

    /** Shadow maps the device cannot create (out of memory) leave the lights needing them
        unshadowed on @p tier, not the frame failed. The same size is not asked for again, so the
        next frame stays unshadowed without retrying it; a new shadowResolution asks again, and the
        shadows come back.
    */
    void checkShadowMapsTheDeviceCannotCreateLeaveTheLightsUnshadowed (RenderSettings::TierPreference tier)
    {
        for (const auto spotScene : { false, true })
        {
            const auto build = [&] (Scene& scene, bool castsShadows)
            {
                if (spotScene)
                    buildSpotOverGround (scene);
                else
                    buildShadowsScene (scene);

                setAllLightsCastShadows (scene, castsShadows);
                setTierForExactFrames (scene, tier);
            };

            Scene unshadowedScene;
            build (unshadowedScene, false);
            SceneRenderer unshadowedRenderer;

            Scene scene;
            build (scene, true);
            SceneRenderer renderer;
            renderer.failShadowMapsForTesting();

            // The failing frame, then one that would succeed if it asked again
            for (int frame = 0; frame < 2; ++frame)
            {
                const auto image = renderToImage (renderer, scene, 128);
                const auto unshadowed = renderToImage (unshadowedRenderer, unshadowedScene, 128);
                ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
                ASSERT_TRUE (image.isValid() && unshadowed.isValid());

                EXPECT_EQ (countDifferences (image, unshadowed), 0) << (spotScene ? "spot" : "sun") << ", frame " << frame;
                EXPECT_EQ (shadows (renderer).getNumRecords(), 0);
                EXPECT_EQ (shadows (renderer).getAtlasSize(), 0);
            }

            auto settings = scene.getRenderSettings();
            settings.shadowResolution = 1024;
            scene.setRenderSettings (settings);

            const auto image = renderToImage (renderer, scene, 128);
            const auto unshadowed = renderToImage (unshadowedRenderer, unshadowedScene, 128);
            ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            EXPECT_GT (shadows (renderer).getNumRecords(), 0);
            EXPECT_GT (countDifferences (image, unshadowed), 0) << (spotScene ? "spot" : "sun");
        }
    }

    /** On @p tier, a shadow map the device could not create does not cost the other one its cache:
        with the sun's cascades drawn, an atlas that fails leaves a still scene drawing no shadow map
        frame after frame, and so do failed cascades next to a drawn atlas.
    */
    void checkFailedShadowMapKeepsTheOtherOneCached (RenderSettings::TierPreference tier)
    {
        for (const auto atlasFails : { true, false })
        {
            Scene scene;
            auto& spot = buildSpotOverGround (scene);
            auto sunEntity = EntityNode::Ptr (new EntityNode ("sun"));
            auto& sun = sunEntity->attach<LightNode>();
            sun.intensity = 2.0f;
            aimCamera (*sunEntity, { 2.0f, 6.0f, 3.0f }, {}, { 0.0f, 1.0f, 0.0f });
            scene.getRoot()->addChild (sunEntity);
            setTier (scene, tier);

            // The map that survives exists first; the other one fails when it is first needed
            sun.castsShadows = atlasFails;
            spot.castsShadows = ! atlasFails;

            SceneRenderer renderer;
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

            renderer.failShadowMapsForTesting();
            sun.castsShadows = true;
            spot.castsShadows = true;
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_EQ (shadows (renderer).getAtlasSize() == 0, atlasFails);

            const auto renders = shadows (renderer).getNumRenderedFrames();
            const auto cascadeRenders = shadows (renderer).getNumCascadeRenders (0);
            const auto draws = shadows (renderer).getNumCasterDraws();

            for (int frame = 0; frame < 3; ++frame)
                ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

            EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), renders) << (atlasFails ? "atlas fails" : "cascades fail");
            EXPECT_EQ (shadows (renderer).getNumCascadeRenders (0), cascadeRenders) << (atlasFails ? "atlas fails" : "cascades fail");
            EXPECT_EQ (shadows (renderer).getNumCasterDraws(), draws) << (atlasFails ? "atlas fails" : "cascades fail");
            EXPECT_TRUE (renderer.waitForGPU().wasOk());
        }
    }

    /** A shadowed point light whose range reaches no receiver takes no tile on @p tier, and the frame
        is the one it gives without shadows; brought within reach of the ground it takes its six faces.
    */
    void checkShadowedLightOutOfReachAllocatesNoAtlas (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (Vector3<float> position, bool castsShadows, int& records, int& atlasSize)
        {
            Scene scene;
            buildSpotOverGround (scene).castsShadows = false;
            setTier (scene, tier);

            auto entity = EntityNode::Ptr (new EntityNode ("point"));
            auto& light = entity->attach<LightNode> (LightNode::Type::point);
            light.range = 1.0f;
            light.castsShadows = castsShadows;
            entity->setPosition (position);
            scene.getRoot()->addChild (entity);

            SceneRenderer renderer;
            const auto image = renderConverged (renderer, scene, 64);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            records = shadows (renderer).getNumRecords();
            atlasSize = shadows (renderer).getAtlasSize();
            return image;
        };

        int records = -1, atlasSize = -1;
        const auto far = render ({ 0.0f, 50.0f, 0.0f }, true, records, atlasSize);
        EXPECT_EQ (records, 0);
        EXPECT_EQ (atlasSize, 0);

        const auto unshadowed = render ({ 0.0f, 50.0f, 0.0f }, false, records, atlasSize);
        ASSERT_TRUE (far.isValid() && unshadowed.isValid());
        EXPECT_EQ (countDifferences (far, unshadowed), 0);

        render ({ 2.0f, 0.5f, 0.0f }, true, records, atlasSize);
        EXPECT_EQ (records, 6);
        EXPECT_GT (atlasSize, 0);
    }

    /** One renderer on @p tier releases the atlas when its only shadowed spot stops casting
        shadows, and creates it again when it casts them again.
    */
    void checkAtlasIsReleasedWhenNoLightNeedsIt (RenderSettings::TierPreference tier)
    {
        Scene scene;
        auto& spot = buildSpotOverGround (scene);
        setTier (scene, tier);

        SceneRenderer renderer;
        for (const auto castsShadows : { true, false, true })
        {
            spot.castsShadows = castsShadows;
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

            if (castsShadows)
                EXPECT_GT (shadows (renderer).getAtlasSize(), 0);
            else
                EXPECT_EQ (shadows (renderer).getAtlasSize(), 0);
        }

        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    /** The spot of buildSpotOverGround() and a shadowed point light of 1.2 m range at (4, 1, 0), its
        own blocker under it at (4, 0.4, 0), out of the spot's cone, plus a third blocker at (-4, 0.3,
        -4) which no light reaches, seen from above both lights. Returns the point light's blocker,
        the spot's blocker and the third one.
    */
    static std::array<EntityNode*, 3> buildSpotAndPointOverGround (Scene& scene)
    {
        buildSpotOverGround (scene);
        auto* spotBlocker = scene.getRoot()->findChild ("blocker");

        auto entity = EntityNode::Ptr (new EntityNode ("point"));
        auto& point = entity->attach<LightNode> (LightNode::Type::point);
        point.intensity = 4.0f;
        point.range = 1.2f;
        point.castsShadows = true;
        entity->setPosition ({ 4.0f, 1.0f, 0.0f });
        scene.getRoot()->addChild (entity);

        addBlocker (scene, { 4.0f, 0.4f, 0.0f });
        auto* pointBlocker = scene.getRoot()->getChildren().back().get();

        addBlocker (scene, { -4.0f, 0.3f, -4.0f });
        auto* unlitBlocker = scene.getRoot()->getChildren().back().get();

        addCamera (scene, { 2.0f, 6.0f, 6.0f }, { 2.0f, 0.0f, 0.0f });
        return { pointBlocker, spotBlocker, unlitBlocker };
    }

    /** On @p tier, a caster moving outside a light's reach does not draw that light's atlas views
        again: moving the point light's blocker costs the draws it costs without the spot's shadow,
        and moving a blocker no light reaches draws no shadow map at all.
    */
    void checkAtlasRedrawsOnlyTheViewsACasterReaches (RenderSettings::TierPreference tier)
    {
        const auto drawsOfAMove = [&] (bool spotCastsShadows)
        {
            Scene scene;
            const auto blockers = buildSpotAndPointOverGround (scene);
            scene.getRoot()->findChild ("spot")->getNode<LightNode>()->castsShadows = spotCastsShadows;
            setTier (scene, tier);

            SceneRenderer renderer;
            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            const auto drawsBefore = shadows (renderer).getNumCasterDraws();

            blockers[0]->setPosition (blockers[0]->getPosition() + Vector3<float> (0.1f, 0.0f, 0.0f));
            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk());
            return shadows (renderer).getNumCasterDraws() - drawsBefore;
        };

        const auto draws = drawsOfAMove (true);
        EXPECT_GT (draws, 0);
        EXPECT_EQ (draws, drawsOfAMove (false));

        Scene scene;
        const auto blockers = buildSpotAndPointOverGround (scene);
        setTier (scene, tier);

        SceneRenderer renderer;
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);

        blockers[2]->setPosition (blockers[2]->getPosition() + Vector3<float> (0.1f, 0.0f, 0.0f));
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    /** On @p tier, an atlas whose views were drawn again one light at a time, each cleared alone
        while the other tiles were kept, holds what a fresh renderer draws at once: the frames match
        exactly.
    */
    void checkPartiallyRedrawnAtlasMatchesAFreshOne (RenderSettings::TierPreference tier)
    {
        Scene scene;
        const auto blockers = buildSpotAndPointOverGround (scene);
        setTierForExactFrames (scene, tier);

        SceneRenderer renderer;
        ASSERT_TRUE (renderToImage (renderer, scene, 128).isValid());

        // Only the point light's views, then only the spot's
        for (auto* blocker : { blockers[0], blockers[1] })
        {
            const auto drawsBefore = shadows (renderer).getNumCasterDraws();
            blocker->setPosition (blocker->getPosition() + Vector3<float> (0.1f, 0.0f, 0.05f));
            ASSERT_TRUE (renderToImage (renderer, scene, 128).isValid());
            ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            EXPECT_GT (shadows (renderer).getNumCasterDraws(), drawsBefore);
        }

        const auto redrawn = renderToImage (renderer, scene, 128);

        SceneRenderer fresh;
        const auto drawnAtOnce = renderToImage (fresh, scene, 128);
        ASSERT_TRUE (redrawn.isValid() && drawnAtOnce.isValid());
        EXPECT_EQ (countDifferences (redrawn, drawnAtOnce), 0);

        // The spot's view and the point light's six
        EXPECT_EQ (shadows (renderer).getNumRecords(), 7);
    }

    /** buildSpotOverGround() plus @p extraSpots more shadow casting spots like it, 2 m from it on the
        diagonals, each over its own blocker, set 15 cm further off its axis than the previous one so
        no two spots draw the same depth: the camera is inside the reach of every spot, so each asks
        for the largest tile. Returns the extra spots. */
    static std::vector<LightNode*> buildSpotsOverGround (Scene& scene, int extraSpots)
    {
        buildSpotOverGround (scene);

        std::vector<LightNode*> spots;
        for (int i = 0; i < extraSpots; ++i)
        {
            const Vector3<float> foot ((i % 2 == 0 ? 2.0f : -2.0f), 0.0f, (i / 2 % 2 == 0 ? 2.0f : -2.0f));
            addBlocker (scene, foot + Vector3<float> (0.15f * static_cast<float> (i + 1), 1.0f, 0.0f));

            auto entity = EntityNode::Ptr (new EntityNode ("spot" + String (i + 1)));
            auto& spot = entity->attach<LightNode> (LightNode::Type::spot);
            spot.intensity = 20.0f;
            spot.castsShadows = true;
            aimCamera (*entity, foot + Vector3<float> (0.0f, 3.0f, 0.0f), foot, { 0.0f, 0.0f, -1.0f });
            scene.getRoot()->addChild (entity);
            spots.push_back (&spot);
        }

        return spots;
    }

    /** The tiles of the point and spot light views of @p renderer's last frame, in atlas texels:
        the records with a map after its @p numCascades cascades. */
    static std::vector<Rectangle<int>> getLocalViewTiles (const SceneRenderer& renderer, int numCascades)
    {
        const auto data = shadows (renderer).getShadowData();
        const auto atlasSize = static_cast<float> (shadows (renderer).getAtlasSize());
        std::vector<Rectangle<int>> tiles;

        for (int r = numCascades; r < shadows (renderer).getNumRecords(); ++r)
        {
            const auto& rect = data.records[r].atlasRect;
            if (rect[2] > 0.0f)
                tiles.emplace_back (roundToInt (rect[0] * atlasSize), roundToInt (rect[1] * atlasSize), roundToInt (rect[2] * atlasSize), roundToInt (rect[2] * atlasSize));
        }

        return tiles;
    }

    /** On @p tier, a single shadow casting spot light near the camera gets a tile of half the atlas side. */
    void checkSingleSpotTakesHalfTheAtlas (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildSpotOverGround (scene);
        setTier (scene, tier);

        SceneRenderer renderer;
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_TRUE (renderer.waitForGPU().wasOk());

        ASSERT_GT (shadows (renderer).getAtlasSize(), 0);
        EXPECT_EQ (shadows (renderer).getTileSize (0), shadows (renderer).getAtlasSize() / 2);
    }

    /** On @p tier, spot lights each asking for the largest tile keep half the atlas side while all of
        them fit at that size (4 of them), and shrink back to a quarter of it, the cap before it was
        raised, once one more would be left without a tile: every spot keeps a tile inside the atlas,
        and no two tiles overlap. */
    void checkLocalViewsShrinkToFitTheAtlas (RenderSettings::TierPreference tier)
    {
        for (const auto& [extraSpots, divisor] : { std::pair { 3, 2 }, std::pair { 4, 4 }, std::pair { 7, 4 } })
        {
            Scene scene;
            buildSpotsOverGround (scene, extraSpots);
            setTier (scene, tier);

            SceneRenderer renderer;
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk());

            const auto atlasSize = shadows (renderer).getAtlasSize();
            ASSERT_GT (atlasSize, 0);

            for (int light = 0; light <= extraSpots; ++light)
                EXPECT_EQ (shadows (renderer).getTileSize (light), atlasSize / divisor) << extraSpots + 1 << " spots, light " << light;

            const auto tiles = getLocalViewTiles (renderer, 0);
            ASSERT_EQ (static_cast<int> (tiles.size()), extraSpots + 1);

            for (size_t i = 0; i < tiles.size(); ++i)
            {
                EXPECT_TRUE (Rectangle<int> (0, 0, atlasSize, atlasSize).contains (tiles[i]));

                for (size_t j = i + 1; j < tiles.size(); ++j)
                    EXPECT_FALSE (tiles[i].intersection (tiles[j]).area() > 0) << extraSpots + 1 << " spots, views " << i << " and " << j;
            }
        }
    }

    /** On @p tier, three spot lights asking for half the atlas side do not squeeze a shadowed point
        light after them below the tile it gets with tiles of at most a quarter: on half tiles the
        spots would take three quarters of the atlas and leave the point light's six views an eighth
        of the side, so every light keeps a quarter, without overlaps. */
    void checkLocalLightsKeepTheirQuarterTiles (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildSpotsOverGround (scene, 2);
        addShadowedPointLight (scene, { 0.0f, 1.5f, 2.0f });
        setTier (scene, tier);

        SceneRenderer renderer;
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_TRUE (renderer.waitForGPU().wasOk());

        const auto atlasSize = shadows (renderer).getAtlasSize();
        ASSERT_GT (atlasSize, 0);

        for (int light = 0; light < 4; ++light)
            EXPECT_EQ (shadows (renderer).getTileSize (light), atlasSize / 4) << "light " << light;

        const auto tiles = getLocalViewTiles (renderer, 0);
        ASSERT_EQ (tiles.size(), 9u);

        for (size_t i = 0; i < tiles.size(); ++i)
            for (size_t j = i + 1; j < tiles.size(); ++j)
                EXPECT_FALSE (tiles[i].intersection (tiles[j]).area() > 0) << "views " << i << " and " << j;
    }

    /** On @p tier, the atlas layout follows the spot lights casting shadows, and each frame draws the
        views whose tile moved again (one more shadow map render) and matches, exactly, a fresh
        renderer drawing the same scene at once: four spots on half the atlas side, then eight
        shrinking to a quarter, then seven, which moves the views after the first one to the tiles
        before them at the same size, then the four again. */
    void checkAtlasRelayoutRedrawsTheMovedTiles (RenderSettings::TierPreference tier)
    {
        Scene scene;
        auto spots = buildSpotsOverGround (scene, 7);
        spots.insert (spots.begin(), scene.getRoot()->findChild ("spot")->getNode<LightNode>());
        setTierForExactFrames (scene, tier);

        const auto castShadows = [&] (int first, int last)
        {
            for (int i = 0; i < static_cast<int> (spots.size()); ++i)
                spots[static_cast<size_t> (i)]->castsShadows = i >= first && i < last;
        };

        castShadows (0, 4);
        SceneRenderer renderer;
        ASSERT_TRUE (renderToImage (renderer, scene, 128).isValid());

        const auto atlasSize = shadows (renderer).getAtlasSize();
        EXPECT_EQ (shadows (renderer).getTileSize (1), atlasSize / 2);

        const struct
        {
            const char* name;
            int first, last, divisor;
        } steps[] = {
            { "eight spots", 0, 8, 4 },
            { "seven spots", 1, 8, 4 },
            { "four spots", 0, 4, 2 },
        };

        for (const auto& step : steps)
        {
            const auto renders = shadows (renderer).getNumRenderedFrames();
            castShadows (step.first, step.last);

            const auto relaidOut = renderToImage (renderer, scene, 128);
            ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), renders + 1) << step.name;
            EXPECT_EQ (shadows (renderer).getTileSize (1), atlasSize / step.divisor) << step.name;

            SceneRenderer fresh;
            const auto drawnAtOnce = renderToImage (fresh, scene, 128);
            ASSERT_TRUE (relaidOut.isValid() && drawnAtOnce.isValid());
            EXPECT_EQ (countDifferences (relaidOut, drawnAtOnce), 0) << step.name;
        }
    }

    /** The scene of the soft shadow tests: a wall 1.2 m high and 2 cm thin across the 4 m ground,
        from x = -2 to its end at x = 0 on z = 0, lit from behind (-z) by a shadow casting sun 45
        degrees high with an angular radius of @p sunRadius. Its shadow covers x < 0 from z = 0 to
        z = 1.2, so the side of the shadow cast by the wall's end runs along z at x = 0, ever further
        below the edge casting it. An orthographic camera looks straight down at x in [-0.85, 0.85]
        and z in [0.05, 1.75]: the rows of the frame run across that side, which crosses the center
        column, and the shadow touches the edge of the frame at the wall. Returns the sun.
    */
    static LightNode& buildPenumbraScene (Scene& scene, float sunRadius)
    {
        addGround (scene, 2.0f);

        auto wall = EntityNode::Ptr (new EntityNode ("wall"));
        wall->attach<MeshNode> (makeBox (0.5f, makeWallMaterial()));
        wall->setScale ({ 2.0f, 1.2f, 0.02f });
        wall->setPosition ({ -1.0f, 0.6f, -0.01f });
        scene.getRoot()->addChild (wall);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        auto& light = sun->attach<LightNode>();
        light.castsShadows = true;
        light.radius = sunRadius;
        aimCamera (*sun, { 0.0f, 7.0f, -7.0f }, {});
        scene.getRoot()->addChild (sun);

        auto camera = EntityNode::Ptr (new EntityNode ("camera"));
        auto& lens = camera->attach<CameraNode>();
        lens.projection = CameraNode::Projection::orthographic;
        lens.xMag = 0.85f;
        lens.yMag = 0.85f;
        aimCamera (*camera, { 0.0f, 5.0f, 0.9f }, { 0.0f, 0.0f, 0.9f }, { 0.0f, 0.0f, -1.0f });
        scene.getRoot()->addChild (camera);
        scene.setActiveCamera (camera.get());

        scene.setAmbientColor (Color (0xff3f3f3f)); // 0.05 once linear
        return light;
    }

    /** buildPenumbraScene() on @p tier at 256 x 256, converged, with soft shadows set to @p softShadows.
        A @p phase above 0 renders that many frames first and restarts the accumulation, so the
        converged frames run through other jitter offsets and noise rotations: the frame index,
        which drives both, keeps counting across a restart. */
    Image renderPenumbraScene (RenderSettings::TierPreference tier, float sunRadius, bool softShadows, int phase = 0)
    {
        Scene scene;
        buildPenumbraScene (scene, sunRadius);
        setTier (scene, tier);

        auto settings = scene.getRenderSettings();
        settings.softShadows = softShadows;
        scene.setRenderSettings (settings);

        SceneRenderer renderer;
        for (int frame = 0; frame < phase; ++frame)
            EXPECT_NE (renderer.render (device, scene, 256, 256), nullptr) << renderer.getLastError();

        if (phase > 0)
            renderer.restartAccumulation();

        const auto image = renderConverged (renderer, scene, 256);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    }

    /** The luma of every pixel of row @p y, from the left. */
    static std::vector<double> luminanceRow (const Image& image, int y)
    {
        std::vector<double> row;
        for (int x = 0; x < image.getWidth(); ++x)
            row.push_back (meanLuminance (image, { x, y, 1, 1 }));

        return row;
    }

    /** The pixels of row @p y between 10% and 90% of the way from its darkest to its brightest level. */
    static int penumbraWidthAtRow (const Image& image, int y)
    {
        const auto row = luminanceRow (image, y);
        const auto [darkest, brightest] = std::minmax_element (row.begin(), row.end());
        const auto low = *darkest + 0.1 * (*brightest - *darkest);
        const auto high = *darkest + 0.9 * (*brightest - *darkest);

        return static_cast<int> (std::count_if (row.begin(), row.end(), [&] (double level) { return level > low && level < high; }));
    }

    /** penumbraWidthAtRow() along the center row. */
    static int penumbraWidth (const Image& image)
    {
        return penumbraWidthAtRow (image, image.getHeight() / 2);
    }

    /** The rows where the wall's shadow of buildPenumbraScene() starts and ends, found down a column
        inside it: first the end at the wall, which touches the edge of the frame, then the far end. */
    static std::pair<int, int> findWallShadowRows (const Image& image)
    {
        const auto x = image.getWidth() / 4;
        std::vector<double> column;
        for (int y = 0; y < image.getHeight(); ++y)
            column.push_back (meanLuminance (image, { x, y, 1, 1 }));

        const auto [darkest, brightest] = std::minmax_element (column.begin(), column.end());
        const auto middle = 0.5 * (*darkest + *brightest);
        const auto isShadowed = [middle] (double level) { return level < middle; };

        const auto first = static_cast<int> (std::find_if (column.begin(), column.end(), isShadowed) - column.begin());
        const auto last = static_cast<int> (column.rend() - std::find_if (column.rbegin(), column.rend(), isShadowed)) - 1;

        return first == 0 ? std::pair { first, last } : std::pair { last, first };
    }

    /** A row a quarter of the way along the wall's shadow from the wall. */
    static int nearRow (const Image& image)
    {
        const auto [wall, end] = findWallShadowRows (image);
        return wall + (end - wall) / 4;
    }

    /** A row a quarter of the way along the wall's shadow from its far end. */
    static int farRow (const Image& image)
    {
        const auto [wall, end] = findWallShadowRows (image);
        return end - (end - wall) / 4;
    }

    /** With soft shadows on @p tier, the side of the wall's shadow is wider under a larger sun. */
    void checkPcssPenumbraWidensWithLightRadius (RenderSettings::TierPreference tier)
    {
        const auto sharpImage = renderPenumbraScene (tier, 0.002f, true);
        const auto softImage = renderPenumbraScene (tier, 0.05f, true);
        ASSERT_TRUE (sharpImage.isValid() && softImage.isValid());

        const auto sharp = penumbraWidth (sharpImage);
        const auto soft = penumbraWidth (softImage);
        RecordProperty ("sharp", sharp);
        RecordProperty ("soft", soft);
        EXPECT_GT (soft, sharp + 4);
    }

    /** With soft shadows on @p tier, the side of the wall's shadow widens away from the wall, as the
        edge casting it lies further above the ground. */
    void checkPenumbraWidensWithDistanceFromTheBlocker (RenderSettings::TierPreference tier)
    {
        const auto image = renderPenumbraScene (tier, 0.05f, true);
        ASSERT_TRUE (image.isValid());

        const auto near = penumbraWidthAtRow (image, nearRow (image));
        const auto far = penumbraWidthAtRow (image, farRow (image));
        RecordProperty ("near", near);
        RecordProperty ("far", far);
        EXPECT_GT (far, near);
    }

    /** Adds to buildPenumbraScene() a shadow casting spot light of radius @p radius, behind the wall
        at (0, 2.4, -2.4) and aimed at the middle of the frame: the side of the wall's shadow still
        runs along x = 0, across the rows of the frame. */
    static void addPenumbraSpot (Scene& scene, float radius)
    {
        auto entity = EntityNode::Ptr (new EntityNode ("spot"));
        auto& spot = entity->attach<LightNode> (LightNode::Type::spot);
        spot.intensity = 40.0f;
        spot.outerConeAngle = 0.8f;
        spot.innerConeAngle = 0.7f;
        spot.radius = radius;
        spot.castsShadows = true;
        aimCamera (*entity, { 0.0f, 2.4f, -2.4f }, { 0.0f, 0.0f, 0.9f });
        scene.getRoot()->addChild (entity);
    }

    /** buildPenumbraScene() lit by addPenumbraSpot() of radius @p radius instead of the sun, on
        @p tier at 256 x 256, converged, with soft shadows set to @p softShadows. */
    Image renderSpotPenumbraScene (RenderSettings::TierPreference tier, float radius, bool softShadows)
    {
        Scene scene;
        buildPenumbraScene (scene, 0.0f).intensity = 0.0f;
        setTier (scene, tier);
        addPenumbraSpot (scene, radius);

        auto settings = scene.getRenderSettings();
        settings.softShadows = softShadows;
        scene.setRenderSettings (settings);

        SceneRenderer renderer;
        const auto image = renderConverged (renderer, scene, 256);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    }

    /** With soft shadows on @p tier, the side of the wall's shadow under a spot light is wider when
        the light is larger, and only through soft shadows: the same large light filtered hard
        stays as narrow as the small one. */
    void checkSpotPenumbraWidensWithLightRadius (RenderSettings::TierPreference tier)
    {
        const auto sharpImage = renderSpotPenumbraScene (tier, 0.002f, true);
        const auto softImage = renderSpotPenumbraScene (tier, 0.2f, true);
        const auto hardImage = renderSpotPenumbraScene (tier, 0.2f, false);
        ASSERT_TRUE (sharpImage.isValid() && softImage.isValid() && hardImage.isValid());

        const auto sharp = penumbraWidth (sharpImage);
        const auto soft = penumbraWidth (softImage);
        const auto hard = penumbraWidth (hardImage);
        RecordProperty ("sharp", sharp);
        RecordProperty ("soft", soft);
        RecordProperty ("hard", hard);
        EXPECT_GT (soft, sharp + 4);
        EXPECT_LE (hard, sharp + 2);
    }

    /** The width in texels of shadow map @p record of the side of the wall's shadow of
        buildPenumbraScene() at 256 x 256, between 10% and 90% of the way from the shadow to the
        light: the linear radiance (reinhardRadiance()) of the rows around the middle of the frame,
        averaged per column, crossing the levels 60 columns (0.4 m) on either side of the edge,
        interpolated. The record's texels are
        measured across the edge, along x, at the middle of the frame. */
    static double getPenumbraTexels (const Image& image, const GpuShadowRecord& record)
    {
        std::vector<double> profile;
        for (int x = 0; x < image.getWidth(); ++x)
        {
            auto radiance = 0.0;
            for (int y = 96; y < 160; ++y)
                radiance += reinhardRadiance (roundToInt (meanLuminance (image, { x, y, 1, 1 })));

            profile.push_back (radiance / 64.0);
        }

        const auto center = image.getWidth() / 2;
        const auto dark = std::min (profile[static_cast<size_t> (center - 60)], profile[static_cast<size_t> (center + 60)]);
        const auto lit = std::max (profile[static_cast<size_t> (center - 60)], profile[static_cast<size_t> (center + 60)]);
        const auto step = profile[static_cast<size_t> (center - 60)] < profile[static_cast<size_t> (center + 60)] ? 1 : -1;

        // The first column from the shadow's side reaching a level, with its fraction
        const auto crossing = [&] (double level)
        {
            for (auto x = center - 60 * step; x != center + 60 * step; x += step)
            {
                const auto a = profile[static_cast<size_t> (x)];
                const auto b = profile[static_cast<size_t> (x + step)];
                if (a < level && b >= level)
                    return static_cast<double> (x) + static_cast<double> (step) * (level - a) / (b - a);
            }

            return static_cast<double> (center + 60 * step);
        };

        const auto pixels = std::abs (crossing (dark + 0.9 * (lit - dark)) - crossing (dark + 0.1 * (lit - dark)));

        std::array<float, 16> values;
        std::copy_n (record.viewProjection, 16, values.begin());
        const Matrix4 viewProjection (values);
        const auto u = [&] (float x)
        {
            const auto clip = viewProjection.transformPoint4 (x, 0.0f, 0.9f, 1.0f);
            return clip[0] / clip[3] * 0.5f * record.atlasRect[2];
        };

        // The orthographic camera of buildPenumbraScene() shows 1.7 m across the frame
        const auto texelsPerMeter = std::abs (u (0.01f) - u (0.0f)) / record.atlasRect[3] / 0.01f;
        const auto pixelsPerMeter = static_cast<double> (image.getWidth()) / 1.7;
        return pixels / pixelsPerMeter * texelsPerMeter;
    }

    /** With soft shadows on @p tier, a sun and a spot light of almost no size still filter their
        shadows over a disk of at least 2 texels of their map, where the shadow starts at its caster:
        the side of the wall's shadow spans at least 2.5 texels between 10% and 90% (a disk of 2
        texels spans 2.75, one of 1 texel 1.4). The maps are coarse (shadowResolution 256), so a
        texel spans several pixels. */
    void checkSoftShadowsFilterAtLeastTwoTexels (RenderSettings::TierPreference tier)
    {
        for (const auto spot : { false, true })
        {
            Scene scene;
            auto& sun = buildPenumbraScene (scene, 0.002f);
            if (spot)
            {
                sun.intensity = 0.0f;
                addPenumbraSpot (scene, 0.002f);
            }

            auto settings = scene.getRenderSettings();
            settings.tier = tier;
            settings.softShadows = true;
            settings.shadowResolution = 256;
            scene.setRenderSettings (settings);

            SceneRenderer renderer;
            const auto image = renderConverged (renderer, scene, 256);
            ASSERT_TRUE (image.isValid()) << renderer.getLastError();
            ASSERT_EQ (shadows (renderer).getNumRecords(), 1);

            const auto texels = getPenumbraTexels (image, shadows (renderer).getShadowData().records[0]);
            RecordProperty (spot ? "spotTexels" : "sunTexels", String (texels, 2).toStdString());
            EXPECT_GE (texels, 2.5) << (spot ? "spot" : "sun");
        }
    }

    /** Raster keeps the hardware filtered shadow by default, and opts in to soft shadows. */
    void checkRasterSoftShadowsOptIn()
    {
        Scene scene;
        buildPenumbraScene (scene, 0.05f);
        setTier (scene, RenderSettings::TierPreference::raster);

        SceneRenderer hard;
        const auto hardWidth = penumbraWidth (renderToImage (hard, scene, 256));
        EXPECT_LE (hardWidth, 4); // hardware PCF: today's narrow tent

        auto settings = scene.getRenderSettings();
        settings.softShadows = true;
        scene.setRenderSettings (settings);

        SceneRenderer soft;
        const auto softImage = renderToImage (soft, scene, 256);
        ASSERT_TRUE (soft.getLastError().isEmpty()) << soft.getLastError();

        const auto softWidth = penumbraWidth (softImage);
        RecordProperty ("hard", hardWidth);
        RecordProperty ("soft", softWidth);
        EXPECT_GT (softWidth, hardWidth + 4);
    }

    /** Raster shades with its soft shadow variant only in the frames that soften a shadow: turning
        soft shadows on and off again within one renderer comes back to the first frame, the soft
        frame matches a renderer drawing it from the start, and a sun without a size casts the same
        shadow with soft shadows on as off. */
    void checkRasterSoftShadowVariantFollowsTheSettings()
    {
        Scene scene;
        auto& sun = buildPenumbraScene (scene, 0.05f);
        setTier (scene, RenderSettings::TierPreference::raster);

        const auto setSoftShadows = [&scene] (bool soft)
        {
            auto settings = scene.getRenderSettings();
            settings.softShadows = soft;
            scene.setRenderSettings (settings);
        };

        SceneRenderer renderer;
        std::array<Image, 3> frames;
        const std::array<bool, 3> soft { false, true, false };

        for (size_t i = 0; i < frames.size(); ++i)
        {
            setSoftShadows (soft[i]);
            frames[i] = renderToImage (renderer, scene, 128);
            ASSERT_TRUE (frames[i].isValid()) << i << ": " << renderer.getLastError();
        }

        // The canvas read back dithers the frames by a level, within what countDifferences() ignores
        EXPECT_EQ (countDifferences (frames[0], frames[2]), 0);
        EXPECT_GT (countDifferences (frames[1], frames[0]), 0);

        setSoftShadows (true);
        SceneRenderer fresh;
        const auto softFrame = renderToImage (fresh, scene, 128);
        ASSERT_TRUE (softFrame.isValid()) << fresh.getLastError();
        EXPECT_EQ (countDifferences (frames[1], softFrame), 0);

        sun.radius = 0.0f;
        const auto pointSun = renderToImage (renderer, scene, 128);
        setSoftShadows (false);
        const auto pointSunHard = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (pointSun.isValid() && pointSunHard.isValid()) << renderer.getLastError();
        EXPECT_EQ (countDifferences (pointSun, pointSunHard), 0);
    }

    /** A large sun (0.2 rad) 15 degrees above an empty ground casts no more shadow on it with soft
        shadows than with the hard filter on @p tier: no pixel darkens by more than 5% on raster, 2%
        on full, and more than the level the canvas read back dithers by. The ground falls steeply
        away in the sun's depth, so its own depth must not shadow it across the wide soft shadow
        kernels. */
    void checkSoftSunLeavesAGrazedGroundLit (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool softShadows)
        {
            Scene scene;
            addGround (scene, 2.0f);
            scene.setAmbientColor (Color (0xff3f3f3f));

            auto sun = EntityNode::Ptr (new EntityNode ("sun"));
            auto& light = sun->attach<LightNode>();
            light.castsShadows = true;
            light.radius = 0.2f;
            aimCamera (*sun, { -10.0f * std::cos (0.26f), 10.0f * std::sin (0.26f), 0.0f }, {});
            scene.getRoot()->addChild (sun);

            addCamera (scene, { 0.0f, 3.0f, 3.0f }, {});
            setTier (scene, tier);

            auto settings = scene.getRenderSettings();
            settings.softShadows = softShadows;
            scene.setRenderSettings (settings);

            SceneRenderer renderer;
            const auto image = renderConverged (renderer, scene, 256);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            return image;
        };

        const auto shadowed = render (true);
        const auto unshadowed = render (false);
        ASSERT_TRUE (shadowed.isValid() && unshadowed.isValid());

        const auto threshold = tier == RenderSettings::TierPreference::full ? 0.98 : 0.95;
        auto worst = 1.0;
        auto darkened = 0;
        for (int y = 0; y < shadowed.getHeight(); ++y)
        {
            for (int x = 0; x < shadowed.getWidth(); ++x)
            {
                const auto lit = meanLuminance (unshadowed, { x, y, 1, 1 });
                if (lit <= 10.0)
                    continue;

                const auto level = meanLuminance (shadowed, { x, y, 1, 1 });
                if (lit - level <= 2.0)
                    continue;

                const auto relative = level / lit;
                worst = jmin (worst, relative);
                darkened += relative < threshold ? 1 : 0;
            }
        }

        RecordProperty ("darkestRelative", String (worst, 3).toStdString());
        RecordProperty ("darkenedPixels", darkened);
        EXPECT_EQ (darkened, 0) << "darkest " << worst;
    }

#if YUP_ENABLE_SHADER_TRANSPILER
    /** Runs @p fragment over @p source (texture at set 0 binding 0, sampler at binding 1) into an
        rgba8 target of its size, and reads the result back. */
    Image runFullscreen (const String& fragment, const GpuTexture::Ptr& source)
    {
        if (source == nullptr)
            return {};

        auto target = GpuTarget::create (device, static_cast<int> (source->getWidth()), static_cast<int> (source->getHeight()));
        if (target == nullptr || ! drawFullscreen (fragment, source, target->asTexture()))
            return {};

        return readBack (target->asTexture());
    }

    /** Draws @p fragment into the render target @p target, binding @p source (if any) like runFullscreen(). */
    bool drawFullscreen (const String& fragment, const GpuTexture::Ptr& source, const GpuTexture::Ptr& target)
    {
        if (target == nullptr)
            return false;

        const auto vertex = File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory()
                                .getChildFile ("modules/yup_3d/shaders/yup_Fullscreen.vert")
                                .loadFileAsString();

        ShaderBundleCompileRequest request;
        request.sourceLanguage = ShaderLanguage::glsl;

        for (const auto& [stage, code] : { std::pair { ShaderStage::vertex, vertex }, std::pair { ShaderStage::fragment, fragment } })
        {
            ShaderBundleEntry entry;
            entry.stage = stage;
            entry.source = code;
            entry.targetLanguages = { ShaderLanguage::msl, ShaderLanguage::glsl };
            request.entries.push_back (entry);
        }

        ShaderBundleCompiler compiler;
        auto bundle = compiler.compile (request);
        if (! bundle.wasOk())
        {
            ADD_FAILURE() << bundle.getErrorMessage();
            return false;
        }

        MemoryBlock block;
        if (! bundle.getReference().saveToMemoryBlock (block).wasOk())
            return false;

        FullscreenPass pass;
        const auto prepared = pass.prepare (device, static_cast<const uint8*> (block.getData()), block.getSize(), target->getFormat());
        if (prepared.failed())
        {
            ADD_FAILURE() << prepared.getErrorMessage();
            return false;
        }

        auto sampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::nearest, GpuWrapMode::clampToEdge));
        if (sampler == nullptr)
            return false;

        auto frame = GpuFrame::begin (device);
        const auto drawn = pass.draw (frame, target, {}, [&] (GpuRenderPass& renderPass)
        {
            if (source != nullptr)
            {
                renderPass.setTexture (0, 0, source);
                renderPass.setSampler (0, 1, sampler);
            }
        }, GpuColor::transparentBlack());
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        return drawn;
    }

    /** True when no texel of @p texture is NaN or infinite. */
    bool isFiniteTexture (const GpuTexture::Ptr& texture)
    {
        // An all-ones exponent is NaN or infinity. Testing the bits keeps fast math from folding isnan away
        const auto image = runFullscreen (R"(#version 450
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(location = 0) out vec4 color;
void main()
{
    vec4 c = texelFetch(sampler2D(source, sourceSampler), ivec2(gl_FragCoord.xy), 0);
    bvec4 nonFinite = equal(floatBitsToUint(c) & uvec4(0x7f800000u), uvec4(0x7f800000u));
    color = any(nonFinite) ? vec4(1.0) : vec4(0.0, 0.0, 0.0, 1.0);
})",
                                          texture);

        if (! image.isValid())
            return false;

        for (int y = 0; y < image.getHeight(); ++y)
            for (int x = 0; x < image.getWidth(); ++x)
                if (image.getPixelColor (x, y).getRed() > 127)
                    return false;

        return true;
    }

    /** True when any texel of @p texture has a color channel at or beyond the half float range.
        Metal returns 65504 for min (NaN, 65504.0), so a clamped NaN reaches the texture as this. */
    bool isSaturatedTexture (const GpuTexture::Ptr& texture)
    {
        const auto image = runFullscreen (R"(#version 450
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(location = 0) out vec4 color;
void main()
{
    vec4 c = texelFetch(sampler2D(source, sourceSampler), ivec2(gl_FragCoord.xy), 0);
    color = any(greaterThanEqual(abs(c.rgb), vec3(65504.0))) ? vec4(1.0) : vec4(0.0, 0.0, 0.0, 1.0);
})",
                                          texture);

        if (! image.isValid())
            return true;

        for (int y = 0; y < image.getHeight(); ++y)
            for (int x = 0; x < image.getWidth(); ++x)
                if (image.getPixelColor (x, y).getRed() > 127)
                    return true;

        return false;
    }

    /** Writes @p expression, a vec4 computed from the texel t of @p texture, into an rgba8 image of
        its size. @p functions are GLSL declared before main(). */
    Image mapTexels (const GpuTexture::Ptr& texture, const String& expression, const String& functions = {})
    {
        return runFullscreen ("#version 450\n" + functions + R"(
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(location = 0) out vec4 color;
void main()
{
    vec4 t = texelFetch(sampler2D(source, sourceSampler), ivec2(gl_FragCoord.xy), 0);
    color = )" + expression + R"(;
})",
                              texture);
    }

    /** The source of lib/octahedral.glsl, for the test shaders decoding a G-buffer RT1. */
    static String loadOctahedralLibrary()
    {
        return File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory()
            .getChildFile ("modules/yup_3d/shaders/lib/octahedral.glsl")
            .loadFileAsString();
    }

    /** The shading normal of a G-buffer RT1 decoded through lib/octahedral.glsl, as n * 0.5 + 0.5 in rgb. */
    Image readGBufferNormal (const GpuTexture::Ptr& normalRoughness)
    {
        return mapTexels (normalRoughness, "vec4(octahedralDecode(t.xy) * 0.5 + 0.5, 1.0)", loadOctahedralLibrary());
    }

    /** The rg16float velocity as signed 8 bit levels of v * @p scale + 0.5, x in red and y in green:
        zero velocity reads 127 or 128. */
    Image readVelocity (const GpuTexture::Ptr& velocity, float scale)
    {
        return runFullscreen (R"(#version 450
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(location = 0) out vec4 color;
void main()
{
    vec2 v = texelFetch(sampler2D(source, sourceSampler), ivec2(gl_FragCoord.xy), 0).xy;
    color = vec4(v * float()" + String (scale)
                                  + R"() + 0.5, 0.0, 1.0);
})",
                              velocity);
    }

    /** Renders @p scene at @p size x @p size with raw radiance in the scene color (no temporal
        anti-aliasing on full, the rgba16float bloom path on raster), and returns the exposed luma of
        every pixel, row by row from the top, or nothing if the frame failed.
    */
    std::vector<double> renderLuma (Scene& scene, int size, RenderSettings::TierPreference tier)
    {
        if (tier == RenderSettings::TierPreference::full)
            setFullTierTaa (scene, 1);
        else
        {
            setTier (scene, RenderSettings::TierPreference::raster);
            setBloom (scene, true);
        }

        SceneRenderer renderer;
        const auto sceneColor = renderer.render (device, scene, size, size) != nullptr ? renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor) : nullptr;
        if (sceneColor == nullptr)
        {
            ADD_FAILURE() << renderer.getLastError();
            return {};
        }

        // Raster scene color is reversibly tone mapped and premultiplied by the MSAA coverage. The
        // luma goes out as 16 bits of its log2 over [-20, 20]: the high byte in red, the low in green
        const auto color = File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory()
                               .getChildFile ("modules/yup_3d/shaders/lib/color.glsl")
                               .loadFileAsString();

        const auto radiance = tier == RenderSettings::TierPreference::full ? "t.rgb" : "yupUnpremultiplyInverse(t)";
        const auto image = mapTexels (sceneColor, String ("encodeLuma(") + radiance + ")", color + R"(
vec4 encodeLuma(vec3 c)
{
    float e = clamp((log2(max(yupLuma(c), 1.0e-6)) + 20.0) / 40.0, 0.0, 1.0) * 255.0;
    return vec4(floor(e) / 255.0, fract(e), 0.0, 1.0);
})");

        if (! image.isValid())
        {
            ADD_FAILURE() << "Unable to read the scene color back";
            return {};
        }

        std::vector<double> luma;
        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                const auto texel = image.getPixelColor (x, y);
                const auto e = (texel.getRed() + texel.getGreen() / 255.0) / 255.0;
                luma.push_back (e > 0.0 ? std::exp2 (e * 40.0 - 20.0) : 0.0);
            }
        }

        return luma;
    }

    /** The mean of @p luma, an image @p size pixels wide from renderLuma(), within @p area. */
    static double meanLuma (const std::vector<double>& luma, int size, Rectangle<int> area)
    {
        if (luma.empty())
            return -1.0;

        auto sum = 0.0;
        for (int y = area.getY(); y < area.getBottom(); ++y)
            for (int x = area.getX(); x < area.getRight(); ++x)
                sum += luma[static_cast<size_t> (y * size + x)];

        return sum / static_cast<double> (area.getWidth() * area.getHeight());
    }

    /** The 256 light grid keeps every quad lit and the scene color finite on @p tier, through the rgba16float path on raster. */
    void checkSceneColorHasNoNaN (RenderSettings::TierPreference tier)
    {
        // The 256 light grid, one draw per quad, plus a zero-area triangle (which draws no fragments
        // and only exercises the bounds, the light selection and the clusters) and a light at the
        // camera position, the real shading stress
        Scene scene;
        buildLightGridScene (scene, false);

        std::vector<Mesh::Vertex> vertices (3);
        for (auto& vertex : vertices)
        {
            vertex.position = { 0.1f, 0.1f, 0.2f };
            vertex.normal = { 0.0f, 0.0f, 1.0f };
        }

        auto mesh = Mesh::Ptr (new Mesh ("degenerate"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2 });
        auto degenerate = EntityNode::Ptr (new EntityNode ("degenerate"));
        degenerate->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (degenerate);

        auto atCamera = EntityNode::Ptr (new EntityNode ("atCamera"));
        atCamera->attach<LightNode> (LightNode::Type::point).intensity = 10.0f;
        atCamera->setPosition ({ 0.0f, 0.0f, 9.0f });
        scene.getRoot()->addChild (atCamera);

        // Scene color holds raw radiance on full without temporal anti-aliasing. Raster rgba8 hides
        // non-finite values, but a NaN pixel reads black there: every quad center must stay lit
        const auto full = tier == RenderSettings::TierPreference::full;
        if (full)
            setFullTierTaa (scene, 1);
        else
            setTier (scene, RenderSettings::TierPreference::raster);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 256);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        const auto lit = countLitGridQuads (image);
        RecordProperty ("litQuads", lit);
        EXPECT_EQ (lit, 256);

        // Raw radiance: the full tier's scene color, and the raster rgba16float bloom path
        if (! full)
        {
            setBloom (scene, true);
            ASSERT_NE (renderer.render (device, scene, 256, 256), nullptr) << renderer.getLastError();
        }

        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        ASSERT_NE (sceneColor, nullptr);
        EXPECT_EQ (sceneColor->getFormat(), GpuTextureFormat::rgba16float);
        EXPECT_TRUE (isFiniteTexture (sceneColor));
        EXPECT_FALSE (isSaturatedTexture (sceneColor));
    }

    /** A point light of tiny radius shades like the punctual one on @p tier. */
    void checkTinyRadiusMatchesPunctual (RenderSettings::TierPreference tier)
    {
        // A point light of radius 0.001 one unit over a rough plate shades through the area path: its
        // sphere radiance I / (pi r^2) over the solid angle it subtends converges to the inverse square law
        const auto meanAt = [this, tier] (float radius)
        {
            auto material = Material::Ptr (new Material());
            material->baseColorFactor = { 0.8f, 0.8f, 0.8f, 1.0f };
            material->metallicFactor = 0.0f;
            material->roughnessFactor = 0.5f;

            Scene scene;
            addPlate (scene, material);
            scene.setAmbientColor (Colors::black);

            auto light = EntityNode::Ptr (new EntityNode ("point"));
            light->attach<LightNode> (LightNode::Type::point).radius = radius;
            light->setPosition ({ 0.0f, 0.0f, 1.0f });
            scene.getRoot()->addChild (light);

            return meanLuma (renderLuma (scene, 128, tier), 128, { 40, 40, 48, 48 });
        };

        const auto punctual = meanAt (0.0f);
        const auto area = meanAt (0.001f);
        RecordProperty ("punctual", String (punctual, 4).toStdString());
        RecordProperty ("area", String (area, 4).toStdString());

        ASSERT_GT (punctual, 0.05);
        EXPECT_NEAR (area / punctual, 1.0, 0.02);
    }

    /** On @p tier, the clearcoat and the sheen reflect a point light of radius 0.001 through their
        area light path (lightsEvaluateLayersArea: the clearcoat's LTC, the sheen's form factor) as
        they reflect the punctual one, within 2%, the tolerance of checkTinyRadiusMatchesPunctual. The
        base reflects nothing (black, no specular), so the layers alone light the plate.
    */
    void checkLayersUnderATinyAreaLightMatchPunctual (RenderSettings::TierPreference tier)
    {
        const auto meanAt = [this, tier] (Material::Ptr material, float radius)
        {
            Scene scene;
            addPlate (scene, material);
            scene.setAmbientColor (Colors::black);

            auto light = EntityNode::Ptr (new EntityNode ("point"));
            light->attach<LightNode> (LightNode::Type::point).radius = radius;
            light->setPosition ({ 0.6f, 0.0f, 0.8f });
            scene.getRoot()->addChild (light);

            return meanLuma (renderLuma (scene, 128, tier), 128, { 40, 40, 48, 48 });
        };

        const std::pair<const char*, Material::Ptr> layers[] = {
            { "clearcoat", makeLayeredMaterial ({ 0.0f, 0.0f, 0.0f }, 1.0f, 0.4f, {}, 0.0f, 0.0f) },
            { "sheen", makeLayeredMaterial ({ 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, { 1.0f, 1.0f, 1.0f }, 0.5f, 0.0f) },
        };

        for (const auto& [name, material] : layers)
        {
            const auto punctual = meanAt (material, 0.0f);
            const auto area = meanAt (material, 0.001f);
            RecordProperty ((String (name) + "Punctual").toStdString(), String (punctual, 4).toStdString());
            RecordProperty ((String (name) + "Area").toStdString(), String (area, 4).toStdString());

            ASSERT_GT (punctual, 0.002) << name;
            EXPECT_NEAR (area / punctual, 1.0, 0.02) << name;
        }
    }

    /** A light radius widens the highlight without changing its energy on @p tier. */
    void checkRadiusWidensTheHighlight (RenderSettings::TierPreference tier)
    {
        // A smooth dark plate seen at 45 degrees mirrors a point light towards the camera. A radius of
        // 0.3 spreads the highlight over more bright pixels (luma above 1.4, a level of 200 once Reinhard
        // tone mapped) while the energy it reflects stays within 10%
        const auto highlightAt = [this, tier] (float radius)
        {
            auto material = Material::Ptr (new Material());
            material->baseColorFactor = { 0.05f, 0.05f, 0.05f, 1.0f };
            material->metallicFactor = 0.0f;
            material->roughnessFactor = 0.1f;

            Scene scene;
            auto* plate = addPlate (scene, material);
            scene.setAmbientColor (Colors::black);

            const auto tilt = Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, -MathConstants<float>::pi * 0.25f);
            plate->setRotation (tilt);

            // The camera direction from the plate center, mirrored about its normal
            const auto normal = tilt.rotate ({ 0.0f, 0.0f, 1.0f });
            const Vector3<float> toCamera { 0.0f, 0.0f, 1.0f };
            const auto mirrored = normal * (2.0f * normal.dotProduct (toCamera)) - toCamera;

            auto light = EntityNode::Ptr (new EntityNode ("point"));
            auto& node = light->attach<LightNode> (LightNode::Type::point);
            node.intensity = 20.0f;
            node.radius = radius;
            light->setPosition (mirrored * 1.5f);
            scene.getRoot()->addChild (light);

            const auto luma = renderLuma (scene, 256, tier);
            const auto bright = std::count_if (luma.begin(), luma.end(), [] (double value) { return value > 1.4; });
            return std::pair { static_cast<int> (bright), std::accumulate (luma.begin(), luma.end(), 0.0) };
        };

        const auto [sharpPixels, sharpEnergy] = highlightAt (0.0f);
        const auto [widePixels, wideEnergy] = highlightAt (0.3f);
        RecordProperty ("sharpPixels", sharpPixels);
        RecordProperty ("widePixels", widePixels);
        RecordProperty ("sharpEnergy", String (sharpEnergy, 1).toStdString());
        RecordProperty ("wideEnergy", String (wideEnergy, 1).toStdString());

        EXPECT_GT (sharpPixels, 0);
        EXPECT_GT (widePixels, sharpPixels);
        ASSERT_GT (sharpEnergy, 0.0);
        EXPECT_NEAR (wideEnergy / sharpEnergy, 1.0, 0.1);
    }

    /** Area lights stay finite on @p tier, through the rgba16float path on raster, over a material
        with a smooth clearcoat and a smooth white sheen when @p layers is set. */
    void checkAreaLightsStayFinite (RenderSettings::TierPreference tier, bool layers = false)
    {
        // A smooth plate inside a sphere light, touching another, under a spot disk and a grazing sun
        // disk, over a gray ambient light so every plate pixel stays lit
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.5f, 0.5f, 0.5f, 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 0.0f;

        if (layers)
        {
            material->clearcoatFactor = 1.0f;
            material->sheenColorFactor = { 1.0f, 1.0f, 1.0f };
        }

        Scene scene;
        addPlate (scene, material);
        scene.setAmbientColor (Color (0xff404040));

        const auto addLight = [&] (LightNode::Type type, float radius, Vector3<float> position, const Quaternion& rotation)
        {
            auto light = EntityNode::Ptr (new EntityNode ("light"));
            auto& node = light->attach<LightNode> (type);
            node.intensity = 3.0f;
            node.radius = radius;
            light->setPosition (position);
            light->setRotation (rotation);
            scene.getRoot()->addChild (light);
        };

        addLight (LightNode::Type::point, 0.3f, { 0.0f, 0.0f, 0.05f }, Quaternion::identity());
        addLight (LightNode::Type::point, 0.1f, { 0.5f, -0.5f, 0.0f }, Quaternion::identity());
        addLight (LightNode::Type::spot, 0.2f, { -0.5f, 0.5f, 0.3f }, Quaternion::identity());
        addLight (LightNode::Type::directional, 0.3f, {}, Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, 1.55f));

        // Scene color holds raw radiance on full without temporal anti-aliasing. Raster rgba8 hides
        // non-finite values, but a NaN pixel reads black there
        const auto full = tier == RenderSettings::TierPreference::full;
        if (full)
            setFullTierTaa (scene, 1);
        else
            setTier (scene, RenderSettings::TierPreference::raster);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        auto darkest = 255.0;
        for (int y = 40; y < 88; ++y)
            for (int x = 40; x < 88; ++x)
                darkest = jmin (darkest, meanLuminance (image, { x, y, 1, 1 }));

        RecordProperty ("darkest", String (darkest, 1).toStdString());
        EXPECT_GT (darkest, 10.0);

        // Raw radiance: the full tier's scene color, and the raster rgba16float bloom path. On Metal the
        // raster path stores a NaN as about 1 (min (NaN, 65504) then the reversible tone map), so there
        // the black rgba8 pixel above is what catches it
        if (! full)
        {
            setBloom (scene, true);
            ASSERT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
        }

        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        ASSERT_NE (sceneColor, nullptr);
        EXPECT_EQ (sceneColor->getFormat(), GpuTextureFormat::rgba16float);
        EXPECT_TRUE (isFiniteTexture (sceneColor));
        EXPECT_FALSE (isSaturatedTexture (sceneColor));
    }

    /** The ground in the shadow of the near cube of buildLongGroundScene() stays finite on @p tier,
        in raw radiance: no temporal anti-aliasing on full, the rgba16float bloom path on raster. */
    void checkShadowedRegionIsFinite (RenderSettings::TierPreference tier)
    {
        const auto full = tier == RenderSettings::TierPreference::full;

        const auto render = [&] (bool castsShadows, GpuTexture::Ptr& sceneColor)
        {
            Scene scene;
            buildLongGroundScene (scene, 2.0f).castsShadows = castsShadows;

            if (full)
                setFullTierTaa (scene, 1);
            else
            {
                setTier (scene, RenderSettings::TierPreference::raster);
                setBloom (scene, true);
            }

            SceneRenderer renderer;
            const auto image = renderToImage (renderer, scene, 256);
            sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
            return image;
        };

        GpuTexture::Ptr sceneColor, unshadowedColor;
        const auto shadowed = render (true, sceneColor);
        const auto unshadowed = render (false, unshadowedColor);
        ASSERT_TRUE (shadowed.isValid() && unshadowed.isValid());
        ASSERT_NE (sceneColor, nullptr);

        // The checked frame does hold the shadow
        EXPECT_GT (shadowContrast (unshadowed, shadowed), 0.35);

        EXPECT_EQ (sceneColor->getFormat(), GpuTextureFormat::rgba16float);
        EXPECT_TRUE (isFiniteTexture (sceneColor));
        EXPECT_FALSE (isSaturatedTexture (sceneColor));
    }

    /** With ambient occlusion on, the raster depth and normal prepass of the plates scene holds the
        G-buffer RT1 encoding: the plates' +Z decodes through lib/octahedral.glsl within 0.02, and only
        drawn texels have a roughness. The full tier reads its G-buffer instead and draws no prepass.
    */
    void checkPrepassMatchesTheGBufferEncoding (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildShadowsScene (scene);
        setTier (scene, tier);

        auto settings = scene.getRenderSettings();
        settings.ambientOcclusion = true;
        scene.setRenderSettings (settings);

        SceneRenderer renderer;
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        ASSERT_TRUE (renderer.waitForGPU().wasOk());

        const auto prepass = renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal);
        if (tier == RenderSettings::TierPreference::full)
        {
            EXPECT_EQ (prepass, nullptr);
            return;
        }

        ASSERT_NE (prepass, nullptr);
        EXPECT_EQ (prepass->getFormat(), GpuTextureFormat::rgba16float);
        EXPECT_EQ (prepass->getSampleCount(), 1u);

        // Red: the distance of the decoded normal from +Z, times 10, so 0.02 reads 51 levels. Green: the roughness
        const auto decoded = mapTexels (prepass, "vec4(length(octahedralDecode(t.xy) - vec3(0.0, 0.0, 1.0)) * 10.0, t.z, 0.0, 1.0)", loadOctahedralLibrary());
        ASSERT_TRUE (decoded.isValid());

        // A cleared texel decodes to +Z as well, so the roughness proves the center was drawn
        EXPECT_LT (decoded.getPixelColor (32, 32).getRed(), 51);
        EXPECT_GT (decoded.getPixelColor (32, 32).getGreen(), 64);
        EXPECT_EQ (decoded.getPixelColor (1, 1).getGreen(), 0);
    }
#endif

    /** A 5 cm high step, 1 x 1 m, on a 50 x 50 m ground, its +x face standing on x = 0, lit by a
        shadow casting sun 15 degrees above the horizon from -x: the step shadows the ground over the
        0.19 m beyond that face, the crease. The sun gets a single 256 x 256 map, fitted to the
        ground the camera sees (texels of about 0.12 m). Its lookups move each receiver along its
        normal by 1.5 texels times (1 - n.l), about 0.13 m under this grazing sun, which lifts the
        whole crease above the step: the map cannot show that shadow, only contact shadows can.
        The camera looks at the crease from +x, above it. The sun asks for contact shadows when
        @p contact is set. Returns the sun.
    */
    static LightNode& buildCreaseScene (Scene& scene, bool contact)
    {
        addGround (scene, 25.0f);

        auto step = EntityNode::Ptr (new EntityNode ("step"));
        step->attach<MeshNode> (makeBox (0.5f, makeWallMaterial()));
        step->setScale ({ 1.0f, 0.05f, 1.0f });
        step->setPosition ({ -0.5f, 0.025f, 0.0f });
        scene.getRoot()->addChild (step);

        auto entity = EntityNode::Ptr (new EntityNode ("sun"));
        auto& sun = entity->attach<LightNode>();
        sun.castsShadows = true;
        sun.contactShadows = contact;
        const auto elevation = degreesToRadians (15.0f);
        aimCamera (*entity, Vector3<float> (-std::cos (elevation), std::sin (elevation), 0.0f) * 10.0f, {});
        scene.getRoot()->addChild (entity);

        addCamera (scene, { 0.55f, 0.35f, 0.0f }, { 0.05f, 0.0f, 0.0f });
        scene.setAmbientColor (Color (0xff3f3f3f)); // 0.05 once linear

        auto settings = scene.getRenderSettings();
        settings.sunCascades = 1;
        settings.shadowResolution = 256;
        scene.setRenderSettings (settings);
        return sun;
    }

    /** The ground of the crease of buildCreaseScene() in a render of @p size x @p size. */
    static Rectangle<int> creaseRect (int size = 256)
    {
        return { 64 * size / 256, 116 * size / 256, 128 * size / 256, 16 * size / 256 };
    }

    /** Open ground of buildCreaseScene(), lit and far from the step, in a render of @p size x @p size. */
    static Rectangle<int> openGroundRect (int size = 256)
    {
        return { 64 * size / 256, 200 * size / 256, 128 * size / 256, 32 * size / 256 };
    }

    /** buildCreaseScene() on @p tier at @p size x @p size, converged, with or without contact
        shadows, the sun casting shadows or not. @p texel receives the world size of a texel of the
        sun map, 0 without one. */
    Image renderCreaseScene (RenderSettings::TierPreference tier, int size, bool contact, bool castsShadows, float& texel)
    {
        Scene scene;
        buildCreaseScene (scene, contact).castsShadows = castsShadows;
        setTier (scene, tier);

        SceneRenderer renderer;
        const auto image = renderConverged (renderer, scene, size);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();

        texel = shadows (renderer).getShadowData().cascadeTexel[0];
        return image;
    }

    /** On @p tier, which has no contact shadows, buildCreaseScene() renders exactly the same with
        and without them. */
    void checkContactShadowsIgnoredOnRaster (RenderSettings::TierPreference tier)
    {
        Scene withContact, without;
        buildCreaseScene (withContact, true);
        buildCreaseScene (without, false);
        setTier (withContact, tier);
        setTier (without, tier);

        // Known, not caused by contact shadows: the first frame a fresh Metal device renders differs
        // from the later ones by one level in a few hundred pixels, with or without shadows (a Phase 3
        // follow-up, see the Task 8 report). Both compared frames come after a warm-up frame
        SceneRenderer warmUp;
        ASSERT_TRUE (renderToImage (warmUp, without, 128).isValid());

        SceneRenderer a, b;
        const auto imageA = renderToImage (a, withContact, 128);
        const auto imageB = renderToImage (b, without, 128);
        ASSERT_TRUE (a.getLastError().isEmpty());
        ASSERT_TRUE (imageA.isValid() && imageB.isValid());
        EXPECT_NEAR (meanLuminance (imageA, creaseRect (128)), meanLuminance (imageB, creaseRect (128)), 1.0f / 255.0f);

        auto differing = 0;
        for (int y = 0; y < imageA.getHeight(); ++y)
            for (int x = 0; x < imageA.getWidth(); ++x)
                differing += imageA.getPixelColor (x, y) != imageB.getPixelColor (x, y) ? 1 : 0;

        EXPECT_EQ (differing, 0);
    }

    /** The contact shadow settings of each light reach the shaders on @p tier through its first
        shadow record: contact[0] the march distance (0 when off), contact[1] the thickness, with
        filterInfo.w flag 1 telling the tier marches them. A light with contact shadows but no
        shadow map takes one record without a map. The raster tier has no contact shadows: no flag,
        no distance and no extra record.
    */
    void checkContactShadowSettingsLiveInTheFirstShadowRecord (RenderSettings::TierPreference tier)
    {
        const auto full = tier == RenderSettings::TierPreference::full;

        for (const auto enabled : { true, false })
        {
            Scene scene;
            buildCreaseScene (scene, true);
            setTier (scene, tier);

            auto settings = scene.getRenderSettings();
            settings.contactShadows = enabled;
            scene.setRenderSettings (settings);

            const auto addPoint = [&] (Vector3<float> position, bool castsShadows, float distance, float thickness)
            {
                auto entity = EntityNode::Ptr (new EntityNode ("point"));
                auto& light = entity->attach<LightNode> (LightNode::Type::point);
                light.castsShadows = castsShadows;
                light.contactShadows = true;
                light.contactShadowDistance = distance;
                light.contactShadowThickness = thickness;
                entity->setPosition (position);
                scene.getRoot()->addChild (entity);
            };

            addPoint ({ 0.5f, 1.0f, 0.5f }, true, 0.4f, 0.02f);
            addPoint ({ -0.5f, 1.0f, -0.5f }, false, 0.3f, 0.1f);

            SceneRenderer renderer;
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk());

            const auto marches = full && enabled;
            const auto data = shadows (renderer).getShadowData();
            const auto cascades = static_cast<int> (data.sunInfo[0]);
            ASSERT_GE (cascades, 1);

            const auto flags = static_cast<int> (data.filterInfo[3]);
            EXPECT_EQ ((flags & 1) != 0, marches) << flags;

            // The sun, then the six faces of the shadowed point light, which keep their view count
            EXPECT_FLOAT_EQ (data.records[0].contact[0], marches ? 0.2f : 0.0f);
            EXPECT_FLOAT_EQ (data.records[cascades].contact[0], marches ? 0.4f : 0.0f);
            if (marches)
                EXPECT_FLOAT_EQ (data.records[cascades].contact[1], 0.02f);

            for (int face = 0; face < 6; ++face)
                EXPECT_FLOAT_EQ (data.records[cascades + face].contact[2], 6.0f) << face;

            // Contact shadows without a shadow map: one record without a map
            ASSERT_EQ (shadows (renderer).getNumRecords(), cascades + 6 + (marches ? 1 : 0));
            if (marches)
            {
                const auto& contactOnly = data.records[cascades + 6];
                EXPECT_EQ (contactOnly.atlasRect[2], 0.0f);
                EXPECT_FLOAT_EQ (contactOnly.contact[0], 0.3f);
                EXPECT_FLOAT_EQ (contactOnly.contact[1], 0.1f);
            }
        }
    }

    /** One shadow filter variant holds soft shadows and, on full, contact shadows: on @p tier a
        frame using neither shades with the default variants even with soft shadows on, a frame
        using either or both with the filter variant. Raster has no contact shadows: contact alone
        keeps its default.
    */
    void checkShadowFilterVariantFollowsSoftAndContactShadows (RenderSettings::TierPreference tier)
    {
        const auto full = tier == RenderSettings::TierPreference::full;

        const struct
        {
            const char* name;
            float sunRadius;
            bool contact;
            bool filters;
        } cases[] = {
            { "neither", 0.0f, false, false },
            { "soft only", 0.05f, false, true },
            { "contact only", 0.0f, true, full },
            { "both", 0.05f, true, true },
        };

        for (const auto& c : cases)
        {
            Scene scene;
            buildCreaseScene (scene, c.contact).radius = c.sunRadius;
            setTier (scene, tier);

            auto settings = scene.getRenderSettings();
            settings.softShadows = true;
            scene.setRenderSettings (settings);

            SceneRenderer renderer;
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << c.name << ": " << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk()) << c.name;
            EXPECT_EQ (shadows (renderer).needsShadowFilters(), c.filters) << c.name;
        }
    }

    /** The depth and normal prepass exists only on raster with ambient occlusion on: none at the
        default settings, one once ambient occlusion is turned on, released when it is turned off
        again. The full tier never draws one, and a raster device that cannot draw it renders without.
    */
    void checkPrepassIsNotCreatedByDefault (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildShadowsScene (scene);
        setTier (scene, tier);

        SceneRenderer renderer;
        const auto drawsPrepass = [&] (std::optional<bool> ambientOcclusion)
        {
            auto settings = scene.getRenderSettings();
            settings.ambientOcclusion = ambientOcclusion;
            scene.setRenderSettings (settings);

            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk());
            return renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal) != nullptr;
        };

        EXPECT_FALSE (drawsPrepass (std::nullopt));
        EXPECT_EQ (drawsPrepass (true), tier == RenderSettings::TierPreference::raster);
        EXPECT_FALSE (drawsPrepass (false));

        renderer.setDeviceFactsOverride ([] (RenderCapabilities::DeviceFacts& facts)
        {
            facts.depthNormalPrepass = false;
        });
        EXPECT_FALSE (drawsPrepass (true));
    }

    /** A prepass that fails to compile leaves the frames without it, with no error. On raster,
        where ambient occlusion reads the prepass, they render like frames without ambient
        occlusion; the full tier never draws the prepass, so its frames are left as they are.
    */
    void checkRendersWithoutTheDepthNormalPrepass (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool ambientOcclusion, bool failPrepass)
        {
            Scene scene;
            buildShadowsScene (scene);
            setTier (scene, tier);

            auto settings = scene.getRenderSettings();
            settings.ambientOcclusion = ambientOcclusion;
            scene.setRenderSettings (settings);

            SceneRenderer renderer;
            if (failPrepass)
                renderer.failDepthNormalPrepassForTesting();

            Image image;
            for (int frame = 0; frame < 2; ++frame)
            {
                image = renderToImage (renderer, scene, 64);
                EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
                EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr) << frame;
            }

            return image;
        };

        const auto withoutPrepass = render (true, true);
        const auto reference = render (tier == RenderSettings::TierPreference::full, false);
        ASSERT_TRUE (withoutPrepass.isValid() && reference.isValid());
        EXPECT_EQ (countDifferences (withoutPrepass, reference), 0);
    }

    /** A 0.6 m box, the entity "box", resting on a 6 x 6 m ground, seen from 0.9 m in front of the
        foot of its +z face and 30 degrees above it: the crease where that face meets the ground
        runs across the middle of the frame, and the ground at the bottom of the frame lies 0.4 m
        and more in front of the face, beyond the default aoDistance of 0.2 m. Without a light, it
        is lit by the flat ambient light alone (0.5 once linear), the indirect light ambient
        occlusion darkens.
    */
    static void buildContactCreaseScene (Scene& scene)
    {
        addGround (scene, 3.0f);

        auto box = EntityNode::Ptr (new EntityNode ("box"));
        box->attach<MeshNode> (makeBox (0.3f, makeWallMaterial()));
        box->setPosition ({ 0.0f, 0.3f, 0.0f });
        scene.getRoot()->addChild (box);

        const Vector3<float> foot (0.0f, 0.0f, 0.3f);
        const auto elevation = degreesToRadians (30.0f);
        addCamera (scene, foot + Vector3<float> (0.0f, std::sin (elevation), std::cos (elevation)) * 0.9f, foot);

        scene.setUsingDefaultLight (false);
        scene.setAmbientColor (Color (0xffbcbcbc));
    }

    /** The ground just in front of the box face of buildContactCreaseScene(), in a texture or image
        of @p width x @p height. */
    static Rectangle<int> contactCreaseRect (int width, int height)
    {
        return { width * 35 / 100, height * 50 / 100, width * 30 / 100, jmax (1, height * 4 / 100) };
    }

    /** Open ground of buildContactCreaseScene(), at least 0.4 m in front of the box, in a texture or
        image of @p width x @p height. */
    static Rectangle<int> openFloorRect (int width, int height)
    {
        return { width * 35 / 100, height * 90 / 100, width * 30 / 100, height * 7 / 100 };
    }

    /** Sets RenderSettings::ambientOcclusion of a scene. */
    static void setAmbientOcclusion (Scene& scene, std::optional<bool> ambientOcclusion)
    {
        auto settings = scene.getRenderSettings();
        settings.ambientOcclusion = ambientOcclusion;
        scene.setRenderSettings (settings);
    }

    /** The pixels of @p image more than one level away from @p reference in any channel. */
    static int countDifferencesAboveOneLevel (const Image& image, const Image& reference)
    {
        int differing = 0;
        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto a = image.getPixelColor (x, y);
                const auto b = reference.getPixelColor (x, y);
                if (std::abs (a.getRed() - b.getRed()) > 1 || std::abs (a.getGreen() - b.getGreen()) > 1 || std::abs (a.getBlue() - b.getBlue()) > 1)
                    ++differing;
            }
        }

        return differing;
    }

#if YUP_ENABLE_SHADER_TRANSPILER
    /** On @p tier, ambient occlusion darkens the crease where a box meets the ground and leaves the
        open ground nearly unoccluded. The pass output is rgba8, full size on full and half size on
        raster, and holds the view-space bent normal, which on open ground is the ground's normal.
        The shaded crease is darker than without ambient occlusion, the open ground is not.
    */
    void checkAmbientOcclusionDarkensTheContactCrease (RenderSettings::TierPreference tier)
    {
        const auto full = tier == RenderSettings::TierPreference::full;

        Scene scene;
        buildContactCreaseScene (scene);
        setTier (scene, tier);
        setAmbientOcclusion (scene, true);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        const auto ao = renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion);
        ASSERT_NE (ao, nullptr);
        EXPECT_EQ (ao->getFormat(), GpuTextureFormat::rgba8unorm);
        EXPECT_EQ (ao->getWidth(), 64u);
        EXPECT_EQ (ao->getHeight(), 64u);

        const auto size = static_cast<int> (ao->getWidth());
        const auto visibility = mapTexels (ao, "vec4(t.aaa, 1.0)");
        ASSERT_TRUE (visibility.isValid());

        const auto crease = meanLuminance (visibility, contactCreaseRect (size, size));
        const auto open = meanLuminance (visibility, openFloorRect (size, size));
        RecordProperty ("creaseVisibility", String (crease, 1).toStdString());
        RecordProperty ("openVisibility", String (open, 1).toStdString());
        EXPECT_LT (crease, open - 30.0);
        EXPECT_GT (open, 220.0); // An open floor is barely occluded

        // Seen 30 degrees from above, the ground's normal in view space is (0, cos 30, sin 30)
        const auto bentNormal = mapTexels (ao, "vec4(t.rgb, 1.0)");
        ASSERT_TRUE (bentNormal.isValid());

        const auto openArea = openFloorRect (size, size);
        const auto bent = bentNormal.getPixelColor (openArea.getCenterX(), openArea.getCenterY());
        EXPECT_NEAR (bent.getRed(), 128, 16);
        EXPECT_NEAR (bent.getGreen(), 238, 16);
        EXPECT_NEAR (bent.getBlue(), 191, 16);

        Scene without;
        buildContactCreaseScene (without);
        setTier (without, tier);
        setAmbientOcclusion (without, false);

        SceneRenderer withoutRenderer;
        const auto reference = renderToImage (withoutRenderer, without, 128);
        ASSERT_TRUE (reference.isValid()) << withoutRenderer.getLastError();

        const auto shadedCrease = meanLuminance (image, contactCreaseRect (128, 128));
        const auto referenceCrease = meanLuminance (reference, contactCreaseRect (128, 128));
        RecordProperty ("shadedCrease", String (shadedCrease, 1).toStdString());
        RecordProperty ("shadedCreaseWithout", String (referenceCrease, 1).toStdString());
        EXPECT_LT (shadedCrease, referenceCrease - 10.0);
        EXPECT_NEAR (meanLuminance (image, openFloorRect (128, 128)), meanLuminance (reference, openFloorRect (128, 128)), 4.0);
    }
#endif

    /** Ambient occlusion off allocates nothing on @p tier: no prepass on raster and no ambient
        occlusion output on either tier. Raster defaults to off, full to on. A pass created by an
        earlier frame keeps no render target on the frames without it, and an aoDistance of 0 turns
        ambient occlusion off.
    */
    void checkAmbientOcclusionOffAllocatesNothing (RenderSettings::TierPreference tier)
    {
        const auto full = tier == RenderSettings::TierPreference::full;

        Scene scene;
        buildContactCreaseScene (scene);
        setTier (scene, tier);

        SceneRenderer renderer;
        const auto render = [&] (std::optional<bool> ambientOcclusion)
        {
            setAmbientOcclusion (scene, ambientOcclusion);
            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk());
            return renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion) != nullptr;
        };

        EXPECT_EQ (render (std::nullopt), full);
        EXPECT_FALSE (render (false));
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr);
        EXPECT_TRUE (render (true));

        const auto* pass = renderer.getAmbientOcclusionPass();
        ASSERT_NE (pass, nullptr);
        EXPECT_GT (pass->getNumCachedTargets(), 0u);

        EXPECT_FALSE (render (false));
        EXPECT_EQ (renderer.getAmbientOcclusionPass(), pass);
        EXPECT_EQ (pass->getNumCachedTargets(), 0u);
        EXPECT_EQ (pass->getOutput(), nullptr);

        auto settings = scene.getRenderSettings();
        settings.aoDistance = 0.0f;
        scene.setRenderSettings (settings);
        EXPECT_FALSE (render (true));
        EXPECT_EQ (pass->getNumCachedTargets(), 0u);
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr);
    }

    /** On a raster device that cannot draw the depth and normal prepass, ambient occlusion is
        unavailable: turning it on renders the same image as leaving it off, without an error and
        without an output. The full tier reads the G-buffer instead, so the fact leaves it on.
    */
    void checkAmbientOcclusionUnavailableOnRasterRendersFallback (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool ambientOcclusion, bool& hasOutput)
        {
            Scene scene;
            buildContactCreaseScene (scene);
            setTier (scene, tier);
            setAmbientOcclusion (scene, ambientOcclusion);

            SceneRenderer renderer;
            renderer.setDeviceFactsOverride ([] (RenderCapabilities::DeviceFacts& facts)
            {
                facts.depthNormalPrepass = false;
            });

            const auto image = renderToImage (renderer, scene, 64);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            hasOutput = renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion) != nullptr;
            return image;
        };

        auto hasOutput = false, hasOutputWithout = false;
        const auto withAmbientOcclusion = render (true, hasOutput);
        const auto without = render (false, hasOutputWithout);
        ASSERT_TRUE (withAmbientOcclusion.isValid() && without.isValid());
        EXPECT_FALSE (hasOutputWithout);

        if (tier == RenderSettings::TierPreference::full)
        {
            EXPECT_TRUE (hasOutput);
            return;
        }

        EXPECT_FALSE (hasOutput);
        EXPECT_EQ (countDifferencesAboveOneLevel (withAmbientOcclusion, without), 0);
    }

    /** On @p tier, ambient occlusion never darkens direct light: lit by a sun alone, without
        ambient light, the crease scene renders the same with and without it.
    */
    void checkAmbientOcclusionLeavesTheDirectLightAlone (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool ambientOcclusion)
        {
            Scene scene;
            buildContactCreaseScene (scene);
            scene.setAmbientColor (Colors::black);

            auto sun = EntityNode::Ptr (new EntityNode ("sun"));
            sun->attach<LightNode>();
            aimCamera (*sun, { -3.0f, 8.0f, 4.0f }, {});
            scene.getRoot()->addChild (sun);

            setTier (scene, tier);
            setAmbientOcclusion (scene, ambientOcclusion);

            SceneRenderer renderer;
            const auto image = renderToImage (renderer, scene, 64);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion) != nullptr, ambientOcclusion);
            return image;
        };

        // A fresh Metal device's first frame can differ by a level in a few pixels, see checkContactShadowsIgnoredOnRaster()
        ASSERT_TRUE (render (false).isValid());

        const auto withAmbientOcclusion = render (true);
        const auto without = render (false);
        ASSERT_TRUE (withAmbientOcclusion.isValid() && without.isValid());
        EXPECT_EQ (countDifferencesAboveOneLevel (withAmbientOcclusion, without), 0);
    }

    /** Ambient occlusion keeps the frames deterministic on @p tier (preflight R13): its noise
        rotates only under temporal anti-aliasing, whose running mean freezes once converged, so
        two renderers converging on the same scene end on the same image.
    */
    void checkConvergedAmbientOcclusionIsDeterministic (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildContactCreaseScene (scene);
        setTier (scene, tier);
        setAmbientOcclusion (scene, true);

        SceneRenderer warmUp;
        ASSERT_TRUE (renderToImage (warmUp, scene, 64).isValid());

        SceneRenderer a, b;
        const auto imageA = renderConverged (a, scene, 64);
        const auto imageB = renderConverged (b, scene, 64);
        ASSERT_TRUE (imageA.isValid() && imageB.isValid());
        EXPECT_TRUE (a.isConverged() && b.isConverged());
        EXPECT_NE (a.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion), nullptr);

        auto differing = 0;
        for (int y = 0; y < imageA.getHeight(); ++y)
            for (int x = 0; x < imageA.getWidth(); ++x)
                differing += imageA.getPixelColor (x, y) != imageB.getPixelColor (x, y) ? 1 : 0;

        EXPECT_EQ (differing, 0);
    }

    /** On @p tier, blended surfaces skip ambient occlusion: the prepass and the G-buffer hold the
        opaque surfaces only, so their occlusion belongs to what lies behind. A blended quad of alpha
        1 hiding the crease of buildContactCreaseScene() renders the same with and without it.
    */
    void checkBlendedSurfacesSkipAmbientOcclusion (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool ambientOcclusion)
        {
            Scene scene;
            buildContactCreaseScene (scene);
            setTier (scene, tier);
            setAmbientOcclusion (scene, ambientOcclusion);

            // Facing the camera halfway to the crease, 0.2 m wide: it covers the crease rectangle
            const Vector3<float> foot (0.0f, 0.0f, 0.3f);
            const auto elevation = degreesToRadians (30.0f);
            const Vector3<float> toCamera (0.0f, std::sin (elevation), std::cos (elevation));
            const Vector3<float> up (0.0f, std::cos (elevation), -std::sin (elevation));
            const Vector3<float> right (1.0f, 0.0f, 0.0f);
            const auto center = foot + toCamera * 0.45f;

            std::vector<Mesh::Vertex> vertices (4);
            const std::array<std::pair<float, float>, 4> corners { { { -1.0f, -1.0f }, { 1.0f, -1.0f }, { 1.0f, 1.0f }, { -1.0f, 1.0f } } };
            for (size_t i = 0; i < 4; ++i)
            {
                vertices[i].position = center + right * (0.1f * corners[i].first) + up * (0.1f * corners[i].second);
                vertices[i].normal = toCamera;
            }

            auto material = Material::Ptr (new Material());
            material->baseColorFactor = { 0.8f, 0.8f, 0.8f, 1.0f };
            material->metallicFactor = 0.0f;
            material->roughnessFactor = 0.5f;
            material->alphaMode = Material::AlphaMode::blend;

            auto mesh = Mesh::Ptr (new Mesh ("pane"));
            mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);

            auto pane = EntityNode::Ptr (new EntityNode ("pane"));
            pane->attach<MeshNode> (mesh);
            scene.getRoot()->addChild (pane);

            SceneRenderer renderer;
            const auto image = renderConverged (renderer, scene, 128);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion) != nullptr, ambientOcclusion);
            return image;
        };

        const auto withAmbientOcclusion = render (true);
        const auto without = render (false);
        ASSERT_TRUE (withAmbientOcclusion.isValid() && without.isValid());

        const auto crease = contactCreaseRect (128, 128);
        RecordProperty ("paneWith", String (meanLuminance (withAmbientOcclusion, crease), 1).toStdString());
        RecordProperty ("paneWithout", String (meanLuminance (without, crease), 1).toStdString());
        EXPECT_NEAR (meanLuminance (withAmbientOcclusion, crease), meanLuminance (without, crease), 1.0);
    }

    /** On @p tier, the bent normals turn the diffuse light of the environment towards the open side:
        under a sky bright above and dark below, the foot of the box's face, whose bent normals lean
        up away from the ground, gets more of the sky than under the same sky turned upside down. The
        face is vertical and has no specular, so without bent normals both skies light it the same.
    */
    void checkBentNormalsTurnTheDiffuseLightToTheOpenSide (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool skyAbove)
        {
            Scene scene;
            buildContactCreaseScene (scene);
            setTier (scene, tier);
            setAmbientOcclusion (scene, true);
            scene.setAmbientColor (Colors::black);

            // Diffuse only: no dielectric specular, so only the irradiance tells the skies apart
            auto material = Material::Ptr (new Material());
            material->baseColorFactor = { 0.8f, 0.8f, 0.8f, 1.0f };
            material->metallicFactor = 0.0f;
            material->roughnessFactor = 1.0f;
            material->specularFactor = 0.0f;
            auto* box = scene.getRoot()->findChild ("box");
            box->getNode<MeshNode>()->mesh = makeBox (0.3f, material);

            scene.setEnvironment (new EnvironmentMap ([skyAbove] (const Vector3<float>& direction)
            {
                const auto up = jlimit (0.0f, 1.0f, (skyAbove ? direction.getY() : -direction.getY()) * 0.5f + 0.5f);
                return Vector3<float> (1.0f, 1.0f, 1.0f) * (0.05f + 2.0f * up * up);
            }, 32, 3));

            SceneRenderer renderer;
            const auto image = renderConverged (renderer, scene, 128);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            return image;
        };

        const auto skyAbove = render (true);
        const auto skyBelow = render (false);
        ASSERT_TRUE (skyAbove.isValid() && skyBelow.isValid());

        // The foot of the face, just above the crease, and the top of the face, open to the sky
        const Rectangle<int> foot (45, 58, 38, 4);
        const Rectangle<int> top (45, 30, 38, 4);
        const auto footAbove = meanLuminance (skyAbove, foot), footBelow = meanLuminance (skyBelow, foot);
        RecordProperty ("footSkyAbove", String (footAbove, 1).toStdString());
        RecordProperty ("footSkyBelow", String (footBelow, 1).toStdString());
        RecordProperty ("topSkyAbove", String (meanLuminance (skyAbove, top), 1).toStdString());
        RecordProperty ("topSkyBelow", String (meanLuminance (skyBelow, top), 1).toStdString());
        EXPECT_GT (footAbove, footBelow + 3.0);
    }

    /** Raster: ambient occlusion turned on, then off again, leaves the frame exactly as it was before,
        the variant without ambient occlusion shading it again.
    */
    void checkRasterAmbientOcclusionOffAgainRestoresTheFrame()
    {
        Scene scene;
        buildContactCreaseScene (scene);
        setTier (scene, RenderSettings::TierPreference::raster);

        // A fresh Metal device's frames can differ by a level in a few hundred pixels (see
        // checkContactShadowsIgnoredOnRaster()): the frames compare within one level, while the
        // ambient occlusion darkens the crease by about ten
        SceneRenderer renderer;
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid());
        const auto before = renderToImage (renderer, scene, 64);

        setAmbientOcclusion (scene, true);
        const auto during = renderToImage (renderer, scene, 64);

        setAmbientOcclusion (scene, false);
        const auto after = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (before.isValid() && during.isValid() && after.isValid()) << renderer.getLastError();

        EXPECT_GT (countDifferencesAboveOneLevel (during, before), 0);
        EXPECT_EQ (countDifferencesAboveOneLevel (after, before), 0);
    }

    /** A mirror floor (white metal, roughness 0.02), 12 x 12 at y = 0, the entity "floor", under a
        large green emitter 2 m up facing down, the entity "emitter". The camera looks down at the
        floor from (0, 1.5, 2.5) at 31 degrees, so the top edge of its frustum points 8 degrees below
        the horizon and only the floor's reflection can show the emitter. No environment and no
        default light: without a reflection probe the floor reflects black, and shows the flat
        ambient light only. @p withEmitter false leaves the emitter out.
    */
    /** A horizontal quad at height @p y over @p area (x, z), facing up or down, the entity @p name. */
    static EntityNode::Ptr makeHorizontalQuad (const char* name, float y, Rectangle<float> area, bool facingUp, Material::Ptr material)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { area.getX(), y, area.getBottom() };
        vertices[1].position = { area.getRight(), y, area.getBottom() };
        vertices[2].position = { area.getRight(), y, area.getY() };
        vertices[3].position = { area.getX(), y, area.getY() };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, facingUp ? 1.0f : -1.0f, 0.0f };

        // Counterclockwise seen from the side the normal points to
        auto mesh = Mesh::Ptr (new Mesh (name));
        mesh->addPrimitive (std::move (vertices), facingUp ? std::vector<uint32> { 0, 1, 2, 0, 2, 3 } : std::vector<uint32> { 0, 2, 1, 0, 3, 2 }, std::move (material));

        auto entity = EntityNode::Ptr (new EntityNode (name));
        entity->attach<MeshNode> (mesh);
        return entity;
    }

    /** A black material emitting @p emission. */
    static Material::Ptr makeEmitterMaterial (std::array<float, 3> emission)
    {
        auto emitter = Material::Ptr (new Material());
        emitter->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        emitter->emissiveFactor = emission;
        return emitter;
    }

    static void buildMirrorFloorScene (Scene& scene, bool withEmitter = true)
    {
        auto mirror = Material::Ptr (new Material());
        mirror->baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
        mirror->metallicFactor = 1.0f;
        mirror->roughnessFactor = 0.02f;
        scene.getRoot()->addChild (makeHorizontalQuad ("floor", 0.0f, { -6.0f, -6.0f, 12.0f, 12.0f }, true, mirror));

        if (withEmitter)
            scene.getRoot()->addChild (makeHorizontalQuad ("emitter", 2.0f, { -30.0f, -40.0f, 60.0f, 41.5f }, false, makeEmitterMaterial ({ 0.0f, 1.0f, 0.0f })));

        addCamera (scene, { 0.0f, 1.5f, 2.5f }, {});
        scene.setUsingDefaultLight (false);
    }

    /** The middle of the floor of buildMirrorFloorScene() in a frame of @p size x @p size. */
    static Rectangle<int> floorRegion (int size = 64)
    {
        return { size / 4, size * 3 / 8, size / 2, size / 2 };
    }

    /** The mean color of the 8 bit pixels within @p area. */
    static Color meanColor (const Image& image, Rectangle<int> area)
    {
        double sum[3] {};
        for (int y = area.getY(); y < area.getBottom(); ++y)
        {
            for (int x = area.getX(); x < area.getRight(); ++x)
            {
                const auto color = image.getPixelColor (x, y);
                sum[0] += color.getRed();
                sum[1] += color.getGreen();
                sum[2] += color.getBlue();
            }
        }

        const auto count = static_cast<double> (area.getWidth() * area.getHeight());
        return Color (static_cast<uint8> (std::lround (sum[0] / count)),
                      static_cast<uint8> (std::lround (sum[1] / count)),
                      static_cast<uint8> (std::lround (sum[2] / count)));
    }

    /** Sets the emission of the emitter of buildMirrorFloorScene(). */
    static void setEmitterColor (Scene& scene, std::array<float, 3> color)
    {
        auto* emitter = scene.getRoot()->findChild ("emitter");
        ASSERT_NE (emitter, nullptr);
        emitter->getNode<MeshNode>()->mesh->getPrimitive (0).material->emissiveFactor = color;
    }

    /** Adds a spherical reflection probe at @p position, the entity "probe", and returns its part. */
    static ReflectionProbeNode& addReflectionProbe (Scene& scene, Vector3<float> position, float influenceRadius, const char* name = "probe")
    {
        auto probe = EntityNode::Ptr (new EntityNode (name));
        auto& node = probe->attach<ReflectionProbeNode>();
        node.influenceRadius = influenceRadius;
        probe->setPosition (position);
        scene.getRoot()->addChild (probe);
        return node;
    }

    /** The number of probes the renderer baked, or -1 before it had a baker. */
    static int getNumBakes (const SceneRenderer& renderer)
    {
        const auto* baker = renderer.getProbeBaker();
        return baker != nullptr ? baker->getNumBakes() : -1;
    }

    /** On @p tier, a reflection probe shows the floor what the camera cannot see: the emitter of
        buildMirrorFloorScene() above the frustum, which the floor reflects only through the probe.
    */
    void checkReflectionProbeShowsWhatTheCameraCannotSee (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        setTier (scene, tier);

        SceneRenderer renderer;
        const auto before = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (before.isValid()) << renderer.getLastError();
        EXPECT_EQ (renderer.getProbeBaker(), nullptr); // Nothing allocated without a probe

        const auto floorBefore = meanColor (before, floorRegion());
        EXPECT_NEAR (floorBefore.getGreen(), floorBefore.getRed(), 2); // The camera does not see the emitter

        addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);

        const auto after = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        ASSERT_TRUE (after.isValid());
        EXPECT_EQ (getNumBakes (renderer), 1);

        const auto floorAfter = meanColor (after, floorRegion());
        RecordProperty ("floorGreenBefore", floorBefore.getGreen());
        RecordProperty ("floorGreenAfter", floorAfter.getGreen());
        EXPECT_GT (floorAfter.getGreen(), floorBefore.getGreen() + 40);
        EXPECT_NEAR (floorAfter.getRed(), floorBefore.getRed(), 2);
    }

    /** On @p tier, a probe part replacing another on the same entity is a new probe: it bakes, and
        does not reuse what the old one captured.
    */
    void checkReplacedProbeIsBakedAgain (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        setTier (scene, tier);
        addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);

        SceneRenderer renderer;
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();

        // Turn the green emitter red, then replace the probe: the new part must bake, not reuse the old tile
        setEmitterColor (scene, { 1.0f, 0.0f, 0.0f });
        auto* entity = scene.getRoot()->findChild ("probe");
        ASSERT_TRUE (entity->detach<ReflectionProbeNode>());
        entity->attach<ReflectionProbeNode>().influenceRadius = 20.0f;

        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 2);
        ASSERT_NE (renderer.getProbeBaker(), nullptr);
        EXPECT_EQ (renderer.getProbeBaker()->getNumBakedProbes(), 1);

        const auto floor = meanColor (image, floorRegion());
        EXPECT_GT (floor.getRed(), floor.getGreen() + 30);
    }

    /** On @p tier, a probe bakes again when it moves or one of its own fields changes, never because
        the rest of the scene changed, until requestBake() asks for it.
    */
    void checkMovingAProbeBakesItAgain (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        setTier (scene, tier);
        auto& probe = addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);

        SceneRenderer renderer;
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 1);

        // Unchanged, it keeps its bake
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 1);

        scene.getRoot()->findChild ("probe")->setPosition ({ 0.5f, 1.0f, -1.0f });
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 2);

        probe.clipNear = 0.2f;
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 3);

        // The emitter turns red: the probe keeps showing what it captured
        setEmitterColor (scene, { 1.0f, 0.0f, 0.0f });
        const auto stale = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (stale.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 3);
        EXPECT_GT (meanColor (stale, floorRegion()).getGreen(), meanColor (stale, floorRegion()).getRed() + 30);

        probe.requestBake();
        const auto rebaked = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (rebaked.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 4);
        EXPECT_GT (meanColor (rebaked, floorRegion()).getRed(), meanColor (rebaked, floorRegion()).getGreen() + 30);
    }

    /** On @p tier, a probe whose falloff crosses the floor, in a scene without an environment and
        with a black emitter, blends to finite values everywhere: the weights are never normalized by
        their sum. The floor shows the ambient light only, so it is even; a non-finite blend would
        show as a block of pixels unlike their neighbors (Metal clamps NaN to the other operand).
    */
    void checkProbeBlendWithoutEnvironmentIsFinite (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        setEmitterColor (scene, { 0.0f, 0.0f, 0.0f });

        if (tier == RenderSettings::TierPreference::full)
            setFullTierTaa (scene, 1);
        else
            setTier (scene, tier);

        // Fading out from 1 m to 3 m around a point 1 m over the floor's center
        auto& probe = addReflectionProbe (scene, { 0.0f, 1.0f, 0.0f }, 3.0f);
        probe.falloff = 2.0f;

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 1);

        const auto floor = floorRegion();
        auto uneven = 0;
        for (int y = floor.getY(); y < floor.getBottom() - 1; ++y)
        {
            for (int x = floor.getX(); x < floor.getRight() - 1; ++x)
            {
                const auto color = image.getPixelColor (x, y);
                for (const auto& neighbor : { image.getPixelColor (x + 1, y), image.getPixelColor (x, y + 1) })
                    if (std::abs (color.getRed() - neighbor.getRed()) > 2 || std::abs (color.getGreen() - neighbor.getGreen()) > 2 || std::abs (color.getBlue() - neighbor.getBlue()) > 2)
                        ++uneven;
            }
        }

        EXPECT_EQ (uneven, 0);
        EXPECT_GT (meanColor (image, floor).getGreen(), 10); // Not black: the ambient light shows

#if YUP_ENABLE_SHADER_TRANSPILER
        if (tier == RenderSettings::TierPreference::full)
        {
            const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
            ASSERT_NE (sceneColor, nullptr);
            EXPECT_TRUE (isFiniteTexture (sceneColor));
            EXPECT_FALSE (isSaturatedTexture (sceneColor));
        }
#endif
    }

    /** On @p tier, a box probe enclosing the scene, its reflections projected onto the box, shows
        the floor the emitter too.
    */
    void checkBoxProbeWithParallaxReflects (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        setTier (scene, tier);

        SceneRenderer renderer;
        const auto before = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (before.isValid()) << renderer.getLastError();

        auto& probe = addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);
        probe.shape = ReflectionProbeNode::Shape::box;
        probe.influenceExtents = { 40.0f, 5.0f, 45.0f };
        ASSERT_TRUE (probe.parallaxCorrection);

        const auto after = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (after.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 1);

        RecordProperty ("floorGreenBefore", meanColor (before, floorRegion()).getGreen());
        RecordProperty ("floorGreenAfter", meanColor (after, floorRegion()).getGreen());
        EXPECT_GT (meanColor (after, floorRegion()).getGreen(), meanColor (before, floorRegion()).getGreen() + 40);
    }

    /** On @p tier, bakeProbes() bakes the probes never baked or asked to bake again, and returns
        how many; render() then has nothing left to bake.
    */
    void checkBakeProbesReportsPendingCount (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        setTier (scene, tier);
        auto& first = addReflectionProbe (scene, { -1.0f, 1.0f, -1.0f }, 20.0f, "first");
        addReflectionProbe (scene, { 1.0f, 1.0f, -1.0f }, 5.0f, "second");

        SceneRenderer renderer;
        EXPECT_EQ (renderer.bakeProbes (device, scene), 2);
        EXPECT_EQ (renderer.bakeProbes (device, scene), 0);

        first.requestBake();
        EXPECT_EQ (renderer.bakeProbes (device, scene), 1);
        EXPECT_EQ (getNumBakes (renderer), 3);

        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 3);
        EXPECT_GT (meanColor (renderToImage (renderer, scene, 64), floorRegion()).getGreen(), 100);
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    /** On @p tier, the probe resources live only as long as probes need them: the capture shadow
        maps and scratch targets until the next call that bakes nothing, the atlas until no probe is
        left. The capture shadow renderer itself, with its pipelines, stays.
    */
    void checkRemovedProbesReleaseTheAtlas (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        setTier (scene, tier);
        addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);

        SceneRenderer renderer;
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        const auto* baker = renderer.getProbeBaker();
        ASSERT_NE (baker, nullptr);
        EXPECT_NE (baker->getReflectionAtlas(), nullptr);
        EXPECT_TRUE (baker->holdsCaptureTargets());

        const auto* captureShadows = baker->getCaptureShadowRenderer();
        ASSERT_NE (captureShadows, nullptr);
        EXPECT_NE (captureShadows->getCascadeTexture(), nullptr);

        // The next call bakes nothing: the capture textures go, the pipelines stay for the frames in flight
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (baker->getCaptureShadowRenderer(), captureShadows);
        EXPECT_EQ (captureShadows->getCascadeTexture(), nullptr);
        EXPECT_EQ (captureShadows->getAtlasTexture(), nullptr);
        EXPECT_FALSE (baker->holdsCaptureTargets());
        EXPECT_NE (baker->getReflectionAtlas(), nullptr);

        scene.getRoot()->removeChild (scene.getRoot()->findChild ("probe"));
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (renderer.getProbeBaker(), baker);
        EXPECT_EQ (baker->getNumBakedProbes(), 0);
        EXPECT_EQ (baker->getReflectionAtlas(), nullptr);
    }

    /** On @p tier, a probe over an open sky reflects what the environment does: its capture faces
        keep the orientation of the cube. The sky's color tells directions apart (red grows towards
        +x, blue towards -z, green upwards), so a face mirrored or turned shows on the floor.
    */
    void checkProbeOfAnOpenSkyMatchesTheEnvironment (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool withProbe)
        {
            Scene scene;
            buildMirrorFloorScene (scene, false);
            setTier (scene, tier);
            scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>& d)
            {
                return Vector3<float> (0.6f + 0.55f * jlimit (-1.0f, 1.0f, 2.5f * d.getX()),
                                       0.3f + 0.3f * d.getY(),
                                       0.6f - 0.55f * jlimit (-1.0f, 1.0f, 2.5f * d.getZ()));
            }, 256, 7));

            // Without parallax the probe reads its capture along the reflected direction, as the environment does
            if (withProbe)
                addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f).parallaxCorrection = false;

            SceneRenderer renderer;
            const auto image = renderToImage (renderer, scene, 64);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            EXPECT_EQ (getNumBakes (renderer), withProbe ? 1 : -1);
            return image;
        };

        const auto environment = render (false);
        const auto probe = render (true);
        ASSERT_TRUE (environment.isValid() && probe.isValid());

        // Left and right, near and far parts of the floor reflect different parts of the sky
        const Rectangle<int> regions[] = { { 8, 30, 12, 10 }, { 44, 30, 12, 10 }, { 8, 50, 12, 10 }, { 44, 50, 12, 10 }, { 26, 26, 12, 8 } };
        const auto left = meanColor (environment, regions[0]), right = meanColor (environment, regions[1]);
        EXPECT_GT (std::abs (left.getRed() - right.getRed()), 20); // The check cannot pass on a uniform sky

        for (const auto& region : regions)
        {
            const auto expected = meanColor (environment, region);
            const auto actual = meanColor (probe, region);
            RecordProperty (("environment" + region.toString()).toStdString(), expected.toString().toStdString());
            RecordProperty (("probe" + region.toString()).toStdString(), actual.toString().toStdString());
            EXPECT_NEAR (actual.getRed(), expected.getRed(), 8) << region.toString();
            EXPECT_NEAR (actual.getGreen(), expected.getGreen(), 8) << region.toString();
            EXPECT_NEAR (actual.getBlue(), expected.getBlue(), 8) << region.toString();
        }
    }

    /** On @p tier, below the float floor the probes hold RGBM, which the passes decode: the
        reflection matches the half float one, channel by channel. The mirror floor reflects a blue
        emitter of radiance 3 on the left, a channel no tone map of the capture may saturate, and a
        dim gray one of 0.006 on the right, which 8 bit linear steps would round by a third.
    */
    void checkRgbmProbeShowsLikeTheFloatOne (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool rgbm)
        {
            Scene scene;
            buildMirrorFloorScene (scene, false);
            setTier (scene, tier);
            scene.setAmbientColor (Colors::black);
            scene.getRoot()->addChild (makeHorizontalQuad ("blue", 2.0f, { -30.0f, -40.0f, 30.0f, 41.5f }, false, makeEmitterMaterial ({ 0.0f, 0.0f, 3.0f })));
            scene.getRoot()->addChild (makeHorizontalQuad ("dim", 2.0f, { 0.0f, -40.0f, 30.0f, 41.5f }, false, makeEmitterMaterial ({ 0.006f, 0.006f, 0.006f })));
            addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);

            SceneRenderer renderer;
            if (rgbm)
                renderer.forceRgbmRadianceForTesting();

            const auto image = renderToImage (renderer, scene, 64);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();

            const auto* baker = renderer.getProbeBaker();
            const auto atlas = baker != nullptr ? baker->getReflectionAtlas() : nullptr;
            EXPECT_TRUE (atlas != nullptr && atlas->getFormat() == (rgbm ? GpuTextureFormat::rgba8unorm : GpuTextureFormat::rgba16float));
            return image;
        };

        const auto floatImage = render (false);
        const auto rgbmImage = render (true);
        ASSERT_TRUE (floatImage.isValid() && rgbmImage.isValid());

        const Rectangle<int> blueRegion (6, 28, 14, 24), dimRegion (44, 28, 14, 24);
        const auto expectedBlue = meanColor (floatImage, blueRegion), actualBlue = meanColor (rgbmImage, blueRegion);
        const auto expectedDim = meanColor (floatImage, dimRegion), actualDim = meanColor (rgbmImage, dimRegion);
        RecordProperty ("blueFloat", expectedBlue.toString().toStdString());
        RecordProperty ("blueRgbm", actualBlue.toString().toStdString());
        RecordProperty ("dimFloat", expectedDim.toString().toStdString());
        RecordProperty ("dimRgbm", actualDim.toString().toStdString());

        // Not vacuous: the blue is well above what a saturated capture would give, the dim region above black
        EXPECT_GT (expectedBlue.getBlue(), 210);
        EXPECT_GT (expectedDim.getGreen(), 10);

        for (const auto& [expected, actual, tolerance] : { std::tuple { expectedBlue, actualBlue, 3 }, std::tuple { expectedDim, actualDim, 1 } })
        {
            EXPECT_NEAR (actual.getRed(), expected.getRed(), tolerance);
            EXPECT_NEAR (actual.getGreen(), expected.getGreen(), tolerance);
            EXPECT_NEAR (actual.getBlue(), expected.getBlue(), tolerance);
        }
    }

    /** On @p tier, the reflection probes keep the multiple scattering compensation of rough metals
        (Fdez-Aguera 2019, preflight R23): a white metal of roughness 1 facing the view, inside a room
        whose walls all emit 0.5, lit only by a probe that captured the room, reflects all of it, as
        under a uniform environment (RoughWhiteMetalPassesTheWhiteFurnaceTest). Single scattering
        alone would reflect 1 - ln 2 of it. The tolerance is the environment furnace's 0.02 plus a
        level for the probe's own capture and filtering.
    */
    void checkProbeKeepsTheMultipleScatteringOfRoughMetals (RenderSettings::TierPreference tier)
    {
        Scene scene;
        auto metal = Material::Ptr (new Material());
        metal->roughnessFactor = 1.0f;
        addPlate (scene, metal);

        auto room = EntityNode::Ptr (new EntityNode ("room"));
        room->attach<MeshNode> (makeBox (10.0f, makeEmitterMaterial ({ 0.5f, 0.5f, 0.5f }), true));
        scene.getRoot()->addChild (room);

        scene.setAmbientColor (Colors::black);
        scene.setUsingDefaultLight (false);
        scene.setToneMapping (Scene::ToneMapping::reinhard);
        setTier (scene, tier);
        addReflectionProbe (scene, { 0.0f, 0.0f, 2.0f }, 30.0f).parallaxCorrection = false;

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 1);

        std::array<double, 3> radiance {};
        for (int y = 28; y < 36; ++y)
        {
            for (int x = 28; x < 36; ++x)
            {
                const auto color = image.getPixelColor (x, y);
                radiance[0] += reinhardRadiance (color.getRed()) / 64.0;
                radiance[1] += reinhardRadiance (color.getGreen()) / 64.0;
                radiance[2] += reinhardRadiance (color.getBlue()) / 64.0;
            }
        }

        RecordProperty ("probeFurnaceBrightness", String (radiance[1] / 0.5, 4).toStdString());

        for (const auto channel : radiance)
            EXPECT_NEAR (channel, 0.5, 0.025);
    }

    /** On @p tier, parallax correction puts a box probe's reflections where the walls are: a mirror
        floor inside a 6 x 6 m room, the box probe filling it, captured from its center 1 m up,
        reflects a small green patch high on the far wall at its true mirror position. Read along the
        reflected direction alone, as without the correction, the patch lands elsewhere.
    */
    void checkBoxProbeParallaxPlacesTheReflectionOnTheWalls (RenderSettings::TierPreference tier)
    {
        constexpr int size = 128;
        const Vector3<float> eye (0.0f, 1.5f, 2.5f);
        const Vector3<float> patchCenter (1.1f, 1.6f, -2.95f);

        const auto render = [&] (bool parallax)
        {
            Scene scene;
            setTier (scene, tier);
            scene.setAmbientColor (Colors::black);
            scene.setUsingDefaultLight (false);

            auto mirror = Material::Ptr (new Material());
            mirror->metallicFactor = 1.0f;
            mirror->roughnessFactor = 0.02f;
            scene.getRoot()->addChild (makeHorizontalQuad ("floor", 0.0f, { -3.0f, -3.0f, 6.0f, 6.0f }, true, mirror));

            // On the far wall, facing into the room
            std::vector<Mesh::Vertex> vertices (4);
            vertices[0].position = patchCenter + Vector3<float> (-0.5f, -0.3f, 0.0f);
            vertices[1].position = patchCenter + Vector3<float> (0.5f, -0.3f, 0.0f);
            vertices[2].position = patchCenter + Vector3<float> (0.5f, 0.3f, 0.0f);
            vertices[3].position = patchCenter + Vector3<float> (-0.5f, 0.3f, 0.0f);
            for (auto& vertex : vertices)
                vertex.normal = { 0.0f, 0.0f, 1.0f };

            auto mesh = Mesh::Ptr (new Mesh ("patch"));
            mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, makeEmitterMaterial ({ 0.0f, 4.0f, 0.0f }));
            auto patch = EntityNode::Ptr (new EntityNode ("patch"));
            patch->attach<MeshNode> (mesh);
            scene.getRoot()->addChild (patch);

            addCamera (scene, eye, {});

            auto& probe = addReflectionProbe (scene, { 0.0f, 1.0f, 0.0f }, 1.0f);
            probe.shape = ReflectionProbeNode::Shape::box;
            probe.influenceExtents = { 3.0f, 1.2f, 3.0f };
            probe.falloff = 0.0f;
            probe.parallaxCorrection = parallax;

            SceneRenderer renderer;
            const auto image = renderToImage (renderer, scene, size);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            EXPECT_EQ (getNumBakes (renderer), 1);
            return image;
        };

        // The green pixels of the floor's reflection: the patch itself lies above the frustum
        const auto centroid = [] (const Image& image) -> std::optional<Point<float>>
        {
            Point<float> sum;
            auto count = 0;
            for (int y = 0; y < image.getHeight(); ++y)
            {
                for (int x = 0; x < image.getWidth(); ++x)
                {
                    const auto color = image.getPixelColor (x, y);
                    if (color.getGreen() > 60 && color.getGreen() > color.getRed() + 40)
                    {
                        sum += Point<float> (static_cast<float> (x) + 0.5f, static_cast<float> (y) + 0.5f);
                        ++count;
                    }
                }
            }

            return count > 0 ? std::optional<Point<float>> (sum / static_cast<float> (count)) : std::nullopt;
        };

        // Where the mirror image of the patch's center, under the floor, shows on screen
        CameraNode camera;
        const auto viewProjection = Matrix4::lookAt (eye, {}, { 0.0f, 1.0f, 0.0f }).followedBy (camera.getProjectionMatrix (1.0f));
        const auto ndc = viewProjection.transformPoint ({ patchCenter.getX(), -patchCenter.getY(), patchCenter.getZ() });
        const Point<float> expected ((ndc.getX() * 0.5f + 0.5f) * size, (0.5f - ndc.getY() * 0.5f) * size);

        const auto corrected = centroid (render (true));
        const auto uncorrected = centroid (render (false));
        ASSERT_TRUE (corrected.has_value() && uncorrected.has_value());

        RecordProperty ("expected", expected.toString().toStdString());
        RecordProperty ("corrected", corrected->toString().toStdString());
        RecordProperty ("uncorrected", uncorrected->toString().toStdString());

        EXPECT_LT (corrected->distanceTo (expected), 3.0f);
        EXPECT_GT (uncorrected->distanceTo (expected), corrected->distanceTo (expected) + 5.0f);
    }

    /** On @p tier, probe captures that fail to record never fail the frame: it renders without the
        probe, retries once, then stops baking until bakeProbes() asks again.
    */
    void checkFailedProbeCapturesLeaveTheFrameWithoutProbes (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        setTier (scene, tier);
        addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);

        SceneRenderer renderer;
        renderer.failProbeCapturesForTesting();

        const auto first = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (first.isValid()) << renderer.getLastError();
        EXPECT_TRUE (renderer.getLastError().isEmpty());
        ASSERT_NE (renderer.getProbeBaker(), nullptr);
        EXPECT_EQ (getNumBakes (renderer), 0);
        EXPECT_FALSE (renderer.getProbeBaker()->hasGivenUp());

        // Without the probe the floor shows no green
        const auto floor = meanColor (first, floorRegion());
        EXPECT_NEAR (floor.getGreen(), floor.getRed(), 2);

        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_TRUE (renderer.getProbeBaker()->hasGivenUp());

        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 0);

        // An explicit bake tries again, and fails once more
        EXPECT_EQ (renderer.bakeProbes (device, scene), 0);
        EXPECT_FALSE (renderer.getProbeBaker()->hasGivenUp());
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    /** buildMirrorFloorScene() without its emitter, plus a red emitter standing on the floor in
        front of the camera, 1.2 wide and 0.8 tall at z = -0.5, facing it: the camera sees both the
        emitter (rows 8 to 26 at 64 x 64) and its reflection in the floor (rows 26 to 40), which only
        screen-space reflections show, since the scene has no environment and no probe.
    */
    static void buildOnScreenEmitterScene (Scene& scene, float emission = 1.0f)
    {
        buildMirrorFloorScene (scene, false);

        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -0.6f, 0.0f, -0.5f };
        vertices[1].position = { 0.6f, 0.0f, -0.5f };
        vertices[2].position = { 0.6f, 0.8f, -0.5f };
        vertices[3].position = { -0.6f, 0.8f, -0.5f };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto mesh = Mesh::Ptr (new Mesh ("emitter"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, makeEmitterMaterial ({ emission, 0.0f, 0.0f }));

        auto emitter = EntityNode::Ptr (new EntityNode ("emitter"));
        emitter->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (emitter);
    }

    /** The floor of buildOnScreenEmitterScene() reflecting the emitter, at 64 x 64: between the
        emitter's foot (row 26) and the bottom of its reflection (row 40), within its columns.
    */
    static Rectangle<int> reflectedEmitterRegion()
    {
        return { 22, 28, 20, 10 };
    }

#if YUP_ENABLE_SHADER_TRANSPILER
    /** The mean linear red of @p texture over @p area, below 1, read back in 16 bits: its 8 high
        bits in red, the next 8 in green. */
    double meanLinearRed (const GpuTexture::Ptr& texture, Rectangle<int> area)
    {
        const auto linear = mapTexels (texture, "vec4(floor(t.r * 255.0) / 255.0, fract(t.r * 255.0), 0.0, 1.0)");
        if (! linear.isValid())
            return -1.0;

        auto sum = 0.0;
        for (int y = area.getY(); y < area.getBottom(); ++y)
        {
            for (int x = area.getX(); x < area.getRight(); ++x)
            {
                const auto color = linear.getPixelColor (x, y);
                sum += (color.getRed() + color.getGreen() / 255.0) / 255.0;
            }
        }

        return sum / (area.getWidth() * area.getHeight());
    }
#endif

    /** Switches screen-space reflections on or off, or back to the tier default. */
    static void setScreenSpaceReflections (Scene& scene, std::optional<bool> screenSpaceReflections)
    {
        auto settings = scene.getRenderSettings();
        settings.screenSpaceReflections = screenSpaceReflections;
        scene.setRenderSettings (settings);
    }

    /** On a raster device, screen-space reflections are unavailable: turning them on renders the
        same image as leaving them off, without an error and without an output.
    */
    void checkScreenSpaceReflectionsUnavailableOnRasterRendersFallback (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool screenSpaceReflections)
        {
            Scene scene;
            buildOnScreenEmitterScene (scene);
            setTier (scene, tier);
            setScreenSpaceReflections (scene, screenSpaceReflections);

            SceneRenderer renderer;
            const auto image = renderToImage (renderer, scene, 64);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);
            EXPECT_EQ (renderer.getReflectionPass(), nullptr);

            const auto* capabilities = renderer.getCapabilities();
            EXPECT_TRUE (capabilities != nullptr && ! capabilities->isAvailable (RenderCapabilities::Feature::screenSpaceReflections));
            return image;
        };

        // A fresh Metal device's first frame can differ by a level in a few pixels, see checkContactShadowsIgnoredOnRaster()
        ASSERT_TRUE (render (false).isValid());

        const auto withReflections = render (true);
        const auto without = render (false);
        ASSERT_TRUE (withReflections.isValid() && without.isValid());
        EXPECT_EQ (countDifferencesAboveOneLevel (withReflections, without), 0);
    }

    /** A vertical quad in the plane x = @p x over @p area (z, y), facing +X or -X, the entity @p name. */
    static EntityNode::Ptr makeWallQuad (const char* name, float x, Rectangle<float> area, bool facingPositiveX, Material::Ptr material)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { x, area.getY(), area.getBottom() };
        vertices[1].position = { x, area.getY(), area.getX() };
        vertices[2].position = { x, area.getBottom(), area.getX() };
        vertices[3].position = { x, area.getBottom(), area.getBottom() };

        for (auto& vertex : vertices)
            vertex.normal = { facingPositiveX ? 1.0f : -1.0f, 0.0f, 0.0f };

        // Counterclockwise seen from the side the normal points to
        auto mesh = Mesh::Ptr (new Mesh (name));
        mesh->addPrimitive (std::move (vertices), facingPositiveX ? std::vector<uint32> { 0, 1, 2, 0, 2, 3 } : std::vector<uint32> { 0, 2, 1, 0, 3, 2 }, std::move (material));

        auto entity = EntityNode::Ptr (new EntityNode (name));
        entity->attach<MeshNode> (mesh);
        return entity;
    }

    /** A white floor 8 x 6 at y = 0, from x = -4 to 4, lit by a constant gray environment of 0.5
        and no light, with no ambient light. A red emissive wall, the entity "red wall", double-sided
        and 3 m high, emitting 6, stands across it at x = -1.5. With @p withDivider an opaque gray wall, the entity
        "divider", double-sided and as high, splits the floor at x = 0: the floor between the two
        walls sees the red wall, the floor right of the divider does not. The volume probes next to
        the divider on its left (addIrradianceVolume()) see a lot of red, so trilinear weights alone
        would leak it to the right. The camera looks straight down from 11 m, +Z towards the bottom
        of the frame, so at 64 x 64 a floor point (x, z) shows at (32 + 6.88 x, 32 + 6.88 z).
    */
    static void buildRedRoomScene (Scene& scene, bool withDivider = true)
    {
        auto white = Material::Ptr (new Material());
        white->baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
        white->metallicFactor = 0.0f;
        white->roughnessFactor = 1.0f;
        scene.getRoot()->addChild (makeHorizontalQuad ("floor", 0.0f, { -4.0f, -3.0f, 8.0f, 6.0f }, true, white));

        auto red = makeEmitterMaterial ({ 6.0f, 0.0f, 0.0f });
        red->doubleSided = true;
        scene.getRoot()->addChild (makeWallQuad ("red wall", -1.5f, { -3.0f, 0.0f, 6.0f, 3.0f }, true, red));

        if (withDivider)
        {
            auto gray = makeWallMaterial();
            gray->doubleSided = true;
            scene.getRoot()->addChild (makeWallQuad ("divider", 0.0f, { -3.0f, 0.0f, 6.0f, 3.0f }, true, gray));
        }

        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
        scene.setAmbientColor (Colors::black);
        scene.setUsingDefaultLight (false);
        addCamera (scene, { 0.0f, 11.0f, 0.0f }, {}, { 0.0f, 0.0f, -1.0f });
    }

    /** The floor between the two walls of buildRedRoomScene() at 64 x 64, from x = -1.3 to -0.15 and z = -2 to 2. */
    static Rectangle<int> leftFloorRegion() { return { 23, 19, 8, 26 }; }

    /** The floor right of the divider of buildRedRoomScene() at 64 x 64, next to it: x = 0.15 to 0.58, z = -2 to 2. */
    static Rectangle<int> rightFloorRegion() { return { 33, 19, 3, 26 }; }

    /** Adds an irradiance volume of @p nx x @p ny x @p nz probes enclosing the room of
        buildRedRoomScene(), the entity "volume", and returns its part. With 2 rows they lie at
        y = 0.5 and 2.5, above the floor, and with 6 columns no probe lies in a wall: the cells are
        1.5 m wide along X, centered at x = -3.75, -2.25, -0.75, 0.75, 2.25 and 3.75. The floor lies
        0.5 m inside the box, beyond the falloff of 0.25.
    */
    static IrradianceVolumeNode& addIrradianceVolume (Scene& scene, int nx, int ny, int nz)
    {
        auto entity = EntityNode::Ptr (new EntityNode ("volume"));
        auto& volume = entity->attach<IrradianceVolumeNode>();
        volume.extents = { 4.5f, 2.0f, 3.5f };
        volume.resolutionX = nx;
        volume.resolutionY = ny;
        volume.resolutionZ = nz;
        volume.falloff = 0.25f;
        entity->setPosition ({ 0.0f, 1.5f, 0.0f });
        scene.getRoot()->addChild (entity);
        return volume;
    }

    /** Adds @p count small gray quads lying on the right of the floor of buildRedRoomScene(), each
        its own entity and draw, for scenes with many draws. */
    static void addSmallCasters (Scene& scene, int count)
    {
        for (int i = 0; i < count; ++i)
        {
            const auto x = 0.5f + 0.3f * static_cast<float> (i % 10);
            const auto z = -2.5f + 0.5f * static_cast<float> (i / 10);
            scene.getRoot()->addChild (makeHorizontalQuad ("caster", 0.05f, { x, z, 0.1f, 0.1f }, true, makeWallMaterial()));
        }
    }

    /** How much redder than green @p color is. */
    static float redOverGreen (Color color)
    {
        return static_cast<float> (color.getRed()) / static_cast<float> (jmax (1, static_cast<int> (color.getGreen())));
    }

    /** buildRedRoomScene() without the divider, on the full tier with 8 TAA samples and screen-space
        diffuse GI on or off, seen from (1.5, 2.5, 0) towards (-1, 0.3, 0): the red wall faces the
        camera across the floor, which giveRedWallFloorRegion() finds. Seen from above, as in the
        other red room tests, the wall would cover about a texel within the reach of the horizon
        search, which screen-space GI cannot gather from. The ambient occlusion distance, which the
        GI gathers within, is 1 m.
    */
    static void buildGIRedRoomScene (Scene& scene, bool screenSpaceGI)
    {
        buildRedRoomScene (scene, false);
        scene.getRoot()->removeChild (scene.getRoot()->findChild ("camera"));
        addCamera (scene, { 1.5f, 2.5f, 0.0f }, { -1.0f, 0.3f, 0.0f });
        setFullTierTaa (scene, 8);

        auto settings = scene.getRenderSettings();
        settings.aoDistance = 1.0f;
        settings.screenSpaceGI = screenSpaceGI;
        scene.setRenderSettings (settings);
    }

    /** The first and last rows of the middle column of a 64 x 64 @p image where an emissive red wall
        saturates the red and not the green, or -1 for none. */
    static std::pair<int, int> findRedWallRows (const Image& image)
    {
        auto top = -1, bottom = -1;
        for (int y = 0; image.isValid() && y < 64; ++y)
        {
            const auto color = image.getPixelColor (32, y);
            if (color.getRed() > 200 && color.getGreen() < 100)
            {
                top = top < 0 ? y : top;
                bottom = y;
            }
        }

        return { top, bottom };
    }

    /** A white floor 8 x 8 at y = 0 under a constant gray environment of 0.5, and a red emissive
        wall 4 m wide and 0.5 m high across it at z = 0, single-sided and facing the camera, which
        looks down at it from (0, 4, 2.5). The floor in front of the wall sees its emitting side; the
        floor behind it, which shows above its top edge from 0.3 m behind it, only its back. GI on the full tier with
        8 TAA samples and an ambient occlusion distance of 1 m.
    */
    static void buildFacingAwayWallScene (Scene& scene, bool screenSpaceGI)
    {
        auto white = Material::Ptr (new Material());
        white->baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
        white->metallicFactor = 0.0f;
        white->roughnessFactor = 1.0f;
        scene.getRoot()->addChild (makeHorizontalQuad ("floor", 0.0f, { -4.0f, -4.0f, 8.0f, 8.0f }, true, white));

        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -2.0f, 0.0f, 0.0f };
        vertices[1].position = { 2.0f, 0.0f, 0.0f };
        vertices[2].position = { 2.0f, 0.5f, 0.0f };
        vertices[3].position = { -2.0f, 0.5f, 0.0f };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto wall = Mesh::Ptr (new Mesh ("red wall"));
        wall->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, makeEmitterMaterial ({ 6.0f, 0.0f, 0.0f }));

        auto entity = EntityNode::Ptr (new EntityNode ("red wall"));
        entity->attach<MeshNode> (wall);
        scene.getRoot()->addChild (entity);

        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
        scene.setAmbientColor (Colors::black);
        scene.setUsingDefaultLight (false);
        addCamera (scene, { 0.0f, 4.0f, 2.5f }, { 0.0f, 0.0f, -0.5f });

        setFullTierTaa (scene, 8);
        auto settings = scene.getRenderSettings();
        settings.aoDistance = 1.0f;
        settings.screenSpaceGI = screenSpaceGI;
        scene.setRenderSettings (settings);
    }

    /** The number of irradiance volumes the renderer baked, or -1 before it had a baker. */
    static int getNumVolumeBakes (const SceneRenderer& renderer)
    {
        const auto* baker = renderer.getProbeBaker();
        return baker != nullptr ? baker->getNumVolumeBakes() : -1;
    }

    /** On @p tier, an irradiance volume tints the floor next to the red wall red: without it the
        floor shows the gray environment only.
    */
    void checkIrradianceVolumeTintsTheFloorNearTheRedWall (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene);
        setTier (scene, tier);

        SceneRenderer renderer;
        const auto without = meanColor (renderToImage (renderer, scene, 64), leftFloorRegion());
        ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        EXPECT_EQ (renderer.getProbeBaker(), nullptr); // Nothing allocated without a volume

        addIrradianceVolume (scene, 6, 2, 4);
        const auto with = meanColor (renderToImage (renderer, scene, 64), leftFloorRegion());
        ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 1);

        RecordProperty ("leftWithout", without.toString().toStdString());
        RecordProperty ("leftWith", with.toString().toStdString());
        EXPECT_NEAR (redOverGreen (without), 1.0f, 0.05f);
        EXPECT_GT (redOverGreen (with), redOverGreen (without) + 0.2f);
    }

    /** On @p tier, the volume's visibility keeps the red light of the probes on the left of the
        divider off the floor right of it, which those probes cannot see: their trilinear weights
        alone would let it through. Below the float floor visibility is skipped, so the check needs
        rg16float render targets.
    */
    void checkVisibilityStopsLightLeakingThroughTheWall (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene);
        setTier (scene, tier);
        addIrradianceVolume (scene, 6, 2, 4);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 1);

        if (! device->isFormatRenderable (GpuTextureFormat::rg16float))
            GTEST_SKIP() << "Visibility is skipped below the float floor";

        ASSERT_NE (renderer.getProbeBaker(), nullptr);
        ASSERT_NE (renderer.getProbeBaker()->getVisibilityAtlas(), nullptr);

        const auto left = meanColor (image, leftFloorRegion());
        const auto right = meanColor (image, rightFloorRegion());
        RecordProperty ("left", left.toString().toStdString());
        RecordProperty ("right", right.toString().toStdString());

        EXPECT_GT (redOverGreen (left), 1.2f); // Not vacuous: the red light is there
        EXPECT_LT (redOverGreen (right), 1.1f);
    }

    /** On @p tier, a volume bakes again when it moves or one of its own fields changes, never because
        the rest of the scene changed, until requestBake() asks for it.
    */
    void checkMovingAVolumeBakesItAgain (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        setTier (scene, tier);
        auto& volume = addIrradianceVolume (scene, 4, 2, 2);

        SceneRenderer renderer;
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 1);

        // Unchanged, it keeps its bake
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 1);

        scene.getRoot()->findChild ("volume")->setPosition ({ 0.0f, 1.6f, 0.0f });
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 2);

        volume.normalBias = 0.1f;
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 3);

        // The wall turns green: the volume keeps showing what it captured
        scene.getRoot()->findChild ("red wall")->getNode<MeshNode>()->mesh->getPrimitive (0).material->emissiveFactor = { 0.0f, 6.0f, 0.0f };
        const auto stale = meanColor (renderToImage (renderer, scene, 64), leftFloorRegion());
        EXPECT_EQ (getNumVolumeBakes (renderer), 3);
        EXPECT_GT (stale.getRed(), stale.getGreen() + 10);

        volume.requestBake();
        const auto rebaked = meanColor (renderToImage (renderer, scene, 64), leftFloorRegion());
        EXPECT_EQ (getNumVolumeBakes (renderer), 4);
        EXPECT_GT (rebaked.getGreen(), rebaked.getRed() + 10);
    }

    /** On @p tier, bakeProbes() bakes the volumes too, and returns how many probes and volumes it
        baked; render() then has nothing left to bake.
    */
    void checkBakeProbesBakesTheVolumes (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        setTier (scene, tier);
        auto& volume = addIrradianceVolume (scene, 4, 2, 2);

        SceneRenderer renderer;
        EXPECT_EQ (renderer.bakeProbes (device, scene), 1);
        EXPECT_EQ (renderer.bakeProbes (device, scene), 0);

        volume.requestBake();
        EXPECT_EQ (renderer.bakeProbes (device, scene), 1);
        EXPECT_EQ (getNumVolumeBakes (renderer), 2);

        const auto floor = meanColor (renderToImage (renderer, scene, 64), leftFloorRegion());
        EXPECT_EQ (getNumVolumeBakes (renderer), 2);
        EXPECT_GT (redOverGreen (floor), 1.2f);
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    /** On @p tier, the atlases of the volumes live as long as a volume needs them. */
    void checkRemovedVolumesReleaseTheAtlases (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        setTier (scene, tier);
        addIrradianceVolume (scene, 4, 2, 2);

        SceneRenderer renderer;
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        const auto* baker = renderer.getProbeBaker();
        ASSERT_NE (baker, nullptr);
        EXPECT_NE (baker->getIrradianceAtlas(), nullptr);
        EXPECT_EQ (baker->getVisibilityAtlas() != nullptr, device->isFormatRenderable (GpuTextureFormat::rg16float));
        EXPECT_EQ (baker->getNumBakedVolumes(), 1);

        // The next call bakes nothing and keeps the atlases
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_FALSE (baker->holdsCaptureTargets());
        EXPECT_NE (baker->getIrradianceAtlas(), nullptr);

        scene.getRoot()->removeChild (scene.getRoot()->findChild ("volume"));
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (renderer.getProbeBaker(), baker);
        EXPECT_EQ (baker->getNumBakedVolumes(), 0);
        EXPECT_EQ (baker->getIrradianceAtlas(), nullptr);
        EXPECT_EQ (baker->getVisibilityAtlas(), nullptr);
    }

    /** On @p tier, below the float floor the volume stores its first band in RGBM and the second as
        ratios to it, and lights the floor as the float volume does.
    */
    void checkRgbmVolumeLightsLikeTheFloatOne (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool rgbm)
        {
            Scene scene;
            buildRedRoomScene (scene, false);
            setTier (scene, tier);
            addIrradianceVolume (scene, 4, 2, 2);

            SceneRenderer renderer;
            if (rgbm)
                renderer.forceRgbmRadianceForTesting();

            const auto image = renderToImage (renderer, scene, 64);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();

            const auto* baker = renderer.getProbeBaker();
            const auto atlas = baker != nullptr ? baker->getIrradianceAtlas() : nullptr;
            EXPECT_TRUE (atlas != nullptr && atlas->getFormat() == (rgbm ? GpuTextureFormat::rgba8unorm : GpuTextureFormat::rgba16float));
            return image;
        };

        const auto floatImage = render (false);
        const auto rgbmImage = render (true);
        ASSERT_TRUE (floatImage.isValid() && rgbmImage.isValid());

        // The floor behind the red wall, in front of it and across the room
        for (const auto& region : { Rectangle<int> (6, 19, 8, 26), Rectangle<int> (23, 19, 8, 26), Rectangle<int> (40, 19, 16, 26) })
        {
            const auto expected = meanColor (floatImage, region), actual = meanColor (rgbmImage, region);
            RecordProperty (("float" + region.toString()).toStdString(), expected.toString().toStdString());
            RecordProperty (("rgbm" + region.toString()).toStdString(), actual.toString().toStdString());

            EXPECT_NEAR (actual.getRed(), expected.getRed(), 3) << region.toString();
            EXPECT_NEAR (actual.getGreen(), expected.getGreen(), 3) << region.toString();
            EXPECT_NEAR (actual.getBlue(), expected.getBlue(), 3) << region.toString();
        }

        // Not vacuous: the volume tints the floor, more next to the wall than across the room
        EXPECT_GT (redOverGreen (meanColor (floatImage, { 23, 19, 8, 26 })), redOverGreen (meanColor (floatImage, { 40, 19, 16, 26 })) + 0.1f);
    }

    /** On @p tier, a volume with more probe positions than one bake records (preflight R28) bakes
        over several frames, and weighs nothing until all of them are captured.
    */
    void checkProgressiveVolumeBakeShadesOnceComplete (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        setTier (scene, tier);
        addIrradianceVolume (scene, 7, 7, 7);

        const auto positions = 7 * 7 * 7;
        ASSERT_GT (positions, ProbeBaker::maxVolumeProbesPerBake);
        const auto expectedFrames = (positions + ProbeBaker::maxVolumeProbesPerBake - 1) / ProbeBaker::maxVolumeProbesPerBake;

        SceneRenderer renderer;
        const auto first = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (first.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 0);
        EXPECT_NEAR (redOverGreen (meanColor (first, leftFloorRegion())), 1.0f, 0.05f); // Weighs nothing yet

        auto frames = 1;
        Image last;
        while (getNumVolumeBakes (renderer) < 1 && frames < 4 * expectedFrames)
        {
            last = renderToImage (renderer, scene, 64);
            ASSERT_TRUE (last.isValid()) << renderer.getLastError();
            ++frames;
        }

        RecordProperty ("framesToBake", frames);
        EXPECT_EQ (frames, expectedFrames);
        EXPECT_EQ (getNumVolumeBakes (renderer), 1);
        EXPECT_GT (redOverGreen (meanColor (last, leftFloorRegion())), 1.2f);

        // Baked, it records nothing more
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 1);
    }

    /** On @p tier, a volume whose captures fail leaves the reflection probes baking and reflecting:
        each feature has its own failure latch.
    */
    void checkFailedVolumeCapturesLeaveTheProbesReflecting (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        setTier (scene, tier);
        addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);
        addIrradianceVolume (scene, 4, 2, 2);

        SceneRenderer renderer;
        renderer.failVolumeCapturesForTesting();

        Image image;
        for (int i = 0; i < 3; ++i)
        {
            image = renderToImage (renderer, scene, 64);
            ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        }

        ASSERT_NE (renderer.getProbeBaker(), nullptr);
        EXPECT_TRUE (renderer.getProbeBaker()->hasGivenUpVolumes());
        EXPECT_FALSE (renderer.getProbeBaker()->hasGivenUpProbes());
        EXPECT_EQ (getNumVolumeBakes (renderer), 0);
        EXPECT_EQ (getNumBakes (renderer), 1);

        const auto floor = meanColor (image, floorRegion());
        EXPECT_GT (floor.getGreen(), floor.getRed() + 40); // The probe still reflects the emitter
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    /** On @p tier, a volume whose resolution changes gives its blocks back and bakes again into new ones. */
    void checkResizingAVolumeBakesItAgain (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        setTier (scene, tier);
        auto& volume = addIrradianceVolume (scene, 4, 2, 2);

        SceneRenderer renderer;
        auto bakes = 0;
        for (const auto resolutionX : { 4, 6, 3, 6 })
        {
            volume.resolutionX = resolutionX;
            const auto image = renderToImage (renderer, scene, 64);
            ASSERT_TRUE (image.isValid()) << renderer.getLastError();
            EXPECT_EQ (getNumVolumeBakes (renderer), ++bakes) << resolutionX;
            EXPECT_GT (redOverGreen (meanColor (image, leftFloorRegion())), 1.2f) << resolutionX;
        }

        ASSERT_NE (renderer.getProbeBaker(), nullptr);
        EXPECT_EQ (renderer.getProbeBaker()->getNumBakedVolumes(), 1);
    }

    /** On @p tier, the volume lights without an environment, and keeps its own intensity when the
        environment's changes: its diffuse light is outside the environment's intensity, and the
        indirect branch opens for it alone (lightsIndirect's probeInfo.z gate). The volume here covers
        the left of the room only, so the floor on the right shows the environment.
    */
    void checkVolumeLightsWithoutTheEnvironmentAndKeepsItsIntensity (RenderSettings::TierPreference tier)
    {
        const auto addLeftVolume = [] (Scene& scene)
        {
            auto& volume = addIrradianceVolume (scene, 4, 2, 2);
            volume.extents = { 2.75f, 2.0f, 3.5f };
            scene.getRoot()->findChild ("volume")->setPosition ({ -1.75f, 1.5f, 0.0f });
        };

        const Rectangle<int> outsideRegion (46, 19, 10, 26); // x = 2 to 3.5, beyond the volume's border at x = 1
        {
            Scene scene;
            buildRedRoomScene (scene, false);
            setTierForExactFrames (scene, tier);
            scene.setEnvironment (nullptr);
            addLeftVolume (scene);

            SceneRenderer renderer;
            const auto image = renderToImage (renderer, scene, 64);
            ASSERT_TRUE (image.isValid()) << renderer.getLastError();
            EXPECT_EQ (getNumVolumeBakes (renderer), 1);

            const auto inside = meanColor (image, leftFloorRegion());
            RecordProperty ("insideWithoutEnvironment", inside.toString().toStdString());
            EXPECT_GT (inside.getRed(), 60);
            EXPECT_GT (redOverGreen (inside), 1.5f);

#if YUP_ENABLE_SHADER_TRANSPILER
            if (tier == RenderSettings::TierPreference::full)
            {
                const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
                ASSERT_NE (sceneColor, nullptr);
                EXPECT_TRUE (isFiniteTexture (sceneColor));
                EXPECT_FALSE (isSaturatedTexture (sceneColor));
            }
#endif
        }

        Scene scene;
        buildRedRoomScene (scene, false);
        setTierForExactFrames (scene, tier);
        addLeftVolume (scene);

        SceneRenderer renderer;
        const auto full = renderToImage (renderer, scene, 64);
        scene.setEnvironmentIntensity (0.25f);
        const auto quarter = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (full.isValid() && quarter.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 1); // An environment change never bakes it again

        const auto insideFull = meanColor (full, leftFloorRegion()), insideQuarter = meanColor (quarter, leftFloorRegion());
        const auto outsideFull = meanColor (full, outsideRegion), outsideQuarter = meanColor (quarter, outsideRegion);
        RecordProperty ("insideFull", insideFull.toString().toStdString());
        RecordProperty ("insideQuarter", insideQuarter.toString().toStdString());
        RecordProperty ("outsideFull", outsideFull.toString().toStdString());
        RecordProperty ("outsideQuarter", outsideQuarter.toString().toStdString());

        // The floor inside keeps the volume's light, the environment's specular share of a rough
        // dielectric aside (a few levels); the floor outside darkens with the environment
        EXPECT_NEAR (insideQuarter.getRed(), insideFull.getRed(), 6);
        EXPECT_NEAR (insideQuarter.getGreen(), insideFull.getGreen(), 6);
        EXPECT_LT (outsideQuarter.getGreen(), outsideFull.getGreen() - 30);
    }

    /** On @p tier, a volume that changes after its bake completed bakes again from its first
        position, weighs nothing until all of them are captured again, and is reported pending
        meanwhile, however many calls that takes (preflight R28).
    */
    void checkChangedVolumeBakesAgainWhole (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        setTier (scene, tier);
        addIrradianceVolume (scene, 7, 7, 7);

        const auto callsPerBake = (7 * 7 * 7 + ProbeBaker::maxVolumeProbesPerBake - 1) / ProbeBaker::maxVolumeProbesPerBake;

        SceneRenderer renderer;
        for (int i = 0; i < 4 * callsPerBake && getNumVolumeBakes (renderer) < 1; ++i)
            ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();

        ASSERT_EQ (getNumVolumeBakes (renderer), 1);
        const auto* baker = renderer.getProbeBaker();
        ASSERT_NE (baker, nullptr);

        // Moved: it bakes again, and weighs nothing until that bake completes
        scene.getRoot()->findChild ("volume")->setPosition ({ 0.0f, 1.6f, 0.0f });

        SceneDrawList list;
        auto calls = 0;
        while (calls < 4 * callsPerBake)
        {
            if (renderer.bakeProbes (device, scene) == 0)
                break;

            ++calls;
            list.build (*scene.getRoot(), Matrix4());

            if (getNumVolumeBakes (renderer) < 2)
            {
                EXPECT_EQ (baker->getNumPending (list), 1) << calls;
                EXPECT_TRUE (baker->packIrradianceVolumesByPriority (list).empty()) << calls;

                const auto packed = baker->packIrradianceVolumes (list);
                const GpuIrradianceVolume empty {};
                EXPECT_TRUE (packed.size() == 1 && std::memcmp (&packed[0], &empty, sizeof (empty)) == 0) << calls;
            }
        }

        EXPECT_EQ (calls, callsPerBake);
        EXPECT_EQ (getNumVolumeBakes (renderer), 2);
        EXPECT_EQ (baker->getNumPending (list), 0);

        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        EXPECT_GT (redOverGreen (meanColor (image, leftFloorRegion())), 1.2f);
    }

    /** On @p tier, reflection probe captures that fail leave the irradiance volumes baking and lighting. */
    void checkFailedProbeCapturesLeaveTheVolumesLighting (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        setTier (scene, tier);
        addIrradianceVolume (scene, 4, 2, 2);
        addReflectionProbe (scene, { 2.0f, 1.0f, 0.0f }, 1.0f);

        SceneRenderer renderer;
        renderer.failProbeCapturesForTesting();

        Image image;
        for (int i = 0; i < 3; ++i)
        {
            image = renderToImage (renderer, scene, 64);
            ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        }

        ASSERT_NE (renderer.getProbeBaker(), nullptr);
        EXPECT_TRUE (renderer.getProbeBaker()->hasGivenUpProbes());
        EXPECT_FALSE (renderer.getProbeBaker()->hasGivenUpVolumes());
        EXPECT_EQ (getNumBakes (renderer), 0);
        EXPECT_EQ (getNumVolumeBakes (renderer), 1);
        EXPECT_GT (redOverGreen (meanColor (image, leftFloorRegion())), 1.2f); // The volume still lights the floor
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    /** On @p tier, a volume whose capture fails leaves the frame rendering without it, and the
        baker gives up after two failures in a row, until bakeProbes() asks again.
    */
    void checkFailedVolumeCapturesLeaveTheFrameWithoutVolumes (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        setTier (scene, tier);
        addIrradianceVolume (scene, 4, 2, 2);

        SceneRenderer renderer;
        renderer.failVolumeCapturesForTesting();

        const auto first = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (first.isValid()) << renderer.getLastError();
        EXPECT_TRUE (renderer.getLastError().isEmpty());
        ASSERT_NE (renderer.getProbeBaker(), nullptr);
        EXPECT_EQ (getNumVolumeBakes (renderer), 0);
        EXPECT_FALSE (renderer.getProbeBaker()->hasGivenUp());

        // Without the volume the floor shows the gray environment only
        EXPECT_NEAR (redOverGreen (meanColor (first, leftFloorRegion())), 1.0f, 0.05f);

        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_TRUE (renderer.getProbeBaker()->hasGivenUp());

        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 0);

        // An explicit bake tries again, and fails once more
        EXPECT_EQ (renderer.bakeProbes (device, scene), 0);
        EXPECT_FALSE (renderer.getProbeBaker()->hasGivenUp());
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    //==============================================================================
    /** Adds a UV sphere of @p radius at the origin, 64 segments by 32 rings, drawn with @p material. */
    static EntityNode* addSphere (Scene& scene, float radius, Material::Ptr material)
    {
        constexpr int segments = 64, rings = 32;
        const auto pi = MathConstants<float>::pi;

        std::vector<Mesh::Vertex> vertices;
        for (int ring = 0; ring <= rings; ++ring)
        {
            const auto theta = pi * static_cast<float> (ring) / static_cast<float> (rings);
            for (int segment = 0; segment <= segments; ++segment)
            {
                // Segment 0 faces the default camera, on +z, and the segments turn towards +x
                const auto phi = 2.0f * pi * static_cast<float> (segment) / static_cast<float> (segments);
                const Vector3<float> normal (std::sin (theta) * std::sin (phi), std::cos (theta), std::sin (theta) * std::cos (phi));

                auto& vertex = vertices.emplace_back();
                vertex.position = normal * radius;
                vertex.normal = normal;
                vertex.uv = { static_cast<float> (segment) / static_cast<float> (segments), static_cast<float> (ring) / static_cast<float> (rings) };
            }
        }

        // Counter-clockwise seen from outside: upper left, lower left, lower right, then upper right
        std::vector<uint32> indices;
        for (int ring = 0; ring < rings; ++ring)
        {
            for (int segment = 0; segment < segments; ++segment)
            {
                const auto upperLeft = static_cast<uint32> (ring * (segments + 1) + segment);
                const auto lowerLeft = upperLeft + static_cast<uint32> (segments + 1);
                indices.insert (indices.end(), { upperLeft, lowerLeft, lowerLeft + 1, upperLeft, lowerLeft + 1, upperLeft + 1 });
            }
        }

        auto mesh = Mesh::Ptr (new Mesh ("sphere"));
        mesh->addPrimitive (std::move (vertices), std::move (indices), std::move (material));

        auto entity = EntityNode::Ptr (new EntityNode ("sphere"));
        entity->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (entity);
        return entity.get();
    }

    /** The radius in pixels of a sphere of @p radius at the origin seen by the default camera, from
        (0, 0, 5) with a vertical field of view of 0.8, in a square image of @p size pixels. */
    static float sphereRadiusInPixels (float radius, int size)
    {
        return static_cast<float> (size) * 0.5f * (radius / std::sqrt (25.0f - radius * radius)) / std::tan (0.4f);
    }

    /** The mean luminance of the pixels of a square image whose centers lie between @p inner and
        @p outer pixels from its center. */
    static double annulusLuminance (const Image& image, float inner, float outer)
    {
        const auto center = static_cast<float> (image.getWidth()) * 0.5f;
        auto sum = 0.0;
        auto count = 0;

        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto distance = std::hypot (static_cast<float> (x) + 0.5f - center, static_cast<float> (y) + 0.5f - center);
                if (distance >= inner && distance < outer)
                {
                    sum += meanLuminance (image, { x, y, 1, 1 });
                    ++count;
                }
            }
        }

        return count > 0 ? sum / count : -1.0;
    }

    /** The luminance of the brightest pixel of @p image. */
    static double brightestLuminance (const Image& image)
    {
        auto brightest = 0.0;
        for (int y = 0; y < image.getHeight(); ++y)
            for (int x = 0; x < image.getWidth(); ++x)
                brightest = jmax (brightest, meanLuminance (image, { x, y, 1, 1 }));

        return brightest;
    }

    /** A dielectric of base color @p base and roughness 1 with the given clearcoat and sheen: the
        base reflects no specular light when @p specularFactor is 0. */
    static Material::Ptr makeLayeredMaterial (std::array<float, 3> base, float clearcoat, float clearcoatRoughness,
                                              std::array<float, 3> sheen, float sheenRoughness, float specularFactor = 1.0f)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { base[0], base[1], base[2], 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 1.0f;
        material->specularFactor = specularFactor;
        material->clearcoatFactor = clearcoat;
        material->clearcoatRoughnessFactor = clearcoatRoughness;
        material->sheenColorFactor = sheen;
        material->sheenRoughnessFactor = sheenRoughness;
        return material;
    }

    /** A sphere of radius 1.5 drawn with @p material, lit head-on by a white sun of intensity 3
        alone, rendered on @p tier at 128 x 128 without temporal anti-aliasing. */
    Image renderLitSphere (Material::Ptr material, RenderSettings::TierPreference tier)
    {
        Scene scene;
        addSphere (scene, 1.5f, std::move (material));
        scene.setAmbientColor (Colors::black);
        setTierForExactFrames (scene, tier);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().intensity = 3.0f;
        scene.getRoot()->addChild (sun);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 128);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    }

    /** On @p tier, a clearcoat (KHR_materials_clearcoat) over a rough red base adds the sharp
        highlight of its own smooth lobe: the head-on sun's mirror point at the center of the sphere
        brightens by more than 40 levels of luminance, within a few pixels.
    */
    void checkClearcoatAddsASharpHighlightOnARoughBase (RenderSettings::TierPreference tier)
    {
        const auto matte = renderLitSphere (makeLayeredMaterial ({ 0.8f, 0.05f, 0.05f }, 0.0f, 0.05f, {}, 0.0f), tier);
        const auto coated = renderLitSphere (makeLayeredMaterial ({ 0.8f, 0.05f, 0.05f }, 1.0f, 0.05f, {}, 0.0f), tier);
        ASSERT_TRUE (matte.isValid() && coated.isValid());

        const auto matteBrightest = brightestLuminance (matte);
        const auto coatedBrightest = brightestLuminance (coated);
        RecordProperty ("matteBrightest", String (matteBrightest, 1).toStdString());
        RecordProperty ("coatedBrightest", String (coatedBrightest, 1).toStdString());
        EXPECT_GT (coatedBrightest, matteBrightest + 40.0);

        // Sharp: the pixels the highlight brightens by more than 20 levels lie at the center, within
        // a few pixels, where the rough base spreads over the whole sphere
        auto brightened = 0;
        for (int y = 0; y < 128; ++y)
        {
            for (int x = 0; x < 128; ++x)
            {
                if (meanLuminance (coated, { x, y, 1, 1 }) > meanLuminance (matte, { x, y, 1, 1 }) + 20.0)
                {
                    ++brightened;
                    EXPECT_LT (std::hypot (x + 0.5 - 64.0, y + 0.5 - 64.0), 8.0) << x << ", " << y;
                }
            }
        }

        RecordProperty ("brightenedPixels", brightened);
        EXPECT_GT (brightened, 0);
    }

    /** On @p tier, a white sheen (KHR_materials_sheen) over a black base without specular light
        brightens the grazing edges of the sphere, lit head-on: its Charlie lobe vanishes where the
        normal faces the view and the light, at the center, and peaks towards the silhouette.
    */
    void checkSheenBrightensGrazingEdges (RenderSettings::TierPreference tier)
    {
        const auto image = renderLitSphere (makeLayeredMaterial ({ 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, { 1.0f, 1.0f, 1.0f }, 0.5f, 0.0f), tier);
        ASSERT_TRUE (image.isValid());

        const auto radius = sphereRadiusInPixels (1.5f, 128);
        const auto center = annulusLuminance (image, 0.0f, 0.3f * radius);
        const auto ring = annulusLuminance (image, 0.85f * radius, 0.95f * radius);
        RecordProperty ("center", String (center, 1).toStdString());
        RecordProperty ("ring", String (ring, 1).toStdString());

        EXPECT_GT (ring, center + 20.0);
    }

    /** On @p tier, a white rough sheen over a black base without specular light, facing the view
        under a uniform environment of radiance 4 and no light, reflects the sheen albedo of the
        tables: 4 E (n.v = 1, roughness 1), 0.566. Within 2.5%: the half float table and its read
        0.2 texel off its first column (under 0.3%), and the rounding of the Reinhard-mapped image
        around level 160, up to half an 8-bit step there (about 1.1%). The scene color stays finite
        and unsaturated.
    */
    void checkSheenPassesTheWhiteFurnaceTest (RenderSettings::TierPreference tier)
    {
        Scene scene;
        addPlate (scene, makeLayeredMaterial ({ 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, { 1.0f, 1.0f, 1.0f }, 1.0f, 0.0f));
        scene.setAmbientColor (Colors::black);
        scene.setUsingDefaultLight (false);
        scene.setToneMapping (Scene::ToneMapping::reinhard);
        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (4.0f, 4.0f, 4.0f); }, 32, 3));
        setTierForExactFrames (scene, tier);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        std::array<double, 3> radiance {};
        for (int y = 28; y < 36; ++y)
        {
            for (int x = 28; x < 36; ++x)
            {
                const auto color = image.getPixelColor (x, y);
                EXPECT_LT (color.getGreen(), 250) << x << ", " << y;
                radiance[0] += reinhardRadiance (color.getRed()) / 64.0;
                radiance[1] += reinhardRadiance (color.getGreen()) / 64.0;
                radiance[2] += reinhardRadiance (color.getBlue()) / 64.0;
            }
        }

        const auto expected = 4.0 * LookupTables::computeSheenAlbedo()[static_cast<size_t> ((LookupTables::size - 1) * LookupTables::size)];
        RecordProperty ("sheenFurnace", String (radiance[1] / expected, 4).toStdString());

        for (const auto channel : radiance)
            EXPECT_NEAR (channel, expected, 0.025 * expected);

#if YUP_ENABLE_SHADER_TRANSPILER
        // Raw radiance: the full tier's scene color, and the raster rgba16float bloom path
        if (tier != RenderSettings::TierPreference::full)
        {
            setBloom (scene, true);
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        }

        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        ASSERT_NE (sceneColor, nullptr);
        EXPECT_TRUE (isFiniteTexture (sceneColor));
        EXPECT_FALSE (isSaturatedTexture (sceneColor));
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
#endif
    }

    /** On @p tier, the frames drawing a material with a clearcoat or a sheen shade with the layers
        variants (preflight R29), local indirect ones that alone compile the layers; frames with
        probes only take the plain local indirect variant, the others the default one. On raster
        the forward pass selects them for any item, on full the lighting pass for its opaque items
        (the full forward pass, for blended items, always compiles the layers).
    */
    void checkLayeredVariantFollowsTheMaterials (RenderSettings::TierPreference tier)
    {
        const auto full = tier == RenderSettings::TierPreference::full;
        auto material = makeLayeredMaterial ({ 0.5f, 0.5f, 0.5f }, 0.0f, 0.0f, {}, 0.0f);

        Scene scene;
        addPlate (scene, material);
        setTierForExactFrames (scene, tier);

        // The variant of the last frame: 0 default, 1 local indirect, 2 local indirect with the layers
        SceneRenderer renderer;
        const auto variant = [&]
        {
            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            if (full)
            {
                const auto* pass = renderer.getLightingPass();
                return pass == nullptr ? -1 : (pass->usesMaterialLayers() ? 2 : (pass->usesLocalIndirect() ? 1 : 0));
            }

            const auto* pass = renderer.getForwardPass();
            return pass == nullptr ? -1 : (pass->usesMaterialLayers() ? 2 : (pass->usesLocalIndirect() ? 1 : 0));
        };

        EXPECT_EQ (variant(), 0);

        material->clearcoatFactor = 0.5f;
        EXPECT_EQ (variant(), 2);

        material->clearcoatFactor = 0.0f;
        material->clearcoatRoughnessFactor = 0.5f;
        material->sheenRoughnessFactor = 0.5f;
        EXPECT_EQ (variant(), 0);

        material->sheenColorFactor = { 0.0f, 0.0f, 0.25f };
        EXPECT_EQ (variant(), 2);

        // A blended layered item: on full the forward pass shades it, the lighting pass stays default
        material->alphaMode = Material::AlphaMode::blend;
        EXPECT_EQ (variant(), full ? 0 : 2);

        // A probe without layered materials takes the local indirect variant without the layers
        material->alphaMode = Material::AlphaMode::opaque;
        material->sheenColorFactor = { 0.0f, 0.0f, 0.0f };
        addReflectionProbe (scene, { 0.0f, 0.0f, 2.0f }, 20.0f);
        EXPECT_EQ (variant(), 1);

        material->clearcoatFactor = 1.0f;
        EXPECT_EQ (variant(), 2);
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    /** On @p tier, a layers variant the device cannot compile is logged once and leaves the frames
        rendering with the plain local indirect variant, without the layers: the clearcoat's
        highlight is gone, and the frame is otherwise the uncoated one.
    */
    void checkFailedLayersVariantShadesWithoutTheLayers (RenderSettings::TierPreference tier)
    {
        const auto render = [this, tier] (float clearcoat, bool fail)
        {
            Scene scene;
            addSphere (scene, 1.5f, makeLayeredMaterial ({ 0.8f, 0.05f, 0.05f }, clearcoat, 0.05f, {}, 0.0f));
            scene.setAmbientColor (Colors::black);
            setTierForExactFrames (scene, tier);

            auto sun = EntityNode::Ptr (new EntityNode ("sun"));
            sun->attach<LightNode>().intensity = 3.0f;
            scene.getRoot()->addChild (sun);

            SceneRenderer renderer;
            if (fail)
                renderer.failMaterialLayersForTesting();

            auto image = renderToImage (renderer, scene, 128);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();

            const auto full = tier == RenderSettings::TierPreference::full;
            const auto* lightingPass = renderer.getLightingPass();
            const auto* forwardPass = renderer.getForwardPass();
            const auto layers = full ? lightingPass != nullptr && lightingPass->usesMaterialLayers() : forwardPass != nullptr && forwardPass->usesMaterialLayers();
            const auto localIndirect = full ? lightingPass != nullptr && lightingPass->usesLocalIndirect() : forwardPass != nullptr && forwardPass->usesLocalIndirect();
            EXPECT_EQ (layers, clearcoat > 0.0f && ! fail);
            EXPECT_EQ (localIndirect, clearcoat > 0.0f);
            return image;
        };

        const auto uncoated = render (0.0f, false);
        const auto failed = render (1.0f, true);
        const auto coated = render (1.0f, false);
        ASSERT_TRUE (uncoated.isValid() && failed.isValid() && coated.isValid());

        RecordProperty ("failedBrightest", String (brightestLuminance (failed), 1).toStdString());
        RecordProperty ("uncoatedBrightest", String (brightestLuminance (uncoated), 1).toStdString());
        EXPECT_NEAR (brightestLuminance (failed), brightestLuminance (uncoated), 1.0);
        EXPECT_GT (brightestLuminance (coated), brightestLuminance (uncoated) + 40.0); // Not vacuous
    }

    /** The mean linear radiance of the 8x8 pixels at the center of a 64 x 64 Reinhard-mapped image. */
    static std::array<double, 3> centerRadiance (const Image& image)
    {
        std::array<double, 3> radiance {};
        for (int y = 28; y < 36; ++y)
        {
            for (int x = 28; x < 36; ++x)
            {
                const auto color = image.getPixelColor (x, y);
                radiance[0] += reinhardRadiance (color.getRed()) / 64.0;
                radiance[1] += reinhardRadiance (color.getGreen()) / 64.0;
                radiance[2] += reinhardRadiance (color.getBlue()) / 64.0;
            }
        }

        return radiance;
    }

    /** On @p tier, a white sheen over a white base keeps the energy of a white furnace: under a
        uniform environment of radiance 1 and no light, the base without specular (specularFactor 0)
        reflects 1 - E (n.v) of it once the sheen dims it, and the sheen E (n.v), so the plate reads
        exactly 1, whatever the albedo E of the tables. Within 2.5%: around level 188 an 8-bit step of
        the Reinhard-mapped image is about 2.4%, so its rounding errs by up to 1.2%, and 1 reads
        1.0116. Without the base scaling it reads
        1 + E, 1.14 at roughness 1 seen head-on.
    */
    void checkSheenOverAWhiteBaseKeepsTheEnergy (RenderSettings::TierPreference tier)
    {
        Scene scene;
        addPlate (scene, makeLayeredMaterial ({ 1.0f, 1.0f, 1.0f }, 0.0f, 0.0f, { 1.0f, 1.0f, 1.0f }, 1.0f, 0.0f));
        scene.setAmbientColor (Colors::black);
        scene.setUsingDefaultLight (false);
        scene.setToneMapping (Scene::ToneMapping::reinhard);
        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (1.0f, 1.0f, 1.0f); }, 32, 3));
        setTierForExactFrames (scene, tier);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        const auto radiance = centerRadiance (image);
        RecordProperty ("whiteSheenFurnace", String (radiance[1], 4).toStdString());

        for (const auto channel : radiance)
            EXPECT_NEAR (channel, 1.0, 0.025);
    }

    /** On @p tier, a clearcoat of factor 1 dims the emission under it by 1 - Fc (n.v), Fc the
        Schlick Fresnel of f0 = 0.04: a black emitter without specular, turned 70 degrees away from
        the view, alone in the scene (the clearcoat reflects nothing there), is dimmed to 0.841 of
        its uncoated radiance at the center of the frame. Within 0.025: a few 8-bit steps of the
        Reinhard-mapped image, and the view angle varying by under a degree over the 8x8 pixels.
    */
    void checkClearcoatDimsTheEmission (RenderSettings::TierPreference tier)
    {
        const auto render = [this, tier] (float clearcoat)
        {
            auto material = makeLayeredMaterial ({ 0.0f, 0.0f, 0.0f }, clearcoat, 0.3f, {}, 0.0f, 0.0f);
            material->emissiveFactor = { 0.5f, 0.5f, 0.5f };

            Scene scene;
            addPlate (scene, material)->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, degreesToRadians (70.0f)));
            scene.setAmbientColor (Colors::black);
            scene.setUsingDefaultLight (false);
            scene.setToneMapping (Scene::ToneMapping::reinhard);
            setTierForExactFrames (scene, tier);

            SceneRenderer renderer;
            const auto image = renderToImage (renderer, scene, 64);
            EXPECT_TRUE (image.isValid()) << renderer.getLastError();
            return image.isValid() ? centerRadiance (image) : std::array<double, 3> {};
        };

        const auto bare = render (0.0f);
        const auto coated = render (1.0f);
        ASSERT_GT (bare[1], 0.2);

        const auto nDotV = std::cos (degreesToRadians (70.0));
        const auto expected = 1.0 - (0.04 + 0.96 * std::pow (1.0 - nDotV, 5.0));
        RecordProperty ("emissionUnderClearcoat", String (coated[1] / bare[1], 4).toStdString());

        for (size_t i = 0; i < 3; ++i)
            EXPECT_NEAR (coated[i] / bare[i], expected, 0.025) << i;
    }

    /** On @p tier, a clearcoat reflects a reflection probe and leaves the environment only the share
        the probe leaves: a clearcoated floor whose base reflects nothing (black, no specular), in
        buildMirrorFloorScene() under a red sky, shows the red sky in its coat without a probe. With
        a probe it shows the green emitter above the frustum, which only the probe sees, instead.
    */
    void checkClearcoatReflectsTheProbe (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        setEmitterColor (scene, { 0.0f, 8.0f, 0.0f });
        auto& floor = *scene.getRoot()->findChild ("floor")->getNode<MeshNode>()->mesh->getPrimitive (0).material;
        floor.baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        floor.metallicFactor = 0.0f;
        floor.roughnessFactor = 1.0f;
        floor.specularFactor = 0.0f;
        floor.clearcoatFactor = 1.0f;
        floor.clearcoatRoughnessFactor = 0.04f;
        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (4.0f, 0.0f, 0.0f); }, 32, 3));
        scene.setAmbientColor (Colors::black);
        setTierForExactFrames (scene, tier);

        SceneRenderer renderer;
        const auto before = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (before.isValid()) << renderer.getLastError();

        addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);
        const auto after = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (after.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 1);

        const auto floorBefore = meanColor (before, floorRegion());
        const auto floorAfter = meanColor (after, floorRegion());
        RecordProperty ("floorBefore", floorBefore.toString().toStdString());
        RecordProperty ("floorAfter", floorAfter.toString().toStdString());

        EXPECT_GT (floorBefore.getRed(), floorBefore.getGreen() + 20); // The coat reflects the red sky
        EXPECT_GT (floorAfter.getGreen(), floorBefore.getGreen() + 20); // then the emitter through the probe
        EXPECT_LT (floorAfter.getRed(), floorBefore.getRed() - 20);     // and the sky no more where the probe weighs 1
    }

    /** The largest difference of any channel of any pixel between two images, in 8 bit levels. */
    static int maxChannelDifference (const Image& image, const Image& reference)
    {
        int difference = 0;
        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto a = image.getPixelColor (x, y);
                const auto b = reference.getPixelColor (x, y);
                difference = std::max ({ difference, std::abs (a.getRed() - b.getRed()), std::abs (a.getGreen() - b.getGreen()), std::abs (a.getBlue() - b.getBlue()) });
            }
        }

        return difference;
    }

    //==============================================================================
    /** The size of the depth of field scenes, rendered with the camera at the origin looking down -Z
        (yFov 0.8: 15.1 px per unit at a distance of 10, a focal length of 28.4 mm). The circles of
        confusion of each scene are checked on the CPU first, through CameraNode::getCircleOfConfusion(),
        so the thresholds stand on known blur sizes.
    */
    static constexpr int depthOfFieldSize = 128;

    /** @p tier with depth of field on (the full tier accumulating @p taaSamples) over a black
        background and ambient light, the camera at the origin looking down -Z at f/1.4, focused at
        @p focusDistance.
    */
    static CameraNode& setUpDepthOfFieldScene (Scene& scene, RenderSettings::TierPreference tier, float focusDistance, int taaSamples = 1)
    {
        if (tier == RenderSettings::TierPreference::full)
            setFullTierTaa (scene, taaSamples);
        else
            setTier (scene, tier);

        setDepthOfField (scene, true);
        scene.setBackgroundColor (Colors::black);
        scene.setAmbientColor (Colors::black);
        addCamera (scene, {}, { 0.0f, 0.0f, -1.0f });

        auto& camera = *scene.getActiveCamera();
        camera.fStop = 1.4f;
        camera.focusDistance = focusDistance;
        return camera;
    }

    static void setDepthOfField (Scene& scene, bool depthOfField, float maxSize = 32.0f)
    {
        auto settings = scene.getRenderSettings();
        settings.depthOfField = depthOfField;
        settings.depthOfFieldMaxSize = maxSize;
        scene.setRenderSettings (settings);
    }

    /** An emissive rectangle facing the camera at @p distance down -Z, @p area in world units,
        blended with @p alpha below 1.
    */
    static void addEmitter (Scene& scene, Rectangle<float> area, float distance, float emission, float alpha = 1.0f)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { area.getX(), area.getY(), -distance };
        vertices[1].position = { area.getRight(), area.getY(), -distance };
        vertices[2].position = { area.getRight(), area.getBottom(), -distance };
        vertices[3].position = { area.getX(), area.getBottom(), -distance };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto mesh = Mesh::Ptr (new Mesh ("emitter"));
        auto material = makeEmitterMaterial ({ emission, emission, emission });
        if (alpha < 1.0f)
        {
            material->alphaMode = Material::AlphaMode::blend;
            material->baseColorFactor[3] = alpha;
        }

        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, std::move (material));

        auto entity = EntityNode::Ptr (new EntityNode ("emitter"));
        entity->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (entity);
    }

    /** Three vertical white emissive bars at @p barDistance, 0.15 of the distance wide with gaps as
        wide (23 px at 128 x 128), the middle one centered: its left edge is at x = 52.7 px. With an
        @p occluderDistance, a brighter square in front, 0.2 of that distance wide (30 px), centered.
    */
    static void buildBarsScene (Scene& scene, float barDistance, float occluderDistance = 0.0f)
    {
        const auto width = 0.15f * barDistance;
        for (int bar = -1; bar <= 1; ++bar)
            addEmitter (scene, { (2.0f * static_cast<float> (bar) - 0.5f) * width, -barDistance, width, 2.0f * barDistance }, barDistance, 2.0f);

        if (occluderDistance > 0.0f)
            addEmitter (scene, { -0.1f * occluderDistance, -0.1f * occluderDistance, 0.2f * occluderDistance, 0.2f * occluderDistance }, occluderDistance, 8.0f);
    }

    /** The pixels between 10% and 90% of the luminance range along the row segment [xFrom, xTo). */
    static int edgeWidth (const Image& image, int row, int xFrom, int xTo)
    {
        std::vector<double> luma;
        for (int x = xFrom; x < xTo; ++x)
            luma.push_back (meanLuminance (image, { x, row, 1, 1 }));

        const auto [low, high] = std::minmax_element (luma.begin(), luma.end());
        const auto range = *high - *low;

        return static_cast<int> (std::count_if (luma.begin(), luma.end(), [&] (double l)
        {
            return l > *low + 0.1 * range && l < *low + 0.9 * range;
        }));
    }

    /** The mean luma of the ring from 2 to 5 px around the occluder of buildBarsScene(), 30 px wide at the center. */
    static double occluderHaloLuminance (const Image& image)
    {
        const auto occluder = Rectangle<int> (49, 49, 30, 30);
        const auto outer = occluder.enlarged (5);
        const auto inner = occluder.enlarged (2);
        const auto outerArea = static_cast<double> (outer.getWidth() * outer.getHeight());
        const auto innerArea = static_cast<double> (inner.getWidth() * inner.getHeight());

        return (meanLuminance (image, outer) * outerArea - meanLuminance (image, inner) * innerArea) / (outerArea - innerArea);
    }

    /** The CoC diameter in pixels of the depth of field frame of a point at @p distance. */
    static float circleOfConfusion (const Scene& scene, float distance)
    {
        return scene.getActiveCamera()->getCircleOfConfusion (distance, static_cast<float> (depthOfFieldSize));
    }

    /** True when depth of field can run on @p tier on this device, at the default sample count. */
    bool isDepthOfFieldAvailable (RenderSettings::TierPreference tier) const
    {
        RenderSettings settings;
        settings.tier = tier;
        return RenderCapabilities (RenderCapabilities::queryDevice (*device), settings, 4).isAvailable (RenderCapabilities::Feature::depthOfField);
    }

    /** On @p tier, bars in focus stay as sharp as without depth of field, and blur out of focus. The
        scene color, before the blur, stays sharp.
    */
    void checkDepthOfFieldBlursOutOfFocusBarsOnly (RenderSettings::TierPreference tier)
    {
        constexpr auto size = depthOfFieldSize;

        Scene scene;
        auto& camera = setUpDepthOfFieldScene (scene, tier, 10.0f);
        buildBarsScene (scene, 10.0f);

        SceneRenderer renderer;
        const auto sharp = renderToImage (renderer, scene, size);
        ASSERT_TRUE (sharp.isValid()) << renderer.getLastError();

        // Focused on the bars, nothing blurs by a texel: the frame is the one without depth of field
        setDepthOfField (scene, false);
        const auto off = renderToImage (renderer, scene, size);
        ASSERT_TRUE (off.isValid()) << renderer.getLastError();
        RecordProperty ("focusedDifference", maxChannelDifference (sharp, off));
        EXPECT_EQ (countDifferences (sharp, off), 0);

        setDepthOfField (scene, true);
        camera.focusDistance = 0.2f;
        ASSERT_GT (circleOfConfusion (scene, 10.0f), 16.0f);

        const auto blurred = renderToImage (renderer, scene, size);
        ASSERT_TRUE (blurred.isValid()) << renderer.getLastError();
        ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr);

        // The left edge of the middle bar, at x = 52.7: a step in focus, a ramp out of focus. The scene
        // color of the frame, the output of TAA on full and the resolved color on raster, stays sharp: the blur runs after it
        const auto sharpWidth = edgeWidth (sharp, size / 2, 40, 66);
        const auto blurredWidth = edgeWidth (blurred, size / 2, 40, 66);
        EXPECT_LE (edgeWidth (readBack (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)), size / 2, 40, 66), 2);
        RecordProperty ("sharpEdgeWidth", sharpWidth);
        RecordProperty ("blurredEdgeWidth", blurredWidth);

        EXPECT_LE (sharpWidth, 2);
        EXPECT_GE (blurredWidth, 6);
    }

    /** On @p tier, bars in focus look the same with depth of field on and off, over the background
        color and over a visible black environment: the composite blurs nothing there, so tone
        mapping keeps the blend of the frames without it, their partly covered antialiased edges
        included (preflight R21). The full tier accumulates 8 jittered frames, which leave the edges
        partly covered, and converges first. Over the environment, the raster reference renders in
        half float too (bloom at intensity 0): the resolve of that path alone moves the edges against
        the default frame.
    */
    void checkInFocusEdgesMatchWithoutDepthOfField (RenderSettings::TierPreference tier)
    {
        constexpr auto size = depthOfFieldSize;
        const auto full = tier == RenderSettings::TierPreference::full;

        for (const auto overEnvironment : { false, true })
        {
            const auto name = overEnvironment ? "over the environment" : "over the background color";

            Scene scene;
            setUpDepthOfFieldScene (scene, tier, 10.0f, 8);
            buildBarsScene (scene, 10.0f);
            if (overEnvironment)
            {
                scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float>(); }, 32, 3));
                scene.setEnvironmentVisible (true);
            }

            SceneRenderer renderer;
            const auto on = full ? renderConverged (renderer, scene, size) : renderToImage (renderer, scene, size);
            ASSERT_TRUE (on.isValid()) << name << ": " << renderer.getLastError();
            ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr) << name;

            auto settings = scene.getRenderSettings();
            settings.depthOfField = false;
            settings.bloom = overEnvironment && ! full;
            settings.bloomIntensity = 0.0f;
            scene.setRenderSettings (settings);

            const auto off = renderToImage (renderer, scene, size);
            ASSERT_TRUE (off.isValid()) << name << ": " << renderer.getLastError();
            EXPECT_TRUE (! full || renderer.isConverged()) << name;

            // Not vacuous: the edges are partly covered
            const auto interior = static_cast<int> (off.getPixelColor (size / 2, size / 2).getGreen());
            int partial = 0;
            for (int y = 0; y < size; ++y)
                for (int x = 0; x < size; ++x)
                    partial += off.getPixelColor (x, y).getGreen() > 2 && off.getPixelColor (x, y).getGreen() < interior - 2 ? 1 : 0;

            RecordProperty (String (name).replace (" ", "_").toStdString() + "_partialPixels", partial);
            RecordProperty (String (name).replace (" ", "_").toStdString() + "_largestDifference", maxChannelDifference (on, off));
            EXPECT_GT (partial, size) << name;
            EXPECT_EQ (countDifferences (on, off), 0) << name;
        }
    }

    /** Raster depth of field that gave up stops paying for its route: the frames after it no longer
        draw the depth and normal prepass nor render in half float.
    */
    void checkRasterDepthOfFieldThatGaveUpStopsItsRoute()
    {
        Scene scene;
        setUpDepthOfFieldScene (scene, RenderSettings::TierPreference::raster, 0.2f);
        buildBarsScene (scene, 10.0f);

        SceneRenderer renderer;
        renderer.failDepthOfFieldForTesting (2);

        for (int frame = 0; frame < 3; ++frame)
        {
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk());

            // The two failing frames try, so they draw the route; the third finds it given up
            const auto tries = frame < 2;
            ASSERT_NE (renderer.getDepthOfField(), nullptr) << frame;
            EXPECT_EQ (renderer.getDepthOfField()->hasGivenUp(), frame >= 1) << frame;
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal) != nullptr, tries) << frame;
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)->getFormat() == GpuTextureFormat::rgba16float, tries) << frame;
        }
    }

    /** On @p tier, a near occluder out of focus spreads beyond its silhouette onto the sharp
        background. On full, with temporal anti-aliasing, so the blur runs after it as in the default
        chain, and the same renderer gives the image without it.
    */
    void checkNearFieldBleedsOverTheFocusedBackground (RenderSettings::TierPreference tier)
    {
        constexpr auto size = depthOfFieldSize;

        Scene scene;
        setUpDepthOfFieldScene (scene, tier, 10.0f, 16);
        buildBarsScene (scene, 10.0f, 0.3f);
        ASSERT_LT (circleOfConfusion (scene, 0.3f), -8.0f);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, size);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr);

        setDepthOfField (scene, false);
        const auto without = renderToImage (renderer, scene, size);
        ASSERT_TRUE (without.isValid()) << renderer.getLastError();

        const auto halo = occluderHaloLuminance (image);
        const auto haloWithout = occluderHaloLuminance (without);
        RecordProperty ("halo", String (halo, 2).toStdString());
        RecordProperty ("haloWithout", String (haloWithout, 2).toStdString());

        EXPECT_GT (halo, haloWithout + 10.0);

        // The halo over the background color is spread light, composited in radiance as over a
        // visible black environment (preflight R20, R21), not dimmed by its partial coverage
        setDepthOfField (scene, true);
        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float>(); }, 32, 3));
        scene.setEnvironmentVisible (true);
        const auto overEnvironment = renderToImage (renderer, scene, size);
        ASSERT_TRUE (overEnvironment.isValid()) << renderer.getLastError();

        const auto haloOverEnvironment = occluderHaloLuminance (overEnvironment);
        RecordProperty ("haloOverEnvironment", String (haloOverEnvironment, 2).toStdString());
        EXPECT_NEAR (halo, haloOverEnvironment, 0.05 * haloOverEnvironment);
    }

    /** On @p tier, with depth of field off, moving the focus changes nothing. */
    void checkDepthOfFieldOffChangesNothing (RenderSettings::TierPreference tier)
    {
        constexpr auto size = depthOfFieldSize;

        Scene scene;
        auto& camera = setUpDepthOfFieldScene (scene, tier, 10.0f);
        setDepthOfField (scene, false);
        buildBarsScene (scene, 10.0f, 0.3f);

        SceneRenderer renderer;
        const auto focusedFar = renderToImage (renderer, scene, size);
        ASSERT_TRUE (focusedFar.isValid()) << renderer.getLastError();

        camera.focusDistance = 1.0f;
        const auto focusedNear = renderToImage (renderer, scene, size);
        ASSERT_TRUE (focusedNear.isValid()) << renderer.getLastError();

        EXPECT_LE (maxChannelDifference (focusedFar, focusedNear), 1);
        EXPECT_EQ (renderer.getDepthOfField(), nullptr);
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr);
    }

    /** On @p tier, a white square out of focus over a red background color, without environment:
        across its edge the blur fades white into red through pink, never through a darker ring (R5).
    */
    void checkDepthOfFieldKeepsTheSilhouetteBlendOverTheBackground (RenderSettings::TierPreference tier)
    {
        constexpr auto size = depthOfFieldSize;

        Scene scene;
        setUpDepthOfFieldScene (scene, tier, 0.2f);
        scene.setBackgroundColor (Colors::red);
        addEmitter (scene, { -1.5f, -1.5f, 3.0f, 3.0f }, 10.0f, 8.0f);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, size);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr);

        // The square spans x 41.4 to 86.6: across its left edge along the middle row
        int blended = 0;
        int darkest = 255;
        for (int x = 20; x < 64; ++x)
        {
            const auto color = image.getPixelColor (x, size / 2);
            darkest = jmin (darkest, static_cast<int> (color.getRed()));
            blended += color.getGreen() > 25 && color.getGreen() < 230 ? 1 : 0;
        }

        RecordProperty ("blendedPixels", blended);
        RecordProperty ("darkestRed", darkest);

        EXPECT_GE (blended, 6);
        EXPECT_GE (darkest, 230);
    }

    /** On @p tier, a bright square far out of focus over the black background color, without an
        environment, against the same square over a visible black environment: the blur spreads the
        same light over both, so the bokeh is as bright, not capped by its coverage (preflight R20).
    */
    void checkBlurredHighlightsKeepTheirBrightnessOverTheBackground (RenderSettings::TierPreference tier)
    {
        constexpr auto size = depthOfFieldSize;

        Scene scene;
        auto& camera = setUpDepthOfFieldScene (scene, tier, 0.1f);
        camera.fStop = 0.7f;
        setDepthOfField (scene, true, 64.0f);
        addEmitter (scene, { -0.2f, -0.2f, 0.4f, 0.4f }, 10.0f, 200.0f);

        SceneRenderer renderer;
        const auto overBackground = renderToImage (renderer, scene, size);
        ASSERT_TRUE (overBackground.isValid()) << renderer.getLastError();

        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float>(); }, 32, 3));
        scene.setEnvironmentVisible (true);
        const auto overEnvironment = renderToImage (renderer, scene, size);
        ASSERT_TRUE (overEnvironment.isValid()) << renderer.getLastError();

        const auto background = meanLuminance (overBackground, { 0, 0, size, size });
        const auto environment = meanLuminance (overEnvironment, { 0, 0, size, size });
        RecordProperty ("overBackground", String (background, 2).toStdString());
        RecordProperty ("overEnvironment", String (environment, 2).toStdString());

        ASSERT_GT (environment, 20.0);
        // Measured 54.43 against 54.52 on full: both tone map the same spread radiance over black. The
        // display space blend gave 0.4, as each pixel kept only its coverage share of the tone mapped highlight
        EXPECT_NEAR (background, environment, 0.05 * environment);
    }

    /** Raster depth of field alone draws the depth and normal prepass for its depth, but creates no
        ambient occlusion pass, and the forward pass shades with its default variant, not the local
        indirect one ambient occlusion selects. With ambient occlusion on too, both run.
    */
    void checkRasterDepthOfFieldDrawsThePrepassWithoutAmbientOcclusion()
    {
        Scene scene;
        setUpDepthOfFieldScene (scene, RenderSettings::TierPreference::raster, 0.2f);
        buildBarsScene (scene, 10.0f);

        SceneRenderer renderer;
        const auto render = [&] (bool ambientOcclusion)
        {
            auto settings = scene.getRenderSettings();
            settings.ambientOcclusion = ambientOcclusion;
            scene.setRenderSettings (settings);

            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk());
            EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr);
            EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr);
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)->getFormat(), GpuTextureFormat::rgba16float);
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion) != nullptr, ambientOcclusion);
            ASSERT_NE (renderer.getForwardPass(), nullptr);
            EXPECT_EQ (renderer.getForwardPass()->usesLocalIndirect(), ambientOcclusion);
        };

        render (false);
        EXPECT_EQ (renderer.getAmbientOcclusionPass(), nullptr);

        // Not vacuous: ambient occlusion takes the variant reading it
        render (true);
        EXPECT_NE (renderer.getAmbientOcclusionPass(), nullptr);
    }

    /** Raster depth of field where the device cannot run it, without the prepass or with too few
        multisampled half-float samples: the frames are the ones without depth of field, with no
        prepass, no HDR scene color and no depth of field object.
    */
    void checkRasterDepthOfFieldUnavailableRecordsNothing()
    {
        Scene scene;
        setUpDepthOfFieldScene (scene, RenderSettings::TierPreference::raster, 0.2f);
        buildBarsScene (scene, 10.0f);

        setDepthOfField (scene, false);
        SceneRenderer reference;
        const auto off = renderToImage (reference, scene, 64);
        ASSERT_TRUE (off.isValid()) << reference.getLastError();
        setDepthOfField (scene, true);

        const std::pair<const char*, std::function<void (RenderCapabilities::DeviceFacts&)>> devices[] = {
            { "no prepass", [] (RenderCapabilities::DeviceFacts& facts) { facts.depthNormalPrepass = false; } },
            { "one half float sample", [] (RenderCapabilities::DeviceFacts& facts) { facts.rgba16floatSamples = 1; } },
        };

        for (const auto& [name, edit] : devices)
        {
            SceneRenderer renderer;
            renderer.setDeviceFactsOverride (edit);

            for (int frame = 0; frame < 2; ++frame)
            {
                const auto image = renderToImage (renderer, scene, 64);
                ASSERT_TRUE (image.isValid()) << name << ": " << renderer.getLastError();
                EXPECT_LE (maxChannelDifference (image, off), 1) << name;
            }

            EXPECT_FALSE (renderer.getCapabilities()->isAvailable (RenderCapabilities::Feature::depthOfField)) << name;
            EXPECT_EQ (renderer.getDepthOfField(), nullptr) << name;
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr) << name;
            EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)->getFormat(), GpuTextureFormat::rgba16float) << name;
        }
    }

    /** Raster depth of field skips the frames whose prepass gave no depth, without counting them as
        failures: a prepass that cannot compile leaves every frame unblurred, and once prepass targets
        that could not be created come back, so does the blur.
    */
    void checkRasterDepthOfFieldSkipsFramesWithoutThePrepassDepth()
    {
        Scene scene;
        setUpDepthOfFieldScene (scene, RenderSettings::TierPreference::raster, 0.2f);
        buildBarsScene (scene, 10.0f);

        // More frames than the two failures in a row that give up
        const auto expectSkipped = [&] (SceneRenderer& renderer, const char* name)
        {
            for (int frame = 0; frame < 3; ++frame)
            {
                ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << name << ": " << renderer.getLastError();
                EXPECT_TRUE (renderer.waitForGPU().wasOk()) << name;
                EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr) << name;
                EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr) << name;
                ASSERT_NE (renderer.getDepthOfField(), nullptr) << name;
                EXPECT_FALSE (renderer.getDepthOfField()->hasGivenUp()) << name;
                EXPECT_EQ (renderer.getDepthOfField()->getNumCachedTargets(), 0u) << name;
            }
        };

        {
            SceneRenderer renderer;
            renderer.failDepthNormalPrepassForTesting();
            expectSkipped (renderer, "prepass not compiled");
        }

        SceneRenderer renderer;
        renderer.failDepthNormalPrepassTargetsForTesting (3);
        expectSkipped (renderer, "prepass without targets");

        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
        EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr);
        EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr);
    }

    //==============================================================================
    /** The size of the motion blur scenes, and the distance of their bar from the camera at the
        origin looking down -Z (yFov 0.8: 75.7 px per unit there at 128 x 128).
    */
    static constexpr int motionBlurSize = 128;
    static constexpr float motionBarDistance = 2.0f;

    /** An emissive quad facing the camera of buildMovingBarScene(), @p area in world units around
        the center of its entity, which sits motionBarDistance down the view axis, blended with
        @p alpha below 1. Returns the entity, which tests move with moveBar().
    */
    static EntityNode* addMotionQuad (Scene& scene, Rectangle<float> area, float emission, float alpha = 1.0f)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { area.getX(), area.getY(), 0.0f };
        vertices[1].position = { area.getRight(), area.getY(), 0.0f };
        vertices[2].position = { area.getRight(), area.getBottom(), 0.0f };
        vertices[3].position = { area.getX(), area.getBottom(), 0.0f };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto material = makeEmitterMaterial ({ emission, emission, emission });
        if (alpha < 1.0f)
        {
            material->alphaMode = Material::AlphaMode::blend;
            material->baseColorFactor[3] = alpha;
        }

        auto mesh = Mesh::Ptr (new Mesh ("quad"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, std::move (material));

        auto entity = EntityNode::Ptr (new EntityNode ("quad"));
        entity->attach<MeshNode> (mesh);
        entity->setPosition ({ 0.0f, 0.0f, -motionBarDistance });
        scene.getRoot()->addChild (entity);
        return entity.get();
    }

    /** One white emissive bar, 0.2 x 1.0 units (about 15 x 76 px at 128 x 128), at the center of
        the view of a camera at the origin looking down -Z, over the black background color, with
        black ambient light. Returns the bar, which tests move with moveBar().
    */
    static EntityNode* buildMovingBarScene (Scene& scene, float emission = 2.0f)
    {
        scene.setBackgroundColor (Colors::black);
        scene.setAmbientColor (Colors::black);
        addCamera (scene, {}, { 0.0f, 0.0f, -1.0f });
        return addMotionQuad (scene, { -0.1f, -0.5f, 0.2f, 1.0f }, emission);
    }

    /** Moves an entity of addMotionQuad() to @p x units right of the view axis. */
    static void moveBar (EntityNode& bar, float x)
    {
        bar.setPosition ({ x, 0.0f, -motionBarDistance });
    }

    /** The pixels per unit at motionBarDistance in the view of buildMovingBarScene(). */
    static float motionPixelsPerUnit (const Scene& scene)
    {
        return static_cast<float> (motionBlurSize) / (2.0f * motionBarDistance * std::tan (scene.getActiveCamera()->yFov * 0.5f));
    }

    static void setMotionBlur (Scene& scene, bool motionBlur, float shutter = 1.0f)
    {
        auto settings = scene.getRenderSettings();
        settings.motionBlur = motionBlur;
        settings.motionBlurShutter = shutter;
        scene.setRenderSettings (settings);
    }

    /** Motion blur on the raster tier, which has no screen-space motion: after a move, the frame
        is the one without it, without an error and without a motion blur object, on the default
        route and on the half float route of bloom, where the post chain runs.
    */
    void checkMotionBlurOnRasterRendersTheUnblurredFallback()
    {
        for (const auto bloom : { false, true })
        {
            const auto name = bloom ? "bloom" : "default";

            Scene scene;
            auto* bar = buildMovingBarScene (scene);
            auto settings = scene.getRenderSettings();
            settings.tier = RenderSettings::TierPreference::raster;
            settings.bloom = bloom;
            scene.setRenderSettings (settings);
            setMotionBlur (scene, true);

            SceneRenderer renderer;
            ASSERT_TRUE (renderToImage (renderer, scene, motionBlurSize).isValid()) << name << ": " << renderer.getLastError();

            moveBar (*bar, 0.1f);
            const auto on = renderToImage (renderer, scene, motionBlurSize);
            ASSERT_TRUE (on.isValid()) << name << ": " << renderer.getLastError();
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << name;
            EXPECT_FALSE (renderer.getCapabilities()->isAvailable (RenderCapabilities::Feature::motionBlur)) << name;
            EXPECT_FALSE (renderer.getCapabilities()->isEnabled (RenderCapabilities::Feature::motionBlur, scene.getRenderSettings())) << name;
            EXPECT_EQ (renderer.getMotionBlur(), nullptr) << name;
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr) << name;

            setMotionBlur (scene, false);
            const auto off = renderToImage (renderer, scene, motionBlurSize);
            ASSERT_TRUE (off.isValid()) << name << ": " << renderer.getLastError();
            EXPECT_LE (maxChannelDifference (on, off), 1) << name;
        }
    }

    GpuDevice::Ptr device;
    std::unique_ptr<GraphicsContext> context;
};
#endif

#if YUP_APPLE
class SceneRendererMetalTests : public SceneRendererBaselineScenes
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr)
            GTEST_SKIP() << "No Metal device available";

        context = GraphicsContext::createContext (GpuPlatform::Metal, {}, device);
        if (context == nullptr || ! context->isGpuAvailable())
            GTEST_SKIP() << "No Metal graphics context available";
    }


    /** The gray levels along row 40 of the edges scene at 128x128, which crosses the background (red
        26), the plate (red 151 to 153) and two silhouettes. The flat areas vary by a level or two, so
        levels within 2 of either count as that area: 2 without antialiasing.
    */
    static size_t countEdgeLevels (const Image& image)
    {
        std::set<int> levels;
        for (int x = 0; image.isValid() && x < 128; ++x)
        {
            const auto red = static_cast<int> (image.getPixelColor (x, 40).getRed());
            levels.insert (red <= 28 ? 26 : (red >= 149 ? 151 : red));
        }

        return levels.size();
    }

    /** Renders the shadows scene with a uniform environment of the given radiance and returns a corner, which shows the environment. */
    Color renderEnvironmentCorner (float radiance, Scene::ToneMapping toneMapping)
    {
        Scene scene;
        buildShadowsScene (scene);
        scene.setEnvironment (new EnvironmentMap ([radiance] (const Vector3<float>&) { return Vector3<float> (radiance, radiance, radiance); }, 32, 3));
        scene.setToneMapping (toneMapping);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);

        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        EXPECT_TRUE (image.isValid());
        return image.isValid() ? image.getPixelColor (1, 1) : Color();
    }

    /** A rough white metal plate and a smoother dielectric plate in front of it, lit by a sphere
        point light, a spot disk and a sun disk the front plate shadows. The back plate is larger
        than the frame region DeferredMatchesForwardForOpaque compares, so its silhouettes, where
        MSAA and TAA differ, stay out of it. */
    static void buildAreaLightsScene (Scene& scene)
    {
        const auto addScaledPlate = [&] (Material::Ptr material, float scale, Vector3<float> position)
        {
            auto* plate = addPlate (scene, std::move (material));
            plate->setScale ({ scale, scale, 1.0f });
            plate->setPosition (position);
        };

        auto metal = Material::Ptr (new Material());
        metal->roughnessFactor = 0.6f;

        auto dielectric = Material::Ptr (new Material());
        dielectric->baseColorFactor = { 0.6f, 0.3f, 0.2f, 1.0f };
        dielectric->metallicFactor = 0.0f;
        dielectric->roughnessFactor = 0.3f;

        addScaledPlate (metal, 1.3f, {});
        addScaledPlate (dielectric, 0.3f, { 0.3f, -0.3f, 0.3f });

        const auto addLight = [&] (LightNode::Type type, float intensity, float radius, Vector3<float> position, const Quaternion& rotation)
        {
            auto light = EntityNode::Ptr (new EntityNode ("light"));
            auto& node = light->attach<LightNode> (type);
            node.intensity = intensity;
            node.radius = radius;
            node.castsShadows = type == LightNode::Type::directional;
            light->setPosition (position);
            light->setRotation (rotation);
            scene.getRoot()->addChild (light);
        };

        addLight (LightNode::Type::point, 1.0f, 0.25f, { 0.4f, 0.3f, 1.0f }, Quaternion::identity());
        addLight (LightNode::Type::spot, 1.5f, 0.15f, { -0.4f, -0.3f, 1.2f }, Quaternion::identity());
        addLight (LightNode::Type::directional, 0.5f, 0.05f, {}, Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.5f));
    }

    /** A rough (roughness 1) plate drawn with @p material and lit head-on by a white sun of
        intensity @p sunIntensity, under @p ambient light: the mean linear radiance of the 8x8
        pixels at the center, where the normal, the view and the light directions coincide. There
        D = 1 / pi, V = 1 / 4 and F = f0, so the sun adds 2 (f0 / 4 + diffuseColor * weight) / pi
        at the default intensity of 2. A positive @p environmentRadiance adds a uniform environment
        of that radiance, and @p failLookupTables renders without the lookup tables.
    */
    std::array<double, 3> renderHeadOnRadiance (Material::Ptr material, RenderSettings::TierPreference tier,
                                                float sunIntensity = 2.0f, Color ambient = Colors::black,
                                                float environmentRadiance = 0.0f, bool failLookupTables = false)
    {
        material->roughnessFactor = 1.0f;

        Scene scene;
        addPlate (scene, std::move (material));
        scene.setAmbientColor (ambient);
        scene.setToneMapping (Scene::ToneMapping::reinhard);
        setTier (scene, tier);

        if (environmentRadiance > 0.0f)
            scene.setEnvironment (new EnvironmentMap ([environmentRadiance] (const Vector3<float>&) { return Vector3<float> (environmentRadiance, environmentRadiance, environmentRadiance); }, 32, 3));

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().intensity = sunIntensity;
        scene.getRoot()->addChild (sun);

        SceneRenderer renderer;
        if (failLookupTables)
            renderer.failLookupTablesForTesting();

        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();

        std::array<double, 3> radiance { -1.0, -1.0, -1.0 };
        if (! image.isValid())
            return radiance;

        radiance = { 0.0, 0.0, 0.0 };
        for (int y = 28; y < 36; ++y)
        {
            for (int x = 28; x < 36; ++x)
            {
                const auto color = image.getPixelColor (x, y);
                radiance[0] += reinhardRadiance (color.getRed()) / 64.0;
                radiance[1] += reinhardRadiance (color.getGreen()) / 64.0;
                radiance[2] += reinhardRadiance (color.getBlue()) / 64.0;
            }
        }

        return radiance;
    }

    /** A dielectric material of the given gray base color, for renderHeadOnRadiance. */
    static Material::Ptr makeDielectric (float base, float ior, float specularFactor, std::array<float, 3> specularColor = { 1.0f, 1.0f, 1.0f })
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { base, base, base, 1.0f };
        material->metallicFactor = 0.0f;
        material->ior = ior;
        material->specularFactor = specularFactor;
        material->specularColorFactor = specularColor;
        return material;
    }

    /** The pixels of row @p y more than 3 levels of red away from both the darkest and the
        brightest pixel of the row: partially covered silhouette pixels, 0 without antialiasing.
    */
    static int countIntermediatePixels (const Image& image, int y)
    {
        int darkest = 255, brightest = 0;
        for (int x = 0; x < image.getWidth(); ++x)
        {
            darkest = jmin (darkest, static_cast<int> (image.getPixelColor (x, y).getRed()));
            brightest = jmax (brightest, static_cast<int> (image.getPixelColor (x, y).getRed()));
        }

        int count = 0;
        for (int x = 0; x < image.getWidth(); ++x)
        {
            const auto red = static_cast<int> (image.getPixelColor (x, y).getRed());
            count += red > darkest + 3 && red < brightest - 3 ? 1 : 0;
        }

        return count;
    }

    /** Two emissive quads standing apart, rotated so their silhouettes cross pixels at every
        coverage: a bright one (10) on the left half of the frame and a mid-gray one (0.2) on the
        right half, over a blue-gray background.
    */
    static void buildSilhouettesScene (Scene& scene)
    {
        const auto addQuad = [&] (const char* name, float emission, float x)
        {
            std::vector<Mesh::Vertex> vertices (4);
            vertices[0].position = { -0.4f, -0.4f, 0.0f };
            vertices[1].position = { 0.4f, -0.4f, 0.0f };
            vertices[2].position = { 0.4f, 0.4f, 0.0f };
            vertices[3].position = { -0.4f, 0.4f, 0.0f };
            for (auto& vertex : vertices)
                vertex.normal = { 0.0f, 0.0f, 1.0f };

            auto material = Material::Ptr (new Material());
            material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
            material->emissiveFactor = { emission, emission, emission };

            auto mesh = Mesh::Ptr (new Mesh (name));
            mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);
            auto entity = EntityNode::Ptr (new EntityNode (name));
            entity->attach<MeshNode> (mesh);
            entity->setPosition ({ x, 0.0f, 0.0f });
            entity->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, 0.3f));
            scene.getRoot()->addChild (entity);
        };

        addQuad ("bright", 10.0f, -0.8f);
        addQuad ("gray", 0.2f, 0.8f);
        scene.setBackgroundColor (Color (static_cast<uint8> (40), static_cast<uint8> (70), static_cast<uint8> (110)));
    }

    /** The mean luma of @p image over the band within 2 px of the silhouettes of @p reference, in
        the columns from @p left to @p right. A silhouette pixel differs by more than 3 levels of
        luma from a 4-neighbor. The band reaches the flat areas on both sides, so it holds every
        partially covered pixel whatever the antialiasing, and its mean is linear in the coverage.
    */
    static double silhouetteBandLuminance (const Image& image, const Image& reference, int left, int right)
    {
        const auto luma = [] (const Image& source, int x, int y)
        {
            const auto color = source.getPixelColor (jlimit (0, source.getWidth() - 1, x), jlimit (0, source.getHeight() - 1, y));
            return 0.2126 * color.getRed() + 0.7152 * color.getGreen() + 0.0722 * color.getBlue();
        };

        const auto isSilhouette = [&] (int x, int y)
        {
            const auto center = luma (reference, x, y);
            return std::abs (center - luma (reference, x - 1, y)) > 3.0 || std::abs (center - luma (reference, x + 1, y)) > 3.0
                || std::abs (center - luma (reference, x, y - 1)) > 3.0 || std::abs (center - luma (reference, x, y + 1)) > 3.0;
        };

        auto sum = 0.0;
        int count = 0;
        for (int y = 0; y < reference.getHeight(); ++y)
        {
            for (int x = left; x < right; ++x)
            {
                bool inBand = false;
                for (int dy = -2; dy <= 2 && ! inBand; ++dy)
                    for (int dx = -2; dx <= 2 && ! inBand; ++dx)
                        inBand = isSilhouette (x + dx, y + dy);

                if (inBand)
                {
                    sum += luma (image, x, y);
                    ++count;
                }
            }
        }

        return count > 0 ? sum / count : -1.0;
    }

    /** Nine gray pads 0.4 wide and 0.55 apart, each its own draw, facing a camera 3 units away,
        under 32 colored point lights of range 0.5 and intensity 0.5, like the light show of the
        Scene3D demo. Returns the light entities, placed by moveOrbitingLights().
    */
    static std::vector<EntityNode*> buildOrbitingLightsScene (Scene& scene)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.5f, 0.5f, 0.5f, 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 0.7f;

        for (int row = 0; row < 3; ++row)
        {
            for (int column = 0; column < 3; ++column)
            {
                auto* pad = addPlate (scene, material);
                pad->setScale ({ 0.2f, 0.2f, 1.0f });
                pad->setPosition ({ 0.55f * static_cast<float> (column - 1), 0.55f * static_cast<float> (row - 1), 0.0f });
            }
        }

        std::vector<EntityNode*> lights;
        for (int i = 0; i < 32; ++i)
        {
            auto light = EntityNode::Ptr (new EntityNode ("showLight"));
            auto& node = light->attach<LightNode> (LightNode::Type::point);
            const auto color = Color::fromHSV (static_cast<float> (i) / 32.0f, 0.85f, 1.0f);
            node.color = { color.getRedFloat(), color.getGreenFloat(), color.getBlueFloat() };
            node.intensity = 0.5f;
            node.range = 0.5f;
            scene.getRoot()->addChild (light);
            lights.push_back (light.get());
        }

        auto camera = EntityNode::Ptr (new EntityNode ("camera"));
        camera->attach<CameraNode>();
        camera->setPosition ({ 0.0f, 0.0f, 3.0f });
        scene.getRoot()->addChild (camera);
        scene.setActiveCamera (camera.get());

        scene.setAmbientColor (Colors::black);
        scene.setBackgroundColor (Colors::black);
        return lights;
    }

    /** Places the lights of buildOrbitingLightsScene() at @p time seconds: four rings of orbits, alternating in direction, as in the demo. */
    static void moveOrbitingLights (const std::vector<EntityNode*>& lights, double time)
    {
        for (size_t i = 0; i < lights.size(); ++i)
        {
            const auto ring = static_cast<float> (i % 4);
            const auto speed = (i % 2 == 0 ? 0.35f : -0.25f) * (1.0f + 0.15f * ring);
            const auto angle = static_cast<float> (i) * 2.39996f + speed * static_cast<float> (time);
            const auto radius = 0.25f + 0.2f * ring;

            lights[i]->setPosition ({ std::cos (angle) * radius, std::sin (angle) * radius, 0.1f + 0.05f * ring });
        }
    }
};

TEST_F (SceneRendererMetalTests, AgxToneMapsTheEnvironment)
{
    const auto mid = renderEnvironmentCorner (0.5f, Scene::ToneMapping::agx);
    const auto expected = SceneRendererAgxMirrorTests::toneMapAgx ({ 0.5f, 0.5f, 0.5f });

    // 0.5 through AgX lands inside the displayable range, neither crushed nor clipped
    EXPECT_GT (mid.getRed(), 0);
    EXPECT_LT (mid.getRed(), 255);
    EXPECT_NEAR (mid.getRed(), expected[0], 3);
    EXPECT_NEAR (mid.getGreen(), expected[1], 3);
    EXPECT_NEAR (mid.getBlue(), expected[2], 3);

    const auto aces = renderEnvironmentCorner (0.5f, Scene::ToneMapping::aces);
    EXPECT_GE (std::abs (static_cast<int> (aces.getRed()) - static_cast<int> (mid.getRed())), 3);

    const auto dim = renderEnvironmentCorner (0.25f, Scene::ToneMapping::agx);
    const auto bright = renderEnvironmentCorner (1.0f, Scene::ToneMapping::agx);
    EXPECT_NEAR (dim.getRed(), SceneRendererAgxMirrorTests::toneMapAgx ({ 0.25f, 0.25f, 0.25f })[0], 3);
    EXPECT_NEAR (bright.getRed(), SceneRendererAgxMirrorTests::toneMapAgx ({ 1.0f, 1.0f, 1.0f })[0], 3);
    EXPECT_GT (bright.getRed(), mid.getRed());
    EXPECT_GT (mid.getRed(), dim.getRed());
}

TEST_F (SceneRendererMetalTests, RendersEnvironmentShadowsAndBackground)
{
    Scene scene;
    scene.getRoot()->addChild (makePlates());

    // Shining down -Z, tilted so the shadow of the small plate falls beside it
    auto sun = EntityNode::Ptr (new EntityNode ("sun"));
    sun->attach<LightNode>().castsShadows = true;
    sun->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.5f));
    scene.getRoot()->addChild (sun);

    scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
    scene.setEnvironmentVisible (true);
    scene.setToneMapping (Scene::ToneMapping::aces);

    SceneRenderer renderer;
    const auto image = renderToImage (renderer, scene, 64);

    ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
    ASSERT_TRUE (image.isValid());

    // A corner shows the environment: 0.5 through ACES is 0.616, 206 once sRGB encoded
    const auto corner = image.getPixelColor (1, 1);
    EXPECT_NEAR (corner.getRed(), 206, 3);
    EXPECT_NEAR (corner.getGreen(), 206, 3);
    EXPECT_NEAR (corner.getBlue(), 206, 3);
}

TEST_F (SceneRendererMetalTests, RgbmEnvironmentShadesLikeTheFloatOne)
{
    // Below the float floor the renderer's environment cube holds RGBM, and LightingBlock.probeInfo
    // tells the background and the surfaces to decode it. Undecoded, the corner would show 0.996, not 0.5
    const auto render = [this] (bool rgbm)
    {
        Scene scene;
        scene.getRoot()->addChild (makePlates());
        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
        scene.setEnvironmentVisible (true);
        scene.setToneMapping (Scene::ToneMapping::aces);

        SceneRenderer renderer;
        if (rgbm)
            renderer.forceRgbmRadianceForTesting();

        auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    };

    const auto floatImage = render (false);
    const auto rgbmImage = render (true);
    ASSERT_TRUE (floatImage.isValid());
    ASSERT_TRUE (rgbmImage.isValid());

    EXPECT_NEAR (rgbmImage.getPixelColor (1, 1).getRed(), 206, 1);
    EXPECT_NEAR (floatImage.getPixelColor (1, 1).getRed(), 206, 1);

    // The plates in the middle reflect the environment
    const Rectangle<int> plates (24, 24, 16, 16);
    EXPECT_NEAR (meanLuminance (rgbmImage, plates), meanLuminance (floatImage, plates), 0.02 * meanLuminance (floatImage, plates));
}

TEST_F (SceneRendererMetalTests, RadianceFormatChangeRebuildsTheCube)
{
    // One source of truth: when the renderer's radiance format changes, the cube it built is
    // dropped and built again in the new format, never decoded with the other one
    Scene scene;
    scene.getRoot()->addChild (makePlates());
    scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
    scene.setEnvironmentVisible (true);
    scene.setToneMapping (Scene::ToneMapping::aces);

    SceneRenderer renderer;
    const auto floatImage = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (floatImage.isValid()) << renderer.getLastError();
    EXPECT_NEAR (floatImage.getPixelColor (1, 1).getRed(), 206, 1);

    renderer.forceRgbmRadianceForTesting();
    const auto rgbmImage = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (rgbmImage.isValid()) << renderer.getLastError();
    EXPECT_NEAR (rgbmImage.getPixelColor (1, 1).getRed(), 206, 1);
}

TEST_F (SceneRendererMetalTests, OneEnvironmentDrawnByTwoRenderers)
{
    // Each renderer keeps the cube of its own device: drawing the same environment alternately
    // builds nothing again and restarts no convergence, and nothing stays behind once they are gone
    auto otherDevice = GpuDevice::create (GpuPlatform::Metal, {});
    ASSERT_NE (otherDevice, nullptr);

    EnvironmentMap::Ptr environment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));

    Scene scene;
    buildEdgesScene (scene);
    scene.setEnvironment (environment);
    scene.setEnvironmentVisible (true);
    setFullTierTaa (scene, 4);

    {
        SceneRenderer first, second;
        for (int i = 0; i < 4; ++i)
        {
            ASSERT_NE (first.render (device, scene, 64, 64), nullptr) << first.getLastError();
            ASSERT_NE (second.render (otherDevice, scene, 64, 64), nullptr) << second.getLastError();
        }

        EXPECT_TRUE (first.isConverged());
        EXPECT_TRUE (second.isConverged());

        ASSERT_NE (first.render (device, scene, 64, 64), nullptr) << first.getLastError();
        ASSERT_NE (second.render (otherDevice, scene, 64, 64), nullptr) << second.getLastError();
        EXPECT_TRUE (first.isConverged());
        EXPECT_TRUE (second.isConverged());

        // The test, the scene and one slot per renderer
        EXPECT_EQ (environment->getReferenceCount(), 4);

        EXPECT_TRUE (first.waitForGPU().wasOk());
        EXPECT_TRUE (second.waitForGPU().wasOk());
    }

    EXPECT_EQ (environment->getReferenceCount(), 2);
}

TEST_F (SceneRendererMetalTests, EnvironmentChangeDropsTheOldCube)
{
    EnvironmentMap::Ptr first (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
    EnvironmentMap::Ptr second (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.25f, 0.25f, 0.25f); }, 32, 3));

    Scene scene;
    buildEdgesScene (scene);
    scene.setEnvironment (first);

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (first->getReferenceCount(), 3); // The test, the scene and the renderer's slot

    scene.setEnvironment (second);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (first->getReferenceCount(), 1);
    EXPECT_EQ (second->getReferenceCount(), 3);

    scene.setEnvironment (nullptr);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (second->getReferenceCount(), 1);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_F (SceneRendererMetalTests, CapturesBaselineScenes)
{
    captureBaselineScenes ("metal");
}

TEST_F (SceneRendererMetalTests, MatchesPhaseZeroBaselineOnRaster)
{
    checkMatchesPhaseZeroBaseline ("metal");
}

TEST_F (SceneRendererMetalTests, RasterBloomOffStaysOnTheBaseline)
{
    const std::pair<const char*, void (*) (Scene&)> scenes[] = {
        { "shadows", buildShadowsScene },
        { "materials", buildMaterialsScene },
        { "edges", buildEdgesScene },
    };

    for (const auto& [name, build] : scenes)
    {
        Scene scene;
        build (scene);

        // Bloom explicitly off, its other settings changed: the frame stays on the rgba8 inline path
        RenderSettings settings;
        settings.tier = RenderSettings::TierPreference::raster;
        settings.bloom = false;
        settings.bloomThreshold = 0.1f;
        settings.bloomIntensity = 1.0f;
        settings.bloomRadius = 1.0f;
        scene.setRenderSettings (settings);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << name << ": " << renderer.getLastError();

        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        ASSERT_NE (sceneColor, nullptr) << name;
        EXPECT_EQ (sceneColor->getFormat(), GpuTextureFormat::rgba8unorm) << name;
        EXPECT_FALSE (renderer.getCapabilities()->isEnabled (RenderCapabilities::Feature::hdrSceneColor, settings)) << name;

        // The cascades replaced the single map of the Phase 0 shadows
        if (String (name) == "shadows")
        {
            checkShadowsAgainstBaseline (image, "metal");
            continue;
        }

        const auto differing = countBaselineDifferences (image, String ("metal_") + name + ".png");
        ASSERT_GE (differing, 0) << "capture the Phase 0 baselines first";

        RecordProperty ((String (name) + "Differing").toStdString(), differing);
        EXPECT_EQ (differing, 0) << name;
    }
}

TEST_F (SceneRendererMetalTests, ShadowCastingLightAfterEightOthersCasts)
{
    // Eight dark lights come first in the list and the shadow casting sun ninth: the frame must
    // still match the same scene without them, shadow included
    const auto render = [this] (int numLamps)
    {
        Scene scene;
        for (int i = 0; i < numLamps; ++i)
        {
            auto lamp = EntityNode::Ptr (new EntityNode ("lamp"));
            lamp->attach<LightNode> (LightNode::Type::point).intensity = 0.0f;
            scene.getRoot()->addChild (lamp);
        }

        buildShadowsScene (scene);
        setTier (scene, RenderSettings::TierPreference::raster);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 128);
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();
        return image;
    };

    const auto withLamps = render (8);
    const auto withoutLamps = render (0);
    ASSERT_TRUE (withLamps.isValid() && withoutLamps.isValid());

    checkShadows (withLamps);

    const auto differing = countDifferences (withLamps, withoutLamps);
    RecordProperty ("shadowsDiffering", differing);
    EXPECT_EQ (differing, 0);
}

TEST_F (SceneRendererMetalTests, RasterBloomIsUnavailableWhenHalfFloatCannotMultisample)
{
    // Metal multisamples rgba16float at 4: pretend it allows only 2, below the default 4 samples
    const auto halfFloatTakesTwoSamples = [] (RenderCapabilities::DeviceFacts& facts)
    {
        facts.rgba16floatSamples = 2;
    };

    Scene scene;
    buildGlowScene (scene);
    setTier (scene, RenderSettings::TierPreference::raster);

    SceneRenderer withoutBloom;
    withoutBloom.setDeviceFactsOverride (halfFloatTakesTwoSamples);
    const auto reference = renderToImage (withoutBloom, scene, 64);
    ASSERT_TRUE (reference.isValid()) << withoutBloom.getLastError();

    setBloom (scene, true);

    SceneRenderer withBloom;
    withBloom.setDeviceFactsOverride (halfFloatTakesTwoSamples);
    const auto image = renderToImage (withBloom, scene, 64);
    ASSERT_TRUE (image.isValid()) << withBloom.getLastError();

    const auto* capabilities = withBloom.getCapabilities();
    ASSERT_NE (capabilities, nullptr);
    EXPECT_EQ (capabilities->getTier(), RenderCapabilities::Tier::raster);
    EXPECT_FALSE (capabilities->isAvailable (RenderCapabilities::Feature::bloom));
    EXPECT_FALSE (capabilities->isEnabled (RenderCapabilities::Feature::hdrSceneColor, scene.getRenderSettings()));

    // Still the multisampled rgba8 inline frame: not black, not one sample, identical to bloom off
    const auto sceneColor = withBloom.getPassOutput (SceneRenderer::PassOutput::sceneColor);
    ASSERT_NE (sceneColor, nullptr);
    EXPECT_EQ (sceneColor->getFormat(), GpuTextureFormat::rgba8unorm);

    // Two renders of one raster frame can differ by a level here and there
    EXPECT_EQ (countDifferences (image, reference), 0);
    EXPECT_GT (meanLuminance (image, findBrightBounds (image)), 200.0);
}

TEST_F (SceneRendererMetalTests, BloomStaysFiniteOverSaturatedHighlights)
{
    // Exposed radiance far above the half float range: the scene color and the TAA history hold
    // the largest half float, and the glow added over it must not overflow to infinity
    Scene scene;
    buildGlowScene (scene, 1.0e5f);
    setFullTierTaa (scene, 4);

    SceneRenderer withoutBloom;
    const auto off = renderToImage (withoutBloom, scene, 128);
    ASSERT_TRUE (off.isValid()) << withoutBloom.getLastError();

    const auto area = findBrightBounds (off).enlarged (8);
    ASSERT_FALSE (area.isEmpty());

    setBloom (scene, true);

    SceneRenderer withBloom;
    for (int frame = 1; frame <= 4; ++frame)
    {
        const auto image = renderToImage (withBloom, scene, 128);
        ASSERT_TRUE (image.isValid()) << withBloom.getLastError();

        int dark = 0;
        for (int y = area.getY(); y < area.getBottom(); ++y)
            for (int x = area.getX(); x < area.getRight(); ++x)
                dark += image.getPixelColor (x, y).getRed() < 100 ? 1 : 0;

        EXPECT_EQ (dark, 0) << "frame " << frame;
    }
}

TEST_F (SceneRendererMetalTests, RasterBloomRingMatchesTheFullTier)
{
    std::map<RenderSettings::TierPreference, double> rings;

    for (const auto tier : { RenderSettings::TierPreference::raster, RenderSettings::TierPreference::full })
    {
        Scene scene;
        buildGlowScene (scene);
        setTier (scene, tier);

        SceneRenderer withoutBloom;
        const auto off = renderToImage (withoutBloom, scene, 128);
        ASSERT_TRUE (off.isValid()) << withoutBloom.getLastError();

        setBloom (scene, true);

        SceneRenderer withBloom;
        const auto on = renderToImage (withBloom, scene, 128);
        ASSERT_TRUE (on.isValid()) << withBloom.getLastError();

        rings[tier] = ringLuminance (on, findBrightBounds (off));
    }

    const auto raster = rings[RenderSettings::TierPreference::raster];
    const auto full = rings[RenderSettings::TierPreference::full];
    RecordProperty ("rasterRing", String (raster, 2).toStdString());
    RecordProperty ("fullRing", String (full, 2).toStdString());

    EXPECT_GT (full, 5.0);
    EXPECT_NEAR (raster, full, full * 0.1);
}

TEST_F (SceneRendererMetalTests, RasterRendererTogglesBloomOnOffOn)
{
    Scene scene;
    buildGlowScene (scene);
    setTier (scene, RenderSettings::TierPreference::raster);

    SceneRenderer reference;
    const auto inline8 = renderToImage (reference, scene, 128);
    ASSERT_TRUE (inline8.isValid()) << reference.getLastError();
    const auto quad = findBrightBounds (inline8);

    SceneRenderer renderer;
    for (const auto bloom : { true, false, true })
    {
        setBloom (scene, bloom);

        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        // Bloom renders the reversible half-float scene color, without it the rgba8 inline frame
        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        ASSERT_NE (sceneColor, nullptr);
        EXPECT_EQ (sceneColor->getFormat(), bloom ? GpuTextureFormat::rgba16float : GpuTextureFormat::rgba8unorm) << "bloom " << bloom;
        EXPECT_EQ (renderer.getCapabilities()->isEnabled (RenderCapabilities::Feature::hdrSceneColor, scene.getRenderSettings()), bloom);

        const auto ring = ringLuminance (image, quad);
        RecordProperty ((String ("ring") + (bloom ? "On" : "Off")).toStdString(), String (ring, 2).toStdString());

        if (bloom)
            EXPECT_GE (ring, 5.0);
        else
            EXPECT_EQ (countDifferences (image, inline8), 0);

        EXPECT_GE (meanLuminance (image, quad.reduced (2)), 245.0) << "bloom " << bloom;
    }
}

TEST_F (SceneRendererMetalTests, BloomChangesKeepTheTaaHistory)
{
    Scene scene;
    buildGlowScene (scene);
    setFullTierTaa (scene, 4);
    setBloom (scene, true);

    SceneRenderer renderer;
    Image converged;
    for (int i = 0; i < 4; ++i)
        converged = renderToImage (renderer, scene, 128);

    ASSERT_TRUE (converged.isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    // Bloom runs after temporal anti-aliasing, so its settings apply to the kept history
    auto settings = scene.getRenderSettings();
    settings.bloomIntensity = 0.5f;
    scene.setRenderSettings (settings);

    const auto image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());

    const auto quad = findBrightBounds (converged);
    const auto before = ringLuminance (converged, quad);
    const auto after = ringLuminance (image, quad);
    RecordProperty ("ringBefore", String (before, 2).toStdString());
    RecordProperty ("ringAfter", String (after, 2).toStdString());
    EXPECT_GT (after, before + 5.0);
}

TEST_F (SceneRendererMetalTests, RendersWithoutMultisamplingIntoTheReturnedTexture)
{
    // An emissive quad above and left of the center makes the frame asymmetric, so a flip shows
    const auto buildScene = [] (Scene& scene)
    {
        buildShadowsScene (scene);

        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -0.7f, 0.3f, 0.6f };
        vertices[1].position = { -0.3f, 0.3f, 0.6f };
        vertices[2].position = { -0.3f, 0.7f, 0.6f };
        vertices[3].position = { -0.7f, 0.7f, 0.6f };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto emissive = Material::Ptr (new Material());
        emissive->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        emissive->emissiveFactor = { 1.0f, 0.5f, 0.1f };

        auto mesh = Mesh::Ptr (new Mesh ("marker"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, emissive);

        auto marker = EntityNode::Ptr (new EntityNode ("marker"));
        marker->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (marker);
    };

    // Orange marker pixels in the top-left and bottom-left quadrants
    const auto countMarker = [] (const Image& image, bool top)
    {
        int count = 0;
        for (int y = top ? 0 : 32; y < (top ? 32 : 64); ++y)
        {
            for (int x = 0; x < 32; ++x)
            {
                const auto color = image.getPixelColor (x, y);
                if (static_cast<int> (color.getRed()) - static_cast<int> (color.getBlue()) > 80)
                    ++count;
            }
        }

        return count;
    };

    // The sample count applies to the raster tier only
    Scene scene;
    buildScene (scene);
    setTier (scene, RenderSettings::TierPreference::raster);

    SceneRenderer single;
    single.setSampleCount (1);
    const auto image = renderToImage (single, scene, 64);
    ASSERT_TRUE (single.getLastError().isEmpty()) << single.getLastError();
    ASSERT_TRUE (image.isValid());

    SceneRenderer multisampled;
    const auto reference = renderToImage (multisampled, scene, 64);
    ASSERT_TRUE (reference.isValid());

    // A corner shows the environment: 0.5 through ACES is 0.616, 206 once sRGB encoded
    EXPECT_NEAR (image.getPixelColor (1, 1).getRed(), 206, 3);

    // The marker lands at the top on both paths, never at the bottom
    EXPECT_GT (countMarker (reference, true), 4);
    EXPECT_EQ (countMarker (reference, false), 0);
    EXPECT_GT (countMarker (image, true), 4);
    EXPECT_EQ (countMarker (image, false), 0);
}

TEST_F (SceneRendererMetalTests, FullTierMatchesRasterExposureAndToneMapping)
{
    std::map<RenderSettings::TierPreference, double> luminance;

    for (const auto tier : { RenderSettings::TierPreference::raster, RenderSettings::TierPreference::full })
    {
        // Both tiers shadow alike: the raster cascades, no soft shadows, the full frame converged
        Scene scene;
        buildShadowsScene (scene);
        setTier (scene, tier);
        pinCrossTierShadows (scene);

        SceneRenderer renderer;
        const auto image = renderConverged (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        const auto* capabilities = renderer.getCapabilities();
        ASSERT_NE (capabilities, nullptr);
        ASSERT_EQ (capabilities->getTier() == RenderCapabilities::Tier::full, tier == RenderSettings::TierPreference::full);

        // The full frame really went through the HDR scene color, not a raster fallback
        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        ASSERT_NE (sceneColor, nullptr);
        EXPECT_EQ (sceneColor->getFormat(), tier == RenderSettings::TierPreference::full ? GpuTextureFormat::rgba16float : GpuTextureFormat::rgba8unorm);

        // A corner shows the environment: 0.5 through ACES is 0.616, 206 once sRGB encoded
        const auto corner = image.getPixelColor (1, 1);
        EXPECT_NEAR (corner.getRed(), 206, 3);
        EXPECT_NEAR (corner.getGreen(), 206, 3);
        EXPECT_NEAR (corner.getBlue(), 206, 3);
        RecordProperty (tier == RenderSettings::TierPreference::full ? "fullCorner" : "rasterCorner", static_cast<int> (corner.getRed()));

        luminance[tier] = meanLuminance (image, { 32, 32, 64, 64 });
    }

    const auto raster = luminance[RenderSettings::TierPreference::raster];
    const auto full = luminance[RenderSettings::TierPreference::full];
    RecordProperty ("rasterLuminance", String (raster, 3).toStdString());
    RecordProperty ("fullLuminance", String (full, 3).toStdString());

    // Same shading, different antialiasing
    EXPECT_NEAR (full, raster, raster * 0.02);
}

TEST_F (SceneRendererMetalTests, FullTierBackgroundColorMatchesRaster)
{
    // No environment: the background color shows around the plates, display-referred on both tiers
    const auto background = Color (static_cast<uint8> (200), static_cast<uint8> (80), static_cast<uint8> (40));

    for (const auto tier : { RenderSettings::TierPreference::raster, RenderSettings::TierPreference::full })
    {
        const auto tierName = tier == RenderSettings::TierPreference::full ? String ("full") : String ("raster");

        Scene scene;
        scene.getRoot()->addChild (makePlates());
        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>();
        scene.getRoot()->addChild (sun);
        scene.setBackgroundColor (background);
        setTier (scene, tier);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << tierName << ": " << renderer.getLastError();
        ASSERT_EQ (renderer.getCapabilities()->getTier() == RenderCapabilities::Tier::full, tier == RenderSettings::TierPreference::full);

        const auto corner = image.getPixelColor (1, 1);
        RecordProperty ((tierName + "Corner").toStdString(), (String (static_cast<int> (corner.getRed())) + "," + String (static_cast<int> (corner.getGreen())) + "," + String (static_cast<int> (corner.getBlue()))).toStdString());
        EXPECT_NEAR (corner.getRed(), background.getRed(), 1) << tierName;
        EXPECT_NEAR (corner.getGreen(), background.getGreen(), 1) << tierName;
        EXPECT_NEAR (corner.getBlue(), background.getBlue(), 1) << tierName;

        // The lit white plate covers the center
        const auto center = image.getPixelColor (32, 32);
        EXPECT_GT (std::abs (static_cast<int> (center.getBlue()) - static_cast<int> (background.getBlue())), 40) << tierName;
    }
}

TEST_F (SceneRendererMetalTests, FullTierBackgroundColorSurvivesTaaConvergence)
{
    // Temporal anti-aliasing resolves the coverage alpha too, so the background still shows exactly
    const auto background = Color (static_cast<uint8> (200), static_cast<uint8> (80), static_cast<uint8> (40));

    Scene scene;
    buildEdgesScene (scene);
    scene.setBackgroundColor (background);
    setFullTierTaa (scene, 4);

    SceneRenderer renderer;
    for (int frame = 1; frame <= 6; ++frame)
    {
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        const auto corner = image.getPixelColor (1, 1);
        EXPECT_NEAR (corner.getRed(), background.getRed(), 1) << "frame " << frame;
        EXPECT_NEAR (corner.getGreen(), background.getGreen(), 1) << "frame " << frame;
        EXPECT_NEAR (corner.getBlue(), background.getBlue(), 1) << "frame " << frame;
    }

    EXPECT_TRUE (renderer.isConverged());
}

TEST_F (SceneRendererMetalTests, RasterIsAlwaysConverged)
{
    Scene scene;
    buildEdgesScene (scene);
    setTier (scene, RenderSettings::TierPreference::raster);

    SceneRenderer renderer;
    EXPECT_TRUE (renderer.isConverged());

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());

    // Changing the scene changes nothing on raster
    scene.setExposure (2.0f);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());
}

TEST_F (SceneRendererMetalTests, ConvergesThenRestartsOnChange)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 4);

    auto material = Material::Ptr (new Material());
    auto* plates = scene.getRoot()->findChild ("plates");
    auto* sun = scene.getRoot()->findChild ("sun");
    ASSERT_NE (plates, nullptr);
    ASSERT_NE (sun, nullptr);
    plates->attach<MaterialNode> (material);

    auto camera = EntityNode::Ptr (new EntityNode ("camera"));
    camera->attach<CameraNode>();
    camera->setPosition ({ 0.0f, 0.0f, 5.0f });
    scene.getRoot()->addChild (camera);
    scene.setActiveCamera (camera.get());

    SceneRenderer renderer;
    const auto expectConvergesAfterFourRenders = [&] (const char* step)
    {
        for (int i = 1; i <= 4; ++i)
        {
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << step << ": " << renderer.getLastError();
            EXPECT_EQ (renderer.isConverged(), i == 4) << step << ", render " << i;
        }

        // Nothing changed: still converged
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << step << ": " << renderer.getLastError();
        EXPECT_TRUE (renderer.isConverged()) << step;
    };

    expectConvergesAfterFourRenders ("first");

    scene.setExposure (2.0f);
    expectConvergesAfterFourRenders ("exposure");

    material->roughnessFactor = 0.5f;
    expectConvergesAfterFourRenders ("material");

    sun->setRotation (Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, 0.4f));
    expectConvergesAfterFourRenders ("light");

    camera->setPosition ({ 0.1f, 0.0f, 5.0f });
    expectConvergesAfterFourRenders ("camera");
}

TEST_F (SceneRendererMetalTests, GpuFailureOfOneRendererDoesNotFailAnother)
{
    Scene scene;
    buildEdgesScene (scene);

    SceneRenderer first, second;
    ASSERT_NE (first.render (device, scene, 32, 32), nullptr) << first.getLastError();
    ASSERT_NE (second.render (device, scene, 32, 32), nullptr) << second.getLastError();
    ASSERT_TRUE (first.waitForGPU().wasOk());
    ASSERT_TRUE (second.waitForGPU().wasOk());

    // Only the first renderer's next frame fails
    device->failNextFrameForTesting ("injected failure");
    EXPECT_NE (first.render (device, scene, 32, 32), nullptr) << first.getLastError();
    EXPECT_NE (second.render (device, scene, 32, 32), nullptr) << second.getLastError();
    EXPECT_TRUE (second.waitForGPU().wasOk());
    EXPECT_NE (second.render (device, scene, 32, 32), nullptr) << second.getLastError();

    EXPECT_TRUE (second.getLastGpuFailure().isEmpty());

    // The failure is found by the next render of its own renderer, which still renders, and by nobody else
    EXPECT_NE (first.render (device, scene, 32, 32), nullptr) << first.getLastError();
    EXPECT_EQ (first.getLastGpuFailure(), "injected failure");
    EXPECT_TRUE (device->takeGpuFailure().wasOk());

    EXPECT_NE (first.render (device, scene, 32, 32), nullptr) << first.getLastError();
    EXPECT_TRUE (first.getLastGpuFailure().isEmpty());
    EXPECT_TRUE (first.waitForGPU().wasOk());
}

TEST_F (SceneRendererMetalTests, GpuFailureStillRendersTheNextFrame)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 4);

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    device->failNextFrameForTesting ("injected failure");
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    // Frames complete in order, so once a later frame did the failed one has too
    {
        auto fence = GpuFrame::begin (device);
        EXPECT_TRUE (fence.waitForGPU().wasOk());
    }

    auto texture = renderer.render (device, scene, 64, 64);
    ASSERT_NE (texture, nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getLastGpuFailure(), "injected failure");
    EXPECT_TRUE (renderer.getLastError().isEmpty());

    // The history starts over, so the renderer keeps refining (and a component repainting)
    EXPECT_FALSE (renderer.isConverged());
    EXPECT_TRUE (renderer.waitForGPU().wasOk());

    const auto image = readBack (texture);
    ASSERT_TRUE (image.isValid());

    bool drewSomething = false;
    for (int y = 0; y < 64 && ! drewSomething; y += 2)
        for (int x = 0; x < 64 && ! drewSomething; x += 2)
            drewSomething = image.getPixelColor (x, y) != scene.getBackgroundColor();

    EXPECT_TRUE (drewSomething);
}

TEST_F (SceneRendererMetalTests, WaitForGpuReportsTheRenderersFailureOnce)
{
    Scene scene;
    buildEdgesScene (scene);

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 32, 32), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.waitForGPU().wasOk());

    device->failNextFrameForTesting ("injected failure");
    EXPECT_NE (renderer.render (device, scene, 32, 32), nullptr);

    EXPECT_EQ (renderer.waitForGPU().getErrorMessage(), "injected failure");
    EXPECT_NE (renderer.render (device, scene, 32, 32), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.getLastGpuFailure().isEmpty());
    EXPECT_TRUE (device->takeGpuFailure().wasOk());
}

TEST_F (SceneRendererMetalTests, ConvergedFrameStopsChanging)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 4);

    SceneRenderer renderer;
    Image converged;
    for (int i = 0; i < 4; ++i)
        converged = renderToImage (renderer, scene, 64);

    ASSERT_TRUE (converged.isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    const auto countChanged = [] (const Image& a, const Image& b)
    {
        int differing = 0;
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x)
                differing += a.getPixelColor (x, y) != b.getPixelColor (x, y) ? 1 : 0;

        return differing;
    };

    // Further frames keep the history as it is, whatever the jitter. Each frame is checked against
    // the converged one and the one before, so a failure tells a restart from a one-frame glitch
    auto previous = converged;
    for (int i = 0; i < 3; ++i)
    {
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        EXPECT_TRUE (renderer.isConverged()) << "frame " << i << ": the history restarted";
        EXPECT_EQ (countChanged (image, converged), 0) << "frame " << i << ": pixels differing from the converged frame";
        EXPECT_EQ (countChanged (image, previous), 0) << "frame " << i << ": pixels differing from the previous frame";

        previous = image;
    }
}

TEST_F (SceneRendererMetalTests, SignedZeroAndNanPayloadsAreNoChange)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 2);

    // No normal texture: the normal scale is never read, so a NaN there renders nothing wrong
    auto material = Material::Ptr (new Material());
    material->metallicFactor = 0.0f;
    material->normalScale = std::bit_cast<float> (0x7fc00000u);
    scene.getRoot()->findChild ("plates")->attach<MaterialNode> (material);

    SceneRenderer renderer;
    for (int i = 0; i < 2; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    material->metallicFactor = -0.0f;
    material->normalScale = std::bit_cast<float> (0xffc00001u);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());
}

TEST_F (SceneRendererMetalTests, SampleCountChangesKeepTheFullTierConverged)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 4);

    // The full tier renders one sample per pixel whatever the count, so its accumulation is kept,
    // and so is the texture it renders into
    SceneRenderer renderer;
    GpuTexture::Ptr output;
    for (int i = 0; i < 4; ++i)
        ASSERT_NE (output = renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    renderer.setSampleCount (1);
    EXPECT_EQ (renderer.render (device, scene, 64, 64), output) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());

    // The raster tier recreates its multisampled targets for the new count
    setTier (scene, RenderSettings::TierPreference::raster);
    renderer.setSampleCount (4);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    renderer.setSampleCount (1);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    renderer.setSampleCount (4);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
}

TEST_F (SceneRendererMetalTests, TaaSmoothsEdges)
{
    const auto edgeLevels = [this] (int taaSamples)
    {
        Scene scene;
        buildEdgesScene (scene);
        setFullTierTaa (scene, taaSamples);

        SceneRenderer renderer;
        Image image;
        for (int i = 0; i < jmax (1, taaSamples); ++i)
            image = renderToImage (renderer, scene, 128);

        EXPECT_TRUE (renderer.isConverged()) << taaSamples;
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();
        return countEdgeLevels (image);
    };

    const auto antialiased = edgeLevels (16);
    const auto aliased = edgeLevels (1);
    RecordProperty ("antialiasedLevels", static_cast<int> (antialiased));
    RecordProperty ("aliasedLevels", static_cast<int> (aliased));

    EXPECT_GE (antialiased, 4u);
    EXPECT_LE (aliased, 2u);
}

TEST_F (SceneRendererMetalTests, MovingObjectKeepsStillPartsAntialiased)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 16);

    // A small quad spinning far below row 40, whose transform changes on every frame
    std::vector<Mesh::Vertex> vertices (4);
    vertices[0].position = { -0.15f, -0.15f, 0.0f };
    vertices[1].position = { 0.15f, -0.15f, 0.0f };
    vertices[2].position = { 0.15f, 0.15f, 0.0f };
    vertices[3].position = { -0.15f, 0.15f, 0.0f };
    for (auto& vertex : vertices)
        vertex.normal = { 0.0f, 0.0f, 1.0f };

    auto mesh = Mesh::Ptr (new Mesh ("spinner"));
    mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 });
    auto spinner = EntityNode::Ptr (new EntityNode ("spinner"));
    spinner->attach<MeshNode> (mesh);
    spinner->setPosition ({ -1.6f, -1.6f, 0.0f });
    scene.getRoot()->addChild (spinner);

    SceneRenderer renderer;
    Image image;
    for (int i = 0; i < 24; ++i)
    {
        spinner->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, 0.2f * static_cast<float> (i)));
        image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    }

    // Moving things never converge, but the still plate keeps accumulating
    EXPECT_FALSE (renderer.isConverged());

    const auto levels = countEdgeLevels (image);
    RecordProperty ("levelsWithMovingObject", static_cast<int> (levels));
    EXPECT_GE (levels, 4u);
}

TEST_F (SceneRendererMetalTests, RedrawnTextureRestartsAndShows)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 4);

    const auto upload = [] (const GpuTexture::Ptr& texture, uint8 r, uint8 g, uint8 b)
    {
        std::vector<uint8> texels;
        for (int i = 0; i < 4; ++i)
            texels.insert (texels.end(), { r, g, b, 255 });

        GpuTextureDataDesc data;
        data.data = texels.data();
        return texture->upload (data);
    };

    auto gpuTexture = GpuTexture::create (device, GpuTextureDesc (2, 2, GpuTextureFormat::rgba8unorm));
    ASSERT_NE (gpuTexture, nullptr);
    ASSERT_TRUE (upload (gpuTexture, 255, 255, 255));

    auto texture = Texture::Ptr (new Texture (gpuTexture));
    auto material = Material::Ptr (new Material());
    material->baseColorTexture = texture;
    scene.getRoot()->findChild ("plates")->attach<MaterialNode> (material);

    SceneRenderer renderer;
    Image image;
    for (int i = 0; i < 4; ++i)
        image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (renderer.isConverged()) << renderer.getLastError();
    EXPECT_NEAR (image.getPixelColor (32, 32).getRed(), image.getPixelColor (32, 32).getGreen(), 4);

    // Redrawn in place: the same GpuTexture handed over again
    ASSERT_TRUE (upload (gpuTexture, 255, 0, 0));
    texture->setGpuTexture (gpuTexture);

    image = renderToImage (renderer, scene, 64);
    EXPECT_FALSE (renderer.isConverged());

    for (int i = 0; i < 4 && ! renderer.isConverged(); ++i)
        image = renderToImage (renderer, scene, 64);

    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());
    EXPECT_GT (image.getPixelColor (32, 32).getRed(), image.getPixelColor (32, 32).getGreen() + 60);
}

TEST_F (SceneRendererMetalTests, SwappingTransformsOfTwoMeshesIsAChange)
{
    Scene scene;
    setFullTierTaa (scene, 4);

    // Two emissive quads, orange on the left and blue on the right
    const auto addQuad = [&] (const char* name, std::array<float, 3> emissive, float x)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -0.4f, -0.4f, 0.0f };
        vertices[1].position = { 0.4f, -0.4f, 0.0f };
        vertices[2].position = { 0.4f, 0.4f, 0.0f };
        vertices[3].position = { -0.4f, 0.4f, 0.0f };
        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        material->emissiveFactor = emissive;

        auto mesh = Mesh::Ptr (new Mesh (name));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);
        auto entity = EntityNode::Ptr (new EntityNode (name));
        entity->attach<MeshNode> (mesh);
        entity->setPosition ({ x, 0.0f, 0.0f });
        scene.getRoot()->addChild (entity);
        return entity;
    };

    auto orange = addQuad ("orange", { 1.0f, 0.5f, 0.1f }, -0.6f);
    auto blue = addQuad ("blue", { 0.1f, 0.2f, 1.0f }, 0.6f);

    SceneRenderer renderer;
    Image image;
    for (int i = 0; i < 4; ++i)
        image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (renderer.isConverged()) << renderer.getLastError();

    const auto isBlue = [] (Color color) { return color.getBlue() > color.getRed() + 60; };
    ASSERT_FALSE (isBlue (image.getPixelColor (20, 32)));
    ASSERT_TRUE (isBlue (image.getPixelColor (44, 32)));

    // The same transforms, the same meshes and materials, but paired the other way round
    orange->setPosition ({ 0.6f, 0.0f, 0.0f });
    blue->setPosition ({ -0.6f, 0.0f, 0.0f });

    image = renderToImage (renderer, scene, 64);
    EXPECT_FALSE (renderer.isConverged());

    for (int i = 0; i < 4 && ! renderer.isConverged(); ++i)
        image = renderToImage (renderer, scene, 64);

    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_TRUE (isBlue (image.getPixelColor (20, 32)));
    EXPECT_FALSE (isBlue (image.getPixelColor (44, 32)));
}

TEST_F (SceneRendererMetalTests, RestartAccumulationStartsOver)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 4);

    SceneRenderer renderer;
    renderer.restartAccumulation(); // Nothing rendered yet: nothing to restart

    for (int i = 0; i < 4; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    renderer.restartAccumulation();
    EXPECT_FALSE (renderer.isConverged());

    for (int i = 1; i <= 4; ++i)
    {
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_EQ (renderer.isConverged(), i == 4) << "render " << i;
    }
}

TEST_F (SceneRendererMetalTests, LightRadiusChangeRestartsTheFullTier)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 4);

    auto* light = scene.getRoot()->findChild ("sun")->getNode<LightNode>();
    ASSERT_NE (light, nullptr);

    SceneRenderer renderer;
    for (int i = 0; i < 4; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    light->radius = 0.05f;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());
}

TEST_F (SceneRendererMetalTests, RebuiltEnvironmentRestartsTheFullTier)
{
    // The environment cube is built in a frame of its own, before the first frame. When the GPU fails
    // to build it, the first frame shades with an undefined cube and the next one rebuilds it: the
    // samples taken with the undefined cube must leave the running mean
    Scene scene;
    buildEdgesScene (scene);
    scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
    setFullTierTaa (scene, 4);

    SceneRenderer renderer;
    device->failNextFrameForTesting ("injected failure");
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    // Frames complete in order, so once a later frame did the build has too
    {
        auto fence = GpuFrame::begin (device);
        EXPECT_TRUE (fence.waitForGPU().wasOk());
    }

    for (int i = 0; i < 3; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_FALSE (renderer.isConverged());

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());
}

TEST_F (SceneRendererMetalTests, RepeatedEnvironmentFailureConvergesWithoutIt)
{
    // Two failed builds in a row leave the renderer without the environment: the background shows
    // instead of it, and the full tier converges instead of restarting every frame
    const auto background = Color (static_cast<uint8> (40), static_cast<uint8> (70), static_cast<uint8> (110));

    Scene scene;
    buildEdgesScene (scene);
    scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
    scene.setEnvironmentVisible (true);
    scene.setToneMapping (Scene::ToneMapping::aces);
    scene.setBackgroundColor (background);
    setFullTierTaa (scene, 4);

    SceneRenderer renderer;
    for (int i = 0; i < 2; ++i)
    {
        device->failNextFrameForTesting ("injected failure");
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

        auto fence = GpuFrame::begin (device);
        EXPECT_TRUE (fence.waitForGPU().wasOk());
    }

    for (int i = 0; i < 4; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_TRUE (renderer.isConverged());

    const auto image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (image.isValid());
    EXPECT_TRUE (renderer.isConverged());

    // The background color, not the environment's 206
    const auto corner = image.getPixelColor (1, 1);
    EXPECT_NEAR (corner.getRed(), background.getRed(), 2);
    EXPECT_NEAR (corner.getGreen(), background.getGreen(), 2);
    EXPECT_NEAR (corner.getBlue(), background.getBlue(), 2);
}

TEST_F (SceneRendererMetalTests, AreaLightVariantSwitchRestartsAndConvergesAgain)
{
    // A radius switches the shading passes to their area light variant mid-session, and back when
    // it goes: like any light change it restarts the convergence over the kept history, and no
    // frame fails
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 4);

    auto* light = scene.getRoot()->findChild ("sun")->getNode<LightNode>();
    ASSERT_NE (light, nullptr);

    SceneRenderer renderer;
    for (const auto radius : { 0.0f, 0.05f, 0.0f })
    {
        light->radius = radius;

        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_FALSE (renderer.isConverged()) << radius;

        for (int i = 0; i < 4; ++i)
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

        EXPECT_TRUE (renderer.isConverged()) << radius;
    }

    const auto gpuResult = renderer.waitForGPU();
    EXPECT_TRUE (gpuResult.wasOk()) << gpuResult.getErrorMessage();
}

TEST_F (SceneRendererMetalTests, KhrMaterialChangesRestartAccumulation)
{
    const std::pair<const char*, std::function<void (Material&)>> changes[] = {
        { "emissiveStrength", [] (Material& material) { material.emissiveStrength = 2.0f; } },
        { "ior", [] (Material& material) { material.ior = 1.33f; } },
        { "specularFactor", [] (Material& material) { material.specularFactor = 0.5f; } },
        { "specularColorFactor", [] (Material& material) { material.specularColorFactor[1] = 0.5f; } },
    };

    for (const auto& [name, change] : changes)
    {
        auto material = Material::Ptr (new Material());
        material->metallicFactor = 0.0f;
        material->emissiveFactor = { 0.1f, 0.1f, 0.1f };

        Scene scene;
        addPlate (scene, material);
        setFullTierTaa (scene, 4);

        SceneRenderer renderer;
        for (int i = 0; i < 4; ++i)
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        ASSERT_TRUE (renderer.isConverged()) << name;

        change (*material);
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_FALSE (renderer.isConverged()) << name;
    }
}

TEST_F (SceneRendererMetalTests, ShadowSettingsRestartAccumulation)
{
    // Each change moves the value the full tier resolves away from its default
    const std::pair<const char*, std::function<void (RenderSettings&)>> changes[] = {
        { "sunCascades", [] (RenderSettings& settings) { settings.sunCascades = 2; } },
        { "shadowResolution", [] (RenderSettings& settings) { settings.shadowResolution = 1024; } },
        { "softShadows", [] (RenderSettings& settings) { settings.softShadows = false; } },
        { "contactShadows", [] (RenderSettings& settings) { settings.contactShadows = false; } },
    };

    for (const auto& [name, change] : changes)
    {
        Scene scene;
        buildEdgesScene (scene);
        setFullTierTaa (scene, 8);

        SceneRenderer renderer;
        for (int i = 0; i < 8; ++i)
            ASSERT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
        ASSERT_TRUE (renderer.isConverged()) << name;

        auto settings = scene.getRenderSettings();
        change (settings);
        scene.setRenderSettings (settings);

        // Restarted, not invalidated: the first frame blends into the kept history, so its
        // silhouettes still have partially covered pixels. A discarded history leaves none
        const auto first = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (first.isValid()) << renderer.getLastError();
        EXPECT_FALSE (renderer.isConverged()) << name;
        EXPECT_GE (countIntermediatePixels (first, 40), 4) << name;
    }
}

TEST_F (SceneRendererMetalTests, LightShadowFieldsRestartAccumulation)
{
    const std::pair<const char*, std::function<void (LightNode&)>> changes[] = {
        { "shadowBias", [] (LightNode& light) { light.shadowBias = 2.0f; } },
        { "contactShadows", [] (LightNode& light) { light.contactShadows = true; } },
        { "contactShadowDistance", [] (LightNode& light) { light.contactShadowDistance = 0.5f; } },
        { "contactShadowThickness", [] (LightNode& light) { light.contactShadowThickness = 0.1f; } },
    };

    for (const auto& [name, change] : changes)
    {
        Scene scene;
        buildEdgesScene (scene);
        setFullTierTaa (scene, 8);

        auto* light = scene.getRoot()->findChild ("sun")->getNode<LightNode>();
        ASSERT_NE (light, nullptr);

        SceneRenderer renderer;
        for (int i = 0; i < 8; ++i)
            ASSERT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
        ASSERT_TRUE (renderer.isConverged()) << name;

        change (*light);

        // Restarted, not invalidated, see ShadowSettingsRestartAccumulation
        const auto first = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (first.isValid()) << renderer.getLastError();
        EXPECT_FALSE (renderer.isConverged()) << name;
        EXPECT_GE (countIntermediatePixels (first, 40), 4) << name;
    }
}

TEST_F (SceneRendererMetalTests, AmbientOcclusionSettingsRestartAccumulation)
{
    // Each change moves the value the full tier resolves away from its default
    const std::pair<const char*, std::function<void (RenderSettings&)>> changes[] = {
        { "ambientOcclusion", [] (RenderSettings& settings) { settings.ambientOcclusion = false; } },
        { "aoDistance", [] (RenderSettings& settings) { settings.aoDistance = 0.5f; } },
    };

    for (const auto& [name, change] : changes)
    {
        Scene scene;
        buildEdgesScene (scene);
        setFullTierTaa (scene, 8);

        SceneRenderer renderer;
        for (int i = 0; i < 8; ++i)
            ASSERT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
        ASSERT_TRUE (renderer.isConverged()) << name;

        auto settings = scene.getRenderSettings();
        change (settings);
        scene.setRenderSettings (settings);

        // Restarted, not invalidated, see ShadowSettingsRestartAccumulation
        const auto first = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (first.isValid()) << renderer.getLastError();
        EXPECT_FALSE (renderer.isConverged()) << name;
        EXPECT_GE (countIntermediatePixels (first, 40), 4) << name;
    }
}

// The screen-space reflection tests converge 8 jittered samples at 64 x 64: the floor of
// buildOnScreenEmitterScene() is a mirror (roughness 0.02), whose rays barely spread, so the running
// mean of 8 frames holds its reflection, and the first frame, which has no history to read it from
// yet, weighs an eighth of it
TEST_F (SceneRendererMetalTests, SsrReflectsAnOnScreenEmitter)
{
    const auto render = [&] (bool screenSpaceReflections, SceneRenderer& renderer)
    {
        Scene scene;
        buildOnScreenEmitterScene (scene);
        setFullTierTaa (scene, 8);
        setScreenSpaceReflections (scene, screenSpaceReflections);

        const auto image = renderConverged (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        EXPECT_TRUE (renderer.isConverged());
        return image;
    };

    SceneRenderer on, off;
    const auto withReflections = render (true, on);
    const auto without = render (false, off);
    ASSERT_TRUE (withReflections.isValid() && without.isValid());

    const auto region = reflectedEmitterRegion();
    const auto floorOn = meanColor (withReflections, region);
    const auto floorOff = meanColor (without, region);
    RecordProperty ("floorRedOn", static_cast<int> (floorOn.getRed()));
    RecordProperty ("floorRedOff", static_cast<int> (floorOff.getRed()));
    // Measured 191 against 85: the threshold keeps three quarters of that margin
    EXPECT_GT (floorOn.getRed(), floorOff.getRed() + 80);
    EXPECT_NEAR (floorOn.getGreen(), floorOff.getGreen(), 2); // The emitter is red only

    EXPECT_EQ (off.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);
    const auto reflections = on.getPassOutput (SceneRenderer::PassOutput::reflections);
    ASSERT_NE (reflections, nullptr);
    EXPECT_EQ (reflections->getFormat(), GpuTextureFormat::rgba16float);
    EXPECT_EQ (reflections->getWidth(), 64u);

#if YUP_ENABLE_SHADER_TRANSPILER
    EXPECT_TRUE (isFiniteTexture (reflections));
    EXPECT_FALSE (isSaturatedTexture (reflections));

    const auto confidence = mapTexels (reflections, "vec4(t.aaa, 1.0)");
    ASSERT_TRUE (confidence.isValid());
    RecordProperty ("floorConfidence", String (meanLuminance (confidence, region), 1).toStdString());
    EXPECT_GT (meanLuminance (confidence, region), 200.0);
#endif
}

TEST_F (SceneRendererMetalTests, SsrConvergedRendersAreIdentical)
{
    // The rays turn with the frame only under temporal anti-aliasing, whose running mean freezes
    // once converged (preflight R13): two renderers converging on the same scene end on one image
    Scene scene;
    buildOnScreenEmitterScene (scene);
    setFullTierTaa (scene, 8);

    SceneRenderer warmUp;
    ASSERT_TRUE (renderToImage (warmUp, scene, 64).isValid());

    SceneRenderer a, b;
    const auto imageA = renderConverged (a, scene, 64);
    const auto imageB = renderConverged (b, scene, 64);
    ASSERT_TRUE (imageA.isValid() && imageB.isValid());
    EXPECT_TRUE (a.isConverged() && b.isConverged());
    EXPECT_NE (a.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);

    auto differing = 0;
    for (int y = 0; y < imageA.getHeight(); ++y)
        for (int x = 0; x < imageA.getWidth(); ++x)
            differing += imageA.getPixelColor (x, y) != imageB.getPixelColor (x, y) ? 1 : 0;

    EXPECT_EQ (differing, 0);
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererMetalTests, SsrWithoutHistoryShadesAsWithoutIt)
{
    // A renderer's first frame has no history to read reflections from: every pixel takes the
    // confidence 0 branch, which leaves the frame bit for bit as without them. The environment the
    // floor reflects would show any confidence taken from it
    SceneRenderer warmUp;
    {
        Scene scene;
        buildOnScreenEmitterScene (scene);
        ASSERT_TRUE (renderToImage (warmUp, scene, 64).isValid());
    }

    const auto renderBits = [&] (bool screenSpaceReflections)
    {
        Scene scene;
        buildOnScreenEmitterScene (scene);
        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
        setFullTierTaa (scene, 8);
        setScreenSpaceReflections (scene, screenSpaceReflections);

        SceneRenderer renderer;
        EXPECT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);

        // The 8 bytes of the half float scene color, three to an opaque rgba8 image
        std::vector<Image> bits;
        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        for (const auto* bytes : { "vec4(lo.xyz, 1.0)", "vec4(lo.w, hi.xy, 1.0)", "vec4(hi.zw, 0.0, 1.0)" })
        {
            const auto* halves = "#define lo unpackUnorm4x8(packHalf2x16(t.rg))\n#define hi unpackUnorm4x8(packHalf2x16(t.ba))\n";
            bits.push_back (sceneColor != nullptr ? mapTexels (sceneColor, bytes, halves) : Image());
        }

        return bits;
    };

    const auto withReflections = renderBits (true);
    const auto without = renderBits (false);

    for (size_t part = 0; part < withReflections.size(); ++part)
    {
        ASSERT_TRUE (withReflections[part].isValid() && without[part].isValid());

        auto differing = 0;
        for (int y = 0; y < without[part].getHeight(); ++y)
            for (int x = 0; x < without[part].getWidth(); ++x)
                differing += withReflections[part].getPixelColor (x, y) != without[part].getPixelColor (x, y) ? 1 : 0;

        EXPECT_EQ (differing, 0) << part;
    }

    // The stand-in the lighting pass binds without reflections is 0 in every channel, so that branch
    // is the one every pixel takes
    const auto* lightingPass = warmUp.getLightingPass();
    ASSERT_NE (lightingPass, nullptr);
    const auto standIn = lightingPass->getReflectionsStandIn();
    ASSERT_NE (standIn, nullptr);
    const auto nonZero = mapTexels (standIn, "any(notEqual(t, vec4(0.0))) ? vec4(1.0) : vec4(0.0, 0.0, 0.0, 1.0)");
    ASSERT_TRUE (nonZero.isValid());
    EXPECT_EQ (nonZero.getPixelColor (0, 0).getRed(), 0);

    // And that branch kept the floor's reflection of the environment, which a stand-in of
    // confidence 1 would have replaced with its black: the floor is brighter than without one
    const auto floorLuminance = [&] (bool environment)
    {
        Scene scene;
        buildOnScreenEmitterScene (scene);
        setFullTierTaa (scene, 8);

        if (environment)
            scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);
        return image.isValid() ? meanLuminance (image, reflectedEmitterRegion()) : -1.0;
    };

    const auto withEnvironment = floorLuminance (true);
    const auto withoutEnvironment = floorLuminance (false);
    RecordProperty ("floorWithEnvironment", String (withEnvironment, 1).toStdString());
    RecordProperty ("floorWithoutEnvironment", String (withoutEnvironment, 1).toStdString());
    EXPECT_GT (withEnvironment, withoutEnvironment + 20.0);
}

TEST_F (SceneRendererMetalTests, SsrScalesWithTheExposure)
{
    // The history holds exposed radiance, divided by the exposure of the frame it was rendered with:
    // converged at exposure 1 then at 2, the reflection scales like the frame without it does.
    // Dividing by nothing, or by the exposure twice, scales it by 4 or 1
    const auto renderRatio = [&] (bool screenSpaceReflections)
    {
        Scene scene;
        buildOnScreenEmitterScene (scene, 0.3f);
        setFullTierTaa (scene, 8);
        setScreenSpaceReflections (scene, screenSpaceReflections);

        SceneRenderer renderer;
        const auto meanRed = [&]
        {
            EXPECT_TRUE (renderConverged (renderer, scene, 64).isValid()) << renderer.getLastError();
            const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
            return sceneColor != nullptr ? meanLinearRed (sceneColor, reflectedEmitterRegion()) : -1.0;
        };

        const auto atOne = meanRed();
        scene.setExposure (2.0f);
        const auto atTwo = meanRed();
        RecordProperty (screenSpaceReflections ? "floorRedOn" : "floorRedOff", (String (atOne, 4) + " -> " + String (atTwo, 4)).toStdString());
        return atOne > 0.0 ? atTwo / atOne : -1.0;
    };

    const auto ratioOn = renderRatio (true);
    const auto ratioOff = renderRatio (false);
    RecordProperty ("ratioOn", String (ratioOn, 3).toStdString());
    RecordProperty ("ratioOff", String (ratioOff, 3).toStdString());
    EXPECT_GT (ratioOff, 1.5);
    EXPECT_NEAR (ratioOn, ratioOff, 0.02 * ratioOff);
}
#endif

TEST_F (SceneRendererMetalTests, SsrFallsBackToProbesOffScreen)
{
    // buildMirrorFloorScene() hides its emitter above the frustum: no ray finds it on screen, so
    // the floor reflects the probe's capture of it alike with and without screen-space reflections
    const auto render = [&] (bool screenSpaceReflections, SceneRenderer& renderer)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        setFullTierTaa (scene, 8);
        setScreenSpaceReflections (scene, screenSpaceReflections);
        addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);

        const auto image = renderConverged (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    };

    SceneRenderer on, off;
    const auto withReflections = render (true, on);
    const auto without = render (false, off);
    ASSERT_TRUE (withReflections.isValid() && without.isValid());

    const auto floorOn = meanColor (withReflections, floorRegion());
    const auto floorOff = meanColor (without, floorRegion());
    RecordProperty ("floorGreenOn", static_cast<int> (floorOn.getGreen()));
    RecordProperty ("floorGreenOff", static_cast<int> (floorOff.getGreen()));
    EXPECT_GT (floorOn.getGreen(), floorOn.getRed() + 40);
    EXPECT_GT (floorOff.getGreen(), floorOff.getRed() + 40);
    EXPECT_NEAR (floorOn.getRed(), floorOff.getRed(), 2);
    EXPECT_NEAR (floorOn.getGreen(), floorOff.getGreen(), 2);
    EXPECT_NEAR (floorOn.getBlue(), floorOff.getBlue(), 2);

    // The rays ran and missed
    const auto reflections = on.getPassOutput (SceneRenderer::PassOutput::reflections);
    ASSERT_NE (reflections, nullptr);

#if YUP_ENABLE_SHADER_TRANSPILER
    const auto confidence = mapTexels (reflections, "vec4(t.aaa, 1.0)");
    ASSERT_TRUE (confidence.isValid());
    EXPECT_LT (meanLuminance (confidence, floorRegion()), 1.0);
#endif
}

TEST_F (SceneRendererMetalTests, SsrSettingRestartsAccumulation)
{
    Scene scene;
    buildOnScreenEmitterScene (scene);
    setFullTierTaa (scene, 8);

    SceneRenderer renderer;
    for (int i = 0; i < 8; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    // Restarted, not invalidated: the history stays valid for the reflections to come back at once
    setScreenSpaceReflections (scene, false);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);

    setScreenSpaceReflections (scene, std::nullopt);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());
    EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererMetalTests, SsrConvergedRegionMatchesAPrimedHistory)
{
    // The first frame has no history to read reflections from, and enters the running mean without
    // them. Temporal anti-aliasing clips it to the neighborhood of the next frames, which hold the
    // reflection, so the converged reflection keeps no share of it: it matches a renderer whose
    // history was primed by an earlier convergence, then restarted by a setting that changes
    // nothing on screen (contact shadows, without a light)
    const auto renderRed = [&] (bool primed)
    {
        Scene scene;
        buildOnScreenEmitterScene (scene, 0.3f);
        setFullTierTaa (scene, 8);

        SceneRenderer renderer;
        EXPECT_TRUE (renderConverged (renderer, scene, 64).isValid()) << renderer.getLastError();

        if (primed)
        {
            auto settings = scene.getRenderSettings();
            settings.contactShadows = false;
            scene.setRenderSettings (settings);
            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr);
            EXPECT_FALSE (renderer.isConverged());
            EXPECT_TRUE (renderConverged (renderer, scene, 64).isValid()) << renderer.getLastError();
        }

        EXPECT_TRUE (renderer.isConverged());
        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        return sceneColor != nullptr ? meanLinearRed (sceneColor, reflectedEmitterRegion()) : -1.0;
    };

    const auto fresh = renderRed (false);
    const auto primed = renderRed (true);
    RecordProperty ("floorRedFresh", String (fresh, 4).toStdString());
    RecordProperty ("floorRedPrimed", String (primed, 4).toStdString());

    // The reflection is 0.3 of the 0.4 the floor shows: a missing eighth of it would be 0.0375,
    // 9% of the floor. The rest of the frame is the same in both, up to the noise of the rays
    EXPECT_GT (fresh, 0.3);
    EXPECT_NEAR (fresh, primed, 0.01 * primed);
}
#endif

TEST_F (SceneRendererMetalTests, GivingUpSsrRestartsAccumulation)
{
    // Reflections that stop failing twice in a row change the image: a converged renderer starts over
    Scene scene;
    buildOnScreenEmitterScene (scene);
    setFullTierTaa (scene, 8);

    SceneRenderer renderer;
    ASSERT_TRUE (renderConverged (renderer, scene, 64).isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);

    renderer.failScreenSpaceReflectionsForTesting();

    for (const auto givesUp : { false, true })
    {
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);
        ASSERT_NE (renderer.getReflectionPass(), nullptr);
        EXPECT_EQ (renderer.getReflectionPass()->hasGivenUp(), givesUp);
    }

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererMetalTests, SsrDoesNotLeakAcrossDepthEdges)
{
    // A rough bar floats in front of the reflecting floor, across rows 32 to 34 of the reflection.
    // The bar traces no rays, so a floor pixel next to it would count the bar's texels as misses if
    // the resolve did not leave out the texels of another depth and normal: its confidence would
    // fall along the bar's edges. With them left out, the rows along the bar keep the confidence of
    // the rows further from it
    Scene scene;
    buildOnScreenEmitterScene (scene);
    setFullTierTaa (scene, 8);

    std::vector<Mesh::Vertex> vertices (4);
    vertices[0].position = { -1.0f, 0.2f, 0.5f };
    vertices[1].position = { 1.0f, 0.2f, 0.5f };
    vertices[2].position = { 1.0f, 0.3f, 0.5f };
    vertices[3].position = { -1.0f, 0.3f, 0.5f };
    for (auto& vertex : vertices)
        vertex.normal = { 0.0f, 0.0f, 1.0f };

    auto rough = Material::Ptr (new Material());
    rough->baseColorFactor = { 0.5f, 0.5f, 0.5f, 1.0f };
    rough->metallicFactor = 0.0f;
    rough->roughnessFactor = 1.0f;

    auto mesh = Mesh::Ptr (new Mesh ("bar"));
    mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, rough);
    auto bar = EntityNode::Ptr (new EntityNode ("bar"));
    bar->attach<MeshNode> (mesh);
    scene.getRoot()->addChild (bar);

    SceneRenderer renderer;
    ASSERT_TRUE (renderConverged (renderer, scene, 64).isValid()) << renderer.getLastError();
    const auto reflections = renderer.getPassOutput (SceneRenderer::PassOutput::reflections);
    ASSERT_NE (reflections, nullptr);

    const auto confidence = mapTexels (reflections, "vec4(t.aaa, 1.0)");
    ASSERT_TRUE (confidence.isValid());

    const auto edges = (meanLuminance (confidence, { 24, 31, 16, 1 }) + meanLuminance (confidence, { 24, 35, 16, 1 })) * 0.5;
    const auto inside = (meanLuminance (confidence, { 24, 28, 16, 2 }) + meanLuminance (confidence, { 24, 37, 16, 1 })) / 2.0;
    RecordProperty ("edgeConfidence", String (edges, 1).toStdString());
    RecordProperty ("insideConfidence", String (inside, 1).toStdString());
    EXPECT_GT (inside, 200.0);
    EXPECT_GT (edges, 0.9 * inside);
}

TEST_F (SceneRendererMetalTests, SsrWeighsNeighborRaysByThePixelsLobe)
{
    // A mirror floor under an emitter of 4 red and green bands. The rays are traced at half
    // resolution, so a pixel reuses the rays of its neighbors, which hit the bands a pixel or so
    // away from its own reflection: weighed by its lobe over the rays' pdf, the nearest of them
    // dominates and the reflected bands stay as distinct as the emitter's own, where a plain mean of
    // the 4 rays would blend each band into the next
    Scene scene;
    buildMirrorFloorScene (scene, false);
    setFullTierTaa (scene, 8);

    for (int band = 0; band < 4; ++band)
    {
        std::vector<Mesh::Vertex> vertices (4);
        const auto bottom = 0.2f * static_cast<float> (band);
        vertices[0].position = { -0.6f, bottom, -0.5f };
        vertices[1].position = { 0.6f, bottom, -0.5f };
        vertices[2].position = { 0.6f, bottom + 0.2f, -0.5f };
        vertices[3].position = { -0.6f, bottom + 0.2f, -0.5f };
        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        const auto red = band % 2 == 0;
        auto mesh = Mesh::Ptr (new Mesh ("band"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, makeEmitterMaterial ({ red ? 1.0f : 0.0f, red ? 0.0f : 1.0f, 0.0f }));
        auto entity = EntityNode::Ptr (new EntityNode ("band"));
        entity->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (entity);
    }

    SceneRenderer renderer;
    const auto image = renderConverged (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();

    // The mean red-green difference over columns 24 to 39 of rows from first to last
    const auto contrast = [&] (int first, int last)
    {
        auto sum = 0.0;
        for (int y = first; y <= last; ++y)
            for (int x = 24; x < 40; ++x)
                sum += std::abs (static_cast<int> (image.getPixelColor (x, y).getRed()) - static_cast<int> (image.getPixelColor (x, y).getGreen()));

        return sum / (16.0 * (last - first + 1));
    };

    const auto emitter = contrast (10, 24);
    const auto reflected = contrast (28, 38);
    RecordProperty ("emitterContrast", String (emitter, 1).toStdString());
    RecordProperty ("reflectedContrast", String (reflected, 1).toStdString());

    // Measured 56.7 of the emitter's 166.2 (0.34), and 49.0 (0.29) with a plain mean of the rays
    EXPECT_GT (reflected, 0.32 * emitter);
}
#endif

TEST_F (SceneRendererMetalTests, SsrOffReleasesItsTargets)
{
    Scene scene;
    buildOnScreenEmitterScene (scene);
    setFullTierTaa (scene, 8);

    SceneRenderer renderer;
    EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getReflectionPass(), nullptr); // Nothing to read on the first frame, nothing created

    EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    const auto* pass = renderer.getReflectionPass();
    ASSERT_NE (pass, nullptr);
    EXPECT_GT (pass->getNumCachedTargets(), 0u);

    const auto* hiZ = renderer.getHiZPyramid();
    ASSERT_NE (hiZ, nullptr);
    EXPECT_GT (hiZ->getNumCachedTargets(), 0u);

    // Kept, with no target, so turning them back on compiles nothing
    setScreenSpaceReflections (scene, false);
    EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getReflectionPass(), pass);
    EXPECT_EQ (pass->getNumCachedTargets(), 0u);
    EXPECT_EQ (hiZ->getNumCachedTargets(), 0u);

    // Back on, then a resize: the new size has no history yet, and keeps no target of the old one
    setScreenSpaceReflections (scene, std::nullopt);
    EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_GT (hiZ->getNumCachedTargets(), 0u);

    EXPECT_NE (renderer.render (device, scene, 48, 48), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getHiZPyramid(), hiZ);
    EXPECT_EQ (pass->getNumCachedTargets(), 0u);
    EXPECT_EQ (hiZ->getNumCachedTargets(), 0u);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_F (SceneRendererMetalTests, FailedSsrShadesWithoutIt)
{
    // Pipelines that cannot compile never fail the frame: it shades as without the reflections,
    // the renderer tries once more, then gives up
    const auto render = [&] (bool fail, std::optional<bool> screenSpaceReflections, SceneRenderer& renderer)
    {
        Scene scene;
        buildOnScreenEmitterScene (scene);
        setFullTierTaa (scene, 8);
        setScreenSpaceReflections (scene, screenSpaceReflections);

        if (fail)
            renderer.failScreenSpaceReflectionsForTesting();

        std::vector<bool> givenUp;
        for (int frame = 0; frame < 3; ++frame)
        {
            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);
            givenUp.push_back (renderer.getReflectionPass() != nullptr && renderer.getReflectionPass()->hasGivenUp());
        }

        // The first frame has no history to trace from, the next two fail
        EXPECT_EQ (givenUp, (std::vector<bool> { false, false, fail }));
        return renderConverged (renderer, scene, 64);
    };

    SceneRenderer failing, off;
    const auto image = render (true, std::nullopt, failing);
    const auto without = render (false, false, off);
    ASSERT_TRUE (image.isValid() && without.isValid());
    EXPECT_TRUE (failing.getLastError().isEmpty()) << failing.getLastError();

    // Giving up restarted the accumulation of the failing renderer, so its silhouettes converged
    // over other jitter offsets: the floor matches, the edges may not to the level
    const auto floorFailing = meanColor (image, reflectedEmitterRegion());
    const auto floorWithout = meanColor (without, reflectedEmitterRegion());
    EXPECT_NEAR (floorFailing.getRed(), floorWithout.getRed(), 1);
    EXPECT_NEAR (floorFailing.getGreen(), floorWithout.getGreen(), 1);
}

TEST_F (SceneRendererMetalTests, SsrUnavailableOnRasterRendersFallback)
{
    checkScreenSpaceReflectionsUnavailableOnRasterRendersFallback (RenderSettings::TierPreference::raster);
}

// The screen-space diffuse GI tests converge 8 jittered samples at 64 x 64 on the full tier. The GI
// reads the TAA history, so the first frame of a renderer gathers nothing (preflight R3)
TEST_F (SceneRendererMetalTests, ScreenSpaceGIBouncesColorOntoTheFloor)
{
    // The red room without a volume: the floor in front of the red wall shows the gray environment
    // alone without GI, and with it also the red the wall emits
    const auto render = [&] (bool screenSpaceGI, SceneRenderer& renderer)
    {
        Scene scene;
        buildGIRedRoomScene (scene, screenSpaceGI);

        const auto image = renderConverged (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        EXPECT_TRUE (renderer.isConverged());
        return image;
    };

    SceneRenderer on, off;
    const auto withGI = render (true, on);
    const auto without = render (false, off);
    ASSERT_TRUE (withGI.isValid() && without.isValid());

    // The floor right in front of the wall, below its bottom edge
    const auto [top, bottom] = findRedWallRows (without);
    ASSERT_GE (top, 0);
    ASSERT_LT (bottom, 56);
    const Rectangle<int> floor { 16, bottom + 2, 32, 6 };

    const auto floorOn = meanColor (withGI, floor);
    const auto floorOff = meanColor (without, floor);
    RecordProperty ("wallRows", (String (top) + " to " + String (bottom)).toStdString());
    RecordProperty ("floorOn", floorOn.toString().toStdString());
    RecordProperty ("floorOff", floorOff.toString().toStdString());
    RecordProperty ("ratioOn", String (redOverGreen (floorOn), 3).toStdString());
    RecordProperty ("ratioOff", String (redOverGreen (floorOff), 3).toStdString());
    EXPECT_NEAR (redOverGreen (floorOff), 1.0f, 0.05f);
    // Measured 1.232 against 1.000: the threshold keeps three quarters of that margin
    EXPECT_GT (redOverGreen (floorOn), redOverGreen (floorOff) + 0.17f);

    // The gathered light adds to the environment's: a mix of the two would trade some of the
    // gray for the wall's red, and the green would drop
    EXPECT_GE (static_cast<int> (floorOn.getGreen()), static_cast<int> (floorOff.getGreen()) - 1);

    // At the default ambient occlusion distance of 0.2 m, for the record
    {
        Scene scene;
        buildGIRedRoomScene (scene, true);
        auto settings = scene.getRenderSettings();
        settings.aoDistance = 0.2f;
        scene.setRenderSettings (settings);

        SceneRenderer renderer;
        const auto image = renderConverged (renderer, scene, 64);
        RecordProperty ("ratioOnDefaultDistance", String (redOverGreen (meanColor (image, floor)), 3).toStdString());
    }

    // Only the renderer with GI keeps its output: rgba16float at half resolution
    ASSERT_NE (off.getAmbientOcclusionPass(), nullptr);
    EXPECT_EQ (off.getAmbientOcclusionPass()->getIndirectDiffuse(), nullptr);
    ASSERT_NE (on.getAmbientOcclusionPass(), nullptr);
    const auto indirect = on.getAmbientOcclusionPass()->getIndirectDiffuse();
    ASSERT_NE (indirect, nullptr);
    EXPECT_EQ (indirect->getFormat(), GpuTextureFormat::rgba16float);
    EXPECT_EQ (indirect->getWidth(), 32u);
    EXPECT_EQ (on.getAmbientOcclusionPass()->getOutput()->getFormat(), GpuTextureFormat::rgba8unorm);

#if YUP_ENABLE_SHADER_TRANSPILER
    EXPECT_TRUE (isFiniteTexture (indirect));
    EXPECT_FALSE (isSaturatedTexture (indirect));
#endif
}

TEST_F (SceneRendererMetalTests, ScreenSpaceGILeavesAnOpenPlaneAsItIs)
{
    // Nothing occludes an open plane: the gather adds nothing, so the frame is the one without GI
    // up to a level of rounding, within two at the far edge of the floor. A regression guard: the
    // samples on the plane itself are rejected as not facing the pixel, so the confidence is about
    // 0 and a mix of the probe light and the gathered light would leave the plane as it is too
    // (ScreenSpaceGIBouncesColorOntoTheFloor checks that the gathered light only adds)
    const auto render = [&] (bool screenSpaceGI, SceneRenderer& renderer)
    {
        Scene scene;
        buildFacingAwayWallScene (scene, screenSpaceGI);
        scene.getRoot()->removeChild (scene.getRoot()->findChild ("red wall"));

        const auto image = renderConverged (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    };

    SceneRenderer on, off;
    const auto withGI = render (true, on);
    const auto without = render (false, off);
    ASSERT_TRUE (withGI.isValid() && without.isValid());
    ASSERT_NE (on.getAmbientOcclusionPass(), nullptr);
    EXPECT_NE (on.getAmbientOcclusionPass()->getIndirectDiffuse(), nullptr);

    // The largest difference of a channel, over the frame and below its first rows, where the far
    // edge of the floor meets the background: a sample there carries the background's tint
    const auto largestDifference = [&] (int firstRow)
    {
        auto largest = 0;
        for (int y = firstRow; y < 64; ++y)
        {
            for (int x = 0; x < 64; ++x)
            {
                const auto a = withGI.getPixelColor (x, y);
                const auto b = without.getPixelColor (x, y);
                largest = jmax (largest, std::abs (a.getRed() - b.getRed()), std::abs (a.getGreen() - b.getGreen()), std::abs (a.getBlue() - b.getBlue()));
            }
        }

        return largest;
    };

    RecordProperty ("largestDifference", largestDifference (0));
    RecordProperty ("largestDifferenceBelowTheEdge", largestDifference (6));
    EXPECT_LE (largestDifference (0), 2);
    EXPECT_LE (largestDifference (6), 1);
    EXPECT_GT (meanLuminance (without, { 0, 32, 64, 32 }), 60.0); // Not vacuous: the floor is lit
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererMetalTests, SsgiWithoutHistoryShadesAsWithoutIt)
{
    // A frame without a history to gather from traces the ambient occlusion alone, which leaves it
    // bit for bit as without GI: a renderer's first frame, and the frame after restartAccumulation(),
    // which keeps the history's textures but no longer trusts them
    SceneRenderer warmUp;
    {
        Scene scene;
        buildGIRedRoomScene (scene, false);
        ASSERT_TRUE (renderToImage (warmUp, scene, 64).isValid());
    }

    // The 8 bytes of the half float scene color, three to an opaque rgba8 image
    const auto sceneColorBits = [&] (SceneRenderer& renderer)
    {
        std::vector<Image> bits;
        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        for (const auto* bytes : { "vec4(lo.xyz, 1.0)", "vec4(lo.w, hi.xy, 1.0)", "vec4(hi.zw, 0.0, 1.0)" })
        {
            const auto* halves = "#define lo unpackUnorm4x8(packHalf2x16(t.rg))\n#define hi unpackUnorm4x8(packHalf2x16(t.ba))\n";
            bits.push_back (sceneColor != nullptr ? mapTexels (sceneColor, bytes, halves) : Image());
        }

        return bits;
    };

    const auto expectSameBits = [&] (const std::vector<Image>& withGI, const std::vector<Image>& without, const char* when)
    {
        for (size_t part = 0; part < withGI.size(); ++part)
        {
            ASSERT_TRUE (withGI[part].isValid() && without[part].isValid()) << when;

            auto differing = 0;
            for (int y = 0; y < without[part].getHeight(); ++y)
                for (int x = 0; x < without[part].getWidth(); ++x)
                    differing += withGI[part].getPixelColor (x, y) != without[part].getPixelColor (x, y) ? 1 : 0;

            EXPECT_EQ (differing, 0) << when << " " << part;
        }
    };

    // Each renderer draws as many frames, so both jitter alike, then the history is invalidated
    const auto renderBits = [&] (bool screenSpaceGI, int frames, bool restart)
    {
        Scene scene;
        buildGIRedRoomScene (scene, screenSpaceGI);

        SceneRenderer renderer;
        for (int i = 0; i < frames; ++i)
            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

        if (restart)
        {
            EXPECT_TRUE (renderer.getAmbientOcclusionPass() != nullptr && (renderer.getAmbientOcclusionPass()->getIndirectDiffuse() != nullptr) == screenSpaceGI);
            renderer.restartAccumulation();
        }

        EXPECT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_TRUE (renderer.getAmbientOcclusionPass() != nullptr && renderer.getAmbientOcclusionPass()->getIndirectDiffuse() == nullptr);
        return sceneColorBits (renderer);
    };

    expectSameBits (renderBits (true, 0, false), renderBits (false, 0, false), "first frame");
    expectSameBits (renderBits (true, 9, true), renderBits (false, 9, true), "after restartAccumulation()");
}
#endif

TEST_F (SceneRendererMetalTests, SsgiConvergedRendersAreIdentical)
{
    // The slices turn with the frame only under temporal anti-aliasing, whose running mean freezes
    // once converged (preflight R13): two renderers converging on the same scene end on one image
    Scene scene;
    buildGIRedRoomScene (scene, true);

    SceneRenderer warmUp;
    ASSERT_TRUE (renderToImage (warmUp, scene, 64).isValid());

    SceneRenderer a, b;
    const auto imageA = renderConverged (a, scene, 64);
    const auto imageB = renderConverged (b, scene, 64);
    ASSERT_TRUE (imageA.isValid() && imageB.isValid());
    EXPECT_TRUE (a.isConverged() && b.isConverged());
    ASSERT_NE (a.getAmbientOcclusionPass(), nullptr);
    EXPECT_NE (a.getAmbientOcclusionPass()->getIndirectDiffuse(), nullptr);

    auto differing = 0;
    for (int y = 0; y < imageA.getHeight(); ++y)
        for (int x = 0; x < imageA.getWidth(); ++x)
            differing += imageA.getPixelColor (x, y) != imageB.getPixelColor (x, y) ? 1 : 0;

    EXPECT_EQ (differing, 0);
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererMetalTests, SsgiScalesWithTheExposure)
{
    // The history holds exposed radiance, divided by the exposure of the frame it was rendered with:
    // converged at exposure 0.25 then at 0.5, the floor with GI doubles like the floor without it.
    // Dividing by nothing, or by the exposure twice, would scale the gathered red by 4 or 1
    const auto renderRatio = [&] (bool screenSpaceGI)
    {
        Scene scene;
        buildGIRedRoomScene (scene, screenSpaceGI);
        scene.setExposure (0.25f);

        SceneRenderer renderer;
        Rectangle<int> floor;
        const auto meanRed = [&]
        {
            const auto [top, bottom] = findRedWallRows (renderConverged (renderer, scene, 64));
            EXPECT_TRUE (top >= 0 && bottom < 56) << top << " " << bottom;
            floor = floor.isEmpty() ? Rectangle<int> { 16, bottom + 2, 32, 6 } : floor;

            const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
            return sceneColor != nullptr ? meanLinearRed (sceneColor, floor) : -1.0;
        };

        const auto atQuarter = meanRed();
        scene.setExposure (0.5f);
        const auto atHalf = meanRed();
        RecordProperty (screenSpaceGI ? "floorRedOn" : "floorRedOff", (String (atQuarter, 4) + " -> " + String (atHalf, 4)).toStdString());
        return atQuarter > 0.0 ? atHalf / atQuarter : -1.0;
    };

    const auto ratioOn = renderRatio (true);
    const auto ratioOff = renderRatio (false);
    RecordProperty ("ratioOn", String (ratioOn, 3).toStdString());
    RecordProperty ("ratioOff", String (ratioOff, 3).toStdString());
    EXPECT_GT (ratioOff, 1.5);

    // Measured 1.977 against 1.995: the gathered light, more than half of the red, changes a lot
    // from frame to frame, and the running mean of temporal anti-aliasing weighs each frame by
    // 1 / (1 + luma), more so at the higher exposure. A missing division moved the ratio to 2.459
    EXPECT_NEAR (ratioOn, ratioOff, 0.03 * ratioOff);
}
#endif

TEST_F (SceneRendererMetalTests, SsgiConvergedRegionMatchesAPrimedHistory)
{
    // A renderer's first frame has no history to gather from, and enters the running mean of temporal
    // anti-aliasing without GI (preflight R31, extended to GI by R33). The converged light then
    // barely depends on the history the renderer started from: it matches a renderer whose history
    // was primed by an earlier convergence, then restarted by a setting that changes nothing on
    // screen (contact shadows, without a light)
    const auto renderRed = [&] (bool primed)
    {
        Scene scene;
        buildGIRedRoomScene (scene, true);
        scene.setExposure (0.5f); // The floor's red stays below 1, which meanLinearRed() reads

        SceneRenderer renderer;
        const auto [top, bottom] = findRedWallRows (renderConverged (renderer, scene, 64));
        EXPECT_TRUE (top >= 0 && bottom < 56) << top << " " << bottom;

        if (primed)
        {
            auto settings = scene.getRenderSettings();
            settings.contactShadows = false;
            scene.setRenderSettings (settings);
            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr);
            EXPECT_FALSE (renderer.isConverged());
            EXPECT_TRUE (renderConverged (renderer, scene, 64).isValid()) << renderer.getLastError();
        }

        EXPECT_TRUE (renderer.isConverged());
        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        return sceneColor != nullptr ? meanLinearRed (sceneColor, { 16, bottom + 2, 32, 6 }) : -1.0;
    };

    const auto fresh = renderRed (false);
    const auto primed = renderRed (true);
    RecordProperty ("floorRedFresh", String (fresh, 4).toStdString());
    RecordProperty ("floorRedPrimed", String (primed, 4).toStdString());
    EXPECT_GT (fresh, 0.3);

    // Measured 0.5107 against 0.5182. The gathered light is noisy, so the converged floor depends on
    // which frames the 8 samples were: renderers starting 0 to 3 frames apart converge between 0.5085
    // and 0.5223, 2.7% apart. The first frame dilutes the converged GI by about 1.4% on average over
    // those starts, which this tolerance cannot see: it guards against a larger dilution
    EXPECT_NEAR (fresh, primed, 0.03 * primed);
}

TEST_F (SceneRendererMetalTests, SsgiIgnoresSurfacesFacingAway)
{
    // The floor behind the wall sees only its back: the wall occludes it, but lights it with nothing.
    // The floor in front of it, which its emitting side faces, turns red
    const auto render = [&] (bool screenSpaceGI)
    {
        Scene scene;
        buildFacingAwayWallScene (scene, screenSpaceGI);

        SceneRenderer renderer;
        const auto image = renderConverged (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    };

    const auto withGI = render (true);
    const auto without = render (false);
    ASSERT_TRUE (withGI.isValid() && without.isValid());

    const auto [top, bottom] = findRedWallRows (without);
    ASSERT_GT (top, 4);
    ASSERT_LT (bottom, 59);

    const Rectangle<int> behind { 16, top - 6, 32, 3 };
    const Rectangle<int> front { 16, bottom + 2, 32, 3 };
    const auto behindOn = meanColor (withGI, behind);
    const auto behindOff = meanColor (without, behind);
    const auto frontOn = meanColor (withGI, front);
    const auto frontOff = meanColor (without, front);
    RecordProperty ("wallRows", (String (top) + " to " + String (bottom)).toStdString());
    RecordProperty ("behindOn", behindOn.toString().toStdString());
    RecordProperty ("behindOff", behindOff.toString().toStdString());
    RecordProperty ("frontOn", frontOn.toString().toStdString());
    RecordProperty ("frontOff", frontOff.toString().toStdString());

    // Measured 1.374 against 1.000 in front, and the same color behind with and without GI, in a
    // band 4 to 6 rows above the wall's edge, which its antialiasing does not reach. Lighting from
    // the wall's back too made the band 1.045 against 1.000: 2 levels of red are about 0.013
    EXPECT_GT (redOverGreen (frontOn), redOverGreen (frontOff) + 0.2f); // Not vacuous: the wall lights what it faces
    EXPECT_NEAR (redOverGreen (behindOn), redOverGreen (behindOff), 0.015f);
}

TEST_F (SceneRendererMetalTests, SsgiSettingRestartsAccumulation)
{
    Scene scene;
    buildGIRedRoomScene (scene, false);

    SceneRenderer renderer;
    for (int i = 0; i < 8; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    // Restarted, not invalidated: the history stays valid for the GI to gather at once
    auto settings = scene.getRenderSettings();
    settings.screenSpaceGI = true;
    scene.setRenderSettings (settings);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());
    ASSERT_NE (renderer.getAmbientOcclusionPass(), nullptr);
    EXPECT_NE (renderer.getAmbientOcclusionPass()->getIndirectDiffuse(), nullptr);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_F (SceneRendererMetalTests, FailedSsgiKeepsTheAmbientOcclusionAndGivesUp)
{
    // A GI failure leaves the frame with its ambient occlusion; the second in a row gives the GI
    // up, which changes the image as much as turning it off: a converged renderer starts over
    Scene scene;
    buildGIRedRoomScene (scene, true);

    // Renders until converged, counting the frames, so a reference renderer can draw as many
    auto frames = 0;
    const auto converge = [&] (SceneRenderer& renderer, Scene& target)
    {
        for (int i = 0; i < 64; ++i)
        {
            ++frames;
            if (renderer.render (device, target, 64, 64) == nullptr || renderer.isConverged())
                break;
        }

        ++frames;
        return renderToImage (renderer, target, 64);
    };

    SceneRenderer renderer;
    ASSERT_TRUE (converge (renderer, scene).isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());
    const auto* pass = renderer.getAmbientOcclusionPass();
    ASSERT_NE (pass, nullptr);
    ASSERT_NE (pass->getIndirectDiffuse(), nullptr);

    renderer.failScreenSpaceGIForTesting();

    for (const auto givesUp : { false, true })
    {
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        ++frames;
        EXPECT_EQ (renderer.getAmbientOcclusionPass(), pass);
        EXPECT_EQ (pass->getIndirectDiffuse(), nullptr);
        EXPECT_NE (pass->getOutput(), nullptr);
        EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion), nullptr);
        EXPECT_EQ (pass->getNumCachedIndirectDiffuseTargets(), 0u);
        EXPECT_EQ (pass->hasGivenUpIndirectDiffuse(), givesUp);
    }

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ++frames;
    EXPECT_FALSE (renderer.isConverged());
    EXPECT_TRUE (renderer.getLastError().isEmpty());
    EXPECT_TRUE (renderer.waitForGPU().wasOk());

    Scene offScene;
    buildGIRedRoomScene (offScene, false);

    // The restart keeps the history, which still holds a share of the light gathered before: how
    // much of it the converged frame keeps, against a renderer that never had GI
    {
        const auto lingering = converge (renderer, scene);
        SceneRenderer off;
        const auto without = renderConverged (off, offScene, 64);
        ASSERT_TRUE (lingering.isValid() && without.isValid());

        const auto [top, bottom] = findRedWallRows (without);
        const Rectangle<int> floor { 16, bottom + 2, 32, 6 };
        RecordProperty ("lingeringPixels", countDifferencesAboveOneLevel (lingering, without));
        RecordProperty ("lingeringFloor", (meanColor (lingering, floor).toString() + " against " + meanColor (without, floor).toString()).toStdString());
    }

    // A new size discards the history: given up, the frame is then the one without GI, against a
    // renderer that drew as many frames before the resize, so both jitter alike
    ASSERT_NE (renderer.render (device, scene, 48, 48), nullptr) << renderer.getLastError();
    const auto failing = renderConverged (renderer, scene, 64);

    SceneRenderer off;
    for (int i = 0; i < frames; ++i)
        ASSERT_NE (off.render (device, offScene, 64, 64), nullptr) << off.getLastError();

    ASSERT_NE (off.render (device, offScene, 48, 48), nullptr) << off.getLastError();
    const auto without = renderConverged (off, offScene, 64);
    ASSERT_TRUE (failing.isValid() && without.isValid());
    EXPECT_EQ (countDifferencesAboveOneLevel (failing, without), 0);
}

TEST_F (SceneRendererMetalTests, SsgiOffReleasesItsTargets)
{
    // A frame without GI releases the GI targets only, the ambient occlusion keeps its own
    Scene scene;
    buildGIRedRoomScene (scene, true);

    SceneRenderer renderer;
    for (int i = 0; i < 2; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto* pass = renderer.getAmbientOcclusionPass();
    ASSERT_NE (pass, nullptr);
    EXPECT_GT (pass->getNumCachedIndirectDiffuseTargets(), 0u);

    auto settings = scene.getRenderSettings();
    settings.screenSpaceGI = false;
    scene.setRenderSettings (settings);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (pass->getNumCachedIndirectDiffuseTargets(), 0u);
    EXPECT_EQ (pass->getIndirectDiffuse(), nullptr);
    EXPECT_GT (pass->getNumCachedTargets(), 0u);

    settings.screenSpaceGI = true;
    scene.setRenderSettings (settings);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_GT (pass->getNumCachedIndirectDiffuseTargets(), 0u);

    // A new size has no history yet: the frame traces the ambient occlusion alone
    ASSERT_NE (renderer.render (device, scene, 48, 48), nullptr) << renderer.getLastError();
    EXPECT_EQ (pass->getNumCachedIndirectDiffuseTargets(), 0u);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

// RenderSettings::debugView on 64 x 64 frames of the full tier
class SceneRendererDebugViewTests : public SceneRendererMetalTests
{
protected:
    static void setDebugView (Scene& scene, RenderSettings::DebugView debugView)
    {
        auto settings = scene.getRenderSettings();
        settings.debugView = debugView;
        scene.setRenderSettings (settings);
    }

    /** The pixels of @p image with any color channel above 0. */
    static int countNonBlackPixels (const Image& image)
    {
        int count = 0;
        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto color = image.getPixelColor (x, y);
                count += color.getRed() > 0 || color.getGreen() > 0 || color.getBlue() > 0 ? 1 : 0;
            }
        }

        return count;
    }

#if YUP_ENABLE_SHADER_TRANSPILER
    /** Shows @p view at an exposure of 0.25, which keeps it below saturation, and checks that each
        pixel shows the texel of @p output covering it as clamp (rgb * @p weight * 0.25). Both the
        render and the expected image round the same float expression to 8 bits, so they agree
        within one level. At least @p minLit pixels must be lit, and @p minPartial texels must have
        a confidence or share below 0.9 with some light, so a wrong weight shows.
    */
    void checkExposedPerTexel (Scene& scene, RenderSettings::DebugView view, SceneRenderer::PassOutput output, const String& weight, int minLit, int minPartial)
    {
        scene.setExposure (0.25f);
        setDebugView (scene, view);

        SceneRenderer renderer;
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        const auto image = renderToImage (renderer, scene, 64);
        const auto source = renderer.getPassOutput (output);
        ASSERT_TRUE (image.isValid());
        ASSERT_NE (source, nullptr);

        const auto expected = mapTexels (source, "vec4(clamp(t.rgb * " + weight + " * 0.25, 0.0, 1.0), 1.0)");
        const auto partial = mapTexels (source, "vec4(t.a < 0.9 && max(t.r, max(t.g, t.b)) * 0.25 > 0.04 ? 1.0 : 0.0, 0.0, 0.0, 1.0)");
        ASSERT_TRUE (expected.isValid() && partial.isValid());
        const auto scale = image.getWidth() / expected.getWidth();

        auto differing = 0, lit = 0, partialTexels = 0;
        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto shown = image.getPixelColor (x, y);
                const auto wanted = expected.getPixelColor (x / scale, y / scale);
                differing += std::abs (shown.getRed() - wanted.getRed()) > 1 || std::abs (shown.getGreen() - wanted.getGreen()) > 1
                                     || std::abs (shown.getBlue() - wanted.getBlue()) > 1 ? 1 : 0;
                lit += wanted.getRed() > 16 ? 1 : 0;
            }
        }

        for (int y = 0; y < partial.getHeight(); ++y)
            for (int x = 0; x < partial.getWidth(); ++x)
                partialTexels += partial.getPixelColor (x, y).getRed() > 127 ? 1 : 0;

        RecordProperty ("litPixels", lit);
        RecordProperty ("partialTexels", partialTexels);
        EXPECT_EQ (differing, 0) << weight;
        EXPECT_GT (lit, minLit) << weight;
        EXPECT_GT (partialTexels, minPartial) << weight;
    }
#endif
};

TEST_F (SceneRendererDebugViewTests, NoneCreatesNoPassAndReleasesItsTargets)
{
    Scene scene;
    buildOnScreenEmitterScene (scene);
    setFullTierTaa (scene, 8);

    SceneRenderer renderer;
    for (int i = 0; i < 2; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_EQ (renderer.getDebugViewPass(), nullptr);

    setDebugView (scene, RenderSettings::DebugView::reflections);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    const auto* pass = renderer.getDebugViewPass();
    ASSERT_NE (pass, nullptr);
    EXPECT_GT (pass->getNumCachedTargets(), 0u);

    // Kept, with no target, so showing a view again compiles nothing
    setDebugView (scene, RenderSettings::DebugView::none);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getDebugViewPass(), pass);
    EXPECT_EQ (pass->getNumCachedTargets(), 0u);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_F (SceneRendererDebugViewTests, ChangingTheViewKeepsTheAccumulation)
{
    Scene scene;
    buildOnScreenEmitterScene (scene);
    setFullTierTaa (scene, 8);

    SceneRenderer renderer;
    ASSERT_TRUE (renderConverged (renderer, scene, 64).isValid());
    ASSERT_TRUE (renderer.isConverged());

    setDebugView (scene, RenderSettings::DebugView::reflections);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());

    setDebugView (scene, RenderSettings::DebugView::none);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());
}

TEST_F (SceneRendererDebugViewTests, ReflectionsShowBlackWithoutThemAndNeverStale)
{
    Scene scene;
    buildOnScreenEmitterScene (scene);
    setFullTierTaa (scene, 8);
    setDebugView (scene, RenderSettings::DebugView::reflections);

    // The first frame has no history to trace the reflections from
    SceneRenderer renderer;
    const auto first = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (first.isValid());
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);
    EXPECT_EQ (countNonBlackPixels (first), 0);

    // The floor reflects the red emitter
    const auto traced = renderToImage (renderer, scene, 64);
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);
    const auto floor = meanColor (traced, reflectedEmitterRegion());
    RecordProperty ("floorRed", static_cast<int> (floor.getRed()));
    EXPECT_GT (floor.getRed(), 200); // Measured 255
    EXPECT_LE (floor.getGreen(), 2);

    // Turned off, the view shows black, not the reflections of the frame before
    setScreenSpaceReflections (scene, false);
    const auto off = renderToImage (renderer, scene, 64);
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);
    EXPECT_EQ (countNonBlackPixels (off), 0);
}

TEST_F (SceneRendererDebugViewTests, IndirectDiffuseShowsTheGatheredLight)
{
    Scene scene;
    buildGIRedRoomScene (scene, true);
    setDebugView (scene, RenderSettings::DebugView::indirectDiffuse);

    SceneRenderer renderer;
    const auto first = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (first.isValid());
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::indirectDiffuse), nullptr);
    EXPECT_EQ (countNonBlackPixels (first), 0);

    // The red wall bounces red onto the floor at its foot
    const auto gathered = renderToImage (renderer, scene, 64);
    const auto indirect = renderer.getPassOutput (SceneRenderer::PassOutput::indirectDiffuse);
    ASSERT_NE (indirect, nullptr);
    EXPECT_EQ (indirect, renderer.getAmbientOcclusionPass()->getIndirectDiffuse());
    EXPECT_EQ (indirect->getWidth(), 32u);

    auto redder = 0;
    for (int y = 0; y < gathered.getHeight(); ++y)
        for (int x = 0; x < gathered.getWidth(); ++x)
            redder += gathered.getPixelColor (x, y).getRed() > gathered.getPixelColor (x, y).getGreen() + 8 ? 1 : 0;

    RecordProperty ("redderPixels", redder);
    EXPECT_GT (redder, 400); // Measured 572 of 4096

    auto settings = scene.getRenderSettings();
    settings.screenSpaceGI = false;
    scene.setRenderSettings (settings);
    const auto off = renderToImage (renderer, scene, 64);
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::indirectDiffuse), nullptr);
    EXPECT_EQ (countNonBlackPixels (off), 0);
}

TEST_F (SceneRendererDebugViewTests, RasterIgnoresIt)
{
    Scene scene;
    buildOnScreenEmitterScene (scene);
    auto settings = scene.getRenderSettings();
    settings.tier = RenderSettings::TierPreference::raster;
    settings.ambientOcclusion = true;
    scene.setRenderSettings (settings);

    SceneRenderer renderer;
    const auto finalImage = renderToImage (renderer, scene, 64);

    setDebugView (scene, RenderSettings::DebugView::ambientOcclusion);
    const auto withView = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (finalImage.isValid() && withView.isValid());
    EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion), nullptr);
    EXPECT_EQ (renderer.getDebugViewPass(), nullptr);
    EXPECT_EQ (countDifferencesAboveOneLevel (withView, finalImage), 0); // Two raster frames of this scene differ by up to one level
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererDebugViewTests, AmbientOcclusionShowsTheVisibilityAsGray)
{
    // Each pixel shows the visibility of the half resolution texel covering it, unfiltered
    Scene scene;
    buildGIRedRoomScene (scene, false);
    setDebugView (scene, RenderSettings::DebugView::ambientOcclusion);

    SceneRenderer renderer;
    const auto image = renderToImage (renderer, scene, 64);
    const auto ambientOcclusion = renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion);
    ASSERT_TRUE (image.isValid());
    ASSERT_NE (ambientOcclusion, nullptr);
    ASSERT_EQ (ambientOcclusion->getWidth(), 32u);

    const auto visibility = mapTexels (ambientOcclusion, "vec4(t.aaa, 1.0)");
    ASSERT_TRUE (visibility.isValid());

    auto differing = 0, occluded = 0;
    for (int y = 0; y < image.getHeight(); ++y)
    {
        for (int x = 0; x < image.getWidth(); ++x)
        {
            const auto shown = image.getPixelColor (x, y);
            const auto expected = visibility.getPixelColor (x / 2, y / 2).getRed();
            differing += std::abs (shown.getRed() - expected) > 1 || shown.getGreen() != shown.getRed() || shown.getBlue() != shown.getRed() ? 1 : 0;
            occluded += expected < 200 ? 1 : 0;
        }
    }

    RecordProperty ("occludedPixels", occluded);
    EXPECT_EQ (differing, 0);
    EXPECT_GT (occluded, 80); // Measured 120: the comparison is not of a white image
}

TEST_F (SceneRendererDebugViewTests, ReflectionsAreExposedAndWeightedByTheirConfidence)
{
    Scene scene;
    buildOnScreenEmitterScene (scene);
    setFullTierTaa (scene, 8);
    checkExposedPerTexel (scene, RenderSettings::DebugView::reflections, SceneRenderer::PassOutput::reflections, "t.a", 250, 30); // Measured 382 and 58
}

TEST_F (SceneRendererDebugViewTests, IndirectDiffuseIsExposedAsGathered)
{
    // Not weighted by a: the gathered rgb already carries the share, as the composite adds it
    Scene scene;
    buildGIRedRoomScene (scene, true);
    checkExposedPerTexel (scene, RenderSettings::DebugView::indirectDiffuse, SceneRenderer::PassOutput::indirectDiffuse, "1.0", 250, 50); // Measured 384 and 96
}
#endif

TEST_F (SceneRendererMetalTests, SilhouetteEdgesMatchRasterOnBothTiers)
{
    Scene scene;
    buildSilhouettesScene (scene);
    setTier (scene, RenderSettings::TierPreference::raster);

    // The reference: raster with bloom off, tone mapped per sample and resolved in display space
    SceneRenderer rasterRenderer;
    const auto raster = renderToImage (rasterRenderer, scene, 128);
    ASSERT_TRUE (raster.isValid()) << rasterRenderer.getLastError();

    // Raster with bloom at zero intensity: the half-float path through the same samples
    auto settings = scene.getRenderSettings();
    settings.bloom = true;
    settings.bloomIntensity = 0.0f;
    scene.setRenderSettings (settings);

    SceneRenderer bloomRenderer;
    const auto bloom = renderToImage (bloomRenderer, scene, 128);
    ASSERT_TRUE (bloom.isValid()) << bloomRenderer.getLastError();
    ASSERT_EQ (bloomRenderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)->getFormat(), GpuTextureFormat::rgba16float);

    // The full tier, converged: its jittered frames estimate each pixel's coverage differently
    setBloom (scene, false);
    setFullTierTaa (scene, 16);

    SceneRenderer fullRenderer;
    Image full;
    for (int i = 0; i < 16; ++i)
        full = renderToImage (fullRenderer, scene, 128);
    ASSERT_TRUE (full.isValid()) << fullRenderer.getLastError();
    ASSERT_TRUE (fullRenderer.isConverged());

    // The same samples as the reference: every pixel matches
    const auto bloomDiffering = countDifferences (bloom, raster);
    RecordProperty ("bloomDiffering", bloomDiffering);
    EXPECT_EQ (bloomDiffering, 0);

    // Other samples: the mean over the silhouettes, where the coverage averages out. Each pixel
    // rounds to its own 8 bit level on each path, so the means may differ by a fraction of a level;
    // tone mapping the premultiplied color instead moved them by 2.3 to 10.8 levels
    for (const auto& [name, left, right] : { std::tuple { "bright", 0, 64 }, std::tuple { "gray", 64, 128 } })
    {
        const auto reference = silhouetteBandLuminance (raster, raster, left, right);
        const auto fullBand = silhouetteBandLuminance (full, raster, left, right);
        const auto bloomBand = silhouetteBandLuminance (bloom, raster, left, right);
        RecordProperty ((String (name) + "Raster").toStdString(), String (reference, 2).toStdString());
        RecordProperty ((String (name) + "Full").toStdString(), String (fullBand, 2).toStdString());
        RecordProperty ((String (name) + "RasterBloom").toStdString(), String (bloomBand, 2).toStdString());

        EXPECT_NEAR (fullBand, reference, 1.0) << name;
        EXPECT_NEAR (bloomBand, reference, 1.0) << name;
    }
}

TEST_F (SceneRendererMetalTests, FullRasterFullRoundTripStartsOver)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 8);

    SceneRenderer reference;
    Image converged;
    for (int i = 0; i < 8; ++i)
        converged = renderToImage (reference, scene, 128);
    ASSERT_TRUE (converged.isValid()) << reference.getLastError();
    ASSERT_TRUE (reference.isConverged());

    SceneRenderer renderer;
    for (int i = 0; i < 8; ++i)
        ASSERT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    setTier (scene, RenderSettings::TierPreference::raster);
    ASSERT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
    ASSERT_EQ (renderer.getCapabilities()->getTier(), RenderCapabilities::Tier::raster);

    // Back on the full tier the history is gone: the first frame stands alone, so no pixel of its
    // silhouettes is partially covered yet
    setTier (scene, RenderSettings::TierPreference::full);
    auto image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());
    EXPECT_EQ (countIntermediatePixels (image, 40), 0);

    for (int i = 2; i <= 8; ++i)
    {
        image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        EXPECT_EQ (renderer.isConverged(), i == 8) << "render " << i;
    }

    // The jitter sequence continued through the raster frame, so edges may differ by a level or two
    const auto differing = countDifferences (image, converged);
    RecordProperty ("differingFromAFreshRenderer", differing);
    EXPECT_LE (differing, 16);
    EXPECT_GE (countIntermediatePixels (image, 40), 4);
}

TEST_F (SceneRendererMetalTests, ToneMappingChangeKeepsTheFullTierConverged)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 8);

    SceneRenderer renderer;
    Image converged;
    for (int i = 0; i < 8; ++i)
        converged = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (converged.isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    // Tone mapping runs after the accumulation: the converged history shows through the new operator
    scene.setToneMapping (Scene::ToneMapping::aces);
    const auto image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());
    EXPECT_GT (countDifferences (image, converged), 1000);
    EXPECT_GE (countIntermediatePixels (image, 40), 4);
}

TEST_F (SceneRendererMetalTests, ExposureAndLightChangesRestartButKeepTheHistory)
{
    // Each change sets a value, applied to the edges scene converged with 8 TAA samples
    const std::pair<const char*, std::function<void (Scene&, float)>> changes[] = {
        { "exposure", [] (Scene& scene, float value) { scene.setExposure (value); } },
        { "light", [] (Scene& scene, float value) { scene.getRoot()->findChild ("sun")->setRotation (Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, value)); } },
    };

    const std::map<String, std::pair<float, float>> values { { "exposure", { 1.0f, 1.5f } }, { "light", { 0.0f, 0.4f } } };

    // Converges at @p before, changes to @p after and renders once, then until converged again
    const auto renderChange = [&] (const std::function<void (Scene&, float)>& change, float before, float after, bool restart, Image& first)
    {
        Scene scene;
        buildEdgesScene (scene);
        setFullTierTaa (scene, 8);
        change (scene, before);

        SceneRenderer renderer;
        for (int i = 0; i < 8; ++i)
            EXPECT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
        EXPECT_TRUE (renderer.isConverged());

        change (scene, after);
        if (restart)
            renderer.restartAccumulation();

        first = renderToImage (renderer, scene, 128);
        EXPECT_FALSE (renderer.isConverged());

        Image converged;
        for (int i = 0; i < 8 && ! renderer.isConverged(); ++i)
            converged = renderToImage (renderer, scene, 128);
        EXPECT_TRUE (renderer.isConverged());
        return converged;
    };

    for (const auto& [name, change] : changes)
    {
        const auto [before, after] = values.at (name);

        // The signal is the first frame after the change. Blended into the kept history, its
        // silhouettes still have partially covered pixels. Started over from that frame alone, as
        // restartAccumulation() does, every pixel is either covered or not
        Image keptFirst, restartedFirst;
        const auto converged = renderChange (change, before, after, false, keptFirst);
        renderChange (change, before, after, true, restartedFirst);
        ASSERT_TRUE (keptFirst.isValid() && restartedFirst.isValid() && converged.isValid()) << name;

        RecordProperty ((name + String ("Kept")).toStdString(), countIntermediatePixels (keptFirst, 40));
        RecordProperty ((name + String ("Restarted")).toStdString(), countIntermediatePixels (restartedFirst, 40));
        EXPECT_GE (countIntermediatePixels (keptFirst, 40), 4) << name;
        EXPECT_EQ (countIntermediatePixels (restartedFirst, 40), 0) << name;

        // Converged again, what remains of the image before the change. The control goes through
        // the same frames and the same restart, from a value too close to tell apart, so only the
        // remains of the change differ. Counting the history as one sample once the scene is still
        // leaves about 1/8 of the change on edge pixels, where the clip box is wide
        Image controlFirst;
        const auto control = renderChange (change, after + 1.0e-4f, after, false, controlFirst);
        ASSERT_TRUE (control.isValid()) << name;

        int largest = 0;
        for (int y = 0; y < 128; ++y)
            for (int x = 0; x < 128; ++x)
                largest = jmax (largest, std::abs (static_cast<int> (converged.getPixelColor (x, y).getRed()) - static_cast<int> (control.getPixelColor (x, y).getRed())));

        // Keeping the history's blend weight instead left 236 pixels beyond 2 levels, up to 7
        const auto differing = countDifferences (converged, control);
        RecordProperty ((name + String ("DifferingFromControl")).toStdString(), differing);
        RecordProperty ((name + String ("LargestDifference")).toStdString(), largest);
        EXPECT_LE (differing, 16) << name;
        EXPECT_LE (largest, 4) << name;
    }
}

TEST_F (SceneRendererMetalTests, UncoveredPixelsShowTheBackgroundExactly)
{
    // The plate moves right: the pixels it leaves keep no trace of it, because the history
    // coverage is clamped to the coverage around them
    const auto background = Color (static_cast<uint8> (200), static_cast<uint8> (80), static_cast<uint8> (40));
    const auto isBackground = [&] (Color color)
    {
        return std::abs (color.getRed() - background.getRed()) <= 1
            && std::abs (color.getGreen() - background.getGreen()) <= 1
            && std::abs (color.getBlue() - background.getBlue()) <= 1;
    };

    Scene scene;
    buildEdgesScene (scene);
    scene.setBackgroundColor (background);
    setFullTierTaa (scene, 4);

    SceneRenderer renderer;
    Image image;
    for (int i = 0; i < 4; ++i)
        image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (renderer.isConverged()) << renderer.getLastError();

    // Just inside the left silhouette on row 64
    EXPECT_FALSE (isBackground (image.getPixelColor (34, 64)));

    scene.getRoot()->findChild ("plates")->setPosition ({ 0.5f, 0.0f, 0.0f });

    for (int frame = 1; frame <= 6; ++frame)
    {
        image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        const auto color = image.getPixelColor (34, 64);
        EXPECT_TRUE (isBackground (color)) << "frame " << frame << ": " << static_cast<int> (color.getRed()) << ","
                                           << static_cast<int> (color.getGreen()) << "," << static_cast<int> (color.getBlue());
    }

    EXPECT_TRUE (renderer.isConverged());
}

TEST_F (SceneRendererMetalTests, CameraMotionReorderingBlendedItemsKeepsTheHistory)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 16);

    // Two blended quads below row 40, 0.005 apart in depth: turning the camera by 0.01 swaps
    // which one is farther, and so their drawing order
    auto blended = Material::Ptr (new Material());
    blended->baseColorFactor = { 0.2f, 0.2f, 0.9f, 0.5f };
    blended->alphaMode = Material::AlphaMode::blend;

    for (const auto& [x, z] : { std::pair { -0.5f, 0.0f }, std::pair { 0.5f, -0.005f } })
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -0.2f, -0.2f, 0.0f };
        vertices[1].position = { 0.2f, -0.2f, 0.0f };
        vertices[2].position = { 0.2f, 0.2f, 0.0f };
        vertices[3].position = { -0.2f, 0.2f, 0.0f };
        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto mesh = Mesh::Ptr (new Mesh ("blended"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, blended);
        auto quad = EntityNode::Ptr (new EntityNode ("blended"));
        quad->attach<MeshNode> (mesh);
        quad->setPosition ({ x, -0.7f, 0.6f + z });
        scene.getRoot()->addChild (quad);
    }

    auto camera = EntityNode::Ptr (new EntityNode ("camera"));
    camera->attach<CameraNode>();
    camera->setPosition ({ 0.0f, 0.0f, 5.0f });
    scene.getRoot()->addChild (camera);
    scene.setActiveCamera (camera.get());

    SceneRenderer renderer;
    for (int i = 0; i < 16; ++i)
        ASSERT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    camera->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.01f));
    const auto image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();

    const auto levels = countEdgeLevels (image);
    RecordProperty ("levelsAfterReorder", static_cast<int> (levels));
    EXPECT_GE (levels, 4u);
}

TEST_F (SceneRendererMetalTests, CameraMotionKeepsTheHistory)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 16);

    auto camera = EntityNode::Ptr (new EntityNode ("camera"));
    camera->attach<CameraNode>();
    camera->setPosition ({ 0.0f, 0.0f, 5.0f });
    scene.getRoot()->addChild (camera);
    scene.setActiveCamera (camera.get());

    SceneRenderer renderer;
    for (int i = 0; i < 16; ++i)
        ASSERT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    // The first frame after a small camera move blends into the reprojected history, so the edges
    // stay antialiased; starting over from that frame alone would alias them
    camera->setPosition ({ 0.01f, 0.0f, 5.0f });
    const auto image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());

    const auto levels = countEdgeLevels (image);
    RecordProperty ("levelsAfterCameraMove", static_cast<int> (levels));
    EXPECT_GE (levels, 4u);
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererMetalTests, SpecularOcclusionIsContinuousAndKeepsGrazingReflections)
{
    // The self-contained block of lib/probes.glsl the shaders reading the ambient occlusion compile,
    // without the environment lookups and their includes around it
    const auto library = "#define YUP_3D_AMBIENT_OCCLUSION\n"
                       + File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory()
                             .getChildFile ("modules/yup_3d/shaders/lib/probes.glsl")
                             .loadFileAsString()
                             .fromFirstOccurrenceOf ("#ifdef YUP_3D_AMBIENT_OCCLUSION", true, false)
                             .upToLastOccurrenceOf ("#endif", false, false);
    ASSERT_TRUE (library.contains ("gtaoSpecularOcclusion"));

    // Red: full visibility hides nothing, even at grazing. Green: no visibility hides everything.
    // Blue: an open floor (visibility 250 / 255) keeps a smooth reflection 85 degrees off its normal
    const auto bounds = mapTexels (GeometryBindings::createSolidTexture (device, 0xffffffff), "vec4("
                                   "gtaoSpecularOcclusion(0.05, 1.0, 0.8), "
                                   "gtaoSpecularOcclusion(0.5, 0.0, 0.5), "
                                   "gtaoSpecularOcclusion(cos(radians(85.0)), 250.0 / 255.0, 0.1), 1.0)",
                                   library);
    ASSERT_TRUE (bounds.isValid());
    EXPECT_EQ (bounds.getPixelColor (0, 0).getRed(), 255);
    EXPECT_EQ (bounds.getPixelColor (0, 0).getGreen(), 0);
    EXPECT_GE (bounds.getPixelColor (0, 0).getBlue(), 230);

    // Over n.v (columns, 0 to 1) and visibility (rows, 0.9 to 1, the open surfaces), for a smooth and
    // a rough lobe: continuous, and rising with the visibility
    GpuTextureDesc desc (64, 64, GpuTextureFormat::rgba8unorm, true);
    const auto grid = GpuTexture::create (device, desc);
    ASSERT_NE (grid, nullptr);

    const auto sweep = mapTexels (grid, "vec4("
                                  "gtaoSpecularOcclusion((gl_FragCoord.x - 0.5) / 63.0, 0.9 + 0.1 * (gl_FragCoord.y - 0.5) / 63.0, 0.1), "
                                  "gtaoSpecularOcclusion((gl_FragCoord.x - 0.5) / 63.0, 0.9 + 0.1 * (gl_FragCoord.y - 0.5) / 63.0, 0.7), 0.0, 1.0)",
                                  library);
    ASSERT_TRUE (sweep.isValid());

    auto largestStep = 0, smallestGrazing = 255, notRising = 0;
    for (int y = 0; y < 64; ++y)
    {
        for (int x = 0; x < 64; ++x)
        {
            const auto c = sweep.getPixelColor (x, y);
            if (x > 0)
                largestStep = jmax (largestStep, std::abs (static_cast<int> (c.getRed()) - static_cast<int> (sweep.getPixelColor (x - 1, y).getRed())));
            if (y > 0)
                largestStep = jmax (largestStep, std::abs (static_cast<int> (c.getRed()) - static_cast<int> (sweep.getPixelColor (x, y - 1).getRed())));

            // Grazing smooth reflections, 80 to 88 degrees off the normal, at visibility 0.98 and above
            const auto nDotV = static_cast<float> (x) / 63.0f, visibility = 0.9f + 0.1f * static_cast<float> (y) / 63.0f;
            if (nDotV >= std::cos (degreesToRadians (88.0f)) && nDotV <= std::cos (degreesToRadians (80.0f)) && visibility >= 0.98f)
                smallestGrazing = jmin (smallestGrazing, static_cast<int> (c.getRed()));

            if (y > 0)
            {
                notRising += c.getRed() + 1 < sweep.getPixelColor (x, y - 1).getRed() ? 1 : 0;
                notRising += c.getGreen() + 1 < sweep.getPixelColor (x, y - 1).getGreen() ? 1 : 0;
            }
        }
    }

    RecordProperty ("largestStep", largestStep);
    RecordProperty ("smallestGrazing", smallestGrazing);
    EXPECT_LE (largestStep, 8);
    EXPECT_GE (smallestGrazing, 230);
    EXPECT_EQ (notRising, 0);
}

TEST_F (SceneRendererMetalTests, AmbientOcclusionUpsampleReadsWhereEachTexelWasTraced)
{
    // Texel i of the half resolution ambient occlusion was traced at the full resolution texel 2i:
    // that fragment must read the center of texel i, and texel 2i + 1 halfway to the next one
    const auto library = File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory()
                             .getChildFile ("modules/yup_3d/shaders/lib/probes.glsl")
                             .loadFileAsString()
                             .fromFirstOccurrenceOf ("#ifdef YUP_3D_AMBIENT_OCCLUSION", true, false)
                             .upToLastOccurrenceOf ("#endif", false, false);

    // Red: 100 + 50 * (the texel coordinate read for fragment 2i, minus i + 0.5); green: the same for 2i + 1, minus i + 1
    const auto mapped = mapTexels (GeometryBindings::createSolidTexture (device, 0xffffffff),
                                   "vec4((100.0 + 50.0 * (gtaoUpsampleUv(vec2(10.5), vec2(32.0)).x * 32.0 - 5.5)) / 255.0, "
                                   "(100.0 + 50.0 * (gtaoUpsampleUv(vec2(11.5), vec2(32.0)).y * 32.0 - 6.0)) / 255.0, 0.0, 1.0)",
                                   "#define YUP_3D_AMBIENT_OCCLUSION\n" + library);
    ASSERT_TRUE (mapped.isValid());
    EXPECT_NEAR (mapped.getPixelColor (0, 0).getRed(), 100, 1);
    EXPECT_NEAR (mapped.getPixelColor (0, 0).getGreen(), 100, 1);
}

TEST_F (SceneRendererMetalTests, NonFiniteCheckFindsNanAndInfinity)
{
    // Half floats: 1.0, NaN and +infinity
    const auto check = [this] (uint16 bits)
    {
        std::vector<uint16> texels (4 * 4 * 4, 0x3c00);
        texels[5 * 4 + 1] = bits;

        auto texture = GpuTexture::create (device, GpuTextureDesc (4, 4, GpuTextureFormat::rgba16float));
        if (texture == nullptr)
            return false;

        GpuTextureDataDesc data;
        data.data = texels.data();
        return texture->upload (data) && isFiniteTexture (texture);
    };

    EXPECT_TRUE (check (0x3c00));
    EXPECT_FALSE (check (0x7e00));
    EXPECT_FALSE (check (0x7c00));
}

TEST_F (SceneRendererMetalTests, VelocityIsZeroForAStaticSceneAndCameraMotionOtherwise)
{
    Scene scene;
    buildShadowsScene (scene);
    setTier (scene, RenderSettings::TierPreference::full);

    auto camera = EntityNode::Ptr (new EntityNode ("camera"));
    camera->attach<CameraNode>();
    camera->setPosition ({ 0.0f, 0.0f, 5.0f });
    scene.getRoot()->addChild (camera);
    scene.setActiveCamera (camera.get());

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto still = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
    ASSERT_TRUE (still.isValid());
    EXPECT_EQ (maxVelocityDeviation (still), 0);

    // Velocity is current uv minus previous uv, with uv from the top-left. Moving the camera right
    // moves the scene left on screen: x decreases, y is unchanged
    camera->setPosition ({ 0.1f, 0.0f, 5.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto right = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
    ASSERT_TRUE (right.isValid());
    EXPECT_LT (right.getPixelColor (32, 32).getRed(), 64);
    EXPECT_EQ (velocityDeviation (right.getPixelColor (32, 32).getGreen()), 0);

    // Moving the camera up moves the scene down on screen, towards larger uv.y: a missing y flip fails here
    camera->setPosition ({ 0.1f, 0.1f, 5.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto up = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
    ASSERT_TRUE (up.isValid());
    EXPECT_GT (up.getPixelColor (32, 32).getGreen(), 192);
    EXPECT_EQ (velocityDeviation (up.getPixelColor (32, 32).getRed()), 0);
}

TEST_F (SceneRendererMetalTests, InfiniteFarPlaneSkyMovesWithTheCameraRotation)
{
    // Turning a glTF camera without zfar (an infinite far plane) moves the sky on screen exactly as
    // it does through the same camera with a distant far plane
    const auto renderTurning = [this] (float zFar)
    {
        Scene scene;
        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
        scene.setEnvironmentVisible (true);
        setTier (scene, RenderSettings::TierPreference::full);

        auto camera = EntityNode::Ptr (new EntityNode ("camera"));
        auto& node = camera->attach<CameraNode>();
        node.zNear = 0.05f;
        node.zFar = zFar;
        scene.getRoot()->addChild (camera);
        scene.setActiveCamera (camera.get());

        SceneRenderer renderer;
        EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

        camera->setRotation (Quaternion::fromAxisAngle (Vector3<float> (0.3f, 1.0f, 0.0f).normalized(), 0.05f));
        EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

        return readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 4.0f);
    };

    const auto infinite = renderTurning (0.0f);
    const auto finite = renderTurning (1.0e5f);
    ASSERT_TRUE (infinite.isValid());
    ASSERT_TRUE (finite.isValid());

    // A turn of 0.05 rad moves the sky by about 0.06 of the frame: not vacuous
    EXPECT_GT (velocityDeviation (finite.getPixelColor (32, 32).getRed()), 20);
    EXPECT_GT (velocityDeviation (infinite.getPixelColor (32, 32).getRed()), 20);
    EXPECT_EQ (countDifferences (infinite, finite), 0);
}

TEST_F (SceneRendererMetalTests, BlendedItemsKeepTheDeferredVelocityAroundThem)
{
    // The blended items draw over the G-buffer velocity: texels outside them keep the motion the
    // G-buffer wrote, which a cleared velocity attachment would zero
    Scene scene;
    buildShadowsScene (scene);
    setTier (scene, RenderSettings::TierPreference::full);

    std::vector<Mesh::Vertex> vertices (4);
    vertices[0].position = { -0.9f, 0.5f, 0.6f };
    vertices[1].position = { -0.5f, 0.5f, 0.6f };
    vertices[2].position = { -0.5f, 0.9f, 0.6f };
    vertices[3].position = { -0.9f, 0.9f, 0.6f };
    for (auto& vertex : vertices)
        vertex.normal = { 0.0f, 0.0f, 1.0f };

    auto material = Material::Ptr (new Material());
    material->baseColorFactor = { 0.0f, 0.0f, 1.0f, 0.5f };
    material->alphaMode = Material::AlphaMode::blend;

    auto mesh = Mesh::Ptr (new Mesh ("blended"));
    mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);
    auto blended = EntityNode::Ptr (new EntityNode ("blended"));
    blended->attach<MeshNode> (mesh);
    scene.getRoot()->addChild (blended);

    auto camera = EntityNode::Ptr (new EntityNode ("camera"));
    camera->attach<CameraNode>();
    camera->setPosition ({ 0.0f, 0.0f, 5.0f });
    scene.getRoot()->addChild (camera);
    scene.setActiveCamera (camera.get());

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    // Moving the camera right moves the scene left on screen: x decreases everywhere
    camera->setPosition ({ 0.1f, 0.0f, 5.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto moved = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
    ASSERT_TRUE (moved.isValid());

    // The opaque plate at the center, away from the blended quad in the top-left corner, then the quad
    EXPECT_LT (moved.getPixelColor (40, 40).getRed(), 64);
    EXPECT_LT (moved.getPixelColor (20, 20).getRed(), 64);
}

TEST_F (SceneRendererMetalTests, TwoRenderersKeepIndependentHistory)
{
    Scene scene;
    buildShadowsScene (scene);
    setTier (scene, RenderSettings::TierPreference::full);

    auto* plates = scene.getRoot()->findChild ("plates");
    ASSERT_NE (plates, nullptr);

    const auto velocityAfterMove = [&] (SceneRenderer& renderer, float x)
    {
        plates->setPosition ({ x, 0.0f, 0.0f });
        if (renderer.render (device, scene, 64, 64) == nullptr)
            return -1;

        const auto velocity = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
        return velocity.isValid() ? maxVelocityDeviation (velocity) : -1;
    };

    // A single renderer sees the plates move between its frames
    SceneRenderer single;
    EXPECT_EQ (velocityAfterMove (single, 0.0f), 0);
    EXPECT_GT (velocityAfterMove (single, 0.1f), 64);

    // Alternating renderers never take the other's transform as their previous frame
    SceneRenderer a;
    SceneRenderer b;
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ (velocityAfterMove (a, 0.1f * static_cast<float> (2 * i)), 0) << "a, frame " << i;
        EXPECT_EQ (velocityAfterMove (b, 0.1f * static_cast<float> (2 * i + 1)), 0) << "b, frame " << i;
    }
}

TEST_F (SceneRendererMetalTests, FirstFrameHasNoObjectMotion)
{
    // A transform set before the first render is not motion, whatever the node held before
    Scene scene;
    buildShadowsScene (scene);
    setTier (scene, RenderSettings::TierPreference::full);
    scene.getRoot()->findChild ("plates")->setPosition ({ 0.3f, 0.2f, 0.0f });

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto velocity = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
    ASSERT_TRUE (velocity.isValid());
    EXPECT_EQ (maxVelocityDeviation (velocity), 0);
}

TEST_F (SceneRendererMetalTests, NodeReappearingMovedHasNoMotion)
{
    Scene scene;
    buildShadowsScene (scene);
    setTier (scene, RenderSettings::TierPreference::full);

    auto* plates = scene.getRoot()->findChild ("plates");
    ASSERT_NE (plates, nullptr);

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    // Hidden for one frame, then shown somewhere else: its last drawn transform is two frames old
    plates->setVisible (false);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    plates->setVisible (true);
    plates->setPosition ({ 0.1f, 0.0f, 0.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto velocity = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
    ASSERT_TRUE (velocity.isValid());
    EXPECT_EQ (maxVelocityDeviation (velocity), 0);
}

TEST_F (SceneRendererMetalTests, FailedRenderClearsThePassOutputs)
{
    Scene scene;
    buildShadowsScene (scene);
    setTier (scene, RenderSettings::TierPreference::full);

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor), nullptr);
    EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), nullptr);
    EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::gbufferNormal), nullptr);

    EXPECT_EQ (renderer.render (device, scene, 0, 64), nullptr);
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor), nullptr);
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), nullptr);
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::gbufferNormal), nullptr);

    // Rendering again would fail again: nothing to wait for
    EXPECT_TRUE (renderer.isConverged());
}

TEST_F (SceneRendererMetalTests, ResizeResetsAccumulation)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 4);

    SceneRenderer renderer;
    for (int i = 0; i < 4; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    ASSERT_NE (renderer.render (device, scene, 80, 80), nullptr) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());

    const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
    ASSERT_NE (sceneColor, nullptr);
    EXPECT_EQ (sceneColor->getWidth(), 80);
    EXPECT_TRUE (isFiniteTexture (sceneColor));
}

TEST_F (SceneRendererMetalTests, NonFiniteInputDoesNotPoisonHistory)
{
    // A huge point light just above a plate corner, a zero-area triangle, and a blended quad with
    // zero normals, a NaN emissive factor and a NaN alpha. Metal saturates overflowing half floats,
    // and max() and the clamp of the shaded color drop NaN, so the alpha is what reliably reaches
    // the scene color, through the blending
    const auto buildScene = [] (Scene& scene, int taaSamples)
    {
        scene.getRoot()->addChild (makePlates());

        std::vector<Mesh::Vertex> vertices (7);
        for (auto& vertex : vertices)
        {
            vertex.position = { 0.2f, 0.2f, 0.6f };
            vertex.normal = {};
        }

        vertices[3].position = { -0.6f, -0.6f, 0.6f };
        vertices[4].position = { -0.4f, -0.6f, 0.6f };
        vertices[5].position = { -0.4f, -0.4f, 0.6f };
        vertices[6].position = { -0.6f, -0.4f, 0.6f };

        auto material = Material::Ptr (new Material());
        material->emissiveFactor = { std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f };
        material->baseColorFactor[3] = std::numeric_limits<float>::quiet_NaN();
        material->alphaMode = Material::AlphaMode::blend;

        auto mesh = Mesh::Ptr (new Mesh ("degenerate"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 3, 4, 5, 3, 5, 6 }, material);
        auto degenerate = EntityNode::Ptr (new EntityNode ("degenerate"));
        degenerate->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (degenerate);

        auto light = EntityNode::Ptr (new EntityNode ("point"));
        auto& node = light->attach<LightNode>();
        node.type = LightNode::Type::point;
        node.range = 0.0f;
        node.intensity = 1.0e8f;
        light->setPosition ({ 1.0f, 1.0f, 0.01f });
        scene.getRoot()->addChild (light);

        setFullTierTaa (scene, taaSamples);
    };

    // Without temporal anti-aliasing the scene color really holds non-finite values
    {
        Scene scene;
        buildScene (scene, 1);

        SceneRenderer renderer;
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        ASSERT_FALSE (isFiniteTexture (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)));
    }

    Scene scene;
    buildScene (scene, 16);

    SceneRenderer renderer;
    for (int i = 0; i < 8; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_TRUE (isFiniteTexture (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)));
}

TEST_F (SceneRendererMetalTests, FullTierWithoutTaaKeepsOverflowingHighlightsFinite)
{
    // Without TAA or bloom nothing sanitizes the scene color before tone mapping. Metal saturates
    // half floats written above their range, so radiance just past 65504 can't show the overflow
    // here; a light this strong overflows the 32 bit shading math itself to infinity
    Scene scene;
    scene.getRoot()->addChild (makePlates());

    auto light = EntityNode::Ptr (new EntityNode ("point"));
    auto& node = light->attach<LightNode>();
    node.type = LightNode::Type::point;
    node.range = 0.0f;
    node.intensity = 1.0e38f;
    light->setPosition ({ 0.5f, 0.5f, 0.05f });
    scene.getRoot()->addChild (light);

    setFullTierTaa (scene, 1);

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_FALSE (renderer.getCapabilities()->isEnabled (RenderCapabilities::Feature::temporalAntiAliasing, scene.getRenderSettings()));
    EXPECT_TRUE (isFiniteTexture (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)));
}
#endif

TEST_F (SceneRendererMetalTests, DeferredMatchesForwardForOpaque)
{
    // The lighting pass shades the G-buffer with the BSDF and the light code of the forward pass:
    // the interior of each baseline scene matches a forced raster frame, away from the edges where
    // MSAA and TAA differ. The mean over the interior catches global drift, the 8x8 blocks of
    // opaque texels catch a local one. The blended quad of the materials scene is drawn forward on
    // both tiers but blended in linear light on full only, so its blocks are left out. The baseline
    // scenes are compact enough for one scene-fitted sun map; the long ground keeps two view-fitted
    // cascades, so the raster blend and the full dither between them are compared too
    const struct
    {
        const char* name;
        void (*build) (Scene&);
        Rectangle<int> blended;
        bool viewFitted = false; // keeps the view-fitted cascades, so the tiers compare their handover too
    } scenes[] = {
        { "shadows", buildShadowsScene, {} },
        { "materials", buildMaterialsScene, { 74, 53, 24, 24 } },
        { "areaLights", buildAreaLightsScene, {} },
        { "longGround", [] (Scene& scene) { buildLongGroundScene (scene, 2.0f); }, {}, true },
    };

    for (const auto& [name, build, blended, viewFitted] : scenes)
    {
        Image images[2];

        for (const auto tier : { RenderSettings::TierPreference::full, RenderSettings::TierPreference::raster })
        {
            // Both tiers shadow alike: the raster cascades, no soft shadows, the full frame converged
            Scene scene;
            build (scene);
            setTier (scene, tier);
            pinCrossTierShadows (scene);

            SceneRenderer renderer;
            auto& image = images[tier == RenderSettings::TierPreference::full ? 0 : 1];
            image = renderConverged (renderer, scene, 128);
            ASSERT_TRUE (image.isValid()) << name << ": " << renderer.getLastError();
            ASSERT_EQ (renderer.getCapabilities()->getTier() == RenderCapabilities::Tier::full, tier == RenderSettings::TierPreference::full) << name;

            if (viewFitted)
                EXPECT_EQ (shadows (renderer).getShadowData().sunInfo[0], 2.0f) << name;
        }

        const auto full = meanLuminance (images[0], { 32, 32, 64, 64 });
        const auto raster = meanLuminance (images[1], { 32, 32, 64, 64 });
        const auto difference = std::abs (full - raster) / raster;
        RecordProperty ((String (name) + "Difference").toStdString(), String (difference * 100.0, 2).toStdString());
        EXPECT_LE (difference, 0.03) << name << ": full " << full << ", raster " << raster;

        auto largestBlockDifference = 0.0;
        for (int y = 32; y < 96; y += 8)
        {
            for (int x = 32; x < 96; x += 8)
            {
                const Rectangle<int> block (x, y, 8, 8);
                if (block.intersects (blended))
                    continue;

                largestBlockDifference = jmax (largestBlockDifference, std::abs (meanLuminance (images[0], block) - meanLuminance (images[1], block)));
            }
        }

        RecordProperty ((String (name) + "LargestBlockDifference").toStdString(), String (largestBlockDifference, 2).toStdString());
        // Measured at most 1.7 levels of luma (half float roughness, 8 bit base color, the converged
        // TAA average); a local shading error moves a block by tens of levels
        EXPECT_LE (largestBlockDifference, 4.0) << name;
    }
}

TEST_F (SceneRendererMetalTests, FullTierShadesManyLights)
{
    // 256 lights on a single draw: more than any per-draw light list holds, so the full tier must
    // find each fragment's lights through the clusters
    Scene scene;
    buildLightGridScene (scene, true);
    setTier (scene, RenderSettings::TierPreference::full);

    SceneRenderer renderer;
    const auto image = renderToImage (renderer, scene, 256);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();

    const auto lit = countLitGridQuads (image);
    RecordProperty ("litQuads", lit);
    EXPECT_EQ (lit, 256);
}

TEST_F (SceneRendererMetalTests, RasterLightSelectionDoesNotPopUnderMovingLights)
{
    // 32 lights orbiting over 9 pads: each pad has more candidates than the 8 slots of a raster
    // draw, and lights crossing the selection cutoff or their range must fade instead of switching
    // on or off within a frame. The full tier shades every light through the clusters: its largest
    // change between two frames is what the motion alone produces
    const auto measureLargestFrameChange = [this] (RenderSettings::TierPreference tier)
    {
        Scene scene;
        const auto lights = buildOrbitingLightsScene (scene);
        setFullTierTaa (scene, 1);
        setTier (scene, tier);

        SceneRenderer renderer;
        Image previous;
        int largest = 0;

        for (int frame = 0; frame < 90; ++frame)
        {
            moveOrbitingLights (lights, frame / 30.0);

            const auto image = renderToImage (renderer, scene, 128);
            EXPECT_TRUE (image.isValid()) << renderer.getLastError();
            if (! image.isValid())
                return -1;

            if (previous.isValid())
                largest = jmax (largest, maxChannelDifference (image, previous));

            previous = image;
        }

        return largest;
    };

    const auto raster = measureLargestFrameChange (RenderSettings::TierPreference::raster);
    const auto full = measureLargestFrameChange (RenderSettings::TierPreference::full);
    ASSERT_GT (full, 0);

    RecordProperty ("rasterLargestFrameChange", raster);
    RecordProperty ("fullLargestFrameChange", full);
    EXPECT_LE (raster, 2 * full);
}

TEST_F (SceneRendererMetalTests, FullTierRendersWithoutAnyLight)
{
    // No light and no default light: the light and cluster buffers stay empty, emission still shows.
    // A light buffer smaller than one GpuLight renders fine without validation: this test only
    // fails on it under Metal API validation (MTL_DEBUG_LAYER=1)
    Scene scene;
    buildGlowScene (scene, 0.5f);
    scene.setUsingDefaultLight (false);
    setTier (scene, RenderSettings::TierPreference::full);

    SceneRenderer renderer;
    const auto image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_GT (image.getPixelColor (64, 64).getRed(), 100);
    EXPECT_EQ (image.getPixelColor (4, 4).getRed(), 0);
}

TEST_F (SceneRendererMetalTests, BackgroundIsKeptBehindLitPixels)
{
    // A corner shows the environment: 0.5 through ACES is 0.616, 206 once sRGB encoded
    {
        Scene scene;
        buildShadowsScene (scene);
        setTier (scene, RenderSettings::TierPreference::full);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        const auto corner = image.getPixelColor (1, 1);
        EXPECT_NEAR (corner.getRed(), 206, 3);
        EXPECT_NEAR (corner.getGreen(), 206, 3);
        EXPECT_NEAR (corner.getBlue(), 206, 3);
    }

    // A red sky over a blue ground: the full tier shows each where the raster tier does
    Image images[2];

    for (const auto tier : { RenderSettings::TierPreference::full, RenderSettings::TierPreference::raster })
    {
        Scene scene;
        buildShadowsScene (scene);
        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>& d)
        {
            return d.getY() > 0.0f ? Vector3<float> (0.8f, 0.1f, 0.05f) : Vector3<float> (0.05f, 0.1f, 0.8f);
        }, 32, 3));
        setTier (scene, tier);

        SceneRenderer renderer;
        images[tier == RenderSettings::TierPreference::full ? 0 : 1] = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (images[tier == RenderSettings::TierPreference::full ? 0 : 1].isValid()) << renderer.getLastError();
    }

    const auto top = images[0].getPixelColor (1, 1);
    const auto bottom = images[0].getPixelColor (1, 62);
    EXPECT_GT (top.getRed(), top.getBlue() + 50);
    EXPECT_GT (bottom.getBlue(), bottom.getRed() + 50);

    for (const auto& [x, y] : { std::pair { 1, 1 }, std::pair { 1, 62 }, std::pair { 62, 1 }, std::pair { 62, 62 } })
    {
        const auto full = images[0].getPixelColor (x, y);
        const auto raster = images[1].getPixelColor (x, y);
        EXPECT_NEAR (full.getRed(), raster.getRed(), 3) << x << ", " << y;
        EXPECT_NEAR (full.getGreen(), raster.getGreen(), 3) << x << ", " << y;
        EXPECT_NEAR (full.getBlue(), raster.getBlue(), 3) << x << ", " << y;
    }
}

TEST_F (SceneRendererMetalTests, SoftShadowFilterFollowsTheTierAndTaa)
{
    // GpuShadowData::filterInfo = (soft, filter taps, blocker taps, flags) per preflight R18: 8 + 8
    // rotating taps where temporal anti-aliasing averages them (flag 2), 16 + 16 still ones on full
    // without it, 8 + 8 still ones for raster's opt-in, and none without soft shadows or without a
    // shadowed light that has a size
    const struct
    {
        const char* name;
        RenderSettings::TierPreference tier;
        int taaSamples;
        std::optional<bool> softShadows;
        float radius;
        float taps;
        bool temporal;
    } cases[] = {
        { "full, TAA", RenderSettings::TierPreference::full, 16, {}, 0.05f, 8.0f, true },
        { "full, no TAA", RenderSettings::TierPreference::full, 1, {}, 0.05f, 16.0f, false },
        { "full, soft off", RenderSettings::TierPreference::full, 16, false, 0.05f, 0.0f, true },
        { "full, punctual sun", RenderSettings::TierPreference::full, 16, {}, 0.0f, 0.0f, true },
        { "raster opt-in", RenderSettings::TierPreference::raster, 16, true, 0.05f, 8.0f, false },
        { "raster default", RenderSettings::TierPreference::raster, 16, {}, 0.05f, 0.0f, false },
    };

    for (const auto& c : cases)
    {
        Scene scene;
        buildPenumbraScene (scene, c.radius);

        auto settings = scene.getRenderSettings();
        settings.tier = c.tier;
        settings.taaSamples = c.taaSamples;
        settings.softShadows = c.softShadows;
        scene.setRenderSettings (settings);

        SceneRenderer renderer;
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << c.name << ": " << renderer.getLastError();

        const auto shadowData = shadows (renderer).getShadowData();
        const auto& filter = shadowData.filterInfo;
        EXPECT_EQ (filter[0], c.taps > 0.0f ? 1.0f : 0.0f) << c.name;
        EXPECT_EQ (filter[1], c.taps) << c.name;
        EXPECT_EQ (filter[2], c.taps) << c.name;
        EXPECT_EQ ((static_cast<int> (filter[3]) & 2) != 0, c.temporal) << c.name;
    }
}

class SceneRendererTierTests : public SceneRendererMetalTests
    , public ::testing::WithParamInterface<RenderSettings::TierPreference>
{
protected:
#if YUP_ENABLE_SHADER_TRANSPILER
    /** Renders a 2x2 plate, u along +x and v down, lit by a sun turned by @p sunRotation, and returns
        the mean luma of its middle half at 128x128. The plate uses @p tangent at every vertex, a 1x1
        normal map of @p normalTexel if there is one, and an x scale of @p xScale.
        Scene color holds raw radiance and is checked for non-finite and saturated values.
    */
    double renderNormalMappedPlate (std::array<float, 4> tangent, std::optional<Color> normalTexel, const Quaternion& sunRotation, float xScale = 1.0f)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -1.0f, -1.0f, 0.0f };
        vertices[1].position = { 1.0f, -1.0f, 0.0f };
        vertices[2].position = { 1.0f, 1.0f, 0.0f };
        vertices[3].position = { -1.0f, 1.0f, 0.0f };
        vertices[0].uv = { 0.0f, 1.0f };
        vertices[1].uv = { 1.0f, 1.0f };
        vertices[2].uv = { 1.0f, 0.0f };
        vertices[3].uv = { 0.0f, 0.0f };

        for (auto& vertex : vertices)
        {
            vertex.normal = { 0.0f, 0.0f, 1.0f };
            vertex.tangent = tangent;
        }

        auto material = Material::Ptr (new Material());
        material->metallicFactor = 0.0f;

        if (normalTexel.has_value())
        {
            Image normal (1, 1, PixelFormat::RGBA);
            normal.setPixelColor (0, 0, *normalTexel);
            material->normalTexture = Texture::Ptr (new Texture (std::move (normal)));
        }

        auto mesh = Mesh::Ptr (new Mesh ("plate"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);

        Scene scene;
        auto plate = EntityNode::Ptr (new EntityNode ("plate"));
        plate->attach<MeshNode> (mesh);
        plate->setScale ({ xScale, 1.0f, 1.0f });
        scene.getRoot()->addChild (plate);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>();
        sun->setRotation (sunRotation);
        scene.getRoot()->addChild (sun);

        // Scene color must hold raw radiance: no temporal anti-aliasing on full, rgba16float on raster
        if (GetParam() == RenderSettings::TierPreference::full)
            setFullTierTaa (scene, 1);
        else
        {
            setTier (scene, RenderSettings::TierPreference::raster);
            setBloom (scene, true);
        }

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 128);
        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        if (! image.isValid() || sceneColor == nullptr)
        {
            ADD_FAILURE() << renderer.getLastError();
            return 0.0;
        }

        EXPECT_EQ (sceneColor->getFormat(), GpuTextureFormat::rgba16float);
        EXPECT_TRUE (isFiniteTexture (sceneColor));
        EXPECT_FALSE (isSaturatedTexture (sceneColor));

        // The plate covers about 60 px of the frame
        return meanLuminance (image, { 49, 49, 30, 30 });
    }

    /** renderLuma() on the tier of the test parameter. */
    std::vector<double> renderLuma (Scene& scene, int size)
    {
        return SceneRendererBaselineScenes::renderLuma (scene, size, GetParam());
    }
#endif
};

TEST_P (SceneRendererTierTests, ReportsTheRequestedTier)
{
    Scene scene;
    buildEdgesScene (scene);
    setTier (scene, GetParam());

    SceneRenderer renderer;
    EXPECT_EQ (renderer.getCapabilities(), nullptr);

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto* capabilities = renderer.getCapabilities();
    ASSERT_NE (capabilities, nullptr);
    EXPECT_EQ (capabilities->getTier(), GetParam() == RenderSettings::TierPreference::full ? RenderCapabilities::Tier::full : RenderCapabilities::Tier::raster);
}

TEST_P (SceneRendererTierTests, AreaLightVariantFollowsTheLightRadius)
{
    // A sun mirrored off a smooth black plate turns from punctual into a disk and back within one
    // renderer, each frame shading with the variant its lights need. The punctual variant ignores
    // the radius, so only the area light variant widens the highlight; the punctual frames before
    // and after the disk match, and the disk frame matches a renderer drawing it from the start
    auto material = Material::Ptr (new Material());
    material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
    material->metallicFactor = 0.0f;
    material->roughnessFactor = 0.1f;

    Scene scene;
    addPlate (scene, material);
    scene.setAmbientColor (Colors::black);

    auto sun = EntityNode::Ptr (new EntityNode ("sun"));
    auto& light = sun->attach<LightNode>();
    scene.getRoot()->addChild (sun);

    // Without temporal anti-aliasing every full tier frame stands on its own
    if (GetParam() == RenderSettings::TierPreference::full)
        setFullTierTaa (scene, 1);
    else
        setTier (scene, GetParam());

    SceneRenderer renderer;
    std::array<Image, 3> frames;
    const std::array<float, 3> radii { 0.0f, 0.05f, 0.0f };

    for (size_t i = 0; i < frames.size(); ++i)
    {
        light.radius = radii[i];
        frames[i] = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (frames[i].isValid()) << i << ": " << renderer.getLastError();
    }

    // The canvas read back dithers the frames by a level, within what countDifferences() ignores
    EXPECT_EQ (countDifferences (frames[0], frames[2]), 0);
    EXPECT_GT (countDifferences (frames[1], frames[0]), 0);

    light.radius = 0.05f;
    SceneRenderer fresh;
    const auto disk = renderToImage (fresh, scene, 128);
    ASSERT_TRUE (disk.isValid()) << fresh.getLastError();
    EXPECT_EQ (countDifferences (frames[1], disk), 0);
}

TEST_P (SceneRendererTierTests, BloomBrightensAroundAnEmissiveQuad)
{
    const auto tierName = GetParam() == RenderSettings::TierPreference::full ? String ("full") : String ("raster");

    Scene scene;
    buildGlowScene (scene);
    setTier (scene, GetParam());

    SceneRenderer withoutBloom;
    const auto off = renderToImage (withoutBloom, scene, 128);
    ASSERT_TRUE (off.isValid()) << withoutBloom.getLastError();

    const auto quad = findBrightBounds (off);
    ASSERT_GE (quad.getWidth(), 12);
    ASSERT_GE (quad.getHeight(), 12);

    setBloom (scene, true);

    SceneRenderer withBloom;
    const auto on = renderToImage (withBloom, scene, 128);
    ASSERT_TRUE (on.isValid()) << withBloom.getLastError();
    EXPECT_TRUE (withBloom.getCapabilities()->isEnabled (RenderCapabilities::Feature::bloom, scene.getRenderSettings()));

    const auto ringOff = ringLuminance (off, quad);
    const auto ringOn = ringLuminance (on, quad);
    const auto inside = meanLuminance (on, quad.reduced (2));
    RecordProperty ((tierName + "RingOff").toStdString(), String (ringOff, 2).toStdString());
    RecordProperty ((tierName + "RingOn").toStdString(), String (ringOn, 2).toStdString());
    RecordProperty ((tierName + "Inside").toStdString(), String (inside, 2).toStdString());

    EXPECT_LT (ringOff, 1.0);
    EXPECT_GE (ringOn, 5.0);
    EXPECT_GE (inside, 245.0);
}

TEST_P (SceneRendererTierTests, BloomThresholdAboveTheHighlightAddsNoGlow)
{
    Scene scene;
    buildGlowScene (scene);
    setTier (scene, GetParam());

    SceneRenderer withoutBloom;
    const auto off = renderToImage (withoutBloom, scene, 128);
    ASSERT_TRUE (off.isValid()) << withoutBloom.getLastError();

    // The quad emits 20: a threshold of 50 keeps nothing, even with the soft knee from 25
    auto settings = scene.getRenderSettings();
    settings.bloom = true;
    settings.bloomThreshold = 50.0f;
    settings.bloomIntensity = 1.0f;
    scene.setRenderSettings (settings);

    SceneRenderer withBloom;
    const auto on = renderToImage (withBloom, scene, 128);
    ASSERT_TRUE (on.isValid()) << withBloom.getLastError();
    ASSERT_TRUE (withBloom.getCapabilities()->isEnabled (RenderCapabilities::Feature::bloom, settings));

    const auto ring = ringLuminance (on, findBrightBounds (off));
    RecordProperty ("ring", String (ring, 2).toStdString());
    EXPECT_LT (ring, 1.0);
}

TEST_P (SceneRendererTierTests, EmissiveStrengthScalesEmission)
{
    // An unlit black metal plate: no diffuse and no reflectance, only its emission
    const auto renderCenter = [this] (float emission, float strength)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        material->emissiveFactor = { emission, emission, emission };
        material->emissiveStrength = strength;

        Scene scene;
        addPlate (scene, material);
        scene.setUsingDefaultLight (false);
        scene.setExposure (1.0f);
        scene.setToneMapping (Scene::ToneMapping::reinhard);
        setTier (scene, GetParam());

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();
        return image.isValid() ? image.getPixelColor (32, 32) : Color();
    };

    const auto plain = renderCenter (0.1f, 1.0f);
    const auto strengthened = renderCenter (0.1f, 4.0f);
    const auto reference = renderCenter (0.4f, 1.0f);
    RecordProperty ("plain", static_cast<int> (plain.getRed()));
    RecordProperty ("strengthened", static_cast<int> (strengthened.getRed()));

    EXPECT_NEAR (strengthened.getRed(), reference.getRed(), 2);
    EXPECT_NEAR (strengthened.getGreen(), reference.getGreen(), 2);
    EXPECT_NEAR (strengthened.getBlue(), reference.getBlue(), 2);
    EXPECT_GT (strengthened.getRed(), plain.getRed() + 20);
}

TEST_P (SceneRendererTierTests, HigherIorBrightensDielectricSpecular)
{
    // A black dielectric only reflects: under a grazing sun, its highlight follows the reflectance
    const auto renderMean = [this] (float ior)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 0.5f;
        material->ior = ior;

        Scene scene;
        addPlate (scene, material);
        scene.setAmbientColor (Colors::black);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().intensity = 10.0f;
        sun->setRotation (Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, 1.3f));
        scene.getRoot()->addChild (sun);
        setTier (scene, GetParam());

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();
        return image.isValid() ? meanLuminance (image, { 24, 24, 16, 16 }) : -1.0;
    };

    const auto water = renderMean (1.33f);
    const auto diamond = renderMean (2.4f);
    RecordProperty ("water", String (water, 2).toStdString());
    RecordProperty ("diamond", String (diamond, 2).toStdString());

    EXPECT_GT (water, 0.0);
    EXPECT_GT (diamond, water + 5.0);
}

TEST_P (SceneRendererTierTests, SpecularFactorAndColorShapeTheDielectricHighlight)
{
    // The highlight of HigherIorBrightensDielectricSpecular, as its mean red and green levels
    const auto renderHighlight = [this] (float specularFactor, std::array<float, 3> specularColor)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 0.5f;
        material->ior = 2.4f;
        material->specularFactor = specularFactor;
        material->specularColorFactor = specularColor;

        Scene scene;
        addPlate (scene, material);
        scene.setAmbientColor (Colors::black);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().intensity = 10.0f;
        sun->setRotation (Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, 1.3f));
        scene.getRoot()->addChild (sun);
        setTier (scene, GetParam());

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();

        std::pair<double, double> redGreen { -1.0, -1.0 };
        if (! image.isValid())
            return redGreen;

        redGreen = { 0.0, 0.0 };
        for (int y = 24; y < 40; ++y)
        {
            for (int x = 24; x < 40; ++x)
            {
                redGreen.first += image.getPixelColor (x, y).getRed() / 256.0;
                redGreen.second += image.getPixelColor (x, y).getGreen() / 256.0;
            }
        }

        return redGreen;
    };

    const auto white = renderHighlight (1.0f, { 1.0f, 1.0f, 1.0f });
    const auto none = renderHighlight (0.0f, { 1.0f, 1.0f, 1.0f });
    const auto red = renderHighlight (1.0f, { 1.0f, 0.0f, 0.0f });
    RecordProperty ("whiteRed", String (white.first, 2).toStdString());
    RecordProperty ("noneRed", String (none.first, 2).toStdString());
    RecordProperty ("redRed", String (red.first, 2).toStdString());
    RecordProperty ("redGreen", String (red.second, 2).toStdString());

    // A zero factor removes the reflection, grazing incidence included; a red color keeps only red
    EXPECT_GT (white.first, 20.0);
    EXPECT_LE (none.first, 1.0);
    EXPECT_GT (red.first, red.second + 20.0);
}

TEST_P (SceneRendererTierTests, SpecularFactorDimsTheDiffuseOnce)
{
    // An ior of 0 reflects f0 = f90 = specularFactor at every angle. The diffuse is what a white base
    // adds over a black one, whose specular is the same
    const auto diffuseAt = [&] (float specularFactor)
    {
        return renderHeadOnRadiance (makeDielectric (1.0f, 0.0f, specularFactor), GetParam())[1]
             - renderHeadOnRadiance (makeDielectric (0.0f, 0.0f, specularFactor), GetParam())[1];
    };

    const auto unreflected = diffuseAt (0.0f);
    const auto ratio = diffuseAt (0.5f) / unreflected;
    RecordProperty ("unreflected", String (unreflected, 3).toStdString());
    RecordProperty ("ratio", String (ratio, 3).toStdString());

    // 2 / pi without any specular; dimmed by 1 - 0.5, where counting the factor twice gives 1 - 0.25
    EXPECT_NEAR (unreflected, 2.0 / MathConstants<double>::pi, 0.03);
    EXPECT_NEAR (ratio, 0.5, 0.05);
}

TEST_P (SceneRendererTierTests, ColoredSpecularDimsEveryDiffuseChannel)
{
    // A red specular color at an ior of 0 reflects all the red: KHR_materials_specular takes the
    // largest channel, so nothing is left to the diffuse, in green either
    const auto white = renderHeadOnRadiance (makeDielectric (1.0f, 0.0f, 1.0f, { 1.0f, 0.0f, 0.0f }), GetParam());
    const auto black = renderHeadOnRadiance (makeDielectric (0.0f, 0.0f, 1.0f, { 1.0f, 0.0f, 0.0f }), GetParam());
    const auto greenDiffuse = white[1] - black[1];
    RecordProperty ("greenDiffuse", String (greenDiffuse, 3).toStdString());

    // Weighting each channel on its own leaves a green diffuse of about 2 / pi
    EXPECT_LT (greenDiffuse, 0.03);
    EXPECT_NEAR (black[0], 2.0 / 4.0 / MathConstants<double>::pi, 0.03); // The red specular, f0 = 1
}

TEST_P (SceneRendererTierTests, DiffuseLiesUnderTheDielectricLayerOnly)
{
    // Half metallic red: the metal reflects f0 = 1 in red, but the diffuse half sits under the
    // dielectric layer alone (f0 = 0.04), as in the glTF mix of the dielectric and metal BRDFs
    auto material = Material::Ptr (new Material());
    material->baseColorFactor = { 1.0f, 0.0f, 0.0f, 1.0f };
    material->metallicFactor = 0.5f;

    const auto red = renderHeadOnRadiance (material, GetParam())[0];
    const auto f0 = 0.5 * 0.04 + 0.5 * 1.0;
    const auto expected = 2.0 * (f0 / 4.0 + 0.5 * (1.0 - 0.04)) / MathConstants<double>::pi;
    RecordProperty ("red", String (red, 3).toStdString());
    RecordProperty ("expected", String (expected, 3).toStdString());

    // Dimming the diffuse by the mixed reflectance instead (1 - 0.52) gives 0.236
    EXPECT_NEAR (red, expected, 0.02);
}

TEST_P (SceneRendererTierTests, AmbientDiffuseLiesUnderTheDielectricLayer)
{
    // Ambient light only. A white dielectric reflects f0 and diffuses what its layer leaves,
    // 1 - f0: all of the ambient light, whatever its ior. An ior of 0 reflects everything, f0 = 1,
    // where an unweighted diffuse would double the energy
    const auto ambient = Color (static_cast<uint8> (128), static_cast<uint8> (128), static_cast<uint8> (128));
    const auto linearAmbient = std::pow ((128.0 / 255.0 + 0.055) / 1.055, 2.4);

    const auto mirror = renderHeadOnRadiance (makeDielectric (1.0f, 0.0f, 1.0f), GetParam(), 0.0f, ambient)[1];
    const auto plastic = renderHeadOnRadiance (makeDielectric (1.0f, 1.5f, 1.0f), GetParam(), 0.0f, ambient)[1];
    RecordProperty ("mirror", String (mirror, 4).toStdString());
    RecordProperty ("plastic", String (plastic, 4).toStdString());

    EXPECT_NEAR (mirror, linearAmbient, 0.01);
    EXPECT_NEAR (plastic, linearAmbient, 0.01);
}

TEST_P (SceneRendererTierTests, DrawsFindTheirLightsWhenDirectionalLightsArePackedFirst)
{
    // The point light comes first in the list, but the sun is packed first: the plate, out of
    // the point light's range, selects only the sun, which must reach it through the packed order
    auto material = Material::Ptr (new Material());
    material->metallicFactor = 0.0f;

    Scene scene;
    scene.setAmbientColor (Colors::black);
    setTier (scene, GetParam());

    auto lamp = EntityNode::Ptr (new EntityNode ("lamp"));
    auto& point = lamp->attach<LightNode> (LightNode::Type::point);
    point.color = { 1.0f, 0.0f, 0.0f };
    point.intensity = 100.0f;
    point.range = 1.0f;
    lamp->setPosition ({ 10.0f, 0.0f, 1.0f });
    scene.getRoot()->addChild (lamp);

    auto sun = EntityNode::Ptr (new EntityNode ("sun"));
    auto& sunLight = sun->attach<LightNode>();
    sunLight.color = { 0.0f, 1.0f, 0.0f };
    sunLight.intensity = 2.0f;
    scene.getRoot()->addChild (sun);

    addPlate (scene, material);

    SceneRenderer renderer;
    const auto image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();

    const auto center = image.getPixelColor (32, 32);
    EXPECT_GE (static_cast<int> (center.getGreen()), 100);
    EXPECT_LE (static_cast<int> (center.getRed()), 10);
}

TEST_P (SceneRendererTierTests, ShadesMoreThanEightLightsPerScene)
{
    checkShadesMoreThanEightLightsPerScene (GetParam());
}

TEST_P (SceneRendererTierTests, BlendItemsDrawOverDeferred)
{
    // A half transparent blue quad in front of a white plate tints it, and one behind the plate is
    // hidden by the depth the plate wrote
    const auto addQuad = [] (Scene& scene, float x, float z)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { x - 0.3f, -0.3f, z };
        vertices[1].position = { x + 0.3f, -0.3f, z };
        vertices[2].position = { x + 0.3f, 0.3f, z };
        vertices[3].position = { x - 0.3f, 0.3f, z };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.0f, 0.0f, 1.0f, 0.5f };
        material->alphaMode = Material::AlphaMode::blend;

        auto mesh = Mesh::Ptr (new Mesh ("blended"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);

        auto entity = EntityNode::Ptr (new EntityNode ("blended"));
        entity->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (entity);
    };

    auto white = Material::Ptr (new Material());
    white->metallicFactor = 0.0f;

    Scene scene;
    addPlate (scene, white);
    addQuad (scene, 0.0f, 0.5f);
    addQuad (scene, -0.6f, -0.5f);
    setTier (scene, GetParam());

    SceneRenderer renderer;
    const auto image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();

    // The plate alone is gray. The full tier blends in linear light and the raster tier after tone
    // mapping, so the mix differs between them, but both darken the red and keep more blue
    const auto plate = image.getPixelColor (44, 32);
    const auto tinted = image.getPixelColor (32, 32);
    EXPECT_NEAR (plate.getRed(), plate.getBlue(), 4);
    EXPECT_GT (static_cast<int> (tinted.getBlue()), static_cast<int> (tinted.getRed()) + 10);
    EXPECT_LT (static_cast<int> (tinted.getRed()), static_cast<int> (plate.getRed()) - 20);
    EXPECT_GT (tinted.getRed(), 0);

    // The hidden quad would cover about 5 px around (24, 32)
    const auto hidden = image.getPixelColor (24, 32);
    EXPECT_NEAR (hidden.getRed(), plate.getRed(), 4);
    EXPECT_NEAR (hidden.getBlue(), plate.getBlue(), 4);
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_P (SceneRendererTierTests, SceneColorHasNoNaN)
{
    checkSceneColorHasNoNaN (GetParam());
}
#endif

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_P (SceneRendererTierTests, RendersEveryBaselineSceneWithoutNonFiniteValues)
{
    const std::pair<const char*, void (*) (Scene&)> scenes[] = {
        { "shadows", buildShadowsScene },
        { "materials", buildMaterialsScene },
        { "edges", buildEdgesScene },
    };

    const auto full = GetParam() == RenderSettings::TierPreference::full;

    for (const auto& [name, build] : scenes)
    {
        Scene scene;
        build (scene);
        setTier (scene, GetParam());

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (renderer.getLastError().isEmpty()) << name << ": " << renderer.getLastError();
        ASSERT_TRUE (image.isValid()) << name;

        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        const auto velocity = renderer.getPassOutput (SceneRenderer::PassOutput::velocity);
        ASSERT_NE (sceneColor, nullptr) << name;
        EXPECT_EQ (sceneColor->getFormat(), full ? GpuTextureFormat::rgba16float : GpuTextureFormat::rgba8unorm) << name;
        EXPECT_EQ (velocity != nullptr, full) << name;

        if (full)
            EXPECT_TRUE (isFiniteTexture (sceneColor)) << name;
    }
}
#endif

TEST_P (SceneRendererTierTests, DefaultSettingsAddNoOptInPass)
{
    checkDefaultSettingsAddNoOptInPass (GetParam());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_P (SceneRendererTierTests, ZeroTangentFallsBackToDerivativeFrame)
{
    // The normal map tilts the plate towards +x, where a grazing sun shines from: the tangent
    // (1, 0, 0, 1) is the frame the derivatives build
    const auto tiltedTowardsX = Color (191, 128, 247);
    const auto sunFromX = Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 1.2f);

    const auto tangentFrame = renderNormalMappedPlate ({ 1.0f, 0.0f, 0.0f, 1.0f }, tiltedTowardsX, sunFromX);
    const auto derivativeFrame = renderNormalMappedPlate ({ 0.0f, 0.0f, 0.0f, 0.0f }, tiltedTowardsX, sunFromX);
    const auto unmapped = renderNormalMappedPlate ({ 1.0f, 0.0f, 0.0f, 1.0f }, std::nullopt, sunFromX);
    const auto flipped = renderNormalMappedPlate ({ -1.0f, 0.0f, 0.0f, 1.0f }, tiltedTowardsX, sunFromX);
    RecordProperty ("tangentFrame", String (tangentFrame, 2).toStdString());
    RecordProperty ("derivativeFrame", String (derivativeFrame, 2).toStdString());
    RecordProperty ("unmapped", String (unmapped, 2).toStdString());
    RecordProperty ("flipped", String (flipped, 2).toStdString());

    EXPECT_NEAR (derivativeFrame, tangentFrame, tangentFrame * 0.02);

    // Both frames really apply the map
    EXPECT_GT (tangentFrame, unmapped * 1.1);

    // Authored tangents are really used: reversed, they tilt the plate away from the sun
    EXPECT_LT (flipped, unmapped);
}

TEST_P (SceneRendererTierTests, BitangentHandednessSurvivesMirroring)
{
    // The normal map tilts the plate towards +y (decreasing v), where a grazing sun shines from:
    // only the bitangent, and so the handedness, decides the shading
    const auto tiltedTowardsY = Color (128, 191, 247);
    const auto sunFromY = Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, -1.2f);

    const auto tangentFrame = renderNormalMappedPlate ({ 1.0f, 0.0f, 0.0f, 1.0f }, tiltedTowardsY, sunFromY);
    const auto derivativeFrame = renderNormalMappedPlate ({ 0.0f, 0.0f, 0.0f, 0.0f }, tiltedTowardsY, sunFromY);
    const auto unmapped = renderNormalMappedPlate ({ 1.0f, 0.0f, 0.0f, 1.0f }, std::nullopt, sunFromY);
    const auto leftHanded = renderNormalMappedPlate ({ 1.0f, 0.0f, 0.0f, -1.0f }, tiltedTowardsY, sunFromY);
    const auto mirroredTangentFrame = renderNormalMappedPlate ({ 1.0f, 0.0f, 0.0f, 1.0f }, tiltedTowardsY, sunFromY, -1.0f);
    const auto mirroredDerivativeFrame = renderNormalMappedPlate ({ 0.0f, 0.0f, 0.0f, 0.0f }, tiltedTowardsY, sunFromY, -1.0f);
    RecordProperty ("tangentFrame", String (tangentFrame, 2).toStdString());
    RecordProperty ("derivativeFrame", String (derivativeFrame, 2).toStdString());
    RecordProperty ("unmapped", String (unmapped, 2).toStdString());
    RecordProperty ("leftHanded", String (leftHanded, 2).toStdString());
    RecordProperty ("mirroredTangentFrame", String (mirroredTangentFrame, 2).toStdString());
    RecordProperty ("mirroredDerivativeFrame", String (mirroredDerivativeFrame, 2).toStdString());

    EXPECT_NEAR (derivativeFrame, tangentFrame, tangentFrame * 0.02);
    EXPECT_GT (tangentFrame, unmapped * 1.1);

    // w = -1 turns the bitangent, and the tilt, away from the sun
    EXPECT_LT (leftHanded, unmapped);

    // A mirroring transform keeps the bitangent pointing up the texture, with or without tangents
    EXPECT_NEAR (mirroredTangentFrame, tangentFrame, tangentFrame * 0.02);
    EXPECT_NEAR (mirroredDerivativeFrame, tangentFrame, tangentFrame * 0.02);
}

TEST_P (SceneRendererTierTests, GBufferNormalFacesTheCamera)
{
    Scene scene;
    buildShadowsScene (scene);
    setTier (scene, GetParam());

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto normalRoughness = renderer.getPassOutput (SceneRenderer::PassOutput::gbufferNormal);
    if (GetParam() == RenderSettings::TierPreference::raster)
    {
        EXPECT_EQ (normalRoughness, nullptr);
        return;
    }

    ASSERT_NE (normalRoughness, nullptr);
    EXPECT_EQ (normalRoughness->getFormat(), GpuTextureFormat::rgba16float);

    // The plates face the camera: +Z
    const auto normal = readGBufferNormal (normalRoughness).getPixelColor (32, 32);
    EXPECT_NEAR (normal.getRed(), 128, 3);
    EXPECT_NEAR (normal.getGreen(), 128, 3);
    EXPECT_NEAR (normal.getBlue(), 255, 3);

    // A cleared texel decodes to +Z as well: only drawn texels have a roughness
    const auto roughness = mapTexels (normalRoughness, "vec4(t.z, 0.0, 0.0, 1.0)");
    ASSERT_TRUE (roughness.isValid());
    EXPECT_GT (roughness.getPixelColor (32, 32).getRed(), 64);
    EXPECT_EQ (roughness.getPixelColor (1, 1).getRed(), 0);
}

TEST_P (SceneRendererTierTests, PrepassMatchesTheGBufferEncoding)
{
    checkPrepassMatchesTheGBufferEncoding (GetParam());
}
#endif

TEST_P (SceneRendererTierTests, PrepassIsNotCreatedByDefault)
{
    checkPrepassIsNotCreatedByDefault (GetParam());
}

TEST_P (SceneRendererTierTests, RendersWithoutTheDepthNormalPrepass)
{
    checkRendersWithoutTheDepthNormalPrepass (GetParam());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_P (SceneRendererTierTests, AmbientOcclusionDarkensTheContactCrease)
{
    checkAmbientOcclusionDarkensTheContactCrease (GetParam());
}
#endif

TEST_P (SceneRendererTierTests, AmbientOcclusionOffAllocatesNothing)
{
    checkAmbientOcclusionOffAllocatesNothing (GetParam());
}

TEST_P (SceneRendererTierTests, AmbientOcclusionUnavailableOnRasterRendersFallback)
{
    checkAmbientOcclusionUnavailableOnRasterRendersFallback (GetParam());
}

TEST_P (SceneRendererTierTests, AmbientOcclusionLeavesTheDirectLightAlone)
{
    checkAmbientOcclusionLeavesTheDirectLightAlone (GetParam());
}

TEST_P (SceneRendererTierTests, ConvergedAmbientOcclusionIsDeterministic)
{
    checkConvergedAmbientOcclusionIsDeterministic (GetParam());
}

TEST_P (SceneRendererTierTests, BlendedSurfacesSkipAmbientOcclusion)
{
    checkBlendedSurfacesSkipAmbientOcclusion (GetParam());
}

TEST_P (SceneRendererTierTests, BentNormalsTurnTheDiffuseLightToTheOpenSide)
{
    checkBentNormalsTurnTheDiffuseLightToTheOpenSide (GetParam());
}

TEST_F (SceneRendererMetalTests, RasterAmbientOcclusionOffAgainRestoresTheFrame)
{
    checkRasterAmbientOcclusionOffAgainRestoresTheFrame();
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_P (SceneRendererTierTests, TinyRadiusMatchesPunctual)
{
    checkTinyRadiusMatchesPunctual (GetParam());
}

TEST_P (SceneRendererTierTests, RadiusWidensTheHighlight)
{
    checkRadiusWidensTheHighlight (GetParam());
}

TEST_P (SceneRendererTierTests, SunDiskSoftensNothingButSpecular)
{
    // A sun of angular radius 0.01 is a disk of radiance I / (pi sin^2 radius), so a rough plate
    // receives the illuminance of the punctual sun, I
    const auto meanAt = [this] (float radius)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.8f, 0.8f, 0.8f, 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 1.0f;

        Scene scene;
        addPlate (scene, material);
        scene.setAmbientColor (Colors::black);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        auto& node = sun->attach<LightNode>();
        node.intensity = 2.0f;
        node.radius = radius;
        sun->setRotation (Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, 0.5f));
        scene.getRoot()->addChild (sun);

        return meanLuma (renderLuma (scene, 128), 128, { 40, 40, 48, 48 });
    };

    const auto punctual = meanAt (0.0f);
    const auto disk = meanAt (0.01f);
    RecordProperty ("punctual", String (punctual, 4).toStdString());
    RecordProperty ("disk", String (disk, 4).toStdString());

    ASSERT_GT (punctual, 0.05);
    EXPECT_NEAR (disk / punctual, 1.0, 0.01);
}

TEST_P (SceneRendererTierTests, AreaLightsStayFinite)
{
    checkAreaLightsStayFinite (GetParam());
}

TEST_P (SceneRendererTierTests, LayeredAreaLightsStayFinite)
{
    checkAreaLightsStayFinite (GetParam(), true);
}

TEST_P (SceneRendererTierTests, LayersUnderATinyAreaLightMatchPunctual)
{
    checkLayersUnderATinyAreaLightMatchPunctual (GetParam());
}

TEST_P (SceneRendererTierTests, LargeLowSphereLightDiffuseIsExact)
{
    // A sphere light of angular radius 25 degrees, its center 17 degrees over a diffuse-only plate:
    // the horizon cuts its cap, and the plate center receives albedo / pi times the radiance
    // I / (pi r^2) integrated over the clipped cap, by quadrature
    const auto sigma = MathConstants<double>::pi * 25.0 / 180.0;
    const auto elevation = MathConstants<double>::pi * 17.0 / 180.0;
    const auto distance = 10.0;
    const auto radius = distance * std::sin (sigma);
    const auto intensity = 400.0;
    const auto albedo = 0.8;

    auto material = Material::Ptr (new Material());
    material->baseColorFactor = { static_cast<float> (albedo), static_cast<float> (albedo), static_cast<float> (albedo), 1.0f };
    material->metallicFactor = 0.0f;
    material->roughnessFactor = 1.0f;
    material->specularFactor = 0.0f;

    Scene scene;
    addPlate (scene, material);
    scene.setAmbientColor (Colors::black);

    auto light = EntityNode::Ptr (new EntityNode ("sphere"));
    auto& node = light->attach<LightNode> (LightNode::Type::point);
    node.intensity = static_cast<float> (intensity);
    node.radius = static_cast<float> (radius);
    light->setPosition ({ static_cast<float> (distance * std::cos (elevation)), 0.0f, static_cast<float> (distance * std::sin (elevation)) });
    scene.getRoot()->addChild (light);

    const auto measured = meanLuma (renderLuma (scene, 128), 128, { 63, 63, 2, 2 });
    const auto radiance = intensity / (MathConstants<double>::pi * radius * radius);
    const auto expected = albedo * radiance * SceneRendererAreaLightMathTests::capQuadrature (sigma, MathConstants<double>::halfPi - elevation);
    RecordProperty ("measured", String (measured, 4).toStdString());
    RecordProperty ("expected", String (expected, 4).toStdString());

    ASSERT_GT (expected, 0.1);
    EXPECT_NEAR (measured / expected, 1.0, 0.03);
}

TEST_P (SceneRendererTierTests, SphereLightSpecularMatchesGgx)
{
    // A white metal plate (f0 = f90 = 1) seen at n.v = 0.5 mirrors a sphere light of angular radius
    // 10 degrees: the LTC specular matches the GGX lobe integrated over the cap by quadrature, within
    // the fit error at roughness 0.5
    const auto sigma = MathConstants<double>::pi * 10.0 / 180.0;
    const auto distance = 3.0;
    const auto radius = distance * std::sin (sigma);
    const auto intensity = 2.0;
    const auto roughness = 0.5;

    auto material = Material::Ptr (new Material());
    material->metallicFactor = 1.0f;
    material->roughnessFactor = static_cast<float> (roughness);

    Scene scene;
    auto* plate = addPlate (scene, material);
    scene.setAmbientColor (Colors::black);

    // Tilted so the normal makes 60 degrees with the direction to the camera
    const auto tilt = Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, -MathConstants<float>::pi / 3.0f);
    plate->setRotation (tilt);

    const auto normal = tilt.rotate ({ 0.0f, 0.0f, 1.0f });
    const Vector3<float> toCamera { 0.0f, 0.0f, 1.0f };
    const auto mirrored = normal * (2.0f * normal.dotProduct (toCamera)) - toCamera;

    auto light = EntityNode::Ptr (new EntityNode ("sphere"));
    auto& node = light->attach<LightNode> (LightNode::Type::point);
    node.intensity = static_cast<float> (intensity);
    node.radius = static_cast<float> (radius);
    light->setPosition (mirrored * static_cast<float> (distance));
    scene.getRoot()->addChild (light);

    const auto measured = meanLuma (renderLuma (scene, 128), 128, { 63, 63, 2, 2 });
    const auto radiance = intensity / (MathConstants<double>::pi * radius * radius);
    const auto expected = radiance * SceneRendererAreaLightMathTests::ggxCapQuadrature (0.5, roughness * roughness, { -std::sqrt (0.75), 0.0, 0.5 }, sigma);
    RecordProperty ("measured", String (measured, 4).toStdString());
    RecordProperty ("expected", String (expected, 4).toStdString());

    ASSERT_GT (expected, 0.1);
    EXPECT_NEAR (measured / expected, 1.0, 0.1);
}

TEST_P (SceneRendererTierTests, TinySpotMatchesPunctualOffAxis)
{
    // A spot of radius 0.001 is a sphere under the cone of its center: 35 degrees off its axis, near
    // the 45 degree edge of its cone, it lights the plate like the punctual spot
    const auto meanAt = [this] (float radius)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.8f, 0.8f, 0.8f, 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 0.5f;

        Scene scene;
        addPlate (scene, material);
        scene.setAmbientColor (Colors::black);

        auto light = EntityNode::Ptr (new EntityNode ("spot"));
        auto& node = light->attach<LightNode> (LightNode::Type::spot);
        node.intensity = 5.0f;
        node.radius = radius;
        light->setPosition ({ 0.0f, 0.0f, 1.0f });
        scene.getRoot()->addChild (light);

        // The plate point at tan (35 degrees) = 0.7 along x
        return meanLuma (renderLuma (scene, 128), 128, { 84, 62, 2, 4 });
    };

    const auto punctual = meanAt (0.0f);
    const auto area = meanAt (0.001f);
    RecordProperty ("punctual", String (punctual, 4).toStdString());
    RecordProperty ("area", String (area, 4).toStdString());

    ASSERT_GT (punctual, 0.01);
    EXPECT_NEAR (area / punctual, 1.0, 0.02);
}

TEST_P (SceneRendererTierTests, SunDiskWidensTheHighlight)
{
    // A sun shining down the view mirrors off a smooth black plate into a small highlight; an angular
    // radius of 0.05 spreads it over more bright pixels
    const auto brightPixelsAt = [this] (float radius)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 0.1f;

        Scene scene;
        addPlate (scene, material);
        scene.setAmbientColor (Colors::black);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().radius = radius;
        scene.getRoot()->addChild (sun);

        const auto luma = renderLuma (scene, 128);
        return static_cast<int> (std::count_if (luma.begin(), luma.end(), [] (double value) { return value > 2.5; }));
    };

    const auto sharp = brightPixelsAt (0.0f);
    const auto wide = brightPixelsAt (0.05f);
    RecordProperty ("sharp", sharp);
    RecordProperty ("wide", wide);

    EXPECT_GT (sharp, 0);
    EXPECT_GT (wide, 2 * sharp);
}

TEST_P (SceneRendererTierTests, InfiniteFarPlaneShowsTheEnvironment)
{
    // A glTF camera without zfar has an infinite far plane: the sky it shows must be finite and
    // look exactly as through the same camera with a distant far plane. The environment changes
    // with the direction, so a flipped or wrong sky direction shows
    const auto full = GetParam() == RenderSettings::TierPreference::full;

    const auto renderSky = [&] (float zFar)
    {
        Scene scene;
        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>& d)
                                                  { return Vector3<float> (0.5f + 0.4f * d.getX(), 0.5f + 0.4f * d.getY(), 0.5f + 0.4f * d.getZ()); },
                                                  64,
                                                  3));
        scene.setEnvironmentVisible (true);

        auto camera = EntityNode::Ptr (new EntityNode ("camera"));
        auto& node = camera->attach<CameraNode>();
        node.zNear = 0.05f;
        node.zFar = zFar;
        camera->setRotation (Quaternion::fromAxisAngle (Vector3<float> (1.0f, 1.0f, 0.0f).normalized(), 0.6f));
        scene.getRoot()->addChild (camera);
        scene.setActiveCamera (camera.get());

        // Scene color holds raw radiance on full without temporal anti-aliasing
        if (full)
            setFullTierTaa (scene, 1);
        else
            setTier (scene, RenderSettings::TierPreference::raster);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();

        if (full)
        {
            const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
            EXPECT_NE (sceneColor, nullptr);
            EXPECT_TRUE (sceneColor != nullptr && isFiniteTexture (sceneColor)) << "zFar " << zFar;
            EXPECT_FALSE (sceneColor == nullptr || isSaturatedTexture (sceneColor)) << "zFar " << zFar;
        }

        return image;
    };

    const auto infinite = renderSky (0.0f);
    const auto finite = renderSky (1.0e5f);
    ASSERT_TRUE (infinite.isValid());
    ASSERT_TRUE (finite.isValid());

    // Not vacuous: the sky varies across the frame, below white
    const auto topLeft = meanLuminance (finite, { 0, 0, 8, 8 });
    const auto bottomRight = meanLuminance (finite, { 56, 56, 8, 8 });
    EXPECT_GT (std::abs (topLeft - bottomRight), 5.0);
    EXPECT_LT (jmax (topLeft, bottomRight), 250.0);

    EXPECT_EQ (countDifferences (infinite, finite), 0);
}

TEST_P (SceneRendererTierTests, RendersWithoutTheLookupTables)
{
    // When the lookup tables cannot be created, frames still render: area lights lose their
    // specular light, never their diffuse light, and nothing turns non-finite
    const auto full = GetParam() == RenderSettings::TierPreference::full;

    const auto render = [&] (bool withTables)
    {
        Scene scene;
        buildAreaLightsScene (scene);

        if (full)
            setFullTierTaa (scene, 1);
        else
            setTier (scene, RenderSettings::TierPreference::raster);

        SceneRenderer renderer;
        if (! withTables)
            renderer.failLookupTablesForTesting();

        const auto image = renderToImage (renderer, scene, 128);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();

        if (full)
        {
            const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
            EXPECT_NE (sceneColor, nullptr);
            EXPECT_TRUE (sceneColor != nullptr && isFiniteTexture (sceneColor));
            EXPECT_FALSE (sceneColor == nullptr || isSaturatedTexture (sceneColor));
        }

        return image;
    };

    const auto with = render (true);
    const auto without = render (false);
    ASSERT_TRUE (with.isValid());
    ASSERT_TRUE (without.isValid());

    // Only specular light is lost: no pixel brightens, some darken, and none goes black (a NaN
    // reads black or white on Metal)
    int brighter = 0, darker = 0;
    auto darkest = 255.0;
    for (int y = 0; y < 128; ++y)
    {
        for (int x = 0; x < 128; ++x)
        {
            const auto before = meanLuminance (with, { x, y, 1, 1 });
            const auto after = meanLuminance (without, { x, y, 1, 1 });
            brighter += after > before + 2.0 ? 1 : 0;
            darker += after < before - 2.0 ? 1 : 0;
            darkest = jmin (darkest, after);
        }
    }

    RecordProperty ("darker", darker);
    EXPECT_EQ (brighter, 0);
    EXPECT_GT (darker, 100);
    EXPECT_GT (darkest, 5.0);

    // The environment's specular light goes too: a white rough metal, which only reflects, turns
    // black under a uniform environment
    const auto metal = renderHeadOnRadiance (Material::Ptr (new Material()), GetParam(), 0.0f, Colors::black, 0.5f);
    const auto blackMetal = renderHeadOnRadiance (Material::Ptr (new Material()), GetParam(), 0.0f, Colors::black, 0.5f, true);
    EXPECT_GT (metal[1], 0.1);
    EXPECT_LT (blackMetal[1], 0.005);
}

TEST_P (SceneRendererTierTests, RoughWhiteMetalPassesTheWhiteFurnaceTest)
{
    // A white rough metal facing the view, under a uniform environment and no light, reflects all
    // of it. The split sum alone reflects the single-scattering albedo, 1 - ln 2 at roughness 1
    // (the analytic fit used before Phase 4 gave 0.45); the multiple-scattering compensation
    // (Fdez-Aguera 2019) returns the rest
    const auto radiance = renderHeadOnRadiance (Material::Ptr (new Material()), GetParam(), 0.0f, Colors::black, 0.5f);
    RecordProperty ("roughMetalBrightness", String (radiance[1] / 0.5, 4).toStdString());

    for (const auto channel : radiance)
        EXPECT_NEAR (channel, 0.5, 0.02);

    // A colored metal tells the average Fresnel apart: each channel regains FssEss Favg Ems /
    // (1 - Favg Ems), with Favg = f0 + (1 - f0) / 21. At f0 = 0.5 that is 0.2409 of the
    // environment, where the tables alone give 0.1534 and Favg = 1 would give 0.5. A brighter
    // environment keeps the darkest channel well above the 8-bit steps of the image
    const std::array<float, 3> f0 { 1.0f, 0.5f, 0.2f };
    const auto environment = 2.0f;

    auto colored = Material::Ptr (new Material());
    colored->baseColorFactor = { f0[0], f0[1], f0[2], 1.0f };
    const auto coloredRadiance = renderHeadOnRadiance (colored, GetParam(), 0.0f, Colors::black, environment);

    const auto amplitude = LtcTables::getAmplitude (LtcTables::size - 1, 0);
    for (size_t channel = 0; channel < f0.size(); ++channel)
    {
        const auto singleScattering = f0[channel] * (amplitude[0] - amplitude[1]) + amplitude[1];
        const auto averageFresnel = f0[channel] + (1.0f - f0[channel]) / 21.0f;
        const auto missing = 1.0f - amplitude[0];
        const auto expected = environment * (singleScattering + singleScattering * averageFresnel * missing / (1.0f - averageFresnel * missing));

        EXPECT_NEAR (coloredRadiance[channel], expected, expected * 0.1) << "channel " << channel;
    }
}

TEST_P (SceneRendererTierTests, RendersWithoutTheShadowCompareSampler)
{
    checkRendersWithoutTheShadowCompareSampler (GetParam());
}

TEST_P (SceneRendererTierTests, BaseColorAboveOneShadesAsOne)
{
    // glTF base colors lie within [0, 1]: a larger factor is clamped alike by the forward pass and
    // the G-buffer, so both tiers agree
    const auto one = renderHeadOnRadiance (makeDielectric (1.0f, 1.5f, 1.0f), GetParam());
    const auto two = renderHeadOnRadiance (makeDielectric (2.0f, 1.5f, 1.0f), GetParam());

    ASSERT_GT (one[0], 0.1);
    for (size_t channel = 0; channel < 3; ++channel)
        EXPECT_NEAR (two[channel] / one[channel], 1.0, 0.02) << "channel " << channel;
}
#endif

TEST_P (SceneRendererTierTests, RendersWithoutTheShadowPipelines)
{
    checkRendersWithoutTheShadowPipelines (GetParam());
}

TEST_P (SceneRendererTierTests, ShadowMapsTheDeviceCannotCreateLeaveTheLightsUnshadowed)
{
    checkShadowMapsTheDeviceCannotCreateLeaveTheLightsUnshadowed (GetParam());
}

TEST_P (SceneRendererTierTests, SunShadowReachesNearAndFarCasters)
{
    checkSunShadowReachesNearAndFarCasters (GetParam());
}

TEST_P (SceneRendererTierTests, CascadeTransitionHasNoSeam)
{
    checkCascadeTransitionHasNoSeam (GetParam());
}

TEST_P (SceneRendererTierTests, AreaSunCastsAShadow)
{
    checkAreaSunCastsAShadow (GetParam());
}

TEST_P (SceneRendererTierTests, MaskedCasterShadowsOnlyWhatItKeeps)
{
    checkMaskedCasterShadowsOnlyWhatItKeeps (GetParam());
}

TEST_P (SceneRendererTierTests, ShadowMapsRenderOnlyWhenTheShadowsChange)
{
    // A still scene draws its shadow maps once, a moved caster draws them again, and so does
    // restartAccumulation(), for changes the renderer cannot see
    Scene scene;
    buildLongGroundScene (scene, 2.0f);
    setTier (scene, GetParam());

    SceneRenderer renderer;
    for (int frame = 0; frame < 3; ++frame)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);

    auto* caster = scene.getRoot()->findChild ("caster");
    ASSERT_NE (caster, nullptr);
    caster->setPosition (caster->getPosition() + Vector3<float> (0.05f, 0.0f, 0.0f));

    for (int frame = 0; frame < 2; ++frame)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 2);

    renderer.restartAccumulation();
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 3);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, ShadowMapsSurviveMovingLightsAndRedrawnOpaqueTextures)
{
    // The Scene3D demo's frames: lights casting no shadow orbit and an opaque caster's texture is
    // redrawn, under the jittered camera of TAA on the full tier. None of it changes the shadows
    auto material = Material::Ptr (new Material());
    auto texture = Texture::Ptr (new Texture (GpuTexture::create (device, GpuTextureDesc (2, 2, GpuTextureFormat::rgba8unorm))));
    material->baseColorTexture = texture;

    Scene scene;
    buildLongGroundScene (scene, 2.0f, material);
    setTier (scene, GetParam());

    auto orbiting = EntityNode::Ptr (new EntityNode ("orbiting"));
    orbiting->attach<LightNode> (LightNode::Type::point);
    scene.getRoot()->addChild (orbiting);

    SceneRenderer renderer;
    for (int frame = 0; frame < 4; ++frame)
    {
        orbiting->setPosition ({ 0.2f * static_cast<float> (frame), 1.0f, -2.0f });
        texture->setGpuTexture (GpuTexture::create (device, GpuTextureDesc (2, 2, GpuTextureFormat::rgba8unorm)));
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    }

    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, MaskedCasterChangesDrawTheShadowMapsAgain)
{
    // A masked caster casts what its alpha test keeps: a new cutoff, base alpha or base color
    // texture content changes the maps of an otherwise still scene
    auto material = Material::Ptr (new Material());
    material->baseColorFactor = { 0.8f, 0.8f, 0.8f, 0.8f };
    material->alphaMode = Material::AlphaMode::mask;

    Scene scene;
    buildLongGroundScene (scene, 2.0f, material);
    setTier (scene, GetParam());

    SceneRenderer renderer;
    for (int frame = 0; frame < 2; ++frame)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);

    material->alphaCutoff = 0.9f;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 2);

    material->baseColorFactor[3] = 1.0f;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 3);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, GpuFailureDrawsTheShadowMapsAgain)
{
    // The frame drawing the maps failed on the GPU: the next frame cannot trust them
    Scene scene;
    buildLongGroundScene (scene, 2.0f);
    setTier (scene, GetParam());

    SceneRenderer renderer;
    device->failNextFrameForTesting ("injected failure");
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);

    // Frames complete in order, so once a later frame did the failed one has too
    {
        auto fence = GpuFrame::begin (device);
        EXPECT_TRUE (fence.waitForGPU().wasOk());
    }

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getLastGpuFailure(), "injected failure");
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 2);

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 2);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, OrbitingACompactSceneDoesNotRedrawTheShadows)
{
    // A device-sized box on a ground of about its size, seen whole: one map fitted to the scene is
    // finer than the first cascade and ignores the camera, so orbiting around draws it only once
    Scene scene;
    auto& box = addCompactScene (scene);

    addCamera (scene, { 5.0f, 4.0f, 0.0f }, {});
    auto* camera = scene.getRoot()->findChild ("camera");
    ASSERT_NE (camera, nullptr);
    setTier (scene, GetParam());

    SceneRenderer renderer;
    for (int frame = 0; frame < 8; ++frame)
    {
        const auto yaw = 0.3f * static_cast<float> (frame);
        aimCamera (*camera, { 5.0f * std::cos (yaw), 4.0f, 5.0f * std::sin (yaw) }, {});

        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_EQ (shadows (renderer).getNumRecords(), 1) << "frame " << frame;
    }

    // Zooming out and panning keep the map too
    aimCamera (*camera, { 6.5f, 5.2f, 0.0f }, {});
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    aimCamera (*camera, { 6.5f, 5.2f, 0.3f }, { 0.0f, 0.0f, 0.3f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getNumRecords(), 1);
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);

    // Moving the box draws it again
    box.setPosition ({ 0.1f, 0.3f, 0.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 2);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, LargeSceneKeepsViewFittedCascades)
{
    // The long ground near one end: no single map over 200 m matches the first cascade, so the tier's
    // cascades stay fitted to the view, and a camera move fits them again
    Scene scene;
    buildLongGroundScene (scene, 2.0f);
    setTier (scene, GetParam());

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    ASSERT_NE (renderer.getCapabilities(), nullptr);
    const auto cascades = renderer.getCapabilities()->getCascadeCount (scene.getRenderSettings());
    EXPECT_EQ (cascades, GetParam() == RenderSettings::TierPreference::full ? 4 : RenderCapabilities::defaultRasterCascades);
    EXPECT_EQ (shadows (renderer).getNumRecords(), cascades);
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);

    auto* camera = scene.getRoot()->findChild ("camera");
    ASSERT_NE (camera, nullptr);
    camera->setPosition (camera->getPosition() + Vector3<float> (0.5f, 0.0f, 0.0f));

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getNumRecords(), cascades);
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 2);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, CasterMovingInsideTheLastCascadeRedrawsOnlyIt)
{
    // The far cube moves sideways, still beyond every cascade but the last: the ground keeps the
    // fit of every cascade, so only the last one holds anything new
    Scene scene;
    buildLongGroundScene (scene, 2.0f);
    auto& farCaster = addFarCaster (scene);
    setTier (scene, GetParam());

    SceneRenderer renderer;
    for (int frame = 0; frame < 2; ++frame)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto cascades = shadows (renderer).getNumRecords();
    ASSERT_GT (cascades, 1);

    auto expected = getCascadeRenderCounts (renderer, cascades);
    EXPECT_EQ (expected, std::vector<int> (static_cast<size_t> (cascades), 1));

    farCaster.setPosition (farCaster.getPosition() + Vector3<float> (0.5f, 0.0f, 0.0f));
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    ++expected.back();
    EXPECT_EQ (getCascadeRenderCounts (renderer, cascades), expected);
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 2);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, CasterLeavingACascadeRedrawsIt)
{
    // The near cube moves beside the far one: the first cascade, which it leaves, and the last
    // one, which it enters, draw again
    Scene scene;
    buildLongGroundScene (scene, 2.0f);
    addFarCaster (scene);
    setTier (scene, GetParam());

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto cascades = shadows (renderer).getNumRecords();
    ASSERT_GT (cascades, 1);

    auto* caster = scene.getRoot()->findChild ("caster");
    ASSERT_NE (caster, nullptr);
    caster->setPosition ({ 3.0f, caster->getPosition().getY(), -60.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto counts = getCascadeRenderCounts (renderer, cascades);
    EXPECT_EQ (counts.front(), 2);
    EXPECT_EQ (counts.back(), 2);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, ShadowSettingOrSunChangeRedrawsEveryCascade)
{
    Scene scene;
    buildLongGroundScene (scene, 2.0f);
    addFarCaster (scene);
    setTier (scene, GetParam());

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto cascades = shadows (renderer).getNumRecords();
    ASSERT_GT (cascades, 1);

    // The sun turns a little
    auto* sun = scene.getRoot()->findChild ("sun");
    ASSERT_NE (sun, nullptr);
    aimCamera (*sun, { -10.0f * std::sin (0.62f), 10.0f * std::cos (0.62f), 0.0f }, {});
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (getCascadeRenderCounts (renderer, cascades), std::vector<int> (static_cast<size_t> (cascades), 2));

    // The shadow resolution changes
    auto settings = scene.getRenderSettings();
    settings.shadowResolution = 1024;
    scene.setRenderSettings (settings);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (getCascadeRenderCounts (renderer, cascades), std::vector<int> (static_cast<size_t> (cascades), 3));
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, MovingAShadowedPointLightKeepsTheSunCascades)
{
    // The point light's views live in the atlas: moving it draws the atlas again, never a cascade
    Scene scene;
    buildLongGroundScene (scene, 2.0f);

    auto lamp = EntityNode::Ptr (new EntityNode ("lamp"));
    auto& light = lamp->attach<LightNode> (LightNode::Type::point);
    light.castsShadows = true;
    light.range = 5.0f;
    lamp->setPosition ({ 1.0f, 1.0f, -2.0f });
    scene.getRoot()->addChild (lamp);
    setTier (scene, GetParam());

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_GT (shadows (renderer).getAtlasSize(), 0);

    const auto cascades = static_cast<int> (shadows (renderer).getShadowData().sunInfo[0]);
    ASSERT_GT (cascades, 1);
    const auto counts = getCascadeRenderCounts (renderer, cascades);

    lamp->setPosition ({ 1.2f, 1.0f, -2.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 2);
    EXPECT_EQ (getCascadeRenderCounts (renderer, cascades), counts);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, OrthographicCameraTakesTheSceneFitThatNeverFadesOut)
{
    // A wide orthographic view from above whose far plane cuts the ground off: its one view-fitted
    // cascade would fade out and follow every pan, the scene fit covers every receiver and stays
    Scene scene;
    addCompactScene (scene);
    addCamera (scene, { 0.0f, 5.0f, 0.0f }, {}, { 0.0f, 0.0f, -1.0f });

    auto* camera = scene.getRoot()->findChild ("camera");
    ASSERT_NE (camera, nullptr);
    auto& lens = *scene.getActiveCamera();
    lens.projection = CameraNode::Projection::orthographic;
    lens.xMag = 3.0f;
    lens.yMag = 3.0f;
    lens.zFar = 4.9f;
    setTier (scene, GetParam());

    SceneRenderer renderer;
    for (const auto& offset : { Vector3<float> (0.0f, 0.0f, 0.0f), Vector3<float> (0.5f, 0.0f, 0.0f), Vector3<float> (0.5f, 0.0f, 0.3f) })
    {
        camera->setPosition (Vector3<float> (0.0f, 5.0f, 0.0f) + offset);
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

        const auto data = shadows (renderer).getShadowData();
        EXPECT_EQ (data.sunInfo[0], 1.0f);
        EXPECT_EQ (data.sunInfo[3], 0.0f);
    }

    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, SceneFitHoldsWhileTheFirstCascadeIsNoFiner)
{
    // The scene fit of the compact scene has a radius of 1.40625. From "near" the first cascade has a
    // radius of 1.5 (one quantization step smaller: 1.375), so a fresh renderer keeps the cascades
    // there; from "far" it has 1.625 on raster, 1.75 on full, so the scene fit takes over, and holds
    // when the camera comes back near, its texels still no larger than the first cascade's
    Scene scene;
    addCompactScene (scene);
    addCamera (scene, { 5.0f, 4.0f, 0.0f }, {});

    auto* camera = scene.getRoot()->findChild ("camera");
    ASSERT_NE (camera, nullptr);
    setTier (scene, GetParam());

    const auto full = GetParam() == RenderSettings::TierPreference::full;
    const auto direction = Vector3<float> (5.0f, 4.0f, 0.0f).normalized();
    const auto nearEye = direction * (full ? 3.25f : 2.75f);
    const auto farEye = direction * (full ? 3.5f : 3.0f);

    {
        SceneRenderer fresh;
        aimCamera (*camera, nearEye, {});
        ASSERT_NE (fresh.render (device, scene, 64, 64), nullptr) << fresh.getLastError();
        EXPECT_GT (shadows (fresh).getShadowData().sunInfo[0], 1.0f);
        EXPECT_TRUE (fresh.waitForGPU().wasOk());
    }

    SceneRenderer renderer;
    for (int frame = 0; frame < 6; ++frame)
    {
        aimCamera (*camera, frame % 2 == 0 ? farEye : nearEye, {});
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_EQ (shadows (renderer).getShadowData().sunInfo[0], 1.0f) << "frame " << frame;
    }

    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, PointLightShadowsOnAllSixFaces)
{
    checkPointLightShadowsOnAllSixFaces (GetParam());
}

TEST_P (SceneRendererTierTests, PointShadowHasNoFaceSeams)
{
    checkPointShadowHasNoFaceSeams (GetParam());
}

TEST_P (SceneRendererTierTests, SpotLightCastsAShadow)
{
    checkSpotCastsAShadow (GetParam(), 0.0f);
}

TEST_P (SceneRendererTierTests, AreaSpotCastsAShadowTowardsItsCenter)
{
    // The area light variant shadows a spot with a radius from its center, as a punctual spot
    checkSpotCastsAShadow (GetParam(), 0.1f);
    checkAreaSpotCastsAShadowTowardsItsCenter (GetParam());
}

TEST_P (SceneRendererTierTests, AtlasNeverExceedsTheShadowResolution)
{
    // The atlas side is a power of two: rounded down, so it never exceeds the setting (raster) or
    // twice the setting (full)
    const auto full = GetParam() == RenderSettings::TierPreference::full;

    Scene scene;
    buildSpotOverGround (scene);
    auto settings = scene.getRenderSettings();
    settings.tier = GetParam();
    settings.shadowResolution = full ? 1100 : 2100;
    scene.setRenderSettings (settings);

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getAtlasSize(), 2048);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, OrthographicCameraSizesTilesWhateverItsDistance)
{
    // An orthographic camera 2 units high over a 256 px viewport sees the 2 m reach of the light
    // as 256 px of radius: a 512 texel tile, as near as far
    const auto tileSizeFrom = [this] (float distance)
    {
        Scene scene;
        addGround (scene, 5.0f);
        setTier (scene, GetParam());

        auto entity = EntityNode::Ptr (new EntityNode ("point"));
        auto& light = entity->attach<LightNode> (LightNode::Type::point);
        light.range = 2.0f;
        light.castsShadows = true;
        entity->setPosition ({ 0.0f, 0.5f, 0.0f });
        scene.getRoot()->addChild (entity);

        addCamera (scene, { 0.0f, distance, 0.0f }, {}, { 0.0f, 0.0f, -1.0f });
        auto& camera = *scene.getActiveCamera();
        camera.projection = CameraNode::Projection::orthographic;
        camera.yMag = 1.0f;

        SceneRenderer renderer;
        EXPECT_NE (renderer.render (device, scene, 256, 256), nullptr) << renderer.getLastError();
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
        return shadows (renderer).getTileSize (0);
    };

    EXPECT_EQ (tileSizeFrom (5.0f), 512);
    EXPECT_EQ (tileSizeFrom (200.0f), 512);
}

TEST_P (SceneRendererTierTests, ManyShadowedLightsRenderWithoutError)
{
    checkManyShadowedLightsRenderWithoutError (GetParam());
}

TEST_P (SceneRendererTierTests, ShadowedLightOutOfReachAllocatesNoAtlas)
{
    checkShadowedLightOutOfReachAllocatesNoAtlas (GetParam());
}

TEST_P (SceneRendererTierTests, FailedShadowMapKeepsTheOtherOneCached)
{
    checkFailedShadowMapKeepsTheOtherOneCached (GetParam());
}

TEST_P (SceneRendererTierTests, AtlasIsReleasedWhenNoLightNeedsIt)
{
    checkAtlasIsReleasedWhenNoLightNeedsIt (GetParam());
}

TEST_P (SceneRendererTierTests, AtlasRedrawsOnlyTheViewsACasterReaches)
{
    checkAtlasRedrawsOnlyTheViewsACasterReaches (GetParam());
}

TEST_P (SceneRendererTierTests, PartiallyRedrawnAtlasMatchesAFreshOne)
{
    checkPartiallyRedrawnAtlasMatchesAFreshOne (GetParam());
}

TEST_P (SceneRendererTierTests, SingleSpotTakesHalfTheAtlas)
{
    checkSingleSpotTakesHalfTheAtlas (GetParam());
}

TEST_P (SceneRendererTierTests, LocalViewsShrinkToFitTheAtlas)
{
    checkLocalViewsShrinkToFitTheAtlas (GetParam());
}

TEST_P (SceneRendererTierTests, LocalLightsKeepTheirQuarterTiles)
{
    checkLocalLightsKeepTheirQuarterTiles (GetParam());
}

TEST_P (SceneRendererTierTests, AtlasRelayoutRedrawsTheMovedTiles)
{
    checkAtlasRelayoutRedrawsTheMovedTiles (GetParam());
}

TEST_F (SceneRendererMetalTests, TierSwitchesKeepTheShadowsRight)
{
    // One renderer through raster, full with soft shadows, raster with soft shadows, raster with an
    // unshadowed sun and full again: the cascade texture changes its layers (2 and 4) and gives way
    // to its placeholder, the soft filter variants come and go, and each frame renders as a fresh
    // renderer of its tier does
    Scene scene;
    auto& sun = buildLongGroundScene (scene, 2.0f);
    sun.radius = 0.02f;

    const struct
    {
        RenderSettings::TierPreference tier;
        bool soft;
        bool castsShadows;
    } steps[] = {
        { RenderSettings::TierPreference::raster, false, true },
        { RenderSettings::TierPreference::full, true, true },
        { RenderSettings::TierPreference::raster, true, true },
        { RenderSettings::TierPreference::raster, false, false },
        { RenderSettings::TierPreference::full, false, true },
    };

    SceneRenderer renderer;
    for (const auto& step : steps)
    {
        setTierForExactFrames (scene, step.tier);
        auto settings = scene.getRenderSettings();
        settings.softShadows = step.soft;
        scene.setRenderSettings (settings);
        sun.castsShadows = step.castsShadows;

        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        ASSERT_NE (renderer.getCapabilities(), nullptr);
        EXPECT_EQ (shadows (renderer).getNumRecords(), step.castsShadows ? renderer.getCapabilities()->getCascadeCount (scene.getRenderSettings()) : 0);
        EXPECT_EQ (shadows (renderer).needsShadowFilters(), step.soft && step.castsShadows);

        SceneRenderer fresh;
        const auto reference = renderToImage (fresh, scene, 128);
        ASSERT_TRUE (image.isValid() && reference.isValid());
        EXPECT_EQ (countDifferences (image, reference), 0);
    }

    EXPECT_EQ (shadows (renderer).getNumRecords(), 4);
}

TEST_P (SceneRendererTierTests, AtlasShadowMapsRenderOnlyWhenTheShadowsChange)
{
    // A still scene with a shadowed spot draws its shadow maps once; a moved spot draws them again,
    // and so does a new range, which only the views of the spot's shadow see
    Scene scene;
    auto& spot = buildSpotOverGround (scene);
    setTier (scene, GetParam());

    SceneRenderer renderer;
    for (int frame = 0; frame < 3; ++frame)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);

    auto* entity = scene.getRoot()->findChild ("spot");
    ASSERT_NE (entity, nullptr);
    entity->setPosition (entity->getPosition() + Vector3<float> (0.05f, 0.0f, 0.0f));

    for (int frame = 0; frame < 2; ++frame)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 2);

    spot.range = 8.0f;
    for (int frame = 0; frame < 2; ++frame)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 3);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_P (SceneRendererTierTests, ShadowedRegionIsFinite)
{
    checkShadowedRegionIsFinite (GetParam());
}
#endif

TEST_P (SceneRendererTierTests, ContactShadowsDarkenSmallFeatures)
{
    if (GetParam() != RenderSettings::TierPreference::full)
        GTEST_SKIP() << "Full tier only";

    // Without contact shadows the sun map leaves the crease lit: its normal offset lifts the crease above the step
    float texel = 0.0f, unshadowedTexel = 0.0f;
    const auto unshadowed = renderCreaseScene (GetParam(), 256, false, false, unshadowedTexel);
    const auto without = renderCreaseScene (GetParam(), 256, false, true, texel);
    const auto withContact = renderCreaseScene (GetParam(), 256, true, true, texel);
    ASSERT_TRUE (unshadowed.isValid() && without.isValid() && withContact.isValid());

    const auto reference = meanLuminance (unshadowed, creaseRect());
    const auto lit = meanLuminance (without, creaseRect());
    const auto contact = meanLuminance (withContact, creaseRect());
    const auto openLit = meanLuminance (without, openGroundRect());
    const auto openContact = meanLuminance (withContact, openGroundRect());

    RecordProperty ("sunMapTexel", String (texel, 4).toStdString());
    RecordProperty ("creaseUnshadowed", String (reference, 2).toStdString());
    RecordProperty ("creaseMapOnly", String (lit, 2).toStdString());
    RecordProperty ("creaseContact", String (contact, 2).toStdString());
    RecordProperty ("openGroundMapOnly", String (openLit, 2).toStdString());
    RecordProperty ("openGroundContact", String (openContact, 2).toStdString());

    EXPECT_GT (lit, 0.95 * reference);
    EXPECT_LT (contact, 0.8f * lit);

    // Open ground marches without hitting anything
    EXPECT_NEAR (openContact, openLit, 0.02 * openLit);
}

TEST_P (SceneRendererTierTests, ContactShadowsIgnoredOnRaster)
{
    if (GetParam() != RenderSettings::TierPreference::raster)
        GTEST_SKIP() << "Raster only";

    checkContactShadowsIgnoredOnRaster (GetParam());
}

TEST_P (SceneRendererTierTests, ContactShadowSettingsLiveInTheFirstShadowRecord)
{
    checkContactShadowSettingsLiveInTheFirstShadowRecord (GetParam());
}

TEST_P (SceneRendererTierTests, ContactShadowOnlyRecordsHaveNoAtlasTile)
{
    if (GetParam() != RenderSettings::TierPreference::full)
        GTEST_SKIP() << "Full tier only";

    Scene scene;
    buildCreaseScene (scene, false);
    setTier (scene, GetParam());

    // A shadowed point light keeps the atlas alive; the second lies inside its own emitter, so its
    // views are dropped after its tiles were allocated, and its contact shadows take a record without a map
    for (const auto radius : { 0.0f, 0.5f })
    {
        auto entity = EntityNode::Ptr (new EntityNode ("point"));
        auto& light = entity->attach<LightNode> (LightNode::Type::point);
        light.castsShadows = true;
        light.contactShadows = true;
        light.radius = radius;
        light.range = 0.3f;
        entity->setPosition ({ 1.0f + 2.0f * radius, 0.1f, 0.0f });
        scene.getRoot()->addChild (entity);
    }

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.waitForGPU().wasOk());

    const auto data = shadows (renderer).getShadowData();
    const auto last = shadows (renderer).getNumRecords() - 1;
    ASSERT_GT (last, 0);
    EXPECT_EQ (data.records[last].atlasRect[2], 0.0f);
    EXPECT_FLOAT_EQ (data.records[last].contact[0], 0.2f);

    EXPECT_GT (shadows (renderer).getTileSize (1), 0);
    EXPECT_EQ (shadows (renderer).getTileSize (2), 0);
}

TEST_P (SceneRendererTierTests, ContactShadowRecordsGoToTheMostImportantLights)
{
    if (GetParam() != RenderSettings::TierPreference::full)
        GTEST_SKIP() << "Full tier only";

    Scene scene;
    buildCreaseScene (scene, true);
    setTier (scene, GetParam());

    // One record more than the budget leaves: 63 small far lights listed first, then a large one
    const auto addPoint = [&] (Vector3<float> position, float range, float distance)
    {
        auto entity = EntityNode::Ptr (new EntityNode ("point"));
        auto& light = entity->attach<LightNode> (LightNode::Type::point);
        light.contactShadows = true;
        light.contactShadowDistance = distance;
        light.range = range;
        entity->setPosition (position);
        scene.getRoot()->addChild (entity);
    };

    for (int i = 0; i < GpuShadowData::maxRecords - 1; ++i)
        addPoint ({ -5.0f - 0.1f * static_cast<float> (i), 0.1f, -10.0f }, 0.05f, 0.1f);

    addPoint ({ 0.5f, 0.5f, 0.0f }, 30.0f, 0.3f);

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.waitForGPU().wasOk());

    const auto data = shadows (renderer).getShadowData();
    ASSERT_EQ (shadows (renderer).getNumRecords(), GpuShadowData::maxRecords);
    EXPECT_TRUE (std::any_of (std::begin (data.records), std::end (data.records), [] (const GpuShadowRecord& record)
    {
        return record.contact[0] == 0.3f;
    }));
}

TEST_P (SceneRendererTierTests, ContactShadowsSpareAreaLightsBelowTheHorizon)
{
    if (GetParam() != RenderSettings::TierPreference::full)
        GTEST_SKIP() << "Full tier only";

    // A sphere light whose center lies 0.1 below the ground, its top poking 0.3 above it: the ray
    // towards its center runs into the ground, which must not shadow what the sphere lights
    const auto render = [&] (bool contact)
    {
        Scene scene;
        addGround (scene, 5.0f);
        addCamera (scene, { 0.0f, 2.0f, 1.5f }, {});
        setTier (scene, GetParam());

        auto entity = EntityNode::Ptr (new EntityNode ("sphere"));
        auto& light = entity->attach<LightNode> (LightNode::Type::point);
        light.intensity = 5.0f;
        light.radius = 0.4f;
        light.contactShadows = contact;
        entity->setPosition ({ 0.0f, -0.1f, 0.0f });
        scene.getRoot()->addChild (entity);

        SceneRenderer renderer;
        const auto image = renderConverged (renderer, scene, 128);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    };

    const auto without = render (false);
    const auto withContact = render (true);
    ASSERT_TRUE (without.isValid() && withContact.isValid());

    const Rectangle<int> around (40, 40, 48, 48);
    const auto lit = meanLuminance (without, around);
    const auto contact = meanLuminance (withContact, around);
    RecordProperty ("lit", String (lit, 2).toStdString());
    RecordProperty ("contact", String (contact, 2).toStdString());

    ASSERT_GT (lit, 20.0);
    EXPECT_GT (contact, 0.98 * lit);
}

TEST_P (SceneRendererTierTests, PcssPenumbraWidensWithLightRadius)
{
    checkPcssPenumbraWidensWithLightRadius (GetParam());
}

TEST_P (SceneRendererTierTests, PenumbraWidensWithDistanceFromTheBlocker)
{
    checkPenumbraWidensWithDistanceFromTheBlocker (GetParam());
}

TEST_P (SceneRendererTierTests, RasterSoftShadowsOptIn)
{
    if (GetParam() != RenderSettings::TierPreference::raster)
        GTEST_SKIP() << "Raster only";

    checkRasterSoftShadowsOptIn();
}

TEST_P (SceneRendererTierTests, SpotPenumbraWidensWithLightRadius)
{
    checkSpotPenumbraWidensWithLightRadius (GetParam());
}

TEST_P (SceneRendererTierTests, SoftShadowsFilterAtLeastTwoTexels)
{
    checkSoftShadowsFilterAtLeastTwoTexels (GetParam());
}

TEST_P (SceneRendererTierTests, SoftPointShadowHasNoFaceSeams)
{
    checkPointShadowHasNoFaceSeams (GetParam(), 0.1f);
}

TEST_P (SceneRendererTierTests, SoftSunLeavesAGrazedGroundLit)
{
    checkSoftSunLeavesAGrazedGroundLit (GetParam());
}

TEST_P (SceneRendererTierTests, RasterSoftShadowVariantFollowsTheSettings)
{
    if (GetParam() != RenderSettings::TierPreference::raster)
        GTEST_SKIP() << "Raster only: the helper builds a raster scene";

    checkRasterSoftShadowVariantFollowsTheSettings();
}

TEST_P (SceneRendererTierTests, SoftShadowsConvergeToAStableImage)
{
    if (GetParam() != RenderSettings::TierPreference::full)
        GTEST_SKIP() << "Full only: raster never accumulates";

    // The rotated taps average out over the accumulation: a renderer that starts 7 frames later,
    // with other jitter offsets and tap rotations, converges to nearly the same image, and the
    // penumbra rises from the shadow to the light without a step back. The accumulation is a true
    // mean of taaSamples frames, so what is left is the statistical spread of two different sets
    // of 16 x 8 rotated taps over the widest penumbra: measured at 452 pixels beyond the 2 levels
    // countDifferences() ignores, at most 5 levels apart (6 pixels with 64 samples). Unresolved
    // noise, one rotated tap per frame, leaves 3612 pixels up to 21 levels apart
    const auto first = renderPenumbraScene (GetParam(), 0.05f, true);
    const auto second = renderPenumbraScene (GetParam(), 0.05f, true, 7);
    ASSERT_TRUE (first.isValid() && second.isValid());

    const auto differing = countDifferences (first, second);
    const auto largest = maxChannelDifference (first, second);
    RecordProperty ("differing", differing);
    RecordProperty ("largestDifference", largest);
    EXPECT_LE (differing, 600);
    EXPECT_LE (largest, 7);

    const auto row = luminanceRow (first, first.getHeight() / 2);
    const auto rising = row.back() > row.front();

    auto stepBack = 0.0;
    for (size_t x = 1; x < row.size(); ++x)
        stepBack = jmax (stepBack, rising ? row[x - 1] - row[x] : row[x] - row[x - 1]);

    const auto width = penumbraWidth (first);
    RecordProperty ("stepBack", String (stepBack, 2).toStdString());
    RecordProperty ("width", width);
    EXPECT_LE (stepBack, 2.0);
    EXPECT_GT (width, 8); // A soft penumbra, not the hard filter's couple of pixels
}

TEST_P (SceneRendererTierTests, SoftShadowsWithoutTaaHoldStill)
{
    if (GetParam() != RenderSettings::TierPreference::full)
        GTEST_SKIP() << "Full only: raster has no temporal anti-aliasing to turn off";

    // Without temporal anti-aliasing nothing averages a moving pattern: the soft shadow taps keep
    // a still rotation, so consecutive frames match, and the penumbra is still soft
    const auto render = [&] (bool softShadows, Image& next)
    {
        Scene scene;
        buildPenumbraScene (scene, 0.05f);
        setFullTierTaa (scene, 1);

        auto settings = scene.getRenderSettings();
        settings.softShadows = softShadows;
        scene.setRenderSettings (settings);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 256);
        next = renderToImage (renderer, scene, 256);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    };

    Image soft, softNext, hard, hardNext;
    soft = render (true, softNext);
    hard = render (false, hardNext);
    ASSERT_TRUE (soft.isValid() && softNext.isValid() && hard.isValid());

    // The canvas read back dithers the frames by a level, within what countDifferences() ignores
    const auto flickering = countDifferences (soft, softNext);
    const auto softWidth = penumbraWidth (soft);
    const auto hardWidth = penumbraWidth (hard);
    RecordProperty ("flickering", flickering);
    RecordProperty ("soft", softWidth);
    RecordProperty ("hard", hardWidth);
    EXPECT_EQ (flickering, 0);
    EXPECT_GT (softWidth, hardWidth + 4);
}

TEST_P (SceneRendererTierTests, ShadowFilterVariantFollowsSoftAndContactShadows)
{
    checkShadowFilterVariantFollowsSoftAndContactShadows (GetParam());
}

TEST_P (SceneRendererTierTests, ContactOnlyLightWithARadiusLeavesSoftFilteringOff)
{
    if (GetParam() != RenderSettings::TierPreference::full)
        GTEST_SKIP() << "Full tier only";

    // A light with contact shadows but no shadow map holds a record without a map: its radius has
    // no shadow to soften, so the frame asks for no soft filtering, while its contact shadows
    // still select the shadow filter variant. The sun casts a hard shadow (radius 0)
    Scene scene;
    buildCreaseScene (scene, false);
    setTier (scene, GetParam());

    auto entity = EntityNode::Ptr (new EntityNode ("point"));
    auto& light = entity->attach<LightNode> (LightNode::Type::point);
    light.contactShadows = true;
    light.radius = 0.1f;
    entity->setPosition ({ 0.5f, 0.5f, 0.0f });
    scene.getRoot()->addChild (entity);

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.waitForGPU().wasOk());

    const auto data = shadows (renderer).getShadowData();
    const auto last = shadows (renderer).getNumRecords() - 1;
    ASSERT_GT (last, 0);
    EXPECT_EQ (data.records[last].atlasRect[2], 0.0f);
    EXPECT_GT (data.records[last].contact[0], 0.0f);

    EXPECT_EQ (data.filterInfo[0], 0.0f);
    EXPECT_EQ (data.filterInfo[1], 0.0f);
    EXPECT_TRUE (shadows (renderer).needsShadowFilters());
}

TEST_P (SceneRendererTierTests, ContactShadowsOfALightOutOfRangeChangeNothing)
{
    if (GetParam() != RenderSettings::TierPreference::full)
        GTEST_SKIP() << "Full tier only";

    // A point light just above the step of buildCreaseScene(), on the sun's side. Within its range
    // its contact shadows darken the ground behind the step. Out of its range it lights nothing,
    // and its contact record, still allocated, leaves the frame as it is without contact shadows,
    // although that frame shades with the shadow filter variant and the other with the default
    const auto render = [&] (float range, bool contact)
    {
        Scene scene;
        buildCreaseScene (scene, false);
        setTier (scene, GetParam());

        auto entity = EntityNode::Ptr (new EntityNode ("point"));
        auto& light = entity->attach<LightNode> (LightNode::Type::point);
        light.contactShadows = contact;
        light.range = range;
        entity->setPosition ({ -0.5f, 0.15f, 0.0f });
        scene.getRoot()->addChild (entity);

        SceneRenderer renderer;
        const auto image = renderConverged (renderer, scene, 256);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        EXPECT_EQ (shadows (renderer).needsShadowFilters(), contact);

        const auto data = shadows (renderer).getShadowData();
        const auto marches = std::any_of (std::begin (data.records), std::end (data.records), [] (const GpuShadowRecord& record)
        {
            return record.contact[0] > 0.0f;
        });

        EXPECT_EQ (marches, contact);
        return image;
    };

    const auto inRange = render (2.0f, false);
    const auto inRangeContact = render (2.0f, true);
    const auto outOfRange = render (0.08f, false);
    const auto outOfRangeContact = render (0.08f, true);
    ASSERT_TRUE (inRange.isValid() && inRangeContact.isValid() && outOfRange.isValid() && outOfRangeContact.isValid());

    const auto darkened = countDifferences (inRangeContact, inRange);
    const auto changed = countDifferences (outOfRangeContact, outOfRange);
    RecordProperty ("inRangeDarkened", darkened);
    RecordProperty ("outOfRangeChanged", changed);

    EXPECT_GT (darkened, 0);
    EXPECT_EQ (changed, 0);
}

TEST_P (SceneRendererTierTests, ReflectionProbeShowsWhatTheCameraCannotSee)
{
    checkReflectionProbeShowsWhatTheCameraCannotSee (GetParam());
}

TEST_P (SceneRendererTierTests, ReplacedProbeIsBakedAgain)
{
    checkReplacedProbeIsBakedAgain (GetParam());
}

TEST_P (SceneRendererTierTests, MovingAProbeBakesItAgain)
{
    checkMovingAProbeBakesItAgain (GetParam());
}

TEST_P (SceneRendererTierTests, ProbeBlendWithoutEnvironmentIsFinite)
{
    checkProbeBlendWithoutEnvironmentIsFinite (GetParam());
}

TEST_P (SceneRendererTierTests, BoxProbeWithParallaxReflects)
{
    checkBoxProbeWithParallaxReflects (GetParam());
}

TEST_P (SceneRendererTierTests, BakeProbesReportsPendingCount)
{
    checkBakeProbesReportsPendingCount (GetParam());
}

TEST_P (SceneRendererTierTests, RemovedProbesReleaseTheAtlas)
{
    checkRemovedProbesReleaseTheAtlas (GetParam());
}

TEST_P (SceneRendererTierTests, ProbeOfAnOpenSkyMatchesTheEnvironment)
{
    checkProbeOfAnOpenSkyMatchesTheEnvironment (GetParam());
}

TEST_P (SceneRendererTierTests, RgbmProbeShowsLikeTheFloatOne)
{
    checkRgbmProbeShowsLikeTheFloatOne (GetParam());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_P (SceneRendererTierTests, GpuFailureBakesTheProbesAgain)
{
    // The frame baking the probe failed on the GPU: the next frame cannot trust its tile
    Scene scene;
    buildMirrorFloorScene (scene);
    setTier (scene, GetParam());
    addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);

    SceneRenderer renderer;
    device->failNextFrameForTesting ("injected failure");
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (getNumBakes (renderer), 1);

    // Frames complete in order, so once a later frame did the failed one has too
    {
        auto fence = GpuFrame::begin (device);
        EXPECT_TRUE (fence.waitForGPU().wasOk());
    }

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getLastGpuFailure(), "injected failure");
    EXPECT_EQ (getNumBakes (renderer), 2);

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (getNumBakes (renderer), 2);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}
#endif

TEST_P (SceneRendererTierTests, ProbeKeepsTheMultipleScatteringOfRoughMetals)
{
    checkProbeKeepsTheMultipleScatteringOfRoughMetals (GetParam());
}

TEST_P (SceneRendererTierTests, BoxProbeParallaxPlacesTheReflectionOnTheWalls)
{
    checkBoxProbeParallaxPlacesTheReflectionOnTheWalls (GetParam());
}

TEST_P (SceneRendererTierTests, FailedProbeCapturesLeaveTheFrameWithoutProbes)
{
    checkFailedProbeCapturesLeaveTheFrameWithoutProbes (GetParam());
}

TEST_F (SceneRendererMetalTests, NestedProbesBlendAlikeOnBothTiers)
{
    // The large probe captures the emitter green; the small one, nested in it over the near floor,
    // captures it red afterwards. Raster picks both for the floor, full blends them per pixel in the
    // same order: the floor matches band by band
    const auto render = [this] (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        setTierForExactFrames (scene, tier);
        addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f, "large");

        SceneRenderer renderer;
        EXPECT_EQ (renderer.bakeProbes (device, scene), 1);

        setEmitterColor (scene, { 1.0f, 0.0f, 0.0f });
        addReflectionProbe (scene, { 0.0f, 0.5f, 0.5f }, 1.5f, "small").falloff = 0.5f;

        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 2);
        return image;
    };

    const auto raster = render (RenderSettings::TierPreference::raster);
    const auto full = render (RenderSettings::TierPreference::full);
    ASSERT_TRUE (raster.isValid() && full.isValid());

    auto green = false, red = false;
    for (const auto& band : { Rectangle<int> (16, 4, 32, 8), Rectangle<int> (16, 16, 32, 8), Rectangle<int> (16, 28, 32, 8), Rectangle<int> (16, 44, 32, 12) })
    {
        const auto expected = meanColor (raster, band);
        const auto actual = meanColor (full, band);
        RecordProperty (("raster" + band.toString()).toStdString(), expected.toString().toStdString());
        RecordProperty (("full" + band.toString()).toStdString(), actual.toString().toStdString());

        EXPECT_NEAR (actual.getRed(), expected.getRed(), 6) << band.toString();
        EXPECT_NEAR (actual.getGreen(), expected.getGreen(), 6) << band.toString();
        EXPECT_NEAR (actual.getBlue(), expected.getBlue(), 6) << band.toString();

        green = green || expected.getGreen() > expected.getRed() + 30;
        red = red || expected.getRed() > expected.getGreen() + 30;
    }

    // Not vacuous: the far floor shows the large probe, the near one the small
    EXPECT_TRUE (green);
    EXPECT_TRUE (red);
}

TEST_F (SceneRendererMetalTests, FullTierShadesAtMost64Probes)
{
    // 65 baked probes: the full tier packs the 64 smallest, the largest left out, logged and not asserted
    Scene scene;
    for (int i = 1; i <= 65; ++i)
        addReflectionProbe (scene, { static_cast<float> (i), 0.0f, 0.0f }, static_cast<float> (i), "probe");

    SceneRenderer renderer;
    ASSERT_EQ (renderer.bakeProbes (device, scene), 65);
    ASSERT_NE (renderer.getProbeBaker(), nullptr);

    SceneDrawList list;
    list.build (*scene.getRoot(), Matrix4());
    ASSERT_EQ (list.getReflectionProbes().size(), 65u);

    const auto packed = renderer.getProbeBaker()->packReflectionProbesByPriority (list);
    ASSERT_EQ (packed.size(), 64u);
    EXPECT_FLOAT_EQ (packed.front().influence[0], 1.0f);
    EXPECT_FLOAT_EQ (packed.back().influence[0], 64.0f);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_F (SceneRendererMetalTests, FullTierPacksTheProbesInTheRasterPriorityOrder)
{
    // The smaller influence first whatever the bake order, so the full tier blends as raster ranks
    Scene scene;
    buildMirrorFloorScene (scene);
    addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f, "large");
    addReflectionProbe (scene, { 1.0f, 1.0f, -1.0f }, 5.0f, "small");

    SceneRenderer renderer;
    ASSERT_EQ (renderer.bakeProbes (device, scene), 2);
    ASSERT_NE (renderer.getProbeBaker(), nullptr);

    SceneDrawList list;
    list.build (*scene.getRoot(), Matrix4());

    const auto packed = renderer.getProbeBaker()->packReflectionProbesByPriority (list);
    ASSERT_EQ (packed.size(), 2u);
    EXPECT_FLOAT_EQ (packed[0].influence[0], 5.0f);
    EXPECT_FLOAT_EQ (packed[1].influence[0], 20.0f);

    // Both baked into their own tile of the atlas
    EXPECT_GT (packed[0].atlasRect[2], 0.0f);
    EXPECT_NE (packed[0].atlasRect[0], packed[1].atlasRect[0]);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_F (SceneRendererMetalTests, ReflectionProbeChangesRestartAccumulation)
{
    // Each change shows on the full tier once converged: a bake asked for, a field of the probe, and
    // its entity moving, which also bakes it again
    const std::pair<const char*, std::function<void (EntityNode&)>> changes[] = {
        { "requestBake", [] (EntityNode& probe) { probe.getNode<ReflectionProbeNode>()->requestBake(); } },
        { "intensity", [] (EntityNode& probe) { probe.getNode<ReflectionProbeNode>()->intensity = 0.5f; } },
        { "falloff", [] (EntityNode& probe) { probe.getNode<ReflectionProbeNode>()->falloff = 2.0f; } },
        { "position", [] (EntityNode& probe) { probe.setPosition ({ 0.5f, 1.0f, -1.0f }); } },
    };

    for (const auto& [name, change] : changes)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        setFullTierTaa (scene, 8);
        addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);

        SceneRenderer renderer;
        for (int i = 0; i < 12; ++i)
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        ASSERT_TRUE (renderer.isConverged()) << name;

        change (*scene.getRoot()->findChild ("probe"));

        // Restarted, not invalidated
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_FALSE (renderer.isConverged()) << name;
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }
}

TEST_P (SceneRendererTierTests, IrradianceVolumeTintsTheFloorNearTheRedWall)
{
    checkIrradianceVolumeTintsTheFloorNearTheRedWall (GetParam());
}

TEST_P (SceneRendererTierTests, VisibilityStopsLightLeakingThroughTheWall)
{
    checkVisibilityStopsLightLeakingThroughTheWall (GetParam());
}

TEST_P (SceneRendererTierTests, MovingAVolumeBakesItAgain)
{
    checkMovingAVolumeBakesItAgain (GetParam());
}

TEST_P (SceneRendererTierTests, BakeProbesBakesTheVolumes)
{
    checkBakeProbesBakesTheVolumes (GetParam());
}

TEST_P (SceneRendererTierTests, RemovedVolumesReleaseTheAtlases)
{
    checkRemovedVolumesReleaseTheAtlases (GetParam());
}

TEST_P (SceneRendererTierTests, RgbmVolumeLightsLikeTheFloatOne)
{
    checkRgbmVolumeLightsLikeTheFloatOne (GetParam());
}

TEST_P (SceneRendererTierTests, FailedVolumeCapturesLeaveTheFrameWithoutVolumes)
{
    checkFailedVolumeCapturesLeaveTheFrameWithoutVolumes (GetParam());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_P (SceneRendererTierTests, GpuFailureBakesTheVolumesAgain)
{
    // The frame baking the volume failed on the GPU: the next frame cannot trust its blocks. The
    // environment cube is built first, in a frame of its own, so the failure hits the bake
    Scene scene;
    buildRedRoomScene (scene, false);
    setTier (scene, GetParam());

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.waitForGPU().wasOk());

    addIrradianceVolume (scene, 4, 2, 2);
    device->failNextFrameForTesting ("injected failure");
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (getNumVolumeBakes (renderer), 1);

    // Frames complete in order, so once a later frame did the failed one has too
    {
        auto fence = GpuFrame::begin (device);
        EXPECT_TRUE (fence.waitForGPU().wasOk());
    }

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getLastGpuFailure(), "injected failure");
    EXPECT_EQ (getNumVolumeBakes (renderer), 2);

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (getNumVolumeBakes (renderer), 2);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}
#endif

TEST_P (SceneRendererTierTests, ChangedVolumeBakesAgainWhole)
{
    checkChangedVolumeBakesAgainWhole (GetParam());
}

TEST_P (SceneRendererTierTests, FailedProbeCapturesLeaveTheVolumesLighting)
{
    checkFailedProbeCapturesLeaveTheVolumesLighting (GetParam());
}

TEST_P (SceneRendererTierTests, ProgressiveVolumeBakeShadesOnceComplete)
{
    checkProgressiveVolumeBakeShadesOnceComplete (GetParam());
}

TEST_P (SceneRendererTierTests, FailedVolumeCapturesLeaveTheProbesReflecting)
{
    checkFailedVolumeCapturesLeaveTheProbesReflecting (GetParam());
}

TEST_P (SceneRendererTierTests, ResizingAVolumeBakesItAgain)
{
    checkResizingAVolumeBakesItAgain (GetParam());
}

TEST_P (SceneRendererTierTests, VolumeLightsWithoutTheEnvironmentAndKeepsItsIntensity)
{
    checkVolumeLightsWithoutTheEnvironmentAndKeepsItsIntensity (GetParam());
}

TEST_F (SceneRendererMetalTests, IrradianceVolumeShadesAlikeOnBothTiers)
{
    // Raster picks the volume per draw by its center, full per pixel: with one volume over the whole
    // floor both shade the floor from the same probes. The tolerance of 6 levels is T7's tier parity
    // one: the two tiers differ by their geometric normal (the full tier stands the shading normal in
    // for it) and their AO and MSAA, about a level here
    const auto render = [this] (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene);
        setTierForExactFrames (scene, tier);
        addIrradianceVolume (scene, 6, 2, 4);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    };

    const auto raster = render (RenderSettings::TierPreference::raster);
    const auto full = render (RenderSettings::TierPreference::full);
    ASSERT_TRUE (raster.isValid() && full.isValid());

    for (const auto& region : { leftFloorRegion(), rightFloorRegion(), Rectangle<int> (40, 19, 16, 26) })
    {
        const auto expected = meanColor (raster, region);
        const auto actual = meanColor (full, region);
        RecordProperty (("raster" + region.toString()).toStdString(), expected.toString().toStdString());
        RecordProperty (("full" + region.toString()).toStdString(), actual.toString().toStdString());

        EXPECT_NEAR (actual.getRed(), expected.getRed(), 6) << region.toString();
        EXPECT_NEAR (actual.getGreen(), expected.getGreen(), 6) << region.toString();
        EXPECT_NEAR (actual.getBlue(), expected.getBlue(), 6) << region.toString();
    }

    // Not vacuous: the volume tints the floor between the walls
    EXPECT_GT (redOverGreen (meanColor (raster, leftFloorRegion())), 1.3f);
}

TEST_F (SceneRendererMetalTests, IrradianceVolumeChangesRestartAccumulation)
{
    // Each change shows on the full tier once converged: a bake asked for, a field of the volume,
    // and its entity moving, which also bakes it again
    const std::pair<const char*, std::function<void (EntityNode&)>> changes[] = {
        { "requestBake", [] (EntityNode& volume) { volume.getNode<IrradianceVolumeNode>()->requestBake(); } },
        { "intensity", [] (EntityNode& volume) { volume.getNode<IrradianceVolumeNode>()->intensity = 0.5f; } },
        { "falloff", [] (EntityNode& volume) { volume.getNode<IrradianceVolumeNode>()->falloff = 1.0f; } },
        { "normalBias", [] (EntityNode& volume) { volume.getNode<IrradianceVolumeNode>()->normalBias = 0.2f; } },
        { "position", [] (EntityNode& volume) { volume.setPosition ({ 0.0f, 1.6f, 0.0f }); } },
    };

    for (const auto& [name, change] : changes)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        setFullTierTaa (scene, 8);
        addIrradianceVolume (scene, 4, 2, 2);

        SceneRenderer renderer;
        for (int i = 0; i < 12; ++i)
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        ASSERT_TRUE (renderer.isConverged()) << name;

        change (*scene.getRoot()->findChild ("volume"));

        // Restarted, not invalidated
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_FALSE (renderer.isConverged()) << name;
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }
}

TEST_F (SceneRendererMetalTests, LightingPassShadesProbesAndVolumesWithItsLocalIndirectVariant)
{
    // Preflight R27: the full tier shades its opaque surfaces with the local indirect variants of
    // the lighting pass only in the frames with reflection probes or irradiance volumes
    Scene scene;
    buildRedRoomScene (scene, false);
    setFullTierTaa (scene, 1);

    SceneRenderer renderer;
    const auto usesLocalIndirect = [&]
    {
        EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        const auto* lightingPass = renderer.getLightingPass();
        return lightingPass != nullptr && lightingPass->usesLocalIndirect();
    };

    EXPECT_FALSE (usesLocalIndirect());

    addIrradianceVolume (scene, 4, 2, 2);
    EXPECT_TRUE (usesLocalIndirect());

    scene.getRoot()->removeChild (scene.getRoot()->findChild ("volume"));
    EXPECT_FALSE (usesLocalIndirect());

    addReflectionProbe (scene, { 0.0f, 1.0f, 0.0f }, 20.0f);
    EXPECT_TRUE (usesLocalIndirect());
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_F (SceneRendererMetalTests, FailedLocalIndirectLightingShadesWithoutVolumes)
{
    // A local indirect variant the device cannot compile leaves the full tier's frames rendering,
    // their opaque surfaces without the volume
    const auto render = [this] (bool fail)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        setFullTierTaa (scene, 1);
        addIrradianceVolume (scene, 4, 2, 2);

        SceneRenderer renderer;
        if (fail)
            renderer.failLocalIndirectLightingForTesting();

        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        EXPECT_NE (renderer.getLightingPass(), nullptr);
        EXPECT_EQ (renderer.getLightingPass() != nullptr && renderer.getLightingPass()->usesLocalIndirect(), ! fail);
        return image;
    };

    const auto failed = render (true);
    const auto working = render (false);
    ASSERT_TRUE (failed.isValid() && working.isValid());

    EXPECT_NEAR (redOverGreen (meanColor (failed, leftFloorRegion())), 1.0f, 0.05f);
    EXPECT_GT (redOverGreen (meanColor (working, leftFloorRegion())), 1.2f); // Not vacuous
}

TEST_F (SceneRendererMetalTests, FullTierShadesAtMost8Volumes)
{
    // 9 baked volumes: the full tier packs the 8 smallest, the largest left out, logged and not asserted
    Scene scene;
    for (int i = 1; i <= 9; ++i)
    {
        auto entity = EntityNode::Ptr (new EntityNode ("volume"));
        auto& volume = entity->attach<IrradianceVolumeNode>();
        volume.extents = { static_cast<float> (i), 1.0f, 1.0f };
        volume.resolutionX = volume.resolutionY = volume.resolutionZ = 2;
        scene.getRoot()->addChild (entity);
    }

    // 72 probe positions take two calls (ProbeBaker::maxVolumeProbesPerBake)
    SceneRenderer renderer;
    auto calls = 0;
    while (renderer.bakeProbes (device, scene) > 0 && calls < 8)
        ++calls;

    EXPECT_EQ (calls, (9 * 8 + ProbeBaker::maxVolumeProbesPerBake - 1) / ProbeBaker::maxVolumeProbesPerBake);
    ASSERT_EQ (getNumVolumeBakes (renderer), 9);
    ASSERT_NE (renderer.getProbeBaker(), nullptr);

    SceneDrawList list;
    list.build (*scene.getRoot(), Matrix4());
    ASSERT_EQ (list.getIrradianceVolumes().size(), 9u);

    const auto packed = renderer.getProbeBaker()->packIrradianceVolumesByPriority (list);
    ASSERT_EQ (packed.size(), 8u);

    // Smallest first: worldToUnit scales x by 1 / (2 * extent)
    EXPECT_FLOAT_EQ (packed.front().worldToUnit[0], 0.5f);
    EXPECT_FLOAT_EQ (packed.back().worldToUnit[0], 1.0f / 16.0f);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_F (SceneRendererMetalTests, VolumeBakeUniformPoolStaysSmall)
{
    // Preflight R18 and R28: volume bakes in the red room plus 100 small casters, each its own draw
    // on every capture face: the brief's 6 x 2 x 4 volume, then the largest one, 7 x 7 x 7, which
    // bakes over several frames, waited on and then not. The device's uniform buffers, pooled or held
    // by frames in flight, are recorded once each bake completed and the frames returned them
    const auto bake = [this] (int nx, int ny, int nz, const char* name, bool waitEachFrame = true)
    {
        Scene scene;
        buildRedRoomScene (scene);
        addSmallCasters (scene, 100);
        addIrradianceVolume (scene, nx, ny, nz);

        SceneRenderer renderer;
        auto frames = 0;
        while (getNumVolumeBakes (renderer) < 1 && frames < 64)
        {
            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            if (waitEachFrame)
                EXPECT_TRUE (renderer.waitForGPU().wasOk());
            ++frames;
        }

        for (int i = 0; i < 3; ++i)
        {
            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk());
        }

        EXPECT_EQ (getNumVolumeBakes (renderer), 1) << name;

        const auto pool = device->getUniformBufferPoolSizeForTesting();
        RecordProperty ((String (name) + "FramesToBake").toStdString(), frames);
        RecordProperty ((String (name) + "UniformBuffers").toStdString(), pool.first);
        RecordProperty ((String (name) + "UniformBytes").toStdString(), static_cast<int> (pool.second));
        return pool;
    };

    // The pool only grows, so each figure is the running peak of the device after that bake, an upper bound of it
    const auto before = device->getUniformBufferPoolSizeForTesting();
    const auto brief = bake (6, 2, 4, "brief");
    const auto largest = bake (IrradianceVolumeNode::maxResolution, IrradianceVolumeNode::maxResolution, IrradianceVolumeNode::maxResolution, "largest");

    // As an application renders, without waiting: the frames in flight hold their buffers longer
    const auto unwaited = bake (IrradianceVolumeNode::maxResolution, IrradianceVolumeNode::maxResolution, IrradianceVolumeNode::maxResolution, "unwaited", false);

    EXPECT_GT (brief.second, before.second);
    EXPECT_LE (brief.second, static_cast<size_t> (16 * 1024 * 1024));
    EXPECT_LE (largest.second, static_cast<size_t> (16 * 1024 * 1024));
    EXPECT_LE (unwaited.second, static_cast<size_t> (16 * 1024 * 1024));
}

TEST_P (SceneRendererTierTests, ClearcoatAddsASharpHighlightOnARoughBase)
{
    checkClearcoatAddsASharpHighlightOnARoughBase (GetParam());
}

TEST_P (SceneRendererTierTests, SheenBrightensGrazingEdges)
{
    checkSheenBrightensGrazingEdges (GetParam());
}

TEST_P (SceneRendererTierTests, SheenPassesTheWhiteFurnaceTest)
{
    checkSheenPassesTheWhiteFurnaceTest (GetParam());
}

TEST_P (SceneRendererTierTests, LayeredVariantFollowsTheMaterials)
{
    checkLayeredVariantFollowsTheMaterials (GetParam());
}

TEST_P (SceneRendererTierTests, FailedLayersVariantShadesWithoutTheLayers)
{
    checkFailedLayersVariantShadesWithoutTheLayers (GetParam());
}

TEST_P (SceneRendererTierTests, SheenOverAWhiteBaseKeepsTheEnergy)
{
    checkSheenOverAWhiteBaseKeepsTheEnergy (GetParam());
}

TEST_P (SceneRendererTierTests, ClearcoatDimsTheEmission)
{
    checkClearcoatDimsTheEmission (GetParam());
}

TEST_P (SceneRendererTierTests, ClearcoatReflectsTheProbe)
{
    checkClearcoatReflectsTheProbe (GetParam());
}

TEST_F (SceneRendererMetalTests, BlendedLayeredItemShadesOnFull)
{
    // The full forward pass shades blended items, and always compiles the layers: a half transparent
    // plate with a clearcoat, under the head-on sun and a sky, gains the coat's highlight there, and
    // stays finite. The lighting pass shades no layer, the plate being its only item
    const auto render = [this] (float clearcoat, GpuTexture::Ptr* sceneColor)
    {
        auto material = makeLayeredMaterial ({ 0.5f, 0.5f, 0.5f }, clearcoat, 0.2f, {}, 0.0f);
        material->baseColorFactor[3] = 0.5f;
        material->alphaMode = Material::AlphaMode::blend;

        Scene scene;
        addPlate (scene, material);
        scene.setAmbientColor (Colors::black);
        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>& d) { return Vector3<float> (0.2f, 0.2f, 0.2f) * (d.getY() * 0.5f + 0.5f); }, 32, 3));
        setFullTierTaa (scene, 1);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().intensity = 3.0f;
        scene.getRoot()->addChild (sun);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        EXPECT_FALSE (renderer.getLightingPass() != nullptr && renderer.getLightingPass()->usesMaterialLayers());

        if (sceneColor != nullptr)
            *sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);

        return image;
    };

    GpuTexture::Ptr sceneColor;
    const auto bare = render (0.0f, nullptr);
    const auto coated = render (1.0f, &sceneColor);
    ASSERT_TRUE (bare.isValid() && coated.isValid());

    const auto bareLuminance = meanLuminance (bare, { 28, 28, 8, 8 });
    const auto coatedLuminance = meanLuminance (coated, { 28, 28, 8, 8 });
    RecordProperty ("bare", String (bareLuminance, 1).toStdString());
    RecordProperty ("coated", String (coatedLuminance, 1).toStdString());
    EXPECT_GT (coatedLuminance, bareLuminance + 10.0);

#if YUP_ENABLE_SHADER_TRANSPILER
    ASSERT_NE (sceneColor, nullptr);
    EXPECT_TRUE (isFiniteTexture (sceneColor));
    EXPECT_FALSE (isSaturatedTexture (sceneColor));
#endif
}

TEST_F (SceneRendererMetalTests, ClearcoatShadesAlikeOnBothTiers)
{
    // A clearcoated sphere without a normal map, where the clearcoat normal of both tiers is the
    // surface normal, under the head-on sun and a sky: its direct and indirect lobes. The tolerance
    // of 6 levels is T7's tier parity one (the full tier's shading normal and the raster MSAA)
    const auto render = [this] (RenderSettings::TierPreference tier, float clearcoat)
    {
        Scene scene;
        addSphere (scene, 1.5f, makeLayeredMaterial ({ 0.6f, 0.1f, 0.1f }, clearcoat, 0.3f, {}, 0.0f));
        scene.setAmbientColor (Colors::black);
        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>& direction)
        {
            const auto up = jlimit (0.0f, 1.0f, direction.getY() * 0.5f + 0.5f);
            return Vector3<float> (0.1f, 0.1f, 0.1f) + Vector3<float> (1.0f, 1.2f, 1.5f) * up;
        }, 32, 3));
        setTierForExactFrames (scene, tier);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().intensity = 2.0f;
        scene.getRoot()->addChild (sun);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 128);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    };

    const auto raster = render (RenderSettings::TierPreference::raster, 1.0f);
    const auto full = render (RenderSettings::TierPreference::full, 1.0f);
    const auto uncoated = render (RenderSettings::TierPreference::raster, 0.0f);
    ASSERT_TRUE (raster.isValid() && full.isValid() && uncoated.isValid());

    // The highlight, the lit body, the upper and lower grazing edges reflecting the sky and the ground
    const Rectangle<int> regions[] = { { 60, 60, 8, 8 }, { 80, 56, 8, 16 }, { 60, 24, 8, 6 }, { 60, 98, 8, 6 } };
    auto coatChanges = 0;

    for (const auto& region : regions)
    {
        const auto expected = meanColor (raster, region);
        const auto actual = meanColor (full, region);
        RecordProperty (("raster" + region.toString()).toStdString(), expected.toString().toStdString());
        RecordProperty (("full" + region.toString()).toStdString(), actual.toString().toStdString());

        EXPECT_NEAR (actual.getRed(), expected.getRed(), 6) << region.toString();
        EXPECT_NEAR (actual.getGreen(), expected.getGreen(), 6) << region.toString();
        EXPECT_NEAR (actual.getBlue(), expected.getBlue(), 6) << region.toString();

        // Not vacuous: the clearcoat changes the region
        const auto bare = meanColor (uncoated, region);
        RecordProperty (("uncoated" + region.toString()).toStdString(), bare.toString().toStdString());
        coatChanges += std::abs (expected.getGreen() - bare.getGreen()) > 10 ? 1 : 0;
    }

    // The highlight and the sky reflected at the upper edge, at least
    EXPECT_GE (coatChanges, 2);
}

TEST_F (SceneRendererMetalTests, MaterialLayerChangesRestartAccumulation)
{
    // Preflight R11: each clearcoat and sheen field shows on the full tier once converged
    const std::pair<const char*, std::function<void (Material&)>> changes[] = {
        { "clearcoatFactor", [] (Material& material) { material.clearcoatFactor = 0.25f; } },
        { "clearcoatRoughnessFactor", [] (Material& material) { material.clearcoatRoughnessFactor = 0.6f; } },
        { "sheenColorFactor", [] (Material& material) { material.sheenColorFactor[1] = 0.5f; } },
        { "sheenRoughnessFactor", [] (Material& material) { material.sheenRoughnessFactor = 0.9f; } },
    };

    for (const auto& [name, change] : changes)
    {
        auto material = makeLayeredMaterial ({ 0.5f, 0.5f, 0.5f }, 1.0f, 0.2f, { 1.0f, 1.0f, 1.0f }, 0.3f);

        Scene scene;
        addPlate (scene, material);
        setFullTierTaa (scene, 4);

        SceneRenderer renderer;
        for (int i = 0; i < 4; ++i)
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        ASSERT_TRUE (renderer.isConverged()) << name;

        change (*material);
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_FALSE (renderer.isConverged()) << name;
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }
}

TEST_P (SceneRendererTierTests, DepthOfFieldBlursOutOfFocusBarsOnly)
{
    checkDepthOfFieldBlursOutOfFocusBarsOnly (GetParam());
}

TEST_P (SceneRendererTierTests, NearFieldBleedsOverTheFocusedBackground)
{
    checkNearFieldBleedsOverTheFocusedBackground (GetParam());
}

TEST_P (SceneRendererTierTests, InFocusEdgesMatchWithoutDepthOfField)
{
    checkInFocusEdgesMatchWithoutDepthOfField (GetParam());
}

TEST_P (SceneRendererTierTests, DepthOfFieldOffChangesNothing)
{
    checkDepthOfFieldOffChangesNothing (GetParam());
}

TEST_P (SceneRendererTierTests, DepthOfFieldKeepsTheSilhouetteBlendOverTheBackground)
{
    checkDepthOfFieldKeepsTheSilhouetteBlendOverTheBackground (GetParam());
}

TEST_P (SceneRendererTierTests, BlurredHighlightsKeepTheirBrightnessOverTheBackground)
{
    checkBlurredHighlightsKeepTheirBrightnessOverTheBackground (GetParam());
}

TEST_P (SceneRendererTierTests, MotionBlurOnRasterRendersTheUnblurredFallback)
{
    if (GetParam() != RenderSettings::TierPreference::raster)
        GTEST_SKIP() << "The full tier smears, see MotionBlurMetalTests";

    checkMotionBlurOnRasterRendersTheUnblurredFallback();
}

INSTANTIATE_TEST_SUITE_P (SceneRendererTiers,
                          SceneRendererTierTests,
                          ::testing::Values (RenderSettings::TierPreference::raster, RenderSettings::TierPreference::full),
                          [] (const auto& info)
                          {
                              return info.param == RenderSettings::TierPreference::full ? "full" : "raster";
                          });

#if YUP_ENABLE_SHADER_TRANSPILER
/** GBufferPass on its own targets, read back before any other pass draws over the depth and
    velocity it shares with them. */
class GBufferPassMetalTests : public SceneRendererMetalTests
{
protected:
    static constexpr int size = 64;

    void SetUp() override
    {
        SceneRendererMetalTests::SetUp();
        if (! IsSkipped())
            frameResources = std::make_unique<FrameResources> (device);
    }

    /** A green alpha masked material of base color alpha @p alpha, cut out below the default 0.5 cutoff. */
    static Material::Ptr makeMasked (float alpha)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.2f, 0.8f, 0.2f, alpha };
        material->alphaMode = Material::AlphaMode::mask;
        return material;
    }

    /** A square of @p halfSize centered at (@p x, 0, 0), facing the default camera. */
    static EntityNode::Ptr makeQuad (float x, float halfSize, Material::Ptr material)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { x - halfSize, -halfSize, 0.0f };
        vertices[1].position = { x + halfSize, -halfSize, 0.0f };
        vertices[2].position = { x + halfSize, halfSize, 0.0f };
        vertices[3].position = { x - halfSize, halfSize, 0.0f };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto mesh = Mesh::Ptr (new Mesh ("quad"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);

        auto entity = EntityNode::Ptr (new EntityNode ("quad"));
        entity->attach<MeshNode> (mesh);
        return entity;
    }

    /** Draws the opaque items of @p scene, moved by @p motion since the previous frame, into the
        targets of a frame. The previous view is the one of the previous call.
    */
    GBufferPass::Targets render (Scene& scene, Vector3<float> motion = {})
    {
        frameResources->beginFrame (size, size);

        const auto acquire = [&] (GpuTextureFormat format)
        {
            return frameResources->acquire (GpuTextureDesc (static_cast<uint32_t> (size), static_cast<uint32_t> (size), format, true));
        };

        const auto targets = GBufferPass::acquireTargets (*frameResources, acquire (GpuTextureFormat::rg16float), acquire (GpuTextureFormat::depth32float));

        view.update (scene, size, size, false, 1, 0);
        drawList.build (*scene.getRoot(), view.getView());

        for (const auto& item : drawList.getOpaqueItems())
            item.node->previousWorld = item.world.followedBy (Matrix4::translation ({ -motion.getX(), -motion.getY(), -motion.getZ() }));

        EXPECT_TRUE (gbufferPass.prepare (device).wasOk());

        auto frame = GpuFrame::begin (device);
        EXPECT_TRUE (gbufferPass.render (frame, view, drawList.getOpaqueItems(), materials, targets));
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        frameResources->endFrame();
        return targets;
    }

    std::unique_ptr<FrameResources> frameResources;
    ViewData view;
    SceneDrawList drawList;
    GBufferPass gbufferPass;
    MaterialTable materials;
};

TEST_F (GBufferPassMetalTests, MaskedTexelsAreNotWritten)
{
    // Below and above the default 0.5 cutoff, centered at about x = 23 and x = 41
    const auto cutOut = makeMasked (0.2f);
    const auto kept = makeMasked (0.8f);

    Scene scene;
    scene.getRoot()->addChild (makeQuad (-0.6f, 0.4f, cutOut));
    scene.getRoot()->addChild (makeQuad (0.6f, 0.4f, kept));

    const auto targets = render (scene);

    const auto ids = mapTexels (targets.emissionMaterial, "vec4(t.a / 255.0, 0.0, 0.0, 1.0)");
    const auto cleared = mapTexels (targets.depth, "vec4(t.r >= 1.0 ? 1.0 : 0.0, 0.0, 0.0, 1.0)");
    ASSERT_TRUE (ids.isValid());
    ASSERT_TRUE (cleared.isValid());

    const auto keptId = materials.getId (*kept);
    EXPECT_GT (keptId, 0);
    EXPECT_EQ (static_cast<int> (ids.getPixelColor (41, 32).getRed()), keptId);
    EXPECT_EQ (cleared.getPixelColor (41, 32).getRed(), 0);

    EXPECT_EQ (ids.getPixelColor (23, 32).getRed(), 0);
    EXPECT_EQ (cleared.getPixelColor (23, 32).getRed(), 255);
}

TEST_F (GBufferPassMetalTests, BackgroundFillCoversOnlyTheEmptyTexels)
{
    Scene scene;
    scene.getRoot()->addChild (makeQuad (0.0f, 0.5f, Material::Ptr (new Material())));

    auto camera = EntityNode::Ptr (new EntityNode ("camera"));
    camera->attach<CameraNode>();
    camera->setPosition ({ 0.0f, 0.0f, 5.0f });
    scene.getRoot()->addChild (camera);
    scene.setActiveCamera (camera.get());

    // A still camera: the quad, moved right since the previous frame, is the only motion
    const auto moved = readVelocity (render (scene, { 0.2f, 0.0f, 0.0f }).velocity, 1000.0f);
    ASSERT_TRUE (moved.isValid());
    EXPECT_GT (moved.getPixelColor (32, 32).getRed(), 192);
    EXPECT_EQ (velocityDeviation (moved.getPixelColor (2, 32).getRed()), 0);
    EXPECT_EQ (velocityDeviation (moved.getPixelColor (2, 32).getGreen()), 0);

    // A turning camera moves the background
    camera->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.05f));
    const auto turned = readVelocity (render (scene).velocity, 1000.0f);
    ASSERT_TRUE (turned.isValid());
    EXPECT_GT (velocityDeviation (turned.getPixelColor (2, 32).getRed()), 64);

    // And stops moving it once the camera stays
    const auto still = readVelocity (render (scene).velocity, 1000.0f);
    ASSERT_TRUE (still.isValid());
    EXPECT_EQ (maxVelocityDeviation (still), 0);
}

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

        auto camera = EntityNode::Ptr (new EntityNode ("camera"));
        camera->attach<CameraNode>();
        aimCamera (*camera, { 0.0f, 1.2f, 1.8f }, { 0.0f, 0.2f, 0.0f });
        scene.getRoot()->addChild (camera);
        scene.setActiveCamera (camera.get());

        frameResources->beginFrame (width, height);

        const auto acquire = [&] (GpuTextureFormat format)
        {
            return frameResources->acquire (GpuTextureDesc (static_cast<uint32_t> (width), static_cast<uint32_t> (height), format, true));
        };

        const auto targets = GBufferPass::acquireTargets (*frameResources, acquire (GpuTextureFormat::rg16float), acquire (GpuTextureFormat::depth32float));

        view.update (scene, width, height, false, 1, 0);
        drawList.build (*scene.getRoot(), view.getView());
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
        const auto vertex = File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory()
                                .getChildFile ("modules/yup_3d/shaders/yup_Fullscreen.vert")
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

/** ReflectionPass on its own, for the failure paths SceneRenderer reaches. */
class ReflectionPassMetalTests : public SceneRendererMetalTests
{
};

TEST_F (ReflectionPassMetalTests, PipelinesThatCannotCompileGiveUpWithTheirReason)
{
    // As SceneRenderer calls it: without pipelines no Hi-Z pyramid is built, so the pass is handed
    // none, and must still report the pipelines, not the pyramid, as why it gave up
    ReflectionPass pass (device);
    pass.failPipelinesForTesting();
    EXPECT_TRUE (pass.prepare().failed());

    FrameResources resources (device);
    ViewData view;

    for (int i = 0; i < 2; ++i)
    {
        EXPECT_FALSE (pass.hasGivenUp());
        resources.beginFrame (64, 64);
        auto frame = GpuFrame::begin (device);
        EXPECT_EQ (pass.render (frame, view, nullptr, nullptr, nullptr, nullptr, 1.0f, resources), nullptr);
        frame.submit();
        resources.endFrame();
    }

    EXPECT_TRUE (pass.hasGivenUp());
    EXPECT_TRUE (pass.getGiveUpReason().contains ("pipelines failed for testing")) << pass.getGiveUpReason();
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

/** DepthOfField through the full tier, without temporal anti-aliasing unless a test asks for it, in
    the depth of field scenes of SceneRendererBaselineScenes (depthOfFieldSize). The behaviors both
    tiers share are SceneRendererTierTests, these are the full tier's own, plus the raster tier's
    prepass and capability rules.
*/
class DepthOfFieldMetalTests : public SceneRendererMetalTests
{
protected:
    static constexpr int size = depthOfFieldSize;

    /** The full tier with depth of field on and @p taaSamples, see setUpDepthOfFieldScene(). */
    static CameraNode& setUpScene (Scene& scene, float focusDistance, int taaSamples = 1)
    {
        return setUpDepthOfFieldScene (scene, RenderSettings::TierPreference::full, focusDistance, taaSamples);
    }

    /** A black backdrop at 10, in focus with a focus of 10, and an emissive square at
        @p occluderDistance in front, 30 px tall, from x = 36 px to its right edge at x = 64 px. That
        edge is the border of the half resolution texels 31 and 32, so of tiles 1 and 2: the halo
        right of it lies in tiles holding no occluder texel, which only the dilation reaches.
    */
    static void buildTileEdgeScene (Scene& scene, float occluderDistance, float emission)
    {
        const auto unitsPerPixel = 2.0f * occluderDistance * std::tan (0.4f) / static_cast<float> (size);
        addEmitter (scene, { -50.0f, -50.0f, 100.0f, 100.0f }, 10.0f, 0.0f);
        addEmitter (scene, { -28.0f * unitsPerPixel, -15.0f * unitsPerPixel, 28.0f * unitsPerPixel, 30.0f * unitsPerPixel }, occluderDistance, emission);
    }

    /** How much further the light of a bokeh centered in the frame reaches towards the vertices of
        a 5-blade aperture than towards its edges: the luma-weighted mean radius of the 5 sectors of
        36 degrees around the vertex directions over that of the 5 around the edge directions, less 1.
        Sectors average over the dots of the sample pattern, so a round bokeh gives about 0.
    */
    static double vertexExcess (const Image& image)
    {
        std::array<double, 2> moment {}, weight {};

        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                const auto dx = x + 0.5 - size / 2;
                const auto dy = y + 0.5 - size / 2;
                const auto luma = meanLuminance (image, { x, y, 1, 1 });
                const auto sector = static_cast<int> (std::round (std::atan2 (dy, dx) / (MathConstants<double>::twoPi / 10.0)));
                const auto parity = static_cast<size_t> ((sector % 2 + 2) % 2);

                moment[parity] += luma * std::sqrt (dx * dx + dy * dy);
                weight[parity] += luma;
            }
        }

        return (moment[0] / jmax (weight[0], 1.0)) / jmax (moment[1] / jmax (weight[1], 1.0), 1.0) - 1.0;
    }
};

TEST_F (DepthOfFieldMetalTests, DepthOfFieldRecordsNothingWhenItCannotBlur)
{
    // Nothing is created, drawn or kept while it is off or the camera cannot blur
    Scene scene;
    auto& camera = setUpScene (scene, 0.2f);
    buildBarsScene (scene, 10.0f);

    const auto expectNoPass = [&] (SceneRenderer& renderer, const char* name)
    {
        ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << name << ": " << renderer.getLastError();
        EXPECT_EQ (renderer.getDepthOfField(), nullptr) << name;
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr) << name;
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::circleOfConfusion), nullptr) << name;
        EXPECT_TRUE (renderer.waitForGPU().wasOk()) << name;
    };

    {
        setDepthOfField (scene, false);
        SceneRenderer renderer;
        expectNoPass (renderer, "off");
        setDepthOfField (scene, true);
    }

    {
        camera.fStop = 0.0f;
        SceneRenderer renderer;
        expectNoPass (renderer, "fStop 0");
        camera.fStop = 1.4f;
    }

    {
        camera.projection = CameraNode::Projection::orthographic;
        SceneRenderer renderer;
        expectNoPass (renderer, "orthographic");
        camera.projection = CameraNode::Projection::perspective;
    }

    {
        setDepthOfField (scene, true, 0.0f);
        SceneRenderer renderer;
        expectNoPass (renderer, "no blur size");
        setDepthOfField (scene, true);
    }

    {
        // The default camera of a scene without one has no depth of field
        Scene withoutCamera;
        setFullTierTaa (withoutCamera, 1);
        setDepthOfField (withoutCamera, true);
        buildBarsScene (withoutCamera, 10.0f);

        SceneRenderer renderer;
        ASSERT_NE (renderer.render (device, withoutCamera, size, size), nullptr) << renderer.getLastError();
        EXPECT_EQ (renderer.getDepthOfField(), nullptr);
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    {
        // The raster tier, where a camera that cannot blur takes neither the HDR route nor the prepass
        auto settings = scene.getRenderSettings();
        settings.tier = RenderSettings::TierPreference::raster;
        scene.setRenderSettings (settings);
        camera.fStop = 0.0f;

        SceneRenderer renderer;
        expectNoPass (renderer, "raster");
        EXPECT_TRUE (renderer.getCapabilities()->isEnabled (RenderCapabilities::Feature::depthOfField, scene.getRenderSettings()));
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr);
        EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)->getFormat(), GpuTextureFormat::rgba16float);

        camera.fStop = 1.4f;
        settings.tier = RenderSettings::TierPreference::full;
        scene.setRenderSettings (settings);
    }

    // Once blurring, a frame that cannot blur any more keeps nothing but the pipelines
    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << renderer.getLastError();
    ASSERT_NE (renderer.getDepthOfField(), nullptr);
    EXPECT_GT (renderer.getDepthOfField()->getNumCachedTargets(), 0u);
    EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::circleOfConfusion), nullptr);

    camera.fStop = 0.0f;
    ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getDepthOfField()->getNumCachedTargets(), 0u);
    EXPECT_EQ (renderer.getDepthOfField()->getOutput(), nullptr);
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (DepthOfFieldMetalTests, DepthOfFieldWithInfiniteFarPlaneStaysFinite)
{
    Scene scene;
    auto& camera = setUpScene (scene, 1.0f);
    camera.zFar = 0.0f;
    buildBarsScene (scene, 1000.0f);

    SceneRenderer renderer;
    const auto image = renderToImage (renderer, scene, size);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_GT (meanLuminance (image, { 0, 0, size, size }), 10.0);

    const auto blurred = renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField);
    const auto coc = renderer.getPassOutput (SceneRenderer::PassOutput::circleOfConfusion);
    ASSERT_NE (blurred, nullptr);
    ASSERT_NE (coc, nullptr);
    EXPECT_EQ (blurred->getFormat(), GpuTextureFormat::rgba16float);
    EXPECT_EQ (coc->getFormat(), GpuTextureFormat::r16float);
    EXPECT_EQ (coc->getWidth(), size / 2);

    // Each check compiles a shader at runtime, about 0.2 s: the result alone stays well within the
    // budget, and DepthOfFieldTests.ShaderLinearizationGivesTheInverseViewDistance pins the finite CoC at depth 1
    EXPECT_TRUE (isFiniteTexture (blurred));
}
#endif

TEST_F (DepthOfFieldMetalTests, PolygonBokehDiffersFromRound)
{
    // A small bright square far out of focus: the blur takes the shape of the aperture, a disk or a
    // pentagon. The pentagon reaches further towards its vertices than towards its edges, so on a
    // circle between its apothem and its corners, every other one of 10 directions 36 degrees apart
    // is darker. One renderer changes the blades between its frames
    Scene scene;
    auto& camera = setUpScene (scene, 0.1f);
    camera.fStop = 0.7f;
    setDepthOfField (scene, true, 64.0f);
    addEmitter (scene, { -0.2f, -0.2f, 0.4f, 0.4f }, 10.0f, 200.0f);

    // A black backdrop covers the frame, so the blur keeps its focus on the aperture shape
    addEmitter (scene, { -50.0f, -50.0f, 100.0f, 100.0f }, 50.0f, 0.0f);
    ASSERT_GT (circleOfConfusion (scene, 10.0f), 64.0f);

    SceneRenderer renderer;
    const auto alternation = [&] (int blades, double* ringLuma = nullptr)
    {
        camera.apertureBlades = blades;
        const auto image = renderToImage (renderer, scene, size);
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();

        // The bokeh is 64 px wide: its edges lie 26 px (apothem) to 32 px (vertices) from the center, plus 3 px of the square
        std::array<double, 2> sums {};
        for (int i = 0; i < 10; ++i)
        {
            const auto angle = MathConstants<double>::twoPi * i / 10.0;
            const auto x = static_cast<int> (std::round (64.0 + 31.0 * std::cos (angle)));
            const auto y = static_cast<int> (std::round (64.0 + 31.0 * std::sin (angle)));
            sums[static_cast<size_t> (i % 2)] += meanLuminance (image, { x - 1, y - 1, 3, 3 });
        }

        if (ringLuma != nullptr)
            *ringLuma = (sums[0] + sums[1]) / 10.0;

        return std::abs (sums[0] - sums[1]) / jmax (sums[0], sums[1], 1.0);
    };

    // Any count below 3 is round, -1 included, from the first frame on
    double negativeRing = 0.0;
    const auto negative = alternation (-1, &negativeRing);
    const auto round = alternation (0);
    const auto pentagon = alternation (5);

    RecordProperty ("round", String (round, 3).toStdString());
    RecordProperty ("pentagon", String (pentagon, 3).toStdString());

    EXPECT_LT (round, 0.1);
    EXPECT_GT (pentagon, 0.1);
    EXPECT_LT (negative, 0.1);
    EXPECT_GT (negativeRing, 50.0);
}

TEST_F (DepthOfFieldMetalTests, PolygonBokehHoldsBelowTheGatherRadius)
{
    // The square's CoC is 0.66 of its backdrop's, which sets the gather radius around it: the
    // gather must still take the pentagon of the square's own CoC, not the polygon of the gather
    // radius cut by a disk of the square's CoC, which keeps the samples near the edges of the third
    // ring and drops those near its vertices
    Scene scene;
    auto& camera = setUpScene (scene, 0.1f);
    camera.fStop = 0.7f;
    setDepthOfField (scene, true, 64.0f);

    constexpr float distance = 0.197f;
    const auto unitsPerPixel = 2.0f * distance * std::tan (0.4f) / static_cast<float> (size);
    addEmitter (scene, { -3.0f * unitsPerPixel, -3.0f * unitsPerPixel, 6.0f * unitsPerPixel, 6.0f * unitsPerPixel }, distance, 200.0f);
    addEmitter (scene, { -50.0f, -50.0f, 100.0f, 100.0f }, 50.0f, 0.0f);

    const auto ratio = circleOfConfusion (scene, distance) / jmin (64.0f, circleOfConfusion (scene, 50.0f));
    ASSERT_GT (ratio, 0.62f);
    ASSERT_LT (ratio, 0.70f);

    SceneRenderer renderer;
    camera.apertureBlades = 5;
    const auto pentagon = renderToImage (renderer, scene, size);
    ASSERT_TRUE (pentagon.isValid()) << renderer.getLastError();

    camera.apertureBlades = 0;
    const auto round = renderToImage (renderer, scene, size);
    ASSERT_TRUE (round.isValid()) << renderer.getLastError();

    const auto pentagonExcess = vertexExcess (pentagon);
    const auto roundExcess = vertexExcess (round);
    RecordProperty ("pentagonExcess", String (pentagonExcess, 3).toStdString());
    RecordProperty ("roundExcess", String (roundExcess, 3).toStdString());

    // Measured 0.095 for the pentagon, 0.052 with the polygon cut by a disk, and 0.002 for the round bokeh
    EXPECT_GT (pentagonExcess, 0.075);
    EXPECT_LT (std::abs (roundExcess), 0.03);
}

TEST_F (DepthOfFieldMetalTests, GivingUpDepthOfFieldRendersUnblurred)
{
    Scene scene;
    setUpScene (scene, 0.2f);
    buildBarsScene (scene, 10.0f);

    SceneRenderer renderer;
    const auto render = [&] (bool blurs, bool givenUp, const char* name)
    {
        ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << name << ": " << renderer.getLastError();
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField) != nullptr, blurs) << name;
        ASSERT_NE (renderer.getDepthOfField(), nullptr) << name;
        EXPECT_EQ (renderer.getDepthOfField()->hasGivenUp(), givenUp) << name;

        // A frame without the blur keeps none of its targets, after a frame that blurred too
        if (! blurs)
            EXPECT_EQ (renderer.getDepthOfField()->getNumCachedTargets(), 0u) << name;
    };

    // Only failures in a row give up: a frame that blurs in between starts the count over
    renderer.failDepthOfFieldForTesting (1);
    render (false, false, "first failure");
    render (true, false, "recovered");
    EXPECT_GT (renderer.getDepthOfField()->getNumCachedTargets(), 0u);

    renderer.failDepthOfFieldForTesting (1);
    render (false, false, "failure after a blur");
    render (true, false, "recovered again");

    renderer.failDepthOfFieldForTesting (2);
    render (false, false, "first of two failures");
    render (false, true, "second of two failures");
    EXPECT_FALSE (renderer.getDepthOfField()->getGiveUpReason().isEmpty());

    // Given up, it no longer tries, and the frames are the ones without depth of field
    const auto image = renderToImage (renderer, scene, size);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr);

    setDepthOfField (scene, false);
    const auto without = renderToImage (renderer, scene, size);
    EXPECT_EQ (countDifferences (image, without), 0);
}

TEST_F (DepthOfFieldMetalTests, MissingInputIsASkipNotAFailure)
{
    // A frame without the depth (the raster prepass that could not be drawn) skips the blur, and
    // never counts towards giving up, so the blur comes back with the depth
    DepthOfField depthOfField (device);
    FrameResources resources (device);
    ViewData view;
    CameraNode camera;
    RenderSettings settings;
    settings.depthOfField = true;

    for (int i = 0; i < 3; ++i)
    {
        resources.beginFrame (64, 64);
        auto frame = GpuFrame::begin (device);
        EXPECT_EQ (depthOfField.render (frame, view, settings, resources, { nullptr, false, nullptr, &camera }), nullptr);
        frame.submit();
        resources.endFrame();
    }

    EXPECT_FALSE (depthOfField.hasGivenUp());
    EXPECT_EQ (depthOfField.getNumCachedTargets(), 0u);
}

TEST_F (DepthOfFieldMetalTests, RasterDepthOfFieldDrawsThePrepassWithoutAmbientOcclusion)
{
    checkRasterDepthOfFieldDrawsThePrepassWithoutAmbientOcclusion();
}

TEST_F (DepthOfFieldMetalTests, RasterDepthOfFieldUnavailableRecordsNothing)
{
    checkRasterDepthOfFieldUnavailableRecordsNothing();
}

TEST_F (DepthOfFieldMetalTests, RasterDepthOfFieldSkipsFramesWithoutThePrepassDepth)
{
    checkRasterDepthOfFieldSkipsFramesWithoutThePrepassDepth();
}

TEST_F (DepthOfFieldMetalTests, RasterDepthOfFieldThatGaveUpStopsItsRoute)
{
    checkRasterDepthOfFieldThatGaveUpStopsItsRoute();
}

TEST_F (DepthOfFieldMetalTests, NearFieldReachesAcrossTileEdges)
{
    // The occluder ends on a tile border, so its halo on the right lies in tiles without it: only
    // the dilation of the tiles gathers it there
    Scene scene;
    setUpScene (scene, 10.0f);
    buildTileEdgeScene (scene, 0.2f, 8.0f);
    ASSERT_LT (circleOfConfusion (scene, 0.2f), -14.0f);

    SceneRenderer renderer;
    const auto image = renderToImage (renderer, scene, size);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();

    setDepthOfField (scene, false);
    const auto without = renderToImage (renderer, scene, size);
    ASSERT_TRUE (without.isValid()) << renderer.getLastError();

    const Rectangle<int> halo (66, 52, 4, 24);
    const auto lit = meanLuminance (image, halo);
    const auto dark = meanLuminance (without, halo);
    RecordProperty ("halo", String (lit, 2).toStdString());
    RecordProperty ("haloWithout", String (dark, 2).toStdString());

    // Measured 222 against 0: without the dilation the tiles right of the edge hold no near CoC
    EXPECT_GT (lit, dark + 20.0);
}

TEST_F (DepthOfFieldMetalTests, NearFieldIsContinuousAcrossTheSilhouette)
{
    // Across the right edge of an out-of-focus occluder over a black backdrop, the radiance falls
    // from the occluder's to 0 over the blur, without a step at the silhouette
    Scene scene;
    setUpScene (scene, 10.0f);
    buildTileEdgeScene (scene, 0.2f, 1.0f);

    SceneRenderer renderer;
    const auto image = renderToImage (renderer, scene, size);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();

    std::vector<double> radiance;
    for (int x = 50; x < 78; ++x)
        radiance.push_back (reinhardRadiance (image.getPixelColor (x, size / 2).getGreen()));

    const auto contrast = *std::max_element (radiance.begin(), radiance.end()) - *std::min_element (radiance.begin(), radiance.end());
    ASSERT_GT (contrast, 0.3);

    double largestStep = 0.0;
    for (size_t i = 1; i < radiance.size(); ++i)
        largestStep = jmax (largestStep, std::abs (radiance[i] - radiance[i - 1]));

    RecordProperty ("largestStep", String (largestStep / contrast, 3).toStdString());

    // A disk of 7.5 px spreads the contrast over 15 px: about 0.07 a pixel, and measured 0.14 at
    // the steepest. A step at the silhouette measured 0.42
    EXPECT_LT (largestStep / contrast, 0.25);
}

TEST_F (DepthOfFieldMetalTests, FaintCoverageKeepsItsLightOverTheBackground)
{
    // A bright square of very low coverage (a blended material of alpha 0.004, as a light much
    // smaller than a pixel resolves after antialiasing) far out of focus: spread by the blur, its
    // coverage falls below 1e-4 in every pixel while its premultiplied light stays visible. Over the
    // black background color it keeps that light, as over a visible black environment
    Scene scene;
    auto& camera = setUpScene (scene, 0.1f);
    camera.fStop = 0.7f;
    setDepthOfField (scene, true, 64.0f);
    addEmitter (scene, { -0.2f, -0.2f, 0.4f, 0.4f }, 10.0f, 60000.0f, 0.004f);

    SceneRenderer renderer;
    const auto overBackground = renderToImage (renderer, scene, size);
    ASSERT_TRUE (overBackground.isValid()) << renderer.getLastError();

    scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float>(); }, 32, 3));
    scene.setEnvironmentVisible (true);
    const auto overEnvironment = renderToImage (renderer, scene, size);
    ASSERT_TRUE (overEnvironment.isValid()) << renderer.getLastError();

    const auto background = meanLuminance (overBackground, { 0, 0, size, size });
    const auto environment = meanLuminance (overEnvironment, { 0, 0, size, size });
    RecordProperty ("overBackground", String (background, 2).toStdString());
    RecordProperty ("overEnvironment", String (environment, 2).toStdString());
    // Measured 56.23 against 56.24; tone mapping the un-premultiplied color times the coverage gave 0
    ASSERT_GT (environment, 10.0);
    EXPECT_NEAR (background, environment, 0.05 * environment);
}

TEST_F (DepthOfFieldMetalTests, RadianceCompositeFailureFallsBackToTheDisplayBlend)
{
    // A blurred frame whose radiance composite cannot compile still tone maps, with the display
    // space blend of the frames without depth of field, instead of failing the frame
    FrameResources resources (device);
    ViewData view;
    ToneMapping toneMapping;
    toneMapping.failRadianceCompositeForTesting();

    for (int i = 0; i < 2; ++i)
    {
        resources.beginFrame (8, 8);
        const auto source = resources.acquire (GpuTextureDesc (8, 8, GpuTextureFormat::rgba16float, true));
        const auto destination = resources.acquire (GpuTextureDesc (8, 8, GpuTextureFormat::rgba8unorm, true));
        const auto blurAmount = resources.acquire (GpuTextureDesc (8, 8, GpuTextureFormat::r8unorm, true));
        ASSERT_NE (source, nullptr);
        ASSERT_NE (destination, nullptr);
        ASSERT_NE (blurAmount, nullptr);

        auto frame = GpuFrame::begin (device);
        EXPECT_TRUE (toneMapping.render (device, frame, view, resources, source, false, nullptr, destination, Colors::black, blurAmount)) << i;
        EXPECT_TRUE (frame.waitForGPU().wasOk()) << i;
        resources.endFrame();
    }
}

TEST_F (DepthOfFieldMetalTests, DepthOfFieldChangesKeepTheAccumulationConverged)
{
    // The blur runs after the accumulation: turning it on and moving the focus change the image
    // but neither restart nor discard the converged history
    Scene scene;
    auto& camera = setUpScene (scene, 10.0f, 8);
    setDepthOfField (scene, false);
    buildBarsScene (scene, 10.0f);

    SceneRenderer renderer;
    const auto converged = renderConverged (renderer, scene, 64);
    ASSERT_TRUE (converged.isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    setDepthOfField (scene, true);
    camera.focusDistance = 0.2f;
    const auto blurred = renderToImage (renderer, scene, 64);
    EXPECT_TRUE (renderer.isConverged());
    EXPECT_GT (countDifferences (blurred, converged), 0);

    camera.fStop = 4.0f;
    camera.apertureBlades = 6;
    const auto stoppedDown = renderToImage (renderer, scene, 64);
    EXPECT_TRUE (renderer.isConverged());
    EXPECT_GT (countDifferences (stoppedDown, blurred), 0);
}

TEST_F (DepthOfFieldMetalTests, DepthOfFieldRunsBeforeBloom)
{
    // A square bright enough to glow when sharp spreads below the bloom threshold once blurred:
    // with the blur before bloom, no glow reaches past the bokeh
    Scene scene;
    auto& camera = setUpScene (scene, 0.1f);
    camera.fStop = 0.7f;
    setDepthOfField (scene, true, 64.0f);
    addEmitter (scene, { -0.2f, -0.2f, 0.4f, 0.4f }, 10.0f, 30.0f);
    addEmitter (scene, { -50.0f, -50.0f, 100.0f, 100.0f }, 50.0f, 0.0f);

    const auto glowRing = [] (const Image& image)
    {
        return meanLuminance (image, { 4, 4, 120, 6 }) + meanLuminance (image, { 4, 118, 120, 6 });
    };

    SceneRenderer renderer;
    const auto withoutBloom = renderToImage (renderer, scene, size);
    ASSERT_TRUE (withoutBloom.isValid()) << renderer.getLastError();

    auto settings = scene.getRenderSettings();
    settings.bloom = true;
    settings.bloomIntensity = 1.0f;
    scene.setRenderSettings (settings);
    const auto withBloom = renderToImage (renderer, scene, size);
    ASSERT_TRUE (withBloom.isValid()) << renderer.getLastError();

    // Sharp, the square glows over the whole frame
    setDepthOfField (scene, false, 64.0f);
    const auto sharpBloom = renderToImage (renderer, scene, size);
    ASSERT_TRUE (sharpBloom.isValid()) << renderer.getLastError();

    ASSERT_GT (glowRing (sharpBloom), glowRing (withoutBloom) + 10.0);
    EXPECT_LT (glowRing (withBloom), glowRing (withoutBloom) + 2.0);
}

//==============================================================================
/** MotionBlur through the full tier, without temporal anti-aliasing unless a test asks for it,
    in the scenes of buildMovingBarScene(): the camera at the origin looking down -Z, a bar
    motionBarDistance away moved between two frames. The raster tier's fallback is
    SceneRendererTierTests.MotionBlurOnRasterRendersTheUnblurredFallback.
*/
class MotionBlurMetalTests : public SceneRendererMetalTests
{
protected:
    static constexpr int size = motionBlurSize;

    /** The full tier accumulating @p taaSamples, with motion blur on, the shutter open @p shutter of the frame. */
    static void setUpMotionBlur (Scene& scene, int taaSamples = 1, float shutter = 1.0f)
    {
        setFullTierTaa (scene, taaSamples);
        setMotionBlur (scene, true, shutter);
    }

    /** The column analogue of edgeWidth(): the pixels between 10% and 90% of the luminance range
        along the column segment [yFrom, yTo).
    */
    static int verticalEdgeWidth (const Image& image, int column, int yFrom, int yTo)
    {
        std::vector<double> luma;
        for (int y = yFrom; y < yTo; ++y)
            luma.push_back (meanLuminance (image, { column, y, 1, 1 }));

        const auto [low, high] = std::minmax_element (luma.begin(), luma.end());
        const auto range = *high - *low;

        return static_cast<int> (std::count_if (luma.begin(), luma.end(), [&] (double l)
        {
            return l > *low + 0.1 * range && l < *low + 0.9 * range;
        }));
    }

    /** The horizontal pixel span [left, right) of the bar of buildMovingBarScene() at @p x units. */
    static Range<float> barSpan (const Scene& scene, float x)
    {
        const auto pixelsPerUnit = motionPixelsPerUnit (scene);
        return { size * 0.5f + (x - 0.1f) * pixelsPerUnit, size * 0.5f + (x + 0.1f) * pixelsPerUnit };
    }

    /** Renders the bar at @p from then at @p to with @p renderer, and returns the second frame. */
    Image renderMove (SceneRenderer& renderer, Scene& scene, EntityNode& bar, float from, float to)
    {
        moveBar (bar, from);
        if (! renderToImage (renderer, scene, size).isValid())
            return {};

        moveBar (bar, to);
        return renderToImage (renderer, scene, size);
    }

    /** The summed luma of the smear outside a bar spanning @p span: the 8 px wide strips left and
        right of it, over the rows the bar covers.
    */
    static double smearLuminance (const Image& image, Range<float> span)
    {
        const auto left = static_cast<int> (std::floor (span.getStart())) - 8;
        const auto right = static_cast<int> (std::ceil (span.getEnd()));
        return (meanLuminance (image, { left, 30, 8, 68 }) + meanLuminance (image, { right, 30, 8, 68 })) * 8.0 * 68.0;
    }
};

TEST_F (MotionBlurMetalTests, MotionBlurSmearsAlongTheMotionOnly)
{
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    setUpMotionBlur (scene);

    // 0.1 units at the bar's distance move it 7.6 px: with the shutter open all frame, the smear
    // spans that much, centered on where the bar is now, so its vertical edges ramp and reach about
    // 3.8 px out of it, and its horizontal ones stay steps
    const auto motion = 0.1f * motionPixelsPerUnit (scene);
    ASSERT_GT (motion, 6.0f);
    ASSERT_LT (motion, 2.0f * MotionBlur::tileSize);

    SceneRenderer renderer;
    const auto blurred = renderMove (renderer, scene, *bar, 0.0f, 0.1f);
    ASSERT_TRUE (blurred.isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

    // The bar spans x 64.0 to 79.1 and y 26.2 to 101.8: the ramps of both vertical edges along the
    // middle row, the plan's measure, the smear past each of them, and the top edge
    const auto rampWidth = edgeWidth (blurred, size / 2, 50, 94);
    const auto leftReach = meanLuminance (blurred, { 61, 40, 2, 48 });
    const auto rightReach = meanLuminance (blurred, { 80, 40, 2, 48 });
    const auto topWidth = verticalEdgeWidth (blurred, 72, 10, 50);
    RecordProperty ("rampWidth", rampWidth);
    RecordProperty ("leftReach", String (leftReach, 1).toStdString());
    RecordProperty ("rightReach", String (rightReach, 1).toStdString());
    RecordProperty ("topEdgeWidth", topWidth);

    EXPECT_GE (rampWidth, 4);
    EXPECT_GT (leftReach, 50.0);
    EXPECT_GT (rightReach, 50.0);
    EXPECT_LE (topWidth, 2);

    // A smear, not a dilation: the outermost lit pixels are partly covered, dimmer than the interior
    const auto interior = meanLuminance (blurred, { 72, 40, 1, 48 });
    EXPECT_LT (meanLuminance (blurred, { 61, 40, 1, 48 }), interior - 30.0);
    EXPECT_LT (meanLuminance (blurred, { 81, 40, 1, 48 }), interior - 30.0);

    // The scene color before the blur stays sharp: the smear runs after it
    EXPECT_LE (edgeWidth (readBack (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)), size / 2, 50, 94), 2);

    // Not the move alone: the same move without motion blur keeps steps
    setMotionBlur (scene, false);
    const auto sharp = renderMove (renderer, scene, *bar, 0.0f, 0.1f);
    ASSERT_TRUE (sharp.isValid()) << renderer.getLastError();
    EXPECT_LE (edgeWidth (sharp, size / 2, 50, 94), 2);
    EXPECT_LT (meanLuminance (sharp, { 61, 40, 2, 48 }), 1.0);
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);
}

TEST_F (MotionBlurMetalTests, FirstFrameIsNotBlurred)
{
    // A transform set before the first frame is no motion: the frame records no smear and equals
    // the frame without motion blur
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    setUpMotionBlur (scene);
    moveBar (*bar, 0.1f);

    SceneRenderer renderer;
    const auto first = renderToImage (renderer, scene, size);
    ASSERT_TRUE (first.isValid()) << renderer.getLastError();
    EXPECT_EQ (renderer.getMotionBlur(), nullptr);
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

    setMotionBlur (scene, false);
    const auto off = renderToImage (renderer, scene, size);
    ASSERT_TRUE (off.isValid()) << renderer.getLastError();
    EXPECT_LE (maxChannelDifference (first, off), 1);
}

TEST_F (MotionBlurMetalTests, TeleportIsClampedToMaximumRadius)
{
    // A jump of a whole unit, 76 px, far more than a smear spans: nothing lights up farther than
    // tileSize from where the bar lands, so no pixel is reached farther than 2 * tileSize, the
    // longest smear. A gather blur can only smear what is on screen, so the bar lands in view
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    setUpMotionBlur (scene);
    ASSERT_GT (motionPixelsPerUnit (scene), 4.0f * MotionBlur::tileSize);

    SceneRenderer renderer;
    const auto blurred = renderMove (renderer, scene, *bar, -0.6f, 0.4f);
    ASSERT_TRUE (blurred.isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

    const auto span = barSpan (scene, 0.4f);
    const auto reach = static_cast<float> (MotionBlur::tileSize) + 2.0f;

    int litBeyondReach = 0;
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            const auto center = static_cast<float> (x) + 0.5f;
            const auto distance = jmax (span.getStart() - center, center - span.getEnd(), 0.0f);
            if ((distance > reach || y < 24 || y > 104) && meanLuminance (blurred, { x, y, 1, 1 }) > 2.0)
                ++litBeyondReach;
        }
    }

    // Within its reach the smear shows, on both sides of the landing place
    const auto leftSmear = meanLuminance (blurred, { static_cast<int> (span.getStart()) - 12, 40, 8, 48 });
    const auto rightSmear = meanLuminance (blurred, { static_cast<int> (span.getEnd()) + 5, 40, 8, 48 });
    RecordProperty ("litBeyondReach", litBeyondReach);
    RecordProperty ("leftSmear", String (leftSmear, 2).toStdString());
    RecordProperty ("rightSmear", String (rightSmear, 2).toStdString());

    EXPECT_EQ (litBeyondReach, 0);
    EXPECT_GT (leftSmear, 10.0);
    EXPECT_GT (rightSmear, 10.0);

#if YUP_ENABLE_SHADER_TRANSPILER
    EXPECT_TRUE (isFiniteTexture (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur)));
#endif
}

TEST_F (MotionBlurMetalTests, CameraMotionBlursStaticObjects)
{
    {
        // Moving the camera 0.1 to the right moves the still bar 7.6 px to the left on screen
        Scene scene;
        buildMovingBarScene (scene);
        setUpMotionBlur (scene);
        auto* camera = scene.getRoot()->findChild ("camera");
        ASSERT_NE (camera, nullptr);

        SceneRenderer renderer;
        ASSERT_TRUE (renderToImage (renderer, scene, size).isValid()) << renderer.getLastError();
        camera->setPosition ({ 0.1f, 0.0f, 0.0f });
        const auto blurred = renderToImage (renderer, scene, size);
        ASSERT_TRUE (blurred.isValid()) << renderer.getLastError();
        ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

        // The bar spans x 48.9 to 64.0
        EXPECT_GE (edgeWidth (blurred, size / 2, 36, 78), 4);
        EXPECT_GT (meanLuminance (blurred, { 46, 40, 2, 48 }), 50.0);
        EXPECT_GT (meanLuminance (blurred, { 65, 40, 2, 48 }), 50.0);
        EXPECT_LE (verticalEdgeWidth (blurred, 56, 10, 50), 2);
    }

    // Turning the camera moves the environment, whose motion only GBufferBackground writes: its
    // sharp edge, a white half to the right of the view axis, smears as the geometry does
    Scene scene;
    setUpMotionBlur (scene);
    scene.setBackgroundColor (Colors::black);
    scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>& direction)
    {
        return direction.getX() > 0.0f ? Vector3<float> (1.0f, 1.0f, 1.0f) : Vector3<float>();
    }, 512, 1));
    scene.setEnvironmentVisible (true);
    addCamera (scene, {}, { 0.0f, 0.0f, -1.0f });
    auto* camera = scene.getRoot()->findChild ("camera");
    ASSERT_NE (camera, nullptr);

    const auto turn = [&] (SceneRenderer& renderer)
    {
        aimCamera (*camera, {}, { 0.0f, 0.0f, -1.0f });
        if (! renderToImage (renderer, scene, size).isValid())
            return Image();

        aimCamera (*camera, {}, { std::sin (0.08f), 0.0f, -std::cos (0.08f) });
        return renderToImage (renderer, scene, size);
    };

    SceneRenderer renderer;
    const auto blurred = turn (renderer);
    ASSERT_TRUE (blurred.isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

    setMotionBlur (scene, false);
    const auto sharp = turn (renderer);
    ASSERT_TRUE (sharp.isValid()) << renderer.getLastError();

    const auto blurredWidth = edgeWidth (blurred, size / 2, 32, 96);
    const auto sharpWidth = edgeWidth (sharp, size / 2, 32, 96);
    RecordProperty ("environmentEdgeWidth", blurredWidth);
    RecordProperty ("environmentSharpEdgeWidth", sharpWidth);
    EXPECT_GE (blurredWidth, sharpWidth + 3);
}

TEST_F (MotionBlurMetalTests, MotionBlurSmearsAfterAMoveUnderTemporalAntiAliasing)
{
    // With TAA, the frame after a move restarts the convergence and reprojects the history along
    // the motion, clipping it to the new frame: the TAA output shows the bar where it moved to,
    // and the motion blur smears that
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    setUpMotionBlur (scene, 8);

    SceneRenderer renderer;
    ASSERT_TRUE (renderConverged (renderer, scene, size).isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());
    EXPECT_EQ (renderer.getMotionBlur(), nullptr);

    moveBar (*bar, 0.1f);
    const auto blurred = renderToImage (renderer, scene, size);
    ASSERT_TRUE (blurred.isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);
    EXPECT_FALSE (renderer.isConverged());

    const auto resolved = readBack (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor));
    const auto resolvedWidth = edgeWidth (resolved, size / 2, 50, 94);
    const auto rampWidth = edgeWidth (blurred, size / 2, 50, 94);
    const auto leftReach = meanLuminance (blurred, { 61, 40, 2, 48 });
    RecordProperty ("resolvedRampWidth", resolvedWidth);
    RecordProperty ("resolvedLeftReach", String (meanLuminance (resolved, { 61, 40, 2, 48 }), 1).toStdString());
    RecordProperty ("rampWidth", rampWidth);
    RecordProperty ("leftReach", String (leftReach, 1).toStdString());

    // TAA shows the bar where it moved to, sharp and without a trail where it was: the smear is the motion blur's
    EXPECT_LE (resolvedWidth, 2);
    EXPECT_LT (meanLuminance (resolved, { 61, 40, 2, 48 }), 1.0);
    EXPECT_GE (rampWidth, resolvedWidth + 2);
    EXPECT_GT (leftReach, 50.0);
}

TEST_F (MotionBlurMetalTests, MotionBlurOnAStillFrameRecordsNoPass)
{
    // With motion blur on, still frames create, draw and keep nothing: the frames converging, the
    // converged ones, and those after a move
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    setUpMotionBlur (scene, 4);

    SceneRenderer renderer;
    for (int frame = 0; frame < 8; ++frame)
    {
        ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << renderer.getLastError();
        EXPECT_EQ (renderer.getMotionBlur(), nullptr) << frame;
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr) << frame;

        if (renderer.isConverged())
            break;
    }

    ASSERT_TRUE (renderer.isConverged());
    ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getMotionBlur(), nullptr);

    // Not vacuous: a move smears and keeps its targets
    moveBar (*bar, 0.1f);
    ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << renderer.getLastError();
    ASSERT_NE (renderer.getMotionBlur(), nullptr);
    EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);
    EXPECT_GT (renderer.getMotionBlur()->getNumCachedTargets(), 0u);

    // The still frames after it release them
    for (int frame = 0; frame < 6; ++frame)
    {
        ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << renderer.getLastError();
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr) << frame;
        EXPECT_EQ (renderer.getMotionBlur()->getOutput(), nullptr) << frame;
        EXPECT_EQ (renderer.getMotionBlur()->getNumCachedTargets(), 0u) << frame;
    }

    EXPECT_TRUE (renderer.isConverged());
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_F (MotionBlurMetalTests, MotionBlurOffOrAClosedShutterRecordsNothingWhileMoving)
{
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    setUpMotionBlur (scene);

    for (const auto [motionBlur, shutter] : { std::pair { false, 1.0f }, std::pair { true, 0.0f }, std::pair { true, -1.0f } })
    {
        setMotionBlur (scene, motionBlur, shutter);

        SceneRenderer renderer;
        const auto image = renderMove (renderer, scene, *bar, 0.0f, 0.1f);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        EXPECT_EQ (renderer.getMotionBlur(), nullptr) << shutter;
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr) << shutter;
        EXPECT_LE (edgeWidth (image, size / 2, 50, 72), 2) << shutter;
    }
}

TEST_F (MotionBlurMetalTests, SmearedHighlightsKeepTheirBrightnessOverTheBackground)
{
    // A bright bar moving over the black background color, without an environment, against the
    // same move over a visible black environment: the smear spreads the same light over both, so
    // it is as bright, not capped by its partial coverage (preflight R22)
    Scene scene;
    auto* bar = buildMovingBarScene (scene, 20.0f);
    setUpMotionBlur (scene);

    SceneRenderer renderer;
    const auto overBackground = renderMove (renderer, scene, *bar, 0.0f, 0.2f);
    ASSERT_TRUE (overBackground.isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

    scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float>(); }, 32, 3));
    scene.setEnvironmentVisible (true);
    const auto overEnvironment = renderMove (renderer, scene, *bar, 0.0f, 0.2f);
    ASSERT_TRUE (overEnvironment.isValid()) << renderer.getLastError();

    // Not vacuous: the smear reaches out of the bar, which spans x 71.6 to 86.7 at 0.2
    ASSERT_GT (meanLuminance (overEnvironment, { 64, 40, 6, 48 }), 20.0);

    const auto background = smearLuminance (overBackground, barSpan (scene, 0.2f));
    const auto environment = smearLuminance (overEnvironment, barSpan (scene, 0.2f));
    RecordProperty ("overBackground", String (background, 1).toStdString());
    RecordProperty ("overEnvironment", String (environment, 1).toStdString());

    // Over black both tone map the same smeared radiance, so only the 8 bit rounding differs
    EXPECT_NEAR (background, environment, 0.02 * environment);
}

TEST_F (MotionBlurMetalTests, SmearedHighlightsKeepTheirBrightnessWhileTheCameraTurns)
{
    // Turning the camera moves the bar and the background alike, so every tile takes the uniform
    // path: its smear too keeps its brightness over the background color (preflight R22)
    Scene scene;
    buildMovingBarScene (scene, 20.0f);
    setUpMotionBlur (scene);
    auto* camera = scene.getRoot()->findChild ("camera");
    ASSERT_NE (camera, nullptr);

    const auto turn = [&] (SceneRenderer& renderer)
    {
        aimCamera (*camera, {}, { 0.0f, 0.0f, -1.0f });
        if (! renderToImage (renderer, scene, size).isValid())
            return Image();

        aimCamera (*camera, {}, { std::sin (0.08f), 0.0f, -std::cos (0.08f) });
        return renderToImage (renderer, scene, size);
    };

    SceneRenderer renderer;
    const auto overBackground = turn (renderer);
    ASSERT_TRUE (overBackground.isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

    scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float>(); }, 32, 3));
    scene.setEnvironmentVisible (true);
    const auto overEnvironment = turn (renderer);
    ASSERT_TRUE (overEnvironment.isValid()) << renderer.getLastError();

    // Turned 0.08 rad to the right, the bar spans x 44.2 to 59.5, and moved about 15 px
    const Range<float> span (44.2f, 59.5f);
    ASSERT_GT (meanLuminance (overEnvironment, { 38, 40, 5, 48 }), 20.0);

    const auto background = smearLuminance (overBackground, span);
    const auto environment = smearLuminance (overEnvironment, span);
    RecordProperty ("overBackground", String (background, 1).toStdString());
    RecordProperty ("overEnvironment", String (environment, 1).toStdString());
    EXPECT_NEAR (background, environment, 0.02 * environment);
}

TEST_F (MotionBlurMetalTests, StillPartialCoverageKeepsItsBlendWhileTheBarMoves)
{
    // Half transparent quads, standing for antialiased edges, keep the frame's blend with the
    // background color while the bar moves: one far from the bar, one in its tiles just above its
    // top edge, which the horizontal motion does not reach (preflight R21, R22). The bar moves
    // 7.6 px, and 1.5 px, a half motion of 0.76 px: slow enough that the tiles around the still
    // quad hold motions of almost the same length
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    addMotionQuad (scene, { -0.8f, -0.1f, 0.2f, 0.2f }, 4.0f, 0.5f);
    addMotionQuad (scene, { -0.1f, 0.56f, 0.3f, 0.2f }, 4.0f, 0.5f);

    // The quads span x 3.4 to 18.6, y 56.4 to 71.6, and x 56.4 to 79.1, y 7.2 to 22.4
    const Rectangle<int> quads[] = { { 4, 57, 14, 14 }, { 57, 8, 22, 14 } };

    for (const auto move : { 0.1f, 0.02f })
    {
        const auto name = String (move);
        setUpMotionBlur (scene);

        SceneRenderer renderer;
        const auto on = renderMove (renderer, scene, *bar, 0.0f, move);
        ASSERT_TRUE (on.isValid()) << renderer.getLastError();
        ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr) << name;

        setMotionBlur (scene, false);
        const auto off = renderMove (renderer, scene, *bar, 0.0f, move);
        ASSERT_TRUE (off.isValid()) << renderer.getLastError();

        for (const auto& quad : quads)
        {
            ASSERT_GT (meanLuminance (off, quad), 20.0) << name << " " << quad.toString();

            int differing = 0;
            int largest = 0;
            for (int y = quad.getY(); y < quad.getBottom(); ++y)
            {
                for (int x = quad.getX(); x < quad.getRight(); ++x)
                {
                    const auto a = on.getPixelColor (x, y);
                    const auto b = off.getPixelColor (x, y);
                    const auto difference = jmax (std::abs (a.getRed() - b.getRed()), std::abs (a.getGreen() - b.getGreen()), std::abs (a.getBlue() - b.getBlue()));
                    largest = jmax (largest, difference);
                    differing += difference > 2 ? 1 : 0;
                }
            }

            RecordProperty ((name + "_" + quad.toString().replace (" ", "_") + "_largestDifference").toStdString(), largest);
            EXPECT_EQ (differing, 0) << name << " " << quad.toString();
        }
    }
}

TEST_F (MotionBlurMetalTests, NoSmearWithoutAContinuingHistory)
{
    // A resize, a restart of the accumulation (a camera cut) and a frame the GPU failed break the
    // history the motion is measured against: the next frame does not smear, though the bar moved,
    // and the one after it does again (preflight R23)
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    setUpMotionBlur (scene);

    const auto render = [&] (SceneRenderer& renderer, float x, int width, bool smears, const char* name)
    {
        moveBar (*bar, x);
        ASSERT_NE (renderer.render (device, scene, width, size), nullptr) << name << ": " << renderer.getLastError();
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur) != nullptr, smears) << name;
    };

    {
        // The aspect ratio changes the projection, which alone would read as motion everywhere
        SceneRenderer renderer;
        render (renderer, 0.0f, size, false, "before the resize");
        render (renderer, 0.1f, size + 32, false, "resized");
        render (renderer, 0.2f, size + 32, true, "after the resize");
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    {
        SceneRenderer renderer;
        render (renderer, 0.0f, size, false, "before the restart");
        renderer.restartAccumulation();
        render (renderer, 0.1f, size, false, "restarted");
        render (renderer, 0.2f, size, true, "after the restart");
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    {
        SceneRenderer renderer;
        render (renderer, 0.0f, size, false, "before the failure");
        device->failNextFrameForTesting ("injected failure");
        render (renderer, 0.0f, size, false, "failing");

        // Frames complete in order, so once a later frame did the failed one has too
        {
            auto fence = GpuFrame::begin (device);
            EXPECT_TRUE (fence.waitForGPU().wasOk());
        }

        render (renderer, 0.1f, size, false, "after the failure");
        EXPECT_EQ (renderer.getLastGpuFailure(), "injected failure");
        render (renderer, 0.2f, size, true, "the next one");
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }
}

TEST_F (MotionBlurMetalTests, SmearedFrameIsNotFinal)
{
    // Without TAA, a smeared frame still reports that it is not converged, so a component repaints
    // once more and the still frame after it shows the bar sharp (preflight R23)
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    setUpMotionBlur (scene);

    SceneRenderer renderer;
    ASSERT_TRUE (renderMove (renderer, scene, *bar, 0.0f, 0.1f).isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);
    EXPECT_FALSE (renderer.isConverged());

    const auto still = renderToImage (renderer, scene, size);
    ASSERT_TRUE (still.isValid()) << renderer.getLastError();
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);
    EXPECT_TRUE (renderer.isConverged());
    EXPECT_LE (edgeWidth (still, size / 2, 50, 94), 2);
}

TEST_F (MotionBlurMetalTests, SmearBlendsItsCoverageOverAGrayBackground)
{
    // The smear carries coverage: outside the bar, a smeared pixel composites the gray background
    // color by the coverage it lacks. So against the same move over black it is brighter by less
    // than the gray: not by the whole gray (no coverage kept), nor by nothing (full coverage). The
    // bar is dim, so the sums stay within the 8 bit range (preflight R22)
    Scene scene;
    auto* bar = buildMovingBarScene (scene, 0.1f);
    setUpMotionBlur (scene);

    SceneRenderer renderer;
    const auto overBlack = renderMove (renderer, scene, *bar, 0.0f, 0.2f);
    ASSERT_TRUE (overBlack.isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

    constexpr int gray = 100;
    scene.setBackgroundColor (Color (static_cast<uint8> (gray), static_cast<uint8> (gray), static_cast<uint8> (gray)));
    const auto overGray = renderMove (renderer, scene, *bar, 0.0f, 0.2f);
    ASSERT_TRUE (overGray.isValid()) << renderer.getLastError();

    // The strips of 8 px on both sides of the bar, which spans x 71.6 to 86.7, where the smear
    // covers at least a fifth (the level over black above 40)
    int smeared = 0;
    int outside = 0;
    for (const auto x0 : { 63, 87 })
    {
        for (int y = 30; y < 98; ++y)
        {
            for (int x = x0; x < x0 + 8; ++x)
            {
                if (overBlack.getPixelColor (x, y).getGreen() <= 40)
                    continue;

                ++smeared;
                const auto added = static_cast<int> (overGray.getPixelColor (x, y).getGreen()) - static_cast<int> (overBlack.getPixelColor (x, y).getGreen());
                outside += added <= 2 || added >= gray - 5 ? 1 : 0;
            }
        }
    }

    RecordProperty ("smearedPixels", smeared);
    RecordProperty ("outsideTheBlend", outside);
    EXPECT_GT (smeared, 40);
    EXPECT_EQ (outside, 0);
}

TEST_F (MotionBlurMetalTests, DiagonalMotionSmearsAlongItsDirectionAcrossTiles)
{
    // A square moving 7.6 px right and 7.6 px up: its smear reaches past its top right and bottom
    // left corners, not past the other two. Its top right corner lands at (78, 50), next to the
    // corner of the 16 px tiles at (80, 48): the tile beyond it holds none of the square, so only
    // its diagonal neighbor carries the motion there
    Scene scene;
    scene.setBackgroundColor (Colors::black);
    scene.setAmbientColor (Colors::black);
    addCamera (scene, {}, { 0.0f, 0.0f, -1.0f });
    auto* square = addMotionQuad (scene, { -0.1f, -0.1f, 0.2f, 0.2f }, 20.0f);
    setUpMotionBlur (scene);

    const auto pixelsPerUnit = motionPixelsPerUnit (scene);
    const auto x = (70.5f - 64.0f) / pixelsPerUnit;
    const auto y = (64.0f - 57.5f) / pixelsPerUnit;

    const auto render = [&] (SceneRenderer& renderer)
    {
        square->setPosition ({ x - 0.1f, y - 0.1f, -motionBarDistance });
        if (! renderToImage (renderer, scene, size).isValid())
            return Image();

        square->setPosition ({ x, y, -motionBarDistance });
        return renderToImage (renderer, scene, size);
    };

    SceneRenderer renderer;
    const auto blurred = render (renderer);
    ASSERT_TRUE (blurred.isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

    setMotionBlur (scene, false);
    const auto sharp = render (renderer);
    ASSERT_TRUE (sharp.isValid()) << renderer.getLastError();

    const auto luma = [] (const Image& image, int px, int py) { return meanLuminance (image, { px, py, 1, 1 }); };
    RecordProperty ("topRight", String (luma (blurred, 80, 47), 1).toStdString());
    RecordProperty ("bottomLeft", String (luma (blurred, 61, 67), 1).toStdString());
    RecordProperty ("topLeft", String (luma (blurred, 61, 47), 1).toStdString());
    RecordProperty ("bottomRight", String (luma (blurred, 80, 67), 1).toStdString());

    EXPECT_GT (luma (blurred, 80, 47), 10.0);
    EXPECT_GT (luma (blurred, 61, 67), 10.0);
    EXPECT_LT (luma (blurred, 61, 47), 2.0);
    EXPECT_LT (luma (blurred, 80, 67), 2.0);
    EXPECT_LT (luma (sharp, 80, 47), 2.0);
    EXPECT_LT (luma (sharp, 61, 67), 2.0);
}

TEST_F (MotionBlurMetalTests, MotionBlurRunsAfterTheDepthOfField)
{
    // Both blur the bar, the depth of field first: the smear carries the share of coverage it
    // spread, so the blurred light stays as bright over the background color as over a visible
    // black environment, and the result is finite. A still square far from the bar, blurred by the
    // depth of field only, lies in tiles where nothing moves, which keep the share as it is
    Scene scene;
    auto* bar = buildMovingBarScene (scene, 20.0f);
    setUpMotionBlur (scene);
    addMotionQuad (scene, { -0.9f, -0.1f, 0.2f, 0.2f }, 20.0f);
    setDepthOfField (scene, true);
    auto& camera = *scene.getActiveCamera();
    camera.fStop = 1.4f;
    camera.focusDistance = 0.5f;
    ASSERT_GT (camera.getCircleOfConfusion (motionBarDistance, static_cast<float> (size)), 4.0f);

    SceneRenderer renderer;
    const auto overBackground = renderMove (renderer, scene, *bar, 0.0f, 0.2f);
    ASSERT_TRUE (overBackground.isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr);
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

#if YUP_ENABLE_SHADER_TRANSPILER
    EXPECT_TRUE (isFiniteTexture (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur)));
#endif

    scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float>(); }, 32, 3));
    scene.setEnvironmentVisible (true);
    const auto overEnvironment = renderMove (renderer, scene, *bar, 0.0f, 0.2f);
    ASSERT_TRUE (overEnvironment.isValid()) << renderer.getLastError();

    const auto background = meanLuminance (overBackground, { 0, 0, size, size }) * size * size;
    const auto environment = meanLuminance (overEnvironment, { 0, 0, size, size }) * size * size;
    RecordProperty ("overBackground", String (background, 1).toStdString());
    RecordProperty ("overEnvironment", String (environment, 1).toStdString());
    EXPECT_NEAR (background, environment, 0.02 * environment);
}

TEST_F (MotionBlurMetalTests, GivingUpMotionBlurRendersUnsmeared)
{
    // The bar moves on every frame, so each one tries
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    setUpMotionBlur (scene);

    SceneRenderer renderer;
    ASSERT_TRUE (renderToImage (renderer, scene, size).isValid()) << renderer.getLastError();

    auto x = 0.0f;
    const auto render = [&] (bool smears, bool givenUp, const char* name)
    {
        x = x > 0.0f ? 0.0f : 0.1f;
        moveBar (*bar, x);
        ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << name << ": " << renderer.getLastError();
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur) != nullptr, smears) << name;
        ASSERT_NE (renderer.getMotionBlur(), nullptr) << name;
        EXPECT_EQ (renderer.getMotionBlur()->hasGivenUp(), givenUp) << name;

        // A frame without the smear keeps none of its targets, after a frame that smeared too
        if (! smears)
            EXPECT_EQ (renderer.getMotionBlur()->getNumCachedTargets(), 0u) << name;
    };

    // Only failures in a row give up: a frame that smears in between starts the count over
    renderer.failMotionBlurForTesting (1);
    render (false, false, "first failure");
    render (true, false, "recovered");
    EXPECT_GT (renderer.getMotionBlur()->getNumCachedTargets(), 0u);

    renderer.failMotionBlurForTesting (1);
    render (false, false, "failure after a smear");
    render (true, false, "recovered again");

    renderer.failMotionBlurForTesting (2);
    render (false, false, "first of two failures");
    render (false, true, "second of two failures");
    EXPECT_FALSE (renderer.getMotionBlur()->getGiveUpReason().isEmpty());

    // Given up, it no longer tries, and a move renders as without motion blur
    const auto image = renderMove (renderer, scene, *bar, 0.0f, 0.1f);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

    setMotionBlur (scene, false);
    const auto without = renderMove (renderer, scene, *bar, 0.0f, 0.1f);
    EXPECT_EQ (countDifferences (image, without), 0);
}

TEST_F (MotionBlurMetalTests, MissingInputIsASkipNotAFailure)
{
    // A frame without the velocity, or without any input, skips the smear, and never counts towards
    // giving up: more frames than the two failures in a row that give up
    MotionBlur motionBlur (device);
    FrameResources resources (device);
    ViewData view;
    RenderSettings settings;
    settings.motionBlur = true;

    for (int i = 0; i < 3; ++i)
    {
        resources.beginFrame (64, 64);
        const auto color = resources.acquire (GpuTextureDesc (64, 64, GpuTextureFormat::rgba16float, true));
        const auto depth = resources.acquire (GpuTextureDesc (64, 64, GpuTextureFormat::depth32float, true));
        ASSERT_NE (color, nullptr);
        ASSERT_NE (depth, nullptr);

        auto frame = GpuFrame::begin (device);
        EXPECT_EQ (motionBlur.render (frame, view, settings, resources, { color, depth, nullptr, nullptr }), nullptr);
        EXPECT_EQ (motionBlur.render (frame, view, settings, resources, {}), nullptr);
        frame.submit();
        resources.endFrame();
    }

    EXPECT_FALSE (motionBlur.hasGivenUp());
    EXPECT_EQ (motionBlur.getNumCachedTargets(), 0u);
}

TEST_F (MotionBlurMetalTests, MotionBlurChangesKeepTheAccumulationConverged)
{
    // Motion blur applies after the accumulation: turning it on or changing the shutter neither
    // restarts nor discards it
    Scene scene;
    buildMovingBarScene (scene);
    setUpMotionBlur (scene, 4);
    setMotionBlur (scene, false);

    SceneRenderer renderer;
    ASSERT_TRUE (renderConverged (renderer, scene, size).isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    for (const auto shutter : { 1.0f, 0.25f })
    {
        setMotionBlur (scene, true, shutter);
        ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << renderer.getLastError();
        EXPECT_TRUE (renderer.isConverged()) << shutter;
    }

    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

class SceneRendererFrameStatsTests : public SceneRendererMetalTests
{
protected:
    static constexpr int size = 64;

    void SetUp() override
    {
        SceneRendererMetalTests::SetUp();
        if (! IsSkipped() && ! device->isScopeTimingAvailable())
            GTEST_SKIP() << "Scope timing is unavailable on this device";
    }

    /** The plates under a shadowed sun, a blended quad in front of them, a reflection probe, and a
        camera at f/1.4 focused on the small plate, on @p tier with ambient occlusion, screen-space
        reflections and GI, depth of field and bloom: every pass of the tier but the debug view. */
    static void buildEveryPassScene (Scene& scene, RenderSettings::TierPreference tier)
    {
        buildShadowsScene (scene);

        auto blended = Material::Ptr (new Material());
        blended->baseColorFactor = { 0.2f, 0.4f, 0.8f, 0.5f };
        blended->alphaMode = Material::AlphaMode::blend;
        auto* quad = addPlate (scene, blended);
        quad->setScale ({ 0.2f, 0.2f, 1.0f });
        quad->setPosition ({ -0.5f, 0.3f, 0.8f });

        addReflectionProbe (scene, { 0.0f, 0.0f, 1.0f }, 3.0f);

        addCamera (scene, { 0.0f, 0.0f, 3.0f }, {});
        auto& camera = *scene.getActiveCamera();
        camera.fStop = 1.4f;
        camera.focusDistance = 2.5f;

        auto settings = scene.getRenderSettings();
        settings.tier = tier;
        settings.ambientOcclusion = true;
        settings.screenSpaceReflections = true;
        settings.screenSpaceGI = true;
        settings.depthOfField = true;
        settings.bloom = true;
        scene.setRenderSettings (settings);
    }

    /** Renders @p frames frames of @p scene at size x size, waiting for each. */
    void renderAndWait (SceneRenderer& renderer, Scene& scene, int frames)
    {
        for (int i = 0; i < frames; ++i)
        {
            ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << renderer.getLastError();
            ASSERT_TRUE (renderer.waitForGPU().wasOk());
        }
    }

    /** The names of the scopes of @p stats, in order, separated by spaces. */
    static String getScopeNames (const SceneRenderer::FrameStats& stats)
    {
        StringArray names;
        for (const auto& scope : stats.gpu.scopes)
            names.add (scope.name);

        return names.joinIntoString (" ");
    }

    /** The milliseconds of the scope named @p name, or -1 when the frame has none or it is unavailable. */
    static double getScopeMilliseconds (const SceneRenderer::FrameStats& stats, const String& name)
    {
        for (const auto& scope : stats.gpu.scopes)
            if (scope.name == name && scope.available)
                return scope.milliseconds;

        return -1.0;
    }

    /** Every scope of @p stats is available, and its time a finite number of milliseconds. */
    static void expectScopesMeasured (const SceneRenderer::FrameStats& stats)
    {
        for (const auto& scope : stats.gpu.scopes)
        {
            EXPECT_TRUE (scope.available) << scope.name;
            EXPECT_TRUE (std::isfinite (scope.milliseconds)) << scope.name;
            EXPECT_GE (scope.milliseconds, 0.0) << scope.name;
        }
    }

    /** @p frames frames of @p scene queued without waiting, but for the frames up to the middle one
        and then all of them, which deliver the stats of those two frames whatever the speed of the
        GPU: the distinct stats seen after each render and after each wait, in the order they
        arrived. @p prepare edits the scene before each frame, given its number in the queue from 1,
        and returns its size. */
    template <class Prepare>
    std::vector<SceneRenderer::FrameStats> queueFrames (SceneRenderer& renderer, Scene& scene, int frames, Prepare&& prepare)
    {
        std::vector<SceneRenderer::FrameStats> seen;
        const auto take = [&]
        {
            const auto& stats = renderer.getLastFrameStats();
            if (stats.frameIndex != 0 && (seen.empty() || seen.back().frameIndex != stats.frameIndex))
                seen.push_back (stats);
        };

        for (int frame = 1; frame <= frames; ++frame)
        {
            const auto [width, height] = prepare (frame);
            EXPECT_NE (renderer.render (device, scene, width, height), nullptr) << renderer.getLastError();
            take();

            if (frame == frames / 2)
            {
                EXPECT_TRUE (renderer.waitForGPU().wasOk());
                take();
            }
        }

        EXPECT_TRUE (renderer.waitForGPU().wasOk());
        take();
        return seen;
    }

    /** Renders a frame with the stats on in the state @p prepare (true) sets up, then one with the
        stats off in the state @p prepare (false) sets up, then waits: the stats that arrive must be
        the first frame's, though the renderer then holds the second frame's state and status.
        @p prepare returns the size to render at. */
    template <class Prepare>
    SceneRenderer::FrameStats measureThenRenderAnother (SceneRenderer& renderer, Scene& scene, Prepare&& prepare)
    {
        for (const auto measured : { true, false })
        {
            renderer.setFrameStatsEnabled (measured);
            const auto [width, height] = prepare (measured);
            EXPECT_NE (renderer.render (device, scene, width, height), nullptr) << renderer.getLastError();
        }

        EXPECT_TRUE (renderer.waitForGPU().wasOk());
        return renderer.getLastFrameStats();
    }

    /** Adds @p count small opaque plates in front of the scene, hidden, and returns them. */
    static std::vector<EntityNode*> addHiddenPlates (Scene& scene, int count)
    {
        std::vector<EntityNode*> plates;
        for (int i = 0; i < count; ++i)
        {
            auto* plate = addPlate (scene, makeWallMaterial());
            plate->setScale ({ 0.05f, 0.05f, 1.0f });
            plate->setPosition ({ -0.8f + 0.2f * static_cast<float> (i), -0.6f, 0.6f });
            plate->setVisible (false);
            plates.push_back (plate);
        }

        return plates;
    }

    /** A 4 x 4 plate facing a camera 3 units away, lit by @p numLights point lights 0.5 in front of
        it whose range covers all of it, in the colors of the light show; on the full tier with TAA
        and nothing else, so the lighting pass shades every pixel with every light. */
    static void buildLightsScene (Scene& scene, int numLights)
    {
        auto material = Material::Ptr (new Material());
        material->metallicFactor = 0.0f;
        addPlate (scene, material)->setScale ({ 2.0f, 2.0f, 1.0f });
        addCamera (scene, { 0.0f, 0.0f, 3.0f }, {});

        for (int i = 0; i < numLights; ++i)
        {
            auto entity = EntityNode::Ptr (new EntityNode ("showLight"));
            auto& light = entity->attach<LightNode> (LightNode::Type::point);
            const auto color = Color::fromHSV (static_cast<float> (i) / static_cast<float> (numLights), 0.85f, 1.0f);
            light.color = { color.getRedFloat(), color.getGreenFloat(), color.getBlueFloat() };
            light.intensity = 0.05f;
            light.range = 10.0f;

            const auto angle = static_cast<float> (i) * 2.39996f;
            entity->setPosition ({ std::cos (angle) * 1.5f, std::sin (angle) * 1.5f, 0.5f });
            scene.getRoot()->addChild (entity);
        }

        auto settings = scene.getRenderSettings();
        settings.tier = RenderSettings::TierPreference::full;
        settings.ambientOcclusion = false;
        settings.screenSpaceReflections = false;
        settings.screenSpaceGI = false;
        scene.setRenderSettings (settings);
    }

    static double median (std::vector<double> values)
    {
        std::sort (values.begin(), values.end());
        return values[values.size() / 2];
    }
};

TEST_F (SceneRendererFrameStatsTests, FrameStatsListTheFullTierPasses)
{
    Scene scene;
    buildEveryPassScene (scene, RenderSettings::TierPreference::full);

    SceneRenderer renderer;
    renderer.setFrameStatsEnabled (true);

    // The first frame bakes the probe, and has no history yet for the screen-space effects to read
    renderAndWait (renderer, scene, 1);
    const auto first = renderer.getLastFrameStats();
    EXPECT_EQ (first.frameIndex, 1u);
    ASSERT_TRUE (first.gpu.available);
    EXPECT_EQ (getScopeNames (first), "probeBake shadows lightCulling gbuffer ambientOcclusion lighting forward temporalAA depthOfField bloom toneMapping");
    expectScopesMeasured (first);
    EXPECT_TRUE (first.shadowsRedrawn);
    EXPECT_GT (first.shadowCasterDraws, 0);

    renderAndWait (renderer, scene, 2);
    const auto& stats = renderer.getLastFrameStats();
    EXPECT_EQ (stats.frameIndex, 3u);
    ASSERT_TRUE (stats.gpu.available);
    EXPECT_GT (stats.gpu.frameMilliseconds, 0.0);
    EXPECT_EQ (getScopeNames (stats), "shadows lightCulling gbuffer ambientOcclusion hiZ screenSpaceReflections lighting indirectDiffuse forward temporalAA depthOfField bloom toneMapping");
    expectScopesMeasured (stats);

    // The two plates and the blended quad, at the size rendered; the shadow maps of the still scene are kept
    EXPECT_EQ (stats.drawItems, 3);
    EXPECT_EQ (stats.internalWidth, size);
    EXPECT_EQ (stats.internalHeight, size);
    EXPECT_FALSE (stats.shadowsRedrawn);
    EXPECT_EQ (stats.shadowCasterDraws, 0);

    // A debug view draws last, over the final image
    auto settings = scene.getRenderSettings();
    settings.debugView = RenderSettings::DebugView::ambientOcclusion;
    scene.setRenderSettings (settings);
    renderAndWait (renderer, scene, 1);

    const auto& debugged = renderer.getLastFrameStats();
    EXPECT_EQ (debugged.frameIndex, 4u);
    EXPECT_TRUE (getScopeNames (debugged).endsWith ("toneMapping debugView")) << getScopeNames (debugged);
    expectScopesMeasured (debugged);
}

TEST_F (SceneRendererFrameStatsTests, FrameStatsListTheRasterPasses)
{
    Scene scene;
    buildEveryPassScene (scene, RenderSettings::TierPreference::raster);

    SceneRenderer renderer;
    renderer.setFrameStatsEnabled (true);
    renderAndWait (renderer, scene, 2);

    const auto& stats = renderer.getLastFrameStats();
    EXPECT_EQ (stats.frameIndex, 2u);
    ASSERT_TRUE (stats.gpu.available);
    EXPECT_EQ (getScopeNames (stats), "shadows depthNormalPrepass ambientOcclusion forward depthOfField bloom toneMapping");
    expectScopesMeasured (stats);
    EXPECT_EQ (stats.drawItems, 3);

    // Depth of field alone draws the prepass for its depth, without the ambient occlusion
    auto settings = scene.getRenderSettings();
    settings.ambientOcclusion = false;
    settings.bloom = false;
    scene.setRenderSettings (settings);
    renderAndWait (renderer, scene, 1);
    EXPECT_EQ (getScopeNames (renderer.getLastFrameStats()), "shadows depthNormalPrepass forward depthOfField toneMapping");
    expectScopesMeasured (renderer.getLastFrameStats());

    // Ambient occlusion alone keeps its frame: no post chain
    settings.ambientOcclusion = true;
    settings.depthOfField = false;
    scene.setRenderSettings (settings);
    renderAndWait (renderer, scene, 1);
    EXPECT_EQ (getScopeNames (renderer.getLastFrameStats()), "shadows depthNormalPrepass ambientOcclusion forward");
    expectScopesMeasured (renderer.getLastFrameStats());
}

TEST_F (SceneRendererFrameStatsTests, MotionBlurIsListedOnMovingFramesOnly)
{
    // Motion blur on, after the depth of field and before bloom, only on the frames that moved
    Scene scene;
    buildEveryPassScene (scene, RenderSettings::TierPreference::full);
    auto settings = scene.getRenderSettings();
    settings.motionBlur = true;
    scene.setRenderSettings (settings);

    SceneRenderer renderer;
    renderer.setFrameStatsEnabled (true);
    renderAndWait (renderer, scene, 2);
    EXPECT_EQ (getScopeNames (renderer.getLastFrameStats()), "shadows lightCulling gbuffer ambientOcclusion hiZ screenSpaceReflections lighting indirectDiffuse forward temporalAA depthOfField bloom toneMapping");

    auto* camera = scene.getRoot()->findChild ("camera");
    ASSERT_NE (camera, nullptr);
    camera->setPosition (camera->getPosition() + Vector3<float> (0.05f, 0.0f, 0.0f));
    renderAndWait (renderer, scene, 1);
    EXPECT_EQ (getScopeNames (renderer.getLastFrameStats()), "shadows lightCulling gbuffer ambientOcclusion hiZ screenSpaceReflections lighting indirectDiffuse forward temporalAA depthOfField motionBlur bloom toneMapping");
    expectScopesMeasured (renderer.getLastFrameStats());

    renderAndWait (renderer, scene, 1);
    EXPECT_EQ (getScopeNames (renderer.getLastFrameStats()), "shadows lightCulling gbuffer ambientOcclusion hiZ screenSpaceReflections lighting indirectDiffuse forward temporalAA depthOfField bloom toneMapping");
}

TEST_F (SceneRendererFrameStatsTests, FrameStatsBelongToOneFrame)
{
    Scene scene;
    buildShadowsScene (scene);
    setFullTierTaa (scene, 16);
    const auto plates = addHiddenPlates (scene, 8);

    SceneRenderer renderer;
    renderer.setFrameStatsEnabled (true);
    renderAndWait (renderer, scene, 1);

    // The odd frames of the queue draw 8 more plates and bloom, the even ones neither
    const auto setUp = [&] (bool odd)
    {
        for (auto* plate : plates)
            plate->setVisible (odd);

        auto settings = scene.getRenderSettings();
        settings.bloom = odd;
        scene.setRenderSettings (settings);
        return std::pair { size, size };
    };

    const auto seen = queueFrames (renderer, scene, 6, [&] (int frame) { return setUp (frame % 2 == 1); });

    RecordProperty ("statsSeen", static_cast<int> (seen.size()));
    ASSERT_GE (seen.size(), 2u);
    EXPECT_EQ (seen.back().frameIndex, 7u);

    for (size_t i = 0; i < seen.size(); ++i)
    {
        const auto& stats = seen[i];
        if (i > 0)
            EXPECT_GT (stats.frameIndex, seen[i - 1].frameIndex);

        // The first frame was rendered before the queue: frame n of the queue is frame n + 1
        const auto odd = (stats.frameIndex - 1) % 2 == 1;
        ASSERT_TRUE (stats.gpu.available) << stats.frameIndex;
        EXPECT_GT (stats.gpu.frameMilliseconds, 0.0) << stats.frameIndex;
        EXPECT_EQ (stats.drawItems, odd ? 10 : 2) << stats.frameIndex;
        EXPECT_EQ (getScopeNames (stats).contains ("bloom"), odd) << stats.frameIndex;
        expectScopesMeasured (stats);
    }

    // Whatever the speed of the GPU: frame 8 measures, frame 9 draws less without bloom and does not
    const auto measured = measureThenRenderAnother (renderer, scene, setUp);
    EXPECT_EQ (measured.frameIndex, 8u);
    ASSERT_TRUE (measured.gpu.available);
    EXPECT_EQ (measured.drawItems, 10);
    EXPECT_TRUE (getScopeNames (measured).contains ("bloom"));
    expectScopesMeasured (measured);
}

TEST_F (SceneRendererFrameStatsTests, ResizeDoesNotMixFrames)
{
    Scene scene;
    buildShadowsScene (scene);
    setFullTierTaa (scene, 16);
    const auto plates = addHiddenPlates (scene, 8);

    SceneRenderer renderer;
    renderer.setFrameStatsEnabled (true);

    // The odd frames are wider and draw 8 more plates
    const auto setUp = [&] (bool odd)
    {
        for (auto* plate : plates)
            plate->setVisible (odd);

        return odd ? std::pair { 96, 48 } : std::pair { size, size };
    };

    const auto seen = queueFrames (renderer, scene, 6, [&] (int frame) { return setUp (frame % 2 == 1); });

    RecordProperty ("statsSeen", static_cast<int> (seen.size()));
    ASSERT_GE (seen.size(), 2u);
    EXPECT_EQ (seen.back().frameIndex, 6u);

    for (const auto& stats : seen)
    {
        const auto odd = stats.frameIndex % 2 == 1;
        ASSERT_TRUE (stats.gpu.available) << stats.frameIndex;
        EXPECT_EQ (stats.internalWidth, odd ? 96 : size) << stats.frameIndex;
        EXPECT_EQ (stats.internalHeight, odd ? 48 : size) << stats.frameIndex;
        EXPECT_EQ (stats.drawItems, odd ? 10 : 2) << stats.frameIndex;
        EXPECT_FALSE (stats.gpu.scopes.empty()) << stats.frameIndex;
        expectScopesMeasured (stats);
    }

    // Whatever the speed of the GPU: frame 7 measures at 96x48, frame 8 is smaller, draws less and does not
    const auto measured = measureThenRenderAnother (renderer, scene, setUp);
    EXPECT_EQ (measured.frameIndex, 7u);
    ASSERT_TRUE (measured.gpu.available);
    EXPECT_FALSE (measured.gpu.scopes.empty());
    EXPECT_EQ (measured.internalWidth, 96);
    EXPECT_EQ (measured.internalHeight, 48);
    EXPECT_EQ (measured.drawItems, 10);
    expectScopesMeasured (measured);
}

TEST_F (SceneRendererFrameStatsTests, StatsOffOpensNoScope)
{
    Scene scene;
    buildEveryPassScene (scene, RenderSettings::TierPreference::full);

    SceneRenderer renderer;
    EXPECT_FALSE (renderer.isFrameStatsEnabled());
    renderAndWait (renderer, scene, 2);
    EXPECT_EQ (renderer.getNumTimingCallsForTesting(), 0);
    EXPECT_EQ (renderer.getLastFrameStats().frameIndex, 0u);

    // On, a frame turns timing on and opens its scopes
    renderer.setFrameStatsEnabled (true);
    renderAndWait (renderer, scene, 1);
    const auto calls = renderer.getNumTimingCallsForTesting();
    EXPECT_GT (calls, 1);
    EXPECT_EQ (renderer.getLastFrameStats().frameIndex, 3u);

    // Off again, the frames make no call, and the stats stay those of the last frame measured
    renderer.setFrameStatsEnabled (false);
    renderAndWait (renderer, scene, 2);
    EXPECT_EQ (renderer.getNumTimingCallsForTesting(), calls);
    EXPECT_EQ (renderer.getLastFrameStats().frameIndex, 3u);
}

TEST_F (SceneRendererFrameStatsTests, HeavierPassMeasuresLonger)
{
    Scene oneLight, manyLights;
    buildLightsScene (oneLight, 1);
    buildLightsScene (manyLights, LightClusters::maxLightsPerCluster);

    SceneRenderer oneRenderer, manyRenderer;
    oneRenderer.setFrameStatsEnabled (true);
    manyRenderer.setFrameStatsEnabled (true);

    // The first frames compile and warm up, and are not counted
    renderAndWait (oneRenderer, oneLight, 1);
    renderAndWait (manyRenderer, manyLights, 1);

    std::vector<double> light, heavy;
    for (int i = 0; i < 5; ++i)
    {
        renderAndWait (oneRenderer, oneLight, 1);
        renderAndWait (manyRenderer, manyLights, 1);

        light.push_back (getScopeMilliseconds (oneRenderer.getLastFrameStats(), "lighting"));
        heavy.push_back (getScopeMilliseconds (manyRenderer.getLastFrameStats(), "lighting"));
    }

    RecordProperty ("oneLight", String (median (light), 4).toStdString());
    RecordProperty ("manyLights", String (median (heavy), 4).toStdString());

    ASSERT_GE (*std::min_element (light.begin(), light.end()), 0.0);
    EXPECT_GT (median (heavy), median (light) * 4.0);
}

/** Frame times of the Scene3D light show, measured by hand: disabled in the normal suites. Run them
    on a Release build with

    GTEST_ALSO_RUN_DISABLED_TESTS=1 just test Release "SceneRendererBenchmarkTests.*"

    The scene is the demo's: examples/graphics/data/gltf/mpc.glb seen from the top, the shadowed
    key light, the orbiting show lights and a dim environment. The "moving model" configurations
    also shift the model every frame, so the shadow maps are drawn again in each one, as while
    dragging it in the demo. The "orbiting camera" configurations turn the camera around the model
    every frame instead, as the demo's free movement does from its top view (or from 0.6 rad above
    the horizon, "low"), and print how many of the timed frames drew shadow maps. Each configuration renders 30
    warm-up frames, then times 120 queued frames by wall time up to the end of the last one on the
    GPU: the GPU cost of a frame wherever the GPU is the limit.
*/
class SceneRendererBenchmarkTests : public SceneRendererMetalTests
{
protected:
    /** mpc.glb lit like the Scene3D demo, its show lights moved by animate(). */
    struct LightShow
    {
        Scene scene;
        std::vector<EntityNode*> showLights;
        BoundingBox bounds;
        EntityNode* modelRoot = nullptr;
        EntityNode* cameraEntity = nullptr;
        std::vector<Material::Ptr> materials; // the model's
        Vector3<float> modelPosition;
        float cameraDistance = 0.0f;
        bool moveModel = false;
        float orbitPitch = 0.0f; // the camera orbits at this pitch when not 0
        EntityNode* knob = nullptr; // turned a little every frame when set, as by automation
        Quaternion knobRotation;
        double time = 0.0;
        int frame = 0;

        /** Builds the scene, or returns false without the model. */
        bool build (int numShowLights, float showRange, float keyRadius, RenderSettings::TierPreference tier, float aspectRatio)
        {
            const auto file = File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory().getChildFile ("examples/graphics/data/gltf/mpc.glb");
            auto model = GltfModel::loadFromFile (file);
            if (model.failed())
                return false;

            auto root = model.getReference().createEntity();
            scene.getRoot()->addChild (root);
            materials = model.getReference().getMaterials();

            auto* device = root->findChild ("Cube");
            auto* framed = device != nullptr ? device : root.get();
            root->setPosition (-framed->computeWorldBounds().getCenter());
            bounds = framed->computeWorldBounds();
            modelRoot = root.get();
            modelPosition = root->getPosition();

            // The top view of the demo, fitted to the aspect ratio
            auto camera = EntityNode::Ptr (new EntityNode ("camera"));
            auto& cameraNode = camera->attach<CameraNode>();
            const auto radius = jmax (0.001f, bounds.getRadius());
            cameraNode.zNear = radius * 0.02f;
            cameraNode.zFar = radius * 40.0f;

            const auto tanHalfFov = std::tan (cameraNode.yFov * 0.5f);
            const auto size = bounds.getSize();
            const auto distance = jmax (size.getX() * 0.5f / tanHalfFov, size.getZ() * 0.5f / (tanHalfFov * aspectRatio)) * 1.03f;
            const auto center = bounds.getCenter();
            aimCamera (*camera, center + Vector3<float> (0.0f, distance, 0.0f), center, { -1.0f, 0.0f, 0.0f });
            scene.getRoot()->addChild (camera);
            scene.setActiveCamera (camera.get());
            cameraEntity = camera.get();
            cameraDistance = distance;

            auto key = EntityNode::Ptr (new EntityNode ("keyLight"));
            auto& keyLight = key->attach<LightNode>();
            keyLight.castsShadows = true;
            keyLight.intensity = 4.0f;
            keyLight.radius = keyRadius;
            keyLight.color = { 1.0f, 0.96f, 0.9f };

            const auto azimuth = degreesToRadians (120.0f);
            const auto elevation = degreesToRadians (28.0f);
            const Vector3<float> keyDirection (std::cos (elevation) * std::cos (azimuth), std::sin (elevation), std::cos (elevation) * std::sin (azimuth));
            aimCamera (*key, keyDirection * 10.0f, {}, { 0.0f, 1.0f, 0.0f });
            scene.getRoot()->addChild (key);

            for (int i = 0; i < numShowLights; ++i)
            {
                auto entity = EntityNode::Ptr (new EntityNode ("showLight"));
                auto& showLight = entity->attach<LightNode> (LightNode::Type::point);
                const auto color = Color::fromHSV (static_cast<float> (i) / static_cast<float> (numShowLights), 0.85f, 1.0f);
                showLight.color = { color.getRedFloat(), color.getGreenFloat(), color.getBlueFloat() };
                showLight.intensity = 0.5f;
                showLight.range = showRange;
                showLights.push_back (entity.get());
                scene.getRoot()->addChild (entity);
            }

            scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>& direction)
            {
                const auto up = jlimit (0.0f, 1.0f, direction.getY() * 0.5f + 0.5f);
                return Vector3<float> (0.05f, 0.05f, 0.05f) + Vector3<float> (2.0f, 1.9f, 1.8f) * (up * up * up);
            }));

            scene.setEnvironmentIntensity (0.35f);
            scene.setAmbientColor (Colors::black);
            scene.setExposure (1.1f);
            scene.setToneMapping (Scene::ToneMapping::agx);
            scene.setEnvironmentVisible (true);
            scene.setEnvironmentBlur (0.5f);
            setTier (scene, tier);
            return true;
        }

        /** Moves the show lights one 120 Hz frame further along the orbits of the demo's LightShowAnimator,
            and the model by a hundredth of its size when moveModel is set. */
        void animate()
        {
            time += 1.0 / 120.0;

            if (knob != nullptr)
                knob->setRotation (knobRotation * Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.05f * static_cast<float> (++frame % 32)));

            if (moveModel)
                modelRoot->setPosition (modelPosition + Vector3<float> (bounds.getSize().getX() * 0.01f * static_cast<float> (++frame % 2), 0.0f, 0.0f));

            const auto center = bounds.getCenter();

            // The demo's orbit, the yaw turning 0.008 rad a frame
            if (orbitPitch != 0.0f)
            {
                const auto yaw = 0.008f * static_cast<float> (++frame);
                const Vector3<float> offset (std::cos (yaw) * std::cos (orbitPitch), std::sin (orbitPitch), -std::sin (yaw) * std::cos (orbitPitch));
                aimCamera (*cameraEntity, center + offset * cameraDistance, center);
            }

            const auto halfSize = bounds.getSize() * 0.5f;

            for (size_t i = 0; i < showLights.size(); ++i)
            {
                const auto ring = static_cast<float> (i % 4);
                const auto scale = 0.3f + 0.25f * ring;
                const auto speed = (i % 2 == 0 ? 0.35f : -0.25f) * (1.0f + 0.15f * ring);
                const auto angle = static_cast<float> (i) * 2.39996f + speed * static_cast<float> (time);
                const auto height = halfSize.getY() * (0.6f + 0.25f * ring);

                showLights[i]->setPosition ({ center.getX() + std::cos (angle) * halfSize.getX() * scale,
                                              center.getY() + height,
                                              center.getZ() + std::sin (angle) * halfSize.getZ() * scale });
            }
        }
    };

    /** The wall milliseconds per frame of 120 queued frames after 30 warm-up ones, or -1 on a failure.
        @p shadowRedraws receives the number of the timed frames that drew shadow maps, and
        @p casterDraws the shadow caster draws of the timed frames. The renderer collects its frame
        stats when @p frameStats is set, and waits for its frames after every @p maxFramesInFlight
        ones when it is above 0. */
    double timeFrames (LightShow& show, int width, int height, int& shadowRedraws, int* casterDraws = nullptr,
                       bool frameStats = false, int maxFramesInFlight = 0)
    {
        SceneRenderer renderer;
        renderer.setFrameStatsEnabled (frameStats);

        const auto renderFrames = [&] (int count)
        {
            for (int i = 0; i < count; ++i)
            {
                show.animate();
                if (renderer.render (device, show.scene, width, height) == nullptr)
                    return false;

                if (maxFramesInFlight > 0 && (i + 1) % maxFramesInFlight == 0 && renderer.waitForGPU().failed())
                    return false;
            }

            return renderer.waitForGPU().wasOk();
        };

        if (! renderFrames (30))
            return -1.0;

        const auto redrawsBefore = shadows (renderer).getNumRenderedFrames();
        const auto drawsBefore = shadows (renderer).getNumCasterDraws();
        const auto start = Time::getMillisecondCounterHiRes();
        if (! renderFrames (120))
            return -1.0;

        shadowRedraws = shadows (renderer).getNumRenderedFrames() - redrawsBefore;
        if (casterDraws != nullptr)
            *casterDraws = shadows (renderer).getNumCasterDraws() - drawsBefore;

        // Frames that measured only their total would leave nothing measured to compare
        if (frameStats)
        {
            const auto& stats = renderer.getLastFrameStats();
            const auto measured = std::count_if (stats.gpu.scopes.begin(), stats.gpu.scopes.end(), [] (const auto& scope) { return scope.available; });
            EXPECT_TRUE (stats.gpu.available);
            EXPECT_GT (measured, 0);
            std::cout << "frame " << stats.frameIndex << ": " << measured << " of " << stats.gpu.scopes.size() << " scopes measured" << std::endl;
        }

        return (Time::getMillisecondCounterHiRes() - start) / 120.0;
    }
};

TEST_F (SceneRendererBenchmarkTests, DISABLED_LightShowFrameTimes)
{
    const struct
    {
        const char* name;
        RenderSettings::TierPreference tier;
        int showLights;
        float range;
        bool moveModel = false;
        float orbitPitch = 0.0f;
        bool keyContactShadows = false;
        bool softShadows = true;
        bool ambientOcclusion = true; // follows the tier default when true, off otherwise
        bool probes = false;          // a box probe around the model and a sphere probe inside it
        bool volume = false;          // a 6 x 2 x 4 irradiance volume around the model
        bool layers = false;          // a clearcoat on the knob caps and a sheen on the pad rubber
        bool screenSpaceReflections = true; // follows the tier default when true, off otherwise
    } configs[] = {
        { "full show64", RenderSettings::TierPreference::full, 64, 0.5f },
        { "raster show64", RenderSettings::TierPreference::raster, 64, 0.5f },
        { "raster key+7", RenderSettings::TierPreference::raster, 7, 0.5f },
        { "full show64 range 0.35", RenderSettings::TierPreference::full, 64, 0.35f },
        { "raster show16 range 0.35", RenderSettings::TierPreference::raster, 16, 0.35f },
        { "full show64 range 0.35 moving model", RenderSettings::TierPreference::full, 64, 0.35f, true },
        { "raster show16 range 0.35 moving model", RenderSettings::TierPreference::raster, 16, 0.35f, true },
        { "full show64 range 0.35 orbiting camera", RenderSettings::TierPreference::full, 64, 0.35f, false, 1.56f },
        { "raster show16 range 0.35 orbiting camera", RenderSettings::TierPreference::raster, 16, 0.35f, false, 1.56f },
        { "full show64 range 0.35 orbiting camera low", RenderSettings::TierPreference::full, 64, 0.35f, false, 0.6f },
        { "raster show16 range 0.35 orbiting camera low", RenderSettings::TierPreference::raster, 16, 0.35f, false, 0.6f },
        { "full show64 key contact shadows", RenderSettings::TierPreference::full, 64, 0.5f, false, 0.0f, true },
        { "full show64 soft shadows off", RenderSettings::TierPreference::full, 64, 0.5f, false, 0.0f, false, false },
        { "full show64 ambient occlusion off", RenderSettings::TierPreference::full, 64, 0.5f, false, 0.0f, false, true, false },
        { "full show64 two probes", RenderSettings::TierPreference::full, 64, 0.5f, false, 0.0f, false, true, true, true },
        { "raster show64 two probes", RenderSettings::TierPreference::raster, 64, 0.5f, false, 0.0f, false, true, true, true },
        { "full show64 one volume", RenderSettings::TierPreference::full, 64, 0.5f, false, 0.0f, false, true, true, false, true },
        { "raster show64 one volume", RenderSettings::TierPreference::raster, 64, 0.5f, false, 0.0f, false, true, true, false, true },
        { "full show64 layered materials", RenderSettings::TierPreference::full, 64, 0.5f, false, 0.0f, false, true, true, false, false, true },
        { "raster show64 layered materials", RenderSettings::TierPreference::raster, 64, 0.5f, false, 0.0f, false, true, true, false, false, true },
        { "full show64 screen-space reflections off", RenderSettings::TierPreference::full, 64, 0.5f, false, 0.0f, false, true, true, false, false, false, false },
    };

    for (const auto& [width, height] : { std::pair { 2560, 1600 }, std::pair { 3456, 2234 } })
    {
        for (const auto& config : configs)
        {
            for (const auto keyRadius : { 0.08f, 0.0f })
            {
                LightShow show;
                if (! show.build (config.showLights, config.range, keyRadius, config.tier, static_cast<float> (width) / static_cast<float> (height)))
                    GTEST_SKIP() << "mpc.glb is missing";

                show.moveModel = config.moveModel;
                show.orbitPitch = config.orbitPitch;
                show.scene.getRoot()->findChild ("keyLight")->getNode<LightNode>()->contactShadows = config.keyContactShadows;

                if (! config.softShadows)
                {
                    auto settings = show.scene.getRenderSettings();
                    settings.softShadows = false;
                    show.scene.setRenderSettings (settings);
                }

                if (! config.ambientOcclusion)
                {
                    auto settings = show.scene.getRenderSettings();
                    settings.ambientOcclusion = false;
                    show.scene.setRenderSettings (settings);
                }

                if (! config.screenSpaceReflections)
                {
                    auto settings = show.scene.getRenderSettings();
                    settings.screenSpaceReflections = false;
                    show.scene.setRenderSettings (settings);
                }

                // Baked by the warm-up frames: the probes stay still
                if (config.probes)
                {
                    const auto bounds = show.modelRoot->computeWorldBounds();
                    const auto center = bounds.getCenter();
                    const auto size = bounds.getSize();

                    auto room = EntityNode::Ptr (new EntityNode ("roomProbe"));
                    auto& box = room->attach<ReflectionProbeNode>();
                    box.shape = ReflectionProbeNode::Shape::box;
                    box.influenceExtents = size * 0.75f;
                    room->setPosition (center);
                    show.scene.getRoot()->addChild (room);

                    auto local = EntityNode::Ptr (new EntityNode ("localProbe"));
                    local->attach<ReflectionProbeNode>().influenceRadius = size.length() * 0.25f;
                    local->setPosition (center + size * 0.2f);
                    show.scene.getRoot()->addChild (local);
                }

                // Baked by the warm-up frames too: the volume stays still
                if (config.volume)
                {
                    const auto bounds = show.modelRoot->computeWorldBounds();

                    auto entity = EntityNode::Ptr (new EntityNode ("volume"));
                    auto& volume = entity->attach<IrradianceVolumeNode>();
                    volume.extents = bounds.getSize() * 0.75f;
                    volume.resolutionX = 6;
                    volume.resolutionY = 2;
                    volume.resolutionZ = 4;
                    entity->setPosition (bounds.getCenter());
                    show.scene.getRoot()->addChild (entity);
                }

                // Shared by every primitive drawn with them, so the knobs and the pads change everywhere
                for (const auto& material : config.layers ? show.materials : std::vector<Material::Ptr>())
                {
                    if (material->name == "Knob Cap")
                    {
                        material->clearcoatFactor = 1.0f;
                        material->clearcoatRoughnessFactor = 0.1f;
                    }
                    else if (material->name == "Pad Rubber")
                    {
                        material->sheenColorFactor = { 0.8f, 0.8f, 0.8f };
                        material->sheenRoughnessFactor = 0.5f;
                    }
                }

                auto shadowRedraws = 0;
                const auto milliseconds = timeFrames (show, width, height, shadowRedraws);
                EXPECT_GT (milliseconds, 0.0) << config.name;

                const auto label = String (config.name) + ", key radius " + String (keyRadius, 2) + ", " + String (width) + "x" + String (height);
                RecordProperty (label.toStdString(), String (milliseconds, 3).toStdString());
                std::cout << label.toStdString() << ": " << String (milliseconds, 3).toStdString() << " ms, "
                          << shadowRedraws << " of 120 frames drew shadow maps" << std::endl;
            }
        }
    }
}

/** The cost of the depth of field on the full tier, in the light show with 16 show lights, at the
    demo size and at 2560 x 1600. Run it like DISABLED_LightShowFrameTimes. Each round times the
    frame without depth of field twice (A/A) and with it twice: focused on the model, where only the
    background blurs, and focused well in front of it, where every pixel blurs by the largest size.
*/
TEST_F (SceneRendererBenchmarkTests, DISABLED_DepthOfFieldFrameTimes)
{
    for (const auto& [width, height] : { std::pair { 1280, 800 }, std::pair { 2560, 1600 } })
    {
        std::map<String, std::vector<double>> times;

        for (int round = 0; round < 3; ++round)
        {
            for (const auto [name, focus] : { std::pair { "off", 0.0f }, std::pair { "focused", 1.0f }, std::pair { "all blurred", 0.3f }, std::pair { "off again", 0.0f } })
            {
                LightShow show;
                if (! show.build (16, 0.5f, 0.08f, RenderSettings::TierPreference::full, static_cast<float> (width) / static_cast<float> (height)))
                    GTEST_SKIP() << "mpc.glb is missing";

                auto settings = show.scene.getRenderSettings();
                settings.depthOfField = focus > 0.0f;
                show.scene.setRenderSettings (settings);

                auto& camera = *show.scene.getActiveCamera();
                camera.fStop = 1.4f;
                camera.focusDistance = show.cameraDistance * focus;

                int shadowRedraws = 0;
                const auto milliseconds = timeFrames (show, width, height, shadowRedraws);
                ASSERT_GT (milliseconds, 0.0) << name;
                times[name].push_back (milliseconds);
            }
        }

        for (const auto& [name, values] : times)
        {
            String line;
            for (const auto value : values)
                line << String (value, 3) << " ";

            std::cout << width << " x " << height << " " << name << ": " << line << "ms" << std::endl;
        }
    }
}

/** Raster depth of field in the light show with 16 show lights, focused on the model at f/1.4: the
    frames without it, with it, with ambient occlusion, with both, and without it again (A/A), on a
    renderer each, interleaved over 3 rounds, at the demo size and at 2560 x 1600. Prints the wall
    milliseconds per frame and the mean GPU milliseconds of the frame and of the passes depth of field
    adds or changes, measured through the frame stats with 2 frames in flight, and the load average
    of each round. Run it like DISABLED_LightShowFrameTimes, in Release.
*/
TEST_F (SceneRendererBenchmarkTests, DISABLED_RasterDepthOfFieldFrameTimes)
{
    if (! device->isScopeTimingAvailable())
        GTEST_SKIP() << "Scope timing is unavailable on this device";

    const char* const columns[] = { "wall", "gpu", "depthNormalPrepass", "ambientOcclusion", "forward", "depthOfField", "toneMapping" };

    const struct
    {
        const char* name;
        bool depthOfField;
        bool ambientOcclusion;
    } configs[] = {
        { "a off", false, false },
        { "b dof", true, false },
        { "c ao", false, true },
        { "d ao + dof", true, true },
        { "e off again", false, false },
    };

    for (const auto& [width, height] : { std::pair { 1280, 800 }, std::pair { 2560, 1600 } })
    {
        std::map<String, std::map<String, std::vector<double>>> times;

        for (int round = 0; round < 3; ++round)
        {
            double load[3] = {};
            getloadavg (load, 3);
            std::cout << width << " x " << height << " round " << round << ": load " << String (load[0], 2) << std::endl;

            for (const auto& config : configs)
            {
                LightShow show;
                if (! show.build (16, 0.5f, 0.08f, RenderSettings::TierPreference::raster, static_cast<float> (width) / static_cast<float> (height)))
                    GTEST_SKIP() << "mpc.glb is missing";

                auto settings = show.scene.getRenderSettings();
                settings.depthOfField = config.depthOfField;
                settings.ambientOcclusion = config.ambientOcclusion;
                show.scene.setRenderSettings (settings);

                auto& camera = *show.scene.getActiveCamera();
                camera.fStop = 1.4f;
                camera.focusDistance = show.cameraDistance;

                SceneRenderer renderer;
                renderer.setFrameStatsEnabled (true);

                std::map<String, double> sums;
                int measured = 0;
                uint64 lastFrame = 0;
                const auto take = [&]
                {
                    const auto& stats = renderer.getLastFrameStats();
                    if (stats.frameIndex == lastFrame || ! stats.gpu.available)
                        return;

                    lastFrame = stats.frameIndex;
                    ++measured;
                    sums["gpu"] += stats.gpu.frameMilliseconds;
                    for (const auto& scope : stats.gpu.scopes)
                        if (scope.available)
                            sums[scope.name] += scope.milliseconds;
                };

                const auto renderFrames = [&] (int count, bool collect)
                {
                    for (int i = 0; i < count; ++i)
                    {
                        show.animate();
                        if (renderer.render (device, show.scene, width, height) == nullptr)
                            return false;

                        if ((i + 1) % 2 == 0 && renderer.waitForGPU().failed())
                            return false;

                        if (collect)
                            take();
                    }

                    return renderer.waitForGPU().wasOk();
                };

                ASSERT_TRUE (renderFrames (30, false)) << config.name;
                lastFrame = renderer.getLastFrameStats().frameIndex;

                const auto start = Time::getMillisecondCounterHiRes();
                ASSERT_TRUE (renderFrames (120, true)) << config.name;
                const auto wall = (Time::getMillisecondCounterHiRes() - start) / 120.0;
                take();
                ASSERT_GT (measured, 0) << config.name;

                times[config.name]["wall"].push_back (wall);
                for (const auto* column : columns)
                    if (String (column) != "wall")
                        times[config.name][column].push_back (sums[column] / measured);
            }
        }

        for (const auto& config : configs)
        {
            String line;
            line << width << " x " << height << " " << config.name << ":";
            for (const auto* column : columns)
            {
                line << " " << column;
                for (const auto value : times[config.name][column])
                    line << " " << String (value, 3);
            }

            std::cout << line << std::endl;
        }
    }
}

/** Motion blur in the full tier light show with 16 show lights: off and on while the camera orbits
    (every frame moves, so every frame smears), on and off while only the lights move (still frames,
    which skip it), and off again while orbiting (A/A), on a renderer each, interleaved over 3
    rounds, at the demo size and at 2560 x 1600. Prints the wall milliseconds per frame and the mean
    GPU milliseconds of the frame and of the passes motion blur adds or changes, measured through
    the frame stats with 2 frames in flight, and the load average of each round. Run it like
    DISABLED_LightShowFrameTimes, in Release.
*/
TEST_F (SceneRendererBenchmarkTests, DISABLED_MotionBlurFrameTimes)
{
    if (! device->isScopeTimingAvailable())
        GTEST_SKIP() << "Scope timing is unavailable on this device";

    const char* const columns[] = { "wall", "gpu", "temporalAA", "motionBlur", "toneMapping" };

    const struct
    {
        const char* name;
        bool motionBlur;
        bool orbit;
    } configs[] = {
        { "a off, orbiting", false, true },
        { "b on, orbiting", true, true },
        { "c on, still", true, false },
        { "d off, still", false, false },
        { "e off, orbiting again", false, true },
    };

    for (const auto& [width, height] : { std::pair { 1280, 800 }, std::pair { 2560, 1600 } })
    {
        std::map<String, std::map<String, std::vector<double>>> times;

        for (int round = 0; round < 3; ++round)
        {
            double load[3] = {};
            getloadavg (load, 3);
            std::cout << width << " x " << height << " round " << round << ": load " << String (load[0], 2) << std::endl;

            for (const auto& config : configs)
            {
                LightShow show;
                if (! show.build (16, 0.5f, 0.08f, RenderSettings::TierPreference::full, static_cast<float> (width) / static_cast<float> (height)))
                    GTEST_SKIP() << "mpc.glb is missing";

                show.orbitPitch = config.orbit ? 0.6f : 0.0f;

                auto settings = show.scene.getRenderSettings();
                settings.motionBlur = config.motionBlur;
                show.scene.setRenderSettings (settings);

                SceneRenderer renderer;
                renderer.setFrameStatsEnabled (true);

                std::map<String, double> sums;
                int measured = 0;
                uint64 lastFrame = 0;
                const auto take = [&]
                {
                    const auto& stats = renderer.getLastFrameStats();
                    if (stats.frameIndex == lastFrame || ! stats.gpu.available)
                        return;

                    lastFrame = stats.frameIndex;
                    ++measured;
                    sums["gpu"] += stats.gpu.frameMilliseconds;
                    for (const auto& scope : stats.gpu.scopes)
                        if (scope.available)
                            sums[scope.name] += scope.milliseconds;
                };

                const auto renderFrames = [&] (int count, bool collect)
                {
                    for (int i = 0; i < count; ++i)
                    {
                        show.animate();
                        if (renderer.render (device, show.scene, width, height) == nullptr)
                            return false;

                        if ((i + 1) % 2 == 0 && renderer.waitForGPU().failed())
                            return false;

                        if (collect)
                            take();
                    }

                    return renderer.waitForGPU().wasOk();
                };

                ASSERT_TRUE (renderFrames (30, false)) << config.name;
                lastFrame = renderer.getLastFrameStats().frameIndex;

                const auto start = Time::getMillisecondCounterHiRes();
                ASSERT_TRUE (renderFrames (120, true)) << config.name;
                const auto wall = (Time::getMillisecondCounterHiRes() - start) / 120.0;
                take();
                ASSERT_GT (measured, 0) << config.name;

                // Orbiting frames smear, still ones never create the pass
                EXPECT_EQ (renderer.getMotionBlur() != nullptr, config.motionBlur && config.orbit) << config.name;

                times[config.name]["wall"].push_back (wall);
                for (const auto* column : columns)
                    if (String (column) != "wall")
                        times[config.name][column].push_back (sums[column] / measured);
            }
        }

        for (const auto& config : configs)
        {
            String line;
            line << width << " x " << height << " " << config.name << ":";
            for (const auto* column : columns)
            {
                line << " " << column;
                for (const auto value : times[config.name][column])
                    line << " " << String (value, 3);
            }

            std::cout << line << std::endl;
        }
    }
}

/** The cost of SceneRenderer::setFrameStatsEnabled() in the light show with 64 show lights on the
    full tier, at the demo size, with 2 frames in flight. Run it like DISABLED_LightShowFrameTimes.
    Each round times the frames with the stats off, on, off again (A/A) and on again, each on a device
    of its own: once a device measured a frame, its later frames pay a timestamp pair each.
*/
TEST_F (SceneRendererBenchmarkTests, DISABLED_FrameStatsOverhead)
{
    std::map<String, std::vector<double>> times;

    for (int round = 0; round < 5; ++round)
    {
        for (const auto [name, frameStats] : { std::pair { "a off", false }, std::pair { "b on", true }, std::pair { "c off again", false }, std::pair { "d on again", true } })
        {
            device = GpuDevice::create (GpuPlatform::Metal, {});
            ASSERT_NE (device, nullptr);

            LightShow show;
            if (! show.build (64, 0.5f, 0.08f, RenderSettings::TierPreference::full, 3456.0f / 2234.0f))
                GTEST_SKIP() << "mpc.glb is missing";

            int shadowRedraws = 0;
            const auto milliseconds = timeFrames (show, 3456, 2234, shadowRedraws, nullptr, frameStats, 2);
            ASSERT_GT (milliseconds, 0.0) << name;
            times[name].push_back (milliseconds);
        }
    }

    for (auto& [name, values] : times)
    {
        String line;
        for (const auto value : values)
            line << String (value, 3) << " ";

        std::sort (values.begin(), values.end());
        std::cout << name << ": " << line << "ms, median " << String (values[values.size() / 2], 3) << " ms" << std::endl;
    }
}

/** The Scene3D panel under automation: the demo's key light and pad spot plus two shadowed point
    lights over the pads, while one knob turns every frame, outside the reach of every point and spot
    light when one is (the test reports whether it is). Run it like DISABLED_LightShowFrameTimes, in
    Debug too, with and without MTL_DEBUG_LAYER=1: it prints the frame time and the shadow caster
    draws per frame.
*/
TEST_F (SceneRendererBenchmarkTests, DISABLED_KnobAutomationFrameTimes)
{
    for (const auto [tier, screenSpaceReflections] : { std::pair { RenderSettings::TierPreference::full, true },
                                                       std::pair { RenderSettings::TierPreference::full, false },
                                                       std::pair { RenderSettings::TierPreference::raster, true } })
    {
        for (const auto [keyShadows, nearestKnob] : { std::pair { true, false }, std::pair { false, false }, std::pair { true, true }, std::pair { false, true } })
        {
            LightShow show;
            if (! show.build (0, 0.5f, 0.08f, tier, 1.6f))
                GTEST_SKIP() << "mpc.glb is missing";

            if (! screenSpaceReflections)
            {
                auto settings = show.scene.getRenderSettings();
                settings.screenSpaceReflections = false;
                show.scene.setRenderSettings (settings);
            }

            auto& root = *show.scene.getRoot();
            auto& key = *root.findChild ("keyLight")->getNode<LightNode>();
            key.contactShadows = true;
            key.castsShadows = keyShadows;

            // The demo's pad spot, from the front right
            BoundingBox padBounds;
            std::vector<EntityNode*> knobs;
            const std::function<void (EntityNode&)> collect = [&] (EntityNode& entity)
            {
                for (const auto& child : entity.getChildren())
                {
                    if (child->getName().startsWith ("Pad."))
                        padBounds.expand (child->computeWorldBounds());
                    else if (child->getName().startsWith ("Knob"))
                        knobs.push_back (child.get());

                    collect (*child);
                }
            };
            collect (*show.modelRoot);
            ASSERT_FALSE (padBounds.isEmpty());
            ASSERT_FALSE (knobs.empty());

            const auto padCenter = padBounds.getCenter();
            const auto fromDegrees = [] (float azimuth, float elevation)
            {
                const auto a = degreesToRadians (azimuth);
                const auto e = degreesToRadians (elevation);
                return Vector3<float> (std::cos (e) * std::cos (a), std::sin (e), std::cos (e) * std::sin (a));
            };

            auto padEntity = EntityNode::Ptr (new EntityNode ("padLight"));
            auto& pad = padEntity->attach<LightNode> (LightNode::Type::spot);
            pad.castsShadows = true;
            pad.intensity = 10.0f;
            pad.radius = 0.03f;
            pad.range = 4.0f;
            pad.innerConeAngle = 0.3f;
            pad.outerConeAngle = 0.6f;
            aimCamera (*padEntity, padCenter + fromDegrees (-45.0f, 45.0f) * 1.6f, padCenter, { 0.0f, 1.0f, 0.0f });
            root.addChild (padEntity);

            // Two small shadowed point lights over the pads
            const auto padSize = padBounds.getSize();
            for (const auto side : { -0.25f, 0.25f })
            {
                auto entity = EntityNode::Ptr (new EntityNode ("padPoint"));
                auto& point = entity->attach<LightNode> (LightNode::Type::point);
                point.castsShadows = true;
                point.intensity = 0.5f;
                point.range = jmax (padSize.getX(), padSize.getZ()) * 0.3f;
                entity->setPosition (padCenter + Vector3<float> (padSize.getX() * side, padSize.getY() + point.range * 0.5f, 0.0f));
                root.addChild (entity);
            }

            // The knob farthest from the pads, which the spot light does not reach, or the nearest one
            const auto byDistance = [&] (EntityNode* a, EntityNode* b)
            {
                return (a->computeWorldBounds().getCenter() - padCenter).length() < (b->computeWorldBounds().getCenter() - padCenter).length();
            };
            auto* knob = nearestKnob ? *std::min_element (knobs.begin(), knobs.end(), byDistance) : *std::max_element (knobs.begin(), knobs.end(), byDistance);

            const auto knobBounds = knob->computeWorldBounds();
            const auto padDirection = (padCenter - padEntity->getPosition()).normalized();
            // The knob's bounding sphere against the spot's cone, widened by a margin over the guard band of its view
            const auto reachesSpot = [&]
            {
                const auto toKnob = knobBounds.getCenter() - padEntity->getPosition();
                const auto distance = toKnob.length();
                const auto angle = std::acos (jlimit (-1.0f, 1.0f, toKnob.dotProduct (padDirection) / distance));
                return angle - std::asin (jmin (1.0f, knobBounds.getRadius() / distance)) <= pad.outerConeAngle + 0.05f;
            };

            auto reachesPoint = false;
            for (const auto& child : root.getChildren())
                if (child->getName() == "padPoint")
                    reachesPoint = reachesPoint || GpuSceneData::lightReachesBox (knobBounds, child->getPosition(), {}, child->getNode<LightNode>()->range, false, 0.0f);

            EXPECT_TRUE (nearestKnob || ! reachesSpot());
            EXPECT_TRUE (nearestKnob || ! reachesPoint);
            show.knob = knob;
            show.knobRotation = knob->getRotation();

            auto shadowRedraws = 0, casterDraws = 0;
            const auto milliseconds = timeFrames (show, 2560, 1600, shadowRedraws, &casterDraws);
            EXPECT_GT (milliseconds, 0.0);

            const auto label = String (tier == RenderSettings::TierPreference::full ? "full" : "raster")
                             + (screenSpaceReflections ? "" : ", screen-space reflections off")
                             + (keyShadows ? ", key shadowed" : ", key unshadowed")
                             + ", knob " + knob->getName() + (reachesSpot() || reachesPoint ? " inside a local light" : " outside every local light");
            RecordProperty (label.toStdString(), String (milliseconds, 3).toStdString());
            std::cout << label.toStdString() << ": " << String (milliseconds, 3).toStdString() << " ms, "
                      << shadowRedraws << " of 120 frames drew shadow maps, "
                      << String (casterDraws / 120.0, 1).toStdString() << " caster draws per frame" << std::endl;
        }
    }
}

#endif

#if YUP_LINUX
class SceneRendererGLTests : public SceneRendererBaselineScenes
{
protected:
    void SetUp() override
    {
        setenv ("LIBGL_ALWAYS_SOFTWARE", "1", 0);
        setenv ("GALLIUM_DRIVER", "llvmpipe", 0);

        SDL_SetHint (SDL_HINT_RENDER_DRIVER, "opengl");
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, YUP_RIVE_OPENGL_MAJOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, YUP_RIVE_OPENGL_MINOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        window = SDL_CreateWindow ("yup_3d_gl_test", 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        if (window == nullptr)
            GTEST_SKIP() << "Cannot create an SDL window: " << SDL_GetError();

        glContext = SDL_GL_CreateContext (window);
        if (glContext == nullptr)
            GTEST_SKIP() << "Cannot create an OpenGL context: " << SDL_GetError();

        SDL_GL_MakeCurrent (window, glContext);

        GpuDevice::Options options;
        options.loaderFunction = (GpuDevice::LoaderFunction) SDL_GL_GetProcAddress;
        options.readableFramebuffer = true;

        device = GpuDevice::create (GpuPlatform::OpenGL, options);
        if (device == nullptr)
            GTEST_SKIP() << "No OpenGL device available";

        context = GraphicsContext::createContext (GpuPlatform::OpenGL, options, device);
        if (context == nullptr || ! context->isGpuAvailable())
            GTEST_SKIP() << "No OpenGL graphics context available";
    }

    void TearDown() override
    {
        context = nullptr;
        device = nullptr;

        if (glContext != nullptr)
            SDL_GL_DestroyContext (glContext);

        if (window != nullptr)
            SDL_DestroyWindow (window);
    }

    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;
};

TEST_F (SceneRendererGLTests, CapturesBaselineScenes)
{
    captureBaselineScenes ("gl");
}

TEST_F (SceneRendererGLTests, MatchesPhaseZeroBaselineOnRaster)
{
    for (const auto* name : { "shadows", "materials", "edges" })
        if (! loadBaseline (String ("gl_") + name + ".png").isValid())
            GTEST_SKIP() << "No gl_" << name << ".png baseline captured yet";

    checkMatchesPhaseZeroBaseline ("gl");
}

TEST_F (SceneRendererGLTests, DefaultSettingsAddNoOptInPass)
{
    checkDefaultSettingsAddNoOptInPass (RenderSettings::TierPreference::raster);
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererGLTests, RasterShadesMoreThanEightLightsPerScene)
{
    checkShadesMoreThanEightLightsPerScene (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, TinyRadiusMatchesPunctual)
{
    checkTinyRadiusMatchesPunctual (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, SceneColorHasNoNaN)
{
    checkSceneColorHasNoNaN (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, AreaLightRadiusWidensTheHighlightAndStaysFinite)
{
    checkRadiusWidensTheHighlight (RenderSettings::TierPreference::raster);
    checkAreaLightsStayFinite (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, LayeredAreaLightsStayFinite)
{
    checkAreaLightsStayFinite (RenderSettings::TierPreference::raster, true);
}

TEST_F (SceneRendererGLTests, LayersUnderATinyAreaLightMatchPunctual)
{
    checkLayersUnderATinyAreaLightMatchPunctual (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ShadowedRegionIsFinite)
{
    checkShadowedRegionIsFinite (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RendersWithoutTheShadowCompareSampler)
{
    checkRendersWithoutTheShadowCompareSampler (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, PrepassMatchesTheGBufferEncoding)
{
    checkPrepassMatchesTheGBufferEncoding (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, AmbientOcclusionDarkensTheContactCrease)
{
    checkAmbientOcclusionDarkensTheContactCrease (RenderSettings::TierPreference::raster);
}
#endif

TEST_F (SceneRendererGLTests, PrepassIsNotCreatedByDefault)
{
    checkPrepassIsNotCreatedByDefault (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RendersWithoutTheDepthNormalPrepass)
{
    checkRendersWithoutTheDepthNormalPrepass (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, AmbientOcclusionOffAllocatesNothing)
{
    checkAmbientOcclusionOffAllocatesNothing (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, AmbientOcclusionUnavailableOnRasterRendersFallback)
{
    checkAmbientOcclusionUnavailableOnRasterRendersFallback (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, SsrUnavailableOnRasterRendersFallback)
{
    checkScreenSpaceReflectionsUnavailableOnRasterRendersFallback (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, AmbientOcclusionLeavesTheDirectLightAlone)
{
    checkAmbientOcclusionLeavesTheDirectLightAlone (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ConvergedAmbientOcclusionIsDeterministic)
{
    checkConvergedAmbientOcclusionIsDeterministic (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, BlendedSurfacesSkipAmbientOcclusion)
{
    checkBlendedSurfacesSkipAmbientOcclusion (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, BentNormalsTurnTheDiffuseLightToTheOpenSide)
{
    checkBentNormalsTurnTheDiffuseLightToTheOpenSide (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterAmbientOcclusionOffAgainRestoresTheFrame)
{
    checkRasterAmbientOcclusionOffAgainRestoresTheFrame();
}

TEST_F (SceneRendererGLTests, RendersWithoutTheShadowPipelines)
{
    checkRendersWithoutTheShadowPipelines (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ShadowMapsTheDeviceCannotCreateLeaveTheLightsUnshadowed)
{
    checkShadowMapsTheDeviceCannotCreateLeaveTheLightsUnshadowed (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ShadowedLightOutOfReachAllocatesNoAtlas)
{
    checkShadowedLightOutOfReachAllocatesNoAtlas (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, FailedShadowMapKeepsTheOtherOneCached)
{
    checkFailedShadowMapKeepsTheOtherOneCached (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, AtlasIsReleasedWhenNoLightNeedsIt)
{
    checkAtlasIsReleasedWhenNoLightNeedsIt (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, AtlasRedrawsOnlyTheViewsACasterReaches)
{
    checkAtlasRedrawsOnlyTheViewsACasterReaches (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, PartiallyRedrawnAtlasMatchesAFreshOne)
{
    checkPartiallyRedrawnAtlasMatchesAFreshOne (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, SingleSpotTakesHalfTheAtlas)
{
    checkSingleSpotTakesHalfTheAtlas (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, LocalViewsShrinkToFitTheAtlas)
{
    checkLocalViewsShrinkToFitTheAtlas (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, LocalLightsKeepTheirQuarterTiles)
{
    checkLocalLightsKeepTheirQuarterTiles (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, AtlasRelayoutRedrawsTheMovedTiles)
{
    checkAtlasRelayoutRedrawsTheMovedTiles (RenderSettings::TierPreference::raster);
}

// With GL storing another depth than the shaders compute (clip z remapped differently on the two
// sides) the near shadow disappears or the whole ground darkens: these two catch either way
TEST_F (SceneRendererGLTests, SunShadowReachesNearAndFarCasters)
{
    checkSunShadowReachesNearAndFarCasters (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, CascadeTransitionHasNoSeam)
{
    checkCascadeTransitionHasNoSeam (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, AreaSunCastsAShadow)
{
    checkAreaSunCastsAShadow (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, MaskedCasterShadowsOnlyWhatItKeeps)
{
    checkMaskedCasterShadowsOnlyWhatItKeeps (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, PointLightShadowsOnAllSixFaces)
{
    checkPointLightShadowsOnAllSixFaces (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, PointShadowHasNoFaceSeams)
{
    checkPointShadowHasNoFaceSeams (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, SpotLightCastsAShadow)
{
    checkSpotCastsAShadow (RenderSettings::TierPreference::raster, 0.0f);
}

TEST_F (SceneRendererGLTests, AreaSpotCastsAShadowTowardsItsCenter)
{
    checkSpotCastsAShadow (RenderSettings::TierPreference::raster, 0.1f);
    checkAreaSpotCastsAShadowTowardsItsCenter (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ManyShadowedLightsRenderWithoutError)
{
    checkManyShadowedLightsRenderWithoutError (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ContactShadowsIgnoredOnRaster)
{
    checkContactShadowsIgnoredOnRaster (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ContactShadowSettingsLiveInTheFirstShadowRecord)
{
    checkContactShadowSettingsLiveInTheFirstShadowRecord (RenderSettings::TierPreference::raster);
}

// The blocker search reads the raw depth through its own texture unit, next to the compared one
TEST_F (SceneRendererGLTests, PcssPenumbraWidensWithLightRadius)
{
    checkPcssPenumbraWidensWithLightRadius (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, PenumbraWidensWithDistanceFromTheBlocker)
{
    checkPenumbraWidensWithDistanceFromTheBlocker (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterSoftShadowsOptIn)
{
    checkRasterSoftShadowsOptIn();
}

TEST_F (SceneRendererGLTests, RasterSoftShadowVariantFollowsTheSettings)
{
    checkRasterSoftShadowVariantFollowsTheSettings();
}

TEST_F (SceneRendererGLTests, SpotPenumbraWidensWithLightRadius)
{
    checkSpotPenumbraWidensWithLightRadius (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, SoftShadowsFilterAtLeastTwoTexels)
{
    checkSoftShadowsFilterAtLeastTwoTexels (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, SoftPointShadowHasNoFaceSeams)
{
    checkPointShadowHasNoFaceSeams (RenderSettings::TierPreference::raster, 0.1f);
}

TEST_F (SceneRendererGLTests, SoftSunLeavesAGrazedGroundLit)
{
    checkSoftSunLeavesAGrazedGroundLit (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ShadowFilterVariantFollowsSoftAndContactShadows)
{
    checkShadowFilterVariantFollowsSoftAndContactShadows (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ReflectionProbeShowsWhatTheCameraCannotSee)
{
    checkReflectionProbeShowsWhatTheCameraCannotSee (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ReplacedProbeIsBakedAgain)
{
    checkReplacedProbeIsBakedAgain (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, MovingAProbeBakesItAgain)
{
    checkMovingAProbeBakesItAgain (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ProbeBlendWithoutEnvironmentIsFinite)
{
    checkProbeBlendWithoutEnvironmentIsFinite (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, BoxProbeWithParallaxReflects)
{
    checkBoxProbeWithParallaxReflects (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, BakeProbesReportsPendingCount)
{
    checkBakeProbesReportsPendingCount (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RemovedProbesReleaseTheAtlas)
{
    checkRemovedProbesReleaseTheAtlas (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ProbeOfAnOpenSkyMatchesTheEnvironment)
{
    checkProbeOfAnOpenSkyMatchesTheEnvironment (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RgbmProbeShowsLikeTheFloatOne)
{
    checkRgbmProbeShowsLikeTheFloatOne (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ProbeKeepsTheMultipleScatteringOfRoughMetals)
{
    checkProbeKeepsTheMultipleScatteringOfRoughMetals (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, BoxProbeParallaxPlacesTheReflectionOnTheWalls)
{
    checkBoxProbeParallaxPlacesTheReflectionOnTheWalls (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, FailedProbeCapturesLeaveTheFrameWithoutProbes)
{
    checkFailedProbeCapturesLeaveTheFrameWithoutProbes (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, IrradianceVolumeTintsTheFloorNearTheRedWall)
{
    checkIrradianceVolumeTintsTheFloorNearTheRedWall (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, VisibilityStopsLightLeakingThroughTheWall)
{
    checkVisibilityStopsLightLeakingThroughTheWall (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, MovingAVolumeBakesItAgain)
{
    checkMovingAVolumeBakesItAgain (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, BakeProbesBakesTheVolumes)
{
    checkBakeProbesBakesTheVolumes (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RemovedVolumesReleaseTheAtlases)
{
    checkRemovedVolumesReleaseTheAtlases (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RgbmVolumeLightsLikeTheFloatOne)
{
    checkRgbmVolumeLightsLikeTheFloatOne (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, FailedVolumeCapturesLeaveTheFrameWithoutVolumes)
{
    checkFailedVolumeCapturesLeaveTheFrameWithoutVolumes (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ProgressiveVolumeBakeShadesOnceComplete)
{
    checkProgressiveVolumeBakeShadesOnceComplete (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, FailedVolumeCapturesLeaveTheProbesReflecting)
{
    checkFailedVolumeCapturesLeaveTheProbesReflecting (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ResizingAVolumeBakesItAgain)
{
    checkResizingAVolumeBakesItAgain (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, VolumeLightsWithoutTheEnvironmentAndKeepsItsIntensity)
{
    checkVolumeLightsWithoutTheEnvironmentAndKeepsItsIntensity (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ChangedVolumeBakesAgainWhole)
{
    checkChangedVolumeBakesAgainWhole (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, FailedProbeCapturesLeaveTheVolumesLighting)
{
    checkFailedProbeCapturesLeaveTheVolumesLighting (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ClearcoatAddsASharpHighlightOnARoughBase)
{
    checkClearcoatAddsASharpHighlightOnARoughBase (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, SheenBrightensGrazingEdges)
{
    checkSheenBrightensGrazingEdges (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, SheenPassesTheWhiteFurnaceTest)
{
    checkSheenPassesTheWhiteFurnaceTest (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, LayeredVariantFollowsTheMaterials)
{
    checkLayeredVariantFollowsTheMaterials (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, FailedLayersVariantShadesWithoutTheLayers)
{
    checkFailedLayersVariantShadesWithoutTheLayers (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, SheenOverAWhiteBaseKeepsTheEnergy)
{
    checkSheenOverAWhiteBaseKeepsTheEnergy (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ClearcoatDimsTheEmission)
{
    checkClearcoatDimsTheEmission (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ClearcoatReflectsTheProbe)
{
    checkClearcoatReflectsTheProbe (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterDepthOfFieldBlursOutOfFocusBarsOnly)
{
    if (! isDepthOfFieldAvailable (RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster depth of field is unavailable on this device";

    checkDepthOfFieldBlursOutOfFocusBarsOnly (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterNearFieldBleedsOverTheFocusedBackground)
{
    if (! isDepthOfFieldAvailable (RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster depth of field is unavailable on this device";

    checkNearFieldBleedsOverTheFocusedBackground (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterInFocusEdgesMatchWithoutDepthOfField)
{
    if (! isDepthOfFieldAvailable (RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster depth of field is unavailable on this device";

    checkInFocusEdgesMatchWithoutDepthOfField (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterDepthOfFieldOffChangesNothing)
{
    checkDepthOfFieldOffChangesNothing (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterDepthOfFieldKeepsTheSilhouetteBlendOverTheBackground)
{
    if (! isDepthOfFieldAvailable (RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster depth of field is unavailable on this device";

    checkDepthOfFieldKeepsTheSilhouetteBlendOverTheBackground (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterBlurredHighlightsKeepTheirBrightnessOverTheBackground)
{
    if (! isDepthOfFieldAvailable (RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster depth of field is unavailable on this device";

    checkBlurredHighlightsKeepTheirBrightnessOverTheBackground (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterDepthOfFieldDrawsThePrepassWithoutAmbientOcclusion)
{
    if (! isDepthOfFieldAvailable (RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster depth of field is unavailable on this device";

    checkRasterDepthOfFieldDrawsThePrepassWithoutAmbientOcclusion();
}

TEST_F (SceneRendererGLTests, RasterDepthOfFieldUnavailableRecordsNothing)
{
    checkRasterDepthOfFieldUnavailableRecordsNothing();
}

TEST_F (SceneRendererGLTests, RasterDepthOfFieldSkipsFramesWithoutThePrepassDepth)
{
    if (! isDepthOfFieldAvailable (RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster depth of field is unavailable on this device";

    checkRasterDepthOfFieldSkipsFramesWithoutThePrepassDepth();
}

TEST_F (SceneRendererGLTests, RasterDepthOfFieldThatGaveUpStopsItsRoute)
{
    if (! isDepthOfFieldAvailable (RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster depth of field is unavailable on this device";

    checkRasterDepthOfFieldThatGaveUpStopsItsRoute();
}

TEST_F (SceneRendererGLTests, RasterMotionBlurRendersTheUnblurredFallback)
{
    checkMotionBlurOnRasterRendersTheUnblurredFallback();
}
#endif
