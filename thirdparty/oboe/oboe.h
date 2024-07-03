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

    ID:               oboe
    vendor:           oboe
    version:          1.8.0
    name:             Android low level audio library
    description:      Oboe is an open-source C++ library designed to help build high-performance audio apps on Android.
    website:          https://developer.android.com/games/sdk/oboe
    license:          Apache-2.0

    dependencies:
    searchpaths:      upstream/include

  END_JUCE_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

#if __ANDROID__
#include "upstream/include/Oboe.h"
#endif
