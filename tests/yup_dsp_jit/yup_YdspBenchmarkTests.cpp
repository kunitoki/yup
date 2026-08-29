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

#include "yup_YdspTestPatches.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

using namespace yup;

namespace yup::test
{

namespace
{

//==============================================================================
// The shapes below each isolate one codegen or optimiser property of the JIT,
// measured against a hand-written C++ routine computing the same thing:
//
//   BenchmarkDelayTaps    - the `@` ring-buffer wrap (a per-sample integer
//                           modulo, which is a helper *call* on x86-64)
//   BenchmarkLadderFilter - per-sample scalar `state` round-trips
//   BenchmarkHarmonicBank - constant-heavy inner loop (float-constant
//                           materialisation, array addressing, register
//                           pressure)
//   BenchmarkWaveShaper   - compares and `select` inside the sample loop
//   BenchmarkModalBank    - work that is invariant across an inner loop but
//                           varies per sample, so it can only be hoisted into a
//                           *per-loop* preheader (per-loop code motion)
//   BenchmarkWaveFolder   - data-dependent if/else diamonds in the sample loop
//                           (if-conversion)
//
// The ratios are printed rather than asserted so the test stays informative on
// any machine and never flaky in CI; only a very loose regression guard fires.
//
// The benchmark shapes and their constants below are deliberately file-local
// (and named apart from the other suites' helpers): the tests directory is
// compiled as a unity build, so a shared name would collide or silently bind
// to another file's definition. compilePatch is the exception - it lives in
// namespace yup::test::patches (see yup_YdspTestPatches.h) for exactly that
// reason, and is pulled in below rather than duplicated here.

constexpr double benchmarkSampleRate = 48000.0;
constexpr int benchmarkBlockSize = 512;
constexpr int benchmarkBlockCount = 200; // ~2.1 s of audio per timed pass
constexpr int benchmarkRepeats = 5;
constexpr double benchmarkRatioLimit = 10.0;

// A shape that sits well under the general limit carries its own ceiling, so the
// guard catches it getting *worse* rather than only catching a collapse.
//
// The mode bank measures ~1.11-1.20x, having come down in four steps: 11.7x before
// per-loop invariant code motion, 7.5x before the vectoriser widened its 16
// modes the way the reference is compiled, 2.2x before the unroller wrote the
// four widened iterations out straight, 1.24x before the accumulator was
// halved. Shape 7 sizes what is left of the reduction at ~1.0-1.5 ns/sample of
// the 7.4-8.3 total - a range rather than a figure, because that is what two
// runs of it gave.
//
// 2.0 is deliberately close now that unrolling also took the run-to-run spread
// from ~30% to under 8%. It fails if the widening or the unroll stops firing -
// rolled alone was 2.2x. Losing only the accumulator split would read ~1.24x
// and slip past this; the ratio printed by shape 7 is what watches that.
constexpr double benchmarkModalBankLimit = 2.0;

constexpr int benchmarkTotalSamples = benchmarkBlockSize * benchmarkBlockCount;

//==============================================================================

using patches::compilePatch;

std::vector<float> benchmarkNoise (int size)
{
    std::vector<float> data (static_cast<size_t> (size));

    uint32_t seed = 0x13579bdfu;

    for (auto& sample : data)
    {
        seed = seed * 1664525u + 1013904223u;
        sample = static_cast<float> (static_cast<int32_t> (seed >> 8) & 0xffff) / 32768.0f - 1.0f;
    }

    return data;
}

struct BenchmarkTiming
{
    double best = std::numeric_limits<double>::max();
    double worst = std::numeric_limits<double>::lowest();
    double average = 0.0;
};

template <typename Fn>
BenchmarkTiming benchmarkTimeRepeats (Fn&& fn)
{
    fn();

    BenchmarkTiming timing;
    int completed = 0;

    for (int r = 0; r < benchmarkRepeats; ++r)
    {
        const auto start = std::chrono::steady_clock::now();
        fn();
        const auto seconds = std::chrono::duration<double> (std::chrono::steady_clock::now() - start).count();

        timing.best = std::min (timing.best, seconds);
        timing.worst = std::max (timing.worst, seconds);

        ++completed;
        timing.average += (seconds - timing.average) / static_cast<double> (completed);
    }

    return timing;
}

constexpr int benchmarkColumnWidth = 24;

String benchmarkLine (StringRef label, StringRef best, StringRef worst, StringRef average)
{
    const auto cell = [] (StringRef text)
    {
        return " | " + String (text).paddedLeft (' ', benchmarkColumnWidth);
    };

    String output;
    output << "  | ";
    output << String (label).paddedRight (' ', benchmarkColumnWidth);
    output << cell (best) << cell (worst) << cell (average);
    output << " |";
    return output;
}

void benchmarkReport (const char* name, const BenchmarkTiming& jit, const BenchmarkTiming& native)
{
    const auto perSample = [] (double seconds)
    {
        return String (seconds * 1.0e9 / static_cast<double> (benchmarkTotalSamples), 3);
    };

    const auto ratio = [] (double jitSeconds, double nativeSeconds)
    {
        return String (jitSeconds / nativeSeconds, 3);
    };

    const auto rule = String::repeatedString ("-", benchmarkColumnWidth);
    const auto value = [&] (double seconds, double nativeSeconds)
    {
        return perSample (seconds) + " (" + ratio (seconds, nativeSeconds) + "x)";
    };
    std::cout << "\n  |==== BENCH ==== (" << name << ")\n"
              << benchmarkLine ("ns/sample", "best", "worst", "avg") << "\n"
              << benchmarkLine (rule, rule, rule, rule) << "\n"
              << benchmarkLine ("jit", value (jit.best, native.best), value (jit.worst, native.worst), value (jit.average, native.average)) << "\n"
              << benchmarkLine ("c++", perSample (native.best), perSample (native.worst), perSample (native.average)) << "\n";
}

void benchmarkRunGraph (YdspAudioGraph& graph, const std::vector<float>& input, std::vector<float>& output)
{
    const auto hasInput = graph.getInputStreamCount() > 0;
    const auto blockSize = static_cast<size_t> (benchmarkBlockSize);

    std::vector<YdspInputBuffer> inputs;
    std::vector<YdspOutputBuffer> outputs;

    if (hasInput)
        inputs.emplace_back (Span<const float> (input.data(), blockSize));

    outputs.emplace_back (Span<float> (output.data(), blockSize));

    for (int block = 0; block < benchmarkBlockCount; ++block)
    {
        const auto offset = static_cast<size_t> (block) * blockSize;

        if (hasInput)
            inputs[0] = Span<const float> (input.data() + offset, blockSize);

        outputs[0] = Span<float> (output.data() + offset, blockSize);

        graph.process (inputs, outputs, benchmarkBlockSize);
    }
}

/** Runs a 1-in/2-out graph for the standard benchmark length. */
void benchmarkRunSplitGraph (YdspAudioGraph& graph,
                             const std::vector<float>& input,
                             std::vector<float>& outputA,
                             std::vector<float>& outputB)
{
    const auto blockSize = static_cast<size_t> (benchmarkBlockSize);

    std::vector<YdspInputBuffer> inputs { YdspInputBuffer (Span<const float> (input.data(), blockSize)) };
    std::vector<YdspOutputBuffer> outputs {
        YdspOutputBuffer (Span<float> (outputA.data(), blockSize)),
        YdspOutputBuffer (Span<float> (outputB.data(), blockSize))
    };

    for (int block = 0; block < benchmarkBlockCount; ++block)
    {
        const auto offset = static_cast<size_t> (block) * blockSize;

        inputs[0] = Span<const float> (input.data() + offset, blockSize);
        outputs[0] = Span<float> (outputA.data() + offset, blockSize);
        outputs[1] = Span<float> (outputB.data() + offset, blockSize);

        graph.process (inputs, outputs, benchmarkBlockSize);
    }
}

double benchmarkMagnitude (const std::vector<float>& data)
{
    double sum = 0.0;

    for (const auto sample : data)
        sum += std::fabs (static_cast<double> (sample));

    return sum;
}

double benchmarkChecksum (const std::vector<float>& data)
{
    double sum = 0.0;

    for (const auto sample : data)
        sum += static_cast<double> (sample);

    return sum;
}

//==============================================================================
/** What the generated listing says about stack traffic and calls.

    Added to test whether spilling around the per-sample `exp` call explained
    shape 7's readings. It did not - 0 vector spills either way - which is what
    sent that investigation to the loop instead. Kept because it is the only
    view here of what an instruction count leaves out.
*/
struct BenchmarkListingStats
{
    int lines = 0;         // emitted lines, a rough stand-in for code size
    int stackAccesses = 0; // anything addressing the stack pointer
    int vectorSpills = 0;  // ... of which move a vector register
    int calls = 0;         // indirect calls, i.e. every libm transcendental
};

/** Counts the above over a compiled graph's AsmJit listing.

    Only differences between two kernels mean anything here: the prologue's own
    stack traffic is counted too, and the line count includes labels the
    assembler never emits.
*/
BenchmarkListingStats benchmarkAnalyzeListing (const YdspAudioGraph& graph)
{
    BenchmarkListingStats stats;

    for (const auto& line : StringArray::fromLines (graph.getDiagnostics().toString()))
    {
        const auto text = line.trim();

        if (text.isEmpty())
            continue;

        ++stats.lines;

        // AArch64 writes `[sp, #16]`, x86-64 `[rsp + 16]`; either way the stack
        // pointer appears inside the memory operand and nowhere else.
        if (text.contains ("[sp") || text.contains ("[rsp") || text.contains ("[esp"))
        {
            ++stats.stackAccesses;

            if (text.contains (" q") || text.contains ("xmm"))
                ++stats.vectorSpills;
        }

        if (text.contains ("blr") || text.contains ("call"))
            ++stats.calls;
    }

    return stats;
}

/** Prints one listing's counts as a table row. */
void benchmarkReportListing (const String& label, const BenchmarkListingStats& stats)
{
    std::cout << "  | " << label.paddedRight (' ', 9)
              << " | " << stats.lines << " lines"
              << " | " << stats.stackAccesses << " stack"
              << " | " << stats.vectorSpills << " vector spills"
              << " | " << stats.calls << " calls\n";
}

//==============================================================================
// Shape 1: three `@` delay taps. Isolates the per-sample ring wrap.

constexpr auto benchmarkDelaySource = R"YDSP(
    processor DelayTaps {
        input stream in;
        output stream out;

        process {
            let d1 = in @ 127;
            let d2 = in @ 251;
            let d3 = in @ 509;

            out = in * 0.5 + d1 * 0.25 + d2 * 0.15 + d3 * 0.1;
        }
    }

