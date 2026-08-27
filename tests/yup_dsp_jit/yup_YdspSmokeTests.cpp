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

#include <gtest/gtest.h>

#include <yup_dsp_jit/yup_dsp_jit.h>

using namespace yup;

//==============================================================================

TEST (YdspJitModuleTests, ModuleIsAvailable)
{
    EXPECT_TRUE (YUP_MODULE_AVAILABLE_yup_dsp_jit);

#if ! YUP_WASM
    EXPECT_TRUE (YUP_MODULE_AVAILABLE_asmjit_library);
#endif
}

TEST (YdspJitModuleTests, HostArchitectureIsSupported)
{
#if YUP_WASM
    // The wasm backend emits portable wasm bytes that run on the browser's
    // native WebAssembly engine, so there is no host-architecture requirement.
    EXPECT_TRUE (true);
#else
    // asmjit must be able to target the host so JIT code can be emitted;
    // x86-64 and AArch64 are the two backends of the yup_dsp_jit module.
    const auto arch = asmjit::Environment::host().arch();
    EXPECT_TRUE (arch == asmjit::Arch::kX64 || arch == asmjit::Arch::kAArch64);
#endif
}
