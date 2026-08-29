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
/** A minimal WebAssembly (MVP) binary emitter.

    Writes a single-module .wasm binary: magic + version, then type, import,
    function, export and code sections. The generated modules follow the
    conventions of the YDSP wasm backend:

      - the host's linear memory is imported as `env.memory` (shared min+max
        limits, matching emscripten's shared, growable wasmMemory), letting
        generated kernels address host buffers directly through i32 offsets
        (no marshalling);
      - libm intrinsics are imported as `env.<name>` functions (`sinf`, `sin`,
        `powf`, ...) and invoked via `call`;
      - a single function `(i32) -> ()` is exported as `ydsp_kernel`; it takes
        the YdspKernelContext (or YdspEventContext) pointer and is meant to be
        instantiated per JS realm via the browser's native WebAssembly API.

    The emitter is deliberately low-level: it owns a byte buffer and a set of
    LEB128/section/opcode helpers. It does not interpret the YDSP IR - that is
    the job of YdspWasmCodegen. The MVP subset (no multi-value, no bulk memory)
    is sufficient for every scalar YdspIrOp and is universally supported by
    browsers, node and emscripten's WebAssembly runtime. When compiled with
    `-msimd128` (the emscripten default, which defines `__wasm_simd128__`), the
    emitter additionally exposes the 128-bit SIMD prefix (`0xFD`) and the f32x4
    subset the YDSP vectorized IR lowers to; without it, the emitter stays
    byte-identical to a pure MVP writer.

    @internal
*/
class YdspWasmEmitter
{
public:
    //==============================================================================
    /** A WebAssembly value type (used for locals, params and block types). */
    enum class ValType : uint8_t
    {
        i32 = 0x7F,
        i64 = 0x7E,
        f32 = 0x7D,
        f64 = 0x7C
#if defined (__wasm_simd128__)
        , v128 = 0x7B
#endif
    };

    //==============================================================================
    // Byte primitives (LEB128 encodings)

    /** Appends a single byte. */
    void u8 (uint8_t value)
    {
        target().push_back (value);
    }

    /** Appends an unsigned LEB128 u32. */
    void u32 (uint32_t value);

    /** Appends a signed LEB128 s32. */
    void s32 (int32_t value);

    /** Appends an unsigned LEB128 u64. */
    void u64 (uint64_t value);

    /** Appends a signed LEB128 s64. */
    void s64 (int64_t value);

    /** Appends raw bytes. */
    void bytes (const void* data, size_t numBytes);

    //==============================================================================
    // Module and section scaffolding

    /** Appends the magic bytes and version. Must be called first. */
    void beginModule();

    /** Starts a section: subsequent entries are buffered and flushed with a
        section id and a size prefix by endSection(). The payload must begin
        with the section's element count (the caller knows it). */
    void beginSection (uint8_t sectionId);

    /** Flushes the buffered section payload (id, size, payload). */
    void endSection();

    //==============================================================================
    // Section entries (valid between beginSection/endSection of the right id)

    /** Appends a function type `(params) -> (results)` to the type section.
        Returns its type index. */
    int funcType (std::initializer_list<ValType> params, std::initializer_list<ValType> results);

    /** Appends a memory import `(module, name, limits)` to the import
        section.

        `shared` must be true to match a shared host memory (emscripten's
        wasmMemory is shared under -sSHARED_MEMORY); a shared import requires a
        maximum. When `maxPages` is zero no maximum is declared (matching any
        host memory without a max constraint); when nonzero, any host memory
        whose maximum does not exceed `maxPages` matches.
    */
    void importMemory (const char* moduleName, const char* name, uint32_t minPages, uint32_t maxPages = 0, bool shared = true);

    /** Appends a function import `(module, name, typeIndex)` to the import
        section. Used for libm intrinsics (`env.sinf`, `env.sin`, ...). */
    void importFunction (const char* moduleName, const char* name, uint32_t typeIndex);

    /** Appends an entry to the function section (a defined function's type).
        Returns the function index (defined functions are numbered after
        imports). */
    int functionEntry (uint32_t typeIndex);

    /** Appends a function export `(name, funcIndex)` to the export section. */
    void exportFunction (const char* name, uint32_t funcIndex);

    //==============================================================================
    // Function bodies (valid between beginBody/endBody of the code section)

    /** Starts a function body: subsequent locals and instructions are
        buffered until endBody() flushes them with a size prefix. */
    void beginBody();

    /** Declares a run of `count` locals of the given type for the current
        body. Must be called before any instruction; runs are compressed
        at endBody(). Local index 0 is always the function parameter. */
    void declareLocals (uint32_t count, ValType type);

