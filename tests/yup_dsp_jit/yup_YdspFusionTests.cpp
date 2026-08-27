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

DspJitGraph fusionCompile (StringRef source, DspJitCompiler& compiler)
{
    auto result = compiler.compile (source);
    EXPECT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    if (! result.wasOk())
        return {};

    return std::move (result).getValue();
}

/** True when the compiled graph contains a synthesized fused kernel. */
bool fusionHappened (const DspJitGraph& graph)
{
    for (const auto& kernel : graph.getExecutionReport().getKernels())
        if (kernel.name.startsWith ("fused("))
            return true;

    return false;
}

/** Dumps every kernel's generated code when a test fails.

    The fused kernel is synthesized, so when its output is wrong there is no
    source to read - the listing is the only way to see what was actually
    built. */
void fusionDumpOnFailure (const DspJitGraph& graph)
{
    if (! ::testing::Test::HasFailure())
        return;

    const auto text = graph.getDiagnostics().toString();

    if (! text.isEmpty())
        std::cout << "\n[fusion] generated kernels:\n"
                  << text << std::endl;
}

void fusionRun (DspJitGraph& graph, const float* input, float* output, int numSamples)
{
    std::vector<DspJitInputBuffer> inputs;
    inputs.emplace_back (Span<const float> (input, static_cast<size_t> (numSamples)));

    std::vector<DspJitOutputBuffer> outputs;
    outputs.emplace_back (Span<float> (output, static_cast<size_t> (numSamples)));

    graph.process (inputs, outputs, numSamples, nullptr, nullptr, 0);
}

constexpr int fusionBlockSize = 16;

std::vector<float> fusionRamp()
{
    std::vector<float> data (static_cast<size_t> (fusionBlockSize));

    for (int i = 0; i < fusionBlockSize; ++i)
        data[static_cast<size_t> (i)] = 0.1f + static_cast<float> (i) * 0.05f;

    return data;
}

// Three stages, each with state, deliberately reusing the same state name `z`
// and the same local name `t` so the rename has to keep them apart.
constexpr auto fusionChainSource = R"YDSP(
    processor A {
        input stream in;
        output stream out;
        state float z;
        process { let t = in * 0.5; z = z * 0.5 + t; out = z; }
    }

    processor B {
        input stream in;
        output stream out;
        state float z;
        process { let t = in + 0.25; z = z * 0.25 + t; out = z; }
    }

    processor C {
        input stream in;
        output stream out;
        state float z;
        process { let t = in * 2.0; z = t - z * 0.125; out = z; }
    }

    graph G {
        input stream x;
        output stream y;

        node a = A;
        node b = B;
        node c = C;

        connection { x -> a.in; a.out -> b.in; b.out -> c.in; c.out -> y; }
    }
)YDSP";

// The same computation written as one processor: the reference the fused kernel
// has to agree with, bit for bit.
constexpr auto fusionEquivalentSource = R"YDSP(
    processor Whole {
        input stream in;
        output stream out;

        state float za;
        state float zb;
        state float zc;

        process {
            let ta = in * 0.5;
            za = za * 0.5 + ta;

            let tb = za + 0.25;
            zb = zb * 0.25 + tb;

            let tc = zb * 2.0;
            zc = tc - zc * 0.125;

            out = zc;
        }
    }

    graph G {
        input stream x;
        output stream y;
        node w = Whole;
        connection { x -> w.in; w.out -> y; }
    }
)YDSP";

} // namespace

//==============================================================================
// What fusion does
//==============================================================================

TEST (YdspFusionTests, FusesAChainOfThreeNodes)
{
    DspJitCompiler compiler;
    auto graph = fusionCompile (fusionChainSource, compiler);

    ASSERT_TRUE (graph.isValid());
    EXPECT_TRUE (fusionHappened (graph));
}

TEST (YdspFusionTests, FusedChainMatchesTheEquivalentSingleProcessor)
{
    DspJitCompiler chainCompiler, wholeCompiler;

    auto chain = fusionCompile (fusionChainSource, chainCompiler);
    auto whole = fusionCompile (fusionEquivalentSource, wholeCompiler);

    ASSERT_TRUE (chain.isValid());
    ASSERT_TRUE (whole.isValid());
    ASSERT_TRUE (fusionHappened (chain));
    ASSERT_FALSE (fusionHappened (whole)); // a single node is not a chain

    chain.prepare (44100.0, fusionBlockSize);
    whole.prepare (44100.0, fusionBlockSize);

    const auto input = fusionRamp();
    std::vector<float> chainOutput (static_cast<size_t> (fusionBlockSize), 0.0f);
    std::vector<float> wholeOutput (static_cast<size_t> (fusionBlockSize), 0.0f);

    // Several blocks, so the state carried across calls is exercised too.
    for (int block = 0; block < 4; ++block)
    {
        fusionRun (chain, input.data(), chainOutput.data(), fusionBlockSize);
        fusionRun (whole, input.data(), wholeOutput.data(), fusionBlockSize);

        for (int i = 0; i < fusionBlockSize; ++i)
            EXPECT_FLOAT_EQ (wholeOutput[static_cast<size_t> (i)], chainOutput[static_cast<size_t> (i)])
                << "block " << block << " sample " << i;
    }

    fusionDumpOnFailure (chain);
}

