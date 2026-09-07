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
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
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

// The transcendental shapes below measure a scalar-libm baseline that the SLEEF
// migration (Phase 2) is expected to pull far down, so their guard is looser
// than the modal bank's until that lands. The printed ratio is the record.
constexpr double benchmarkTranscendentalLimit = 25.0;

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
    int calls = 0;         // call/blr sites of any kind
    int mathCalls = 0;     // register-indirect call/blr sites
    int otherCalls = 0;    // direct call sites (immediate/label target)
};

/** Counts the above over a compiled graph's AsmJit listing.

    Only differences between two kernels mean anything here: the prologue's own
    stack traffic is counted too, and the line count includes labels the
    assembler never emits.

    The `mathCalls` bucket is what makes the transcendental work observable: a
    libm transcendental is lowered as a call through a register whose address
    was just materialised (`mov <gp>, Imm` + `call <gp>`, or `blr` on AArch64),
    so the register-indirect sites are exactly the per-sample math calls in the
    shapes below. A register-indirect call to a non-math helper (the `@`
    modulo helper on x86-64, event emission) lands here too, which is why the
    shapes that want a clean math count keep their loops free of those.
*/
BenchmarkListingStats benchmarkAnalyzeListing (const YdspAudioGraph& graph)
{
    const auto isGpRegister = [] (const String& token)
    {
        static constexpr const char* registers[] = { "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
                                                     "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15" };

        for (const auto* reg : registers)
            if (token == String (reg))
                return true;

        return false;
    };

    BenchmarkListingStats stats;

    for (const auto& line : StringArray::fromLines (graph.getDiagnostics().toString()))
    {
        const auto text = line.trim();

        if (text.isEmpty())
            continue;

        ++stats.lines;

        if (text.contains ("[sp") || text.contains ("[rsp") || text.contains ("[esp"))
        {
            ++stats.stackAccesses;

            if (text.contains (" q") || text.contains ("xmm"))
                ++stats.vectorSpills;
        }

        const auto isBlr = text.contains ("blr");
        const auto isCall = text.contains ("call");

        if (isBlr || isCall)
        {
            ++stats.calls;

            // Operand token following the mnemonic (machine-code bytes can
            // precede the instruction on the same line, so skip them).
            const auto mnemonicLength = isBlr ? 3 : 4;
            const auto mnemonicIndex = isBlr ? text.indexOf ("blr") : text.indexOf ("call");
            auto target = text.substring (mnemonicIndex + mnemonicLength).trim().upToFirstOccurrenceOf (" ", false, false);

            if (isBlr || isGpRegister (target.trim()))
                ++stats.mathCalls;
            else
                ++stats.otherCalls;
        }
    }

    return stats;
}

/** Prints one listing's counts as a table row. */
void benchmarkReportListing (const String& label, const BenchmarkListingStats& stats)
{
    std::cout << "  " << label.paddedRight (' ', 9) << ": "
              << stats.lines << " lines, "
              << stats.stackAccesses << " stack, "
              << stats.vectorSpills << " vector spills, "
              << stats.mathCalls << " math calls, "
              << stats.otherCalls << " other calls\n";
}

/** Dumps a compiled kernel's full listing plus an opcode histogram.

    Gated on the YUP_DUMP_KERNEL environment variable so it costs nothing in
    normal runs: set it to a comma-separated list of labels (or `*`) to print
    the AsmJit listing of those kernels to stdout, one mnemonic-frequency
    table per kernel. This is the diagnostic for deciding where a shape still
    loses to the native C++ reference - spills around a call, address
    arithmetic, register moves - instead of guessing from the summary counts.
*/
void benchmarkDumpListingIfRequested (const String& label, const YdspAudioGraph& graph)
{
    const auto* filter = std::getenv ("YUP_DUMP_KERNEL");

    if (filter == nullptr || String (filter).isEmpty())
        return;

    if (String (filter) != "*" && ! StringArray::fromTokens (filter, ",", {}).contains (label))
        return;

    std::cout << "\n--- kernel listing: " << label << " ---\n";

    std::map<String, int> histogram;

    for (const auto& line : StringArray::fromLines (graph.getDiagnostics().toString()))
    {
        const auto text = line.trim();

        if (text.isEmpty())
            continue;

        std::cout << text << "\n";

        // The mnemonic is the first non-address, non-byte token: AsmJit lines
        // carry a hex address and space-separated machine-code bytes before the
        // instruction, and operands can look like hex too, so pick the first
        // token that is neither a label, a byte value, nor a hex address.
        String mnemonic;

        for (const auto& token : StringArray::fromTokens (text, " \t", {}))
        {
            const auto candidate = token.trim();

            if (candidate.isEmpty() || candidate.endsWith (":") || candidate.startsWith ("0x"))
                continue;

            if (candidate.length() <= 8)
            {
                bool isHex = ! candidate.isEmpty();

                for (int i = 0; i < candidate.length(); ++i)
                {
                    const auto c = candidate[i];

                    if (! ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
                    {
                        isHex = false;
                        break;
                    }
                }

                if (isHex)
                    continue;
            }

            mnemonic = candidate;
            break;
        }

        if (! mnemonic.isEmpty())
            ++histogram[mnemonic];
    }

    std::cout << "  histogram:\n";

    for (const auto& [mnemonic, count] : histogram)
        std::cout << "    " << mnemonic.paddedRight (' ', 12) << count << "\n";
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
// Shape 3b: an oscillator bank whose phase advance is a per-sample `sin()`
// inside the loop - the transcendental the vectorizer refuses to widen, so the
// whole loop stays scalar today. The phase is left unwrapped on purpose: the
// reference and the JIT feed the same float phase to the same libm `sinf`, so
// an exact mirror needs no wrap logic.

constexpr auto benchmarkSineBankSource = R"YDSP(
    let sines = 8;

    processor SineBank {
        output stream out;

        state float phase[sines];
        state float freq[sines];
        state float amp[sines];

        init {
            for i in 0..sines {
                let w = 2.0 * pi * 220.0 * float (i + 1) * samplePeriod;

                phase[i] = 0.0;
                freq[i] = w;
                amp[i] = 0.5 / float (i + 1);
            }
        }

        process {
            float sum = 0.0;

            for i in 0..sines {
                phase[i] = phase[i] + freq[i];
                sum = sum + sin (phase[i]) * amp[i];
            }

            out = sum;
        }
    }

    graph G {
        output stream y;

        node b = SineBank;

        connection { b.out -> y; }
    }
)YDSP";

class BenchmarkNativeSineBank
{
public:
    explicit BenchmarkNativeSineBank (double sampleRate)
    {
        const auto samplePeriod = static_cast<float> (1.0 / sampleRate);

        for (int i = 0; i < sines; ++i)
        {
            const auto w = 2.0f * 3.14159265358979323846f * 220.0f * static_cast<float> (i + 1) * samplePeriod;

            phase[i] = 0.0f;
            freq[i] = w;
            amp[i] = 0.5f / static_cast<float> (i + 1);
        }
    }

    void process (float* out, int numSamples)
    {
        for (int s = 0; s < numSamples; ++s)
        {
            auto sum = 0.0f;

            for (int i = 0; i < sines; ++i)
            {
                phase[i] += freq[i];
                sum += std::sin (phase[i]) * amp[i];
            }

            out[s] = sum;
        }
    }

private:
    static constexpr int sines = 8;

    float phase[sines] {}, freq[sines] {}, amp[sines] {};
};

//==============================================================================
// Shape 3c: a scalar per-sample `tanh()` waveshaper with a stateful drive
// envelope. Measures the per-sample scalar transcendental path (libm) under
// fastMath contraction.

constexpr auto benchmarkTanhShaperSource = R"YDSP(
    processor TanhShaper {
        input stream in;
        output stream out;

        state float env;

        process {
            env = env * 0.999 + abs (in) * 0.001;

            out = tanh (in * (1.0 + env * 4.0));
        }
    }

    graph G {
        input stream x;
        output stream y;

        node t = TanhShaper;

        connection { x -> t.in; t.out -> y; }
    }
)YDSP";