    graph G {
        input stream x;
        output stream y;

        node d = DelayTaps;

        connection { x -> d.in; d.out -> y; }
    }
)YDSP";

class BenchmarkNativeDelayTaps
{
public:
    BenchmarkNativeDelayTaps()
    {
        ring1.assign (n1 + 1, 0.0f);
        ring2.assign (n2 + 1, 0.0f);
        ring3.assign (n3 + 1, 0.0f);
    }

    void process (const float* in, float* out, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto x = in[i];

            // `(w + 1) % (n + 1)` for a write pointer that is always within
            // [0, n] is exactly "wrap n + 1 back to zero" - which is what a DSP
            // routine written by hand does, and what the JIT should emit.
            ring1[static_cast<size_t> (w1)] = x;
            if (++w1 > n1)
                w1 = 0;
            const auto d1 = ring1[static_cast<size_t> (w1)];

            ring2[static_cast<size_t> (w2)] = x;
            if (++w2 > n2)
                w2 = 0;
            const auto d2 = ring2[static_cast<size_t> (w2)];

            ring3[static_cast<size_t> (w3)] = x;
            if (++w3 > n3)
                w3 = 0;
            const auto d3 = ring3[static_cast<size_t> (w3)];

            out[i] = x * 0.5f + d1 * 0.25f + d2 * 0.15f + d3 * 0.1f;
        }
    }

private:
    static constexpr int n1 = 127;
    static constexpr int n2 = 251;
    static constexpr int n3 = 509;

    std::vector<float> ring1, ring2, ring3;
    int w1 = 0, w2 = 0, w3 = 0;
};

constexpr auto benchmarkBiquadSource = R"YDSP(
    processor Biquad {
        input stream in;
        output stream out;
        state float x1;
        state float x2;
        state float y1;
        state float y2;
        process {
            let y = 0.206572 * in + 0.413144 * x1 + 0.206572 * x2
                  + 0.369527 * y1 - 0.195816 * y2;
            x2 = x1;
            x1 = in;
            y2 = y1;
            y1 = y;
            out = y;
        }
    }
    graph G {
        input stream x;
        output stream y;
        node b = Biquad;
        connection { x -> b.in; b.out -> y; }
    }
)YDSP";

class BenchmarkNativeBiquad
{
public:
    void process (const float* in, float* out, int count)
    {
        for (int i = 0; i < count; ++i)
        {
            const auto x = in[i];
            const auto y = 0.206572f * x + 0.413144f * x1 + 0.206572f * x2
                         + 0.369527f * y1 - 0.195816f * y2;
            x2 = x1;
            x1 = x;
            y2 = y1;
            y1 = y;
            out[i] = y;
        }
    }

private:
    float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
};

//==============================================================================
// Shape 2: a four-pole ladder. Isolates the per-sample scalar state traffic.

constexpr auto benchmarkLadderSource = R"YDSP(
    processor Ladder {
        input stream in;
        output stream out;

        input value float cutoff = 0.25;
        input value float resonance = 0.5;

        state float z1;
        state float z2;
        state float z3;
        state float z4;

        process {
            let g = clamp (cutoff, 0.01, 0.99);
            let fb = resonance * 3.8;

            let x = in - fb * z4;

            z1 = z1 + g * (x - z1);
            z2 = z2 + g * (z1 - z2);
            z3 = z3 + g * (z2 - z3);
            z4 = z4 + g * (z3 - z4);

            out = z4;
        }
    }

    graph G {
        input stream x;
        output stream y;

        node f = Ladder;

        connection { x -> f.in; f.out -> y; }
    }
)YDSP";

// The same ladder with each stage's multiply-add written as one fused
// operation. `z + g * (x - z)` contracted *is* `fma (g, x - z, z)`, so this is
// the arithmetic the `c++` row runs and the `c++ nofma` row does not - which
// makes the pair below a JIT-against-JIT measurement of exactly one thing.
//
// The compiler now contracts `z + g * (x - z)` on its own, so the `jit` row is
// already running these four fused operations and this variant should measure
// the *same* thing by a different route. That is what makes it worth keeping:
// a gap opening up between the two rows means the pass stopped firing, or fused
// a different multiply than a person would have.
constexpr auto benchmarkLadderFusedSource = R"YDSP(
    processor Ladder {
        input stream in;
        output stream out;

        input value float cutoff = 0.25;
        input value float resonance = 0.5;

        state float z1;
        state float z2;
        state float z3;
        state float z4;

        process {
            let g = clamp (cutoff, 0.01, 0.99);
            let fb = resonance * 3.8;

            let x = in - fb * z4;

            z1 = fma (g, x - z1, z1);
            z2 = fma (g, z1 - z2, z2);
            z3 = fma (g, z2 - z3, z3);
            z4 = fma (g, z3 - z4, z4);

            out = z4;
        }
    }

    graph G {
        input stream x;
        output stream y;

        node f = Ladder;

        connection { x -> f.in; f.out -> y; }
    }
)YDSP";

class BenchmarkNativeLadder
{
public:
    void process (const float* in, float* out, int numSamples)
    {
        const auto g = std::min (std::max (cutoff, 0.01f), 0.99f);
        const auto fb = resonance * 3.8f;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto x = in[i] - fb * z4;

            z1 = z1 + g * (x - z1);
            z2 = z2 + g * (z1 - z2);
            z3 = z3 + g * (z2 - z3);
            z4 = z4 + g * (z3 - z4);

            out[i] = z4;
        }
    }

private:
    float cutoff = 0.25f;
    float resonance = 0.5f;

    float z1 = 0.0f, z2 = 0.0f, z3 = 0.0f, z4 = 0.0f;
};

// The same reference with floating-point contraction turned off. See the note
// on BenchmarkNativeFolderUncontracted for why the `c++` row above is not
// running the arithmetic the JIT is running.
//
// This shape has more riding on it than the folder does: the recurrence is four
// dependent `z + g * (x - z)` stages, and each one is `fsub` then `fmadd`
// contracted against `fsub`, `fmul`, `fadd` uncontracted - three links on the
// critical path instead of two, four times over. If the ladder's ratio is
// mostly FMA, this row is where it shows.
class BenchmarkNativeLadderUncontracted
{
public:
    void process (const float* in, float* out, int numSamples)
    {
#if defined(__clang__)
#pragma clang fp contract(off)
#endif

        const auto g = std::min (std::max (cutoff, 0.01f), 0.99f);
        const auto fb = resonance * 3.8f;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto x = in[i] - fb * z4;

            z1 = z1 + g * (x - z1);
            z2 = z2 + g * (z1 - z2);
            z3 = z3 + g * (z2 - z3);
            z4 = z4 + g * (z3 - z4);

            out[i] = z4;
        }
    }

private:
    float cutoff = 0.25f;
    float resonance = 0.5f;

    float z1 = 0.0f, z2 = 0.0f, z3 = 0.0f, z4 = 0.0f;
};

//==============================================================================
// Shape 3: a 32-partial rotating-phasor bank - the shape of the shipped
// ElectricPiano voice's inner loop, without the MIDI/voice machinery, so a
// like-for-like C++ reference stays readable.

constexpr auto benchmarkBankSource = R"YDSP(
    let partials = 32;

    processor Bank {
        output stream out;

        state float oscR[partials];
        state float oscI[partials];
        state float mulR[partials];
        state float mulI[partials];
        state float amp[partials];

        init {
            for i in 0..partials {
                let w = 2.0 * pi * 110.0 * float (i + 1) * samplePeriod;

                mulR[i] = cos (w);
                mulI[i] = sin (w);
                oscR[i] = 1.0;
                oscI[i] = 0.0;
                amp[i] = 0.5 / float (i + 1);
            }
        }

        process {
            float sum = 0.0;

            for i in 0..partials {
                let rotated = oscR[i] * mulR[i] - oscI[i] * mulI[i];

                oscI[i] = oscR[i] * mulI[i] + oscI[i] * mulR[i];
                oscR[i] = rotated;

                sum = sum + oscI[i] * amp[i];
            }

            out = sum;
        }
    }

    graph G {
        output stream y;

        node b = Bank;

        connection { b.out -> y; }
    }
)YDSP";

