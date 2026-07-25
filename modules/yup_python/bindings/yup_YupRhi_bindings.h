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

#if ! YUP_MODULE_AVAILABLE_yup_graphics
#error This binding file requires adding the yup_graphics module in the project
#else
#include <yup_graphics/yup_graphics.h>
#endif

#include "yup_YupCore_bindings.h"
#include "yup_YupGraphics_bindings.h"

#define YUP_PYTHON_INCLUDE_PYBIND11_OPERATORS
#define YUP_PYTHON_INCLUDE_PYBIND11_STL
#include "../utilities/yup_PyBind11Includes.h"

namespace yup::Bindings
{

//==============================================================================

void registerYupRhiBindings (pybind11::module_& m);

} // namespace yup::Bindings
