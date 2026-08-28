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

std::unique_ptr<YdspAnalyzedProgram> analyze (StringRef source, YdspDiagnostics& diagnostics)
{
    YdspLexer lexer (source, diagnostics);
    auto tokens = lexer.tokenize();

    YdspParser parser (std::move (tokens), diagnostics);
    auto program = parser.parseProgram();

    if (program == nullptr)
        return nullptr;

    YdspSemanticAnalyzer analyzer (diagnostics);
    return analyzer.analyze (std::move (program));
}

} // namespace

//==============================================================================

TEST (YdspSemanticAnalyzerTests, AnalyzesPassThroughGraph)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P { input stream in; output stream out; process { out = in; } }
        graph G { input stream a; output stream b; node p = P; connection { a -> p.in; p.out -> b; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, analyzed);

    EXPECT_EQ (1u, analyzed->processors.size());
    EXPECT_EQ (1u, analyzed->graph.inputStreams.size());
    EXPECT_EQ (1u, analyzed->graph.outputStreams.size());
    ASSERT_EQ (1u, analyzed->graph.nodes.size());
    EXPECT_EQ ("p", analyzed->graph.nodes[0].instanceName);

    ASSERT_EQ (2u, analyzed->graph.edges.size());
    EXPECT_EQ (-1, analyzed->graph.edges[0].srcNode);
    EXPECT_EQ (0, analyzed->graph.edges[0].srcStream);
    EXPECT_EQ (0, analyzed->graph.edges[0].dstNode);
    EXPECT_EQ (-1, analyzed->graph.edges[1].dstNode);

    ASSERT_EQ (1u, analyzed->graph.topoOrder.size());
    EXPECT_EQ (0, analyzed->graph.topoOrder[0]);
}

TEST (YdspSemanticAnalyzerTests, AnalyzesSampleProcessorWithPrevAndParams)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor OnePole {
            input stream in;
            output stream out;
            input value float a = 0.5;
            output value float level;
            state float z;
            process {
                out = (1 - a) * in + a * out';
                z = 0.999 * z + in;
                level = abs (z);
            }
        }
        graph G { input stream x; output stream y; node f = OnePole; connection { x -> f.in; f.out -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, analyzed);

    const auto& processor = analyzed->processors[0];
    EXPECT_EQ (YdspProcessMode::sample, processor.mode);
    EXPECT_EQ (1u, processor.inputValues.size());
    EXPECT_EQ (1u, processor.outputValues.size());
    EXPECT_EQ (1u, processor.states.size());
    EXPECT_EQ (1, processor.hiddenStateCount); // one ' operator
    EXPECT_TRUE (processor.provenRealtimeSafe);
    EXPECT_TRUE (processor.loops.empty());
}

TEST (YdspSemanticAnalyzerTests, AnalyzesBlockProcessorWithLoop)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Delay {
            input stream in;
            output stream out;
            state float mem[256];
            state int wp;
            process block {
                for i in 0..blockSize {
                    mem[wp] = in[i];
                    out[i] = mem[wp];
                    wp = wp + 1;
                }
            }
        }
        graph G { input stream x; output stream y; node d = Delay; connection { x -> d.in; d.out -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, analyzed);

    const auto& processor = analyzed->processors[0];
    EXPECT_EQ (YdspProcessMode::block, processor.mode);
    EXPECT_TRUE (processor.provenRealtimeSafe);
    ASSERT_EQ (1u, processor.loops.size());
    EXPECT_EQ (YdspLoopBoundKind::blockSize, processor.loops[0].bound.kind);
}

TEST (YdspSemanticAnalyzerTests, AnalyzesConstantLoopBound)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Taps {
            input stream in;
            output stream out;
            state float mem[16];
            process block {
                for i in 0..4 { out[i] = mem[i]; }
            }
        }
        graph G { input stream x; output stream y; node t = Taps; connection { x -> t.in; t.out -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, analyzed);
    ASSERT_EQ (1u, analyzed->processors[0].loops.size());
    EXPECT_EQ (YdspLoopBoundKind::constant, analyzed->processors[0].loops[0].bound.kind);
    EXPECT_EQ (4, analyzed->processors[0].loops[0].bound.constant);
}

TEST (YdspSemanticAnalyzerTests, AnalyzesGraphWithAlgebra)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Gain { input stream in; output stream out; input value float g = 1; process { out = in * g; } }
        graph Chain {
            input stream dry;
            output stream wet;
            process = dry : Gain (g = 0.5) : wet;
        }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, analyzed);

    const auto& graph = analyzed->graph;
    ASSERT_EQ (1u, graph.nodes.size());
    ASSERT_EQ (1u, graph.nodes[0].paramDefaults.size());
    EXPECT_EQ (YdspValueType::float32Type, graph.nodes[0].paramDefaults[0].type);
    EXPECT_EQ (0.5, graph.nodes[0].paramDefaults[0].asDouble);

    // dry -> gain.in, gain.out -> wet
    ASSERT_EQ (2u, graph.edges.size());
    EXPECT_EQ (-1, graph.edges[0].srcNode);
    EXPECT_EQ (0, graph.edges[0].srcStream);
    EXPECT_EQ (0, graph.edges[0].dstNode);
    EXPECT_EQ (0, graph.edges[0].dstStream);
    EXPECT_EQ (0, graph.edges[1].srcNode);
    EXPECT_EQ (-1, graph.edges[1].dstNode);
}

TEST (YdspSemanticAnalyzerTests, AnalyzesValueAndMeterEdges)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Sat {
            input stream in;
            output stream out;
            input value float drive = 1;
            output value float level;
            process { out = tanh (in * drive); level = abs (out); }
        }
        graph G {
            input stream x;
            output stream y;
            output value float meter;
            input value float master = 0.8;
            node sat = Sat;
            connection {
                x -> sat.in;
                sat.out -> y;
                master -> sat.drive;
                sat.level -> meter;
            }
        }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, analyzed);

    const auto& graph = analyzed->graph;
    ASSERT_EQ (1u, graph.valueEdges.size());
    EXPECT_EQ (0, graph.valueEdges[0].srcParam);
    EXPECT_EQ (0, graph.valueEdges[0].dstNode);
    EXPECT_EQ (0, graph.valueEdges[0].dstParam);

    ASSERT_EQ (1u, graph.meterEdges.size());
    EXPECT_EQ (0, graph.meterEdges[0].srcNode);
    EXPECT_EQ (0, graph.meterEdges[0].srcMeter);
    EXPECT_EQ (0, graph.meterEdges[0].dstMeter);
}

TEST (YdspSemanticAnalyzerTests, AnalyzesIdentityAlgebra)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            process = _;
        }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, analyzed);

    ASSERT_EQ (1u, analyzed->graph.edges.size());
    EXPECT_EQ (-1, analyzed->graph.edges[0].srcNode);
    EXPECT_EQ (0, analyzed->graph.edges[0].srcStream);
    EXPECT_EQ (-1, analyzed->graph.edges[0].dstNode);
    EXPECT_EQ (0, analyzed->graph.edges[0].dstStream);
}

//==============================================================================

