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

#include <algorithm>
#include <map>
#include <vector>

using namespace yup;

/** The LightingPass bundle against the set 1 binding table of the contract. The GPU behavior is
    tested through the SceneRenderer, in yup_SceneRenderer.cpp. */
class LightingPassTests : public ::testing::Test
{
protected:
    static const ShaderReflection* fragmentOf (const ShaderBundle& bundle)
    {
        const auto* shader = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        return shader != nullptr ? &shader->reflection : nullptr;
    }

    static ShaderBundle load (const uint8* data, size_t size)
    {
        auto bundle = ShaderBundle::loadFromData (data, size);
        EXPECT_TRUE (bundle.wasOk()) << bundle.getErrorMessage();
        return bundle.wasOk() ? std::move (bundle.getReference()) : ShaderBundle();
    }

    static ShaderBundle lightingPass()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_LightingPass_full.inc>
        };

        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassAreaLights()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_LightingPass_full_area_lights.inc>
        };

        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassSoftShadows()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_LightingPass_full_soft_shadows.inc>
        };

        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassAreaLightsSoftShadows()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_LightingPass_full_area_lights_soft_shadows.inc>
        };

        return load (data, sizeof (data));
    }

    /** The (set, binding) pairs of @p resources, sorted. */
    static std::vector<std::pair<uint32_t, uint32_t>> bindingsOf (const std::vector<ShaderReflection::ResourceBinding>& resources)
    {
        std::vector<std::pair<uint32_t, uint32_t>> bindings;
        for (const auto& resource : resources)
            bindings.emplace_back (resource.set, resource.binding);

        std::sort (bindings.begin(), bindings.end());
        return bindings;
    }
};

TEST_F (LightingPassTests, BindsTheContractSetOneLayout)
{
    using Bindings = std::vector<std::pair<uint32_t, uint32_t>>;

    // Set 0 holds the shared lookups: the environment cube, in every variant the lookup tables and
    // the reflection atlas (8), all read through linearClamp, and the shadow comparison sampler
    // (bindings 11 and 14 are spare). Set 1 holds RT0-RT2 and the depth, keeping their binding
    // numbers, then the ambient occlusion (13), and set 3 the sun cascades and the point and spot
    // light atlas, each compared (0, 2), and read raw (1, 3) by the shadow filter variants only
    const struct
    {
        const char* name;
        ShaderBundle bundle;
        Bindings images;
        Bindings samplers;
    } variants[] = {
        { "punctual", lightingPass(), { { 0, 6 }, { 0, 7 }, { 0, 8 }, { 1, 9 }, { 1, 10 }, { 1, 11 }, { 1, 12 }, { 1, 13 }, { 3, 0 }, { 3, 2 } }, { { 0, 1 }, { 0, 3 }, { 0, 4 } } },
        { "area lights", lightingPassAreaLights(), { { 0, 6 }, { 0, 7 }, { 0, 8 }, { 1, 9 }, { 1, 10 }, { 1, 11 }, { 1, 12 }, { 1, 13 }, { 3, 0 }, { 3, 2 } }, { { 0, 1 }, { 0, 3 }, { 0, 4 } } },
        { "shadow filters", lightingPassSoftShadows(), { { 0, 6 }, { 0, 7 }, { 0, 8 }, { 1, 9 }, { 1, 10 }, { 1, 11 }, { 1, 12 }, { 1, 13 }, { 3, 0 }, { 3, 1 }, { 3, 2 }, { 3, 3 } }, { { 0, 1 }, { 0, 3 }, { 0, 4 } } },
        { "area lights, shadow filters", lightingPassAreaLightsSoftShadows(), { { 0, 6 }, { 0, 7 }, { 0, 8 }, { 1, 9 }, { 1, 10 }, { 1, 11 }, { 1, 12 }, { 1, 13 }, { 3, 0 }, { 3, 1 }, { 3, 2 }, { 3, 3 } }, { { 0, 1 }, { 0, 3 }, { 0, 4 } } },
    };

    for (const auto& variant : variants)
    {
        const auto* fragment = fragmentOf (variant.bundle);
        ASSERT_NE (fragment, nullptr) << variant.name;

        // The ViewBlock, the LightingBlock and the ShadowBlock
        EXPECT_EQ (bindingsOf (fragment->uniformBuffers), (Bindings { { 0, 0 }, { 1, 0 }, { 3, 4 } })) << variant.name;

        EXPECT_EQ (bindingsOf (fragment->separateImages), variant.images) << variant.name;
        EXPECT_EQ (bindingsOf (fragment->separateSamplers), variant.samplers) << variant.name;
        EXPECT_TRUE (fragment->sampledImages.empty()) << variant.name;

        // Each set stays within the 8 textures an ore bind group holds
        std::map<uint32_t, int> texturesPerSet;
        for (const auto& image : fragment->separateImages)
            ++texturesPerSet[image.set];

        EXPECT_LE (texturesPerSet[0], 8) << variant.name;
        EXPECT_LE (texturesPerSet[1], 8) << variant.name;
        EXPECT_LE (texturesPerSet[3], 8) << variant.name;

        // The lights, the light lists of the clusters, the material table and the reflection probes, read only
        EXPECT_EQ (bindingsOf (fragment->storageBuffers), (Bindings { { 1, 6 }, { 1, 7 }, { 1, 8 }, { 1, 14 } })) << variant.name;
        for (const auto& buffer : fragment->storageBuffers)
            EXPECT_TRUE (buffer.readOnly) << variant.name << ": " << buffer.name;
    }
}

TEST_F (LightingPassTests, MaterialTableEntriesMirrorGpuMaterial)
{
    const auto bundle = lightingPass();
    const auto* fragment = fragmentOf (bundle);
    ASSERT_NE (fragment, nullptr);

    const auto& buffers = fragment->storageBuffers;
    const auto materials = std::find_if (buffers.begin(), buffers.end(), [] (const auto& b) { return b.binding == 8; });
    ASSERT_NE (materials, buffers.end());
    ASSERT_EQ (materials->members.size(), 1u);
    EXPECT_EQ (materials->members[0].arrayStride, static_cast<uint32_t> (sizeof (GpuMaterial)));
}

TEST_F (LightingPassTests, WritesOnlyTheSceneColor)
{
    const auto bundle = lightingPass();
    const auto* fragment = fragmentOf (bundle);
    ASSERT_NE (fragment, nullptr);

    ASSERT_EQ (fragment->stageOutputs.size(), 1u);
    EXPECT_EQ (fragment->stageOutputs[0].location, 0u);
}
