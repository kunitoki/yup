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
#include "yup_YdspAllocationCounter.h"

#include <cmath>
#include <vector>

using namespace yup;

namespace
{

//==============================================================================
constexpr const char* subgraphGainProcessor =
    "processor Gain { input stream in; output stream out; input parameter float g = 2.0; process { out = in * g; } }\n";

YdspAudioGraph subgraphCompile (StringRef source, YdspCompiler& compiler)
{
    auto result = compiler.compile (String (subgraphGainProcessor) + source);
    EXPECT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    if (! result.wasOk())
        return {};

    return std::move (result).getValue();
}

String subgraphCompileError (StringRef source)
{
    YdspCompiler compiler;
    auto result = compiler.compile (String (subgraphGainProcessor) + source);

    EXPECT_FALSE (result.wasOk());

    return compiler.getDiagnostics().toString();
}

std::vector<float> subgraphRun (YdspAudioGraph& graph, std::vector<float> input)
{
    std::vector<float> output (input.size(), 0.0f);

    std::vector<YdspInputBuffer> inputBuffers { YdspInputBuffer (Span<const float> (input.data(), input.size())) };
    std::vector<YdspOutputBuffer> outputBuffers { YdspOutputBuffer (Span<float> (output.data(), output.size())) };

    graph.process (yup::YdspProcessRequest { inputBuffers, outputBuffers, static_cast<int> (input.size()) });

    return output;
}

const std::vector<float> subgraphInput { 1.0f, 2.0f, -0.5f, 0.25f, 4.0f, 0.0f, -3.0f, 1.5f };

} // namespace

//==============================================================================

TEST (YdspSubgraphTests, InlinesASubgraphUnderTheParentNodeNamePrefix)
{
    YdspCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        graph Sub {
            input stream in;
            output stream out;
            node a = Gain (g = 3.0);
            connection { in -> a.in; a.out -> out; }
        }
        graph Main [[ main ]] {
            input stream x;
            output stream y;
            node s = Sub;
            connection { x -> s.in; s.out -> y; }
        }
    )YDSP",
                                  compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    EXPECT_TRUE (graph.hasParameter ("s.a.g"));
    EXPECT_FALSE (graph.hasParameter ("a.g"));
    EXPECT_NEAR (3.0f, graph.getParameter ("s.a.g"), 1e-6f);

    const auto output = subgraphRun (graph, subgraphInput);

    for (size_t i = 0; i < subgraphInput.size(); ++i)
        EXPECT_NEAR (subgraphInput[i] * 3.0f, output[i], 1e-5f);
}

TEST (YdspSubgraphTests, SingleGraphNeedsNoMainAnnotation)
{
    YdspCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        graph Only {
            input stream x;
            output stream y;
            node a = Gain;
            connection { x -> a.in; a.out -> y; }
        }
    )YDSP",
                                  compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto output = subgraphRun (graph, subgraphInput);

    for (size_t i = 0; i < subgraphInput.size(); ++i)
        EXPECT_NEAR (subgraphInput[i] * 2.0f, output[i], 1e-5f);
}

TEST (YdspSubgraphTests, RejectsSeveralGraphsWithNoMainAnnotation)
{
    const auto message = subgraphCompileError (R"YDSP(
        graph A { input stream x; output stream y; node a = Gain; connection { x -> a.in; a.out -> y; } }
        graph B { input stream x; output stream y; node b = Gain; connection { x -> b.in; b.out -> y; } }
    )YDSP");

    EXPECT_TRUE (message.contains ("none is annotated"));
    EXPECT_TRUE (message.contains ("'A'"));
    EXPECT_TRUE (message.contains ("'B'"));
}

TEST (YdspSubgraphTests, RejectsTwoMainAnnotations)
{
    const auto message = subgraphCompileError (R"YDSP(
        graph A [[ main ]] { input stream x; output stream y; node a = Gain; connection { x -> a.in; a.out -> y; } }
        graph B [[ main ]] { input stream x; output stream y; node b = Gain; connection { x -> b.in; b.out -> y; } }
    )YDSP");

    EXPECT_TRUE (message.contains ("Only one graph can be annotated"));
}

TEST (YdspSubgraphTests, NodeOverrideSetsTheInnerNodeDefault)
{
    YdspCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        graph Sub {
            input stream in;
            output stream out;
            input parameter float amount = 3.0;
            node a = Gain (g = 99.0);
            connection { in -> a.in; a.out -> out; amount -> a.g; }
        }
        graph Main [[ main ]] {
            input stream x;
            output stream y;
            node s = Sub (amount = 5.0);
            connection { x -> s.in; s.out -> y; }
        }
    )YDSP",
                                  compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    EXPECT_NEAR (5.0f, graph.getParameter ("s.a.g"), 1e-6f);

    const auto output = subgraphRun (graph, subgraphInput);

    for (size_t i = 0; i < subgraphInput.size(); ++i)
        EXPECT_NEAR (subgraphInput[i] * 5.0f, output[i], 1e-5f);
}