TEST (YdspFusionTests, FusesTheAlgebraFormIdenticallyToTheConnectionForm)
{
    // Both syntaxes converge on the same analyzed edges, so both fuse.
    DspJitCompiler algebraCompiler, connectionCompiler;

    auto algebra = fusionCompile (R"YDSP(
        processor Half { input stream in; output stream out; state float z; process { z = z * 0.5 + in; out = z; } }
        processor Twice { input stream in; output stream out; process { out = in * 2.0; } }
        graph G {
            input stream x;
            output stream y;
            process = x : Half : Twice : y;
        }
    )YDSP",
                                  algebraCompiler);

    auto connections = fusionCompile (R"YDSP(
        processor Half { input stream in; output stream out; state float z; process { z = z * 0.5 + in; out = z; } }
        processor Twice { input stream in; output stream out; process { out = in * 2.0; } }
        graph G {
            input stream x;
            output stream y;
            node h = Half;
            node t = Twice;
            connection { x -> h.in; h.out -> t.in; t.out -> y; }
        }
    )YDSP",
                                      connectionCompiler);

    ASSERT_TRUE (algebra.isValid());
    ASSERT_TRUE (connections.isValid());

    EXPECT_TRUE (fusionHappened (algebra));
    EXPECT_TRUE (fusionHappened (connections));

    algebra.prepare (44100.0, fusionBlockSize);
    connections.prepare (44100.0, fusionBlockSize);

    const auto input = fusionRamp();
    std::vector<float> algebraOutput (static_cast<size_t> (fusionBlockSize), 0.0f);
    std::vector<float> connectionOutput (static_cast<size_t> (fusionBlockSize), 0.0f);

    fusionRun (algebra, input.data(), algebraOutput.data(), fusionBlockSize);
    fusionRun (connections, input.data(), connectionOutput.data(), fusionBlockSize);

    for (int i = 0; i < fusionBlockSize; ++i)
        EXPECT_FLOAT_EQ (connectionOutput[static_cast<size_t> (i)], algebraOutput[static_cast<size_t> (i)]) << "sample " << i;
}

TEST (YdspFusionTests, DropsTheKernelsOfTheMembersItAbsorbed)
{
    // A member the fused body absorbed is no longer instantiated anywhere, so
    // compiling it would be dead machine code - and it would show up in the
    // report as a kernel the patch appears to run.
    DspJitCompiler compiler;
    auto graph = fusionCompile (fusionChainSource, compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    ASSERT_TRUE (fusionHappened (graph));

    StringArray names;

    for (const auto& kernel : graph.getExecutionReport().getKernels())
        names.add (kernel.name);

    EXPECT_EQ (1, names.size()) << "compiled " << names.joinIntoString (", ");
    EXPECT_FALSE (names.contains ("A"));
    EXPECT_FALSE (names.contains ("B"));
    EXPECT_FALSE (names.contains ("C"));
}

TEST (YdspFusionTests, KeepsAMemberKernelStillInstantiatedElsewhere)
{
    // `Half` is used twice: once inside the chain that fuses and once on its own
    // branch. Absorbing the first instance must not take the processor away from
    // the second, so exactly the fused kernel and `Half` survive.
    DspJitCompiler compiler;

    auto graph = fusionCompile (R"YDSP(
        processor Half { input stream in; output stream out; process { out = in * 0.5; } }
        processor Twice { input stream in; output stream out; process { out = in * 2.0; } }

        graph G {
            input stream x;
            input stream w;
            output stream y;
            output stream z;

            node chained = Half;
            node tail = Twice;
            node alone = Half;

            connection { x -> chained.in; chained.out -> tail.in; tail.out -> y; w -> alone.in; alone.out -> z; }
        }
    )YDSP",
                                compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    ASSERT_TRUE (fusionHappened (graph));

    StringArray names;

    for (const auto& kernel : graph.getExecutionReport().getKernels())
        names.add (kernel.name);

    EXPECT_TRUE (names.contains ("Half")) << "compiled " << names.joinIntoString (", ");
    EXPECT_FALSE (names.contains ("Twice"));
    EXPECT_EQ (2, names.size());
}

//==============================================================================
// The host-visible surface has to survive
//==============================================================================

TEST (YdspFusionTests, ParameterNamesSurviveFusion)
{
    DspJitCompiler compiler;

    auto graph = fusionCompile (R"YDSP(
        processor Gain {
            input stream in;
            output stream out;
            input value float g = 1.0 [[ name: "Gain" ]];
            process { out = in * g; }
        }

        processor Trim {
            input stream in;
            output stream out;
            input value float t = 1.0;
            process { out = in * t; }
        }

        graph G {
            input stream x;
            output stream y;
            node first = Gain;
            node second = Trim;
            connection { x -> first.in; first.out -> second.in; second.out -> y; }
        }
    )YDSP",
                                compiler);

    ASSERT_TRUE (graph.isValid());
    ASSERT_TRUE (fusionHappened (graph));

    // The patch named these `first.g` and `second.t`; fusing the two nodes into
    // one must not rename them out from under the host.
    ASSERT_TRUE (graph.hasParameter ("first.g")) << "fusion renamed a parameter";
    ASSERT_TRUE (graph.hasParameter ("second.t"));

    EXPECT_FLOAT_EQ (1.0f, graph.getParameter ("first.g"));
    EXPECT_FLOAT_EQ (1.0f, graph.getParameter ("second.t"));

    graph.prepare (44100.0, fusionBlockSize);
    graph.setParameter ("first.g", 2.0f);
    graph.setParameter ("second.t", 3.0f);

    const auto input = fusionRamp();
    std::vector<float> output (static_cast<size_t> (fusionBlockSize), 0.0f);

    fusionRun (graph, input.data(), output.data(), fusionBlockSize);

    // Both parameters reach the fused kernel: 2 * 3 = 6.
    for (int i = 0; i < fusionBlockSize; ++i)
        EXPECT_FLOAT_EQ (input[static_cast<size_t> (i)] * 6.0f, output[static_cast<size_t> (i)]) << "sample " << i;

    fusionDumpOnFailure (graph);
}

