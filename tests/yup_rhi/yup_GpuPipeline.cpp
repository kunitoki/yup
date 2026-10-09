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

#include <yup_rhi/yup_rhi.h>

#include <set>

using namespace yup;

namespace
{

GpuShaderSource makeStubShader (const char* code, const uint8_t* map, size_t mapSize)
{
    GpuShaderSource src;
    src.language = GpuShaderLanguage::glsl;
    src.code = gpuShaderSourceBytes (code);
    src.bindingMap = std::vector<uint8> (map, map + mapSize);
    return src;
}

} // namespace

class GpuPipelineTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        context = GpuDevice::create (GpuPlatform::Headless, {});
        ASSERT_NE (context, nullptr);
    }

    GpuDevice::Ptr context;
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

TEST (ShaderBindingMapTests, DepthSamplerIsEncodedAsComparisonSampler)
{
    ShaderReflection refl;

    ShaderReflection::ResourceBinding img;
    img.binding = 0;
    img.imageIsDepth = true;
    refl.separateImages.push_back (img);

    ShaderReflection::ResourceBinding shadowSampler;
    shadowSampler.binding = 1;
    shadowSampler.imageIsDepth = true;
    refl.separateSamplers.push_back (shadowSampler);

    ShaderReflection::ResourceBinding plainSampler;
    plainSampler.binding = 2;
    refl.separateSamplers.push_back (plainSampler);

    const auto blob = makeShaderBindingMapBlob (refl, ShaderStage::fragment);

    rive::ore::BindingMap map;
    ASSERT_TRUE (rive::ore::BindingMap::fromBlob (blob.data(), blob.size(), &map));

    const auto* texture = map.lookupEntry (0, 0);
    const auto* comparison = map.lookupEntry (0, 1);
    const auto* filtering = map.lookupEntry (0, 2);
    ASSERT_NE (texture, nullptr);
    ASSERT_NE (comparison, nullptr);
    ASSERT_NE (filtering, nullptr);

    EXPECT_EQ (texture->textureSampleType, rive::ore::TextureSampleType::Depth);
    EXPECT_EQ (comparison->kind, rive::ore::ResourceKind::ComparisonSampler);
    EXPECT_EQ (filtering->kind, rive::ore::ResourceKind::Sampler);
}

TEST (ShaderBindingMapTests, StorageBufferIsEncoded)
{
    ShaderReflection refl;

    ShaderReflection::ResourceBinding sb;
    sb.name = "OutputBuffer";
    sb.set = 0;
    sb.binding = 0;
    sb.backendSlot = 0;
    refl.storageBuffers.push_back (sb);

    auto blob = makeShaderBindingMapBlob (refl, ShaderStage::vertex);
    EXPECT_FALSE (blob.empty());
}

TEST (ShaderBindingMapTests, StorageBufferKindFollowsReadOnly)
{
    ShaderReflection refl;

    ShaderReflection::ResourceBinding readOnly;
    readOnly.binding = 0;
    readOnly.backendSlot = 0;
    readOnly.readOnly = true;
    refl.storageBuffers.push_back (readOnly);

    ShaderReflection::ResourceBinding writable;
    writable.binding = 1;
    writable.backendSlot = 1;
    refl.storageBuffers.push_back (writable);

    const auto blob = makeShaderBindingMapBlob (refl, ShaderStage::fragment);

    rive::ore::BindingMap map;
    ASSERT_TRUE (rive::ore::BindingMap::fromBlob (blob.data(), blob.size(), &map));

    const auto* readOnlyEntry = map.lookupEntry (0, 0);
    const auto* writableEntry = map.lookupEntry (0, 1);
    ASSERT_NE (readOnlyEntry, nullptr);
    ASSERT_NE (writableEntry, nullptr);

    EXPECT_EQ (readOnlyEntry->kind, rive::ore::ResourceKind::StorageBufferRO);
    EXPECT_EQ (writableEntry->kind, rive::ore::ResourceKind::StorageBufferRW);
}

TEST (ShaderBindingMapTests, StorageImageIsEncoded)
{
    ShaderReflection refl;

    ShaderReflection::ResourceBinding si;
    si.name = "OutputImage";
    si.set = 0;
    si.binding = 0;
    si.backendSlot = 0;
    refl.storageImages.push_back (si);

    auto blob = makeShaderBindingMapBlob (refl, ShaderStage::fragment);
    EXPECT_FALSE (blob.empty());
}

TEST (ShaderBindingMapTests, ComputeStageUsesCorrectStageMask)
{
    ShaderReflection refl;

    ShaderReflection::ResourceBinding ub;
    ub.name = "Uniforms";
    ub.set = 0;
    ub.binding = 0;
    ub.backendSlot = 0;
    refl.uniformBuffers.push_back (ub);

    auto blob = makeShaderBindingMapBlob (refl, ShaderStage::compute);
    EXPECT_FALSE (blob.empty());
}

TEST (ShaderBindingMapTests, GLFixupBlobEmptyReflectionReturnsEmpty)
{
    ShaderReflection refl;
    auto blob = makeGLFixupBlob (refl);
    EXPECT_TRUE (blob.empty());
}

