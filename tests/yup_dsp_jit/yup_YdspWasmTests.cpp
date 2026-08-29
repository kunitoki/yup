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

#include <algorithm>
#include <string>
#include <vector>

using namespace yup;

namespace
{

//==============================================================================
// Minimal wasm binary parsing helpers (MVP subset).

struct WasmSection
{
    uint8_t id = 0;
    std::vector<uint8_t> payload;
};

std::vector<WasmSection> parseSections (const std::vector<uint8_t>& bytes)
{
    std::vector<WasmSection> sections;
    size_t pos = 8; // skip magic + version

    while (pos < bytes.size())
    {
        WasmSection section;
        section.id = bytes[pos++];

        uint32_t size = 0;
        uint32_t shift = 0;

        while (pos < bytes.size())
        {
            const auto byte = bytes[pos++];
            size |= static_cast<uint32_t> (byte & 0x7F) << shift;
            shift += 7;

            if ((byte & 0x80) == 0)
                break;
        }

        if (size > bytes.size() - pos)
            break;

        section.payload.assign (bytes.begin() + static_cast<ptrdiff_t> (pos),
                                bytes.begin() + static_cast<ptrdiff_t> (pos + size));
        pos += size;
        sections.push_back (std::move (section));
    }

    return sections;
}

uint32_t readLebU (const std::vector<uint8_t>& data, size_t& pos)
{
    uint32_t value = 0;
    uint32_t shift = 0;

    while (pos < data.size())
    {
        const auto byte = data[pos++];
        value |= static_cast<uint32_t> (byte & 0x7F) << shift;
        shift += 7;

        if ((byte & 0x80) == 0)
            break;
    }

    return value;
}

std::string readName (const std::vector<uint8_t>& data, size_t& pos)
{
    const auto length = readLebU (data, pos);

    std::string name;
    name.reserve (length);

    for (uint32_t i = 0; i < length && pos < data.size(); ++i)
        name.push_back (static_cast<char> (data[pos++]));

    return name;
}

struct WasmImport
{
    std::string moduleName;
    std::string name;
    uint8_t kind = 0;
    uint32_t typeIndex = 0;
    bool isMemory = false;
    uint32_t memoryMin = 0;
    uint32_t memoryMax = 0;
    bool memoryShared = false;
};

std::vector<WasmImport> parseImports (const WasmSection& section)
{
    std::vector<WasmImport> imports;
    size_t pos = 0;

    const auto count = readLebU (section.payload, pos);

    for (uint32_t i = 0; i < count; ++i)
    {
        WasmImport import;
        import.moduleName = readName (section.payload, pos);
        import.name = readName (section.payload, pos);
        import.kind = section.payload[pos++];

        if (import.kind == YdspWasmEmitter::importKindFunc)
        {
            import.typeIndex = readLebU (section.payload, pos);
        }
        else if (import.kind == YdspWasmEmitter::importKindMemory)
        {
            import.isMemory = true;
            const auto limitsFlag = section.payload[pos++];
            import.memoryShared = (limitsFlag & 0x02) != 0;
            import.memoryMin = readLebU (section.payload, pos);

            if ((limitsFlag & 0x01) != 0)
                import.memoryMax = readLebU (section.payload, pos);
        }

        imports.push_back (std::move (import));
    }

    return imports;
}

struct WasmFuncType
{
    std::vector<uint8_t> params;
    std::vector<uint8_t> results;
};

std::vector<WasmFuncType> parseFuncTypes (const WasmSection& section)
{
    std::vector<WasmFuncType> types;
    size_t pos = 0;

    const auto count = readLebU (section.payload, pos);

    for (uint32_t i = 0; i < count; ++i)
    {
        WasmFuncType type;
        const auto form = section.payload[pos++];
        (void) form; // 0x60

        const auto numParams = readLebU (section.payload, pos);
        type.params.assign (section.payload.begin() + static_cast<ptrdiff_t> (pos),
                            section.payload.begin() + static_cast<ptrdiff_t> (pos + numParams));
        pos += numParams;

        const auto numResults = readLebU (section.payload, pos);
        type.results.assign (section.payload.begin() + static_cast<ptrdiff_t> (pos),
                             section.payload.begin() + static_cast<ptrdiff_t> (pos + numResults));
        pos += numResults;

        types.push_back (std::move (type));
    }

    return types;
}

std::vector<uint8_t> collectCodeInstructions (const WasmSection& section)
{
    std::vector<uint8_t> instructions;
    size_t pos = 0;

    const auto count = readLebU (section.payload, pos);

    for (uint32_t i = 0; i < count; ++i)
    {
        const auto bodySize = readLebU (section.payload, pos);
        const auto bodyStart = pos;

        const auto numGroups = readLebU (section.payload, pos);

        for (uint32_t g = 0; g < numGroups; ++g)
        {
            (void) readLebU (section.payload, pos); // run length
            ++pos;                                  // valtype byte
        }

        instructions.insert (instructions.end(),
                             section.payload.begin() + static_cast<ptrdiff_t> (pos),
                             section.payload.begin() + static_cast<ptrdiff_t> (bodyStart + bodySize - 1));

        pos = bodyStart + bodySize;
    }

    return instructions;
}

bool containsSubsequence (const std::vector<uint8_t>& haystack, const std::vector<uint8_t>& needle)
{
    if (needle.empty() || needle.size() > haystack.size())
        return false;

    for (size_t i = 0; i + needle.size() <= haystack.size(); ++i)
    {
        bool match = true;

        for (size_t j = 0; j < needle.size(); ++j)
            if (haystack[i + j] != needle[j])
            {
                match = false;
                break;
            }

        if (match)
            return true;
    }

    return false;
}

#if defined (__wasm_simd128__)
// A minimal stack-balance validator over the emitted code section. Walks the
// instruction bytes, consumes each opcode's immediates and tracks the value
// stack depth, asserting it never underflows and ends balanced. This is what
// catches a missing-operand lowering (e.g. a binary op fed one value) on the
// desktop, where no wasm engine is available to reject the module at
// instantiation.
void expectBalancedStack (const std::vector<uint8_t>& code, const std::vector<WasmSection>& sections)
{
    std::vector<WasmFuncType> types;

    for (const auto& section : sections)
        if (section.id == YdspWasmEmitter::sectionType)
            types = parseFuncTypes (section);

    // Function index space: function imports first (in import order), then
    // defined functions (function-section order).
    std::vector<int> arity;

    for (const auto& section : sections)
        if (section.id == YdspWasmEmitter::sectionImport)
            for (const auto& import : parseImports (section))
                if (import.kind == YdspWasmEmitter::importKindFunc && import.typeIndex < types.size())
                    arity.push_back (static_cast<int> (types[import.typeIndex].params.size()));

    for (const auto& section : sections)
        if (section.id == YdspWasmEmitter::sectionFunction)
        {
            size_t pos = 0;
            const auto count = readLebU (section.payload, pos);

            for (uint32_t i = 0; i < count && pos < section.payload.size(); ++i)
            {
                const auto typeIndex = readLebU (section.payload, pos);
                arity.push_back (typeIndex < types.size() ? static_cast<int> (types[typeIndex].params.size()) : 0);
            }
        }

    int depth = 0;
    size_t pos = 0;

    const auto byte = [&] { return code[pos++]; };
    const auto lebU = [&] { return readLebU (code, pos); };

    const auto delta = [&] (int d)
    {
        depth += d;
        EXPECT_GE (depth, 0) << "value stack underflow at byte offset " << pos;
    };

    while (pos < code.size())
    {
        const auto op = byte();

        switch (op)
        {
            case 0x00: // unreachable
            case 0x01: // nop
            case 0x05: // else
            case 0x0B: // end
            case 0x0F: // return (void kernel)
                break;

            case 0x02: // block
            case 0x03: // loop
                (void) byte(); // blocktype
                break;

            case 0x04: // if
                (void) byte(); // blocktype
                delta (-1);    // condition
                break;

            case 0x0C: // br
                (void) lebU();
                break;

            case 0x0D: // br_if
                (void) lebU();
                delta (-1);
                break;

            case 0x10: // call
            {
                const auto index = lebU();
                delta (index < arity.size() ? -arity[index] : 0);
                break;
            }

            case 0x1A: // drop
                delta (-1);
                break;

            case 0x1B: // select (untyped)
                delta (-2);
                break;

            case 0x20: // local.get
            case 0x21: // local.set
            case 0x22: // local.tee
                (void) lebU();
                delta (op == 0x20 ? 1 : (op == 0x21 ? -1 : 0));
                break;

            case 0x41: // i32.const
            case 0x42: // i64.const
                (void) lebU();
                delta (+1);
                break;

            case 0x43: // f32.const
                pos += 4;
                delta (+1);
                break;

            case 0x44: // f64.const
                pos += 8;
                delta (+1);
                break;

            case 0x28: case 0x29: case 0x2A: case 0x2B: // loads
            case 0x2C: case 0x2D: case 0x2E: case 0x2F:
            case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35:
                (void) lebU(); // align
                (void) lebU(); // offset
                break; // pop the address, push the value: net 0

            case 0x36: case 0x37: case 0x38: case 0x39: // stores
            case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3E:
                (void) lebU();
                (void) lebU();
                delta (-2); // pop the address and the value
                break;

            case 0x45: case 0x50: // eqz
            case 0x67: case 0x68: case 0x69: // i32 clz/ctz/popcnt
            case 0x79: case 0x7A: case 0x7B: // i64 clz/ctz/popcnt
            case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F: case 0x90: case 0x91: // f32 unary
            case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9E: case 0x9F: // f64 unary
            case 0xA7: case 0xAC: case 0xB6: case 0xBB: // wrap/extend/demote/promote
            case 0xBC: case 0xBD: case 0xBE: case 0xBF: // reinterpret
            case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: // sign-extension
            case 0xA8: case 0xA9: case 0xAA: case 0xAB: // i32.trunc_f*
            case 0xAE: case 0xAF: case 0xB0: case 0xB1: // i64.trunc_f*
            case 0xB2: case 0xB3: case 0xB4: case 0xB5: // f32.convert_i*
            case 0xB7: case 0xB8: case 0xB9: case 0xBA: // f64.convert_i*
                break; // unary: pop 1 push 1

            case 0x46: case 0x47: case 0x48: case 0x49: case 0x4A: case 0x4B:
            case 0x4C: case 0x4D: case 0x4E: case 0x4F: // i32 comparisons
            case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56:
            case 0x57: case 0x58: case 0x59: case 0x5A: // i64 comparisons
            case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F: case 0x60: // f32 comparisons
            case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: // f64 comparisons
            case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F: case 0x70: // i32 arith
            case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76:
            case 0x77: case 0x78: // i32 bitwise/shifts/rotates
            case 0x7C: case 0x7D: case 0x7E: case 0x7F: case 0x80: case 0x81: case 0x82: // i64 arith
            case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: case 0x88:
            case 0x89: case 0x8A: // i64 bitwise/shifts/rotates
            case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: case 0x98: // f32 binary
            case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA6: // f64 binary
                delta (-1); // binary: pop 2 push 1
                break;

            case YdspWasmEmitter::simdPrefix:
            {
                const auto sub = lebU();

                switch (sub)
                {
                    case YdspWasmEmitter::opV128Load:
                        (void) lebU();
                        (void) lebU();
                        break; // pop the address, push the value: net 0

                    case YdspWasmEmitter::opV128Store:
                        (void) lebU();
                        (void) lebU();
                        delta (-2); // pop the address and the value
                        break;

                    case YdspWasmEmitter::opI8x16Shuffle:
                        pos += 16; // lane mask
                        delta (-1); // pop 2 push 1
                        break;

                    case YdspWasmEmitter::opF32x4ExtractLane:
                        (void) byte(); // lane index
                        break; // pop 1 push 1

                    case YdspWasmEmitter::opF32x4Splat:
                    case YdspWasmEmitter::opF32x4Ceil:
                    case YdspWasmEmitter::opF32x4Floor:
                    case YdspWasmEmitter::opF32x4Nearest:
                    case YdspWasmEmitter::opF32x4Abs:
                    case YdspWasmEmitter::opF32x4Neg:
                    case YdspWasmEmitter::opF32x4Sqrt:
                        break; // unary

                    case YdspWasmEmitter::opF32x4Add:
                    case YdspWasmEmitter::opF32x4Sub:
                    case YdspWasmEmitter::opF32x4Mul:
                    case YdspWasmEmitter::opF32x4Div:
                    case YdspWasmEmitter::opF32x4Min:
                    case YdspWasmEmitter::opF32x4Max:
                        delta (-1);
                        break;

                    default:
                        break; // subopcode the codegen never emits
                }

                break;
            }

            default:
                break; // opcode the codegen never emits
        }
    }

    EXPECT_EQ (0, depth) << "value stack not balanced at end of code section";
}
#endif

//==============================================================================
// Runs the full YDSP pipeline (lexer -> parser -> analyzer -> optimizer) and
// emits the wasm module for the named kernel.

std::vector<uint8_t> compileWasm (StringRef source, const char* kernelName, YdspDiagnostics& diagnostics, bool enableVectorization = false)
{
    YdspLexer lexer (source, diagnostics);
    auto tokens = lexer.tokenize();

    YdspParser parser (std::move (tokens), diagnostics);
    auto program = parser.parseProgram();

    if (program == nullptr || diagnostics.hasErrors())
        return {};

    YdspSemanticAnalyzer analyzer (diagnostics);
    auto analyzed = analyzer.analyze (std::move (program));

    if (analyzed == nullptr || diagnostics.hasErrors())
        return {};

    YdspOptimizer optimizer (diagnostics);

    if (enableVectorization)
    {
        optimizer.setVectorizationEnabled (true);
        optimizer.setVectorWidth (4);
    }

    auto ir = optimizer.build (*analyzed);

    if (ir == nullptr || diagnostics.hasErrors())
        return {};

    const YdspIrFunction* targetFn = nullptr;

    for (const auto& fn : ir->kernels)
    {
        if (fn->name == kernelName)
        {
            targetFn = fn.get();
            break;
        }
    }

    if (targetFn == nullptr)
        return {};

    return YdspWasmCodegen::compile (*targetFn, diagnostics);
}

//==============================================================================
// YDSP source snippets

constexpr const char* passThroughSource = R"YDSP(
    processor P { input stream in; output stream out; process { out = in; } }
    graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
)YDSP";

