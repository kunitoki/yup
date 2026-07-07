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
#endif

#include "source/gl/gl_state.cpp"
#include "source/gl/gl_utils.cpp"
#include "source/gl/load_store_actions_ext.cpp"
#include "source/gl/pls_impl_ext_native.cpp"
#include "source/gl/pls_impl_rw_texture.cpp"
#include "source/gl/pls_impl_webgl.cpp"
#include "source/gl/render_buffer_gl_impl.cpp"
#include "source/gl/render_context_gl_impl.cpp"
#include "source/gl/render_target_gl.cpp"

#include "source/ore/gl/ore_bind_group_gl.cpp"
#include "source/ore/gl/ore_buffer_gl.cpp"
#include "source/ore/gl/ore_context_gl.cpp"
#include "source/ore/gl/ore_pipeline_gl.cpp"
#define oreCompareFunctionToGL oreCompareFunctionToGL_rp
#include "source/ore/gl/ore_render_pass_gl.cpp"
#undef oreCompareFunctionToGL
#include "source/ore/gl/ore_sampler_gl.cpp"
#include "source/ore/gl/ore_shader_module_gl.cpp"
#define oreFormatToGLInternal oreFormatToGLInternal_tex
#include "source/ore/gl/ore_texture_gl.cpp"
#undef oreFormatToGLInternal

#if __clang__
 #pragma clang diagnostic pop
#elif __GNUC__
 #pragma GCC diagnostic pop
#endif
