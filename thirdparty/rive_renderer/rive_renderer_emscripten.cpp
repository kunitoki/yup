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

#if RIVE_WEBGPU
#include "source/webgpu/em_js_handle.cpp"
#include "source/webgpu/render_context_webgpu_impl.cpp"
//#include "source/webgpu/render_context_webgpu_vulkan.cpp"

#elif RIVE_WEBGL
#include "source/gl/gl_state.cpp"
#include "source/gl/gl_utils.cpp"
#include "source/gl/load_gles_extensions.cpp"
#include "source/gl/load_store_actions_ext.cpp"
#include "source/gl/pls_impl_webgl.cpp"
#include "source/gl/render_buffer_gl_impl.cpp"
#include "source/gl/render_context_gl_impl.cpp"
#include "source/gl/render_target_gl.cpp"

#else
#error "No renderer backend defined"

#endif
