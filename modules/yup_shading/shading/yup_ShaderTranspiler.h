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
/** SPIR-V optimization mode applied during compilation. */
enum class SpvOptimizationMode
{
    none,       /**< No optimization (glslang optimizer disabled). */
    size,       /**< Optimize the generated SPIR-V for size. */
    performance /**< Optimize the generated SPIR-V for performance. */
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

    /** When true, compileToSPIRV() negates @c gl_Position.y once a vertex shader's entry
        point returns, mapping the Y-up clip space of the other targets onto Vulkan's
        Y-down one. Ignored by the other stages. */
    bool spirvInvertY = false;

    /** SPIR-V optimization mode (requires SPIRV-Tools linked into glslang). */
    SpvOptimizationMode spirvOptimization = SpvOptimizationMode::none;

    /** When true, run SPIR-V validation after compilation. */
    bool spirvValidate = false;

    /** When true, emit debug info into the SPIR-V binary. */
    bool spirvDebugInfo = false;

    /** Preprocessor defines (name → value, empty string for no-value defines). */
    HashMap<String, String> defines;

    /** Include search paths for resolving #include directives (-I&lt;dir&gt; style). */
    std::vector<String> includePaths;

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
                << "|flipY:" << (flipVertY ? '1' : '0')
                << "|spvInvertY:" << (spirvInvertY ? '1' : '0')
                << "|spvOpt:" << static_cast<int> (spirvOptimization)
                << "|spvValidate:" << (spirvValidate ? '1' : '0')
                << "|spvDebug:" << (spirvDebugInfo ? '1' : '0');

        // Include paths sorted for determinism
        std::vector<String> sortedPaths (includePaths.begin(), includePaths.end());
        std::sort (sortedPaths.begin(), sortedPaths.end());

        for (const auto& p : sortedPaths)
            payload << "|inc:" << p;

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
    The native slots of one graphics pipeline's resources on one backend.

    Metal, Direct3D 11 and OpenGL ignore the descriptor set, so resources of different sets
    that share a binding number would land on the same native slot. A graphics pipeline
    therefore numbers its resources per backend resource class instead, over the union of
    its vertex and fragment resources sorted by (set, binding):
    - MSL: buffers (uniform and storage), textures (sampled and storage) and samplers.
    - HLSL: @c b (uniform buffers), @c t (textures and read-only storage buffers), @c s
      (samplers) and @c u (read-write storage buffers and storage images).
    - GLSL / ESSL: uniform buffer binding points, texture units and storage buffer binding
      points, shared by both stages. A separate sampler takes the unit of the texture it is
      paired with; storage images keep their binding.

    Vulkan (SPIR-V) and WebGPU (WGSL) bind (set, binding) natively and get no entries.

    @see ShaderTranspiler::assignGraphicsNativeSlots
*/
struct ShaderNativeSlots
{
    /** The native slots of the resource at one (set, binding). */
    struct Entry
    {
        /** Descriptor set of the resource. */
        uint32_t set = 0;

        /** Binding of the resource within its set. */
        uint32_t binding = 0;

        /** Index of the resource in its own class: buffer, texture, sampler or UAV register,
            GL uniform buffer or storage buffer binding point, or GL texture unit. */
        uint32_t slot = ~0u;

        /** Sampler index of a combined image sampler (MSL and HLSL), ~0u otherwise. */
        uint32_t samplerSlot = ~0u;
    };

    /** One entry per (set, binding) that has a native slot on this backend. */
    std::vector<Entry> entries;

    /** Returns the entry for a (set, binding), or nullptr when it has no assigned slot. */
    [[nodiscard]] const Entry* find (uint32_t set, uint32_t binding) const
    {
        for (const auto& entry : entries)
        {
            if (entry.set == set && entry.binding == binding)
                return &entry;
        }

        return nullptr;
    }
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

        @param spirv        SPIR-V binary data.
        @param targetLang   Target language (glsl, essl, hlsl, msl).
        @param options      Decompilation options.
        @param nativeSlots  The graphics pipeline's slots from assignGraphicsNativeSlots(), applied
                            to the emitted MSL and HLSL resource indices. When null, MSL and HLSL
                            use each resource's binding.

        @returns A ResultValue containing the target source code on success,
                 or an error message on failure.
    */
    ResultValue<String> decompileFromSPIRV (const MemoryBlock& spirv,
                                            ShaderLanguage targetLang,
                                            const TranspileOptions& options = {},
                                            const ShaderNativeSlots* nativeSlots = nullptr);

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

        Creates a backend-specific compiler (MSL, GLSL, HLSL), compiles the SPIR-V
        to trigger slot allocation, and extracts reflection data that includes
        the backend-assigned slot indices in each ResourceBinding::backendSlot.

        For MSL: queries CompilerMSL::get_automatic_msl_resource_binding() after compile().
        For GLSL/ESSL: the slot from @p nativeSlots, or the SPIR-V binding without it.
        For HLSL: parses register(bN)/register(tN)/register(sN)/register(uN) from
        the compiled source. This relies on spirv-cross's stable output format.

        @param spirv        SPIR-V binary data.
        @param targetLang   Target backend language (e.g., msl, glsl, essl, hlsl).
        @param options      Options that affect slot assignment (e.g., entry point).
        @param nativeSlots  The graphics pipeline's slots from assignGraphicsNativeSlots(). Pass
                            the same slots given to decompileFromSPIRV(), so the reflection
                            matches the emitted source.

        @returns A ResultValue containing ShaderReflection on success,
                 or an error message on failure.
    */
    ResultValue<ShaderReflection> reflectFromSPIRV (const MemoryBlock& spirv,
                                                    ShaderLanguage targetLang,
                                                    const TranspileOptions& options = {},
                                                    const ShaderNativeSlots* nativeSlots = nullptr);

    //==========================================================================
    /**
        Assigns the native slots of one graphics pipeline on one backend.

        Walks the union of both stages' resources sorted by (set, binding) and numbers them
        sequentially per resource class of @p targetLang, as described in ShaderNativeSlots.
        A resource used by both stages gets the same slot in each. Arrays take one slot per
        element.

        @param vertex      Reflection of the vertex stage's SPIR-V, from reflectFromSPIRV().
        @param fragment    Reflection of the fragment stage's SPIR-V, from reflectFromSPIRV().
        @param targetLang  Backend language. Only msl, hlsl, glsl and essl get entries.

        @returns The slots to pass to decompileFromSPIRV() and reflectFromSPIRV() for both stages.
    */
    static ShaderNativeSlots assignGraphicsNativeSlots (const ShaderReflection& vertex,
                                                        const ShaderReflection& fragment,
                                                        ShaderLanguage targetLang);
};

} // namespace yup
