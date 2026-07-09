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
/** Shader source or target language. */
enum class ShaderLanguage
{
    glsl = 0, /**< OpenGL GLSL. */
    essl,     /**< OpenGL ES GLSL. */
    hlsl,     /**< Direct3D HLSL. */
    msl,      /**< Metal Shading Language (output only). */
    spirv,    /**< SPIR-V binary (intermediate representation). */
    wgsl      /**< WebGPU Shading Language. */
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
    /** A folded texture+sampler pair emitted by the GLSL/ESSL backend.

        When targeting OpenGL/OpenGL ES, separate @c texture2D / @c sampler
        declarations are combined into a single @c sampler2D uniform (GLES has no
        separate sampler objects). This records the emitted combined uniform name
        and the GL texture unit it must bind to, so the runtime can fix up the
        sampler uniform via @c glUniform1i without parsing the generated source. */
    struct GLCombinedSampler
    {
        /** Emitted combined uniform name (e.g. "yup_combined_u_tex_u_samp"). */
        String name;

        /** GL texture unit the combined sampler must bind to. */
        uint32_t textureSlot = 0;
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

    /** GL combined texture+sampler uniforms emitted for the glsl/essl targets.
        Empty for non-GL targets. Populated by reflectFromSPIRV() when the target
        language is glsl or essl. */
    std::vector<GLCombinedSampler> glCombinedSamplers;

    /** Declared SPIR-V capabilities (e.g. "Shader", "Float64"). */
    std::vector<String> capabilities;

    /** Declared SPIR-V extensions (e.g. "SPV_KHR_16bit_storage"). */
    std::vector<String> extensions;
};

} // namespace yup
