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
namespace
{

constexpr const char* kTestVertex = R"glsl(
#version 450
void main()
{
    gl_Position = vec4(0.0);
}
)glsl";

constexpr const char* kTestFragment = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
void main()
{
    outColor = vec4(1.0);
}
)glsl";

constexpr const char* kAnotherShader = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
void main()
{
    outColor = vec4(0.5);
}
)glsl";

} // namespace

//==============================================================================
// ShaderCache tests
//==============================================================================

class ShaderCacheTests : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        transpiler = new ShaderTranspiler();
        ASSERT_NE (transpiler, nullptr);
    }

    static void TearDownTestSuite()
    {
        transpiler = nullptr;
    }

    void SetUp() override
    {
        cache = std::make_unique<ShaderCache> (*transpiler);
    }

    void TearDown() override
    {
        cache = nullptr;
    }

    static ShaderTranspiler::Ptr transpiler;
    std::unique_ptr<ShaderCache> cache;
};

ShaderTranspiler::Ptr ShaderCacheTests::transpiler {};

//==============================================================================
// getOrCompile
//==============================================================================

TEST_F (ShaderCacheTests, GetOrCompile_MissReturnsCompiledSPIRV)
{
    auto key = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);

    auto result = cache->getOrCompile (
        key, kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_GT (result.getValue().getSize(), sizeof (uint32_t) * 5u);
}