constexpr const char* sinF32Source = R"YDSP(
    processor P { input stream in; output stream out; process { out = sin (in); } }
    graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
)YDSP";

constexpr const char* sinF64Source = R"YDSP(
    processor P { input stream float64 in; output stream float64 out; process { out = sin (in); } }
    graph G { input stream float64 x; output stream float64 y; node p = P; connection { x -> p.in; p.out -> y; } }
)YDSP";

constexpr const char* powSource = R"YDSP(
    processor P { input stream in; output stream out; process { out = pow (in, 2.0); } }
    graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
)YDSP";

constexpr const char* emitEventSource = R"YDSP(
    processor P {
        input stream in;
        output stream out;
        output event noteOn;
        process {
            emit noteOn (pitch: in) -> noteOn;
            out = in;
        }
    }
    graph G { input stream x; output stream y; output event noteOn; node p = P; connection { x -> p.in; p.out -> y; p.noteOn -> noteOn; } }
)YDSP";

constexpr const char* intDivSource = R"YDSP(
    processor P { output stream out; process { out = float32 (blockSize / 2); } }
    graph G { output stream y; node p = P; connection { p.out -> y; } }
)YDSP";

constexpr const char* modSource = R"YDSP(
    processor P { input stream in; output stream out; process { out = in % 2.0; } }
    graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
)YDSP";