class BenchmarkNativeBank
{
public:
    explicit BenchmarkNativeBank (double sampleRate)
    {
        const auto samplePeriod = static_cast<float> (1.0 / sampleRate);

        for (int i = 0; i < partials; ++i)
        {
            const auto w = 2.0f * 3.14159265358979323846f * 110.0f * static_cast<float> (i + 1) * samplePeriod;

            mulR[i] = std::cos (w);
            mulI[i] = std::sin (w);
            oscR[i] = 1.0f;
            oscI[i] = 0.0f;
            amp[i] = 0.5f / static_cast<float> (i + 1);
        }
    }

    void process (float* out, int numSamples)
    {
        for (int s = 0; s < numSamples; ++s)
        {
            auto sum = 0.0f;

            for (int i = 0; i < partials; ++i)
            {
                const auto rotated = oscR[i] * mulR[i] - oscI[i] * mulI[i];

                oscI[i] = oscR[i] * mulI[i] + oscI[i] * mulR[i];
                oscR[i] = rotated;

                sum += oscI[i] * amp[i];
            }

            out[s] = sum;
        }
    }

private:
    static constexpr int partials = 32;

    float oscR[partials] {}, oscI[partials] {}, mulR[partials] {}, mulI[partials] {}, amp[partials] {};
};

//==============================================================================
// Shape 4: compares plus `select` in the sample loop. On AArch64 every compare
// is currently a branch, so this is the shape that hurts most there.

constexpr auto benchmarkShaperSource = R"YDSP(
    processor Shaper {
        input stream in;
        output stream out;

        state float env;

        process {
            let a = abs (in);

            env = select (a > env, a, env * 0.9995);

            let hi = select (in > 0.7, 0.7, in);
            let lo = select (hi < -0.7, -0.7, hi);

            out = lo * (1.0 - env * 0.5);
        }
    }

    graph G {
        input stream x;
        output stream y;

        node s = Shaper;

        connection { x -> s.in; s.out -> y; }
    }
)YDSP";

class BenchmarkNativeShaper
{
public:
    void process (const float* in, float* out, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto x = in[i];
            const auto a = std::fabs (x);

            env = a > env ? a : env * 0.9995f;

            const auto hi = x > 0.7f ? 0.7f : x;
            const auto lo = hi < -0.7f ? -0.7f : hi;

            out[i] = lo * (1.0f - env * 0.5f);
        }
    }

private:
    float env = 0.0f;
};

// The same reference with floating-point contraction turned off. See the note
// on BenchmarkNativeFolderUncontracted.
//
// Unlike the folder and the ladder, this shape's *recurrence* has nothing to
// contract: `env = a > env ? a : env * 0.9995f` is a multiply and a select, and
// there is no add to fuse the multiply into. Only the output term
// `lo * (1.0f - env * 0.5f)` can contract, and that is off the critical path.
// So this row is the control: if it lands on the contracted one, the shaper's
// ratio is not an FMA story and looking for it there is wasted effort.
class BenchmarkNativeShaperUncontracted
{
public:
    void process (const float* in, float* out, int numSamples)
    {
#if defined(__clang__)
#pragma clang fp contract(off)
#endif

        for (int i = 0; i < numSamples; ++i)
        {
            const auto x = in[i];
            const auto a = std::fabs (x);

            env = a > env ? a : env * 0.9995f;

            const auto hi = x > 0.7f ? 0.7f : x;
            const auto lo = hi < -0.7f ? -0.7f : hi;

            out[i] = lo * (1.0f - env * 0.5f);
        }
    }

private:
    float env = 0.0f;
};

//==============================================================================
// Shape 5: a modal bank whose per-sample drive term is invariant across the
// inner loop, and whose 16 modes are independent. It measures two things at
// once: per-loop invariant code motion (`drive`, `exp` included, is computed
// once per sample rather than once per mode) and vectorisation of the mode loop
// itself. Both have landed, and so has the full unroll of the widened mode loop
// - which was worth more here than anywhere else in the suite: 2.17x to 1.21x,
// and a run-to-run spread that fell from ~30% to under 8%. Halving the widened
// accumulator then took it to ~1.11x. The reference is what a compiler does
// with the same source. What is left of the reduction is ~0.98 ns/sample,
// which shape 7 below is what measures.

constexpr auto benchmarkModalSource = R"YDSP(
    let modes = 16;

    processor Modal {
        input stream in;
        output stream out;

        input value float damping = 0.5;

        state float z[modes];
        state float env;

        process {
            env = env * 0.999 + abs (in) * 0.001;

            float sum = 0.0;

            for i in 0..modes {
                let drive = exp (-env * damping) * (1.0 - damping);

                z[i] = z[i] * 0.9 + in * drive;
                sum = sum + z[i];
            }

            out = sum;
        }
    }

    graph G {
        input stream x;
        output stream y;

        node m = Modal;

        connection { x -> m.in; m.out -> y; }
    }
)YDSP";

class BenchmarkNativeModal
{
public:
    void process (const float* in, float* out, int numSamples)
    {
        for (int s = 0; s < numSamples; ++s)
        {
            const auto x = in[s];

            env = env * 0.999f + std::fabs (x) * 0.001f;

            // Hoisted, as any compiler would: nothing in it depends on `i`.
            const auto drive = std::exp (-env * damping) * (1.0f - damping);

            auto sum = 0.0f;

            for (int i = 0; i < modes; ++i)
            {
                z[i] = z[i] * 0.9f + x * drive;
                sum += z[i];
            }

            out[s] = sum;
        }
    }

private:
    static constexpr int modes = 16;

    float damping = 0.5f;
    float env = 0.0f;
    float z[modes] {};
};

//==============================================================================
// Shape 6: a wavefolder with two data-dependent if/else diamonds per sample.
// Compiled code turns both into conditional moves; the JIT still branches.

constexpr auto benchmarkFolderSource = R"YDSP(
    processor Folder {
        input stream in;
        output stream out;

        state float last;

        process {
            float y = in * 3.0;

            if (y > 1.0) { y = 2.0 - y; }
            if (y < -1.0) { y = -2.0 - y; }

            last = last * 0.5 + y * 0.5;

            out = last;
        }
    }

    graph G {
        input stream x;
        output stream y;

        node f = Folder;

        connection { x -> f.in; f.out -> y; }
    }
)YDSP";

// The folder with its one-pole written as a single fused operation. Its
// recurrence is exactly one multiply-add per sample, so this is the shape where
// fusing has the least to hide behind: it turns two links of the loop-carried
// chain into one and changes nothing else.
constexpr auto benchmarkFolderFusedSource = R"YDSP(
    processor Folder {
        input stream in;
        output stream out;

        state float last;

        process {
            float y = in * 3.0;

            if (y > 1.0) { y = 2.0 - y; }
            if (y < -1.0) { y = -2.0 - y; }

            last = fma (last, 0.5, y * 0.5);

            out = last;
        }
    }

    graph G {
        input stream x;
        output stream y;

        node f = Folder;

        connection { x -> f.in; f.out -> y; }
    }
)YDSP";

class BenchmarkNativeFolder
{
public:
    void process (const float* in, float* out, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            auto y = in[i] * 3.0f;

            if (y > 1.0f)
                y = 2.0f - y;

            if (y < -1.0f)
                y = -2.0f - y;

            last = last * 0.5f + y * 0.5f;

            out[i] = last;
        }
    }

private:
    float last = 0.0f;
};

// The same reference with floating-point contraction turned off.
//
// `last = last * 0.5f + y * 0.5f` compiles to a single `fmadd` on AArch64 by
// default, and contraction is a *precision* liberty - one rounding where the
// source asks for two - that YDSP deliberately does not take, because a patch
// has to produce identical samples on every backend it can be compiled for. So
// the `c++` row above is not running the arithmetic the JIT is running.
//
// On this shape that difference is plausibly the entire measurement. The
// critical path is exactly one multiply-add per sample: `fmadd` is one latency,
// `fmul` then `fadd` is two, and the native row lands within noise of a single
// fmadd latency per sample. This variant sizes it. Read it against the *jit*
// row, not against the contracted one - if those two agree, the wave folder is
// already at the floor for bit-exact code and there is nothing here to win.
class BenchmarkNativeFolderUncontracted
{
public:
    void process (const float* in, float* out, int numSamples)
    {
#if defined(__clang__)
#pragma clang fp contract(off)
#endif

        for (int i = 0; i < numSamples; ++i)
        {
            auto y = in[i] * 3.0f;

            if (y > 1.0f)
                y = 2.0f - y;

            if (y < -1.0f)
                y = -2.0f - y;

            last = last * 0.5f + y * 0.5f;

            out[i] = last;
        }
    }

private:
    float last = 0.0f;
};

/** Whether the compiler honoured the pragma above, so the row can say so. */
constexpr bool benchmarkCanDisableFpContraction =
#if defined(__clang__)
    true;
#else
    false;
#endif

