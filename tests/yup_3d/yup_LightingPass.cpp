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

    static ShaderBundle lightingPass()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_LightingPass_full.inc>
        };

        auto bundle = ShaderBundle::loadFromData (data, sizeof (data));
        EXPECT_TRUE (bundle.wasOk()) << bundle.getErrorMessage();
        return bundle.wasOk() ? std::move (bundle.getReference()) : ShaderBundle();
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
    const auto bundle = lightingPass();
    const auto* fragment = fragmentOf (bundle);
    ASSERT_NE (fragment, nullptr);

    using Bindings = std::vector<std::pair<uint32_t, uint32_t>>;

    // The ViewBlock and the LightingBlock
    EXPECT_EQ (bindingsOf (fragment->uniformBuffers), (Bindings { { 0, 0 }, { 1, 0 } }));

    // The environment, the shadow map and the two area light tables, then RT0-RT2 and the depth:
    // the 8 textures a bind group holds at most
    EXPECT_EQ (bindingsOf (fragment->separateImages), (Bindings { { 1, 1 }, { 1, 3 }, { 1, 4 }, { 1, 5 }, { 1, 9 }, { 1, 10 }, { 1, 11 }, { 1, 12 } }));
    EXPECT_EQ (bindingsOf (fragment->separateSamplers), (Bindings { { 0, 1 }, { 0, 3 }, { 1, 2 } }));
    EXPECT_TRUE (fragment->sampledImages.empty());

    // The lights, the light lists of the clusters and the material table, read only
    EXPECT_EQ (bindingsOf (fragment->storageBuffers), (Bindings { { 1, 6 }, { 1, 7 }, { 1, 8 } }));
    for (const auto& buffer : fragment->storageBuffers)
        EXPECT_TRUE (buffer.readOnly) << buffer.name;
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