constexpr const char* stateSource = R"YDSP(
    processor P { input stream in; output stream out; state float s; process { s = in; out = s; } }
    graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
)YDSP";

constexpr const char* ifElseSource = R"YDSP(
    processor P { input stream in; output stream out; process { if (in > 0) { out = in; } else { out = 0; } } }
    graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
)YDSP";

constexpr const char* forLoopSource = R"YDSP(
    processor P { input stream in; output stream out; process block { for i in 0..blockSize { out[i] = in[i] * 2; } } }
    graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
)YDSP";

// The vectoriser widens this constant-bound bank loop to four f32 lanes; the
// wasm backend emits f32x4 only when compiled with -msimd128.
constexpr const char* wasmVectorBankSource = R"YDSP(
    let modes = 8;

    processor P {
        input stream in;
        output stream out;

        state float z[modes];

        process {
            float sum = 0.0;

            for i in 0..modes {
                z[i] = z[i] * 0.5 + in;
                sum = sum + z[i];
            }

            out = sum;
        }
    }

    graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
)YDSP";

} // namespace

//==============================================================================
// YdspWasmEmitter: raw encoding tests
//==============================================================================

TEST (WasmEmitterTests, ModuleStartsWithMagicAndVersion)
{
    YdspWasmEmitter emitter;
    emitter.beginModule();

    const auto& bytes = emitter.getBytes();
    ASSERT_EQ (8u, bytes.size());

    EXPECT_EQ (0x00, bytes[0]);
    EXPECT_EQ (0x61, bytes[1]);
    EXPECT_EQ (0x73, bytes[2]);
    EXPECT_EQ (0x6D, bytes[3]);
    EXPECT_EQ (1u, bytes[4]); // version 1
    EXPECT_EQ (0u, bytes[5]);
    EXPECT_EQ (0u, bytes[6]);
    EXPECT_EQ (0u, bytes[7]);
}

