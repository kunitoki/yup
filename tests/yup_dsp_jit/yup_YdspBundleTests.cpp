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

namespace
{

const char* simplePatch = R"YDSP(
    processor Passthrough
    {
        input stream in;
        output stream out;
        process { out = in; }
    }

    graph Main
    {
        input stream in;
        output stream out;
        node p = Passthrough;
        connection
        {
            in -> p.in;
            p.out -> out;
        }
    }
)YDSP";

} // namespace

class YdspBundleArtifactTests : public ::testing::Test
{
public:
    static YdspBundleCompileOptions allTargets()
    {
        YdspBundleCompileOptions options;

        for (const auto os : { YdspTargetOperatingSystem::macosTarget, YdspTargetOperatingSystem::linuxTarget, YdspTargetOperatingSystem::windowsTarget })
            for (const auto arch : { YdspTargetArchitecture::arm64, YdspTargetArchitecture::x64 })
                options.nativeTargets.push_back ({ os, arch });

        return options;
    }

    static void expectGain (YdspAudioGraph& graph, float gain)
    {
        graph.prepare (48000.0, 8);

        const std::array<float, 8> input { 0.25f, -0.25f, 0.5f, -0.5f, 1.0f, -1.0f, 0.0f, 0.125f };
        std::array<float, 8> output {};
        const YdspInputBuffer inputs[] { Span<const float> (input.data(), input.size()) };
        YdspOutputBuffer outputs[] { Span<float> (output.data(), output.size()) };

        ASSERT_EQ (YdspProcessResult::ok, graph.process (yup::YdspProcessRequest { inputs, outputs, 8 }));

        for (size_t i = 0; i < output.size(); ++i)
            EXPECT_FLOAT_EQ (input[i] * gain, output[i]);
    }
};

TEST (YdspBundleTests, DiagnosticRangesAndExcerptsSurviveRoundTrip)
{
    YdspCompiler compiler;
    auto compiled = compiler.compileBundle (String ("declare unknown_key \"value\";\n") + simplePatch,
                                            YdspBundleArtifactTests::allTargets());
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();
    ASSERT_GT (compiled.getReference().getDiagnostics().getCount(), 0);

    MemoryBlock bytes;
    ASSERT_TRUE (compiled.getReference().saveToMemoryBlock (bytes).wasOk());
    auto loaded = YdspBundle::loadFromMemoryBlock (bytes);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    const auto& diagnostics = loaded.getReference().getDiagnostics();
    EXPECT_EQ (compiled.getReference().getDiagnostics().toString(), diagnostics.toString());
    EXPECT_EQ ("source-0", diagnostics.getItem (0).range.sourceId);
    EXPECT_TRUE (diagnostics.toString().contains ("source-0:1:9: warning:"));
    EXPECT_TRUE (diagnostics.toString().contains ("^~~~~~~~~~~"));
}

TEST (YdspBundleTests, CompileAndLoadMemoryRoundTrip)
{
    YdspCompiler compiler;
    const auto options = YdspBundleArtifactTests::allTargets();

    auto compiled = compiler.compileBundle (simplePatch, options);
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();

    MemoryBlock bytes;
    ASSERT_TRUE (compiled.getReference().saveToMemoryBlock (bytes).wasOk());

    auto loaded = YdspBundle::loadFromMemoryBlock (bytes);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    EXPECT_EQ (1u, loaded.getReference().getSources().size());
    EXPECT_EQ (options.nativeTargets.size(), static_cast<size_t> (loaded.getReference().getNativeTargets().size()));

    auto graph = loaded.getReference().instantiate();
    ASSERT_TRUE (graph.wasOk()) << graph.getErrorMessage();
    EXPECT_TRUE (graph.getReference().isValid());
    YdspBundleArtifactTests::expectGain (graph.getReference(), 1.0f);
}

TEST (YdspBundleTests, FileRoundTripPreservesBundle)
{
    YdspCompiler compiler;
    auto compiled = compiler.compileBundle (simplePatch, {});
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();

    const auto file = File::createTempFile ("ydsp-bundle.ydsb");
    ASSERT_TRUE (compiled.getReference().saveToFile (file).wasOk());

    auto loaded = YdspBundle::loadFromFile (file);
    EXPECT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    if (loaded.wasOk())
        EXPECT_EQ (compiled.getReference().getSources()[0].source,
                   loaded.getReference().getSources()[0].source);

    file.deleteFile();
}

