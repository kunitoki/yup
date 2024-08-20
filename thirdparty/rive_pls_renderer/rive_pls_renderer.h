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

/*
  ==============================================================================

  BEGIN_JUCE_MODULE_DECLARATION

    ID:                 rive_pls_renderer
    vendor:             rive
    version:            1.0
    name:               Rive Renderer.
    description:        The Rive Renderer is a vector and raster graphics renderer custom-built for Rive content, for animation, and for runtime.
    website:            https://github.com/rive-app/rive-renderer
    license:            MIT
    minimumCppStandard: 17

    dependencies:       rive glad
    OSXFrameworks:      Metal QuartzCore
    defines:            WITH_RIVE_TEXT=1
    WASMDefines:        RIVE_WEBGL=1
    searchpaths:        include source
    enableARC:          1

  END_JUCE_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

#if __clang__
 #pragma clang diagnostic push
 #pragma clang diagnostic ignored "-Wattributes"
#endif

#include "include/rive/pls/pls_render_context.hpp"

#if __clang__
 #pragma clang diagnostic pop
#endif
