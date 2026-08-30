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

namespace yup::test
{

namespace
{

std::vector<YdspToken> tokenize (StringRef source, YdspDiagnostics& diagnostics)
{
    YdspLexer lexer (source, diagnostics);
    return lexer.tokenize();
}

std::unique_ptr<YdspProgram> parse (StringRef source, YdspDiagnostics& diagnostics)
{
    YdspLexer lexer (source, diagnostics);
    auto tokens = lexer.tokenize();

    YdspParser parser (std::move (tokens), diagnostics);
    return parser.parseProgram();
}

std::vector<YdspTokenType> typesOf (const std::vector<YdspToken>& tokens)
{
    std::vector<YdspTokenType> types;

    for (const auto& token : tokens)
        types.push_back (token.type);

    return types;
}

} // namespace

//==============================================================================

TEST (YdspLexerTests, TokenizesKeywordsAndIdentifiers)
{
    YdspDiagnostics diagnostics;

    auto tokens = tokenize ("processor Saturator { input stream in; }", diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors());
    EXPECT_EQ ((std::vector<YdspTokenType> {
                   YdspTokenType::kwProcessor,
                   YdspTokenType::identifier,
                   YdspTokenType::lBrace,
                   YdspTokenType::kwInput,
                   YdspTokenType::kwStream,
                   YdspTokenType::identifier,
                   YdspTokenType::semi,
                   YdspTokenType::rBrace,
                   YdspTokenType::endOfFile }),
               typesOf (tokens));
}

TEST (YdspLexerTests, TokenizesNumbersAndRange)
{
    YdspDiagnostics diagnostics;

    auto tokens = tokenize ("0..blockSize 1.5 2e3 42", diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors());
    EXPECT_EQ ((std::vector<YdspTokenType> {
                   YdspTokenType::intLiteral,
                   YdspTokenType::range,
                   YdspTokenType::identifier,
                   YdspTokenType::floatLiteral,
                   YdspTokenType::floatLiteral,
                   YdspTokenType::intLiteral,
                   YdspTokenType::endOfFile }),
               typesOf (tokens));
}

TEST (YdspLexerTests, TokenizesLeadingAndTrailingDotFloats)
{
    YdspDiagnostics diagnostics;

    auto tokens = tokenize (".5 1. 0..blockSize 1.sin", diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors());
    EXPECT_EQ ((std::vector<YdspTokenType> {
                   YdspTokenType::floatLiteral, // .5
                   YdspTokenType::floatLiteral, // 1.
                   YdspTokenType::intLiteral,   // 0
                   YdspTokenType::range,        // ..
                   YdspTokenType::identifier,   // blockSize
                   YdspTokenType::intLiteral,   // 1
                   YdspTokenType::dot,          // . (member access, not swallowed)
                   YdspTokenType::identifier,   // sin
                   YdspTokenType::endOfFile }),
               typesOf (tokens));

    EXPECT_EQ (String (".5"), tokens[0].text);
    EXPECT_EQ (String ("1."), tokens[1].text);
}

TEST (YdspLexerTests, TokenizesHexBinaryAndDigitSeparatorIntegers)
{
    YdspDiagnostics diagnostics;

    auto tokens = tokenize ("0x1F 0b1010 1_000 _", diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors());
    EXPECT_EQ ((std::vector<YdspTokenType> {
                   YdspTokenType::intLiteral, // 0x1F
                   YdspTokenType::intLiteral, // 0b1010
                   YdspTokenType::intLiteral, // 1_000
                   YdspTokenType::underscore, // standalone '_' is still the graph-algebra wildcard
                   YdspTokenType::endOfFile }),
               typesOf (tokens));

    EXPECT_EQ (String ("0x1F"), tokens[0].text);
    EXPECT_EQ (String ("0b1010"), tokens[1].text);
    EXPECT_EQ (String ("1_000"), tokens[2].text);
}

TEST (YdspLexerTests, ReportsErrorOnHexOrBinaryLiteralWithNoDigits)
{
    YdspDiagnostics hexDiagnostics;
    tokenize ("0x", hexDiagnostics);
    EXPECT_TRUE (hexDiagnostics.hasErrors());

    YdspDiagnostics binDiagnostics;
    tokenize ("0b", binDiagnostics);
    EXPECT_TRUE (binDiagnostics.hasErrors());
}

TEST (YdspLexerTests, ParsesStringEscapes)
{
    YdspDiagnostics diagnostics;

    auto tokens = tokenize (R"(  "a\nb\t\"c\"\\d"  )", diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors());
    ASSERT_EQ (2u, tokens.size());
    EXPECT_EQ (YdspTokenType::stringLiteral, tokens[0].type);
    EXPECT_EQ (String ("a\nb\t\"c\"\\d"), tokens[0].text);
}

TEST (YdspLexerTests, ReportsErrorOnUnknownStringEscape)
{
    YdspDiagnostics diagnostics;

    auto tokens = tokenize (R"( "a\qb" )", diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspLexerTests, TokenizesSpecialOperators)
{
    YdspDiagnostics diagnostics;

    auto tokens = tokenize ("<: :> ~ @ ' -> [[ ]] .. _ ? && || ! ==", diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors());
    EXPECT_EQ ((std::vector<YdspTokenType> {
                   YdspTokenType::lessColon,
                   YdspTokenType::colonGreater,
                   YdspTokenType::tilde,
                   YdspTokenType::at,
                   YdspTokenType::apostrophe,
                   YdspTokenType::arrow,
                   YdspTokenType::lAnnotation,
                   YdspTokenType::rAnnotation,
                   YdspTokenType::range,
                   YdspTokenType::underscore,
                   YdspTokenType::question,
                   YdspTokenType::andAnd,
                   YdspTokenType::orOr,
                   YdspTokenType::not_,
                   YdspTokenType::equal,
                   YdspTokenType::endOfFile }),
               typesOf (tokens));
}

TEST (YdspLexerTests, TokenizesBitwiseOperators)
{
    YdspDiagnostics diagnostics;

    auto tokens = tokenize ("a & b | c ^ d << e >> f ~g &= |= ^= <<= >>= 1 < 2 > 3", diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors());
    EXPECT_EQ ((std::vector<YdspTokenType> {
                   YdspTokenType::identifier,
                   YdspTokenType::ampersand,
                   YdspTokenType::identifier,
                   YdspTokenType::pipe,
                   YdspTokenType::identifier,
                   YdspTokenType::caret,
                   YdspTokenType::identifier,
                   YdspTokenType::shl,
                   YdspTokenType::identifier,
                   YdspTokenType::shr,
                   YdspTokenType::identifier,
                   YdspTokenType::tilde,
                   YdspTokenType::identifier,
                   YdspTokenType::ampersandEq,
                   YdspTokenType::pipeEq,
                   YdspTokenType::caretEq,
                   YdspTokenType::shlEq,
                   YdspTokenType::shrEq,
                   YdspTokenType::intLiteral,
                   YdspTokenType::less,
                   YdspTokenType::intLiteral,
                   YdspTokenType::greater,
                   YdspTokenType::intLiteral,
                   YdspTokenType::endOfFile }),
               typesOf (tokens));
}

TEST (YdspLexerTests, TokenizesDivideAndModuloCompoundAssignment)
{
    YdspDiagnostics diagnostics;

    auto tokens = tokenize ("a /= b % c %= d", diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors());
    EXPECT_EQ ((std::vector<YdspTokenType> {
                   YdspTokenType::identifier,
                   YdspTokenType::slashEq,
                   YdspTokenType::identifier,
                   YdspTokenType::percent,
                   YdspTokenType::identifier,
                   YdspTokenType::percentEq,
                   YdspTokenType::identifier,
                   YdspTokenType::endOfFile }),
               typesOf (tokens));
}

TEST (YdspLexerTests, SkipsCommentsAndTracksLines)
{
    YdspDiagnostics diagnostics;

    auto tokens = tokenize ("// line comment\nprocessor /* block\ncomment */ a", diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors());
    EXPECT_EQ (YdspTokenType::kwProcessor, tokens[0].type);
    EXPECT_EQ (2, tokens[0].line);
    EXPECT_EQ (YdspTokenType::identifier, tokens[1].type);
    EXPECT_EQ (3, tokens[1].line);
}

TEST (YdspLexerTests, DoesNotEndACommentEarlyOnANonAsciiCharacterThatLooksLikeNewlineWhenTruncated)
{
    YdspDiagnostics diagnostics;

    auto tokens = tokenize (String (CharPointer_UTF8 ("// abc\xC4\x8A"
                                                      "def\nprocessor")),
                            diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors());
    ASSERT_EQ (2u, tokens.size());
    EXPECT_EQ (YdspTokenType::kwProcessor, tokens[0].type);
    EXPECT_EQ (2, tokens[0].line);
}

TEST (YdspLexerTests, PreservesNonAsciiCharactersInStringLiterals)
{
    YdspDiagnostics diagnostics;

    auto tokens = tokenize (String (CharPointer_UTF8 ("\"a\xC4\x8A"
                                                      "b\"")),
                            diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors());
    ASSERT_EQ (YdspTokenType::stringLiteral, tokens[0].type);
    EXPECT_EQ (String (CharPointer_UTF8 ("a\xC4\x8A"
                                         "b")),
               tokens[0].text);
}

TEST (YdspLexerTests, ReportsErrorOnUnknownCharacter)
{
    YdspDiagnostics diagnostics;

    auto tokens = tokenize ("a $ b", diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_EQ (YdspTokenType::identifier, tokens[0].type);
    EXPECT_EQ (1, diagnostics.getItem (0).line);
    EXPECT_EQ (3, diagnostics.getItem (0).column);
}

TEST (YdspLexerTests, ReportsErrorOnUnterminatedString)
{
    YdspDiagnostics diagnostics;

    auto tokens = tokenize ("\"hello", diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_EQ (YdspTokenType::stringLiteral, tokens[0].type);
    EXPECT_EQ ("hello", tokens[0].text);
}

//==============================================================================

TEST (YdspLexerTests, TokenizesFuncAndReturnKeywords)
{
    YdspDiagnostics diagnostics;

    auto tokens = tokenize ("func return", diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors());
    EXPECT_EQ ((std::vector<YdspTokenType> {
                   YdspTokenType::kwFunc,
                   YdspTokenType::kwReturn,
                   YdspTokenType::endOfFile }),
               typesOf (tokens));
}

TEST (YdspLexerTests, TokenizesImportKeyword)
{
    YdspDiagnostics diagnostics;

    auto tokens = tokenize ("import \"filters.ydsp\" as flt;", diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors());
    EXPECT_EQ ((std::vector<YdspTokenType> {
                   YdspTokenType::kwImport,
                   YdspTokenType::stringLiteral,
                   YdspTokenType::identifier,
                   YdspTokenType::identifier,
                   YdspTokenType::semi,
                   YdspTokenType::endOfFile }),
               typesOf (tokens));
}

//==============================================================================

TEST (YdspParserTests, ParsesProcessorWithEndpointsStateAndProcess)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor Saturator {
            input  stream in;
            input  stream side;
            output stream out;
            input  value float drive = 0.5 [[ name: "Drive", min: 0, max: 2 ]];
            output value float level;
            state  float z;
            state  float buf[256];
            process {
                out = tanh (in * drive) * (1 + 0.5 * side);
                z = 0.999 * z + in;
                level = abs (z);
            }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->processors.size());

    const auto& processor = program->processors[0];
    EXPECT_EQ ("Saturator", processor.name);

    ASSERT_EQ (5u, processor.endpoints.size());
    EXPECT_EQ (YdspEndpointKind::inputStream, processor.endpoints[0].kind);
    EXPECT_EQ ("in", processor.endpoints[0].name);
    EXPECT_EQ (YdspEndpointKind::outputStream, processor.endpoints[2].kind);
    EXPECT_EQ (YdspEndpointKind::inputValue, processor.endpoints[3].kind);
    EXPECT_NE (nullptr, processor.endpoints[3].defaultValue);
    EXPECT_EQ (YdspExprKind::floatLiteral, processor.endpoints[3].defaultValue->kind);
    ASSERT_EQ (3u, processor.endpoints[3].annotations.size());
    EXPECT_EQ ("name", processor.endpoints[3].annotations[0].first);
    EXPECT_EQ ("Drive", processor.endpoints[3].annotations[0].second);

    ASSERT_EQ (2u, processor.states.size());
    EXPECT_EQ ("z", processor.states[0].name);
    EXPECT_EQ (0, processor.states[0].arraySize);
    EXPECT_EQ ("buf", processor.states[1].name);
    EXPECT_EQ (256, processor.states[1].arraySize);

    ASSERT_NE (nullptr, processor.process);
    EXPECT_EQ (YdspProcessMode::sample, processor.process->mode);
    EXPECT_EQ (3u, processor.process->body.size());
}

TEST (YdspParserTests, ParsesBracedListAnnotationValue)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor Selector {
            output stream out;
            input value float wave = 0.0 [[ name: "Waveform", min: 0.0, max: 3.0, values: { "Saw", "Square", "Triangle", "Pulse" } ]];
            process { out = wave; }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->processors.size());

    const auto& annotations = program->processors[0].endpoints[1].annotations;
    ASSERT_EQ (4u, annotations.size());
    EXPECT_EQ ("values", annotations[3].first);

    // The braced entries arrive joined by commas, ready to be split apart.
    const auto entries = StringArray::fromTokens (annotations[3].second, ",", "");
    ASSERT_EQ (4, entries.size());
    EXPECT_EQ ("Saw", entries[0]);
    EXPECT_EQ ("Square", entries[1]);
    EXPECT_EQ ("Triangle", entries[2]);
    EXPECT_EQ ("Pulse", entries[3]);
}

TEST (YdspParserTests, ParsesBooleanAnnotationValue)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor P {
            output stream out;
            input value float pan = 0.0 [[ name: "Pan", min: -1.0, max: 1.0, bipolar: true ]];
            input value float depth = 0.0 [[ bipolar: false ]];
            process { out = pan + depth; }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->processors.size());

    const auto& panAnnotations = program->processors[0].endpoints[1].annotations;
    ASSERT_EQ (4u, panAnnotations.size());
    EXPECT_EQ ("bipolar", panAnnotations[3].first);
    EXPECT_EQ ("true", panAnnotations[3].second);

    const auto& depthAnnotations = program->processors[0].endpoints[2].annotations;
    ASSERT_EQ (1u, depthAnnotations.size());
    EXPECT_EQ ("bipolar", depthAnnotations[0].first);
    EXPECT_EQ ("false", depthAnnotations[0].second);
}

TEST (YdspParserTests, DoesNotPushANullStatementIntoTheProcessBody)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor P {
            output stream out;
            process { ; out = 1.0; }
        }
    )YDSP",
                          diagnostics);

    ASSERT_TRUE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->processors.size());
    ASSERT_NE (nullptr, program->processors[0].process);

    for (const auto& stmt : program->processors[0].process->body)
        EXPECT_NE (nullptr, stmt);
}