TEST (YdspSubgraphTests, InstantiatesTheSameSubgraphTwiceIndependently)
{
    YdspCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        graph Sub {
            input stream in;
            output stream out;
            input parameter float amount = 3.0;
            node a = Gain;
            connection { in -> a.in; a.out -> out; amount -> a.g; }
        }
        graph Main [[ main ]] {
            input stream x;
            output stream y;
            node s1 = Sub (amount = 2.0);
            node s2 = Sub (amount = 5.0);
            connection { x -> s1.in; s1.out -> s2.in; s2.out -> y; }
        }
    )YDSP",
                                  compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    EXPECT_NEAR (2.0f, graph.getParameter ("s1.a.g"), 1e-6f);
    EXPECT_NEAR (5.0f, graph.getParameter ("s2.a.g"), 1e-6f);

    const auto output = subgraphRun (graph, subgraphInput);

    for (size_t i = 0; i < subgraphInput.size(); ++i)
        EXPECT_NEAR (subgraphInput[i] * 10.0f, output[i], 1e-4f);
}

TEST (YdspSubgraphTests, ParentGraphParameterAliasesThroughASubgraph)
{
    YdspCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        graph Sub {
            input stream in;
            output stream out;
            input parameter float amount = 3.0;
            node a = Gain;
            connection { in -> a.in; a.out -> out; amount -> a.g; }
        }
        graph Main [[ main ]] {
            input stream x;
            output stream y;
            input parameter float master = 4.0 [[ name: "Master", min: 0.0, max: 10.0 ]];
            node s = Sub;
            connection { x -> s.in; s.out -> y; master -> s.amount; }
        }
    )YDSP",
                                  compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    ASSERT_TRUE (graph.hasParameter ("master"));
    ASSERT_TRUE (graph.hasParameter ("s.a.g"));
    EXPECT_EQ (graph.getParameterSlot ("master"), graph.getParameterSlot ("s.a.g"));
    EXPECT_NEAR (4.0f, graph.getParameter ("master"), 1e-6f);

    graph.setParameter ("master", 6.0f);

    const auto output = subgraphRun (graph, subgraphInput);

    for (size_t i = 0; i < subgraphInput.size(); ++i)
        EXPECT_NEAR (subgraphInput[i] * 6.0f, output[i], 1e-5f);
}

TEST (YdspSubgraphTests, SubgraphParameterDrivesSeveralInnerNodes)
{
    YdspCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        graph Sub {
            input stream in;
            output stream out;
            input parameter float amount = 1.0;
            node a = Gain;
            node b = Gain;
            connection { in -> a.in; a.out -> b.in; b.out -> out; amount -> a.g; amount -> b.g; }
        }
        graph Main [[ main ]] {
            input stream x;
            output stream y;
            input parameter float master = 2.0;
            node s = Sub;
            connection { x -> s.in; s.out -> y; master -> s.amount; }
        }
    )YDSP",
                                  compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    EXPECT_EQ (graph.getParameterSlot ("master"), graph.getParameterSlot ("s.a.g"));
    EXPECT_EQ (graph.getParameterSlot ("master"), graph.getParameterSlot ("s.b.g"));

    graph.setParameter ("master", 3.0f);

    const auto output = subgraphRun (graph, subgraphInput);

    for (size_t i = 0; i < subgraphInput.size(); ++i)
        EXPECT_NEAR (subgraphInput[i] * 9.0f, output[i], 1e-4f);
}

TEST (YdspSubgraphTests, MeterAliasesOutThroughASubgraph)
{
    YdspCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        processor Peak {
            input stream in;
            output stream out;
            output parameter float peak;
            process { out = in; peak = max (peak, abs (in)); }
        }
        graph Sub {
            input stream in;
            output stream out;
            output parameter float level;
            node p = Peak;
            connection { in -> p.in; p.out -> out; p.peak -> level; }
        }
        graph Main [[ main ]] {
            input stream x;
            output stream y;
            output parameter float meter;
            node s = Sub;
            connection { x -> s.in; s.out -> y; s.level -> meter; }
        }
    )YDSP",
                                  compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    subgraphRun (graph, subgraphInput);

    EXPECT_NEAR (4.0f, graph.getOutputValue ("meter"), 1e-5f);
}

TEST (YdspSubgraphTests, NestsTwoLevelsDeep)
{
    YdspCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        graph Inner {
            input stream in;
            output stream out;
            node a = Gain (g = 3.0);
            connection { in -> a.in; a.out -> out; }
        }
        graph Middle {
            input stream in;
            output stream out;
            node i = Inner;
            node b = Gain (g = 5.0);
            connection { in -> i.in; i.out -> b.in; b.out -> out; }
        }
        graph Main [[ main ]] {
            input stream x;
            output stream y;
            node m = Middle;
            connection { x -> m.in; m.out -> y; }
        }
    )YDSP",
                                  compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    EXPECT_TRUE (graph.hasParameter ("m.i.a.g"));
    EXPECT_TRUE (graph.hasParameter ("m.b.g"));

    const auto output = subgraphRun (graph, subgraphInput);

    for (size_t i = 0; i < subgraphInput.size(); ++i)
        EXPECT_NEAR (subgraphInput[i] * 15.0f, output[i], 1e-4f);
}

TEST (YdspSubgraphTests, AccumulatesInlineDelaysAcrossTheBoundary)
{
    YdspCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        graph Sub {
            input stream in;
            output stream out;
            node a = Gain (g = 1.0);
            connection { in -> [3] -> a.in; a.out -> out; }
        }
        graph Main [[ main ]] {
            input stream x;
            output stream y;
            node s = Sub;
            connection { x -> [2] -> s.in; s.out -> y; }
        }
    )YDSP",
                                  compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto output = subgraphRun (graph, subgraphInput);

    for (size_t i = 0; i < subgraphInput.size(); ++i)
        EXPECT_NEAR (i >= 5 ? subgraphInput[i - 5] : 0.0f, output[i], 1e-5f);
}

