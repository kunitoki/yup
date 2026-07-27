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

#pragma once

//==============================================================================

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#define VC_EXTRALEAN_DEFINED_HERE
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN_DEFINED_HERE
#endif

#ifndef NOMINMAX
#define NOMINMAX
#define NOMINMAX_DEFINED_HERE
#endif

#ifndef NOGDI
#define NOGDI
#define NOGDI_DEFINED_HERE
#endif

#include <windows.h>

#ifdef VC_EXTRALEAN_DEFINED_HERE
#undef VC_EXTRALEAN
#endif

#ifdef WIN32_LEAN_AND_MEAN_DEFINED_HERE
#undef WIN32_LEAN_AND_MEAN
#endif

#ifdef NOMINMAX_DEFINED_HERE
#undef NOMINMAX
#endif

#ifdef NOGDI_DEFINED_HERE
#undef NOGDI
#endif

//==============================================================================

#include <dbghelp.h>
