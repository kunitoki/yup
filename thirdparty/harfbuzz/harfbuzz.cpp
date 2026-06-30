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

#if __clang__
 #pragma clang diagnostic push
 #pragma clang diagnostic ignored "-Wempty-body"
 #pragma clang diagnostic ignored "-Wunused-function"
 #pragma clang diagnostic ignored "-Wunused-member-function"
 #pragma clang diagnostic ignored "-Wdeprecated-declarations"
 #pragma clang diagnostic ignored "-Wformat"
#elif __GNUC__
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wempty-body"
 #pragma GCC diagnostic ignored "-Wunused-function"
 #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
 #pragma GCC diagnostic ignored "-Wformat"
#elif _MSC_VER
 #pragma warning(push)
 #pragma warning(disable : 4244)
 #pragma warning(disable : 4146)
#endif

#include "harfbuzz.h"

#if !defined(HB_NO_PRAGMA_GCC_DIAGNOSTIC_ERROR)
 #define YUP_HARFBUZZ_DEFINED_HB_NO_PRAGMA_GCC_DIAGNOSTIC_ERROR 1
 #define HB_NO_PRAGMA_GCC_DIAGNOSTIC_ERROR 1
#endif

#if !defined(HB_NO_PRAGMA_GCC_DIAGNOSTIC_WARNING)
 #define YUP_HARFBUZZ_DEFINED_HB_NO_PRAGMA_GCC_DIAGNOSTIC_WARNING 1
 #define HB_NO_PRAGMA_GCC_DIAGNOSTIC_WARNING 1
#endif

#include "upstream/harfbuzz.cc"

#if defined(YUP_HARFBUZZ_DEFINED_HB_NO_PRAGMA_GCC_DIAGNOSTIC_WARNING)
 #undef HB_NO_PRAGMA_GCC_DIAGNOSTIC_WARNING
 #undef YUP_HARFBUZZ_DEFINED_HB_NO_PRAGMA_GCC_DIAGNOSTIC_WARNING
#endif

#if defined(YUP_HARFBUZZ_DEFINED_HB_NO_PRAGMA_GCC_DIAGNOSTIC_ERROR)
 #undef HB_NO_PRAGMA_GCC_DIAGNOSTIC_ERROR
 #undef YUP_HARFBUZZ_DEFINED_HB_NO_PRAGMA_GCC_DIAGNOSTIC_ERROR
#endif

#if __clang__
 #pragma clang diagnostic pop
#elif __GNUC__
 #pragma GCC diagnostic pop
#elif _MSC_VER 
 #pragma warning(pop)
#endif
