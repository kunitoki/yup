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

#include <yup_core/yup_core.h>

namespace yup
{

//==============================================================================
/** Shader source or target language. */
enum class ShaderLanguage
{
    glsl = 0, /**< OpenGL GLSL. */
    essl,     /**< OpenGL ES GLSL. */
    hlsl,     /**< Direct3D HLSL. */
    msl,      /**< Metal Shading Language (output only). */
    spirv     /**< SPIR-V binary (intermediate representation). */
};

//==============================================================================
/** Shader pipeline stage. */
enum class ShaderStage
{
    vertex,
    fragment,
    compute,
    geometry,
    tessControl,
    tessEval
};

//==============================================================================
/** Options controlling transpilation behavior. */
struct TranspileOptions
{
    /** Entry point function name. Default is "main". */
    String entryPoint = "main";

    /** GLSL version (e.g. 330, 450). Default is 450. */
    int glslVersion = 450;

    /** If true, generate "#version N es" instead of "#version N". */
    bool es = false;

    /** HLSL shader model version (e.g. 50 = SM 5.0, 60 = SM 6.0). */
    int hlslShaderModel = 50;

    /** Enable Metal framebuffer fetch for subpass inputs. */
    bool mslUsesFramebufferFetch = false;

    /** When true, flip the Y coordinate in vertex output (MSL). */
    bool flipVertY = false;

    /** Preprocessor defines (name → value, empty string for no-value defines). */
    HashMap<String, String> defines;

    //==========================================================================
    /**
        Generates a deterministic string payload suitable for cache key hashing.

        All fields that affect compilation output are included. When adding new
        fields to TranspileOptions, update this method.
    */
    [[nodiscard]] String toCacheKeyPayload() const
    {
        String payload;
        payload << "entry:" << entryPoint
                << "|glslV:" << glslVersion
                << "|es:" << (es ? '1' : '0')
                << "|hlslSM:" << hlslShaderModel
                << "|mslFBF:" << (mslUsesFramebufferFetch ? '1' : '0')
                << "|flipY:" << (flipVertY ? '1' : '0');

        // Defines sorted for determinism
        std::vector<std::pair<String, String>> sortedDefines;
        sortedDefines.reserve (defines.size());

        HashMap<String, String>::Iterator it (defines);
        while (it.next())
            sortedDefines.push_back (std::make_pair (it.getKey(), it.getValue()));

        std::sort (sortedDefines.begin(), sortedDefines.end(), [] (const auto& a, const auto& b)
        {
            return a.first < b.first;
        });

        for (const auto& [key, value] : sortedDefines)
        {
            payload << "|d:" << key;

            if (value.isNotEmpty())
                payload << '=' << value;
        }

        return payload;
    }
};

//==============================================================================
/**
    Complete shader reflection data extracted from a compiled shader.

    Contains all uniforms, textures, samplers, inputs, outputs, push constants,
    specialization constants, built-in variables, compute workgroup size, and
    declared capabilities/extensions.

    @see ShaderTranspiler::reflect, ShaderTranspiler::reflectFromSPIRV
*/
struct ShaderReflection
{
    //==========================================================================
    /** Base data type (mirrors SPIR-V type system). */
    enum class BaseType
    {
        unknown,
        voidType,
        boolean,
        int8,
        uint8,
        int16,
        uint16,
        int32,
        uint32,
        int64,
        uint64,
        half,    /**< 16-bit float. */
        float32, /**< 32-bit float. */
        float64, /**< 64-bit float. */
        atomicCounter,
        structType,
        image,
        sampledImage,
        sampler,
        accelerationStructure,
        rayQuery
    };

    //==========================================================================
    /** Resource classification. */
    enum class ResourceType
    {
        unknown,
        uniformBuffer,
        storageBuffer,
        stageInput,
        stageOutput,
        subpassInput,
        storageImage,
        sampledImage,
        atomicCounter,
        pushConstant,
        separateImage,
        separateSamplers,
        accelerationStructure,
        rayQuery,
        shaderRecordBuffer,
        glPlainUniform,
        tensor
    };

