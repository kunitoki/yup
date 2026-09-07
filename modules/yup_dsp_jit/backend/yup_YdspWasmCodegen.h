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

#pragma once

namespace yup
{

//==============================================================================
/** Compiles one YdspIrFunction into a self-contained WebAssembly module.

    The emitted binary follows the YDSP wasm backend ABI:

      - it imports the host's linear memory as `env.memory`, so generated
        kernels address the host's buffers, params, meters and state directly
        through i32 offsets (no marshalling);
      - libm intrinsics (sinf, pow, ...) are imported from `env` by name;
      - a single function `(i32) -> ()` is exported as `ydsp_kernel`; it takes
        the YdspKernelContext (or YdspEventContext) pointer and is meant to be
        instantiated per JS realm via the browser's native WebAssembly API.

    The lowering maps every IR value id to a typed wasm local and lowers the
    (verified structured) IR control flow to wasm's block/loop/if forms. It is
    pure C++ and platform-independent: it compiles on every target that builds
    the yup_dsp_jit module and is exercised by byte-level tests on the desktop.

    @internal
*/
class YdspWasmCodegen
{
public:
    /** Compiles the IR function into a wasm module.
    
        @param fn the IR function to compile.
        @param diagnostics receives any errors or warnings that occur during compilation.
    
        @return empty bytes and records a diagnostic on failure.
    */
    static std::vector<uint8_t> compile (const YdspIrFunction& fn, YdspDiagnostics& diagnostics);

    /** Renders an emitted module in the WebAssembly text format (a readable
        debug dump of types, imports, exports, locals and instructions).

        This is a one-way listing, not a round-trippable wat source; it is the
        wasm counterpart of the asmjit assembly log the native backend records
        at compile time. Returns an empty string when `module` is not a valid
        MVP binary.

        @param module the wasm module bytes to render.

        @return the wat text, or empty when `module` is not a valid wasm binary.
    */
    static String toText (const std::vector<uint8_t>& module);
};

} // namespace yup