TEST (ShaderBindingMapTests, GLFixupBlobEncodesUniformBuffers)
{
    ShaderReflection refl;

    ShaderReflection::ResourceBinding ub;
    ub.name = "Uniforms";
    ub.set = 0;
    ub.binding = 0;
    ub.backendSlot = 0;
    refl.uniformBuffers.push_back (ub);

    auto blob = makeGLFixupBlob (refl);
    EXPECT_FALSE (blob.empty());

    // Version byte
    EXPECT_EQ (blob[0], 1u);
    // Entry count (uint16 LE) = 1
    EXPECT_EQ (blob[1], 1u);
    EXPECT_EQ (blob[2], 0u);
    // Kind = UBO block (0)
    EXPECT_EQ (blob[3], 0u);
    // Slot = 0
    EXPECT_EQ (blob[4], 0u);
    // Name length follows, after that the name bytes
}

TEST (ShaderBindingMapTests, GLFixupBlobEncodesCombinedSamplers)
{
    ShaderReflection refl;

    ShaderReflection::GLCombinedSampler cs;
    cs.name = "texSampler";
    cs.textureSlot = 2;
    refl.glCombinedSamplers.push_back (cs);

    auto blob = makeGLFixupBlob (refl);
    EXPECT_FALSE (blob.empty());

    // Kind = sampler uniform (1)
    EXPECT_EQ (blob[3], 1u);
    // Slot = 2
    EXPECT_EQ (blob[4], 2u);
}

TEST (ShaderBindingMapTests, GLFixupBlobEncodesBothUniformBuffersAndSamplers)
{
    ShaderReflection refl;

    ShaderReflection::ResourceBinding ub;
    ub.name = "Uniforms";
    ub.set = 0;
    ub.binding = 0;
    ub.backendSlot = 0;
    refl.uniformBuffers.push_back (ub);

    ShaderReflection::GLCombinedSampler cs;
    cs.name = "texSampler";
    cs.textureSlot = 1;
    refl.glCombinedSamplers.push_back (cs);

    auto blob = makeGLFixupBlob (refl);
    EXPECT_FALSE (blob.empty());

    // Entry count = 2
    EXPECT_EQ (blob[1], 2u);
    EXPECT_EQ (blob[2], 0u);
}

TEST (ShaderBindingMapTests, GLTextureSamplerPairsBlobIsEmptyWithoutCombinedSamplers)
{
    EXPECT_TRUE (makeGLTextureSamplerPairsBlob ({}).empty());
}

TEST (ShaderBindingMapTests, GLTextureSamplerPairsBlobListsEveryTextureOfASharedSampler)
{
    ShaderReflection refl;

    ShaderReflection::ResourceBinding shadow;
    shadow.name = "u_shadow";
    shadow.set = 1;
    shadow.binding = 3;
    refl.separateImages.push_back (shadow);

    ShaderReflection::ResourceBinding material = shadow;
    material.name = "u_material";
    material.set = 2;
    refl.separateImages.push_back (material);

    ShaderReflection::ResourceBinding pointClamp;
    pointClamp.name = "pointClamp";
    pointClamp.set = 0;
    pointClamp.binding = 3;
    refl.separateSamplers.push_back (pointClamp);

    refl.glCombinedSamplers = { { "yup_combined_u_shadow_pointClamp", 0 }, { "yup_combined_u_material_pointClamp", 1 } };

    const std::vector<uint8_t> expected { 1, 3, 0, 3, 2, 3, 0, 3 };
    EXPECT_EQ (makeGLTextureSamplerPairsBlob (refl), expected);
}

TEST (ShaderBindingMapTests, GLTextureSamplerPairsBlobMatchesUnnamedResourcesByTheirId)
{
    ShaderReflection refl;

    ShaderReflection::ResourceBinding image;
    image.resourceId = 12;
    image.set = 0;
    image.binding = 1;
    refl.separateImages.push_back (image);

    ShaderReflection::ResourceBinding sampler;
    sampler.resourceId = 13;
    sampler.set = 0;
    sampler.binding = 2;
    refl.separateSamplers.push_back (sampler);

    refl.glCombinedSamplers = { { "yup_combined_img12_samp13", 0 } };

    const std::vector<uint8_t> expected { 0, 1, 0, 2 };
    EXPECT_EQ (makeGLTextureSamplerPairsBlob (refl), expected);
}

#if YUP_ENABLE_SHADER_TRANSPILER

//==============================================================================
// GL - samplers of a graphics pipeline with several bind groups
//==============================================================================

class GpuPipelineGLSamplerTests : public ::testing::Test
{
protected:
    /** Declares a sampler it never uses. */
    static constexpr const char* vertexSource = R"(#version 450
layout(set = 0, binding = 0) uniform ViewBlock { vec4 value; } view;
layout(set = 0, binding = 4) uniform sampler unusedVertexSampler;
void main() { gl_Position = view.value; })";

    /** pointClamp serves the textures of sets 1 and 2; s_unused is never used. */
    static constexpr const char* fragmentSource = R"(#version 450
layout(set = 0, binding = 3) uniform sampler pointClamp;
layout(set = 1, binding = 3) uniform texture2D u_shadow;
layout(set = 2, binding = 3) uniform texture2D u_material;
layout(set = 2, binding = 7) uniform sampler s_unused;
layout(location = 0) out vec4 color;
void main()
{
    color = texture (sampler2D (u_shadow, pointClamp), vec2 (0.5))
          + texture (sampler2D (u_material, pointClamp), vec2 (0.5));
})";

    /** Names whose combined uniform holds "__", which spirv-cross collapses when it compiles. */
    static constexpr const char* underscoreFragmentSource = R"(#version 450
