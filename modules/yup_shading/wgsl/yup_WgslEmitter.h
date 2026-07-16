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
/** Options controlling WGSL code emission. */
struct WgslEmitOptions
{
    /** Output entry-point name (defaults to "main"). */
    String outputEntryPoint = "main";
};

//==============================================================================
/**
    Emits WGSL 1.0 source code from a lowered AST.

    Handles D3 type mapping, D4 builtin mapping, D5 function/operator mapping,
    and entry-point generation with IO structs as specified in the plan.
*/
class WgslEmitter
{
public:
    WgslEmitter() = default;
    ~WgslEmitter() = default;

    //==========================================================================
    /**
        Emit WGSL 1.0 source code from a lowered program.

        @param program  The lowered program from WgslLowering.
        @param options  Emission options (entry-point name).
        @returns        WGSL 1.0 source code or an error.
    */
    static ResultValue<String> emit (const LoweredProgram& program,
                                     const WgslEmitOptions& options = {});

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WgslEmitter)
};

} // namespace wgsl
} // namespace yup
