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
    version:          1.22.0
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

#if defined(_M_X64) || defined(__x86_64__)
  #define ASMJIT_ARCH_X86 64
#elif defined(_M_IX86) || defined(__X86__) || defined(__i386__)
  #define ASMJIT_ARCH_X86 32
#else
  #define ASMJIT_ARCH_X86 0
#endif

#if defined(_M_ARM64) || defined(__arm64__) || defined(__aarch64__)
  #define ASMJIT_ARCH_ARM 64
#elif defined(_M_ARM) || defined(_M_ARMT) || defined(__arm__) || defined(__thumb__) || defined(__thumb2__)
  #define ASMJIT_ARCH_ARM 32
#else
  #define ASMJIT_ARCH_ARM 0
#endif

#include <asmjit/core.h>

#if !defined(ASMJIT_NO_X86)
  #include <asmjit/x86.h>
#endif

#if !defined(ASMJIT_NO_AARCH64)
  #include <asmjit/a64.h>
#endif

#include <asmjit/ujit.h>
#include <asmjit/host.h>