TEST (YdspFusionTests, GraphParameterAliasStillDrivesAFusedMember)
{
    DspJitCompiler compiler;

    auto graph = fusionCompile (R"YDSP(
        processor Gain {
            input stream in;
            output stream out;
            input value float g = 1.0;
            process { out = in * g; }
        }

        processor Twice { input stream in; output stream out; process { out = in * 2.0; } }

        graph G {
            input stream x;
            output stream y;
            input value float master = 0.5;

            node first = Gain;
            node second = Twice;

            connection { x -> first.in; first.out -> second.in; second.out -> y; master -> first.g; }
        }
    )YDSP",
                                compiler);

    ASSERT_TRUE (graph.isValid());
    ASSERT_TRUE (fusionHappened (graph));

    graph.prepare (44100.0, fusionBlockSize);

    ASSERT_TRUE (graph.hasParameter ("master"));
    graph.setParameter ("master", 0.25f);

    const auto input = fusionRamp();
    std::vector<float> output (static_cast<size_t> (fusionBlockSize), 0.0f);

    fusionRun (graph, input.data(), output.data(), fusionBlockSize);

    // The graph parameter still reaches the (now fused) member: 0.25 * 2.
    for (int i = 0; i < fusionBlockSize; ++i)
        EXPECT_FLOAT_EQ (input[static_cast<size_t> (i)] * 0.5f, output[static_cast<size_t> (i)]) << "sample " << i;
}

TEST (YdspFusionTests, FusesMembersCarryingStateInitialisersAndFunctions)
{
    DspJitCompiler compiler;

    auto graph = fusionCompile (R"YDSP(
        processor Scaled {
            input stream in;
            output stream out;

            state float coeff = 0.25;

            func scale (v: float) : float { return v * 2.0; }

            process { out = scale (in) * coeff; }
        }

        processor Offset {
            input stream in;
            output stream out;

            state float bias = 1.0;

            func scale (v: float) : float { return v + v; }

            process { out = scale (in) + bias; }
        }

        graph G {
            input stream x;
            output stream y;
            node s = Scaled;
            node o = Offset;
            connection { x -> s.in; s.out -> o.in; o.out -> y; }
        }
    )YDSP",
                                compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    ASSERT_TRUE (fusionHappened (graph));

    graph.prepare (44100.0, fusionBlockSize);

    const auto input = fusionRamp();
    std::vector<float> output (static_cast<size_t> (fusionBlockSize), 0.0f);

    fusionRun (graph, input.data(), output.data(), fusionBlockSize);

    // Both members declare a function called `scale` with different bodies, so
    // the renaming has to keep the two of them distinct.
    for (int i = 0; i < fusionBlockSize; ++i)
    {
        const auto x = input[static_cast<size_t> (i)];
        const auto expected = (x * 2.0f * 0.25f) * 2.0f + 1.0f;

        EXPECT_FLOAT_EQ (expected, output[static_cast<size_t> (i)]) << "sample " << i;
    }
}

