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

/*
  The demo patches under examples/graphics/data/synths/ are the corpus that
  exercises the language's surface - one patch per feature cluster - and until
  now nothing but a human clicking through the graphics demo's combo box ever
  compiled them. Every other reference to them in this suite is a hand-copied
  excerpt, which cannot go stale loudly.

  The tests below reach outside tests/ on purpose: the point is to compile the
  files that ship, not a copy of them.
*/
File exampleSynthsFolder()
{
    return File (__FILE__)
        .getParentDirectory() // tests/yup_dsp_jit
        .getParentDirectory() // tests
        .getParentDirectory() // repository root
        .getChildFile ("examples")
        .getChildFile ("graphics")
        .getChildFile ("data")
        .getChildFile ("synths");
}

struct ElectricPianoDefaultParam
{
    const char* name;
    double value;
};

} // namespace

//==============================================================================

class YdspExamplePatchTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // On wasm the tests run against a preloaded virtual filesystem that
        // carries tests/data only, so the example folder is not reachable.
        if (! exampleSynthsFolder().isDirectory())
            GTEST_SKIP() << "example synth folder not available on this platform";
    }

    // Compiles one patch with its own path as the import base, which is what the
    // demo app does, so a patch's relative `import fx.Delay` resolves.
    YdspAudioGraph compilePatch (const File& patchFile, YdspCompiler& compiler)
    {
        auto result = compiler.compile (patchFile.loadFileAsString(), patchFile.getFullPathName());

        EXPECT_TRUE (result.wasOk())
            << patchFile.getFileName() << ":\n"
            << compiler.getDiagnostics().toString();

        if (! result.wasOk())
            return YdspAudioGraph {};

        return std::move (result).getValue();
    }

    void testPatch (const char* patchName)
    {
        const auto patchFile = exampleSynthsFolder().getChildFile (patchName);
        ASSERT_TRUE (patchFile.existsAsFile()) << patchName;

        YdspCompiler compiler;
        auto graph = compilePatch (patchFile, compiler);

        EXPECT_TRUE (graph.isValid()) << patchName;

        if (! graph.isValid())
            return;

        EXPECT_GT (graph.getParameterCount(), 0) << patchName;
        EXPECT_EQ (graph.getInputStreamCount(), 0) << patchName;

        const auto isMidiOnlyPatch = patchFile.getFileName() == "ArpTranspose.ydsp";

        if (! isMidiOnlyPatch)
            EXPECT_GE (graph.getOutputStreamCount(), 1) << patchName;

        EXPECT_LE (graph.getOutputStreamCount(), 2) << patchName;

        constexpr int blockSize = 128;
        constexpr double sampleRate = 48000.0;
        graph.prepare (sampleRate, blockSize);

        const auto numOutputs = graph.getOutputStreamCount();

        std::vector<float> left (blockSize, 0.0f);
        std::vector<float> right (blockSize, 0.0f);

        YdspOutputBuffer outputs[] = {
            Span<float> (left.data(), left.size()),
            Span<float> (right.data(), right.size())
        };

        MidiBuffer midi;
        midi.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);

        int nonFinite = 0;

        for (int block = 0; block < 8; ++block)
        {
            graph.process ({},
                           Span<YdspOutputBuffer> (outputs, static_cast<size_t> (numOutputs)),
                           blockSize,
                           &midi,
                           nullptr,
                           0);

            midi.clear();

            for (int channel = 0; channel < numOutputs; ++channel)
            {
                const auto& buffer = channel == 0 ? left : right;

                for (int i = 0; i < blockSize; ++i)
                    if (! std::isfinite (buffer[static_cast<size_t> (i)]))
                        ++nonFinite;
            }
        }

        EXPECT_EQ (nonFinite, 0) << patchName << " produced non-finite samples";
    }

    static float measureHeldNote (YdspAudioGraph& graph, std::vector<float>& energy)
    {
        constexpr double sampleRate = 44100.0;
        constexpr int blockSize = 256;
        constexpr int numBlocks = 96; // 0.56 s of held note at 44100 / 256

        graph.prepare (sampleRate, blockSize);

        std::vector<float> left (blockSize, 0.0f);
        std::vector<float> right (blockSize, 0.0f);

        YdspOutputBuffer outputs[] = {
            Span<float> (left.data(), left.size()),
            Span<float> (right.data(), right.size())
        };

        MidiBuffer midi;
        midi.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);

        energy.assign (static_cast<size_t> (numBlocks), 0.0f);

        for (int block = 0; block < numBlocks; ++block)
        {
            graph.process ({},
                           Span<YdspOutputBuffer> (outputs, 2),
                           blockSize,
                           &midi,
                           nullptr,
                           0);

            midi.clear();

            for (int i = 0; i < blockSize; ++i)
            {
                const auto l = left[static_cast<size_t> (i)];
                const auto r = right[static_cast<size_t> (i)];
                energy[static_cast<size_t> (block)] += l * l + r * r;
            }
        }

        return energy[0];
    }
};

