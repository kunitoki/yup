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
/**
    Emits WGSL 1.0 source code from a lowered AST.

    The lowered program only contains constructs WGSL can express, so emission is a
    direct print of the AST plus the entry-point wrapper described by the program's
    stage IO metadata.
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
        @returns        WGSL 1.0 source code or an error.
    */
    static ResultValue<String> emit (const LoweredProgram& program);

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WgslEmitter)
};

} // namespace wgsl
} // namespace yup
