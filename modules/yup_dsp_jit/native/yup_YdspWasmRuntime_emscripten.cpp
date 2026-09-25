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

namespace yup
{

//==============================================================================

EM_JS (int, yupDspWasmRegisterKernelAt, (int targetHandle, const uint8_t* bytes, int numBytes, char* errorBuffer, int errorBufferSize), {
    function roundHalfAwayFromZero (x)
    {
        return Math.sign (x) * Math.floor (Math.abs (x) + 0.5);
    }

    function jsFmod (a, b)
    {
        return a % b; // JS % matches C fmod for finite values
    }

    function jsCopysign (a, b)
    {
        return (b < 0 || Object.is (b, -0)) ? -Math.abs (a) : Math.abs (a);
    }

    function buildEnv ()
    {
        var memory = (typeof Module !== 'undefined' && Module.wasmMemory) ? Module.wasmMemory : (typeof wasmMemory !== 'undefined' ? wasmMemory : null);

        if (! memory)
            throw new Error ('Module.wasmMemory is unavailable; export it with -sEXPORTED_RUNTIME_METHODS=wasmMemory');

        var unary = {
            sin: Math.sin, cos: Math.cos, tan: Math.tan,
            asin: Math.asin, acos: Math.acos, atan: Math.atan,
            sinh: Math.sinh, cosh: Math.cosh, tanh: Math.tanh,
            asinh: Math.asinh, acosh: Math.acosh, atanh: Math.atanh,
            exp: Math.exp, log: Math.log, log10: Math.log10,
            round: roundHalfAwayFromZero
        };

        var binary = {
            pow: Math.pow, atan2: Math.atan2,
            fmod: jsFmod, copysign: jsCopysign
        };

        var env = { memory: memory };

        env.ydspCommitOutputEvent = Module._ydspCommitOutputEventWasm;
        env.ydspTraceInt = Module._ydspTraceIntWasm;
        env.ydspTraceFloat = Module._ydspTraceFloat;
        env.ydspTraceCommit = Module._ydspTraceCommit;

        for (var key in unary)
        {
            env[key] = unary[key];
            env[key + 'f'] = unary[key];
        }

        for (var binaryKey in binary)
        {
            env[binaryKey] = binary[binaryKey];
            env[binaryKey + 'f'] = binary[binaryKey];
        }

        return env;
    }

    var registry = Module.yupDspKernelRegistry || (Module.yupDspKernelRegistry = []);

    if (registry[targetHandle])
        return 0; // already compiled this module (e.g. a prewarmed one): skip

    try
    {
        var module = new WebAssembly.Module (HEAPU8.subarray (bytes, bytes + numBytes));
        var instance = new WebAssembly.Instance (module, { env: buildEnv() });
        var kernel = instance.exports.ydsp_kernel;

        if (typeof kernel !== 'function')
            throw new Error ('generated wasm module does not export ydsp_kernel');

        registry[targetHandle] = kernel;

        return 0;
    }
    catch (e)
    {
        if (errorBuffer && errorBufferSize > 0)
            stringToUTF8 (String (e && e.message ? e.message : e), errorBuffer, errorBufferSize);

        return -1;
    }
});

EM_JS (void, yupDspWasmCallKernel, (int kernelId, const uint8_t* bytes, int numBytes, int ctxPtr), {
    var registry = Module.yupDspKernelRegistry || (Module.yupDspKernelRegistry = []);
    var kernel = registry[kernelId];

    if (! kernel)
    {
        if (yupDspWasmRegisterKernelAt (kernelId, bytes, numBytes, 0, 0) != 0)
            return;

        kernel = registry[kernelId];
    }

    try
    {
        kernel (ctxPtr);
    }
    catch (e)
    {
        console.error ('YDSP kernel #' + kernelId + ' trapped: ' + (e && e.stack ? e.stack : e));
    }
});

EM_JS (void, yupDspWasmFreeKernel, (int kernelId), {
    var registry = Module.yupDspKernelRegistry || (Module.yupDspKernelRegistry = []);
    registry[kernelId] = null;
});

//==============================================================================

namespace
{
    /** Hands out kernel ids that are unique for the whole module, not just for
        the realm that asks for one.

        Every realm keeps its own `Module.yupDspKernelRegistry`, and a handle
        minted in one realm is looked up in whichever realm runs the graph. Two
        realms numbering independently would therefore hand out the same number
        for different kernels, and since `yupDspWasmRegisterKernelAt` treats an
        occupied slot as already registered, the second realm would keep the
        first realm's module and silently run it with its own context. Sitting
        in the module's static memory, which every pthread worker shares, this
        counter is the one place the numbers can be made to never repeat.

        @see YdspWasmRuntime::registerKernel
    */
    std::atomic<int> ydspNextWasmKernelId { 1 };
}

//==============================================================================

YdspWasmKernelHandle YdspWasmRuntime::registerKernel (const uint8_t* bytes, size_t numBytes, String& errorMessage)
{
    char errorBuffer[512] = {};

    const auto handle = ydspNextWasmKernelId.fetch_add (1);

    if (yupDspWasmRegisterKernelAt (handle, bytes, static_cast<int> (numBytes), errorBuffer, static_cast<int> (sizeof (errorBuffer))) != 0)
    {
        errorMessage = errorBuffer;
        return -1;
    }

    return handle;
}

void YdspWasmRuntime::callKernel (YdspWasmKernelHandle handle, const uint8_t* bytes, size_t numBytes, YdspKernelContext* ctx)
{
    jassert (handle >= 0);

    yupDspWasmCallKernel (handle, bytes, static_cast<int> (numBytes), static_cast<int> (reinterpret_cast<uintptr_t> (ctx)));
}

void YdspWasmRuntime::callEventHandler (YdspWasmKernelHandle handle, const uint8_t* bytes, size_t numBytes, YdspEventContext* ctx)
{
    jassert (handle >= 0);

    yupDspWasmCallKernel (handle, bytes, static_cast<int> (numBytes), static_cast<int> (reinterpret_cast<uintptr_t> (ctx)));
}

void YdspWasmRuntime::freeKernel (YdspWasmKernelHandle handle)
{
    jassert (handle >= 0);

    yupDspWasmFreeKernel (handle);
}

void YdspWasmRuntime::prewarmInCurrentRealm (const std::vector<YdspWasmKernelHandle>& kernelIds, const std::vector<std::vector<uint8_t>>& modules)
{
    for (size_t i = 0; i < modules.size() && i < kernelIds.size(); ++i)
        yupDspWasmRegisterKernelAt (kernelIds[i], modules[i].data(), static_cast<int> (modules[i].size()), nullptr, 0);
}

} // namespace yup