layout(set = 0, binding = 3) uniform sampler _pointClamp;
layout(set = 1, binding = 3) uniform texture2D u_shadow_;
layout(set = 2, binding = 3) uniform texture2D u_material;
layout(location = 0) out vec4 color;
void main()
{
    color = texture (sampler2D (u_shadow_, _pointClamp), vec2 (0.5))
          + texture (sampler2D (u_material, _pointClamp), vec2 (0.5));
})";

    static ResultValue<ShaderBundle> compileEssl (const char* fragment = fragmentSource)
    {
        ShaderBundleCompileRequest request;

        for (const auto& [stage, source] : { std::pair (ShaderStage::vertex, vertexSource), std::pair (ShaderStage::fragment, fragment) })
        {
            ShaderBundleEntry entry;
            entry.stage = stage;
            entry.source = source;
            entry.targetLanguages = { ShaderLanguage::essl };
            request.entries.push_back (entry);
        }

        return ShaderBundleCompiler().compile (request);
    }

    static const ShaderReflection::ResourceBinding* find (const std::vector<ShaderReflection::ResourceBinding>& resources, const char* name)
    {
        for (const auto& r : resources)
        {
            if (r.name == name)
                return &r;
        }

        return nullptr;
    }
};

TEST_F (GpuPipelineGLSamplerTests, ASharedSamplerIsPairedWithEachOfItsTextures)
{
    const auto result = compileEssl();
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& bundle = result.getReference();
    const auto* fs = bundle.findShader (ShaderStage::fragment, ShaderLanguage::essl);
    ASSERT_NE (fs, nullptr);

    const auto* shadow = find (fs->reflection.separateImages, "u_shadow");
    const auto* material = find (fs->reflection.separateImages, "u_material");
    const auto* pointClamp = find (fs->reflection.separateSamplers, "pointClamp");
    ASSERT_NE (shadow, nullptr);
    ASSERT_NE (material, nullptr);
    ASSERT_NE (pointClamp, nullptr);
    EXPECT_NE (shadow->backendSlot, material->backendSlot);

    std::set<uint32_t> combinedUnits;
    for (const auto& cs : fs->reflection.glCombinedSamplers)
        combinedUnits.insert (cs.textureSlot);

    EXPECT_EQ (combinedUnits, (std::set<uint32_t> { shadow->backendSlot, material->backendSlot }));
    EXPECT_EQ (combinedUnits.count (pointClamp->backendSlot), 1u);

    const auto pairs = makeGLTextureSamplerPairsBlob (fs->reflection);
    ASSERT_EQ (pairs.size(), 8u);

    std::set<std::vector<uint8_t>> pairSet { { pairs.begin(), pairs.begin() + 4 }, { pairs.begin() + 4, pairs.end() } };
    EXPECT_EQ (pairSet, (std::set<std::vector<uint8_t>> { { 1, 3, 0, 3 }, { 2, 3, 0, 3 } }));
}

TEST_F (GpuPipelineGLSamplerTests, PairsMatchCombinedNamesSpirvCrossSanitized)
{
    const auto result = compileEssl (underscoreFragmentSource);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto* fs = result.getReference().findShader (ShaderStage::fragment, ShaderLanguage::essl);
    ASSERT_NE (fs, nullptr);
    ASSERT_EQ (fs->reflection.glCombinedSamplers.size(), 2u);

    const auto pairs = makeGLTextureSamplerPairsBlob (fs->reflection);
    EXPECT_EQ (pairs.size(), 4u * fs->reflection.glCombinedSamplers.size());
}

TEST_F (GpuPipelineGLSamplerTests, UnpairedSamplersHaveNoSlot)
{
    const auto result = compileEssl();
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& bundle = result.getReference();
    const auto* vs = bundle.findShader (ShaderStage::vertex, ShaderLanguage::essl);
    const auto* fs = bundle.findShader (ShaderStage::fragment, ShaderLanguage::essl);
    ASSERT_NE (vs, nullptr);
    ASSERT_NE (fs, nullptr);

    const auto* unusedVertex = find (vs->reflection.separateSamplers, "unusedVertexSampler");
    const auto* unusedFragment = find (fs->reflection.separateSamplers, "s_unused");
    ASSERT_NE (unusedVertex, nullptr);
    ASSERT_NE (unusedFragment, nullptr);
    EXPECT_EQ (unusedVertex->backendSlot, ~0u);
    EXPECT_EQ (unusedFragment->backendSlot, ~0u);
    EXPECT_TRUE (makeGLTextureSamplerPairsBlob (vs->reflection).empty());

    // Both stages' binding maps report the unpaired samplers absent, so ore never binds them on a texture's unit
    const auto vsBlob = makeShaderBindingMapBlob (vs->reflection, ShaderStage::vertex);
    const auto fsBlob = makeShaderBindingMapBlob (fs->reflection, ShaderStage::fragment);

    rive::ore::BindingMap vsMap, fsMap;
    ASSERT_TRUE (rive::ore::BindingMap::fromBlob (vsBlob.data(), vsBlob.size(), &vsMap));
    ASSERT_TRUE (rive::ore::BindingMap::fromBlob (fsBlob.data(), fsBlob.size(), &fsMap));

    const auto* vertexEntry = vsMap.lookupEntry (0, 4);
    const auto* fragmentEntry = fsMap.lookupEntry (2, 7);
    ASSERT_NE (vertexEntry, nullptr);
    ASSERT_NE (fragmentEntry, nullptr);
    EXPECT_EQ (vertexEntry->backendSlot[0], rive::ore::BindingMap::kAbsent);
    EXPECT_EQ (fragmentEntry->backendSlot[1], rive::ore::BindingMap::kAbsent);
}

