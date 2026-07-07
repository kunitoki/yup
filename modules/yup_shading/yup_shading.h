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

    ID:                 yup_shading
    vendor:             yup
    version:            2.0.0
    name:               YUP Shading Classes
    description:        The essential set of basic YUP shading classes.
    website:            https://github.com/kunitoki/yup
    license:            ISC

    dependencies:       yup_core glslang spirv_cross

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_SHADING_H_INCLUDED

#include <yup_core/yup_core.h>

//==============================================================================
#include "shading/yup_ShaderTranspiler.h"
#include "shading/yup_ShaderCache.h"