//==============================================================================

TEST_F (YdspExamplePatchTests, AnalogSawCompilesAndRenders)
{
    testPatch ("AnalogSaw.ydsp");
}

TEST_F (YdspExamplePatchTests, ArpPolySineCompilesAndRenders)
{
    testPatch ("ArpPolySine.ydsp");
}

TEST_F (YdspExamplePatchTests, ControlRateWahCompilesAndRenders)
{
    testPatch ("ControlRateWah.ydsp");
}

TEST_F (YdspExamplePatchTests, DigitalDrumsCompilesAndRenders)
{
    testPatch ("DigitalDrums.ydsp");
}

TEST_F (YdspExamplePatchTests, ElectricPianoCompilesAndRenders)
{
    testPatch ("ElectricPiano.ydsp");
}

TEST_F (YdspExamplePatchTests, ElectricPianoSustainsAtDeclaredDefaults)
{
    // Regression guard for the graphics demo: when every EPVoice envelope
    // parameter (decayRate, brightness, ...) read 0 at noteOn time, each note
    // collapsed to a ~0.15 s click, and the suite's short sweeps (~46 ms) could
    // not tell that from the ~7 s decay the declared defaults should produce.
    // This renders the shipped patch for over half a second at untouched
    // defaults and requires the note body to still be sounding.

    const auto patchFile = exampleSynthsFolder().getChildFile ("ElectricPiano.ydsp");
    ASSERT_TRUE (patchFile.existsAsFile());

    YdspCompiler compiler;
    auto graph = compilePatch (patchFile, compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();

    // The graph must already hold its declared defaults - no setParameter call.
    const ElectricPianoDefaultParam declaredDefaults[] = {
        { "brightness", 30.0 },
        { "velocitySensitivity", 60.0 },
        { "decayRate", 50.0 },
        { "harmonicDecayRate", 50.0 },
        { "keyScaling", 50.0 },
        { "releaseRate", 40.0 },
        { "vibratoRate", 4.0 },
        { "vibratoDepth", 0.5 }
    };

    for (const auto& param : declaredDefaults)
        EXPECT_DOUBLE_EQ (param.value, graph.getParameter (param.name)) << param.name;

    std::vector<float> energy;
    const auto attackEnergy = measureHeldNote (graph, energy);

    EXPECT_GT (attackEnergy, 0.0f) << "The note's attack is silent";

    // Block 24 sits at t ~0.14 s, far past where a 0.15 s click has died.
    EXPECT_GT (energy[24], attackEnergy * 0.1f) << "The note died within its attack";

    // The tail window (0.28-0.56 s) must still carry a clearly sounding body.
    // A correct patch is quieter there by design: the per-partial envelope
    // (harmonicDecayRate = 50) sends each partial to -60 dB within ~0.1-0.7 s,
    // so by half a second only the fundamental region is left ringing, at
    // roughly a tenth of the full-attack energy. A zeroed decayRate instead
    // collapses everything to ~1e-12 (and puts the voice to sleep) well before
    // 0.2 s.
    float sustainedEnergy = 0.0f;
    for (int block = 48; block < static_cast<int> (energy.size()); ++block)
        sustainedEnergy += energy[static_cast<size_t> (block)];

    sustainedEnergy /= static_cast<float> (energy.size() - 48);

    EXPECT_GT (sustainedEnergy, attackEnergy * 0.05f) << "The held note died instead of sustaining";
}

TEST_F (YdspExamplePatchTests, ElectricPianoSustainsWhenDefaultsAreExplicitlySet)
{
    // Diagnostic twin of the test above: pushes the declared EPVoice defaults
    // through the host setParameter/ring path before the note. If this one
    // sustains while the untouched-defaults one dies, the compile-time seed
    // never reaches the slot the noteOn kernel reads; if both die, the kernel
    // binds its parameters to a slot that is never written at all.

    const auto patchFile = exampleSynthsFolder().getChildFile ("ElectricPiano.ydsp");

    YdspCompiler compiler;
    auto graph = compilePatch (patchFile, compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();

    // Only the EPVoice envelope params shape the note body.
    const ElectricPianoDefaultParam voiceDefaults[] = {
        { "brightness", 30.0 },
        { "velocitySensitivity", 60.0 },
        { "decayRate", 50.0 },
        { "harmonicDecayRate", 50.0 },
        { "keyScaling", 50.0 },
        { "releaseRate", 40.0 }
    };

    for (const auto& param : voiceDefaults)
        graph.setParameter (param.name, static_cast<float> (param.value));

    std::vector<float> energy;
    const auto attackEnergy = measureHeldNote (graph, energy);

    EXPECT_GT (attackEnergy, 0.0f) << "The note's attack is silent";
    EXPECT_GT (energy[24], attackEnergy * 0.1f) << "The note died within its attack";

    // Same tail-window rationale as ElectricPianoSustainsAtDeclaredDefaults:
    // the per-partial decay leaves the fundamental-only body at roughly a
    // tenth of the full-attack energy by 0.3-0.56 s, far above the ~1e-12 a
    // collapsed click leaves behind.
    float sustainedEnergy = 0.0f;
    for (int block = 48; block < static_cast<int> (energy.size()); ++block)
        sustainedEnergy += energy[static_cast<size_t> (block)];

    sustainedEnergy /= static_cast<float> (energy.size() - 48);

    EXPECT_GT (sustainedEnergy, attackEnergy * 0.05f) << "The held note died instead of sustaining";
}

TEST_F (YdspExamplePatchTests, FMBellCompilesAndRenders)
{
    testPatch ("FMBell.ydsp");
}

TEST_F (YdspExamplePatchTests, FormantsCompilesAndRenders)
{
    testPatch ("Formants.ydsp");
}

TEST_F (YdspExamplePatchTests, HaasWidenerCompilesAndRenders)
{
    testPatch ("HaasWidener.ydsp");
}

TEST_F (YdspExamplePatchTests, PolySineCompilesAndRenders)
{
    testPatch ("PolySine.ydsp");
}

TEST_F (YdspExamplePatchTests, PulseBassCompilesAndRenders)
{
    testPatch ("PulseBass.ydsp");
}

TEST_F (YdspExamplePatchTests, WaveLabCompilesAndRenders)
{
    testPatch ("WaveLab.ydsp");
}

TEST_F (YdspExamplePatchTests, WobbleLeadCompilesAndRenders)
{
    testPatch ("WobbleLead.ydsp");
}

TEST_F (YdspExamplePatchTests, PerVoiceEchoCompilesAndRenders)
{
    testPatch ("PerVoiceEcho.ydsp");
}

//==============================================================================
// Minimal probe for the ElectricPiano regression: does a voice-bank noteOn
// handler actually read its processor's `input value` parameter? EPVoice reads
// all six envelope params (decayRate etc.) only inside noteOn, so a broken
// event-handler param load collapses every note to the same silent click and
// no host setParameter can fix it (both scenarios were bit-identical above).

TEST (YdspParamProbeTests, VoiceBankNoteOnReadsItsValueParam)
{
    YdspCompiler compiler;

    auto result = compiler.compile (R"YDSP(
        processor ParamVoice
        {
            output stream out;
            input value float level = 1.0;
            input event midi;
            state float heldLevel;
            event midi (e: noteOn) { heldLevel = level; }
            process { out = heldLevel; }
        }
        graph Probe
        {
            input event midi;
            output stream y;
            node v = ParamVoice[4] [[ mode: poly, stealing: oldest ]];
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP");

    ASSERT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    auto graph = std::move (result).getValue();

    constexpr int blockSize = 128;
    graph.prepare (44100.0, blockSize);

    std::vector<float> output (blockSize, 0.0f);

    YdspOutputBuffer outputs[] = {
        Span<float> (output.data(), output.size())
    };

    MidiBuffer midi;
    midi.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);

    for (int block = 0; block < 4; ++block)
    {
        graph.process ({},
                       Span<YdspOutputBuffer> (outputs, 1),
                       blockSize,
                       &midi,
                       nullptr,
                       0);

        midi.clear();

        float energy = 0.0f;
        for (int i = 0; i < blockSize; ++i)
            energy += output[static_cast<size_t> (i)] * output[static_cast<size_t> (i)];

        // heldLevel = level = 1.0 makes out a constant 1.0 -> energy ~= 128.
        EXPECT_GT (energy, 10.0f) << "noteOn handler did not read its level param (block " << block << ")";
    }
}

TEST (YdspParamProbeTests, VoiceBankNoteOnReadsAliasedValueParam)
{
    // Same probe, but with the param driven through a graph `input value`
    // endpoint and an explicit `level -> v.level` wire - the shape ElectricPiano
    // uses for all six EPVoice params (brightness -> voices.brightness, etc.).

    YdspCompiler compiler;

    auto result = compiler.compile (R"YDSP(
        processor ParamVoice
        {
            output stream out;
            input value float level = 1.0;
            input event midi;
            state float heldLevel;
            event midi (e: noteOn) { heldLevel = level; }
            process { out = heldLevel; }
        }
        graph ProbeAliased
        {
            input event midi;
            input value float level = 1.0;
            output stream y;
            node v = ParamVoice[4] [[ mode: poly, stealing: oldest ]];
            connection { midi -> v.midi; level -> v.level; v.out -> y; }
        }
    )YDSP");

    ASSERT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    auto graph = std::move (result).getValue();

    constexpr int blockSize = 128;
    graph.prepare (44100.0, blockSize);

    std::vector<float> output (blockSize, 0.0f);

    YdspOutputBuffer outputs[] = {
        Span<float> (output.data(), output.size())
    };

    MidiBuffer midi;
    midi.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);

    for (int block = 0; block < 4; ++block)
    {
        graph.process ({},
                       Span<YdspOutputBuffer> (outputs, 1),
                       blockSize,
                       &midi,
                       nullptr,
                       0);

        midi.clear();

        float energy = 0.0f;
        for (int i = 0; i < blockSize; ++i)
            energy += output[static_cast<size_t> (i)] * output[static_cast<size_t> (i)];

        // heldLevel = level = 1.0 makes out a constant 1.0 -> energy ~= 128.
        EXPECT_GT (energy, 10.0f) << "noteOn handler did not read its aliased level param (block " << block << ")";
    }
}

TEST (YdspParamProbeTests, VoiceBankNoteOnHonorsUpdatedValueParam)
{
    // Control for the two probes above: they cannot tell a *dynamic* param
    // read from a compile-time fold of the declared default (both yield 1.0).
    // Here the aliased param is rewritten to 0.25 via setParameter before the
    // note: a dynamic read outputs 0.25 (energy ~= 8), while a fold of the
    // declared default keeps outputting 1.0 (energy ~= 128) regardless.

    YdspCompiler compiler;

    auto result = compiler.compile (R"YDSP(
        processor ParamVoice
        {
            output stream out;
            input value float level = 1.0;
            input event midi;
            state float heldLevel;
            event midi (e: noteOn) { heldLevel = level; }
            process { out = heldLevel; }
        }
        graph ProbeUpdated
        {
            input event midi;
            input value float level = 1.0;
            output stream y;
            node v = ParamVoice[4] [[ mode: poly, stealing: oldest ]];
            connection { midi -> v.midi; level -> v.level; v.out -> y; }
        }
    )YDSP");

    ASSERT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    auto graph = std::move (result).getValue();

    constexpr int blockSize = 128;
    graph.prepare (44100.0, blockSize);

    graph.setParameter ("level", 0.25f);

    std::vector<float> output (blockSize, 0.0f);

    YdspOutputBuffer outputs[] = {
        Span<float> (output.data(), output.size())
    };

    MidiBuffer midi;
    midi.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);

    for (int block = 0; block < 2; ++block)
    {
        graph.process ({},
                       Span<YdspOutputBuffer> (outputs, 1),
                       blockSize,
                       &midi,
                       nullptr,
                       0);

        midi.clear();

        float energy = 0.0f;
        for (int i = 0; i < blockSize; ++i)
            energy += output[static_cast<size_t> (i)] * output[static_cast<size_t> (i)];

        // out = heldLevel = 0.25 constantly -> energy = 128 * 0.0625 = 8.
        EXPECT_NEAR (8.0f, energy, 1.0f)
            << "noteOn ignored the setParameter update; param read is not dynamic (block " << block << ")";
    }
}

TEST (YdspParamProbeTests, VoiceBankNoteOnReadsEventFields)
{
    // The ElectricPiano noteOn reads e.velocity / e.pitch; the probes above
    // never touched an `e.*` field. A noteOn that stores the incoming velocity
    // into the held output pins down whether event-field delivery is intact:
    // velocity 0.8 -> out 0.8 constantly -> energy = 128 * 0.64 ~= 82.

    YdspCompiler compiler;

    auto result = compiler.compile (R"YDSP(
        processor ParamVoice
        {
            output stream out;
            input event midi;
            state float heldValue;
            event midi (e: noteOn) { heldValue = e.velocity; }
            process { out = heldValue; }
        }
        graph ProbeFields
        {
            input event midi;
            output stream y;
            node v = ParamVoice[4] [[ mode: poly, stealing: oldest ]];
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP");

    ASSERT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    auto graph = std::move (result).getValue();

    constexpr int blockSize = 128;
    graph.prepare (44100.0, blockSize);

    std::vector<float> output (blockSize, 0.0f);

    YdspOutputBuffer outputs[] = {
        Span<float> (output.data(), output.size())
    };

    MidiBuffer midi;
    midi.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);

    for (int block = 0; block < 2; ++block)
    {
        graph.process ({},
                       Span<YdspOutputBuffer> (outputs, 1),
                       blockSize,
                       &midi,
                       nullptr,
                       0);

        midi.clear();

        float energy = 0.0f;
        for (int i = 0; i < blockSize; ++i)
            energy += output[static_cast<size_t> (i)] * output[static_cast<size_t> (i)];

        // out = e.velocity = 0.8 constantly -> energy = 128 * 0.64 = 81.92.
        EXPECT_NEAR (81.92f, energy, 2.0f)
            << "noteOn misread e.velocity (block " << block << ")";
    }
}

TEST (YdspParamProbeTests, VoiceBankNoteOnReadsEventPitch)
{
    // Same idea for e.pitch: note 60 -> out 60.0 constantly -> energy = 128 * 3600.

    YdspCompiler compiler;

    auto result = compiler.compile (R"YDSP(
        processor ParamVoice
        {
            output stream out;
            input event midi;
            state float heldValue;
            event midi (e: noteOn) { heldValue = e.pitch; }
            process { out = heldValue; }
        }
        graph ProbePitch
        {
            input event midi;
            output stream y;
            node v = ParamVoice[4] [[ mode: poly, stealing: oldest ]];
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP");

    ASSERT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    auto graph = std::move (result).getValue();

    constexpr int blockSize = 128;
    graph.prepare (44100.0, blockSize);

    std::vector<float> output (blockSize, 0.0f);

    YdspOutputBuffer outputs[] = {
        Span<float> (output.data(), output.size())
    };

    MidiBuffer midi;
    midi.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);

    for (int block = 0; block < 2; ++block)
    {
        graph.process ({},
                       Span<YdspOutputBuffer> (outputs, 1),
                       blockSize,
                       &midi,
                       nullptr,
                       0);

        midi.clear();

        float energy = 0.0f;
        for (int i = 0; i < blockSize; ++i)
            energy += output[static_cast<size_t> (i)] * output[static_cast<size_t> (i)];

        // out = e.pitch = 60.0 constantly -> energy = 128 * 3600 = 460800.
        EXPECT_NEAR (460800.0f, energy, 1000.0f)
            << "noteOn misread e.pitch (block " << block << ")";
    }
}

} // namespace yup::test
