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

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <vector>

namespace yup::test
{

namespace
{

//==============================================================================
/** Helper: runs the full pipeline and returns the compiled kernel function. */
struct CompiledKernel
{
    YdspKernelFn fn = nullptr;
    size_t stateBytes = 0;
    size_t stateScalarBytes = 0;
    int numInputs = 0;
    int numOutputs = 0;
    int numParams = 0;
    int numParamsOut = 0;
    String asmText;
    String asmName;
};

CompiledKernel compileKernel (StringRef source, const char* kernelName, YdspDiagnostics& diagnostics, bool fastMath = false)
{
    // Lexer
    YdspLexer lexer (source, diagnostics);
    auto tokens = lexer.tokenize();

    // Parser
    YdspParser parser (std::move (tokens), diagnostics);
    auto program = parser.parseProgram();
    if (program == nullptr || diagnostics.hasErrors())
        return {};

    // Semantic analysis
    YdspSemanticAnalyzer analyzer (diagnostics);
    auto analyzed = analyzer.analyze (std::move (program));
    if (analyzed == nullptr || diagnostics.hasErrors())
        return {};

    // Optimizer (build IR)
    YdspOptimizer optimizer (diagnostics);
    optimizer.setFastMath (fastMath);
    auto ir = optimizer.build (*analyzed);
    if (ir == nullptr || diagnostics.hasErrors())
        return {};

    // Find the kernel by name
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

    // Codegen
    static asmjit::JitRuntime jitRuntime;
    auto kernelFn = YdspAsmJitCodegen::compile (jitRuntime, *targetFn, diagnostics);

    if (kernelFn == nullptr || diagnostics.hasErrors())
        return {};

    // Capture generated assembly for on-failure dump only
    String asmText;
    if (diagnostics.getCount() > 0)
        asmText = diagnostics.toString();

    return {
        kernelFn,
        YdspAsmJitCodegen::stateSize (*targetFn),
        YdspAsmJitCodegen::stateScalarSize (*targetFn),
        targetFn->numInputs,
        targetFn->numOutputs,
        targetFn->numParams,
        targetFn->numParamsOut,
        std::move (asmText),
        String (kernelName)
    };
}

/** Helper: allocates a context and runs the kernel for one block.
    Returns the output buffer (size = numOutputs * numSamples).
*/
std::vector<float> runKernel (const CompiledKernel& kernel, const std::vector<float>& input, int numSamples, const std::vector<float>& initialParams = {})
{
    EXPECT_NE (nullptr, kernel.fn);

    // Allocate state
    std::vector<uint8_t> state (kernel.stateBytes, 0);

    // Allocate params - use initial values when provided, else zero
    std::vector<float> params;
    if (initialParams.empty())
        params.assign (static_cast<size_t> (kernel.numParams), 0.0f);
    else
        params = initialParams;

    std::vector<float> paramsOut (static_cast<size_t> (kernel.numParamsOut), 0.0f);

    // Set up input pointers
    std::vector<float*> inPtrs (static_cast<size_t> (kernel.numInputs));
    size_t offset = 0;
    for (int i = 0; i < kernel.numInputs; ++i)
    {
        inPtrs[static_cast<size_t> (i)] = const_cast<float*> (input.data() + offset);
        offset += static_cast<size_t> (numSamples);
    }

    // Allocate output buffers
    std::vector<float> output (static_cast<size_t> (kernel.numOutputs * numSamples), 0.0f);
    std::vector<float*> outPtrs (static_cast<size_t> (kernel.numOutputs));
    offset = 0;
    for (int i = 0; i < kernel.numOutputs; ++i)
    {
        outPtrs[static_cast<size_t> (i)] = output.data() + offset;
        offset += static_cast<size_t> (numSamples);
    }

    YdspKernelContext ctx;
    ctx.inputs = reinterpret_cast<void* const*> (inPtrs.data());
    ctx.outputs = reinterpret_cast<void* const*> (outPtrs.data());
    ctx.params = params.data();
    ctx.paramOut = paramsOut.data();
    ctx.state = reinterpret_cast<float*> (state.data());
    ctx.stateArrays = state.empty() ? nullptr : state.data() + kernel.stateScalarBytes;
    ctx.sampleRate = 44100.0f;
    ctx.numSamples = numSamples;

    kernel.fn (&ctx);

    return output;
}

/** Helper: runs a kernel whose streams/params are double-precision (f64). */
std::vector<double> runKernel64 (const CompiledKernel& kernel, const std::vector<double>& input, int numSamples, const std::vector<double>& initialParams = {})
{
    EXPECT_NE (nullptr, kernel.fn);

    std::vector<uint8_t> state (kernel.stateBytes, 0);

    std::vector<double> params;
    if (initialParams.empty())
        params.assign (static_cast<size_t> (kernel.numParams), 0.0);
    else
        params = initialParams;

    std::vector<double> paramsOut (static_cast<size_t> (kernel.numParamsOut), 0.0);

    std::vector<double*> inPtrs (static_cast<size_t> (kernel.numInputs));
    size_t offset = 0;
    for (int i = 0; i < kernel.numInputs; ++i)
    {
        inPtrs[static_cast<size_t> (i)] = const_cast<double*> (input.data() + offset);
        offset += static_cast<size_t> (numSamples);
    }

    std::vector<double> output (static_cast<size_t> (kernel.numOutputs * numSamples), 0.0);
    std::vector<double*> outPtrs (static_cast<size_t> (kernel.numOutputs));
    offset = 0;
    for (int i = 0; i < kernel.numOutputs; ++i)
    {
        outPtrs[static_cast<size_t> (i)] = output.data() + offset;
        offset += static_cast<size_t> (numSamples);
    }

    YdspKernelContext ctx;
    ctx.inputs = reinterpret_cast<void* const*> (inPtrs.data());
    ctx.outputs = reinterpret_cast<void* const*> (outPtrs.data());
    ctx.params = params.data();
    ctx.paramOut = paramsOut.data();
    ctx.state = reinterpret_cast<double*> (state.data());
    ctx.stateArrays = state.empty() ? nullptr : state.data() + kernel.stateScalarBytes;
    ctx.sampleRate = 44100.0f;
    ctx.numSamples = numSamples;

    kernel.fn (&ctx);

    return output;
}

std::vector<float> makeRampCodegen (int size, float start = 0.0f)
{
    std::vector<float> data (static_cast<size_t> (size));

    for (int i = 0; i < size; ++i)
        data[static_cast<size_t> (i)] = start + static_cast<float> (i) * 0.01f;

    return data;
}

//==============================================================================
/** Prints the generated assembly for `kernel` if the current test has failed. */
void dumpAsmOnFailureCodegen (const CompiledKernel& kernel)
{
    if (::testing::Test::HasFailure() && ! kernel.asmText.isEmpty())
        std::cout << "\n[AsmJit] " << kernel.asmName << ":\n"
                  << kernel.asmText << std::endl;
}

} // namespace

//==============================================================================
// YdspAsmJitCodegenTests: direct codegen compilation and audio validation
//==============================================================================

TEST (YdspAsmJitCodegenTests, CompilesPassThroughAndProducesCorrectOutput)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor P { input stream in; output stream out; process { out = in; } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                                 "P",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);
    EXPECT_EQ (1, kernel.numInputs);
    EXPECT_EQ (1, kernel.numOutputs);
    EXPECT_GE (kernel.stateBytes, 0u);

    auto input = makeRampCodegen (64);
    auto output = runKernel (kernel, input, 64);

    for (int i = 0; i < 64; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)], output[static_cast<size_t> (i)], 1e-6f);

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, CompilesGainKernel)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor Gain { input stream in; output stream out; input parameter float g = 2; process { out = in * g; } }
        graph G { input stream x; output stream y; node p = Gain; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                                 "Gain",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    auto input = makeRampCodegen (64);
    auto output = runKernel (kernel, input, 64, { 2.0f });

    for (int i = 0; i < 64; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 2.0f, output[static_cast<size_t> (i)], 1e-5f);

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, CompilesTanhClipper)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor Clip { input stream in; output stream out; process { out = tanh (in); } }
        graph G { input stream x; output stream y; node c = Clip; connection { x -> c.in; c.out -> y; } }
    )YDSP",
                                 "Clip",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    auto input = makeRampCodegen (64, -2.0f);
    auto output = runKernel (kernel, input, 64);

    for (int i = 0; i < 64; ++i)
    {
        const auto x = input[static_cast<size_t> (i)];
        const auto expected = tanhf (x);
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-4f);
    }

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, ScalarTanhPreservesAccuracyAndExceptionalValues)
{
    const auto infinity = std::numeric_limits<float>::infinity();
    std::vector<float> input { 0.0f, -0.0f, infinity, -infinity,
                               std::numeric_limits<float>::quiet_NaN() };

    for (int i = -4096; i <= 4096; ++i)
        input.push_back (static_cast<float> (i) / 256.0f);

    for (int i = -32768; i <= 32768; ++i)
        input.push_back (static_cast<float> (i) / 16384.0f);

    for (int exponent = -149; exponent <= 1; ++exponent)
    {
        const auto value = std::ldexp (1.0f, exponent);
        input.push_back (value);
        input.push_back (-value);
    }

    for (const float boundary : { std::numeric_limits<float>::denorm_min(),
                                 std::numeric_limits<float>::min(),
                                 0x1p-13f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 9.0f, 10.0f,
                                 std::numeric_limits<float>::max() })
    {
        for (const float value : { std::nextafter (boundary, 0.0f), boundary,
                                  std::nextafter (boundary, infinity) })
        {
            input.push_back (value);
            input.push_back (-value);
        }
    }

    for (const bool fastMath : { false, true })
    {
        SCOPED_TRACE (fastMath);
        YdspDiagnostics diagnostics;
        const auto kernel = compileKernel (R"YDSP(
            processor Clip { input stream in; output stream out; process { out = tanh (in); } }
            graph G { input stream x; output stream y; node c = Clip; connection { x -> c.in; c.out -> y; } }
        )YDSP", "Clip", diagnostics, fastMath);
        ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
        ASSERT_NE (nullptr, kernel.fn);
        const auto output = runKernel (kernel, input, static_cast<int> (input.size()));

        for (size_t i = 0; i < input.size(); ++i)
        {
            SCOPED_TRACE (input[i]);
            if (std::isnan (input[i]))
            {
                EXPECT_TRUE (std::isnan (output[i]));
                continue;
            }

            if (input[i] == 0.0f || std::isinf (input[i]))
            {
                EXPECT_EQ (std::tanh (input[i]), output[i]);
                EXPECT_EQ (std::signbit (input[i]), std::signbit (output[i]));
                continue;
            }

            const double expected = std::tanh (static_cast<double> (input[i]));
            // Use the binade of the double reference, not a rounded float that
            // could cross a power-of-two boundary and double the tolerance.
            int exponent = 0;
            std::frexp (std::abs (expected), &exponent);
            const double ulp = std::max (std::ldexp (1.0, exponent - 24),
                                        static_cast<double> (std::numeric_limits<float>::denorm_min()));
            EXPECT_TRUE (std::isfinite (output[i]));
            EXPECT_LE (std::abs (static_cast<double> (output[i]) - expected),
                       (fastMath ? 3.5 : 1.0) * ulp);
        }
    }
}

TEST (YdspAsmJitCodegenTests, FastTanhFeedbackPreservesStateAcrossBlockPartitions)
{
    YdspDiagnostics diagnostics;
    const auto kernel = compileKernel (R"YDSP(
        processor Feedback {
            input stream in; output stream out; state float z;
            process { z = tanh (in + z * 0.5); out = z; }
        }
        graph G { input stream x; output stream y; node p = Feedback; connection { x -> p.in; p.out -> y; } }
    )YDSP", "Feedback", diagnostics, true);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    std::vector<float> input (16387), expected (input.size()), actual (input.size());
    for (size_t i = 0; i < input.size(); ++i)
        input[i] = static_cast<float> (static_cast<int> (i % 257) - 128) / 32.0f;
    std::vector<uint8_t> referenceState (kernel.stateBytes, 0), partitionedState (kernel.stateBytes, 0);
    void* inputs[] { input.data() };
    void* outputs[] { expected.data() };
    YdspKernelContext ctx;
    ctx.inputs = inputs;
    ctx.outputs = outputs;
    ctx.state = referenceState.data();
    ctx.stateArrays = referenceState.data() + kernel.stateScalarBytes;
    ctx.sampleRate = 48000.0f;
    ctx.numSamples = static_cast<int> (input.size());
    kernel.fn (&ctx);

    double reference = 0.0;
    for (size_t i = 0; i < input.size(); ++i)
    {
        reference = std::tanh (static_cast<double> (input[i]) + 0.5 * reference);
        EXPECT_NEAR (reference, expected[i], 2.0e-6) << i;
    }

    for (const bool inPlace : { false, true })
    {
        std::fill (partitionedState.begin(), partitionedState.end(), 0);
        actual = input;
        ctx.state = partitionedState.data();
        ctx.stateArrays = partitionedState.data() + kernel.stateScalarBytes;
        const int sizes[] { 1, 3, 4, 5, 31, 128, 512 };
        size_t offset = 0;
        size_t block = 0;
        while (offset < input.size())
        {
            inputs[0] = (inPlace ? actual.data() : input.data()) + offset;
            outputs[0] = actual.data() + offset;
            const auto savedState = partitionedState;
            const auto savedSample = actual[offset];
            ctx.numSamples = 0;
            kernel.fn (&ctx);
            EXPECT_EQ (savedState, partitionedState);
            EXPECT_EQ (savedSample, actual[offset]);
            ctx.numSamples = static_cast<int> (std::min (input.size() - offset,
                                                       static_cast<size_t> (sizes[block++ % 7])));
            kernel.fn (&ctx);
            offset += static_cast<size_t> (ctx.numSamples);
        }
        EXPECT_EQ (0, std::memcmp (expected.data(), actual.data(), actual.size() * sizeof (float)));
        EXPECT_EQ (referenceState, partitionedState);
    }
}

TEST (YdspAsmJitCodegenTests, CompilesTwoInputTanhClipper)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor SidechainClip {
            input stream in;
            input stream side;
            output stream out;
            process { out = tanh (in * (1 + 0.5 * side)); }
        }
        graph G {
            input stream dry;
            input stream sc;
            output stream wet;
            node c = SidechainClip;
            connection { dry -> c.in; sc -> c.side; c.out -> wet; }
        }
    )YDSP",
                                 "SidechainClip",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);
    EXPECT_EQ (2, kernel.numInputs);
    EXPECT_EQ (1, kernel.numOutputs);

    constexpr int numSamples = 64;
    std::vector<float> input (static_cast<size_t> (numSamples * 2));

    for (int i = 0; i < numSamples; ++i)
    {
        input[static_cast<size_t> (i)] = static_cast<float> (i) * 0.01f - 0.25f;
        input[static_cast<size_t> (numSamples + i)] = 0.1f + static_cast<float> (i) * 0.005f;
    }

    auto output = runKernel (kernel, input, numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto dry = input[static_cast<size_t> (i)];
        const auto side = input[static_cast<size_t> (numSamples + i)];
        EXPECT_NEAR (tanhf (dry * (1.0f + 0.5f * side)), output[static_cast<size_t> (i)], 1e-5f);
    }

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, CompilesSampleModeOnePoleWithPrev)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor OnePole {
            input stream in;
            output stream out;
            state float z;
            process { out = 0.5 * in + 0.5 * out'; z = out; }
        }
        graph G { input stream x; output stream y; node p = OnePole; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                                 "OnePole",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    auto input = makeRampCodegen (32);
    auto output = runKernel (kernel, input, 32);

    float previous = 0.0f;
    for (int i = 0; i < 32; ++i)
    {
        const auto expected = 0.5f * input[static_cast<size_t> (i)] + 0.5f * previous;
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-5f);
        previous = output[static_cast<size_t> (i)];
    }

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, SampleLoopTailPreservesEmptyBlocksAndExactTripCounts)
{
    YdspDiagnostics diagnostics;
    const auto kernel = compileKernel (R"YDSP(
        processor Counter {
            input stream in; output stream out; state float sum;
            process { sum = sum + in; out = sum; }
        }
        graph G { input stream x; output stream y; node p = Counter; connection { x -> p.in; p.out -> y; } }
    )YDSP", "Counter", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    std::vector<uint8_t> state (kernel.stateBytes, 0);
    std::vector<float> input (18, 1.0f), output (18, -999.0f);
    void* inputs[] { input.data() };
    void* outputs[] { output.data() };
    YdspKernelContext context;
    context.inputs = inputs;
    context.outputs = outputs;
    context.state = state.data();
    context.stateArrays = state.data() + kernel.stateScalarBytes;
    context.sampleRate = 48000.0f;

    float sum = 0.0f;
    for (const int size : { 0, 1, 2, 7, 0, 17 })
    {
        std::fill (output.begin(), output.end(), -999.0f);
        context.numSamples = size;
        kernel.fn (&context);

        for (int i = 0; i < size; ++i)
            EXPECT_EQ (++sum, output[static_cast<size_t> (i)]);
        for (size_t i = static_cast<size_t> (size); i < output.size(); ++i)
            EXPECT_EQ (-999.0f, output[i]);
    }

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, SampleLoopIsClosedByItsOwnIncrementOnArm64)
{
    YdspDiagnostics diagnostics;
    const auto kernel = compileKernel (R"YDSP(
        processor Counter {
            input stream in; output stream out; state float sum;
            process { sum = sum + in; out = sum; }
        }
        graph G { input stream x; output stream y; node p = Counter; connection { x -> p.in; p.out -> y; } }
    )YDSP", "Counter", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    const auto input = makeRampCodegen (37, 0.5f);
    const auto output = runKernel (kernel, input, 37);

    float sum = 0.0f;
    for (size_t i = 0; i < output.size(); ++i)
        EXPECT_EQ (sum += input[i], output[i]) << i;

#if YUP_ARM && YUP_64BIT
    // The loop counts from -n up to zero against the end of each stream, so
    // the only compare left is the guard for an empty block.
    int compares = 0;
    for (const auto& line : StringArray::fromLines (kernel.asmText))
        compares += line.trimStart().startsWith ("cmp ") ? 1 : 0;

    EXPECT_EQ (1, compares) << kernel.asmText;
    EXPECT_TRUE (kernel.asmText.contains ("adds ")) << kernel.asmText;
#endif

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, HandWrittenRingWrapsItsReadAndWritePositions)
{
    YdspDiagnostics diagnostics;
    const auto kernel = compileKernel (R"YDSP(
        processor Ring {
            input stream in; output stream out;
            state float buf[5000]; state int wp;
            process {
                int r = wp - 300;
                if (r < 0) { r = r + 5000; }
                out = buf[r];
                buf[wp] = in;
                wp = wp + 1;
                if (wp >= 5000) { wp = 0; }
            }
        }
        graph G { input stream x; output stream y; node p = Ring; connection { x -> p.in; p.out -> y; } }
    )YDSP", "Ring", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    // Long enough for both positions to pass the end of the ring twice.
    const int numSamples = 12000;
    const auto input = makeRampCodegen (numSamples, 1.0f);
    const auto output = runKernel (kernel, input, numSamples);

    for (int i = 0; i < numSamples; ++i)
        ASSERT_EQ (i < 300 ? 0.0f : input[static_cast<size_t> (i - 300)], output[static_cast<size_t> (i)]) << i;

#if YUP_ARM && YUP_64BIT
    // The read wrap tests the flags of its own subtraction, and the write
    // wrap compares against a ring length loaded once, ahead of the loop.
    EXPECT_TRUE (kernel.asmText.contains ("subs ")) << kernel.asmText;
    EXPECT_TRUE (kernel.asmText.contains ("csinc ")) << kernel.asmText;
    EXPECT_LT (kernel.asmText.indexOf (", 4999"), kernel.asmText.indexOf ("b.ge ")) << kernel.asmText;
#endif

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, TwoIterationVectorMathUnrollPreservesOutputAndState)
{
    YdspDiagnostics diagnostics;
    const auto source = R"YDSP(
        processor P {
            input stream in; output stream out;
            state float phase[8];
            process {
                float sum = 0.0;
                for i in 0..8 {
                    phase[i] = phase[i] + in;
                    sum = sum + sin (phase[i]);
                }
                out = sum;
            }
        }
        graph G { input stream x; output stream y; node p = P;
            connection { x -> p.in; p.out -> y; } }
    )YDSP";
    YdspLexer lexer (source, diagnostics);
    YdspParser parser (lexer.tokenize(), diagnostics);
    auto parsed = parser.parseProgram();
    ASSERT_NE (nullptr, parsed) << diagnostics.toString();
    YdspSemanticAnalyzer analyzer (diagnostics);
    auto analyzed = analyzer.analyze (std::move (parsed));
    ASSERT_NE (nullptr, analyzed) << diagnostics.toString();
    YdspOptimizer optimizer (diagnostics);
    optimizer.setFastMath (true);
    optimizer.setVectorizationEnabled (true);
    optimizer.setVectorWidth (4);
    optimizer.setTargetHasVectorMath (true);
    auto ir = optimizer.build (*analyzed);
    ASSERT_NE (nullptr, ir) << diagnostics.toString();
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_FALSE (ir->kernels.empty());
    const auto& rolled = *ir->kernels.front();
    ASSERT_TRUE (rolled.vectorized);
    auto unrolled = rolled;
    optimizer.fullyUnrollBoundedLoops (unrolled);
    ASSERT_TRUE (std::any_of (unrolled.loops.begin(), unrolled.loops.end(), [] (const auto& loop)
    {
        return loop.unrolled;
    }));
    EXPECT_EQ (YdspAsmJitCodegen::stateSize (rolled), YdspAsmJitCodegen::stateSize (unrolled));

    asmjit::JitRuntime runtime;
    const auto rolledKernel = YdspAsmJitCodegen::compile (runtime, rolled, diagnostics);
    const auto unrolledKernel = YdspAsmJitCodegen::compile (runtime, unrolled, diagnostics);
    ASSERT_NE (nullptr, rolledKernel) << diagnostics.toString();
    ASSERT_NE (nullptr, unrolledKernel) << diagnostics.toString();
    const auto bytes = YdspAsmJitCodegen::stateSize (rolled);
    const auto scalarBytes = YdspAsmJitCodegen::stateScalarSize (rolled);
    std::vector<uint8_t> rolledState (bytes, 0), unrolledState (bytes, 0);
    std::vector<float> input (512), rolledOutput (512), unrolledOutput (512);
    void* inputs[] { input.data() };
    void* rolledOutputs[] { rolledOutput.data() };
    void* unrolledOutputs[] { unrolledOutput.data() };
    YdspKernelContext rolledContext;
    rolledContext.inputs = inputs;
    rolledContext.outputs = rolledOutputs;
    rolledContext.state = rolledState.data();
    rolledContext.stateArrays = rolledState.data() + scalarBytes;
    rolledContext.sampleRate = 48000.0f;
    auto unrolledContext = rolledContext;
    unrolledContext.outputs = unrolledOutputs;
    unrolledContext.state = unrolledState.data();
    unrolledContext.stateArrays = unrolledState.data() + scalarBytes;

    for (const float phase : { 0.0f, -0.0f, 1000000.0f, std::numeric_limits<float>::infinity(),
                               std::numeric_limits<float>::quiet_NaN() })
    {
        for (int mode = 0; mode < 8; ++mode)
            std::memcpy (rolledState.data() + scalarBytes + static_cast<size_t> (mode) * sizeof (float), &phase, sizeof (float));
        std::copy (rolledState.begin(), rolledState.end(), unrolledState.begin());
        for (size_t sample = 0; sample < input.size(); ++sample)
            input[sample] = static_cast<float> (static_cast<int> (sample % 17) - 8) * 0.0125f;
        for (int repeat = 0; repeat < 8; ++repeat)
            for (const int size : { 0, 1, 3, 4, 5, 31, 512 })
            {
                SCOPED_TRACE (size);
                const bool inPlace = repeat % 2 != 0;
                std::fill (rolledOutput.begin(), rolledOutput.end(), -999.0f);
                std::fill (unrolledOutput.begin(), unrolledOutput.end(), -999.0f);
                if (inPlace)
                {
                    std::copy (input.begin(), input.end(), rolledOutput.begin());
                    std::copy (input.begin(), input.end(), unrolledOutput.begin());
                }
                rolledContext.inputs = inPlace ? rolledOutputs : inputs;
                unrolledContext.inputs = inPlace ? unrolledOutputs : inputs;
                const auto stateBefore = rolledState;
                rolledContext.numSamples = unrolledContext.numSamples = size;
                rolledKernel (&rolledContext);
                unrolledKernel (&unrolledContext);
                EXPECT_EQ (0, std::memcmp (rolledOutput.data(), unrolledOutput.data(), rolledOutput.size() * sizeof (float)));
                EXPECT_EQ (rolledState, unrolledState);
                if (size == 0)
                    EXPECT_EQ (stateBefore, rolledState);
                for (size_t sample = static_cast<size_t> (size); sample < rolledOutput.size(); ++sample)
                    EXPECT_EQ (inPlace ? input[sample] : -999.0f, unrolledOutput[sample]);
            }
    }
}

