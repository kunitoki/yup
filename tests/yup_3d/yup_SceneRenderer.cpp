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
    // lists. The area light variants add the two area light tables. The shared lookups (the
    // environment and the area light tables) live in set 0, set 1 holds no texture, and set 3 the
    // sun cascades and the point and spot light atlas, read with the ShadowBlock. The soft shadow
    // variants read both maps raw too
    for (const auto& [name, bundle, textures, lookups, shadowMaps] : { std::tuple { "ForwardPass", forwardPass(), 8, 1, 2 },
                                                                       std::tuple { "ForwardPass_area_lights", forwardPassAreaLights(), 10, 3, 2 },
                                                                       std::tuple { "ForwardPass_soft_shadows", forwardPassSoftShadows(), 10, 1, 4 },
                                                                       std::tuple { "ForwardPass_area_lights_soft_shadows", forwardPassAreaLightsSoftShadows(), 12, 3, 4 } })
    {
        const auto* vertex = bundle.findShader (ShaderStage::vertex, ShaderLanguage::spirv);
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (vertex, nullptr) << name;
        ASSERT_NE (fragment, nullptr) << name;

        auto budget = countResources (fragment->reflection);
        EXPECT_EQ (countResources (vertex->reflection).sampledImages, 0) << name;
        EXPECT_EQ (countResources (vertex->reflection).uniformBlocks, 2) << name;
        EXPECT_EQ (budget.sampledImages, textures) << name;
        EXPECT_EQ (budget.uniformBlocks, 5) << name;
        EXPECT_EQ (budget.texturesPerSet[0], lookups) << name;
        EXPECT_EQ (budget.texturesPerSet[1], 0) << name;
        EXPECT_EQ (budget.texturesPerSet[3], shadowMaps) << name;
        EXPECT_EQ (budget.uniformBlocksPerSet[3], 1) << name;
    }

    // The full tier geometry bundles: the forward pass without the raster light list, and the G-buffer
    for (const auto& [name, bundle, textures, blocks, lookups] : { std::tuple { "ForwardPass_full", forwardPassFull(), 10, 4, 1 },
                                                                   std::tuple { "ForwardPass_full_area_lights", forwardPassFullAreaLights(), 12, 4, 3 },
                                                                   std::tuple { "GBufferPass_full", gbufferPassFull(), 5, 1, 0 } })
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;

        auto budget = countResources (fragment->reflection);
        EXPECT_EQ (budget.sampledImages, textures) << name;
        EXPECT_EQ (budget.uniformBlocks, blocks) << name;
        EXPECT_EQ (budget.texturesPerSet[0], lookups) << name;
    }

    const std::pair<const char*, ShaderBundle> bundles[] = {
        { "ForwardPass", forwardPass() },
        { "ForwardPass_area_lights", forwardPassAreaLights() },
        { "ForwardPass_soft_shadows", forwardPassSoftShadows() },
        { "ForwardPass_area_lights_soft_shadows", forwardPassAreaLightsSoftShadows() },
        { "ForwardPass_full", forwardPassFull() },
        { "ForwardPass_full_area_lights", forwardPassFullAreaLights() },
        { "Background", background() },
        { "GBufferPass_full", gbufferPassFull() },
        { "LightingPass_full", lightingPassFull() },
        { "LightingPass_full_area_lights", lightingPassFullAreaLights() },
        { "ShadowPass", shadowPass() },
        { "ShadowPass_masked", shadowPassMasked() },
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

TEST_F (SceneRendererBundleTests, OnlyFullVariantsWriteVelocity)
{
    const auto fragmentOf = [] (const ShaderBundle& bundle) -> const ShaderReflection*
    {
        const auto* shader = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        return shader != nullptr ? &shader->reflection : nullptr;
    };

    const auto raster = forwardPass();
    const auto rasterAreaLights = forwardPassAreaLights();
    const auto full = forwardPassFull();
    const auto fullAreaLights = forwardPassFullAreaLights();
    const auto rasterBackground = background();

    for (const auto* reflection : { fragmentOf (raster), fragmentOf (rasterAreaLights), fragmentOf (rasterBackground) })
    {
        ASSERT_NE (reflection, nullptr);
        EXPECT_TRUE (writesLocation (*reflection, 0));
        EXPECT_FALSE (writesLocation (*reflection, 1));
    }

    // Blended items keep writing their motion over the deferred surfaces
    for (const auto* reflection : { fragmentOf (full), fragmentOf (fullAreaLights) })
    {
        ASSERT_NE (reflection, nullptr);
        EXPECT_TRUE (writesLocation (*reflection, 0));
        EXPECT_TRUE (writesLocation (*reflection, 1));
    }
}

TEST_F (SceneRendererBundleTests, MaterialBlockEndsWithTheGpuMaterial)
{
    const ShaderBundle bundles[] = { forwardPass(), forwardPassAreaLights(), forwardPassFull(), forwardPassFullAreaLights() };

    for (const auto& bundle : bundles)
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr);

        const auto& blocks = fragment->reflection.uniformBuffers;
        const auto block = std::find_if (blocks.begin(), blocks.end(), [] (const auto& b) { return b.set == 2 && b.binding == 1; });
        ASSERT_NE (block, blocks.end());
        EXPECT_EQ (block->blockSize, 96u);

        std::map<String, uint32_t> offsets;
        for (const auto& member : block->members)
            offsets[member.name] = member.offset;

        EXPECT_EQ (offsets["specular"], 64u);
        EXPECT_EQ (offsets["extended"], 64u + static_cast<uint32_t> (offsetof (GpuMaterial, extended)));
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
        { "ForwardPass_full", forwardPassFull() },
        { "ForwardPass_full_area_lights", forwardPassFullAreaLights() },
        { "Background", background() },
        { "LightingPass_full", lightingPassFull() },
        { "LightingPass_full_area_lights", lightingPassFullAreaLights() },
    };

    for (const auto& [name, bundle] : bundles)
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;

        const auto& blocks = fragment->reflection.uniformBuffers;
        const auto block = std::find_if (blocks.begin(), blocks.end(), [] (const auto& b) { return b.set == 1 && b.binding == 0; });
        ASSERT_NE (block, blocks.end()) << name;
        EXPECT_EQ (block->blockSize, 208u) << name;

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
    }
}

