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
/** Opaque handle of a wasm kernel registered in a JS realm.

    The handle indexes a JS-side registry owned by the emscripten glue; it is
    only meaningful in the realm that registered it (the main thread, or the
    audio-worklet thread, which has its own realm).
*/
using YdspWasmKernelHandle = int;

//==============================================================================
/** Executes kernels emitted by YdspWasmCodegen via the browser's native
    WebAssembly API.

    The static functions are declared on every target but only implemented by
    the emscripten glue (backend/native/yup_YdspWasmRuntime_emscripten.cpp),
    and are only referenced under YUP_WASM. Registration is synchronous and
    realm-local:

    @code
      new WebAssembly.Instance (new WebAssembly.Module (bytes), { env: { memory: wasmMemory, ... libm } })
    @endcode
      
    in the calling JS realm, so host pointers reachable through the shared
    linear memory stay valid.

    @internal
*/
class YdspWasmRuntime
{
public:
    /** Registers a wasm module in the calling realm.
    
        @param bytes The wasm module bytes (must remain valid for the kernel's lifetime).
        @param numBytes The number of bytes in `bytes`.
        @param errorMessage On failure, set to a human-readable error message.

        @return a handle >= 0 or -1 with `errorMessage` set on failure.
    */
    static YdspWasmKernelHandle registerKernel (const uint8_t* bytes, size_t numBytes, String& errorMessage);

    /** Invokes a registered kernel on the given context.

        The module is registered lazily in the calling realm on first use:
        each JS realm (the main thread, or the audio-worklet Wasm Worker, which
        has its own realm) instantiates its own copy from `bytes`, so the same
        handle is valid on every thread that runs the graph. `bytes`/`numBytes`
        must remain valid for the kernel's lifetime (the graph owns them).
    
        @param handle The kernel handle returned by `registerKernel`.
        @param bytes The wasm module bytes (must remain valid for the kernel's lifetime).
        @param numBytes The number of bytes in `bytes`.
        @param ctx The kernel context to run the kernel on.
    */
    static void callKernel (YdspWasmKernelHandle handle, const uint8_t* bytes, size_t numBytes, YdspKernelContext* ctx);

    /** Invokes a registered event handler on the given context (see callKernel).
    
        @param handle The kernel handle returned by `registerKernel`.
        @param bytes The wasm module bytes (must remain valid for the kernel's lifetime).
        @param numBytes The number of bytes in `bytes`.
        @param ctx The event context to run the handler on.
    */
    static void callEventHandler (YdspWasmKernelHandle handle, const uint8_t* bytes, size_t numBytes, YdspEventContext* ctx);

    /** Releases a registered kernel in the calling realm.
    
        @param handle The kernel handle returned by `registerKernel`.
    */
    static void freeKernel (YdspWasmKernelHandle handle);

    /** Registers every kernel in `kernelIds`/`modules` in the calling realm.
        
        Call from a realm before it runs the graph — e.g. post a function to the
        audio-worklet thread — to avoid the one-time instantiation cost on the
        first audio block.

        @warning This is only a performance hint for WASM, it is a no-op on native asmjit.

        @param kernelIds The handles returned by `registerKernel` for each module.
        @param modules The wasm module bytes for each kernel (must remain valid for the kernel's lifetime).
    */
    static void prewarmInCurrentRealm (const std::vector<YdspWasmKernelHandle>& kernelIds, const std::vector<std::vector<uint8_t>>& modules);
};

//==============================================================================
/** A compiled kernel ready to run, regardless of the codegen backend.

    Wraps either a native asmjit function pointer (desktop) or a wasm kernel
    handle registered in the current JS realm (wasm). Non-owning: native
    pointers are owned by the asmjit runtime, and wasm handles are owned by
    the graph runtime (freed when the graph is destroyed) - copies of the
    wrapper share the same underlying kernel, matching how one compiled kernel
    or event handler is shared by several nodes.

    The call syntax is identical to the raw function pointer it replaces:
    `kernel (&ctx)` / `handler (&eventCtx)`.
*/
class YdspCompiledKernel
{
public:
    /** Default constructor: an invalid (uncompiled) kernel. */
    YdspCompiledKernel() = default;

    /** Wraps a native kernel function pointer. */
    explicit YdspCompiledKernel (YdspKernelFn nativeFn) noexcept
        : nativeFn (ydspFnPtrCast<void (*) (void*)> (nativeFn))
    {
    }

    /** Wraps a native event-handler function pointer. */
    explicit YdspCompiledKernel (YdspEventHandlerFn nativeFn) noexcept
        : nativeFn (ydspFnPtrCast<void (*) (void*)> (nativeFn))
    {
    }

#if YUP_WASM
    /** Wraps a wasm kernel registered in the current realm.

        `kernelId` is the unique per-kernel registry key (never reused across
        graphs, so a worklet realm can never invoke an older graph's kernel
        with a newer graph's context). `wasmModules` is the owning graph's byte
        storage and `wasmIndex` the position of this kernel within it; the
        bytes let other realms (e.g. the audio worklet) lazily instantiate
        their own copy on first use. */
    explicit YdspCompiledKernel (YdspWasmKernelHandle kernelId, const std::vector<std::vector<uint8_t>>* wasmModules, size_t wasmIndex) noexcept
        : kernelId (kernelId)
        , wasmModules (wasmModules)
        , wasmIndex (wasmIndex)
    {
    }
#endif

    /** Returns true when a kernel is attached. */
    bool isValid() const noexcept
    {
#if YUP_WASM
        if (kernelId >= 0)
            return true;
#endif

        return nativeFn != nullptr;
    }

    /** Runs the kernel on the given kernel context. */
    void operator() (YdspKernelContext* ctx) const
    {
#if YUP_WASM
        if (kernelId >= 0)
        {
            YdspWasmRuntime::callKernel (kernelId, wasmBytes(), wasmSize(), ctx);
            return;
        }
#endif

        jassert (nativeFn != nullptr);
        ydspFnPtrCast<YdspKernelFn> (nativeFn) (ctx);
    }

    /** Runs the kernel as an event handler on the given event context. */
    void operator() (YdspEventContext* ctx) const
    {
#if YUP_WASM
        if (kernelId >= 0)
        {
            YdspWasmRuntime::callEventHandler (kernelId, wasmBytes(), wasmSize(), ctx);
            return;
        }
#endif

        jassert (nativeFn != nullptr);
        ydspFnPtrCast<YdspEventHandlerFn> (nativeFn) (ctx);
    }

private:
#if YUP_WASM
    const uint8_t* wasmBytes() const noexcept
    {
        return wasmModules != nullptr && wasmIndex < wasmModules->size()
            ? (*wasmModules)[wasmIndex].data()
            : nullptr;
    }

    size_t wasmSize() const noexcept
    {
        return wasmModules != nullptr && wasmIndex < wasmModules->size()
            ? (*wasmModules)[wasmIndex].size()
            : 0;
    }
#endif

    void (*nativeFn) (void*) = nullptr;
    YdspWasmKernelHandle kernelId = -1;
    const std::vector<std::vector<uint8_t>>* wasmModules = nullptr;
    size_t wasmIndex = 0;
};

} // namespace yup