TEST (YdspParserTests, ReportsDeeplyNestedExpressionsInsteadOfOverflowingTheStack)
{
    YdspDiagnostics diagnostics;

    String source = "processor P { output stream out; process { out = ";

    for (int i = 0; i < 10000; ++i)
        source += "(";

    source += "1.0";

    for (int i = 0; i < 10000; ++i)
        source += ")";

    source += "; } }";

    auto program = parse (source, diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspParserTests, ParsesBlockProcessWithForLoop)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor Delay {
            input stream in;
            output stream out;
            state float mem[8192];
            process block {
                for i in 0..blockSize {
                    out[i] = mem[i];
                }
            }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);

    const auto& processor = program->processors[0];
    ASSERT_NE (nullptr, processor.process);
    EXPECT_EQ (YdspProcessMode::block, processor.process->mode);
    ASSERT_EQ (1u, processor.process->body.size());
    EXPECT_EQ (YdspStmtKind::forStmt, processor.process->body[0]->kind);
    EXPECT_EQ ("i", processor.process->body[0]->name);
    EXPECT_EQ (YdspExprKind::intLiteral, processor.process->body[0]->startExpr->kind);
    EXPECT_EQ (YdspExprKind::identifier, processor.process->body[0]->endExpr->kind);
    EXPECT_EQ ("blockSize", processor.process->body[0]->endExpr->text);
}