class BenchmarkNativeTanhShaper
{
public:
    void process (const float* in, float* out, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto x = in[i];

            env = env * 0.999f + std::fabs (x) * 0.001f;

            out[i] = std::tanh (x * (1.0f + env * 4.0f));
        }
    }

private:
    float env = 0.0f;
};

//==============================================================================
// Shape 3d: a scalar one-pole envelope whose coefficient is an `exp()` of the
// (per-sample) input, so the call cannot be hoisted. This is the `exp` behind
// `smooth` named in the ARM64 x30/LR comment.

constexpr auto benchmarkExpEnvelopeSource = R"YDSP(
    processor ExpEnvelope {
        input stream in;
        output stream out;

        state float env;

        process {
            env = env * exp (-0.001 * (1.0 + abs (in))) + in * 0.001;

            out = env;
        }
    }

    graph G {
        input stream x;
        output stream y;

        node e = ExpEnvelope;

        connection { x -> e.in; e.out -> y; }
    }
)YDSP";

class BenchmarkNativeExpEnvelope
{
public:
    void process (const float* in, float* out, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto x = in[i];

            env = env * std::exp (-0.001f * (1.0f + std::fabs (x))) + x * 0.001f;

            out[i] = env;
        }
    }

private:
    float env = 0.0f;
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
// inner loop, and whose 16 modes are independent.

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
// Idle-voice skipping.

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
        options.fastMath = false; // keep the scalar reference row strict
        return options;
    }

    std::array<BenchmarkTiming, 4> benchmarkPolicies (StringRef source)
    {
        const auto baselineOptions = noVectorisation();
        auto baselineFastOptions = baselineOptions;
        baselineFastOptions.fastMath = true;

        // fastMath is now the native default, so the strict host row has to
        // opt out to stay a distinct policy; the default is row 4.
        auto hostStrictOptions = YdspCompileOptions {};
        hostStrictOptions.fastMath = false;

        const auto hostFastOptions = YdspCompileOptions {};

        const std::array<YdspCompileOptions, 4> options { baselineOptions, baselineFastOptions, hostStrictOptions, hostFastOptions };

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

    benchmarkReportPolicies ("delay taps (@)", { "baseline (strict)", "baseline + fastMath", "host (strict)", "host + fastMath (default)" }, benchmarkPolicies (benchmarkDelaySource), nativeTiming);
    benchmarkDumpListingIfRequested ("delay", graph);
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
    benchmarkDumpListingIfRequested ("ladder", graph);
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

    benchmarkReportPolicies ("ladder filter (state)", { "baseline (strict)", "baseline + fastMath", "host (strict)", "host + fastMath (default)" }, benchmarkPolicies (benchmarkLadderSource), nativeTiming);

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

    auto fused = compilePatch (benchmarkLadderFusedSource, compiler, allOptimizations());
    ASSERT_TRUE (fused.isValid());
    fused.prepare (benchmarkSampleRate, benchmarkBlockSize);

    std::vector<float> fusedOutput (static_cast<size_t> (benchmarkTotalSamples), 0.0f);

    const auto fusedTiming = benchmarkTimeRepeats ([&]
    {
        fused.reset();
        benchmarkRunGraph (fused, input, fusedOutput);
    });

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
    const std::array<const char*, 4> labels { "baseline (strict)", "baseline + fastMath", "host (strict)", "host + fastMath (default)" };
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
    benchmarkDumpListingIfRequested ("harmonic", graph);
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

    EXPECT_GT (benchmarkChecksum (nativeOutput) * benchmarkChecksum (nativeOutput), 0.0);

    benchmarkReportPolicies ("harmonic bank (32 partials)", { "baseline (strict)", "baseline + fastMath", "host (strict)", "host + fastMath (default)" }, benchmarkPolicies (benchmarkBankSource), nativeTiming);

    for (const auto& kernel : graph.getExecutionReport().getKernels())
    {
        if (kernel.name != "Bank")
            continue;

        std::cout << "\n  kernel: " << kernel.instructionCount << " insts, vectorized "
                  << (kernel.vectorized ? "yes" : "no") << ", x" << kernel.vectorWidth
                  << ", unrolled " << (kernel.unrolled ? "yes" : "no") << "\n";
        break;
    }
}

TEST_F (YdspBenchmarkTests, SineBankAgainstNative)
{
    auto graph = compilePatch (benchmarkSineBankSource, compiler, allOptimizations());
    ASSERT_TRUE (graph.isValid());
    graph.prepare (benchmarkSampleRate, benchmarkBlockSize);

    const auto jitTiming = benchmarkTimeRepeats ([&]
    {
        graph.reset();
        benchmarkRunGraph (graph, input, jitOutput);
    });

    const auto nativeTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeSineBank reference (benchmarkSampleRate);

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (nativeOutput.data() + offset, benchmarkBlockSize);
        }
    });

    // Relative, not exact: the reference's compiled `sinf` and the JIT's can
    // differ by a ULP or two under fastMath, and this shape is exactly where
    // the SLEEF migration is allowed to move the result.
    const auto native = benchmarkMagnitude (nativeOutput);
    ASSERT_GT (native, 0.0);
    EXPECT_NEAR (native, benchmarkMagnitude (jitOutput), 1.0e-3 * native);

    benchmarkReportPolicies ("sine bank (8 sin()/sample)", { "baseline (strict)", "baseline + fastMath", "host (strict)", "host + fastMath (default)" }, benchmarkPolicies (benchmarkSineBankSource), nativeTiming, benchmarkTranscendentalLimit);
    benchmarkDumpListingIfRequested ("sine bank", graph);
    benchmarkReportListing ("sine bank", benchmarkAnalyzeListing (graph));

    for (const auto& kernel : graph.getExecutionReport().getKernels())
    {
        if (kernel.name != "SineBank")
            continue;

        std::cout << "\n  kernel: " << kernel.instructionCount << " insts, vectorized "
                  << (kernel.vectorized ? "yes" : "no") << ", x" << kernel.vectorWidth
                  << ", unrolled " << (kernel.unrolled ? "yes" : "no") << "\n";
        break;
    }
}

TEST_F (YdspBenchmarkTests, TanhShaperAgainstNative)
{
    auto graph = compilePatch (benchmarkTanhShaperSource, compiler, allOptimizations());
    benchmarkDumpListingIfRequested ("tanh shaper", graph);
    ASSERT_TRUE (graph.isValid());
    graph.prepare (benchmarkSampleRate, benchmarkBlockSize);

    const auto jitTiming = benchmarkTimeRepeats ([&]
    {
        graph.reset();
        benchmarkRunGraph (graph, input, jitOutput);
    });

    const auto nativeTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeTanhShaper reference;

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, nativeOutput.data() + offset, benchmarkBlockSize);
        }
    });

    const auto native = benchmarkMagnitude (nativeOutput);
    ASSERT_GT (native, 0.0);
    EXPECT_NEAR (native, benchmarkMagnitude (jitOutput), 1.0e-3 * native);

    benchmarkReportPolicies ("tanh shaper (scalar tanh())", { "baseline (strict)", "baseline + fastMath", "host (strict)", "host + fastMath (default)" }, benchmarkPolicies (benchmarkTanhShaperSource), nativeTiming, benchmarkTranscendentalLimit);
    benchmarkReportListing ("tanh shaper", benchmarkAnalyzeListing (graph));
}

