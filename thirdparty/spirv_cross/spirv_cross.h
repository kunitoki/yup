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

    ID:                 spirv_cross
    vendor:             khronos
    version:            0.68.0
    name:               SPIRV-Cross
    description:        SPIRV-Cross is a tool designed for parsing and converting SPIR-V to other shader languages (GLSL, HLSL, MSL, JSON reflection).
    website:            https://github.com/KhronosGroup/SPIRV-Cross
    license:            Apache-2.0 OR MIT

    searchpaths:        upstream

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

#include "upstream/spirv_cross.hpp"
#include "upstream/spirv_parser.hpp"
#include "upstream/spirv_glsl.hpp"
#include "upstream/spirv_hlsl.hpp"
#include "upstream/spirv_msl.hpp"
#include "upstream/spirv_reflect.hpp"
#include "upstream/spirv_cross_util.hpp"
#include "upstream/spirv_cross_c.h"