#endif

// ---------------------------------------------------------------------------
// GpuBuffer::create — validation and null paths

TEST_F (GpuPipelineTests, GpuBufferCreateHeadlessReturnsNull)
{
    const float verts[] = { 0.0f, 1.0f, 2.0f };
    // Headless backend has no ore context — creation fails gracefully.
    EXPECT_EQ (GpuBuffer::create (*context, GpuBufferType::vertex, verts, sizeof (verts)), nullptr);
}

TEST (GpuBufferDefaults, DefaultPtrIsNull)
{
    GpuBuffer::Ptr b;
    EXPECT_EQ (b, nullptr);
}

// ---------------------------------------------------------------------------
// GpuDevice::updateBuffer — base contract (default returns false)

TEST_F (GpuPipelineTests, UpdateBufferWithNullBufferReturnsFalse)
{
    const float data[] = { 1.0f, 2.0f };
    EXPECT_FALSE (context->updateBuffer (nullptr, data, sizeof (data)));
}

TEST_F (GpuPipelineTests, UpdateBufferWithNullDataReturnsFalse)
{
    EXPECT_FALSE (context->updateBuffer (nullptr, nullptr, 16));
}

TEST_F (GpuPipelineTests, UpdateBufferWithZeroSizeReturnsFalse)
{
    const float data[] = { 1.0f };
    EXPECT_FALSE (context->updateBuffer (nullptr, data, 0));
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
    vs.bindingMap = std::vector<uint8> (map, map + sizeof (map));

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

    GpuPipelineOptions options;
    options.vertexBuffers.emplace_back (12, GpuVertexStepMode::vertex,
                                        std::vector<GpuVertexAttribute> { { GpuVertexFormat::float3, 0, 0 } });
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
        context = GpuDevice::create (GpuPlatform::Headless, {});
        ASSERT_NE (context, nullptr);
    }

    GpuDevice::Ptr context;
};

TEST_F (GpuPipelineCacheTests, SameInputsProduceSameKey)
{
    ShaderBundle bundle;
    GpuPipelineOptions options;

    const auto key1 = GpuPipelineCache::generateCacheKey (bundle, options, GpuPlatform::Metal);
    const auto key2 = GpuPipelineCache::generateCacheKey (bundle, options, GpuPlatform::Metal);
    EXPECT_EQ (key1, key2);
}

TEST_F (GpuPipelineCacheTests, DifferentApiProducesDifferentKey)
{
    ShaderBundle bundle;
    GpuPipelineOptions options;

    const auto keyMetal = GpuPipelineCache::generateCacheKey (bundle, options, GpuPlatform::Metal);
    const auto keyD3D = GpuPipelineCache::generateCacheKey (bundle, options, GpuPlatform::Direct3D);
    EXPECT_NE (keyMetal, keyD3D);
}

TEST_F (GpuPipelineCacheTests, VulkanKeyTellsSpirvModulesApart)
{
    // SPIR-V variants carry no source text, only their module bytes differ
    auto makeBundle = [] (uint8 firstByte)
    {
        ShaderBundle bundle;

        for (const auto stage : { ShaderStage::vertex, ShaderStage::fragment })
        {
            ShaderInfo info;
            info.stage = stage;
            info.language = ShaderLanguage::spirv;
            info.entryPoint = "main";
            info.binary.setSize (8, true);
            static_cast<uint8*> (info.binary.getData())[0] = firstByte;
            bundle.addShader (std::move (info));
        }

        return bundle;
    };

    GpuPipelineOptions options;

    const auto keyA = GpuPipelineCache::generateCacheKey (makeBundle (1), options, GpuPlatform::Vulkan);
    const auto keyB = GpuPipelineCache::generateCacheKey (makeBundle (2), options, GpuPlatform::Vulkan);
    const auto keyA2 = GpuPipelineCache::generateCacheKey (makeBundle (1), options, GpuPlatform::Vulkan);

    EXPECT_NE (keyA, keyB);
    EXPECT_EQ (keyA, keyA2);
}

TEST_F (GpuPipelineCacheTests, DifferentOptionsProduceDifferentKey)
{
    ShaderBundle bundle;

    GpuPipelineOptions a;
    GpuPipelineOptions b;
    b.cullMode = GpuCullMode::back;

    const auto keyA = GpuPipelineCache::generateCacheKey (bundle, a, GpuPlatform::Metal);
    const auto keyB = GpuPipelineCache::generateCacheKey (bundle, b, GpuPlatform::Metal);
    EXPECT_NE (keyA, keyB);
}