TEST_F (YdspBenchmarkTests, ExpEnvelopeAgainstNative)
{
    auto graph = compilePatch (benchmarkExpEnvelopeSource, compiler, allOptimizations());
    benchmarkDumpListingIfRequested ("exp envelope", graph);
    ASSERT_TRUE (graph.isValid());
    graph.prepare (benchmarkSampleRate, benchmarkBlockSize);

    const auto jitTiming = benchmarkTimeRepeats ([&]
    {
        graph.reset();
        benchmarkRunGraph (graph, input, jitOutput);
    });

    const auto nativeTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeExpEnvelope reference;

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, nativeOutput.data() + offset, benchmarkBlockSize);
        }
    });

    const auto native = benchmarkMagnitude (nativeOutput);
    ASSERT_GT (native, 0.0);
    EXPECT_NEAR (native, benchmarkMagnitude (jitOutput), 1.0e-3 * native);

    benchmarkReportPolicies ("exp envelope (scalar exp())", { "baseline (strict)", "baseline + fastMath", "host (strict)", "host + fastMath (default)" }, benchmarkPolicies (benchmarkExpEnvelopeSource), nativeTiming, benchmarkTranscendentalLimit);
    benchmarkReportListing ("exp envelope", benchmarkAnalyzeListing (graph));
}

TEST_F (YdspBenchmarkTests, WaveShaperAgainstNative)
{
    auto graph = compilePatch (benchmarkShaperSource, compiler, allOptimizations());
    benchmarkDumpListingIfRequested ("wave shaper", graph);
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

    benchmarkReportPolicies ("wave shaper (compare + select)", { "baseline (strict)", "baseline + fastMath", "host (strict)", "host + fastMath (default)" }, benchmarkPolicies (benchmarkShaperSource), nativeTiming);
}

TEST_F (YdspBenchmarkTests, ModalBankAgainstNative)
{
    auto graph = compilePatch (benchmarkModalSource, compiler, allOptimizations());
    benchmarkDumpListingIfRequested ("modal", graph);
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

    const auto native = benchmarkMagnitude (nativeOutput);
    ASSERT_GT (native, 0.0);
    EXPECT_NEAR (native, benchmarkMagnitude (jitOutput), 1.0e-3 * native);

    benchmarkReportPolicies ("modal bank (loop-invariant inner work)", { "baseline (strict)", "baseline + fastMath", "host (strict)", "host + fastMath (default)" }, benchmarkPolicies (benchmarkModalSource), nativeTiming, benchmarkModalBankLimit);
}

TEST_F (YdspBenchmarkTests, WaveFolderAgainstNative)
{
    auto graph = compilePatch (benchmarkFolderSource, compiler, allOptimizations());
    benchmarkDumpListingIfRequested ("wave folder", graph);
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

    benchmarkReportPolicies ("wave folder (branchy if/else)", { "baseline (strict)", "baseline + fastMath", "host (strict)", "host + fastMath (default)" }, benchmarkPolicies (benchmarkFolderSource), nativeTiming);

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

//==============================================================================
// Real-life effect kernels (shapes 9-17). These are the DSP blocks a real
// effect chain is made of - feedback delays, modulated delays, reverb,
// dynamics, drive, filters and lo-fi processors - each measured against a
// hand-written C++ routine computing the same thing. The first five mirror the
// shipped fx/ example processors (UI annotations stripped, parameters left at
// their declared defaults); the last four are classic published algorithms.
//
//   BenchmarkEcho         - fractionally addressed feedback delay with a
//                           one-pole damping lowpass in the feedback path
//                           (fx/Delay.ydsp): an interpolated ring read, an int
//                           write-position state, and a mul/add feedback loop
//   BenchmarkChorus       - modulated delay tap (fx/Chorus.ydsp): a sine LFO
//                           moves the read position of a fixed ring, so one
//                           libm sin rides on top of the ring traffic
//   BenchmarkReverb       - Freeverb topology (fx/Reverb.ydsp): eight parallel
//                           combs with per-comb one-pole damping feeding four
//                           series allpass diffusers; the most delay memory
//                           and state-pointer traffic of the set
//   BenchmarkCompressor   - dB-domain feedforward bus compressor
//                           (fx/Compressor.ydsp): an envelope follower with
//                           separate attack/release coefficients, a log10 gain
//                           computer and a pow gain stage
//   BenchmarkDistortion   - tanh drive with a one-pole tone control and a
//                           parallel dry/wet mix (fx/Distortion.ydsp)
//   BenchmarkSvf          - TPT (zero-delay-feedback) state-variable low-pass:
//                           a tan-derived coefficient paid once per block and a
//                           tight two-state recurrence per sample
//   BenchmarkPhaser       - six series first-order allpasses swept by one LFO:
//                           a libm sin per sample over twelve scalar state
//                           round-trips
//   BenchmarkKarplus      - Karplus-Strong plucked string: a ring written
//                           through a two-tap averaged damping loop
//   BenchmarkLoFi         - 12:1 sample-rate reduction plus a 6-bit
//                           quantization stage: integer counter state feeding a
//                           floor-based bitcrusher
//
// Like the shapes above, the JIT-vs-native ratios are printed rather than
// asserted. The parity guard is the same loose one the other native-reference
// shapes use: a checksum agreement where the per-sample loop carries no libm
// call, and a relative-magnitude agreement where it does (scalar JIT
// transcendentals and the reference's can differ by an ulp or two).

namespace
{

//==============================================================================
// Shape 9: a fractionally read feedback delay with damped feedback (the
// fx/Delay.ydsp algorithm). Each sample does an int cast, a conditional wrap
// and two data-dependent ring reads in addition to the feedback one-pole, so
// the ring addressing - not just the taps - is what gets measured.

constexpr auto benchmarkEchoSource = R"YDSP(
    processor Echo {
        input stream in;
        output stream out;

        input value float time = 0.375;
        input value float feedback = 0.4;
        input value float damping = 0.5;
        input value float mix = 0.35;

        state float buf[96000];
        state int wp;
        state float damped;

        process {
            float fdelay = time * sampleRate;
            if (fdelay > 96000.0) { fdelay = 96000.0; }
            if (fdelay < 1.0) { fdelay = 1.0; }

            int d0 = int (fdelay);
            float frac = fdelay - float (d0);

            int readA = wp - d0;
            if (readA < 0) { readA = readA + 96000; }

            int readB = readA - 1;
            if (readB < 0) { readB = readB + 96000; }

            float delayed = buf[readA] * (1.0 - frac) + buf[readB] * frac;

            damped = damped * damping + delayed * (1.0 - damping);

            buf[wp] = in + feedback * damped;
            wp = wp + 1;
            if (wp >= 96000) { wp = 0; }

            out = (1.0 - mix) * in + mix * delayed;
        }
    }

    graph G {
        input stream x;
        output stream y;

        node e = Echo;

        connection { x -> e.in; e.out -> y; }
    }
)YDSP";

class BenchmarkNativeEcho
{
public:
    BenchmarkNativeEcho()
    {
        ring.assign (static_cast<size_t> (ringSize), 0.0f);
    }

    void process (const float* in, float* out, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto x = in[i];

            auto fdelay = delaySeconds * sampleRate;
            if (fdelay > static_cast<float> (ringSize)) fdelay = static_cast<float> (ringSize);
            if (fdelay < 1.0f) fdelay = 1.0f;

            const auto d0 = static_cast<int> (fdelay);
            const auto frac = fdelay - static_cast<float> (d0);

            auto readA = wp - d0;
            if (readA < 0) readA += ringSize;

            auto readB = readA - 1;
            if (readB < 0) readB += ringSize;

            const auto delayed = ring[static_cast<size_t> (readA)] * (1.0f - frac) + ring[static_cast<size_t> (readB)] * frac;

            damped = damped * damping + delayed * (1.0f - damping);

            ring[static_cast<size_t> (wp)] = x + feedback * damped;

            if (++wp == ringSize)
                wp = 0;

            out[i] = (1.0f - mix) * x + mix * delayed;
        }
    }

private:
    static constexpr int ringSize = 96000;
    static constexpr float sampleRate = 48000.0f;
    static constexpr float delaySeconds = 0.375f;
    static constexpr float feedback = 0.4f;
    static constexpr float damping = 0.5f;
    static constexpr float mix = 0.35f;

    std::vector<float> ring;
    int wp = 0;
    float damped = 0.0f;
};