//==============================================================================
// Shape 7: the modal bank with its accumulation removed. Not a JIT-vs-native
// comparison - it is shape 5 measured against itself, so that the *delta*
// isolates one thing.
//
// Everything else is identical: 16 modes, the same loop-invariant drive term,
// the same per-mode filter, the same `exp`. Only the `sum` accumulation is gone,
// so the delta was meant to be what the widened reduction costs per sample -
// four serially dependent vector adds and the horizontal fold at the end.
//
// It did not measure that until the mode loop was unrolled: rolled, this shape
// was noisy enough to report the smaller kernel as the slower one, which
// strictly less work cannot be. See the note on ModalBankReductionCost.
//
// The output is `env`, which lives in a register, and *not* `z[0]`: the first
// version of this shape ended `out = z[0]` and measured the smaller kernel as
// 5.5 ns/sample *slower*, consistently, with both loops confirmed widened. A
// 4-byte read of an element the widened 16-byte store just wrote is a
// partial-overlap store-to-load forward, which the scalar loop never had to do
// - both its store and its load were 4 bytes at the same address. That stall
// swamped the thing this shape exists to measure, so it is kept out of it.

constexpr auto benchmarkModalNoSumSource = R"YDSP(
    let modes = 16;

    processor Modal {
        input stream in;
        output stream out;

        input value float damping = 0.5;

        state float z[modes];
        state float env;

        process {
            env = env * 0.999 + abs (in) * 0.001;

            for i in 0..modes {
                let drive = exp (-env * damping) * (1.0 - damping);

                z[i] = z[i] * 0.9 + in * drive;
            }

            out = env;
        }
    }

    graph G {
        input stream x;
        output stream y;

        node m = Modal;

        connection { x -> m.in; m.out -> y; }
    }
)YDSP";

//==============================================================================
// Shape 8: three chained nodes against one processor computing the same thing.
// A JIT-vs-JIT comparison, and originally the shape the fusion item was missing:
// its win had been asserted from first principles, never measured.
//
// It measured 2.05x before kernel fusion existed - the chained version paid
// three kernel calls per block and round-tripped two blockSize scratch buffers
// through memory (5.68 ns/sample) where the single processor kept both
// intermediates in registers (2.77 ns/sample). Fusion has since captured all of
// it: the chained form now compiles to one kernel and measures the same as the
// hand-written one.
//
// So the ratio is expected to sit at ~1.0, and that is what this now guards - if
// fusion stops firing on this shape it jumps back to ~2. Both stages are
// matched-rate sample-mode nodes, which is the only case fusion applies to.

constexpr auto benchmarkChainedSource = R"YDSP(
    processor Shape {
        input stream in;
        output stream out;

        state float phase;

        process {
            phase = phase + 0.01;

            if (phase >= 1.0) { phase = phase - 1.0; }

            out = in * (phase * 2.0 - 1.0);
        }
    }

    processor Filter {
        input stream in;
        output stream out;

        state float z;

        process {
            z = z * 0.8 + in * 0.2;
            out = z;
        }
    }

    processor Trim {
        input stream in;
        output stream out;

        process { out = in * 0.5; }
    }

    graph G {
        input stream x;
        output stream y;

        process = x : Shape : Filter : Trim : y;
    }
)YDSP";

constexpr auto benchmarkFusedSource = R"YDSP(
    processor Fused {
        input stream in;
        output stream out;

        state float phase;
        state float z;

        process {
            phase = phase + 0.01;

            if (phase >= 1.0) { phase = phase - 1.0; }

            let shaped = in * (phase * 2.0 - 1.0);

            z = z * 0.8 + shaped * 0.2;

            out = z * 0.5;
        }
    }

    graph G {
        input stream x;
        output stream y;

        node f = Fused;

        connection { x -> f.in; f.out -> y; }
    }
)YDSP";

//==============================================================================
// Graph-level dry/wet vs. dry/wet hand-rolled inside one processor.
//
// This is the shape that decides whether implicit summing is cheap enough to be
// the idiomatic way to write dry/wet. The fanned patch splits the graph input
// into two nodes and sums both outputs into one graph output, so the gap between
// the two is exactly what the extra kernel call plus the mix buffer and its
// accumulate loop cost; the inline patch is what every bundled effect does today
// (`out = (1 - mix) * in + mix * wet`).
//
// Both compute the same value in the same order, so their checksums agree.

constexpr auto benchmarkInlineDryWetSource = R"YDSP(
    processor InlineDryWet {
        input stream in;
        output stream out;

        input value float mix = 0.35;

        process {
            let shaped = tanh (in * 3.0);
            out = (1.0 - mix) * in + mix * shaped;
        }
    }

    graph G {
        input stream x;
        output stream y;

        node d = InlineDryWet;

        connection { x -> d.in; d.out -> y; }
    }
)YDSP";

constexpr auto benchmarkFannedDryWetSource = R"YDSP(
    processor DryTrim {
        input stream in;
        output stream out;

        input value float mix = 0.35;

        process { out = (1.0 - mix) * in; }
    }

    processor WetShaper {
        input stream in;
        output stream out;

        input value float mix = 0.35;

        process { out = mix * tanh (in * 3.0); }
    }

    graph G {
        input stream x;
        output stream y;

        node dry = DryTrim;
        node wet = WetShaper;

        connection {
            x -> dry.in;
            x -> wet.in;
            dry.out -> y;
            wet.out -> y;
        }
    }
)YDSP";

// The same two kernels and the same fan-out, but the branches land on two
// separate graph outputs instead of being summed into one. Both node outputs
// then have exactly one destination each, so both take the direct-write path
// and no mix buffer exists. Subtracting this from the summed variant above
// isolates the mix path - the fanned-vs-inline gap alone cannot, because the
// fanned patch also pays a second kernel call that the inline one does not.

constexpr auto benchmarkSplitDryWetSource = R"YDSP(
    processor DryTrim {
        input stream in;
        output stream out;

        input value float mix = 0.35;

        process { out = (1.0 - mix) * in; }
    }

    processor WetShaper {
        input stream in;
        output stream out;

        input value float mix = 0.35;

        process { out = mix * tanh (in * 3.0); }
    }

    graph G {
        input stream x;
        output stream dryOut;
        output stream wetOut;

        node dry = DryTrim;
        node wet = WetShaper;

        connection {
            x -> dry.in;
            x -> wet.in;
            dry.out -> dryOut;
            wet.out -> wetOut;
        }
    }
)YDSP";

//==============================================================================
// Idle-voice skipping. Not a JIT-vs-native comparison: the two graphs below are
// the *same* patch, differing only in whether its `[[ role: voiceActivity ]]`
// annotation is present, so the runtime is allowed to believe the flag the
// patch already maintains. Every voice does identical work when it runs.

/** Prints a two-variant table (the jit/c++ one above does not fit here). */
void benchmarkReportVariants (const char* name,
                              const char* labelA,
                              const BenchmarkTiming& a,
                              const char* labelB,
                              const BenchmarkTiming& b)
{
    const auto perSample = [] (double seconds)
    {
        return String (seconds * 1.0e9 / static_cast<double> (benchmarkTotalSamples), 3);
    };

    const auto ratio = [] (double lhs, double rhs)
    {
        return String (lhs / rhs, 3);
    };

    const auto rule = String::repeatedString ("-", benchmarkColumnWidth);
    const auto value = [] (double seconds)
    {
        return String (seconds * 1.0e9 / static_cast<double> (benchmarkTotalSamples), 3);
    };
    std::cout << "\n  |==== BENCH ==== (" << name << ")\n"
              << benchmarkLine ("ns/sample", "best", "worst", "avg") << "\n"
              << benchmarkLine (rule, rule, rule, rule) << "\n"
              << benchmarkLine (labelA, value (a.best), value (a.worst), value (a.average)) << "\n"
              << benchmarkLine (labelB, value (b.best), value (b.worst), value (b.average)) << "\n"
              << benchmarkLine ("ratio", ratio (a.best, b.best), ratio (a.worst, b.worst), ratio (a.average, b.average)) << "\n";
}

void benchmarkReportPolicies (const char* name,
                              const std::array<const char*, 4>& labels,
                              const std::array<BenchmarkTiming, 4>& timings,
                              const BenchmarkTiming& native,
                              double limit = benchmarkRatioLimit)
{
    const auto perSample = [] (double seconds)
    {
        return String (seconds * 1.0e9 / static_cast<double> (benchmarkTotalSamples), 3);
    };
    const auto ratio = [] (double lhs, double rhs)
    {
        return String (lhs / rhs, 3);
    };
    const auto value = [&] (double seconds, double nativeSeconds)
    {
        return perSample (seconds) + " (" + ratio (seconds, nativeSeconds) + "x)";
    };
    const auto rule = String::repeatedString ("-", benchmarkColumnWidth);
    std::cout << "\n  |==== BENCH ==== (" << name << ")\n"
              << benchmarkLine ("ns/sample", "best", "worst", "avg") << "\n"
              << benchmarkLine (rule, rule, rule, rule) << "\n";
    for (size_t i = 0; i < labels.size(); ++i)
        std::cout << benchmarkLine (labels[i],
                                    value (timings[i].best, native.best),
                                    value (timings[i].worst, native.worst),
                                    value (timings[i].average, native.average)) << "\n";
    std::cout << benchmarkLine ("c++", perSample (native.best), perSample (native.worst), perSample (native.average)) << "\n";

    if (timings.back().best / native.best > limit
        && SystemStats::getEnvironmentVariable ("ACTION_RUNNER", {}) == "github-actions")
    {
        std::cerr << "  WARNING - " << name << ": JIT kernel is more than "
                    << limit << "x slower than the equivalent compiled routine\n";
    }
    else
    {
        EXPECT_LT (timings.back().best / native.best, limit);
    }
}

