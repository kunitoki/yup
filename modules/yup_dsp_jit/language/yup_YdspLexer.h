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

namespace yup
{

//==============================================================================
/** The kinds of tokens produced by the YDSP lexer. */
enum class YdspTokenType
{
    endOfFile,

    identifier,
    intLiteral,
    floatLiteral,
    stringLiteral,

    kwProcessor,
    kwGraph,
    kwNode,
    kwConnection,
    kwInput,
    kwOutput,
    kwValue,
    kwStream,
    kwState,
    kwProcess,
    kwBlock,
    kwFor,
    kwIf,
    kwElse,
    kwLet,
    kwTrue,
    kwFalse,
    kwDeclare,
    kwFunc,
    kwReturn,
    kwImport,
    kwStruct,
    kwInit,
    kwEvent,
    kwEmit, // keep last of the keyword run (see isKeywordToken)

    lBrace,
    rBrace,
    lParen,
    rParen,
    lBracket,
    rBracket,

    semi,
    comma,
    dot,
    colon,
    tilde,
    underscore,
    apostrophe,

    arrow,       // ->
    range,       // ..
    lAnnotation, // [[
    rAnnotation, // ]]
    question,    // ?
    at,          // @

    assign,       // =
    plusEq,       // +=
    minusEq,      // -=
    starEq,       // *=
    slashEq,      // /=
    percentEq,    // %=
    ampersand,    // &
    pipe,         // |
    caret,        // ^
    shl,          // <<
    shr,          // >>
    ampersandEq,  // &=
    pipeEq,       // |=
    caretEq,      // ^=
    shlEq,        // <<=
    shrEq,        // >>=
    equal,        // ==
    notEqual,     // !=
    less,         // <
    lessEqual,    // <=
    greater,      // >
    greaterEqual, // >=
    plus,         // +
    minus,        // -
    star,         // *
    slash,        // /
    percent,      // %
    lessColon,    // <:  (Faust split composition)
    colonGreater, // :>  (Faust merge composition)
    andAnd,       // &&
    orOr,         // ||
    not_          // !
};

/** Returns true for the reserved-word tokens (the contiguous `kw*` run).

    A keyword's spelling is still a valid identifier, so grammar positions that
    can only hold a name - a member after '.', for instance - accept these too.
*/
inline constexpr bool isKeywordToken (YdspTokenType type) noexcept
{
    return type >= YdspTokenType::kwProcessor && type <= YdspTokenType::kwEmit;
}

//==============================================================================
/** A single token with its source location (1-based line/column). */
struct YdspToken
{
    YdspTokenType type = YdspTokenType::endOfFile;
    String text;
    int line = 0;
    int column = 0;

    bool is (YdspTokenType other) const noexcept { return type == other; }
};

//==============================================================================
/** Tokenizes YDSP source text.

    Produces the full token stream up front (the parser then walks it) and
    records lexing errors (unterminated strings, stray characters) in the
    provided DspJitDiagnostics.
*/
class YdspLexer
{
public:
    /** Constructs a lexer over the given source text.
    
     * @param source The source text to tokenize.
     * @param diagnostics The diagnostics object to record lexing errors.
    */
    YdspLexer (StringRef source, DspJitDiagnostics& diagnostics);

    /** Tokenizes the whole source and returns the token stream.
    
        @return A vector of tokens, ending with an end-of-file token.
    */
    std::vector<YdspToken> tokenize();

private:
    [[nodiscard]] int peek (int offset = 0) const noexcept;
    [[nodiscard]] int current() const noexcept;
    void advance() noexcept;

    void skipWhitespaceAndComments();
    YdspToken lexIdentifierOrKeyword();
    YdspToken lexNumber();
    YdspToken lexString();
    YdspToken lexSymbol();

    String source;
    DspJitDiagnostics& diagnostics;

    size_t pos = 0;
    int line = 1;
    int column = 1;
};

} // namespace yup
