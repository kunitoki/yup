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

#pragma once

namespace yup
{

//==============================================================================
/**
    A single transpiled shader variant stored in a ShaderBundle.

    Holds the transpiled source code and full reflection data for one
    (stage, language) combination.

    @see ShaderBundle, ShaderBundleCompiler
*/
struct ShaderInfo
{
    /** Pipeline stage this shader targets. */
    ShaderStage stage = ShaderStage::vertex;

    /** Shading language of @c source. */
    ShaderLanguage language = ShaderLanguage::glsl;

    /** Name of the entry-point function (e.g. "main" or "vertexMain"). */
    String entryPoint;

    /** Transpiled source code in @c language. */
    String source;

    /** Reflection data extracted for this (SPIR-V, target language) pair. */
    ShaderReflection reflection;
};

//==============================================================================
/**
    A compiled and transpiled shader bundle in RIFF binary format.

    A ShaderBundle stores:
    - The original shader source used for compilation.
    - The SPIR-V binary for each compiled stage.
    - One ShaderInfo per (stage × target language) combination, each containing
      the transpiled source and full ShaderReflection.

    Bundles can be persisted to / loaded from streams, files, and MemoryBlocks
    using the YSLB RIFF format so that shaders compiled once can be reused
    without hitting glslang at runtime.

    @code
    // Compile once and save:
    ShaderBundleCompiler compiler;
    auto result = compiler.compile (request);
    if (result)
        result.getValue().saveToFile (File ("myShader.ysl"));

    // Load at runtime and look up a variant:
    auto loaded = ShaderBundle::loadFromFile (File ("myShader.ysl"));
    if (loaded)
        if (auto* info = loaded.getValue().findShader (ShaderStage::vertex, ShaderLanguage::msl))
            useSource (info->source);
    @endcode

    @see ShaderBundleCompiler, ShaderInfo
*/
class YUP_API ShaderBundle final
{
public:
    ShaderBundle() = default;

    //==========================================================================
    ShaderBundle (ShaderBundle&& other) = default;
    ShaderBundle& operator= (ShaderBundle&& other) = default;

    //==========================================================================
    /** Set the original source code that was compiled to produce this bundle. */
    void setOriginalSource (const String& src);

    /** Add a transpiled variant (one ShaderInfo per stage × language pair). */
    void addShader (ShaderInfo info);

    /** Store the SPIR-V binary for a stage. */
    void setSPIRV (ShaderStage stage, ShaderLanguage sourceLang, MemoryBlock spirv);

    //==========================================================================
    /** Returns the original source code used for compilation. */
    const String& getOriginalSource() const;

    /** Returns all transpiled variants in this bundle. */
    const std::vector<ShaderInfo>& getShaders() const;

    /** Find the transpiled variant for a specific stage and target language.
        @returns Pointer to the matching ShaderInfo, or nullptr if not present. */
    const ShaderInfo* findShader (ShaderStage stage, ShaderLanguage language) const;

    //==========================================================================
    /** Serialise the bundle to the YSLB RIFF binary format. */
    Result saveToStream (OutputStream& stream) const;

    /** Serialise the bundle to a file in the YSLB RIFF binary format. */
    Result saveToFile (const File& file) const;

    /** Serialise the bundle to a MemoryBlock in the YSLB RIFF binary format. */
    Result saveToMemoryBlock (MemoryBlock& block) const;

    //==========================================================================
    /** Deserialise a bundle from an YSLB RIFF stream. */
    static ResultValue<ShaderBundle> loadFromStream (InputStream& stream);

    /** Deserialise a bundle from an YSLB RIFF file. */
    static ResultValue<ShaderBundle> loadFromFile (const File& file);

    /** Deserialise a bundle from raw bytes. */
    static ResultValue<ShaderBundle> loadFromData (const void* data, size_t size);

    /** Deserialise a bundle from a MemoryBlock. */
    static ResultValue<ShaderBundle> loadFromMemoryBlock (const MemoryBlock& block);

private:
    String originalSource;

    struct StageSPIRV
    {
        ShaderLanguage sourceLang = ShaderLanguage::glsl;
        MemoryBlock spirv;
    };

    std::map<ShaderStage, StageSPIRV> spirvBinaries;
    std::vector<ShaderInfo> shaders;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShaderBundle)
};

} // namespace yup

//==============================================================================
// SerialisationTraits for ShaderReflection nested types
//==============================================================================

namespace yup
{

#ifndef DOXYGEN

template <>
struct SerialisationTraits<ShaderReflection::EntryPoint>
{
    static constexpr auto marshallingVersion = std::nullopt;

    template <typename Archive, typename T>
    static void serialise (Archive& archive, T& t)
    {
        archive (named ("name", t.name), named ("stage", t.stage));
    }
};

template <>
struct SerialisationTraits<ShaderReflection::WorkgroupSize>
{
    static constexpr auto marshallingVersion = std::nullopt;