TEST_F (GpuPipelineCacheTests, DepthBiasProducesDifferentKeys)
{
    ShaderBundle bundle;

    GpuPipelineOptions base;
    base.depthStencil.enabled = true;

    auto withBias = base;
    withBias.depthStencil.depthBias = 2;

    auto withSlope = base;
    withSlope.depthStencil.depthBiasSlopeScale = 1.0f;

    auto withClamp = base;
    withClamp.depthStencil.depthBiasClamp = 0.01f;

    const auto key = GpuPipelineCache::generateCacheKey (bundle, base, GpuPlatform::Metal);
    EXPECT_NE (key, GpuPipelineCache::generateCacheKey (bundle, withBias, GpuPlatform::Metal));
    EXPECT_NE (key, GpuPipelineCache::generateCacheKey (bundle, withSlope, GpuPlatform::Metal));
    EXPECT_NE (key, GpuPipelineCache::generateCacheKey (bundle, withClamp, GpuPlatform::Metal));
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

TEST_F (GpuPipelineCacheTests, EmptyBundleGeneratesKeyWithNoneMarkers)
{
    ShaderBundle bundle;
    GpuPipelineOptions options;

    const auto key = GpuPipelineCache::generateCacheKey (bundle, options, GpuPlatform::Metal);
    EXPECT_FALSE (key.isEmpty());
}

TEST_F (GpuPipelineCacheTests, OpenGLES_ApisGenerateDifferentKeys)
{
    ShaderBundle bundle;
    GpuPipelineOptions options;

    const auto keyGL = GpuPipelineCache::generateCacheKey (bundle, options, GpuPlatform::OpenGL);
    const auto keyGLES = GpuPipelineCache::generateCacheKey (bundle, options, GpuPlatform::OpenGLES);
    EXPECT_NE (keyGL, keyGLES);
}

TEST_F (GpuPipelineCacheTests, WebGPU_GeneratesKey)
{
    ShaderBundle bundle;
    GpuPipelineOptions options;

    const auto key = GpuPipelineCache::generateCacheKey (bundle, options, GpuPlatform::WebGPU);
    EXPECT_FALSE (key.isEmpty());
}

TEST_F (GpuPipelineCacheTests, ESSL_FallbackUsesGlslWhenEsslMissing)
{
    ShaderBundle bundle;

    ShaderInfo vsGlsl;
    vsGlsl.stage = ShaderStage::vertex;
    vsGlsl.language = ShaderLanguage::glsl;
    vsGlsl.entryPoint = "main";
    vsGlsl.source = "// glsl vertex";
    bundle.addShader (vsGlsl);

    ShaderInfo fsGlsl;
    fsGlsl.stage = ShaderStage::fragment;
    fsGlsl.language = ShaderLanguage::glsl;
    fsGlsl.entryPoint = "main";
    fsGlsl.source = "// glsl fragment";
    bundle.addShader (fsGlsl);

    GpuPipelineOptions options;

    // With ESSL API, the cache should fall back to GLSL since no ESSL variant exists.
    const auto keyESSL = GpuPipelineCache::generateCacheKey (bundle, options, GpuPlatform::OpenGLES);
    EXPECT_FALSE (keyESSL.isEmpty());
}

TEST_F (GpuPipelineCacheTests, ESSL_FallbackUsesEsslWhenAvailable)
{
    ShaderBundle bundle;

    ShaderInfo vsGlsl;
    vsGlsl.stage = ShaderStage::vertex;
    vsGlsl.language = ShaderLanguage::glsl;
    vsGlsl.entryPoint = "main";
    vsGlsl.source = "// glsl vertex";
    bundle.addShader (vsGlsl);

    ShaderInfo vsEssl;
    vsEssl.stage = ShaderStage::vertex;
    vsEssl.language = ShaderLanguage::essl;
    vsEssl.entryPoint = "main";
    vsEssl.source = "// essl vertex";
    bundle.addShader (vsEssl);

    ShaderInfo fsEssl;
    fsEssl.stage = ShaderStage::fragment;
    fsEssl.language = ShaderLanguage::essl;
    fsEssl.entryPoint = "main";
    fsEssl.source = "// essl fragment";
    bundle.addShader (fsEssl);

    GpuPipelineOptions options;

    const auto key = GpuPipelineCache::generateCacheKey (bundle, options, GpuPlatform::OpenGLES);
    EXPECT_FALSE (key.isEmpty());
}

TEST_F (GpuPipelineCacheTests, KeyChangesWhenShaderSourceDiffers)
{
    ShaderBundle bundleA;
    ShaderInfo vsA;
    vsA.stage = ShaderStage::vertex;
    vsA.language = ShaderLanguage::glsl;
    vsA.entryPoint = "main";
    vsA.source = "// source A";
    bundleA.addShader (vsA);

    ShaderBundle bundleB;
    ShaderInfo vsB;
    vsB.stage = ShaderStage::vertex;
    vsB.language = ShaderLanguage::glsl;
    vsB.entryPoint = "main";
    vsB.source = "// source B";
    bundleB.addShader (vsB);

    GpuPipelineOptions options;

    const auto keyA = GpuPipelineCache::generateCacheKey (bundleA, options, GpuPlatform::OpenGL);
    const auto keyB = GpuPipelineCache::generateCacheKey (bundleB, options, GpuPlatform::OpenGL);
    EXPECT_NE (keyA, keyB);
}

TEST_F (GpuPipelineCacheTests, KeyChangesWhenEntryPointDiffers)
{
    ShaderBundle bundleA;
    ShaderInfo vsA;
    vsA.stage = ShaderStage::vertex;
    vsA.language = ShaderLanguage::glsl;
    vsA.entryPoint = "vsMain";
    vsA.source = "// source";
    bundleA.addShader (vsA);

    ShaderBundle bundleB;
    ShaderInfo vsB;
    vsB.stage = ShaderStage::vertex;
    vsB.language = ShaderLanguage::glsl;
    vsB.entryPoint = "vsMain2";
    vsB.source = "// source";
    bundleB.addShader (vsB);

    GpuPipelineOptions options;

    const auto keyA = GpuPipelineCache::generateCacheKey (bundleA, options, GpuPlatform::OpenGL);
    const auto keyB = GpuPipelineCache::generateCacheKey (bundleB, options, GpuPlatform::OpenGL);
    EXPECT_NE (keyA, keyB);
}

TEST_F (GpuPipelineCacheTests, MaxEntriesZeroNeverEvicts)
{
    GpuPipelineCache cache (*context);
    cache.setMaxEntries (0);
    EXPECT_EQ (cache.getMaxEntries(), 0u);

    cache.store ("a", nullptr);
    cache.store ("b", nullptr);
    cache.store ("c", nullptr);

    EXPECT_EQ (cache.getNumEntries(), 3u);
}

TEST_F (GpuPipelineCacheTests, EvictsOldestEntry)
{
    GpuPipelineCache cache (*context);
    cache.setMaxEntries (2);

    cache.store ("a", nullptr);

    // Access "a" so it's not the oldest.
    EXPECT_TRUE (cache.contains ("a"));

    cache.store ("b", nullptr);

    // Now "a" was accessed before "b" was added, so "a" should be oldest.
    cache.store ("c", nullptr);

    EXPECT_EQ (cache.getNumEntries(), 2u);
}

TEST_F (GpuPipelineCacheTests, CacheHitReturnsStoredPipeline)
{
    GpuPipelineCache cache (*context);

    cache.store ("key", nullptr);

    ShaderBundle bundle;
    GpuPipelineOptions options;

    auto result = cache.getOrCompile ("key", bundle, options);
    EXPECT_TRUE (result.wasOk());
    EXPECT_EQ (result.getValue(), nullptr);
}

TEST_F (GpuPipelineCacheTests, GetOrCompileWithExplicitKeyStoresCompiledPipeline)
{
    GpuPipelineCache cache (*context);

    ShaderBundle bundle;
    GpuPipelineOptions options;

    const auto key = GpuPipelineCache::generateCacheKey (bundle, options, context->getPlatform());
    auto result = cache.getOrCompile (key, bundle, options);
    // Headless compile fails, but we verify the path doesn't crash.
    EXPECT_TRUE (result.failed());
}

// ---------------------------------------------------------------------------
// GpuFrame — headless invalid, RAII submit idempotency

TEST_F (GpuPipelineTests, GpuFrameHeadlessIsInvalid)
{
    auto frame = GpuFrame::begin (context);
    EXPECT_FALSE (frame.isValid());
    EXPECT_FALSE (frame.submit());
}

TEST_F (GpuPipelineTests, GpuFrameSubmitIsIdempotentOnInvalid)
{
    auto frame = GpuFrame::begin (context);
    EXPECT_FALSE (frame.submit());
    EXPECT_FALSE (frame.submit());
    EXPECT_NO_THROW (frame.waitForGPU());
}

TEST_F (GpuPipelineTests, GpuFrameMoveAssignmentMovesInvalidState)
{
    auto src = GpuFrame::begin (context);
    EXPECT_FALSE (src.isValid());

    auto dst = GpuFrame::begin (context);
    dst = std::move (src);

    EXPECT_FALSE (dst.isValid());
    EXPECT_FALSE (dst.submit());
}

TEST_F (GpuPipelineTests, GpuFrameMoveConstructionFromInvalidIsInvalid)
{
    auto src = GpuFrame::begin (context);
    GpuFrame dst (std::move (src));

    EXPECT_FALSE (dst.isValid());
    EXPECT_FALSE (dst.submit());
}

TEST_F (GpuPipelineTests, GpuFrameDestructorDoesNotCrashOnInvalid)
{
    {
        auto frame = GpuFrame::begin (context);
        EXPECT_FALSE (frame.isValid());
    }
    // Destructor calls submit() which is idempotent.
    EXPECT_TRUE (true);
}

//==============================================================================
// Metal - a graphics pipeline whose bind groups reuse binding numbers
//==============================================================================

#if YUP_APPLE && YUP_ENABLE_SHADER_TRANSPILER

class GpuPipelineMultiSetMetalTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr || ! device->isGpuAvailable())
            GTEST_SKIP() << "No Metal device available";
    }

    /** Reads ViewBlock and DrawBlock, which share binding 0 in sets 0 and 2, into a flat varying. */
    static constexpr const char* vertexSource = R"(#version 450