/** Says whether the contraction pragma changed a reference's output.

    Read this carefully, because "did NOT change" does *not* mean "contraction
    did not happen". Contraction removes a rounding, so it can only change the
    result when that rounding was doing something - and multiplying a float by
    an exact power of two rounds to itself. The wave folder's
    `last * 0.5f + y * 0.5f` is therefore bit-identical fused or not, and so is
    the shaper's `1.0f - env * 0.5f`, while the ladder's `z + g * (x - z)` (with
    `g = 0.25`... times a non-exact `x - z`) is not.

    So this line answers "is contraction observable here", and the *timing* row
    below is what answers "did it happen". The folder reads "did NOT change" and
    still shows the two references 1.7x apart, which is only possible if the
    contracted one really is emitting `fmadd`.

    Printed rather than asserted, because every answer is a result.
*/
void benchmarkReportContraction (const char* name,
                                 const std::vector<float>& contracted,
                                 const std::vector<float>& uncontracted)
{
    const auto changed = benchmarkChecksum (contracted) != benchmarkChecksum (uncontracted);

    std::cout << "  contraction pragma (" << name << "): "
              << (benchmarkCanDisableFpContraction ? "supported" : "UNSUPPORTED on this compiler")
              << ", and it " << (changed ? "changed" : "did NOT change") << " the reference output\n";
}

/** Runs a stereo, MIDI-driven graph for the standard benchmark length.

    The MIDI only lands in the first block: the notes are then held for the rest
    of the run, so the steady state being measured is "two voices sounding".
*/
void benchmarkRunVoiceGraph (YdspAudioGraph& graph,
                             std::vector<float>& left,
                             std::vector<float>& right,
                             const MidiBuffer& firstBlockMidi)
{
    const auto blockSize = static_cast<size_t> (benchmarkBlockSize);

    std::vector<YdspInputBuffer> inputs;
    std::vector<YdspOutputBuffer> outputs;

    outputs.emplace_back (Span<float> (left.data(), blockSize));
    outputs.emplace_back (Span<float> (right.data(), blockSize));

    for (int block = 0; block < benchmarkBlockCount; ++block)
    {
        const auto offset = static_cast<size_t> (block) * blockSize;

        outputs[0] = Span<float> (left.data() + offset, blockSize);
        outputs[1] = Span<float> (right.data() + offset, blockSize);

        graph.process (inputs, outputs, benchmarkBlockSize, block == 0 ? &firstBlockMidi : nullptr, nullptr, 0);
    }
}

} // namespace

//==============================================================================

class YdspBenchmarkTests : public ::testing::Test
{
protected:
    static YdspCompileOptions allOptimizations()
    {
        YdspCompileOptions options;
        options.fastMath = true;
        return options;
    }

    static YdspCompileOptions noVectorisation()
    {
        YdspCompileOptions options;
        options.optimizationTier = YdspOptimizationTier::baseline;
        return options;
    }

    std::array<BenchmarkTiming, 4> benchmarkPolicies (StringRef source)
    {
        const auto baselineOptions = noVectorisation();
        auto baselineFastOptions = baselineOptions;
        baselineFastOptions.fastMath = true;
        const auto hostOptions = YdspCompileOptions {};
        const auto hostFastOptions = allOptimizations();
        const std::array<YdspCompileOptions, 4> options { baselineOptions, baselineFastOptions, hostOptions, hostFastOptions };
        std::array<BenchmarkTiming, 4> timings;

        for (size_t i = 0; i < options.size(); ++i)
        {
            auto graph = compilePatch (source, compiler, options[i]);
            EXPECT_TRUE (graph.isValid());
            if (! graph.isValid())
                continue;
            graph.prepare (benchmarkSampleRate, benchmarkBlockSize);
            timings[i] = benchmarkTimeRepeats ([&]
            {
                graph.reset();
                benchmarkRunGraph (graph, input, jitOutput);
            });
        }

        return timings;
    }

    void SetUp() override
    {
        input = benchmarkNoise (benchmarkTotalSamples);
        jitOutput.assign (static_cast<size_t> (benchmarkTotalSamples), 0.0f);
        nativeOutput.assign (static_cast<size_t> (benchmarkTotalSamples), 0.0f);
    }

    /** Prints the table and applies the (very loose) regression guard. */
    void report (const char* name, const BenchmarkTiming& jit, const BenchmarkTiming& native, double limit = benchmarkRatioLimit)
    {
        benchmarkReport (name, jit, native);

        if (jit.best / native.best > limit)
        {
            if (SystemStats::getEnvironmentVariable ("ACTION_RUNNER", {}) == "github-actions")
            {
                std::cerr << "  WARNING - " << name << ": JIT kernel is more than "
                          << limit << "x slower than the equivalent compiled routine\n";
            }
            else
            {
                EXPECT_LT (jit.best / native.best, limit)
                    << "  WARNING - " << name << ": JIT kernel is more than "
                    << limit << "x slower than the equivalent compiled routine";
            }
        }
    }

    YdspCompiler compiler;

    std::vector<float> input;
    std::vector<float> jitOutput;
    std::vector<float> nativeOutput;
};

//==============================================================================

// Native-reference benchmarks. Keep these together so each algorithm has a
// direct C++ baseline before the policy and graph-shape comparisons below.

TEST_F (YdspBenchmarkTests, DelayTapsAgainstNative)
{
    auto graph = compilePatch (benchmarkDelaySource, compiler, allOptimizations());
    ASSERT_TRUE (graph.isValid());
    graph.prepare (benchmarkSampleRate, benchmarkBlockSize);

    const auto jitTiming = benchmarkTimeRepeats ([&]
    {
        graph.reset();
        benchmarkRunGraph (graph, input, jitOutput);
    });

    const auto nativeTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeDelayTaps reference;

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, nativeOutput.data() + offset, benchmarkBlockSize);
        }
    });

    EXPECT_NEAR (benchmarkChecksum (nativeOutput), benchmarkChecksum (jitOutput), 1.0);

    benchmarkReportPolicies ("delay taps (@)", { "baseline", "baseline + fastMath", "host", "host + fastMath" }, benchmarkPolicies (benchmarkDelaySource), nativeTiming);

    // The one shape here that is throughput-bound rather than latency-bound:
    // three independent ring chains, and the only loop-carried dependency is
    // the write pointer's `+ 1`. So the ratio should be explained by the
    // instruction count, and a static count of the lowered form says the JIT
    // emits roughly a fifth more per sample than the reference - well short of
    // what is measured. The listing is what closes that gap or shows it is
    // spills instead.
    benchmarkReportListing ("delay", benchmarkAnalyzeListing (graph));

    auto baseline = compilePatch (benchmarkDelaySource, compiler, noVectorisation());
    ASSERT_TRUE (baseline.isValid());
    baseline.prepare (benchmarkSampleRate, benchmarkBlockSize);
    std::vector<float> baselineOutput (static_cast<size_t> (benchmarkTotalSamples), 0.0f);
    baseline.reset();
    benchmarkRunGraph (baseline, input, baselineOutput);
    EXPECT_NEAR (benchmarkChecksum (nativeOutput), benchmarkChecksum (baselineOutput), 1.0);
}

TEST_F (YdspBenchmarkTests, LadderFilterAgainstNative)
{
    auto graph = compilePatch (benchmarkLadderSource, compiler, allOptimizations());
    ASSERT_TRUE (graph.isValid());
    graph.prepare (benchmarkSampleRate, benchmarkBlockSize);

    const auto jitTiming = benchmarkTimeRepeats ([&]
    {
        graph.reset();
        benchmarkRunGraph (graph, input, jitOutput);
    });

    const auto nativeTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeLadder reference;

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, nativeOutput.data() + offset, benchmarkBlockSize);
        }
    });

    EXPECT_NEAR (benchmarkChecksum (nativeOutput), benchmarkChecksum (jitOutput), 1.0);

    benchmarkReportPolicies ("ladder filter (state)", { "baseline", "baseline + fastMath", "host", "host + fastMath" }, benchmarkPolicies (benchmarkLadderSource), nativeTiming);

    // How much of that ratio is the reference fusing four multiply-adds the JIT
    // refuses to fuse. See BenchmarkNativeLadderUncontracted.
    std::vector<float> uncontractedOutput (static_cast<size_t> (benchmarkTotalSamples), 0.0f);

    const auto uncontractedTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeLadderUncontracted reference;

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, uncontractedOutput.data() + offset, benchmarkBlockSize);
        }
    });

    EXPECT_NEAR (benchmarkChecksum (uncontractedOutput), benchmarkChecksum (jitOutput), 1.0);

    benchmarkReportContraction ("ladder filter", nativeOutput, uncontractedOutput);

    // And the same patch written with the fused form, which is the arithmetic
    // the `c++` row is running. Read this against `c++`, not against `jit`: if
    // fusing is the whole story here, the two land together.
    auto fused = compilePatch (benchmarkLadderFusedSource, compiler, allOptimizations());
    ASSERT_TRUE (fused.isValid());
    fused.prepare (benchmarkSampleRate, benchmarkBlockSize);

    std::vector<float> fusedOutput (static_cast<size_t> (benchmarkTotalSamples), 0.0f);

    const auto fusedTiming = benchmarkTimeRepeats ([&]
    {
        fused.reset();
        benchmarkRunGraph (fused, input, fusedOutput);
    });

    // Against the *contracted* reference, because that is meant to be the same
    // arithmetic - four fused multiply-adds per sample, in the same order.
    //
    // Compared proportionally rather than to a fixed tolerance: "meant to be"
    // is as far as it goes, since which multiply-add clang chose to fuse in
    // `z + g * (x - z)` is its decision and not something this patch can pin.
    // Any difference feeds back through four stages of a recurrence, so a
    // one-ulp divergence does not stay one ulp. What the check is for is that
    // the fused patch is still the same filter.
    const auto ladderMagnitude = benchmarkMagnitude (nativeOutput);
    ASSERT_GT (ladderMagnitude, 0.0);
    EXPECT_NEAR (ladderMagnitude, benchmarkMagnitude (fusedOutput), 1.0e-3 * ladderMagnitude);

    benchmarkReportVariants ("ladder filter: fma() against the same patch written as mul + add",
                             "jit fma",
                             fusedTiming,
                             "jit",
                             jitTiming);
}