TEST (YdspBundleTests, RejectsCorruptHeader)
{
    YdspCompiler compiler;
    auto compiled = compiler.compileBundle (simplePatch, {});
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();

    MemoryBlock bytes;
    ASSERT_TRUE (compiled.getReference().saveToMemoryBlock (bytes).wasOk());
    static_cast<uint8_t*> (bytes.getData())[0] ^= 0xff;

    EXPECT_TRUE (YdspBundle::loadFromMemoryBlock (bytes).failed());
}

TEST (YdspBundleTests, RejectsOldLanguageVersionWithParameterMigrationDiagnostic)
{
    YdspCompiler compiler;
    auto compiled = compiler.compileBundle (simplePatch, {});
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();

    MemoryBlock bytes;
    ASSERT_TRUE (compiled.getReference().saveToMemoryBlock (bytes).wasOk());

    // RIFF header, VERS chunk, then the META header and language version.
    ASSERT_GE (bytes.getSize(), 36u);
    auto* data = static_cast<uint8_t*> (bytes.getData());
    ASSERT_EQ ('M', data[24]);
    ASSERT_EQ ('E', data[25]);
    ASSERT_EQ ('T', data[26]);
    ASSERT_EQ ('A', data[27]);
    ASSERT_EQ (4, data[32]);
    data[32] = 2;

    const auto loaded = YdspBundle::loadFromMemoryBlock (bytes);
    ASSERT_TRUE (loaded.failed());
    EXPECT_TRUE (loaded.getErrorMessage().contains ("'input value' has been replaced by 'input parameter'"));
}

TEST (YdspBundleTests, StoresImportedSourceClosure)
{
    const auto directory = File::getSpecialLocation (File::tempDirectory).getChildFile ("yup_ydsp_bundle_import_test");
    directory.deleteRecursively();
    ASSERT_TRUE (directory.getChildFile ("fx").createDirectory());

    directory.getChildFile ("fx/Gain.ydsp").replaceWithText ("processor Gain { input stream in; output stream out; process { out = in; } }\n");

    const auto source = R"YDSP(
        import fx.Gain as fx;
        graph Main {
            input stream in;
            output stream out;
            node gain = fx.Gain;
            connection { in -> gain.in; gain.out -> out; }
        }
    )YDSP";

    YdspCompiler compiler;
    auto compiled = compiler.compileBundle (source, YdspBundleArtifactTests::allTargets(), directory.getChildFile ("Main.ydsp").getFullPathName());
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();
    ASSERT_EQ (2u, compiled.getReference().getSources().size());
    EXPECT_EQ ("source-0", compiled.getReference().getSources()[0].id);
    EXPECT_TRUE (compiled.getReference().getSources()[0].isRoot);
    EXPECT_EQ ("source-1", compiled.getReference().getSources()[1].id);
    EXPECT_FALSE (compiled.getReference().getSources()[1].isRoot);

    MemoryBlock bytes;
    ASSERT_TRUE (compiled.getReference().saveToMemoryBlock (bytes).wasOk());
    auto loaded = YdspBundle::loadFromMemoryBlock (bytes);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    EXPECT_EQ (2u, loaded.getReference().getSources().size());
    directory.deleteRecursively();

    // The packaged import must remain usable after its source file is gone.
    auto graph = loaded.getReference().instantiate();
    ASSERT_TRUE (graph.wasOk()) << graph.getErrorMessage();
    YdspBundleArtifactTests::expectGain (graph.getReference(), 1.0f);
}