TEST (YdspAsmJitCodegenTests, StatePersistsAcrossBlocks)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor OnePole {
            input stream in;
            output stream out;
            process { out = 0.5 * in + 0.5 * out'; }
        }
        graph G { input stream x; output stream y; node p = OnePole; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                                 "OnePole",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    // Two consecutive blocks share state
    std::vector<float> fullInput (32, 0.1f);
    std::vector<float> fullOutput (32);

    // Allocate shared state
    std::vector<uint8_t> state (kernel.stateBytes, 0);
    std::vector<float> params (static_cast<size_t> (kernel.numParams), 0.0f);
    std::vector<float> paramsOut (static_cast<size_t> (kernel.numParamsOut), 0.0f);
    std::vector<float> blockOut (32, 0.0f);

    auto runBlock = [&] (float* inPtr, float* outPtr, int n, bool first)
    {
        void* inPtrs[1] = { inPtr };
        void* outPtrs[1] = { outPtr };

        YdspKernelContext ctx;
        ctx.inputs = inPtrs;
        ctx.outputs = outPtrs;
        ctx.params = params.data();
        ctx.paramOut = paramsOut.data();
        ctx.state = reinterpret_cast<float*> (state.data());
        ctx.stateArrays = state.empty() ? nullptr : state.data() + kernel.stateScalarBytes;
        ctx.sampleRate = 44100.0f;
        ctx.numSamples = n;

        kernel.fn (&ctx);

        if (first)
            std::memcpy (fullOutput.data(), outPtr, static_cast<size_t> (n) * sizeof (float));
        else
            std::memcpy (fullOutput.data() + n, outPtr, static_cast<size_t> (n) * sizeof (float));
    };

    runBlock (fullInput.data(), blockOut.data(), 16, true);
    runBlock (fullInput.data() + 16, blockOut.data(), 16, false);

    float previous = 0.0f;
    for (int i = 0; i < 32; ++i)
    {
        const auto expected = 0.5f * 0.1f + 0.5f * previous;
        EXPECT_NEAR (expected, fullOutput[static_cast<size_t> (i)], 1e-5f);
        previous = fullOutput[static_cast<size_t> (i)];
    }

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, CompilesBlockModeWithLoop)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor BlockGain {
            input stream in;
            output stream out;
            input parameter float drive = 3;
            process block {
                for i in 0..blockSize { out[i] = in[i] * drive; }
            }
        }
        graph G { input stream x; output stream y; node b = BlockGain; connection { x -> b.in; b.out -> y; } }
    )YDSP",
                                 "BlockGain",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    auto input = makeRampCodegen (32);
    auto output = runKernel (kernel, input, 32, { 3.0f });

    for (int i = 0; i < 32; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 3.0f, output[static_cast<size_t> (i)], 1e-5f);

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, CompilesFixedDelay)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor Delay3 { input stream in; output stream out; process { out = in @ 3; } }
        graph G { input stream x; output stream y; node d = Delay3; connection { x -> d.in; d.out -> y; } }
    )YDSP",
                                 "Delay3",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    auto input = makeRampCodegen (32);
    auto output = runKernel (kernel, input, 32);

    for (int i = 0; i < 32; ++i)
    {
        const auto expected = i >= 3 ? input[static_cast<size_t> (i - 3)] : 0.0f;
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-6f);
    }

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, IntegerImmediateOperandsPreserveSignedArithmetic)
{
    YdspDiagnostics diagnostics;
    const auto kernel = compileKernel (R"YDSP(
        processor Immediate {
            input stream in; output stream out;
            process { let x = int (in); out = float (((x + 7) - 3) & 511); }
        }
        graph G { input stream x; output stream y; node p = Immediate; connection { x -> p.in; p.out -> y; } }
    )YDSP", "Immediate", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    const std::vector<float> input { -1024.0f, -513.0f, -7.0f, -1.0f, 0.0f, 508.0f, 511.0f, 1024.0f };
    const auto output = runKernel (kernel, input, static_cast<int> (input.size()));
    ASSERT_EQ (input.size(), output.size());
    for (size_t i = 0; i < input.size(); ++i)
        EXPECT_EQ (static_cast<float> (((static_cast<int> (input[i]) + 7) - 3) & 511), output[i]);

#if ASMJIT_ARCH_ARM == 64
    bool sawImmediateMask = false;
    for (const auto& line : StringArray::fromLines (kernel.asmText))
        sawImmediateMask |= line.trimStart().startsWith ("and ") && line.contains (", 511");
    EXPECT_TRUE (sawImmediateMask) << kernel.asmText;
#endif
    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, IntegerImmediateSelectionKeepsDynamicAndUnencodableOperands)
{
    YdspDiagnostics diagnostics;
    const auto kernel = compileKernel (R"YDSP(
        processor Masks {
            input stream in; output stream out;
            process {
                let x = int (in);
                int mask = 511;
                if (x < 0) { mask = 255; }
                out = float (((x + 5000) & mask) + (x & 341));
            }
        }
        graph G { input stream x; output stream y; node p = Masks; connection { x -> p.in; p.out -> y; } }
    )YDSP", "Masks", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    const std::vector<float> input { -513.0f, -1.0f, 0.0f, 511.0f, 1024.0f };
    const auto output = runKernel (kernel, input, static_cast<int> (input.size()));
    ASSERT_EQ (input.size(), output.size());
    for (size_t i = 0; i < input.size(); ++i)
    {
        const auto x = static_cast<int> (input[i]);
        EXPECT_EQ (static_cast<float> (((x + 5000) & (x < 0 ? 255 : 511)) + (x & 341)), output[i]);
    }
    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, VectorMathTargetsAreLoadedOnceBeforeCalls)
{
    for (const auto architecture : { YdspTargetArchitecture::arm64, YdspTargetArchitecture::x64 })
        for (const bool fastMath : { false, true })
        {
            YdspIrFunction fn;
            fn.name = "VectorMathTargets";
            fn.fastMath = fastMath;
            fn.float32ArrayElements = 16;
            fn.valueTypes = { YdspValueType::int32Type, YdspValueType::float32Type,
                              YdspValueType::float32Type, YdspValueType::float32Type,
                              YdspValueType::float32Type };
            fn.valueLanes = { 1, 4, 4, 4, 4 };
            fn.vectorized = true;
            fn.vectorWidth = 4;
            fn.blocks.resize (1);
            fn.blocks[0].insts = {
                { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
                { YdspIrOp::loadStateArrayF, 1, 0, -1, -1, 0 },
                { YdspIrOp::sinF, 2, 1 },
                { YdspIrOp::sinF, 3, 2 },
                { YdspIrOp::powF, 4, 3, 2 },
                { YdspIrOp::storeStateArrayF, -1, 0, 4, -1, 4 }
            };
            YdspDiagnostics diagnostics;
            const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, architecture }, fn, diagnostics);
            ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
            ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
            const auto& symbols = artifact.getReference().symbols;
            ASSERT_EQ (2u, symbols.size());
            EXPECT_EQ (String ("pow.v4.u10"), symbols[0].name);
            EXPECT_EQ (String (fastMath ? "sin.v4.u35" : "sin.v4.u10"), symbols[1].name);

            if (architecture == YdspTargetArchitecture::arm64)
            {
                int targetLoads = 0;
                int calls = 0;
                bool sawVectorLoad = false;
                for (const auto& line : StringArray::fromLines (diagnostics.toString()))
                {
                    const auto instruction = line.trimStart();
                    sawVectorLoad |= instruction.startsWith ("ldr q");
                    if (instruction.startsWith ("ldr x") && instruction.contains ("[L"))
                    {
                        EXPECT_FALSE (sawVectorLoad) << diagnostics.toString();
                        ++targetLoads;
                    }
                    if (instruction.startsWith ("blr "))
                        ++calls;
                }
                EXPECT_EQ (2, targetLoads) << diagnostics.toString();
                EXPECT_EQ (3, calls) << diagnostics.toString();
            }
        }
}

TEST (YdspAsmJitCodegenTests, ReservesArrayBasesOnlyForIndexedAccesses)
{
    for (const bool indexed : { false, true })
    {
        YdspIrFunction fn;
        fn.name = "ArrayBaseReservation";
        fn.float32ArrayElements = 128;
        fn.valueTypes = { YdspValueType::int32Type, YdspValueType::float32Type, YdspValueType::int32Type };
        fn.valueLanes = { 1, 4, 1 };
        fn.vectorized = true;
        fn.vectorWidth = 4;
        fn.blocks.resize (1);
        fn.blocks[0].insts = {
            { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
            { YdspIrOp::loadStateArrayF, 1, 0, -1, -1, 64 },
            { YdspIrOp::storeStateArrayF, -1, 0, 1, -1, 64 }
        };
        if (indexed)
        {
            fn.blocks[0].insts.push_back ({ YdspIrOp::loadBlockSize, 2 });
            fn.blocks[0].insts.push_back ({ YdspIrOp::storeStateArrayF, -1, 2, 1, -1, 64 });
        }
        YdspDiagnostics diagnostics;
        const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::arm64 }, fn, diagnostics);
        ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
        int addressAdds = 0;
        for (const auto& line : StringArray::fromLines (diagnostics.toString()))
            if (line.trimStart().startsWith ("add "))
                ++addressAdds;
        EXPECT_EQ (indexed ? 1 : 0, addressAdds) << diagnostics.toString();
    }
}

TEST (YdspAsmJitCodegenTests, VectorFusedOperationsReuseOnlyDeadLocalAddends)
{
    for (const auto architecture : { YdspTargetArchitecture::arm64, YdspTargetArchitecture::x64 })
        for (const auto op : { YdspIrOp::fmaF, YdspIrOp::fmsubF })
            for (const bool shared : { false, true })
            {
                YdspIrFunction fn;
                fn.name = "VectorAccumulatorReuse";
                fn.float32ArrayElements = 32;
                fn.valueTypes = { YdspValueType::int32Type, YdspValueType::float32Type,
                                  YdspValueType::float32Type, YdspValueType::float32Type,
                                  YdspValueType::float32Type, YdspValueType::float32Type };
                fn.valueLanes = { 1, 4, 4, 4, 4, 4 };
                fn.vectorized = true;
                fn.vectorWidth = 4;
                fn.packedFmaAvailable = true;
                fn.blocks.resize (1);
                fn.blocks[0].insts = {
                    { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
                    { YdspIrOp::loadStateArrayF, 1, 0, -1, -1, 0 },
                    { YdspIrOp::loadStateArrayF, 2, 0, -1, -1, 4 },
                    { YdspIrOp::loadStateArrayF, 3, 0, -1, -1, 8 },
                    { YdspIrOp::mulF, 4, 1, 2 },
                    { op, 5, 1, 3, 4 },
                    { YdspIrOp::storeStateArrayF, -1, 0, 5, -1, 12 }
                };
                if (shared)
                    fn.blocks[0].insts.push_back ({ YdspIrOp::storeStateArrayF, -1, 0, 4, -1, 16 });
                YdspDiagnostics diagnostics;
                const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, architecture }, fn, diagnostics);
                ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
                ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
                const auto listing = diagnostics.toString();
                if (architecture == YdspTargetArchitecture::arm64)
                {
                    int vectorCopies = 0;
                    for (const auto& line : StringArray::fromLines (listing))
                        if (line.trimStart().startsWith ("mov ") && line.contains (".16b"))
                            ++vectorCopies;
                    EXPECT_EQ (shared ? 1 : 0, vectorCopies) << listing;
                }
                else if (! shared)
                    EXPECT_TRUE (listing.contains (op == YdspIrOp::fmaF ? "vfmadd231ps" : "vfnmadd231ps")) << listing;
            }
}

TEST (YdspAsmJitCodegenTests, ArrayLoadEliminationPreservesOverlappingVectorWrites)
{
    for (const int testCase : { 0, 1, 2, 3, 4, 5 })
    {
        SCOPED_TRACE (testCase);
        const int offsets[] { 0, 1, 4 };
        const int offset = offsets[testCase % 3];
        const bool cse = testCase >= 3;
        YdspIrFunction original;
        original.name = "ArrayForwarding";
        original.isSampleMode = false;
        original.float32ArrayElements = 32;
        original.valueTypes = { YdspValueType::int32Type, YdspValueType::int32Type,
                                YdspValueType::float32Type, YdspValueType::float32Type, YdspValueType::float32Type };
        original.valueLanes = { 1, 1, 4, 4, 4 };
        original.vectorized = true;
        original.vectorWidth = 4;
        original.blocks.resize (1);
        original.blocks[0].insts = {
            { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
            { YdspIrOp::constI, 1, -1, -1, -1, -1, 0.0, offset },
            { YdspIrOp::loadStateArrayF, 2, 0, -1, -1, 16 },
            { YdspIrOp::loadStateArrayF, 3, 0, -1, -1, 20 },
            { YdspIrOp::storeStateArrayF, -1, 0, 2, -1, 0 },
            { YdspIrOp::storeStateArrayF, -1, 1, 3, -1, 0 },
            { YdspIrOp::loadStateArrayF, 4, 0, -1, -1, 0 },
            { YdspIrOp::storeStateArrayF, -1, 0, 4, -1, 8 }
        };
        if (cse)
        {
            original.blocks[0].insts[5].a = 0;
            original.blocks[0].insts[5].memIndex = 16 + offset;
            original.blocks[0].insts[6].memIndex = 16;
        }
        auto forwarded = original;
        YdspDiagnostics diagnostics;
        YdspOptimizer optimizer (diagnostics);
        if (cse)
        {
            optimizer.commonSubexpressionElimination (forwarded);
            EXPECT_EQ (offset == 4 ? YdspIrOp::movF : YdspIrOp::loadStateArrayF,
                       forwarded.blocks[0].insts[6].op);
        }
        else
        {
            EXPECT_EQ (offset != 1, optimizer.storeToLoadForwarding (forwarded));
        }
        for (const auto architecture : { YdspTargetArchitecture::arm64, YdspTargetArchitecture::x64 })
        {
            auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, architecture }, forwarded, diagnostics);
            ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
        }
        asmjit::JitRuntime runtime;
        const auto before = YdspAsmJitCodegen::compile (runtime, original, diagnostics);
        const auto after = YdspAsmJitCodegen::compile (runtime, forwarded, diagnostics);
        ASSERT_NE (nullptr, before) << diagnostics.toString();
        ASSERT_NE (nullptr, after) << diagnostics.toString();
        std::vector<float> reference (32, 0.0f);
        reference[16] = -0.0f;
        reference[17] = std::numeric_limits<float>::quiet_NaN();
        reference[18] = std::numeric_limits<float>::infinity();
        reference[19] = -1.0f;
        for (int i = 20; i < 24; ++i)
            reference[static_cast<size_t> (i)] = static_cast<float> (i);
        auto actual = reference;
        YdspKernelContext context;
        context.numSamples = 4;
        context.state = reference.data();
        context.stateArrays = reference.data();
        before (&context);
        context.state = actual.data();
        context.stateArrays = actual.data();
        after (&context);
        EXPECT_EQ (0, std::memcmp (reference.data(), actual.data(), reference.size() * sizeof (float)));
    }
}

TEST (YdspAsmJitCodegenTests, ConstantVectorArrayOffsetsRespectEncodingLimits)
{
    for (const int index : { 4, 5, 16380, 16384 })
        for (const auto architecture : { YdspTargetArchitecture::arm64, YdspTargetArchitecture::x64 })
        {
            YdspIrFunction fn;
            fn.name = "VectorArrayDisplacement";
            fn.float32ArrayElements = 16400;
            fn.valueTypes = { YdspValueType::int32Type, YdspValueType::float32Type };
            fn.valueLanes = { 1, 4 };
            fn.vectorized = true;
            fn.vectorWidth = 4;
            fn.blocks.resize (1);
            fn.blocks[0].insts = {
                { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, index },
                { YdspIrOp::loadStateArrayF, 1, 0, -1, -1, 0 },
                { YdspIrOp::storeStateArrayF, -1, 0, 1, -1, 0 }
            };
            YdspDiagnostics diagnostics;
            const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, architecture }, fn, diagnostics);
            ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
            ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
            const auto listing = diagnostics.toString();
            if (architecture == YdspTargetArchitecture::arm64)
                EXPECT_EQ (index == 5 || index == 16384, listing.contains ("lsl ")) << listing;
            else
                EXPECT_FALSE (listing.contains ("*4")) << listing;
        }
}

TEST (YdspAsmJitCodegenTests, ConstantArrayIndicesUseByteDisplacementsOnBothTargets)
{
    for (const auto type : { YdspValueType::float32Type, YdspValueType::float64Type,
                            YdspValueType::int32Type, YdspValueType::int64Type })
        for (const int index : { 5, 8192 })
        {
            YdspIrFunction fn;
            fn.name = "ArrayDisplacement";
            fn.float32ArrayElements = fn.float64ArrayElements = fn.int32ArrayElements = fn.int64ArrayElements = 8200;
            // Test each type at the beginning of its own array segment.
            if (type != YdspValueType::float32Type)
                fn.float32ArrayElements = 0;
            if (type != YdspValueType::float64Type)
                fn.float64ArrayElements = 0;
            if (type != YdspValueType::int32Type)
                fn.int32ArrayElements = 0;
            const bool floating = type == YdspValueType::float32Type || type == YdspValueType::float64Type;
            fn.valueTypes = { YdspValueType::int32Type, type };
            fn.blocks.resize (1);
            fn.blocks[0].insts = {
                { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, index },
                { floating ? YdspIrOp::loadStateArrayF : YdspIrOp::loadStateArrayI, 1, 0, -1, -1, 0 },
                { floating ? YdspIrOp::storeStateArrayF : YdspIrOp::storeStateArrayI, -1, 0, 1, -1, 0 }
            };
            for (const auto architecture : { YdspTargetArchitecture::arm64, YdspTargetArchitecture::x64 })
            {
                YdspDiagnostics diagnostics;
                const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, architecture }, fn, diagnostics);
                ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
                ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
                if (architecture == YdspTargetArchitecture::arm64)
                    EXPECT_EQ (index == 8192, diagnostics.toString().contains ("lsl 2") || diagnostics.toString().contains ("lsl 3"))
                        << diagnostics.toString();
                else
                {
                    EXPECT_FALSE (diagnostics.toString().contains ("*4")) << diagnostics.toString();
                    EXPECT_FALSE (diagnostics.toString().contains ("*8")) << diagnostics.toString();
                }
                if (index == 5)
                {
                    const int bytes = type == YdspValueType::float64Type || type == YdspValueType::int64Type ? 8 : 4;
                    const auto offset = String (bytes * index);
                    EXPECT_TRUE (diagnostics.toString().contains (String (", ") + offset + "]")
                                 || diagnostics.toString().contains (String ("+") + offset + "]")) << diagnostics.toString();
                }
            }
        }
}

TEST (YdspAsmJitCodegenTests, OperandLoweringEmitsFusedSubtractAndImmediatesOnBothTargets)
{
    for (const auto type : { YdspValueType::float32Type, YdspValueType::float64Type })
    {
        YdspIrFunction fn;
        fn.name = "OperandLowering";
        fn.numInputs = fn.numOutputs = 1;
        fn.inputTypes = fn.outputTypes = { type };
        fn.valueTypes = { YdspValueType::int32Type, YdspValueType::int32Type,
                          YdspValueType::int32Type, YdspValueType::int32Type,
                          YdspValueType::int32Type, type, type, type, type };
        fn.blocks.resize (1);
        fn.blocks[0].insts = {
            { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
            { YdspIrOp::constI, 1, -1, -1, -1, -1, 0.0, 7 },
            { YdspIrOp::addI, 2, 0, 1 },
            { YdspIrOp::constI, 3, -1, -1, -1, -1, 0.0, 511 },
            { YdspIrOp::andI, 4, 2, 3 },
            { YdspIrOp::loadInput, 5, 4, -1, -1, 0 },
            { YdspIrOp::constF, 6, -1, -1, -1, -1, 2.0 },
            { YdspIrOp::negF, 7, 5 },
            { YdspIrOp::fmaF, 8, 5, 6, 7 },
            { YdspIrOp::storeOutput, -1, 0, 8, -1, 0 }
        };

        for (const auto architecture : { YdspTargetArchitecture::arm64, YdspTargetArchitecture::x64 })
        {
            YdspDiagnostics diagnostics;
            const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, architecture }, fn, diagnostics);
            ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
            ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
            const auto listing = diagnostics.toString();
            const auto mnemonic = architecture == YdspTargetArchitecture::arm64 ? "fnmsub "
                                : type == YdspValueType::float64Type ? "vfmsub213sd " : "vfmsub213ss ";
            EXPECT_TRUE (listing.contains (mnemonic)) << listing;

            bool sawImmediateMask = false;
            for (const auto& line : StringArray::fromLines (listing))
                sawImmediateMask |= line.trimStart().startsWith ("and ") && line.contains (", 511");
            EXPECT_TRUE (sawImmediateMask) << listing;
        }
    }
}

TEST (YdspAsmJitCodegenTests, FusedIntegerComparisonsUseWidthCorrectImmediatesOnBothTargets)
{
    for (const auto architecture : { YdspTargetArchitecture::arm64, YdspTargetArchitecture::x64 })
    {
        for (const int64_t bound : { int64_t (1116), int64_t (4096), int64_t (4294967295) })
        {
            YdspIrFunction fn;
            fn.name = "CompareImmediate";
            fn.numInputs = fn.numOutputs = 1;
            fn.inputTypes = fn.outputTypes = { YdspValueType::float32Type };
            fn.valueTypes = { YdspValueType::int32Type, YdspValueType::int64Type,
                              YdspValueType::int64Type, YdspValueType::boolType,
                              YdspValueType::float32Type, YdspValueType::float32Type,
                              YdspValueType::float32Type };
            fn.blocks.resize (1);
            fn.blocks[0].insts = {
                { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
                { YdspIrOp::constI, 1, -1, -1, -1, -1, 0.0, 7 },
                { YdspIrOp::constI, 2, -1, -1, -1, -1, 0.0, bound },
                { YdspIrOp::ltI, 3, 1, 2 },
                { YdspIrOp::loadInput, 4, 0, -1, -1, 0 },
                { YdspIrOp::constF, 5, -1, -1, -1, -1, 1.0 },
                { YdspIrOp::selectB, 6, 3, 4, 5 },
                { YdspIrOp::storeOutput, -1, 0, 6, -1, 0 }
            };

            YdspDiagnostics diagnostics;
            const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, architecture }, fn, diagnostics);
            ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
            const auto listing = diagnostics.toString();
            bool sawImmediate = false;
            for (const auto& line : StringArray::fromLines (listing))
                sawImmediate |= line.trimStart().startsWith ("cmp ") && line.contains (String (", ") + String (bound));
            const bool encodable = architecture == YdspTargetArchitecture::arm64 ? bound <= 4095
                                                                                 : bound <= std::numeric_limits<int32_t>::max();
            EXPECT_EQ (encodable, sawImmediate) << listing;
        }
    }
}

TEST (YdspAsmJitCodegenTests, IntegerSaturationOnlyEmitsOverflowHandlingWhenRequired)
{
    for (const auto architecture : { YdspTargetArchitecture::arm64, YdspTargetArchitecture::x64 })
        for (const bool wide : { false, true })
            for (const bool saturating : { false, true })
                for (const auto op : { YdspIrOp::addI, YdspIrOp::subI, YdspIrOp::mulI })
                {
                    SCOPED_TRACE (::testing::Message() << "arch=" << static_cast<int> (architecture) << ", wide=" << wide
                                                      << ", saturating=" << saturating << ", op=" << static_cast<int> (op));
                    YdspIrFunction fn;
                    fn.name = "IntegerArithmetic";
                    fn.numInputs = fn.numOutputs = 1;
                    fn.inputTypes = fn.outputTypes = { YdspValueType::float32Type };
                    const auto type = wide ? YdspValueType::int64Type : YdspValueType::int32Type;
                    fn.valueTypes = { YdspValueType::int32Type, YdspValueType::float32Type, type, type, type, YdspValueType::float32Type };
                    fn.blocks.resize (1);
                    auto& insts = fn.blocks[0].insts;
                    insts = {
                        { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
                        { YdspIrOp::loadInput, 1, 0, -1, -1, 0 },
                        { YdspIrOp::ftoi, 2, 1 },
                        { YdspIrOp::constI, 3, -1, -1, -1, -1, 0.0, 3 },
                        { op, 4, 2, 3 },
                        { YdspIrOp::itof, 5, 4 },
                        { YdspIrOp::storeOutput, -1, 0, 5, -1, 0 }
                    };
                    insts[4].saturatingIntegerArithmetic = saturating;
                    YdspDiagnostics diagnostics;
                    const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, architecture }, fn, diagnostics);
                    ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
                    const auto listing = diagnostics.toString();
                    if (architecture == YdspTargetArchitecture::x64)
                        EXPECT_EQ (saturating, listing.contains ("cmovo")) << listing;
                    else if (op == YdspIrOp::mulI)
                        EXPECT_EQ (saturating, listing.contains (wide ? "smulh" : "smull")) << listing;
                    else
                        EXPECT_EQ (saturating, listing.contains (op == YdspIrOp::addI ? "adds " : "subs ")) << listing;
                }
}

