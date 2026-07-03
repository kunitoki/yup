/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#undef IS_BIG_ENDIAN

#if defined(__BYTE_ORDER) && defined(__BIG_ENDIAN)
#if __BYTE_ORDER == __BIG_ENDIAN
#define IS_BIG_ENDIAN 1
#else
#define IS_BIG_ENDIAN 0
#endif
#elif defined(_BIG_ENDIAN) || defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__MIPSEB__)
#define IS_BIG_ENDIAN 1
#else
#define IS_BIG_ENDIAN 0
#endif

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#if defined(__x86_64__) || defined(_M_X64)
#define IS_INTEL_64 1
#else
#define IS_INTEL_64 0
#endif
#endif

#undef USE_DITHERING
#undef WEBP_HAVE_GIF
#undef WEBP_HAVE_GL
#undef WEBP_HAVE_JPEG
#undef WEBP_HAVE_NEON_RTCD
#undef WEBP_HAVE_PNG
#undef WEBP_HAVE_SDL
#undef WEBP_HAVE_TIFF
#undef WEBP_USE_THREAD
#undef HAVE_GLUT_GLUT_H
#undef HAVE_OPENGL_GLUT_H

#undef HAVE_CONFIG_H
#define HAVE_CONFIG_H 1

#undef WEBP_DISABLE_STATS
#define WEBP_DISABLE_STATS 1

#undef AC_APPLE_UNIVERSAL_BUILD
#if (__APPLE__) && __intel__
#define AC_APPLE_UNIVERSAL_BUILD 1
#endif

#undef HAVE_BUILTIN_BSWAP16
#undef HAVE_BUILTIN_BSWAP32
#undef HAVE_BUILTIN_BSWAP64
#if ! _MSC_VER
#define HAVE_BUILTIN_BSWAP16 1
#define HAVE_BUILTIN_BSWAP32 1
#define HAVE_BUILTIN_BSWAP64 1
#endif

#undef HAVE_CPU_FEATURES_H
#define HAVE_CPU_FEATURES_H 1

#undef HAVE_DLFCN_H
#define HAVE_DLFCN_H 1

#undef HAVE_GL_GLUT_H
#define HAVE_GL_GLUT_H 1

#undef HAVE_INTTYPES_H
#define HAVE_INTTYPES_H 1

#undef HAVE_MEMORY_H
#define HAVE_MEMORY_H 1

#undef HAVE_PTHREAD_PRIO_INHERIT
#define HAVE_PTHREAD_PRIO_INHERIT (! _MSC_VER)

#undef HAVE_SHLWAPI_H
#define HAVE_SHLWAPI_H 1

#undef HAVE_STDINT_H
#define HAVE_STDINT_H 1

#undef HAVE_STDLIB_H
#define HAVE_STDLIB_H 1

#undef HAVE_STRINGS_H
#define HAVE_STRINGS_H 1

#undef HAVE_STRING_H
#define HAVE_STRING_H 1

#undef HAVE_SYS_STAT_H
#undef HAVE_SYS_TYPES_H
#undef HAVE_UNISTD_H

#if ! _MSC_VER
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#endif

#undef HAVE_WINCODEC_H
#if _MSC_VER
#define HAVE_WINCODEC_H 1
#endif

#undef HAVE_WINDOWS_H
#if _MSC_VER
#define HAVE_WINDOWS_H 1
#endif

#undef STDC_HEADERS
#define STDC_HEADERS 1

#undef WEBP_HAVE_NEON
#if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#define WEBP_HAVE_NEON 1
#endif

#undef WEBP_HAVE_SSE2
#if IS_INTEL_64
#define WEBP_HAVE_SSE2 1
#endif

#undef WEBP_HAVE_SSE41
#if IS_INTEL_64 && _MSC_VER
// #define WEBP_HAVE_SSE41 1
#endif

#undef WEBP_NEAR_LOSSLESS
#define WEBP_NEAR_LOSSLESS 1

#undef WORDS_BIGENDIAN
#if IS_BIG_ENDIAN
#define WORDS_BIGENDIAN 1
#endif

#undef LT_OBJDIR
#define LT_OBJDIR ""

#undef PACKAGE
#define PACKAGE "WebP"
#undef PACKAGE_NAME
#define PACKAGE_NAME PACKAGE
#undef PACKAGE_TARNAME
#define PACKAGE_TARNAME PACKAGE

#undef PACKAGE_VERSION
#define PACKAGE_VERSION "1.4.0"
#undef VERSION
#define VERSION PACKAGE_VERSION

#undef PACKAGE_BUGREPORT
#define PACKAGE_BUGREPORT "https://bugs.chromium.org/p/webp"

#undef PACKAGE_STRING
#define PACKAGE_STRING (PACKAGE_NAME " " PACKAGE_VERSION)

#undef PACKAGE_URL
#define PACKAGE_URL "http://developers.google.com/speed/webp"