TEST_F (YdspBundleArtifactTests, EmitsBothArchitecturesAndPortableWasm)
{
    YdspCompiler compiler;

    auto compiled = compiler.compileBundle (simplePatch, allTargets());
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    const auto& bundle = compiled.getReference();
    ASSERT_EQ (6u, bundle.getNativeArtifacts().size());

    for (const auto& target : bundle.getNativeArtifacts())
    {
        ASSERT_FALSE (target.kernels.empty());
        EXPECT_FALSE (target.kernels.front().code.empty());
    }

    EXPECT_NE (bundle.getNativeArtifacts()[0].kernels.front().code,
               bundle.getNativeArtifacts()[1].kernels.front().code);

    ASSERT_FALSE (bundle.getWasmModules().empty());
    const auto& wasm = bundle.getWasmModules().front();
    ASSERT_GE (wasm.size(), 8u);
    EXPECT_EQ (0, wasm[0]);
    EXPECT_EQ ('a', wasm[1]);
    EXPECT_EQ ('s', wasm[2]);
    EXPECT_EQ ('m', wasm[3]);
}

TEST_F (YdspBundleArtifactTests, EmitsShortCircuitBranchesForEveryTarget)
{
    const auto source = R"YDSP(
        processor P {
            input stream in; output stream out;
            state float gain;
            func setGain (value: float): float { gain = value; return value; }
            init { let value = true ? setGain (1.0) : setGain (100.0); }
            process {
                let positive = in > 0.0;
                let a = positive && (setGain (2.0) > 0.0);
                let b = positive || (setGain (3.0) > 0.0);
                out = (a ? in : (b ? in : 0.0));
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";
    YdspCompiler compiler;
    auto compiled = compiler.compileBundle (source, allTargets());
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();
    EXPECT_EQ (6u, compiled.getReference().getNativeArtifacts().size());
    EXPECT_EQ (2u, compiled.getReference().getWasmModules().size());
}

TEST_F (YdspBundleArtifactTests, PreservesExactIntegerLiteralsThroughSerialization)
{
    const auto source = R"YDSP(
        processor P {
            input parameter int64 initial = 9223372036854775807;
            output parameter int64 maximum, minimum, exact;
            output stream out;
            state int64 saved = 9007199254740993;
            func lowest(): int64 { return -9223372036854775808; }
            process { maximum = initial; minimum = lowest(); exact = saved; out = 0.0; }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP";
    YdspCompiler compiler;
    auto compiled = compiler.compileBundle (source, allTargets());
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();
    EXPECT_EQ (6u, compiled.getReference().getNativeArtifacts().size());
    EXPECT_EQ (2u, compiled.getReference().getWasmModules().size());

    MemoryBlock bytes;
    ASSERT_TRUE (compiled.getReference().saveToMemoryBlock (bytes).wasOk());
    auto loaded = YdspBundle::loadFromMemoryBlock (bytes);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    auto instance = loaded.getReference().instantiate();
    ASSERT_TRUE (instance.wasOk()) << instance.getErrorMessage();
    auto& graph = instance.getReference();
    ASSERT_TRUE (graph.prepare (48000.0, 1).wasOk());
    float output = 0.0f;
    YdspOutputBuffer outputs[] { Span<float> (&output, 1) };
    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 1 }));
    EXPECT_EQ (std::numeric_limits<int64_t>::max(), graph.getIntOutputValue ("p.maximum"));
    EXPECT_EQ (std::numeric_limits<int64_t>::min(), graph.getIntOutputValue ("p.minimum"));
    EXPECT_EQ (9007199254740993LL, graph.getIntOutputValue ("p.exact"));
}

TEST_F (YdspBundleArtifactTests, EmitsSaturatingFloatConversionsForEveryTarget)
{
    const auto source = R"YDSP(
        processor P {
            input parameter float32 a = 1.0e20;
            input parameter float64 b = 1.0e20;
            output parameter int64 a32, a64, b32, b64;
            output stream out;
            process {
                a32 = int64 (int32 (a)); a64 = int64 (a);
                b32 = int64 (int32 (b)); b64 = int64 (b);
                out = 0.0;
            }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP";
    YdspCompiler compiler;
    auto compiled = compiler.compileBundle (source, allTargets());
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();
    EXPECT_EQ (6u, compiled.getReference().getNativeArtifacts().size());
    EXPECT_EQ (1u, compiled.getReference().getWasmModules().size());
    MemoryBlock bytes;
    ASSERT_TRUE (compiled.getReference().saveToMemoryBlock (bytes).wasOk());
    auto loaded = YdspBundle::loadFromMemoryBlock (bytes);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    auto instance = loaded.getReference().instantiate();
    ASSERT_TRUE (instance.wasOk()) << instance.getErrorMessage();
    auto& graph = instance.getReference();
    ASSERT_TRUE (graph.prepare (48000.0, 1).wasOk());
    float output = 0.0f;
    YdspOutputBuffer outputs[] { Span<float> (&output, 1) };
    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 1 }));
    for (const auto* meter : { "p.a32", "p.b32" })
        EXPECT_EQ (std::numeric_limits<int32_t>::max(), graph.getIntOutputValue (meter));
    for (const auto* meter : { "p.a64", "p.b64" })
        EXPECT_EQ (std::numeric_limits<int64_t>::max(), graph.getIntOutputValue (meter));
}

