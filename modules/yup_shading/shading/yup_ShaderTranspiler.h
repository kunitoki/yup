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

        Creates a backend-specific compiler (MSL, GLSL, HLSL), compiles the SPIR-V
        to trigger slot allocation, and extracts reflection data that includes
        the backend-assigned slot indices in each ResourceBinding::backendSlot.

        For MSL: queries CompilerMSL::get_automatic_msl_resource_binding() after compile().
        For GLSL/ESSL: copies the SPIR-V binding (no remapping occurs).
        For HLSL: parses register(bN)/register(tN)/register(sN)/register(uN) from
        the compiled source. This relies on spirv-cross's stable output format.

        @param spirv       SPIR-V binary data.
        @param targetLang  Target backend language (e.g., msl, glsl, essl, hlsl).
        @param options     Options that affect slot assignment (e.g., entry point).

        @returns A ResultValue containing ShaderReflection on success,
                 or an error message on failure.
    */
    ResultValue<ShaderReflection> reflectFromSPIRV (const MemoryBlock& spirv,
                                                    ShaderLanguage targetLang,
                                                    const TranspileOptions& options = {});
};

} // namespace yup
