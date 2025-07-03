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

#include "yup_ClassDemangling.h"

#include <cstddef>
#include <ciso646>

#if YUP_WINDOWS
#include "yup_WindowsIncludes.h"

#pragma comment (lib, "dbghelp.lib")

using malloc_func_t = void* (*)(size_t);
using free_func_t = void (*)(void*);

extern "C" char* __unDName (char*, const char*, int, malloc_func_t, free_func_t, unsigned short int);
#endif

#if defined(_LIBCPP_VERSION) || defined(__GLIBCXX__) || defined(__GLIBCPP__)
#include <cxxabi.h>
#endif

namespace yup::Helpers {

// =================================================================================================

String demangleClassName (StringRef className)
{
    String name = className;

#if YUP_WINDOWS
    if (name.startsWith("class "))
    {
        char demangledName[1024] = { 0 };
        auto offset = name.startsWithChar (L'?') ? 1 : 0;
        __unDName(demangledName, name.toRawUTF8() + offset, numElementsInArray(demangledName), malloc, free, 0x2800);
        name = String::fromUTF8(demangledName).replace("class ", "");
    }

#else
    int status = -1;
    char* demangledName = abi::__cxa_demangle (name.toUTF8(), nullptr, nullptr, &status);
    name = String::fromUTF8 (demangledName);
    std::free (demangledName);

#endif

    return name;
}

// =================================================================================================

String pythonizeClassName (StringRef className, int maxTemplateArgs)
{
    String name = demangleClassName (className);

    if (maxTemplateArgs > 0 && name.contains("<"))
    {
        String tempName;

        auto foundComma = name.indexOf (0, ",");
        if (foundComma >= 0)
        {
            while (foundComma >= 0 && --maxTemplateArgs >= 0)
                foundComma = name.indexOf (foundComma, ",");

            tempName << name.substring (0, foundComma);
            tempName << name.fromLastOccurrenceOf (">", true, false);
        }

        name = tempName;
    }

    return name
        .replace ("yup::", "")
        .replace ("", "")
        .replace ("::", ".")
        .replace ("<", "[")
        .replace (">", "]");
}

// =================================================================================================

String pythonizeCompoundClassName (StringRef prefixName, StringRef className, int maxTemplateArgs)
{
    const auto pythonizedName = pythonizeClassName (className, maxTemplateArgs);

    String result;

    result
        << prefixName
        << pythonizedName.substring(0, 1).toUpperCase()
        << pythonizedName.substring(1);

    return result;
}

// =================================================================================================

String pythonizeModuleClassName (StringRef moduleName, StringRef className, int maxTemplateArgs)
{
    const auto pythonizedName = pythonizeClassName (className, maxTemplateArgs);

    String result;

    result << moduleName << "." << pythonizedName;

    return result;
}

} // namespace yup::Helpers
