/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#include "rive.h"

#if YUP_RIVE_USE_VULKAN

#if __clang__
 #pragma clang diagnostic push
 #pragma clang diagnostic ignored "-Wshorten-64-to-32"
 #pragma clang diagnostic ignored "-Wattributes"
 #pragma clang diagnostic ignored "-Wnullability-completeness"
 #pragma clang diagnostic ignored "-Wunused-parameter"
 #pragma clang diagnostic ignored "-Wunused-variable"
#elif __GNUC__
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wattributes"
 #pragma GCC diagnostic ignored "-Wunused-parameter"
 #pragma GCC diagnostic ignored "-Wunused-variable"
#elif _MSC_VER
 #pragma warning (push)
 #pragma warning (disable : 4244 4189 4100 4127 4324)
#endif

#include "source/renderer/vulkan/vulkan_memory_allocator.cpp"
#include "source/renderer/vulkan/vkutil.cpp"
#include "source/renderer/vulkan/vulkan_context.cpp"
#include "source/renderer/vulkan/vulkan_shaders.cpp"
#include "source/renderer/vulkan/draw_shader_vulkan.cpp"
#include "source/renderer/vulkan/draw_pipeline_layout_vulkan.cpp"
#include "source/renderer/vulkan/draw_pipeline_vulkan.cpp"
#include "source/renderer/vulkan/pipeline_manager_vulkan.cpp"
#include "source/renderer/vulkan/render_pass_vulkan.cpp"
#include "source/renderer/vulkan/render_target_vulkan.cpp"
#include "source/renderer/vulkan/render_context_vulkan_impl.cpp"

#include "source/renderer/ore/vulkan/ore_bind_group_vulkan.cpp"
#include "source/renderer/ore/vulkan/ore_buffer_vulkan.cpp"
#include "source/renderer/ore/vulkan/ore_context_vulkan.cpp"
#define hasStencilLocal hasStencilLocal_pipeline
#include "source/renderer/ore/vulkan/ore_pipeline_vulkan.cpp"
#undef hasStencilLocal
#include "source/renderer/ore/vulkan/ore_render_pass_vulkan.cpp"
#include "source/renderer/ore/vulkan/ore_sampler_vulkan.cpp"
#include "source/renderer/ore/vulkan/ore_shader_module_vulkan.cpp"
#include "source/renderer/ore/vulkan/ore_texture_vulkan.cpp"

#if __clang__
 #pragma clang diagnostic pop
#elif __GNUC__
 #pragma GCC diagnostic pop
#elif _MSC_VER
 #pragma warning (pop)
#endif

#endif // YUP_RIVE_USE_VULKAN