TEST (YdspSemanticAnalyzerTests, RejectsUnknownSymbol)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P { input stream in; output stream out; process { out = missing; } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_EQ (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, RejectsTypeMismatch)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P { input stream in; output stream out; process { int x = 1.5; out = x; } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsUnboundedLoop)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                let n = 4;
                for i in 0..(n * 2) { out[i] = in[i]; }
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsParamWriteInSampleMode)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input value float drive = 1;
            process { drive = drive + 1; out = in; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, AllowsParamWriteInBlockMode)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input value float drive = 1;
            process block { drive = drive + 1; for i in 0..blockSize { out[i] = in[i] * drive; } }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsDelayPrimitiveInBlockMode)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..blockSize { out[i] = in[i]'; }
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsStreamIndexInSampleMode)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P { input stream in; output stream out; process { out[0] = in; } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsArityMismatchInAlgebra)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Mix { input stream a; input stream b; output stream out; process { out = a + b; } }
        graph G {
            input stream dry;
            output stream wet;
            process = dry : Mix : wet;
        }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsUnconnectedGraphInput)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P { input stream in; output stream out; process { out = in; } }
        graph G {
            input stream a;
            input stream b;
            output stream y;
            node p = P;
            connection { a -> p.in; p.out -> y; }
        }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsDuplicateSymbol)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z;
            state int z;
            process { out = in; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsUnknownProcessorInGraph)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node p = Nope;
            connection { x -> p.in; p.out -> y; }
        }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsMultiChannelStream)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in[2];
            output stream out;
            process { out = in; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, AcceptsSingleChannelStream)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in[1];
            output stream out;
            process { out = in; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, AcceptsIntegerArgumentsToMinMaxClampAbsSign)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            output stream out;
            state int n;
            process {
                let mn = min (n, 3);
                let mx = max (n, -3);
                let cl = clamp (n, -10, 10);
                let ab = abs (n);
                let sg = sign (n);
                out = float32 (mn + mx + cl + ab + sg);
            }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, RejectsMixedIntAndFloatArgumentsToMin)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            output stream out;
            state int n;
            state float f;
            process { out = min (n, f); }
        }
        graph G { input event midi; output stream y; node p = P; connection { p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsMixedIntAndFloatArgumentsToClamp)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            output stream out;
            state int n;
            state float f;
            process { out = clamp (n, 0, f); }
        }
        graph G { input event midi; output stream y; node p = P; connection { p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, IntegerLiteralArgumentAloneStaysFloat)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            output stream out;
            process { out = min (3, 5); }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, WarnsOnUnknownEndpointAnnotationKey)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            output stream out;
            input value float cutoff = 100.0 [[ mim: 0.5 ]];
            process { out = cutoff; }
        }
        graph G { output stream y; node v = P; connection { v.out -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_NE (nullptr, analyzed);
    EXPECT_FALSE (diagnostics.hasErrors());

    bool foundExpectedWarning = false;

    for (int i = 0; i < diagnostics.getCount(); ++i)
    {
        const auto& item = diagnostics.getItem (i);

        if (item.severity == YdspSeverity::warning
            && item.message.contains ("mim")
            && item.message.contains ("name")
            && item.message.contains ("style"))
        {
            foundExpectedWarning = true;
        }
    }

    EXPECT_TRUE (foundExpectedWarning);
}

TEST (YdspSemanticAnalyzerTests, DoesNotWarnOnKnownEndpointAnnotationKeys)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            output stream out;
            input value float cutoff = 100.0 [[ name: "Cutoff", min: 0.0, max: 1000.0, unit: "Hz", step: 1.0, style: "knob" ]];
            process { out = cutoff; }
        }
        graph G { output stream y; node v = P; connection { v.out -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_NE (nullptr, analyzed);
    EXPECT_FALSE (diagnostics.hasErrors());

    for (int i = 0; i < diagnostics.getCount(); ++i)
        EXPECT_FALSE (diagnostics.getItem (i).message.contains ("Unknown endpoint annotation"))
            << diagnostics.getItem (i).message;
}

TEST (YdspSemanticAnalyzerTests, RejectsRecursionOperator)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor F { input stream a; input stream b; output stream out; process { out = a + b; } }
        graph G {
            input stream x;
            output stream y;
            process = x : F ~ F : y;
        }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsAssignmentToLet)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                let t = in;
                t = 0;
                out = t;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, AnalyzesProcessorWithFunctionCall)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            func scale(x: float) : float {
                return x * 2.0;
            }
            process { out = scale(in); }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, AnalyzesProcessorWithMultiParamFunction)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            func mix(a: float, b: float, t: float) : float {
                return a + (b - a) * t;
            }
            process { out = mix(in, out', 0.5); }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, AnalyzesProcessorWithMultipleFunctions)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            output stream out;
            input event midi;
            state float freq;
            state float phase;
            func noteToFreq(pitch: float) : float {
                return 440.0 * pow(2.0, (pitch - 69.0) / 12.0);
            }
            func polyBlep(t: float, dt: float) : float {
                float r = 0.0;
                if (t < dt) {
                    r = -1.0;
                } else if (t > 1.0 - dt) {
                    r = 1.0;
                }
                return r;
            }
            event midi (e: noteOn) {
                freq = noteToFreq(e.pitch);
            }
            process {
                phase = phase + freq / sampleRate;
                out = polyBlep(phase, freq / sampleRate);
            }
        }
        graph G { input event midi; output stream y; node p = P[1]; connection { midi -> p.midi; p.out -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, RejectsRecursiveFunction)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            func recurse(x: float) : float {
                return recurse(x);
            }
            process { out = in; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, AnalyzesFloat64AndInt64Program)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input value float64 acc = 0.0;
            input value int64   counter = 0;
            output value float64 level;
            state  float64 z;
            process {
                float64 d = 0.25;
                int64   j = 2;
                acc = acc + d;
                counter = counter + j;
                z = z * 0.999 + float64 (in);
                level = abs (z);
                out = in;
            }
        }
        graph G {
            input stream x;
            output stream y;
            input value float64 gacc = 1.0;
            input value int64 gcnt = 7;
            node p = P;
            connection { x -> p.in; p.out -> y; gacc -> p.acc; gcnt -> p.counter; }
        }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, analyzed);

    const auto& processor = analyzed->processors[0];
    ASSERT_EQ (2u, processor.inputValues.size());
    EXPECT_EQ (YdspPrimitiveType::float64Type, processor.inputValues[0]->type);
    EXPECT_EQ (YdspPrimitiveType::int64Type, processor.inputValues[1]->type);
    EXPECT_EQ (YdspPrimitiveType::float64Type, processor.outputValues[0]->type);
    EXPECT_EQ (YdspPrimitiveType::float64Type, processor.states[0]->type);

    const auto& graph = analyzed->graph;
    ASSERT_EQ (2u, graph.inputValueDefaults.size());
    EXPECT_EQ (YdspValueType::float64Type, graph.inputValueDefaults[0].type);
    EXPECT_EQ (1.0, graph.inputValueDefaults[0].asDouble);
    EXPECT_EQ (YdspValueType::int64Type, graph.inputValueDefaults[1].type);
    EXPECT_EQ (7, graph.inputValueDefaults[1].asInt);

    ASSERT_EQ (1u, graph.nodes.size());
    ASSERT_EQ (2u, graph.nodes[0].paramDefaults.size());
    EXPECT_EQ (YdspValueType::float64Type, graph.nodes[0].paramDefaults[0].type);
    EXPECT_EQ (0.0, graph.nodes[0].paramDefaults[0].asDouble);
    EXPECT_EQ (YdspValueType::int64Type, graph.nodes[0].paramDefaults[1].type);
    EXPECT_EQ (0, graph.nodes[0].paramDefaults[1].asInt);
}

TEST (YdspSemanticAnalyzerTests, AllowsLiteralAdaptationAcrossWidths)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                float64 d = 0.9 * 2.0;   // float literals adapt to float64
                int64   j = 2 * 3;       // int literals adapt to int64
                out = in * (1 - 0.5);    // int literal adapts to float32
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, RejectsMixedWidthBinary)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                float64 d = 0.5;
                out = in * d;   // float32 * float64 requires an explicit cast
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("Cannot mix 'float32' and 'float64'"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, RejectsImplicitIntFloatMix)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                int   i = 2;
                out = in * i;   // int32 * float32 requires an explicit cast
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsIntCondition)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                int i = 1;
                if (i) { out = in; } else { out = 0; }
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsInt64Index)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float buf[16];
            process block {
                int64 j = 0;
                for i in 0..blockSize { buf[int32(j)] = in[i]; out[i] = buf[int32(j)]; }
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());

    YdspDiagnostics diagnostics2;
    auto analyzed2 = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float buf[16];
            process block {
                int64 j = 0;
                for i in 0..blockSize { buf[j] = in[i]; out[i] = buf[j]; }
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                              diagnostics2);

    EXPECT_TRUE (diagnostics2.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsNarrowingAssignment)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                float64 d = 0.5;
                float32 f = d;   // float64 -> float32 requires an explicit cast
                out = f;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, CastsFixStrictViolations)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                int     i = 2;
                float64 d = 0.5;
                float32 f = float32 (d);            // explicit narrowing
                out = in * float32 (i);             // explicit int -> float32
                float64 acc = float64 (out) + float64 (f);  // both float64
                acc = acc + float64 (in);
                out = float32 (acc);
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, RejectsStreamTypeMismatchInGraph)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P { input stream float64 in; output stream out; process { out = float32(in); } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, AnalyzesStructStateAndInit)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            struct Voice { float phase; float buf[8]; int idx; }
            Voice mono;
            Voice voices[4];
            init {
                mono.phase = 0.5;
                mono.buf[2] = 1.0;
                voices[1].idx = 3;
                voices[0].buf[0] = 0.25;
            }
            process {
                mono.idx = (mono.idx + 1) & 7;
                mono.buf[mono.idx] = in;
                out = mono.phase + mono.buf[(mono.idx - 3) & 7] + float (voices[1].idx);
                mono.phase = mono.phase + 0.25;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, analyzed);
    EXPECT_TRUE (analyzed->processors[0].provenRealtimeSafe);
}

