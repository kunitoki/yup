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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once

//==============================================================================
/** Current YUP version number.

    See also SystemStats::getYUPVersion() for a string version.
*/
#define YUP_MAJOR_VERSION 1
#define YUP_MINOR_VERSION 0
#define YUP_BUILDNUMBER 0

/** Current YUP version number.

    Bits 16 to 32 = major version.
    Bits 8 to 16 = minor version.
    Bits 0 to 8 = point release.

    See also SystemStats::getYUPVersion() for a string version.
*/
#define YUP_VERSION ((YUP_MAJOR_VERSION << 16) + (YUP_MINOR_VERSION << 8) + YUP_BUILDNUMBER)

#if ! DOXYGEN
#define YUP_VERSION_ID \
    [[maybe_unused]] volatile auto yupVersionId = "yup_version_" YUP_STRINGIFY (YUP_MAJOR_VERSION) "_" YUP_STRINGIFY (YUP_MINOR_VERSION) "_" YUP_STRINGIFY (YUP_BUILDNUMBER);
#endif

//==============================================================================
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <queue>
#include <set>
#include <sstream>
#include <string_view>
#include <thread>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

//==============================================================================
#include "yup_CompilerSupport.h"
#include "yup_CompilerWarnings.h"
#include "yup_PlatformDefs.h"

//==============================================================================
// Now we'll include some common OS headers..
YUP_BEGIN_IGNORE_WARNINGS_MSVC (4514 4245 4100)

#if YUP_MSVC
#include <intrin.h>
#endif

#if YUP_MAC || YUP_IOS
#include <libkern/OSAtomic.h>
#include <libkern/OSByteOrder.h>
#include <xlocale.h>
#include <signal.h>
#endif

#if YUP_LINUX || YUP_BSD || YUP_WASM
#include <cstring>
#include <signal.h>
#endif

#if YUP_MSVC && YUP_DEBUG
#include <crtdbg.h>
#endif

YUP_END_IGNORE_WARNINGS_MSVC

#if YUP_ANDROID
#include <cstring>
#include <byteswap.h>
#endif

// undef symbols that are sometimes set by misguided 3rd-party headers..
#undef TYPE_BOOL
#undef max
#undef min
#undef major
#undef minor
#undef KeyPress

//==============================================================================
// DLL building settings on Windows
#if YUP_MSVC
#ifdef YUP_DLL_BUILD
#define YUP_API __declspec (dllexport)
#pragma warning(disable : 4251)
#elif defined(YUP_DLL)
#define YUP_API __declspec (dllimport)
#pragma warning(disable : 4251)
#endif
#elif defined(YUP_DLL) || defined(YUP_DLL_BUILD)
#define YUP_API __attribute__ ((visibility ("default")))
#endif

//==============================================================================
#ifndef YUP_API
#define YUP_API /**< This macro is added to all YUP public class declarations. */
#endif

#if YUP_MSVC && YUP_DLL_BUILD
#define YUP_PUBLIC_IN_DLL_BUILD(declaration) \
public:                                      \
    declaration;                             \
                                             \
private:
#else
#define YUP_PUBLIC_IN_DLL_BUILD(declaration) declaration;
#endif

/** This macro is added to all YUP public function declarations. */
#define YUP_PUBLIC_FUNCTION YUP_API YUP_CALLTYPE

#ifndef DOXYGEN
#define YUP_NAMESPACE yup // This old macro is deprecated: you should just use the yup namespace directly.
#endif