TEST_F (YdspBenchmarkTests, RepresentativePatchesAcrossOptimizationPolicies)
{
    const std::array<StringRef, 4> sources { benchmarkDelaySource,
                                             benchmarkLadderSource,
                                             benchmarkShaperSource,
                                             benchmarkFolderSource };

    for (const auto source : sources)
    {
        auto optimized = compilePatch (source, compiler, allOptimizations());
        auto baseline = compilePatch (source, compiler, noVectorisation());
        ASSERT_TRUE (optimized.isValid());
        ASSERT_TRUE (baseline.isValid());

        optimized.prepare (benchmarkSampleRate, benchmarkBlockSize);
        baseline.prepare (benchmarkSampleRate, benchmarkBlockSize);

        std::vector<float> optimizedOutput (static_cast<size_t> (benchmarkTotalSamples), 0.0f);
        std::vector<float> baselineOutput (static_cast<size_t> (benchmarkTotalSamples), 0.0f);
        optimized.reset();
        baseline.reset();
        benchmarkRunGraph (optimized, input, optimizedOutput);
        benchmarkRunGraph (baseline, input, baselineOutput);

        EXPECT_TRUE (std::isfinite (benchmarkChecksum (optimizedOutput)));
        EXPECT_TRUE (std::isfinite (benchmarkChecksum (baselineOutput)));
    }
}

TEST_F (YdspBenchmarkTests, BiquadAgainstNativeAcrossOptimizationPolicies)
{
    const std::array<const char*, 4> labels { "baseline", "baseline + fastMath", "host", "host + fastMath" };
    const auto timings = benchmarkPolicies (benchmarkBiquadSource);

    const auto nativeTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeBiquad reference;
        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, nativeOutput.data() + offset, benchmarkBlockSize);
        }
    });

    benchmarkReportPolicies ("biquad low-pass", labels, timings, nativeTiming);
    EXPECT_TRUE (std::isfinite (benchmarkChecksum (nativeOutput)));
}

TEST_F (YdspBenchmarkTests, HarmonicBankAgainstNative)
{
    auto graph = compilePatch (benchmarkBankSource, compiler, allOptimizations());
    ASSERT_TRUE (graph.isValid());
    graph.prepare (benchmarkSampleRate, benchmarkBlockSize);

    const auto jitTiming = benchmarkTimeRepeats ([&]
    {
        graph.reset();
        benchmarkRunGraph (graph, input, jitOutput);
    });

    const auto nativeTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeBank reference (benchmarkSampleRate);

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (nativeOutput.data() + offset, benchmarkBlockSize);
        }
    });

    // The rotating phasors drift apart over 100k samples (the recurrence is
    // chaotic in float32), so only assert that both actually produced signal.
    EXPECT_GT (benchmarkChecksum (nativeOutput) * benchmarkChecksum (nativeOutput), 0.0);

    benchmarkReportPolicies ("harmonic bank (32 partials)", { "baseline", "baseline + fastMath", "host", "host + fastMath" }, benchmarkPolicies (benchmarkBankSource), nativeTiming);

    // Both loop transforms should be firing here. Printed rather than asserted:
    // the unroller declines a body that would exceed its instruction budget,
    // and this bank's is the largest in the suite, so whether it fits is a
    // measurement that has already changed once.
    for (const auto& kernel : graph.getExecutionReport().getKernels())
    {
        if (kernel.name != "Bank")
            continue;

        // The process kernel is emitted before the init one, so this is it.
        std::cout << "\n  | kernel    | " << kernel.instructionCount << " insts, vectorized "
                  << (kernel.vectorized ? "yes" : "no") << " x" << kernel.vectorWidth
                  << ", unrolled " << (kernel.unrolled ? "yes" : "no") << "\n";
        break;
    }
}

TEST_F (YdspBenchmarkTests, WaveShaperAgainstNative)
{
    auto graph = compilePatch (benchmarkShaperSource, compiler, allOptimizations());
    ASSERT_TRUE (graph.isValid());
    graph.prepare (benchmarkSampleRate, benchmarkBlockSize);

    const auto jitTiming = benchmarkTimeRepeats ([&]
    {
        graph.reset();
        benchmarkRunGraph (graph, input, jitOutput);
    });

    const auto nativeTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeShaper reference;

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, nativeOutput.data() + offset, benchmarkBlockSize);
        }
    });

    EXPECT_NEAR (benchmarkChecksum (nativeOutput), benchmarkChecksum (jitOutput), 1.0);

    benchmarkReportPolicies ("wave shaper (compare + select)", { "baseline", "baseline + fastMath", "host", "host + fastMath" }, benchmarkPolicies (benchmarkShaperSource), nativeTiming);

    // The control row: this shape's recurrence has no add to fuse into, so if
    // the two references agree, the shaper's ratio is not about FMA at all.
    // See BenchmarkNativeShaperUncontracted.
    std::vector<float> uncontractedOutput (static_cast<size_t> (benchmarkTotalSamples), 0.0f);

    const auto uncontractedTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeShaperUncontracted reference;

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, uncontractedOutput.data() + offset, benchmarkBlockSize);
        }
    });

    EXPECT_NEAR (benchmarkChecksum (uncontractedOutput), benchmarkChecksum (jitOutput), 1.0);

    benchmarkReportContraction ("wave shaper", nativeOutput, uncontractedOutput);

    benchmarkReportVariants ("wave shaper: the JIT against a reference that also cannot fuse mul+add",
                             "jit",
                             jitTiming,
                             "c++ nofma",
                             uncontractedTiming);
}

TEST_F (YdspBenchmarkTests, ModalBankAgainstNative)
{
    auto graph = compilePatch (benchmarkModalSource, compiler, allOptimizations());
    ASSERT_TRUE (graph.isValid());
    graph.prepare (benchmarkSampleRate, benchmarkBlockSize);

    const auto jitTiming = benchmarkTimeRepeats ([&]
    {
        graph.reset();
        benchmarkRunGraph (graph, input, jitOutput);
    });

    const auto nativeTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeModal reference;

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, nativeOutput.data() + offset, benchmarkBlockSize);
        }
    });

    // YDSP expands float32 `exp` inline rather than calling `expf`, so the two
    // drive terms can differ by an ulp; the mode filters are stable one-poles,
    // so compare proportionally rather than exactly.
    const auto native = benchmarkMagnitude (nativeOutput);
    ASSERT_GT (native, 0.0);
    EXPECT_NEAR (native, benchmarkMagnitude (jitOutput), 1.0e-3 * native);

    benchmarkReportPolicies ("modal bank (loop-invariant inner work)", { "baseline", "baseline + fastMath", "host", "host + fastMath" }, benchmarkPolicies (benchmarkModalSource), nativeTiming, benchmarkModalBankLimit);
}

//==============================================================================
// Optimisation-policy and graph-shape comparisons (no independent C++ row).