TEST (YdspFusionTests, MeterNamesSurviveFusion)
{
    // Three meters over two members, and three values that are pairwise
    // distinct on the last sample - a name that survives but resolves to the
    // wrong slot is a different failure from one that stops resolving, and only
    // a patch with more than one meter can tell them apart.
    DspJitCompiler compiler;

    auto graph = fusionCompile (R"YDSP(
        processor Metered {
            input stream in;
            output stream out;
            output value float level;
            output value float raw;
            process { out = in * 0.5; level = abs (in); raw = in; }
        }

        processor Twice {
            input stream in;
            output stream out;
            output value float seen;
            process { out = in * 2.0; seen = in; }
        }

        graph G {
            input stream x;
            output stream y;
            node m = Metered;
            node t = Twice;
            connection { x -> m.in; m.out -> t.in; t.out -> y; }
        }
    )YDSP",
                                compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    ASSERT_TRUE (fusionHappened (graph));

    graph.prepare (44100.0, fusionBlockSize);

    // Descending through zero, so `abs (in)` and `in` differ on the last sample
    // and the two meters of the same member cannot be confused for each other.
    std::vector<float> input (static_cast<size_t> (fusionBlockSize));

    for (int i = 0; i < fusionBlockSize; ++i)
        input[static_cast<size_t> (i)] = 0.75f - static_cast<float> (i) * 0.1f;

    std::vector<float> output (static_cast<size_t> (fusionBlockSize), 0.0f);

    fusionRun (graph, input.data(), output.data(), fusionBlockSize);

    for (int i = 0; i < fusionBlockSize; ++i)
        EXPECT_FLOAT_EQ (input[static_cast<size_t> (i)], output[static_cast<size_t> (i)]) << "sample " << i;

    // A meter is written every sample, so what the host reads back is the last
    // one of the block - and each is still addressable under the name the patch
    // gave it, not one derived from the synthesized fused node.
    const auto last = input.back();

    ASSERT_LT (last, 0.0f); // the premise of the three expectations below

    EXPECT_FLOAT_EQ (-last, graph.getOutputValue ("m.level")) << "fusion renamed or reordered a meter";
    EXPECT_FLOAT_EQ (last, graph.getOutputValue ("m.raw"));
    EXPECT_FLOAT_EQ (last * 0.5f, graph.getOutputValue ("t.seen"));

    fusionDumpOnFailure (graph);
}

TEST (YdspFusionTests, MeterEdgeToAGraphMeterSurvivesFusion)
{
    // The edge itself has to be rerouted onto the fused node, not just the name:
    // the compaction that follows maps a still-dead source node to -1 without a
    // word in release builds, so a missed edge stops reporting silently.
    DspJitCompiler compiler;

    auto graph = fusionCompile (R"YDSP(
        processor Twice { input stream in; output stream out; process { out = in * 2.0; } }

        processor Metered {
            input stream in;
            output stream out;
            output value float level;
            process { out = in; level = in * 0.25; }
        }

        graph G {
            input stream x;
            output stream y;
            output value float peak;

            node t = Twice;
            node m = Metered;

            connection { x -> t.in; t.out -> m.in; m.out -> y; m.level -> peak; }
        }
    )YDSP",
                                compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    ASSERT_TRUE (fusionHappened (graph));

    graph.prepare (44100.0, fusionBlockSize);

    const auto input = fusionRamp();
    std::vector<float> output (static_cast<size_t> (fusionBlockSize), 0.0f);

    fusionRun (graph, input.data(), output.data(), fusionBlockSize);

    // The meter reads the second stage's input, which is twice the graph input.
    EXPECT_FLOAT_EQ (input.back() * 2.0f * 0.25f, graph.getOutputValue ("peak"));
    EXPECT_FLOAT_EQ (input.back() * 2.0f * 0.25f, graph.getOutputValue ("m.level"));

    fusionDumpOnFailure (graph);
}

