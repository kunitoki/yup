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
#include <iostream>
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

CompiledKernel compileKernel (StringRef source, const char* kernelName, DspJitDiagnostics& diagnostics)
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
    DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor Gain { input stream in; output stream out; input value float g = 2; process { out = in * g; } }
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
    DspJitDiagnostics diagnostics;

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

TEST (YdspAsmJitCodegenTests, CompilesTwoInputTanhClipper)
{
    DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

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

TEST (YdspAsmJitCodegenTests, StatePersistsAcrossBlocks)
{
    DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor BlockGain {
            input stream in;
            output stream out;
            input value float drive = 3;
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
    DspJitDiagnostics diagnostics;

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

TEST (YdspAsmJitCodegenTests, CompilesMathIntrinsics)
{
    DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

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

TEST (YdspAsmJitCodegenTests, IntegerAbsOfIntMinReturnsIntMin)
{
    // The branchless idiom's true magnitude is unrepresentable for INT_MIN,
    // so it returns INT_MIN unchanged - documented in the language spec
    // (section 2.8) rather than left for a caller to discover.
    //
    // `x` is derived from a state load (always 0, but never a compile-time
    // literal) rather than a literal expression: the constant-folding pass
    // computes in unbounded int64_t, so folding `-2147483648` at int32 width
    // would not reproduce the hardware's 32-bit wraparound the way the real
    // subI/absI codegen below does. This exercises that codegen, not the
    // folder.
    DspJitDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor AbsIntMin {
            input stream in;
            output stream out;
            state int base;
            process {
                let x = base - 2147483648;
                let y = abs (x);
                out = in + select (y == x, 1.0, 0.0);
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
    DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

    auto kernel = compileKernel (R"YDSP(
        processor StereoGain {
            input stream leftIn;
            input stream rightIn;
            output stream leftOut;
            output stream rightOut;
            input value float gain = 1;
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
    DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

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

TEST (YdspAsmJitCodegenTests, CompilesFloat64StreamsAndMath)
{
    DspJitDiagnostics diagnostics;

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

TEST (YdspAsmJitCodegenTests, CompilesInt64ArithmeticAndConversion)
{
    DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

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

CompiledEventHandler compileEventHandlerFn (StringRef source, const char* handlerName, DspJitDiagnostics& diagnostics)
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
    DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

    auto handler = compileEventHandlerFn (R"YDSP(
        processor Voice {
            output stream out;
            input event midi;
            input value float gain = 0.25;
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
    DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

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
        DspJitDiagnostics diagnostics;

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
        DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

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
    DspJitDiagnostics diagnostics;

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

} // namespace yup::test
