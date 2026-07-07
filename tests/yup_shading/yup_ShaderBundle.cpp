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
// Shared test helpers (used by both ShaderBundleTests and compiler tests)
//==============================================================================

namespace
{

constexpr const char* kShaderBundleMinimalVertexGLSL = R"glsl(
#version 450
void main()
{
    gl_Position = vec4(0.0);
}
)glsl";

constexpr const char* kShaderBundleMinimalFragmentGLSL = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
void main()
{
    outColor = vec4(1.0);
}
)glsl";

ShaderInfo makeSyntheticShaderInfo (ShaderStage stage, ShaderLanguage language)
{
    ShaderInfo info;
    info.stage = stage;
    info.language = language;
    info.entryPoint = "main";
    info.source = "void main() {}";

    info.reflection.positionInvariant = true;
    info.reflection.capabilities = { "Shader" };
    info.reflection.extensions = { "SPV_KHR_16bit_storage" };

    ShaderReflection::EntryPoint ep;
    ep.name = "main";
    ep.stage = stage;
    info.reflection.entryPoints = { ep };

    ShaderReflection::WorkgroupSize wgs;
    wgs.x = 32;
    wgs.y = 1;
    wgs.z = 1;
    wgs.usesSpecializationConstants = false;
    info.reflection.workgroupSize = wgs;

    ShaderReflection::ResourceBinding ub;
    ub.name = "GlobalUniforms";
    ub.type = ShaderReflection::ResourceType::uniformBuffer;
    ub.set = 0;
    ub.binding = 0;
    ub.baseType = ShaderReflection::BaseType::structType;
    ub.blockSize = 64;

    ShaderReflection::ResourceMember member;
    member.name = "transform";
    member.offset = 0;
    member.size = 64;
    member.baseType = ShaderReflection::BaseType::float32;
    member.vecSize = 4;
    member.columns = 4;
    member.bitWidth = 32;
    ub.members = { member };
    info.reflection.uniformBuffers = { ub };

    return info;
}

void verifyShaderInfoEqual (const ShaderInfo& a, const ShaderInfo& b)
{
    EXPECT_EQ (a.stage, b.stage);
    EXPECT_EQ (a.language, b.language);
    EXPECT_EQ (a.entryPoint, b.entryPoint);
    EXPECT_EQ (a.source, b.source);

    EXPECT_EQ (a.reflection.positionInvariant, b.reflection.positionInvariant);
    EXPECT_EQ (a.reflection.capabilities, b.reflection.capabilities);
    EXPECT_EQ (a.reflection.extensions, b.reflection.extensions);

    ASSERT_EQ (a.reflection.entryPoints.size(), b.reflection.entryPoints.size());
    if (! a.reflection.entryPoints.empty())
    {
        EXPECT_EQ (a.reflection.entryPoints[0].name, b.reflection.entryPoints[0].name);
        EXPECT_EQ (a.reflection.entryPoints[0].stage, b.reflection.entryPoints[0].stage);
    }

    EXPECT_EQ (a.reflection.workgroupSize.x, b.reflection.workgroupSize.x);
    EXPECT_EQ (a.reflection.workgroupSize.y, b.reflection.workgroupSize.y);
    EXPECT_EQ (a.reflection.workgroupSize.z, b.reflection.workgroupSize.z);

    ASSERT_EQ (a.reflection.uniformBuffers.size(), b.reflection.uniformBuffers.size());
    if (! a.reflection.uniformBuffers.empty())
    {
        const auto& ubA = a.reflection.uniformBuffers[0];
        const auto& ubB = b.reflection.uniformBuffers[0];
        EXPECT_EQ (ubA.name, ubB.name);
        EXPECT_EQ (ubA.type, ubB.type);
        EXPECT_EQ (ubA.set, ubB.set);
        EXPECT_EQ (ubA.binding, ubB.binding);
        EXPECT_EQ (ubA.blockSize, ubB.blockSize);

        ASSERT_EQ (ubA.members.size(), ubB.members.size());
        if (! ubA.members.empty())
        {
            EXPECT_EQ (ubA.members[0].name, ubB.members[0].name);
            EXPECT_EQ (ubA.members[0].offset, ubB.members[0].offset);
            EXPECT_EQ (ubA.members[0].size, ubB.members[0].size);
            EXPECT_EQ (ubA.members[0].baseType, ubB.members[0].baseType);
        }
    }
}

} // namespace

