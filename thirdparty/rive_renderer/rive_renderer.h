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

    ID:                 rive_renderer
    vendor:             rive
    version:            1.0
    name:               Rive Renderer.
    description:        The Rive Renderer is a vector and raster graphics renderer custom-built for Rive content, for animation, and for runtime.
    website:            https://github.com/rive-app/rive-runtime
    license:            MIT

    dependencies:       rive glad
    osxFrameworks:      Metal QuartzCore
    defines:            WITH_RIVE_TEXT=1
    linuxDefines:       RIVE_DESKTOP_GL=1
    wasmDefines:        RIVE_WEBGL=1
    searchpaths:        include source
    enableARC:          1

  END_JUCE_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

// TODO - Other deps: rive-decoders rive-dependencies
