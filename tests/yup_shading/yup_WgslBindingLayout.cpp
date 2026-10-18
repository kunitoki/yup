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

#include <yup_shading/yup_shading.h>

using namespace yup;

//==============================================================================

class WgslBindingLayoutTests : public ::testing::Test
{
protected:
    using Layout = WgslBindingLayout;

    static std::vector<Layout::Entry> parseOk (const String& wgsl)
    {
        auto parsed = Layout::parse (wgsl);
        EXPECT_TRUE (parsed.wasOk()) << parsed.getErrorMessage() << "\n" << wgsl;
        return parsed.wasOk() ? parsed.getReference() : std::vector<Layout::Entry> {};
    }

    static const Layout::Entry* find (const std::vector<Layout::Entry>& entries, uint32 group, uint32 binding)
    {
        for (const auto& entry : entries)
        {
            if (entry.group == group && entry.binding == binding)
                return std::addressof (entry);
        }

        return nullptr;
    }

#if YUP_ENABLE_SHADER_TRANSPILER
    /** Parses the WGSL ShaderTranspiler produces for a GLSL compute shader. */
    static std::vector<Layout::Entry> parseCompute (const String& glsl)
    {
        auto wgsl = ShaderTranspiler().transpile (glsl, ShaderStage::compute, ShaderLanguage::glsl, ShaderLanguage::wgsl);
        EXPECT_TRUE (wgsl.wasOk()) << wgsl.getErrorMessage();
        return wgsl.wasOk() ? parseOk (wgsl.getReference()) : std::vector<Layout::Entry> {};
    }
#endif
};

//==============================================================================

#if YUP_ENABLE_SHADER_TRANSPILER

TEST_F (WgslBindingLayoutTests, FetchedTextureIsUnfilterableWithItsSamplerStillListed)
{
    const auto entries = parseCompute (R"(#version 450
layout(local_size_x = 1) in;
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(std430, set = 0, binding = 2) buffer Result { vec4 texel; };
void main() { texel = texelFetch (sampler2D (source, sourceSampler), ivec2 (1, 1), 0); })");

    ASSERT_EQ (entries.size(), 3u);

    const auto* texture = find (entries, 0, 0);
    ASSERT_NE (texture, nullptr);
    EXPECT_EQ (texture->kind, Layout::ResourceKind::texture);
    EXPECT_EQ (texture->sampleType, Layout::SampleType::floatUnfilterable);
    EXPECT_EQ (texture->viewDimension, Layout::ViewDimension::texture2D);
    EXPECT_FALSE (texture->multisampled);

    const auto* sampler = find (entries, 0, 1);
    ASSERT_NE (sampler, nullptr);
    EXPECT_EQ (sampler->kind, Layout::ResourceKind::sampler);
    EXPECT_EQ (sampler->samplerType, Layout::SamplerType::filtering);
}

TEST_F (WgslBindingLayoutTests, SampledTextureIsFilterable)
{
    const auto entries = parseCompute (R"(#version 450
layout(local_size_x = 1) in;
layout(set = 0, binding = 0) uniform texture2DArray source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(std430, set = 0, binding = 2) buffer Result { vec4 texel; };
void main() { texel = textureLod (sampler2DArray (source, sourceSampler), vec3 (0.5, 0.5, 0.0), 0.0); })");

    const auto* texture = find (entries, 0, 0);
    ASSERT_NE (texture, nullptr);
    EXPECT_EQ (texture->sampleType, Layout::SampleType::floatFilterable);
    EXPECT_EQ (texture->viewDimension, Layout::ViewDimension::array2D);
}

TEST_F (WgslBindingLayoutTests, ShadowSamplingUsesDepthTextureAndComparisonSampler)
{
    const auto entries = parseCompute (R"(#version 450
layout(local_size_x = 1) in;
layout(set = 0, binding = 0) uniform texture2D shadowMap;
layout(set = 0, binding = 1) uniform samplerShadow shadowSampler;
layout(std430, set = 0, binding = 2) buffer Result { float lit; };
void main() { lit = textureLod (sampler2DShadow (shadowMap, shadowSampler), vec3 (0.5), 0.0); })");

    const auto* texture = find (entries, 0, 0);
    const auto* sampler = find (entries, 0, 1);
    ASSERT_NE (texture, nullptr);
    ASSERT_NE (sampler, nullptr);
    EXPECT_EQ (texture->sampleType, Layout::SampleType::depth);
    EXPECT_EQ (sampler->samplerType, Layout::SamplerType::comparison);
}

