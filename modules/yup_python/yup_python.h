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

/*
 BEGIN_YUP_MODULE_DECLARATION

  ID:                 yup_python
  vendor:             kunitoki
  version:            1.0.0
  name:               YUP Python Bindings
  description:        The python bindings to create and work on YUP apps.
  website:            https://github.com/kunitoki/yup
  license:            ISC

  dependencies:       yup_core
  needsPython:        true

 END_YUP_MODULE_DECLARATION
*/

#pragma once
#define YUP_GUI_H_INCLUDED

//==============================================================================
/** Config: YUP_PYTHON_USE_EXTERNAL_PYBIND11

    Enable externally provided pybind11 installation.
*/
#ifndef YUP_PYTHON_USE_EXTERNAL_PYBIND11
#define YUP_PYTHON_USE_EXTERNAL_PYBIND11 0
#endif

//==============================================================================
/** Config: YUP_PYTHON_EMBEDDED_INTERPRETER

    Enable or disable embedding the interpreter. This should be disabled when building standalone wheels.
*/
#ifndef YUP_PYTHON_EMBEDDED_INTERPRETER
#define YUP_PYTHON_EMBEDDED_INTERPRETER 1
#endif

//==============================================================================
/** Config: YUP_PYTHON_SCRIPT_CATCH_EXCEPTION

    Enable or disable catching script exceptions.
*/
#ifndef YUP_PYTHON_SCRIPT_CATCH_EXCEPTION
#define YUP_PYTHON_SCRIPT_CATCH_EXCEPTION 1
#endif

//==============================================================================
/** Config: YUP_PYTHON_THREAD_CATCH_EXCEPTION

    Enable or disable catching yup::Thread exceptions raised from python.
*/
#ifndef YUP_PYTHON_THREAD_CATCH_EXCEPTION
#define YUP_PYTHON_THREAD_CATCH_EXCEPTION 1
#endif

//==============================================================================

#include "utilities/yup_MacroHelpers.h"

/**
 * @brief Custom module name, it's possible to change but beware to update your `import` statements !
 */
#ifndef YUP_PYTHON_MODULE_NAME
#define YUP_PYTHON_MODULE_NAME yup
#endif

/**
 * @brief Custom python module name as string.
 */
namespace yup
{

static inline constexpr const char* const PythonModuleName = YUP_PYTHON_STRINGIFY (YUP_PYTHON_MODULE_NAME);

} // namespace yup

//==============================================================================
/**
 * @brief Modal loops are required for yup python to work when built as a wheel.
 */
#if ! YUP_PYTHON_EMBEDDED_INTERPRETER && ! YUP_MODAL_LOOPS_PERMITTED
#error When building yup_python with YUP_PYTHON_EMBEDDED_INTERPRETER=0 it is mandatory to also set YUP_MODAL_LOOPS_PERMITTED=1
#endif

//==============================================================================

#include "scripting/yup_ScriptException.h"
#include "scripting/yup_ScriptEngine.h"
#include "scripting/yup_ScriptBindings.h"
#include "scripting/yup_ScriptUtilities.h"
#include "utilities/yup_ClassDemangling.h"
#include "utilities/yup_CrashHandling.h"
#include "utilities/yup_PythonInterop.h"

#include "bindings/yup_YupCore_bindings.h"