    /** Flushes the current body: locals declaration, instructions and the
        terminating `end` opcode, prefixed with the body size. */
    void endBody();

    //==============================================================================
    // Instructions (appended to the current body)

    /** Appends a raw opcode (no immediate operands). */
    void op (uint8_t opcode);

    /** Appends `block` with an empty block type (0x40). */
    void block();

    /** Appends `loop` with an empty block type (0x40). */
    void loop();

    /** Appends `if` with an empty block type (0x40). */
    void if_();

    /** Appends `if` producing a single value of the given type (used when both
        arms leave one value on the stack, e.g. the zero-division guard). */
    void ifValue (ValType resultType);

    /** Appends `else`. */
    void else_();

    /** Appends `end`. */
    void end();

    /** Appends `br <depth>`. */
    void br (uint32_t depth);

    /** Appends `br_if <depth>`. */
    void brIf (uint32_t depth);

    /** Appends `return`. */
    void ret();

    /** Appends `call <funcIndex>`. */
    void call (uint32_t funcIndex);

    /** Appends `drop`. */
    void drop();

    /** Appends `select` (untyped; both operands have the same type). */
    void select();

    /** Appends `local.get <index>`. */
    void localGet (uint32_t index);

    /** Appends `local.set <index>`. */
    void localSet (uint32_t index);

    /** Appends `local.tee <index>`. */
    void localTee (uint32_t index);

    /** Appends `i32.const <value>`. */
    void i32Const (int32_t value);

    /** Appends `i64.const <value>`. */
    void i64Const (int64_t value);

    /** Appends `f32.const <value>` (IEEE-754 bit pattern). */
    void f32Const (float value);

    /** Appends `f64.const <value>` (IEEE-754 bit pattern). */
    void f64Const (double value);

    /** Appends a memory load instruction with the given memarg. The value on
        the stack is the i32 byte address (an offset into the imported host
        memory). `alignLog2` is log2 of the access width (2 for 4-byte, 3 for
        8-byte); `offset` is a byte offset added to the address. */
    void load (uint8_t opcode, uint32_t alignLog2, uint32_t offset);

    /** Appends a memory store instruction (see load()). */
    void store (uint8_t opcode, uint32_t alignLog2, uint32_t offset);

    /** Appends `nop`. */
    void nop();

    /** Appends `unreachable`. */
    void unreachable();

#if defined (__wasm_simd128__)
    //==============================================================================
    // SIMD instructions (0xFD prefix, available with -msimd128)

    /** Appends a 0xFD-prefixed SIMD instruction with the given subopcode. */
    void simdOp (uint32_t subopcode);

    /** Appends a SIMD memory load (e.g. `v128.load`) with a memarg. */
    void simdLoad (uint32_t subopcode, uint32_t alignLog2, uint32_t offset);

    /** Appends a SIMD memory store (e.g. `v128.store`) with a memarg. */
    void simdStore (uint32_t subopcode, uint32_t alignLog2, uint32_t offset);

    /** Appends `v128.const` with a raw 16-byte lane payload. */
    void v128Const (const uint8_t data[16]);

    /** Appends `i8x16.shuffle` with a 16-byte lane mask. */
    void i8x16Shuffle (const uint8_t mask[16]);

    /** Appends `f32x4.extract_lane <lane>`. */
    void f32x4ExtractLane (uint32_t lane);
#endif

    //==============================================================================
    // Result

    /** Returns the assembled module bytes. */
    const std::vector<uint8_t>& getBytes() const noexcept
    {
        return out;
    }

    /** Returns the assembled module bytes and clears the emitter. */
    std::vector<uint8_t> takeBytes()
    {
        return std::move (out);
    }

    //==============================================================================
    // Opcode constants (the subset used by the YDSP lowering)

    // Control
    static constexpr uint8_t opUnreachable = 0x00;
    static constexpr uint8_t opNop = 0x01;
    static constexpr uint8_t opBlock = 0x02;
    static constexpr uint8_t opLoop = 0x03;
    static constexpr uint8_t opIf = 0x04;
    static constexpr uint8_t opElse = 0x05;
    static constexpr uint8_t opEnd = 0x0B;
    static constexpr uint8_t opBr = 0x0C;
    static constexpr uint8_t opBrIf = 0x0D;
    static constexpr uint8_t opReturn = 0x0F;
    static constexpr uint8_t opCall = 0x10;
    static constexpr uint8_t opDrop = 0x1A;
    static constexpr uint8_t opSelect = 0x1B;

    // Locals
    static constexpr uint8_t opLocalGet = 0x20;
    static constexpr uint8_t opLocalSet = 0x21;
    static constexpr uint8_t opLocalTee = 0x22;

