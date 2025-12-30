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

#if defined _WIN32 && !defined __CYGWIN__
 #include <io.h>
#else
 #include <unistd.h>
#endif

#if defined _MSC_VER || defined __BORLANDC__ || defined __MINGW32__
 #include <sys/types.h> /* for off_t */
#endif

#if HAVE_INTTYPES_H
 #define __STDC_FORMAT_MACROS
 #include <inttypes.h>
#endif

#if defined _MSC_VER || defined __MINGW32__ || defined __CYGWIN__ || defined __EMX__
 #include <io.h> /* for _setmode(), chmod() */
 #include <fcntl.h> /* for _O_BINARY */
#else
 #include <unistd.h> /* for chown(), unlink() */
#endif

#if defined _MSC_VER || defined __BORLANDC__ || defined __MINGW32__
 #if defined __BORLANDC__
  #include <utime.h> /* for utime() */
 #else
  #include <sys/utime.h> /* for utime() */
 #endif
#else
 #include <sys/types.h> /* some flavors of BSD (like OS X) require this to get time_t */
 #include <utime.h> /* for utime() */
#endif

#if defined _MSC_VER
 #if _MSC_VER >= 1600
  #include <stdint.h>
 #else
  #include <limits.h>
 #endif
#endif

#ifdef _WIN32
 #include <stdio.h>
 #include <sys/stat.h>
 #include <stdarg.h>
 #include <windows.h>
#endif

#if __APPLE__
 #include <TargetConditionals.h>
 #if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
 #elif TARGET_OS_IPHONE
 #else
  #define TARGET_OS_OSX 1
 #endif
#endif

#ifdef DEBUG
 #include <assert.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#undef PACKAGE_VERSION
#define PACKAGE_VERSION "1.5.0"

#define FLAC__NO_DLL 1
#define FLAC__HAS_OGG 0

#if !defined _MSC_VER
 #define HAVE_LROUND 1
#endif

#if TARGET_OS_OSX
 #define FLAC__SYS_DARWIN 1
#endif

#ifndef SIZE_MAX
 #define SIZE_MAX 0xffffffff
#endif

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable: 4267)
#pragma warning (disable: 4127)
#pragma warning (disable: 4244)
#pragma warning (disable: 4996)
#pragma warning (disable: 4100)
#pragma warning (disable: 4701)
#pragma warning (disable: 4702)
#pragma warning (disable: 4013)
#pragma warning (disable: 4133)
#pragma warning (disable: 4312)
#pragma warning (disable: 4505)
#pragma warning (disable: 4365)
#pragma warning (disable: 4005)
#pragma warning (disable: 4334)
#pragma warning (disable: 181)
#pragma warning (disable: 111)
#pragma warning (disable: 6340)
#pragma warning (disable: 6308)
#pragma warning (disable: 6297)
#pragma warning (disable: 6001)
#pragma warning (disable: 6320)
#else
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wdeprecated-register"
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#pragma clang diagnostic ignored "-Wredundant-decls"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wstatic-in-inline"
#pragma clang diagnostic ignored "-Wswitch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#if __X86_64__ || __amd64__ || __amd64 || _M_X64 || _M_AMD64
 #define FLAC__CPU_X86_64 1
 #define FLAC__HAS_X86INTRIN 1
#elif __i386__ || _M_IX86
 #define FLAC__CPU_IA32 1
 #define FLAC__HAS_X86INTRIN 1
#endif

#if __aarch64__
 #define FLAC__CPU_ARM64 1
 #if __ARM_NEON__
   #define FLAC__HAS_NEONINTRIN 1
   #define FLAC__HAS_A64NEONINTRIN 1
 #endif
#endif

#define flac_max(a, b) ((a) > (b) ? (a) : (b))
#define flac_min(a, b) ((a) < (b) ? (a) : (b))

#pragma push_macro ("DEBUG")
#pragma push_macro ("NDEBUG")
#undef  DEBUG  // (some flac code dumps debug trace if the app defines this macro)

#ifndef NDEBUG
 #define NDEBUG // (some flac code prints cpu info if this isn't defined)
#endif

#include "flac_library.h"