TEST (YdspFusionTests, EventEdgeSurvivesCompactionAfterAnUnrelatedFusion)
{
    // `a`/`b` are an unrelated, otherwise-fusable pair declared first, so they
    // are the ones that fuse away and free up the low node indices; `arp`/
    // `voice` are declared after them and hold the graph's only event edge, so
    // it is their indices the fusion pass's own compaction has to shift. A
    // missing (or wrong) `liveIndex` remap on `graph.eventEdges` would resolve
    // the routed event against stale, pre-compaction node indices.
    DspJitCompiler compiler;

    auto graph = fusionCompile (R"YDSP(
        processor Half { input stream in; output stream out; process { out = in * 0.5; } }
        processor Twice { input stream in; output stream out; process { out = in * 2.0; } }

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
            event midi (e: noteOff) { sounding = 0.0; }
            process { out = sounding; }
        }

        graph G {
            input stream x;
            input stream trig;
            output stream y;
            output stream v;

            node a = Half;
            node b = Twice;
            node arp = Arp;
            node voice = Voice;

            connection { x -> a.in; a.out -> b.in; b.out -> y; trig -> arp.trig; arp.noteOn -> voice.midi; voice.out -> v; }
        }
    )YDSP",
                                compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    ASSERT_TRUE (fusionHappened (graph));

    graph.prepare (44100.0, fusionBlockSize);

    const auto input = fusionRamp();
    std::vector<float> trig (static_cast<size_t> (fusionBlockSize), 0.0f);
    trig[0] = 1.0f;

    std::vector<float> output (static_cast<size_t> (fusionBlockSize), 0.0f);
    std::vector<float> voiceOut (static_cast<size_t> (fusionBlockSize), 0.0f);

    std::vector<DspJitInputBuffer> inputs {
        DspJitInputBuffer (Span<const float> (input.data(), input.size())),
        DspJitInputBuffer (Span<const float> (trig.data(), trig.size()))
    };
    std::vector<DspJitOutputBuffer> outputs {
        DspJitOutputBuffer (Span<float> (output.data(), output.size())),
        DspJitOutputBuffer (Span<float> (voiceOut.data(), voiceOut.size()))
    };

    graph.process (inputs, outputs, fusionBlockSize);

    for (int i = 0; i < fusionBlockSize; ++i)
        EXPECT_FLOAT_EQ (72.0f, voiceOut[static_cast<size_t> (i)]) << "sample " << i;

    fusionDumpOnFailure (graph);
}

//==============================================================================
// What fusion must leave alone
//==============================================================================

// The `fanOut != 1` half of the fusion rule became reachable when node outputs
// gained the ability to fan out: a tapped intermediate is observable, so the
// link it sits on must not fuse. See DoesNotFuseATappedIntermediate below.

TEST (YdspFusionTests, DoesNotFuseIntoAMultipleInputNode)
{
    // Only a single-in/single-out kernel can become a stage of one fused loop,
    // so a chain running into a mixer stops there. The second graph input keeps
    // the two mixer inputs on distinct producers, which is what this test is
    // about - one producer feeding both would now be legal but would be testing
    // fan-out instead.
    DspJitCompiler compiler;

    auto graph = fusionCompile (R"YDSP(
        processor Half { input stream in; output stream out; process { out = in * 0.5; } }
        processor Mix { input stream a; input stream b; output stream out; process { out = a + b; } }

        graph G {
            input stream x;
            input stream w;
            output stream y;

            node h = Half;
            node m = Mix;

            connection { x -> h.in; h.out -> m.a; w -> m.b; m.out -> y; }
        }
    )YDSP",
                                compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    EXPECT_FALSE (fusionHappened (graph));
}

//==============================================================================
// Fusing a member that owns hidden state
//
// `@` allocates a ring in the *array* segment plus an int32 write pointer in the
// scalar segment, and `smooth` allocates another scalar - none of them declared,
// all of them assigned by the IR builder as it lowers the body. Fusing two such
// members concatenates their bodies into one, so those hidden allocations have to
// accumulate across both. Getting it wrong writes outside the node's state slice.
//
// The two shapes below are the same patch; the second taps the intermediate to a
// second graph output, which makes the producer fan out and so blocks the fusion.
// Comparing them is what turns a layout error into a number rather than a crash.

namespace
{

constexpr auto fusionHiddenStateBody =
    "processor Rings {\n"
    "    input stream in;\n"
    "    output stream out;\n"
    "    state float c1;\n"
    "    state float c2;\n"
    "    process {\n"
    "        let d1 = c1 @ 7;\n"
    "        c1 = d1 * 0.5 + in;\n"
    "        let d2 = c2 @ 5;\n"
    "        c2 = d2 * 0.25 + in;\n"
    "        out = d1 + d2 + (in @ 3);\n"
    "    }\n"
    "}\n"
    "processor Trim {\n"
    "    input stream in;\n"
    "    output stream out;\n"
    "    input value float g = 0.5 [[ smoothing: 0.02 ]];\n"
    "    process { out = in * g; }\n"
    "}\n";

} // namespace