    //==========================================================================
    /** Built-in shader variable type. */
    enum class BuiltInType
    {
        unknown,
        position,
        pointSize,
        clipDistance,
        cullDistance,
        vertexId,
        instanceId,
        primitiveId,
        vertexIndex,
        instanceIndex,
        baseVertex,
        baseInstance,
        drawIndex,
        fragCoord,
        pointCoord,
        frontFacing,
        sampleId,
        samplePosition,
        sampleMask,
        fragDepth,
        helperInvocation,
        numWorkgroups,
        workgroupSize,
        workgroupId,
        localInvocationId,
        globalInvocationId,
        localInvocationIndex,
        viewportIndex,
        layer,
        tessLevelOuter,
        tessLevelInner,
        tessCoord,
        patchVertices,
        baryCoordNoPerspAMD,
        baryCoordNoPerspCentroidAMD,
        baryCoordPullModel,
        deviceIndex,
        fragStencilRefEXT,
        viewportMaskNV,
        secondaryPositionNV,
        secondaryViewportMaskNV,
        positionPerViewNV,
        viewportMaskPerViewNV,
        fullyCoveredEXT,
        fragSizeEXT,
        fragInvocationCountEXT,
        launchIdKHR,
        launchSizeKHR,
        subgroupIdKHR,
        numSubgroupsKHR,
        subgroupEqMaskKHR,
        subgroupGeMaskKHR,
        subgroupGtMaskKHR,
        subgroupLeMaskKHR,
        subgroupLtMaskKHR,
        subgroupLocalInvocationIdKHR,
        subgroupSizeKHR,
        cullPrimitiveEXT,
        hitKindKHR,
        incomingRayFlagsKHR,
        instanceCustomIndexKHR,
        rayGeometryIndexKHR,
        objectRayDirectionKHR,
        objectRayOriginKHR,
        objectToWorldKHR,
        worldToObjectKHR,
        rayTminKHR,
        rayTmaxKHR,
        worldRayDirectionKHR,
        worldRayOriginKHR,
        shaderIndexKHR,
        baryCoordKHR,
        baryCoordNoPerspKHR
    };

    //==========================================================================
    /** Image/texture dimension. */
    enum class ImageDimension
    {
        unknown,
        dim1D,
        dim2D,
        dim2DArray,
        dim3D,
        cube,
        cubeArray,
        dimRect,
        dimBuffer,
        dimSubpass
    };

    //==========================================================================
    /** Per-member layout info for struct/block types. */
    struct ResourceMember
    {
        String name;
        uint32_t offset = 0;
        uint32_t size = 0;
        uint32_t arrayStride = 0;
        uint32_t matrixStride = 0;
        BaseType baseType = BaseType::unknown;
        uint32_t vecSize = 1;
        uint32_t columns = 1;
        uint32_t bitWidth = 32;
        std::vector<uint32_t> arraySizes;
    };

    //==========================================================================
    /** Describes a single shader resource binding. */
    struct ResourceBinding
    {
        /** Shader variable name. */
        String name;

        /** Resource classification. */
        ResourceType type = ResourceType::unknown;

        /** Descriptor set index (Vulkan) or register space (D3D12). */
        uint32_t set = 0;

        /** Binding point (Vulkan/GL) or register number (D3D). */
        uint32_t binding = 0;

        /** Input/output location (for stage I/O variables). */
        uint32_t location = 0;

        /** Number of descriptors if the resource is an array. */
        uint32_t descriptorCount = 1;

        /** Base type of the resource. */
        BaseType baseType = BaseType::unknown;

        /** Vector size: 1=scalar, 2=vec2, 3=vec3, 4=vec4. */
        uint32_t vecSize = 1;

        /** Matrix columns: 1=not a matrix, 2=mat2, 3=mat3, 4=mat4. */
        uint32_t columns = 1;

        /** Bit width of the underlying scalar type. */
        uint32_t bitWidth = 32;

        /** Array dimensions (outermost first). Empty for non-array types. */
        std::vector<uint32_t> arraySizes;

        /** Member layout (only populated for struct/block types). */
        std::vector<ResourceMember> members;

        /** Image dimension (valid when baseType is image/sampledImage). */
        ImageDimension imageDim = ImageDimension::unknown;

        /** Whether the image has depth comparison. */
        bool imageIsDepth = false;

        /** Whether the image is arrayed. */
        bool imageArrayed = false;

        /** Whether the image is multisampled. */
        bool imageMS = false;

