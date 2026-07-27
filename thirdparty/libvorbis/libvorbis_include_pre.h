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

#if _MSC_VER
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