TEST (YdspParserTests, DesugarsCompoundAssignmentOnIndexedAndMemberTargets)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor P {
            output stream out;
            state float buf[8];
            process block {
                for i in 0..8 {
                    buf[i] += 1.0;
                }
            }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);

    const auto& forBody = program->processors[0].process->body[0]->body->children;
    ASSERT_EQ (1u, forBody.size());

    const auto& assign = *forBody[0];
    ASSERT_EQ (YdspStmtKind::assign, assign.kind);
    ASSERT_EQ (YdspExprKind::index, assign.target->kind);

    ASSERT_EQ (YdspExprKind::binary, assign.value->kind);
    EXPECT_EQ (YdspOperator::add, assign.value->op);
    ASSERT_EQ (YdspExprKind::index, assign.value->children[0]->kind);
    EXPECT_EQ ("buf", assign.value->children[0]->children[0]->text);
    EXPECT_EQ ("i", assign.value->children[0]->children[1]->text);

    YdspDiagnostics memberDiagnostics;

    auto memberProgram = parse (R"YDSP(
        processor P {
            output stream out;
            struct Voice { float phase; }
            Voice v;
            process { v.phase += 0.25; }
        }
    )YDSP",
                                memberDiagnostics);

    ASSERT_FALSE (memberDiagnostics.hasErrors());
    ASSERT_NE (nullptr, memberProgram);

    const auto& memberAssign = *memberProgram->processors[0].process->body[0];
    ASSERT_EQ (YdspStmtKind::assign, memberAssign.kind);
    ASSERT_EQ (YdspExprKind::member, memberAssign.target->kind);

    ASSERT_EQ (YdspExprKind::binary, memberAssign.value->kind);
    EXPECT_EQ (YdspOperator::add, memberAssign.value->op);
    ASSERT_EQ (YdspExprKind::member, memberAssign.value->children[0]->kind);
    EXPECT_EQ ("phase", memberAssign.value->children[0]->text);
    EXPECT_EQ ("v", memberAssign.value->children[0]->children[0]->text);
}

