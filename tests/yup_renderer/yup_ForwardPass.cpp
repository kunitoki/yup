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
#include <optional>
#include <utility>
#include <vector>

using namespace yup;

/** ForwardPass recording on a mocked device: the pipelines of the material shaders, which the pass
    compiles, keeps beside its own and lets go. What they draw is tested on Metal, in
    native/yup_MaterialShader_apple.cpp.
*/
class ForwardPassMockTests : public RendererMockTests
{
protected:
    void SetUp() override
    {
        using ::testing::_;
        using ::testing::Invoke;

        RendererMockTests::SetUp();

        // As the fixture makes them, keeping each one to tell who else holds it
        ON_CALL (*oreCtx, makePipeline (_, _)).WillByDefault (Invoke ([this] (const rive::ore::PipelineDesc&, std::string* error) -> rive::rcp<rive::ore::Pipeline>
        {
            if (failPipelines)
            {
                if (error != nullptr)
                    *error = "failed for testing";

                return nullptr;
            }

            ++pipelinesMade;
            madePipelines.push_back (rive::rcp<rive::ore::Pipeline> (rive::make_rcp<TestOrePipeline>()));
            return madePipelines.back();
        }));

        resources = std::make_unique<FrameResources> (device);
        resources->beginFrame (64, 64);

        ASSERT_NE (resources->getSamplers(), nullptr);
        lighting.samplers = *resources->getSamplers();
        lighting.lookupTables = makeTarget (4, 4, GpuTextureFormat::rgba8unorm);
        lighting.shadowCascades = makeTarget (16, 16, GpuTextureFormat::depth32float);
        lighting.shadowAtlas = makeTarget (16, 16, GpuTextureFormat::depth32float);
        lighting.lights = makeStorage();
        lighting.clusterLights = makeStorage();
        lighting.reflectionProbeBuffer = makeStorage();
        lighting.irradianceVolumeBuffer = makeStorage();

        mesh->addPrimitive ({ { { 0.0f, 1.0f, 0.0f } }, { { -1.0f, -1.0f, 0.0f } }, { { 1.0f, -1.0f, 0.0f } } }, { 0, 1, 2 });
        twoSided->doubleSided = true;
        blended->alphaMode = Material::AlphaMode::blend;
    }

    /** The attachments SceneRenderer draws the pass into on the raster tier, or on the full one
        with its velocity. */
    ForwardPass::Targets makeTargets (bool full) const
    {
        ForwardPass::Targets targets;
        targets.color = makeTarget (64, 64, full ? GpuTextureFormat::rgba16float : GpuTextureFormat::rgba8unorm);
        targets.depth = makeTarget (64, 64, full ? GpuTextureFormat::depth32float : GpuTextureFormat::depth24plusStencil8);
        targets.velocity = full ? makeTarget (64, 64, GpuTextureFormat::rg16float) : nullptr;
        return targets;
    }

    SceneDrawList::DrawItem makeItem (const Material::Ptr& material, const MaterialShaderNode* materialShader = nullptr) const
    {
        SceneDrawList::DrawItem item;
        item.mesh = mesh.get();
        item.primitive = &mesh->getPrimitive (0);
        item.material = material.get();
        item.materialShader = materialShader;
        return item;
    }

    /** Draws a frame as SceneRenderer does: raster over cleared targets, full over the loaded ones. */
    bool renderFrame (ForwardPass& pass, Span<const SceneDrawList::DrawItem> opaque, Span<const SceneDrawList::DrawItem> blend, const ForwardPass::Targets& targets)
    {
        const auto full = targets.velocity != nullptr;
        const auto clear = full ? std::optional<GpuColor>() : std::optional<GpuColor> (GpuColor::transparentBlack());

        auto frame = GpuFrame::begin (device);
        const auto rendered = pass.render (device, frame, view, lighting, opaque, blend, targets, full ? ForwardPass::Output::hdrLinear : ForwardPass::Output::ldrInline, 1, clear);
        frame.submit();
        return rendered;
    }