TEST_F (YdspBundleArtifactTests, EmitsSaturatingIntegerOperationsForEveryTarget)
{
    const auto source = R"YDSP(
        processor P {
            input parameter int32 a = -2147483648, b = -1;
            input parameter int64 c = -9223372036854775808, d = -1;
            output parameter int64 neg32, abs32, div32, rem32;
            output parameter int64 neg64, abs64, div64, rem64;
            output parameter int64 sum32, sub32, mul32, sum64, sub64, mul64;
            output stream out;
            process {
                neg32 = int64 (-a); abs32 = int64 (abs (a)); div32 = int64 (a / b); rem32 = int64 (a % b);
                neg64 = -c; abs64 = abs (c); div64 = c / d; rem64 = c % d;
                sum32 = int64 (a + b); sub32 = int64 (b - a); mul32 = int64 (a * b);
                sum64 = c + d; sub64 = d - c; mul64 = c * d;
                out = 0.0;
            }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP";
    YdspCompiler compiler;
    auto compiled = compiler.compileBundle (source, allTargets());
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();
    EXPECT_EQ (6u, compiled.getReference().getNativeArtifacts().size());
    EXPECT_EQ (1u, compiled.getReference().getWasmModules().size());
    MemoryBlock bytes;
    ASSERT_TRUE (compiled.getReference().saveToMemoryBlock (bytes).wasOk());
    auto loaded = YdspBundle::loadFromMemoryBlock (bytes);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    auto instance = loaded.getReference().instantiate();
    ASSERT_TRUE (instance.wasOk()) << instance.getErrorMessage();
    auto& graph = instance.getReference();
    ASSERT_TRUE (graph.prepare (48000.0, 1).wasOk());
    float output = 0.0f;
    YdspOutputBuffer outputs[] { Span<float> (&output, 1) };
    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 1 }));
    for (const auto* meter : { "p.neg32", "p.abs32", "p.div32", "p.sub32", "p.mul32" })
        EXPECT_EQ (std::numeric_limits<int32_t>::max(), graph.getIntOutputValue (meter));
    for (const auto* meter : { "p.neg64", "p.abs64", "p.div64", "p.sub64", "p.mul64" })
        EXPECT_EQ (std::numeric_limits<int64_t>::max(), graph.getIntOutputValue (meter));
    EXPECT_EQ (0, graph.getIntOutputValue ("p.rem32"));
    EXPECT_EQ (0, graph.getIntOutputValue ("p.rem64"));
    EXPECT_EQ (std::numeric_limits<int32_t>::min(), graph.getIntOutputValue ("p.sum32"));
    EXPECT_EQ (std::numeric_limits<int64_t>::min(), graph.getIntOutputValue ("p.sum64"));
}

