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

#if YUP_RIVE_USE_D3D
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")
#pragma comment (lib, "dxgi.lib")

#include "source/d3d/pipeline_manager.cpp"
#include "source/d3d11/render_context_d3d_impl.cpp"
//#include "source/d3d12/d3d12_pipeline_manager.cpp"
//#include "source/d3d12/d3d12_utils.cpp"
//#include "source/d3d12/render_context_d3d12_impl.cpp"
#endif

#if YUP_RIVE_USE_OPENGL
#pragma comment (lib, "opengl32.lib")

#include "source/gl/gl_state.cpp"
#include "source/gl/gl_utils.cpp"
#include "source/gl/load_gles_extensions.cpp"
#include "source/gl/load_store_actions_ext.cpp"
#include "source/gl/pls_impl_ext_native.cpp"
#include "source/gl/pls_impl_rw_texture.cpp"
#include "source/gl/render_buffer_gl_impl.cpp"
#include "source/gl/render_context_gl_impl.cpp"
#include "source/gl/render_target_gl.cpp"
#endif