TEST (YdspFusionTests, FusingAMemberWithHiddenDelayStateMatchesTheUnfusedChain)
{
    DspJitCompiler fusedCompiler;
    DspJitCompiler splitCompiler;

    auto fused = fusionCompile (String (fusionHiddenStateBody) + R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node r = Rings;
            node t = Trim;
            connection { x -> r.in; r.out -> t.in; t.out -> y; }
        }
    )YDSP",
                                fusedCompiler);

    // The tap gives `r.out` a second destination, so this one cannot fuse - it
    // is the reference the fused version has to agree with.
    auto split = fusionCompile (String (fusionHiddenStateBody) + R"YDSP(
        graph G {
            input stream x;
            output stream y;
            output stream tap;
            node r = Rings;
            node t = Trim;
            connection { x -> r.in; r.out -> t.in; r.out -> tap; t.out -> y; }
        }
    )YDSP",
                                splitCompiler);

    ASSERT_TRUE (fused.isValid()) << fusedCompiler.getDiagnostics().toString();
    ASSERT_TRUE (split.isValid()) << splitCompiler.getDiagnostics().toString();

    EXPECT_TRUE (fusionHappened (fused));
    EXPECT_FALSE (fusionHappened (split));

    constexpr int blockSize = 64;
    constexpr int blockCount = 4;

    fused.prepare (48000.0, blockSize);
    split.prepare (48000.0, blockSize);

    std::vector<float> input (static_cast<size_t> (blockSize));
    for (int i = 0; i < blockSize; ++i)
        input[static_cast<size_t> (i)] = 0.1f + static_cast<float> (i % 9) * 0.07f;

    // Several blocks, so the rings wrap and the smoother settles: a layout error
    // that only shows up after the write pointer has come round would be missed
    // by a single block.
    for (int block = 0; block < blockCount; ++block)
    {
        std::vector<float> fusedOut (static_cast<size_t> (blockSize), 0.0f);
        std::vector<float> splitOut (static_cast<size_t> (blockSize), 0.0f);
        std::vector<float> splitTap (static_cast<size_t> (blockSize), 0.0f);

        fusionRun (fused, input.data(), fusedOut.data(), blockSize);

        {
            std::vector<DspJitInputBuffer> inputs {
                DspJitInputBuffer (Span<const float> (input.data(), input.size()))
            };
            std::vector<DspJitOutputBuffer> outputs {
                DspJitOutputBuffer (Span<float> (splitOut.data(), splitOut.size())),
                DspJitOutputBuffer (Span<float> (splitTap.data(), splitTap.size()))
            };
            split.process (inputs, outputs, blockSize);
        }

        for (int i = 0; i < blockSize; ++i)
            EXPECT_NEAR (splitOut[static_cast<size_t> (i)], fusedOut[static_cast<size_t> (i)], 1e-5f)
                << "block " << block << " sample " << i;
    }

    fusionDumpOnFailure (fused);
}

TEST (YdspFusionTests, ReverbSizedDelayRingsWorkWithoutFusion)
{
    // The control for FusesAMemberWithReverbSizedDelayRings below: the *same*
    // processor, alone in the graph so there is no chain to fuse. If this
    // crashes too then fusion is irrelevant and the bug is in large multi-ring
    // state on its own; if it passes, fusion is genuinely implicated.
    DspJitCompiler compiler;

    auto graph = fusionCompile (R"YDSP(
        processor Combs {
            input stream in;
            output stream out;
            input value float feedback = 0.7 [[ smoothing: 0.02 ]];
            state float c1;
            state float c2;
            state float c3;
            state float c4;
            process {
                let d1 = c1 @ 1116;
                c1 = d1 * feedback + in * 0.015;
                let d2 = c2 @ 1188;
                c2 = d2 * feedback + in * 0.015;
                let d3 = c3 @ 1277;
                c3 = d3 * feedback + in * 0.015;
                let d4 = c4 @ 1356;
                c4 = d4 * feedback + in * 0.015;
                out = d1 + d2 + d3 + d4;
            }
        }
        graph G {
            input stream x;
            output stream y;
            node c = Combs;
            connection { x -> c.in; c.out -> y; }
        }
    )YDSP",
                                compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    ASSERT_FALSE (fusionHappened (graph)); // one node is not a chain

    constexpr int blockSize = 64;
    graph.prepare (48000.0, blockSize);

    std::vector<float> input (static_cast<size_t> (blockSize), 0.25f);

    for (int block = 0; block < 32; ++block)
    {
        std::vector<float> output (static_cast<size_t> (blockSize), 0.0f);
        fusionRun (graph, input.data(), output.data(), blockSize);

        for (int i = 0; i < blockSize; ++i)
        {
            const auto sample = output[static_cast<size_t> (i)];
            ASSERT_TRUE (std::isfinite (sample)) << "block " << block << " sample " << i;
            ASSERT_LT (std::fabs (sample), 100.0f) << "block " << block << " sample " << i;
        }
    }
}

