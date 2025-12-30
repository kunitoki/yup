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

    ID:               flac_library
    vendor:           flac_library
    version:          1.5.0
    name:             FLAC stands for Free Lossless Audio Codec, an audio format similar to MP3, but lossless
    description:      FLAC stands for Free Lossless Audio Codec, an audio format similar to MP3, but lossless.
    website:          https://xiph.org/flac
    upstream:         https://ftp.osuosl.org/pub/xiph/releases/flac/flac-1.5.0.tar.xz
    license:          BSD

    defines:          FLAC__NO_DLL=1 FLAC__HAS_OGG=0
    macDefines:       FLAC__SYS_DARWIN=1
    searchpaths:      include src src/libFLAC src/libFLAC/include

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

#include <FLAC/all.h>