TEST (YdspSemanticAnalyzerTests, RejectsUnknownStructType)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            output stream out;
            Voice mono; // no struct named Voice
            process { out = mono.phase; }
        }
        graph G { input event midi; output stream y; node p = P; connection { p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("Unknown struct type 'Voice'"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, RejectsUnknownStructField)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            output stream out;
            struct Voice { float phase; }
            Voice mono;
            process { out = mono.volume; } // no such field
        }
        graph G { input event midi; output stream y; node p = P; connection { p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("has no field 'volume'"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, ResetsStateIndexAcrossProcessors)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor A {
            output stream out;
            struct Voice { float phase; }
            state Voice v;
            process { out = v.phase; }
        }
        processor B {
            output stream out;
            state int x;
            process { out = float (x.bogus); } // x is a plain int state, not struct-typed
        }
        graph G { output stream y; output stream z; node a = A; node b = B; connection { a.out -> y; b.out -> z; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("'x' is not a struct-typed state"))
            found = true;

    EXPECT_TRUE (found) << diagnostics.toString();
}

TEST (YdspSemanticAnalyzerTests, RejectsStreamAccessInInit)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float gain;
            init { gain = in; } // streams are not accessible during init
            process { out = in * gain; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("Streams are not accessible during init"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, RejectsDelayPrimitiveInInit)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            output stream out;
            state float mem;
            init { mem = mem'; } // ' is only allowed in the per-sample body
            process { out = mem; }
        }
        graph G { input event midi; output stream y; node p = P; connection { p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsBareStructStateAccess)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            output stream out;
            struct Voice { float phase; }
            Voice mono;
            process { out = mono; } // must use state.field
        }
        graph G { input event midi; output stream y; node p = P; connection { p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsIndexingScalarStructField)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            output stream out;
            struct Voice { float phase; }
            Voice mono;
            process { out = mono.phase[1]; } // scalar field cannot be indexed
        }
        graph G { input event midi; output stream y; node p = P; connection { p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsIntrinsicOnNonFloat)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                int i = 2;
                out = sin (i);   // sin requires float operands
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, AcceptsBitwiseOpsOnInts)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                int a = 60;
                int64 b = int64(-1);
                let c = a & 15;
                let d = a | 240;
                let ex = a ^ 85;
                let f = a << 3;
                let g = a >> 2;
                let h = ~a;
                let i = b & int64(a);
                let j = b >> 63;
                out = float (int64(c + d + ex + f + g + h) + i + j);
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, analyzed);
    EXPECT_TRUE (analyzed->processors[0].provenRealtimeSafe);
}

TEST (YdspSemanticAnalyzerTests, RejectsBitwiseOpsOnFloats)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                out = in & 3;   // float32 & int32 is not allowed
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("Bitwise operators require integer operands"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, RejectsBitwiseNotOnFloats)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                out = ~in;   // float32 has no bitwise complement
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("'~' requires an integer operand"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, RejectsMixedWidthBitwiseOps)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                int64 x = int64(7);
                out = float (x ^ in);   // int64 ^ float32 is not allowed
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

//==============================================================================
// Event endpoints, handlers and voice banks

TEST (YdspSemanticAnalyzerTests, AnalyzesStreamFreeParameterProcessor)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Voice {
            output stream out;
            input value float decay = 0.25;
            process { out = decay; }
        }
        graph G { output stream y; node v = Voice; connection { v.out -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    ASSERT_EQ (1u, analyzed->graph.nodes.size());
    EXPECT_EQ (1u, analyzed->graph.nodes[0].paramDefaults.size());
}

TEST (YdspSemanticAnalyzerTests, AnalyzesEventDrivenProcessor)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Voice {
            output stream out;
            input value float decay = 0.25;
            input event midi;
            state float freq;
            state float env;
            event midi (e: noteOn) {
                freq = e.pitch;
                env = e.velocity * decay;
            }
            event midi (e: noteOff) {
                env = 0.0;
            }
            process { out = env; }
        }
        graph G {
            input event midi;
            output stream y;
            node v = Voice[4];
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    ASSERT_EQ (1u, analyzed->processors.size());

    const auto& processor = analyzed->processors[0];
    EXPECT_EQ (1u, processor.inputEvents.size());
    ASSERT_EQ (2u, processor.eventHandlers.size());
    EXPECT_EQ (YdspEventShape::noteOn, processor.eventHandlers[0].shape);
    EXPECT_EQ (YdspEventShape::noteOff, processor.eventHandlers[1].shape);

    ASSERT_EQ (1u, analyzed->graph.nodes.size());
    EXPECT_EQ (4, analyzed->graph.nodes[0].voiceCount);
    EXPECT_TRUE (analyzed->graph.nodes[0].isEventDriven);
}

TEST (YdspSemanticAnalyzerTests, ResolvesEventFieldsToFloat32)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Voice {
            output stream out;
            input event midi;
            state float f;
            event midi (e: noteOn) {
                f = e.pitch + e.velocity;
            }
            process { out = f; }
        }
        graph G { input event midi; output stream y; node v = Voice; connection { midi -> v.midi; v.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    EXPECT_NE (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, AcceptsNamedEventInputsAtProcessorScope)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input event midi1;
            input event midi2;
            process { }
        }
        graph G { input stream x; output stream y; connection { x -> y; } }
    )YDSP",
             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, AcceptsSingleMidiInputAtProcessorScope)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input event midi;
            process { }
        }
        graph G { input stream x; output stream y; connection { x -> y; } }
    )YDSP",
             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsDuplicateEventEndpoint)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input event midi;
            input event midi;
            process { }
        }
        graph G { output stream y; connection { } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("Duplicate event input 'midi'"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, RejectsAnUnconnectedNodeEventInput)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input event midi;
            output stream out;
            event midi (e: noteOn) { }
            process { out = 0; }
        }
        graph G {
            input event other;
            output stream y;
            node p = P;
            connection { p.out -> y; }
        }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("Node 'p' input event 'midi' is not connected: it must be driven by at least one source"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, RejectsDuplicateMidiEndpoint)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P { process { } }
        graph G {
            input event midi;
            input event midi;
            output stream y;
            node p = P;
        }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("Duplicate event input 'midi'"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, AcceptsSameShapeOnDifferentEventInputs)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input event midi1;
            input event midi2;
            output stream out;
            state float f;
            event midi1 (e: noteOn) { f = e.pitch; }
            event midi2 (e: noteOn) { f = 0.0; }
            process { out = f; }
        }
        graph G {
            input event midi1;
            input event midi2;
            output stream y;
            node p = P;
            connection { midi1 -> p.midi1; midi2 -> p.midi2; p.out -> y; }
        }
    )YDSP",
             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
}

TEST (YdspSemanticAnalyzerTests, RejectsHandlerForUnknownEndpoint)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input event midi;
            event other (e: noteOn) { }
            process { }
        }
        graph G { output stream y; connection { } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("unknown event endpoint 'other'"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, RejectsUnknownShapeInEventHandler)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input event midi;
            event midi (e: foo) { }
            process { }
        }
        graph G { output stream y; connection { } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("Unknown event shape 'foo'"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, RejectsDuplicateShapeHandlerOnSameInput)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input event midi;
            event midi (e: noteOn) { }
            event midi (f: noteOn) { }
            process { }
        }
        graph G { output stream y; connection { } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("Duplicate event handler for shape 'noteOn' on input 'midi'"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, RejectsEventFieldAccess)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input event midi;
            state float f;
            event midi (e: noteOn) {
                f = e.bogus;
            }
            process { }
        }
        graph G { output stream y; connection { } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("Event 'noteOn' has no field 'bogus'")
            && diagnostics.getItem (i).message.contains ("'pitch', 'velocity', 'bendSemitones'"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, AcceptsEveryProcessorScopeEventShape)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Voice {
            output stream out;
            input event midi;
            state float f;
            state int i;
            event midi (e: noteOn)        { f = e.pitch + e.velocity + e.bendSemitones; if (e.isLegato) { f = 0.0; } }
            event midi (e: noteOff)       { f = e.pitch + e.velocity; }
            event midi (e: pitchBend)     { f = e.bendSemitones; }
            event midi (e: pressure)      { f = e.pressure; }
            event midi (e: slide)         { f = e.slide; }
            event midi (e: controlChange) { i = e.control; f = e.value; }
            event midi (e: programChange) { i = e.program; }
            process { out = f; }
        }
        graph G { input event midi; output stream y; node v = Voice; connection { midi -> v.midi; v.out -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    ASSERT_EQ (1u, analyzed->processors.size());

    const auto& processor = analyzed->processors[0];
    ASSERT_EQ (7u, processor.eventHandlers.size());

    EXPECT_EQ (YdspEventShape::noteOn, processor.eventHandlers[0].shape);
    EXPECT_EQ (YdspEventShape::noteOff, processor.eventHandlers[1].shape);
    EXPECT_EQ (YdspEventShape::pitchBend, processor.eventHandlers[2].shape);
    EXPECT_EQ (YdspEventShape::pressure, processor.eventHandlers[3].shape);
    EXPECT_EQ (YdspEventShape::slide, processor.eventHandlers[4].shape);
    EXPECT_EQ (YdspEventShape::controlChange, processor.eventHandlers[5].shape);
    EXPECT_EQ (YdspEventShape::programChange, processor.eventHandlers[6].shape);
}

TEST (YdspSemanticAnalyzerTests, RejectsFieldBelongingToAnotherShape)
{
    struct Case
    {
        const char* shape;
        const char* field;
        const char* expectedFields;
    };

    for (const auto& testCase : { Case { "noteOff", "isLegato", "'pitch', 'velocity'" },
                                  Case { "pitchBend", "pitch", "'bendSemitones'" },
                                  Case { "controlChange", "pressure", "'control', 'value'" } })
    {
        YdspDiagnostics diagnostics;

        analyze ("processor P { output stream out; input event midi; state float f; event midi (e: " + String (testCase.shape) + ") { f = e." + String (testCase.field) + "; } process { out = f; } } graph G { input event midi; output stream y; node p = P; connection { p.out -> y; } }",
                 diagnostics);

        bool found = false;
        for (int i = 0; i < diagnostics.getCount(); ++i)
            if (diagnostics.getItem (i).message.contains (String ("Event '") + testCase.shape + "' has no field '" + testCase.field + "'")
                && diagnostics.getItem (i).message.contains (testCase.expectedFields))
                found = true;

        EXPECT_TRUE (found) << testCase.shape << "." << testCase.field << ": " << diagnostics.toString();
    }
}

TEST (YdspSemanticAnalyzerTests, ParsesVoiceModeAnnotationsOnNodes)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Voice {
            output stream out;
            input event midi;
            state float f;
            event midi (e: noteOn) { f = e.pitch; }
            process { out = f; }
        }
        graph G {
            input event midi;
            output stream leadOut;
            output stream bassOut;
            node lead = Voice[4] [[ mode: poly, stealing: newest ]];
            node bass = Voice    [[ mode: mono, priority: low ]];
            connection { midi -> lead.midi; midi -> bass.midi; lead.out -> leadOut; bass.out -> bassOut; }
        }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    ASSERT_EQ (2u, analyzed->graph.nodes.size());

    EXPECT_EQ (YdspVoiceMode::poly, analyzed->graph.nodes[0].voiceMode);
    EXPECT_EQ (YdspVoiceStealing::newest, analyzed->graph.nodes[0].stealing);

    EXPECT_EQ (YdspVoiceMode::mono, analyzed->graph.nodes[1].voiceMode);
    EXPECT_EQ (YdspMonoPriority::low, analyzed->graph.nodes[1].monoPriority);

    // Unannotated defaults must reproduce the previous behaviour exactly.
    EXPECT_EQ (YdspVoiceStealing::oldest, analyzed->graph.nodes[1].stealing);
    EXPECT_EQ (YdspMonoPriority::last, analyzed->graph.nodes[0].monoPriority);
}

TEST (YdspSemanticAnalyzerTests, RejectsMonoModeWithAVoiceBank)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor Voice {
            output stream out;
            input event midi;
            state float f;
            event midi (e: noteOn) { f = e.pitch; }
            process { out = f; }
        }
        graph G {
            input event midi;
            output stream y;
            node v = Voice[4] [[ mode: mono ]];
            connection { v.out -> y; }
        }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("'mode: mono' and must declare exactly one voice"))
            found = true;

    EXPECT_TRUE (found) << diagnostics.toString();
}