TEST (YdspAsmJitCodegenTests, ProvenFloatConversionsKeepTheDirectInstructionPath)
{
    for (const auto architecture : { YdspTargetArchitecture::arm64, YdspTargetArchitecture::x64 })
        for (const bool bounded : { false, true })
        {
            YdspIrFunction fn;
            fn.name = "Conversion";
            fn.numInputs = fn.numOutputs = 1;
            fn.inputTypes = fn.outputTypes = { YdspValueType::float32Type };
            fn.valueTypes = { YdspValueType::int32Type, YdspValueType::float32Type,
                              YdspValueType::int32Type, YdspValueType::float32Type };
            fn.blocks.resize (1);
            auto& insts = fn.blocks[0].insts;
            insts = {
                { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
                { YdspIrOp::loadInput, 1, 0, -1, -1, 0 },
                { YdspIrOp::ftoi, 2, 1 },
                { YdspIrOp::itof, 3, 2 },
                { YdspIrOp::storeOutput, -1, 0, 3, -1, 0 },
            };
            insts[2].boundedFloatToInt = bounded;
            YdspDiagnostics diagnostics;
            const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, architecture }, fn, diagnostics);
            ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
            const auto listing = diagnostics.toString();
            if (architecture == YdspTargetArchitecture::x64)
            {
                EXPECT_TRUE (listing.contains ("cvttss2si")) << listing;
                EXPECT_EQ (! bounded, listing.contains ("cmovp")) << listing;
                EXPECT_EQ (! bounded, listing.contains ("cmovae") || listing.contains ("cmovnb")) << listing;
            }
            else
            {
                EXPECT_TRUE (listing.contains ("fcvtzs")) << listing;
                EXPECT_FALSE (listing.contains ("fcmp")) << listing;
            }
        }
}

TEST (YdspAsmJitCodegenTests, SharedSelectComparisonsFuseOnlyWhileEveryUseIsStable)
{
    for (const auto architecture : { YdspTargetArchitecture::arm64, YdspTargetArchitecture::x64 })
        for (const bool overwriteOperand : { false, true })
        {
            YdspIrFunction fn;
            fn.name = "SharedCompare";
            fn.numInputs = fn.numOutputs = 1;
            fn.inputTypes = fn.outputTypes = { YdspValueType::float32Type };
            fn.valueTypes = { YdspValueType::int32Type, YdspValueType::int32Type,
                              YdspValueType::int32Type, YdspValueType::boolType,
                              YdspValueType::float32Type, YdspValueType::float32Type,
                              YdspValueType::float32Type, YdspValueType::float32Type,
                              YdspValueType::float32Type };
            fn.blocks.resize (1);
            auto& insts = fn.blocks[0].insts;
            insts = {
                { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
                { YdspIrOp::constI, 1, -1, -1, -1, -1, 0.0, 7 },
                { YdspIrOp::constI, 2, -1, -1, -1, -1, 0.0, 8 },
                { YdspIrOp::ltUI, 3, 1, 2 },
                { YdspIrOp::loadInput, 4, 0, -1, -1, 0 },
                { YdspIrOp::constF, 5, -1, -1, -1, -1, 1.0 },
                { YdspIrOp::selectB, 6, 3, 4, 5 }
            };
            if (overwriteOperand)
                insts.push_back ({ YdspIrOp::constI, 1, -1, -1, -1, -1, 0.0, 99 });
            insts.push_back ({ YdspIrOp::selectB, 7, 3, 5, 4 });
            insts.push_back ({ YdspIrOp::addF, 8, 6, 7 });
            insts.push_back ({ YdspIrOp::storeOutput, -1, 0, 8, -1, 0 });

            YdspDiagnostics diagnostics;
            const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, architecture }, fn, diagnostics);
            ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
            bool materializedBoolean = false;
            for (const auto& line : StringArray::fromLines (diagnostics.toString()))
            {
                const auto instruction = line.trimStart();
                materializedBoolean |= architecture == YdspTargetArchitecture::arm64
                                         ? instruction.startsWith ("cset ")
                                         : instruction.startsWith ("setb ") || instruction.startsWith ("rex setb ");
            }
            EXPECT_EQ (overwriteOperand, materializedBoolean) << diagnostics.toString();
        }
}

TEST (YdspAsmJitCodegenTests, FusedIntegerComparisonPreservesThresholdBoundaries)
{
    YdspDiagnostics diagnostics;
    const auto kernel = compileKernel (R"YDSP(
        processor Threshold {
            input stream in; output stream out;
            process { out = int (in) >= 1116 ? in + 1.0 : in - 1.0; }
        }
        graph G { input stream x; output stream y; node p = Threshold; connection { x -> p.in; p.out -> y; } }
    )YDSP", "Threshold", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);
    const std::vector<float> input { -1116.0f, -1.0f, 0.0f, 1115.0f, 1116.0f, 1117.0f };
    const auto output = runKernel (kernel, input, static_cast<int> (input.size()));
    ASSERT_EQ (input.size(), output.size());
    for (size_t i = 0; i < input.size(); ++i)
        EXPECT_EQ (input[i] >= 1116.0f ? input[i] + 1.0f : input[i] - 1.0f, output[i]);
}

TEST (YdspAsmJitCodegenTests, WideIntegerOperandsDoNotSignExtendUnsignedMasks)
{
    YdspDiagnostics diagnostics;
    const auto kernel = compileKernel (R"YDSP(
        processor Wide {
            input stream in; output stream out;
            process {
                let x = int64 (in);
                let mask = (int64 (1) << 32) - int64 (1);
                out = float ((x + mask) >> 32) + float ((x & mask) >> 32);
            }
        }
        graph G { input stream x; output stream y; node p = Wide; connection { x -> p.in; p.out -> y; } }
    )YDSP", "Wide", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);
    const std::vector<float> input { -1024.0f, -1.0f, 0.0f, 1.0f, 1024.0f };
    const auto output = runKernel (kernel, input, static_cast<int> (input.size()));
    ASSERT_EQ (input.size(), output.size());
    for (size_t i = 0; i < input.size(); ++i)
        EXPECT_EQ (input[i] >= 1.0f ? 1.0f : 0.0f, output[i]);
    dumpAsmOnFailureCodegen (kernel);
}

#if ASMJIT_ARCH_ARM == 64
TEST (YdspAsmJitCodegenTests, FusedNegatedAddendPreservesSignedZero)
{
    YdspDiagnostics diagnostics;
    const auto kernel = compileKernel (R"YDSP(
        processor Difference {
            input stream in; output stream out;
            process { out = fma (in, 2.0, -in); }
        }
        graph G { input stream x; output stream y; node p = Difference; connection { x -> p.in; p.out -> y; } }
    )YDSP", "Difference", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);
    const std::vector<float> input { -0.0f, 0.0f, -0.3f, 0.3f };
    const auto output = runKernel (kernel, input, static_cast<int> (input.size()));
    ASSERT_EQ (input.size(), output.size());
    for (size_t i = 0; i < input.size(); ++i)
    {
        const auto expected = std::fma (input[i], 2.0f, -input[i]);
        EXPECT_EQ (expected, output[i]);
        EXPECT_EQ (std::signbit (expected), std::signbit (output[i]));
    }
    EXPECT_TRUE (kernel.asmText.contains ("fnmsub ")) << kernel.asmText;
    dumpAsmOnFailureCodegen (kernel);
}
#endif

TEST (YdspAsmJitCodegenTests, CompilesMathIntrinsics)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor Maths {
            input stream in;
            output stream out;
            process { out = sqrt (abs (in) + 1) * sin (in); }
        }
        graph G { input stream x; output stream y; node m = Maths; connection { x -> m.in; m.out -> y; } }
    )YDSP",
                                 "Maths",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    auto input = makeRampCodegen (64, -1.0f);
    auto output = runKernel (kernel, input, 64);

    for (int i = 0; i < 64; ++i)
    {
        const auto x = input[static_cast<size_t> (i)];
        const auto expected = sqrtf (fabsf (x) + 1.0f) * sinf (x);
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-4f);
    }

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, CompilesSignIntrinsic)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor Sign { input stream in; output stream out; process { out = sign (in); } }
        graph G { input stream x; output stream y; node s = Sign; connection { x -> s.in; s.out -> y; } }
    )YDSP",
                                 "Sign",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    // Ramp crosses zero at sample 35, covering the negative, zero and positive branches.
    auto input = makeRampCodegen (64, -0.35f);
    auto output = runKernel (kernel, input, 64);

    for (int i = 0; i < 64; ++i)
    {
        const auto x = input[static_cast<size_t> (i)];
        const auto expected = x > 0.0f ? 1.0f : (x < 0.0f ? -1.0f : 0.0f);
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-6f);
    }

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, CompilesIntegerMinMaxClampAbsSign)
{
    // min/max/clamp/abs/sign have an integer overload (minI/maxI/clampI/
    // absI/signI) alongside the existing float intrinsics: an int-typed
    // argument must select it, not silently fall back to the float form.
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor IntMath {
            input stream in;
            output stream out;
            process {
                let n = int32 (in * 100.0) - 50; // -50 .. 49 over a 100-sample ramp
                let mn = min (n, 3);
                let mx = max (n, -3);
                let cl = clamp (n, -10, 10);
                let ab = abs (n);
                let sg = sign (n);
                out = float32 (mn + mx + cl + ab + sg);
            }
        }
        graph G { input stream x; output stream y; node p = IntMath; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                                 "IntMath",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    auto input = makeRampCodegen (100);
    auto output = runKernel (kernel, input, 100);

    for (int i = 0; i < 100; ++i)
    {
        const auto n = static_cast<int> (input[static_cast<size_t> (i)] * 100.0f) - 50;
        const auto mn = std::min (n, 3);
        const auto mx = std::max (n, -3);
        const auto cl = std::min (std::max (n, -10), 10);
        const auto ab = n < 0 ? -n : n;
        const auto sg = n > 0 ? 1 : (n < 0 ? -1 : 0);
        const auto expected = static_cast<float> (mn + mx + cl + ab + sg);

        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-3f) << "at " << i;
    }

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, IntegerAbsOfIntMinSaturates)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor AbsIntMin {
            input stream in;
            output stream out;
            state int base;
            process {
                let x = base - 2147483648;
                let y = abs (x);
                out = in + select (y == 2147483647, 1.0, 0.0);
            }
        }
        graph G { input stream x; output stream y; node p = AbsIntMin; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                                 "AbsIntMin",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    std::vector<float> input (4, 0.0f);
    auto output = runKernel (kernel, input, 4);

    for (const auto sample : output)
        EXPECT_NEAR (1.0f, sample, 1e-6f);

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, CompilesTernaryExpression)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor Gate {
            input stream in;
            output stream out;
            process { out = (in > 0) ? in : 0; }
        }
        graph G { input stream x; output stream y; node g = Gate; connection { x -> g.in; g.out -> y; } }
    )YDSP",
                                 "Gate",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    auto input = makeRampCodegen (64, -0.3f);
    auto output = runKernel (kernel, input, 64);

    for (int i = 0; i < 64; ++i)
    {
        const auto x = input[static_cast<size_t> (i)];
        const auto expected = x > 0.0f ? x : 0.0f;
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-6f);
    }

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, CompilesBlockModeWithStateArray)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor DelayLine {
            input stream in;
            output stream out;
            state float mem[256];
            state int wp;
            process block {
                for i in 0..blockSize {
                    mem[wp] = in[i];
                    out[i] = mem[wp];
                    wp = (wp + 1) % 256;
                }
            }
        }
        graph G { input stream x; output stream y; node d = DelayLine; connection { x -> d.in; d.out -> y; } }
    )YDSP",
                                 "DelayLine",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    auto input = makeRampCodegen (64);
    auto output = runKernel (kernel, input, 64);

    // With write pointer at 0 and no prior reads, out[i] == in[i] (identity for first pass)
    for (int i = 0; i < 64; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)], output[static_cast<size_t> (i)], 1e-6f);

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, StateSizeIsPositiveForKernelWithState)
{
    YdspDiagnostics diagnostics;

    auto kernelNoState = compileKernel (R"YDSP(
        processor P { input stream in; output stream out; process { out = in; } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                                        "P",
                                        diagnostics);

    ASSERT_NE (nullptr, kernelNoState.fn);
    // A pass-through with no state variables has zero state (sample-loop counter is virtual)
    EXPECT_EQ (0u, kernelNoState.stateBytes);

    auto kernelWithState = compileKernel (R"YDSP(
        processor Q {
            input stream in;
            output stream out;
            state float buf[128];
            process block {
                for i in 0..blockSize { buf[i] = in[i]; out[i] = buf[i]; }
            }
        }
        graph G { input stream x; output stream y; node q = Q; connection { x -> q.in; q.out -> y; } }
    )YDSP",
                                          "Q",
                                          diagnostics);

    ASSERT_NE (nullptr, kernelWithState.fn);
    EXPECT_GT (kernelWithState.stateBytes, kernelNoState.stateBytes);

    dumpAsmOnFailureCodegen (kernelNoState);
    dumpAsmOnFailureCodegen (kernelWithState);
}

TEST (YdspAsmJitCodegenTests, CompilesStereoProcessor)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor StereoGain {
            input stream leftIn;
            input stream rightIn;
            output stream leftOut;
            output stream rightOut;
            input parameter float gain = 1;
            process {
                leftOut = leftIn * gain;
                rightOut = rightIn * gain;
            }
        }
        graph G {
            input stream L, R;
            output stream outL, outR;
            node sg = StereoGain (gain = 0.5);
            connection { L -> sg.leftIn; R -> sg.rightIn; sg.leftOut -> outL; sg.rightOut -> outR; }
        }
    )YDSP",
                                 "StereoGain",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);
    EXPECT_EQ (2, kernel.numInputs);
    EXPECT_EQ (2, kernel.numOutputs);

    // Build interleaved stereo input
    std::vector<float> interleaved (128);
    for (int i = 0; i < 64; ++i)
    {
        interleaved[static_cast<size_t> (i * 2)] = static_cast<float> (i) * 0.01f;
        interleaved[static_cast<size_t> (i * 2 + 1)] = static_cast<float> (i) * 0.02f;
    }

    auto output = runKernel (kernel, interleaved, 64, { 0.5f });

    for (int i = 0; i < 64; ++i)
    {
        EXPECT_NEAR (interleaved[static_cast<size_t> (i * 2)] * 0.5f, output[static_cast<size_t> (i * 2)], 1e-5f);
        EXPECT_NEAR (interleaved[static_cast<size_t> (i * 2 + 1)] * 0.5f, output[static_cast<size_t> (i * 2 + 1)], 1e-5f);
    }

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, CompilesBlockModeWithConstantLoopBound)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor Taps {
            input stream in;
            output stream out;
            state float mem[16];
            process block {
                for i in 0..8 { out[i] = mem[i] + in[i]; }
            }
        }
        graph G { input stream x; output stream y; node t = Taps; connection { x -> t.in; t.out -> y; } }
    )YDSP",
                                 "Taps",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    auto input = makeRampCodegen (32);
    auto output = runKernel (kernel, input, 32);

    // First 8 samples: mem[i] is 0 + in[i]; next 24 samples: untouched (0)
    for (int i = 0; i < 8; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)], output[static_cast<size_t> (i)], 1e-6f);

    for (int i = 8; i < 32; ++i)
        EXPECT_NEAR (0.0f, output[static_cast<size_t> (i)], 1e-6f);

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, CompilesLetBindings)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor WithLet {
            input stream in;
            output stream out;
            process {
                let a = in * 2;
                let b = a + 1;
                out = b * 0.5;
            }
        }
        graph G { input stream x; output stream y; node w = WithLet; connection { x -> w.in; w.out -> y; } }
    )YDSP",
                                 "WithLet",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    auto input = makeRampCodegen (64);
    auto output = runKernel (kernel, input, 64);

    for (int i = 0; i < 64; ++i)
    {
        const auto x = input[static_cast<size_t> (i)];
        const auto expected = (x * 2.0f + 1.0f) * 0.5f;
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-5f);
    }

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, CompilesComparisonOperators)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor Compare {
            input stream in;
            output stream out;
            process {
                let eq = (in == 0) ? 1 : 0;
                let ne = (in != 0) ? 1 : 0;
                let lt = (in < 0) ? 1 : 0;
                let le = (in <= 0) ? 1 : 0;
                let gt = (in > 0) ? 1 : 0;
                let ge = (in >= 0) ? 1 : 0;
                out = eq + ne + lt + le + gt + ge;
            }
        }
        graph G { input stream x; output stream y; node c = Compare; connection { x -> c.in; c.out -> y; } }
    )YDSP",
                                 "Compare",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    std::vector<float> input (8);
    input[0] = -1.0f;
    input[1] = -0.1f;
    input[2] = 0.0f;
    input[3] = 0.0f;
    input[4] = 0.1f;
    input[5] = 1.0f;

    auto output = runKernel (kernel, input, 8);

    for (int i = 0; i < 6; ++i)
    {
        const auto x = input[static_cast<size_t> (i)];
        float expected = 0.0f;
        if (x == 0.0f)
            expected += 1.0f; // eq
        if (x != 0.0f)
            expected += 1.0f; // ne
        if (x < 0.0f)
            expected += 1.0f; // lt
        if (x <= 0.0f)
            expected += 1.0f; // le
        if (x > 0.0f)
            expected += 1.0f; // gt
        if (x >= 0.0f)
            expected += 1.0f; // ge
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-5f) << "at index " << i << " with x=" << x;
    }

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, CompilesLogicalOperators)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor Logic {
            input stream in;
            output stream out;
            process {
                let pos = (in > 0);
                let neg = (in < 0);
                out = ((pos && neg) ? 1 : 0) + ((pos || neg) ? 2 : 0) + ((!pos) ? 4 : 0);
            }
        }
        graph G { input stream x; output stream y; node l = Logic; connection { x -> l.in; l.out -> y; } }
    )YDSP",
                                 "Logic",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    std::vector<float> input (8, 0.0f);
    input[0] = 1.0f;  // pos=true, neg=false -> out = 0 + 2 + 0 = 2
    input[1] = -1.0f; // pos=false, neg=true -> out = 0 + 2 + 4 = 6
    input[2] = 0.0f;  // pos=false, neg=false -> out = 0 + 0 + 4 = 4

    auto output = runKernel (kernel, input, 8);

    EXPECT_NEAR (2.0f, output[0], 1e-5f); // positive
    EXPECT_NEAR (6.0f, output[1], 1e-5f); // negative
    EXPECT_NEAR (4.0f, output[2], 1e-5f); // zero

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, CompilesFmodIntrinsic)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor FmodTest { input stream in; output stream out; process { out = fmod (in, 2.0); } }
        graph G { input stream x; output stream y; node f = FmodTest; connection { x -> f.in; f.out -> y; } }
    )YDSP",
                                 "FmodTest",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    std::vector<float> input (64);
    for (int i = 0; i < 64; ++i)
        input[static_cast<size_t> (i)] = -6.0f + static_cast<float> (i) * 0.2f;

    auto output = runKernel (kernel, input, 64);

    for (int i = 0; i < 64; ++i)
    {
        const auto x = input[static_cast<size_t> (i)];
        const auto expected = fmodf (x, 2.0f);
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-4f);
    }

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, IntDivModByRuntimeZeroDivisorReturnsZero)
{
    YdspDiagnostics diagnostics;

    // The divisor is derived from the input stream (int (in)) rather than a
    // literal, so the optimizer cannot constant-fold the div/mod away - this
    // exercises the actual codegen div/mod-by-zero guard at runtime.
    auto kernel = compileKernel (R"YDSP(
        processor IntDivMod {
            input stream in;
            output stream out;
            process {
                let n = int (in);
                let q = 100 / n;
                let r = 100 % n;
                out = float (q) + float (r);
            }
        }
        graph G { input stream x; output stream y; node d = IntDivMod; connection { x -> d.in; d.out -> y; } }
    )YDSP",
                                 "IntDivMod",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    std::vector<float> input { -3.0f, 0.0f, 2.0f, 5.0f, -5.0f, 1.0f, -1.0f, 0.0f };
    auto output = runKernel (kernel, input, static_cast<int> (input.size()));

    for (size_t i = 0; i < input.size(); ++i)
    {
        const auto n = static_cast<int> (input[i]);
        const auto q = n != 0 ? 100 / n : 0;
        const auto r = n != 0 ? 100 % n : 0;
        const auto expected = static_cast<float> (q) + static_cast<float> (r);
        EXPECT_NEAR (expected, output[i], 1e-5f) << "at index " << i << " with n=" << n;
    }

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, IntDivModGuardsTheOverflowingPair)
{
    YdspDiagnostics diagnostics;

    // Stream operands keep division and remainder live at runtime.
    auto kernel = compileKernel (R"YDSP(
        processor IntDivMod2 {
            input stream num;
            input stream den;
            output stream quot;
            output stream rem;
            process {
                let a = int (num);
                let b = int (den);
                quot = float (a / b);
                rem = float (a % b);
            }
        }
        graph G {
            input stream x; input stream y;
            output stream p; output stream q;
            node d = IntDivMod2;
            connection { x -> d.num; y -> d.den; d.quot -> p; d.rem -> q; }
        }
    )YDSP",
                                 "IntDivMod2",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);
    ASSERT_EQ (2, kernel.numInputs);
    ASSERT_EQ (2, kernel.numOutputs);

    constexpr auto intMin = std::numeric_limits<int32_t>::min();

    const std::vector<int32_t> numerators { 100, -100, 100, -100, 100, intMin, intMin, 0 };
    const std::vector<int32_t> denominators { 7, 7, -7, -7, 0, -1, 1, 5 };

    const auto numSamples = static_cast<int> (numerators.size());

    // runKernel lays the streams out back to back: all of `num`, then all of `den`.
    std::vector<float> input;
    input.reserve (numerators.size() * 2);

    for (const auto value : numerators)
        input.push_back (static_cast<float> (value));

    for (const auto value : denominators)
        input.push_back (static_cast<float> (value));

    auto output = runKernel (kernel, input, numSamples);

    ASSERT_EQ (static_cast<size_t> (numSamples * 2), output.size());

    for (size_t i = 0; i < numerators.size(); ++i)
    {
        const auto a = numerators[i];
        const auto b = denominators[i];
        const bool defined = b != 0 && ! (a == intMin && b == -1);

        EXPECT_FLOAT_EQ (static_cast<float> (defined ? a / b : (b == -1 ? std::numeric_limits<int32_t>::max() : 0)), output[i])
            << "quotient at index " << i << " with a=" << a << " b=" << b;
        EXPECT_FLOAT_EQ (static_cast<float> (defined ? a % b : 0), output[numerators.size() + i])
            << "remainder at index " << i << " with a=" << a << " b=" << b;
    }

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, CompilesFloat64StreamsAndMath)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor F64Proc {
            input stream float64 in;
            output stream float64 out;
            state float64 acc;
            process {
                acc = acc * 0.9999 + in;
                out = acc;
            }
        }
        graph G { input stream float64 x; output stream float64 y; node p = F64Proc; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                                 "F64Proc",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);
    EXPECT_GT (kernel.stateBytes, 0u);

    // Build the full input sequence and run as a single block so state persists.
    const int totalSamples = 16;
    std::vector<double> input (static_cast<size_t> (totalSamples));
    for (int i = 0; i < totalSamples; ++i)
        input[static_cast<size_t> (i)] = 0.001 * static_cast<double> (i) - 0.01;

    std::vector<uint8_t> state (kernel.stateBytes, 0);
    std::vector<double> params (static_cast<size_t> (kernel.numParams), 0.0);
    std::vector<double> paramsOut (static_cast<size_t> (kernel.numParamsOut), 0.0);
    std::vector<double> output (static_cast<size_t> (totalSamples), 0.0);

    double* inPtr = input.data();
    double* outPtr = output.data();

    YdspKernelContext ctx;
    ctx.inputs = reinterpret_cast<void* const*> (&inPtr);
    ctx.outputs = reinterpret_cast<void* const*> (&outPtr);
    ctx.params = params.data();
    ctx.paramOut = paramsOut.data();
    ctx.state = state.data();
    ctx.stateArrays = state.empty() ? nullptr : state.data() + kernel.stateScalarBytes;
    ctx.sampleRate = 44100.0f;
    ctx.numSamples = totalSamples;

    kernel.fn (&ctx);

    double acc = 0.0;
    for (int i = 0; i < totalSamples; ++i)
    {
        acc = acc * 0.9999 + input[static_cast<size_t> (i)];
        EXPECT_NEAR (acc, output[static_cast<size_t> (i)], 1e-9) << "at sample " << i;
    }

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, AdaptsFloat64LiteralWithoutRoundingThroughFloat32)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor F64LiteralProc {
            input stream float64 in;
            output stream float64 scaled;
            output stream float64 flipped;
            process {
                scaled = in * 0.9999;
                flipped = in * (-0.9999);
            }
        }
        graph G {
            input stream float64 x;
            output stream float64 a;
            output stream float64 b;
            node p = F64LiteralProc;
            connection { x -> p.in; p.scaled -> a; p.flipped -> b; }
        }
    )YDSP",
                                 "F64LiteralProc",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    const std::vector<double> input { 1.0, -0.5, 0.1, 3.14159265358979, -2.718281828459045 };
    const auto output = runKernel64 (kernel, input, static_cast<int> (input.size()));

    ASSERT_EQ (input.size() * 2, output.size());

    // The literals adapt to the float64 context, so the constants keep the double
    // precision of the source values instead of being rounded through float32 first.
    for (size_t i = 0; i < input.size(); ++i)
    {
        EXPECT_DOUBLE_EQ (input[i] * 0.9999, output[i]) << "at sample " << i;
        EXPECT_DOUBLE_EQ (input[i] * -0.9999, output[input.size() + i]) << "at sample " << i;
    }

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, KeepsFloat32ValueRoundingWhenWidenedToFloat64)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor F64WidenProc {
            input stream float64 in;
            output stream float64 out;
            process {
                float narrow = 0.1;
                out = in + float64 (narrow);
            }
        }
        graph G { input stream float64 x; output stream float64 y; node p = F64WidenProc; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                                 "F64WidenProc",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    const std::vector<double> input { 1.0, -0.5, 0.1 };
    const auto output = runKernel64 (kernel, input, static_cast<int> (input.size()));

    ASSERT_EQ (input.size(), output.size());

    // `narrow` is float32, so widening it must reproduce its float32 value - only a
    // contextual float literal takes the precision of the surrounding expression.
    const auto narrow = static_cast<double> (static_cast<float> (0.1));

    for (size_t i = 0; i < input.size(); ++i)
        EXPECT_DOUBLE_EQ (input[i] + narrow, output[i]) << "at sample " << i;

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, CompilesInt64ArithmeticAndConversion)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor I64Proc {
            input stream in;
            output stream out;
            state int64 counter;
            process {
                counter = counter + 1000000000;
                out = float32 (float64 (counter) * 0.000001);
            }
        }
        graph G { input stream x; output stream y; node p = I64Proc; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                                 "I64Proc",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    std::vector<float> input (6, 0.0f);
    auto output = runKernel (kernel, input, 6);

    // counter grows by 1e9 per sample: 1000, 2000, ... 6000 (exceeds 2^31 at
    // sample 3, exercising the 64-bit addition).
    for (int i = 0; i < 6; ++i)
        EXPECT_NEAR (static_cast<float> ((i + 1) * 1000.0), output[static_cast<size_t> (i)], 1e-3f);

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, CompilesConversionRoundTripThroughFloat64)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor Conv {
            input stream float64 in64;
            output stream float64 out64;
            process {
                int64 j = int64 (in64);
                out64 = float64 (j);
            }
        }
        graph G { input stream float64 x; output stream float64 y; node c = Conv; connection { x -> c.in64; c.out64 -> y; } }
    )YDSP",
                                 "Conv",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    std::vector<double> input { 3.5, -2.7, 0.0, 123456789012345.0, -42.0, 1e6 };
    auto output = runKernel64 (kernel, input, static_cast<int> (input.size()));

    for (size_t i = 0; i < input.size(); ++i)
    {
        const auto j = static_cast<int64_t> (input[i]);
        const auto expected = static_cast<double> (j);
        EXPECT_NEAR (expected, output[i], 1e-3f) << "at index " << i;
    }

    dumpAsmOnFailureCodegen (kernel);
}