TEST_F (WgslBindingLayoutTests, SamplingThroughAHelperFunctionIsFilterable)
{
    const auto sampled = parseCompute (R"(#version 450
layout(local_size_x = 1) in;
layout(set = 0, binding = 0) uniform sampler2D tex;
layout(std430, set = 0, binding = 1) buffer Result { vec4 texel; };
vec4 fetch (sampler2D s, vec2 uv) { return textureLod (s, uv, 0.0); }
vec4 indirect (sampler2D s) { return fetch (s, vec2 (0.5)); }
void main() { texel = indirect (tex); })");

    const auto* texture = find (sampled, 0, 0);
    ASSERT_NE (texture, nullptr);
    EXPECT_EQ (texture->sampleType, Layout::SampleType::floatFilterable);

    // The combined sampler's companion is listed too
    EXPECT_EQ (sampled.size(), 3u);

    const auto loaded = parseCompute (R"(#version 450
layout(local_size_x = 1) in;
layout(set = 0, binding = 0) uniform sampler2D tex;
layout(std430, set = 0, binding = 1) buffer Result { vec4 texel; };
vec4 fetch (sampler2D s) { return texelFetch (s, ivec2 (0), 0); }
void main() { texel = fetch (tex); })");

    texture = find (loaded, 0, 0);
    ASSERT_NE (texture, nullptr);
    EXPECT_EQ (texture->sampleType, Layout::SampleType::floatUnfilterable);
}

TEST_F (WgslBindingLayoutTests, StorageTexturesKeepFormatAccessAndDimension)
{
    const auto entries = parseCompute (R"(#version 450
layout(local_size_x = 1) in;
layout(set = 0, binding = 0, rgba16f) uniform writeonly image2D color;
layout(set = 0, binding = 1, rgba32f) uniform writeonly image3D volume;
layout(set = 0, binding = 2, r32f) uniform readonly image2D heights;
layout(set = 0, binding = 3, r32f) uniform image2DArray accumulation;
void main()
{
    const float h = imageLoad (heights, ivec2 (0)).r;
    imageStore (color, ivec2 (0), vec4 (h));
    imageStore (volume, ivec3 (0), vec4 (h));
    imageStore (accumulation, ivec3 (0), imageLoad (accumulation, ivec3 (0)) + h);
})");

    ASSERT_EQ (entries.size(), 4u);

    const auto* color = find (entries, 0, 0);
    ASSERT_NE (color, nullptr);
    EXPECT_EQ (color->kind, Layout::ResourceKind::storageTexture);
    EXPECT_EQ (color->storageFormat, "rgba16float");
    EXPECT_EQ (color->storageAccess, Layout::StorageAccess::writeOnly);
    EXPECT_EQ (color->viewDimension, Layout::ViewDimension::texture2D);

    const auto* volume = find (entries, 0, 1);
    ASSERT_NE (volume, nullptr);
    EXPECT_EQ (volume->storageFormat, "rgba32float");
    EXPECT_EQ (volume->viewDimension, Layout::ViewDimension::texture3D);

    const auto* heights = find (entries, 0, 2);
    ASSERT_NE (heights, nullptr);
    EXPECT_EQ (heights->storageFormat, "r32float");
    EXPECT_EQ (heights->storageAccess, Layout::StorageAccess::readOnly);

    const auto* accumulation = find (entries, 0, 3);
    ASSERT_NE (accumulation, nullptr);
    EXPECT_EQ (accumulation->storageAccess, Layout::StorageAccess::readWrite);
    EXPECT_EQ (accumulation->viewDimension, Layout::ViewDimension::array2D);
}

TEST_F (WgslBindingLayoutTests, BuffersMapToTheirBindingTypes)
{
    const auto entries = parseCompute (R"(#version 450
layout(local_size_x = 1) in;
layout(std140, set = 0, binding = 0) uniform Params { float scale; };
layout(std430, set = 0, binding = 1) readonly buffer Input { float inputs[]; };
layout(std430, set = 0, binding = 2) buffer Output { float outputs[]; };
void main() { outputs[0] = inputs[0] * scale; })");

    ASSERT_EQ (entries.size(), 3u);
    EXPECT_EQ (find (entries, 0, 0)->bufferType, Layout::BufferType::uniform);
    EXPECT_EQ (find (entries, 0, 1)->bufferType, Layout::BufferType::readOnlyStorage);
    EXPECT_EQ (find (entries, 0, 2)->bufferType, Layout::BufferType::storage);

    for (const auto& entry : entries)
        EXPECT_EQ (entry.kind, Layout::ResourceKind::buffer);
}

TEST_F (WgslBindingLayoutTests, GroupGapsKeepTheirGroupIndices)
{
    const auto entries = parseCompute (R"(#version 450
layout(local_size_x = 4) in;
layout(std430, set = 2, binding = 5) buffer Data { float values[]; };
void main() { values[gl_GlobalInvocationID.x] *= 2.0; })");

    ASSERT_EQ (entries.size(), 1u);
    EXPECT_EQ (entries[0].group, 2u);
    EXPECT_EQ (entries[0].binding, 5u);
}