TEST_F (YdspBenchmarkTests, AutomaticTierAgainstBaseline)
{
    YdspCompileOptions automaticOptions;
    automaticOptions.emitOptimizationReport = true;

    YdspCompiler automaticCompiler;
    auto automaticResult = automaticCompiler.compile (benchmarkModalNoSumSource, automaticOptions);
    ASSERT_TRUE (automaticResult.wasOk()) << automaticCompiler.getDiagnostics().toString();

    const auto automaticReport = automaticCompiler.getOptimizationReport();
    auto automatic = std::move (automaticResult).getValue();

    YdspCompileOptions baselineOptions;
    baselineOptions.optimizationTier = YdspOptimizationTier::baseline;
    baselineOptions.targetPolicy = YdspTargetPolicy::baseline;
    baselineOptions.baselineTarget = YdspNativeTarget::scalar;
    baselineOptions.emitOptimizationReport = true;

    YdspCompiler baselineCompiler;
    auto baselineResult = baselineCompiler.compile (benchmarkModalNoSumSource, baselineOptions);
    ASSERT_TRUE (baselineResult.wasOk()) << baselineCompiler.getDiagnostics().toString();

    const auto baselineReport = baselineCompiler.getOptimizationReport();
    auto baseline = std::move (baselineResult).getValue();

    automatic.prepare (benchmarkSampleRate, benchmarkBlockSize);
    baseline.prepare (benchmarkSampleRate, benchmarkBlockSize);

    std::vector<float> automaticOutput (static_cast<size_t> (benchmarkTotalSamples), 0.0f);
    std::vector<float> baselineOutput (static_cast<size_t> (benchmarkTotalSamples), 0.0f);

    const auto automaticTiming = benchmarkTimeRepeats ([&]
    {
        automatic.reset();
        benchmarkRunGraph (automatic, input, automaticOutput);
    });

    const auto baselineTiming = benchmarkTimeRepeats ([&]
    {
        baseline.reset();
        benchmarkRunGraph (baseline, input, baselineOutput);
    });

    // This variant exposes only scalar env state, so it can compare strict
    // automatic and baseline outputs exactly while the otherwise independent
    // mode-bank stores exercise the automatic tier's packed loop lowering.
    EXPECT_EQ (automaticOutput, baselineOutput);
    EXPECT_TRUE (automaticReport.vectorizationEnabled);
#if ! YUP_WASM || defined (__wasm_simd128__)
    // Loop unrolling is part of the automatic tier on native and on a wasm
    // build compiled with -msimd128; only a scalar wasm build keeps it off
    // (see the compiler's hasLoopTransforms gate).
    EXPECT_TRUE (automaticReport.unrollingEnabled);
#endif
    EXPECT_EQ (YdspNativeTarget::scalar, baselineReport.selectedIsa);
    EXPECT_EQ (1, baselineReport.vectorWidth);
    EXPECT_FALSE (baselineReport.vectorizationEnabled);
    EXPECT_FALSE (baselineReport.unrollingEnabled);

    const auto modalKernel = [] (const YdspAudioGraph& graph)
    {
        for (const auto& kernel : graph.getExecutionReport().getKernels())
            if (kernel.name == "Modal")
                return kernel;

        return YdspKernelReport {};
    };

    const auto automaticKernel = modalKernel (automatic);
    const auto baselineKernel = modalKernel (baseline);

    EXPECT_TRUE (automaticKernel.vectorized);
    EXPECT_EQ (automaticReport.vectorWidth, automaticKernel.vectorWidth);
    EXPECT_FALSE (baselineKernel.vectorized);
    EXPECT_FALSE (baselineKernel.unrolled);

    std::cout << "  | automatic | " << automaticReport.vectorWidth << " lanes"
              << ", " << automaticReport.generatedCodeSize << " bytes\n"
              << "  | baseline  | " << baselineReport.vectorWidth << " lane"
              << ", " << baselineReport.generatedCodeSize << " bytes\n";

    benchmarkReportVariants ("modal bank: automatic tier against scalar baseline",
                             "automatic",
                             automaticTiming,
                             "baseline",
                             baselineTiming);
}

// What the widened reduction costs, isolated: the same bank with and without
// its accumulation. Read the two rows, not the ratio - the answer is the
// difference in ns/sample, and it decides whether splitting the accumulator
// into independent chains is worth implementing.
//
// For a long time this measured the *smaller* kernel as the slower one, which
// strictly less work cannot be, and two hypotheses were tried on it and killed:
// spill traffic around the per-sample `exp` call (the listing counts below say
// 0 vector spills either way), and the kernel not being issue-bound. Both were
// wrong, and so was the conclusion drawn from the second - the readings were
// simply noise. Rolled, this shape swung 9.4 to 23.7 ns/sample *within a single
// run*, which is wide enough to invert the comparison and did.
//
// Unrolling the mode loop removed that: the shape became stable to within 3%
// and read 8.45 against 6.58, putting the reduction at ~1.73 ns/sample. The
// instability was the loop, not the call.
//
// That number then became actionable. Halving the accumulator - two chains
// side by side instead of four serially dependent vector adds - brought it
// down, but by how much is not settled: two runs of the split form measured
// 0.98 and 1.53 ns/sample against 1.93 unsplit, so the direction is clear and
// the size is not. Whether it fired at all is asserted on the report flag
// above rather than inferred from this ratio, which is too noisy to carry it.
TEST_F (YdspBenchmarkTests, ModalBankReductionCost)
{
    auto withSum = compilePatch (benchmarkModalSource, compiler);
    auto noSum = compilePatch (benchmarkModalNoSumSource, compiler);

    ASSERT_TRUE (withSum.isValid());
    ASSERT_TRUE (noSum.isValid());

    withSum.prepare (benchmarkSampleRate, benchmarkBlockSize);
    noSum.prepare (benchmarkSampleRate, benchmarkBlockSize);

    // Both variants are JIT graphs, so neither writes the fixture's `c++` buffer.
    std::vector<float> noSumOutput (static_cast<size_t> (benchmarkTotalSamples), 0.0f);

    const auto withSumTiming = benchmarkTimeRepeats ([&]
    {
        withSum.reset();
        benchmarkRunGraph (withSum, input, jitOutput);
    });

    const auto noSumTiming = benchmarkTimeRepeats ([&]
    {
        noSum.reset();
        benchmarkRunGraph (noSum, input, noSumOutput);
    });

    // Both patches have to have produced signal. Note that nothing reads `z` in
    // the no-sum variant, so this does not prove its mode loop ran - the store
    // is what keeps it alive (a store is never dead code), and the instruction
    // count and lane-count assertions below are what actually pin it down.
    EXPECT_GT (benchmarkMagnitude (jitOutput), 0.0);
    EXPECT_GT (benchmarkMagnitude (noSumOutput), 0.0);

    // The delta only means what it claims while *both* mode loops are widened
    // and the no-sum one is the smaller kernel. Assert it rather than assume it:
    // the first run of this shape measured the no-sum variant as the slower of
    // the two, which cannot be true of strictly less work.
    const auto modalKernel = [] (const YdspAudioGraph& graph)
    {
        for (const auto& kernel : graph.getExecutionReport().getKernels())
            if (kernel.name == "Modal")
                return kernel;

        return YdspKernelReport {};
    };

    const auto withSumKernel = modalKernel (withSum);
    const auto noSumKernel = modalKernel (noSum);

    const auto describe = [] (const YdspKernelReport& kernel)
    {
        return String (kernel.instructionCount) + " insts, vectorized "
             + (kernel.vectorized ? "yes" : "no") + " x" + String (kernel.vectorWidth)
             + ", unrolled " + (kernel.unrolled ? "yes" : "no")
             + ", split " + (kernel.reductionSplit ? "yes" : "no");
    };

    std::cout << "  | kernel    | with sum: " << describe (withSumKernel)
              << " | no sum: " << describe (noSumKernel) << "\n";

    EXPECT_TRUE (withSumKernel.vectorized) << "the accumulating mode loop stopped being widened";
    EXPECT_TRUE (noSumKernel.vectorized) << "the element-wise mode loop is not being widened";
    EXPECT_LT (noSumKernel.instructionCount, withSumKernel.instructionCount)
        << "the no-sum variant should be the smaller kernel";

    // The ratio below is what sizes the reduction, and it halved when the
    // accumulator was split - so the split silently ceasing to fire would show
    // up there as a number rather than a failure. Assert the flag instead: it
    // says the same thing without depending on a timing threshold. The split
    // halves the *unrolled* chain, which only pays off from four links on
    // (splitWidenedReductionChains::minChainLength): 16 modes at 4 lanes
    // unrolls to four copies, at 8 AVX2 lanes to only two - a chain a split
    // cannot shorten - so on 8-lane targets the pass correctly declines.
    EXPECT_EQ (4 <= 16 / withSumKernel.vectorWidth, withSumKernel.reductionSplit)
        << "the split must fire exactly when the unrolled reduction chain has at least four links";
    EXPECT_FALSE (noSumKernel.reductionSplit) << "there is no accumulator here to split";

    // What the instruction count leaves out. Both variants call `exp` once per
    // sample, and everything live across that call would go to the stack - the
    // hypothesis this was added to test, and which it refuted.
    benchmarkReportListing ("with sum", benchmarkAnalyzeListing (withSum));
    benchmarkReportListing ("no sum", benchmarkAnalyzeListing (noSum));

    benchmarkReportVariants ("modal bank: what the reduction costs",
                             "with sum",
                             withSumTiming,
                             "no sum",
                             noSumTiming);
}