TEST (WasmEmitterTests, Leb128EncodingsAreCorrect)
{
    YdspWasmEmitter emitter;

    emitter.u32 (624485);
    EXPECT_EQ ((std::vector<uint8_t> { 0xE5, 0x8E, 0x26 }), emitter.getBytes());

    YdspWasmEmitter emitter2;
    emitter2.u32 (127);
    EXPECT_EQ ((std::vector<uint8_t> { 0x7F }), emitter2.getBytes());

    YdspWasmEmitter emitter3;
    emitter3.u32 (128);
    EXPECT_EQ ((std::vector<uint8_t> { 0x80, 0x01 }), emitter3.getBytes());

    YdspWasmEmitter emitter4;
    emitter4.s32 (-123456);
    EXPECT_EQ ((std::vector<uint8_t> { 0xC0, 0xBB, 0x78 }), emitter4.getBytes());

    YdspWasmEmitter emitter5;
    emitter5.s64 (-2);
    EXPECT_EQ ((std::vector<uint8_t> { 0x7E }), emitter5.getBytes());
}

TEST (WasmEmitterTests, SectionPayloadIsSizePrefixed)
{
    YdspWasmEmitter emitter;
    emitter.beginModule();

    emitter.beginSection (YdspWasmEmitter::sectionType);
    emitter.u32 (1);
    emitter.funcType ({ YdspWasmEmitter::ValType::i32 }, {});
    emitter.endSection();

    const auto& bytes = emitter.getBytes();
    ASSERT_GE (bytes.size(), 12u);

    EXPECT_EQ (YdspWasmEmitter::sectionType, bytes[8]); // section id
    EXPECT_EQ (0x05, bytes[9]);                         // payload size (5 bytes)
    EXPECT_EQ (0x01, bytes[10]);                        // one type entry
    EXPECT_EQ (0x60, bytes[11]);                        // functype form
}