TEST (YdspSubgraphTests, ResolvesAGraphLeafInTheAlgebraForm)
{
    YdspCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        graph Sub {
            input stream in;
            output stream out;
            node a = Gain (g = 3.0);
            connection { in -> a.in; a.out -> out; }
        }
        graph Main [[ main ]] {
            input stream x;
            output stream y;
            node s = Sub;
            node d = Gain (g = 5.0);
            process = x : s : d : y;
        }
    )YDSP",
                                  compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    EXPECT_TRUE (graph.hasParameter ("s.a.g"));

    const auto output = subgraphRun (graph, subgraphInput);

    for (size_t i = 0; i < subgraphInput.size(); ++i)
        EXPECT_NEAR (subgraphInput[i] * 15.0f, output[i], 1e-4f);
}

TEST (YdspSubgraphTests, ReportsAGraphInstantiationCycle)
{
    const auto message = subgraphCompileError (R"YDSP(
        graph A [[ main ]] { input stream x; output stream y; node b = B; connection { x -> b.in; b.out -> y; } }
        graph B { input stream in; output stream out; node a = A; connection { in -> a.x; a.y -> out; } }
    )YDSP");

    EXPECT_TRUE (message.contains ("instantiates itself"));
    EXPECT_TRUE (message.contains ("A -> B -> A"));
}

