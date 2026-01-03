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

#include "pffft_library.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-W#pragma-messages"
#endif

#if _MSC_VER && defined(_USE_MATH_DEFINES)
#undef _USE_MATH_DEFINES
#endif

#include <pffft_library/pffft_double.c>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