        /** SPIR-V ImageFormat value (0 = unknown). */
        uint32_t imageFormat = 0;

        /** Total block size in bytes (for uniform/storage buffer blocks). */
        uint32_t blockSize = 0;

        /** Internal resource variable ID (for backend slot queries). */
        uint32_t resourceId = 0;

        /** Backend-assigned native slot index.
            Populated by reflectFromSPIRV() when a target language is provided.
            For MSL: the [[buffer(N)]], [[texture(N)]], [[sampler(N)]], or [[id(N)]] index.
            For GLSL/ESSL: same as the SPIR-V binding point.
            Set to ~0u (~0u) when not populated. */
        uint32_t backendSlot = ~0u;

        /** Secondary backend-assigned slot.
            Populated by reflectFromSPIRV() when a target language is provided.
            For MSL: the sampler half of a combined-image-sampler resource.
            Set to ~0u (~0u) when not applicable. */
        uint32_t backendSlotSecondary = ~0u;
    };

    //==========================================================================
    /** Describes a built-in variable (e.g. gl_Position, SV_Position). */
    struct BuiltInBinding
    {
        /** Which built-in this is. */
        BuiltInType builtin = BuiltInType::unknown;

        /** Base type of the built-in's value. */
        BaseType valueBaseType = BaseType::unknown;

        /** Vector size of the built-in's value. */
        uint32_t valueVecSize = 1;

        /** Matrix columns of the built-in's value. */
        uint32_t valueColumns = 1;

        /** The resource binding that contains this built-in (if part of a block). */
        ResourceBinding resource;
    };

    //==========================================================================
    /** Describes a specialization constant. */
    struct SpecializationConstant
    {
        /** Shader variable name. */
        String name;

        /** The specialization constant ID (used at pipeline creation). */
        uint32_t constantId = 0;

        /** Base type. */
        BaseType baseType = BaseType::unknown;

        /** Vector size. */
        uint32_t vecSize = 1;

        /** Matrix columns. */
        uint32_t columns = 1;

        /** Bit width. */
        uint32_t bitWidth = 32;
    };

    //==========================================================================
    /** An entry point in the shader. */
    struct EntryPoint
    {
        String name;
        ShaderStage stage;
    };

    //==========================================================================
    /** Workgroup size for compute shaders. */
    struct WorkgroupSize
    {
        uint32_t x = 1;
        uint32_t y = 1;
        uint32_t z = 1;

        /** Whether any dimension uses a specialization constant. */
        bool usesSpecializationConstants = false;

        /** Specialization constant IDs for each dimension (0 if unused). */
        uint32_t specConstantIdX = 0;
        uint32_t specConstantIdY = 0;
        uint32_t specConstantIdZ = 0;
    };

    // -- Data --

    std::vector<EntryPoint> entryPoints;

    std::vector<ResourceBinding> uniformBuffers;
    std::vector<ResourceBinding> storageBuffers;
    std::vector<ResourceBinding> stageInputs;
    std::vector<ResourceBinding> stageOutputs;
    std::vector<ResourceBinding> subpassInputs;
    std::vector<ResourceBinding> storageImages;
    std::vector<ResourceBinding> sampledImages;
    std::vector<ResourceBinding> atomicCounters;
    std::vector<ResourceBinding> accelerationStructures;
    std::vector<ResourceBinding> glPlainUniforms;
    std::vector<ResourceBinding> tensors;
    std::vector<ResourceBinding> pushConstantBuffers;
    std::vector<ResourceBinding> shaderRecordBuffers;
    std::vector<ResourceBinding> separateImages;
    std::vector<ResourceBinding> separateSamplers;

    std::vector<BuiltInBinding> builtinInputs;
    std::vector<BuiltInBinding> builtinOutputs;

    std::vector<SpecializationConstant> specConstants;

    WorkgroupSize workgroupSize;

    bool positionInvariant = false;

    /** Declared SPIR-V capabilities (e.g. "Shader", "Float64"). */
    std::vector<String> capabilities;

