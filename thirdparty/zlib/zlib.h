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

/*
  ==============================================================================

  BEGIN_YUP_MODULE_DECLARATION

    ID:               zlib
    vendor:           zlib
    version:          1.2.3
    name:             Interface of the 'zlib' general purpose compression library
    description:      Interface of the 'zlib' general purpose compression library.
    website:          https://www.zlib.net/
    license:          Public Domain

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

#if defined (__APPLE__)
#include <TargetConditionals.h>
#endif

#if __cplusplus
extern "C" {
#endif

#define ZLIB_INTERNAL
#define NO_DUMMY_DECL
#include "src/zlib.h"

#if ! defined (Z_PREFIX)
typedef uInt z_uInt;
#endif

#if defined (_MSC_VER)
#define YUP_ZLIB_BEGIN_IGNORE_WARNINGS \
    __pragma (warning (push))          \
    __pragma (warning (disable: 4309)) \
    __pragma (warning (disable: 4305)) \
    __pragma (warning (disable: 4365)) \
    __pragma (warning (disable: 6385)) \
    __pragma (warning (disable: 6326)) \
    __pragma (warning (disable: 6340))
#define YUP_ZLIB_END_IGNORE_WARNINGS __pragma (warning (pop))
#elif defined (__clang__)
#define YUP_ZLIB_PRAGMA(x) _Pragma (#x)
#define YUP_ZLIB_BEGIN_IGNORE_WARNINGS                                      \
    YUP_ZLIB_PRAGMA (clang diagnostic push)                                 \
    YUP_ZLIB_PRAGMA (clang diagnostic ignored "-Wconversion")               \
    YUP_ZLIB_PRAGMA (clang diagnostic ignored "-Wsign-conversion")          \
    YUP_ZLIB_PRAGMA (clang diagnostic ignored "-Wshadow")                   \
    YUP_ZLIB_PRAGMA (clang diagnostic ignored "-Wdeprecated-register")      \
    YUP_ZLIB_PRAGMA (clang diagnostic ignored "-Wswitch-enum")              \
    YUP_ZLIB_PRAGMA (clang diagnostic ignored "-Wswitch-default")           \
    YUP_ZLIB_PRAGMA (clang diagnostic ignored "-Wredundant-decls")          \
    YUP_ZLIB_PRAGMA (clang diagnostic ignored "-Wimplicit-fallthrough")     \
    YUP_ZLIB_PRAGMA (clang diagnostic ignored "-Wzero-as-null-pointer-constant") \
    YUP_ZLIB_PRAGMA (clang diagnostic ignored "-Wcomma")
#define YUP_ZLIB_END_IGNORE_WARNINGS YUP_ZLIB_PRAGMA (clang diagnostic pop)
#else
#define YUP_ZLIB_BEGIN_IGNORE_WARNINGS
#define YUP_ZLIB_END_IGNORE_WARNINGS
#endif

#if __cplusplus
} // extern "C"
#endif
