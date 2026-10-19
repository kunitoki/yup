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

/** LightingPass recording on a mocked device: the variant each frame picks, the fallbacks and the
    classification. The shaded pixels are tested on Metal, through the scene renderer.
*/
class LightingPassMockTests : public RendererMockTests
{
protected:
    void SetUp() override
    {
        RendererMockTests::SetUp();
        resources = std::make_unique<FrameResources> (device);
        resources->beginFrame (64, 64);

        sceneColor = makeTarget (64, 64);
        gbuffer.albedoOcclusion = makeTarget (64, 64, GpuTextureFormat::rgba8unorm);
        gbuffer.normalRoughness = makeTarget (64, 64);
        gbuffer.emissionMaterial = makeTarget (64, 64);
        gbuffer.velocity = makeTarget (64, 64, GpuTextureFormat::rg16float);
        gbuffer.depth = makeTarget (64, 64, GpuTextureFormat::depth32float);

        ASSERT_NE (resources->getSamplers(), nullptr);
        lighting.samplers = *resources->getSamplers();
        lighting.lookupTables = makeTarget (4, 4, GpuTextureFormat::rgba8unorm);
        lighting.shadowCascades = makeTarget (16, 16, GpuTextureFormat::depth32float);
        lighting.shadowAtlas = makeTarget (16, 16, GpuTextureFormat::depth32float);
        lighting.lights = makeStorage();
        lighting.clusterLights = makeStorage();
        lighting.materials = makeStorage();
        lighting.reflectionProbeBuffer = makeStorage();
        lighting.irradianceVolumeBuffer = makeStorage();
    }

    bool renderFrame (LightingPass& pass)
    {
        auto frame = GpuFrame::begin (device);
        const auto rendered = pass.render (device, frame, view, gbuffer, lighting, sceneColor);
        frame.submit();
        return rendered;
    }

    void setProbes (bool probes) { lighting.block.probeInfo[1] = probes ? 1.0f : 0.0f; }

    std::vector<float> lastViewportDepths() const { return passes.empty() ? std::vector<float> {} : passes.back().viewportDepths; }

    ViewData view;
    std::unique_ptr<FrameResources> resources;
    GBufferPass::Targets gbuffer;
    LightingInputs lighting;
    GpuTexture::Ptr sceneColor;
};

TEST_F (LightingPassMockTests, PreparesEveryVariantOnce)
{
    LightingPass pass;

    for (const auto layers : { false, true })
        for (const auto localIndirect : { false, true })
            for (const auto shadowFilters : { false, true })
                for (const auto areaLights : { false, true })
                    EXPECT_TRUE (pass.prepare (device, areaLights, shadowFilters, localIndirect, layers).wasOk());

    // The layers variants are local indirect variants already
    EXPECT_EQ (pipelinesMade, 12);

    EXPECT_TRUE (pass.prepare (device, true, true, true, true).wasOk());
    EXPECT_EQ (pipelinesMade, 12);
}

TEST_F (LightingPassMockTests, RemembersAMissingStandIn)
{
    LightingPass pass;
    failTextures = true;

    auto result = pass.prepare (device, false, false);
    ASSERT_TRUE (result.failed());
    EXPECT_EQ (result.getErrorMessage(), "Unable to create the lighting pass textures");
    EXPECT_EQ (pass.getReflectionsStandIn(), nullptr);

    failTextures = false;
    EXPECT_TRUE (pass.prepare (device, false, false).failed());
    EXPECT_EQ (pipelinesMade, 0);
}

TEST_F (LightingPassMockTests, PrepareCreatesTheReflectionsStandIn)
{
    LightingPass pass;
    ASSERT_TRUE (pass.prepare (device, false, false).wasOk());
    EXPECT_NE (pass.getReflectionsStandIn(), nullptr);
}