    /** Declared SPIR-V extensions (e.g. "SPV_KHR_16bit_storage"). */
    std::vector<String> extensions;
};

//==============================================================================
/**
    A shader transpiler that can compile and decompile between shading languages.

    Uses glslang for source-to-SPIR-V compilation and spirv_cross for
    SPIR-V-to-target decompilation. Also provides full shader reflection.

    The transpiler must outlive any ShaderCache that references it.

    @code
    auto transpiler = makeReferenceCounted<ShaderTranspiler>();

    auto result = transpiler->transpile (glslSource, ShaderStage::vertex,
                                         ShaderLanguage::glsl,
                                         ShaderLanguage::msl);
    if (result)
        DBG ("MSL output: " << result.getValue());

    auto reflect = transpiler->reflect (glslSource, ShaderStage::fragment,
                                        ShaderLanguage::glsl);
    if (reflect)
        for (auto& ub : reflect.getValue().uniformBuffers)
            DBG ("UBO: " << ub.name << " binding=" << ub.binding);
    @endcode

    @see ShaderCache
*/
class YUP_API ShaderTranspiler final : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<ShaderTranspiler>;

    ShaderTranspiler();
    ~ShaderTranspiler() override;

    //==========================================================================
    /**
        Compile shader source to SPIR-V binary.

        @param source       The shader source code (GLSL or HLSL).
        @param stage        Pipeline stage of the shader.
        @param sourceLang   Source language (glsl, essl, or hlsl).
        @param options      Compilation options (entry point, defines, version).

        @returns A ResultValue containing SPIR-V binary data on success,
                 or an error message on failure.
    */
    ResultValue<MemoryBlock> compileToSPIRV (const String& source,
                                             ShaderStage stage,
                                             ShaderLanguage sourceLang,
                                             const TranspileOptions& options = {});

    //==========================================================================
    /**
        Decompile SPIR-V binary to a target shading language.

        @param spirv       SPIR-V binary data.
        @param targetLang  Target language (glsl, essl, hlsl, msl).
        @param options     Decompilation options.

        @returns A ResultValue containing the target source code on success,
                 or an error message on failure.
    */
    ResultValue<String> decompileFromSPIRV (const MemoryBlock& spirv,
                                            ShaderLanguage targetLang,
                                            const TranspileOptions& options = {});

    //==========================================================================
    /**
        One-shot transpile: source language → SPIR-V → target language.

        This is equivalent to compileToSPIRV() followed by decompileFromSPIRV().

        @param source       The shader source code.
        @param stage        Pipeline stage.
        @param sourceLang   Source language (glsl, essl, or hlsl).
        @param targetLang   Target language (glsl, essl, hlsl, msl).
        @param options      Transpilation options.

        @returns A ResultValue containing the target source code on success,
                 or an error message on failure.
    */
    ResultValue<String> transpile (const String& source,
                                   ShaderStage stage,
                                   ShaderLanguage sourceLang,
                                   ShaderLanguage targetLang,
                                   const TranspileOptions& options = {});

    //==========================================================================
    /**
        Extract full reflection data from shader source.

        Internally compiles to SPIR-V and then reflects.

        @param source       The shader source code.
        @param stage        Pipeline stage.
        @param sourceLang   Source language (glsl, essl, or hlsl).

        @returns A ResultValue containing ShaderReflection on success,
                 or an error message on failure.
    */
    ResultValue<ShaderReflection> reflect (const String& source,
                                           ShaderStage stage,
                                           ShaderLanguage sourceLang);

    //==========================================================================
    /**
        Extract full reflection data from SPIR-V binary.

        @param spirv  SPIR-V binary data.

        @returns A ResultValue containing ShaderReflection on success,
                 or an error message on failure.
    */
    ResultValue<ShaderReflection> reflectFromSPIRV (const MemoryBlock& spirv);

    //==========================================================================
    /**
        Extract reflection data with backend-assigned native slot numbers.

        Creates a backend-specific compiler (MSL, GLSL, etc.), compiles the SPIR-V
        to trigger slot allocation, and extracts reflection data that includes
        the backend-assigned slot indices in each ResourceBinding::backendSlot.

        @param spirv       SPIR-V binary data.
        @param targetLang  Target backend language (e.g., msl, glsl, essl).
        @param options     Options that affect slot assignment (e.g., entry point).

        @returns A ResultValue containing ShaderReflection on success,
                 or an error message on failure.
    */
    ResultValue<ShaderReflection> reflectFromSPIRV (const MemoryBlock& spirv,
                                                    ShaderLanguage targetLang,
                                                    const TranspileOptions& options = {});
};

} // namespace yup
