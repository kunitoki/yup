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

namespace wgsl
{

//==============================================================================
/** Symbol table entry: variable type, mutability, and parameter qualifier. */
struct SymbolInfo
{
    TypeSpecifier type;
    bool isConst = false;      // declared as const
    bool isReassigned = false; // set true when the symbol is written to after init
    bool isParameter = false;
    bool isOutParam = false;
    bool isInoutParam = false;
    bool isGlobal = false;
    bool isBuiltin = false;
};

//==============================================================================
/** Options controlling WGSL lowering behavior. */
struct WgslLoweringOptions
{
    ShaderStage stage = ShaderStage::vertex;
    uint32_t defaultGroup = 0;
    std::array<uint32_t, 3> defaultWorkgroupSize { 1, 1, 1 };
};

//==============================================================================
/** Result of lowering: the transformed AST + metadata for the emitter. */
struct LoweredProgram
{
    TranslationUnit ast;

    // Entry-point wrapping (from Task 2.5)
    struct InputOutputInfo
    {
        std::string name;
        TypeSpecifier wgslType;
        uint32_t location = 0;
        bool isBuiltin = false;
        std::string builtinName; // e.g. "position", "vertex_index"
    };

    struct EntryPointWrapper
    {
        std::string originalEntryPoint; // main
        std::string wgslEntryPoint;     // main
        std::string innerFunction;      // main_inner
        bool isVertex = false;
        bool isFragment = false;
        bool isCompute = false;

        std::vector<InputOutputInfo> inputs;
        std::vector<InputOutputInfo> outputs;
        uint32_t workgroupSizeX = 1;
        uint32_t workgroupSizeY = 1;
        uint32_t workgroupSizeZ = 1;
    };

    EntryPointWrapper entryPoint;

    // Resource bindings assigned during lowering
    struct ResourceAssignment
    {
        std::string name;
        uint32_t group = 0;
        uint32_t binding = 0;
        uint32_t samplerBinding = ~0u; // secondary binding for combined sampler split
        bool isSampler = false;
    };

    std::vector<ResourceAssignment> resources;

    // Polyfill functions needed (inverse, etc.)
    std::vector<std::string> polyfills;
};

//==============================================================================
/**
    Transforms a GLSL AST into a lowering-ready form for WGSL emission.

    Applies all lowering passes specified in Tasks 2.1–2.7:
    - Symbol table + minimal type inference
    - Mutability analysis (let/var, param reassignment → shadow copy)
    - out/inout → ptr<function, T>
    - Resource lowering (uniform blocks, combined samplers, binding assignment)
    - Stage IO lowering (entry-point wrapping)
    - Statement/expression legalization (floor-mod, ternary, do-while, etc.)
    - Diagnostics (unsupported types/stages/features)
*/
class WgslLowering
{
public:
    WgslLowering() = default;
    ~WgslLowering() = default;

    //==========================================================================
    /**
        Lower a GLSL TranslationUnit AST into a form ready for WGSL emission.

        @param ast      The parsed GLSL AST.
        @param options  Stage and binding configuration.
        @returns        The lowered program with metadata, or an error.
    */
    static ResultValue<LoweredProgram> lower (TranslationUnit ast,
                                              const WgslLoweringOptions& options = {});

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WgslLowering)
};

} // namespace wgsl
} // namespace yup