TEST (YdspFusionTests, FusesTwoMembersEachOwningLargeDelayRings)
{
    // Narrows the ingredient list further: rings in *both* members, and no
    // `smooth` anywhere. If this crashes but the smoothed version does not (or
    // vice versa) that isolates whether the hidden int32 write pointers and the
    // hidden `smooth` slots are interfering with each other's allocation.
    DspJitCompiler compiler;

    auto graph = fusionCompile (R"YDSP(
        processor CombA {
            input stream in;
            output stream out;
            state float c1;
            state float c2;
            process {
                let d1 = c1 @ 1116;
                c1 = d1 * 0.7 + in * 0.015;
                let d2 = c2 @ 1188;
                c2 = d2 * 0.7 + in * 0.015;
                out = d1 + d2;
            }
        }
        processor CombB {
            input stream in;
            output stream out;
            state float c3;
            state float c4;
            process {
                let d3 = c3 @ 1277;
                c3 = d3 * 0.7 + in * 0.015;
                let d4 = c4 @ 1356;
                c4 = d4 * 0.7 + in * 0.015;
                out = d3 + d4;
            }
        }
        graph G {
            input stream x;
            output stream y;
            node a = CombA;
            node b = CombB;
            connection { x -> a.in; a.out -> b.in; b.out -> y; }
        }
    )YDSP",
                                compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    EXPECT_TRUE (fusionHappened (graph));

    constexpr int blockSize = 64;
    graph.prepare (48000.0, blockSize);

    std::vector<float> input (static_cast<size_t> (blockSize), 0.25f);

    for (int block = 0; block < 32; ++block)
    {
        std::vector<float> output (static_cast<size_t> (blockSize), 0.0f);
        fusionRun (graph, input.data(), output.data(), blockSize);

        for (int i = 0; i < blockSize; ++i)
        {
            const auto sample = output[static_cast<size_t> (i)];
            ASSERT_TRUE (std::isfinite (sample)) << "block " << block << " sample " << i;
            ASSERT_LT (std::fabs (sample), 100.0f) << "block " << block << " sample " << i;
        }
    }
}

TEST (YdspFusionTests, FusesAMemberWithReverbSizedDelayRings)
{
    // The shape that crashed: rings of the size a Schroeder reverb uses, so an
    // out-of-bounds state-array store lands well outside the node's slice rather
    // than merely corrupting a neighbouring scalar.
    DspJitCompiler compiler;

    auto graph = fusionCompile (R"YDSP(
        processor Combs {
            input stream in;
            output stream out;
            input value float feedback = 0.7 [[ smoothing: 0.02 ]];
            state float c1;
            state float c2;
            state float c3;
            state float c4;
            process {
                let d1 = c1 @ 1116;
                c1 = d1 * feedback + in * 0.015;
                let d2 = c2 @ 1188;
                c2 = d2 * feedback + in * 0.015;
                let d3 = c3 @ 1277;
                c3 = d3 * feedback + in * 0.015;
                let d4 = c4 @ 1356;
                c4 = d4 * feedback + in * 0.015;
                out = d1 + d2 + d3 + d4;
            }
        }
        processor Level {
            input stream in;
            output stream out;
            input value float g = 1.0 [[ smoothing: 0.02 ]];
            process { out = in * g; }
        }
        graph G {
            input stream x;
            output stream y;
            node c = Combs;
            node l = Level;
            connection { x -> c.in; c.out -> l.in; l.out -> y; }
        }
    )YDSP",
                                compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    EXPECT_TRUE (fusionHappened (graph));

    constexpr int blockSize = 64;
    graph.prepare (48000.0, blockSize);

    std::vector<float> input (static_cast<size_t> (blockSize), 0.25f);

    // Well past the longest ring (1356), so every write pointer wraps.
    for (int block = 0; block < 32; ++block)
    {
        std::vector<float> output (static_cast<size_t> (blockSize), 0.0f);
        fusionRun (graph, input.data(), output.data(), blockSize);

        for (int i = 0; i < blockSize; ++i)
        {
            const auto sample = output[static_cast<size_t> (i)];
            ASSERT_TRUE (std::isfinite (sample)) << "block " << block << " sample " << i;
            ASSERT_LT (std::fabs (sample), 100.0f) << "block " << block << " sample " << i;
        }
    }

    fusionDumpOnFailure (graph);
}