//==============================================================================
// ShaderBundle tests — always compiled (no transpiler dependency)
//==============================================================================

class ShaderBundleTests : public ::testing::Test
{
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F (ShaderBundleTests, RoundtripEmptyBundle)
{
    ShaderBundle original;
    original.setOriginalSource ("");

    MemoryBlock mem;
    ASSERT_TRUE (original.saveToMemoryBlock (mem).wasOk());

    auto loaded = ShaderBundle::loadFromMemoryBlock (mem);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    EXPECT_EQ (loaded.getReference().getOriginalSource(), "");
    EXPECT_TRUE (loaded.getReference().getShaders().empty());
}

TEST_F (ShaderBundleTests, RoundtripOriginalSourcePreserved)
{
    const String source = "void main() { gl_Position = vec4(1.0); }";

    ShaderBundle bundle;
    bundle.setOriginalSource (source);

    MemoryBlock mem;
    ASSERT_TRUE (bundle.saveToMemoryBlock (mem).wasOk());

    auto loaded = ShaderBundle::loadFromMemoryBlock (mem);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    EXPECT_EQ (loaded.getReference().getOriginalSource(), source);
}

TEST_F (ShaderBundleTests, RoundtripWithShaders)
{
    ShaderBundle original;
    original.setOriginalSource (kShaderBundleMinimalVertexGLSL);

    const auto info = makeSyntheticShaderInfo (ShaderStage::vertex, ShaderLanguage::msl);
    original.addShader (info);

    MemoryBlock mem;
    ASSERT_TRUE (original.saveToMemoryBlock (mem).wasOk());

    auto loaded = ShaderBundle::loadFromMemoryBlock (mem);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    const auto& shaders = loaded.getReference().getShaders();
    ASSERT_EQ (shaders.size(), 1u);
    verifyShaderInfoEqual (shaders[0], info);
}

TEST_F (ShaderBundleTests, RoundtripMultipleShaders)
{
    ShaderBundle original;
    original.setOriginalSource (kShaderBundleMinimalVertexGLSL);

    original.addShader (makeSyntheticShaderInfo (ShaderStage::vertex, ShaderLanguage::msl));
    original.addShader (makeSyntheticShaderInfo (ShaderStage::vertex, ShaderLanguage::hlsl));
    original.addShader (makeSyntheticShaderInfo (ShaderStage::fragment, ShaderLanguage::glsl));

    MemoryBlock mem;
    ASSERT_TRUE (original.saveToMemoryBlock (mem).wasOk());

    auto loaded = ShaderBundle::loadFromMemoryBlock (mem);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    EXPECT_EQ (loaded.getReference().getShaders().size(), 3u);
}

TEST_F (ShaderBundleTests, FindShaderReturnsCorrectVariant)
{
    ShaderBundle bundle;
    bundle.addShader (makeSyntheticShaderInfo (ShaderStage::vertex, ShaderLanguage::msl));
    bundle.addShader (makeSyntheticShaderInfo (ShaderStage::vertex, ShaderLanguage::hlsl));
    bundle.addShader (makeSyntheticShaderInfo (ShaderStage::fragment, ShaderLanguage::glsl));

    EXPECT_NE (bundle.findShader (ShaderStage::vertex, ShaderLanguage::msl), nullptr);
    EXPECT_NE (bundle.findShader (ShaderStage::vertex, ShaderLanguage::hlsl), nullptr);
    EXPECT_NE (bundle.findShader (ShaderStage::fragment, ShaderLanguage::glsl), nullptr);
    EXPECT_EQ (bundle.findShader (ShaderStage::vertex, ShaderLanguage::glsl), nullptr);
    EXPECT_EQ (bundle.findShader (ShaderStage::fragment, ShaderLanguage::msl), nullptr);
}

TEST_F (ShaderBundleTests, FindShaderAfterRoundtrip)
{
    ShaderBundle original;
    original.addShader (makeSyntheticShaderInfo (ShaderStage::vertex, ShaderLanguage::msl));
    original.addShader (makeSyntheticShaderInfo (ShaderStage::fragment, ShaderLanguage::hlsl));

    MemoryBlock mem;
    ASSERT_TRUE (original.saveToMemoryBlock (mem).wasOk());

    auto loaded = ShaderBundle::loadFromMemoryBlock (mem);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    EXPECT_NE (loaded.getReference().findShader (ShaderStage::vertex, ShaderLanguage::msl), nullptr);
    EXPECT_NE (loaded.getReference().findShader (ShaderStage::fragment, ShaderLanguage::hlsl), nullptr);
    EXPECT_EQ (loaded.getReference().findShader (ShaderStage::vertex, ShaderLanguage::hlsl), nullptr);
}

TEST_F (ShaderBundleTests, LoadFromInvalidDataFails)
{
    const uint8_t garbage[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04 };
    auto result = ShaderBundle::loadFromData (garbage, sizeof (garbage));
    EXPECT_FALSE (result.wasOk());
}

TEST_F (ShaderBundleTests, LoadFromEmptyDataFails)
{
    auto result = ShaderBundle::loadFromData (nullptr, 0);
    EXPECT_FALSE (result.wasOk());
}

TEST_F (ShaderBundleTests, WrongMagicRejected)
{
    ShaderBundle original;
    original.setOriginalSource ("test");

    MemoryBlock mem;
    ASSERT_TRUE (original.saveToMemoryBlock (mem).wasOk());

    // Corrupt the RIFF magic (first 4 bytes)
    if (mem.getSize() >= 4)
    {
        mem[0] = 'X';
        mem[1] = 'X';
        mem[2] = 'X';
        mem[3] = 'X';
    }

    auto result = ShaderBundle::loadFromMemoryBlock (mem);
    EXPECT_FALSE (result.wasOk());
}

TEST_F (ShaderBundleTests, WrongFormTypeRejected)
{
    ShaderBundle original;
    original.setOriginalSource ("test");

    MemoryBlock mem;
    ASSERT_TRUE (original.saveToMemoryBlock (mem).wasOk());

    // Corrupt the form type at bytes 8-11 (after "RIFF" + size)
    if (mem.getSize() >= 12)
    {
        mem[8] = 'X';
        mem[9] = 'X';
        mem[10] = 'X';
        mem[11] = 'X';
    }

    auto result = ShaderBundle::loadFromMemoryBlock (mem);
    EXPECT_FALSE (result.wasOk());
}

TEST_F (ShaderBundleTests, SaveToFileAndLoadBack)
{
    const auto tmpFile = File::getSpecialLocation (File::tempDirectory)
                             .getChildFile ("yup_shader_bundle_test.ysl");

    ShaderBundle original;
    original.setOriginalSource (kShaderBundleMinimalFragmentGLSL);
    original.addShader (makeSyntheticShaderInfo (ShaderStage::fragment, ShaderLanguage::msl));

    ASSERT_TRUE (original.saveToFile (tmpFile).wasOk());
    ASSERT_TRUE (tmpFile.existsAsFile());

    auto loaded = ShaderBundle::loadFromFile (tmpFile);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    EXPECT_EQ (loaded.getReference().getOriginalSource(), String (kShaderBundleMinimalFragmentGLSL));
    EXPECT_EQ (loaded.getReference().getShaders().size(), 1u);

    tmpFile.deleteFile();
}

TEST_F (ShaderBundleTests, SPIRVStoredAndRecovered)
{
    ShaderBundle original;
    original.setOriginalSource ("dummy");

    MemoryBlock spirv (16, false);
    for (size_t i = 0; i < 16; ++i)
        static_cast<uint8_t*> (spirv.getData())[i] = static_cast<uint8_t> (i);

    original.setSPIRV (ShaderStage::vertex, ShaderLanguage::glsl, spirv);
    original.addShader (makeSyntheticShaderInfo (ShaderStage::vertex, ShaderLanguage::msl));

    MemoryBlock mem;
    ASSERT_TRUE (original.saveToMemoryBlock (mem).wasOk());

    auto loaded = ShaderBundle::loadFromMemoryBlock (mem);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    EXPECT_EQ (loaded.getReference().getShaders().size(), 1u);
}

TEST_F (ShaderBundleTests, RoundtripAllThreeLanguages)
{
    ShaderBundle original;
    original.setOriginalSource ("multi-lang");
    original.addShader (makeSyntheticShaderInfo (ShaderStage::vertex, ShaderLanguage::glsl));
    original.addShader (makeSyntheticShaderInfo (ShaderStage::vertex, ShaderLanguage::hlsl));
    original.addShader (makeSyntheticShaderInfo (ShaderStage::vertex, ShaderLanguage::msl));

    MemoryBlock mem;
    ASSERT_TRUE (original.saveToMemoryBlock (mem).wasOk());

    auto loaded = ShaderBundle::loadFromMemoryBlock (mem);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    ASSERT_EQ (loaded.getReference().getShaders().size(), 3u);
    EXPECT_NE (loaded.getReference().findShader (ShaderStage::vertex, ShaderLanguage::glsl), nullptr);
    EXPECT_NE (loaded.getReference().findShader (ShaderStage::vertex, ShaderLanguage::hlsl), nullptr);
    EXPECT_NE (loaded.getReference().findShader (ShaderStage::vertex, ShaderLanguage::msl), nullptr);
}

TEST_F (ShaderBundleTests, RoundtripVertexAndFragmentStages)
{
    ShaderBundle original;
    original.setOriginalSource ("vert+frag");
    original.addShader (makeSyntheticShaderInfo (ShaderStage::vertex, ShaderLanguage::glsl));
    original.addShader (makeSyntheticShaderInfo (ShaderStage::vertex, ShaderLanguage::msl));
    original.addShader (makeSyntheticShaderInfo (ShaderStage::fragment, ShaderLanguage::glsl));
    original.addShader (makeSyntheticShaderInfo (ShaderStage::fragment, ShaderLanguage::msl));

    MemoryBlock mem;
    ASSERT_TRUE (original.saveToMemoryBlock (mem).wasOk());

    auto loaded = ShaderBundle::loadFromMemoryBlock (mem);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    ASSERT_EQ (loaded.getReference().getShaders().size(), 4u);
    EXPECT_NE (loaded.getReference().findShader (ShaderStage::vertex, ShaderLanguage::glsl), nullptr);
    EXPECT_NE (loaded.getReference().findShader (ShaderStage::vertex, ShaderLanguage::msl), nullptr);
    EXPECT_NE (loaded.getReference().findShader (ShaderStage::fragment, ShaderLanguage::glsl), nullptr);
    EXPECT_NE (loaded.getReference().findShader (ShaderStage::fragment, ShaderLanguage::msl), nullptr);
}

TEST_F (ShaderBundleTests, RoundtripExactShaderContent)
{
    const auto vert = makeSyntheticShaderInfo (ShaderStage::vertex, ShaderLanguage::glsl);
    const auto frag = makeSyntheticShaderInfo (ShaderStage::fragment, ShaderLanguage::hlsl);

    ShaderBundle original;
    original.setOriginalSource ("exact-content");
    original.addShader (vert);
    original.addShader (frag);

    MemoryBlock mem;
    ASSERT_TRUE (original.saveToMemoryBlock (mem).wasOk());

    auto loaded = ShaderBundle::loadFromMemoryBlock (mem);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    const auto* loadedVert = loaded.getReference().findShader (ShaderStage::vertex, ShaderLanguage::glsl);
    ASSERT_NE (loadedVert, nullptr);
    verifyShaderInfoEqual (*loadedVert, vert);

    const auto* loadedFrag = loaded.getReference().findShader (ShaderStage::fragment, ShaderLanguage::hlsl);
    ASSERT_NE (loadedFrag, nullptr);
    verifyShaderInfoEqual (*loadedFrag, frag);
}

TEST_F (ShaderBundleTests, RoundtripShaderOrderPreserved)
{
    ShaderBundle original;
    original.setOriginalSource ("order-check");
    original.addShader (makeSyntheticShaderInfo (ShaderStage::vertex, ShaderLanguage::glsl));
    original.addShader (makeSyntheticShaderInfo (ShaderStage::vertex, ShaderLanguage::msl));
    original.addShader (makeSyntheticShaderInfo (ShaderStage::fragment, ShaderLanguage::glsl));

    MemoryBlock mem;
    ASSERT_TRUE (original.saveToMemoryBlock (mem).wasOk());

    auto loaded = ShaderBundle::loadFromMemoryBlock (mem);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    const auto& shaders = loaded.getReference().getShaders();
    ASSERT_EQ (shaders.size(), 3u);
    EXPECT_EQ (shaders[0].stage, ShaderStage::vertex);
    EXPECT_EQ (shaders[0].language, ShaderLanguage::glsl);
    EXPECT_EQ (shaders[1].stage, ShaderStage::vertex);
    EXPECT_EQ (shaders[1].language, ShaderLanguage::msl);
    EXPECT_EQ (shaders[2].stage, ShaderStage::fragment);
    EXPECT_EQ (shaders[2].language, ShaderLanguage::glsl);
}

TEST_F (ShaderBundleTests, RoundtripStorageBuffersInReflection)
{
    ShaderInfo info;
    info.stage = ShaderStage::vertex;
    info.language = ShaderLanguage::glsl;
    info.entryPoint = "main";
    info.source = "void main() {}";

    ShaderReflection::ResourceBinding sb;
    sb.name = "StorageData";
    sb.type = ShaderReflection::ResourceType::storageBuffer;
    sb.set = 0;
    sb.binding = 1;
    sb.blockSize = 256;
    info.reflection.storageBuffers = { sb };

    ShaderBundle original;
    original.setOriginalSource ("storage-buf");
    original.addShader (info);

    MemoryBlock mem;
    ASSERT_TRUE (original.saveToMemoryBlock (mem).wasOk());

    auto loaded = ShaderBundle::loadFromMemoryBlock (mem);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    const auto* loadedInfo = loaded.getReference().findShader (ShaderStage::vertex, ShaderLanguage::glsl);
    ASSERT_NE (loadedInfo, nullptr);
    ASSERT_EQ (loadedInfo->reflection.storageBuffers.size(), 1u);
    EXPECT_EQ (loadedInfo->reflection.storageBuffers[0].name, String ("StorageData"));
    EXPECT_EQ (loadedInfo->reflection.storageBuffers[0].binding, 1u);
    EXPECT_EQ (loadedInfo->reflection.storageBuffers[0].blockSize, 256u);
}

TEST_F (ShaderBundleTests, RoundtripSpecializationConstantsInReflection)
{
    ShaderInfo info;
    info.stage = ShaderStage::vertex;
    info.language = ShaderLanguage::glsl;
    info.entryPoint = "main";
    info.source = "void main() {}";

    ShaderReflection::SpecializationConstant sc;
    sc.name = "LOCAL_SIZE";
    sc.constantId = 3;
    sc.baseType = ShaderReflection::BaseType::uint32;
    sc.bitWidth = 32;
    info.reflection.specConstants = { sc };

    ShaderBundle original;
    original.setOriginalSource ("spec-const");
    original.addShader (info);

    MemoryBlock mem;
    ASSERT_TRUE (original.saveToMemoryBlock (mem).wasOk());

    auto loaded = ShaderBundle::loadFromMemoryBlock (mem);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    const auto* loadedInfo = loaded.getReference().findShader (ShaderStage::vertex, ShaderLanguage::glsl);
    ASSERT_NE (loadedInfo, nullptr);
    ASSERT_EQ (loadedInfo->reflection.specConstants.size(), 1u);
    EXPECT_EQ (loadedInfo->reflection.specConstants[0].name, String ("LOCAL_SIZE"));
    EXPECT_EQ (loadedInfo->reflection.specConstants[0].constantId, 3u);
    EXPECT_EQ (loadedInfo->reflection.specConstants[0].bitWidth, 32u);
}

TEST_F (ShaderBundleTests, RoundtripStageInputsAndOutputsInReflection)
{
    ShaderInfo info;
    info.stage = ShaderStage::vertex;
    info.language = ShaderLanguage::glsl;
    info.entryPoint = "main";
    info.source = "void main() {}";

    ShaderReflection::ResourceBinding input;
    input.name = "inPosition";
    input.type = ShaderReflection::ResourceType::stageInput;
    input.location = 0;
    input.baseType = ShaderReflection::BaseType::float32;
    input.vecSize = 4;
    info.reflection.stageInputs = { input };

    ShaderReflection::ResourceBinding output;
    output.name = "outColor";
    output.type = ShaderReflection::ResourceType::stageOutput;
    output.location = 0;
    output.baseType = ShaderReflection::BaseType::float32;
    output.vecSize = 4;
    info.reflection.stageOutputs = { output };

    ShaderBundle original;
    original.setOriginalSource ("io");
    original.addShader (info);

    MemoryBlock mem;
    ASSERT_TRUE (original.saveToMemoryBlock (mem).wasOk());

    auto loaded = ShaderBundle::loadFromMemoryBlock (mem);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    const auto* loadedInfo = loaded.getReference().findShader (ShaderStage::vertex, ShaderLanguage::glsl);
    ASSERT_NE (loadedInfo, nullptr);
    ASSERT_EQ (loadedInfo->reflection.stageInputs.size(), 1u);
    EXPECT_EQ (loadedInfo->reflection.stageInputs[0].name, String ("inPosition"));
    ASSERT_EQ (loadedInfo->reflection.stageOutputs.size(), 1u);
    EXPECT_EQ (loadedInfo->reflection.stageOutputs[0].name, String ("outColor"));
}

TEST_F (ShaderBundleTests, RoundtripUnicodeOriginalSource)
{
    const String unicodeSource = String (L"// 日本語 éàü shader\nvoid main() {}");

    ShaderBundle original;
    original.setOriginalSource (unicodeSource);

    MemoryBlock mem;
    ASSERT_TRUE (original.saveToMemoryBlock (mem).wasOk());

    auto loaded = ShaderBundle::loadFromMemoryBlock (mem);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    EXPECT_EQ (loaded.getReference().getOriginalSource(), unicodeSource);
}

TEST_F (ShaderBundleTests, RoundtripOddLengthSource)
{
    for (const char* src : { "a", "abc", "abcde" })
    {
        ShaderBundle original;
        original.setOriginalSource (src);

        MemoryBlock mem;
        ASSERT_TRUE (original.saveToMemoryBlock (mem).wasOk()) << "source: " << src;

        auto loaded = ShaderBundle::loadFromMemoryBlock (mem);
        ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage() << " source: " << src;

        EXPECT_EQ (loaded.getReference().getOriginalSource(), String (src));
    }
}

TEST_F (ShaderBundleTests, RoundtripEmptyShaderSourceAndEntryPoint)
{
    ShaderInfo info;
    info.stage = ShaderStage::vertex;
    info.language = ShaderLanguage::glsl;
    info.entryPoint = "";
    info.source = "";

    ShaderBundle original;
    original.setOriginalSource ("empty-fields");
    original.addShader (info);

    MemoryBlock mem;
    ASSERT_TRUE (original.saveToMemoryBlock (mem).wasOk());

    auto loaded = ShaderBundle::loadFromMemoryBlock (mem);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    const auto* loaded_info = loaded.getReference().findShader (ShaderStage::vertex, ShaderLanguage::glsl);
    ASSERT_NE (loaded_info, nullptr);
    EXPECT_EQ (loaded_info->entryPoint, String());
    EXPECT_EQ (loaded_info->source, String());
}

TEST_F (ShaderBundleTests, VersionTooHighRejected)
{
    ShaderBundle original;
    original.setOriginalSource ("ver-test");

    MemoryBlock mem;
    ASSERT_TRUE (original.saveToMemoryBlock (mem).wasOk());
    ASSERT_GE (mem.getSize(), 24u);

    // Version field is at bytes 20-23 (big-endian uint32 written by writeInt).
    // kCurrentVersion == 1, stored as [0x00, 0x00, 0x00, 0x01].
    // Setting the most-significant byte to 0xFF makes version >> 1.
    auto* bytes = static_cast<uint8_t*> (mem.getData());
    bytes[20] = 0xFF;

    auto result = ShaderBundle::loadFromMemoryBlock (mem);
    EXPECT_FALSE (result.wasOk());
}

TEST_F (ShaderBundleTests, TruncatedStreamFails)
{
    ShaderBundle original;
    original.setOriginalSource (kShaderBundleMinimalVertexGLSL);
    original.addShader (makeSyntheticShaderInfo (ShaderStage::vertex, ShaderLanguage::glsl));

    MemoryBlock full;
    ASSERT_TRUE (original.saveToMemoryBlock (full).wasOk());

    // Try loading a buffer truncated to every power-of-two size up to half the real size.
    for (size_t truncLen = 4; truncLen < full.getSize() / 2; truncLen *= 2)
    {
        auto result = ShaderBundle::loadFromData (full.getData(), truncLen);
        EXPECT_FALSE (result.wasOk()) << "Expected failure for truncLen=" << truncLen;
    }
}

TEST_F (ShaderBundleTests, OverwriteExistingFileSucceeds)
{
    const auto tmpFile = File::getSpecialLocation (File::tempDirectory)
                             .getChildFile ("yup_shader_bundle_overwrite_test.ysl");

    // First save
    ShaderBundle first;
    first.setOriginalSource ("first-version");
    ASSERT_TRUE (first.saveToFile (tmpFile).wasOk());

    // Second save to same path
    ShaderBundle second;
    second.setOriginalSource ("second-version");
    second.addShader (makeSyntheticShaderInfo (ShaderStage::vertex, ShaderLanguage::glsl));
    ASSERT_TRUE (second.saveToFile (tmpFile).wasOk());

    auto loaded = ShaderBundle::loadFromFile (tmpFile);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    EXPECT_EQ (loaded.getReference().getOriginalSource(), String ("second-version"));
    EXPECT_EQ (loaded.getReference().getShaders().size(), 1u);

    tmpFile.deleteFile();
}

TEST_F (ShaderBundleTests, MultipleRoundtripsProduceSameResult)
{
    ShaderBundle original;
    original.setOriginalSource (kShaderBundleMinimalFragmentGLSL);
    original.addShader (makeSyntheticShaderInfo (ShaderStage::fragment, ShaderLanguage::msl));

    MemoryBlock mem1;
    ASSERT_TRUE (original.saveToMemoryBlock (mem1).wasOk());

    auto loaded1 = ShaderBundle::loadFromMemoryBlock (mem1);
    ASSERT_TRUE (loaded1.wasOk()) << loaded1.getErrorMessage();

    MemoryBlock mem2;
    ASSERT_TRUE (loaded1.getReference().saveToMemoryBlock (mem2).wasOk());

    // Both serialised forms must be byte-identical.
    ASSERT_EQ (mem1.getSize(), mem2.getSize());
    EXPECT_EQ (memcmp (mem1.getData(), mem2.getData(), mem1.getSize()), 0);
}

TEST_F (ShaderBundleTests, SaveAndLoadViaStreamDirectly)
{
    ShaderBundle original;
    original.setOriginalSource ("stream-test");
    original.addShader (makeSyntheticShaderInfo (ShaderStage::vertex, ShaderLanguage::hlsl));

    MemoryBlock mem;
    {
        MemoryOutputStream mos (mem, false);
        ASSERT_TRUE (original.saveToStream (mos).wasOk());
    }

    {
        MemoryInputStream mis (mem, false);
        auto loaded = ShaderBundle::loadFromStream (mis);
        ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

        EXPECT_EQ (loaded.getReference().getOriginalSource(), String ("stream-test"));
        ASSERT_EQ (loaded.getReference().getShaders().size(), 1u);
        EXPECT_EQ (loaded.getReference().getShaders()[0].stage, ShaderStage::vertex);
        EXPECT_EQ (loaded.getReference().getShaders()[0].language, ShaderLanguage::hlsl);
    }
}

//==============================================================================
// ShaderBundleCompiler tests — only when transpiler is available
//==============================================================================

#if YUP_ENABLE_SHADER_TRANSPILER

class ShaderBundleCompilerTests : public ::testing::Test
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