TEST (YdspParserTests, ParsesDivideAndModuloCompoundAssignment)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor P {
            output stream out;
            state float x = 8.0;
            process {
                x /= 2.0;
                x %= 3.0;
            }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);

    const auto& body = program->processors[0].process->body;
    ASSERT_EQ (2u, body.size());

    ASSERT_EQ (YdspStmtKind::assign, body[0]->kind);
    EXPECT_EQ (YdspOperator::div, body[0]->value->op);

    ASSERT_EQ (YdspStmtKind::assign, body[1]->kind);
    EXPECT_EQ (YdspOperator::mod, body[1]->value->op);
}

TEST (YdspParserTests, ParsesExpressions)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z;
            process {
                let a = -in + z * 2;
                out = (a > 0.5) ? sin (a) : a';
                z = a @ 10;
            }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);

    const auto& body = program->processors[0].process->body;

    // let a = -in + z * 2;
    ASSERT_EQ (YdspStmtKind::localDecl, body[0]->kind);
    EXPECT_TRUE (body[0]->isLet);
    EXPECT_EQ (YdspExprKind::binary, body[0]->value->kind);
    EXPECT_EQ (YdspOperator::add, body[0]->value->op);
    EXPECT_EQ (YdspExprKind::unary, body[0]->value->children[0]->kind);
    EXPECT_EQ (YdspOperator::neg, body[0]->value->children[0]->op);

    // out = (a > 0.5) ? sin (a) : a';
    ASSERT_EQ (YdspStmtKind::assign, body[1]->kind);
    EXPECT_EQ (YdspExprKind::ternary, body[1]->value->kind);
    EXPECT_EQ (YdspExprKind::call, body[1]->value->children[1]->kind);
    EXPECT_EQ ("sin", body[1]->value->children[1]->text);
    EXPECT_EQ (YdspExprKind::prev, body[1]->value->children[2]->kind);

    // z = a @ 10;
    ASSERT_EQ (YdspStmtKind::assign, body[2]->kind);
    EXPECT_EQ (YdspExprKind::delay, body[2]->value->kind);
    EXPECT_EQ (YdspExprKind::intLiteral, body[2]->value->children[1]->kind);
}

TEST (YdspParserTests, ParsesGraphWithConnections)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        graph MyPatch {
            input  stream dry;
            input  stream side;
            output stream wet;
            input  value float master = 0.8;
            node sat = Saturator (drive = 1.5);
            node dly = Delay (time = 0.25);
            connection {
                dry -> sat.in;
                side -> sat.side;
                sat.out -> dly.in;
                dly.out -> wet;
                master -> sat.drive;
            }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->graphs.size());

    const auto& graph = program->graphs[0];
    EXPECT_EQ ("MyPatch", graph.name);
    ASSERT_EQ (4u, graph.endpoints.size());
    ASSERT_EQ (2u, graph.nodes.size());
    EXPECT_EQ ("sat", graph.nodes[0].instanceName);
    EXPECT_EQ ("Saturator", graph.nodes[0].processorName);
    ASSERT_EQ (1u, graph.nodes[0].overrides.size());
    EXPECT_EQ ("drive", graph.nodes[0].overrides[0].first);

    EXPECT_EQ (YdspGraphBodyKind::connections, graph.bodyKind);
    ASSERT_EQ (5u, graph.connections.size());
    EXPECT_EQ ("dry", graph.connections[0].sourcePath);
    EXPECT_EQ ("sat.in", graph.connections[0].destPath);
    EXPECT_EQ ("sat.out", graph.connections[2].sourcePath);
    EXPECT_EQ ("master", graph.connections[4].sourcePath);
    EXPECT_EQ ("sat.drive", graph.connections[4].destPath);
}

TEST (YdspParserTests, ParsesGraphWithAlgebra)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        graph Chain {
            input stream dry;
            output stream wet;
            node sat = Saturator (drive = 1.5);
            process = dry : sat : wet;
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_EQ (1u, program->graphs.size());

    const auto& graph = program->graphs[0];
    EXPECT_EQ (YdspGraphBodyKind::algebra, graph.bodyKind);
    ASSERT_NE (nullptr, graph.algebraRoot);
    EXPECT_EQ (YdspExprKind::graphOp, graph.algebraRoot->kind);
    EXPECT_EQ (YdspOperator::seq, graph.algebraRoot->op);

    // Left-associative: ((dry : sat) : wet)
    ASSERT_EQ (2u, graph.algebraRoot->children.size());
    EXPECT_EQ (YdspExprKind::graphLeaf, graph.algebraRoot->children[1]->kind);
    EXPECT_EQ ("wet", graph.algebraRoot->children[1]->text);

    const auto& inner = *graph.algebraRoot->children[0];
    EXPECT_EQ (YdspExprKind::graphOp, inner.kind);
    EXPECT_EQ (YdspOperator::seq, inner.op);
    EXPECT_EQ ("dry", inner.children[0]->text);
    EXPECT_EQ ("sat", inner.children[1]->text);
}