    template <typename Archive, typename T>
    static void serialise (Archive& archive, T& t)
    {
        archive (named ("x", t.x),
                 named ("y", t.y),
                 named ("z", t.z),
                 named ("usesSpecConst", t.usesSpecializationConstants),
                 named ("specConstIdX", t.specConstantIdX),
                 named ("specConstIdY", t.specConstantIdY),
                 named ("specConstIdZ", t.specConstantIdZ));
    }
};

template <>
struct SerialisationTraits<ShaderReflection::ResourceMember>
{
    static constexpr auto marshallingVersion = std::nullopt;

    template <typename Archive, typename T>
    static void serialise (Archive& archive, T& t)
    {
        archive (named ("name", t.name),
                 named ("offset", t.offset),
                 named ("size", t.size),
                 named ("arrayStride", t.arrayStride),
                 named ("matrixStride", t.matrixStride),
                 named ("baseType", t.baseType),
                 named ("vecSize", t.vecSize),
                 named ("columns", t.columns),
                 named ("bitWidth", t.bitWidth),
                 named ("arraySizes", t.arraySizes));
    }
};

template <>
struct SerialisationTraits<ShaderReflection::ResourceBinding>
{
    static constexpr auto marshallingVersion = std::nullopt;

    template <typename Archive, typename T>
    static void serialise (Archive& archive, T& t)
    {
        archive (named ("name", t.name),
                 named ("type", t.type),
                 named ("set", t.set),
                 named ("binding", t.binding),
                 named ("location", t.location),
                 named ("descriptorCount", t.descriptorCount),
                 named ("baseType", t.baseType),
                 named ("vecSize", t.vecSize),
                 named ("columns", t.columns),
                 named ("bitWidth", t.bitWidth),
                 named ("arraySizes", t.arraySizes),
                 named ("members", t.members),
                 named ("imageDim", t.imageDim),
                 named ("imageIsDepth", t.imageIsDepth),
                 named ("imageArrayed", t.imageArrayed),
                 named ("imageMS", t.imageMS),
                 named ("imageFormat", t.imageFormat),
                 named ("blockSize", t.blockSize),
                 named ("resourceId", t.resourceId),
                 named ("backendSlot", t.backendSlot),
                 named ("backendSlotSecondary", t.backendSlotSecondary));
    }
};

template <>
struct SerialisationTraits<ShaderReflection::BuiltInBinding>
{
    static constexpr auto marshallingVersion = std::nullopt;

    template <typename Archive, typename T>
    static void serialise (Archive& archive, T& t)
    {
        archive (named ("builtin", t.builtin),
                 named ("valueBaseType", t.valueBaseType),
                 named ("valueVecSize", t.valueVecSize),
                 named ("valueColumns", t.valueColumns),
                 named ("resource", t.resource));
    }
};

template <>
struct SerialisationTraits<ShaderReflection::SpecializationConstant>
{
    static constexpr auto marshallingVersion = std::nullopt;

    template <typename Archive, typename T>
    static void serialise (Archive& archive, T& t)
    {
        archive (named ("name", t.name),
                 named ("constantId", t.constantId),
                 named ("baseType", t.baseType),
                 named ("vecSize", t.vecSize),
                 named ("columns", t.columns),
                 named ("bitWidth", t.bitWidth));
    }
};

template <>
struct SerialisationTraits<ShaderReflection::GLCombinedSampler>
{
    static constexpr auto marshallingVersion = std::nullopt;

    template <typename Archive, typename T>
    static void serialise (Archive& archive, T& t)
    {
        archive (named ("name", t.name),
                 named ("textureSlot", t.textureSlot));
    }
};

template <>
struct SerialisationTraits<ShaderReflection>
{
    static constexpr auto marshallingVersion = std::nullopt;

    template <typename Archive, typename T>
    static void serialise (Archive& archive, T& t)
    {
        archive (named ("entryPoints", t.entryPoints),
                 named ("uniformBuffers", t.uniformBuffers),
                 named ("storageBuffers", t.storageBuffers),
                 named ("stageInputs", t.stageInputs),
                 named ("stageOutputs", t.stageOutputs),
                 named ("subpassInputs", t.subpassInputs),
                 named ("storageImages", t.storageImages),
                 named ("sampledImages", t.sampledImages),
                 named ("atomicCounters", t.atomicCounters),
                 named ("accelerationStructures", t.accelerationStructures),
                 named ("glPlainUniforms", t.glPlainUniforms),
                 named ("tensors", t.tensors),
                 named ("pushConstantBuffers", t.pushConstantBuffers),
                 named ("shaderRecordBuffers", t.shaderRecordBuffers),
                 named ("separateImages", t.separateImages),
                 named ("separateSamplers", t.separateSamplers),
                 named ("builtinInputs", t.builtinInputs),
                 named ("builtinOutputs", t.builtinOutputs),
                 named ("specConstants", t.specConstants),
                 named ("workgroupSize", t.workgroupSize),
                 named ("positionInvariant", t.positionInvariant),
                 named ("glCombinedSamplers", t.glCombinedSamplers),
                 named ("capabilities", t.capabilities),
                 named ("extensions", t.extensions));
    }
};

#endif // DOXYGEN

} // namespace yup
