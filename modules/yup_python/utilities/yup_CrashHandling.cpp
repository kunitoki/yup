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

#include "yup_CrashHandling.h"

#if defined(_LIBCPP_VERSION) || defined(__GLIBCXX__) || defined(__GLIBCPP__)
#include <cxxabi.h>
#include <execinfo.h>
#include <dlfcn.h>
#else
#include "yup_WindowsIncludes.h"
#endif

namespace yup::Helpers
{

//==============================================================================

String getStackBacktrace()
{
    String result;

#if YUP_WINDOWS
    HANDLE process = GetCurrentProcess();
    SymInitialize (process, nullptr, TRUE);

    void* stack[128];
    int frames = static_cast<int> (CaptureStackBackTrace (0, numElementsInArray (stack), stack, nullptr));

    HeapBlock<SYMBOL_INFO> symbol;
    symbol.calloc (sizeof (SYMBOL_INFO) + 256, 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof (SYMBOL_INFO);

    for (int i = 0; i < frames; ++i)
    {
        DWORD64 displacement = 0;

        if (SymFromAddr (process, reinterpret_cast<DWORD64> (stack[i]), &displacement, symbol))
        {
            result << i << ": ";

            IMAGEHLP_MODULE64 moduleInfo;
            zerostruct (moduleInfo);
            moduleInfo.SizeOfStruct = sizeof (moduleInfo);

            if (::SymGetModuleInfo64 (process, symbol->ModBase, &moduleInfo))
                result << moduleInfo.ModuleName << ": ";

            result << symbol->Name << " + 0x" << String::toHexString (static_cast<int64> (displacement)) << newLine;
        }
    }

#else
    void* stack[128];
    auto frames = backtrace (stack, numElementsInArray (stack));
    char** frameStrings = backtrace_symbols (stack, frames);

    for (int i = 0; i < frames; ++i)
    {
        Dl_info info;
        if (dladdr (stack[i], &info))
        {
            int status = 0;

            std::unique_ptr<char, decltype (::free)*> demangled (abi::__cxa_demangle (info.dli_sname, nullptr, nullptr, &status), ::free);
            if (status == 0)
            {
                result
                    << String (i).paddedRight (' ', 3)
                    << " " << File (String (info.dli_fname)).getFileName().paddedRight (' ', 35)
                    << " 0x" << String::toHexString (reinterpret_cast<size_t> (stack[i])).paddedLeft ('0', sizeof (void*) * 2)
                    << " " << demangled.get()
                    << " + " << (reinterpret_cast<char*> (stack[i]) - reinterpret_cast<char*> (info.dli_saddr)) << newLine;

                continue;
            }
        }

        result << frameStrings[i] << newLine;
    }

    ::free (frameStrings);
#endif

    return result;
}

//==============================================================================

void applicationCrashHandler ([[maybe_unused]] void* stackFrame)
{
    Logger::getCurrentLogger()->outputDebugString (getStackBacktrace());
}

} // namespace yup::Helpers
