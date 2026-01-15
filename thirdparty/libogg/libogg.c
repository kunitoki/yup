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

#include "libogg.h"

#if _MSC_VER
#pragma warning (push)
#pragma warning (disable: 6308)
#else
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif

#include <libogg/src/bitwise.c>
#include <libogg/src/framing.c>

#if _MSC_VER
#pragma warning (pop)
#else
#pragma clang diagnostic pop
#endif