TEST_F (SceneRendererBundleTests, ShadowBlockIsSharedByEveryShadingBundle)
{
    // One declaration in lib/shadows.glsl, mirrored by GpuShadowData: the ShadowBlock at set 3
    // binding 4, the sun cascades at set 3 binding 0 and the point and spot light atlas at set 3
    // binding 2, both compared through shadowCompare at set 0 binding 4. WebGPU needs them as depth
    // textures (an array and a 2D one) and a comparison sampler, and Metal gives every texture and
    // every sampler of a stage its own slot, whatever its set
    const std::tuple<const char*, ShaderBundle, bool> bundles[] = {
        { "ForwardPass", forwardPass(), true },
        { "ForwardPass_area_lights", forwardPassAreaLights(), true },
        { "ForwardPass_soft_shadows", forwardPassSoftShadows(), false },
        { "ForwardPass_area_lights_soft_shadows", forwardPassAreaLightsSoftShadows(), false },
        { "ForwardPass_full", forwardPassFull(), false },
        { "ForwardPass_full_area_lights", forwardPassFullAreaLights(), false },
        { "LightingPass_full", lightingPassFull(), false },
        { "LightingPass_full_area_lights", lightingPassFullAreaLights(), false },
    };

    const auto distinctSlots = [] (const std::vector<ShaderReflection::ResourceBinding>& resources)
    {
        std::set<uint32_t> slots;
        for (const auto& resource : resources)
            slots.insert (resource.backendSlot);

        return slots.size() == resources.size();
    };

    for (const auto& [name, bundle, rasterDefault] : bundles)
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
        // default raster bundles leave soft shadows to their own variants and declare neither
        for (const auto& [binding, dimension] : { std::pair { 1u, WgslBindingLayout::ViewDimension::array2D },
                                                  std::pair { 3u, WgslBindingLayout::ViewDimension::texture2D } })
        {
            const auto* raw = find (3, binding);
            EXPECT_EQ (raw != nullptr, ! rasterDefault) << name << " binding " << (int) binding;
            EXPECT_EQ (declares (fragment->reflection.separateImages, 3, binding), ! rasterDefault) << name << " binding " << (int) binding;

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
    // the light lists of the clusters at binding 7, both read only
    const auto bundle = forwardPassFull();
    const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
    ASSERT_NE (fragment, nullptr);

    const auto& blocks = fragment->reflection.uniformBuffers;
    EXPECT_TRUE (std::none_of (blocks.begin(), blocks.end(), [] (const auto& b) { return b.set == 2 && b.binding == 12; }));

    std::vector<uint32_t> bindings;
    for (const auto& buffer : fragment->reflection.storageBuffers)
    {
        EXPECT_EQ (buffer.set, 1u) << buffer.name;
        EXPECT_TRUE (buffer.readOnly) << buffer.name;
        bindings.push_back (buffer.binding);
    }

    std::sort (bindings.begin(), bindings.end());
    EXPECT_EQ (bindings, (std::vector<uint32_t> { 6, 7 }));
}

TEST_F (SceneRendererBundleTests, RasterAreaLightsCompile)
{
    // The area light variants of the shading passes read the area light tables, set 0 lookups at
    // bindings 7 and 11, through the set 0 linearClamp sampler. RasterBundlesStayWithinBudget
    // counts the two tables in the raster budget
    const std::pair<const char*, ShaderBundle> bundles[] = {
        { "ForwardPass_area_lights", forwardPassAreaLights() },
        { "ForwardPass_area_lights_soft_shadows", forwardPassAreaLightsSoftShadows() },
        { "ForwardPass_full_area_lights", forwardPassFullAreaLights() },
        { "LightingPass_full_area_lights", lightingPassFullAreaLights() },
    };

    for (const auto& [name, bundle] : bundles)
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;

        EXPECT_TRUE (declares (fragment->reflection.separateImages, 0, 7)) << name << ": ltcMatrix";
        EXPECT_TRUE (declares (fragment->reflection.separateImages, 0, 11)) << name << ": ltcAmplitude";
        EXPECT_TRUE (declares (fragment->reflection.separateSamplers, 0, 1)) << name << ": linearClamp";
        EXPECT_FALSE (declares (fragment->reflection.separateImages, 1, 4)) << name;
        EXPECT_FALSE (declares (fragment->reflection.separateImages, 1, 5)) << name;
    }
}