TEST (YdspSubgraphTests, AllowsAVoiceBankOfAGraphWithAnEventInput)
{
    YdspCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        processor PitchVoice {
            output stream out;
            input event midi;
            state float s;
            event midi (e: noteOn) { s = e.pitch; }
            process { out = s; }
        }
        graph VoiceChain {
            input event midi;
            output stream out;
            node osc = PitchVoice;
            node gain = Gain (g = 2.0);
            connection { midi -> osc.midi; osc.out -> gain.in; gain.out -> out; }
        }
        graph Main [[ main ]] {
            input event midi;
            output stream y;
            node v = VoiceChain[4];
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP",
                                  compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    EXPECT_TRUE (graph.hasParameter ("v.gain.g"));
}

TEST (YdspSubgraphTests, RejectsAVoiceBankOfAGraphWithoutAnEventInput)
{
    const auto message = subgraphCompileError (R"YDSP(
        graph Sub { input stream in; output stream out; node a = Gain; connection { in -> a.in; a.out -> out; } }
        graph Main [[ main ]] { input stream x; output stream y; node s = Sub[4]; connection { x -> s.in; s.out -> y; } }
    )YDSP");

    EXPECT_TRUE (message.contains ("voice bank"));
    EXPECT_TRUE (message.contains ("must declare an input event"));
}

TEST (YdspSubgraphTests, RejectsMonoModeOnAVoiceBankOfAGraph)
{
    const auto message = subgraphCompileError (R"YDSP(
        processor PitchVoice {
            output stream out;
            input event midi;
            state float s;
            event midi (e: noteOn) { s = e.pitch; }
            process { out = s; }
        }
        graph VoiceChain {
            input event midi;
            output stream out;
            node osc = PitchVoice;
            node gain = Gain (g = 2.0);
            connection { midi -> osc.midi; osc.out -> gain.in; gain.out -> out; }
        }
        graph Main [[ main ]] {
            input event midi;
            output stream y;
            node v = VoiceChain[4] [[ mode: mono ]];
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP");

    EXPECT_TRUE (message.contains ("'mode: mono'"));
}

TEST (YdspSubgraphTests, RejectsNestedVoiceBanks)
{
    const auto message = subgraphCompileError (R"YDSP(
        processor PitchVoice {
            output stream out;
            input event midi;
            state float s;
            event midi (e: noteOn) { s = e.pitch; }
            process { out = s; }
        }
        graph Inner {
            input event midi;
            output stream out;
            node osc = PitchVoice;
            node gain = Gain (g = 2.0);
            connection { midi -> osc.midi; osc.out -> gain.in; gain.out -> out; }
        }
        graph Outer {
            input event midi;
            output stream out;
            node osc = PitchVoice;
            node inner = Inner[4];
            node tail = Gain;
            connection { midi -> osc.midi; midi -> inner.midi; osc.out -> tail.in; inner.out -> tail.in; tail.out -> out; }
        }
        graph Main [[ main ]] {
            input event midi;
            output stream y;
            node v = Outer[2];
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP");

    EXPECT_TRUE (message.contains ("nested voice banks"));
}

TEST (YdspSubgraphTests, RejectsOversamplingAGraph)
{
    const auto message = subgraphCompileError (R"YDSP(
        graph Sub { input stream in; output stream out; node a = Gain; connection { in -> a.in; a.out -> out; } }
        graph Main [[ main ]] { input stream x; output stream y; node s = Sub * 4; connection { x -> s.in; s.out -> y; } }
    )YDSP");

    EXPECT_TRUE (message.contains ("oversampling"));
}

TEST (YdspSubgraphTests, RejectsAGraphWithAnEventInputUsedAsANode)
{
    const auto message = subgraphCompileError (R"YDSP(
        processor Tap { input event midi; process { } }
        graph Sub { input event midi; input stream in; output stream out; node a = Gain; node t = Tap; connection { in -> a.in; a.out -> out; midi -> t.midi; } }
        graph Main [[ main ]] { input stream x; output stream y; node s = Sub; connection { x -> s.in; s.out -> y; } }
    )YDSP");

    EXPECT_TRUE (message.contains ("event input"));
    EXPECT_TRUE (message.contains ("main graph"));
}

TEST (YdspSubgraphTests, ImportsAGraphUnderItsAlias)
{
    const auto tempDir = File::getSpecialLocation (File::tempDirectory)
                             .getChildFile ("yup_ydsp_subgraph_test");

    tempDir.deleteRecursively();
    tempDir.getChildFile ("fx").createDirectory();

    tempDir.getChildFile ("fx/Boost.ydsp")
        .replaceWithText ("processor Boost { input stream in; output stream out; input parameter float g = 2.0; process { out = in * g; } }\n");

    tempDir.getChildFile ("fx/Chain.ydsp")
        .replaceWithText (R"YDSP(
            import Boost as b;
            graph Chain [[ main ]] {
                input stream in;
                output stream out;
                input parameter float amount = 3.0;
                node first = b.Boost;
                node second = b.Boost (g = 2.0);
                connection { in -> first.in; first.out -> second.in; second.out -> out; amount -> first.g; }
            }
        )YDSP");

    const auto patch = R"YDSP(
        import fx.Chain as fx;

        graph Patch {
            input stream x;
            output stream y;
            node master = fx.Chain (amount = 4.0);
            connection { x -> master.in; master.out -> y; }
        }
    )YDSP";

    YdspCompiler compiler;
    auto result = compiler.compile (patch, tempDir.getChildFile ("Patch.ydsp").getFullPathName());
    ASSERT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    auto graph = std::move (result).getValue();
    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    EXPECT_TRUE (graph.hasParameter ("master.first.g"));
    EXPECT_TRUE (graph.hasParameter ("master.second.g"));
    EXPECT_NEAR (4.0f, graph.getParameter ("master.first.g"), 1e-6f);

    const auto output = subgraphRun (graph, subgraphInput);

    for (size_t i = 0; i < subgraphInput.size(); ++i)
        EXPECT_NEAR (subgraphInput[i] * 8.0f, output[i], 1e-4f);

    tempDir.deleteRecursively();
}

//==============================================================================

TEST (YdspSubgraphTests, SplicesFanOutAndFanInInsideASubgraph)
{
    YdspCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        graph Sub {
            input stream in;
            output stream out;
            node dry = Gain (g = 2.0);
            node wet = Gain (g = 5.0);
            connection { in -> dry.in; in -> wet.in; dry.out -> out; wet.out -> out; }
        }
        graph Main [[ main ]] {
            input stream x;
            output stream y;
            node s = Sub;
            connection { x -> s.in; s.out -> y; }
        }
    )YDSP",
                                  compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto output = subgraphRun (graph, subgraphInput);

    for (size_t i = 0; i < subgraphInput.size(); ++i)
        EXPECT_NEAR (subgraphInput[i] * 7.0f, output[i], 1e-5f) << "sample " << i;
}

TEST (YdspSubgraphTests, SplicesSeveralParentSourcesIntoASubgraphInput)
{
    YdspCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        graph Sub {
            input stream in;
            output stream out;
            node a = Gain (g = 1.0);
            connection { in -> a.in; a.out -> out; }
        }
        graph Main [[ main ]] {
            input stream x;
            output stream y;
            node p = Gain (g = 2.0);
            node q = Gain (g = 3.0);
            node s = Sub;
            connection { x -> p.in; x -> q.in; p.out -> s.in; q.out -> s.in; s.out -> y; }
        }
    )YDSP",
                                  compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto output = subgraphRun (graph, subgraphInput);

    for (size_t i = 0; i < subgraphInput.size(); ++i)
        EXPECT_NEAR (subgraphInput[i] * 5.0f, output[i], 1e-5f) << "sample " << i;
}

TEST (YdspSubgraphTests, SplicesASubgraphOutputReadFromSeveralPlaces)
{
    YdspCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        graph Sub {
            input stream in;
            output stream out;
            node a = Gain (g = 2.0);
            connection { in -> a.in; a.out -> out; }
        }
        graph Main [[ main ]] {
            input stream x;
            output stream y;
            node p = Gain (g = 1.0);
            node q = Gain (g = 4.0);
            node s = Sub;
            connection { x -> s.in; s.out -> p.in; s.out -> q.in; p.out -> y; q.out -> y; }
        }
    )YDSP",
                                  compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto output = subgraphRun (graph, subgraphInput);

    for (size_t i = 0; i < subgraphInput.size(); ++i)
        EXPECT_NEAR (subgraphInput[i] * 10.0f, output[i], 1e-4f) << "sample " << i;
}

TEST (YdspSubgraphTests, SplicesAPassThroughSubgraphFannedOnBothSides)
{
    YdspCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        graph Wire {
            input stream in;
            output stream out;
            connection { in -> out; }
        }
        graph Main [[ main ]] {
            input stream x;
            output stream y;
            node p = Gain (g = 1.0);
            node q = Gain (g = 2.0);
            node r = Gain (g = 4.0);
            node t = Gain (g = 8.0);
            node w = Wire;
            connection {
                x -> p.in;
                x -> q.in;
                p.out -> w.in;
                q.out -> w.in;
                w.out -> r.in;
                w.out -> t.in;
                r.out -> y;
                t.out -> y;
            }
        }
    )YDSP",
                                  compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto output = subgraphRun (graph, subgraphInput);

    for (size_t i = 0; i < subgraphInput.size(); ++i)
        EXPECT_NEAR (subgraphInput[i] * 36.0f, output[i], 1e-3f) << "sample " << i;
}