TEST (YdspAsmJitCodegenTests, EmitEventFromProcessCommitsOneEntryPerSample)
{
    YdspDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
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
    )YDSP",
                                 "P",
                                 diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    const int numSamples = 4;
    std::vector<float> input { 10.0f, 20.0f, 30.0f, 40.0f };
    std::vector<float> output (static_cast<size_t> (numSamples), 0.0f);
    std::vector<uint8_t> state (kernel.stateBytes, 0);
    std::vector<float> params (static_cast<size_t> (kernel.numParams), 0.0f);
    std::vector<float> paramsOut (static_cast<size_t> (kernel.numParamsOut), 0.0f);

    float* inPtr = input.data();
    float* outPtr = output.data();

    YdspOutputEventQueue queue;
    queue.entries.reserve (static_cast<size_t> (numSamples));

    YdspKernelContext ctx;
    ctx.inputs = reinterpret_cast<void* const*> (&inPtr);
    ctx.outputs = reinterpret_cast<void* const*> (&outPtr);
    ctx.params = params.data();
    ctx.paramOut = paramsOut.data();
    ctx.state = reinterpret_cast<float*> (state.data());
    ctx.stateArrays = state.empty() ? nullptr : state.data() + kernel.stateScalarBytes;
    ctx.sampleRate = 44100.0f;
    ctx.numSamples = numSamples;
    ctx.outputEvents = &queue;

    kernel.fn (&ctx);

    ASSERT_EQ (static_cast<size_t> (numSamples), queue.entries.size());

    for (int i = 0; i < numSamples; ++i)
    {
        const auto& entry = queue.entries[static_cast<size_t> (i)];
        EXPECT_EQ (i, entry.sampleOffset);
        EXPECT_EQ (0, entry.endpointIndex);
        EXPECT_EQ (static_cast<int64_t> (YdspEventShape::noteOn), entry.shapeTag);
        EXPECT_FLOAT_EQ (input[static_cast<size_t> (i)], entry.fields.pitch);
    }

    EXPECT_EQ (0u, queue.droppedCount.load());

    dumpAsmOnFailureCodegen (kernel);
}

//==============================================================================
// Event-handler codegen

namespace
{

/** Helper: runs the full pipeline and returns the compiled event handler. */
struct CompiledEventHandler
{
    YdspEventHandlerFn fn = nullptr;
    size_t stateBytes = 0;
    size_t stateScalarBytes = 0;
    int numParams = 0;
};

CompiledEventHandler compileEventHandlerFn (StringRef source, const char* handlerName, YdspDiagnostics& diagnostics)
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

    for (const auto& fn : ir->eventHandlers)
        if (fn->name == handlerName)
        {
            targetFn = fn.get();
            break;
        }

    if (targetFn == nullptr)
        return {};

    static asmjit::JitRuntime jitRuntime;
    auto handlerFn = YdspAsmJitCodegen::compileEventHandler (jitRuntime, *targetFn, diagnostics);

    if (handlerFn == nullptr || diagnostics.hasErrors())
        return {};

    return { handlerFn, YdspAsmJitCodegen::stateSize (*targetFn), YdspAsmJitCodegen::stateScalarSize (*targetFn), targetFn->numParams };
}

/** A zero-initialised event context wired to the handler's state and params, so
    a payload field the test does not set reads as zero rather than as garbage. */
YdspEventContext makeEventContext (const CompiledEventHandler& handler,
                                   std::vector<float>& state,
                                   std::vector<float>& params,
                                   float sampleRate = 44100.0f)
{
    YdspEventContext ctx {};
    ctx.state = state.data();
    ctx.stateArrays = state.data() + handler.stateScalarBytes / sizeof (float);
    ctx.params = params.data();
    ctx.sampleRate = sampleRate;

    return ctx;
}

} // namespace

TEST (YdspAsmJitCodegenTests, CompilesEventHandlerWritingState)
{
    YdspDiagnostics diagnostics;

    auto handler = compileEventHandlerFn (R"YDSP(
        processor Voice {
            output stream out;
            input event midi;
            state float freq;
            state float env;
            event midi (e: noteOn) {
                freq = e.pitch * 2.0;
                env  = e.velocity;
            }
            process { out = freq; }
        }
        graph G { input event midi; output stream y; node v = Voice; connection { midi -> v.midi; v.out -> y; } }
    )YDSP",
                                          "Voice.noteOn",
                                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, handler.fn);
    ASSERT_GE (handler.stateBytes, 8u); // two float32 scalars

    std::vector<float> state (handler.stateBytes / sizeof (float), 0.0f);
    std::vector<float> params (static_cast<size_t> (handler.numParams), 0.0f);

    auto ctx = makeEventContext (handler, state, params, 48000.0f);
    ctx.pitch = 69.0f;
    ctx.velocity = 0.5f;

    handler.fn (&ctx);

    // Slot indices follow the shared (kernel) layout: freq = 0, env = 1.
    EXPECT_FLOAT_EQ (69.0f * 2.0f, state[0]);
    EXPECT_FLOAT_EQ (0.5f, state[1]);
}

TEST (YdspAsmJitCodegenTests, CompilesEventHandlerReadingParam)
{
    YdspDiagnostics diagnostics;

    auto handler = compileEventHandlerFn (R"YDSP(
        processor Voice {
            output stream out;
            input event midi;
            input parameter float gain = 0.25;
            state float amp;
            event midi (e: noteOn) {
                amp = gain * e.velocity;
            }
            process { out = amp; }
        }
        graph G { input event midi; output stream y; node v = Voice; connection { midi -> v.midi; v.out -> y; } }
    )YDSP",
                                          "Voice.noteOn",
                                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, handler.fn);
    ASSERT_EQ (1, handler.numParams);

    std::vector<float> state (handler.stateBytes / sizeof (float), 0.0f);
    std::vector<float> params { 0.25f };

    auto ctx = makeEventContext (handler, state, params);
    ctx.pitch = 60.0f;
    ctx.velocity = 0.8f;

    handler.fn (&ctx);

    EXPECT_FLOAT_EQ (0.25f * 0.8f, state[0]);
}

TEST (YdspAsmJitCodegenTests, CompilesEventHandlerCallingFunc)
{
    YdspDiagnostics diagnostics;

    auto handler = compileEventHandlerFn (R"YDSP(
        processor Voice {
            output stream out;
            input event midi;
            state float freq;
            func noteToFreq (pitch: float) : float {
                return 440.0 * pow (2.0, (pitch - 69.0) / 12.0);
            }
            event midi (e: noteOn) {
                freq = noteToFreq (e.pitch);
            }
            process { out = freq; }
        }
        graph G { input event midi; output stream y; node v = Voice; connection { midi -> v.midi; v.out -> y; } }
    )YDSP",
                                          "Voice.noteOn",
                                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, handler.fn);

    std::vector<float> state (handler.stateBytes / sizeof (float), 0.0f);
    std::vector<float> params (static_cast<size_t> (handler.numParams), 0.0f);

    auto ctx = makeEventContext (handler, state, params);
    ctx.pitch = 69.0f;
    ctx.velocity = 1.0f;

    handler.fn (&ctx);

    // A4 (MIDI 69) maps to 440 Hz via the inlined func.
    EXPECT_NEAR (440.0f, state[0], 1e-3f);
}

TEST (YdspAsmJitCodegenTests, CompilesNoteOffHandler)
{
    YdspDiagnostics diagnostics;

    auto handler = compileEventHandlerFn (R"YDSP(
        processor Voice {
            output stream out;
            input event midi;
            state float env;
            event midi (e: noteOn) {
                env = 1.0;
            }
            event midi (e: noteOff) {
                env = 0.0;
            }
            process { out = env; }
        }
        graph G { input event midi; output stream y; node v = Voice; connection { midi -> v.midi; v.out -> y; } }
    )YDSP",
                                          "Voice.noteOff",
                                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, handler.fn);

    std::vector<float> state (handler.stateBytes / sizeof (float), 0.0f);
    std::vector<float> params (static_cast<size_t> (handler.numParams), 0.0f);

    auto ctx = makeEventContext (handler, state, params);
    ctx.pitch = 69.0f;
    ctx.velocity = 0.0f;

    handler.fn (&ctx);

    EXPECT_FLOAT_EQ (0.0f, state[0]);
}

namespace
{

/** One processor declaring a handler per shape, each storing its payload into
    the state slot of the same index - so the compiled handler for any shape can
    be run against a hand-built context and its field read back. */
constexpr const char* everyEventShapeSource = R"YDSP(
    processor Voice {
        output stream out;

        input event midi;

        state float a;
        state float b;
        state float c;

        event midi (e: noteOn) {
            a = e.pitch;
            b = e.velocity;
            if (e.isLegato) { c = 1.0; } else { c = -1.0; }
        }
        event midi (e: noteOff)       { a = e.pitch; b = e.velocity; }
        event midi (e: pitchBend)     { a = e.bendSemitones; }
        event midi (e: pressure)      { a = e.pressure; }
        event midi (e: slide)         { a = e.slide; }
        event midi (e: controlChange) { a = float (e.control); b = e.value; }
        event midi (e: programChange) { a = float (e.program); }

        process { out = a + b + c; }
    }
    graph G { input event midi; output stream y; node v = Voice; connection { midi -> v.midi; v.out -> y; } }
)YDSP";

} // namespace

TEST (YdspAsmJitCodegenTests, EachEventShapeReadsItsOwnPayloadFields)
{
    struct Case
    {
        const char* handlerName;
        void (*fill) (YdspEventContext&);
        float expectedA;
        float expectedB;
    };

    const Case cases[] = {
        { "Voice.noteOff", [] (YdspEventContext& ctx)
    {
        ctx.pitch = 48.0f;
        ctx.velocity = 0.75f;
    },
          48.0f,
          0.75f },
        { "Voice.pitchBend", [] (YdspEventContext& ctx)
    {
        ctx.bend = -3.5f;
    },
          -3.5f,
          0.0f },
        { "Voice.pressure", [] (YdspEventContext& ctx)
    {
        ctx.pressure = 0.625f;
    },
          0.625f,
          0.0f },
        { "Voice.slide", [] (YdspEventContext& ctx)
    {
        ctx.slide = 0.125f;
    },
          0.125f,
          0.0f },
        { "Voice.controlChange", [] (YdspEventContext& ctx)
    {
        ctx.index = 74;
        ctx.value = 0.5f;
    },
          74.0f,
          0.5f },
        { "Voice.programChange", [] (YdspEventContext& ctx)
    {
        ctx.index = 12;
    },
          12.0f,
          0.0f },
    };

    for (const auto& testCase : cases)
    {
        YdspDiagnostics diagnostics;

        auto handler = compileEventHandlerFn (everyEventShapeSource, testCase.handlerName, diagnostics);

        ASSERT_FALSE (diagnostics.hasErrors()) << testCase.handlerName << ": " << diagnostics.toString();
        ASSERT_NE (nullptr, handler.fn) << testCase.handlerName;

        std::vector<float> state (handler.stateBytes / sizeof (float), 0.0f);
        std::vector<float> params (static_cast<size_t> (handler.numParams), 0.0f);

        auto ctx = makeEventContext (handler, state, params);
        testCase.fill (ctx);

        handler.fn (&ctx);

        EXPECT_FLOAT_EQ (testCase.expectedA, state[0]) << testCase.handlerName;
        EXPECT_FLOAT_EQ (testCase.expectedB, state[1]) << testCase.handlerName;
    }
}

TEST (YdspAsmJitCodegenTests, IsLegatoReadsBitZeroOfTheEventFlags)
{
    for (const bool legato : { false, true })
    {
        YdspDiagnostics diagnostics;

        auto handler = compileEventHandlerFn (everyEventShapeSource, "Voice.noteOn", diagnostics);

        ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
        ASSERT_NE (nullptr, handler.fn);

        std::vector<float> state (handler.stateBytes / sizeof (float), 0.0f);
        std::vector<float> params (static_cast<size_t> (handler.numParams), 0.0f);

        auto ctx = makeEventContext (handler, state, params);
        ctx.pitch = 60.0f;
        ctx.velocity = 1.0f;
        ctx.flags = legato ? ydspEventFlagLegato : 0;

        handler.fn (&ctx);

        EXPECT_FLOAT_EQ (60.0f, state[0]);
        EXPECT_FLOAT_EQ (1.0f, state[1]);
        EXPECT_FLOAT_EQ (legato ? 1.0f : -1.0f, state[2]) << "legato " << legato;
    }
}

// A constant-bound loop inside an event handler, filling a state array from an
// event payload field and a transcendental. This is the shape an additive
// voice's noteOn uses to seed its oscillator bank.
TEST (YdspAsmJitCodegenTests, CompilesConstantBoundLoopInsideAnEventHandler)
{
    YdspDiagnostics diagnostics;

    auto handler = compileEventHandlerFn (R"YDSP(
        let partials = 4;

        processor Voice {
            output stream out;
            input event midi;

            state float amp[partials];
            state float mul[partials];

            event midi (e: noteOn) {
                for i in 0..partials {
                    let partial = float (i + 1);
                    amp[i] = e.velocity / partial;
                    mul[i] = cos (partial * 0.25);
                }
            }

            process { out = amp[0] * mul[0]; }
        }
        graph G { input event midi; output stream y; node v = Voice; connection { midi -> v.midi; v.out -> y; } }
    )YDSP",
                                          "Voice.noteOn",
                                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, handler.fn);

    std::vector<float> state (handler.stateBytes / sizeof (float), 0.0f);
    std::vector<float> params (1, 0.0f); // the processor declares no parameters

    auto ctx = makeEventContext (handler, state, params);
    ctx.velocity = 1.0f;

    handler.fn (&ctx);

    // No scalar state is declared, so the array segment starts at the base and
    // the two float arrays follow in declaration order.
    const auto* amp = state.data() + handler.stateScalarBytes / sizeof (float);
    const auto* mul = amp + 4;

    for (int i = 0; i < 4; ++i)
    {
        const auto partial = static_cast<float> (i + 1);

        EXPECT_FLOAT_EQ (1.0f / partial, amp[i]) << "amp[" << i << "]";
        EXPECT_FLOAT_EQ (std::cos (partial * 0.25f), mul[i]) << "mul[" << i << "]";
    }
}

//==============================================================================
// storeEventFieldF/I + emitEvent codegen

TEST (YdspAsmJitCodegenTests, EmitEventCommitsAnOutputEventEntry)
{
    YdspDiagnostics diagnostics;

    auto handler = compileEventHandlerFn (R"YDSP(
        processor Voice {
            output stream out;
            input event midi;
            output event noteOn;
            state float freq;

            event midi (e: noteOn) {
                freq = e.pitch;
                emit noteOn (pitch: e.pitch, velocity: e.velocity) -> noteOn;
            }

            process { out = freq; }
        }
        graph G { input event midi; output stream y; output event noteOn; node v = Voice; connection { midi -> v.midi; v.out -> y; v.noteOn -> noteOn; } }
    )YDSP",
                                          "Voice.noteOn",
                                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, handler.fn);

    std::vector<float> state (handler.stateBytes / sizeof (float), 0.0f);
    std::vector<float> params (static_cast<size_t> (handler.numParams), 0.0f);

    auto ctx = makeEventContext (handler, state, params);
    ctx.pitch = 60.0f;
    ctx.velocity = 0.8f;
    ctx.sampleOffset = 12;

    YdspOutputEventQueue queue;
    queue.entries.reserve (4);
    ctx.outputEvents = &queue;

    handler.fn (&ctx);

    EXPECT_FLOAT_EQ (60.0f, state[0]);

    ASSERT_EQ (1u, queue.entries.size());
    const auto& entry = queue.entries[0];
    EXPECT_EQ (12, entry.sampleOffset);
    EXPECT_EQ (0, entry.endpointIndex);
    EXPECT_EQ (static_cast<int64_t> (YdspEventShape::noteOn), entry.shapeTag);
    EXPECT_FLOAT_EQ (60.0f, entry.fields.pitch);
    EXPECT_FLOAT_EQ (0.8f, entry.fields.velocity);
    EXPECT_EQ (0u, queue.droppedCount.load());
}

TEST (YdspAsmJitCodegenTests, EmitEventDropsWhenTheQueueIsAtCapacity)
{
    YdspDiagnostics diagnostics;

    auto handler = compileEventHandlerFn (R"YDSP(
        processor Voice {
            output stream out;
            input event midi;
            output event noteOn;
            state float freq;

            event midi (e: noteOn) {
                freq = e.pitch;
                emit noteOn (pitch: e.pitch, velocity: e.velocity) -> noteOn;
            }

            process { out = freq; }
        }
        graph G { input event midi; output stream y; output event noteOn; node v = Voice; connection { midi -> v.midi; v.out -> y; v.noteOn -> noteOn; } }
    )YDSP",
                                          "Voice.noteOn",
                                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, handler.fn);

    std::vector<float> state (handler.stateBytes / sizeof (float), 0.0f);
    std::vector<float> params (static_cast<size_t> (handler.numParams), 0.0f);

    auto ctx = makeEventContext (handler, state, params);
    ctx.pitch = 60.0f;
    ctx.velocity = 0.8f;

    YdspOutputEventQueue queue; // capacity 0: never reserved
    ctx.outputEvents = &queue;

    handler.fn (&ctx);

    EXPECT_EQ (0u, queue.entries.size());
    EXPECT_EQ (1u, queue.droppedCount.load());
}

//==============================================================================
// x86-64 codegen parity: branchless advanceWrap, end-relative loops, flag reuse,
// VEX 4-lane vectors, xorps/vxorps zeroing, and 2-address / scalar FMA coalescing

TEST (YdspAsmJitCodegenTests, BranchlessAdvanceWrapOnX64)
{
    YdspDiagnostics diagnostics;
    const auto source = R"YDSP(
        processor RingWrap {
            input stream in; output stream out;
            state float buf[5000]; state int wp;
            process {
                out = buf[wp];
                buf[wp] = in;
                wp = wp + 1;
                if (wp >= 5000) { wp = 0; }
            }
        }
        graph G { input stream x; output stream y; node p = RingWrap; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    YdspLexer lexer (source, diagnostics);
    YdspParser parser (lexer.tokenize(), diagnostics);
    auto parsed = parser.parseProgram();
    ASSERT_NE (nullptr, parsed) << diagnostics.toString();

    YdspSemanticAnalyzer analyzer (diagnostics);
    auto analyzed = analyzer.analyze (std::move (parsed));
    ASSERT_NE (nullptr, analyzed) << diagnostics.toString();

    YdspOptimizer optimizer (diagnostics);
    auto ir = optimizer.build (*analyzed);
    ASSERT_NE (nullptr, ir) << diagnostics.toString();
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels.front();

    {
        YdspDiagnostics x64Diagnostics;
        const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::x64 }, fn, x64Diagnostics);
        ASSERT_TRUE (artifact.wasOk()) << x64Diagnostics.toString();
        const auto listing = x64Diagnostics.toString();

        EXPECT_TRUE (listing.contains ("cmovnl ")) << listing;
        EXPECT_TRUE (listing.contains (", 5000")) << listing;
        const auto loopBodyStart = listing.indexOf ("L4:");
        const auto loopLatchStart = listing.indexOf ("L5:");
        ASSERT_GE (loopBodyStart, 0);
        ASSERT_GT (loopLatchStart, loopBodyStart);
        EXPECT_FALSE (listing.substring (loopBodyStart, loopLatchStart).contains ("jl ")) << listing;
    }

    // Verify execution equivalence across wrap boundaries (including dst != value).
    const auto kernel = compileKernel (source, "RingWrap", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, kernel.fn);

    constexpr int numSamples = 10050;
    const auto input = makeRampCodegen (numSamples, 1.0f);
    const auto output = runKernel (kernel, input, numSamples);

    for (int i = 0; i < numSamples; ++i)
        ASSERT_FLOAT_EQ (i < 5000 ? 0.0f : input[static_cast<size_t> (i - 5000)], output[static_cast<size_t> (i)]) << i;
}

