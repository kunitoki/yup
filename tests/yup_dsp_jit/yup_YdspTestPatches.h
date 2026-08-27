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

#include <unordered_map>
#include <vector>

/** Full YDSP patch sources shared by more than one test file.

    Anything long enough that inlining it twice would be a maintenance hazard
    lives here, so the desktop (asmjit) and wasm test suites exercise the exact
    same source text. Short single-purpose snippets stay inline in the test that
    uses them.

    This header is unity-built alongside every other test file, so the helpers
    below live in namespace yup::test::patches to avoid clashing with
    identically-named locals in the files that include it.
*/

namespace yup::test::patches
{

/** Compiles `source` with a fresh compiler, failing the current test via
    EXPECT_TRUE if it does not compile. Shared by the graph, wasm and
    benchmark test suites, which were each carrying a byte-for-byte identical
    copy of this function.
*/
inline DspJitGraph compilePatch (StringRef source, DspJitCompiler& compiler)
{
    auto result = compiler.compile (source);
    EXPECT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    if (! result.wasOk())
        return {};

    return std::move (result).getValue();
}

/** Returns a ramp of `size` samples starting at `start` and increasing by
    0.01 per sample. Shared by the graph and wasm test suites.
*/
inline std::vector<float> makeRamp (int size, float start = 0.0f)
{
    std::vector<float> data (static_cast<size_t> (size));

    for (int i = 0; i < size; ++i)
        data[static_cast<size_t> (i)] = start + static_cast<float> (i) * 0.01f;

    return data;
}

namespace detail
{

struct CachedPatch
{
    DspJitGraph graph;
    String diagnostics;
};

/** Compiles `source` at most once per process, keyed by its text. Deliberately
    does not EXPECT/ASSERT here: this runs inside a function-local static
    initializer, so a failure here would be silently attributed to whichever
    test happens to trigger the first compile, while every later test using the
    same source would receive an invalid graph with no complaint of its own.
    Callers must check isValid() themselves (see cachedPatch/cachedPatchDiagnostics).
*/
inline CachedPatch& cachedPatchEntry (StringRef source)
{
    static std::unordered_map<String, CachedPatch> cache;

    const String key (source);
    auto it = cache.find (key);

    if (it != cache.end())
        return it->second;

    DspJitCompiler compiler;
    auto result = compiler.compile (source);

    CachedPatch entry;
    entry.diagnostics = compiler.getDiagnostics().toString();

    if (result.wasOk())
        entry.graph = std::move (result).getValue();

    return cache.emplace (key, std::move (entry)).first->second;
}

} // namespace detail

/** Returns the graph compiled from `source`, compiling it on first request and
    reusing the same graph (and its JIT-owned executable memory) for every
    later call with the same source text. The graph outlives the compiler that
    produced it, so this is safe to hold onto for the lifetime of the process.

    Callers must check isValid() (and report cachedPatchDiagnostics() on
    failure) themselves - see restoreFreshState() for the accompanying
    per-test state reset.
*/
inline DspJitGraph& cachedPatch (StringRef source)
{
    return detail::cachedPatchEntry (source).graph;
}

/** Returns the compiler diagnostics produced when `source` was first compiled
    by cachedPatch(). Empty when compilation succeeded with nothing to report.
*/
inline const String& cachedPatchDiagnostics (StringRef source)
{
    return detail::cachedPatchEntry (source).diagnostics;
}

/** Restores a cached graph to a freshly-prepared state: prepare(), then
    reset(), then every parameter is written back to its declared default.

    The explicit parameter write-back is required because reset() leaves
    parameter values untouched by design (see DspJitGraph::reset()) - without
    it, a test that changes a parameter would leak that change into the next
    test sharing the same cached graph.
*/
inline void restoreFreshState (DspJitGraph& graph, double sampleRate, int blockSize)
{
    graph.prepare (sampleRate, blockSize);
    graph.reset();

    for (int i = 0; i < graph.getParameterCount(); ++i)
    {
        const auto& info = graph.getParameterInfo (i);

        switch (info.type)
        {
            case DspJitElementType::float32:
                graph.setParameter (info.name, static_cast<float> (info.defaultValue));
                break;

            case DspJitElementType::float64:
                graph.setDoubleParameter (info.name, info.defaultValue);
                break;

            case DspJitElementType::int32:
            case DspJitElementType::int64:
                graph.setIntParameter (info.name, static_cast<int64_t> (info.defaultValue));
                break;

            case DspJitElementType::boolean:
                break;
        }
    }
}

/** The shipped Electric Piano patch (examples/graphics/data/synths/ElectricPiano.ydsp).

    Kept in sync by hand: the test target has no path into the examples tree. The
    shipped file's comments and `declare` metadata are dropped here and everything
    is indented one level; the code is otherwise identical, so a diff against it
    should show only those differences.
*/
inline constexpr auto electricPiano = R"YDSP(
    let harmonics = 32;
    let rampSteps = 64;