TEST_F (YdspBundleArtifactTests, ResolvesLibmAndInitKernelsAfterSerialization)
{
    const auto source = R"YDSP(
        processor Gain {
            input stream in;
            output stream out;
            state float gain;
            init { gain = cos (0.0) + 1.0; }
            process { out = in * gain + sin (in); }
        }
        graph Main {
            input stream in;
            output stream out;
            node gain = Gain;
            connection { in -> gain.in; gain.out -> out; }
        }
    )YDSP";

    YdspCompiler compiler;

    auto compiled = compiler.compileBundle (source, allTargets());
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    for (const auto& target : compiled.getReference().getNativeArtifacts())
    {
        ASSERT_EQ (2u, target.kernels.size());
        ASSERT_FALSE (target.kernels[0].symbols.empty());
        for (const auto& symbol : target.kernels[0].symbols)
        {
            ASSERT_LE (symbol.offset + 8, target.kernels[0].code.size());
            for (size_t i = 0; i < 8; ++i)
                EXPECT_EQ (0, target.kernels[0].code[static_cast<size_t> (symbol.offset) + i]);
        }
    }

    MemoryBlock bytes;
    ASSERT_TRUE (compiled.getReference().saveToMemoryBlock (bytes).wasOk());
    auto loaded = YdspBundle::loadFromMemoryBlock (bytes);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    auto graph = loaded.getReference().instantiate();
    ASSERT_TRUE (graph.wasOk()) << graph.getErrorMessage();
    graph.getReference().prepare (48000.0, 1);

    const float input = 0.25f;
    float output = 0.0f;
    const YdspInputBuffer inputs[] { Span<const float> (&input, 1) };
    YdspOutputBuffer outputs[] { Span<float> (&output, 1) };

    ASSERT_EQ (YdspProcessResult::ok, graph.getReference().process (yup::YdspProcessRequest { inputs, outputs, 1 }));
    EXPECT_NEAR (0.5f + std::sin (input), output, 1.0e-6f);
}

#if ! YUP_WASM
TEST_F (YdspBundleArtifactTests, ForeignOnlyBundleCannotExecute)
{
    auto options = allTargets();
    options.includeWasm = false;
    for (const auto target : options.nativeTargets)
    {
        YdspBundleCompileOptions single;
        single.includeWasm = false;
        single.nativeTargets = { target };

        YdspCompiler compiler;
        auto compiled = compiler.compileBundle (simplePatch, single);
        ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

#if YUP_WINDOWS
        const auto expectedOs = YdspTargetOperatingSystem::windowsTarget;
#elif YUP_MAC
        const auto expectedOs = YdspTargetOperatingSystem::macosTarget;
#else
        const auto expectedOs = YdspTargetOperatingSystem::linuxTarget;
#endif

        const auto expectedArch = ASMJIT_ARCH_ARM == 64 ? YdspTargetArchitecture::arm64 : YdspTargetArchitecture::x64;

        auto graph = compiled.getReference().instantiate();
        EXPECT_EQ (target.operatingSystem == expectedOs && target.architecture == expectedArch, graph.wasOk());
    }
}
#endif

TEST_F (YdspBundleArtifactTests, RejectsEmptyTargetSelection)
{
    YdspBundleCompileOptions options;
    options.includeWasm = false;

    YdspCompiler compiler;
    EXPECT_TRUE (compiler.compileBundle (simplePatch, options).failed());
}

TEST_F (YdspBundleArtifactTests, LoadsEventHandlersAndRebindsEmissionHelper)
{
    const auto source = R"YDSP(
        processor Source {
            input stream trig;
            output event notes;
            process { if (trig > 0.5) { emit noteOn (pitch: 72, velocity: 0.5) -> notes; } }
        }
        processor Voice {
            input event midi;
            output stream out;
            state float sounding;
            event midi (e: noteOn) { sounding = e.pitch + e.velocity; }
            process { out = sounding; }
        }
        graph Main {
            input stream trig;
            output stream y;
            node source = Source;
            node voice = Voice;
            connection { trig -> source.trig; source.notes -> voice.midi; voice.out -> y; }
        }
    )YDSP";

    YdspCompiler compiler;
    auto compiled = compiler.compileBundle (source, allTargets());
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    MemoryBlock bytes;
    ASSERT_TRUE (compiled.getReference().saveToMemoryBlock (bytes).wasOk());
    auto loaded = YdspBundle::loadFromMemoryBlock (bytes);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    auto graph = loaded.getReference().instantiate();
    ASSERT_TRUE (graph.wasOk()) << graph.getErrorMessage();
    graph.getReference().prepare (48000.0, 4);

    const std::array<float, 4> input { 0.0f, 0.0f, 1.0f, 0.0f };
    std::array<float, 4> output {};
    const YdspInputBuffer inputs[] { Span<const float> (input.data(), input.size()) };
    YdspOutputBuffer outputs[] { Span<float> (output.data(), output.size()) };

    ASSERT_EQ (YdspProcessResult::ok, graph.getReference().process (yup::YdspProcessRequest { inputs, outputs, 4 }));
    EXPECT_FLOAT_EQ (0.0f, output[0]);
    EXPECT_FLOAT_EQ (0.0f, output[1]);
    EXPECT_FLOAT_EQ (72.5f, output[2]);
    EXPECT_FLOAT_EQ (72.5f, output[3]);
}