TEST (YdspAsmJitCodegenTests, EndRelativeLoopAndFlagReuseOnX64)
{
    auto buildIr = [] (StringRef source, YdspDiagnostics& diagnostics) -> std::unique_ptr<YdspIrProgram>
    {
        YdspLexer lexer (source, diagnostics);
        YdspParser parser (lexer.tokenize(), diagnostics);
        auto parsed = parser.parseProgram();
        if (parsed == nullptr)
            return nullptr;
        YdspSemanticAnalyzer analyzer (diagnostics);
        auto analyzed = analyzer.analyze (std::move (parsed));
        if (analyzed == nullptr)
            return nullptr;
        YdspOptimizer optimizer (diagnostics);
        return optimizer.build (*analyzed);
    };

    // 1. Sample loop closes on its own increment without a latch compare.
    {
        YdspDiagnostics diagnostics;
        const auto source = R"YDSP(
            processor Counter {
                input stream in; output stream out; state float sum;
                process { sum = sum + in; out = sum; }
            }
            graph G { input stream x; output stream y; node p = Counter; connection { x -> p.in; p.out -> y; } }
        )YDSP";
        auto ir = buildIr (source, diagnostics);
        ASSERT_NE (nullptr, ir) << diagnostics.toString();
        ASSERT_FALSE (ir->kernels.empty());

        YdspDiagnostics x64Diagnostics;
        const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::x64 }, *ir->kernels.front(), x64Diagnostics);
        ASSERT_TRUE (artifact.wasOk()) << x64Diagnostics.toString();
        const auto listing = x64Diagnostics.toString();

        int compares = 0;
        for (const auto& line : StringArray::fromLines (listing))
            compares += line.trimStart().startsWith ("cmp ") ? 1 : 0;

        EXPECT_EQ (1, compares) << listing;
        EXPECT_TRUE (listing.contains ("neg ")) << listing;
        EXPECT_TRUE (listing.contains ("jnz ")) << listing;

        const auto kernel = compileKernel (source, "Counter", diagnostics);
        ASSERT_NE (nullptr, kernel.fn);
        const auto input = makeRampCodegen (37, 0.5f);
        const auto output = runKernel (kernel, input, 37);
        float sum = 0.0f;
        for (size_t i = 0; i < output.size(); ++i)
            EXPECT_EQ (sum += input[i], output[i]) << i;
    }

    // 2. Hand-written ring wrap reuses sub flags and preserves flags across + 5000 via lea.
    {
        YdspDiagnostics diagnostics;
        const auto source = R"YDSP(
            processor Ring {
                input stream in; output stream out;
                state float buf[5000]; state int wp;
                process {
                    int r = wp - 300;
                    if (r < 0) { r = r + 5000; }
                    out = buf[r];
                    buf[wp] = in;
                    wp = wp + 1;
                    if (wp >= 5000) { wp = 0; }
                }
            }
            graph G { input stream x; output stream y; node p = Ring; connection { x -> p.in; p.out -> y; } }
        )YDSP";
        auto ir = buildIr (source, diagnostics);
        ASSERT_NE (nullptr, ir) << diagnostics.toString();
        ASSERT_FALSE (ir->kernels.empty());

        YdspDiagnostics x64Diagnostics;
        const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::x64 }, *ir->kernels.front(), x64Diagnostics);
        ASSERT_TRUE (artifact.wasOk()) << x64Diagnostics.toString();
        const auto listing = x64Diagnostics.toString();

        EXPECT_TRUE (listing.contains ("cmovs ")) << listing;
        EXPECT_TRUE (listing.contains ("lea ") && listing.contains ("+5000]")) << listing;
    }
}

TEST (YdspAsmJitCodegenTests, ZeroFloatConstUsesXorpsAndVxorpsOnX64)
{
    for (const bool avx : { false, true })
    {
        SCOPED_TRACE (avx);
        YdspIrFunction fn;
        fn.name = "ZeroFloatConsts";
        fn.vectorWidth = avx ? 8 : 4;
        fn.packedFmaAvailable = avx;
        fn.float32ArrayElements = 4;
        fn.float64ArrayElements = 4;
        fn.valueTypes = {
            YdspValueType::int32Type,
            YdspValueType::int32Type,
            YdspValueType::float32Type,
            YdspValueType::float32Type,
            YdspValueType::float64Type,
            YdspValueType::float64Type
        };
        fn.blocks.resize (1);
        fn.blocks[0].insts = {
            { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
            { YdspIrOp::constI, 1, -1, -1, -1, -1, 0.0, 1 },
            { YdspIrOp::constF, 2, -1, -1, -1, -1, 0.0 },
            { YdspIrOp::constF, 3, -1, -1, -1, -1, -0.0 },
            { YdspIrOp::constF, 4, -1, -1, -1, -1, 0.0 },
            { YdspIrOp::constF, 5, -1, -1, -1, -1, -0.0 },
            { YdspIrOp::storeStateArrayF, -1, 0, 2, -1, 0 },
            { YdspIrOp::storeStateArrayF, -1, 1, 3, -1, 0 },
            { YdspIrOp::storeStateArrayF, -1, 0, 4, -1, 0 },
            { YdspIrOp::storeStateArrayF, -1, 1, 5, -1, 0 }
        };

        YdspDiagnostics diagnostics;
        const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::x64 }, fn, diagnostics);
        ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
        const auto listing = diagnostics.toString();

        if (avx)
        {
            EXPECT_TRUE (listing.contains ("vxorps ")) << listing;
            EXPECT_TRUE (listing.contains ("vxorpd ")) << listing;
        }
        else
        {
            EXPECT_TRUE (listing.contains ("xorps ")) << listing;
            EXPECT_TRUE (listing.contains ("xorpd ")) << listing;
            EXPECT_FALSE (listing.contains ("vxorps ")) << listing;
            EXPECT_FALSE (listing.contains ("vxorpd ")) << listing;
        }

        // Negative zero still loads its sign bit from the constant pool.
        EXPECT_TRUE (listing.contains ("movss ")) << listing;
        EXPECT_TRUE (listing.contains ("movsd ")) << listing;
    }

    // Verify +0.0 vs -0.0 signbit preservation on host execution.
    YdspIrFunction runtimeFn;
    runtimeFn.name = "ZeroFloatRuntime";
    runtimeFn.float32ArrayElements = 4;
    runtimeFn.float64ArrayElements = 4;
    runtimeFn.valueTypes = {
        YdspValueType::int32Type,
        YdspValueType::int32Type,
        YdspValueType::float32Type,
        YdspValueType::float32Type,
        YdspValueType::float64Type,
        YdspValueType::float64Type
    };
    runtimeFn.blocks.resize (1);
    runtimeFn.blocks[0].insts = {
        { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
        { YdspIrOp::constI, 1, -1, -1, -1, -1, 0.0, 1 },
        { YdspIrOp::constF, 2, -1, -1, -1, -1, 0.0 },
        { YdspIrOp::constF, 3, -1, -1, -1, -1, -0.0 },
        { YdspIrOp::constF, 4, -1, -1, -1, -1, 0.0 },
        { YdspIrOp::constF, 5, -1, -1, -1, -1, -0.0 },
        { YdspIrOp::storeStateArrayF, -1, 0, 2, -1, 0 },
        { YdspIrOp::storeStateArrayF, -1, 1, 3, -1, 0 },
        { YdspIrOp::storeStateArrayF, -1, 0, 4, -1, 0 },
        { YdspIrOp::storeStateArrayF, -1, 1, 5, -1, 0 }
    };

    YdspDiagnostics diagnostics;
    asmjit::JitRuntime runtime;
    const auto compiled = YdspAsmJitCodegen::compile (runtime, runtimeFn, diagnostics);
    ASSERT_NE (nullptr, compiled) << diagnostics.toString();

    std::vector<uint8_t> state (YdspAsmJitCodegen::stateSize (runtimeFn), 0xff);
    YdspKernelContext ctx;
    ctx.numSamples = 1;
    ctx.state = state.data();
    ctx.stateArrays = state.data() + YdspAsmJitCodegen::stateScalarSize (runtimeFn);
    compiled (&ctx);

    float fPos = 1.0f;
    float fNeg = 1.0f;
    double dPos = 1.0;
    double dNeg = 1.0;
    const auto* arrays = static_cast<const uint8_t*> (ctx.stateArrays);
    std::memcpy (&fPos, arrays + runtimeFn.stateArrayByteOffset (YdspValueType::float32Type, 0), sizeof (float));
    std::memcpy (&fNeg, arrays + runtimeFn.stateArrayByteOffset (YdspValueType::float32Type, 1), sizeof (float));
    std::memcpy (&dPos, arrays + runtimeFn.stateArrayByteOffset (YdspValueType::float64Type, 0), sizeof (double));
    std::memcpy (&dNeg, arrays + runtimeFn.stateArrayByteOffset (YdspValueType::float64Type, 1), sizeof (double));

    EXPECT_EQ (0.0f, fPos);
    EXPECT_FALSE (std::signbit (fPos));
    EXPECT_EQ (0.0f, fNeg);
    EXPECT_TRUE (std::signbit (fNeg));
    EXPECT_EQ (0.0, dPos);
    EXPECT_FALSE (std::signbit (dPos));
    EXPECT_EQ (0.0, dNeg);
    EXPECT_TRUE (std::signbit (dNeg));
}

TEST (YdspAsmJitCodegenTests, FourLaneVectorLoweringRespectsSse2VsAvxOnX64)
{
    for (const bool avx : { false, true })
    {
        SCOPED_TRACE (avx);
        YdspIrFunction fn;
        fn.name = "FourLaneVectors";
        fn.vectorized = true;
        fn.vectorWidth = 4;
        fn.packedFmaAvailable = avx;
        fn.float32ArrayElements = 16;
        fn.valueTypes = {
            YdspValueType::int32Type,
            YdspValueType::float32Type,
            YdspValueType::float32Type,
            YdspValueType::float32Type,
            YdspValueType::float32Type,
            YdspValueType::float32Type,
            YdspValueType::float32Type,
            YdspValueType::float32Type,
            YdspValueType::float32Type,
            YdspValueType::float32Type,
            YdspValueType::float32Type,
            YdspValueType::boolType,
            YdspValueType::float32Type
        };
        fn.valueLanes = { 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 };
        fn.blocks.resize (1);
        fn.blocks[0].insts = {
            { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
            { YdspIrOp::loadStateArrayF, 1, 0, -1, -1, 0 },
            { YdspIrOp::loadStateArrayF, 2, 0, -1, -1, 4 },
            { YdspIrOp::addF, 3, 1, 2 },
            { YdspIrOp::subF, 4, 3, 2 },
            { YdspIrOp::mulF, 5, 4, 1 },
            { YdspIrOp::divF, 6, 5, 2 },
            { YdspIrOp::minF, 7, 6, 1 },
            { YdspIrOp::maxF, 8, 7, 2 },
            { YdspIrOp::sqrtF, 9, 8 },
            { YdspIrOp::floorF, 10, 9 },
            { YdspIrOp::ltF, 11, 10, 1 },
            { YdspIrOp::selectB, 12, 11, 10, 2 },
            { YdspIrOp::storeStateArrayF, -1, 0, 12, -1, 8 }
        };

        YdspDiagnostics diagnostics;
        const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::x64 }, fn, diagnostics);
        ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
        const auto listing = diagnostics.toString();

        if (avx)
        {
            EXPECT_TRUE (listing.contains ("vaddps ")) << listing;
            EXPECT_TRUE (listing.contains ("vsubps ")) << listing;
            EXPECT_TRUE (listing.contains ("vmulps ")) << listing;
            EXPECT_TRUE (listing.contains ("vdivps ")) << listing;
            EXPECT_TRUE (listing.contains ("vminps ")) << listing;
            EXPECT_TRUE (listing.contains ("vmaxps ")) << listing;
            EXPECT_TRUE (listing.contains ("vsqrtps ")) << listing;
            EXPECT_TRUE (listing.contains ("vroundps ")) << listing;
            EXPECT_TRUE (listing.contains ("vcmpps ")) << listing;
            EXPECT_TRUE (listing.contains ("vblendvps ")) << listing;
        }
        else
        {
            EXPECT_TRUE (listing.contains ("addps ")) << listing;
            EXPECT_TRUE (listing.contains ("andps ")) << listing;
            EXPECT_TRUE (listing.contains ("andnps ")) << listing;
            EXPECT_TRUE (listing.contains ("orps ")) << listing;
            EXPECT_FALSE (listing.contains ("vaddps ")) << listing;
            EXPECT_FALSE (listing.contains ("vblendvps ")) << listing;
            EXPECT_FALSE (listing.contains ("blendvps ")) << listing;
            EXPECT_FALSE (listing.contains ("vcmpps ")) << listing;
            EXPECT_FALSE (listing.contains ("vroundps ")) << listing;
            EXPECT_FALSE (listing.contains ("vzeroupper")) << listing;
        }
    }

    // Execution equivalence between scalar baseline, SSE2 baseline, and host target.
    const auto source = R"YDSP(
        processor VecSelect {
            input stream in; output stream out;
            process {
                float x = abs (in) + 0.25;
                float s = sqrt (x);
                float f = floor (s * 4.0) * 0.25;
                out = f < 1.0 ? f : min (s, 2.0);
            }
        }
        graph G { input stream x; output stream y; node p = VecSelect; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    YdspCompiler compiler;
    YdspCompileOptions scalarOpts;
    scalarOpts.targetPolicy = YdspTargetPolicy::baseline;
    scalarOpts.baselineTarget = YdspNativeTarget::scalar;
    scalarOpts.optimizationTier = YdspOptimizationTier::baseline;

    YdspCompileOptions hostOpts;
    hostOpts.targetPolicy = YdspTargetPolicy::host;

    auto scalarGraph = compiler.compile (source, scalarOpts);
    ASSERT_TRUE (scalarGraph.wasOk()) << scalarGraph.getErrorMessage();
    auto hostGraph = compiler.compile (source, hostOpts);
    ASSERT_TRUE (hostGraph.wasOk()) << hostGraph.getErrorMessage();

    constexpr int numSamples = 64;
    ASSERT_TRUE (scalarGraph.getReference().prepare (48000.0, numSamples).wasOk());
    ASSERT_TRUE (hostGraph.getReference().prepare (48000.0, numSamples).wasOk());

    auto input = makeRampCodegen (numSamples, -1.5f);
    std::vector<float> scalarOut (numSamples, 0.0f);
    std::vector<float> hostOut (numSamples, 0.0f);

    std::vector<YdspInputBuffer> inBufs;
    inBufs.emplace_back (Span<const float> (input.data(), static_cast<size_t> (numSamples)));
    std::vector<YdspOutputBuffer> scalarOutBufs;
    scalarOutBufs.emplace_back (Span<float> (scalarOut.data(), static_cast<size_t> (numSamples)));
    std::vector<YdspOutputBuffer> hostOutBufs;
    hostOutBufs.emplace_back (Span<float> (hostOut.data(), static_cast<size_t> (numSamples)));

    scalarGraph.getReference().process (YdspProcessRequest { inBufs, scalarOutBufs, numSamples });
    hostGraph.getReference().process (YdspProcessRequest { inBufs, hostOutBufs, numSamples });

    for (int i = 0; i < numSamples; ++i)
        EXPECT_NEAR (scalarOut[static_cast<size_t> (i)], hostOut[static_cast<size_t> (i)], 1e-6f) << i;
}

TEST (YdspAsmJitCodegenTests, TwoAddressAndScalarFmaCoalescingAvoidsRedundantMovesOnX64)
{
    // Scalar FMA coalescing on dead local addend c -> vfmadd231ss / vfnmadd231ss without movss.
    for (const auto op : { YdspIrOp::fmaF, YdspIrOp::fmsubF })
    {
        SCOPED_TRACE (static_cast<int> (op));
        YdspIrFunction fn;
        fn.name = "ScalarFmaCoalesce";
        fn.float32ArrayElements = 8;
        fn.valueTypes = {
            YdspValueType::int32Type,
            YdspValueType::float32Type,
            YdspValueType::float32Type,
            YdspValueType::float32Type,
            YdspValueType::float32Type,
            YdspValueType::float32Type,
            YdspValueType::float32Type,
            YdspValueType::float32Type
        };
        fn.blocks.resize (1);
        fn.blocks[0].insts = {
            { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
            { YdspIrOp::loadStateArrayF, 1, 0, -1, -1, 0 },
            { YdspIrOp::loadStateArrayF, 2, 0, -1, -1, 1 },
            { YdspIrOp::loadStateArrayF, 3, 0, -1, -1, 2 },
            { YdspIrOp::loadStateArrayF, 4, 0, -1, -1, 3 },
            { YdspIrOp::loadStateArrayF, 5, 0, -1, -1, 4 },
            { YdspIrOp::mulF, 6, 1, 2 },
            { op, 7, 3, 4, 6 },
            { YdspIrOp::storeStateArrayF, -1, 0, 7, -1, 5 }
        };

        YdspDiagnostics diagnostics;
        const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::x64 }, fn, diagnostics);
        ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
        const auto listing = diagnostics.toString();

        EXPECT_TRUE (listing.contains (op == YdspIrOp::fmaF ? "vfmadd231ss " : "vfnmadd231ss ")) << listing;

        // Only memory loads/stores should use movss; no register-to-register movss.
        for (const auto& line : StringArray::fromLines (listing))
        {
            const auto inst = line.trimStart();
            if (inst.startsWith ("movss "))
                EXPECT_TRUE (inst.contains ("[")) << listing;
        }
    }

    // Chained 2-address scalar float ops reuse srcA without movss copies.
    {
        YdspIrFunction fn;
        fn.name = "TwoAddressChain";
        fn.float32ArrayElements = 8;
        fn.valueTypes = {
            YdspValueType::int32Type,
            YdspValueType::float32Type,
            YdspValueType::float32Type,
            YdspValueType::float32Type,
            YdspValueType::float32Type
        };
        fn.blocks.resize (1);
        fn.blocks[0].insts = {
            { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
            { YdspIrOp::loadStateArrayF, 1, 0, -1, -1, 0 },
            { YdspIrOp::loadStateArrayF, 2, 0, -1, -1, 1 },
            { YdspIrOp::addF, 3, 1, 2 },
            { YdspIrOp::mulF, 4, 3, 2 },
            { YdspIrOp::storeStateArrayF, -1, 0, 4, -1, 2 }
        };

        YdspDiagnostics diagnostics;
        const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::x64 }, fn, diagnostics);
        ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
        const auto listing = diagnostics.toString();

        for (const auto& line : StringArray::fromLines (listing))
        {
            const auto inst = line.trimStart();
            if (inst.startsWith ("movss "))
                EXPECT_TRUE (inst.contains ("[")) << listing;
        }

        // Verify numerical equivalence on host.
        asmjit::JitRuntime runtime;
        const auto compiled = YdspAsmJitCodegen::compile (runtime, fn, diagnostics);
        ASSERT_NE (nullptr, compiled) << diagnostics.toString();

        std::vector<float> state (8, 0.0f);
        state[0] = 3.5f;
        state[1] = 2.0f;
        YdspKernelContext ctx;
        ctx.numSamples = 1;
        ctx.state = state.data();
        ctx.stateArrays = state.data();
        compiled (&ctx);
        EXPECT_FLOAT_EQ ((3.5f + 2.0f) * 2.0f, state[2]);
    }
}

TEST (YdspAsmJitCodegenTests, ZeroFloatConstAndVectorZeroSplatUseFmovAndMoviOnArm64)
{
    YdspIrFunction fn;
    fn.name = "Arm64ZeroConsts";
    fn.vectorized = true;
    fn.vectorWidth = 4;
    fn.float32ArrayElements = 16;
    fn.float64ArrayElements = 4;
    fn.int32ArrayElements = 8;
    fn.valueTypes = {
        YdspValueType::int32Type,
        YdspValueType::int32Type,
        YdspValueType::float32Type,
        YdspValueType::float32Type,
        YdspValueType::float64Type,
        YdspValueType::float64Type,
        YdspValueType::float32Type,
        YdspValueType::float32Type,
        YdspValueType::int32Type,
        YdspValueType::int32Type
    };
    fn.valueLanes = { 1, 1, 1, 1, 1, 1, 1, 4, 1, 4 };
    fn.blocks.resize (1);
    fn.blocks[0].insts = {
        { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
        { YdspIrOp::constI, 1, -1, -1, -1, -1, 0.0, 1 },
        { YdspIrOp::constF, 2, -1, -1, -1, -1, 0.0 },
        { YdspIrOp::constF, 3, -1, -1, -1, -1, -0.0 },
        { YdspIrOp::constF, 4, -1, -1, -1, -1, 0.0 },
        { YdspIrOp::constF, 5, -1, -1, -1, -1, -0.0 },
        { YdspIrOp::constF, 6, -1, -1, -1, -1, 0.0 },
        { YdspIrOp::vsplat, 7, 6 },
        { YdspIrOp::constI, 8, -1, -1, -1, -1, 0.0, 0 },
        { YdspIrOp::vsplat, 9, 8 },
        { YdspIrOp::storeStateArrayF, -1, 0, 2, -1, 0 },
        { YdspIrOp::storeStateArrayF, -1, 1, 3, -1, 0 },
        { YdspIrOp::storeStateArrayF, -1, 0, 4, -1, 0 },
        { YdspIrOp::storeStateArrayF, -1, 1, 5, -1, 0 },
        { YdspIrOp::storeStateArrayF, -1, 0, 7, -1, 4 },
        { YdspIrOp::storeStateArrayI, -1, 0, 9, -1, 0 }
    };

    YdspDiagnostics diagnostics;
    const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::arm64 }, fn, diagnostics);
    ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    const auto listing = diagnostics.toString();

    bool sawFmovWzr = false;
    bool sawFmovXzr = false;
    int moviZeroCount = 0;
    int scalarPoolLoads = 0;

    for (const auto& line : StringArray::fromLines (listing))
    {
        const auto inst = line.trimStart();
        if (inst.startsWith ("fmov ") && inst.contains ("wzr"))
            sawFmovWzr = true;
        if (inst.startsWith ("fmov ") && inst.contains ("xzr"))
            sawFmovXzr = true;
        if (inst.startsWith ("movi ") && inst.contains (".16b, 0"))
            ++moviZeroCount;
        if ((inst.startsWith ("ldr s") || inst.startsWith ("ldr d")) && inst.contains ("[L"))
            ++scalarPoolLoads;
        EXPECT_FALSE (inst.startsWith ("dup ")) << listing;
    }

    EXPECT_TRUE (sawFmovWzr) << listing;
    EXPECT_TRUE (sawFmovXzr) << listing;
    EXPECT_EQ (2, moviZeroCount) << listing;
    EXPECT_EQ (2, scalarPoolLoads) << listing;

    // Verify +0.0 / -0.0 signbit preservation and zeroed vectors on host.
    asmjit::JitRuntime runtime;
    const auto compiled = YdspAsmJitCodegen::compile (runtime, fn, diagnostics);
    ASSERT_NE (nullptr, compiled) << diagnostics.toString();

    std::vector<uint8_t> state (YdspAsmJitCodegen::stateSize (fn), 0xff);
    YdspKernelContext ctx;
    ctx.numSamples = 1;
    ctx.state = state.data();
    ctx.stateArrays = state.data() + YdspAsmJitCodegen::stateScalarSize (fn);
    compiled (&ctx);

    const auto* arrays = static_cast<const uint8_t*> (ctx.stateArrays);
    float fPos = 1.0f;
    float fNeg = 1.0f;
    double dPos = 1.0;
    double dNeg = 1.0;
    std::memcpy (&fPos, arrays + fn.stateArrayByteOffset (YdspValueType::float32Type, 0), sizeof (float));
    std::memcpy (&fNeg, arrays + fn.stateArrayByteOffset (YdspValueType::float32Type, 1), sizeof (float));
    std::memcpy (&dPos, arrays + fn.stateArrayByteOffset (YdspValueType::float64Type, 0), sizeof (double));
    std::memcpy (&dNeg, arrays + fn.stateArrayByteOffset (YdspValueType::float64Type, 1), sizeof (double));

    EXPECT_EQ (0.0f, fPos);
    EXPECT_FALSE (std::signbit (fPos));
    EXPECT_EQ (0.0f, fNeg);
    EXPECT_TRUE (std::signbit (fNeg));
    EXPECT_EQ (0.0, dPos);
    EXPECT_FALSE (std::signbit (dPos));
    EXPECT_EQ (0.0, dNeg);
    EXPECT_TRUE (std::signbit (dNeg));

    for (int lane = 0; lane < 4; ++lane)
    {
        float vf = 1.0f;
        int32_t vi = 1;
        std::memcpy (&vf, arrays + fn.stateArrayByteOffset (YdspValueType::float32Type, 4 + lane), sizeof (float));
        std::memcpy (&vi, arrays + fn.stateArrayByteOffset (YdspValueType::int32Type, lane), sizeof (int32_t));
        EXPECT_EQ (0.0f, vf);
        EXPECT_FALSE (std::signbit (vf));
        EXPECT_EQ (0, vi);
    }
}