TEST (YdspSemanticAnalyzerTests, RejectsUnknownNodeAnnotations)
{
    struct Case
    {
        const char* annotation;
        const char* expected;
    };

    for (const auto& testCase : { Case { "mode: duo", "Unknown voice mode 'duo'" },
                                  Case { "stealing: quietest", "Unknown stealing policy 'quietest'" },
                                  Case { "priority: middle", "Unknown note priority 'middle'" },
                                  Case { "glide: 5", "Unknown node annotation 'glide'" } })
    {
        YdspDiagnostics diagnostics;

        analyze ("processor Voice { output stream out; input event midi; state float f; event midi (e: noteOn) { f = e.pitch; } process { out = f; } } graph G { input event midi; output stream y; node v = Voice [[ " + String (testCase.annotation) + " ]]; connection { v.out -> y; } }",
                 diagnostics);

        bool found = false;
        for (int i = 0; i < diagnostics.getCount(); ++i)
            if (diagnostics.getItem (i).message.contains (testCase.expected))
                found = true;

        EXPECT_TRUE (found) << testCase.annotation << ": " << diagnostics.toString();
    }
}

TEST (YdspSemanticAnalyzerTests, RejectsBareEventValue)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input event midi;
            state float f;
            event midi (e: noteOn) {
                f = e;
            }
            process { }
        }
        graph G { output stream y; connection { } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("must be accessed via a member"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, RejectsAssignmentToEventValue)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input event midi;
            event midi (e: noteOn) {
                e.pitch = 3.0;
            }
            process { }
        }
        graph G { output stream y; connection { } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("Cannot assign to the event value"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, RejectsDelayPrimitiveInEventHandler)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input event midi;
            state float f;
            event midi (e: noteOn) {
                f = e.pitch' ;
            }
            process { }
        }
        graph G { output stream y; connection { } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("Delay primitives are not available inside event handlers"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, RejectsSmoothInEventHandler)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input event midi;
            input value float gain = 0.5;
            state float f;
            event midi (e: noteOn) {
                f = smooth (gain, 0.02);
            }
            process { }
        }
        graph G { output stream y; connection { } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("smooth() is not available inside event handlers"))
            found = true;

    EXPECT_TRUE (found) << diagnostics.toString();
}

TEST (YdspSemanticAnalyzerTests, RejectsSmoothInsideLoop)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            output stream out;
            input value float gain = 0.5;
            process {
                float acc = 0.0;
                for i in 0..4 { acc = acc + smooth (gain, 0.02); }
                out = acc;
            }
        }
        graph G { input event midi; output stream y; node p = P; connection { p.out -> y; } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("smooth() is only allowed in the per-sample body, outside loops"))
            found = true;

    EXPECT_TRUE (found) << diagnostics.toString();
}

TEST (YdspSemanticAnalyzerTests, RejectsSmoothInBlockModeBody)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            output stream out;
            input value float gain = 0.5;
            process block {
                out[0] = smooth (gain, 0.02);
            }
        }
        graph G { input event midi; output stream y; node p = P; connection { p.out -> y; } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("smooth() is only allowed in the per-sample body, outside loops"))
            found = true;

    EXPECT_TRUE (found) << diagnostics.toString();
}

TEST (YdspSemanticAnalyzerTests, RejectsSmoothWithWrongArity)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            output stream out;
            input value float gain = 0.5;
            process { out = smooth (gain); }
        }
        graph G { input event midi; output stream y; node p = P; connection { p.out -> y; } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("Function 'smooth' expects")
            && diagnostics.getItem (i).message.contains ("got 1"))
            found = true;

    EXPECT_TRUE (found) << diagnostics.toString();
}

TEST (YdspSemanticAnalyzerTests, RejectsSmoothWithNonFloatOperand)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            output stream out;
            input value int steps = 4;
            process { out = smooth (steps, 0.02); }
        }
        graph G { input event midi; output stream y; node p = P; connection { p.out -> y; } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("smooth() requires float32 operands"))
            found = true;

    EXPECT_TRUE (found) << diagnostics.toString();
}

TEST (YdspSemanticAnalyzerTests, AcceptsSmoothingAnnotationOnFloatParameter)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input value float gain = 0.5 [[ smoothing: 0.02 ]];
            process { out = in * gain; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);

    ASSERT_FALSE (analyzed->processors.empty());
    EXPECT_EQ (1, analyzed->processors[0].hiddenStateCount);
}

TEST (YdspSemanticAnalyzerTests, RejectsSmoothingAnnotationOnOutputValue)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            output value float meter [[ smoothing: 0.02 ]];
            process { out = in; meter = in; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("[[ smoothing ]] is only valid on an 'input value' parameter"))
            found = true;

    EXPECT_TRUE (found) << diagnostics.toString();
}

TEST (YdspSemanticAnalyzerTests, RejectsSmoothingAnnotationOnInputStream)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input stream in [[ smoothing: 0.02 ]];
            output stream out;
            process { out = in; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("[[ smoothing ]] is only valid on an 'input value' parameter"))
            found = true;

    EXPECT_TRUE (found) << diagnostics.toString();
}

TEST (YdspSemanticAnalyzerTests, RejectsSmoothingAnnotationOnNonFloat32Parameter)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input value int steps = 4 [[ smoothing: 0.02 ]];
            process { out = in * float (steps); }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("[[ smoothing ]] requires a float32 parameter"))
            found = true;

    EXPECT_TRUE (found) << diagnostics.toString();
}

TEST (YdspSemanticAnalyzerTests, RejectsSmoothingAnnotationWithNonPositiveTimeConstant)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input value float gain = 0.5 [[ smoothing: 0.0 ]];
            process { out = in * gain; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("[[ smoothing ]] requires a positive time constant"))
            found = true;

    EXPECT_TRUE (found) << diagnostics.toString();
}

TEST (YdspSemanticAnalyzerTests, RejectsSmoothingAnnotationInBlockModeProcessor)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input value float gain = 0.5 [[ smoothing: 0.02 ]];
            process block {
                for i in 0..blockSize { out[i] = in[i] * gain; }
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("[[ smoothing ]] requires a per-sample 'process { }' body"))
            found = true;

    EXPECT_TRUE (found) << diagnostics.toString();
}

TEST (YdspSemanticAnalyzerTests, RejectsSmoothingAnnotationOnGraphEndpoint)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input value float gain = 0.5;
            process { out = in * gain; }
        }
        graph G {
            input stream x;
            output stream y;
            input value float level = 0.5 [[ smoothing: 0.02 ]];
            node p = P;
            connection { x -> p.in; p.out -> y; }
        }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("[[ smoothing ]] is not available on a graph endpoint"))
            found = true;

    EXPECT_TRUE (found) << diagnostics.toString();
}

TEST (YdspSemanticAnalyzerTests, SmoothingAnnotationLeavesEventHandlersReadingTheRawTarget)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            output stream out;
            input event midi;
            input value float decay = 0.5 [[ smoothing: 0.02 ]];
            state float env;
            event midi (e: noteOn) { env = e.velocity * decay; }
            process { out = env * decay; }
        }
        graph G { input event midi; output stream y; node p = P; connection { midi -> p.midi; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    EXPECT_NE (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, RejectsStreamAccessInEventHandler)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input event midi;
            event midi (e: noteOn) {
                out = in;
            }
            process { out = in; }
        }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsBlockSizeInEventHandler)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input event midi;
            state float f;
            event midi (e: noteOn) {
                f = blockSize;
            }
            process { }
        }
        graph G { output stream y; connection { } }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspSemanticAnalyzerTests, RejectsNonConstantLoopBoundInEventHandler)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input event midi;
            state float f;
            event midi (e: noteOn) {
                for i in 0 .. blockSize {
                    f = f + 1;
                }
            }
            process { }
        }
        graph G { output stream y; connection { } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("Loop bounds in an event handler must be compile-time constants"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, AcceptsConstantLoopInEventHandler)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            output stream out;
            input event midi;
            state float f;
            event midi (e: noteOn) {
                for i in 0 .. 16 {
                    f = f + 1;
                }
            }
            process { out = 0; }
        }
        graph G {
            input event midi;
            output stream y;
            node p = P;
            connection { midi -> p.midi; p.out -> y; }
        }
    )YDSP",
                             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    EXPECT_NE (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, RejectsVoiceBankOnNonEventProcessor)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            output stream out;
            process { out = 0; }
        }
        graph G {
            output stream y;
            node v = P[8];
            connection { v.out -> y; }
        }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("declares a voice bank ([N]) but processor 'P' has no event input"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, RejectsVoiceBankWithOversampling)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor Voice {
            output stream out;
            input event midi;
            event midi (e: noteOn) { }
            process { out = 0; }
        }
        graph G {
            output stream y;
            node v = Voice[4] * 2;
            connection { v.out -> y; }
        }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("An event-driven node cannot use oversampling/undersampling"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, RejectsZeroVoiceCount)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor Voice {
            output stream out;
            input event midi;
            process { out = 0; }
        }
        graph G {
            output stream y;
            node v = Voice[0];
            connection { v.out -> y; }
        }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("must be a positive integer"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, RejectsEventParamShadowing)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input event midi;
            state float freq;
            event midi (freq: noteOn) {
                freq = 1.0;
            }
            process { }
        }
        graph G { output stream y; connection { } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("shadows an existing symbol"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, RejectsEventDrivenProcessorWithInputStreamButNoOutputStream)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input stream in;
            input event midi;
            event midi (e: noteOn) { }
            process { }
        }
        graph G {
            input stream x;
            output stream y;
            node p = P;
            connection { x -> p.in; x -> y; }
        }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("must declare exactly one output stream"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, AcceptsEventDrivenProcessorWithNoStreamsAtAll)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input event midi;
            output event noteOn;
            event midi (e: noteOn) {
                emit noteOn (pitch: e.pitch, velocity: e.velocity) -> noteOn;
            }
            process { }
        }
        graph G {
            input event midi;
            output event noteOn;
            node p = P;
            connection { midi -> p.midi; p.noteOn -> noteOn; }
        }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    EXPECT_TRUE (analyzed->graph.nodes[0].isEventDriven);
}

//==============================================================================
// Program-scope `let` constants

