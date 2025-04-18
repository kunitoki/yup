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

#include "rive_renderer.h"

#if __clang__
 #pragma clang diagnostic push
 #pragma clang diagnostic ignored "-Wshorten-64-to-32"
 #pragma clang diagnostic ignored "-Wattributes"
#elif __GNUC__
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wattributes"
#elif _MSC_VER
 __pragma (warning (push))
 __pragma (warning (disable: 4244))
#endif

#include "source/rive_renderer.cpp"
#include "source/render_context.cpp"
#include "source/rive_render_paint.cpp"
#include "source/rive_render_path.cpp"
#include "source/rive_render_image.cpp"
#include "source/intersection_board.cpp"
#include "source/draw.cpp"
#include "source/gr_triangulator.cpp"
#include "source/gradient.cpp"
#include "source/sk_rectanizer_skyline.cpp"
#include "source/gpu.cpp"
#include "source/rive_render_factory.cpp"
#include "source/render_context_helper_impl.cpp"

#if __clang__
 #pragma clang diagnostic pop
#elif __GNUC__
 #pragma GCC diagnostic pop
#elif _MSC_VER
 __pragma (warning (pop))
#endif
