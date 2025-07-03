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

#ifdef YUP_PYTHON_H_INCLUDED
/* When you add this cpp file to your project, you mustn't include it in a file where you've
   already included any other headers - just put it inside a file on its own, possibly with your config
   flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
   header files that the compiler may be using.
*/
#error "Incorrect use of YUP cpp file"
#endif

#include "yup_python.h"

//==============================================================================

#include "scripting/yup_ScriptEngine.cpp"
#include "scripting/yup_ScriptBindings.cpp"
#include "scripting/yup_ScriptUtilities.cpp"

//==============================================================================

//#if YUP_MODULE_AVAILABLE_yup_core
#include "bindings/yup_YupCore_bindings.cpp"
//#endif

//==============================================================================

#include "modules/yup_YupMain_module.cpp"
#include "modules/yup_YupInternal_module.cpp"

//==============================================================================

#include "utilities/yup_CrashHandling.cpp"
#include "utilities/yup_ClassDemangling.cpp"