    // Constants
    static constexpr uint8_t opI32Const = 0x41;
    static constexpr uint8_t opI64Const = 0x42;
    static constexpr uint8_t opF32Const = 0x43;
    static constexpr uint8_t opF64Const = 0x44;

    // Comparison (i32)
    static constexpr uint8_t opI32Eqz = 0x45;
    static constexpr uint8_t opI32Eq = 0x46;
    static constexpr uint8_t opI32Ne = 0x47;
    static constexpr uint8_t opI32LtS = 0x48;
    static constexpr uint8_t opI32GtS = 0x4A;
    static constexpr uint8_t opI32LeS = 0x4C;
    static constexpr uint8_t opI32GeS = 0x4E;

    // Comparison (i64)
    static constexpr uint8_t opI64Eqz = 0x50;
    static constexpr uint8_t opI64Eq = 0x51;
    static constexpr uint8_t opI64Ne = 0x52;
    static constexpr uint8_t opI64LtS = 0x53;
    static constexpr uint8_t opI64GtS = 0x55;
    static constexpr uint8_t opI64LeS = 0x57;
    static constexpr uint8_t opI64GeS = 0x59;

    // Comparison (f32)
    static constexpr uint8_t opF32Eq = 0x5B;
    static constexpr uint8_t opF32Ne = 0x5C;
    static constexpr uint8_t opF32Lt = 0x5D;
    static constexpr uint8_t opF32Gt = 0x5E;
    static constexpr uint8_t opF32Le = 0x5F;
    static constexpr uint8_t opF32Ge = 0x60;

    // Comparison (f64)
    static constexpr uint8_t opF64Eq = 0x61;
    static constexpr uint8_t opF64Ne = 0x62;
    static constexpr uint8_t opF64Lt = 0x63;
    static constexpr uint8_t opF64Gt = 0x64;
    static constexpr uint8_t opF64Le = 0x65;
    static constexpr uint8_t opF64Ge = 0x66;

    // Integer arithmetic (i32)
    static constexpr uint8_t opI32Add = 0x6A;
    static constexpr uint8_t opI32Sub = 0x6B;
    static constexpr uint8_t opI32Mul = 0x6C;
    static constexpr uint8_t opI32DivS = 0x6D;
    static constexpr uint8_t opI32RemS = 0x6F;
    static constexpr uint8_t opI32And = 0x71;
    static constexpr uint8_t opI32Or = 0x72;
    static constexpr uint8_t opI32Xor = 0x73;
    static constexpr uint8_t opI32Shl = 0x74;
    static constexpr uint8_t opI32ShrS = 0x75;

    // Integer arithmetic (i64)
    static constexpr uint8_t opI64Add = 0x7C;
    static constexpr uint8_t opI64Sub = 0x7D;
    static constexpr uint8_t opI64Mul = 0x7E;
    static constexpr uint8_t opI64DivS = 0x7F;
    static constexpr uint8_t opI64RemS = 0x81;
    static constexpr uint8_t opI64And = 0x83;
    static constexpr uint8_t opI64Or = 0x84;
    static constexpr uint8_t opI64Xor = 0x85;
    static constexpr uint8_t opI64Shl = 0x86;
    static constexpr uint8_t opI64ShrS = 0x87;

    // Float arithmetic (f32)
    static constexpr uint8_t opF32Abs = 0x8B;
    static constexpr uint8_t opF32Neg = 0x8C;
    static constexpr uint8_t opF32Ceil = 0x8D;
    static constexpr uint8_t opF32Floor = 0x8E;
    static constexpr uint8_t opF32Trunc = 0x8F;
    static constexpr uint8_t opF32Nearest = 0x90;
    static constexpr uint8_t opF32Sqrt = 0x91;
    static constexpr uint8_t opF32Add = 0x92;
    static constexpr uint8_t opF32Sub = 0x93;
    static constexpr uint8_t opF32Mul = 0x94;
    static constexpr uint8_t opF32Div = 0x95;
    static constexpr uint8_t opF32Min = 0x96;
    static constexpr uint8_t opF32Max = 0x97;
    static constexpr uint8_t opF32Copysign = 0x98;

    // Float arithmetic (f64)
    static constexpr uint8_t opF64Abs = 0x99;
    static constexpr uint8_t opF64Neg = 0x9A;
    static constexpr uint8_t opF64Ceil = 0x9B;
    static constexpr uint8_t opF64Floor = 0x9C;
    static constexpr uint8_t opF64Trunc = 0x9D;
    static constexpr uint8_t opF64Nearest = 0x9E;
    static constexpr uint8_t opF64Sqrt = 0x9F;
    static constexpr uint8_t opF64Add = 0xA0;
    static constexpr uint8_t opF64Sub = 0xA1;
    static constexpr uint8_t opF64Mul = 0xA2;
    static constexpr uint8_t opF64Div = 0xA3;
    static constexpr uint8_t opF64Min = 0xA4;
    static constexpr uint8_t opF64Max = 0xA5;
    static constexpr uint8_t opF64Copysign = 0xA6;