//==============================================================================
// Shape 10: a chorus (fx/Chorus.ydsp). The ring write and read share one
// buffer while a sine LFO sweeps the read position around a 20 ms base delay,
// so the per-sample `sin` is what keeps the whole loop scalar.

constexpr auto benchmarkChorusSource = R"YDSP(
    processor Chorus {
        input stream in;
        output stream out;

        input value float rate = 1.5;
        input value float depth = 0.004;
        input value float mix = 0.5;

        state float buf[4096];
        state int wp;
        state float lfoPhase;

        process {
            buf[wp] = in;
            wp = wp + 1;
            if (wp >= 4096) { wp = 0; }

            lfoPhase = lfoPhase + rate / sampleRate;
            if (lfoPhase >= 1.0) { lfoPhase = lfoPhase - 1.0; }

            float lfo = sin (lfoPhase * 6.283185307);
            float delaySamples = clamp (0.02 * sampleRate + depth * sampleRate * lfo, 0.0, 4000.0);

            int readIdx = wp - int (delaySamples);
            if (readIdx < 0) { readIdx = readIdx + 4096; }

            out = (1.0 - mix) * in + mix * buf[readIdx];
        }
    }

    graph G {
        input stream x;
        output stream y;

        node c = Chorus;

        connection { x -> c.in; c.out -> y; }
    }
)YDSP";

class BenchmarkNativeChorus
{
public:
    BenchmarkNativeChorus()
    {
        ring.assign (static_cast<size_t> (ringSize), 0.0f);
    }

    void process (const float* in, float* out, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto x = in[i];

            ring[static_cast<size_t> (wp)] = x;

            if (++wp == ringSize)
                wp = 0;

            lfoPhase = lfoPhase + rate / sampleRate;
            if (lfoPhase >= 1.0f) lfoPhase = lfoPhase - 1.0f;

            const auto lfo = std::sin (lfoPhase * twoPi);
            const auto delaySamples = std::min (std::max (0.02f * sampleRate + depth * sampleRate * lfo, 0.0f), 4000.0f);

            auto readIdx = wp - static_cast<int> (delaySamples);
            if (readIdx < 0) readIdx += ringSize;

            out[i] = (1.0f - mix) * x + mix * ring[static_cast<size_t> (readIdx)];
        }
    }

private:
    static constexpr int ringSize = 4096;
    static constexpr float sampleRate = 48000.0f;
    static constexpr float rate = 1.5f;
    static constexpr float depth = 0.004f;
    static constexpr float mix = 0.5f;
    static constexpr float twoPi = 6.283185307f;

    std::vector<float> ring;
    int wp = 0;
    float lfoPhase = 0.0f;
};

//==============================================================================
// Shape 11: a Freeverb reverb (fx/Reverb.ydsp): eight parallel comb filters
// with per-comb one-pole damping, whose summed outputs run through four series
// allpass diffusers. Written with explicit ring buffers and write positions
// (the shipped file uses '@' delay slots for the same signal flow), because
// the point here is the cost of many independent delay lines and their pointer
// state in one per-sample loop.

constexpr auto benchmarkReverbSource = R"YDSP(
    processor Reverb {
        input stream in;
        output stream out;

        input value float mix = 0.33;
        input value float damping = 0.5;
        input value float roomSize = 0.8;

        state float buf1[1116];
        state int w1;
        state float damp1;

        state float buf2[1188];
        state int w2;
        state float damp2;

        state float buf3[1277];
        state int w3;
        state float damp3;

        state float buf4[1356];
        state int w4;
        state float damp4;

        state float buf5[1422];
        state int w5;
        state float damp5;

        state float buf6[1491];
        state int w6;
        state float damp6;

        state float buf7[1557];
        state int w7;
        state float damp7;

        state float buf8[1617];
        state int w8;
        state float damp8;

        state float ap1[225];
        state int wa1;

        state float ap2[341];
        state int wa2;

        state float ap3[441];
        state int wa3;

        state float ap4[556];
        state int wa4;

        process {
            float feedback = roomSize * 0.28 + 0.7;

            let d1 = buf1[w1];
            damp1 = damp1 * damping + d1 * (1.0 - damping);
            buf1[w1] = feedback * damp1 + 0.015 * in;
            w1 = w1 + 1;
            if (w1 >= 1116) { w1 = 0; }

            let d2 = buf2[w2];
            damp2 = damp2 * damping + d2 * (1.0 - damping);
            buf2[w2] = feedback * damp2 + 0.015 * in;
            w2 = w2 + 1;
            if (w2 >= 1188) { w2 = 0; }

            let d3 = buf3[w3];
            damp3 = damp3 * damping + d3 * (1.0 - damping);
            buf3[w3] = feedback * damp3 + 0.015 * in;
            w3 = w3 + 1;
            if (w3 >= 1277) { w3 = 0; }

            let d4 = buf4[w4];
            damp4 = damp4 * damping + d4 * (1.0 - damping);
            buf4[w4] = feedback * damp4 + 0.015 * in;
            w4 = w4 + 1;
            if (w4 >= 1356) { w4 = 0; }

            let d5 = buf5[w5];
            damp5 = damp5 * damping + d5 * (1.0 - damping);
            buf5[w5] = feedback * damp5 + 0.015 * in;
            w5 = w5 + 1;
            if (w5 >= 1422) { w5 = 0; }

            let d6 = buf6[w6];
            damp6 = damp6 * damping + d6 * (1.0 - damping);
            buf6[w6] = feedback * damp6 + 0.015 * in;
            w6 = w6 + 1;
            if (w6 >= 1491) { w6 = 0; }

            let d7 = buf7[w7];
            damp7 = damp7 * damping + d7 * (1.0 - damping);
            buf7[w7] = feedback * damp7 + 0.015 * in;
            w7 = w7 + 1;
            if (w7 >= 1557) { w7 = 0; }

            let d8 = buf8[w8];
            damp8 = damp8 * damping + d8 * (1.0 - damping);
            buf8[w8] = feedback * damp8 + 0.015 * in;
            w8 = w8 + 1;
            if (w8 >= 1617) { w8 = 0; }

            let wet = d1 + d2 + d3 + d4 + d5 + d6 + d7 + d8;

            let a1d = ap1[wa1];
            ap1[wa1] = wet + a1d * 0.5;
            wa1 = wa1 + 1;
            if (wa1 >= 225) { wa1 = 0; }
            let x1 = a1d - wet;

            let a2d = ap2[wa2];
            ap2[wa2] = x1 + a2d * 0.5;
            wa2 = wa2 + 1;
            if (wa2 >= 341) { wa2 = 0; }
            let x2 = a2d - x1;

            let a3d = ap3[wa3];
            ap3[wa3] = x2 + a3d * 0.5;
            wa3 = wa3 + 1;
            if (wa3 >= 441) { wa3 = 0; }
            let x3 = a3d - x2;

            let a4d = ap4[wa4];
            ap4[wa4] = x3 + a4d * 0.5;
            wa4 = wa4 + 1;
            if (wa4 >= 556) { wa4 = 0; }
            let x4 = a4d - x3;

            out = (1.0 - mix) * in + mix * x4;
        }
    }

    graph G {
        input stream x;
        output stream y;

        node r = Reverb;

        connection { x -> r.in; r.out -> y; }
    }
)YDSP";

class BenchmarkNativeReverb
{
public:
    BenchmarkNativeReverb()
    {
        for (int c = 0; c < 8; ++c)
            combs[static_cast<size_t> (c)].assign (static_cast<size_t> (combLengths[static_cast<size_t> (c)]), 0.0f);

        for (int c = 0; c < 4; ++c)
            allpasses[static_cast<size_t> (c)].assign (static_cast<size_t> (allpassLengths[static_cast<size_t> (c)]), 0.0f);
    }