TEST (WasmEmitterTests, MemoryImportEncodesSharedEnvMemory)
{
    YdspWasmEmitter emitter;
    emitter.beginModule();

    emitter.beginSection (YdspWasmEmitter::sectionImport);
    emitter.u32 (1);
    emitter.importMemory ("env", "memory", 1, 65536, true);
    emitter.endSection();

    const auto sections = parseSections (emitter.getBytes());
    ASSERT_EQ (1u, sections.size());
    ASSERT_EQ (YdspWasmEmitter::sectionImport, sections[0].id);

    const auto imports = parseImports (sections[0]);
    ASSERT_EQ (1u, imports.size());
    EXPECT_EQ ("env", imports[0].moduleName);
    EXPECT_EQ ("memory", imports[0].name);
    EXPECT_TRUE (imports[0].isMemory);
    EXPECT_TRUE (imports[0].memoryShared);
    EXPECT_EQ (1u, imports[0].memoryMin);
    EXPECT_EQ (65536u, imports[0].memoryMax);
}

TEST (WasmEmitterTests, MemoryImportCanBeNonSharedMinOnly)
{
    YdspWasmEmitter emitter;
    emitter.beginModule();

    emitter.beginSection (YdspWasmEmitter::sectionImport);
    emitter.u32 (1);
    emitter.importMemory ("env", "memory", 1, 0, false);
    emitter.endSection();

    const auto sections = parseSections (emitter.getBytes());
    const auto imports = parseImports (sections[0]);
    ASSERT_EQ (1u, imports.size());
    EXPECT_TRUE (imports[0].isMemory);
    EXPECT_FALSE (imports[0].memoryShared);
    EXPECT_EQ (1u, imports[0].memoryMin);
    EXPECT_EQ (0u, imports[0].memoryMax);
}

TEST (WasmEmitterTests, LocalRunsAreCompressed)
{
    YdspWasmEmitter emitter;
    emitter.beginModule();

    emitter.beginSection (YdspWasmEmitter::sectionCode);
    emitter.u32 (1);
    emitter.beginBody();
    emitter.declareLocals (3, YdspWasmEmitter::ValType::f32);
    emitter.declareLocals (2, YdspWasmEmitter::ValType::i64);
    emitter.nop();
    emitter.endBody();
    emitter.endSection();

    const auto sections = parseSections (emitter.getBytes());
    ASSERT_EQ (1u, sections.size());

    // payload = [body count=1][body size][vec(locals)=2 groups][3 x f32][2 x i64][nop][end]
    // f32 = 0x7D, i64 = 0x7E.
    EXPECT_EQ ((std::vector<uint8_t> { 0x01, 0x07, 0x02, 0x03, 0x7D, 0x02, 0x7E, 0x01, 0x0B }),
               sections[0].payload);
}

