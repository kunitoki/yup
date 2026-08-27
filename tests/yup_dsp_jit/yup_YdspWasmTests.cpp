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

//==============================================================================
// Runs the full YDSP pipeline (lexer -> parser -> analyzer -> optimizer) and
// emits the wasm module for the named kernel.

std::vector<uint8_t> compileWasm (StringRef source, const char* kernelName, DspJitDiagnostics& diagnostics)
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
    DspJitDiagnostics diagnostics;
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
    DspJitDiagnostics diagnostics;
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
    DspJitDiagnostics diagnostics;
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
    DspJitDiagnostics diagnostics;
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
    DspJitDiagnostics diagnostics;
    auto bytes = compileWasm (sinF32Source, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();

    const auto text = YdspWasmCodegen::toText (bytes);
    EXPECT_TRUE (text.contains ("(import \"env\" \"sinf\""));
    EXPECT_TRUE (text.contains ("call 0")); // sinf is function import 0
}

TEST (YdspWasmTests, CompilesIfElseIfChainInsideInlinedFunction)
{
    DspJitDiagnostics diagnostics;
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
    DspJitDiagnostics diagnostics;
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
    DspJitDiagnostics diagnostics;
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
    DspJitDiagnostics diagnostics;
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
    DspJitDiagnostics diagnostics;
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
    DspJitDiagnostics diagnostics;
    auto bytes = compileWasm (intDivSource, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    const auto sections = parseSections (bytes);
    const auto code = collectCodeInstructions (sections[4]);

    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opI32Eqz, YdspWasmEmitter::opIf, 0x7F }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opI32DivS }));
}

TEST (YdspWasmTests, FloatModuloUsesTruncOpcode)
{
    DspJitDiagnostics diagnostics;
    auto bytes = compileWasm (modSource, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    const auto sections = parseSections (bytes);
    const auto code = collectCodeInstructions (sections[4]);

    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opF32Trunc }));
}

TEST (YdspWasmTests, StateAccessLoadsTheStatePointerFromCtx)
{
    DspJitDiagnostics diagnostics;
    auto bytes = compileWasm (stateSource, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    const auto sections = parseSections (bytes);
    const auto code = collectCodeInstructions (sections[4]);

    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opI32Load, 0x02, 0x10 }));
}

TEST (YdspWasmTests, IfElseEmitsBlockAndIfElse)
{
    DspJitDiagnostics diagnostics;
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
    DspJitDiagnostics diagnostics;
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
    DspJitDiagnostics diagnostics;
    auto bytes = compileWasm (sinF64Source, "P", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    const auto sections = parseSections (bytes);

    const auto& payload = sections[4].payload;
    EXPECT_NE (payload.end(), std::find (payload.begin(), payload.end(), 0x7C));
}