    void process (const float* in, float* out, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto x = in[i];
            const auto feedback = roomSize * 0.28f + 0.7f;

            auto wet = 0.0f;

            for (int c = 0; c < 8; ++c)
            {
                const auto idx = combWrite[static_cast<size_t> (c)];
                const auto d = combs[static_cast<size_t> (c)][static_cast<size_t> (idx)];

                damp[static_cast<size_t> (c)] = damp[static_cast<size_t> (c)] * damping + d * (1.0f - damping);

                combs[static_cast<size_t> (c)][static_cast<size_t> (idx)] = feedback * damp[static_cast<size_t> (c)] + 0.015f * x;

                auto& wp = combWrite[static_cast<size_t> (c)];
                if (++wp == combLengths[static_cast<size_t> (c)])
                    wp = 0;

                wet += d;
            }

            auto stage = wet;

            for (int c = 0; c < 4; ++c)
            {
                const auto idx = allpassWrite[static_cast<size_t> (c)];
                const auto d = allpasses[static_cast<size_t> (c)][static_cast<size_t> (idx)];

                allpasses[static_cast<size_t> (c)][static_cast<size_t> (idx)] = stage + d * 0.5f;

                auto& wp = allpassWrite[static_cast<size_t> (c)];
                if (++wp == allpassLengths[static_cast<size_t> (c)])
                    wp = 0;

                stage = d - stage;
            }

            out[i] = (1.0f - mix) * x + mix * stage;
        }
    }

private:
    static constexpr int combLengths[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
    static constexpr int allpassLengths[4] = { 225, 341, 441, 556 };
    static constexpr float damping = 0.5f;
    static constexpr float roomSize = 0.8f;
    static constexpr float mix = 0.33f;

    std::vector<float> combs[8];
    std::vector<float> allpasses[4];
    int combWrite[8] = {};
    int allpassWrite[4] = {};
    float damp[8] = {};
};

//==============================================================================
// Shape 12: a dB-domain feedforward bus compressor (fx/Compressor.ydsp): the
// envelope follower picks attack or release per sample, the gain computer
// works in dB with log10 and the gain stage is a pow - two libm calls per
// sample whenever the level sits above the threshold.

constexpr auto benchmarkCompressorSource = R"YDSP(
    processor Compressor {
        input stream in;
        output stream out;

        input value float threshold = -18.0;
        input value float ratio = 4.0;
        input value float attack = 0.005;
        input value float release = 0.15;
        input value float makeup = 1.0;

        state float env;

        process {
            float attackCoeff = 1.0 - exp (-1.0 / (attack * sampleRate));
            float releaseCoeff = 1.0 - exp (-1.0 / (release * sampleRate));

            float level = abs (in);
            float coeff = level > env ? attackCoeff : releaseCoeff;
            env = env + coeff * (level - env);

            float envDb = 20.0 * log10 (env + 1e-6);
            float gainDb = 0.0;
            if (envDb > threshold) { gainDb = (threshold - envDb) * (1.0 - 1.0 / ratio); }

            float gain = pow (10.0, gainDb / 20.0);
            out = in * gain * makeup;
        }
    }

    graph G {
        input stream x;
        output stream y;

        node c = Compressor;

        connection { x -> c.in; c.out -> y; }
    }
)YDSP";

class BenchmarkNativeCompressor
{
public:
    void process (const float* in, float* out, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto x = in[i];

            const auto attackCoeff = 1.0f - std::exp (-1.0f / (attack * sampleRate));
            const auto releaseCoeff = 1.0f - std::exp (-1.0f / (release * sampleRate));

            const auto level = std::fabs (x);
            const auto coeff = level > env ? attackCoeff : releaseCoeff;
            env = env + coeff * (level - env);

            const auto envDb = 20.0f * std::log10 (env + 1.0e-6f);

            auto gainDb = 0.0f;
            if (envDb > threshold) gainDb = (threshold - envDb) * (1.0f - 1.0f / ratio);

            const auto gain = std::pow (10.0f, gainDb / 20.0f);
            out[i] = x * gain * makeup;
        }
    }

private:
    static constexpr float sampleRate = 48000.0f;
    static constexpr float threshold = -18.0f;
    static constexpr float ratio = 4.0f;
    static constexpr float attack = 0.005f;
    static constexpr float release = 0.15f;
    static constexpr float makeup = 1.0f;

    float env = 0.0f;
};

//==============================================================================
// Shape 13: a tanh drive distortion with a one-pole tone control and a
// parallel dry/wet mix (fx/Distortion.ydsp). A scalar libm tanh per sample,
// with the tone one-pole and the mix as pure multiply/add after it.

constexpr auto benchmarkDistortionSource = R"YDSP(
    processor Distortion {
        input stream in;
        output stream out;

        input value float drive = 1.0;
        input value float tone = 0.5;
        input value float mix = 0.7;

        state float z1;

        process {
            float shaped = tanh (in * drive);

            float k = 0.05 + 0.95 * tone;
            z1 = z1 + k * (shaped - z1);

            out = in + mix * (z1 - in);
        }
    }

    graph G {
        input stream x;
        output stream y;

        node d = Distortion;

        connection { x -> d.in; d.out -> y; }
    }
)YDSP";

class BenchmarkNativeDistortion
{
public:
    void process (const float* in, float* out, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto x = in[i];

            const auto shaped = std::tanh (x * drive);

            const auto k = 0.05f + 0.95f * tone;
            z1 = z1 + k * (shaped - z1);

            out[i] = x + mix * (z1 - x);
        }
    }

private:
    static constexpr float drive = 1.0f;
    static constexpr float tone = 0.5f;
    static constexpr float mix = 0.7f;

    float z1 = 0.0f;
};

//==============================================================================
// Shape 14: a TPT (zero-delay-feedback) state-variable low-pass. The
// tan-derived coefficients only depend on parameters, so they are paid once
// per block; the per-sample loop is a tight two-state recurrence of pure
// multiply/adds - the shape a real parametric filter bank spends most of its
// time in. The `tan` sits in the per-sample source but is hoisted (its
// operands are parameters); even if hoisting ever stopped firing, the parity
// guard below would still hold, because the native reference recomputes the
// same invariant libm call per sample.

constexpr auto benchmarkSvfSource = R"YDSP(
    processor Svf {
        input stream in;
        output stream out;

        input value float cutoff = 1000.0;
        input value float resonance = 0.7;

        state float ic1eq;
        state float ic2eq;

        process {
            float g = tan (pi * cutoff * samplePeriod);
            float k = 1.0 / clamp (resonance, 0.01, 1.0);

            float a1 = 1.0 / (1.0 + g * (g + k));
            float a2 = g * a1;
            float a3 = g * a2;

            float v3 = in - ic2eq;
            float v1 = a1 * ic1eq + a2 * v3;
            float v2 = ic2eq + a2 * ic1eq + a3 * v3;

            ic1eq = 2.0 * v1 - ic1eq;
            ic2eq = 2.0 * v2 - ic2eq;

            out = v2;
        }
    }

    graph G {
        input stream x;
        output stream y;

        node f = Svf;

        connection { x -> f.in; f.out -> y; }
    }
)YDSP";

class BenchmarkNativeSvf
{
public:
    void process (const float* in, float* out, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto x = in[i];

            const auto g = std::tan (pi * cutoff * samplePeriod);
            const auto k = 1.0f / std::min (std::max (resonance, 0.01f), 1.0f);

            const auto a1 = 1.0f / (1.0f + g * (g + k));
            const auto a2 = g * a1;
            const auto a3 = g * a2;

            const auto v3 = x - ic2eq;
            const auto v1 = a1 * ic1eq + a2 * v3;
            const auto v2 = ic2eq + a2 * ic1eq + a3 * v3;

            ic1eq = 2.0f * v1 - ic1eq;
            ic2eq = 2.0f * v2 - ic2eq;

            out[i] = v2;
        }
    }