    static ShaderTranspiler::Ptr transpiler;
};

ShaderTranspiler::Ptr ShaderBundleCompilerTests::transpiler {};

TEST_F (ShaderBundleCompilerTests, CompileEmptyRequestProducesEmptyBundle)
{
    ShaderBundleCompiler compiler (transpiler);

    ShaderBundleCompileRequest req;
    req.source = "";
    req.sourceLanguage = ShaderLanguage::glsl;

    auto result = compiler.compile (req);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (result.getReference().getShaders().empty());
}

TEST_F (ShaderBundleCompilerTests, CompileVertexToGLSL)
{
    ShaderBundleCompiler compiler (transpiler);

    ShaderBundleCompileRequest req;
    req.source = kShaderBundleMinimalVertexGLSL;
    req.sourceLanguage = ShaderLanguage::glsl;

    ShaderBundleEntry entry;
    entry.stage = ShaderStage::vertex;
    entry.targetLanguages = { ShaderLanguage::glsl };
    req.entries.push_back (entry);

    auto result = compiler.compile (req);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto* info = result.getReference().findShader (ShaderStage::vertex, ShaderLanguage::glsl);
    ASSERT_NE (info, nullptr);
    EXPECT_FALSE (info->source.isEmpty());
    EXPECT_EQ (info->entryPoint, "main");
}