    processor EPVoice {
        output stream out;

        input value float brightness = 30.0;
        input value float velocitySensitivity = 60.0;
        input value float decayRate = 50.0;
        input value float harmonicDecayRate = 50.0;
        input value float keyScaling = 50.0;
        input value float releaseRate = 40.0;

        input event midi;

        state float bendFactor = 1.0;
        state float modWheel;
        state float vibratoPhase;

        state float velAt100[harmonics] = {
            0.150869, 0.385766, 0.166484, 0.087412, 0.114967, 0.062138, 0.038751, 0.048902,
            0.031164, 0.019870, 0.024413, 0.015082, 0.010337, 0.012061, 0.007988, 0.005541,
            0.006272, 0.004319, 0.003043, 0.003371, 0.002397, 0.001723, 0.001858, 0.001361,
            0.000994, 0.001048, 0.000784, 0.000582, 0.000602, 0.000459, 0.000345, 0.000352
        };

        state float velAt0[harmonics] = {
            0.020000, 0.050000, 0.012000, 0.004500, 0.003000, 0.001200, 0.000600, 0.000350,
            0.000200, 0.000120, 0.000075, 0.000048, 0.000030, 0.000020, 0.000013, 0.000009
        };

        state float oscR[harmonics];
        state float oscI[harmonics];
        state float mulR[harmonics];
        state float mulI[harmonics];

        state float amp[harmonics];
        state float inc[harmonics];
        state float dec[harmonics];
        state float rel[harmonics];

        state int released;
        state int rampCount;

        state float baseOmega;
        state float lastPitchFactor;

        // The runtime skips a released voice whose activity flag reads 0, so this
        // patch only has to answer "am I still audible?" - a held key keeps its
        // voice running regardless. Starts at 0, so every voice starts asleep.
        state int active [[ role: voiceActivity ]];

        event midi (e: noteOn) {
            let freq = 440.0 * pow (2.0, (e.pitch - 69.0) / 12.0);
            let nyquist = sampleRate * 0.5;
            let omega = 2.0 * pi * freq * samplePeriod;

            baseOmega = omega;

            let keyFactor = pow (0.5, ((e.pitch - 60.0) / 24.0) * (keyScaling * 0.01));
            let decaySeconds = (0.15 + decayRate * 0.14) * keyFactor;
            let releaseSeconds = 0.02 + releaseRate * 0.008;

            let velocity = clamp (e.velocity, 0.0, 1.0);
            let sensitivity = clamp (velocitySensitivity * 0.01, 0.0, 1.0);
            let blend = clamp (1.0 - sensitivity + velocity * sensitivity, 0.0, 1.0);
            let tilt = brightness * 0.01 - 0.5;

            released = 0;
            rampCount = 0;
            active = 1;

            bendFactor = pow (2.0, e.bendSemitones / 12.0);
            vibratoPhase = 0.0;
            lastPitchFactor = 1.0;

            for i in 0..harmonics {
                let partial = float (i + 1);
                let w = omega * partial;
                let audible = select (freq * partial < nyquist, 1.0, 0.0);

                mulR[i] = cos (w);
                mulI[i] = sin (w);
                oscR[i] = 1.0;
                oscI[i] = 0.0;

                amp[i] = lerp (velAt0[i], velAt100[i], blend) * pow (partial, tilt) * velocity * audible;
                inc[i] = 0.0;

                let harmonicFactor = 1.0 + (partial - 1.0) * (harmonicDecayRate * 0.05);
                dec[i] = pow (0.001, (rampSteps * samplePeriod) / (decaySeconds / harmonicFactor));
                rel[i] = pow (0.001, (rampSteps * samplePeriod) / releaseSeconds);
            }
        }

        event midi (e: noteOff) {
            released = 1;
        }

        event midi (e: pitchBend) {
            bendFactor = pow (2.0, e.bendSemitones / 12.0);
        }

        event midi (e: controlChange) {
            if (e.control == 1) { modWheel = e.value; }
        }

        process {
            float peak = 0.0;

            if (rampCount <= 0) {
                rampCount = rampSteps;

                vibratoPhase = vibratoPhase + 5.5 * float (rampSteps) * samplePeriod;
                vibratoPhase = vibratoPhase - floor (vibratoPhase);

                float pitchFactor = bendFactor
                                  * (1.0 + 0.02888113 * modWheel * sin (vibratoPhase * 2.0 * pi));

                if (pitchFactor != lastPitchFactor) {
                    lastPitchFactor = pitchFactor;

                    for i in 0..harmonics {
                        let w = baseOmega * float (i + 1) * pitchFactor;
                        mulR[i] = cos (w);
                        mulI[i] = sin (w);
                    }
                }

                for i in 0..harmonics {
                    let target = amp[i] * select (released > 0, rel[i], dec[i]);
                    inc[i] = (target - amp[i]) / rampSteps;
                    peak = max (peak, abs (amp[i]));
                }

                // The partial amplitudes *are* this voice's output envelope, so once
                // the loudest of them is below -120 dBFS the voice is finished. Folded
                // into the chunk-boundary loop, so this costs one compare per partial
                // every `rampSteps` samples rather than per sample - and it never tests
                // `sum`, which crosses zero.
                active = select (peak < 0.000001, 0, 1);
            }

            rampCount = rampCount - 1;

            float sum = 0.0;

            for i in 0..harmonics {
                let rotated = oscR[i] * mulR[i] - oscI[i] * mulI[i];
                oscI[i] = oscR[i] * mulI[i] + oscI[i] * mulR[i];
                oscR[i] = rotated;
                amp[i] = amp[i] + inc[i];
                sum = sum + oscI[i] * amp[i];
            }

            out = sum;
        }
    }

