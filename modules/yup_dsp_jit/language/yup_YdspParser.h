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
/** Recursive-descent parser for YDSP source.

    Produces a YdspProgram AST. Most syntax errors are recovered from
    (synchronize() skips to the next statement/declaration boundary) and
    recorded in the provided YdspDiagnostics, so the returned program is
    usually non-null and structurally complete even when errors were
    reported - check diagnostics.hasErrors(), not just the return value, to
    decide whether the program is usable. Supports both graph body forms:
    connection blocks and the Faust-style composition algebra.
*/
class YdspParser
{
public:
    /** Constructs a parser over a token stream. */
    YdspParser (std::vector<YdspToken> tokens, YdspDiagnostics& diagnostics);

    /** Parses a whole program. Almost always non-null, even when syntax
        errors were recorded - check diagnostics.hasErrors(), not this return
        value, to decide whether the program compiled cleanly. */
    std::unique_ptr<YdspProgram> parseProgram();

private:
    //==============================================================================
    // Token stream helpers

    [[nodiscard]] const YdspToken& current() const noexcept;
    [[nodiscard]] const YdspToken& peekNext() const noexcept;
    void advance() noexcept;

    [[nodiscard]] bool at (YdspTokenType type) const noexcept;
    [[nodiscard]] bool atIdentifier (StringRef text) const noexcept;

    bool match (YdspTokenType type);
    bool matchIdentifier (StringRef text);

    const YdspToken& expect (YdspTokenType type, StringRef what);
    const YdspToken& expectIdentifier (StringRef what);

    /** Consumes a name in a position that can only hold one, so a reserved
        word (`value`, `state`, ...) is accepted as an ordinary member name. */
    const YdspToken& expectMemberName (StringRef what);

    void error (const YdspToken& token, StringRef message);
    void errorCurrent (StringRef message);
    void synchronize();

    //==============================================================================
    // Program structure

    std::unique_ptr<YdspProgram> parseProgramBody();

    void parseDeclare (YdspProgram& program);
    void parseImport (YdspProgram& program);
    void parseProgramConstant (YdspProgram& program);
    std::unique_ptr<YdspProcessorDecl> parseProcessor();
    std::unique_ptr<YdspGraphDecl> parseGraph();

    //==============================================================================
    // Declarations

    YdspEndpointDecl parseEndpoint (YdspEndpointKind kind, const YdspLocation& location, std::optional<YdspPrimitiveType> sharedType = std::nullopt);
    std::vector<YdspEndpointDecl> parseEndpointWithKind();
    std::vector<YdspEndpointDecl> parseEndpointList (YdspEndpointKind kind, const YdspLocation& location);
    YdspStructDecl parseStruct();
    YdspStateDecl parseState();
    std::unique_ptr<YdspProcessDecl> parseProcess();
    YdspEventHandlerDecl parseEventHandler();
    YdspNodeDecl parseNode();
    std::vector<std::pair<String, String>> parseAnnotations();

    //==============================================================================
    // Types

    /** Maps a type-name token ("float", "float32", "float64", "int", "int32",
        "int64" or "bool") to its primitive type. Returns std::nullopt when the
        token is not a known type name. Does not consume the token. */
    std::optional<YdspPrimitiveType> parsePrimitiveType (const YdspToken& token) const noexcept;

    //==============================================================================
    // Graph body forms

    void parseConnectionBlock (YdspGraphDecl& graph);
    void parseAlgebraDefinition (YdspGraphDecl& graph);

    //==============================================================================
    // Statements

    YdspStmtPtr parseStatement();
    YdspStmtPtr parseBlockStatement();
    YdspStmtPtr parseIfStatement();
    YdspStmtPtr parseForStatement();
    YdspStmtPtr parseLocalDeclaration();
    YdspStmtPtr parseEmitStatement();
    YdspStmtPtr parseAssignment();
    YdspStmtPtr parseReturnStatement();

    // Functions
    YdspFuncDecl parseFunction();

    //==============================================================================
    // Expressions

    YdspExprPtr parseExpression();
    YdspExprPtr parseTernary();
    YdspExprPtr parseOr();
    YdspExprPtr parseAnd();
    YdspExprPtr parseBitOr();
    YdspExprPtr parseBitXor();
    YdspExprPtr parseBitAnd();
    YdspExprPtr parseEquality();
    YdspExprPtr parseRelational();
    YdspExprPtr parseShift();
    YdspExprPtr parseDelay();
    YdspExprPtr parseAdditive();
    YdspExprPtr parseMultiplicative();
    YdspExprPtr parseUnary();
    YdspExprPtr parsePostfix();
    YdspExprPtr parsePrimary();

    // Graph algebra expressions
    YdspExprPtr parseAlgebra();
    YdspExprPtr parseAlgebraParallel();
    YdspExprPtr parseAlgebraSequential();
    YdspExprPtr parseAlgebraRecursive();
    YdspExprPtr parseAlgebraPrimary();

    YdspExprPtr parseParenthesized();

    //==============================================================================

    std::vector<YdspToken> tokens;
    YdspDiagnostics& diagnostics;
    size_t index = 0;

    int recursionDepth = 0;
};

} // namespace yup
