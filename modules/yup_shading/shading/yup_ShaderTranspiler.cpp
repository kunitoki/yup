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

//==============================================================================
namespace yup
{
namespace
{

//==============================================================================
// glslang global init/finalize - reference-counted across all ShaderTranspiler instances
//==============================================================================

static std::atomic<int>& getGlslangInitCount()
{
    static std::atomic<int> count { 0 };
    return count;
}

static void incrementGlslangInit()
{
    if (getGlslangInitCount().fetch_add (1) == 0)
        glslang::InitializeProcess();
}

static void decrementGlslangInit()
{
    if (getGlslangInitCount().fetch_sub (1) == 1)
        glslang::FinalizeProcess();
}

//==============================================================================
// Default resource limits for glslang compilation
//==============================================================================

static TBuiltInResource getDefaultResources()
{
    TBuiltInResource res = {};

    res.maxLights = 32;
    res.maxClipPlanes = 6;
    res.maxTextureUnits = 32;
    res.maxTextureCoords = 32;
    res.maxVertexAttribs = 64;
    res.maxVertexUniformComponents = 4096;
    res.maxVaryingFloats = 64;
    res.maxVertexTextureImageUnits = 32;
    res.maxCombinedTextureImageUnits = 80;
    res.maxTextureImageUnits = 32;
    res.maxFragmentUniformComponents = 4096;
    res.maxDrawBuffers = 32;
    res.maxVertexUniformVectors = 128;
    res.maxVaryingVectors = 8;
    res.maxFragmentUniformVectors = 16;
    res.maxVertexOutputVectors = 16;
    res.maxFragmentInputVectors = 15;
    res.minProgramTexelOffset = -8;
    res.maxProgramTexelOffset = 7;
    res.maxClipDistances = 8;
    res.maxComputeWorkGroupCountX = 65535;
    res.maxComputeWorkGroupCountY = 65535;
    res.maxComputeWorkGroupCountZ = 65535;
    res.maxComputeWorkGroupSizeX = 1024;
    res.maxComputeWorkGroupSizeY = 1024;
    res.maxComputeWorkGroupSizeZ = 64;
    res.maxComputeUniformComponents = 1024;
    res.maxComputeTextureImageUnits = 16;
    res.maxComputeImageUniforms = 8;
    res.maxComputeAtomicCounters = 8;
    res.maxComputeAtomicCounterBuffers = 1;
    res.maxVaryingComponents = 60;
    res.maxVertexOutputComponents = 64;
    res.maxGeometryInputComponents = 64;
    res.maxGeometryOutputComponents = 128;
    res.maxFragmentInputComponents = 128;
    res.maxImageUnits = 8;
    res.maxCombinedImageUnitsAndFragmentOutputs = 8;
    res.maxCombinedShaderOutputResources = 8;
    res.maxImageSamples = 0;
    res.maxVertexImageUniforms = 0;
    res.maxTessControlImageUniforms = 0;
    res.maxTessEvaluationImageUniforms = 0;
    res.maxGeometryImageUniforms = 0;
    res.maxFragmentImageUniforms = 8;
    res.maxCombinedImageUniforms = 8;
    res.maxGeometryTextureImageUnits = 16;
    res.maxGeometryOutputVertices = 256;
    res.maxGeometryTotalOutputComponents = 1024;
    res.maxGeometryUniformComponents = 1024;
    res.maxGeometryVaryingComponents = 64;
    res.maxTessControlInputComponents = 128;
    res.maxTessControlOutputComponents = 128;
    res.maxTessControlTextureImageUnits = 16;
    res.maxTessControlUniformComponents = 1024;
    res.maxTessControlTotalOutputComponents = 4096;
    res.maxTessEvaluationInputComponents = 128;
    res.maxTessEvaluationOutputComponents = 128;
    res.maxTessEvaluationTextureImageUnits = 16;
    res.maxTessEvaluationUniformComponents = 1024;
    res.maxTessPatchComponents = 120;
    res.maxPatchVertices = 32;
    res.maxTessGenLevel = 64;
    res.maxViewports = 16;
    res.maxVertexAtomicCounters = 0;
    res.maxTessControlAtomicCounters = 0;
    res.maxTessEvaluationAtomicCounters = 0;
    res.maxGeometryAtomicCounters = 0;
    res.maxFragmentAtomicCounters = 8;
    res.maxCombinedAtomicCounters = 8;
    res.maxAtomicCounterBindings = 1;
    res.maxVertexAtomicCounterBuffers = 0;
    res.maxTessControlAtomicCounterBuffers = 0;
    res.maxTessEvaluationAtomicCounterBuffers = 0;
    res.maxGeometryAtomicCounterBuffers = 0;
    res.maxFragmentAtomicCounterBuffers = 1;
    res.maxCombinedAtomicCounterBuffers = 1;
    res.maxAtomicCounterBufferSize = 16384;
    res.maxTransformFeedbackBuffers = 4;
    res.maxTransformFeedbackInterleavedComponents = 64;
    res.maxCullDistances = 8;
    res.maxCombinedClipAndCullDistances = 8;
    res.maxSamples = 4;

    res.limits.nonInductiveForLoops = true;
    res.limits.whileLoops = true;
    res.limits.doWhileLoops = true;
    res.limits.generalUniformIndexing = true;
    res.limits.generalAttributeMatrixVectorIndexing = true;
    res.limits.generalVaryingIndexing = true;
    res.limits.generalSamplerIndexing = true;
    res.limits.generalVariableIndexing = true;
    res.limits.generalConstantMatrixVectorIndexing = true;

    return res;
}

//==============================================================================
// Enum mapping: YUP <-> glslang / spirv_cross
//==============================================================================

static EShLanguage toGlslangStage (ShaderStage stage)
{
    switch (stage)
    {
        case ShaderStage::vertex:
            return EShLangVertex;
        case ShaderStage::fragment:
            return EShLangFragment;
        case ShaderStage::compute:
            return EShLangCompute;
        case ShaderStage::geometry:
            return EShLangGeometry;
        case ShaderStage::tessControl:
            return EShLangTessControl;
        case ShaderStage::tessEval:
            return EShLangTessEvaluation;
    }
    return EShLangVertex;
}

static glslang::EShSource toGlslangSource (ShaderLanguage lang)
{
    switch (lang)
    {
        case ShaderLanguage::glsl:
        case ShaderLanguage::essl:
            return glslang::EShSourceGlsl;
        case ShaderLanguage::hlsl:
            return glslang::EShSourceHlsl;
        default:
            return glslang::EShSourceNone;
    }
}

static ShaderReflection::BaseType toBaseType (spirv_cross::SPIRType::BaseType bt)
{
    switch (bt)
    {
        case spirv_cross::SPIRType::Unknown:
            return ShaderReflection::BaseType::unknown;
        case spirv_cross::SPIRType::Void:
            return ShaderReflection::BaseType::voidType;
        case spirv_cross::SPIRType::Boolean:
            return ShaderReflection::BaseType::boolean;
        case spirv_cross::SPIRType::SByte:
            return ShaderReflection::BaseType::int8;
        case spirv_cross::SPIRType::UByte:
            return ShaderReflection::BaseType::uint8;
        case spirv_cross::SPIRType::Short:
            return ShaderReflection::BaseType::int16;
        case spirv_cross::SPIRType::UShort:
            return ShaderReflection::BaseType::uint16;
        case spirv_cross::SPIRType::Int:
            return ShaderReflection::BaseType::int32;
        case spirv_cross::SPIRType::UInt:
            return ShaderReflection::BaseType::uint32;
        case spirv_cross::SPIRType::Int64:
            return ShaderReflection::BaseType::int64;
        case spirv_cross::SPIRType::UInt64:
            return ShaderReflection::BaseType::uint64;
        case spirv_cross::SPIRType::Half:
            return ShaderReflection::BaseType::half;
        case spirv_cross::SPIRType::Float:
            return ShaderReflection::BaseType::float32;
        case spirv_cross::SPIRType::Double:
            return ShaderReflection::BaseType::float64;
        case spirv_cross::SPIRType::AtomicCounter:
            return ShaderReflection::BaseType::atomicCounter;
        case spirv_cross::SPIRType::Struct:
            return ShaderReflection::BaseType::structType;
        case spirv_cross::SPIRType::Image:
            return ShaderReflection::BaseType::image;
        case spirv_cross::SPIRType::SampledImage:
            return ShaderReflection::BaseType::sampledImage;
        case spirv_cross::SPIRType::Sampler:
            return ShaderReflection::BaseType::sampler;
        case spirv_cross::SPIRType::AccelerationStructure:
            return ShaderReflection::BaseType::accelerationStructure;
        case spirv_cross::SPIRType::RayQuery:
            return ShaderReflection::BaseType::rayQuery;
        default:
            return ShaderReflection::BaseType::unknown;
    }
}

static ShaderReflection::ImageDimension toImageDim (spv::Dim dim)
{
    switch (dim)
    {
        case spv::Dim1D:
            return ShaderReflection::ImageDimension::dim1D;
        case spv::Dim2D:
            return ShaderReflection::ImageDimension::dim2D;
        case spv::Dim3D:
            return ShaderReflection::ImageDimension::dim3D;
        case spv::DimCube:
            return ShaderReflection::ImageDimension::cube;
        case spv::DimRect:
            return ShaderReflection::ImageDimension::dimRect;
        case spv::DimBuffer:
            return ShaderReflection::ImageDimension::dimBuffer;
        case spv::DimSubpassData:
            return ShaderReflection::ImageDimension::dimSubpass;
        default:
            return ShaderReflection::ImageDimension::unknown;
    }
}

static ShaderReflection::BuiltInType fromSpvBuiltin (spv::BuiltIn bi)
{
    switch (bi)
    {
        case spv::BuiltInPosition:
            return ShaderReflection::BuiltInType::position;
        case spv::BuiltInPointSize:
            return ShaderReflection::BuiltInType::pointSize;
        case spv::BuiltInClipDistance:
            return ShaderReflection::BuiltInType::clipDistance;
        case spv::BuiltInCullDistance:
            return ShaderReflection::BuiltInType::cullDistance;
        case spv::BuiltInVertexId:
            return ShaderReflection::BuiltInType::vertexId;
        case spv::BuiltInInstanceId:
            return ShaderReflection::BuiltInType::instanceId;
        case spv::BuiltInPrimitiveId:
            return ShaderReflection::BuiltInType::primitiveId;
        case spv::BuiltInVertexIndex:
            return ShaderReflection::BuiltInType::vertexIndex;
        case spv::BuiltInInstanceIndex:
            return ShaderReflection::BuiltInType::instanceIndex;
        case spv::BuiltInBaseVertex:
            return ShaderReflection::BuiltInType::baseVertex;
        case spv::BuiltInBaseInstance:
            return ShaderReflection::BuiltInType::baseInstance;
        case spv::BuiltInDrawIndex:
            return ShaderReflection::BuiltInType::drawIndex;
        case spv::BuiltInFragCoord:
            return ShaderReflection::BuiltInType::fragCoord;
        case spv::BuiltInPointCoord:
            return ShaderReflection::BuiltInType::pointCoord;
        case spv::BuiltInFrontFacing:
            return ShaderReflection::BuiltInType::frontFacing;
        case spv::BuiltInSampleId:
            return ShaderReflection::BuiltInType::sampleId;
        case spv::BuiltInSamplePosition:
            return ShaderReflection::BuiltInType::samplePosition;
        case spv::BuiltInSampleMask:
            return ShaderReflection::BuiltInType::sampleMask;
        case spv::BuiltInFragDepth:
            return ShaderReflection::BuiltInType::fragDepth;
        case spv::BuiltInHelperInvocation:
            return ShaderReflection::BuiltInType::helperInvocation;
        case spv::BuiltInNumWorkgroups:
            return ShaderReflection::BuiltInType::numWorkgroups;
        case spv::BuiltInWorkgroupSize:
            return ShaderReflection::BuiltInType::workgroupSize;
        case spv::BuiltInWorkgroupId:
            return ShaderReflection::BuiltInType::workgroupId;
        case spv::BuiltInLocalInvocationId:
            return ShaderReflection::BuiltInType::localInvocationId;
        case spv::BuiltInGlobalInvocationId:
            return ShaderReflection::BuiltInType::globalInvocationId;
        case spv::BuiltInLocalInvocationIndex:
            return ShaderReflection::BuiltInType::localInvocationIndex;
        case spv::BuiltInViewportIndex:
            return ShaderReflection::BuiltInType::viewportIndex;
        case spv::BuiltInLayer:
            return ShaderReflection::BuiltInType::layer;
        case spv::BuiltInTessLevelOuter:
            return ShaderReflection::BuiltInType::tessLevelOuter;
        case spv::BuiltInTessLevelInner:
            return ShaderReflection::BuiltInType::tessLevelInner;
        case spv::BuiltInTessCoord:
            return ShaderReflection::BuiltInType::tessCoord;
        case spv::BuiltInPatchVertices:
            return ShaderReflection::BuiltInType::patchVertices;
        case spv::BuiltInBaryCoordNoPerspAMD:
            return ShaderReflection::BuiltInType::baryCoordNoPerspAMD;
        case spv::BuiltInBaryCoordNoPerspCentroidAMD:
            return ShaderReflection::BuiltInType::baryCoordNoPerspCentroidAMD;
        case spv::BuiltInBaryCoordPullModelAMD:
            return ShaderReflection::BuiltInType::baryCoordPullModel;
        case spv::BuiltInDeviceIndex:
            return ShaderReflection::BuiltInType::deviceIndex;
        case spv::BuiltInFragStencilRefEXT:
            return ShaderReflection::BuiltInType::fragStencilRefEXT;
        case spv::BuiltInViewportMaskNV:
            return ShaderReflection::BuiltInType::viewportMaskNV;
        case spv::BuiltInSecondaryPositionNV:
            return ShaderReflection::BuiltInType::secondaryPositionNV;
        case spv::BuiltInSecondaryViewportMaskNV:
            return ShaderReflection::BuiltInType::secondaryViewportMaskNV;
        case spv::BuiltInPositionPerViewNV:
            return ShaderReflection::BuiltInType::positionPerViewNV;
        case spv::BuiltInViewportMaskPerViewNV:
            return ShaderReflection::BuiltInType::viewportMaskPerViewNV;
        case spv::BuiltInFullyCoveredEXT:
            return ShaderReflection::BuiltInType::fullyCoveredEXT;
        case spv::BuiltInFragSizeEXT:
            return ShaderReflection::BuiltInType::fragSizeEXT;
        case spv::BuiltInFragInvocationCountEXT:
            return ShaderReflection::BuiltInType::fragInvocationCountEXT;
        case spv::BuiltInLaunchIdKHR:
            return ShaderReflection::BuiltInType::launchIdKHR;
        case spv::BuiltInLaunchSizeKHR:
            return ShaderReflection::BuiltInType::launchSizeKHR;
        case spv::BuiltInSubgroupId:
            return ShaderReflection::BuiltInType::subgroupIdKHR;
        case spv::BuiltInNumSubgroups:
            return ShaderReflection::BuiltInType::numSubgroupsKHR;
        case spv::BuiltInSubgroupEqMask:
            return ShaderReflection::BuiltInType::subgroupEqMaskKHR;
        case spv::BuiltInSubgroupGeMask:
            return ShaderReflection::BuiltInType::subgroupGeMaskKHR;
        case spv::BuiltInSubgroupGtMask:
            return ShaderReflection::BuiltInType::subgroupGtMaskKHR;
        case spv::BuiltInSubgroupLeMask:
            return ShaderReflection::BuiltInType::subgroupLeMaskKHR;
        case spv::BuiltInSubgroupLtMask:
            return ShaderReflection::BuiltInType::subgroupLtMaskKHR;
        case spv::BuiltInSubgroupLocalInvocationId:
            return ShaderReflection::BuiltInType::subgroupLocalInvocationIdKHR;
        case spv::BuiltInSubgroupSize:
            return ShaderReflection::BuiltInType::subgroupSizeKHR;
        case spv::BuiltInCullPrimitiveEXT:
            return ShaderReflection::BuiltInType::cullPrimitiveEXT;
        case spv::BuiltInHitKindKHR:
            return ShaderReflection::BuiltInType::hitKindKHR;
        case spv::BuiltInIncomingRayFlagsKHR:
            return ShaderReflection::BuiltInType::incomingRayFlagsKHR;
        case spv::BuiltInInstanceCustomIndexKHR:
            return ShaderReflection::BuiltInType::instanceCustomIndexKHR;
        case spv::BuiltInRayGeometryIndexKHR:
            return ShaderReflection::BuiltInType::rayGeometryIndexKHR;
        case spv::BuiltInObjectRayDirectionKHR:
            return ShaderReflection::BuiltInType::objectRayDirectionKHR;
        case spv::BuiltInObjectRayOriginKHR:
            return ShaderReflection::BuiltInType::objectRayOriginKHR;
        case spv::BuiltInObjectToWorldKHR:
            return ShaderReflection::BuiltInType::objectToWorldKHR;
        case spv::BuiltInWorldToObjectKHR:
            return ShaderReflection::BuiltInType::worldToObjectKHR;
        case spv::BuiltInRayTminKHR:
            return ShaderReflection::BuiltInType::rayTminKHR;
        case spv::BuiltInRayTmaxKHR:
            return ShaderReflection::BuiltInType::rayTmaxKHR;
        case spv::BuiltInWorldRayDirectionKHR:
            return ShaderReflection::BuiltInType::worldRayDirectionKHR;
        case spv::BuiltInWorldRayOriginKHR:
            return ShaderReflection::BuiltInType::worldRayOriginKHR;
        case spv::BuiltInShaderIndexAMDX:
            return ShaderReflection::BuiltInType::shaderIndexKHR;
        case spv::BuiltInBaryCoordKHR:
            return ShaderReflection::BuiltInType::baryCoordKHR;
        case spv::BuiltInBaryCoordNoPerspKHR:
            return ShaderReflection::BuiltInType::baryCoordNoPerspKHR;
        default:
            return ShaderReflection::BuiltInType::unknown;
    }
}

static ShaderStage fromSpvExecutionModel (spv::ExecutionModel model)
{
    switch (model)
    {
        case spv::ExecutionModelVertex:
            return ShaderStage::vertex;
        case spv::ExecutionModelFragment:
            return ShaderStage::fragment;
        case spv::ExecutionModelGLCompute:
            return ShaderStage::compute;
        case spv::ExecutionModelGeometry:
            return ShaderStage::geometry;
        case spv::ExecutionModelTessellationControl:
            return ShaderStage::tessControl;
        case spv::ExecutionModelTessellationEvaluation:
            return ShaderStage::tessEval;
        default:
            return ShaderStage::vertex;
    }
}

//==============================================================================
// Utility: get decoration value from a variable id, returns 0 if absent
//==============================================================================

static uint32_t getVariableDecoration (const spirv_cross::Compiler& compiler,
                                       spirv_cross::VariableID id,
                                       spv::Decoration decoration)
{
    if (compiler.has_decoration (id, decoration))
        return compiler.get_decoration (id, decoration);
    return 0;
}

//==============================================================================
// Recursively fills ShaderReflection::ResourceBinding type info from a SPIRType
//==============================================================================

static void fillTypeInfo (const spirv_cross::Compiler& compiler,
                          spirv_cross::TypeID typeId,
                          ShaderReflection::ResourceBinding& binding)
{
    const auto& type = compiler.get_type (typeId);

    binding.baseType = toBaseType (type.basetype);
    binding.vecSize = type.vecsize;
    binding.columns = type.columns;
    binding.bitWidth = type.width;

    if (! type.array.empty())
    {
        binding.arraySizes.assign (type.array.begin(), type.array.end());

        // The outermost array dimension is the descriptor count (for arrays of resources)
        if (type.array[0] != 0)
            binding.descriptorCount = type.array[0];

        // Recurse into element type to get proper scalar type info
        if (type.array.size() == 1 && type.array[0] != 0)
            fillTypeInfo (compiler, type.self, binding);

        return;
    }

    if (type.basetype == spirv_cross::SPIRType::Image
        || type.basetype == spirv_cross::SPIRType::SampledImage)
    {
        binding.imageDim = toImageDim (type.image.dim);
        binding.imageIsDepth = type.image.depth;
        binding.imageArrayed = type.image.arrayed;
        binding.imageMS = type.image.ms;
        binding.imageFormat = static_cast<uint32_t> (type.image.format);
    }

    if (type.basetype == spirv_cross::SPIRType::Struct)
    {
        const uint32_t memberCount = static_cast<uint32_t> (type.member_types.size());

        // Built-in blocks (e.g. gl_PerVertex) have no Offset decorations on
        // their members. spirv-cross throws when queried for offsets/sizes on
        // such members, so detect this case up front and skip reflection -
        // these blocks are not user-facing resources anyway.
        bool hasLaidOutMembers = false;
        for (uint32_t i = 0; i < memberCount; ++i)
        {
            if (compiler.has_member_decoration (type.self, i, spv::DecorationOffset))
            {
                hasLaidOutMembers = true;
                break;
            }
        }

        if (! hasLaidOutMembers)
            return;

        binding.members.reserve (memberCount);

        try
        {
            uint32_t computedBlockSize = 0;

            for (uint32_t i = 0; i < memberCount; ++i)
            {
                if (! compiler.has_member_decoration (type.self, i, spv::DecorationOffset))
                    continue;

                ShaderReflection::ResourceMember member;
                member.name = compiler.get_member_name (typeId, i).c_str();
                member.offset = compiler.type_struct_member_offset (type, i);
                member.size = static_cast<uint32_t> (compiler.get_declared_struct_member_size (type, i));

                const auto& memberType = compiler.get_type (type.member_types[i]);

                // Array/matrix strides live in separate decorations that are
                // only present for array/matrix members. Query them guarded so
                // spirv-cross doesn't throw on plain scalar/vector members.
                if (! memberType.array.empty())
                    member.arrayStride = compiler.type_struct_member_array_stride (type, i);

                if (memberType.columns > 1)
                    member.matrixStride = compiler.type_struct_member_matrix_stride (type, i);

                member.baseType = toBaseType (memberType.basetype);
                member.vecSize = memberType.vecsize;
                member.columns = memberType.columns;
                member.bitWidth = memberType.width;

                if (! memberType.array.empty())
                    member.arraySizes.assign (memberType.array.begin(), memberType.array.end());

                computedBlockSize = jmax (computedBlockSize, member.offset + member.size);

                binding.members.push_back (std::move (member));
            }

            binding.blockSize = computedBlockSize;
        }
        catch (const std::exception&)
        {
            // Built-in blocks (e.g., gl_PerVertex) may not have explicit
            // offset/size decorations - clear partial member data gracefully.
            binding.members.clear();
            binding.blockSize = 0;
        }
    }
}

//==============================================================================
// Extracts a single ResourceBinding from a spirv_cross::Resource
//==============================================================================

static ShaderReflection::ResourceBinding extractResourceBinding (
    const spirv_cross::Compiler& compiler,
    const spirv_cross::Resource& resource,
    ShaderReflection::ResourceType resType)
{
    ShaderReflection::ResourceBinding binding;
    binding.name = resource.name.c_str();
    binding.type = resType;

    binding.set = getVariableDecoration (compiler, resource.id, spv::DecorationDescriptorSet);
    binding.binding = getVariableDecoration (compiler, resource.id, spv::DecorationBinding);
    binding.location = getVariableDecoration (compiler, resource.id, spv::DecorationLocation);
    binding.resourceId = resource.id;
    binding.descriptorCount = 1; // default, may be overridden by array type in fillTypeInfo

    fillTypeInfo (compiler, resource.base_type_id, binding);

    return binding;
}

//==============================================================================
// Creates a spirv_cross::Compiler from SPIR-V words
//==============================================================================

static std::unique_ptr<spirv_cross::Compiler> createSpirvCompiler (const uint32_t* words, size_t wordCount)
{
    return std::make_unique<spirv_cross::Compiler> (words, wordCount);
}

//==============================================================================
// Extracts full ShaderReflection from a spirv_cross::Compiler
//==============================================================================

static ShaderReflection extractReflection (spirv_cross::Compiler& compiler)
{
    ShaderReflection ref;

    // Entry points
    for (auto& ep : compiler.get_entry_points_and_stages())
    {
        ShaderReflection::EntryPoint e;
        e.name = ep.name.c_str();
        e.stage = fromSpvExecutionModel (ep.execution_model);
        ref.entryPoints.push_back (e);
    }

    // Shader resources
    auto resources = compiler.get_shader_resources();

    auto extractVec = [&] (const spirv_cross::SmallVector<spirv_cross::Resource>& src,
                           ShaderReflection::ResourceType resType,
                           std::vector<ShaderReflection::ResourceBinding>& dst)
    {
        dst.reserve (src.size());
        for (auto& r : src)
            dst.push_back (extractResourceBinding (compiler, r, resType));
    };

    extractVec (resources.uniform_buffers, ShaderReflection::ResourceType::uniformBuffer, ref.uniformBuffers);
    extractVec (resources.storage_buffers, ShaderReflection::ResourceType::storageBuffer, ref.storageBuffers);
    extractVec (resources.stage_inputs, ShaderReflection::ResourceType::stageInput, ref.stageInputs);
    extractVec (resources.stage_outputs, ShaderReflection::ResourceType::stageOutput, ref.stageOutputs);
    extractVec (resources.subpass_inputs, ShaderReflection::ResourceType::subpassInput, ref.subpassInputs);
    extractVec (resources.storage_images, ShaderReflection::ResourceType::storageImage, ref.storageImages);
    extractVec (resources.sampled_images, ShaderReflection::ResourceType::sampledImage, ref.sampledImages);
    extractVec (resources.atomic_counters, ShaderReflection::ResourceType::atomicCounter, ref.atomicCounters);
    extractVec (resources.acceleration_structures, ShaderReflection::ResourceType::accelerationStructure, ref.accelerationStructures);
    extractVec (resources.gl_plain_uniforms, ShaderReflection::ResourceType::glPlainUniform, ref.glPlainUniforms);
    extractVec (resources.tensors, ShaderReflection::ResourceType::tensor, ref.tensors);
    extractVec (resources.push_constant_buffers, ShaderReflection::ResourceType::pushConstant, ref.pushConstantBuffers);
    extractVec (resources.shader_record_buffers, ShaderReflection::ResourceType::shaderRecordBuffer, ref.shaderRecordBuffers);
    extractVec (resources.separate_images, ShaderReflection::ResourceType::separateImage, ref.separateImages);
    extractVec (resources.separate_samplers, ShaderReflection::ResourceType::separateSamplers, ref.separateSamplers);

    // Builtins
    compiler.update_active_builtins();

    for (auto& bi : resources.builtin_inputs)
    {
        ShaderReflection::BuiltInBinding b;
        b.builtin = fromSpvBuiltin (bi.builtin);
        b.resource = extractResourceBinding (compiler, bi.resource, ShaderReflection::ResourceType::stageInput);

        const auto& valType = compiler.get_type (bi.value_type_id);
        b.valueBaseType = toBaseType (valType.basetype);
        b.valueVecSize = valType.vecsize;
        b.valueColumns = valType.columns;

        ref.builtinInputs.push_back (std::move (b));
    }

    for (auto& bo : resources.builtin_outputs)
    {
        ShaderReflection::BuiltInBinding b;
        b.builtin = fromSpvBuiltin (bo.builtin);
        b.resource = extractResourceBinding (compiler, bo.resource, ShaderReflection::ResourceType::stageOutput);

        const auto& valType = compiler.get_type (bo.value_type_id);
        b.valueBaseType = toBaseType (valType.basetype);
        b.valueVecSize = valType.vecsize;
        b.valueColumns = valType.columns;

        ref.builtinOutputs.push_back (std::move (b));
    }

    // Specialization constants
    for (auto& sc : compiler.get_specialization_constants())
    {
        ShaderReflection::SpecializationConstant s;
        s.name = compiler.get_name (sc.id).c_str();
        s.constantId = sc.constant_id;

        const auto& c = compiler.get_constant (sc.id);

        if (c.constant_type != 0)
        {
            const auto& t = compiler.get_type (c.constant_type);
            s.baseType = toBaseType (t.basetype);
            s.vecSize = t.vecsize;
            s.columns = t.columns;
            s.bitWidth = t.width;
        }

        ref.specConstants.push_back (std::move (s));
    }

    // Workgroup size (compute shaders)
    auto execModel = compiler.get_execution_model();
    if (execModel == spv::ExecutionModelGLCompute
        || execModel == spv::ExecutionModelTaskNV
        || execModel == spv::ExecutionModelMeshNV)
    {
        auto& wg = ref.workgroupSize;

        wg.x = compiler.get_execution_mode_argument (spv::ExecutionModeLocalSize, 0);
        wg.y = compiler.get_execution_mode_argument (spv::ExecutionModeLocalSize, 1);
        wg.z = compiler.get_execution_mode_argument (spv::ExecutionModeLocalSize, 2);

        if (wg.x == 0)
            wg.x = 1;
        if (wg.y == 0)
            wg.y = 1;
        if (wg.z == 0)
            wg.z = 1;

        spirv_cross::SpecializationConstant scX, scY, scZ;

        if (compiler.get_work_group_size_specialization_constants (scX, scY, scZ))
        {
            wg.usesSpecializationConstants = true;
            wg.specConstantIdX = scX.constant_id;
            wg.specConstantIdY = scY.constant_id;
            wg.specConstantIdZ = scZ.constant_id;
        }
    }

    // Position invariant
    ref.positionInvariant = compiler.is_position_invariant();

    // Capabilities
    for (auto& cap : compiler.get_declared_capabilities())
    {
        ref.capabilities.push_back (String::formatted ("Capability(%d)", static_cast<int> (cap)));
    }

    // Extensions
    for (auto& ext : compiler.get_declared_extensions())
    {
        ref.extensions.push_back (ext.c_str());
    }

    return ref;
}

//==============================================================================
// Creates a MemoryBlock from SPIR-V uint32_t vector
//==============================================================================

static MemoryBlock spirvToMemoryBlock (const std::vector<uint32_t>& spirv)
{
    if (spirv.empty())
        return {};

    return MemoryBlock (spirv.data(), spirv.size() * sizeof (uint32_t));
}

//==============================================================================
// Creates a uint32_t span from a MemoryBlock
//==============================================================================

static const uint32_t* memoryBlockToSpirvWords (const MemoryBlock& block, size_t& wordCount)
{
    wordCount = block.getSize() / sizeof (uint32_t);
    return static_cast<const uint32_t*> (block.getData());
}

//==============================================================================
// Folds separate image + sampler pairs into combined samplers for the GL/ESSL
// backend and assigns each combined sampler a stable, explicit name.
//
// build_combined_image_samplers() creates global combined-sampler variables with
// no alias, so both the emitted GLSL and the reflection would otherwise fall
// back to spirv-cross's synthetic "_<id>" name - and that id is not stable
// between two separate compiler instances (decompile vs reflect). Setting an
// explicit deterministic name derived from the paired image/sampler names keeps
// the emitted uniform name and the reflected name in lock-step. The name is a
// purely internal handle: the GL backend matches it via glGetUniformLocation to
// fix up the sampler's texture unit, so any collision-free identifier works.
//==============================================================================

static void setupGLCombinedSamplers (spirv_cross::CompilerGLSL& compiler)
{
    compiler.build_combined_image_samplers();
    spirv_cross_util::inherit_combined_sampler_bindings (compiler);

    for (const auto& combined : compiler.get_combined_image_samplers())
    {
        std::string imageName = compiler.get_name (combined.image_id);
        std::string samplerName = compiler.get_name (combined.sampler_id);

        if (imageName.empty())
            imageName = "img" + std::to_string ((uint32_t) combined.image_id);
        if (samplerName.empty())
            samplerName = "samp" + std::to_string ((uint32_t) combined.sampler_id);

        compiler.set_name (combined.combined_id, "yup_combined_" + imageName + "_" + samplerName);
    }
}

//==============================================================================
// Fills MSL backend slot numbers into a ShaderReflection after CompilerMSL::compile() has run
//==============================================================================

static void fillMSLBackendSlots (spirv_cross::CompilerMSL& mslCompiler, ShaderReflection& ref)
{
    auto fillVec = [&] (std::vector<ShaderReflection::ResourceBinding>& bindings)
    {
        for (auto& b : bindings)
        {
            b.backendSlot = mslCompiler.get_automatic_msl_resource_binding (b.resourceId);
            b.backendSlotSecondary = mslCompiler.get_automatic_msl_resource_binding_secondary (b.resourceId);
        }
    };

    fillVec (ref.uniformBuffers);
    fillVec (ref.storageBuffers);
    fillVec (ref.sampledImages);
    fillVec (ref.separateImages);
    fillVec (ref.separateSamplers);
    fillVec (ref.storageImages);
    fillVec (ref.subpassInputs);
    fillVec (ref.atomicCounters);
    fillVec (ref.accelerationStructures);
    fillVec (ref.glPlainUniforms);
    fillVec (ref.tensors);
    fillVec (ref.pushConstantBuffers);
    fillVec (ref.shaderRecordBuffers);
}

//==============================================================================
// Fills GLSL/ESSL backend slot numbers into a ShaderReflection.
//
// OpenGL / OpenGL ES fold each separate image + sampler into a single combined
// sampler2D (there are no separate sampler objects). To keep the generated GLSL,
// the binding-map sidecar and the GL uniform fixup in agreement:
//
//   - Uniform buffers keep their SPIR-V binding as the UBO binding point.
//   - Separate images keep their SPIR-V binding as the GL texture unit.
//   - Each separate sampler adopts the GL texture unit of the image it is paired
//     with (glBindSampler binds to the same unit the combined sampler samples).
//   - The emitted combined-sampler uniform name + texture unit are captured in
//     ShaderReflection::glCombinedSamplers so the GL backend can bind them via
//     glUniform1i without parsing the generated source.
//==============================================================================

static void fillGLSLBackendSlots (spirv_cross::CompilerGLSL& compiler, ShaderReflection& ref)
{
    auto fillVec = [] (std::vector<ShaderReflection::ResourceBinding>& bindings)
    {
        for (auto& b : bindings)
            b.backendSlot = b.binding;
    };

    fillVec (ref.uniformBuffers);
    fillVec (ref.storageBuffers);
    fillVec (ref.sampledImages);
    fillVec (ref.separateImages);
    fillVec (ref.separateSamplers);
    fillVec (ref.storageImages);
    fillVec (ref.subpassInputs);
    fillVec (ref.atomicCounters);
    fillVec (ref.accelerationStructures);
    fillVec (ref.glPlainUniforms);
    fillVec (ref.tensors);
    fillVec (ref.pushConstantBuffers);
    fillVec (ref.shaderRecordBuffers);

    // Fold texture+sampler pairs into combined samplers, capturing the emitted
    // uniform name and the GL texture unit (the paired image's binding).
    ref.glCombinedSamplers.clear();

    for (const auto& combined : compiler.get_combined_image_samplers())
    {
        const uint32_t textureUnit = getVariableDecoration (compiler, combined.image_id, spv::DecorationBinding);

        ShaderReflection::GLCombinedSampler cs;
        cs.name = compiler.get_name (combined.combined_id).c_str();
        cs.textureSlot = textureUnit;
        ref.glCombinedSamplers.push_back (std::move (cs));

        // The paired sampler must bind to the same texture unit as its image.
        for (auto& samp : ref.separateSamplers)
        {
            if (samp.resourceId == combined.sampler_id)
                samp.backendSlot = textureUnit;
        }
    }
}

//==============================================================================
// Fills HLSL backend slot numbers by parsing compiled HLSL source for register() annotations
//==============================================================================

static void fillHLSLBackendSlots (const std::string& hlslSource, ShaderReflection& ref)
{
    // Parse "identifier : register(XN)" patterns where X is b/t/u/s.
    // Uses a linear-scan vector since shader resource counts are small.

    struct NameSlot
    {
        std::string name;
        uint32_t slot;
    };

    std::vector<NameSlot> parsedSlots;

    const char* p = hlslSource.c_str();
    const char* const srcStart = p;
    const char* const srcEnd = p + hlslSource.size();

    while (p < srcEnd)
    {
        const char* regStart = strstr (p, "register(");
        if (regStart == nullptr)
            break;

        const char* reg = regStart + 9; // skip past "register("

        if (reg >= srcEnd)
            break;
        ++reg; // skip prefix (b/t/u/s), we only need the slot number

        uint32_t slot = 0;
        while (reg < srcEnd && *reg >= '0' && *reg <= '9')
        {
            slot = slot * 10 + static_cast<uint32_t> (*reg - '0');
            ++reg;
        }

        // Backtrack from "register(" to find the preceding ':'
        const char* colon = regStart;
        while (colon > srcStart && *colon != ':' && *colon != '\n')
            --colon;

        if (colon > srcStart && *colon == ':')
        {
            const char* nameEnd2 = colon - 1;

            while (nameEnd2 > srcStart && (*nameEnd2 == ' ' || *nameEnd2 == '\t'))
                --nameEnd2;

            const char* nameStart = nameEnd2;
            while (nameStart >= srcStart
                   && (isalnum (static_cast<unsigned char> (*nameStart)) || *nameStart == '_'))
                --nameStart;
            ++nameStart;

            if (nameStart <= nameEnd2)
            {
                std::string name (nameStart, nameEnd2 - nameStart + 1);
                if (! name.empty())
                    parsedSlots.push_back ({ std::move (name), slot });
            }
        }

        p = reg;
    }

    auto findSlot = [&] (const std::string& name) -> uint32_t
    {
        for (const auto& ns : parsedSlots)
        {
            if (ns.name == name)
                return ns.slot;
        }
        return ~0u;
    };

    auto fillVec = [&] (std::vector<ShaderReflection::ResourceBinding>& bindings)
    {
        for (auto& b : bindings)
            b.backendSlot = findSlot (b.name.toStdString());
    };

    fillVec (ref.uniformBuffers);
    fillVec (ref.storageBuffers);
    fillVec (ref.sampledImages);
    fillVec (ref.separateImages);
    fillVec (ref.separateSamplers);
    fillVec (ref.storageImages);
    fillVec (ref.subpassInputs);
    fillVec (ref.atomicCounters);
    fillVec (ref.accelerationStructures);
    fillVec (ref.glPlainUniforms);
    fillVec (ref.tensors);
    fillVec (ref.pushConstantBuffers);
    fillVec (ref.shaderRecordBuffers);
}

} // namespace

//==============================================================================
// ShaderTranspiler
//==============================================================================

ShaderTranspiler::ShaderTranspiler()
{
    incrementGlslangInit();
}

ShaderTranspiler::~ShaderTranspiler()
{
    decrementGlslangInit();
}

ResultValue<MemoryBlock> ShaderTranspiler::compileToSPIRV (const String& source,
                                                           ShaderStage stage,
                                                           ShaderLanguage sourceLang,
                                                           const TranspileOptions& options)
{
    if (sourceLang != ShaderLanguage::glsl && sourceLang != ShaderLanguage::essl && sourceLang != ShaderLanguage::hlsl)
        return makeResultValueFail ("Unsupported source language for SPIR-V compilation");

    const auto glslStage = toGlslangStage (stage);

    glslang::TShader shader (glslStage);
    shader.setEnvInput (toGlslangSource (sourceLang), glslStage, glslang::EShClientVulkan, 100);

    auto sourceUtf8 = source.toStdString();
    const char* srcPtr = sourceUtf8.c_str();
    const int srcLen = static_cast<int> (sourceUtf8.length());
    shader.setStringsWithLengths (&srcPtr, &srcLen, 1);

    if (options.entryPoint.isNotEmpty())
        shader.setEntryPoint (options.entryPoint.toRawUTF8());

    shader.setSourceEntryPoint (options.entryPoint.toRawUTF8());

    // Inject defines as a preamble
    String preamble;

    HashMap<String, String>::Iterator i (options.defines);
    while (i.next())
    {
        if (i.getValue().isNotEmpty())
            preamble << "#define " << i.getKey() << " " << i.getValue() << "\n";
        else
            preamble << "#define " << i.getKey() << "\n";
    }

    if (preamble.isNotEmpty())
        shader.setPreamble (preamble.toRawUTF8());

    TBuiltInResource resources = getDefaultResources();

    EShMessages messages = static_cast<EShMessages> (EShMsgDefault | EShMsgSpvRules);

    if (sourceLang != ShaderLanguage::essl)
        messages = static_cast<EShMessages> (messages | EShMsgVulkanRules);

    if (sourceLang == ShaderLanguage::hlsl)
        messages = static_cast<EShMessages> (messages | EShMsgReadHlsl);

    int defaultVersion = (sourceLang == ShaderLanguage::essl) ? 300 : 100;

    if (! shader.parse (&resources, defaultVersion, false, messages))
    {
        String infoLog = shader.getInfoLog();
        String debugLog = shader.getInfoDebugLog();

        if (infoLog.isEmpty() && debugLog.isNotEmpty())
            return makeResultValueFail (debugLog);

        return makeResultValueFail (infoLog);
    }

    glslang::TProgram program;
    program.addShader (&shader);

    if (! program.link (messages))
    {
        String infoLog = program.getInfoLog();
        String debugLog = program.getInfoDebugLog();

        if (infoLog.isEmpty() && debugLog.isNotEmpty())
            return makeResultValueFail (debugLog);

        return makeResultValueFail (infoLog);
    }

    std::vector<uint32_t> spirv;
    glslang::SpvOptions spvOptions;
    spvOptions.disableOptimizer = ! options.spirvOptimize;
    spvOptions.optimizeSize = options.spirvOptimize;
    glslang::GlslangToSpv (*program.getIntermediate (glslStage), spirv, &spvOptions);

    if (spirv.empty())
        return makeResultValueFail ("No SPIR-V output produced");

    return makeResultValueOk (spirvToMemoryBlock (spirv));
}

ResultValue<String> ShaderTranspiler::decompileFromSPIRV (const MemoryBlock& spirv,
                                                          ShaderLanguage targetLang,
                                                          const TranspileOptions& options)
{
    if (spirv.getSize() < sizeof (uint32_t) * 5) // smallest valid SPIR-V has 5 words
        return makeResultValueFail ("SPIR-V data is too small to be valid");

    size_t wordCount = 0;
    const uint32_t* words = memoryBlockToSpirvWords (spirv, wordCount);

    if (wordCount == 0)
        return makeResultValueFail ("SPIR-V data is empty");

    try
    {
        const auto entryName = options.entryPoint.toStdString();

        switch (targetLang)
        {
            case ShaderLanguage::glsl:
            case ShaderLanguage::essl:
            {
                spirv_cross::CompilerGLSL compiler (words, wordCount);

                // OpenGL / OpenGL ES have no separate texture / sampler objects:
                // fold every image + sampler pair into a single combined
                // sampler2D, mirroring the inherited descriptor set/binding.
                setupGLCombinedSamplers (compiler);

                spirv_cross::CompilerGLSL::Options glslOpts;

                const bool es = options.es || (targetLang == ShaderLanguage::essl);
                glslOpts.es = es;
                glslOpts.version = es ? 300u : static_cast<uint32_t> (options.glslVersion);
                glslOpts.vulkan_semantics = false;
                glslOpts.vertex.flip_vert_y = ! options.flipVertY;

                compiler.set_common_options (glslOpts);

                if (! entryName.empty())
                {
                    auto entries = compiler.get_entry_points_and_stages();

                    if (! entries.empty())
                        compiler.set_entry_point (entryName, entries[0].execution_model);
                }

                return makeResultValueOk (String (compiler.compile().c_str()));
            }

            case ShaderLanguage::hlsl:
            {
                spirv_cross::CompilerHLSL compiler (words, wordCount);

                spirv_cross::CompilerGLSL::Options commonOpts;
                commonOpts.vertex.flip_vert_y = options.flipVertY;
                compiler.set_common_options (commonOpts);

                spirv_cross::CompilerHLSL::Options hlslOpts;

                if (options.hlslShaderModel >= 10)
                    hlslOpts.shader_model = static_cast<uint32_t> (options.hlslShaderModel);

                compiler.set_hlsl_options (hlslOpts);

                if (! entryName.empty())
                {
                    auto entries = compiler.get_entry_points_and_stages();

                    if (! entries.empty())
                        compiler.set_entry_point (entryName, entries[0].execution_model);
                }

                return makeResultValueOk (String (compiler.compile().c_str()));
            }

            case ShaderLanguage::msl:
            {
                spirv_cross::CompilerMSL compiler (words, wordCount);

                spirv_cross::CompilerGLSL::Options commonOpts;
                commonOpts.vertex.flip_vert_y = options.flipVertY;
                compiler.set_common_options (commonOpts);

                spirv_cross::CompilerMSL::Options mslOpts;
                mslOpts.use_framebuffer_fetch_subpasses = options.mslUsesFramebufferFetch;
                compiler.set_msl_options (mslOpts);

                if (! entryName.empty())
                {
                    auto entries = compiler.get_entry_points_and_stages();

                    if (! entries.empty())
                        compiler.set_entry_point (entryName, entries[0].execution_model);
                }

                return makeResultValueOk (String (compiler.compile().c_str()));
            }

            default:
                return makeResultValueFail ("Unsupported target language for SPIR-V decompilation");
        }
    }
    catch (const std::exception& e)
    {
        return makeResultValueFail (String ("SPIR-V decompilation error: ") + e.what());
    }
}

ResultValue<String> ShaderTranspiler::transpile (const String& source,
                                                 ShaderStage stage,
                                                 ShaderLanguage sourceLang,
                                                 ShaderLanguage targetLang,
                                                 const TranspileOptions& options)
{
    auto spirvResult = compileToSPIRV (source, stage, sourceLang, options);

    if (spirvResult.failed())
        return makeResultValueFail (spirvResult.getErrorMessage());

    return decompileFromSPIRV (spirvResult.getValue(), targetLang, options);
}

ResultValue<ShaderReflection> ShaderTranspiler::reflect (const String& source,
                                                         ShaderStage stage,
                                                         ShaderLanguage sourceLang)
{
    auto spirvResult = compileToSPIRV (source, stage, sourceLang);

    if (spirvResult.failed())
        return makeResultValueFail (spirvResult.getErrorMessage());

    return reflectFromSPIRV (spirvResult.getValue());
}

ResultValue<ShaderReflection> ShaderTranspiler::reflectFromSPIRV (const MemoryBlock& spirv)
{
    if (spirv.getSize() < sizeof (uint32_t) * 5)
        return makeResultValueFail ("SPIR-V data is too small to be valid");

    size_t wordCount = 0;
    const uint32_t* words = memoryBlockToSpirvWords (spirv, wordCount);

    if (wordCount == 0)
        return makeResultValueFail ("SPIR-V data is empty");

    try
    {
        auto compiler = createSpirvCompiler (words, wordCount);
        return makeResultValueOk (extractReflection (*compiler));
    }
    catch (const std::exception& e)
    {
        return makeResultValueFail (String ("SPIR-V reflection error: ") + e.what());
    }
}

ResultValue<ShaderReflection> ShaderTranspiler::reflectFromSPIRV (const MemoryBlock& spirv,
                                                                  ShaderLanguage targetLang,
                                                                  const TranspileOptions& options)
{
    if (spirv.getSize() < sizeof (uint32_t) * 5)
        return makeResultValueFail ("SPIR-V data is too small to be valid");

    size_t wordCount = 0;
    const uint32_t* words = memoryBlockToSpirvWords (spirv, wordCount);

    if (wordCount == 0)
        return makeResultValueFail ("SPIR-V data is empty");

    try
    {
        const auto entryName = options.entryPoint.toStdString();

        switch (targetLang)
        {
            case ShaderLanguage::msl:
            {
                spirv_cross::CompilerMSL compiler (words, wordCount);

                spirv_cross::CompilerGLSL::Options commonOpts;
                commonOpts.vertex.flip_vert_y = options.flipVertY;
                compiler.set_common_options (commonOpts);

                spirv_cross::CompilerMSL::Options mslOpts;
                mslOpts.use_framebuffer_fetch_subpasses = options.mslUsesFramebufferFetch;
                compiler.set_msl_options (mslOpts);

                if (! entryName.empty())
                {
                    auto entries = compiler.get_entry_points_and_stages();

                    if (! entries.empty())
                        compiler.set_entry_point (entryName, entries[0].execution_model);
                }

                compiler.compile(); // triggers slot allocation + combined sampler splitting

                auto ref = extractReflection (compiler);
                fillMSLBackendSlots (compiler, ref);

                return makeResultValueOk (std::move (ref));
            }

            case ShaderLanguage::glsl:
            case ShaderLanguage::essl:
            {
                spirv_cross::CompilerGLSL compiler (words, wordCount);

                // Match decompileFromSPIRV(): fold image+sampler pairs so the
                // reflected slots line up with the emitted combined samplers.
                setupGLCombinedSamplers (compiler);

                spirv_cross::CompilerGLSL::Options glslOpts;
                const bool es = options.es || (targetLang == ShaderLanguage::essl);
                glslOpts.es = es;
                glslOpts.version = es ? 300u : static_cast<uint32_t> (options.glslVersion);
                glslOpts.vulkan_semantics = false;
                glslOpts.vertex.flip_vert_y = options.flipVertY;
                compiler.set_common_options (glslOpts);

                if (! entryName.empty())
                {
                    auto entries = compiler.get_entry_points_and_stages();

                    if (! entries.empty())
                        compiler.set_entry_point (entryName, entries[0].execution_model);
                }

                compiler.compile();

                auto ref = extractReflection (compiler);
                fillGLSLBackendSlots (compiler, ref);

                return makeResultValueOk (std::move (ref));
            }

            case ShaderLanguage::hlsl:
            {
                spirv_cross::CompilerHLSL compiler (words, wordCount);

                spirv_cross::CompilerGLSL::Options commonOpts;
                commonOpts.vertex.flip_vert_y = options.flipVertY;
                compiler.set_common_options (commonOpts);

                spirv_cross::CompilerHLSL::Options hlslOpts;

                if (options.hlslShaderModel >= 10)
                    hlslOpts.shader_model = static_cast<uint32_t> (options.hlslShaderModel);

                compiler.set_hlsl_options (hlslOpts);

                if (! entryName.empty())
                {
                    auto entries = compiler.get_entry_points_and_stages();

                    if (! entries.empty())
                        compiler.set_entry_point (entryName, entries[0].execution_model);
                }

                auto hlslSource = compiler.compile(); // triggers register allocation

                auto ref = extractReflection (compiler);
                fillHLSLBackendSlots (hlslSource, ref);

                return makeResultValueOk (std::move (ref));
            }

            default:
                return makeResultValueFail ("Unsupported target language for backend-aware reflection");
        }
    }
    catch (const std::exception& e)
    {
        return makeResultValueFail (String ("SPIR-V reflection error: ") + e.what());
    }
}

} // namespace yup