TEST (YdspParserTests, ParsesAlgebraWithParallelSplitAndRecursion)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        graph G {
            input stream x, y;
            output stream a, b;
            process = x , y : ( _ , _ ) : (a , b);
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_EQ (1u, program->graphs.size());
    EXPECT_EQ (YdspExprKind::graphOp, program->graphs[0].algebraRoot->kind);
}

TEST (YdspParserTests, ParsesDeclareMetadata)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        declare name "MyPatch";
        declare author "Jane";
        processor P { input stream in; output stream out; process { out = in; } }
        graph G { input stream in; output stream out; node p = P; process = in : p : out; }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (2u, program->declares.size());
    EXPECT_EQ ("name", program->declares[0].key);
    EXPECT_EQ ("MyPatch", program->declares[0].value);
    EXPECT_EQ ("author", program->declares[1].key);
}

TEST (YdspParserTests, ReportsMissingClosingBrace)
{
    YdspDiagnostics diagnostics;

    auto program = parse ("processor P { input stream in;", diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_EQ (1, diagnostics.getItem (0).line);
    EXPECT_NE (nullptr, program);
}

TEST (YdspParserTests, ParsesMultipleGraphsAndTheMainAnnotation)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        graph A { input stream i; output stream o; process = i : o; }
        graph B [[ main ]] { input stream i; output stream o; process = i : o; }
    )YDSP",
                          diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (2u, program->graphs.size());
    EXPECT_EQ ("A", program->graphs[0].name);
    EXPECT_TRUE (program->graphs[0].annotations.empty());
    EXPECT_EQ ("B", program->graphs[1].name);
    ASSERT_EQ (1u, program->graphs[1].annotations.size());
    EXPECT_EQ ("main", program->graphs[1].annotations[0].first);
}

TEST (YdspParserTests, RejectsStatementOutsideProcess)
{
    YdspDiagnostics diagnostics;

    auto program = parse ("processor P { out = in; }", diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
}

TEST (YdspParserTests, ReportsExpressionErrorWithLocation)
{
    YdspDiagnostics diagnostics;

    auto program = parse ("processor P { input stream in; output stream out; process { out = + ; } }", diagnostics);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_EQ (1, diagnostics.getItem (0).line);
}

TEST (YdspParserTests, ParsesFunctionDeclaration)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor WithFunc {
            input stream in;
            output stream out;
            func add(a: float, b: float) : float {
                return a + b;
            }
            process { out = in; }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->processors.size());

    const auto& processor = program->processors[0];
    ASSERT_EQ (1u, processor.functions.size());
    EXPECT_EQ ("add", processor.functions[0].name);
    EXPECT_EQ (2u, processor.functions[0].params.size());
    EXPECT_EQ ("a", processor.functions[0].params[0].first);
    EXPECT_EQ (YdspPrimitiveType::float32Type, processor.functions[0].params[0].second);
    EXPECT_EQ ("b", processor.functions[0].params[1].first);
    EXPECT_TRUE (processor.functions[0].hasReturnType);
    EXPECT_EQ (YdspPrimitiveType::float32Type, processor.functions[0].returnType);
    ASSERT_EQ (1u, processor.functions[0].body.size());
    EXPECT_EQ (YdspStmtKind::returnStmt, processor.functions[0].body[0]->kind);
}

TEST (YdspParserTests, RejectsLegacyTypeFirstFunctionParameters)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor WithFunc {
            input stream in;
            output stream out;
            func add(float a, float b) : float {
                return a + b;
            }
            process { out = in; }
        }
    )YDSP",
                          diagnostics);

    ASSERT_TRUE (diagnostics.hasErrors());

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
    {
        if (diagnostics.getItem (i).message.contains ("':' after the parameter name"))
            found = true;
    }

    EXPECT_TRUE (found);
}

TEST (YdspParserTests, ParsesFunctionWithoutReturnType)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            func doit(n: int) {
                out = in * float(n);
            }
            process { out = in; }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    const auto& processor = program->processors[0];
    EXPECT_FALSE (processor.functions[0].hasReturnType);
}

TEST (YdspParserTests, ParsesFunctionCallInProcess)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            func scale(x: float, s: float) : float {
                return x * s;
            }
            process { out = scale(in, 2.0); }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    const auto& body = program->processors[0].process->body;
    ASSERT_EQ (1u, body.size());
    ASSERT_EQ (YdspStmtKind::assign, body[0]->kind);
    EXPECT_EQ (YdspExprKind::call, body[0]->value->kind);
    EXPECT_EQ ("scale", body[0]->value->text);
    EXPECT_EQ (2u, body[0]->value->children.size());
}

TEST (YdspParserTests, ParsesImportDirective)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        import filters as flt;
        processor P { input stream in; output stream out; process { out = in; } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->imports.size());
    EXPECT_EQ ("filters", program->imports[0].path);
    EXPECT_EQ ("flt", program->imports[0].alias);
}

TEST (YdspParserTests, ParsesImportWithoutAlias)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        import lib.utils;
        processor P { input stream in; output stream out; process { out = in; } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->imports.size());
    EXPECT_EQ ("lib.utils", program->imports[0].path);
    EXPECT_TRUE (program->imports[0].alias.isEmpty());
}

TEST (YdspParserTests, ParsesNamespacedNodeProcessorName)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        graph G { input stream x; output stream y; node d = fx.Delay (time = 0.5); connection { x -> d.in; d.out -> y; } }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->graphs.size());
    ASSERT_EQ (1u, program->graphs[0].nodes.size());
    EXPECT_EQ ("fx.Delay", program->graphs[0].nodes[0].processorName);
    EXPECT_EQ (1u, program->graphs[0].nodes[0].overrides.size());
}

