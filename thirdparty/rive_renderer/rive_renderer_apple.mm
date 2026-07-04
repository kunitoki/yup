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

#define YUP_RIVE_RENDERER_NO_INCLUDES 1
#include "rive_renderer.h"
#undef YUP_RIVE_RENDERER_NO_INCLUDES

#if YUP_RIVE_USE_METAL
#include <TargetConditionals.h>

#include "source/metal/render_context_metal_impl.mm"
#include "source/metal/background_shader_compiler.mm"

#if TARGET_OS_SIMULATOR
#include "source/generated/shaders/rive_pls_ios_simulator.metallib.c"
#elif TARGET_OS_IOS
#include "source/generated/shaders/rive_pls_ios.metallib.c"
#elif TARGET_OS_MAC
#include "source/generated/shaders/rive_pls_macosx.metallib.c"
#endif

#include "source/ore/metal/ore_bind_group_metal_gl.mm"
#include "source/ore/metal/ore_bind_group_metal.mm"
#include "source/ore/metal/ore_buffer_metal_gl.mm"
#include "source/ore/metal/ore_buffer_metal.mm"
#include "source/ore/metal/ore_context_metal.mm"
#include "source/ore/metal/ore_pipeline_metal_gl.mm"
#include "source/ore/metal/ore_pipeline_metal.mm"
#include "source/ore/metal/ore_render_pass_metal_gl.mm"
#define orePrimitiveTopologyToMTL orePrimitiveTopologyToMTL_render_pass
#define oreIndexFormatToMTL oreIndexFormatToMTL_render_pass
#define oreCullModeToMTL oreCullModeToMTL_render_pass
#define oreWindingToMTL oreWindingToMTL_render_pass
#define kMetalVertexBufferBase kMetalVertexBufferBase_render_pass
#include "source/ore/metal/ore_render_pass_metal.mm"
#undef orePrimitiveTopologyToMTL
#undef oreIndexFormatToMTL
#undef oreCullModeToMTL
#undef oreWindingToMTL
#undef kMetalVertexBufferBase
#include "source/ore/metal/ore_sampler_metal_gl.mm"
#include "source/ore/metal/ore_sampler_metal.mm"
#include "source/ore/metal/ore_shader_module_metal_gl.mm"
#include "source/ore/metal/ore_shader_module_metal.mm"
#include "source/ore/metal/ore_texture_metal_gl.mm"
#include "source/ore/metal/ore_texture_metal.mm"
#endif