TEST (YdspAsmJitCodegenTests, VrampFConstantIsHoistedOutOfLoopsOnArm64)
{
    YdspIrFunction fn;
    fn.name = "HoistedVrampLoop";
    fn.vectorized = true;
    fn.vectorWidth = 4;
    fn.float32ArrayElements = 16;
    fn.valueTypes = {
        YdspValueType::int32Type, // 0: zero
        YdspValueType::int32Type, // 1: bound (16)
        YdspValueType::int32Type, // 2: step (4)
        YdspValueType::int32Type, // 3: induction i
        YdspValueType::boolType,  // 4: i < 16
        YdspValueType::float32Type, // 5: float(i)
        YdspValueType::float32Type, // 6: vrampF(float(i))
        YdspValueType::int32Type, // 7: i + 4
        YdspValueType::float32Type // 8: stored / reduced
    };
    fn.valueLanes = { 1, 1, 1, 1, 1, 1, 4, 1, 4 };
    fn.blocks.resize (4);

    // Block 0: preheader
    fn.blocks[0].insts = {
        { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
        { YdspIrOp::constI, 1, -1, -1, -1, -1, 0.0, 16 },
        { YdspIrOp::constI, 2, -1, -1, -1, -1, 0.0, 4 },
        { YdspIrOp::movI, 3, 0 }
    };
    fn.blocks[0].term = YdspIrTerm::fallthrough;

    // Block 1: header
    fn.blocks[1].insts = {
        { YdspIrOp::ltI, 4, 3, 1 }
    };
    fn.blocks[1].term = YdspIrTerm::branchIf;
    fn.blocks[1].termCond = 4;
    fn.blocks[1].termTarget = 2;
    fn.blocks[1].termTarget2 = 3;

    // Block 2: body
    fn.blocks[2].insts = {
        { YdspIrOp::itof, 5, 3 },
        { YdspIrOp::vrampF, 6, 5 },
        { YdspIrOp::storeStateArrayF, -1, 3, 6, -1, 0 },
        { YdspIrOp::addI, 7, 3, 2 },
        { YdspIrOp::movI, 3, 7 }
    };
    fn.blocks[2].term = YdspIrTerm::branch;
    fn.blocks[2].termTarget = 1;

    // Block 3: exit
    fn.blocks[3].term = YdspIrTerm::fallthrough;

    YdspIrLoop loop;
    loop.id = 0;
    loop.headerBlock = 1;
    loop.exitBlock = 3;
    loop.induction = 3;
    loop.bound.kind = YdspLoopBoundKind::constant;
    loop.bound.constant = 16;
    fn.loops.push_back (loop);

    YdspDiagnostics diagnostics;
    const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::arm64 }, fn, diagnostics);
    ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    const auto listing = diagnostics.toString();

    int rampConstLoads = 0;
    int lastRampLoadLine = -1;
    int firstVectorAddLine = -1;
    const auto lines = StringArray::fromLines (listing);

    for (int i = 0; i < lines.size(); ++i)
    {
        const auto inst = lines[i].trimStart();
        if (inst.startsWith ("ldr q") && inst.contains ("[L"))
        {
            ++rampConstLoads;
            lastRampLoadLine = i;
        }
        if (firstVectorAddLine < 0 && inst.startsWith ("fadd ") && inst.contains (".4s"))
            firstVectorAddLine = i;
    }

    EXPECT_EQ (1, rampConstLoads) << listing;
    EXPECT_GE (lastRampLoadLine, 0) << listing;
    EXPECT_GT (firstVectorAddLine, lastRampLoadLine) << listing;

    // Verify execution on host.
    asmjit::JitRuntime runtime;
    const auto compiled = YdspAsmJitCodegen::compile (runtime, fn, diagnostics);
    ASSERT_NE (nullptr, compiled) << diagnostics.toString();

    std::vector<uint8_t> state (YdspAsmJitCodegen::stateSize (fn), 0);
    YdspKernelContext ctx;
    ctx.numSamples = 1;
    ctx.state = state.data();
    ctx.stateArrays = state.data() + YdspAsmJitCodegen::stateScalarSize (fn);
    compiled (&ctx);

    const auto* arrays = static_cast<const float*> (ctx.stateArrays);
    for (int i = 0; i < 16; ++i)
        EXPECT_FLOAT_EQ (static_cast<float> (i), arrays[i]) << i;
}

TEST (YdspAsmJitCodegenTests, VectorFmaCoalescesCarriedAccumulatorsAndPreservesAliasedOperandsOnArm64)
{
    // 1. Rolled loop-carried accumulator: fmla updates acc in place with zero .16b moves.
    for (const auto op : { YdspIrOp::fmaF, YdspIrOp::fmsubF })
    {
        SCOPED_TRACE (static_cast<int> (op));
        YdspIrFunction fn;
        fn.name = "RolledFmaAccumulator";
        fn.vectorized = true;
        fn.vectorWidth = 4;
        fn.packedFmaAvailable = true;
        fn.float32ArrayElements = 16;
        fn.valueTypes = {
            YdspValueType::int32Type,   // 0: zero int
            YdspValueType::int32Type,   // 1: bound (8)
            YdspValueType::int32Type,   // 2: step (4)
            YdspValueType::int32Type,   // 3: i
            YdspValueType::boolType,    // 4: i < 8
            YdspValueType::float32Type, // 5: zero float
            YdspValueType::float32Type, // 6: acc
            YdspValueType::float32Type, // 7: a
            YdspValueType::float32Type, // 8: b
            YdspValueType::float32Type, // 9: fmaTmp
            YdspValueType::int32Type    // 10: i + 4
        };
        fn.valueLanes = { 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 1 };
        fn.blocks.resize (4);

        fn.blocks[0].insts = {
            { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
            { YdspIrOp::constI, 1, -1, -1, -1, -1, 0.0, 8 },
            { YdspIrOp::constI, 2, -1, -1, -1, -1, 0.0, 4 },
            { YdspIrOp::movI, 3, 0 },
            { YdspIrOp::constF, 5, -1, -1, -1, -1, 0.0 },
            { YdspIrOp::vsplat, 6, 5 }
        };
        fn.blocks[0].term = YdspIrTerm::fallthrough;

        fn.blocks[1].insts = {
            { YdspIrOp::ltI, 4, 3, 1 }
        };
        fn.blocks[1].term = YdspIrTerm::branchIf;
        fn.blocks[1].termCond = 4;
        fn.blocks[1].termTarget = 2;
        fn.blocks[1].termTarget2 = 3;

        fn.blocks[2].insts = {
            { YdspIrOp::loadStateArrayF, 7, 3, -1, -1, 0 },
            { YdspIrOp::loadStateArrayF, 8, 3, -1, -1, 8 },
            { op, 9, 7, 8, 6 },
            { YdspIrOp::movF, 6, 9 },
            { YdspIrOp::addI, 10, 3, 2 },
            { YdspIrOp::movI, 3, 10 }
        };
        fn.blocks[2].term = YdspIrTerm::branch;
        fn.blocks[2].termTarget = 1;

        fn.blocks[3].insts = {
            { YdspIrOp::storeStateArrayF, -1, 0, 6, -1, 12 }
        };
        fn.blocks[3].term = YdspIrTerm::fallthrough;

        YdspIrLoop loop;
        loop.id = 0;
        loop.headerBlock = 1;
        loop.exitBlock = 3;
        loop.induction = 3;
        loop.bound.kind = YdspLoopBoundKind::constant;
        loop.bound.constant = 8;
        fn.loops.push_back (loop);

        YdspDiagnostics diagnostics;
        const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::arm64 }, fn, diagnostics);
        ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
        ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
        const auto listing = diagnostics.toString();

        EXPECT_TRUE (listing.contains (op == YdspIrOp::fmaF ? "fmla " : "fmls ")) << listing;

        int vectorCopies = 0;
        for (const auto& line : StringArray::fromLines (listing))
            if (line.trimStart().startsWith ("mov ") && line.contains (".16b"))
                ++vectorCopies;

        EXPECT_EQ (0, vectorCopies) << listing;
    }

    // 2. Unrolled multi-step FMA chain updating carried accumulator without .16b moves.
    {
        YdspIrFunction fn;
        fn.name = "UnrolledFmaChain";
        fn.vectorized = true;
        fn.vectorWidth = 4;
        fn.packedFmaAvailable = true;
        fn.float32ArrayElements = 20;
        fn.valueTypes = {
            YdspValueType::int32Type,   // 0: zero
            YdspValueType::float32Type, // 1: zero float
            YdspValueType::float32Type, // 2: acc
            YdspValueType::float32Type, // 3: a0
            YdspValueType::float32Type, // 4: b0
            YdspValueType::float32Type, // 5: t0
            YdspValueType::float32Type, // 6: a1
            YdspValueType::float32Type, // 7: b1
            YdspValueType::float32Type  // 8: t1
        };
        fn.valueLanes = { 1, 1, 4, 4, 4, 4, 4, 4, 4 };
        fn.blocks.resize (1);
        fn.blocks[0].insts = {
            { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
            { YdspIrOp::constF, 1, -1, -1, -1, -1, 0.0 },
            { YdspIrOp::vsplat, 2, 1 },
            { YdspIrOp::loadStateArrayF, 3, 0, -1, -1, 0 },
            { YdspIrOp::loadStateArrayF, 4, 0, -1, -1, 4 },
            { YdspIrOp::fmaF, 5, 3, 4, 2 },
            { YdspIrOp::movF, 2, 5 },
            { YdspIrOp::loadStateArrayF, 6, 0, -1, -1, 8 },
            { YdspIrOp::loadStateArrayF, 7, 0, -1, -1, 12 },
            { YdspIrOp::fmaF, 8, 6, 7, 2 },
            { YdspIrOp::movF, 2, 8 },
            { YdspIrOp::storeStateArrayF, -1, 0, 2, -1, 16 }
        };

        YdspDiagnostics diagnostics;
        const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::arm64 }, fn, diagnostics);
        ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
        const auto listing = diagnostics.toString();

        int fmlaCount = 0;
        int vectorCopies = 0;
        for (const auto& line : StringArray::fromLines (listing))
        {
            const auto inst = line.trimStart();
            if (inst.startsWith ("fmla "))
                ++fmlaCount;
            if (inst.startsWith ("mov ") && inst.contains (".16b"))
                ++vectorCopies;
        }

        EXPECT_EQ (2, fmlaCount) << listing;
        EXPECT_EQ (0, vectorCopies) << listing;

        asmjit::JitRuntime runtime;
        const auto compiled = YdspAsmJitCodegen::compile (runtime, fn, diagnostics);
        ASSERT_NE (nullptr, compiled) << diagnostics.toString();

        std::vector<float> state (20, 0.0f);
        for (size_t i = 0; i < 4; ++i)
        {
            state[i] = static_cast<float> (i + 1);
            state[4 + i] = 2.0f;
            state[8 + i] = static_cast<float> (i + 5);
            state[12 + i] = 3.0f;
        }
        YdspKernelContext ctx;
        ctx.numSamples = 1;
        ctx.state = state.data();
        ctx.stateArrays = state.data();
        compiled (&ctx);

        for (size_t i = 0; i < 4; ++i)
            EXPECT_FLOAT_EQ (static_cast<float> (i + 1) * 2.0f + static_cast<float> (i + 5) * 3.0f, state[16 + i]);
    }

    // 3. Safety when dst == a or dst == b and dst != c: c must not clobber a/b before fmla/fmls.
    for (const auto op : { YdspIrOp::fmaF, YdspIrOp::fmsubF })
        for (const bool aliasB : { false, true })
        {
            SCOPED_TRACE (static_cast<int> (op) * 2 + (aliasB ? 1 : 0));
            YdspIrFunction fn;
            fn.name = "AliasedVectorFmaOperands";
            fn.vectorized = true;
            fn.vectorWidth = 4;
            fn.packedFmaAvailable = true;
            fn.float32ArrayElements = 16;
            fn.valueTypes = {
                YdspValueType::int32Type,
                YdspValueType::float32Type,
                YdspValueType::float32Type,
                YdspValueType::float32Type
            };
            fn.valueLanes = { 1, 4, 4, 4 };
            fn.blocks.resize (1);
            fn.blocks[0].insts = {
                { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
                { YdspIrOp::loadStateArrayF, 1, 0, -1, -1, 0 },
                { YdspIrOp::loadStateArrayF, 2, 0, -1, -1, 4 },
                { YdspIrOp::loadStateArrayF, 3, 0, -1, -1, 8 },
                { op, aliasB ? 2 : 1, 1, 2, 3 },
                { YdspIrOp::storeStateArrayF, -1, 0, aliasB ? 2 : 1, -1, 12 }
            };

            YdspDiagnostics diagnostics;
            const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::arm64 }, fn, diagnostics);
            ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
            ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();

            asmjit::JitRuntime runtime;
            const auto compiled = YdspAsmJitCodegen::compile (runtime, fn, diagnostics);
            ASSERT_NE (nullptr, compiled) << diagnostics.toString();

            std::vector<float> state (16, 0.0f);
            for (size_t i = 0; i < 4; ++i)
            {
                state[i] = static_cast<float> (i + 2);
                state[4 + i] = 10.0f;
                state[8 + i] = static_cast<float> (i + 1);
            }
            YdspKernelContext ctx;
            ctx.numSamples = 1;
            ctx.state = state.data();
            ctx.stateArrays = state.data();
            compiled (&ctx);

            for (size_t i = 0; i < 4; ++i)
            {
                const float prod = state[i] * state[4 + i];
                const float expected = op == YdspIrOp::fmaF ? (prod + state[8 + i]) : (state[8 + i] - prod);
                EXPECT_FLOAT_EQ (expected, state[12 + i]) << i;
            }
        }
}

TEST (YdspAsmJitCodegenTests, HighArrayCountKernelCapsHoistedBasesAndReducesSpillsOnArm64)
{
    // 1. Shared index across 24 state arrays: caps hoisted bases at 8 (+ base 0)
    //    and keeps the sample loop free of stack spills.
    {
        constexpr int numArrays = 24;
        constexpr int arrayLength = 16;

        YdspIrFunction fn;
        fn.name = "SharedIndex24Arrays";
        fn.numInputs = 1;
        fn.numOutputs = 1;
        fn.inputTypes = { YdspValueType::float32Type };
        fn.outputTypes = { YdspValueType::float32Type };
        fn.int32Scalars = 1;
        fn.float32ArrayElements = numArrays * arrayLength;

        const auto addValue = [&] (YdspValueType type) -> int
        {
            const int id = static_cast<int> (fn.valueTypes.size());
            fn.valueTypes.push_back (type);
            fn.valueLanes.push_back (1);
            return id;
        };

        const int zeroVal = addValue (YdspValueType::int32Type);
        const int numSamplesVal = addValue (YdspValueType::int32Type);
        const int oneVal = addValue (YdspValueType::int32Type);
        const int sampleIdxVal = addValue (YdspValueType::int32Type);
        const int wpVal = addValue (YdspValueType::int32Type);
        const int condVal = addValue (YdspValueType::boolType);
        const int inSampleVal = addValue (YdspValueType::float32Type);
        const int nextWpVal = addValue (YdspValueType::int32Type);

        fn.blocks.resize (4);
        fn.blocks[0].insts = {
            { YdspIrOp::constI, zeroVal, -1, -1, -1, -1, 0.0, 0 },
            { YdspIrOp::loadBlockSize, numSamplesVal },
            { YdspIrOp::constI, oneVal, -1, -1, -1, -1, 0.0, 1 },
            { YdspIrOp::movI, sampleIdxVal, zeroVal },
            { YdspIrOp::loadStateI, wpVal, 0 }
        };
        fn.blocks[0].term = YdspIrTerm::fallthrough;

        fn.blocks[1].insts = {
            { YdspIrOp::ltI, condVal, sampleIdxVal, numSamplesVal }
        };
        fn.blocks[1].term = YdspIrTerm::branchIf;
        fn.blocks[1].termCond = condVal;
        fn.blocks[1].termTarget = 2;
        fn.blocks[1].termTarget2 = 3;

        fn.blocks[2].insts = {
            { YdspIrOp::loadInput, inSampleVal, sampleIdxVal, -1, -1, 0 },
            { YdspIrOp::advanceWrapI, nextWpVal, wpVal, -1, -1, -1, 0.0, arrayLength },
            { YdspIrOp::movI, wpVal, nextWpVal }
        };

        int accVal = inSampleVal;
        for (int k = 0; k < numArrays; ++k)
        {
            const int tapVal = addValue (YdspValueType::float32Type);
            const int sumVal = addValue (YdspValueType::float32Type);
            const int baseElem = k * arrayLength;
            fn.blocks[2].insts.push_back ({ YdspIrOp::storeStateArrayF, -1, wpVal, inSampleVal, -1, baseElem });
            fn.blocks[2].insts.push_back ({ YdspIrOp::loadStateArrayF, tapVal, wpVal, -1, -1, baseElem });
            fn.blocks[2].insts.push_back ({ YdspIrOp::addF, sumVal, accVal, tapVal });
            accVal = sumVal;
        }

        const int nextSampleVal = addValue (YdspValueType::int32Type);
        fn.blocks[2].insts.push_back ({ YdspIrOp::storeOutput, -1, sampleIdxVal, accVal, -1, 0 });
        fn.blocks[2].insts.push_back ({ YdspIrOp::addI, nextSampleVal, sampleIdxVal, oneVal });
        fn.blocks[2].insts.push_back ({ YdspIrOp::movI, sampleIdxVal, nextSampleVal });
        fn.blocks[2].term = YdspIrTerm::branch;
        fn.blocks[2].termTarget = 1;

        fn.blocks[3].insts = {
            { YdspIrOp::storeStateI, -1, wpVal, -1, -1, 0 }
        };
        fn.blocks[3].term = YdspIrTerm::fallthrough;

        YdspIrLoop loop;
        loop.id = 0;
        loop.headerBlock = 1;
        loop.exitBlock = 3;
        loop.induction = sampleIdxVal;
        loop.bound.kind = YdspLoopBoundKind::blockSize;
        fn.loops.push_back (loop);

        YdspDiagnostics diagnostics;
        const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::arm64 }, fn, diagnostics);
        ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
        ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();

        const auto listing = diagnostics.toString();
        int stackAccesses = 0;
        int preheaderAdds = 0;
        bool inPreheader = true;

        for (const auto& line : StringArray::fromLines (listing))
        {
            const auto trimmed = line.trimStart();
            if (trimmed.contains ("[sp"))
                ++stackAccesses;

            if (inPreheader)
            {
                if (trimmed.startsWith ("cmp "))
                    inPreheader = false;
                else if (trimmed.startsWith ("add ") && ! trimmed.contains ("sp"))
                    ++preheaderAdds;
            }
        }

        EXPECT_LE (preheaderAdds, 8) << listing;
        EXPECT_LE (stackAccesses, 8) << listing;

        asmjit::JitRuntime runtime;
        const auto compiled = YdspAsmJitCodegen::compile (runtime, fn, diagnostics);
        ASSERT_NE (nullptr, compiled) << diagnostics.toString();

        std::vector<uint8_t> state (YdspAsmJitCodegen::stateSize (fn), 0);
        std::vector<float> input { 1.0f, 2.0f, -0.5f, 0.25f };
        std::vector<float> output (input.size(), 0.0f);
        void* inPtrs[] { input.data() };
        void* outPtrs[] { output.data() };
        YdspKernelContext ctx;
        ctx.inputs = inPtrs;
        ctx.outputs = outPtrs;
        ctx.state = state.data();
        ctx.stateArrays = state.data() + YdspAsmJitCodegen::stateScalarSize (fn);
        ctx.numSamples = static_cast<int> (input.size());
        compiled (&ctx);

        for (size_t i = 0; i < input.size(); ++i)
            EXPECT_NEAR (input[i] * static_cast<float> (numArrays + 1), output[i], 1e-5f) << i;
    }

    // 2. 24 independent delay lines with loop-carried ring indices and >4095 bounds:
    //    caps hoisted arrayBase and wrapLast registers under high GP pressure.
    {
        constexpr int numDelays = 24;
        std::vector<int> bounds (numDelays);
        std::vector<int> baseElems (numDelays);
        int totalElems = 0;

        for (int k = 0; k < numDelays; ++k)
        {
            bounds[static_cast<size_t> (k)] = k < 18 ? (32 + k) : (4600 + (k - 18) * 100);
            baseElems[static_cast<size_t> (k)] = totalElems;
            totalElems += bounds[static_cast<size_t> (k)];
        }

        YdspIrFunction fn;
        fn.name = "Diffuser24Delays";
        fn.numInputs = 1;
        fn.numOutputs = 1;
        fn.inputTypes = { YdspValueType::float32Type };
        fn.outputTypes = { YdspValueType::float32Type };
        fn.int32Scalars = numDelays;
        fn.float32ArrayElements = totalElems;

        const auto addValue = [&] (YdspValueType type) -> int
        {
            const int id = static_cast<int> (fn.valueTypes.size());
            fn.valueTypes.push_back (type);
            fn.valueLanes.push_back (1);
            return id;
        };

        const int zeroVal = addValue (YdspValueType::int32Type);
        const int numSamplesVal = addValue (YdspValueType::int32Type);
        const int oneVal = addValue (YdspValueType::int32Type);
        const int sampleIdxVal = addValue (YdspValueType::int32Type);
        std::vector<int> wpVals (numDelays);
        for (int k = 0; k < numDelays; ++k)
            wpVals[static_cast<size_t> (k)] = addValue (YdspValueType::int32Type);

        const int condVal = addValue (YdspValueType::boolType);
        const int inSampleVal = addValue (YdspValueType::float32Type);

        fn.blocks.resize (4);
        fn.blocks[0].insts = {
            { YdspIrOp::constI, zeroVal, -1, -1, -1, -1, 0.0, 0 },
            { YdspIrOp::loadBlockSize, numSamplesVal },
            { YdspIrOp::constI, oneVal, -1, -1, -1, -1, 0.0, 1 },
            { YdspIrOp::movI, sampleIdxVal, zeroVal }
        };
        for (int k = 0; k < numDelays; ++k)
            fn.blocks[0].insts.push_back ({ YdspIrOp::loadStateI, wpVals[static_cast<size_t> (k)], k });
        fn.blocks[0].term = YdspIrTerm::fallthrough;

        fn.blocks[1].insts = {
            { YdspIrOp::ltI, condVal, sampleIdxVal, numSamplesVal }
        };
        fn.blocks[1].term = YdspIrTerm::branchIf;
        fn.blocks[1].termCond = condVal;
        fn.blocks[1].termTarget = 2;
        fn.blocks[1].termTarget2 = 3;

        fn.blocks[2].insts = {
            { YdspIrOp::loadInput, inSampleVal, sampleIdxVal, -1, -1, 0 }
        };

        int accVal = inSampleVal;
        for (int k = 0; k < numDelays; ++k)
        {
            const int wp = wpVals[static_cast<size_t> (k)];
            const int nextWp = addValue (YdspValueType::int32Type);
            const int tapVal = addValue (YdspValueType::float32Type);
            const int sumVal = addValue (YdspValueType::float32Type);
            const int baseElem = baseElems[static_cast<size_t> (k)];
            const int bound = bounds[static_cast<size_t> (k)];

            fn.blocks[2].insts.push_back ({ YdspIrOp::storeStateArrayF, -1, wp, inSampleVal, -1, baseElem });
            fn.blocks[2].insts.push_back ({ YdspIrOp::advanceWrapI, nextWp, wp, -1, -1, -1, 0.0, bound });
            fn.blocks[2].insts.push_back ({ YdspIrOp::movI, wp, nextWp });
            fn.blocks[2].insts.push_back ({ YdspIrOp::loadStateArrayF, tapVal, wp, -1, -1, baseElem });
            fn.blocks[2].insts.push_back ({ YdspIrOp::addF, sumVal, accVal, tapVal });
            accVal = sumVal;
        }

        const int nextSampleVal = addValue (YdspValueType::int32Type);
        fn.blocks[2].insts.push_back ({ YdspIrOp::storeOutput, -1, sampleIdxVal, accVal, -1, 0 });
        fn.blocks[2].insts.push_back ({ YdspIrOp::addI, nextSampleVal, sampleIdxVal, oneVal });
        fn.blocks[2].insts.push_back ({ YdspIrOp::movI, sampleIdxVal, nextSampleVal });
        fn.blocks[2].term = YdspIrTerm::branch;
        fn.blocks[2].termTarget = 1;

        for (int k = 0; k < numDelays; ++k)
            fn.blocks[3].insts.push_back ({ YdspIrOp::storeStateI, -1, wpVals[static_cast<size_t> (k)], -1, -1, k });
        fn.blocks[3].term = YdspIrTerm::fallthrough;

        YdspIrLoop loop;
        loop.id = 0;
        loop.headerBlock = 1;
        loop.exitBlock = 3;
        loop.induction = sampleIdxVal;
        loop.bound.kind = YdspLoopBoundKind::blockSize;
        fn.loops.push_back (loop);

        YdspDiagnostics diagnostics;
        const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::arm64 }, fn, diagnostics);
        ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
        ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();

        const auto listing = diagnostics.toString();
        int stackAccesses = 0;
        int preheaderAdds = 0;
        bool inPreheader = true;

        for (const auto& line : StringArray::fromLines (listing))
        {
            const auto trimmed = line.trimStart();
            if (trimmed.contains ("[sp"))
                ++stackAccesses;

            if (inPreheader)
            {
                if (trimmed.startsWith ("cmp "))
                    inPreheader = false;
                else if (trimmed.startsWith ("add ") && ! trimmed.contains ("sp"))
                    ++preheaderAdds;
            }
        }

        EXPECT_LE (preheaderAdds, 2) << listing;
        EXPECT_LT (stackAccesses, 32) << listing;

        asmjit::JitRuntime runtime;
        const auto compiled = YdspAsmJitCodegen::compile (runtime, fn, diagnostics);
        ASSERT_NE (nullptr, compiled) << diagnostics.toString();

        std::vector<uint8_t> state (YdspAsmJitCodegen::stateSize (fn), 0);
        std::vector<int32_t> refWps (numDelays, 0);
        std::vector<float> refArrays (static_cast<size_t> (totalElems), 0.0f);

        constexpr int blockSamples = 40;
        for (int block = 0; block < 2; ++block)
        {
            std::vector<float> input (blockSamples);
            for (int i = 0; i < blockSamples; ++i)
                input[static_cast<size_t> (i)] = 0.1f * static_cast<float> (block * blockSamples + i + 1);

            std::vector<float> actual (blockSamples, 0.0f);
            void* inPtrs[] { input.data() };
            void* outPtrs[] { actual.data() };
            YdspKernelContext ctx;
            ctx.inputs = inPtrs;
            ctx.outputs = outPtrs;
            ctx.state = state.data();
            ctx.stateArrays = state.data() + YdspAsmJitCodegen::stateScalarSize (fn);
            ctx.numSamples = blockSamples;
            compiled (&ctx);

            for (int i = 0; i < blockSamples; ++i)
            {
                const float x = input[static_cast<size_t> (i)];
                float acc = x;
                for (int k = 0; k < numDelays; ++k)
                {
                    const int baseElem = baseElems[static_cast<size_t> (k)];
                    const int bound = bounds[static_cast<size_t> (k)];
                    refArrays[static_cast<size_t> (baseElem + refWps[static_cast<size_t> (k)])] = x;
                    refWps[static_cast<size_t> (k)] = (refWps[static_cast<size_t> (k)] + 1 >= bound) ? 0 : (refWps[static_cast<size_t> (k)] + 1);
                    acc += refArrays[static_cast<size_t> (baseElem + refWps[static_cast<size_t> (k)])];
                }
                EXPECT_NEAR (acc, actual[static_cast<size_t> (i)], 1e-4f) << "block=" << block << " i=" << i;
            }
        }
    }
}