TEST_F (LightingPassMockTests, RejectsAMissingInput)
{
    LightingPass pass;

    for (auto missing : { &GBufferPass::Targets::albedoOcclusion, &GBufferPass::Targets::normalRoughness,
                          &GBufferPass::Targets::emissionMaterial, &GBufferPass::Targets::depth })
    {
        const auto kept = std::exchange (gbuffer.*missing, nullptr);
        EXPECT_FALSE (renderFrame (pass));
        gbuffer.*missing = kept;
    }

    for (auto missing : { &LightingInputs::lookupTables, &LightingInputs::shadowCascades, &LightingInputs::shadowAtlas })
    {
        const auto kept = std::exchange (lighting.*missing, nullptr);
        EXPECT_FALSE (renderFrame (pass));
        lighting.*missing = kept;
    }

    for (auto missing : { &LightingInputs::lights, &LightingInputs::clusterLights, &LightingInputs::materials })
    {
        const auto kept = std::exchange (lighting.*missing, nullptr);
        EXPECT_FALSE (renderFrame (pass));
        lighting.*missing = kept;
    }

    const auto linearClamp = std::exchange (lighting.samplers.linearClamp, nullptr);
    EXPECT_FALSE (renderFrame (pass));
    lighting.samplers.linearClamp = linearClamp;

    const auto kept = std::exchange (sceneColor, nullptr);
    EXPECT_FALSE (renderFrame (pass));
    sceneColor = kept;

    // The probe buffers are needed by the local indirect variant only
    lighting.reflectionProbeBuffer = nullptr;
    EXPECT_TRUE (renderFrame (pass));

    setProbes (true);
    EXPECT_FALSE (renderFrame (pass));
    EXPECT_EQ (passes.size(), 1u);
}

TEST_F (LightingPassMockTests, ShadesEveryPixelWithOneVariantWithoutProbes)
{
    LightingPass pass;
    lighting.shadowFilters = true;
    lighting.areaLights = true;

    ASSERT_TRUE (renderFrame (pass));
    EXPECT_FALSE (pass.usesLocalIndirect());
    EXPECT_FALSE (pass.usesMaterialLayers());
    EXPECT_FALSE (pass.usesClassification());
    EXPECT_FALSE (pass.holdsClassDepth());

    ASSERT_EQ (passes.size(), 1u);
    EXPECT_EQ (passes[0].colorCount, 1u);
    EXPECT_FALSE (passes[0].hasDepth);
    EXPECT_EQ (passes[0].draws, 1);
}

TEST_F (LightingPassMockTests, ClassifiesTheProbePixels)
{
    LightingPass pass;
    setProbes (true);

    ASSERT_TRUE (renderFrame (pass));
    EXPECT_TRUE (pass.usesLocalIndirect());
    EXPECT_TRUE (pass.usesClassification());
    EXPECT_TRUE (pass.holdsClassDepth());

    // The classification over the whole depth range, then the plain and the local indirect pixels
    ASSERT_EQ (passes.size(), 1u);
    EXPECT_TRUE (passes[0].hasDepth);
    EXPECT_EQ (passes[0].draws, 3);
    EXPECT_EQ (passes[0].countDistinctPipelines(), 3u);
    EXPECT_EQ (lastViewportDepths(), (std::vector<float> { 0.0f, 0.25f, 0.5f }));
}

TEST_F (LightingPassMockTests, ClassifiesTheLayeredPixels)
{
    LightingPass pass;
    lighting.materialLayers = true;

    ASSERT_TRUE (renderFrame (pass));
    EXPECT_TRUE (pass.usesLocalIndirect());
    EXPECT_TRUE (pass.usesMaterialLayers());
    EXPECT_TRUE (pass.usesClassification());

    ASSERT_EQ (passes.size(), 1u);
    EXPECT_EQ (passes[0].draws, 3);
    EXPECT_EQ (lastViewportDepths(), (std::vector<float> { 0.0f, 0.25f, 0.75f }));
}

TEST_F (LightingPassMockTests, ClassifiesTheProbeAndTheLayeredPixels)
{
    LightingPass pass;
    setProbes (true);
    lighting.materialLayers = true;

    ASSERT_TRUE (renderFrame (pass));

    ASSERT_EQ (passes.size(), 1u);
    EXPECT_EQ (passes[0].draws, 4);
    EXPECT_EQ (passes[0].countDistinctPipelines(), 4u);
    EXPECT_EQ (lastViewportDepths(), (std::vector<float> { 0.0f, 0.25f, 0.5f, 0.75f }));
}