TEST (YdspSubgraphTests, AccumulatesInlineDelaysAcrossAFannedBoundary)
{
    YdspCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        graph Sub {
            input stream in;
            output stream out;
            node a = Gain (g = 1.0);
            connection { in -> [3] -> a.in; a.out -> out; }
        }
        graph Main [[ main ]] {
            input stream x;
            output stream y;
            node p = Gain (g = 1.0);
            node q = Gain (g = 10.0);
            node s = Sub;
            connection {
                x -> p.in;
                x -> q.in;
                p.out -> [2] -> s.in;
                q.out -> [5] -> s.in;
                s.out -> y;
            }
        }
    )YDSP",
                                  compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 16);

    const std::vector<float> impulse { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    const auto output = subgraphRun (graph, impulse);

    for (size_t i = 0; i < impulse.size(); ++i)
    {
        const auto expected = i == 5 ? 1.0f : (i == 8 ? 10.0f : 0.0f);
        EXPECT_NEAR (expected, output[i], 1e-5f) << "sample " << i;
    }
}

//==============================================================================
// Event edges across a subgraph boundary
//
// There is no dedicated test here for an event wire with both ends fully
// inside a subgraph that is itself used as a node ("no boundary crossing"):
// it cannot be expressed in valid YDSP. Any processor declaring `input event
// X` unconditionally requires its *direct* containing graph to also declare
// a matching `input event X` (yup_YdspSemanticGraph.cpp, the node-event-input
// resolution loop guarded by `if (processor != nullptr)`), and separately,
// any graph that declares an `input event` is unconditionally rejected the
// moment it is used as a node (the same pre-existing check this task's brief
// says not to touch). A subgraph housing both ends of an internal event wire
// would need the first rule to force it into declaring an `input event`, and
// the second rule then forbids that same subgraph from ever being
// instantiated as a node - so the scenario is unreachable through the
// compiler's public surface. The splice code's `edge.dstNode >= 0` branch
// (copy + remap both ends by `+base`) is exercised structurally by the same
// mechanism the streams/meters splices already rely on, but has no reachable
// integration-test surface of its own given these two independent rules.

TEST (YdspSubgraphTests, RoutesASubgraphsOutputEventOntoAParentNodeAfterCompaction)
{
    YdspCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        processor Arp {
            input stream trig;
            output event noteOn;
            process { if (trig > 0.5) { emit noteOn (pitch: 72, velocity: 0.8) -> noteOn; } }
        }
        processor Voice {
            input event midi;
            output stream out;
            state float sounding;
            event midi (e: noteOn) { sounding = e.pitch; }
            process { out = sounding; }
        }
        processor Silence {
            output stream out;
            process { out = 0.0; }
        }
        graph Sub {
            input stream trig;
            output event noteOn;
            node arp = Arp;
            connection { trig -> arp.trig; arp.noteOn -> noteOn; }
        }
        graph Main [[ main ]] {
            input stream trig;
            output stream y;
            node pad1 = Silence;
            node pad2 = Silence;
            node s = Sub;
            node pad3 = Silence;
            node voice = Voice;
            connection {
                trig -> s.trig;
                s.noteOn -> voice.midi;
                pad1.out -> y;
                pad2.out -> y;
                pad3.out -> y;
                voice.out -> y;
            }
        }
    )YDSP",
                                  compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto output = subgraphRun (graph, subgraphInput);

    for (size_t i = 0; i < subgraphInput.size(); ++i)
        EXPECT_NEAR (72.0f, output[i], 1e-5f) << "sample " << i;
}

TEST (YdspSubgraphTests, FansASubgraphsOutputEventToTwoParentDestinations)
{
    YdspCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        processor Arp {
            input stream trig;
            output event noteOn;
            process { if (trig > 0.5) { emit noteOn (pitch: 72, velocity: 0.8) -> noteOn; } }
        }
        processor Voice {
            input event midi;
            output stream out;
            state float sounding;
            event midi (e: noteOn) { sounding = e.pitch; }
            process { out = sounding; }
        }
        graph Sub {
            input stream trig;
            output event noteOn;
            node arp = Arp;
            connection { trig -> arp.trig; arp.noteOn -> noteOn; }
        }
        graph Main [[ main ]] {
            input stream trig;
            output stream y;
            node s = Sub;
            node voiceA = Voice;
            node voiceB = Voice;
            connection {
                trig -> s.trig;
                s.noteOn -> voiceA.midi;
                s.noteOn -> voiceB.midi;
                voiceA.out -> y;
                voiceB.out -> y;
            }
        }
    )YDSP",
                                  compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto output = subgraphRun (graph, subgraphInput);

    for (size_t i = 0; i < subgraphInput.size(); ++i)
        EXPECT_NEAR (144.0f, output[i], 1e-4f) << "sample " << i;
}