    // Conversions
    static constexpr uint8_t opI32WrapI64 = 0xA7;
    static constexpr uint8_t opI32TruncF32S = 0xA8;
    static constexpr uint8_t opI32TruncF64S = 0xAA;
    static constexpr uint8_t opI64ExtendI32S = 0xAC;
    static constexpr uint8_t opI64TruncF32S = 0xAE;
    static constexpr uint8_t opI64TruncF64S = 0xB0;
    static constexpr uint8_t opF32ConvertI32S = 0xB2;
    static constexpr uint8_t opF32ConvertI64S = 0xB4;
    static constexpr uint8_t opF32DemoteF64 = 0xB6;
    static constexpr uint8_t opF64ConvertI32S = 0xB7;
    static constexpr uint8_t opF64ConvertI64S = 0xB9;
    static constexpr uint8_t opF64PromoteF32 = 0xBB;

    // Memory
    static constexpr uint8_t opI32Load = 0x28;
    static constexpr uint8_t opI64Load = 0x29;
    static constexpr uint8_t opF32Load = 0x2A;
    static constexpr uint8_t opF64Load = 0x2B;
    static constexpr uint8_t opI32Store = 0x36;
    static constexpr uint8_t opI64Store = 0x37;
    static constexpr uint8_t opF32Store = 0x38;
    static constexpr uint8_t opF64Store = 0x39;

#if defined (__wasm_simd128__)
    // SIMD (Table D, 0xFD prefix; subopcodes per the WebAssembly SIMD spec)
    static constexpr uint8_t simdPrefix = 0xFD;

    // Memory
    static constexpr uint32_t opV128Load = 0x00;
    static constexpr uint32_t opV128Store = 0x0B;

    // Lane manipulation
    static constexpr uint32_t opI8x16Shuffle = 0x0D;
    static constexpr uint32_t opF32x4Splat = 0x13;
    static constexpr uint32_t opF32x4ExtractLane = 0x1F;

    // Rounding
    static constexpr uint32_t opF32x4Ceil = 0x67;
    static constexpr uint32_t opF32x4Floor = 0x68;
    static constexpr uint32_t opF32x4Nearest = 0x6A;

    // Float arithmetic (f32x4)
    static constexpr uint32_t opF32x4Abs = 0xE0;
    static constexpr uint32_t opF32x4Neg = 0xE1;
    static constexpr uint32_t opF32x4Sqrt = 0xE3;
    static constexpr uint32_t opF32x4Add = 0xE4;
    static constexpr uint32_t opF32x4Sub = 0xE5;
    static constexpr uint32_t opF32x4Mul = 0xE6;
    static constexpr uint32_t opF32x4Div = 0xE7;
    static constexpr uint32_t opF32x4Min = 0xE8;
    static constexpr uint32_t opF32x4Max = 0xE9;
#endif

    //==============================================================================
    // Section ids

    static constexpr uint8_t sectionType = 1;
    static constexpr uint8_t sectionImport = 2;
    static constexpr uint8_t sectionFunction = 3;
    static constexpr uint8_t sectionExport = 7;
    static constexpr uint8_t sectionCode = 10;

    //==============================================================================
    // Import/export kinds

    static constexpr uint8_t importKindFunc = 0x00;
    static constexpr uint8_t importKindMemory = 0x02;
    static constexpr uint8_t exportKindFunc = 0x00;

    // Empty block type (0x40) for control constructs without a result.
    static constexpr uint8_t emptyBlockType = 0x40;

private:
    // The buffer the primitive writers currently append to: the current
    // function body while emitting instructions, the buffered section payload
    // while a section is open, and the final module bytes otherwise.
    std::vector<uint8_t>& target() noexcept
    {
        if (inBody)
            return body;

        if (inSection)
            return sectionPayload;

        return out;
    }

    std::vector<uint8_t> out;
    std::vector<uint8_t> sectionPayload;
    std::vector<uint8_t> body; // current function body (locals + instructions)
    std::vector<std::pair<uint32_t, ValType>> localRuns;

    uint8_t currentSectionId = 0;
    uint32_t typeCount = 0;
    uint32_t importCount = 0;
    uint32_t funcImportCount = 0; // function imports only (function index space)
    uint32_t funcCount = 0;
    uint32_t exportCount = 0;
    bool inSection = false;
    bool inBody = false;
};

} // namespace yup