layout(set = 0, binding = 0) uniform ViewBlock { vec4 value; } view;
layout(set = 2, binding = 0) uniform DrawBlock { vec4 value; } draw;
layout(location = 0) out vec2 uv;
layout(location = 1) flat out vec2 vertexValues;
void main()
{
    uv = vec2 ((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    vertexValues = vec2 (view.value.x, draw.value.x);
    gl_Position = vec4 (uv * 2.0 - 1.0, 0.0, 1.0);
})";

    /** Writes four vertical stripes: the vertex values with ViewBlock, then LightingBlock with
        DrawBlock, then the set 1 texture, then the set 2 texture. The three blocks all sit at
        binding 0, and both textures at binding 3. */
    static constexpr const char* fragmentSource = R"(#version 450
layout(set = 0, binding = 0) uniform ViewBlock { vec4 value; } view;
layout(set = 0, binding = 1) uniform sampler pointClamp;
layout(set = 1, binding = 0) uniform LightingBlock { vec4 value; } lighting;
layout(set = 1, binding = 3) uniform texture2D u_shadow;
layout(set = 2, binding = 0) uniform DrawBlock { vec4 value; } draw;
layout(set = 2, binding = 3) uniform texture2D u_material;
layout(location = 0) in vec2 uv;
layout(location = 1) flat in vec2 vertexValues;
layout(location = 0) out vec4 color;
void main()
{
    if (uv.x < 0.25)
        color = vec4 (vertexValues, view.value.x, 1.0);
    else if (uv.x < 0.5)
        color = vec4 (lighting.value.x, draw.value.x, 0.0, 1.0);
    else if (uv.x < 0.75)
        color = vec4 (texture (sampler2D (u_shadow, pointClamp), vec2 (0.5)).rgb, 1.0);
    else
        color = vec4 (texture (sampler2D (u_material, pointClamp), vec2 (0.5)).rgb, 1.0);
})";

    /** A fullscreen triangle declaring no resources, so the fragment shader alone decides the native slots. */
    static constexpr const char* plainVertexSource = R"(#version 450