    processor Tremolo {
        input stream in;
        output stream outL, outR;

        input value float vibratoRate = 4.0;
        input value float vibratoDepth = 0.5;

        state float phase;
        state float direction = 1.0;

        process {
            let step = 4.0 * vibratoRate * samplePeriod;

            phase = phase + step * direction;

            if (phase >= 1.0) { phase = 1.0; direction = -1.0; }
            if (phase <= -1.0) { phase = -1.0; direction = 1.0; }

            let depth = clamp (vibratoDepth, 0.0, 1.0) * 0.5;

            outL = in * (1.0 - depth + depth * phase);
            outR = in * (1.0 - depth - depth * phase);
        }
    }

    graph ElectricPiano {
        input event midi;

        output stream outL, outR;

        input value float brightness = 30.0 [[ name: "Brightness", min: 0.0, max: 100.0 ]];
        input value float velocitySensitivity = 60.0 [[ name: "Velocity Sensitivity", min: 0.0, max: 100.0 ]];
        input value float decayRate = 50.0 [[ name: "Decay", min: 0.0, max: 100.0 ]];
        input value float harmonicDecayRate = 50.0 [[ name: "Harmonic Decay", min: 0.0, max: 100.0 ]];
        input value float keyScaling = 50.0 [[ name: "Key Scaling", min: 0.0, max: 100.0 ]];
        input value float releaseRate = 40.0 [[ name: "Release", min: 0.0, max: 100.0 ]];
        input value float vibratoRate [[ name: "Vibrato Rate", min: 0.5, max: 12.0, init: 4 ]];
        input value float vibratoDepth = 0.5 [[ name: "Vibrato Depth", min: 0.0, max: 1.0 ]];

        node voices = EPVoice[16] [[ mode: poly, stealing: oldest ]];
        node trem = Tremolo;

        connection {
            midi -> voices.midi;

            voices.out -> trem.in;

            trem.outL -> outL;
            trem.outR -> outR;

            brightness -> voices.brightness;
            velocitySensitivity -> voices.velocitySensitivity;
            decayRate -> voices.decayRate;
            harmonicDecayRate -> voices.harmonicDecayRate;
            keyScaling -> voices.keyScaling;
            releaseRate -> voices.releaseRate;

            vibratoRate -> trem.vibratoRate;
            vibratoDepth -> trem.vibratoDepth;
        }
    }
)YDSP";

} // namespace yup::test::patches
