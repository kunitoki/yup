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
/*  This file figures out which platform is being built, and defines some macros
    that the rest of the code can use for OS-specific compilation.

    Macros that will be set here are:

    - One of YUP_WINDOWS, YUP_MAC YUP_LINUX, YUP_IOS, YUP_ANDROID, etc.
    - Either YUP_32BIT or YUP_64BIT, depending on the architecture.
    - Either YUP_LITTLE_ENDIAN or YUP_BIG_ENDIAN.
    - Either YUP_INTEL or YUP_ARM
    - Either YUP_GCC or YUP_CLANG or YUP_MSVC
*/

//==============================================================================
#ifdef YUP_APP_CONFIG_HEADER
#include YUP_APP_CONFIG_HEADER
#elif ! defined(YUP_GLOBAL_MODULE_SETTINGS_INCLUDED)
/*
    Most projects will contain a global header file containing various settings that
    should be applied to all the code in your project. You may want to set the
    YUP_APP_CONFIG_HEADER macro with the name of a file to include, or just include
    one before all the module cpp files, in which you set YUP_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    to silence this error. (Or if you don't need a global header, then you can just define
    YUP_GLOBAL_MODULE_SETTINGS_INCLUDED globally to avoid this error).
 */
#error "No global header file was included!"
#endif

//==============================================================================
#if defined(_WIN32) || defined(_WIN64)
#define YUP_WINDOWS 1

#elif defined(ANDROID) || defined(__ANDROID__)
#define YUP_ANDROID 1

#elif defined(__FreeBSD__) || defined(__OpenBSD__)
#define YUP_BSD 1

#elif defined(LINUX) || defined(__linux__)
#define YUP_LINUX 1

#elif defined(__APPLE_CPP__) || defined(__APPLE_CC__)
#define CF_EXCLUDE_CSTD_HEADERS 1
#include <TargetConditionals.h> // (needed to find out what platform we're using)
#include <AvailabilityMacros.h>
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#define YUP_IPHONE 1
#define YUP_IOS 1
#if TARGET_IPHONE_SIMULATOR
#define YUP_IOS_SIMULATOR 1
#endif
#else
#define YUP_MAC 1
#endif

#elif defined(__wasm__) && defined(__EMSCRIPTEN__)
#define YUP_WASM 1
#define YUP_EMSCRIPTEN 1

#elif defined(__wasm__)
#define YUP_WASM 1

#else
#error "Unknown platform!"

#endif

//==============================================================================
#if YUP_WINDOWS
#ifdef _MSC_VER
#ifdef _WIN64
#define YUP_64BIT 1
#else
#define YUP_32BIT 1
#endif
#endif

#ifdef _DEBUG
#define YUP_DEBUG 1
#endif

#ifdef __MINGW32__
#define YUP_MINGW 1
#ifdef __MINGW64__
#define YUP_64BIT 1
#else
#define YUP_32BIT 1
#endif
#endif

/** If defined, this indicates that the processor is little-endian. */
#define YUP_LITTLE_ENDIAN 1

#if defined(_M_ARM) || defined(_M_ARM64) || defined(_M_ARM64EC) || defined(__arm__) || defined(__aarch64__)
#define YUP_ARM 1
#else
#define YUP_INTEL 1
#endif
#endif

//==============================================================================
#if YUP_MAC || YUP_IOS

// Expands to true if the API of the specified version is available at build time, false otherwise
#define YUP_MAC_API_VERSION_CAN_BE_BUILT(major, minor) \
    ((major) * 10000 + (minor) * 100 <= MAC_OS_X_VERSION_MAX_ALLOWED)

// Expands to true if the API of the specified version is available at build time, false otherwise
#define YUP_IOS_API_VERSION_CAN_BE_BUILT(major, minor) \
    ((major) * 10000 + (minor) * 100 <= __IPHONE_OS_VERSION_MAX_ALLOWED)

// Expands to true if the deployment target is greater or equal to the specified version, false otherwise
#define YUP_MAC_API_VERSION_MIN_REQUIRED_AT_LEAST(major, minor) \
    ((major) * 10000 + (minor) * 100 <= MAC_OS_X_VERSION_MIN_REQUIRED)

// Expands to true if the deployment target is greater or equal to the specified version, false otherwise
#define YUP_IOS_API_VERSION_MIN_REQUIRED_AT_LEAST(major, minor) \
    ((major) * 10000 + (minor) * 100 <= __IPHONE_OS_VERSION_MIN_REQUIRED)

#if defined(DEBUG) || defined(_DEBUG) || ! (defined(NDEBUG) || defined(_NDEBUG))
#define YUP_DEBUG 1
#endif

#if ! (defined(DEBUG) || defined(_DEBUG) || defined(NDEBUG) || defined(_NDEBUG))
#warning "Neither NDEBUG or DEBUG has been defined - you should set one of these to make it clear whether this is a release build,"
#endif

#ifdef __LITTLE_ENDIAN__
#define YUP_LITTLE_ENDIAN 1
#else
#define YUP_BIG_ENDIAN 1
#endif

#ifdef __LP64__
#define YUP_64BIT 1
#else
#define YUP_32BIT 1
#endif

#if defined(__ppc__) || defined(__ppc64__)
#error "PowerPC is no longer supported by YUP!"
#elif defined(__arm__) || defined(__arm64__)
#define YUP_ARM 1
#else
#define YUP_INTEL 1
#endif

#if YUP_MAC
#if ! YUP_MAC_API_VERSION_CAN_BE_BUILT(11, 1)
#error "The macOS 11.1 SDK (Xcode 12.4+) is required to build YUP apps. You can create apps that run on macOS 10.11+ by changing the deployment target."
#elif ! YUP_MAC_API_VERSION_MIN_REQUIRED_AT_LEAST(10, 11)
#error "Building for OSX 10.10 and earlier is no longer supported!"
#endif
#endif
#endif

//==============================================================================
#if YUP_LINUX || YUP_ANDROID || YUP_BSD

#ifdef _DEBUG
#define YUP_DEBUG 1
#endif

// Allow override for big-endian Linux platforms
#if defined(__LITTLE_ENDIAN__) || ! defined(YUP_BIG_ENDIAN)
#define YUP_LITTLE_ENDIAN 1
#undef YUP_BIG_ENDIAN
#else
#undef YUP_LITTLE_ENDIAN
#define YUP_BIG_ENDIAN 1
#endif

#if defined(__LP64__) || defined(_LP64) || defined(__arm64__) || defined(__aarch64__)
#define YUP_64BIT 1
#else
#define YUP_32BIT 1
#endif

#if defined(__arm__) || defined(__arm64__) || defined(__aarch64__)
#define YUP_ARM 1
#elif __MMX__ || __SSE__ || __amd64__
#define YUP_INTEL 1
#endif
#endif

//==============================================================================
#if YUP_WASM
#ifdef _DEBUG
#define YUP_DEBUG 1
#endif

#define YUP_LITTLE_ENDIAN 1
#define YUP_32BIT 1
#endif

//==============================================================================
// Compiler type macros.

#if defined(__clang__)
#define YUP_CLANG 1

#elif defined(__GNUC__)
#define YUP_GCC 1

#elif defined(_MSC_VER)
#define YUP_MSVC 1

#else
#error unknown compiler
#endif
