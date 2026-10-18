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
#include "yup_renderer_shaders/yup_LightingPass_tier_full.inc"
        };

        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassAreaLights()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_area_lights.inc"
        };

        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassSoftShadows()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_soft_shadows.inc"
        };

        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassAreaLightsSoftShadows()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_area_lights_soft_shadows.inc"
        };

        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_local_indirect.inc"
        };

        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassAreaLightsLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_area_lights_local_indirect.inc"
        };

        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassSoftShadowsLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_soft_shadows_local_indirect.inc"
        };

        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassAreaLightsSoftShadowsLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_area_lights_soft_shadows_local_indirect.inc"
        };

        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_local_indirect_layers.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassAreaLightsLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_area_lights_local_indirect_layers.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassSoftShadowsLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_soft_shadows_local_indirect_layers.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingPassAreaLightsSoftShadowsLocalIndirectLayers()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_area_lights_soft_shadows_local_indirect_layers.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingClassify()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingClassify_tier_full.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle lightingClassifyLocalIndirect()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_LightingClassify_tier_full_local_indirect.inc"
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

    // Set 0 holds the shared lookups: the environment cube, in every variant the lookup tables, and
    // in the local indirect variants (preflight R27) the reflection atlas (8) and the irradiance and
    // visibility atlases (9, 10), all read through linearClamp, and the shadow comparison sampler
    // (bindings 11 and 14 are spare). Set 1 holds RT0-RT2 and the depth, keeping their binding
    // numbers, then the ambient occlusion (13) and the screen-space reflections (16) in every
    // variant, and set 3 the sun cascades and the point and spot light atlas, each compared (0, 2),
    // and read raw (1, 3) by the shadow filter variants only. The storage buffers are the lights, the light lists of the clusters and the
    // material table, plus the reflection probes (14) and the irradiance volumes (15) in the local
    // indirect variants only
    const Bindings lookups { { 0, 6 }, { 0, 7 } };
    const Bindings probeAtlases { { 0, 8 }, { 0, 9 }, { 0, 10 } };
    const Bindings passInputs { { 1, 9 }, { 1, 10 }, { 1, 11 }, { 1, 12 }, { 1, 13 }, { 1, 16 } };
    const Bindings compared { { 3, 0 }, { 3, 2 } };
    const Bindings filtered { { 3, 0 }, { 3, 1 }, { 3, 2 }, { 3, 3 } };
    const Bindings storage { { 1, 6 }, { 1, 7 }, { 1, 8 } };
    const Bindings probeStorage { { 1, 6 }, { 1, 7 }, { 1, 8 }, { 1, 14 }, { 1, 15 } };

    const auto join = [] (std::initializer_list<Bindings> parts)
    {
        Bindings joined;
        for (const auto& part : parts)
            joined.insert (joined.end(), part.begin(), part.end());

        std::sort (joined.begin(), joined.end());
        return joined;
    };

    const struct
    {
        const char* name;
        ShaderBundle bundle;
        Bindings images;
        Bindings buffers;
    } variants[] = {
        { "punctual", lightingPass(), join ({ lookups, passInputs, compared }), storage },
        { "area lights", lightingPassAreaLights(), join ({ lookups, passInputs, compared }), storage },
        { "shadow filters", lightingPassSoftShadows(), join ({ lookups, passInputs, filtered }), storage },
        { "area lights, shadow filters", lightingPassAreaLightsSoftShadows(), join ({ lookups, passInputs, filtered }), storage },
        { "local indirect", lightingPassLocalIndirect(), join ({ lookups, probeAtlases, passInputs, compared }), probeStorage },
        { "area lights, local indirect", lightingPassAreaLightsLocalIndirect(), join ({ lookups, probeAtlases, passInputs, compared }), probeStorage },
        { "shadow filters, local indirect", lightingPassSoftShadowsLocalIndirect(), join ({ lookups, probeAtlases, passInputs, filtered }), probeStorage },
        { "area lights, shadow filters, local indirect", lightingPassAreaLightsSoftShadowsLocalIndirect(), join ({ lookups, probeAtlases, passInputs, filtered }), probeStorage },
        { "local indirect, layers", lightingPassLocalIndirectLayers(), join ({ lookups, probeAtlases, passInputs, compared }), probeStorage },
        { "area lights, local indirect, layers", lightingPassAreaLightsLocalIndirectLayers(), join ({ lookups, probeAtlases, passInputs, compared }), probeStorage },
        { "shadow filters, local indirect, layers", lightingPassSoftShadowsLocalIndirectLayers(), join ({ lookups, probeAtlases, passInputs, filtered }), probeStorage },
        { "area lights, shadow filters, local indirect, layers", lightingPassAreaLightsSoftShadowsLocalIndirectLayers(), join ({ lookups, probeAtlases, passInputs, filtered }), probeStorage },
    };

    for (const auto& variant : variants)
    {
        const auto* fragment = fragmentOf (variant.bundle);
        ASSERT_NE (fragment, nullptr) << variant.name;

        // The ViewBlock, the LightingBlock and the ShadowBlock
        EXPECT_EQ (bindingsOf (fragment->uniformBuffers), (Bindings { { 0, 0 }, { 1, 0 }, { 3, 4 } })) << variant.name;

        EXPECT_EQ (bindingsOf (fragment->separateImages), variant.images) << variant.name;
        EXPECT_EQ (bindingsOf (fragment->separateSamplers), (Bindings { { 0, 1 }, { 0, 3 }, { 0, 4 } })) << variant.name;
        EXPECT_TRUE (fragment->sampledImages.empty()) << variant.name;

        // Each set stays within the 8 textures an ore bind group holds
        std::map<uint32_t, int> texturesPerSet;
        for (const auto& image : fragment->separateImages)
            ++texturesPerSet[image.set];

        EXPECT_LE (texturesPerSet[0], 8) << variant.name;
        EXPECT_LE (texturesPerSet[1], 8) << variant.name;
        EXPECT_LE (texturesPerSet[3], 8) << variant.name;

        // Read only
        EXPECT_EQ (bindingsOf (fragment->storageBuffers), variant.buffers) << variant.name;
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

TEST_F (LightingPassTests, ClassificationReadsTheLightingBindings)
{
    // LightingClassify reads, at the lighting pass's own bindings, which a classified frame binds
    // once for it and the variants: RT2 and the depth through pointClamp and the material table,
    // and in its local indirect variant the view and lighting blocks with the probes and volumes
    using Bindings = std::vector<std::pair<uint32_t, uint32_t>>;

    const struct
    {
        const char* name;
        ShaderBundle bundle;
        Bindings blocks;
        Bindings buffers;
    } variants[] = {
        { "default", lightingClassify(), {}, { { 1, 8 } } },
        { "local indirect", lightingClassifyLocalIndirect(), { { 0, 0 }, { 1, 0 } }, { { 1, 8 }, { 1, 14 }, { 1, 15 } } },
    };

    for (const auto& variant : variants)
    {
        const auto* fragment = fragmentOf (variant.bundle);
        ASSERT_NE (fragment, nullptr) << variant.name;

        EXPECT_EQ (bindingsOf (fragment->uniformBuffers), variant.blocks) << variant.name;
        EXPECT_EQ (bindingsOf (fragment->separateImages), (Bindings { { 1, 11 }, { 1, 12 } })) << variant.name;
        EXPECT_EQ (bindingsOf (fragment->separateSamplers), (Bindings { { 0, 3 } })) << variant.name;
        EXPECT_EQ (bindingsOf (fragment->storageBuffers), variant.buffers) << variant.name;
        for (const auto& buffer : fragment->storageBuffers)
            EXPECT_TRUE (buffer.readOnly) << variant.name << ": " << buffer.name;
    }
}