TEST (YdspParserTests, ParsesNamespacedAlgebraProcessor)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        graph G { input stream x; output stream y; process = x : fx.Delay : y; }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->graphs.size());
    ASSERT_NE (nullptr, program->graphs[0].algebraRoot);

    // x : fx.Delay : y parses as (x : fx.Delay) : y; find the fx.Delay leaf.
    const auto* root = program->graphs[0].algebraRoot.get();
    ASSERT_EQ (YdspExprKind::graphOp, root->kind);
    ASSERT_EQ (2u, root->children.size());

    const auto* lhs = root->children[0].get();
    ASSERT_EQ (YdspExprKind::graphOp, lhs->kind);
    ASSERT_EQ (2u, lhs->children.size());

    const auto* leaf = lhs->children[1].get();
    EXPECT_EQ (YdspExprKind::graphLeaf, leaf->kind);
    EXPECT_EQ ("fx.Delay", leaf->text);
}

TEST (YdspParserTests, ParsesNodeWithOversampling)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor Sat { input stream in; output stream out; process { out = tanh(in); } }
        graph G {
            input stream x;
            output stream y;
            node sat = Sat * 4;
            connection { x -> sat.in; sat.out -> y; }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->graphs.size());
    ASSERT_EQ (1u, program->graphs[0].nodes.size());
    EXPECT_EQ (4, program->graphs[0].nodes[0].rateMultiplier);
    EXPECT_EQ (1, program->graphs[0].nodes[0].rateDivider);
}

TEST (YdspParserTests, ParsesNodeWithUndersampling)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor Sat { input stream in; output stream out; process { out = tanh(in); } }
        graph G {
            input stream x;
            output stream y;
            node sat = Sat (drive = 1.5) / 2;
            connection { x -> sat.in; sat.out -> y; }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->graphs.size());
    ASSERT_EQ (1u, program->graphs[0].nodes.size());
    EXPECT_EQ (1, program->graphs[0].nodes[0].rateMultiplier);
    EXPECT_EQ (2, program->graphs[0].nodes[0].rateDivider);
}

TEST (YdspParserTests, ParsesStructAndInitDeclarations)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor P {
            struct Voice { float phase; float buf[8]; int idx; }
            Voice mono;
            Voice voices[4];
            init { mono.phase = 0.5; voices[1].idx = 3; }
            process { out = mono.phase + voices[1].idx; }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);

    const auto& processor = program->processors[0];

    ASSERT_EQ (1u, processor.structs.size());
    EXPECT_EQ ("Voice", processor.structs[0].name);
    ASSERT_EQ (3u, processor.structs[0].fields.size());
    EXPECT_EQ ("phase", processor.structs[0].fields[0].name);
    EXPECT_EQ ("buf", processor.structs[0].fields[1].name);
    EXPECT_EQ (8, processor.structs[0].fields[1].arraySize);
    EXPECT_EQ ("idx", processor.structs[0].fields[2].name);

    ASSERT_EQ (2u, processor.states.size());
    EXPECT_EQ ("Voice", processor.states[0].structName);
    EXPECT_EQ (0, processor.states[0].arraySize);
    EXPECT_EQ ("Voice", processor.states[1].structName);
    EXPECT_EQ (4, processor.states[1].arraySize);

    ASSERT_NE (nullptr, processor.init);
    EXPECT_FALSE (processor.init->body.empty());
    EXPECT_NE (nullptr, processor.process);
}

TEST (YdspParserTests, ParsesFloatIntAliasesAs32BitTypes)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor P {
            input  value float p = 0.5;
            input  value int   q = 1;
            state  float z;
            state  int   wp;
            func   mix(a: float, b: int) : float { return a + float(b); }
            process {
                float f = 0.5;
                int   i = 1;
                out = f * float(i);
            }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    const auto& processor = program->processors[0];

    EXPECT_EQ (YdspPrimitiveType::float32Type, processor.endpoints[0].type);
    EXPECT_EQ (YdspPrimitiveType::int32Type, processor.endpoints[1].type);
    EXPECT_EQ (YdspPrimitiveType::float32Type, processor.states[0].type);
    EXPECT_EQ (YdspPrimitiveType::int32Type, processor.states[1].type);
    EXPECT_EQ (YdspPrimitiveType::float32Type, processor.functions[0].params[0].second);
    EXPECT_EQ (YdspPrimitiveType::int32Type, processor.functions[0].params[1].second);
    EXPECT_EQ (YdspPrimitiveType::float32Type, processor.functions[0].returnType);

    ASSERT_NE (nullptr, processor.process);
    const auto& f = processor.process->body[0];
    const auto& i = processor.process->body[1];
    ASSERT_EQ (YdspStmtKind::localDecl, f->kind);
    ASSERT_EQ (YdspStmtKind::localDecl, i->kind);
    EXPECT_EQ (YdspPrimitiveType::float32Type, f->declType);
    EXPECT_EQ (YdspPrimitiveType::int32Type, i->declType);
}

