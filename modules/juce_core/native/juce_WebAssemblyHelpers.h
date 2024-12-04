/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

namespace juce {
namespace {

std::chrono::steady_clock::time_point juce_getTimeSinceStartupFallback() noexcept
{
    static const auto timeSinceStartup = std::chrono::steady_clock::now();
    return timeSinceStartup;
}

bool juce_isRunningUnderBrowser()
{
#if JUCE_EMSCRIPTEN
    static bool hasBrowserWindowObject = []
    {
        return EM_ASM_INT({
            return typeof window !== "undefined" ? 1 : 0;
        });
    }();

    return hasBrowserWindowObject;
#else
    return false;
#endif
}

} // namespace
} // namespace juce