TEST_F (LightingPassMockTests, ShadesWithOneVariantWithoutTheClassification)
{
    for (const auto failing : { false, true })
    {
        passes.clear();

        LightingPass pass;
        if (failing)
            pass.failClassificationForTesting();
        else
            pass.disableClassificationForTesting (true);

        setProbes (true);

        ASSERT_TRUE (renderFrame (pass)) << failing;
        EXPECT_TRUE (pass.usesLocalIndirect());
        EXPECT_FALSE (pass.usesClassification());
        EXPECT_FALSE (pass.holdsClassDepth());

        ASSERT_EQ (passes.size(), 1u);
        EXPECT_FALSE (passes[0].hasDepth);
        EXPECT_EQ (passes[0].draws, 1);
    }
}

TEST_F (LightingPassMockTests, FallsBackWithoutTheLayersVariant)
{
    LightingPass pass;
    pass.failMaterialLayersForTesting();
    setProbes (true);
    lighting.materialLayers = true;

    ASSERT_TRUE (renderFrame (pass));
    EXPECT_TRUE (pass.usesLocalIndirect());
    EXPECT_FALSE (pass.usesMaterialLayers());

    // The layered pixels still have their class, shaded by the local indirect variant
    ASSERT_EQ (passes.size(), 1u);
    EXPECT_EQ (passes[0].draws, 4);
    EXPECT_EQ (passes[0].countDistinctPipelines(), 3u);
}

TEST_F (LightingPassMockTests, FallsBackWithoutTheLocalIndirectVariant)
{
    LightingPass pass;
    pass.failLocalIndirectForTesting();
    setProbes (true);
    lighting.materialLayers = true;

    ASSERT_TRUE (renderFrame (pass));
    EXPECT_FALSE (pass.usesLocalIndirect());
    EXPECT_FALSE (pass.usesMaterialLayers());
    EXPECT_FALSE (pass.usesClassification());

    ASSERT_EQ (passes.size(), 1u);
    EXPECT_EQ (passes[0].draws, 1);
}

TEST_F (LightingPassMockTests, FailsWhenNoVariantCompiles)
{
    LightingPass pass;
    failPipelines = true;
    lighting.shadowFilters = true;

    EXPECT_FALSE (renderFrame (pass));
    EXPECT_TRUE (passes.empty());
}

TEST_F (LightingPassMockTests, TrimsTheClassDepthOnceUnused)
{
    LightingPass pass;
    setProbes (true);

    ASSERT_TRUE (renderFrame (pass));
    pass.trimUnusedTargets();
    EXPECT_TRUE (pass.holdsClassDepth());

    // A frame without probes leaves it unused
    setProbes (false);
    ASSERT_TRUE (renderFrame (pass));
    pass.trimUnusedTargets();
    EXPECT_FALSE (pass.holdsClassDepth());

    setProbes (true);
    ASSERT_TRUE (renderFrame (pass));
    EXPECT_TRUE (pass.holdsClassDepth());

    pass.releaseTargets();
    EXPECT_FALSE (pass.holdsClassDepth());

    ASSERT_TRUE (renderFrame (pass));
    EXPECT_TRUE (pass.holdsClassDepth());
}

TEST_F (LightingPassMockTests, RecreatesTheClassDepthAtANewSize)
{
    LightingPass pass;
    setProbes (true);
    ASSERT_TRUE (renderFrame (pass));

    sceneColor = makeTarget (32, 32);
    ASSERT_TRUE (renderFrame (pass));
    EXPECT_TRUE (pass.holdsClassDepth());

    // The class depth of the new size cannot be created
    sceneColor = makeTarget (48, 48);
    failTextures = true;
    EXPECT_FALSE (renderFrame (pass));
    EXPECT_FALSE (pass.holdsClassDepth());
}
