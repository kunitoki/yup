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

class GpuPipelineTests : public ::testing::Test
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

TEST_F (GpuPipelineTests, GpuBufferCreateHeadlessReturnsNull)
{
    const float verts[] = { 0.0f, 1.0f, 2.0f };
    // Headless backend has no ore context — creation fails gracefully.
    EXPECT_EQ (GpuBuffer::create (*context, GpuBufferType::vertex, verts, sizeof (verts)), nullptr);
}

TEST_F (GpuPipelineTests, GpuBufferCreateWithNullDataReturnsNull)
{
    EXPECT_EQ (GpuBuffer::create (*context, GpuBufferType::vertex, nullptr, 16), nullptr);
}

TEST_F (GpuPipelineTests, GpuBufferCreateWithZeroSizeReturnsNull)
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
// GpuPipeline::compile — validation and headless (no ore) paths

TEST_F (GpuPipelineTests, CompileHeadlessReturnsNullWithError)
{
    const uint8_t map[] = { 2, 1 };
    auto vs = makeStubShader ("void main() {}", map, sizeof (map));
    auto fs = makeStubShader ("void main() {}", map, sizeof (map));

    auto result = GpuPipeline::compile (*context, vs, fs);
    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST_F (GpuPipelineTests, CompileWithEmptyVertexCodeFailsFastWhenOreAvailable)
{
    // On headless, ore is unavailable so we get the ore error first; either way
    // the call must not crash and must return null.
    const uint8_t map[] = { 2, 1 };
    GpuShaderSource vs;
    vs.language = GpuShaderLanguage::glsl;
    vs.bindingMap = map;
    vs.bindingMapSize = sizeof (map);

    auto fs = makeStubShader ("void main() {}", map, sizeof (map));

    auto result = GpuPipeline::compile (*context, vs, fs);
    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST_F (GpuPipelineTests, CompileWithPipelineOptionsHeadlessReturnsNull)
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

    auto result = GpuPipeline::compile (*context, vs, fs, options);
    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST (GpuPipelineDefaults, DefaultPtrIsNull)
{
    GpuPipeline::Ptr p;
    EXPECT_EQ (p, nullptr);
}

// ---------------------------------------------------------------------------
// GpuPipelineCache — key determinism, hit/miss, eviction, failure propagation

class GpuPipelineCacheTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        context = GraphicsContext::createContext (GraphicsContext::Headless, {});
        ASSERT_NE (context, nullptr);
    }

    std::unique_ptr<GraphicsContext> context;
};

TEST_F (GpuPipelineCacheTests, SameInputsProduceSameKey)
{
    ShaderBundle bundle;
    GpuPipelineOptions options;

    const auto key1 = GpuPipelineCache::generateCacheKey (bundle, options, GraphicsContext::Metal);
    const auto key2 = GpuPipelineCache::generateCacheKey (bundle, options, GraphicsContext::Metal);
    EXPECT_EQ (key1, key2);
}

TEST_F (GpuPipelineCacheTests, DifferentApiProducesDifferentKey)
{
    ShaderBundle bundle;
    GpuPipelineOptions options;

    const auto keyMetal = GpuPipelineCache::generateCacheKey (bundle, options, GraphicsContext::Metal);
    const auto keyD3D = GpuPipelineCache::generateCacheKey (bundle, options, GraphicsContext::Direct3D);
    EXPECT_NE (keyMetal, keyD3D);
}

TEST_F (GpuPipelineCacheTests, DifferentOptionsProduceDifferentKey)
{
    ShaderBundle bundle;

    GpuPipelineOptions a;
    GpuPipelineOptions b;
    b.cullMode = GpuCullMode::back;

    const auto keyA = GpuPipelineCache::generateCacheKey (bundle, a, GraphicsContext::Metal);
    const auto keyB = GpuPipelineCache::generateCacheKey (bundle, b, GraphicsContext::Metal);
    EXPECT_NE (keyA, keyB);
}

TEST_F (GpuPipelineCacheTests, StoreAndContainsAndCount)
{
    GpuPipelineCache cache (*context);
    EXPECT_EQ (cache.getNumEntries(), 0u);
    EXPECT_FALSE (cache.contains ("key"));

    cache.store ("key", nullptr);
    EXPECT_TRUE (cache.contains ("key"));
    EXPECT_EQ (cache.getNumEntries(), 1u);

    cache.remove ("key");
    EXPECT_FALSE (cache.contains ("key"));
    EXPECT_EQ (cache.getNumEntries(), 0u);
}

TEST_F (GpuPipelineCacheTests, EvictsAtMaxEntries)
{
    GpuPipelineCache cache (*context);
    cache.setMaxEntries (2);
    EXPECT_EQ (cache.getMaxEntries(), 2u);

    cache.store ("a", nullptr);
    cache.store ("b", nullptr);
    cache.store ("c", nullptr);

    EXPECT_EQ (cache.getNumEntries(), 2u);
}

TEST_F (GpuPipelineCacheTests, ClearRemovesAllEntries)
{
    GpuPipelineCache cache (*context);
    cache.store ("a", nullptr);
    cache.store ("b", nullptr);
    cache.clear();
    EXPECT_EQ (cache.getNumEntries(), 0u);
}

TEST_F (GpuPipelineCacheTests, GetOrCompileHeadlessFails)
{
    GpuPipelineCache cache (*context);
    ShaderBundle bundle;

    auto result = cache.getOrCompile (bundle);
    EXPECT_TRUE (result.failed());
}

// ---------------------------------------------------------------------------
// GpuFrame — headless invalid, RAII submit idempotency

TEST_F (GpuPipelineTests, GpuFrameHeadlessIsInvalid)
{
    auto frame = GpuFrame::begin (*context);
    EXPECT_FALSE (frame.isValid());
    EXPECT_FALSE (frame.submit());
}

TEST_F (GpuPipelineTests, GpuFrameSubmitIsIdempotentOnInvalid)
{
    auto frame = GpuFrame::begin (*context);
    EXPECT_FALSE (frame.submit());
    EXPECT_FALSE (frame.submit());
    EXPECT_NO_THROW (frame.waitForGPU());
}