TEST (YdspAsmJitCodegenTests, AddGpImmUsesImmediateEncodingWithoutTemporaryOnArm64)
{
    struct Case
    {
        int memIndex;
        int expectedAdds;
        bool expectMov;
        const char* expectedToken;
    };

    const Case cases[] {
        { 64, 1, false, ", 256" },         // <= 4095: single add #imm12
        { 2048, 1, false, ", 2, 12" },     // 8192 = 2 << 12: single shifted add
        { 2055, 2, false, ", 28" },        // 8220 = (2 << 12) + 28: two adds, zero mov temps
        { 4194304, 1, true, nullptr }      // 0x1000000 > 0xFFFFFF: register fallback
    };

    for (const auto& testCase : cases)
    {
        SCOPED_TRACE (testCase.memIndex);

        YdspIrFunction fn;
        fn.name = "AddGpImmCase";
        fn.float32ArrayElements = testCase.memIndex + 8;
        fn.valueTypes = { YdspValueType::int32Type, YdspValueType::float32Type };
        fn.valueLanes = { 1, 1 };
        fn.blocks.resize (1);
        fn.blocks[0].insts = {
            { YdspIrOp::loadBlockSize, 0 },
            { YdspIrOp::loadStateArrayF, 1, 0, -1, -1, testCase.memIndex },
            { YdspIrOp::storeStateArrayF, -1, 0, 1, -1, testCase.memIndex }
        };

        YdspDiagnostics diagnostics;
        const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::arm64 }, fn, diagnostics);
        ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
        ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();

        const auto listing = diagnostics.toString();
        int addCount = 0;
        int movCount = 0;
        bool sawExpected = testCase.expectedToken == nullptr;

        for (const auto& line : StringArray::fromLines (listing))
        {
            const auto trimmed = line.trimStart();
            if (trimmed.startsWith ("add "))
            {
                ++addCount;
                if (testCase.expectedToken != nullptr && trimmed.contains (testCase.expectedToken))
                    sawExpected = true;
            }
            else if (trimmed.startsWith ("mov"))
            {
                ++movCount;
            }
        }

        EXPECT_EQ (testCase.expectedAdds, addCount) << listing;
        EXPECT_EQ (testCase.expectMov, movCount > 0) << listing;
        EXPECT_TRUE (sawExpected) << listing;
    }

    // Verify runtime execution for a 4096..0xFFFFFF offset (memIndex = 2055 -> base = 8220).
    YdspIrFunction fn;
    fn.name = "AddGpImmRuntime";
    fn.float32ArrayElements = 2064;
    fn.valueTypes = { YdspValueType::int32Type, YdspValueType::float32Type, YdspValueType::float32Type };
    fn.valueLanes = { 1, 1, 1 };
    fn.blocks.resize (1);
    fn.blocks[0].insts = {
        { YdspIrOp::loadBlockSize, 0 },
        { YdspIrOp::loadStateArrayF, 1, 0, -1, -1, 2055 },
        { YdspIrOp::addF, 2, 1, 1 },
        { YdspIrOp::storeStateArrayF, -1, 0, 2, -1, 2055 }
    };

    YdspDiagnostics diagnostics;
    asmjit::JitRuntime runtime;
    const auto compiled = YdspAsmJitCodegen::compile (runtime, fn, diagnostics);
    ASSERT_NE (nullptr, compiled) << diagnostics.toString();

    std::vector<float> state (2064, 0.0f);
    state[2055 + 3] = 7.5f;
    YdspKernelContext ctx;
    ctx.numSamples = 3;
    ctx.state = state.data();
    ctx.stateArrays = state.data();
    compiled (&ctx);

    EXPECT_FLOAT_EQ (15.0f, state[2055 + 3]);
}

TEST (YdspAsmJitCodegenTests, BitcastFloatAndIntScalarAndVectorRoundTrip)
{
    // 1. Scalar bitcastFtoI -> xorI (sign flip) -> bitcastItoF
    {
        YdspIrFunction fn;
        fn.name = "ScalarBitcast";
        fn.numInputs = 1;
        fn.numOutputs = 1;
        fn.inputTypes = { YdspValueType::float32Type };
        fn.outputTypes = { YdspValueType::float32Type };
        fn.valueTypes = {
            YdspValueType::int32Type,   // 0: idx
            YdspValueType::float32Type, // 1: in
            YdspValueType::int32Type,   // 2: bits
            YdspValueType::int32Type,   // 3: signMask
            YdspValueType::int32Type,   // 4: flippedBits
            YdspValueType::float32Type  // 5: out
        };
        fn.valueLanes = { 1, 1, 1, 1, 1, 1 };
        fn.blocks.resize (1);
        fn.blocks[0].insts = {
            { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
            { YdspIrOp::loadInput, 1, 0, -1, -1, 0 },
            { YdspIrOp::bitcastFtoI, 2, 1 },
            { YdspIrOp::constI, 3, -1, -1, -1, -1, 0.0, static_cast<int64_t> (static_cast<int32_t> (0x80000000u)) },
            { YdspIrOp::xorI, 4, 2, 3 },
            { YdspIrOp::bitcastItoF, 5, 4 },
            { YdspIrOp::storeOutput, -1, 0, 5, -1, 0 }
        };

        for (const auto arch : { YdspTargetArchitecture::x64, YdspTargetArchitecture::arm64 })
        {
            YdspDiagnostics diagnostics;
            const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, arch }, fn, diagnostics);
            ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
            ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
        }

        YdspDiagnostics diagnostics;
        asmjit::JitRuntime runtime;
        const auto compiled = YdspAsmJitCodegen::compile (runtime, fn, diagnostics);
        ASSERT_NE (nullptr, compiled) << diagnostics.toString();

        for (const float x : { 0.0f, -0.0f, 1.5f, -3.25f, 42.125f })
        {
            float in = x;
            float out = 0.0f;
            void* inPtrs[] { &in };
            void* outPtrs[] { &out };
            YdspKernelContext ctx;
            ctx.inputs = inPtrs;
            ctx.outputs = outPtrs;
            ctx.numSamples = 1;
            compiled (&ctx);

            const auto expectedBits = std::bit_cast<uint32_t> (x) ^ 0x80000000u;
            EXPECT_EQ (expectedBits, std::bit_cast<uint32_t> (out));
        }
    }

    // 2. Vector bitcastFtoI -> xorI -> bitcastItoF (4-lane SSE2/AVX/ARM64 and 8-lane AVX2)
    for (const int width : { 4, 8 })
    {
        YdspIrFunction fn;
        fn.name = "VectorBitcast";
        fn.vectorized = true;
        fn.vectorWidth = width;
        fn.float32ArrayElements = 16;
        fn.valueTypes = {
            YdspValueType::int32Type,   // 0: idx
            YdspValueType::float32Type, // 1: vecF
            YdspValueType::int32Type,   // 2: vecI
            YdspValueType::int32Type,   // 3: maskScalar
            YdspValueType::int32Type,   // 4: maskVec
            YdspValueType::int32Type,   // 5: xored
            YdspValueType::float32Type  // 6: outF
        };
        fn.valueLanes = { 1, width, width, 1, width, width, width };
        fn.blocks.resize (1);
        fn.blocks[0].insts = {
            { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
            { YdspIrOp::loadStateArrayF, 1, 0, -1, -1, 0 },
            { YdspIrOp::bitcastFtoI, 2, 1 },
            { YdspIrOp::constI, 3, -1, -1, -1, -1, 0.0, static_cast<int64_t> (static_cast<int32_t> (0x80000000u)) },
            { YdspIrOp::vsplat, 4, 3 },
            { YdspIrOp::xorI, 5, 2, 4 },
            { YdspIrOp::bitcastItoF, 6, 5 },
            { YdspIrOp::storeStateArrayF, -1, 0, 6, -1, 8 }
        };

        if (width == 4)
        {
            YdspDiagnostics armDiag;
            const auto armArt = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::arm64 }, fn, armDiag);
            ASSERT_TRUE (armArt.wasOk()) << armDiag.toString();
        }

        YdspDiagnostics x64Diag;
        const auto x64Art = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::x64 }, fn, x64Diag);
        ASSERT_TRUE (x64Art.wasOk()) << x64Diag.toString();

        if (width == 4)
        {
            YdspDiagnostics diagnostics;
            asmjit::JitRuntime runtime;
            const auto compiled = YdspAsmJitCodegen::compile (runtime, fn, diagnostics);
            ASSERT_NE (nullptr, compiled) << diagnostics.toString();

            std::vector<float> state (16, 0.0f);
            state[0] = 1.25f;
            state[1] = -2.5f;
            state[2] = 0.0f;
            state[3] = -0.0f;

            YdspKernelContext ctx;
            ctx.numSamples = 1;
            ctx.state = state.data();
            ctx.stateArrays = state.data();
            compiled (&ctx);

            for (int k = 0; k < 4; ++k)
                EXPECT_EQ (std::bit_cast<uint32_t> (state[static_cast<size_t> (k)]) ^ 0x80000000u,
                           std::bit_cast<uint32_t> (state[8 + static_cast<size_t> (k)]));
        }
    }
}

TEST (YdspAsmJitCodegenTests, VectorInsertAndExtractAllLanesAndCoalescing)
{
    for (const int width : { 4, 8 })
    {
        for (const bool enableAvx : { false, true })
        {
            if (width == 8 && ! enableAvx)
                continue;

            YdspIrFunction fn;
            fn.name = "InsertExtractRoundTrip";
            fn.vectorized = true;
            fn.vectorWidth = width;
            fn.packedFmaAvailable = enableAvx;
            fn.float32ArrayElements = 32;

            // Values:
            //   0..width-1: scalar inputs (loaded from state[0..width-1])
            //   width..2*width-1: chained vinsertF vectors
            //   2*width..3*width-1: extracted scalars
            //   3*width: constI 0
            //   3*width+1..4*width: constI lane indices
            fn.valueTypes.assign (static_cast<size_t> (width * 3 + 1 + width), YdspValueType::float32Type);
            fn.valueLanes.assign (fn.valueTypes.size(), 1);

            for (int k = 0; k < width; ++k)
                fn.valueLanes[static_cast<size_t> (width + k)] = width;

            const int zeroConst = width * 3;
            fn.valueTypes[static_cast<size_t> (zeroConst)] = YdspValueType::int32Type;

            for (int k = 0; k < width; ++k)
                fn.valueTypes[static_cast<size_t> (zeroConst + 1 + k)] = YdspValueType::int32Type;

            fn.blocks.resize (1);
            auto& insts = fn.blocks[0].insts;

            for (int k = 0; k < width; ++k)
            {
                const int idxId = zeroConst + 1 + k;
                insts.push_back ({ YdspIrOp::constI, idxId, -1, -1, -1, -1, 0.0, k });
                insts.push_back ({ YdspIrOp::loadStateArrayF, k, idxId, -1, -1, 0 });
            }

            for (int k = 0; k < width; ++k)
            {
                const int prev = k == 0 ? -1 : (width + k - 1);
                insts.push_back ({ YdspIrOp::vinsertF, width + k, prev, k, -1, k });
            }

            const int finalVec = width + width - 1;
            insts.push_back ({ YdspIrOp::constI, zeroConst, -1, -1, -1, -1, 0.0, 0 });
            insts.push_back ({ YdspIrOp::storeStateArrayF, -1, zeroConst, finalVec, -1, 16 });

            for (int k = 0; k < width; ++k)
            {
                const int extId = width * 2 + k;
                insts.push_back ({ YdspIrOp::vextractF, extId, finalVec, -1, -1, k });
                insts.push_back ({ YdspIrOp::storeStateArrayF, -1, zeroConst + 1 + k, extId, -1, 8 });
            }

            YdspDiagnostics x64Diag;
            const auto x64Art = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::x64 }, fn, x64Diag);
            ASSERT_TRUE (x64Art.wasOk()) << x64Diag.toString();
            ASSERT_FALSE (x64Diag.hasErrors()) << x64Diag.toString();

            if (width == 4 && ! enableAvx)
            {
                YdspDiagnostics armDiag;
                const auto armArt = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::arm64 }, fn, armDiag);
                ASSERT_TRUE (armArt.wasOk()) << armDiag.toString();
                ASSERT_FALSE (armDiag.hasErrors()) << armDiag.toString();

                const auto listing = armDiag.toString();
                int insCount = 0;
                int vecMoveCount = 0;

                for (const auto& line : StringArray::fromLines (listing))
                {
                    const auto trimmed = line.trimStart();
                    if (trimmed.startsWith ("ins "))
                        ++insCount;
                    else if (trimmed.startsWith ("mov ") && trimmed.contains (".16b"))
                        ++vecMoveCount;
                }

                EXPECT_EQ (3, insCount) << listing;
                EXPECT_EQ (0, vecMoveCount) << listing;

                YdspDiagnostics runtimeDiag;
                asmjit::JitRuntime runtime;
                const auto compiled = YdspAsmJitCodegen::compile (runtime, fn, runtimeDiag);
                ASSERT_NE (nullptr, compiled) << runtimeDiag.toString();

                std::vector<float> state (32, 0.0f);
                state[0] = 1.5f;
                state[1] = -2.25f;
                state[2] = 3.75f;
                state[3] = 4.5f;

                YdspKernelContext ctx;
                ctx.numSamples = 1;
                ctx.state = state.data();
                ctx.stateArrays = state.data();
                compiled (&ctx);

                for (int k = 0; k < 4; ++k)
                {
                    EXPECT_FLOAT_EQ (state[static_cast<size_t> (k)], state[16 + static_cast<size_t> (k)]);
                    EXPECT_FLOAT_EQ (state[static_cast<size_t> (k)], state[8 + static_cast<size_t> (k)]);
                }
            }
        }
    }

    // Verify `a < 0` at a non-zero lane zeroes the untouched lanes.
    {
        YdspIrFunction fn;
        fn.name = "InsertNonZeroStart";
        fn.vectorized = true;
        fn.vectorWidth = 4;
        fn.float32ArrayElements = 8;
        fn.valueTypes = {
            YdspValueType::int32Type,   // 0: const 0
            YdspValueType::float32Type, // 1: scalar
            YdspValueType::float32Type  // 2: vec
        };
        fn.valueLanes = { 1, 1, 4 };
        fn.blocks.resize (1);
        fn.blocks[0].insts = {
            { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
            { YdspIrOp::constF, 1, -1, -1, -1, -1, 9.25 },
            { YdspIrOp::vinsertF, 2, -1, 1, -1, 2 },
            { YdspIrOp::storeStateArrayF, -1, 0, 2, -1, 0 }
        };

        YdspDiagnostics diagnostics;
        asmjit::JitRuntime runtime;
        const auto compiled = YdspAsmJitCodegen::compile (runtime, fn, diagnostics);
        ASSERT_NE (nullptr, compiled) << diagnostics.toString();

        std::vector<float> state (8, 99.0f);
        YdspKernelContext ctx;
        ctx.numSamples = 1;
        ctx.state = state.data();
        ctx.stateArrays = state.data();
        compiled (&ctx);

        EXPECT_FLOAT_EQ (0.0f, state[0]);
        EXPECT_FLOAT_EQ (0.0f, state[1]);
        EXPECT_FLOAT_EQ (9.25f, state[2]);
        EXPECT_FLOAT_EQ (0.0f, state[3]);
    }
}

TEST (YdspAsmJitCodegenTests, AdvancingLoopPointersEliminateIndexScalingInRolledLoops)
{
    // 1. Rolled 4-lane state-array loop (like Additive64 / ModalBank).
    {
        constexpr int count = 16;

        YdspIrFunction fn;
        fn.name = "RolledVectorArrayLoop";
        fn.vectorized = true;
        fn.vectorWidth = 4;
        fn.float32ArrayElements = count * 3;
        fn.valueTypes = {
            YdspValueType::int32Type,   // 0: zero
            YdspValueType::int32Type,   // 1: bound (16)
            YdspValueType::int32Type,   // 2: step (4)
            YdspValueType::int32Type,   // 3: induction k
            YdspValueType::boolType,    // 4: cond
            YdspValueType::float32Type, // 5: vecA
            YdspValueType::float32Type, // 6: vecB
            YdspValueType::float32Type, // 7: sum
            YdspValueType::int32Type    // 8: kNext
        };
        fn.valueLanes = { 1, 1, 1, 1, 1, 4, 4, 4, 1 };

        fn.blocks.resize (4);
        fn.blocks[0].insts = {
            { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
            { YdspIrOp::constI, 1, -1, -1, -1, -1, 0.0, count },
            { YdspIrOp::constI, 2, -1, -1, -1, -1, 0.0, 4 },
            { YdspIrOp::movI, 3, 0 }
        };
        fn.blocks[0].term = YdspIrTerm::fallthrough;

        fn.blocks[1].insts = {
            { YdspIrOp::ltI, 4, 3, 1 }
        };
        fn.blocks[1].term = YdspIrTerm::branchIf;
        fn.blocks[1].termCond = 4;
        fn.blocks[1].termTarget = 2;
        fn.blocks[1].termTarget2 = 3;

        fn.blocks[2].insts = {
            { YdspIrOp::loadStateArrayF, 5, 3, -1, -1, 0 },
            { YdspIrOp::loadStateArrayF, 6, 3, -1, -1, count },
            { YdspIrOp::addF, 7, 5, 6 },
            { YdspIrOp::storeStateArrayF, -1, 3, 7, -1, count * 2 },
            { YdspIrOp::addI, 8, 3, 2 },
            { YdspIrOp::movI, 3, 8 }
        };
        fn.blocks[2].term = YdspIrTerm::branch;
        fn.blocks[2].termTarget = 1;

        fn.blocks[3].term = YdspIrTerm::fallthrough;

        YdspIrLoop loop;
        loop.id = 0;
        loop.headerBlock = 1;
        loop.exitBlock = 3;
        loop.induction = 3;
        loop.bound = { YdspLoopBoundKind::constant, count };
        fn.loops.push_back (loop);

        // ARM64: advancing pointers eliminate sxtw + lsl inside the loop.
        {
            YdspDiagnostics armDiag;
            const auto armArt = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::arm64 }, fn, armDiag);
            ASSERT_TRUE (armArt.wasOk()) << armDiag.toString();

            const auto listing = armDiag.toString();
            int sxtwCount = 0;
            int stepAddCount = 0;

            for (const auto& line : StringArray::fromLines (listing))
            {
                const auto trimmed = line.upToFirstOccurrenceOf (";", false, false).trim();
                if (trimmed.startsWith ("sxtw "))
                    ++sxtwCount;
                else if (trimmed.startsWith ("add ") && trimmed.endsWith (", 16"))
                    ++stepAddCount;
            }

            EXPECT_EQ (0, sxtwCount) << listing;
            EXPECT_EQ (3, stepAddCount) << listing;
        }

        // x64: advancing pointers step by 16 at the latch.
        {
            YdspDiagnostics x64Diag;
            const auto x64Art = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::x64 }, fn, x64Diag);
            ASSERT_TRUE (x64Art.wasOk()) << x64Diag.toString();

            const auto listing = x64Diag.toString();
            int stepLeaCount = 0;

            for (const auto& line : StringArray::fromLines (listing))
            {
                const auto trimmed = line.trimStart();
                if (trimmed.startsWith ("lea ") && trimmed.contains ("+16]"))
                    ++stepLeaCount;
            }

            EXPECT_EQ (3, stepLeaCount) << listing;
        }

        YdspDiagnostics diagnostics;
        asmjit::JitRuntime runtime;
        const auto compiled = YdspAsmJitCodegen::compile (runtime, fn, diagnostics);
        ASSERT_NE (nullptr, compiled) << diagnostics.toString();

        std::vector<float> state (static_cast<size_t> (count * 3), 0.0f);
        for (int i = 0; i < count; ++i)
        {
            state[static_cast<size_t> (i)] = static_cast<float> (i) + 1.0f;
            state[static_cast<size_t> (count + i)] = static_cast<float> (i) * 2.0f;
        }

        YdspKernelContext ctx;
        ctx.numSamples = 1;
        ctx.state = state.data();
        ctx.stateArrays = state.data();
        compiled (&ctx);

        for (int i = 0; i < count; ++i)
            EXPECT_FLOAT_EQ (state[static_cast<size_t> (i)] + state[static_cast<size_t> (count + i)],
                             state[static_cast<size_t> (count * 2 + i)]);
    }

    // 2. Chunked SampleLoopDistributor-style loop with stream offset `pos + k`
    //    inside an outer loop.
    {
        constexpr int chunkSize = 16;

        YdspIrFunction fn;
        fn.name = "ChunkedStreamOffsetLoop";
        fn.vectorized = true;
        fn.vectorWidth = 4;
        fn.numInputs = 1;
        fn.numOutputs = 1;
        fn.inputTypes = { YdspValueType::float32Type };
        fn.outputTypes = { YdspValueType::float32Type };
        fn.float32ArrayElements = chunkSize;
        fn.valueTypes = {
            YdspValueType::int32Type,   // 0: zero
            YdspValueType::int32Type,   // 1: chunkConst (16)
            YdspValueType::int32Type,   // 2: step (4)
            YdspValueType::int32Type,   // 3: numSamples
            YdspValueType::int32Type,   // 4: pos
            YdspValueType::boolType,    // 5: outerCond
            YdspValueType::int32Type,   // 6: k
            YdspValueType::boolType,    // 7: innerCond
            YdspValueType::int32Type,   // 8: streamIdx (pos + k)
            YdspValueType::float32Type, // 9: inVec
            YdspValueType::float32Type, // 10: scaled
            YdspValueType::int32Type,   // 11: kNext
            YdspValueType::int32Type    // 12: posNext
        };
        fn.valueLanes = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 1, 1 };

        // Blocks:
        //   0: preheader (pos = 0)
        //   1: outer header (pos < numSamples)
        //   2: inner preheader (k = 0)
        //   3: inner header (k < 16)
        //   4: inner body
        //   5: outer latch (pos += 16)
        //   6: exit
        fn.blocks.resize (7);
        fn.blocks[0].insts = {
            { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
            { YdspIrOp::constI, 1, -1, -1, -1, -1, 0.0, chunkSize },
            { YdspIrOp::constI, 2, -1, -1, -1, -1, 0.0, 4 },
            { YdspIrOp::loadBlockSize, 3 },
            { YdspIrOp::movI, 4, 0 }
        };
        fn.blocks[0].term = YdspIrTerm::fallthrough;

        fn.blocks[1].insts = { { YdspIrOp::ltI, 5, 4, 3 } };
        fn.blocks[1].term = YdspIrTerm::branchIf;
        fn.blocks[1].termCond = 5;
        fn.blocks[1].termTarget = 2;
        fn.blocks[1].termTarget2 = 6;

        fn.blocks[2].insts = { { YdspIrOp::movI, 6, 0 } };
        fn.blocks[2].term = YdspIrTerm::fallthrough;

        fn.blocks[3].insts = { { YdspIrOp::ltI, 7, 6, 1 } };
        fn.blocks[3].term = YdspIrTerm::branchIf;
        fn.blocks[3].termCond = 7;
        fn.blocks[3].termTarget = 4;
        fn.blocks[3].termTarget2 = 5;

        fn.blocks[4].insts = {
            { YdspIrOp::addI, 8, 4, 6 },
            { YdspIrOp::loadInput, 9, 8, -1, -1, 0 },
            { YdspIrOp::addF, 10, 9, 9 },
            { YdspIrOp::storeOutput, -1, 8, 10, -1, 0 },
            { YdspIrOp::addI, 11, 6, 2 },
            { YdspIrOp::movI, 6, 11 }
        };
        fn.blocks[4].term = YdspIrTerm::branch;
        fn.blocks[4].termTarget = 3;

        fn.blocks[5].insts = {
            { YdspIrOp::addI, 12, 4, 1 },
            { YdspIrOp::movI, 4, 12 }
        };
        fn.blocks[5].term = YdspIrTerm::branch;
        fn.blocks[5].termTarget = 1;

        fn.blocks[6].term = YdspIrTerm::fallthrough;

        fn.loops.push_back ({ 0, 1, 6, 4, { YdspLoopBoundKind::blockSize, 0 }, false });
        fn.loops.push_back ({ 1, 3, 5, 6, { YdspLoopBoundKind::constant, chunkSize }, false });

        for (const auto arch : { YdspTargetArchitecture::x64, YdspTargetArchitecture::arm64 })
        {
            YdspDiagnostics diag;
            const auto art = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, arch }, fn, diag);
            ASSERT_TRUE (art.wasOk()) << diag.toString();
        }

        YdspDiagnostics diagnostics;
        asmjit::JitRuntime runtime;
        const auto compiled = YdspAsmJitCodegen::compile (runtime, fn, diagnostics);
        ASSERT_NE (nullptr, compiled) << diagnostics.toString();

        std::vector<float> input (32);
        for (size_t i = 0; i < input.size(); ++i)
            input[i] = static_cast<float> (i) * 0.5f - 4.0f;

        std::vector<float> output (32, 0.0f);
        std::vector<uint8_t> state (YdspAsmJitCodegen::stateSize (fn), 0);
        void* inPtrs[] { input.data() };
        void* outPtrs[] { output.data() };

        YdspKernelContext ctx;
        ctx.inputs = inPtrs;
        ctx.outputs = outPtrs;
        ctx.state = state.data();
        ctx.stateArrays = state.data() + YdspAsmJitCodegen::stateScalarSize (fn);
        ctx.numSamples = 32;
        compiled (&ctx);

        for (size_t i = 0; i < input.size(); ++i)
            EXPECT_FLOAT_EQ (input[i] * 2.0f, output[i]) << i;
    }
}