TEST_F (WgslBindingLayoutTests, CompanionSamplerWinsTheSlotOfAnUnusedResourceDeclaredAfterIt)
{
    // The companion sampler takes the first binding after the live ones, which the unused texture keeps
    const auto entries = parseCompute (R"(#version 450
layout(local_size_x = 1) in;
layout(set = 0, binding = 0) uniform sampler2D tex;
layout(std430, set = 0, binding = 1) buffer Result { vec4 texel; };
layout(set = 0, binding = 2) uniform texture2D unused;
void main() { texel = textureLod (tex, vec2 (0.5), 0.0); })");

    ASSERT_EQ (entries.size(), 3u);

    const auto* shared = find (entries, 0, 2);
    ASSERT_NE (shared, nullptr);
    EXPECT_EQ (shared->name, "tex_sampler");
    EXPECT_EQ (shared->kind, Layout::ResourceKind::sampler);
}

TEST_F (WgslBindingLayoutTests, CompanionSamplerWinsTheSlotOfAnUnusedResourceDeclaredBeforeIt)
{
    const auto entries = parseCompute (R"(#version 450
layout(local_size_x = 1) in;
layout(set = 0, binding = 2) uniform texture2D unused;
layout(set = 0, binding = 0) uniform sampler2D tex;
layout(std430, set = 0, binding = 1) buffer Result { vec4 texel; };
void main() { texel = textureLod (tex, vec2 (0.5), 0.0); })");

    ASSERT_EQ (entries.size(), 3u);

    const auto* shared = find (entries, 0, 2);
    ASSERT_NE (shared, nullptr);
    EXPECT_EQ (shared->name, "tex_sampler");
    EXPECT_EQ (shared->kind, Layout::ResourceKind::sampler);
}

#endif // YUP_ENABLE_SHADER_TRANSPILER

TEST_F (WgslBindingLayoutTests, HelperFunctionsNamedLikeBuiltinsAreFollowed)
{
    const auto entries = parseOk (R"(
@group(0) @binding(0) var tex: texture_2d<f32>;
@group(0) @binding(1) var tex_sampler: sampler;
@group(0) @binding(2) var<storage, read_write> result: vec4<f32>;
fn textureBicubic(t: texture_2d<f32>, s: sampler, uv: vec2<f32>) -> vec4<f32> {
    return textureSampleLevel(t, s, uv, 0.0);
}
@compute @workgroup_size(1)
fn main() {
    result = textureBicubic(tex, tex_sampler, vec2<f32>(0.5));
}
)");

    const auto* texture = find (entries, 0, 0);
    ASSERT_NE (texture, nullptr);
    EXPECT_EQ (texture->sampleType, Layout::SampleType::floatFilterable);
}

TEST_F (WgslBindingLayoutTests, SharedSlotKeepsTheReferencedResource)
{
    const auto entries = parseOk (R"(
struct Input { unused: f32, }
@group(0) @binding(0) var tex: texture_2d<f32>;
@group(0) @binding(1) var tex_sampler: sampler;
@group(0) @binding(1) var unused: texture_2d<f32>;
@group(0) @binding(2) var<storage, read_write> result: array<vec4<f32>>;
@compute @workgroup_size(1)
fn main() {
    var input: Input;
    result[0] = textureSampleLevel(tex, tex_sampler, vec2<f32>(input.unused), 0.0);
}
)");

    ASSERT_EQ (entries.size(), 3u);

    // input.unused is a member, not a reference to the texture
    const auto* shared = find (entries, 0, 1);
    ASSERT_NE (shared, nullptr);
    EXPECT_EQ (shared->name, "tex_sampler");
    EXPECT_EQ (shared->kind, Layout::ResourceKind::sampler);
}

TEST_F (WgslBindingLayoutTests, CommentsAreIgnored)
{
    const auto entries = parseOk (R"(
// @group(0) @binding(5) var old: sampler;
/* @group(0) @binding(6) var older: texture_2d<f32>;
   /* nested */ @group(0) @binding(7) var oldest: sampler; */
@binding(0) @group(1) var tex: texture_2d<f32>; // textureSample(tex, s, uv)
@group(1) @binding(1) var<storage, read_write> result: vec4<f32>;
@compute @workgroup_size(1)
fn main() {
    // result = textureSampleLevel(tex, s, vec2<f32>(0.5), 0.0);
    result = textureLoad(tex, vec2<i32>(0), 0);
}
)");

    ASSERT_EQ (entries.size(), 2u);

    const auto* texture = find (entries, 1, 0);
    ASSERT_NE (texture, nullptr);
    EXPECT_EQ (texture->sampleType, Layout::SampleType::floatUnfilterable);
}

TEST_F (WgslBindingLayoutTests, UnsupportedResourceTypeFails)
{
    const auto parsed = Layout::parse ("@group(0) @binding(0) var video: texture_external;\n");
    ASSERT_TRUE (parsed.failed());
    EXPECT_TRUE (parsed.getErrorMessage().contains ("video"));
}

TEST_F (WgslBindingLayoutTests, ModuleWithoutResourcesIsEmpty)
{
    EXPECT_TRUE (parseOk ("@compute @workgroup_size(1)\nfn main() { var x: f32 = 1.0; }\n").empty());
}