TEST_F (ShaderCacheTests, GetOrCompile_HitReturnsCachedSPIRV)
{
    auto key = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);

    auto first = cache->getOrCompile (
        key, kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    ASSERT_TRUE (first.wasOk());

    auto second = cache->getOrCompile (
        key, kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    ASSERT_TRUE (second.wasOk());

    EXPECT_EQ (1u, cache->getNumEntries());
    EXPECT_EQ (first.getValue().getSize(), second.getValue().getSize());
}

TEST_F (ShaderCacheTests, GetOrCompile_DifferentKeysProduceDifferentEntries)
{
    auto key1 = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    auto key2 = ShaderCache::generateCacheKey (
        kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl);

    auto r1 = cache->getOrCompile (
        key1, kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    auto r2 = cache->getOrCompile (
        key2, kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (r1.wasOk());
    ASSERT_TRUE (r2.wasOk());
    EXPECT_EQ (2u, cache->getNumEntries());
}

TEST_F (ShaderCacheTests, GetOrCompile_InvalidSourceReturnsError)
{
    auto key = ShaderCache::generateCacheKey (
        "", ShaderStage::vertex, ShaderLanguage::glsl);

    auto result = cache->getOrCompile (
        key, "", ShaderStage::vertex, ShaderLanguage::glsl);

    EXPECT_TRUE (result.failed());
    EXPECT_EQ (0u, cache->getNumEntries());
}

//==============================================================================
// getOrTranspile
//==============================================================================

TEST_F (ShaderCacheTests, GetOrTranspile_ReturnsTranspiledCode)
{
    auto key = ShaderCache::generateCacheKey (
        kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl);

    auto result = cache->getOrTranspile (
        key, kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_FALSE (result.getValue().isEmpty());
    EXPECT_EQ (1u, cache->getNumEntries());
}

TEST_F (ShaderCacheTests, GetOrTranspile_HitDoesNotRecompile)
{
    auto key = ShaderCache::generateCacheKey (
        kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl);

    auto first = cache->getOrTranspile (
        key, kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::msl);
    ASSERT_TRUE (first.wasOk());

    auto second = cache->getOrTranspile (
        key, kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::msl);
    ASSERT_TRUE (second.wasOk());

    EXPECT_EQ (1u, cache->getNumEntries());
    EXPECT_EQ (first.getValue(), second.getValue());
}

TEST_F (ShaderCacheTests, GetOrTranspile_InvalidSourceFails)
{
    auto key = ShaderCache::generateCacheKey (
        "", ShaderStage::vertex, ShaderLanguage::glsl);

    auto result = cache->getOrTranspile (
        key, "", ShaderStage::vertex, ShaderLanguage::glsl, ShaderLanguage::glsl);

    EXPECT_TRUE (result.failed());
    EXPECT_EQ (0u, cache->getNumEntries());
}

//==============================================================================
// MSL-specific getOrTranspile
//==============================================================================

TEST_F (ShaderCacheTests, GetOrTranspile_ToMSLVertex)
{
    auto key = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);

    auto result = cache->getOrTranspile (
        key, kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl, ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_TRUE (result.getValue().contains ("vertex"));
}

TEST_F (ShaderCacheTests, GetOrTranspile_ToMSLFragment)
{
    auto key = ShaderCache::generateCacheKey (
        kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl);

    auto result = cache->getOrTranspile (
        key, kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_TRUE (result.getValue().contains ("fragment"));
}

TEST_F (ShaderCacheTests, GetOrTranspile_MSLWithFlipVertY)
{
    TranspileOptions opts;
    opts.flipVertY = true;

    auto key = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl, opts);

    auto result = cache->getOrTranspile (
        key, kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl, ShaderLanguage::msl, opts);

    ASSERT_TRUE (result.wasOk());
    EXPECT_FALSE (result.getValue().isEmpty());
}

TEST_F (ShaderCacheTests, GetOrTranspile_MSLWithFramebufferFetch)
{
    TranspileOptions opts;
    opts.mslUsesFramebufferFetch = true;

    auto key = ShaderCache::generateCacheKey (
        kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl, opts);

    auto result = cache->getOrTranspile (
        key, kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::msl, opts);

    ASSERT_TRUE (result.wasOk());
    EXPECT_FALSE (result.getValue().isEmpty());
}

TEST_F (ShaderCacheTests, GetOrTranspile_MSLEntryPoint)
{
    TranspileOptions opts;
    opts.entryPoint = "main";

    auto key = ShaderCache::generateCacheKey (
        kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl, opts);

    auto result = cache->getOrTranspile (
        key, kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::msl, opts);

    ASSERT_TRUE (result.wasOk());
    EXPECT_FALSE (result.getValue().isEmpty());
}

TEST_F (ShaderCacheTests, GetOrTranspile_MSLThenSameKeyReusesCache)
{
    auto key = ShaderCache::generateCacheKey (
        kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl);

    auto first = cache->getOrTranspile (
        key, kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::msl);
    ASSERT_TRUE (first.wasOk());

    // Same key should hit cache even for different target (cache stores SPIR-V)
    auto second = cache->getOrTranspile (
        key, kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::hlsl);
    ASSERT_TRUE (second.wasOk());

    EXPECT_EQ (1u, cache->getNumEntries());
}

TEST_F (ShaderCacheTests, GenerateCacheKey_MSLOptionsChangeKey)
{
    TranspileOptions opts1;
    opts1.flipVertY = false;

    TranspileOptions opts2;
    opts2.flipVertY = true;

    auto key1 = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl, opts1);
    auto key2 = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl, opts2);

    EXPECT_NE (key1, key2);
}

//==============================================================================
// store / contains / remove / clear
//==============================================================================

TEST_F (ShaderCacheTests, StoreThenContains)
{
    auto key = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    auto spirv = transpiler->compileToSPIRV (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    cache->store (key, spirv.getValue());

    EXPECT_TRUE (cache->contains (key));
    EXPECT_EQ (1u, cache->getNumEntries());
}

TEST_F (ShaderCacheTests, Remove)
{
    auto key = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    auto spirv = transpiler->compileToSPIRV (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    cache->store (key, spirv.getValue());
    EXPECT_TRUE (cache->contains (key));

    cache->remove (key);
    EXPECT_FALSE (cache->contains (key));
    EXPECT_EQ (0u, cache->getNumEntries());
}

TEST_F (ShaderCacheTests, RemoveUnknownKeyIsNoop)
{
    cache->remove ("nonexistent_key");
    EXPECT_EQ (0u, cache->getNumEntries());
}

TEST_F (ShaderCacheTests, Clear)
{
    auto key1 = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    auto key2 = ShaderCache::generateCacheKey (
        kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl);

    auto spirv = transpiler->compileToSPIRV (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    cache->store (key1, spirv.getValue());
    cache->store (key2, spirv.getValue());
    EXPECT_EQ (2u, cache->getNumEntries());

    cache->clear();
    EXPECT_EQ (0u, cache->getNumEntries());
    EXPECT_FALSE (cache->contains (key1));
    EXPECT_FALSE (cache->contains (key2));
}

//==============================================================================
// getNumEntries / getMemoryUsage
//==============================================================================

TEST_F (ShaderCacheTests, GetNumEntries_TracksCorrectly)
{
    EXPECT_EQ (0u, cache->getNumEntries());

    auto key1 = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    auto key2 = ShaderCache::generateCacheKey (
        kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl);

    cache->getOrCompile (key1, kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    EXPECT_EQ (1u, cache->getNumEntries());

    cache->getOrCompile (key2, kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl);
    EXPECT_EQ (2u, cache->getNumEntries());
}

TEST_F (ShaderCacheTests, GetMemoryUsage_ReturnsNonZero)
{
    auto key = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);

    cache->getOrCompile (key, kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);

    EXPECT_GT (cache->getMemoryUsage(), 0u);
}

TEST_F (ShaderCacheTests, GetMemoryUsage_EmptyCacheIsZero)
{
    EXPECT_EQ (0u, cache->getMemoryUsage());
}

//==============================================================================
// Eviction
//==============================================================================

TEST_F (ShaderCacheTests, MaxEntries_Zero_Unlimited)
{
    cache->setMaxEntries (0);

    for (int i = 0; i < 20; ++i)
    {
        String source = String::formatted (
            "#version 450\nlayout(location = 0) out vec4 c%d;\nvoid main() { c%d = vec4(1.0); }\n", i, i);

        auto key = ShaderCache::generateCacheKey (
            source, ShaderStage::fragment, ShaderLanguage::glsl);

        cache->getOrCompile (
            key, source, ShaderStage::fragment, ShaderLanguage::glsl);
    }

    EXPECT_EQ (20u, cache->getNumEntries());
}

TEST_F (ShaderCacheTests, MaxEntries_EvictsOldest)
{
    cache->setMaxEntries (2);

    // Fill with 3 entries
    auto key1 = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    auto key2 = ShaderCache::generateCacheKey (
        kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl);
    auto key3 = ShaderCache::generateCacheKey (
        kAnotherShader, ShaderStage::fragment, ShaderLanguage::glsl);

    cache->getOrCompile (key1, kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    cache->getOrCompile (key2, kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl);
    cache->getOrCompile (key3, kAnotherShader, ShaderStage::fragment, ShaderLanguage::glsl);

    EXPECT_EQ (2u, cache->getNumEntries());
}

TEST_F (ShaderCacheTests, MaxEntries_RecentlyUsedSurvives)
{
    cache->setMaxEntries (2);

    auto key1 = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    auto key2 = ShaderCache::generateCacheKey (
        kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl);
    auto key3 = ShaderCache::generateCacheKey (
        kAnotherShader, ShaderStage::fragment, ShaderLanguage::glsl);

    cache->getOrCompile (key1, kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    cache->getOrCompile (key2, kTestFragment, ShaderStage::fragment, ShaderLanguage::glsl);

    // Access key1 again to mark it recently used
    cache->getOrCompile (key1, kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);

    // Now add key3 — key2 should be evicted (least recently used)
    cache->getOrCompile (key3, kAnotherShader, ShaderStage::fragment, ShaderLanguage::glsl);

    EXPECT_EQ (2u, cache->getNumEntries());
    EXPECT_TRUE (cache->contains (key1));
    EXPECT_TRUE (cache->contains (key3));
}

//==============================================================================
// generateCacheKey
//==============================================================================

TEST_F (ShaderCacheTests, GenerateCacheKey_HexString)
{
    auto key = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);

    EXPECT_EQ (40, key.length()); // SHA1 hex is 40 chars

    for (auto c : key)
        EXPECT_TRUE ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
}

TEST_F (ShaderCacheTests, GenerateCacheKey_DifferentSourceProducesDifferentKey)
{
    auto key1 = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    auto key2 = ShaderCache::generateCacheKey (
        kTestFragment, ShaderStage::vertex, ShaderLanguage::glsl);

    EXPECT_NE (key1, key2);
}

TEST_F (ShaderCacheTests, GenerateCacheKey_DifferentStageProducesDifferentKey)
{
    auto key1 = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    auto key2 = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::fragment, ShaderLanguage::glsl);

    EXPECT_NE (key1, key2);
}

TEST_F (ShaderCacheTests, GenerateCacheKey_DifferentLanguageProducesDifferentKey)
{
    auto key1 = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    auto key2 = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::essl);

    EXPECT_NE (key1, key2);
}

TEST_F (ShaderCacheTests, GenerateCacheKey_SameInputProducesSameKey)
{
    auto key1 = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
    auto key2 = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);

    EXPECT_EQ (key1, key2);
}

TEST_F (ShaderCacheTests, GenerateCacheKey_DifferentOptionsProducesDifferentKey)
{
    TranspileOptions opts1;
    opts1.entryPoint = "main";

    TranspileOptions opts2;
    opts2.entryPoint = "custom_entry";

    auto key1 = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl, opts1);
    auto key2 = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl, opts2);

    EXPECT_NE (key1, key2);
}

TEST_F (ShaderCacheTests, GenerateCacheKey_DefinesAffectKey)
{
    TranspileOptions opts1;
    HashMap<String, String> defines1;
    defines1.set ("FOO", "1");
    opts1.defines = std::move (defines1);

    TranspileOptions opts2;
    HashMap<String, String> defines2;
    defines2.set ("BAR", "2");
    opts2.defines = std::move (defines2);

    auto key1 = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl, opts1);
    auto key2 = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl, opts2);

    EXPECT_NE (key1, key2);
}

//==============================================================================
// Thread safety (basic single-threaded verification)
//==============================================================================

TEST_F (ShaderCacheTests, RepeatedAccessDoesNotCorrupt)
{
    auto key = ShaderCache::generateCacheKey (
        kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);

    for (int i = 0; i < 100; ++i)
    {
        auto result = cache->getOrCompile (
            key, kTestVertex, ShaderStage::vertex, ShaderLanguage::glsl);
        ASSERT_TRUE (result.wasOk());
    }

    EXPECT_EQ (1u, cache->getNumEntries());
}

//==============================================================================
// maxEntries accessors
//==============================================================================

TEST_F (ShaderCacheTests, GetMaxEntries_DefaultIsNonZero)
{
    EXPECT_GT (cache->getMaxEntries(), 0u);
}

TEST_F (ShaderCacheTests, SetMaxEntries_UpdatesValue)
{
    cache->setMaxEntries (10);
    EXPECT_EQ (10u, cache->getMaxEntries());

    cache->setMaxEntries (0);
    EXPECT_EQ (0u, cache->getMaxEntries());
}
