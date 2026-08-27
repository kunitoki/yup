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

#include <vector>

using namespace yup;

namespace
{

//==============================================================================
constexpr const char* subgraphGainProcessor =
    "processor Gain { input stream in; output stream out; input value float g = 2.0; process { out = in * g; } }\n";

DspJitGraph subgraphCompile (StringRef source, DspJitCompiler& compiler)
{
    auto result = compiler.compile (String (subgraphGainProcessor) + source);
    EXPECT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    if (! result.wasOk())
        return {};

    return std::move (result).getValue();
}

String subgraphCompileError (StringRef source)
{
    DspJitCompiler compiler;
    auto result = compiler.compile (String (subgraphGainProcessor) + source);

    EXPECT_FALSE (result.wasOk());

    return compiler.getDiagnostics().toString();
}

std::vector<float> subgraphRun (DspJitGraph& graph, std::vector<float> input)
{
    std::vector<float> output (input.size(), 0.0f);

    std::vector<DspJitInputBuffer> inputBuffers { DspJitInputBuffer (Span<const float> (input.data(), input.size())) };
    std::vector<DspJitOutputBuffer> outputBuffers { DspJitOutputBuffer (Span<float> (output.data(), output.size())) };

    graph.process (inputBuffers, outputBuffers, static_cast<int> (input.size()));

    return output;
}

const std::vector<float> subgraphInput { 1.0f, 2.0f, -0.5f, 0.25f, 4.0f, 0.0f, -3.0f, 1.5f };

} // namespace

//==============================================================================

TEST (YdspSubgraphTests, InlinesASubgraphUnderTheParentNodeNamePrefix)
{
    DspJitCompiler compiler;

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
    DspJitCompiler compiler;

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
    DspJitCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        graph Sub {
            input stream in;
            output stream out;
            input value float amount = 3.0;
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
    DspJitCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        graph Sub {
            input stream in;
            output stream out;
            input value float amount = 3.0;
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
    DspJitCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        graph Sub {
            input stream in;
            output stream out;
            input value float amount = 3.0;
            node a = Gain;
            connection { in -> a.in; a.out -> out; amount -> a.g; }
        }
        graph Main [[ main ]] {
            input stream x;
            output stream y;
            input value float master = 4.0 [[ name: "Master", min: 0.0, max: 10.0 ]];
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
    DspJitCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        graph Sub {
            input stream in;
            output stream out;
            input value float amount = 1.0;
            node a = Gain;
            node b = Gain;
            connection { in -> a.in; a.out -> b.in; b.out -> out; amount -> a.g; amount -> b.g; }
        }
        graph Main [[ main ]] {
            input stream x;
            output stream y;
            input value float master = 2.0;
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
    DspJitCompiler compiler;

    auto graph = subgraphCompile (R"YDSP(
        processor Peak {
            input stream in;
            output stream out;
            output value float peak;
            process { out = in; peak = max (peak, abs (in)); }
        }
        graph Sub {
            input stream in;
            output stream out;
            output value float level;
            node p = Peak;
            connection { in -> p.in; p.out -> out; p.peak -> level; }
        }
        graph Main [[ main ]] {
            input stream x;
            output stream y;
            output value float meter;
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
    DspJitCompiler compiler;

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
    DspJitCompiler compiler;

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
    DspJitCompiler compiler;

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

TEST (YdspSubgraphTests, RejectsAVoiceBankOfAGraph)
{
    const auto message = subgraphCompileError (R"YDSP(
        graph Sub { input stream in; output stream out; node a = Gain; connection { in -> a.in; a.out -> out; } }
        graph Main [[ main ]] { input stream x; output stream y; node s = Sub[4]; connection { x -> s.in; s.out -> y; } }
    )YDSP");

    EXPECT_TRUE (message.contains ("voice banks"));
    EXPECT_TRUE (message.contains ("only supported on processors"));
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
        graph Sub { input event midi; input stream in; output stream out; node a = Gain; connection { in -> a.in; a.out -> out; } }
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
        .replaceWithText ("processor Boost { input stream in; output stream out; input value float g = 2.0; process { out = in * g; } }\n");

    tempDir.getChildFile ("fx/Chain.ydsp")
        .replaceWithText (R"YDSP(
            import Boost as b;
            graph Chain [[ main ]] {
                input stream in;
                output stream out;
                input value float amount = 3.0;
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

    DspJitCompiler compiler;
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
    DspJitCompiler compiler;

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
    DspJitCompiler compiler;

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
    DspJitCompiler compiler;

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
    DspJitCompiler compiler;

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
    DspJitCompiler compiler;

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
    DspJitCompiler compiler;

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
            input event midi;
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
    DspJitCompiler compiler;

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
            input event midi;
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
