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
#include <set>
#include <utility>

using namespace yup;

/** CPU only: the reflection of the precompiled G-buffer bundles. The GPU tests live with the
    SceneRenderer ones, whose fixtures read textures back (GBufferPassMetalTests). */
class GBufferPassTests : public ::testing::Test
{
protected:
    static ShaderBundle load (const uint8* data, size_t size)
    {
        auto bundle = ShaderBundle::loadFromData (data, size);
        EXPECT_TRUE (bundle.wasOk()) << bundle.getErrorMessage();
        return bundle.wasOk() ? std::move (bundle.getReference()) : ShaderBundle();
    }

    static ShaderBundle gbufferPass()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_GBufferPass_tier_full.inc"
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle gbufferBackground()
    {
        static constexpr uint8 data[] = {
#include "yup_renderer_shaders/yup_GBufferBackground.inc"
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

    static const ShaderReflection* reflectionOf (const ShaderBundle& bundle, ShaderStage stage)
    {
        const auto* shader = bundle.findShader (stage, ShaderLanguage::spirv);
        return shader != nullptr ? &shader->reflection : nullptr;
    }

    static std::set<uint32_t> outputLocations (const ShaderReflection& reflection)
    {
        std::set<uint32_t> locations;
        for (const auto& output : reflection.stageOutputs)
            locations.insert (output.location);

        return locations;
    }

    static const ShaderReflection::ResourceBinding* findBlock (const ShaderReflection& reflection, uint32_t set, uint32_t binding)
    {
        const auto& blocks = reflection.uniformBuffers;
        const auto block = std::find_if (blocks.begin(), blocks.end(), [&] (const auto& b) { return b.set == set && b.binding == binding; });
        return block != blocks.end() ? &*block : nullptr;
    }

    static std::map<String, uint32_t> memberOffsets (const ShaderReflection::ResourceBinding& block)
    {
        std::map<String, uint32_t> offsets;
        for (const auto& member : block.members)
            offsets[member.name] = member.offset;

        return offsets;
    }

    /** The (set, binding) of every texture and sampler. */
    static std::set<std::pair<uint32_t, uint32_t>> textureAndSamplerBindings (const ShaderReflection& reflection)
    {
        std::set<std::pair<uint32_t, uint32_t>> bindings;
        for (const auto* resources : { &reflection.separateImages, &reflection.separateSamplers })
            for (const auto& resource : *resources)
                bindings.insert ({ resource.set, resource.binding });

        return bindings;
    }
};

TEST_F (GBufferPassTests, BothBundlesWriteTheFourTargets)
{
    const std::pair<const char*, ShaderBundle> bundles[] = {
        { "GBufferPass_full", gbufferPass() },
        { "GBufferBackground", gbufferBackground() },
    };

    for (const auto& [name, bundle] : bundles)
    {
        const auto* fragment = reflectionOf (bundle, ShaderStage::fragment);
        ASSERT_NE (fragment, nullptr) << name;
        EXPECT_EQ (outputLocations (*fragment), (std::set<uint32_t> { 0, 1, 2, 3 })) << name;
    }
}

TEST_F (GBufferPassTests, DrawBlockCarriesTheMaterialId)
{
    const ShaderBundle bundles[] = { gbufferPass(), forwardPassFull() };

    for (const auto& bundle : bundles)
    {
        const auto* vertex = reflectionOf (bundle, ShaderStage::vertex);
        ASSERT_NE (vertex, nullptr);

        const auto* block = findBlock (*vertex, 2, 0);
        ASSERT_NE (block, nullptr);
        EXPECT_EQ (block->blockSize, static_cast<uint32_t> (sizeof (GeometryBindings::DrawBlock)));
        EXPECT_EQ (memberOffsets (*block)["ids"], static_cast<uint32_t> (offsetof (GeometryBindings::DrawBlock, ids)));
    }
}

TEST_F (GBufferPassTests, SharesTheMaterialBindingsOfTheForwardPass)
{
    const auto gbuffer = gbufferPass();
    const auto forward = forwardPassFull();
    const auto* gbufferFragment = reflectionOf (gbuffer, ShaderStage::fragment);
    const auto* forwardFragment = reflectionOf (forward, ShaderStage::fragment);
    ASSERT_NE (gbufferFragment, nullptr);
    ASSERT_NE (forwardFragment, nullptr);

    const auto* gbufferMaterial = findBlock (*gbufferFragment, 2, 1);
    const auto* forwardMaterial = findBlock (*forwardFragment, 2, 1);
    ASSERT_NE (gbufferMaterial, nullptr);
    ASSERT_NE (forwardMaterial, nullptr);
    // The G-buffer declares the block without the clearcoat and sheen rows; the full forward pass,
    // which shades the layers, declares them. Every draw binds all of it
    const auto layerRows = static_cast<uint32_t> (offsetof (GeometryBindings::MaterialBlock, extension) + offsetof (GpuMaterial, clearcoat));
    EXPECT_EQ (gbufferMaterial->blockSize, layerRows);
    EXPECT_EQ (forwardMaterial->blockSize, static_cast<uint32_t> (sizeof (GeometryBindings::MaterialBlock)));

    auto forwardOffsets = memberOffsets (*forwardMaterial);
    EXPECT_EQ (forwardOffsets["clearcoat"], layerRows);
    EXPECT_EQ (forwardOffsets["sheen"], layerRows + 16u);
    forwardOffsets.erase ("clearcoat");
    forwardOffsets.erase ("sheen");
    EXPECT_EQ (memberOffsets (*gbufferMaterial), forwardOffsets);

    // The five material textures at set 2 bindings 2-6 and their samplers at 7-11, nothing else
    std::set<std::pair<uint32_t, uint32_t>> material;
    for (uint32_t binding = 2; binding <= 11; ++binding)
        material.insert ({ 2u, binding });

    EXPECT_EQ (textureAndSamplerBindings (*gbufferFragment), material);
}

TEST_F (GBufferPassTests, ReadsOnlyTheViewAndTheDraw)
{
    const std::pair<const char*, ShaderBundle> bundles[] = {
        { "GBufferPass_full", gbufferPass() },
        { "GBufferBackground", gbufferBackground() },
    };

    for (const auto& [name, bundle] : bundles)
    {
        for (const auto stage : { ShaderStage::vertex, ShaderStage::fragment })
        {
            const auto* reflection = reflectionOf (bundle, stage);
            ASSERT_NE (reflection, nullptr) << name;

            for (const auto& block : reflection->uniformBuffers)
                EXPECT_TRUE (block.set == 0 || block.set == 2) << name << " " << toString (stage) << " set " << (int) block.set;
        }
    }
}