TEST (YdspSemanticAnalyzerTests, AcceptsTopLevelLetAsArraySizeLoopBoundAndValue)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        let taps = 8;
        let gain = 0.5;

        processor P {
            input stream in;
            output stream out;
            state float buf[taps];
            process {
                float sum = 0.0;
                for i in 0..taps { sum = sum + buf[i]; }
                buf[0] = in;
                out = sum * gain;
            }
        }
        graph G { input stream a; output stream b; node p = P; connection { a -> p.in; p.out -> b; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);

    ASSERT_EQ (1u, analyzed->processors.size());
    ASSERT_EQ (1u, analyzed->processors[0].states.size());
    EXPECT_EQ (8, analyzed->processors[0].states[0]->arraySize);

    ASSERT_EQ (1u, analyzed->processors[0].loops.size());
    EXPECT_EQ (YdspLoopBoundKind::constant, analyzed->processors[0].loops[0].bound.kind);
    EXPECT_EQ (8, analyzed->processors[0].loops[0].bound.constant);
    EXPECT_TRUE (analyzed->processors[0].provenRealtimeSafe);
}

TEST (YdspSemanticAnalyzerTests, FoldsLetDefinedFromEarlierConstants)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        let base = 4;
        let taps = base * 2 + 1;

        processor P {
            input stream in;
            output stream out;
            state float buf[taps];
            process { buf[0] = in; out = buf[0]; }
        }
        graph G { input stream a; output stream b; node p = P; connection { a -> p.in; p.out -> b; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    EXPECT_EQ (9, analyzed->processors[0].states[0]->arraySize);
}

TEST (YdspSemanticAnalyzerTests, RejectsNonConstantLet)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        let bad = sampleRate;
        processor P { input stream in; output stream out; process { out = in; } }
        graph G { input stream a; output stream b; node p = P; connection { a -> p.in; p.out -> b; } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("not a compile-time constant"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, RejectsDeclarationShadowingAProgramConstant)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        let taps = 8;
        processor P {
            input stream in;
            output stream out;
            state float taps;
            process { out = in; }
        }
        graph G { input stream a; output stream b; node p = P; connection { a -> p.in; p.out -> b; } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("redeclares the program constant"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, RejectsUnknownConstantAsArraySize)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float buf[nope];
            process { out = in; }
        }
        graph G { input stream a; output stream b; node p = P; connection { a -> p.in; p.out -> b; } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("Unknown program constant"))
            found = true;

    EXPECT_TRUE (found);
}

//==============================================================================
// State initialisers

TEST (YdspSemanticAnalyzerTests, LowersStateInitialisersIntoTheInitBlock)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float feedback = 0.5;
            state float table[4] = { 1.0, 2.0 };
            process { out = in * feedback + table[0]; }
        }
        graph G { input stream a; output stream b; node p = P; connection { a -> p.in; p.out -> b; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);

    const auto* decl = analyzed->processors[0].decl;
    ASSERT_NE (nullptr, decl->init);
    EXPECT_EQ (3u, decl->init->body.size());
}

TEST (YdspSemanticAnalyzerTests, StateInitialisersRunBeforeAnExplicitInitBlock)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float feedback = 0.5;
            init { feedback = feedback * 2.0; }
            process { out = in * feedback; }
        }
        graph G { input stream a; output stream b; node p = P; connection { a -> p.in; p.out -> b; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);

    const auto* decl = analyzed->processors[0].decl;
    ASSERT_NE (nullptr, decl->init);
    ASSERT_EQ (2u, decl->init->body.size());

    ASSERT_NE (nullptr, decl->init->body[0]->value);
    EXPECT_EQ (YdspExprKind::floatLiteral, decl->init->body[0]->value->kind);
}

TEST (YdspSemanticAnalyzerTests, RejectsTooManyStateInitialisers)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float table[2] = { 1.0, 2.0, 3.0 };
            process { out = in + table[0]; }
        }
        graph G { input stream a; output stream b; node p = P; connection { a -> p.in; p.out -> b; } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("but holds only"))
            found = true;

    EXPECT_TRUE (found);
}

//==============================================================================
// samplePeriod and [[ init: ... ]]

TEST (YdspSemanticAnalyzerTests, AcceptsSamplePeriodBuiltin)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float phase;
            input event midi;
            event midi (e: noteOn) { phase = samplePeriod; }
            process { phase = phase + 100.0 * samplePeriod; out = in * phase; }
        }
        graph G { input event midi; input stream a; output stream b; node p = P; connection { midi -> p.midi; a -> p.in; p.out -> b; } }
    )YDSP",
                             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    EXPECT_NE (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, InitAnnotationSuppliesTheParameterDefault)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P { input stream in; output stream out; process { out = in; } }
        graph G {
            input stream a;
            output stream b;
            input value float rate [[ name: "Rate", min: 0.5, max: 12.0, init: 4 ]];
            input value float depth = 0.25 [[ init: 9 ]];
            node p = P;
            connection { a -> p.in; p.out -> b; }
        }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    ASSERT_EQ (2u, analyzed->graph.inputValueDefaults.size());

    EXPECT_DOUBLE_EQ (4.0, analyzed->graph.inputValueDefaults[0].asDouble);
    EXPECT_DOUBLE_EQ (0.25, analyzed->graph.inputValueDefaults[1].asDouble);
}

//==============================================================================
// Loop-variable scoping

TEST (YdspSemanticAnalyzerTests, SiblingLoopsMayReuseTheSameVariableName)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float a[4];
            state float b[4];
            process {
                for i in 0..4 { a[i] = in; }
                for i in 0..4 { b[i] = a[i]; }
                out = b[0];
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    EXPECT_NE (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, RejectsLoopVariableUseAfterTheLoop)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float a[4];
            process {
                for i in 0..4 { a[i] = in; }
                out = a[i];
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("Unknown symbol 'i'"))
            found = true;

    EXPECT_TRUE (found);
}

//==============================================================================

TEST (YdspSemanticAnalyzerTests, RejectsANestedBlockRedeclaringAnOuterLocal)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                let x = 1.0;
                { let x = 2.0; }
                out = in * x;
            }
        }
        graph G { input stream a; output stream b; node p = P; connection { a -> p.in; p.out -> b; } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("Duplicate symbol 'x'"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspSemanticAnalyzerTests, SiblingIfElseBranchesMayReuseTheSameLocalName)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                if (in > 0.0) { let x = 1.0; out = in + x; }
                else { let x = -1.0; out = in + x; }
            }
        }
        graph G { input stream a; output stream b; node p = P; connection { a -> p.in; p.out -> b; } }
    )YDSP",
                             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    EXPECT_NE (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, SiblingIfElseBranchesMayDeclareDifferentTypesForTheSameName)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                if (in > 0.0) { let x = 1.0; out = in + x; }
                else { let x = 1; out = in + float (x); }
            }
        }
        graph G { input stream a; output stream b; node p = P; connection { a -> p.in; p.out -> b; } }
    )YDSP",
                             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    EXPECT_NE (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, SiblingPlainBlocksMayReuseTheSameLocalName)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                { let x = 1.0; out = in + x; }
                { let x = 2.0; out = in - x; }
            }
        }
        graph G { input stream a; output stream b; node p = P; connection { a -> p.in; p.out -> b; } }
    )YDSP",
                             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    EXPECT_NE (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, RejectsUseOfABlockScopedLocalAfterItsBlockEnds)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                if (in > 0.0) { let x = 1.0; out = x; }
                out = x;
            }
        }
        graph G { input stream a; output stream b; node p = P; connection { a -> p.in; p.out -> b; } }
    )YDSP",
             diagnostics);

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains ("Unknown symbol 'x'"))
            found = true;

    EXPECT_TRUE (found);
}

//==============================================================================
// The [[ role: voiceActivity ]] state annotation.

namespace
{

/** A voice processor with `activityDecl` spliced in as its only extra state. */
String voiceActivitySource (StringRef activityDecl)
{
    return String (R"YDSP(
        processor V {
            output stream out;
            input event midi;
            state float env;
            )YDSP")
         + activityDecl
         + R"YDSP(
            event midi (e: noteOn) { env = e.velocity; }
            event midi (e: noteOff) { env = 0.0; }
            process { out = env; }
        }
        graph G { input event midi; output stream y; node v = V[4]; connection { midi -> v.midi; v.out -> y; } }
    )YDSP";
}

/** Returns true if any diagnostic message contains `fragment`. */
bool anyDiagnosticContains (const YdspDiagnostics& diagnostics, StringRef fragment)
{
    for (int i = 0; i < diagnostics.getCount(); ++i)
        if (diagnostics.getItem (i).message.contains (fragment))
            return true;

    return false;
}

} // namespace

TEST (YdspSemanticAnalyzerTests, RecordsVoiceActivityState)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (voiceActivitySource ("state int active [[ role: voiceActivity ]];"), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    ASSERT_EQ (1u, analyzed->processors.size());

    const auto* activityState = analyzed->processors[0].activityState;
    ASSERT_NE (nullptr, activityState);
    EXPECT_EQ ("active", activityState->name);
    EXPECT_EQ (YdspPrimitiveType::int32Type, activityState->type);
}

TEST (YdspSemanticAnalyzerTests, LeavesVoiceActivityUnsetWithoutTheAnnotation)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (voiceActivitySource ("state int active;"), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    ASSERT_EQ (1u, analyzed->processors.size());

    EXPECT_EQ (nullptr, analyzed->processors[0].activityState);
}

TEST (YdspSemanticAnalyzerTests, RejectsUnknownStateRole)
{
    YdspDiagnostics diagnostics;

    analyze (voiceActivitySource ("state int active [[ role: whatever ]];"), diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "Unknown state role"));
}

TEST (YdspSemanticAnalyzerTests, RejectsUnknownStateAnnotationKey)
{
    YdspDiagnostics diagnostics;

    analyze (voiceActivitySource ("state int active [[ rol: voiceActivity ]];"), diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "Unknown state annotation"));
}

TEST (YdspSemanticAnalyzerTests, RejectsTwoVoiceActivityStates)
{
    YdspDiagnostics diagnostics;

    analyze (voiceActivitySource ("state int active [[ role: voiceActivity ]];\n"
                                  "            state int alsoActive [[ role: voiceActivity ]];"),
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "more than one 'voiceActivity' state"));
}