//==============================================================================

namespace
{

YdspCompileOptions crossVoiceBaselineOptions()
{
    YdspCompileOptions options;
    options.optimizationTier = YdspOptimizationTier::baseline;
    options.targetPolicy = YdspTargetPolicy::baseline;
    options.baselineTarget = YdspNativeTarget::scalar;
    return options;
}

YdspCompileOptions crossVoiceSimdOptions()
{
    YdspCompileOptions options;
    options.optimizationTier = YdspOptimizationTier::automatic;
    options.targetPolicy = YdspTargetPolicy::host;
    return options;
}

void runMidiStereoBlock (YdspAudioGraph& graph,
                         const MidiBuffer* midi,
                         std::vector<float>& outL,
                         std::vector<float>& outR,
                         int blockSize)
{
    outL.assign (static_cast<size_t> (blockSize), 0.0f);
    outR.assign (static_cast<size_t> (blockSize), 0.0f);

    YdspOutputBuffer outputs[] {
        Span<float> (outL.data(), outL.size()),
        Span<float> (outR.data(), outR.size())
    };

    const MidiBuffer* eventInputs[] { midi };

    ASSERT_EQ (YdspProcessResult::ok,
               graph.process (YdspProcessRequest {
                   {},
                   outputs,
                   blockSize,
                   Span<const MidiBuffer*> (eventInputs, midi != nullptr ? 1u : 0u),
                   {}
               }));
}

void runMidiMonoBlock (YdspAudioGraph& graph,
                       const MidiBuffer* midi,
                       std::vector<float>& out,
                       int blockSize)
{
    out.assign (static_cast<size_t> (blockSize), 0.0f);

    YdspOutputBuffer outputs[] { Span<float> (out.data(), out.size()) };

    const MidiBuffer* eventInputs[] { midi };

    ASSERT_EQ (YdspProcessResult::ok,
               graph.process (YdspProcessRequest {
                   {},
                   outputs,
                   blockSize,
                   Span<const MidiBuffer*> (eventInputs, midi != nullptr ? 1u : 0u),
                   {}
               }));
}

void runInputMidiMonoBlock (YdspAudioGraph& graph,
                            const std::vector<float>& in,
                            const MidiBuffer* midi,
                            std::vector<float>& out,
                            int blockSize)
{
    out.assign (static_cast<size_t> (blockSize), 0.0f);

    const YdspInputBuffer inputs[] { Span<const float> (in.data(), static_cast<size_t> (blockSize)) };
    YdspOutputBuffer outputs[] { Span<float> (out.data(), static_cast<size_t> (blockSize)) };
    const MidiBuffer* eventInputs[] { midi };

    ASSERT_EQ (YdspProcessResult::ok,
               graph.process (YdspProcessRequest {
                   inputs,
                   outputs,
                   blockSize,
                   Span<const MidiBuffer*> (eventInputs, midi != nullptr ? 1u : 0u),
                   {}
               }));
}

} // namespace

TEST (YdspSubgraphTests, CrossVoiceSimdPolySynthMatchesScalarBaselineAcrossVoiceCountsAndEvents)
{
    static constexpr const char* polySynthSource = R"YDSP(
        processor PolySynthVoice {
            output stream out;

            input parameter float gain = 0.25;
            input parameter float cutoff = 0.35;
            input event midi;

            state float phase;
            state float inc;
            state float env;
            state float decayMul = 0.9995;
            state float filterState;
            state float pan = 0.5;
            state int released;
            state int active [[ role: voiceActivity ]];

            event midi (e: noteOn) {
                let freq = 440.0 * pow (2.0, (e.pitch - 69.0) / 12.0);
                inc = freq * samplePeriod;
                phase = 0.0;
                env = clamp (e.velocity, 0.05, 1.0);
                decayMul = 0.9995;
                filterState = 0.0;
                pan = clamp ((e.pitch - 48.0) / 36.0, 0.1, 0.9);
                released = 0;
                active = 1;
            }

            event midi (e: noteOff) {
                released = 1;
                decayMul = 0.985;
            }

            process {
                phase = phase + inc;
                if (phase >= 1.0) { phase = phase - 1.0; }

                let saw = 2.0 * phase - 1.0;
                let shaped = tanh (saw * 1.5);
                filterState = filterState + cutoff * (shaped - filterState);

                env = env * decayMul;
                let voiceOut = filterState * env * gain;

                active = select (env < 0.0001, select (released > 0, 0, 1), 1);

                out = voiceOut * (0.5 + 0.5 * pan);
            }
        }

        graph PolySynth [[ main ]] {
            input event midi;
            output stream out;

            node voices = PolySynthVoice[16] [[ mode: poly, stealing: oldest ]];

            connection {
                midi -> voices.midi;
                voices.out -> out;
            }
        }
    )YDSP";

    YdspCompiler baselineCompiler;
    auto baselineGraph = yup::test::patches::compilePatch (polySynthSource, baselineCompiler, crossVoiceBaselineOptions());
    ASSERT_TRUE (baselineGraph.isValid());

    YdspCompiler simdCompiler;
    auto simdGraph = yup::test::patches::compilePatch (polySynthSource, simdCompiler, crossVoiceSimdOptions());
    ASSERT_TRUE (simdGraph.isValid());

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 64;
    ASSERT_TRUE (baselineGraph.prepare (sampleRate, blockSize).wasOk());
    ASSERT_TRUE (simdGraph.prepare (sampleRate, blockSize).wasOk());

    std::vector<float> baseOut, simdOut;
    MidiBuffer midi;

    auto verifyBlock = [&] (const char* label)
    {
        runMidiMonoBlock (baselineGraph, &midi, baseOut, blockSize);
        runMidiMonoBlock (simdGraph, &midi, simdOut, blockSize);
        midi.clear();

        EXPECT_EQ (baselineGraph.getActiveVoiceCount ("voices"), simdGraph.getActiveVoiceCount ("voices")) << label;

        for (int i = 0; i < blockSize; ++i)
        {
            const auto idx = static_cast<size_t> (i);
            EXPECT_NEAR (baseOut[idx], simdOut[idx], 1e-4f) << label << " sample " << i;
        }
    };

    // Block 1: 1 voice triggered (scalar <3 fallback).
    midi.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);
    verifyBlock ("1 voice noteOn");

    // Block 2: 2 voices triggered -> 2 sustaining (<3 fallback).
    midi.addEvent (MidiMessage::noteOn (1, 64, 0.7f), 16);
    verifyBlock ("2 voices noteOn");
    verifyBlock ("2 voices steady");

    // Block 3: +4 voices mid-block (mixed sustaining + mid-block split voices).
    midi.addEvent (MidiMessage::noteOn (1, 67, 0.9f), 0);
    midi.addEvent (MidiMessage::noteOn (1, 71, 0.6f), 12);
    midi.addEvent (MidiMessage::noteOn (1, 72, 0.85f), 27);
    midi.addEvent (MidiMessage::noteOn (1, 76, 0.75f), 45);
    verifyBlock ("6 voices mid-block noteOn");

    // Block 4: 6 voices sustaining cleanly in SoA SIMD batch (with zeroed padding lanes).
    verifyBlock ("6 voices steady batch");