    /** For each pipeline @p pass switched to, in order, whether it is one of the first
        @p numShaderPipelines made: the ones of a material shader prepared before anything else. */
    std::vector<bool> getShaderPipelineSwitches (const RecordedPass& pass, size_t numShaderPipelines) const
    {
        std::vector<bool> switches;
        const rive::ore::Pipeline* current = nullptr;

        for (const auto* pipeline : pass.pipelines)
        {
            if (std::exchange (current, pipeline) == pipeline)
                continue;

            const auto made = std::find_if (madePipelines.begin(), madePipelines.end(), [pipeline] (const auto& candidate)
            {
                return candidate.get() == pipeline;
            });

            switches.push_back (static_cast<size_t> (made - madePipelines.begin()) < numShaderPipelines);
        }

        return switches;
    }

    ViewData view;
    std::unique_ptr<FrameResources> resources;
    LightingInputs lighting;
    std::vector<rive::rcp<rive::ore::Pipeline>> madePipelines; // Every pipeline the device made, in order
    Mesh::Ptr mesh { new Mesh ("triangle") };
    Material::Ptr solid { new Material() };
    Material::Ptr twoSided { new Material() };
    Material::Ptr blended { new Material() };
};

TEST_F (ForwardPassMockTests, MaterialShaderPipelinesAreCompiledOncePerShaderAndAttachments)
{
    ForwardPass pass;
    const auto shader = MaterialShader::createDefault();
    const auto other = MaterialShader::createDefault();
    const auto raster = makeTargets (false);
    const auto full = makeTargets (true);

    // One per blending, culling and winding
    EXPECT_TRUE (pass.prepareMaterialShader (device, shader, raster, 1));
    EXPECT_EQ (pipelinesMade, 8);

    EXPECT_TRUE (pass.prepareMaterialShader (device, shader, raster, 1));
    EXPECT_TRUE (pass.prepareMaterialShader (device, shader, makeTargets (false), 1));
    EXPECT_EQ (pipelinesMade, 8);

    // Other attachments, another sample count, another shader
    EXPECT_TRUE (pass.prepareMaterialShader (device, shader, full, 1));
    EXPECT_EQ (pipelinesMade, 16);

    EXPECT_TRUE (pass.prepareMaterialShader (device, shader, raster, 4));
    EXPECT_EQ (pipelinesMade, 24);

    EXPECT_TRUE (pass.prepareMaterialShader (device, other, raster, 1));
    EXPECT_EQ (pipelinesMade, 32);

    for (const auto& prepared : { shader, other })
        EXPECT_TRUE (pass.prepareMaterialShader (device, prepared, raster, 1));

    EXPECT_TRUE (pass.prepareMaterialShader (device, shader, full, 1));
    EXPECT_EQ (pipelinesMade, 32);
}

TEST_F (ForwardPassMockTests, NothingIsPreparedWithoutAShaderADeviceOrTargets)
{
    ForwardPass pass;
    const auto shader = MaterialShader::createDefault();
    const auto targets = makeTargets (false);

    auto noDepth = targets;
    noDepth.depth = nullptr;

    EXPECT_FALSE (pass.prepareMaterialShader (device, nullptr, targets, 1));
    EXPECT_FALSE (pass.prepareMaterialShader (nullptr, shader, targets, 1));
    EXPECT_FALSE (pass.prepareMaterialShader (device, shader, {}, 1));
    EXPECT_FALSE (pass.prepareMaterialShader (device, shader, noDepth, 1));
    EXPECT_EQ (pipelinesMade, 0);

    // None of them was remembered as a failure
    EXPECT_TRUE (pass.prepareMaterialShader (device, shader, targets, 1));
}

TEST_F (ForwardPassMockTests, ShaderWithoutTheVariantOfTheTierIsRefused)
{
    ForwardPass pass;
    const auto stock = MaterialShader::createDefault();
    const auto rasterOnly = MaterialShader::createFromBundles (stock->getRasterBundle(), nullptr);
    const auto fullOnly = MaterialShader::createFromBundles (nullptr, stock->getFullBundle());
    ASSERT_TRUE (rasterOnly.wasOk());
    ASSERT_TRUE (fullOnly.wasOk());

    EXPECT_TRUE (pass.prepareMaterialShader (device, rasterOnly.getValue(), makeTargets (false), 1));
    EXPECT_FALSE (pass.prepareMaterialShader (device, rasterOnly.getValue(), makeTargets (true), 1));
    EXPECT_EQ (pipelinesMade, 8);

    EXPECT_FALSE (pass.prepareMaterialShader (device, fullOnly.getValue(), makeTargets (false), 1));
    EXPECT_TRUE (pass.prepareMaterialShader (device, fullOnly.getValue(), makeTargets (true), 1));
    EXPECT_EQ (pipelinesMade, 16);
}

