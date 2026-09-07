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
/** The fixed ABI shared by every generated kernel.

    All buffers and state are preallocated by the host; a kernel performs no
    allocation and touches only the memory reachable from this context. Buffer
    element types are declared per kernel (inputTypes/outputTypes/paramTypes/
    paramOutTypes in YdspIrFunction); pointers are void* so f32, f64, i32 and
    i64 streams/params can share the same ABI.

    This layout is shared by every backend: the asmjit (native) codegen
    addresses it through the host ABI, while the wasm codegen hardcodes the
    wasm32 field offsets (see YdspWasmCodegen, which static-asserts them when
    compiled for a wasm target).
*/
struct YdspKernelContext
{
    void* const* inputs = nullptr;  // input stream pointers (graph order)
    void* const* outputs = nullptr; // output stream pointers (graph order)
    void* params = nullptr;         // input value endpoints (sampled once per block)
    void* paramOut = nullptr;       // output value endpoints (meters)
    void* state = nullptr;          // scalar segment: every scalar slot (small offsets)
    void* stateArrays = nullptr;    // array segment: every array (delay lines, rings)
    float sampleRate = 0.0f;        // host sample rate
    int32_t numSamples = 0;         // samples in this block
    void* outputEvents = nullptr;   // this invocation's output-event staging queue
};

using YdspKernelFn = void (*) (YdspKernelContext*);

//==============================================================================
/** YdspEventContext::flags bit 0: this noteOn continues a mono-mode legato
    phrase rather than starting a new note. */
inline constexpr int32_t ydspEventFlagLegato = 1;

/** The fixed ABI of an event handler: one voice's state slice, the owning
    node's shared params, and the dispatching event's payload.

    Every event shape shares this one payload block; which fields carry meaning
    depends on the shape that dispatched the handler (see ydspEventShapes in
    yup_YdspTypes.h, the single source of truth). Fields the shape does not
    carry are zero. The IR addresses the payload by byte offset, so a shape may
    grow a field without a new opcode.
*/
struct YdspEventContext
{
    float* state = nullptr;       // scalar segment (this voice's scalar slice)
    float* stateArrays = nullptr; // array segment (this voice's array slice)
    float* params = nullptr;      // the node's param block (shared across voices)
    float sampleRate = 0.0f;      // host sample rate
    float pitch = 0.0f;           // MIDI note scale
    float velocity = 0.0f;        // 0..1
    float pressure = 0.0f;        // 0..1
    float slide = 0.0f;           // 0..1
    float bend = 0.0f;            // semitones, signed
    float value = 0.0f;           // control value, 0..1
    int32_t index = 0;            // control: CC number; programChange: program number
    int32_t flags = 0;            // bit flags (see ydspEventFlagLegato)
    int32_t channel = 0;          // 0-based channel number
    int32_t sampleOffset = 0;     // block-relative sample offset of the dispatching event
    void* outputEvents = nullptr; // this invocation's output-event staging queue
};

using YdspEventHandlerFn = void (*) (YdspEventContext*);

//==============================================================================
// What `outputEvents` points to: `staging` must stay the first member, since
// the IR's `memIndex` byte offsets for storeEventFieldF/I are measured from
// YdspEventContext's own base and so apply here with no adjustment.
struct YdspOutputEventQueue
{
    YdspEventContext staging {};

    struct Entry // a committed event: staging's snapshot plus emitEvent's operands
    {
        int32_t sampleOffset = 0;
        int32_t endpointIndex = 0;
        int64_t shapeTag = 0;
        YdspEventContext fields {};
    };

    std::vector<Entry> entries; // reserved once in prepare(), never reallocated mid-block
    std::atomic<uint64_t> droppedCount { 0 };

    YdspOutputEventQueue() = default;

    YdspOutputEventQueue (YdspOutputEventQueue&& other) noexcept
        : staging (other.staging)
        , entries (std::move (other.entries))
        , droppedCount (other.droppedCount.load (std::memory_order_relaxed))
    {
    }

    YdspOutputEventQueue& operator= (YdspOutputEventQueue&& other) noexcept
    {
        staging = other.staging;
        entries = std::move (other.entries);
        droppedCount.store (other.droppedCount.load (std::memory_order_relaxed), std::memory_order_relaxed);
        return *this;
    }
};

static_assert (offsetof (YdspOutputEventQueue, staging) == 0);

// Commits queue->staging (shape shapeTag, at sampleOffset, endpoint
// endpointIndex) as a new entry, or increments queue->droppedCount if the
// queue is at its reserved capacity. Defined out-of-line in yup_YdspGraph.cpp.
extern "C" void ydspCommitOutputEvent (YdspOutputEventQueue* queue, int64_t shapeTag, int32_t sampleOffset, int32_t endpointIndex);

//==============================================================================
/** Converts between ABI-compatible function-pointer types.

    Every native kernel/event-handler entry point shares the `void (*) (void*)`
    shape at the machine level, so backends and YdspCompiledKernel need to
    convert between YdspKernelFn, YdspEventHandlerFn and that generic shape.
    This is the one place such a conversion should happen - don't
    reinterpret_cast between function-pointer types elsewhere. */
template <typename To, typename From>
inline To ydspFnPtrCast (From fn) noexcept
{
    static_assert (std::is_pointer_v<To> && std::is_pointer_v<From>, "ydspFnPtrCast converts between function pointer types");
    return reinterpret_cast<To> (fn);
}

/** Converts a native kernel-call target's function pointer into the 64-bit
    immediate value the codegen embeds as a call address.

    Requires a 64-bit target, which always holds here: this conversion is
    only used by the native (asmjit) backends, which are x86-64 or AArch64. */
template <typename Fn>
inline int64_t ydspFnPtrToInt64 (Fn fn) noexcept
{
    static_assert (std::is_pointer_v<Fn>, "ydspFnPtrToInt64 converts a function pointer");
    static_assert (sizeof (Fn) == sizeof (int64_t), "ydspFnPtrToInt64 requires a 64-bit target");
    return std::bit_cast<int64_t> (fn);
}

} // namespace yup
