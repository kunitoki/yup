/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#if !YUP_MODULE_AVAILABLE_yup_gui
#error This binding file requires adding the yup_gui module in the project
#else
#include <yup_gui/yup_gui.h>
#endif

#include "../utilities/yup_PyBind11Includes.h"

namespace yup::Bindings {

// =================================================================================================

void registerYupGuiEntryPointsBindings (pybind11::module_& m);

} // namespace yup::Bindings
