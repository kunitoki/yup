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

    ID:               bungee
    vendor:           dr_libs
    version:          2.4.8
    name:             Bungee is a modern, open-source C++ library for high-quality audio time-stretching and pitch-shifting in real-time or offline
    description:      Bungee is a modern, open-source C++ library for high-quality audio time-stretching and pitch-shifting in real-time or offline.
    website:          https://github.com/mackron/dr_libs
    license:          MPL2.0

    dependencies:     pffft_library eigen_library
    searchpaths:      upstream
    defines:          BUNGEE_USE_PFFFT=1 BUNGEE_VERSION="2.4.8"

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

#include <bungee/Bungee.h>
#include <bungee/Stream.h>