#if YUP_ENABLE_ALLOCATION_HOOKS
    {
        yup::test::YdspAllocationCounter allocations;
        allocations.start();
        runMidiMonoBlock (simdGraph, &midi, simdOut, blockSize);
        EXPECT_EQ (0u, allocations.stop());

        // Advance baseline to stay in lockstep after the allocation check block.
        runMidiMonoBlock (baselineGraph, &midi, baseOut, blockSize);
        for (int i = 0; i < blockSize; ++i)
            EXPECT_NEAR (baseOut[static_cast<size_t> (i)], simdOut[static_cast<size_t> (i)], 1e-4f);
    }
#endif

    // Block 5: 8 voices total (full 8-lane AVX2 or 2x4-lane ASIMD/SSE2 batch).
    midi.addEvent (MidiMessage::noteOn (1, 55, 0.9f), 8);
    midi.addEvent (MidiMessage::noteOn (1, 59, 0.65f), 32);
    verifyBlock ("8 voices mid-block noteOn");
    verifyBlock ("8 voices steady batch");

    // Block 6: release 3 notes mid-block, leaving 5 held + 3 releasing, then let released tails decay to sleep.
    midi.addEvent (MidiMessage::noteOff (1, 60, 0.5f), 5);
    midi.addEvent (MidiMessage::noteOff (1, 64, 0.5f), 20);
    midi.addEvent (MidiMessage::noteOff (1, 67, 0.5f), 40);
    verifyBlock ("mid-block noteOff");

    for (int block = 0; block < 14; ++block)
        verifyBlock ("release decay");

    EXPECT_EQ (5, simdGraph.getActiveVoiceCount ("voices"));
}

TEST (YdspSubgraphTests, CrossVoiceSimdWithSharedInputAndNonZeroBiasMatchesBaseline)
{
    static constexpr const char* sharedInputSource = R"YDSP(
        processor ModVoice {
            input stream in;
            output stream out;
            input event midi;

            state float gain = 0.0;
            state float z1 = 0.0;
            state float bias = 0.0;
            state int active [[ role: voiceActivity ]];

            event midi (e: noteOn) {
                gain = 0.1 + 0.4 * e.velocity;
                bias = 0.02;
                active = 1;
            }

            event midi (e: noteOff) {
                gain = 0.0;
                bias = 0.0;
                active = 0;
            }

            process {
                z1 = z1 + 0.25 * (in * gain - z1);
                out = z1 + bias;
            }
        }

        graph Main [[ main ]] {
            input stream x;
            input event midi;
            output stream y;

            node voices = ModVoice[8] [[ mode: poly, stealing: oldest ]];

            connection {
                x -> voices.in;
                midi -> voices.midi;
                voices.out -> y;
            }
        }
    )YDSP";

    YdspCompiler baselineCompiler;
    auto baselineGraph = yup::test::patches::compilePatch (sharedInputSource, baselineCompiler, crossVoiceBaselineOptions());
    ASSERT_TRUE (baselineGraph.isValid());

    YdspCompiler simdCompiler;
    auto simdGraph = yup::test::patches::compilePatch (sharedInputSource, simdCompiler, crossVoiceSimdOptions());
    ASSERT_TRUE (simdGraph.isValid());

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 32;
    ASSERT_TRUE (baselineGraph.prepare (sampleRate, blockSize).wasOk());
    ASSERT_TRUE (simdGraph.prepare (sampleRate, blockSize).wasOk());

    const auto input = yup::test::patches::makeRamp (blockSize, -0.5f);
    std::vector<float> baseOut, simdOut;
    MidiBuffer midi;

    // Trigger 3 voices first (exercises 1 padding lane on V=4, 5 padding lanes on V=8).
    midi.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);
    midi.addEvent (MidiMessage::noteOn (1, 64, 0.6f), 0);
    midi.addEvent (MidiMessage::noteOn (1, 67, 0.9f), 0);

    runInputMidiMonoBlock (baselineGraph, input, &midi, baseOut, blockSize);
    runInputMidiMonoBlock (simdGraph, input, &midi, simdOut, blockSize);
    midi.clear();

    for (int i = 0; i < blockSize; ++i)
        EXPECT_NEAR (baseOut[static_cast<size_t> (i)], simdOut[static_cast<size_t> (i)], 1e-4f) << "3 voices noteOn sample " << i;