private:
    static constexpr float samplePeriod = 1.0f / 48000.0f;
    static constexpr float pi = 3.14159265358979323846f;
    static constexpr float cutoff = 1000.0f;
    static constexpr float resonance = 0.7f;

    float ic1eq = 0.0f;
    float ic2eq = 0.0f;
};

//==============================================================================
// Shape 15: a six-stage phaser. Six series first-order allpasses share one
// LFO-driven coefficient, so every sample pays a libm sin plus twelve scalar
// state round-trips - the feedback-free counterpart to the ladder's serial
// state chain, and a genuinely scalar loop.

constexpr auto benchmarkPhaserSource = R"YDSP(
    processor Phaser {
        input stream in;
        output stream out;

        input value float rate = 0.4;
        input value float depth = 0.7;

        state float phase;
        state float xm[6];
        state float ym[6];

        process {
            phase = phase + rate / sampleRate;
            if (phase >= 1.0) { phase = phase - 1.0; }

            let g = depth * sin (phase * 2.0 * pi);

            let s1 = in;
            let o1 = g * s1 + xm[0] - g * ym[0];
            xm[0] = s1;
            ym[0] = o1;

            let s2 = o1;
            let o2 = g * s2 + xm[1] - g * ym[1];
            xm[1] = s2;
            ym[1] = o2;

            let s3 = o2;
            let o3 = g * s3 + xm[2] - g * ym[2];
            xm[2] = s3;
            ym[2] = o3;

            let s4 = o3;
            let o4 = g * s4 + xm[3] - g * ym[3];
            xm[3] = s4;
            ym[3] = o4;

            let s5 = o4;
            let o5 = g * s5 + xm[4] - g * ym[4];
            xm[4] = s5;
            ym[4] = o5;

            let s6 = o5;
            let o6 = g * s6 + xm[5] - g * ym[5];
            xm[5] = s6;
            ym[5] = o6;

            out = (in + o6) * 0.5;
        }
    }

    graph G {
        input stream x;
        output stream y;

        node p = Phaser;

        connection { x -> p.in; p.out -> y; }
    }
)YDSP";

class BenchmarkNativePhaser
{
public:
    void process (const float* in, float* out, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto x = in[i];

            phase = phase + rate / sampleRate;
            if (phase >= 1.0f) phase = phase - 1.0f;

            const auto g = depth * std::sin (phase * twoPi);

            auto s = x;

            for (int stage = 0; stage < 6; ++stage)
            {
                const auto o = g * s + xm[stage] - g * ym[stage];
                xm[stage] = s;
                ym[stage] = o;
                s = o;
            }

            out[i] = (x + s) * 0.5f;
        }
    }

private:
    static constexpr float sampleRate = 48000.0f;
    static constexpr float rate = 0.4f;
    static constexpr float depth = 0.7f;
    static constexpr float twoPi = 6.283185307f;

    float phase = 0.0f;
    float xm[6] = {};
    float ym[6] = {};
};

//==============================================================================
// Shape 16: a Karplus-Strong plucked string. A two-tap averaged damping loop
// writes a ring through its own write position - the integer wrap and the
// second (one-older) tap are the interesting per-sample work.

constexpr auto benchmarkKarplusSource = R"YDSP(
    let stringLen = 550;

    processor Karplus {
        input stream in;
        output stream out;

        state float buf[stringLen];
        state int wp;

        process {
            int read = wp - 1;
            if (read < 0) { read = stringLen - 1; }

            let d0 = buf[wp];
            let d1 = buf[read];

            let damped = in + (d0 + d1) * 0.498;

            buf[wp] = damped;
            wp = wp + 1;
            if (wp >= stringLen) { wp = 0; }

            out = damped;
        }
    }

    graph G {
        input stream x;
        output stream y;

        node k = Karplus;

        connection { x -> k.in; k.out -> y; }
    }
)YDSP";

class BenchmarkNativeKarplus
{
public:
    BenchmarkNativeKarplus()
    {
        ring.assign (static_cast<size_t> (stringLen), 0.0f);
    }

    void process (const float* in, float* out, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto x = in[i];

            auto read = wp - 1;
            if (read < 0) read = stringLen - 1;

            const auto d0 = ring[static_cast<size_t> (wp)];
            const auto d1 = ring[static_cast<size_t> (read)];

            const auto damped = x + (d0 + d1) * 0.498f;

            ring[static_cast<size_t> (wp)] = damped;

            if (++wp == stringLen)
                wp = 0;

            out[i] = damped;
        }
    }

private:
    static constexpr int stringLen = 550;

    std::vector<float> ring;
    int wp = 0;
};

//==============================================================================
// Shape 17: a lo-fi processor - 12:1 sample-and-hold rate reduction feeding a
// 6-bit quantization stage. The per-sample loop is mostly integer state (the
// downsampling counter) plus one floor-based bitcrush, so it isolates the
// int/float crossover the transcendental shapes do not touch.

constexpr auto benchmarkLoFiSource = R"YDSP(
    processor LoFi {
        input stream in;
        output stream out;

        input value float drive = 2.0;

        state float held;
        state int counter;

        process {
            if (counter == 0) { held = in; }
            counter = counter + 1;
            if (counter >= 12) { counter = 0; }

            let quantized = floor (held * 64.0) * 0.015625;
            out = clamp (quantized * drive, -1.0, 1.0);
        }
    }

    graph G {
        input stream x;
        output stream y;

        node l = LoFi;

        connection { x -> l.in; l.out -> y; }
    }
)YDSP";

class BenchmarkNativeLoFi
{
public:
    void process (const float* in, float* out, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto x = in[i];

            if (counter == 0)
                held = x;

            if (++counter >= hold)
                counter = 0;

            const auto quantized = std::floor (held * 64.0f) * 0.015625f;
            out[i] = std::min (std::max (quantized * drive, -1.0f), 1.0f);
        }
    }

private:
    static constexpr int hold = 12;
    static constexpr float drive = 2.0f;

    float held = 0.0f;
    int counter = 0;
};

} // namespace

//==============================================================================
// Real-life native-reference benchmarks (shapes 9-17).

TEST_F (YdspBenchmarkTests, EchoFeedbackDelayAgainstNative)
{
    auto graph = compilePatch (benchmarkEchoSource, compiler, allOptimizations());
    benchmarkDumpListingIfRequested ("echo", graph);
    benchmarkReportListing ("echo", benchmarkAnalyzeListing (graph));
    ASSERT_TRUE (graph.isValid());
    graph.prepare (benchmarkSampleRate, benchmarkBlockSize);

    const auto jitTiming = benchmarkTimeRepeats ([&]
    {
        graph.reset();
        benchmarkRunGraph (graph, input, jitOutput);
    });

    const auto nativeTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeEcho reference;

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, nativeOutput.data() + offset, benchmarkBlockSize);
        }
    });

    EXPECT_NEAR (benchmarkChecksum (nativeOutput), benchmarkChecksum (jitOutput), 1.0);

    benchmarkReportPolicies ("feedback echo (fractional delay + damping)", { "baseline (strict)", "baseline + fastMath", "host (strict)", "host + fastMath (default)" }, benchmarkPolicies (benchmarkEchoSource), nativeTiming);
}