TEST_F (ShaderBundleCompilerTests, CompileToMultipleTargets)
{
    ShaderBundleCompiler compiler (transpiler);

    ShaderBundleCompileRequest req;
    req.source = kShaderBundleMinimalVertexGLSL;
    req.sourceLanguage = ShaderLanguage::glsl;

    ShaderBundleEntry entry;
    entry.stage = ShaderStage::vertex;
    entry.targetLanguages = { ShaderLanguage::glsl, ShaderLanguage::msl };
    req.entries.push_back (entry);

    auto result = compiler.compile (req);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    EXPECT_NE (result.getReference().findShader (ShaderStage::vertex, ShaderLanguage::glsl), nullptr);
    EXPECT_NE (result.getReference().findShader (ShaderStage::vertex, ShaderLanguage::msl), nullptr);
    EXPECT_EQ (result.getReference().getShaders().size(), 2u);
}

TEST_F (ShaderBundleCompilerTests, CompileRoundtrip)
{
    ShaderBundleCompiler compiler (transpiler);

    ShaderBundleCompileRequest req;
    req.source = kShaderBundleMinimalVertexGLSL;
    req.sourceLanguage = ShaderLanguage::glsl;

    ShaderBundleEntry entry;
    entry.stage = ShaderStage::vertex;
    entry.targetLanguages = { ShaderLanguage::glsl };
    req.entries.push_back (entry);

    auto compiled = compiler.compile (req);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    MemoryBlock mem;
    ASSERT_TRUE (compiled.getReference().saveToMemoryBlock (mem).wasOk());

    auto reloaded = ShaderBundle::loadFromMemoryBlock (mem);
    ASSERT_TRUE (reloaded.wasOk()) << reloaded.getErrorMessage();

    EXPECT_EQ (reloaded.getReference().getOriginalSource(), String (kShaderBundleMinimalVertexGLSL));
    EXPECT_EQ (reloaded.getReference().getShaders().size(), 1u);

    const auto* info = reloaded.getReference().findShader (ShaderStage::vertex, ShaderLanguage::glsl);
    ASSERT_NE (info, nullptr);
    EXPECT_FALSE (info->source.isEmpty());
    EXPECT_FALSE (info->reflection.entryPoints.empty());
}

TEST_F (ShaderBundleCompilerTests, CompileInvalidSourceFails)
{
    ShaderBundleCompiler compiler (transpiler);

    ShaderBundleCompileRequest req;
    req.source = "this is not valid GLSL @@@";
    req.sourceLanguage = ShaderLanguage::glsl;

    ShaderBundleEntry entry;
    entry.stage = ShaderStage::vertex;
    entry.targetLanguages = { ShaderLanguage::glsl };
    req.entries.push_back (entry);

    auto result = compiler.compile (req);
    EXPECT_FALSE (result.wasOk());
}

TEST_F (ShaderBundleCompilerTests, DefaultConstructorCreatesOwnTranspiler)
{
    ShaderBundleCompiler compiler;

    ShaderBundleCompileRequest req;
    req.source = kShaderBundleMinimalVertexGLSL;
    req.sourceLanguage = ShaderLanguage::glsl;

    ShaderBundleEntry entry;
    entry.stage = ShaderStage::vertex;
    entry.targetLanguages = { ShaderLanguage::glsl };
    req.entries.push_back (entry);

    auto result = compiler.compile (req);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_EQ (result.getReference().getShaders().size(), 1u);
}

#endif // YUP_ENABLE_SHADER_TRANSPILER