TEST_F (ForwardPassMockTests, FailedMaterialShaderCompileIsRememberedInsteadOfRetried)
{
    ForwardPass pass;
    const auto shader = MaterialShader::createDefault();

    failPipelines = true;
    EXPECT_FALSE (pass.prepareMaterialShader (device, shader, makeTargets (false), 1));

    failPipelines = false;
    EXPECT_FALSE (pass.prepareMaterialShader (device, shader, makeTargets (false), 1));
    EXPECT_EQ (pipelinesMade, 0);

    // Other attachments are other pipelines
    EXPECT_TRUE (pass.prepareMaterialShader (device, shader, makeTargets (true), 1));
    EXPECT_EQ (pipelinesMade, 8);
}

TEST_F (ForwardPassMockTests, WhatPreventsDrawingWithAMaterialShaderIsToldEachTimeItIsAsked)
{
    ForwardPass pass;
    const auto stock = MaterialShader::createDefault();
    const auto rasterOnly = MaterialShader::createFromBundles (stock->getRasterBundle(), nullptr);
    ASSERT_TRUE (rasterOnly.wasOk());

    // A shader without the variant of the tier
    const auto missing = pass.prepareMaterialShader (device, rasterOnly.getValue(), makeTargets (true), 1);
    EXPECT_TRUE (missing.failed());
    EXPECT_EQ (missing.getErrorMessage(), "it has no full tier variant");

    // Pipelines the device does not compile: what it said is told again once the failure is remembered
    failPipelines = true;
    const auto refused = pass.prepareMaterialShader (device, stock, makeTargets (false), 1);

    failPipelines = false;
    const auto remembered = pass.prepareMaterialShader (device, stock, makeTargets (false), 1);

    EXPECT_TRUE (refused.failed());
    EXPECT_TRUE (remembered.failed());
    EXPECT_EQ (remembered.getErrorMessage(), refused.getErrorMessage());

    EXPECT_TRUE (pass.prepareMaterialShader (device, stock, makeTargets (true), 1).wasOk());
}

TEST_F (ForwardPassMockTests, ItemsOfAMaterialShaderDrawWithItsPipelinesAmongTheOthers)
{
    ForwardPass pass;
    const auto targets = makeTargets (false);
    const MaterialShaderNode node (MaterialShader::createDefault());
    ASSERT_TRUE (pass.prepareMaterialShader (device, node.getShader(), targets, 1));

    const std::vector<SceneDrawList::DrawItem> opaque { makeItem (solid), makeItem (solid, &node), makeItem (solid) };
    const std::vector<SceneDrawList::DrawItem> blend { makeItem (blended, &node), makeItem (blended) };
    ASSERT_TRUE (renderFrame (pass, opaque, blend, targets));

    // The eight of the shader, then the eight of the pass and its background
    EXPECT_EQ (pipelinesMade, 17);

    ASSERT_EQ (passes.size(), 1u);
    EXPECT_EQ (passes[0].draws, 5);
    EXPECT_EQ (passes[0].countDistinctPipelines(), 4u);
    EXPECT_EQ (getShaderPipelineSwitches (passes[0], 8), (std::vector<bool> { false, true, false, true, false }));
}

TEST_F (ForwardPassMockTests, ItemOfAShaderThatWasNotPreparedDrawsWithItsMaterial)
{
    ForwardPass pass;
    const MaterialShaderNode node (MaterialShader::createDefault());

    const std::vector<SceneDrawList::DrawItem> opaque { makeItem (solid, &node), makeItem (solid) };
    ASSERT_TRUE (renderFrame (pass, opaque, {}, makeTargets (false)));

    EXPECT_EQ (pipelinesMade, 9);
    ASSERT_EQ (passes.size(), 1u);
    EXPECT_EQ (passes[0].draws, 2);
    EXPECT_EQ (passes[0].countDistinctPipelines(), 1u);
}