//==============================================================================
// YdspWasmCodegen: module-level structural tests
//==============================================================================

TEST (YdspWasmTests, CompilesPassThroughToAValidModule)
{
    YdspDiagnostics diagnostics;
    auto bytes = compileWasm (passThroughSource, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_FALSE (bytes.empty());

    const auto sections = parseSections (bytes);
    ASSERT_EQ (5u, sections.size());

    EXPECT_EQ (YdspWasmEmitter::sectionType, sections[0].id);
    EXPECT_EQ (YdspWasmEmitter::sectionImport, sections[1].id);
    EXPECT_EQ (YdspWasmEmitter::sectionFunction, sections[2].id);
    EXPECT_EQ (YdspWasmEmitter::sectionExport, sections[3].id);
    EXPECT_EQ (YdspWasmEmitter::sectionCode, sections[4].id);
}

TEST (YdspWasmTests, ImportsHostMemoryFirst)
{
    YdspDiagnostics diagnostics;
    auto bytes = compileWasm (passThroughSource, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    const auto sections = parseSections (bytes);

    const auto imports = parseImports (sections[1]);
    ASSERT_FALSE (imports.empty());
    EXPECT_EQ ("env", imports[0].moduleName);
    EXPECT_EQ ("memory", imports[0].name);
    EXPECT_TRUE (imports[0].isMemory);

    EXPECT_TRUE (imports[0].memoryShared);
    EXPECT_EQ (1u, imports[0].memoryMin);
    EXPECT_GE (imports[0].memoryMax, 1u);
}

TEST (YdspWasmTests, ExportsKernelFunction)
{
    YdspDiagnostics diagnostics;
    auto bytes = compileWasm (passThroughSource, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    const auto sections = parseSections (bytes);

    const auto& exportSection = sections[3];
    size_t pos = 0;
    const auto count = readLebU (exportSection.payload, pos);
    ASSERT_EQ (1u, count);

    EXPECT_EQ ("ydsp_kernel", readName (exportSection.payload, pos));
    EXPECT_EQ (YdspWasmEmitter::exportKindFunc, exportSection.payload[pos++]);
    EXPECT_EQ (0u, readLebU (exportSection.payload, pos)); // no function imports -> defined function 0
}

TEST (YdspWasmTests, ToTextRendersReadableWasmListing)
{
    YdspDiagnostics diagnostics;
    auto bytes = compileWasm (passThroughSource, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_FALSE (bytes.empty());

    const auto text = YdspWasmCodegen::toText (bytes);

    EXPECT_TRUE (text.contains ("(module"));
    EXPECT_TRUE (text.contains ("(import \"env\" \"memory\""));
    EXPECT_TRUE (text.contains ("(export \"ydsp_kernel\""));

    EXPECT_TRUE (text.contains ("local.get"));
    EXPECT_TRUE (text.contains ("i32.load"));
    EXPECT_TRUE (text.contains ("f32.store"));
}

TEST (YdspWasmTests, ToTextIncludesLibmImportsAndCalls)
{
    YdspDiagnostics diagnostics;
    auto bytes = compileWasm (sinF32Source, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();

    const auto text = YdspWasmCodegen::toText (bytes);
    EXPECT_TRUE (text.contains ("(import \"env\" \"sinf\""));
    EXPECT_TRUE (text.contains ("call 0")); // sinf is function import 0
}

TEST (YdspWasmTests, CompilesIfElseIfChainInsideInlinedFunction)
{
    YdspDiagnostics diagnostics;
    auto bytes = compileWasm (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float phase;

            func pick (t: float, dt: float) : float {
                float r = 0.0;
                if (t < dt) {
                    r = t + t - 1.0;
                } else if (t > 1.0 - dt) {
                    r = t * t + 1.0;
                }
                return r;
            }

            process {
                float dt = 0.01;
                phase = phase + dt;
                if (phase >= 1.0) { phase = phase - 1.0; }
                out = in * 2.0 - pick (phase, dt);
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                              "P",
                              diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_FALSE (bytes.empty());

    const auto sections = parseSections (bytes);
    const auto code = collectCodeInstructions (sections[4]);

    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opIf, YdspWasmEmitter::emptyBlockType }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opElse }));
}

TEST (YdspWasmTests, SinF32IsImportedFromEnv)
{
    YdspDiagnostics diagnostics;
    auto bytes = compileWasm (sinF32Source, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    const auto sections = parseSections (bytes);

    const auto imports = parseImports (sections[1]);
    ASSERT_EQ (2u, imports.size()); // memory + sinf

    EXPECT_EQ ("env", imports[1].moduleName);
    EXPECT_EQ ("sinf", imports[1].name);
    EXPECT_EQ (YdspWasmEmitter::importKindFunc, imports[1].kind);

    const auto types = parseFuncTypes (sections[0]);
    ASSERT_LT (imports[1].typeIndex, types.size());
    EXPECT_EQ ((std::vector<uint8_t> { 0x7D }), types[imports[1].typeIndex].params);
    EXPECT_EQ ((std::vector<uint8_t> { 0x7D }), types[imports[1].typeIndex].results);
}

TEST (YdspWasmTests, SinF64IsImportedAsSin)
{
    YdspDiagnostics diagnostics;
    auto bytes = compileWasm (sinF64Source, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    const auto sections = parseSections (bytes);

    const auto imports = parseImports (sections[1]);
    ASSERT_EQ (2u, imports.size());

    EXPECT_EQ ("sin", imports[1].name);

    const auto types = parseFuncTypes (sections[0]);
    EXPECT_EQ ((std::vector<uint8_t> { 0x7C }), types[imports[1].typeIndex].params);
    EXPECT_EQ ((std::vector<uint8_t> { 0x7C }), types[imports[1].typeIndex].results);
}

TEST (YdspWasmTests, PowIsABinaryFunctionImport)
{
    YdspDiagnostics diagnostics;
    auto bytes = compileWasm (powSource, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    const auto sections = parseSections (bytes);

    const auto imports = parseImports (sections[1]);
    ASSERT_EQ (2u, imports.size());

    EXPECT_EQ ("powf", imports[1].name);

    const auto types = parseFuncTypes (sections[0]);
    EXPECT_EQ ((std::vector<uint8_t> { 0x7D, 0x7D }), types[imports[1].typeIndex].params);
    EXPECT_EQ ((std::vector<uint8_t> { 0x7D }), types[imports[1].typeIndex].results);
}

TEST (YdspWasmTests, EmitEventImportsTheCommitFunctionFromEnv)
{
    YdspDiagnostics diagnostics;
    auto bytes = compileWasm (emitEventSource, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    const auto sections = parseSections (bytes);

    const auto imports = parseImports (sections[1]);
    ASSERT_EQ (2u, imports.size()); // memory + ydspCommitOutputEvent

    EXPECT_EQ ("env", imports[1].moduleName);
    EXPECT_EQ ("ydspCommitOutputEvent", imports[1].name);
    EXPECT_EQ (YdspWasmEmitter::importKindFunc, imports[1].kind);

    const auto types = parseFuncTypes (sections[0]);
    ASSERT_LT (imports[1].typeIndex, types.size());
    EXPECT_EQ ((std::vector<uint8_t> { 0x7F, 0x7F, 0x7F, 0x7F }), types[imports[1].typeIndex].params);
    EXPECT_TRUE (types[imports[1].typeIndex].results.empty());

    // No libm calls in this source, so ydspCommitOutputEvent is function index 0.
    const auto code = collectCodeInstructions (sections[4]);
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opCall, 0x00 }));
}

TEST (YdspWasmTests, IntegerDivisionGuardsZeroDivisor)
{
    YdspDiagnostics diagnostics;
    auto bytes = compileWasm (intDivSource, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    const auto sections = parseSections (bytes);
    const auto code = collectCodeInstructions (sections[4]);

    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opI32Eqz, YdspWasmEmitter::opIf, 0x7F }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opI32DivS }));
}

TEST (YdspWasmTests, FloatModuloUsesTruncOpcode)
{
    YdspDiagnostics diagnostics;
    auto bytes = compileWasm (modSource, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    const auto sections = parseSections (bytes);
    const auto code = collectCodeInstructions (sections[4]);

    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opF32Trunc }));
}