TEST (YdspSemanticAnalyzerTests, RejectsNonIntVoiceActivityState)
{
    YdspDiagnostics diagnostics;

    analyze (voiceActivitySource ("state float active [[ role: voiceActivity ]];"), diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "must be a scalar 'int'"));
}

TEST (YdspSemanticAnalyzerTests, RejectsBoolVoiceActivityState)
{
    YdspDiagnostics diagnostics;

    analyze (voiceActivitySource ("state bool active [[ role: voiceActivity ]];"), diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "must be a scalar 'int'"));
}

TEST (YdspSemanticAnalyzerTests, RejectsArrayVoiceActivityState)
{
    YdspDiagnostics diagnostics;

    analyze (voiceActivitySource ("state int active[4] [[ role: voiceActivity ]];"), diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "must be a scalar 'int'"));
}

TEST (YdspSemanticAnalyzerTests, RejectsInitialisedVoiceActivityState)
{
    YdspDiagnostics diagnostics;

    analyze (voiceActivitySource ("state int active = 1 [[ role: voiceActivity ]];"), diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "must not have an initialiser"));
}

TEST (YdspSemanticAnalyzerTests, RejectsVoiceActivityStateWithoutEventHandlers)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state int active [[ role: voiceActivity ]];
            process { out = in; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "at least one event handler"));
}

//==============================================================================
// Rate changes

TEST (YdspSemanticAnalyzerTests, AcceptsUndersamplingOnANode)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Sat { input stream in; output stream out; process { out = in * 0.5; } }
        graph G { input stream x; output stream y; node s = Sat / 2; connection { x -> s.in; s.out -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    ASSERT_EQ (1u, analyzed->graph.nodes.size());
    EXPECT_EQ (2, analyzed->graph.nodes[0].rateDivider);
    EXPECT_EQ (ydspOversamplerLatencySamples * 2 + 1, analyzed->graph.latencySamples);
}

TEST (YdspSemanticAnalyzerTests, RejectsUndersamplingOnANonFloat32Stream)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor Sat { input stream float64 in; output stream float64 out; process { out = in * 0.5; } }
        graph G { input stream float64 x; output stream float64 y; node s = Sat / 4; connection { x -> s.in; s.out -> y; } }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "a rate change (*N or /N) is only supported on float32 streams"));
}

TEST (YdspSemanticAnalyzerTests, RejectsOversamplingOnANonFloat32Stream)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor Sat { input stream float64 in; output stream float64 out; process { out = in * 0.5; } }
        graph G { input stream float64 x; output stream float64 y; node s = Sat * 4; connection { x -> s.in; s.out -> y; } }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "a rate change (*N or /N) is only supported on float32 streams"));
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "declares stream 'in' as float64"));
}

TEST (YdspSemanticAnalyzerTests, AcceptsOversamplingOnFloat32Streams)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Sat { input stream in; output stream out; process { out = in * 0.5; } }
        graph G { input stream x; output stream y; node s = Sat * 4; connection { x -> s.in; s.out -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    ASSERT_EQ (1u, analyzed->graph.nodes.size());
    EXPECT_EQ (4, analyzed->graph.nodes[0].rateMultiplier);
}

//==============================================================================
// Feedback cycles

TEST (YdspSemanticAnalyzerTests, RejectsAFeedbackCycleWithoutOfferingADelayAsTheFix)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor Fork { input stream a; input stream b; output stream c; output stream d; process { c = a + b; d = a - b; } }
        processor Pass { input stream in; output stream out; process { out = in; } }
        graph G {
            input stream x;
            output stream y;
            node f = Fork;
            node p = Pass;
            connection { x -> f.a; p.out -> f.b; f.c -> p.in; f.d -> y; }
        }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "feedback cycle, which is not supported in this version"));
    EXPECT_FALSE (anyDiagnosticContains (diagnostics, "cycle without a delay"));
}

//==============================================================================
// Algebra-form shapes that used to read past the end of a port vector

TEST (YdspSemanticAnalyzerTests, RejectsSequencingPastAGraphOutputStream)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor Gain { input stream in; output stream out; process { out = in * 2; } }
        graph G { input stream dry; output stream wet; process = dry : wet : Gain; }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "left side of ':' has no output to connect from"));
}

TEST (YdspSemanticAnalyzerTests, RejectsSequencingIntoAGraphInputStream)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor Gain { input stream in; output stream out; process { out = in * 2; } }
        graph G { input stream dry; output stream wet; process = Gain : dry; }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "right side of ':' has no input to connect to"));
}

TEST (YdspSemanticAnalyzerTests, RejectsIdentityMixedWithAProcessorInsideParallel)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor Gain { input stream in; output stream out; process { out = in * 2; } }
        graph G {
            input stream a;
            input stream b;
            output stream c;
            output stream d;
            process = (a , b) : (_ , Gain) : (c , d);
        }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "'_' cannot be combined with another operand inside ','"));
}

TEST (YdspSemanticAnalyzerTests, RejectsIdentityOnEitherSideOfParallel)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor Gain { input stream in; output stream out; process { out = in * 2; } }
        graph G {
            input stream a;
            input stream b;
            output stream c;
            output stream d;
            process = (a , b) : (Gain , _) : (c , d);
        }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "'_' cannot be combined with another operand inside ','"));
}

TEST (YdspSemanticAnalyzerTests, StillAcceptsIdentityOnBothSidesOfParallel)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Gain { input stream in; output stream out; process { out = in * 2; } }
        graph G {
            input stream a;
            input stream b;
            output stream c;
            output stream d;
            process = (a , b) : (_ , _) : (Gain , Gain) : (c , d);
        }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    EXPECT_EQ (4u, analyzed->graph.edges.size());
}

//==============================================================================
// Fan-out, summing fan-in, and the zero-use rejections that replaced "connected exactly once"

namespace
{

constexpr const char* fanGainProcessor =
    "processor Gain { input stream in; output stream out; input value float g = 1.0; process { out = in * g; } }\n"
    "processor Mix { input stream a; input stream b; output stream out; process { out = a + b; } }\n";

std::unique_ptr<YdspAnalyzedProgram> analyzeFan (StringRef source, YdspDiagnostics& diagnostics)
{
    return analyze (String (fanGainProcessor) + source, diagnostics);
}

} // namespace

TEST (YdspSemanticAnalyzerTests, AcceptsFanOutFromAGraphInput)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyzeFan (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node a = Gain (g = 2.0);
            node b = Gain (g = 3.0);
            node m = Mix;
            connection { x -> a.in; x -> b.in; a.out -> m.a; b.out -> m.b; m.out -> y; }
        }
    )YDSP",
                                diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);

    int fromInput = 0;

    for (const auto& edge : analyzed->graph.edges)
        if (edge.srcNode == -1 && edge.srcStream == 0)
            ++fromInput;

    EXPECT_EQ (2, fromInput);
}

TEST (YdspSemanticAnalyzerTests, AcceptsFanOutFromANodeOutput)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyzeFan (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            output stream tap;
            node a = Gain (g = 2.0);
            node b = Gain (g = 3.0);
            connection { x -> a.in; a.out -> b.in; a.out -> tap; b.out -> y; }
        }
    )YDSP",
                                diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);

    int fromA = 0;

    for (const auto& edge : analyzed->graph.edges)
        if (edge.srcNode == 0 && edge.srcStream == 0)
            ++fromA;

    EXPECT_EQ (2, fromA);
}

TEST (YdspSemanticAnalyzerTests, AcceptsSummingFanInIntoANodeInput)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyzeFan (R"YDSP(
        graph G {
            input stream x;
            input stream w;
            output stream y;
            node a = Gain (g = 1.0);
            connection { x -> a.in; w -> a.in; a.out -> y; }
        }
    )YDSP",
                                diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);

    int intoA = 0;

    for (const auto& edge : analyzed->graph.edges)
        if (edge.dstNode == 0 && edge.dstStream == 0)
            ++intoA;

    EXPECT_EQ (2, intoA);
}

TEST (YdspSemanticAnalyzerTests, AcceptsSummingFanInIntoAGraphOutput)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyzeFan (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node a = Gain (g = 2.0);
            node b = Gain (g = 3.0);
            connection { x -> a.in; x -> b.in; a.out -> y; b.out -> y; }
        }
    )YDSP",
                                diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);

    int intoY = 0;

    for (const auto& edge : analyzed->graph.edges)
        if (edge.dstNode == -1 && edge.dstStream == 0)
            ++intoY;

    EXPECT_EQ (2, intoY);
}

TEST (YdspSemanticAnalyzerTests, RejectsAnUnconnectedGraphInput)
{
    YdspDiagnostics diagnostics;

    analyzeFan (R"YDSP(
        graph G {
            input stream x;
            input stream unused;
            output stream y;
            node a = Gain (g = 1.0);
            connection { x -> a.in; a.out -> y; }
        }
    )YDSP",
                diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "Graph input 'unused' is not connected: it must feed at least one destination"));
}

TEST (YdspSemanticAnalyzerTests, RejectsAnUnconnectedGraphOutput)
{
    YdspDiagnostics diagnostics;

    analyzeFan (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            output stream silent;
            node a = Gain (g = 1.0);
            connection { x -> a.in; a.out -> y; }
        }
    )YDSP",
                diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "Graph output 'silent' is not connected: it must be driven by at least one source"));
}

TEST (YdspSemanticAnalyzerTests, RejectsAnUnconnectedNodeInput)
{
    YdspDiagnostics diagnostics;

    analyzeFan (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node m = Mix;
            connection { x -> m.a; m.out -> y; }
        }
    )YDSP",
                diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "Node 'm' input 'b' is not connected: it must be driven by at least one source"));
}