layout(location = 0) out vec2 uv;
void main()
{
    uv = vec2 ((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4 (uv * 2.0 - 1.0, 0.0, 1.0);
})";

    GpuPipeline::Ptr compileWithPlainVertex (const char* fragment)
    {
        GpuPipelineOptions options;
        options.colorTargets.emplace_back().format = GpuTextureFormat::rgba8unorm;

        auto pipeline = GpuPipeline::compileFromGlsl (device, plainVertexSource, fragment, options);
        EXPECT_TRUE (pipeline.wasOk()) << pipeline.getErrorMessage();
        return pipeline.wasOk() ? pipeline.getValue() : nullptr;
    }

    GpuTexture::Ptr makeSolidTexture (uint8 red, uint8 green, uint8 blue)
    {
        auto texture = GpuTexture::create (device, GpuTextureDesc (2, 2, GpuTextureFormat::rgba8unorm));
        if (texture == nullptr)
            return nullptr;

        std::vector<uint8> texels;
        for (int i = 0; i < 4; ++i)
            texels.insert (texels.end(), { red, green, blue, 255 });

        GpuTextureDataDesc data;
        data.data = texels.data();
        return texture->upload (data) ? texture : nullptr;
    }

    static void expectPixel (const std::vector<uint8>& pixels, int column, int red, int green, int blue)
    {
        const auto* pixel = pixels.data() + column * 4;
        EXPECT_NEAR (pixel[0], red, 2) << "stripe " << column << " red";
        EXPECT_NEAR (pixel[1], green, 2) << "stripe " << column << " green";
        EXPECT_NEAR (pixel[2], blue, 2) << "stripe " << column << " blue";
    }

    GpuDevice::Ptr device;
};

TEST_F (GpuPipelineMultiSetMetalTests, KeepsEqualBindingNumbersOfDifferentSetsApart)
{
    constexpr int size = 4;
    auto target = GpuTarget::create (device, size, size);
    ASSERT_NE (target, nullptr);

    GpuPipelineOptions options;
    options.colorTargets.emplace_back().format = GpuTextureFormat::rgba8unorm;

    auto pipeline = GpuPipeline::compileFromGlsl (device, vertexSource, fragmentSource, options);
    ASSERT_TRUE (pipeline.wasOk()) << pipeline.getErrorMessage();

    auto shadow = makeSolidTexture (64, 128, 192);
    auto material = makeSolidTexture (192, 32, 96);
    ASSERT_NE (shadow, nullptr);
    ASSERT_NE (material, nullptr);

    const float viewValue[4] = { 0.2f, 0.2f, 0.2f, 0.2f };
    const float lightingValue[4] = { 0.4f, 0.4f, 0.4f, 0.4f };
    const float drawValue[4] = { 0.6f, 0.6f, 0.6f, 0.6f };

    auto frame = GpuFrame::begin (device);
    auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
    pass.setPipeline (pipeline.getValue());
    pass.setUniformBuffer (0, 0, viewValue, sizeof (viewValue));
    pass.setUniformBuffer (1, 0, lightingValue, sizeof (lightingValue));
    pass.setUniformBuffer (2, 0, drawValue, sizeof (drawValue));
    pass.setTexture (1, 3, shadow);
    pass.setTexture (2, 3, material);
    pass.draw (3);
    pass.finish();
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    std::vector<uint8> pixels (size * size * 4);
    ASSERT_TRUE (target->readPixels (pixels.data(), pixels.size()));

    expectPixel (pixels, 0, 51, 153, 51);
    expectPixel (pixels, 1, 102, 153, 0);
    expectPixel (pixels, 2, 64, 128, 192);
    expectPixel (pixels, 3, 192, 32, 96);
}

