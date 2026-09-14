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
 #pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif

#include <TargetConditionals.h>

#if !defined(_RIVE_INTERNAL_)
#define _RIVE_INTERNAL_ 1
#endif

#define YUP_RIVE_NO_INCLUDES 1
#include "rive.h"
#undef YUP_RIVE_NO_INCLUDES

#include "source/text/font_hb_apple.mm"

#if YUP_RIVE_USE_METAL
#include "source/renderer/metal/render_context_metal_impl.mm"
#include "source/renderer/metal/background_shader_compiler.mm"

#if TARGET_OS_SIMULATOR
#include "source/renderer/generated/shaders/rive_pls_ios_simulator.metallib.c"
#elif TARGET_OS_IOS
#include "source/renderer/generated/shaders/rive_pls_ios.metallib.c"
#elif TARGET_OS_MAC
#include "source/renderer/generated/shaders/rive_pls_macosx.metallib.c"
#endif

#include "source/renderer/ore/metal/ore_bind_group_metal.mm"
#include "source/renderer/ore/metal/ore_context_metal.mm"
#include "source/renderer/ore/metal/ore_texture_metal.mm"
#include "source/renderer/ore/metal/ore_shader_module_metal.mm"
#include "source/renderer/ore/metal/ore_sampler_metal.mm"
#define kMetalVertexBufferBase kMetalVertexBufferBase_render_pass
#include "source/renderer/ore/metal/ore_render_pass_metal.mm"
#undef kMetalVertexBufferBase
#include "source/renderer/ore/metal/ore_pipeline_metal.mm"
#include "source/renderer/ore/metal/ore_buffer_metal.mm"
#include "source/renderer/ore/metal/ore_bind_group_metal.mm"
#endif

#if __clang__
 #pragma clang diagnostic pop
#endif