TEST (YdspSemanticAnalyzerTests, RejectsAnUnconnectedNodeOutput)
{
    YdspDiagnostics diagnostics;

    analyzeFan (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node a = Gain (g = 1.0);
            node dangling = Gain (g = 1.0);
            connection { x -> a.in; a.out -> y; x -> dangling.in; }
        }
    )YDSP",
                diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "Node 'dangling' output 'out' is not connected: it must feed at least one destination"));
}

TEST (YdspSemanticAnalyzerTests, RejectsSummingFanInOnANonFloatStream)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor Bump { input stream int32 in; output stream int32 out; process { out = in + 1; } }
        graph G {
            input stream int32 x;
            input stream int32 w;
            output stream int32 y;
            node a = Bump;
            connection { x -> a.in; w -> a.in; a.out -> y; }
        }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "is driven by 2 sources, but implicit summing is only supported on float32 and float64 streams (this one is int32)"));
}

TEST (YdspSemanticAnalyzerTests, AcceptsFanOutOnANonFloatStream)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Bump { input stream int32 in; output stream int32 out; process { out = in + 1; } }
        graph G {
            input stream int32 x;
            output stream int32 y;
            output stream int32 z;
            node a = Bump;
            node b = Bump;
            connection { x -> a.in; x -> b.in; a.out -> y; b.out -> z; }
        }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    EXPECT_EQ (4u, analyzed->graph.edges.size());
}

TEST (YdspSemanticAnalyzerTests, AcceptsSummingFanInOnAFloat64Stream)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Wide { input stream float64 in; output stream float64 out; process { out = in * 2.0; } }
        graph G {
            input stream float64 x;
            output stream float64 y;
            node a = Wide;
            node b = Wide;
            connection { x -> a.in; x -> b.in; a.out -> y; b.out -> y; }
        }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    EXPECT_EQ (4u, analyzed->graph.edges.size());
}

//==============================================================================
// The split and merge algebra operators

namespace
{

constexpr const char* splitMergeProcessors =
    "processor Gain { input stream in; output stream out; input value float g = 1.0; process { out = in * g; } }\n"
    "processor Fork { input stream in; output stream a; output stream b; process { a = in; b = in * 0.5; } }\n"
    "processor Pair { input stream a; input stream b; output stream out; process { out = a + b; } }\n";

std::unique_ptr<YdspAnalyzedProgram> analyzeSplitMerge (StringRef source, YdspDiagnostics& diagnostics)
{
    return analyze (String (splitMergeProcessors) + source, diagnostics);
}

int countEdgesFromGraphInput (const YdspAnalyzedGraph& graph, int index)
{
    int count = 0;

    for (const auto& edge : graph.edges)
        if (edge.srcNode == -1 && edge.srcStream == index)
            ++count;

    return count;
}

int countEdgesIntoGraphOutput (const YdspAnalyzedGraph& graph, int index)
{
    int count = 0;

    for (const auto& edge : graph.edges)
        if (edge.dstNode == -1 && edge.dstStream == index)
            ++count;

    return count;
}

} // namespace

TEST (YdspSemanticAnalyzerTests, SplitsAndMergesAroundAParallelPair)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyzeSplitMerge (R"YDSP(
        graph G {
            input stream dry;
            output stream wet;
            process = dry <: (Gain (g = 2.0) , Gain (g = 3.0)) :> wet;
        }
    )YDSP",
                                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);

    EXPECT_EQ (4u, analyzed->graph.edges.size());
    EXPECT_EQ (2, countEdgesFromGraphInput (analyzed->graph, 0));
    EXPECT_EQ (2, countEdgesIntoGraphOutput (analyzed->graph, 0));
}

TEST (YdspSemanticAnalyzerTests, SplitRepeatsTheSourceChannelsCyclically)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyzeSplitMerge (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            process = x : Fork <: (Pair , Pair) :> y;
        }
    )YDSP",
                                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);

    EXPECT_EQ (7u, analyzed->graph.edges.size());
    EXPECT_EQ (2, countEdgesIntoGraphOutput (analyzed->graph, 0));

    int fromForkA = 0;
    int fromForkB = 0;

    for (const auto& edge : analyzed->graph.edges)
    {
        if (edge.srcNode < 0)
            continue;

        if (analyzed->graph.nodes[static_cast<size_t> (edge.srcNode)].targetName() != "Fork")
            continue;

        if (edge.srcStream == 0)
            ++fromForkA;
        else
            ++fromForkB;
    }

    EXPECT_EQ (2, fromForkA);
    EXPECT_EQ (2, fromForkB);
}

TEST (YdspSemanticAnalyzerTests, RejectsANonDividingSplitArity)
{
    YdspDiagnostics diagnostics;

    analyzeSplitMerge (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            process = x : Fork <: (Gain , Gain , Gain) :> y;
        }
    )YDSP",
                       diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "Arity mismatch in '<:' : the left side has 2 outputs, which does not divide the right side's 3 inputs"));
}

TEST (YdspSemanticAnalyzerTests, RejectsANonDividingMergeArity)
{
    YdspDiagnostics diagnostics;

    analyzeSplitMerge (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            process = x <: (Gain , Gain , Gain) :> Pair : y;
        }
    )YDSP",
                       diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "Arity mismatch in ':>' : the right side has 2 inputs, which does not divide the left side's 3 outputs"));
}

TEST (YdspSemanticAnalyzerTests, IdentityDefaultsToArityOneOnTheSplitSide)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyzeSplitMerge (R"YDSP(
        graph G {
            input stream dry;
            output stream wet;
            process = dry : _ <: (Gain (g = 2.0) , Gain (g = 3.0)) :> wet;
        }
    )YDSP",
                                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);

    EXPECT_EQ (4u, analyzed->graph.edges.size());
    EXPECT_EQ (2, countEdgesFromGraphInput (analyzed->graph, 0));
    EXPECT_EQ (2, countEdgesIntoGraphOutput (analyzed->graph, 0));
}

TEST (YdspSemanticAnalyzerTests, IdentityDefaultsToArityOneOnTheMergeSide)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyzeSplitMerge (R"YDSP(
        graph G {
            input stream dry;
            output stream wet;
            process = dry <: (Gain (g = 2.0) , Gain (g = 3.0)) :> _ : wet;
        }
    )YDSP",
                                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);

    EXPECT_EQ (4u, analyzed->graph.edges.size());
    EXPECT_EQ (2, countEdgesFromGraphInput (analyzed->graph, 0));
    EXPECT_EQ (2, countEdgesIntoGraphOutput (analyzed->graph, 0));
}

TEST (YdspSemanticAnalyzerTests, SequentialCompositionStillRequiresEqualArities)
{
    YdspDiagnostics diagnostics;

    analyzeSplitMerge (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            process = x : Pair : y;
        }
    )YDSP",
                       diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "Arity mismatch in ':' : the left side has 1 outputs but the right side has 2 inputs"));
}

//==============================================================================
// output event endpoints and the emit statement

TEST (YdspSemanticAnalyzerTests, AcceptsOutputEventDeclarationMatchingShapeName)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            output event noteOn;
            process { }
        }
        graph G { input stream x; output stream y; connection { x -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    ASSERT_EQ (1u, analyzed->processors[0].outputEvents.size());
    EXPECT_EQ ("noteOn", analyzed->processors[0].outputEvents[0]->name);
}

TEST (YdspSemanticAnalyzerTests, AcceptsOutputEventDeclarationWithAnArbitraryChannelName)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            output event notAShapeName;
            process { emit noteOn (pitch: 60, velocity: 0.8) -> notAShapeName; }
        }
        graph G { input stream x; output stream y; connection { x -> y; } }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    ASSERT_EQ (1u, analyzed->processors[0].outputEvents.size());
    EXPECT_EQ ("notAShapeName", analyzed->processors[0].outputEvents[0]->name);
}

TEST (YdspSemanticAnalyzerTests, RegistersOutputEventAtGraphScope)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P { output event noteOn; process { emit noteOn (pitch: 60, velocity: 0.8) -> noteOn; } }
        graph G {
            input stream x;
            output stream y;
            output event noteOn;
            node p = P;
            connection { x -> y; p.noteOn -> noteOn; }
        }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    ASSERT_EQ (1u, analyzed->graph.outputEvents.size());
    EXPECT_EQ ("noteOn", analyzed->graph.outputEvents[0]->name);
}

TEST (YdspSemanticAnalyzerTests, RejectsDuplicateOutputEventAtGraphScope)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            output event noteOn;
            output event noteOn;
            connection { x -> y; }
        }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "Duplicate event output 'noteOn'"));
}

TEST (YdspSemanticAnalyzerTests, RejectsEmitWithUnknownShape)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            output event noteOn;
            process { emit bogus () -> noteOn; }
        }
        graph G { input stream x; output stream y; connection { x -> y; } }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "Unknown event shape 'bogus'"));
}

TEST (YdspSemanticAnalyzerTests, RejectsEmitWithUnknownField)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            output event noteOn;
            process { emit noteOn (bogus: 1.0) -> noteOn; }
        }
        graph G { input stream x; output stream y; connection { x -> y; } }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "has no field 'bogus'"));
}

TEST (YdspSemanticAnalyzerTests, RejectsEmitToUnknownTarget)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            output event noteOn;
            process { emit noteOn (pitch: 60, velocity: 0.8) -> nope; }
        }
        graph G { input stream x; output stream y; connection { x -> y; } }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "Unknown emit target 'nope'"));
}

TEST (YdspSemanticAnalyzerTests, AcceptsEmitOfAnyShapeToAnEventChannelNamedAfterADifferentShape)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            output event noteOn;
            output event noteOff;
            process { emit noteOn (pitch: 60, velocity: 0.8) -> noteOff; }
        }
        graph G { input stream x; output stream y; connection { x -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, RejectsEmitInInit)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            output event noteOn;
            init { emit noteOn (pitch: 60, velocity: 0.8) -> noteOn; }
            process { }
        }
        graph G { input stream x; output stream y; connection { x -> y; } }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "emit is not allowed in init or block-mode process"));
}