TEST_F (ForwardPassMockTests, FullTierDrawsTheOpaqueItemsOfMaterialShadersBeforeTheBlendedOnes)
{
    ForwardPass pass;
    const auto targets = makeTargets (true);
    const MaterialShaderNode node (MaterialShader::createDefault());
    ASSERT_TRUE (pass.prepareMaterialShader (device, node.getShader(), targets, 1));

    const std::vector<SceneDrawList::DrawItem> opaque { makeItem (solid, &node), makeItem (twoSided, &node) };
    const std::vector<SceneDrawList::DrawItem> blend { makeItem (blended), makeItem (blended, &node) };
    ASSERT_TRUE (renderFrame (pass, opaque, blend, targets));

    // The pass has blended pipelines only there: the opaque ones are the shader's
    EXPECT_EQ (pipelinesMade, 12);

    // Over the color, the velocity and the depth of the frame
    ASSERT_EQ (passes.size(), 1u);
    EXPECT_EQ (passes[0].colorCount, 2u);
    EXPECT_TRUE (passes[0].hasDepth);
    EXPECT_EQ (passes[0].draws, 4);
    EXPECT_EQ (getShaderPipelineSwitches (passes[0], 8), (std::vector<bool> { true, true, false, true }));
}

TEST_F (ForwardPassMockTests, PipelinesOfADestroyedShaderAreLetGoByTheNextRender)
{
    ForwardPass pass;
    const auto targets = makeTargets (true);
    auto shader = MaterialShader::createDefault();
    const auto kept = MaterialShader::createDefault();

    ASSERT_TRUE (pass.prepareMaterialShader (device, shader, targets, 1));
    ASSERT_TRUE (pass.prepareMaterialShader (device, kept, targets, 1));
    ASSERT_EQ (madePipelines.size(), 16u);

    // The pass holds them from frame to frame while the shader lives
    const auto held = madePipelines[0]->debugging_refcnt();
    EXPECT_GT (held, 1);

    EXPECT_TRUE (renderFrame (pass, {}, {}, targets));
    for (const auto& pipeline : madePipelines)
        EXPECT_EQ (pipeline->debugging_refcnt(), held);

    // Destroying the shader touches none of them, the pass lets them go when it is next asked to draw
    shader = nullptr;
    for (const auto& pipeline : madePipelines)
        EXPECT_EQ (pipeline->debugging_refcnt(), held);

    EXPECT_TRUE (renderFrame (pass, {}, {}, targets));
    for (size_t index = 0; index < madePipelines.size(); ++index)
        EXPECT_EQ (madePipelines[index]->debugging_refcnt(), index < 8 ? 1 : held) << index;

    // The ones of the living shader are still the ones it draws with
    EXPECT_TRUE (pass.prepareMaterialShader (device, kept, targets, 1));
    EXPECT_EQ (pipelinesMade, 16);
}

TEST_F (ForwardPassMockTests, ItemsOutOfViewAreNotDrawn)
{
    const MaterialShaderNode node (MaterialShader::createDefault());

    for (const auto full : { false, true })
    {
        ForwardPass pass;
        const auto targets = makeTargets (full);
        ASSERT_TRUE (pass.prepareMaterialShader (device, node.getShader(), targets, 1));

        // Of a material shader, the only opaque items the full tier takes
        std::vector<SceneDrawList::DrawItem> opaque { makeItem (solid, &node), makeItem (solid, &node), makeItem (twoSided, &node) };
        std::vector<SceneDrawList::DrawItem> blend { makeItem (blended), makeItem (blended), makeItem (blended, &node) };
        opaque[1].inView = false;
        blend[0].inView = false;
        blend[2].inView = false;

        const auto numPasses = passes.size();
        ASSERT_TRUE (renderFrame (pass, opaque, blend, targets));

        ASSERT_EQ (passes.size(), numPasses + 1) << full;
        EXPECT_EQ (passes.back().draws, 3) << full;
    }
}
