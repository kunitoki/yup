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

    ID:               asmjit
    vendor:           asmjit
    version:          1.21.0
    name:             AsmJit
    description:      AsmJit is a lightweight library for generating machine code at runtime.
    website:          https://asmjit.com
    license:          Zlib

    defines:          ASMJIT_STATIC=1 ASMJIT_NO_SHM_OPEN=1
    searchpaths:      upstream

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

#include <asmjit/core.h>

#if !defined(ASMJIT_NO_X86)
  #include <asmjit/x86.h>
#endif

#if !defined(ASMJIT_NO_AARCH64)
  #include <asmjit/a64.h>
#endif

#include <asmjit/ujit.h>
#include <asmjit/host.h>