TEST (YdspSemanticAnalyzerTests, RejectsEmitInBlockModeProcess)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor P {
            output event noteOn;
            process block { emit noteOn (pitch: 60, velocity: 0.8) -> noteOn; }
        }
        graph G { input stream x; output stream y; connection { x -> y; } }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "emit is not allowed in init or block-mode process"));
}

TEST (YdspSemanticAnalyzerTests, AcceptsEmitInsideForLoopInSampleModeProcess)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            output event noteOn;
            process {
                for i in 0..4 {
                    emit noteOn (pitch: 60, velocity: 0.8) -> noteOn;
                }
            }
        }
        graph G { input stream x; output stream y; connection { x -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, AcceptsEmitInsideEventHandler)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input event midi;
            output event noteOn;
            event midi (e: noteOn) {
                emit noteOn (pitch: e.pitch, velocity: e.velocity) -> noteOn;
            }
            process { }
        }
        graph G { input stream x; output stream y; connection { x -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
}

TEST (YdspSemanticAnalyzerTests, AcceptsEmitInsideEventHandlerOnBlockModeProcessor)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor P {
            input event midi;
            output event noteOn;
            event midi (e: noteOn) {
                emit noteOn (pitch: e.pitch, velocity: e.velocity) -> noteOn;
            }
            process block { }
        }
        graph G { input stream x; output stream y; connection { x -> y; } }
    )YDSP",
                             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
}

//==============================================================================
// event edges: node -> node, node -> graph boundary, and connectivity

TEST (YdspSemanticAnalyzerTests, ResolvesNodeToNodeEventConnectionForASpecificShape)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Source { output event noteOn; process { emit noteOn (pitch: 60, velocity: 0.8) -> noteOn; } }
        processor Sink { input event noteOn; output stream out; process { out = 0.0; } }
        graph G {
            output stream y;
            node src = Source;
            node snk = Sink;
            connection { src.noteOn -> snk.noteOn; snk.out -> y; }
        }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);

    ASSERT_EQ (1u, analyzed->graph.eventEdges.size());
    const auto& edge = analyzed->graph.eventEdges[0];
    EXPECT_EQ (0, edge.srcNode);
    EXPECT_EQ (0, edge.srcEndpoint);
    EXPECT_EQ (1, edge.dstNode);
    EXPECT_EQ (0, edge.dstEndpoint);
}

TEST (YdspSemanticAnalyzerTests, ResolvesNodeToNodeEventConnectionTargetingTheMidiPolymorphicEndpoint)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Source { output event pitchBend; process { emit pitchBend (bendSemitones: 2.0) -> pitchBend; } }
        processor Sink { input event midi; output stream out; process { out = 0.0; } }
        graph G {
            output stream y;
            node src = Source;
            node snk = Sink;
            connection { src.pitchBend -> snk.midi; snk.out -> y; }
        }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);

    ASSERT_EQ (1u, analyzed->graph.eventEdges.size());
}

TEST (YdspSemanticAnalyzerTests, ResolvesAnEventConnectionAgainstASpecificInputEventNamedAfterADifferentShape)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Source { output event noteOn; process { emit noteOn (pitch: 60, velocity: 0.8) -> noteOn; } }
        processor Sink { input event noteOff; output stream out; process { out = 0.0; } }
        graph G {
            output stream y;
            node src = Source;
            node snk = Sink;
            connection { src.noteOn -> snk.noteOff; snk.out -> y; }
        }
    )YDSP",
                             diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    ASSERT_EQ (1u, analyzed->graph.eventEdges.size());
}

TEST (YdspSemanticAnalyzerTests, RejectsAnInlineDelayOnAnEventConnection)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor Source { output event noteOn; process { emit noteOn (pitch: 60, velocity: 0.8) -> noteOn; } }
        processor Sink { input event midi; output stream out; process { out = 0.0; } }
        graph G {
            input event midi;
            output stream y;
            node src = Source;
            node snk = Sink;
            connection { src.noteOn -> [4] -> snk.midi; snk.out -> y; }
        }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "An inline delay is not supported on an event connection"));
}

TEST (YdspSemanticAnalyzerTests, ResolvesNodeToGraphBoundaryOutputEventConnection)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Source { output event noteOn; process { emit noteOn (pitch: 60, velocity: 0.8) -> noteOn; } }
        graph G {
            output event noteOn;
            node src = Source;
            connection { src.noteOn -> noteOn; }
        }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);

    ASSERT_EQ (1u, analyzed->graph.eventEdges.size());
    const auto& edge = analyzed->graph.eventEdges[0];
    EXPECT_EQ (0, edge.srcNode);
    EXPECT_EQ (0, edge.srcEndpoint);
    EXPECT_EQ (-1, edge.dstNode);
    EXPECT_EQ (0, edge.dstEndpoint);
}

TEST (YdspSemanticAnalyzerTests, RejectsAnUnconnectedNodeOutputEvent)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor Source { output event noteOn; process { emit noteOn (pitch: 60, velocity: 0.8) -> noteOn; } }
        graph G { input stream x; output stream y; node src = Source; connection { x -> y; } }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "Node 'src' output event 'noteOn' is not connected: it must feed at least one destination"));
}

TEST (YdspSemanticAnalyzerTests, RejectsAnUnconnectedGraphOutputEvent)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        graph G { input stream x; output stream y; output event noteOn; connection { x -> y; } }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "Graph output event 'noteOn' is not connected: it must be driven by at least one source"));
}

TEST (YdspSemanticAnalyzerTests, RejectsAnUnconnectedInputEventEndpoint)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor Sink { input event midi; output stream out; process { out = 0.0; } }
        graph G { input event midi; output stream y; node snk = Sink; connection { snk.out -> y; } }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "Graph input event 'midi' is not connected: it must feed at least one destination"));
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "Node 'snk' input event 'midi' is not connected: it must be driven by at least one source"));
}

//==============================================================================
// event edges feed the same topological sort as audio edges

TEST (YdspSemanticAnalyzerTests, RejectsAPureEventFeedbackCycle)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor A { input event midi; output event noteOn; state float f; event midi (e: noteOn) { f = e.pitch; } process { } }
        processor B { input event midi; output event noteOn; state float f; event midi (e: noteOn) { f = e.pitch; } process { } }
        graph G {
            node a = A;
            node b = B;
            connection { a.noteOn -> b.midi; b.noteOn -> a.midi; }
        }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "feedback cycle, which is not supported in this version"));
}

TEST (YdspSemanticAnalyzerTests, RejectsAMixedAudioAndEventFeedbackCycle)
{
    YdspDiagnostics diagnostics;

    analyze (R"YDSP(
        processor Fork { input stream a; output stream c; output event noteOn; process { c = a; emit noteOn (pitch: 60, velocity: 0.8) -> noteOn; } }
        processor Pass { input event midi; output stream out; state float f; event midi (e: noteOn) { f = e.pitch; } process { out = f; } }
        graph G {
            output stream y;
            node f = Fork;
            node p = Pass;
            connection { p.out -> f.a; f.noteOn -> p.midi; f.c -> y; }
        }
    )YDSP",
             diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (anyDiagnosticContains (diagnostics, "feedback cycle, which is not supported in this version"));
}

TEST (YdspSemanticAnalyzerTests, TopoSortsAnAcyclicEventOnlyGraphWithNoAudio)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Arp { output event noteOn; process { emit noteOn (pitch: 60, velocity: 0.8) -> noteOn; } }
        processor Voice { input event midi; process { } }
        graph G {
            node arp = Arp;
            node voice = Voice;
            connection { arp.noteOn -> voice.midi; }
        }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    ASSERT_EQ (2u, analyzed->graph.topoOrder.size());
    EXPECT_EQ (0, analyzed->graph.topoOrder[0]);
    EXPECT_EQ (1, analyzed->graph.topoOrder[1]);
}

//==============================================================================
// event edge latency compensation

TEST (YdspSemanticAnalyzerTests, CompensatesANodeToNodeEventEdgeByTheSourcesDeclaredLatency)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Source [[ latency: 8 ]] { output event noteOn; process { emit noteOn (pitch: 60, velocity: 0.8) -> noteOn; } }
        processor Sink { input event noteOn; output stream out; process { out = 0.0; } }
        graph G {
            output stream y;
            node src = Source;
            node snk = Sink;
            connection { src.noteOn -> snk.noteOn; snk.out -> y; }
        }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);

    ASSERT_EQ (1u, analyzed->graph.eventEdges.size());
    EXPECT_EQ (8, analyzed->graph.eventEdges[0].compensationSamples);
}

TEST (YdspSemanticAnalyzerTests, CompensatesANodeToGraphBoundaryEventEdgeByTheGraphsOverallLatency)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Source [[ latency: 8 ]] { output event noteOn; process { emit noteOn (pitch: 60, velocity: 0.8) -> noteOn; } }
        processor Slow [[ latency: 64 ]] { input stream in; output stream out; process { out = in; } }
        graph G {
            input stream x;
            output stream y;
            output event noteOn;
            node src = Source;
            node slow = Slow;
            connection { src.noteOn -> noteOn; x -> slow.in; slow.out -> y; }
        }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);
    ASSERT_EQ (64, analyzed->graph.latencySamples);

    ASSERT_EQ (1u, analyzed->graph.eventEdges.size());
    EXPECT_EQ (64, analyzed->graph.eventEdges[0].compensationSamples);
}

TEST (YdspSemanticAnalyzerTests, LeavesAnEventEdgeWithNoLatencyAnywhereUncompensated)
{
    YdspDiagnostics diagnostics;

    auto analyzed = analyze (R"YDSP(
        processor Source { output event noteOn; process { emit noteOn (pitch: 60, velocity: 0.8) -> noteOn; } }
        processor Sink { input event noteOn; output stream out; process { out = 0.0; } }
        graph G {
            output stream y;
            node src = Source;
            node snk = Sink;
            connection { src.noteOn -> snk.noteOn; snk.out -> y; }
        }
    )YDSP",
                             diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, analyzed);

    ASSERT_EQ (1u, analyzed->graph.eventEdges.size());
    EXPECT_EQ (0, analyzed->graph.eventEdges[0].compensationSamples);
}