TEST_F (SceneRendererBundleTests, EveryShadingBundleReadsTheEnvironmentFromSetZero)
{
    // The environment is a set 0 lookup at binding 6, with its interim sampler at binding 14
    const std::pair<const char*, ShaderBundle> bundles[] = {
        { "ForwardPass", forwardPass() },
        { "ForwardPass_area_lights", forwardPassAreaLights() },
        { "ForwardPass_soft_shadows", forwardPassSoftShadows() },
        { "ForwardPass_area_lights_soft_shadows", forwardPassAreaLightsSoftShadows() },
        { "ForwardPass_full", forwardPassFull() },
        { "ForwardPass_full_area_lights", forwardPassFullAreaLights() },
        { "Background", background() },
        { "LightingPass_full", lightingPassFull() },
        { "LightingPass_full_area_lights", lightingPassFullAreaLights() },
    };

    for (const auto& [name, bundle] : bundles)
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;

        EXPECT_TRUE (declares (fragment->reflection.separateImages, 0, 6)) << name << ": environment";
        EXPECT_TRUE (declares (fragment->reflection.separateSamplers, 0, 14)) << name << ": environment sampler";
        EXPECT_FALSE (declares (fragment->reflection.separateImages, 1, 1)) << name;
        EXPECT_FALSE (declares (fragment->reflection.separateSamplers, 1, 2)) << name;
    }
}

