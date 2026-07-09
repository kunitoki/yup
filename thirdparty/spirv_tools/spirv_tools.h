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

    ID:                 spirv_tools
    vendor:             khronos
    version:            0.68.0
    name:               SPIRV-Tools
    description:        SPIRV-Tools is a collection of tools for processing SPIR-V, including validation, optimization, and reflection.
    website:            https://github.com/KhronosGroup/SPIRV-Tools
    license:            Apache-2.0 OR MIT

    searchpaths:        upstream upstream/include upstream/include/spirv/unified1 upstream/generated
    defines:            ENABLE_OPT=1

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

//==============================================================================
/** Config: YUP_SPIRV_TOOLS_ENABLE_LINTER

    Enable SPIRV-Tools linter support.
*/

#if ! YUP_SPIRV_TOOLS_ENABLE_LINTER
#define YUP_SPIRV_TOOLS_ENABLE_LINTER 1
#endif

//==============================================================================
#include "upstream/include/spirv-tools/optimizer.hpp"

#if YUP_SPIRV_TOOLS_ENABLE_LINTER
#include "upstream/include/spirv-tools/linter.hpp"
#endif

//==============================================================================
#include "upstream/include/spirv-tools/libspirv.h"
#include "upstream/include/spirv-tools/libspirv.hpp"
