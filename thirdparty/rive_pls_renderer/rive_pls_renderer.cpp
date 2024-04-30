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

#include "rive_pls_renderer.h"

#if __clang__
 #pragma clang diagnostic push
 #pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif

#include "source/path_utils.cpp"
#include "source/pls_paint.cpp"
#include "source/pls_render_context_helper_impl.cpp"
#include "source/pls.cpp"
#include "source/pls_image.cpp"
#include "source/pls_render_context.cpp"
#include "source/pls_draw.cpp"
#include "source/pls_factory.cpp"
#include "source/pls_path.cpp"
#include "source/pls_renderer.cpp"
#include "source/intersection_board.cpp"
#include "source/gr_triangulator.cpp"

#if __clang__
 #pragma clang diagnostic pop
#endif