TEST (YdspAsmJitCodegenTests, NegativeRingWrapReusesSubFlagsAndEliminatesCompare)
{
    // 1. Direct IR: Echo-style negative wrap with N = 96000 (> 4095) and an
    //    intervening float op between subI and selectB.
    {
        YdspIrFunction fn;
        fn.name = "EchoNegativeWrap";
        fn.numInputs = 1;
        fn.numOutputs = 1;
        fn.inputTypes = { YdspValueType::float32Type };
        fn.outputTypes = { YdspValueType::float32Type };
        fn.float32ArrayElements = 8;
        fn.valueTypes = {
            YdspValueType::int32Type,   // 0: idx
            YdspValueType::int32Type,   // 1: delay (4)
            YdspValueType::int32Type,   // 2: zero
            YdspValueType::int32Type,   // 3: ringSize (96000)
            YdspValueType::int32Type,   // 4: diff
            YdspValueType::float32Type, // 5: input sample (flag-preserving intervening op)
            YdspValueType::float32Type, // 6: scaled sample
            YdspValueType::boolType,    // 7: cond (diff < 0)
            YdspValueType::int32Type,   // 8: wrapped (diff + 96000)
            YdspValueType::int32Type,   // 9: readIdx
            YdspValueType::float32Type  // 10: float(readIdx)
        };
        fn.valueLanes = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

        YdspIrInst subInst { YdspIrOp::subI, 4, 0, 1 };
        subInst.saturatingIntegerArithmetic = false;
        YdspIrInst addInst { YdspIrOp::addI, 8, 4, 3 };
        addInst.saturatingIntegerArithmetic = false;

        fn.blocks.resize (1);
        fn.blocks[0].insts = {
            { YdspIrOp::loadBlockSize, 0 },
            { YdspIrOp::constI, 1, -1, -1, -1, -1, 0.0, 4 },
            { YdspIrOp::constI, 2, -1, -1, -1, -1, 0.0, 0 },
            { YdspIrOp::constI, 3, -1, -1, -1, -1, 0.0, 96000 },
            subInst,
            { YdspIrOp::loadInput, 5, 2, -1, -1, 0 },
            { YdspIrOp::addF, 6, 5, 5 },
            { YdspIrOp::ltI, 7, 4, 2 },
            addInst,
            { YdspIrOp::selectB, 9, 7, 8, 4 },
            { YdspIrOp::itof, 10, 9 },
            { YdspIrOp::addF, 10, 10, 6 },
            { YdspIrOp::storeOutput, -1, 2, 10, -1, 0 }
        };

        // ARM64: subs + add + csel mi, zero cmp.
        {
            YdspDiagnostics armDiag;
            const auto armArt = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::arm64 }, fn, armDiag);
            ASSERT_TRUE (armArt.wasOk()) << armDiag.toString();

            const auto listing = armDiag.toString();
            int subsCount = 0;
            int cmpCount = 0;
            int cselMiCount = 0;

            for (const auto& line : StringArray::fromLines (listing))
            {
                const auto trimmed = line.upToFirstOccurrenceOf (";", false, false).trim();
                if (trimmed.startsWith ("subs "))
                    ++subsCount;
                else if (trimmed.startsWith ("cmp "))
                    ++cmpCount;
                else if (trimmed.startsWith ("csel ")
                         && (trimmed.endsWith (", mi")
                             || trimmed.endsWith (", " + String (static_cast<uint32_t> (asmjit::arm::CondCode::kMI)))))
                    ++cselMiCount;
            }

            EXPECT_EQ (1, subsCount) << listing;
            EXPECT_EQ (0, cmpCount) << listing;
            EXPECT_EQ (1, cselMiCount) << listing;
        }

        // x64: sub + lea + cmovs, zero cmp.
        {
            YdspDiagnostics x64Diag;
            const auto x64Art = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::x64 }, fn, x64Diag);
            ASSERT_TRUE (x64Art.wasOk()) << x64Diag.toString();

            const auto listing = x64Diag.toString();
            int cmpCount = 0;
            int cmovsCount = 0;

            for (const auto& line : StringArray::fromLines (listing))
            {
                const auto trimmed = line.trimStart();
                if (trimmed.startsWith ("cmp "))
                    ++cmpCount;
                else if (trimmed.startsWith ("cmovs "))
                    ++cmovsCount;
            }

            EXPECT_EQ (0, cmpCount) << listing;
            EXPECT_EQ (1, cmovsCount) << listing;
        }

        YdspDiagnostics diagnostics;
        asmjit::JitRuntime runtime;
        const auto compiled = YdspAsmJitCodegen::compile (runtime, fn, diagnostics);
        ASSERT_NE (nullptr, compiled) << diagnostics.toString();

        for (const int idx : { 0, 2, 4, 10 })
        {
            float in = 0.5f;
            float out = 0.0f;
            void* inPtrs[] { &in };
            void* outPtrs[] { &out };
            YdspKernelContext ctx;
            ctx.inputs = inPtrs;
            ctx.outputs = outPtrs;
            ctx.numSamples = idx;
            compiled (&ctx);

            const int expectedIdx = (idx - 4 < 0) ? (idx - 4 + 96000) : (idx - 4);
            EXPECT_FLOAT_EQ (static_cast<float> (expectedIdx) + 1.0f, out) << idx;
        }
    }

    // 2. Full Echo & Karplus YDSP patches compile and wrap negative indices accurately.
    {
        YdspDiagnostics diagnostics;
        auto echoKernel = compileKernel (R"YDSP(
            processor EchoWrap {
                input stream in;
                output stream out;
                input parameter float delaySamples = 3.0;
                state float buf[16];
                state int wp;
                process {
                    int d = int (clamp (delaySamples, 1.0, 15.0));
                    int readA = wp - d;
                    if (readA < 0) { readA = readA + 16; }
                    int readB = readA - 1;
                    if (readB < 0) { readB = 15; }
                    buf[wp] = in;
                    out = buf[readA] + buf[readB];
                    wp = wp + 1;
                    if (wp >= 16) { wp = 0; }
                }
            }
            graph G { input stream x; output stream y; node e = EchoWrap; connection { x -> e.in; e.out -> y; } }
        )YDSP",
                                         "EchoWrap",
                                         diagnostics);

        ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
        ASSERT_NE (nullptr, echoKernel.fn);

        std::vector<float> input (20, 0.0f);
        for (size_t i = 0; i < input.size(); ++i)
            input[i] = static_cast<float> (i + 1);

        const auto output = runKernel (echoKernel, input, static_cast<int> (input.size()), { 3.0f });

        std::vector<float> ring (16, 0.0f);
        int wp = 0;

        for (size_t i = 0; i < input.size(); ++i)
        {
            int readA = wp - 3;
            if (readA < 0)
                readA += 16;

            int readB = readA - 1;
            if (readB < 0)
                readB = 15;

            ring[static_cast<size_t> (wp)] = input[i];
            const float expected = ring[static_cast<size_t> (readA)] + ring[static_cast<size_t> (readB)];
            wp = (wp + 1 >= 16) ? 0 : (wp + 1);

            EXPECT_FLOAT_EQ (expected, output[i]) << i;
        }

        dumpAsmOnFailureCodegen (echoKernel);
    }
}

TEST (YdspAsmJitCodegenTests, StrictScalarLibmCachesDoubleTargetInPrologueOnArm64)
{
    YdspIrFunction fn;
    fn.name = "StrictSinLoop";
    fn.fastMath = false;
    fn.numInputs = 1;
    fn.numOutputs = 1;
    fn.inputTypes = { YdspValueType::float32Type };
    fn.outputTypes = { YdspValueType::float32Type };
    fn.valueTypes = {
        YdspValueType::int32Type,   // 0: zero
        YdspValueType::int32Type,   // 1: numSamples
        YdspValueType::int32Type,   // 2: one
        YdspValueType::int32Type,   // 3: i
        YdspValueType::boolType,    // 4: cond
        YdspValueType::float32Type, // 5: x
        YdspValueType::float32Type, // 6: sin(x)
        YdspValueType::int32Type    // 7: next
    };
    fn.valueLanes = { 1, 1, 1, 1, 1, 1, 1, 1 };
    fn.blocks.resize (4);
    fn.blocks[0].insts = {
        { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
        { YdspIrOp::loadBlockSize, 1 },
        { YdspIrOp::constI, 2, -1, -1, -1, -1, 0.0, 1 },
        { YdspIrOp::movI, 3, 0 }
    };
    fn.blocks[0].term = YdspIrTerm::fallthrough;
    fn.blocks[1].insts = { { YdspIrOp::ltI, 4, 3, 1 } };
    fn.blocks[1].term = YdspIrTerm::branchIf;
    fn.blocks[1].termCond = 4;
    fn.blocks[1].termTarget = 2;
    fn.blocks[1].termTarget2 = 3;
    fn.blocks[2].insts = {
        { YdspIrOp::loadInput, 5, 3, -1, -1, 0 },
        { YdspIrOp::sinF, 6, 5 },
        { YdspIrOp::storeOutput, -1, 3, 6, -1, 0 },
        { YdspIrOp::addI, 7, 3, 2 },
        { YdspIrOp::movI, 3, 7 }
    };
    fn.blocks[2].term = YdspIrTerm::branch;
    fn.blocks[2].termTarget = 1;
    fn.blocks[3].term = YdspIrTerm::fallthrough;
    fn.loops.push_back ({ 0, 1, 3, 3, { YdspLoopBoundKind::blockSize, 0 }, false });

    YdspDiagnostics diag;
    const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::arm64 }, fn, diag);
    ASSERT_TRUE (artifact.wasOk()) << diag.toString();
    const auto listing = diag.toString();

    // The f64 libm target pointer is loaded once in the prologue (before loop header L3:),
    // not inside the loop body.
    const auto headerPos = listing.indexOf ("L3:");
    const auto poolLoadPos = listing.indexOf ("ldr x23, [L");
    ASSERT_GT (headerPos, 0) << listing;
    ASSERT_GE (poolLoadPos, 0) << listing;
    EXPECT_LT (poolLoadPos, headerPos) << listing;
}

TEST (YdspAsmJitCodegenTests, ReusesAdjacentUnhoistedArrayBaseOnArm64)
{
    // Build > 22 live GP values so prepareStateAddressing sets maxHoistedBases = 0,
    // and verify adjacent load+store pairs on the same high-offset array reuse
    // the computed base address instead of repeating add #hi, lsl #12 + add #lo.
    constexpr int numDelays = 24;
    constexpr int arrayLen = 200;
    YdspIrFunction fn;
    fn.name = "AdjacentUnhoistedBase";
    fn.int32Scalars = numDelays;
    fn.float32ArrayElements = numDelays * arrayLen;

    const auto addVal = [&] (YdspValueType t) -> int
    {
        const int id = static_cast<int> (fn.valueTypes.size());
        fn.valueTypes.push_back (t);
        fn.valueLanes.push_back (1);
        return id;
    };

    std::vector<int> idxVals (numDelays);
    for (int k = 0; k < numDelays; ++k)
        idxVals[static_cast<size_t> (k)] = addVal (YdspValueType::int32Type);

    fn.blocks.resize (1);
    for (int k = 0; k < numDelays; ++k)
        fn.blocks[0].insts.push_back ({ YdspIrOp::loadStateI, idxVals[static_cast<size_t> (k)], k });

    for (int k = 0; k < numDelays; ++k)
    {
        const int baseElem = k * arrayLen;
        const int loaded = addVal (YdspValueType::float32Type);
        const int doubled = addVal (YdspValueType::float32Type);
        const int idx = idxVals[static_cast<size_t> (k)];
        fn.blocks[0].insts.push_back ({ YdspIrOp::loadStateArrayF, loaded, idx, -1, -1, baseElem });
        fn.blocks[0].insts.push_back ({ YdspIrOp::addF, doubled, loaded, loaded });
        fn.blocks[0].insts.push_back ({ YdspIrOp::storeStateArrayF, -1, idx, doubled, -1, baseElem });
    }

    YdspDiagnostics diag;
    const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::arm64 }, fn, diag);
    ASSERT_TRUE (artifact.wasOk()) << diag.toString();
    const auto listing = diag.toString();

    int shiftedAdds = 0;
    for (const auto& line : StringArray::fromLines (listing))
    {
        const auto inst = line.upToFirstOccurrenceOf (";", false, false).trim();
        if (inst.startsWith ("add ") && inst.endsWith (", 12"))
            ++shiftedAdds;
    }

    // Arrays 6..23 (18 arrays) have byte offsets > 4095; each pair shares one
    // shifted add instead of emitting two.
    EXPECT_EQ (18, shiftedAdds) << listing;
}

TEST (YdspAsmJitCodegenTests, LocalizesExcessRingIndicesUnderHighGpPressureOnArm64)
{
    constexpr int numDelays = 24;
    constexpr int arrayStride = 160;
    YdspIrFunction fn;
    fn.isSampleMode = true;
    fn.name = "LocalizeIndices24";
    fn.numInputs = 1;
    fn.numOutputs = 1;
    fn.inputTypes = { YdspValueType::float32Type };
    fn.outputTypes = { YdspValueType::float32Type };
    fn.int32Scalars = numDelays;
    fn.float32ArrayElements = numDelays * arrayStride;

    const auto addVal = [&] (YdspValueType t) -> int
    {
        const int id = static_cast<int> (fn.valueTypes.size());
        fn.valueTypes.push_back (t);
        fn.valueLanes.push_back (1);
        return id;
    };

    const int zeroVal = addVal (YdspValueType::int32Type);
    const int numSamplesVal = addVal (YdspValueType::int32Type);
    const int oneVal = addVal (YdspValueType::int32Type);
    const int sampleIdxVal = addVal (YdspValueType::int32Type);
    const int condVal = addVal (YdspValueType::boolType);
    const int inSampleVal = addVal (YdspValueType::float32Type);

    std::vector<int> wpVals (numDelays);
    for (int k = 0; k < numDelays; ++k)
        wpVals[static_cast<size_t> (k)] = addVal (YdspValueType::int32Type);

    fn.blocks.resize (4);
    fn.blocks[0].insts = {
        { YdspIrOp::constI, zeroVal, -1, -1, -1, -1, 0.0, 0 },
        { YdspIrOp::loadBlockSize, numSamplesVal },
        { YdspIrOp::constI, oneVal, -1, -1, -1, -1, 0.0, 1 },
        { YdspIrOp::movI, sampleIdxVal, zeroVal }
    };
    for (int k = 0; k < numDelays; ++k)
        fn.blocks[0].insts.push_back ({ YdspIrOp::loadStateI, wpVals[static_cast<size_t> (k)], k });
    fn.blocks[0].term = YdspIrTerm::fallthrough;

    fn.blocks[1].insts = { { YdspIrOp::ltI, condVal, sampleIdxVal, numSamplesVal } };
    fn.blocks[1].term = YdspIrTerm::branchIf;
    fn.blocks[1].termCond = condVal;
    fn.blocks[1].termTarget = 2;
    fn.blocks[1].termTarget2 = 3;

    fn.blocks[2].insts = { { YdspIrOp::loadInput, inSampleVal, sampleIdxVal, -1, -1, 0 } };
    int accVal = inSampleVal;
    for (int k = 0; k < numDelays; ++k)
    {
        const int wp = wpVals[static_cast<size_t> (k)];
        const int tapVal = addVal (YdspValueType::float32Type);
        const int sumVal = addVal (YdspValueType::float32Type);
        const int baseElem = k * arrayStride;
        fn.blocks[2].insts.push_back ({ YdspIrOp::loadStateArrayF, tapVal, wp, -1, -1, baseElem });
        fn.blocks[2].insts.push_back ({ YdspIrOp::storeStateArrayF, -1, wp, inSampleVal, -1, baseElem });
        fn.blocks[2].insts.push_back ({ YdspIrOp::advanceWrapI, wp, wp, -1, -1, -1, 0.0, 113 + k });
        fn.blocks[2].insts.push_back ({ YdspIrOp::addF, sumVal, accVal, tapVal });
        accVal = sumVal;
    }
    const int nextSampleVal = addVal (YdspValueType::int32Type);
    fn.blocks[2].insts.push_back ({ YdspIrOp::storeOutput, -1, sampleIdxVal, accVal, -1, 0 });
    fn.blocks[2].insts.push_back ({ YdspIrOp::addI, nextSampleVal, sampleIdxVal, oneVal });
    fn.blocks[2].insts.push_back ({ YdspIrOp::movI, sampleIdxVal, nextSampleVal });
    fn.blocks[2].term = YdspIrTerm::branch;
    fn.blocks[2].termTarget = 1;

    for (int k = 0; k < numDelays; ++k)
        fn.blocks[3].insts.push_back ({ YdspIrOp::storeStateI, -1, wpVals[static_cast<size_t> (k)], -1, -1, k });
    fn.blocks[3].term = YdspIrTerm::fallthrough;
    fn.loops.push_back ({ 0, 1, 3, sampleIdxVal, { YdspLoopBoundKind::blockSize, 0 }, false });

    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    optimizer.setTargetHasFusedMultiplyAdd (true);
    optimizer.setVectorWidth (4);
    optimizer.localizeHighPressureStateIndices (fn);

    int preheaderLoads = 0;
    for (const auto& inst : fn.blocks[0].insts)
        if (inst.op == YdspIrOp::loadStateI)
            ++preheaderLoads;

    int bodyLoads = 0;
    int bodyStores = 0;
    for (const auto& inst : fn.blocks[2].insts)
    {
        if (inst.op == YdspIrOp::loadStateI)
            ++bodyLoads;
        else if (inst.op == YdspIrOp::storeStateI)
            ++bodyStores;
    }

    EXPECT_EQ (14, preheaderLoads);
    EXPECT_EQ (10, bodyLoads);
    EXPECT_EQ (10, bodyStores);

    // Verify compiled runtime behavior matches scalar ring reference across blocks.
    asmjit::JitRuntime runtime;
    const auto compiled = YdspAsmJitCodegen::compile (runtime, fn, diagnostics);
    ASSERT_NE (nullptr, compiled) << diagnostics.toString();

    std::vector<uint8_t> state (YdspAsmJitCodegen::stateSize (fn), 0);
    std::vector<int32_t> refWps (numDelays, 0);
    std::vector<float> refArrays (static_cast<size_t> (numDelays * arrayStride), 0.0f);

    for (int block = 0; block < 3; ++block)
    {
        constexpr int nSamples = 64;
        std::vector<float> input (nSamples);
        for (int i = 0; i < nSamples; ++i)
            input[static_cast<size_t> (i)] = 0.05f * static_cast<float> (block * nSamples + i + 1);

        std::vector<float> actual (nSamples, 0.0f);
        void* inPtrs[] { input.data() };
        void* outPtrs[] { actual.data() };
        YdspKernelContext ctx;
        ctx.inputs = inPtrs;
        ctx.outputs = outPtrs;
        ctx.state = state.data();
        ctx.stateArrays = state.data() + YdspAsmJitCodegen::stateScalarSize (fn);
        ctx.numSamples = nSamples;
        compiled (&ctx);

        for (int i = 0; i < nSamples; ++i)
        {
            const float x = input[static_cast<size_t> (i)];
            float acc = x;
            for (int k = 0; k < numDelays; ++k)
            {
                const int baseElem = k * arrayStride;
                acc += refArrays[static_cast<size_t> (baseElem + refWps[static_cast<size_t> (k)])];
                refArrays[static_cast<size_t> (baseElem + refWps[static_cast<size_t> (k)])] = x;
                const int bound = 113 + k;
                refWps[static_cast<size_t> (k)] = (refWps[static_cast<size_t> (k)] + 1 >= bound) ? 0 : (refWps[static_cast<size_t> (k)] + 1);
            }
            EXPECT_NEAR (acc, actual[static_cast<size_t> (i)], 1e-4f) << "block=" << block << " i=" << i;
        }
    }
}

TEST (YdspAsmJitCodegenTests, SpillsBeyondTheLoadStoreOffsetRangeAssemble)
{
    // 5000 values loaded before the first add are all live at once, so their spill slots span about 20 KB of
    // stack: past the 16380-byte offset a 32-bit AArch64 load or store can encode.
    constexpr int numValues = 5000;
    constexpr int numElements = 64;

    YdspIrFunction fn;
    fn.name = "LargeSpillArea";
    fn.isSampleMode = false;
    fn.float32ArrayElements = numElements + 1;
    fn.valueTypes.push_back (YdspValueType::int32Type);
    fn.blocks.resize (1);
    auto& insts = fn.blocks[0].insts;
    insts.push_back ({ YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 });

    for (int i = 0; i < numValues; ++i)
    {
        fn.valueTypes.push_back (YdspValueType::float32Type);
        insts.push_back ({ YdspIrOp::loadStateArrayF, i + 1, 0, -1, -1, i % numElements });
    }

    int sum = 1;
    for (int i = 1; i < numValues; ++i)
    {
        fn.valueTypes.push_back (YdspValueType::float32Type);
        const int result = static_cast<int> (fn.valueTypes.size()) - 1;
        insts.push_back ({ YdspIrOp::addF, result, sum, i + 1 });
        sum = result;
    }

    insts.push_back ({ YdspIrOp::storeStateArrayF, -1, 0, sum, -1, numElements });

    for (const auto architecture : { YdspTargetArchitecture::arm64, YdspTargetArchitecture::x64 })
    {
        YdspDiagnostics diagnostics;
        const auto artifact = YdspAsmJitCodegen::emit ({ YdspTargetOperatingSystem::linuxTarget, architecture }, fn, diagnostics);
        ASSERT_TRUE (artifact.wasOk()) << diagnostics.toString();
    }

    YdspDiagnostics diagnostics;
    asmjit::JitRuntime runtime;
    const auto kernel = YdspAsmJitCodegen::compile (runtime, fn, diagnostics);
    ASSERT_NE (nullptr, kernel) << diagnostics.toString();

    std::vector<float> stateArrays (static_cast<size_t> (numElements + 1), 0.0f);
    for (int i = 0; i < numElements; ++i)
        stateArrays[static_cast<size_t> (i)] = static_cast<float> (i + 1);

    YdspKernelContext context;
    context.stateArrays = stateArrays.data();
    kernel (&context);

    float expected = 0.0f;
    for (int i = 0; i < numValues; ++i)
        expected += static_cast<float> (i % numElements + 1);

    EXPECT_EQ (expected, stateArrays[static_cast<size_t> (numElements)]);
}

} // namespace yup::test
