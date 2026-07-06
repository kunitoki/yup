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

/*
  ==============================================================================

  BEGIN_YUP_MODULE_DECLARATION

    ID:                 glslang
    vendor:             khronos
    version:            16.3.0
    name:               glslang GLSL/HLSL Reference Shader Compiler
    description:        glslang is the official Khronos reference front-end for GLSL, ESSL, and HLSL shader languages, with SPIR-V backend code generation.
    website:            https://github.com/KhronosGroup/glslang
    license:            BSD-3-Clause

    defines:            ENABLE_HLSL=1
    searchpaths:        upstream upstream/SPIRV

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

#include "upstream/glslang/Public/ShaderLang.h"
#include "upstream/glslang/Public/ResourceLimits.h"
#include "upstream/SPIRV/GlslangToSpv.h"