#if YUP_ENABLE_ALLOCATION_HOOKS
    {
        yup::test::YdspAllocationCounter allocations;
        allocations.start();
        runInputMidiMonoBlock (simdGraph, input, &midi, simdOut, blockSize);
        EXPECT_EQ (0u, allocations.stop());
    }
    runInputMidiMonoBlock (baselineGraph, input, &midi, baseOut, blockSize);
#else
    runInputMidiMonoBlock (baselineGraph, input, &midi, baseOut, blockSize);
    runInputMidiMonoBlock (simdGraph, input, &midi, simdOut, blockSize);
#endif

    for (int i = 0; i < blockSize; ++i)
        EXPECT_NEAR (baseOut[static_cast<size_t> (i)], simdOut[static_cast<size_t> (i)], 1e-4f) << "3 voices steady sample " << i;

    // Trigger 2 more voices (5 voices total).
    midi.addEvent (MidiMessage::noteOn (1, 71, 0.5f), 4);
    midi.addEvent (MidiMessage::noteOn (1, 74, 0.7f), 12);

    runInputMidiMonoBlock (baselineGraph, input, &midi, baseOut, blockSize);
    runInputMidiMonoBlock (simdGraph, input, &midi, simdOut, blockSize);
    midi.clear();

    runInputMidiMonoBlock (baselineGraph, input, &midi, baseOut, blockSize);
    runInputMidiMonoBlock (simdGraph, input, &midi, simdOut, blockSize);

    for (int i = 0; i < blockSize; ++i)
        EXPECT_NEAR (baseOut[static_cast<size_t> (i)], simdOut[static_cast<size_t> (i)], 1e-4f) << "5 voices steady sample " << i;
}

TEST (YdspSubgraphTests, CrossVoiceSimdElectricPianoMatchesBaselineAndDoesNotAllocate)
{
    YdspCompiler baselineCompiler;
    auto baselineGraph = yup::test::patches::compilePatch (yup::test::patches::electricPiano, baselineCompiler, crossVoiceBaselineOptions());
    ASSERT_TRUE (baselineGraph.isValid());

    YdspCompiler simdCompiler;
    auto simdGraph = yup::test::patches::compilePatch (yup::test::patches::electricPiano, simdCompiler, crossVoiceSimdOptions());
    ASSERT_TRUE (simdGraph.isValid());

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 64;
    ASSERT_TRUE (baselineGraph.prepare (sampleRate, blockSize).wasOk());
    ASSERT_TRUE (simdGraph.prepare (sampleRate, blockSize).wasOk());

    std::vector<float> baseL, baseR, simdL, simdR;
    MidiBuffer midi;
    midi.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);
    midi.addEvent (MidiMessage::noteOn (1, 64, 0.75f), 0);
    midi.addEvent (MidiMessage::noteOn (1, 67, 0.85f), 0);
    midi.addEvent (MidiMessage::noteOn (1, 71, 0.7f), 0);
    midi.addEvent (MidiMessage::noteOn (1, 74, 0.9f), 16);

    runMidiStereoBlock (baselineGraph, &midi, baseL, baseR, blockSize);
    runMidiStereoBlock (simdGraph, &midi, simdL, simdR, blockSize);
    midi.clear();

    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_NEAR (baseL[static_cast<size_t> (i)], simdL[static_cast<size_t> (i)], 1e-4f);
        EXPECT_NEAR (baseR[static_cast<size_t> (i)], simdR[static_cast<size_t> (i)], 1e-4f);
    }

#if YUP_ENABLE_ALLOCATION_HOOKS
    {
        yup::test::YdspAllocationCounter allocations;
        allocations.start();
        runMidiStereoBlock (simdGraph, &midi, simdL, simdR, blockSize);
        EXPECT_EQ (0u, allocations.stop());
    }
    runMidiStereoBlock (baselineGraph, &midi, baseL, baseR, blockSize);
#else
    runMidiStereoBlock (baselineGraph, &midi, baseL, baseR, blockSize);
    runMidiStereoBlock (simdGraph, &midi, simdL, simdR, blockSize);
#endif

    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_NEAR (baseL[static_cast<size_t> (i)], simdL[static_cast<size_t> (i)], 1e-4f);
        EXPECT_NEAR (baseR[static_cast<size_t> (i)], simdR[static_cast<size_t> (i)], 1e-4f);
    }
}
