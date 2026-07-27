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

#include "mocks/rive_gpu.h"
#include "mocks/rive_ore.h"
#include "mocks/yup_rhi.h"

#include "yup_rhi/yup_GpuTarget.cpp"
#include "yup_rhi/yup_GpuPipeline.cpp"
#include "yup_rhi/yup_GpuPipelineMocked.cpp"

#if YUP_LINUX
#include "yup_rhi/native/yup_GpuDevice_linux.cpp"
#endif
