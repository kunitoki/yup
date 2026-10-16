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

class SceneRendererTests : public ::testing::Test
{
};

TEST_F (SceneRendererTests, UsesFourSamplesByDefault)
{
    const SceneRenderer renderer;

    EXPECT_EQ (renderer.getSampleCount(), 4);
    EXPECT_TRUE (renderer.getLastError().isEmpty());
}

TEST_F (SceneRendererTests, RenderSettingsDefaultAndAreKept)
{
    SceneRenderer renderer;
    EXPECT_TRUE (renderer.getRenderSettings() == RenderSettings {});

    RenderSettings settings;
    settings.bloom = true;
    settings.taaSamples = 8;
    renderer.setRenderSettings (settings);

    EXPECT_TRUE (renderer.getRenderSettings() == settings);
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
#include "yup_renderer_shaders/yup_ForwardPass.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassAreaLights()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_ForwardPass_area_lights.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassSoftShadows()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_ForwardPass_soft_shadows.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassAreaLightsSoftShadows()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_ForwardPass_area_lights_soft_shadows.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_ForwardPass_local_indirect.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassAreaLightsLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_ForwardPass_area_lights_local_indirect.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassSoftShadowsLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_ForwardPass_soft_shadows_local_indirect.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassAreaLightsSoftShadowsLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_ForwardPass_area_lights_soft_shadows_local_indirect.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassFull()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_ForwardPass_tier_full.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassFullAreaLights()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_ForwardPass_tier_full_area_lights.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassFullSoftShadows()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_ForwardPass_tier_full_soft_shadows.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassFullAreaLightsSoftShadows()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_ForwardPass_tier_full_area_lights_soft_shadows.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle background()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_Background.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle shadowPass()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_ShadowPass.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle shadowPassMasked()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_ShadowPass_masked.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFull()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullAreaLights()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_area_lights.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullSoftShadows()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_soft_shadows.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullAreaLightsSoftShadows()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_area_lights_soft_shadows.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_local_indirect.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullAreaLightsLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_area_lights_local_indirect.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullSoftShadowsLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_soft_shadows_local_indirect.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullAreaLightsSoftShadowsLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_area_lights_soft_shadows_local_indirect.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_ForwardPass_local_indirect_layers.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassAreaLightsLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_ForwardPass_area_lights_local_indirect_layers.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassSoftShadowsLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_ForwardPass_soft_shadows_local_indirect_layers.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassAreaLightsSoftShadowsLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_ForwardPass_area_lights_soft_shadows_local_indirect_layers.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_local_indirect_layers.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullAreaLightsLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_area_lights_local_indirect_layers.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullSoftShadowsLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_soft_shadows_local_indirect_layers.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassFullAreaLightsSoftShadowsLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_area_lights_soft_shadows_local_indirect_layers.inc"
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
#include "yup_renderer_shaders/yup_GBufferPass_tier_full.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle gtaoTrace()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_GtaoTrace.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle gtaoDenoise()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_GtaoDenoise.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle ssrTrace()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_SsrTrace.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle ssrResolve()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_SsrResolve.inc"
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

#if YUP_APPLE
TEST_F (SceneRendererMetalTests, CapturesBaselineScenes)
{
    captureBaselineScenes ("metal");
}

TEST_F (SceneRendererMetalTests, MatchesPhaseZeroBaselineOnRaster)
{
    checkMatchesPhaseZeroBaseline ("metal");
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

    SceneRenderer single;
    setTier (single, RenderSettings::TierPreference::raster);
    single.setSampleCount (1);
    const auto image = renderToImage (single, scene, 64);
    ASSERT_TRUE (single.getLastError().isEmpty()) << single.getLastError();
    ASSERT_TRUE (image.isValid());

    SceneRenderer multisampled;
    setTier (multisampled, RenderSettings::TierPreference::raster);
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

    SceneRenderer renderer;
    setFullTierTaa (renderer, 4);
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

TEST_F (SceneRendererMetalTests, SwappingTransformsOfTwoMeshesIsAChange)
{
    Scene scene;

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
    setFullTierTaa (renderer, 4);
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

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererMetalTests, FailedRenderClearsThePassOutputs)
{
    Scene scene;
    buildShadowsScene (scene);

    SceneRenderer renderer;
    setTier (renderer, RenderSettings::TierPreference::full);
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

            SceneRenderer renderer;
            setTier (renderer, tier);
            pinCrossTierShadows (renderer);
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

TEST_F (SceneRendererMetalTests, BackgroundIsKeptBehindLitPixels)
{
    // A corner shows the environment: 0.5 through ACES is 0.616, 206 once sRGB encoded
    {
        Scene scene;
        buildShadowsScene (scene);

        SceneRenderer renderer;
        setTier (renderer, RenderSettings::TierPreference::full);
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

        SceneRenderer renderer;
        setTier (renderer, tier);
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

TEST_P (SceneRendererTierTests, ReportsTheRequestedTier)
{
    Scene scene;
    buildEdgesScene (scene);

    SceneRenderer renderer;
    setTier (renderer, GetParam());
    EXPECT_EQ (renderer.getCapabilities(), nullptr);

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto* capabilities = renderer.getCapabilities();
    ASSERT_NE (capabilities, nullptr);
    EXPECT_EQ (capabilities->getTier(), GetParam() == RenderSettings::TierPreference::full ? RenderCapabilities::Tier::full : RenderCapabilities::Tier::raster);
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

    SceneRenderer renderer;
    setTier (renderer, GetParam());
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

TEST_P (SceneRendererTierTests, DefaultSettingsAddNoOptInPass)
{
    checkDefaultSettingsAddNoOptInPass (GetParam());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_P (SceneRendererTierTests, PrepassMatchesTheGBufferEncoding)
{
    checkPrepassMatchesTheGBufferEncoding (GetParam());
}
#endif

TEST_P (SceneRendererTierTests, PrepassIsNotCreatedByDefault)
{
    checkPrepassIsNotCreatedByDefault (GetParam());
}

INSTANTIATE_TEST_SUITE_P (SceneRendererTiers,
                          SceneRendererTierTests,
                          ::testing::Values (RenderSettings::TierPreference::raster, RenderSettings::TierPreference::full),
                          [] (const auto& info)
                          {
                              return info.param == RenderSettings::TierPreference::full ? "full" : "raster";
                          });
#endif