TEST_F (YdspBenchmarkTests, ChorusAgainstNative)
{
    auto graph = compilePatch (benchmarkChorusSource, compiler, allOptimizations());
    benchmarkDumpListingIfRequested ("chorus", graph);
    benchmarkReportListing ("chorus", benchmarkAnalyzeListing (graph));
    ASSERT_TRUE (graph.isValid());
    graph.prepare (benchmarkSampleRate, benchmarkBlockSize);

    const auto jitTiming = benchmarkTimeRepeats ([&]
    {
        graph.reset();
        benchmarkRunGraph (graph, input, jitOutput);
    });

    const auto nativeTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeChorus reference;

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, nativeOutput.data() + offset, benchmarkBlockSize);
        }
    });

    const auto native = benchmarkMagnitude (nativeOutput);
    ASSERT_GT (native, 0.0);
    EXPECT_NEAR (native, benchmarkMagnitude (jitOutput), 1.0e-3 * native);

    benchmarkReportPolicies ("chorus (modulated delay tap)", { "baseline (strict)", "baseline + fastMath", "host (strict)", "host + fastMath (default)" }, benchmarkPolicies (benchmarkChorusSource), nativeTiming, benchmarkTranscendentalLimit);
}

TEST_F (YdspBenchmarkTests, ReverbAgainstNative)
{
    auto graph = compilePatch (benchmarkReverbSource, compiler, allOptimizations());
    benchmarkDumpListingIfRequested ("reverb", graph);
    benchmarkReportListing ("reverb", benchmarkAnalyzeListing (graph));
    ASSERT_TRUE (graph.isValid());
    graph.prepare (benchmarkSampleRate, benchmarkBlockSize);

    const auto jitTiming = benchmarkTimeRepeats ([&]
    {
        graph.reset();
        benchmarkRunGraph (graph, input, jitOutput);
    });

    const auto nativeTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeReverb reference;

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, nativeOutput.data() + offset, benchmarkBlockSize);
        }
    });

    EXPECT_NEAR (benchmarkChecksum (nativeOutput), benchmarkChecksum (jitOutput), 1.0);

    benchmarkReportPolicies ("reverb (8 combs + 4 allpasses)", { "baseline (strict)", "baseline + fastMath", "host (strict)", "host + fastMath (default)" }, benchmarkPolicies (benchmarkReverbSource), nativeTiming);
}

TEST_F (YdspBenchmarkTests, CompressorAgainstNative)
{
    auto graph = compilePatch (benchmarkCompressorSource, compiler, allOptimizations());
    benchmarkDumpListingIfRequested ("compressor", graph);
    benchmarkReportListing ("compressor", benchmarkAnalyzeListing (graph));
    ASSERT_TRUE (graph.isValid());
    graph.prepare (benchmarkSampleRate, benchmarkBlockSize);

    const auto jitTiming = benchmarkTimeRepeats ([&]
    {
        graph.reset();
        benchmarkRunGraph (graph, input, jitOutput);
    });

    const auto nativeTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeCompressor reference;

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, nativeOutput.data() + offset, benchmarkBlockSize);
        }
    });

    const auto native = benchmarkMagnitude (nativeOutput);
    ASSERT_GT (native, 0.0);
    EXPECT_NEAR (native, benchmarkMagnitude (jitOutput), 1.0e-3 * native);

    benchmarkReportPolicies ("bus compressor (dB gain computer)", { "baseline (strict)", "baseline + fastMath", "host (strict)", "host + fastMath (default)" }, benchmarkPolicies (benchmarkCompressorSource), nativeTiming, benchmarkTranscendentalLimit);
}

TEST_F (YdspBenchmarkTests, DistortionAgainstNative)
{
    auto graph = compilePatch (benchmarkDistortionSource, compiler, allOptimizations());
    benchmarkDumpListingIfRequested ("distortion", graph);
    benchmarkReportListing ("distortion", benchmarkAnalyzeListing (graph));
    ASSERT_TRUE (graph.isValid());
    graph.prepare (benchmarkSampleRate, benchmarkBlockSize);

    const auto jitTiming = benchmarkTimeRepeats ([&]
    {
        graph.reset();
        benchmarkRunGraph (graph, input, jitOutput);
    });

    const auto nativeTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeDistortion reference;

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, nativeOutput.data() + offset, benchmarkBlockSize);
        }
    });

    const auto native = benchmarkMagnitude (nativeOutput);
    ASSERT_GT (native, 0.0);
    EXPECT_NEAR (native, benchmarkMagnitude (jitOutput), 1.0e-3 * native);

    benchmarkReportPolicies ("distortion (tanh drive + tone)", { "baseline (strict)", "baseline + fastMath", "host (strict)", "host + fastMath (default)" }, benchmarkPolicies (benchmarkDistortionSource), nativeTiming, benchmarkTranscendentalLimit);
}

TEST_F (YdspBenchmarkTests, SvfAgainstNative)
{
    auto graph = compilePatch (benchmarkSvfSource, compiler, allOptimizations());
    benchmarkDumpListingIfRequested ("svf", graph);
    benchmarkReportListing ("svf", benchmarkAnalyzeListing (graph));
    ASSERT_TRUE (graph.isValid());
    graph.prepare (benchmarkSampleRate, benchmarkBlockSize);

    const auto jitTiming = benchmarkTimeRepeats ([&]
    {
        graph.reset();
        benchmarkRunGraph (graph, input, jitOutput);
    });

    const auto nativeTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeSvf reference;

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, nativeOutput.data() + offset, benchmarkBlockSize);
        }
    });

    EXPECT_NEAR (benchmarkChecksum (nativeOutput), benchmarkChecksum (jitOutput), 1.0);

    benchmarkReportPolicies ("state-variable low-pass (TPT)", { "baseline (strict)", "baseline + fastMath", "host (strict)", "host + fastMath (default)" }, benchmarkPolicies (benchmarkSvfSource), nativeTiming);
}

TEST_F (YdspBenchmarkTests, PhaserAgainstNative)
{
    auto graph = compilePatch (benchmarkPhaserSource, compiler, allOptimizations());
    benchmarkDumpListingIfRequested ("phaser", graph);
    benchmarkReportListing ("phaser", benchmarkAnalyzeListing (graph));
    ASSERT_TRUE (graph.isValid());
    graph.prepare (benchmarkSampleRate, benchmarkBlockSize);

    const auto jitTiming = benchmarkTimeRepeats ([&]
    {
        graph.reset();
        benchmarkRunGraph (graph, input, jitOutput);
    });

    const auto nativeTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativePhaser reference;

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, nativeOutput.data() + offset, benchmarkBlockSize);
        }
    });

    const auto native = benchmarkMagnitude (nativeOutput);
    ASSERT_GT (native, 0.0);
    EXPECT_NEAR (native, benchmarkMagnitude (jitOutput), 1.0e-3 * native);

    benchmarkReportPolicies ("phaser (6-stage allpass)", { "baseline (strict)", "baseline + fastMath", "host (strict)", "host + fastMath (default)" }, benchmarkPolicies (benchmarkPhaserSource), nativeTiming, benchmarkTranscendentalLimit);
}

TEST_F (YdspBenchmarkTests, KarplusStrongAgainstNative)
{
    auto graph = compilePatch (benchmarkKarplusSource, compiler, allOptimizations());
    benchmarkDumpListingIfRequested ("karplus", graph);
    benchmarkReportListing ("karplus", benchmarkAnalyzeListing (graph));
    ASSERT_TRUE (graph.isValid());
    graph.prepare (benchmarkSampleRate, benchmarkBlockSize);

    const auto jitTiming = benchmarkTimeRepeats ([&]
    {
        graph.reset();
        benchmarkRunGraph (graph, input, jitOutput);
    });

    const auto nativeTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeKarplus reference;

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, nativeOutput.data() + offset, benchmarkBlockSize);
        }
    });

    EXPECT_NEAR (benchmarkChecksum (nativeOutput), benchmarkChecksum (jitOutput), 1.0);

    benchmarkReportPolicies ("karplus-strong (plucked string)", { "baseline (strict)", "baseline + fastMath", "host (strict)", "host + fastMath (default)" }, benchmarkPolicies (benchmarkKarplusSource), nativeTiming);
}