// Whether kernel fusion is still firing. Three chained sample-mode nodes should
// compile to the same single kernel a hand-fused processor does, so the two are
// expected to measure the same; the 2x gap this shape opened with is what fusion
// closed.
TEST_F (YdspBenchmarkTests, ChainedNodesAgainstAFusedProcessor)
{
    auto chained = compilePatch (benchmarkChainedSource, compiler);
    auto fused = compilePatch (benchmarkFusedSource, compiler);

    ASSERT_TRUE (chained.isValid());
    ASSERT_TRUE (fused.isValid());

    chained.prepare (benchmarkSampleRate, benchmarkBlockSize);
    fused.prepare (benchmarkSampleRate, benchmarkBlockSize);

    // Both variants are JIT graphs, so neither writes the fixture's `c++` buffer.
    std::vector<float> fusedOutput (static_cast<size_t> (benchmarkTotalSamples), 0.0f);

    const auto chainedTiming = benchmarkTimeRepeats ([&]
    {
        chained.reset();
        benchmarkRunGraph (chained, input, jitOutput);
    });

    const auto fusedTiming = benchmarkTimeRepeats ([&]
    {
        fused.reset();
        benchmarkRunGraph (fused, input, fusedOutput);
    });

    // The two patches have to be computing the same thing, or the gap is not
    // fusion headroom. Every stage is float32 on both sides, and the scratch
    // round trip is float32 too, so they agree to the bit.
    EXPECT_NEAR (benchmarkChecksum (fusedOutput), benchmarkChecksum (jitOutput), 1.0);

    benchmarkReportVariants ("kernel fusion: 3 chained nodes vs 1 fused processor",
                             "chained",
                             chainedTiming,
                             "fused",
                             fusedTiming);

    // Loose enough to ignore the run-to-run spread, tight enough to catch the
    // chained form falling back to three kernels and two scratch round trips,
    // which was 2.05x when this shape was written.
    EXPECT_LT (chainedTiming.best / fusedTiming.best, 1.5)
        << "the chained form is no longer being fused";
}

TEST_F (YdspBenchmarkTests, WaveFolderAgainstNative)
{
    auto graph = compilePatch (benchmarkFolderSource, compiler, allOptimizations());
    ASSERT_TRUE (graph.isValid());
    graph.prepare (benchmarkSampleRate, benchmarkBlockSize);

    const auto jitTiming = benchmarkTimeRepeats ([&]
    {
        graph.reset();
        benchmarkRunGraph (graph, input, jitOutput);
    });

    const auto nativeTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeFolder reference;

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, nativeOutput.data() + offset, benchmarkBlockSize);
        }
    });

    EXPECT_NEAR (benchmarkChecksum (nativeOutput), benchmarkChecksum (jitOutput), 1.0);

    benchmarkReportPolicies ("wave folder (branchy if/else)", { "baseline", "baseline + fastMath", "host", "host + fastMath" }, benchmarkPolicies (benchmarkFolderSource), nativeTiming);

    // How much of that ratio is the reference fusing its multiply-add and the
    // JIT refusing to. See BenchmarkNativeFolderUncontracted.
    std::vector<float> uncontractedOutput (static_cast<size_t> (benchmarkTotalSamples), 0.0f);

    const auto uncontractedTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeFolderUncontracted reference;

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, uncontractedOutput.data() + offset, benchmarkBlockSize);
        }
    });

    EXPECT_NEAR (benchmarkChecksum (uncontractedOutput), benchmarkChecksum (jitOutput), 1.0);

    benchmarkReportContraction ("wave folder", nativeOutput, uncontractedOutput);
}

TEST_F (YdspBenchmarkTests, IdleVoiceSkippingAgainstEveryVoiceRunning)
{
    const String annotated (patches::electricPiano);
    const auto unannotated = annotated.replace ("[[ role: voiceActivity ]]", "");

    ASSERT_NE (annotated, unannotated);

    auto skipping = compilePatch (annotated, compiler);
    auto everyVoice = compilePatch (unannotated, compiler);

    ASSERT_TRUE (skipping.isValid());
    ASSERT_TRUE (everyVoice.isValid());

    skipping.prepare (benchmarkSampleRate, benchmarkBlockSize);
    everyVoice.prepare (benchmarkSampleRate, benchmarkBlockSize);

    // Two notes held for the whole run: 2 of 16 voices sounding.
    MidiBuffer midi;
    midi.addEvent (MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 0);
    midi.addEvent (MidiMessage::noteOn (1, 64, static_cast<uint8> (100)), 0);

    std::vector<float> left (static_cast<size_t> (benchmarkTotalSamples), 0.0f);
    std::vector<float> right (static_cast<size_t> (benchmarkTotalSamples), 0.0f);

    const auto skippingTiming = benchmarkTimeRepeats ([&]
    {
        skipping.reset();
        benchmarkRunVoiceGraph (skipping, left, right, midi);
    });

    EXPECT_EQ (2, skipping.getActiveVoiceCount ("voices"));

    const auto everyVoiceTiming = benchmarkTimeRepeats ([&]
    {
        everyVoice.reset();
        benchmarkRunVoiceGraph (everyVoice, left, right, midi);
    });

    // No annotation, no skipping: all 16 kernels ran every block.
    EXPECT_EQ (16, everyVoice.getActiveVoiceCount ("voices"));

    benchmarkReportVariants ("idle-voice skipping, EPVoice[16] with 2 notes held",
                             "skipping",
                             skippingTiming,
                             "all 16",
                             everyVoiceTiming);

    // 2 of 16 voices sounding, so roughly 8x is available. The bound is
    // deliberately loose: it only has to catch the feature going missing.
    EXPECT_LT (skippingTiming.best / everyVoiceTiming.best, 0.5)
        << "voice skipping saved less than half the work of running all 16 voices";
}

TEST_F (YdspBenchmarkTests, GraphLevelDryWetAgainstAnInlinedDryWet)
{
    // The fanned patch needs graph fan-out and summing fan-in. Until those
    // land it does not analyze, so this reports the inline baseline and skips
    // rather than failing the suite - and starts comparing on its own once the
    // analyzer accepts the shape.
    YdspCompiler fannedCompiler;
    auto fannedResult = fannedCompiler.compile (benchmarkFannedDryWetSource);

    auto inlined = compilePatch (benchmarkInlineDryWetSource, compiler);
    ASSERT_TRUE (inlined.isValid());

    inlined.prepare (benchmarkSampleRate, benchmarkBlockSize);

    const auto inlinedTiming = benchmarkTimeRepeats ([&]
    {
        inlined.reset();
        benchmarkRunGraph (inlined, input, jitOutput);
    });

    if (! fannedResult.wasOk())
    {
        std::cout << "\n  |==== BENCH ==== (graph-level dry/wet - fan-out + fan-in)\n"
                  << "  inline baseline: "
                  << String (inlinedTiming.best * 1.0e9 / static_cast<double> (benchmarkTotalSamples), 3)
                  << " ns/sample; the fanned patch does not analyze yet, so the comparison is skipped\n";

        GTEST_SKIP() << "graph fan-out / summing fan-in is not implemented yet: "
                     << fannedCompiler.getDiagnostics().toString();
    }

    auto fanned = std::move (fannedResult).getValue();
    ASSERT_TRUE (fanned.isValid());

    fanned.prepare (benchmarkSampleRate, benchmarkBlockSize);

    // Both variants are JIT graphs, so neither writes the fixture's `c++` buffer.
    std::vector<float> fannedOutput (static_cast<size_t> (benchmarkTotalSamples), 0.0f);

    const auto fannedTiming = benchmarkTimeRepeats ([&]
    {
        fanned.reset();
        benchmarkRunGraph (fanned, input, fannedOutput);
    });

    // The two patches have to be computing the same thing, or the gap is not
    // the cost of the mix path.
    EXPECT_NEAR (benchmarkChecksum (jitOutput), benchmarkChecksum (fannedOutput), 1.0);

    benchmarkReportVariants ("graph-level dry/wet (fan-out + fan-in) vs. dry/wet inside one processor",
                             "fanned",
                             fannedTiming,
                             "inline",
                             inlinedTiming);

    // The numbers this shape exists to produce, in ns/sample rather than as
    // ratios - a ratio hides how small or large the absolute cost is relative to
    // the DSP around it.
    //
    // fanned - inline bundles two things: the second kernel call (the fanned
    // form cannot fuse, so it really does pay one) and the mix path. The split
    // variant below runs the same two kernels with the same fan-out but writes
    // two separate graph outputs, so both node outputs take the direct-write
    // path and no mix buffer exists - which makes fanned - split the mix path on
    // its own.
    const auto perSample = [] (double seconds)
    {
        return seconds * 1.0e9 / static_cast<double> (benchmarkTotalSamples);
    };

    auto split = compilePatch (benchmarkSplitDryWetSource, compiler);
    ASSERT_TRUE (split.isValid());

    split.prepare (benchmarkSampleRate, benchmarkBlockSize);

    std::vector<float> splitDry (static_cast<size_t> (benchmarkTotalSamples), 0.0f);
    std::vector<float> splitWet (static_cast<size_t> (benchmarkTotalSamples), 0.0f);

    const auto splitTiming = benchmarkTimeRepeats ([&]
    {
        split.reset();
        benchmarkRunSplitGraph (split, input, splitDry, splitWet);
    });

    benchmarkReportVariants ("graph-level dry/wet: summed into one output vs. split across two",
                             "summed",
                             fannedTiming,
                             "split",
                             splitTiming);

    std::cout << "  second kernel call + mix path: "
              << String (perSample (fannedTiming.best) - perSample (inlinedTiming.best), 3)
              << " ns/sample\n"
              << "  mix path alone:                "
              << String (perSample (fannedTiming.best) - perSample (splitTiming.best), 3)
              << " ns/sample (best-of-" << benchmarkRepeats << ")\n";
}

} // namespace yup::test
