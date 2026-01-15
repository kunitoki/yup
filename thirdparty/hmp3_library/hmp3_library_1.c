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

#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcomment"
#pragma clang diagnostic ignored "-Wpointer-to-int-cast"
#pragma clang diagnostic ignored "-Wint-to-pointer-cast"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wincompatible-pointer-types"
#endif

#define mdct_init hmp3_mdct_init
#include <hmp3_library/hmp3/src/cnt.c>
#include <hmp3_library/hmp3/src/emap.c>
#include <hmp3_library/hmp3/src/filter2.c>
#include <hmp3_library/hmp3/src/hwin.c>
#include <hmp3_library/hmp3/src/l3init.c>
#include <hmp3_library/hmp3/src/l3pack.c>
#include <hmp3_library/hmp3/src/pcmhpm.c>
#include <hmp3_library/hmp3/src/sbt.c>
#include <hmp3_library/hmp3/src/spdsmr.c>
#include <hmp3_library/hmp3/src/xhwin.c>
#include <hmp3_library/hmp3/src/xsbt.c>
#include <hmp3_library/hmp3/src/detect.c>
#undef mdct_init

#if __clang__
#pragma clang diagnostic pop
#endif