TEST (YdspFusionTests, DoesNotFuseATappedIntermediate)
{
    // `h.out` feeds both the next stage and a graph output, so the intermediate
    // is observable and the h -> t link cannot become a register. This is the
    // `fanOut != 1` half of the fusion rule, unreachable before node outputs
    // could fan out.
    DspJitCompiler compiler;

    auto graph = fusionCompile (R"YDSP(
        processor Half { input stream in; output stream out; process { out = in * 0.5; } }
        processor Twice { input stream in; output stream out; process { out = in * 2.0; } }

        graph G {
            input stream x;
            output stream y;
            output stream tap;

            node h = Half;
            node t = Twice;

            connection { x -> h.in; h.out -> t.in; h.out -> tap; t.out -> y; }
        }
    )YDSP",
                                compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    EXPECT_FALSE (fusionHappened (graph));

    graph.prepare (44100.0, 8);

    std::vector<float> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
    std::vector<float> out (8, 0.0f);
    std::vector<float> tap (8, 0.0f);

    std::vector<DspJitInputBuffer> inputs { DspJitInputBuffer (Span<const float> (input.data(), input.size())) };
    std::vector<DspJitOutputBuffer> outputs {
        DspJitOutputBuffer (Span<float> (out.data(), out.size())),
        DspJitOutputBuffer (Span<float> (tap.data(), tap.size()))
    };

    graph.process (inputs, outputs, 8);

    for (size_t i = 0; i < input.size(); ++i)
    {
        EXPECT_NEAR (input[i], out[i], 1e-6f) << "sample " << i;
        EXPECT_NEAR (input[i] * 0.5f, tap[i], 1e-6f) << "sample " << i;
    }
}

TEST (YdspFusionTests, DoesNotFuseAcrossAnInlineDelay)
{
    // Only the runtime's delay buffer can hold samples between the two stages;
    // a register cannot.
    DspJitCompiler compiler;

    auto graph = fusionCompile (R"YDSP(
        processor Half { input stream in; output stream out; process { out = in * 0.5; } }
        processor Twice { input stream in; output stream out; process { out = in * 2.0; } }

        graph G {
            input stream x;
            output stream y;
            node a = Half;
            node b = Twice;
            connection { x -> a.in; a.out -> [4] -> b.in; b.out -> y; }
        }
    )YDSP",
                                compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    EXPECT_FALSE (fusionHappened (graph));
}

TEST (YdspFusionTests, DoesNotFuseABlockModeNode)
{
    DspJitCompiler compiler;

    auto graph = fusionCompile (R"YDSP(
        processor Half { input stream in; output stream out; process { out = in * 0.5; } }
        processor Blocky {
            input stream in;
            output stream out;
            process block { for i in 0..blockSize { out[i] = in[i] * 2.0; } }
        }

        graph G {
            input stream x;
            output stream y;
            node a = Half;
            node b = Blocky;
            connection { x -> a.in; a.out -> b.in; b.out -> y; }
        }
    )YDSP",
                                compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    EXPECT_FALSE (fusionHappened (graph));
}

TEST (YdspFusionTests, DoesNotFuseAnOversampledNode)
{
    // The runtime resamples around the kernel call, which one fused loop cannot
    // reproduce.
    DspJitCompiler compiler;

    auto graph = fusionCompile (R"YDSP(
        processor Half { input stream in; output stream out; process { out = in * 0.5; } }
        processor Twice { input stream in; output stream out; process { out = in * 2.0; } }

        graph G {
            input stream x;
            output stream y;
            node a = Half;
            node b = Twice * 2;
            connection { x -> a.in; a.out -> b.in; b.out -> y; }
        }
    )YDSP",
                                compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    EXPECT_FALSE (fusionHappened (graph));
}

TEST (YdspFusionTests, DoesNotFuseANodeThatOnlyEmitsAnOutputEvent)
{
    // `Gate` has no input handler and no `input event` at all - just an
    // `output event` it emits from `process` - so absent the exclusion it
    // would otherwise qualify for fusion (1 audio in, 1 audio out, sample
    // mode, voice count 1, no oversampling) and this whole three-node chain
    // would collapse into one fused kernel.
    DspJitCompiler compiler;

    auto graph = fusionCompile (R"YDSP(
        processor Half { input stream in; output stream out; process { out = in * 0.5; } }

        processor Gate {
            input stream in;
            output stream out;
            output event noteOn;
            process {
                out = in;
                emit noteOn (pitch: in) -> noteOn;
            }
        }

        processor Twice { input stream in; output stream out; process { out = in * 2.0; } }

        graph G {
            input stream x;
            output stream y;
            output event noteOn;

            node a = Half;
            node g = Gate;
            node b = Twice;

            connection { x -> a.in; a.out -> g.in; g.out -> b.in; b.out -> y; g.noteOn -> noteOn; }
        }
    )YDSP",
                                compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    EXPECT_FALSE (fusionHappened (graph));
}

} // namespace yup::test
