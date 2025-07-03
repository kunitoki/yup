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

#include <yup_core/yup_core.h>

#include <functional>

namespace yup::Helpers {

// =================================================================================================

/**
 * @brief Demangle a C++ class name.
 *
 * This function takes a StringRef containing a mangled class name and returns the demangled class name as a String.
 *
 * @param className The mangled class name to demangle.
 *
 * @return A String containing the demangled class name.
 */
String demangleClassName (StringRef className);

// =================================================================================================

/**
 * @brief Demangle a C++ class name and pythonize it.
 *
 * This function takes a StringRef containing a mangled class name and returns the demangled and pythonized class name as a String.
 *
 * @param className The mangled class name to demangle and pythonize.
 *
 * @return A String containing the demangled and pythonized class name.
 */
String pythonizeClassName (StringRef className, int maxTemplateArgs = -1);

// =================================================================================================

/**
 * @brief Demangle a C++ class name and pythonize it by compunding to another class name.
 *
 * @param prefixName The prefix to apply to the class name.
 * @param className The mangled class name to demangle and pythonize.
 *
 * @return A String containing the demangled and pythonized class name with a prefix.
 */
String pythonizeCompoundClassName (StringRef prefixName, StringRef className, int maxTemplateArgs = -1);

// =================================================================================================

/**
 * @brief Demangle a C++ class name and pythonize it by making it part of a module.
 *
 * @param moduleName The name of the module to prepend.
 * @param className The mangled class name to demangle and pythonize.
 *
 * @return A String containing the demangled and pythonized class name belonging to a module.
 */
String pythonizeModuleClassName (StringRef moduleName, StringRef className, int maxTemplateArgs = -1);

} // namespace yup::Helpers