TEST_F (YdspBenchmarkTests, LoFiAgainstNative)
{
    auto graph = compilePatch (benchmarkLoFiSource, compiler, allOptimizations());
    benchmarkDumpListingIfRequested ("lo-fi", graph);
    benchmarkReportListing ("lo-fi", benchmarkAnalyzeListing (graph));
    ASSERT_TRUE (graph.isValid());
    graph.prepare (benchmarkSampleRate, benchmarkBlockSize);

    const auto jitTiming = benchmarkTimeRepeats ([&]
    {
        graph.reset();
        benchmarkRunGraph (graph, input, jitOutput);
    });

    const auto nativeTiming = benchmarkTimeRepeats ([&]
    {
        BenchmarkNativeLoFi reference;

        for (int block = 0; block < benchmarkBlockCount; ++block)
        {
            const auto offset = static_cast<size_t> (block * benchmarkBlockSize);
            reference.process (input.data() + offset, nativeOutput.data() + offset, benchmarkBlockSize);
        }
    });

    EXPECT_NEAR (benchmarkChecksum (nativeOutput), benchmarkChecksum (jitOutput), 1.0);

    benchmarkReportPolicies ("lo-fi (12:1 sample-hold + bitcrush)", { "baseline (strict)", "baseline + fastMath", "host (strict)", "host + fastMath (default)" }, benchmarkPolicies (benchmarkLoFiSource), nativeTiming);
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

    EXPECT_GT (benchmarkMagnitude (jitOutput), 0.0);
    EXPECT_GT (benchmarkMagnitude (noSumOutput), 0.0);

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

    std::cout << "\n  | with sum: " << describe (withSumKernel)
              << "\n  | no sum: " << describe (noSumKernel) << "\n";

    EXPECT_TRUE (withSumKernel.vectorized) << "the accumulating mode loop stopped being widened";
    EXPECT_TRUE (noSumKernel.vectorized) << "the element-wise mode loop is not being widened";
    EXPECT_LT (noSumKernel.instructionCount, withSumKernel.instructionCount)
        << "the no-sum variant should be the smaller kernel";

    EXPECT_EQ (4 <= 16 / withSumKernel.vectorWidth, withSumKernel.reductionSplit)
        << "the split must fire exactly when the unrolled reduction chain has at least four links";
    EXPECT_FALSE (noSumKernel.reductionSplit) << "there is no accumulator here to split";

    benchmarkDumpListingIfRequested ("with sum", withSum);
    benchmarkReportListing ("with sum", benchmarkAnalyzeListing (withSum));
    benchmarkDumpListingIfRequested ("no sum", noSum);
    benchmarkReportListing ("no sum", benchmarkAnalyzeListing (noSum));

    benchmarkReportVariants ("modal bank: what the reduction costs",
                             "with sum",
                             withSumTiming,
                             "no sum",
                             noSumTiming);
}

TEST_F (YdspBenchmarkTests, ChainedNodesAgainstAFusedProcessor)
{
    auto chained = compilePatch (benchmarkChainedSource, compiler);
    auto fused = compilePatch (benchmarkFusedSource, compiler);

    ASSERT_TRUE (chained.isValid());
    ASSERT_TRUE (fused.isValid());

    chained.prepare (benchmarkSampleRate, benchmarkBlockSize);
    fused.prepare (benchmarkSampleRate, benchmarkBlockSize);

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

    EXPECT_NEAR (benchmarkChecksum (fusedOutput), benchmarkChecksum (jitOutput), 1.0);

    benchmarkReportVariants ("kernel fusion: 3 chained nodes vs 1 fused processor",
                             "chained",
                             chainedTiming,
                             "fused",
                             fusedTiming);

    EXPECT_LT (chainedTiming.best / fusedTiming.best, 1.5)
        << "the chained form is no longer being fused";
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

    EXPECT_EQ (16, everyVoice.getActiveVoiceCount ("voices"));

    benchmarkReportVariants ("idle-voice skipping, EPVoice[16] with 2 notes held",
                             "skipping",
                             skippingTiming,
                             "all 16",
                             everyVoiceTiming);

    EXPECT_LT (skippingTiming.best / everyVoiceTiming.best, 0.5)
        << "voice skipping saved less than half the work of running all 16 voices";
}

TEST_F (YdspBenchmarkTests, GraphLevelDryWetAgainstAnInlinedDryWet)
{
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

    std::vector<float> fannedOutput (static_cast<size_t> (benchmarkTotalSamples), 0.0f);

    const auto fannedTiming = benchmarkTimeRepeats ([&]
    {
        fanned.reset();
        benchmarkRunGraph (fanned, input, fannedOutput);
    });

    EXPECT_NEAR (benchmarkChecksum (jitOutput), benchmarkChecksum (fannedOutput), 1.0);

    benchmarkReportVariants ("graph-level dry/wet (fan-out + fan-in) vs. dry/wet inside one processor",
                             "fanned",
                             fannedTiming,
                             "inline",
                             inlinedTiming);

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

//==============================================================================

TEST_F (YdspBenchmarkTests, ScalarLibmCallCostProbe)
{
    // Print-only: the host libm scalar cost per call for the intrinsics the
    // per-sample shapes call, so the compressor ratio can be read as
    // "N libm calls + JIT overhead around them" instead of an opaque 1.4x.
    // The scalar transcendental path in the JIT is always libm (fastMath only
    // adds contraction and the AArch64 inline exp), so the callee cost below
    // is the floor both sides pay; anything above it per sample is call+parking
    // and loop overhead on the JIT side.
    constexpr int numSamples = 4096;
    constexpr int numIterations = 2000;

    std::vector<float> input (static_cast<size_t> (numSamples));
    std::vector<float> exponent (static_cast<size_t> (numSamples));

    for (int i = 0; i < numSamples; ++i)
    {
        input[static_cast<size_t> (i)] = 0.25f + 3.75f * static_cast<float> (i % 997) / 996.0f;
        exponent[static_cast<size_t> (i)] = 1.25f + 0.75f * static_cast<float> (i % 97) / 96.0f;
    }

    float sink = 0.0f;

    const auto measureUnary = [&] (const char* name, float (*fn) (float))
    {
        const auto timing = benchmarkTimeRepeats ([&]
        {
            for (int it = 0; it < numIterations; ++it)
                for (int i = 0; i < numSamples; ++i)
                    sink += fn (input[static_cast<size_t> (i)]);
        });

        std::cout << "  " << name << ": " << String (timing.best * 1e9 / (numSamples * numIterations), 3)
                  << " ns/call (host libm)\n";
    };

    const auto measureBinary = [&] (const char* name, float (*fn) (float, float))
    {
        // The exponent varies per element so the host compiler cannot fold the
        // call to a cheaper closed form (powf(x, 1.5) would become x*sqrtf(x)).
        const auto timing = benchmarkTimeRepeats ([&]
        {
            for (int it = 0; it < numIterations; ++it)
                for (int i = 0; i < numSamples; ++i)
                    sink += fn (input[static_cast<size_t> (i)], exponent[static_cast<size_t> (i)]);
        });

        std::cout << "  " << name << ": " << String (timing.best * 1e9 / (numSamples * numIterations), 3)
                  << " ns/call (host libm)\n";
    };

    std::cout << "  scalar libm cost per call (floor both JIT and C++ pay):\n";

    measureUnary ("expf", &::expf);
    measureUnary ("log10f", &::log10f);
    measureUnary ("logf", &::logf);
    measureUnary ("tanhf", &::tanhf);
    measureUnary ("sinf", &::sinf);
    measureBinary ("powf", &::powf);

    std::cout << "  shapes for reference (JIT total ns/sample, avg, fastMath): "
              << "exp envelope 1x expf, tanh shaper 1x tanhf, compressor 1x log10f + 1x powf\n"
              << "  (sink " << String (sink, 2) << ")\n";
}

} // namespace yup::test
