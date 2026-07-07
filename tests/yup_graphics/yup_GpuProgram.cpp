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

#include <yup_graphics/yup_graphics.h>

using namespace yup;

namespace
{

GpuShaderSource makeStubShader (const char* code, const uint8_t* map, size_t mapSize)
{
    GpuShaderSource src;
    src.language = GpuShaderLanguage::glsl;
    src.code = code;
    src.codeSize = (uint32_t) strlen (code);
    src.bindingMap = map;
    src.bindingMapSize = (uint32_t) mapSize;
    return src;
}

} // namespace

class GpuProgramTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        context = GraphicsContext::createContext (GraphicsContext::Headless, {});
        ASSERT_NE (context, nullptr);
    }

    std::unique_ptr<GraphicsContext> context;
};

// ---------------------------------------------------------------------------
// makeShaderBindingMapBlob — reflection → blob conversion

TEST (ShaderBindingMapTests, EmptyReflectionProducesBlob)
{
    ShaderReflection refl;
    auto blob = makeShaderBindingMapBlob (refl, ShaderStage::vertex);

    // Even with no resources, the blob carries a version header.
    EXPECT_FALSE (blob.empty());
}

TEST (ShaderBindingMapTests, UniformBufferIsEncoded)
{
    ShaderReflection refl;

    ShaderReflection::ResourceBinding ub;
    ub.name = "Uniforms";
    ub.set = 0;
    ub.binding = 0;
    ub.backendSlot = 0;
    refl.uniformBuffers.push_back (ub);

    auto vsBlob = makeShaderBindingMapBlob (refl, ShaderStage::vertex);
    auto fsBlob = makeShaderBindingMapBlob (refl, ShaderStage::fragment);

    EXPECT_FALSE (vsBlob.empty());
    EXPECT_FALSE (fsBlob.empty());
}

TEST (ShaderBindingMapTests, TextureAndSamplerAreEncoded)
{
    ShaderReflection refl;

    ShaderReflection::ResourceBinding img;
    img.set = 0;
    img.binding = 0;
    img.backendSlot = 0;
    refl.separateImages.push_back (img);

    ShaderReflection::ResourceBinding samp;
    samp.set = 0;
    samp.binding = 1;
    samp.backendSlot = 0;
    refl.separateSamplers.push_back (samp);

    auto blob = makeShaderBindingMapBlob (refl, ShaderStage::fragment);
    EXPECT_FALSE (blob.empty());
}

// ---------------------------------------------------------------------------
// GpuBuffer::create — validation and null paths

TEST_F (GpuProgramTests, GpuBufferCreateHeadlessReturnsNull)
{
    const float verts[] = { 0.0f, 1.0f, 2.0f };
    // Headless backend has no ore context — creation fails gracefully.
    EXPECT_EQ (GpuBuffer::create (*context, GpuBufferType::vertex, verts, sizeof (verts)), nullptr);
}

TEST_F (GpuProgramTests, GpuBufferCreateWithNullDataReturnsNull)
{
    EXPECT_EQ (GpuBuffer::create (*context, GpuBufferType::vertex, nullptr, 16), nullptr);
}

TEST_F (GpuProgramTests, GpuBufferCreateWithZeroSizeReturnsNull)
{
    const float verts[] = { 0.0f };
    EXPECT_EQ (GpuBuffer::create (*context, GpuBufferType::vertex, verts, 0), nullptr);
}

TEST (GpuBufferDefaults, DefaultPtrIsNull)
{
    GpuBuffer::Ptr b;
    EXPECT_EQ (b, nullptr);
}

// ---------------------------------------------------------------------------
// GpuProgram::compile — validation and headless (no ore) paths

TEST_F (GpuProgramTests, CompileHeadlessReturnsNullWithError)
{
    const uint8_t map[] = { 2, 1 };
    auto vs = makeStubShader ("void main() {}", map, sizeof (map));
    auto fs = makeStubShader ("void main() {}", map, sizeof (map));

    auto result = GpuProgram::compile (*context, vs, fs);
    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST_F (GpuProgramTests, CompileWithEmptyVertexCodeFailsFastWhenOreAvailable)
{
    // On headless, ore is unavailable so we get the ore error first; either way
    // the call must not crash and must return null.
    const uint8_t map[] = { 2, 1 };
    GpuShaderSource vs;
    vs.language = GpuShaderLanguage::glsl;
    vs.bindingMap = map;
    vs.bindingMapSize = sizeof (map);

    auto fs = makeStubShader ("void main() {}", map, sizeof (map));

    auto result = GpuProgram::compile (*context, vs, fs);
    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST_F (GpuProgramTests, CompileWithPipelineOptionsHeadlessReturnsNull)
{
    const uint8_t map[] = { 2, 1 };
    auto vs = makeStubShader ("void main() {}", map, sizeof (map));
    auto fs = makeStubShader ("void main() {}", map, sizeof (map));

    GpuVertexAttribute attr { GpuVertexFormat::float3, 0, 0 };
    GpuVertexBufferLayout layout { 12, GpuVertexStepMode::vertex, &attr, 1 };

    GpuPipelineOptions options;
    options.vertexBuffers = &layout;
    options.vertexBufferCount = 1;
    options.indexFormat = GpuIndexFormat::uint16;
    options.cullMode = GpuCullMode::back;

    auto result = GpuProgram::compile (*context, vs, fs, options);
    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST (GpuProgramDefaults, DefaultPtrIsNull)
{
    GpuProgram::Ptr p;
    EXPECT_EQ (p, nullptr);
}