TEST_F (SceneRendererBundleTests, PunctualVariantsDeclareNoAreaLightTables)
{
    // Without YUP_3D_AREA_LIGHTS every light is punctual: neither the tables nor their sampler
    const std::pair<const char*, ShaderBundle> bundles[] = {
        { "ForwardPass", forwardPass() },
        { "ForwardPass_soft_shadows", forwardPassSoftShadows() },
        { "ForwardPass_full", forwardPassFull() },
        { "LightingPass_full", lightingPassFull() },
    };

    for (const auto& [name, bundle] : bundles)
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr) << name;

        EXPECT_FALSE (declares (fragment->reflection.separateImages, 0, 7)) << name << ": ltcMatrix";
        EXPECT_FALSE (declares (fragment->reflection.separateImages, 0, 11)) << name << ": ltcAmplitude";
        EXPECT_FALSE (declares (fragment->reflection.separateSamplers, 0, 1)) << name << ": linearClamp";
        EXPECT_FALSE (fragment->reflection.separateSamplers.empty()) << name;
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

    /** How many frames drew each of the first @p count sun cascades. */
    static std::vector<int> getCascadeRenderCounts (const SceneRenderer& renderer, int count)
    {
        std::vector<int> counts;
        for (int c = 0; c < count; ++c)
            counts.push_back (renderer.getShadowCascadeRenderCount (c));

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

    /** Silhouettes have intermediate gray levels, which only MSAA coverage produces. */
    static void checkEdges (const Image& image)
    {
        std::set<int> levels;
        for (int y = 30; y < 100; ++y)
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto red = static_cast<int> (image.getPixelColor (x, y).getRed());
                if (red > 30 && red < 147)
                    levels.insert (red);
            }

        EXPECT_GE (levels.size(), 3u) << "edges: intermediate levels";
    }

    /** Selects the rendering tier of a scene. */
    static void setTier (Scene& scene, RenderSettings::TierPreference tier)
    {
        auto settings = scene.getRenderSettings();
        settings.tier = tier;
        scene.setRenderSettings (settings);
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
        (checkShadows), covers about the same pixels, and nothing differs outside of it. The shadow
        is what tells each image apart from the scene rendered without it.
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

        int shadowPixels = 0, baselineShadowPixels = 0, differingOutside = 0;
        for (size_t i = 0; i < shadow.size(); ++i)
        {
            shadowPixels += shadow[i] ? 1 : 0;
            baselineShadowPixels += baselineShadow[i] ? 1 : 0;
            differingOutside += differing[i] && ! shadow[i] && ! baselineShadow[i] ? 1 : 0;
        }

        RecordProperty ("shadowsDifferingFromBaseline", static_cast<int> (std::count (differing.begin(), differing.end(), true)));
        RecordProperty ("shadowPixels", shadowPixels);
        RecordProperty ("baselineShadowPixels", baselineShadowPixels);
        RecordProperty ("differingOutsideShadows", differingOutside);

        EXPECT_GT (baselineShadowPixels, 0);
        EXPECT_GE (shadowPixels, baselineShadowPixels * 4 / 5);
        EXPECT_LE (shadowPixels, baselineShadowPixels * 5 / 4);
        EXPECT_EQ (differingOutside, 0);
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

    /** Each baseline scene on the raster tier stays within 0.5% of the pixels of the Phase 0 image of
        @p platform, except the shadows scene, checked by statistics (checkShadowsAgainstBaseline()). */
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

            if (String (name) == "shadows")
            {
                checkShadowsAgainstBaseline (image, platform);
                continue;
            }

            const auto differing = countBaselineDifferences (image, platform + "_" + name + ".png");
            ASSERT_GE (differing, 0) << "capture the Phase 0 baselines first";

            RecordProperty ((String (name) + "Differing").toStdString(), differing);
            EXPECT_LE (differing, 128 * 128 / 200) << name; // At most 0.5% of the pixels beyond 2 levels
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
        EXPECT_EQ (renderer.getShadowRecordCount(), full ? 60 : 12);

#if YUP_ENABLE_SHADER_TRANSPILER
        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        ASSERT_NE (sceneColor, nullptr);
        EXPECT_EQ (sceneColor->getFormat(), GpuTextureFormat::rgba16float);
        EXPECT_TRUE (isFiniteTexture (sceneColor));
        EXPECT_FALSE (isSaturatedTexture (sceneColor));
#endif
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

    /** buildPenumbraScene() lit by a shadow casting spot light of radius @p radius instead of the
        sun, behind the wall at (0, 2.4, -2.4) and aimed at the middle of the frame: the side of the
        wall's shadow still runs along x = 0, across the rows of the frame, on @p tier at 256 x 256,
        converged, with soft shadows set to @p softShadows. */
    Image renderSpotPenumbraScene (RenderSettings::TierPreference tier, float radius, bool softShadows)
    {
        Scene scene;
        buildPenumbraScene (scene, 0.0f).intensity = 0.0f;
        setTier (scene, tier);

        auto entity = EntityNode::Ptr (new EntityNode ("spot"));
        auto& spot = entity->attach<LightNode> (LightNode::Type::spot);
        spot.intensity = 40.0f;
        spot.outerConeAngle = 0.8f;
        spot.innerConeAngle = 0.7f;
        spot.radius = radius;
        spot.castsShadows = true;
        aimCamera (*entity, { 0.0f, 2.4f, -2.4f }, { 0.0f, 0.0f, 0.9f });
        scene.getRoot()->addChild (entity);

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

    /** The shading normal of a G-buffer RT1 decoded through lib/octahedral.glsl, as n * 0.5 + 0.5 in rgb. */
    Image readGBufferNormal (const GpuTexture::Ptr& normalRoughness)
    {
        const auto octahedral = File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory()
                                    .getChildFile ("modules/yup_3d/shaders/lib/octahedral.glsl")
                                    .loadFileAsString();

        return mapTexels (normalRoughness, "vec4(octahedralDecode(t.xy) * 0.5 + 0.5, 1.0)", octahedral);
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

    /** Area lights stay finite on @p tier, through the rgba16float path on raster. */
    void checkAreaLightsStayFinite (RenderSettings::TierPreference tier)
    {
        // A smooth plate inside a sphere light, touching another, under a spot disk and a grazing sun
        // disk, over a gray ambient light so every plate pixel stays lit
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.5f, 0.5f, 0.5f, 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 0.0f;

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

        texel = renderer.getShadowData().cascadeTexel[0];
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
            const auto data = renderer.getShadowData();
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
            ASSERT_EQ (renderer.getShadowRecordCount(), cascades + 6 + (marches ? 1 : 0));
            if (marches)
            {
                const auto& contactOnly = data.records[cascades + 6];
                EXPECT_EQ (contactOnly.atlasRect[2], 0.0f);
                EXPECT_FLOAT_EQ (contactOnly.contact[0], 0.3f);
                EXPECT_FLOAT_EQ (contactOnly.contact[1], 0.1f);
            }
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

    /** The linear radiance behind an 8 bit level that was Reinhard tone mapped and sRGB encoded. */
    static double reinhardRadiance (int level)
    {
        const auto encoded = level / 255.0;
        const auto mapped = encoded <= 0.04045 ? encoded / 12.92 : std::pow ((encoded + 0.055) / 1.055, 2.4);
        return mapped / (1.0 - mapped);
    }

    /** A rough (roughness 1) plate drawn with @p material and lit head-on by a white sun of
        intensity @p sunIntensity, under @p ambient light: the mean linear radiance of the 8x8
        pixels at the center, where the normal, the view and the light directions coincide. There
        D = 1 / pi, V = 1 / 4 and F = f0, so the sun adds 2 (f0 / 4 + diffuseColor * weight) / pi
        at the default intensity of 2.
    */
    std::array<double, 3> renderHeadOnRadiance (Material::Ptr material, RenderSettings::TierPreference tier,
                                                float sunIntensity = 2.0f, Color ambient = Colors::black)
    {
        material->roughnessFactor = 1.0f;

        Scene scene;
        addPlate (scene, std::move (material));
        scene.setAmbientColor (ambient);
        scene.setToneMapping (Scene::ToneMapping::reinhard);
        setTier (scene, tier);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().intensity = sunIntensity;
        scene.getRoot()->addChild (sun);

        SceneRenderer renderer;
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
                EXPECT_EQ (renderer.getShadowData().sunInfo[0], 2.0f) << name;
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

        const auto shadowData = renderer.getShadowData();
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
#endif

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

TEST_P (SceneRendererTierTests, RendersWithoutTheAreaLightTables)
{
    // When the area light tables cannot be created, frames still render: area lights lose their
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
            renderer.failAreaLightTablesForTesting();

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
}

TEST_P (SceneRendererTierTests, RendersWithoutTheShadowCompareSampler)
{
    // A device unable to create the comparison sampler cannot read shadow maps: it renders every
    // frame, without the shadows, as if the sun cast none
    const auto full = GetParam() == RenderSettings::TierPreference::full;

    const auto render = [&] (bool withSampler, bool castsShadows)
    {
        Scene scene;
        buildShadowsScene (scene);
        scene.getRoot()->findChild ("sun")->getNode<LightNode>()->castsShadows = castsShadows;

        if (full)
            setFullTierTaa (scene, 1);
        else
            setTier (scene, RenderSettings::TierPreference::raster);

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

    EXPECT_EQ (renderer.getShadowMapRenderCount(), 1);

    auto* caster = scene.getRoot()->findChild ("caster");
    ASSERT_NE (caster, nullptr);
    caster->setPosition (caster->getPosition() + Vector3<float> (0.05f, 0.0f, 0.0f));

    for (int frame = 0; frame < 2; ++frame)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_EQ (renderer.getShadowMapRenderCount(), 2);

    renderer.restartAccumulation();
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getShadowMapRenderCount(), 3);
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

    EXPECT_EQ (renderer.getShadowMapRenderCount(), 1);
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

    EXPECT_EQ (renderer.getShadowMapRenderCount(), 1);

    material->alphaCutoff = 0.9f;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getShadowMapRenderCount(), 2);

    material->baseColorFactor[3] = 1.0f;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getShadowMapRenderCount(), 3);
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
    EXPECT_EQ (renderer.getShadowMapRenderCount(), 1);

    // Frames complete in order, so once a later frame did the failed one has too
    {
        auto fence = GpuFrame::begin (device);
        EXPECT_TRUE (fence.waitForGPU().wasOk());
    }

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getLastGpuFailure(), "injected failure");
    EXPECT_EQ (renderer.getShadowMapRenderCount(), 2);

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getShadowMapRenderCount(), 2);
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
        EXPECT_EQ (renderer.getShadowRecordCount(), 1) << "frame " << frame;
    }

    // Zooming out and panning keep the map too
    aimCamera (*camera, { 6.5f, 5.2f, 0.0f }, {});
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    aimCamera (*camera, { 6.5f, 5.2f, 0.3f }, { 0.0f, 0.0f, 0.3f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getShadowRecordCount(), 1);
    EXPECT_EQ (renderer.getShadowMapRenderCount(), 1);

    // Moving the box draws it again
    box.setPosition ({ 0.1f, 0.3f, 0.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getShadowMapRenderCount(), 2);
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
    EXPECT_EQ (renderer.getShadowRecordCount(), cascades);
    EXPECT_EQ (renderer.getShadowMapRenderCount(), 1);

    auto* camera = scene.getRoot()->findChild ("camera");
    ASSERT_NE (camera, nullptr);
    camera->setPosition (camera->getPosition() + Vector3<float> (0.5f, 0.0f, 0.0f));

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getShadowRecordCount(), cascades);
    EXPECT_EQ (renderer.getShadowMapRenderCount(), 2);
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

    const auto cascades = renderer.getShadowRecordCount();
    ASSERT_GT (cascades, 1);

    auto expected = getCascadeRenderCounts (renderer, cascades);
    EXPECT_EQ (expected, std::vector<int> (static_cast<size_t> (cascades), 1));

    farCaster.setPosition (farCaster.getPosition() + Vector3<float> (0.5f, 0.0f, 0.0f));
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    ++expected.back();
    EXPECT_EQ (getCascadeRenderCounts (renderer, cascades), expected);
    EXPECT_EQ (renderer.getShadowMapRenderCount(), 2);
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

    const auto cascades = renderer.getShadowRecordCount();
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

    const auto cascades = renderer.getShadowRecordCount();
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
    ASSERT_GT (renderer.getShadowAtlasSize(), 0);

    const auto cascades = static_cast<int> (renderer.getShadowData().sunInfo[0]);
    ASSERT_GT (cascades, 1);
    const auto counts = getCascadeRenderCounts (renderer, cascades);

    lamp->setPosition ({ 1.2f, 1.0f, -2.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getShadowMapRenderCount(), 2);
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

        const auto shadows = renderer.getShadowData();
        EXPECT_EQ (shadows.sunInfo[0], 1.0f);
        EXPECT_EQ (shadows.sunInfo[3], 0.0f);
    }

    EXPECT_EQ (renderer.getShadowMapRenderCount(), 1);
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
        EXPECT_GT (fresh.getShadowData().sunInfo[0], 1.0f);
        EXPECT_TRUE (fresh.waitForGPU().wasOk());
    }

    SceneRenderer renderer;
    for (int frame = 0; frame < 6; ++frame)
    {
        aimCamera (*camera, frame % 2 == 0 ? farEye : nearEye, {});
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_EQ (renderer.getShadowData().sunInfo[0], 1.0f) << "frame " << frame;
    }

    EXPECT_EQ (renderer.getShadowMapRenderCount(), 1);
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
    EXPECT_EQ (renderer.getShadowAtlasSize(), 2048);
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
        return renderer.getShadowTileSize (0);
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
    // A shadowed point light whose range reaches no receiver takes no tile, and the frame is the
    // one it gives without shadows; brought within reach of the ground it takes its six faces
    const auto render = [this] (Vector3<float> position, bool castsShadows, int& records, int& atlasSize)
    {
        Scene scene;
        buildSpotOverGround (scene).castsShadows = false;
        setTier (scene, GetParam());

        auto entity = EntityNode::Ptr (new EntityNode ("point"));
        auto& light = entity->attach<LightNode> (LightNode::Type::point);
        light.range = 1.0f;
        light.castsShadows = castsShadows;
        entity->setPosition (position);
        scene.getRoot()->addChild (entity);

        SceneRenderer renderer;
        const auto image = renderConverged (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        records = renderer.getShadowRecordCount();
        atlasSize = renderer.getShadowAtlasSize();
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

    EXPECT_EQ (renderer.getShadowMapRenderCount(), 1);

    auto* entity = scene.getRoot()->findChild ("spot");
    ASSERT_NE (entity, nullptr);
    entity->setPosition (entity->getPosition() + Vector3<float> (0.05f, 0.0f, 0.0f));

    for (int frame = 0; frame < 2; ++frame)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_EQ (renderer.getShadowMapRenderCount(), 2);

    spot.range = 8.0f;
    for (int frame = 0; frame < 2; ++frame)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_EQ (renderer.getShadowMapRenderCount(), 3);
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

    const auto data = renderer.getShadowData();
    const auto last = renderer.getShadowRecordCount() - 1;
    ASSERT_GT (last, 0);
    EXPECT_EQ (data.records[last].atlasRect[2], 0.0f);
    EXPECT_FLOAT_EQ (data.records[last].contact[0], 0.2f);

    EXPECT_GT (renderer.getShadowTileSize (1), 0);
    EXPECT_EQ (renderer.getShadowTileSize (2), 0);
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

    const auto data = renderer.getShadowData();
    ASSERT_EQ (renderer.getShadowRecordCount(), GpuShadowData::maxRecords);
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
        GTEST_SKIP() << "Raster only: the full tier always holds the soft shadow code";

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
    const auto masked = [] (float alpha)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.2f, 0.8f, 0.2f, alpha };
        material->alphaMode = Material::AlphaMode::mask;
        return material;
    };

    // Below and above the default 0.5 cutoff, centered at about x = 23 and x = 41
    const auto cutOut = masked (0.2f);
    const auto kept = masked (0.8f);

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
#endif

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
        Vector3<float> modelPosition;
        float cameraDistance = 0.0f;
        bool moveModel = false;
        float orbitPitch = 0.0f; // the camera orbits at this pitch when not 0
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
        @p shadowRedraws receives the number of the timed frames that drew shadow maps. */
    double timeFrames (LightShow& show, int width, int height, int& shadowRedraws)
    {
        SceneRenderer renderer;

        const auto renderFrames = [&] (int count)
        {
            for (int i = 0; i < count; ++i)
            {
                show.animate();
                if (renderer.render (device, show.scene, width, height) == nullptr)
                    return false;
            }

            return renderer.waitForGPU().wasOk();
        };

        if (! renderFrames (30))
            return -1.0;

        const auto redrawsBefore = renderer.getShadowMapRenderCount();
        const auto start = Time::getMillisecondCounterHiRes();
        if (! renderFrames (120))
            return -1.0;

        shadowRedraws = renderer.getShadowMapRenderCount() - redrawsBefore;
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

TEST_F (SceneRendererGLTests, ShadowedRegionIsFinite)
{
    checkShadowedRegionIsFinite (RenderSettings::TierPreference::raster);
}
#endif

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

TEST_F (SceneRendererGLTests, SoftPointShadowHasNoFaceSeams)
{
    checkPointShadowHasNoFaceSeams (RenderSettings::TierPreference::raster, 0.1f);
}

TEST_F (SceneRendererGLTests, SoftSunLeavesAGrazedGroundLit)
{
    checkSoftSunLeavesAGrazedGroundLit (RenderSettings::TierPreference::raster);
}
#endif