TEST_F (GpuPipelineMultiSetMetalTests, PipelinesSharingSetZeroDrawFromOneBindGroup)
{
    constexpr int size = 4;
    auto target = GpuTarget::create (device, size, size);
    ASSERT_NE (target, nullptr);

    // Set 0, including its default sampler, is the same in both; set 2 differs
    auto first = compileWithPlainVertex (R"(#version 450
layout(set = 0, binding = 0) uniform ViewBlock { vec4 value; } view;
layout(set = 0, binding = 1) uniform texture2D u_view;
layout(set = 0, binding = 2) uniform sampler linearClamp;
layout(set = 2, binding = 0) uniform DrawBlock { vec4 value; } draw;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    color = vec4 (view.value.x, texture (sampler2D (u_view, linearClamp), vec2 (0.5)).g, draw.value.x, 1.0);
})");

    auto second = compileWithPlainVertex (R"(#version 450
layout(set = 0, binding = 0) uniform ViewBlock { vec4 value; } view;
layout(set = 0, binding = 1) uniform texture2D u_view;
layout(set = 0, binding = 2) uniform sampler linearClamp;
layout(set = 2, binding = 0) uniform DrawBlock { vec4 value; } draw;
layout(set = 2, binding = 1) uniform texture2D u_material;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    color = vec4 (view.value.x,
                  texture (sampler2D (u_view, linearClamp), vec2 (0.5)).g,
                  texture (sampler2D (u_material, linearClamp), vec2 (0.5)).b * draw.value.y,
                  1.0);
})");

    ASSERT_NE (first, nullptr);
    ASSERT_NE (second, nullptr);

    auto viewTexture = makeSolidTexture (0, 128, 0);
    auto material = makeSolidTexture (0, 0, 200);
    ASSERT_NE (viewTexture, nullptr);
    ASSERT_NE (material, nullptr);

    const float viewValue[4] = { 0.2f, 0.2f, 0.2f, 0.2f };
    const float drawValue[4] = { 0.6f, 1.0f, 0.0f, 0.0f };

    auto frame = GpuFrame::begin (device);
    auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
    pass.setPipeline (first);
    pass.setUniformBuffer (0, 0, viewValue, sizeof (viewValue));
    pass.setTexture (0, 1, viewTexture);
    pass.setUniformBuffer (2, 0, drawValue, sizeof (drawValue));
    pass.setViewport (0.0f, 0.0f, size / 2.0f, (float) size);
    pass.draw (3);

    pass.setPipeline (second);
    pass.setTexture (2, 1, material);
    pass.setViewport (size / 2.0f, 0.0f, size / 2.0f, (float) size);
    pass.draw (3);
    pass.finish();
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    std::vector<uint8> pixels (size * size * 4);
    ASSERT_TRUE (target->readPixels (pixels.data(), pixels.size()));

    expectPixel (pixels, 0, 51, 128, 153);
    expectPixel (pixels, 3, 51, 128, 200);
}

TEST_F (GpuPipelineMultiSetMetalTests, SwitchingToAPipelineWithOtherSetLayoutsRebuildsThem)
{
    constexpr int size = 4;
    auto target = GpuTarget::create (device, size, size);
    ASSERT_NE (target, nullptr);

    auto first = compileWithPlainVertex (R"(#version 450
layout(set = 0, binding = 0) uniform ViewBlock { vec4 value; } view;
layout(set = 1, binding = 0) uniform LightingBlock { vec4 value; } lighting;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    color = vec4 (view.value.x, lighting.value.x, 0.0, 1.0);
})");

    // Set 0 gains a block, which also moves the native slot of the otherwise equal set 1
    auto second = compileWithPlainVertex (R"(#version 450
layout(set = 0, binding = 0) uniform ViewBlock { vec4 value; } view;
layout(set = 0, binding = 1) uniform TintBlock { vec4 value; } tint;
layout(set = 1, binding = 0) uniform LightingBlock { vec4 value; } lighting;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    color = vec4 (view.value.x, lighting.value.x, tint.value.x, 1.0);
})");

    ASSERT_NE (first, nullptr);
    ASSERT_NE (second, nullptr);

    const float viewValue[4] = { 0.2f, 0.2f, 0.2f, 0.2f };
    const float lightingValue[4] = { 0.4f, 0.4f, 0.4f, 0.4f };
    const float tintValue[4] = { 0.8f, 0.8f, 0.8f, 0.8f };

    // Every binding is set up front: only the pipeline switch can bring the tint into a bind group
    auto frame = GpuFrame::begin (device);
    auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
    pass.setPipeline (first);
    pass.setUniformBuffer (0, 0, viewValue, sizeof (viewValue));
    pass.setUniformBuffer (0, 1, tintValue, sizeof (tintValue));
    pass.setUniformBuffer (1, 0, lightingValue, sizeof (lightingValue));
    pass.setViewport (0.0f, 0.0f, size / 2.0f, (float) size);
    pass.draw (3);

    pass.setPipeline (second);
    pass.setViewport (size / 2.0f, 0.0f, size / 2.0f, (float) size);
    pass.draw (3);
    pass.finish();
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    std::vector<uint8> pixels (size * size * 4);
    ASSERT_TRUE (target->readPixels (pixels.data(), pixels.size()));

    expectPixel (pixels, 0, 51, 102, 0);
    expectPixel (pixels, 3, 51, 102, 204);
}

#endif
