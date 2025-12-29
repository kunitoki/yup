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

/*
  ==============================================================================

  BEGIN_YUP_MODULE_DECLARATION

    ID:               opus
    vendor:           Xiph.Org
    version:          1.6
    name:             Opus is a totally open, royalty-free, highly versatile audio codec
    description:      Opus is a totally open, royalty-free, highly versatile audio codec.
    website:          https://opus-codec.org/
    upstream:         https://downloads.xiph.org/releases/opus/opus-1.6.tar.gz
    sha256:           b7637334527201fdfd6dd6a02e67aceffb0e5e60155bbd89175647a80301c92c
    license:          BSD

    defines:
    searchpaths:      include celt silk silk/float silk/fixed dnn

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

#if defined(__aarch64__) || defined(__ARM_NEON) || defined(__ARM_NEON__)
#define OPUS_ARM_PRESUME_NEON_INTR 1
#define OPUS_ARM_PRESUME_NEON 1
#endif

#include <opus.h>