TEST (YdspParserTests, ParsesExplicit32And64BitTypes)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor P {
            input  value float64 acc = 0.0;
            input  value int64   counter = 0;
            input  stream float64 in64;
            output stream float64 out64;
            state  float64 z;
            state  int64   wp;
            func   add(a: float64, b: int64) : float64 { return a + float64(b); }
            process {
                float32 f = 0.5;
                float64 d = 0.25;
                int32   i = 1;
                int64   j = 2;
                out64 = d * f;
                acc = acc + d;
                counter = counter + j;
            }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    const auto& processor = program->processors[0];

    ASSERT_EQ (4u, processor.endpoints.size());
    EXPECT_EQ (YdspPrimitiveType::float64Type, processor.endpoints[0].type);
    EXPECT_EQ (YdspEndpointKind::inputValue, processor.endpoints[0].kind);
    EXPECT_EQ (YdspPrimitiveType::int64Type, processor.endpoints[1].type);
    EXPECT_EQ (YdspPrimitiveType::float64Type, processor.endpoints[2].type);
    EXPECT_EQ (YdspEndpointKind::inputStream, processor.endpoints[2].kind);
    EXPECT_EQ (YdspPrimitiveType::float64Type, processor.endpoints[3].type);
    EXPECT_EQ (YdspEndpointKind::outputStream, processor.endpoints[3].kind);
    EXPECT_EQ (YdspPrimitiveType::float64Type, processor.states[0].type);
    EXPECT_EQ (YdspPrimitiveType::int64Type, processor.states[1].type);
    EXPECT_EQ (YdspPrimitiveType::float64Type, processor.functions[0].params[0].second);
    EXPECT_EQ (YdspPrimitiveType::int64Type, processor.functions[0].params[1].second);
    EXPECT_EQ (YdspPrimitiveType::float64Type, processor.functions[0].returnType);

    ASSERT_NE (nullptr, processor.process);
    ASSERT_EQ (7u, processor.process->body.size());
    EXPECT_EQ (YdspPrimitiveType::float32Type, processor.process->body[0]->declType);
    EXPECT_EQ (YdspPrimitiveType::float64Type, processor.process->body[1]->declType);
    EXPECT_EQ (YdspPrimitiveType::int32Type, processor.process->body[2]->declType);
    EXPECT_EQ (YdspPrimitiveType::int64Type, processor.process->body[3]->declType);
}

TEST (YdspParserTests, ReportsUnknownTypeName)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor P {
            input value double x = 0.0;
            process { x = 1; }
        }
    )YDSP",
                          diagnostics);

    ASSERT_TRUE (diagnostics.hasErrors());

    bool found = false;
    for (int i = 0; i < diagnostics.getCount(); ++i)
    {
        if (diagnostics.getItem (i).message.contains ("Unknown type 'double'"))
            found = true;
    }

    EXPECT_TRUE (found);
    ASSERT_NE (nullptr, program);
}

TEST (YdspParserTests, ToStringRoundTripsTypeNames)
{
    EXPECT_EQ (StringRef ("float32"), yup::toString (YdspPrimitiveType::float32Type));
    EXPECT_EQ (StringRef ("float64"), yup::toString (YdspPrimitiveType::float64Type));
    EXPECT_EQ (StringRef ("int32"), yup::toString (YdspPrimitiveType::int32Type));
    EXPECT_EQ (StringRef ("int64"), yup::toString (YdspPrimitiveType::int64Type));
    EXPECT_EQ (StringRef ("bool"), yup::toString (YdspPrimitiveType::boolType));
}

//==============================================================================

TEST (YdspLexerTests, TokenizesEventKeyword)
{
    YdspDiagnostics diagnostics;

    auto tokens = tokenize ("event noteOn", diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors());
    ASSERT_EQ (3u, tokens.size());
    EXPECT_EQ (YdspTokenType::kwEvent, tokens[0].type);
    EXPECT_EQ (YdspTokenType::identifier, tokens[1].type);
    EXPECT_EQ (YdspTokenType::endOfFile, tokens[2].type);
}

TEST (YdspParserTests, ParsesInputEventEndpoints)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor Voice {
            input event midi;
            output stream out;
            process { out = 0; }
        }
        graph G {
            input event midi;
            output stream y;
            node v = Voice;
            connection { v.out -> y; }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->processors.size());

    const auto& processor = program->processors[0];
    ASSERT_EQ (2u, processor.endpoints.size());
    EXPECT_EQ (YdspEndpointKind::inputEvent, processor.endpoints[0].kind);
    EXPECT_EQ ("midi", processor.endpoints[0].name);
    EXPECT_EQ (YdspEndpointKind::outputStream, processor.endpoints[1].kind);
    EXPECT_EQ ("out", processor.endpoints[1].name);

    ASSERT_EQ (1u, program->graphs.size());
    ASSERT_EQ (2u, program->graphs[0].endpoints.size());
    EXPECT_EQ (YdspEndpointKind::inputEvent, program->graphs[0].endpoints[0].kind);
    EXPECT_EQ ("midi", program->graphs[0].endpoints[0].name);
    EXPECT_EQ (YdspEndpointKind::outputStream, program->graphs[0].endpoints[1].kind);
    EXPECT_EQ ("y", program->graphs[0].endpoints[1].name);
}

TEST (YdspParserTests, ParsesOutputEventEndpointAtProcessorScope)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor P {
            output event noteOn;
            process { }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->processors.size());

    const auto& processor = program->processors[0];
    ASSERT_EQ (1u, processor.endpoints.size());
    EXPECT_EQ (YdspEndpointKind::outputEvent, processor.endpoints[0].kind);
    EXPECT_EQ ("noteOn", processor.endpoints[0].name);
}

TEST (YdspParserTests, ParsesOutputEventEndpointAtGraphScope)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        graph G {
            output event noteOn;
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->graphs.size());

    const auto& graph = program->graphs[0];
    ASSERT_EQ (1u, graph.endpoints.size());
    EXPECT_EQ (YdspEndpointKind::outputEvent, graph.endpoints[0].kind);
    EXPECT_EQ ("noteOn", graph.endpoints[0].name);
}

TEST (YdspParserTests, ParsesEmitStatementInProcessBody)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor P {
            output event noteOn;
            process {
                emit noteOn (pitch: 60, velocity: 0.8) -> noteOn;
            }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->processors.size());

    const auto& processor = program->processors[0];
    ASSERT_NE (nullptr, processor.process);
    ASSERT_EQ (1u, processor.process->body.size());

    const auto& stmt = *processor.process->body[0];
    EXPECT_EQ (YdspStmtKind::emitStmt, stmt.kind);
    EXPECT_EQ ("noteOn", stmt.shapeName);
    EXPECT_EQ ("noteOn", stmt.endpointName);
    ASSERT_EQ (2u, stmt.emitFields.size());
    EXPECT_EQ ("pitch", stmt.emitFields[0].first);
    EXPECT_EQ ("velocity", stmt.emitFields[1].first);
}