TEST (YdspWasmTests, StateAccessLoadsTheStatePointerFromCtx)
{
    YdspDiagnostics diagnostics;
    auto bytes = compileWasm (stateSource, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    const auto sections = parseSections (bytes);
    const auto code = collectCodeInstructions (sections[4]);

    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opI32Load, 0x02, 0x10 }));
}

TEST (YdspWasmTests, IfElseEmitsBlockAndIfElse)
{
    YdspDiagnostics diagnostics;
    auto bytes = compileWasm (ifElseSource, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    const auto sections = parseSections (bytes);
    const auto code = collectCodeInstructions (sections[4]);

    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opBlock, YdspWasmEmitter::emptyBlockType }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opIf, YdspWasmEmitter::emptyBlockType }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opElse }));
}

TEST (YdspWasmTests, LoopsEmitBlockLoopBrIfAndBr)
{
    YdspDiagnostics diagnostics;
    auto bytes = compileWasm (forLoopSource, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    const auto sections = parseSections (bytes);
    const auto code = collectCodeInstructions (sections[4]);

    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opBlock, YdspWasmEmitter::emptyBlockType }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opLoop, YdspWasmEmitter::emptyBlockType }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opI32Eqz, YdspWasmEmitter::opBrIf }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opBr }));
}

TEST (YdspWasmTests, DoubleKernelDeclaresF64Locals)
{
    YdspDiagnostics diagnostics;
    auto bytes = compileWasm (sinF64Source, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    const auto sections = parseSections (bytes);

    const auto& payload = sections[4].payload;
    EXPECT_NE (payload.end(), std::find (payload.begin(), payload.end(), 0x7C));
}

//==============================================================================
// SIMD lowering: the vectoriser widens the constant-bound bank loop to four
// f32 lanes. Without -msimd128 the wasm backend rejects a widened function;
// with it, the module carries f32x4 opcodes.

#if ! defined (__wasm_simd128__)
TEST (YdspWasmTests, VectorizedKernelIsRejectedWithoutSimd)
{
    YdspDiagnostics diagnostics;
    auto bytes = compileWasm (wasmVectorBankSource, "P", diagnostics, true);

    EXPECT_TRUE (bytes.empty());
    ASSERT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (diagnostics.getItem (0).message.contains ("msimd128"));
}
#endif

#if defined (__wasm_simd128__)
TEST (YdspWasmTests, VectorizedKernelEmitsF32x4Opcodes)
{
    YdspDiagnostics diagnostics;
    auto bytes = compileWasm (wasmVectorBankSource, "P", diagnostics, true);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    const auto sections = parseSections (bytes);
    const auto code = collectCodeInstructions (sections[4]);

    // The module must be stack-balanced: a missing operand would only be
    // caught by the wasm engine at instantiation, so validate it here.
    expectBalancedStack (code, sections);

    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::simdPrefix, YdspWasmEmitter::opV128Load }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::simdPrefix, YdspWasmEmitter::opV128Store }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::simdPrefix, YdspWasmEmitter::opF32x4Splat }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::simdPrefix, YdspWasmEmitter::opF32x4Mul }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::simdPrefix, YdspWasmEmitter::opF32x4Add }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::simdPrefix, YdspWasmEmitter::opI8x16Shuffle }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::simdPrefix, YdspWasmEmitter::opF32x4ExtractLane }));

    const auto text = YdspWasmCodegen::toText (bytes);

    EXPECT_TRUE (text.contains ("v128.load"));
    EXPECT_TRUE (text.contains ("v128.store"));
    EXPECT_TRUE (text.contains ("f32x4.splat"));
    EXPECT_TRUE (text.contains ("f32x4.mul"));
    EXPECT_TRUE (text.contains ("f32x4.add"));
    EXPECT_TRUE (text.contains ("i8x16.shuffle"));
    EXPECT_TRUE (text.contains ("f32x4.extract_lane"));
}

TEST (YdspWasmTests, RejectsVectorWidthsBeyondF32x4)
{
    YdspDiagnostics diagnostics;
    YdspLexer lexer (wasmVectorBankSource, diagnostics);
    auto tokens = lexer.tokenize();

    YdspParser parser (std::move (tokens), diagnostics);
    auto program = parser.parseProgram();
    ASSERT_NE (nullptr, program);

    YdspSemanticAnalyzer analyzer (diagnostics);
    auto analyzed = analyzer.analyze (std::move (program));
    ASSERT_NE (nullptr, analyzed);

    YdspOptimizer optimizer (diagnostics);
    optimizer.setVectorizationEnabled (true);
    optimizer.setVectorWidth (8); // AVX2 width; wasm SIMD is 128-bit only
    auto ir = optimizer.build (*analyzed);

    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());
    ASSERT_TRUE (ir->kernels[0]->vectorized);

    auto bytes = YdspWasmCodegen::compile (*ir->kernels[0], diagnostics);

    EXPECT_TRUE (bytes.empty());
    ASSERT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (diagnostics.getItem (0).message.contains ("width"));
}
#endif