TEST_F (YdspBundleArtifactTests, EmitsDuplicateTargetsOnce)
{
    auto options = allTargets();
    options.nativeTargets.push_back (options.nativeTargets.front());

    YdspCompiler compiler;

    auto compiled = compiler.compileBundle (simplePatch, options);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();
    EXPECT_EQ (6u, compiled.getReference().getNativeArtifacts().size());
}

TEST_F (YdspBundleArtifactTests, RejectsInvalidTargetArchitecture)
{
    auto options = allTargets();
    options.nativeTargets.front().architecture = static_cast<YdspTargetArchitecture> (99);

    YdspCompiler compiler;
    EXPECT_TRUE (compiler.compileBundle (simplePatch, options).failed());
}

TEST_F (YdspBundleArtifactTests, RejectsIncompatibleCodegenRevision)
{
    YdspCompiler compiler;
    auto compiled = compiler.compileBundle (simplePatch, allTargets());
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    MemoryBlock bytes;
    ASSERT_TRUE (compiled.getReference().saveToMemoryBlock (bytes).wasOk());
    ASSERT_GT (bytes.getSize(), 44u);
    static_cast<uint8_t*> (bytes.getData())[44] = 0xff;
    EXPECT_TRUE (YdspBundle::loadFromMemoryBlock (bytes).failed());
}

TEST_F (YdspBundleArtifactTests, TraceRoundTripPreservesSitesAndAllTargetHelpers)
{
    YdspCompiler compiler;
    auto options = allTargets();
    options.enableTracing = true;
    auto compiled = compiler.compileBundle (R"YDSP(
        processor P {
            output stream out;
            init { trace("init"); }
            process {
                int64 value = -9007199254740993;
                float f = 0.5; bool b = true;
                trace("value={value}, f={f}, b={b}");
                out = 0.0;
            }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP", options);
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();
    MemoryBlock bytes;
    ASSERT_TRUE (compiled.getReference().saveToMemoryBlock (bytes).wasOk());
    auto loaded = YdspBundle::loadFromMemoryBlock (bytes);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    auto instance = loaded.getReference().instantiate();
    ASSERT_TRUE (instance.wasOk()) << instance.getErrorMessage();
    auto& graph = instance.getReference();
    ASSERT_TRUE (graph.prepare (48000.0, 1).wasOk());
    float sample = 0.0f;
    YdspOutputBuffer outputs[] { Span<float> (&sample, 1) };
    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 1 }));
    const auto messages = graph.drainTraceMessages();
    ASSERT_EQ (2, messages.size());
    EXPECT_EQ (String ("init"), messages[0]);
    EXPECT_EQ (String ("value=-9007199254740993, f=") + String (0.5) + ", b=1", messages[1]);
}

TEST_F (YdspBundleArtifactTests, MatchRoundTripPreservesArmSelection)
{
    YdspCompiler compiler;
    auto compiled = compiler.compileBundle (R"YDSP(
        processor P {
            output stream out; state int counter;
            process {
                match (counter) { 0 => { out = 0.25; }, _ => { out = 0.75; } }
                counter = counter + 1;
            }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP", allTargets());
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();
    MemoryBlock bytes;
    ASSERT_TRUE (compiled.getReference().saveToMemoryBlock (bytes).wasOk());
    auto loaded = YdspBundle::loadFromMemoryBlock (bytes);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    auto instance = loaded.getReference().instantiate();
    ASSERT_TRUE (instance.wasOk()) << instance.getErrorMessage();
    auto& graph = instance.getReference();
    ASSERT_TRUE (graph.prepare (48000.0, 2).wasOk());
    float samples[2] {};
    YdspOutputBuffer outputs[] { Span<float> (samples, 2) };
    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 2 }));
    EXPECT_EQ (0.25f, samples[0]);
    EXPECT_EQ (0.75f, samples[1]);
}