TEST (YdspParserTests, ParsesEmitStatementInEventHandlerBody)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor P {
            input event midi;
            output event noteOn;
            event midi (e: noteOn) {
                emit noteOn (pitch: 60, velocity: 0.8) -> noteOn;
            }
            process { }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->processors.size());

    const auto& processor = program->processors[0];
    ASSERT_EQ (1u, processor.eventHandlers.size());
    ASSERT_EQ (1u, processor.eventHandlers[0].body.size());

    const auto& stmt = *processor.eventHandlers[0].body[0];
    EXPECT_EQ (YdspStmtKind::emitStmt, stmt.kind);
    EXPECT_EQ ("noteOn", stmt.shapeName);
    EXPECT_EQ ("noteOn", stmt.endpointName);
    ASSERT_EQ (2u, stmt.emitFields.size());
}

TEST (YdspParserTests, ParsesEmitStatementWithEmptyFieldList)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor P {
            output event noteOn;
            process {
                emit noteOn () -> noteOn;
            }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->processors.size());

    const auto& processor = program->processors[0];
    ASSERT_NE (nullptr, processor.process);
    ASSERT_EQ (1u, processor.process->body.size());

    const auto& stmt = *processor.process->body[0];
    EXPECT_EQ (YdspStmtKind::emitStmt, stmt.kind);
    EXPECT_EQ ("noteOn", stmt.shapeName);
    EXPECT_EQ ("noteOn", stmt.endpointName);
    EXPECT_TRUE (stmt.emitFields.empty());
}

TEST (YdspParserTests, ParsesEventHandlerDeclarations)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor Voice {
            input event midi;
            state float env;
            event midi (e: noteOn) {
                env = e.velocity;
            }
            event midi (e: noteOff) {
                env = 0.0;
            }
            process { }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->processors.size());

    const auto& processor = program->processors[0];
    ASSERT_EQ (2u, processor.eventHandlers.size());
    EXPECT_EQ ("midi", processor.eventHandlers[0].endpointName);
    EXPECT_EQ ("noteOn", processor.eventHandlers[0].shapeName);
    EXPECT_EQ ("e", processor.eventHandlers[0].paramName);
    EXPECT_FALSE (processor.eventHandlers[0].body.empty());
    EXPECT_EQ ("midi", processor.eventHandlers[1].endpointName);
    EXPECT_EQ ("noteOff", processor.eventHandlers[1].shapeName);
}

TEST (YdspParserTests, ParsesVoiceBankNodeGrammar)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor Voice {
            output stream out;
            process { out = 0; }
        }
        graph G {
            output stream y;
            node a = Voice[8];
            node b = Voice;
            node c = Voice[8] * 4;
            connection { a.out -> y; }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->graphs.size());
    ASSERT_EQ (3u, program->graphs[0].nodes.size());

    EXPECT_EQ (8, program->graphs[0].nodes[0].voiceCount);
    EXPECT_EQ (1, program->graphs[0].nodes[0].rateMultiplier);
    EXPECT_EQ (1, program->graphs[0].nodes[1].voiceCount);
    EXPECT_EQ (8, program->graphs[0].nodes[2].voiceCount);
    EXPECT_EQ (4, program->graphs[0].nodes[2].rateMultiplier);
}

TEST (YdspParserTests, ParsesNodeVoiceAnnotations)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor Voice {
            output stream out;
            process { out = 0; }
        }
        graph G {
            output stream y;
            node lead = Voice[8] [[ mode: poly, stealing: oldest ]];
            node bass = Voice (gain = 2) / 2 [[ mode: mono, priority: last ]];
            node plain = Voice;
            connection { lead.out -> y; }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->graphs.size());
    ASSERT_EQ (3u, program->graphs[0].nodes.size());

    const auto& lead = program->graphs[0].nodes[0].annotations;
    ASSERT_EQ (2u, lead.size());
    EXPECT_EQ ("mode", lead[0].first);
    EXPECT_EQ ("poly", lead[0].second);
    EXPECT_EQ ("stealing", lead[1].first);
    EXPECT_EQ ("oldest", lead[1].second);

    const auto& bass = program->graphs[0].nodes[1].annotations;
    ASSERT_EQ (2u, bass.size());
    EXPECT_EQ ("mono", bass[0].second);
    EXPECT_EQ ("last", bass[1].second);
    EXPECT_EQ (2, program->graphs[0].nodes[1].rateDivider);

    EXPECT_TRUE (program->graphs[0].nodes[2].annotations.empty());
}

TEST (YdspParserTests, ParsesReservedWordsAsMemberNames)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor P {
            output stream out;
            input event midi;
            state float f;
            event midi (e: controlChange) {
                f = e.value + float (e.control);
            }
            process { out = f; }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP",
                          diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, program);
}

TEST (YdspParserTests, ParsesStateAnnotations)
{
    YdspDiagnostics diagnostics;

    auto program = parse (R"YDSP(
        processor P {
            output stream out;
            input event midi;
            state int active [[ role: voiceActivity ]];
            state float plain;
            state float seeded[4] = { 1, 2 } [[ role: whatever ]];
            event midi (e: noteOn) { active = 1; }
            process { out = float (active) + plain + seeded[0]; }
        }
    )YDSP",
                          diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, program);
    ASSERT_EQ (1u, program->processors.size());

    const auto& states = program->processors[0].states;
    ASSERT_EQ (3u, states.size());

    ASSERT_EQ (1u, states[0].annotations.size());
    EXPECT_EQ ("role", states[0].annotations[0].first);
    EXPECT_EQ ("voiceActivity", states[0].annotations[0].second);

    EXPECT_TRUE (states[1].annotations.empty());

    EXPECT_EQ (4, states[2].arraySize);
    ASSERT_EQ (2u, states[2].initialisers.size());
    ASSERT_EQ (1u, states[2].annotations.size());
    EXPECT_EQ ("whatever", states[2].annotations[0].second);
}

} // namespace yup::test
