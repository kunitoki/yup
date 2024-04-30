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

#ifdef YUP_GRAPHICS_H_INCLUDED
 /* When you add this cpp file to your project, you mustn't include it in a file where you've
    already included any other headers - just put it inside a file on its own, possibly with your config
    flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
    header files that the compiler may be using.
 */
 #error "Incorrect use of YUP cpp file"
#endif

#include "yup_graphics.h"

//==============================================================================

#if JUCE_WINDOWS
 #include "native/yup_LowLevelRenderContext_d3d.cpp"

#elif JUCE_MAC || JUCE_IOS
 #include "native/yup_LowLevelRenderContext_metal.cpp"

#elif JUCE_LINUX || JUCE_WASM || JUCE_ANDROID
 #include "native/yup_LowLevelRenderContext_gl.cpp"

#endif

#include "native/yup_LowLevelRenderContext_dawn.cpp"
#include "native/yup_LowLevelRenderContext_dawn_helper.cpp"
