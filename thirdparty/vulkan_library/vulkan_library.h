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

/*
  ==============================================================================

  BEGIN_YUP_MODULE_DECLARATION

    ID:               vulkan_library
    vendor:           khronos
    version:          1.4.321
    name:             Vulkan headers and Vulkan Memory Allocator
    description:      Khronos Vulkan-Headers (C API) and AMD Vulkan Memory Allocator, header-only.
    website:          https://github.com/KhronosGroup/Vulkan-Headers
    license:          Apache-2.0 OR MIT

    defines:          VK_NO_PROTOTYPES=1 VMA_STATIC_VULKAN_FUNCTIONS=0 VMA_DYNAMIC_VULKAN_FUNCTIONS=1
    searchpaths:      include

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

// The loader is never linked: every entry point is resolved at runtime from the
// vkGetInstanceProcAddr handed out by the windowing layer, so a device without a
// Vulkan driver falls back to another backend instead of failing to load.
#include <vulkan/vulkan.h>
