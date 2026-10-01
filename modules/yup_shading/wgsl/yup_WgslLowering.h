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
/** Options controlling WGSL lowering behavior. */
struct WgslLoweringOptions
{
    ShaderStage stage = ShaderStage::vertex;
    uint32_t defaultGroup = 0;
    std::array<uint32_t, 3> defaultWorkgroupSize { 1, 1, 1 };

    /** Name of the WGSL entry-point function. */
    std::string entryPointName = "main";
};

//==============================================================================
/** Result of lowering: an AST using only constructs WGSL can express, plus the metadata the emitter needs. */
struct LoweredProgram
{
    TranslationUnit ast;

    /** One field of the entry-point input or output struct. */
    struct InputOutputInfo
    {
        std::string name;        // global (private) variable the field is copied from or to
        std::string fieldName;   // member name in the IO struct
        TypeSpecifier wgslType;  // type of the IO struct member
        uint32_t location = 0;
        bool isBuiltin = false;
        std::string builtinName;       // e.g. "position", "vertex_index"
        std::string interpolation;     // e.g. "flat", "linear, centroid"; empty for the default
        bool invariant = false;
        int blendSource = -1;          // dual-source blending index, -1 when unused
    };

    struct EntryPointWrapper
    {
        std::string wgslEntryPoint;   // name of the @vertex/@fragment/@compute function
        std::string innerFunction;    // the lowered GLSL main()
        std::string inputStruct;      // name of the input struct type
        std::string outputStruct;     // name of the output struct type
        std::string inputParameter;   // name of the input struct parameter
        std::string outputVariable;   // name of the output struct local
        bool isVertex = false;
        bool isFragment = false;
        bool isCompute = false;

        std::vector<InputOutputInfo> inputs;
        std::vector<InputOutputInfo> outputs;

        std::vector<std::string> inputCopies;  // WGSL statements filling the private variables from the input struct
        std::vector<std::string> outputCopies; // WGSL statements filling the output struct from the private variables

        uint32_t workgroupSizeX = 1;
        uint32_t workgroupSizeY = 1;
        uint32_t workgroupSizeZ = 1;
        std::array<std::string, 3> workgroupSizeOverrides; // override names for local_size_*_id, empty otherwise
    };

    EntryPointWrapper entryPoint;

    /** A resource and the @group/@binding it was given. */
    struct ResourceAssignment
    {
        std::string name;
        uint32_t group = 0;
        uint32_t binding = 0;
        uint32_t samplerBinding = ~0u; // companion sampler binding for a split combined sampler
        std::string samplerName;       // companion sampler variable name for a split combined sampler
        bool isSampler = false;
    };

    std::vector<ResourceAssignment> resources;

    /** WGSL helper functions GLSL builtins were lowered to, emitted before any user code. */
    std::vector<std::string> polyfills;

    /** Separate textures sampled with a shadow sampler, which WGSL declares as depth textures. */
    std::set<std::string> depthTextures;

    /** WGSL enable directives the output needs, e.g. "dual_source_blending". */
    std::vector<std::string> enables;

    /** Non-fatal diagnostics, formatted as "line:column: message". */
    std::vector<std::string> warnings;
};

//==============================================================================
/** A resource as seen by the companion sampler allocation rule. */
struct WgslBindingSlot
{
    uint32_t group = 0;
    uint32_t binding = 0;
    bool isCombinedSampler = false;
};

/**
    Assigns the binding of the sampler that a combined image sampler is split into.

    Within each group, companion samplers take the bindings after the highest binding
    used by any resource of that group, in ascending order of their texture binding.
    Both the WGSL lowering and ShaderTranspiler::reflectFromSPIRV() use this rule, so
    the emitted @binding values and the reflected backendSlotSecondary always agree.

    @returns One entry per input slot: the companion binding for combined samplers, ~0u otherwise.
*/
std::vector<uint32_t> assignWgslCompanionSamplerBindings (const std::vector<WgslBindingSlot>& slots);

//==============================================================================
/**
    Transforms a GLSL AST into a form the WGSL emitter can print directly.

    - Normalizes declarations: names interface blocks, hoists struct definitions,
      flattens stage IO blocks and renames identifiers that are reserved in WGSL
    - Rejects every construct WGSL can't express faithfully with a line:column error
    - Assigns @group/@binding to resources and @location to stage IO
    - Makes the implicit conversions of GLSL explicit (WgslTypeLegalizer)
    - Rewrites statements and expressions WGSL lacks: out/inout parameters, side
      effects inside expressions, switch fallthrough, do-while, swizzle stores,
      GLSL builtin functions and texture sampling
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
        @returns        The lowered program with metadata, or a "line:column: message" error.
    */
    static ResultValue<LoweredProgram> lower (TranslationUnit ast,
                                              const WgslLoweringOptions& options = {});

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WgslLowering)
};

} // namespace wgsl
} // namespace yup
