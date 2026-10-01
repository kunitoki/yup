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

namespace yup
{

namespace wgsl
{

namespace
{

//==============================================================================
// Token types
//==============================================================================

enum class TokenType
{
    // Literals
    identifier,
    intConst,
    uintConst,
    floatConst,
    boolConst,

    // Punctuation
    semicolon, // ;
    comma,     // ,
    lParen,    // (
    rParen,    // )
    lBrace,    // {
    rBrace,    // }
    lBracket,  // [
    rBracket,  // ]
    dot,       // .
    question,  // ?
    colon,     // :
    hash,      // #

    // Operators
    plus,    // +
    minus,   // -
    star,    // *
    slash,   // /
    percent, // %
    lt,      // <
    gt,      // >
    le,      // <=
    ge,      // >=
    eq,      // ==
    ne,      // !=
    amp,     // &
    caret,   // ^
    pipe,    // |
    land,    // &&
    lor,     // ||
    lxor,    // ^^
    lnot,    // !
    bnot,    // ~
    lshift,  // <<
    rshift,  // >>
    inc,     // ++
    dec,     // --

    // Assignment operators
    assign,       // =
    addAssign,    // +=
    subAssign,    // -=
    mulAssign,    // *=
    divAssign,    // /=
    modAssign,    // %=
    lshiftAssign, // <<=
    rshiftAssign, // >>=
    andAssign,    // &=
    xorAssign,    // ^=
    orAssign,     // |=

    endOfFile
};

const char* toString (TokenType tt)
{
    static constexpr const char* names[] = {
        "identifier", "intConst", "uintConst", "floatConst", "boolConst", ";", ",", "(", ")", "{", "}", "[", "]", ".", "?", ":", "#", "+", "-", "*", "/", "%", "<", ">", "<=", ">=", "==", "!=", "&", "^", "|", "&&", "||", "^^", "!", "~", "<<", ">>", "++", "--", "=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=", "EOF"
    };

    static_assert (std::size (names) == static_cast<std::size_t> (TokenType::endOfFile) + 1);
    return names[static_cast<std::size_t> (tt)];
}

bool isAssignmentOp (TokenType tt)
{
    switch (tt)
    {
        case TokenType::assign:
        case TokenType::addAssign:
        case TokenType::subAssign:
        case TokenType::mulAssign:
        case TokenType::divAssign:
        case TokenType::modAssign:
        case TokenType::lshiftAssign:
        case TokenType::rshiftAssign:
        case TokenType::andAssign:
        case TokenType::xorAssign:
        case TokenType::orAssign:
            return true;
        default:
            return false;
    }
}

//==============================================================================
// Token
//==============================================================================

struct Token
{
    TokenType type = TokenType::endOfFile;
    SourceLocation loc;
    std::string text; // raw text of the token
    int64_t intValue = 0;
    unsigned int uintValue = 0;
    double floatValue = 0.0;
    bool isDouble = false;
    bool boolValue = false;
};

[[noreturn]] void throwError (SourceLocation l, const std::string& message)
{
    throw std::runtime_error (std::to_string (l.line) + ":" + std::to_string (l.column) + ": " + message);
}

std::string describe (const Token& tok)
{
    if (tok.type == TokenType::endOfFile)
        return "end of file";

    return tok.text.empty() ? std::string (toString (tok.type)) : tok.text;
}

//==============================================================================
// Lexer: tokenizes preprocessed GLSL source
//==============================================================================

class Lexer
{
public:
    explicit Lexer (const std::string& source)
        : src (source)
    {
        loc.line = 1;
        loc.column = 1;

        // UTF-8 byte order mark
        if (src.size() >= 3 && static_cast<unsigned char> (src[0]) == 0xef
            && static_cast<unsigned char> (src[1]) == 0xbb
            && static_cast<unsigned char> (src[2]) == 0xbf)
            pos = 3;
    }

    const Token& peek() { return peekAt (0); }

    const Token& peekAt (size_t index)
    {
        while (lookahead.size() <= index)
            lookahead.push_back (lexToken());

        return lookahead[index];
    }

    Token advance()
    {
        peek();

        Token t = std::move (lookahead.front());
        lookahead.pop_front();
        return t;
    }

    std::vector<PreprocessorDirective> takeDirectives()
    {
        return std::exchange (directives, {});
    }

private:
    Token lexToken()
    {
        skipTrivia();

        currentLoc = loc;

        if (pos >= src.size())
            return makeToken (TokenType::endOfFile);

        const char c = src[pos];

        if (isAlpha (c) || c == '_')
            return lexIdentifier();

        if (isDigit (c) || (c == '.' && pos + 1 < src.size() && isDigit (src[pos + 1])))
            return lexNumber();

        const auto single = [this] (TokenType tt)
        {
            advanceChar();
            return makeToken (tt, 1);
        };

        switch (c)
        {
            case ';':
                return single (TokenType::semicolon);
            case ',':
                return single (TokenType::comma);
            case '(':
                return single (TokenType::lParen);
            case ')':
                return single (TokenType::rParen);
            case '{':
                return single (TokenType::lBrace);
            case '}':
                return single (TokenType::rBrace);
            case '[':
                return single (TokenType::lBracket);
            case ']':
                return single (TokenType::rBracket);
            case '.':
                return single (TokenType::dot);
            case '?':
                return single (TokenType::question);
            case ':':
                return single (TokenType::colon);
            case '~':
                return single (TokenType::bnot);

            case '+':
                return lexOperator ({ { "++", TokenType::inc }, { "+=", TokenType::addAssign }, { "+", TokenType::plus } });
            case '-':
                return lexOperator ({ { "--", TokenType::dec }, { "-=", TokenType::subAssign }, { "-", TokenType::minus } });
            case '*':
                return lexOperator ({ { "*=", TokenType::mulAssign }, { "*", TokenType::star } });
            case '/':
                return lexOperator ({ { "/=", TokenType::divAssign }, { "/", TokenType::slash } });
            case '%':
                return lexOperator ({ { "%=", TokenType::modAssign }, { "%", TokenType::percent } });
            case '<':
                return lexOperator ({ { "<<=", TokenType::lshiftAssign }, { "<<", TokenType::lshift }, { "<=", TokenType::le }, { "<", TokenType::lt } });
            case '>':
                return lexOperator ({ { ">>=", TokenType::rshiftAssign }, { ">>", TokenType::rshift }, { ">=", TokenType::ge }, { ">", TokenType::gt } });
            case '=':
                return lexOperator ({ { "==", TokenType::eq }, { "=", TokenType::assign } });
            case '!':
                return lexOperator ({ { "!=", TokenType::ne }, { "!", TokenType::lnot } });
            case '&':
                return lexOperator ({ { "&&", TokenType::land }, { "&=", TokenType::andAssign }, { "&", TokenType::amp } });
            case '|':
                return lexOperator ({ { "||", TokenType::lor }, { "|=", TokenType::orAssign }, { "|", TokenType::pipe } });
            case '^':
                return lexOperator ({ { "^^", TokenType::lxor }, { "^=", TokenType::xorAssign }, { "^", TokenType::caret } });

            default:
                throwError (loc, String::formatted ("Unexpected character '%c' (0x%02x)", c, static_cast<unsigned> (static_cast<unsigned char> (c))).toStdString());
        }
    }

    Token lexOperator (std::initializer_list<std::pair<const char*, TokenType>> candidates)
    {
        for (const auto& [text, type] : candidates)
        {
            const auto length = std::strlen (text);
            if (src.compare (pos, length, text) == 0)
            {
                for (size_t i = 0; i < length; ++i)
                    advanceChar();

                return makeToken (type, length);
            }
        }

        throwError (loc, "Unexpected character");
    }

    //==========================================================================
    void skipTrivia()
    {
        while (pos < src.size())
        {
            const char c = src[pos];

            if (c == ' ' || c == '\t' || c == '\f' || c == '\v')
            {
                advanceChar();
            }
            else if (c == '\n' || c == '\r')
            {
                consumeNewline();
            }
            else if (c == '/' && pos + 1 < src.size() && src[pos + 1] == '/')
            {
                while (pos < src.size() && src[pos] != '\n' && src[pos] != '\r')
                    advanceChar();
            }
            else if (c == '/' && pos + 1 < src.size() && src[pos + 1] == '*')
            {
                const auto start = loc;
                advanceChar();
                advanceChar();

                while (pos < src.size() && ! (src[pos] == '*' && pos + 1 < src.size() && src[pos + 1] == '/'))
                {
                    if (src[pos] == '\n' || src[pos] == '\r')
                        consumeNewline();
                    else
                        advanceChar();
                }

                if (pos >= src.size())
                    throwError (start, "Unterminated block comment");

                advanceChar();
                advanceChar();
            }
            else if (c == '#' && atLineStart)
            {
                lexDirective();
            }
            else
            {
                break;
            }
        }

        atLineStart = false;
    }

    void consumeNewline()
    {
        if (src[pos] == '\r' && pos + 1 < src.size() && src[pos + 1] == '\n')
            ++pos;

        ++pos;
        ++loc.line;
        loc.column = 1;
        atLineStart = true;
    }

    /** Consumes a whole preprocessor line. Only directives that survive preprocessing are accepted. */
    void lexDirective()
    {
        const auto start = loc;
        const auto begin = pos;

        while (pos < src.size() && src[pos] != '\n' && src[pos] != '\r')
            advanceChar();

        auto text = String (src.substr (begin + 1, pos - begin - 1)).trim();
        const auto keyword = text.upToFirstOccurrenceOf (" ", false, false).trim().toStdString();

        if (keyword == "line")
        {
            const auto arguments = StringArray::fromTokens (text.fromFirstOccurrenceOf ("line", false, false).trim(), true);
            if (arguments.isEmpty() || arguments[0].isEmpty() || ! arguments[0].containsOnly ("0123456789"))
                throwError (start, "Malformed #line directive");

            if (pos < src.size())
                consumeNewline();

            loc.line = arguments[0].getIntValue();
            loc.column = 1;
        }
        else if (keyword.empty() || keyword == "version" || keyword == "extension" || keyword == "pragma")
        {
            if (! keyword.empty())
                directives.push_back ({ start, text.toStdString() });
        }
        else
        {
            throwError (start, "Preprocessor directive '#" + keyword + "' must be resolved before transpiling (run the glslang preprocessor first)");
        }
    }

    void advanceChar()
    {
        ++pos;
        ++loc.column;
    }

    static bool isAlpha (char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    static bool isDigit (char c)
    {
        return c >= '0' && c <= '9';
    }

    static bool isHexDigit (char c)
    {
        return isDigit (c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    static bool isIdentifierChar (char c)
    {
        return isAlpha (c) || isDigit (c) || c == '_';
    }

    Token lexIdentifier()
    {
        const size_t start = pos;
        while (pos < src.size() && isIdentifierChar (src[pos]))
            advanceChar();

        Token tok = makeToken (TokenType::identifier);
        tok.text = src.substr (start, pos - start);

        if (tok.text == "true" || tok.text == "false")
        {
            tok.type = TokenType::boolConst;
            tok.boolValue = tok.text == "true";
        }

        return tok;
    }

    //==========================================================================
    Token lexNumber()
    {
        const size_t begin = pos;

        if (src[pos] == '0' && pos + 1 < src.size() && (src[pos + 1] == 'x' || src[pos + 1] == 'X'))
        {
            advanceChar();
            advanceChar();

            const size_t digitsBegin = pos;
            while (pos < src.size() && isHexDigit (src[pos]))
                advanceChar();

            if (pos == digitsBegin)
                throwError (currentLoc, "Hexadecimal literal has no digits");

            return makeIntegerToken (begin, src.substr (digitsBegin, pos - digitsBegin), 16);
        }

        bool isFloat = false;

        while (pos < src.size() && isDigit (src[pos]))
            advanceChar();

        if (pos < src.size() && src[pos] == '.')
        {
            isFloat = true;
            advanceChar();

            while (pos < src.size() && isDigit (src[pos]))
                advanceChar();
        }

        if (pos < src.size() && (src[pos] == 'e' || src[pos] == 'E'))
        {
            size_t exponent = pos + 1;
            if (exponent < src.size() && (src[exponent] == '+' || src[exponent] == '-'))
                ++exponent;

            if (exponent < src.size() && isDigit (src[exponent]))
            {
                isFloat = true;

                while (pos < exponent)
                    advanceChar();

                while (pos < src.size() && isDigit (src[pos]))
                    advanceChar();
            }
        }

        const size_t numberEnd = pos;
        const std::string number = src.substr (begin, numberEnd - begin);

        if (! isFloat && pos < src.size() && (src[pos] == 'u' || src[pos] == 'U'))
        {
            auto tok = number.size() > 1 && number[0] == '0'
                         ? makeIntegerToken (begin, checkedOctalDigits (number), 8)
                         : makeIntegerToken (begin, number, 10);
            return tok;
        }

        bool isDouble = false;
        if (pos + 1 < src.size() && (src.compare (pos, 2, "lf") == 0 || src.compare (pos, 2, "LF") == 0))
        {
            isFloat = true;
            isDouble = true;
            advanceChar();
            advanceChar();
        }
        else if (pos < src.size() && (src[pos] == 'f' || src[pos] == 'F'))
        {
            isFloat = true;
            advanceChar();
        }

        if (! isFloat)
        {
            return number.size() > 1 && number[0] == '0'
                     ? makeIntegerToken (begin, checkedOctalDigits (number), 8)
                     : makeIntegerToken (begin, number, 10);
        }

        rejectTrailingIdentifierChars();

        std::istringstream stream (number);
        stream.imbue (std::locale::classic());

        double value = 0.0;
        stream >> value;

        if (stream.fail())
            throwError (currentLoc, "Floating point literal '" + number + "' is out of range");

        Token tok = makeToken (TokenType::floatConst);
        tok.text = src.substr (begin, pos - begin);
        tok.floatValue = value;
        tok.isDouble = isDouble;
        return tok;
    }

    std::string checkedOctalDigits (const std::string& number) const
    {
        for (const char c : number)
        {
            if (c < '0' || c > '7')
                throwError (currentLoc, "Invalid digit '" + std::string (1, c) + "' in octal literal '" + number + "'");
        }

        return number.substr (1);
    }

    Token makeIntegerToken (size_t begin, const std::string& digits, int radix)
    {
        bool isUnsigned = false;
        if (pos < src.size() && (src[pos] == 'u' || src[pos] == 'U'))
        {
            isUnsigned = true;
            advanceChar();
        }

        rejectTrailingIdentifierChars();

        const auto text = src.substr (begin, pos - begin);

        uint64_t value = 0;
        for (const char c : digits)
        {
            const int digit = isDigit (c) ? c - '0' : (c >= 'a' ? c - 'a' + 10 : c - 'A' + 10);
            value = value * static_cast<uint64_t> (radix) + static_cast<uint64_t> (digit);

            if (value > 0xffffffffull)
                throwError (currentLoc, "Integer literal '" + text + "' does not fit in 32 bits");
        }

        Token tok = makeToken (isUnsigned ? TokenType::uintConst : TokenType::intConst);
        tok.text = text;

        if (isUnsigned)
        {
            tok.uintValue = static_cast<unsigned int> (value);
        }
        else
        {
            // Signed literals keep their 32-bit pattern: 0xFFFFFFFF and 4294967295 are both -1
            tok.intValue = static_cast<int32_t> (static_cast<uint32_t> (value));
        }

        return tok;
    }

    void rejectTrailingIdentifierChars()
    {
        if (pos < src.size() && isIdentifierChar (src[pos]))
            throwError (currentLoc, "Invalid suffix '" + std::string (1, src[pos]) + "' on numeric literal");
    }

    Token makeToken (TokenType tt, size_t length = 0)
    {
        Token tok;
        tok.type = tt;
        tok.loc = currentLoc;

        if (length > 0)
            tok.text = src.substr (pos - length, length);

        return tok;
    }

    const std::string& src;
    size_t pos = 0;
    bool atLineStart = true;
    SourceLocation loc;
    SourceLocation currentLoc;
    std::deque<Token> lookahead;
    std::vector<PreprocessorDirective> directives;
};

//==============================================================================
// Keyword and name tables
//==============================================================================

bool isKeyword (const Token& tok, const char* kw)
{
    return tok.type == TokenType::identifier && tok.text == kw;
}

std::optional<LayoutQualifierId> findLayoutQualifier (const std::string& name)
{
    static constexpr std::pair<std::string_view, LayoutQualifierId> names[] = {
        { "location", LayoutQualifierId::location },
        { "binding", LayoutQualifierId::binding },
        { "set", LayoutQualifierId::descriptorSet },
        { "component", LayoutQualifierId::component },
        { "offset", LayoutQualifierId::offset },
        { "align", LayoutQualifierId::align },
        { "index", LayoutQualifierId::index },
        { "push_constant", LayoutQualifierId::pushConstant },
        { "constant_id", LayoutQualifierId::constantId },
        { "local_size_x", LayoutQualifierId::localSizeX },
        { "local_size_y", LayoutQualifierId::localSizeY },
        { "local_size_z", LayoutQualifierId::localSizeZ },
        { "local_size_x_id", LayoutQualifierId::localSizeXId },
        { "local_size_y_id", LayoutQualifierId::localSizeYId },
        { "local_size_z_id", LayoutQualifierId::localSizeZId },
        { "vertices", LayoutQualifierId::vertices },
        { "invocations", LayoutQualifierId::invocations },
        { "max_vertices", LayoutQualifierId::maxVertices },
        { "stream", LayoutQualifierId::stream },
        { "spacing", LayoutQualifierId::tessSpacing },
        { "vertex_spacing", LayoutQualifierId::tessVertices },
        { "output_topology", LayoutQualifierId::tessOutputTopology },
        { "input_topology", LayoutQualifierId::tessInputMode },
        { "points", LayoutQualifierId::points },
        { "lines", LayoutQualifierId::lines },
        { "line_strip", LayoutQualifierId::lineStrip },
        { "lines_adjacency", LayoutQualifierId::linesAdjacency },
        { "triangles", LayoutQualifierId::triangles },
        { "triangle_strip", LayoutQualifierId::triangleStrip },
        { "triangles_adjacency", LayoutQualifierId::trianglesAdjacency },
        { "fractional_even_spacing", LayoutQualifierId::fractionalEvenSpacing },
        { "fractional_odd_spacing", LayoutQualifierId::fractionalOddSpacing },
        { "equal_spacing", LayoutQualifierId::equalSpacing },
        { "cw", LayoutQualifierId::cw },
        { "ccw", LayoutQualifierId::ccw },
        { "isolines", LayoutQualifierId::isolines },
        { "quads", LayoutQualifierId::quads },
        { "xfb_buffer", LayoutQualifierId::xfbBuffer },
        { "xfb_stride", LayoutQualifierId::xfbStride },
        { "xfb_offset", LayoutQualifierId::xfbOffset },
        { "input_attachment_index", LayoutQualifierId::inputAttachmentIndex },
        { "std140", LayoutQualifierId::std140 },
        { "std430", LayoutQualifierId::std430 },
        { "shared", LayoutQualifierId::sharedLayout },
        { "packed", LayoutQualifierId::packedLayout },
        { "scalar", LayoutQualifierId::scalarLayout },
        { "column_major", LayoutQualifierId::columnMajor },
        { "row_major", LayoutQualifierId::rowMajor },
        { "early_fragment_tests", LayoutQualifierId::earlyFragmentTests },
        { "depth_greater", LayoutQualifierId::depthGreater },
        { "depth_less", LayoutQualifierId::depthLess },
        { "depth_unchanged", LayoutQualifierId::depthUnchanged },
        { "depth_any", LayoutQualifierId::depthAny },
        { "origin_upper_left", LayoutQualifierId::originUpperLeft },
        { "pixel_center_integer", LayoutQualifierId::pixelCenterInteger },
        { "buffer_reference", LayoutQualifierId::bufferReference }
    };

    for (const auto& [candidate, id] : names)
    {
        if (candidate == name)
            return id;
    }

    return std::nullopt;
}

bool isImageFormatName (const std::string& name)
{
    static const std::unordered_set<std::string> formats = {
        "rgba32f", "rgba16f", "rg32f", "rg16f", "r11f_g11f_b10f", "r32f", "r16f",
        "rgba16", "rgb10_a2", "rgba8", "rg16", "rg8", "r16", "r8",
        "rgba16_snorm", "rgba8_snorm", "rg16_snorm", "rg8_snorm", "r16_snorm", "r8_snorm",
        "rgba32i", "rgba16i", "rgba8i", "rg32i", "rg16i", "rg8i", "r32i", "r16i", "r8i",
        "rgba32ui", "rgba16ui", "rgb10_a2ui", "rgba8ui", "rg32ui", "rg16ui", "rg8ui", "r32ui", "r16ui", "r8ui"
    };

    return formats.count (name) > 0;
}

//==============================================================================
// GLSL 4.50 / ESSL 3.x recursive descent parser
//==============================================================================

class Parser
{
public:
    explicit Parser (Lexer& lex)
        : lexer (lex)
    {
    }

    TranslationUnit parseTranslationUnit()
    {
        char probe = 0;
        stackBase = reinterpret_cast<std::uintptr_t> (&probe);

        TranslationUnit unit;
        unit.loc = loc();

        for (;;)
        {
            const auto type = lexer.peek().type;
            flushDirectives (unit);

            if (type == TokenType::endOfFile)
                break;

            if (match (TokenType::semicolon))
                continue;

            parseExternalDeclaration (unit.declarations);
        }

        return unit;
    }

private:
    static constexpr int maxStatementDepth = 128;
    static constexpr int maxExpressionDepth = 64;
    static constexpr int maxChainLength = 1024;
    static constexpr std::uintptr_t maxStackBytes = 256 * 1024;

    /** Bounds the recursion of the descent so hostile input fails cleanly instead of overflowing the stack. */
    struct DepthGuard
    {
        DepthGuard (Parser& p, SourceLocation l, bool isStatement = false)
            : parser (p)
            , counter (isStatement ? p.statementDepth : p.expressionDepth)
        {
            char probe = 0;
            const auto address = reinterpret_cast<std::uintptr_t> (&probe);
            const auto used = parser.stackBase > address ? parser.stackBase - address : address - parser.stackBase;

            if (++counter > (isStatement ? maxStatementDepth : maxExpressionDepth) || used > maxStackBytes)
                throwError (l, "Nesting is too deep");
        }

        ~DepthGuard() { --counter; }

        Parser& parser;
        int& counter;
    };

    //==========================================================================
    SourceLocation loc() { return lexer.peek().loc; }

    void flushDirectives (TranslationUnit& unit)
    {
        for (auto& directive : lexer.takeDirectives())
            unit.declarations.push_back (std::move (directive));
    }

    bool match (TokenType tt)
    {
        if (lexer.peek().type != tt)
            return false;

        lexer.advance();
        return true;
    }

    bool matchKeyword (const char* kw)
    {
        if (! isKeyword (lexer.peek(), kw))
            return false;

        lexer.advance();
        return true;
    }

    Token expect (TokenType tt)
    {
        if (lexer.peek().type == tt)
            return lexer.advance();

        throwError (lexer.peek().loc, std::string ("Expected '") + toString (tt) + "', got '" + describe (lexer.peek()) + "'");
    }

    Token expectKeyword (const char* kw)
    {
        if (isKeyword (lexer.peek(), kw))
            return lexer.advance();

        throwError (lexer.peek().loc, std::string ("Expected keyword '") + kw + "', got '" + describe (lexer.peek()) + "'");
    }

    Token expectIdentifier (const char* what)
    {
        if (lexer.peek().type == TokenType::identifier)
            return lexer.advance();

        throwError (lexer.peek().loc, std::string ("Expected ") + what + ", got '" + describe (lexer.peek()) + "'");
    }

    //==========================================================================
    // External declarations
    //==========================================================================

    void parseExternalDeclaration (std::vector<ExternalDeclaration>& out)
    {
        const SourceLocation l = loc();

        if (isKeyword (lexer.peek(), "precision"))
        {
            skipPrecisionStatement();
            return;
        }

        auto qualifier = parseTypeQualifier();

        if (matchKeyword ("struct"))
        {
            auto ss = parseStructSpecifier (l);
            const auto structName = ss.name;

            Declaration structDecl;
            structDecl.loc = l;
            structDecl.structSpecifier = std::make_unique<StructSpecifier> (std::move (ss));
            out.push_back (std::move (structDecl));

            if (match (TokenType::semicolon))
                return;

            // struct S { ... } s; declares the type and then a variable of that type
            auto name = expectIdentifier ("identifier");
            Declaration varDecl;
            varDecl.loc = l;
            varDecl.initDeclaratorList = parseInitDeclaratorList (std::move (qualifier), TypeSpecifier::makeNamed (l, structName), name, l);
            expect (TokenType::semicolon);
            out.push_back (std::move (varDecl));
            return;
        }

        // Standalone qualifier declaration: layout(local_size_x = 8) in;
        if (qualifier && match (TokenType::semicolon))
        {
            Declaration decl;
            decl.loc = l;
            decl.qualifier = std::make_unique<TypeQualifier> (std::move (*qualifier));
            out.push_back (std::move (decl));
            return;
        }

        // Qualifier-only redeclaration: invariant gl_Position; precise x, y;
        if (qualifier && lexer.peek().type == TokenType::identifier && ! isTypeStart (lexer.peek())
            && (lexer.peekAt (1).type == TokenType::semicolon || lexer.peekAt (1).type == TokenType::comma))
        {
            Declaration decl;
            decl.loc = l;
            decl.qualifier = std::make_unique<TypeQualifier> (std::move (*qualifier));

            do
                decl.qualifiedNames.push_back (expectIdentifier ("identifier").text);
            while (match (TokenType::comma));

            expect (TokenType::semicolon);
            out.push_back (std::move (decl));
            return;
        }

        // Interface block: qualifier [BlockName] { members } [instance];
        if (qualifier && isBlockQualifier (*qualifier)
            && (lexer.peek().type == TokenType::lBrace
                || (lexer.peek().type == TokenType::identifier && lexer.peekAt (1).type == TokenType::lBrace)))
        {
            out.push_back (parseInterfaceBlock (std::move (*qualifier), l));
            return;
        }

        auto type = parseTypeSpecifier();
        auto name = expectIdentifier ("identifier after type");

        if (lexer.peek().type == TokenType::lParen)
        {
            if (auto function = parseFunction (std::move (type), name.text, l))
                out.push_back (std::move (*function));

            return;
        }

        Declaration decl;
        decl.loc = l;
        decl.initDeclaratorList = parseInitDeclaratorList (std::move (qualifier), std::move (type), name, l);
        expect (TokenType::semicolon);
        out.push_back (std::move (decl));
    }

    static bool isBlockQualifier (const TypeQualifier& q)
    {
        return q.hasStorage (StorageQualifier::uniform) || q.hasStorage (StorageQualifier::buffer)
            || q.hasStorage (StorageQualifier::in) || q.hasStorage (StorageQualifier::out);
    }

    Declaration parseInterfaceBlock (TypeQualifier qualifier, SourceLocation l)
    {
        StructSpecifier block;
        block.loc = l;

        if (lexer.peek().type == TokenType::identifier)
            block.name = lexer.advance().text;

        parseStructMembers (block);

        Declaration decl;
        decl.loc = l;
        decl.qualifier = std::make_unique<TypeQualifier> (std::move (qualifier));

        if (lexer.peek().type == TokenType::identifier)
        {
            auto instance = lexer.advance();
            decl.initDeclaratorList = parseInitDeclaratorList (copyTypeQualifier (decl.qualifier.get()),
                                                               TypeSpecifier::makeNamed (l, block.name),
                                                               instance,
                                                               l);
        }

        decl.structSpecifier = std::make_unique<StructSpecifier> (std::move (block));
        expect (TokenType::semicolon);
        return decl;
    }

    std::unique_ptr<InitDeclaratorList> parseInitDeclaratorList (std::optional<TypeQualifier> qualifier,
                                                                 TypeSpecifier type,
                                                                 const Token& firstName,
                                                                 SourceLocation l)
    {
        return parseInitDeclaratorList (qualifier ? std::make_unique<TypeQualifier> (std::move (*qualifier)) : nullptr,
                                        std::move (type),
                                        firstName,
                                        l);
    }

    std::unique_ptr<InitDeclaratorList> parseInitDeclaratorList (std::unique_ptr<TypeQualifier> qualifier,
                                                                 TypeSpecifier type,
                                                                 const Token& firstName,
                                                                 SourceLocation l)
    {
        auto list = std::make_unique<InitDeclaratorList>();
        list->loc = l;
        list->qualifier = std::move (qualifier);
        list->type = std::move (type);
        list->declarations.push_back (parseSingleDeclarationRest (firstName));

        while (match (TokenType::comma))
            list->declarations.push_back (parseSingleDeclarationRest (expectIdentifier ("identifier")));

        return list;
    }

    SingleDeclaration parseSingleDeclarationRest (const Token& name)
    {
        SingleDeclaration decl;
        decl.loc = name.loc;
        decl.name = name.text;

        while (lexer.peek().type == TokenType::lBracket)
            decl.arraySpecifiers.push_back (parseArraySpecifier());

        if (match (TokenType::assign))
            decl.initializer = std::make_unique<Initializer> (parseInitializer());

        return decl;
    }

    void skipPrecisionStatement()
    {
        expectKeyword ("precision");

        if (! (matchKeyword ("lowp") || matchKeyword ("mediump") || matchKeyword ("highp")))
            throwError (loc(), "Expected precision qualifier, got '" + describe (lexer.peek()) + "'");

        parseTypeSpecifier();
        expect (TokenType::semicolon);
    }

    //==========================================================================
    // Functions
    //==========================================================================

    std::optional<FunctionDefinition> parseFunction (TypeSpecifier returnType, const std::string& name, SourceLocation l)
    {
        FunctionPrototype proto;
        proto.loc = l;
        proto.returnType = std::move (returnType);
        proto.name = name;

        expect (TokenType::lParen);

        if (isKeyword (lexer.peek(), "void") && lexer.peekAt (1).type == TokenType::rParen)
            lexer.advance();
        else if (lexer.peek().type != TokenType::rParen)
        {
            do
                proto.parameters.push_back (parseFunctionParameter());
            while (match (TokenType::comma));
        }

        expect (TokenType::rParen);

        // Prototypes carry no information WGSL needs: functions can be called before their definition
        if (match (TokenType::semicolon))
            return std::nullopt;

        if (lexer.peek().type != TokenType::lBrace)
            throwError (loc(), "Expected '{' or ';' after function declaration, got '" + describe (lexer.peek()) + "'");

        FunctionDefinition function;
        function.loc = l;
        function.prototype = std::move (proto);
        function.body = std::make_unique<Statement> (parseCompoundStatement());
        return function;
    }

    FunctionParameterDeclaration parseFunctionParameter()
    {
        FunctionParameterDeclaration param;
        param.loc = loc();

        if (auto qualifier = parseTypeQualifier())
        {
            for (const auto storage : qualifier->storage)
            {
                if (storage != StorageQualifier::constQual && storage != StorageQualifier::in
                    && storage != StorageQualifier::out && storage != StorageQualifier::inout)
                    throwError (qualifier->loc, "Invalid qualifier on function parameter");
            }

            if (qualifier->layout != nullptr || ! qualifier->interpolation.empty() || qualifier->invariant)
                throwError (qualifier->loc, "Invalid qualifier on function parameter");

            param.qualifier = std::make_unique<TypeQualifier> (std::move (*qualifier));
        }

        param.type = parseTypeSpecifier();

        if (lexer.peek().type == TokenType::identifier)
            param.name = lexer.advance().text;

        while (lexer.peek().type == TokenType::lBracket)
            param.arraySpecifiers.push_back (parseArraySpecifier());

        return param;
    }

    //==========================================================================
    // Statements
    //==========================================================================

    Statement makeStatement (SourceLocation l, StatementVariant value)
    {
        Statement s;
        s.loc = l;
        s.value = std::move (value);
        return s;
    }

    Statement makeJump (SourceLocation l, JumpKind kind)
    {
        expect (TokenType::semicolon);
        return makeStatement (l, StmtJump { l, kind, nullptr });
    }

    Statement parseStatement()
    {
        DepthGuard guard (*this, loc(), true);

        skipAttributes();

        const SourceLocation l = loc();

        if (lexer.peek().type == TokenType::lBrace)
            return parseCompoundStatement();

        if (matchKeyword ("if"))
            return parseIfStatement (l);
        if (matchKeyword ("switch"))
            return parseSwitchStatement (l);
        if (matchKeyword ("for"))
            return parseForStatement (l);
        if (matchKeyword ("while"))
            return parseWhileStatement (l);
        if (matchKeyword ("do"))
            return parseDoWhileStatement (l);
        if (matchKeyword ("return"))
            return parseReturnStatement (l);
        if (matchKeyword ("break"))
            return makeJump (l, JumpKind::breakJump);
        if (matchKeyword ("continue"))
            return makeJump (l, JumpKind::continueJump);
        if (matchKeyword ("discard") || matchKeyword ("demote"))
            return makeJump (l, JumpKind::discardJump);

        if (isKeyword (lexer.peek(), "case") || isKeyword (lexer.peek(), "default"))
            throwError (l, "'" + lexer.peek().text + "' label outside of a switch statement");

        if (isKeyword (lexer.peek(), "precision"))
        {
            skipPrecisionStatement();
            return Statement::makeEmpty (l);
        }

        if (match (TokenType::semicolon))
            return Statement::makeEmpty (l);

        return parseSimpleStatement();
    }

    /** A declaration or expression statement, including its terminating semicolon. */
    Statement parseSimpleStatement()
    {
        const SourceLocation l = loc();

        if (isDeclarationStart())
        {
            auto decl = parseDeclaration();
            expect (TokenType::semicolon);
            return makeStatement (l, StmtDeclaration { l, std::move (decl) });
        }

        auto expr = parseExpression();
        expect (TokenType::semicolon);
        return makeStatement (l, StmtExpr { l, std::make_unique<Expr> (std::move (expr)) });
    }

    bool isDeclarationStart()
    {
        const auto& tok = lexer.peek();
        if (tok.type != TokenType::identifier)
            return false;

        // A type followed by '(' is a constructor call starting an expression statement
        if (isTypeStart (tok))
            return lexer.peekAt (1).type != TokenType::lParen;

        // Two identifiers in a row can only be a declaration, possibly of an unknown type
        if (lexer.peekAt (1).type == TokenType::identifier)
            return true;

        static const std::unordered_set<std::string> declarationKeywords = {
            "struct", "const", "in", "out", "inout", "uniform", "buffer", "shared", "centroid", "sample",
            "flat", "smooth", "noperspective", "lowp", "mediump", "highp", "invariant", "precise", "layout",
            "readonly", "writeonly", "coherent", "volatile", "restrict"
        };

        return declarationKeywords.count (tok.text) > 0;
    }

    /** Skips [[unroll]] style statement attributes, which only carry optimization hints. */
    void skipAttributes()
    {
        while (lexer.peek().type == TokenType::lBracket && lexer.peekAt (1).type == TokenType::lBracket)
        {
            lexer.advance();
            lexer.advance();

            while (! (lexer.peek().type == TokenType::rBracket && lexer.peekAt (1).type == TokenType::rBracket))
            {
                if (lexer.peek().type == TokenType::endOfFile)
                    throwError (loc(), "Unterminated attribute list");

                lexer.advance();
            }

            lexer.advance();
            lexer.advance();
        }
    }

    Statement parseCompoundStatement()
    {
        const SourceLocation l = loc();
        expect (TokenType::lBrace);

        StmtCompound compound;
        compound.loc = l;

        while (lexer.peek().type != TokenType::rBrace)
        {
            if (lexer.peek().type == TokenType::endOfFile)
                throwError (loc(), "Expected '}' before end of file");

            compound.statements.push_back (parseStatement());
        }

        expect (TokenType::rBrace);
        return makeStatement (l, std::move (compound));
    }

    Statement parseIfStatement (SourceLocation l)
    {
        StmtSelection sel;
        sel.loc = l;

        expect (TokenType::lParen);
        sel.condition = std::make_unique<Expr> (parseExpression());
        expect (TokenType::rParen);

        sel.thenBranch = std::make_unique<Statement> (parseStatement());

        if (matchKeyword ("else"))
            sel.elseBranch = std::make_unique<Statement> (parseStatement());

        return makeStatement (l, std::move (sel));
    }

    Statement parseSwitchStatement (SourceLocation l)
    {
        StmtSwitch sw;
        sw.loc = l;

        expect (TokenType::lParen);
        sw.selector = std::make_unique<Expr> (parseExpression());
        expect (TokenType::rParen);
        expect (TokenType::lBrace);

        while (lexer.peek().type != TokenType::rBrace)
        {
            if (lexer.peek().type == TokenType::endOfFile)
                throwError (loc(), "Expected '}' before end of file");

            const SourceLocation caseLoc = loc();

            if (matchKeyword ("case"))
            {
                auto label = parseConditional();
                expect (TokenType::colon);
                sw.body.push_back (makeStatement (caseLoc, StmtCaseLabel { caseLoc, std::make_unique<Expr> (std::move (label)) }));
            }
            else if (matchKeyword ("default"))
            {
                expect (TokenType::colon);
                sw.body.push_back (makeStatement (caseLoc, StmtCaseLabel { caseLoc, nullptr }));
            }
            else
            {
                sw.body.push_back (parseStatement());
            }
        }

        expect (TokenType::rBrace);
        return makeStatement (l, std::move (sw));
    }

    void rejectDeclarationCondition()
    {
        if (isTypeStart (lexer.peek()) && lexer.peekAt (1).type == TokenType::identifier)
            throwError (loc(), "Declarations in loop conditions are not supported");
    }

    Statement parseForStatement (SourceLocation l)
    {
        StmtFor forStmt;
        forStmt.loc = l;

        expect (TokenType::lParen);

        if (! match (TokenType::semicolon))
            forStmt.init = std::make_unique<Statement> (parseSimpleStatement());

        if (! match (TokenType::semicolon))
        {
            rejectDeclarationCondition();
            forStmt.condition = std::make_unique<Expr> (parseExpression());
            expect (TokenType::semicolon);
        }

        if (! match (TokenType::rParen))
        {
            forStmt.update = std::make_unique<Expr> (parseExpression());
            expect (TokenType::rParen);
        }

        forStmt.body = std::make_unique<Statement> (parseStatement());
        return makeStatement (l, std::move (forStmt));
    }

    Statement parseWhileStatement (SourceLocation l)
    {
        StmtWhile whileStmt;
        whileStmt.loc = l;

        expect (TokenType::lParen);
        rejectDeclarationCondition();
        whileStmt.condition = std::make_unique<Expr> (parseExpression());
        expect (TokenType::rParen);

        whileStmt.body = std::make_unique<Statement> (parseStatement());
        return makeStatement (l, std::move (whileStmt));
    }

    Statement parseDoWhileStatement (SourceLocation l)
    {
        StmtDoWhile doStmt;
        doStmt.loc = l;
        doStmt.body = std::make_unique<Statement> (parseStatement());

        expectKeyword ("while");
        expect (TokenType::lParen);
        doStmt.condition = std::make_unique<Expr> (parseExpression());
        expect (TokenType::rParen);
        expect (TokenType::semicolon);

        return makeStatement (l, std::move (doStmt));
    }

    Statement parseReturnStatement (SourceLocation l)
    {
        StmtJump jump;
        jump.loc = l;
        jump.kind = JumpKind::returnJump;

        if (lexer.peek().type != TokenType::semicolon)
            jump.returnValue = std::make_unique<Expr> (parseExpression());

        expect (TokenType::semicolon);
        return makeStatement (l, std::move (jump));
    }

    //==========================================================================
    // Declarations inside function bodies
    //==========================================================================

    Declaration parseDeclaration()
    {
        const SourceLocation l = loc();

        auto qualifier = parseTypeQualifier();

        Declaration decl;
        decl.loc = l;

        if (matchKeyword ("struct"))
        {
            auto ss = parseStructSpecifier (l);
            const auto structName = ss.name;
            decl.structSpecifier = std::make_unique<StructSpecifier> (std::move (ss));

            // Both the struct and the variables stay in one declaration statement; lowering hoists the struct
            if (lexer.peek().type == TokenType::identifier)
            {
                auto name = lexer.advance();
                decl.initDeclaratorList = parseInitDeclaratorList (std::move (qualifier), TypeSpecifier::makeNamed (l, structName), name, l);
            }

            return decl;
        }

        auto type = parseTypeSpecifier();
        auto name = expectIdentifier ("identifier after type");
        decl.initDeclaratorList = parseInitDeclaratorList (std::move (qualifier), std::move (type), name, l);
        return decl;
    }

    StructSpecifier parseStructSpecifier (SourceLocation l)
    {
        StructSpecifier ss;
        ss.loc = l;

        if (lexer.peek().type == TokenType::identifier)
            ss.name = lexer.advance().text;
        else
            ss.name = "_AnonymousStruct" + std::to_string (anonymousStructCount++);

        parseStructMembers (ss);
        userStructNames.insert (ss.name);
        return ss;
    }

    void parseStructMembers (StructSpecifier& ss)
    {
        expect (TokenType::lBrace);

        while (lexer.peek().type != TokenType::rBrace)
        {
            if (lexer.peek().type == TokenType::endOfFile)
                throwError (loc(), "Expected '}' before end of file");

            parseStructFieldSpecifiers (ss.fields);
        }

        expect (TokenType::rBrace);

        if (ss.fields.empty())
            throwError (ss.loc, "Empty structs and blocks are not allowed");
    }

    /** Parses one struct or interface-block member declaration and appends the
        members it declares to @p fields.

        A single declaration can introduce several members sharing a base type,
        each with its own array specifiers - `uniform Params { float s, r, pad[2]; }` -
        which is why the members are appended rather than returned one at a time.
    */
    void parseStructFieldSpecifiers (std::vector<StructFieldSpecifier>& fields)
    {
        const SourceLocation l = loc();

        auto qualifier = parseTypeQualifier();

        if (isKeyword (lexer.peek(), "struct"))
            throwError (loc(), "Embedded struct definitions are not supported");

        const auto baseType = parseTypeSpecifier();

        do
        {
            StructFieldSpecifier field;
            field.loc = l;
            field.type = baseType;
            field.name = expectIdentifier ("member name").text;

            // Array specifiers bind to the declarator, not to the shared base type.
            while (lexer.peek().type == TokenType::lBracket)
                field.type.arraySpecifiers.push_back (parseArraySpecifier());

            field.qualifier = qualifier ? copyTypeQualifier (&*qualifier) : nullptr;
            fields.push_back (std::move (field));
        } while (match (TokenType::comma));

        expect (TokenType::semicolon);
    }

    ArraySpecifier parseArraySpecifier()
    {
        ArraySpecifier arr;
        arr.loc = loc();

        expect (TokenType::lBracket);

        if (lexer.peek().type == TokenType::rBracket)
            arr.isUnsized = true;
        else
            arr.sizeExpr = std::make_unique<Expr> (parseConditional());

        expect (TokenType::rBracket);
        return arr;
    }

    Initializer parseInitializer()
    {
        DepthGuard guard (*this, loc());

        Initializer init;
        init.loc = loc();

        if (match (TokenType::lBrace))
        {
            do
            {
                if (lexer.peek().type == TokenType::rBrace)
                    break; // trailing comma

                init.aggregate.push_back (parseInitializer());
            } while (match (TokenType::comma));

            expect (TokenType::rBrace);

            if (init.aggregate.empty())
                throwError (init.loc, "Empty initializer list");

            return init;
        }

        init.expr = std::make_unique<Expr> (parseAssignment());
        return init;
    }

    //==========================================================================
    // Expressions - full GLSL precedence ladder
    //==========================================================================

    static Expr makeExpr (SourceLocation l, ExprVariant value)
    {
        Expr e;
        e.loc = l;
        e.value = std::move (value);
        return e;
    }

    static Expr makeBinary (SourceLocation l, Expr left, BinaryOp op, Expr right)
    {
        return makeExpr (l, ExprBinary { l, op, std::make_unique<Expr> (std::move (left)), std::make_unique<Expr> (std::move (right)) });
    }

    static Expr makeUnary (SourceLocation l, UnaryOp op, Expr operand)
    {
        return makeExpr (l, ExprUnary { l, op, std::make_unique<Expr> (std::move (operand)) });
    }

    static void checkChainLength (int& length, SourceLocation l)
    {
        if (++length > maxChainLength)
            throwError (l, "Expression is too long");
    }

    Expr parseExpression()
    {
        const SourceLocation l = loc();
        auto left = parseAssignment();

        for (int length = 0; match (TokenType::comma);)
        {
            checkChainLength (length, l);
            auto right = parseAssignment();
            left = makeExpr (l, ExprComma { l, std::make_unique<Expr> (std::move (left)), std::make_unique<Expr> (std::move (right)) });
        }

        return left;
    }

    Expr parseAssignment()
    {
        const SourceLocation l = loc();
        auto left = parseConditional();

        if (! isAssignmentOp (lexer.peek().type))
            return left;

        const auto op = tokenToAssignmentOp (lexer.advance().type);

        DepthGuard guard (*this, l);
        auto right = parseAssignment();
        return makeExpr (l, ExprAssignment { l, op, std::make_unique<Expr> (std::move (left)), std::make_unique<Expr> (std::move (right)) });
    }

    // conditional -> binary ('?' expression ':' assignment)?
    Expr parseConditional()
    {
        const SourceLocation l = loc();
        auto cond = parseBinary (1);

        if (! match (TokenType::question))
            return cond;

        DepthGuard guard (*this, l);

        auto trueBranch = parseExpression();
        expect (TokenType::colon);
        auto falseBranch = parseAssignment();

        return makeExpr (l, ExprTernary { l,
                                          std::make_unique<Expr> (std::move (cond)),
                                          std::make_unique<Expr> (std::move (trueBranch)),
                                          std::make_unique<Expr> (std::move (falseBranch)) });
    }

    /** GLSL binary operator precedence, lowest first; 0 for tokens that aren't binary operators. */
    static int binaryPrecedence (TokenType type, BinaryOp& op)
    {
        static const std::tuple<TokenType, BinaryOp, int> operators[] = {
            { TokenType::lor, BinaryOp::logicalOr, 1 },
            { TokenType::lxor, BinaryOp::logicalXor, 2 },
            { TokenType::land, BinaryOp::logicalAnd, 3 },
            { TokenType::pipe, BinaryOp::bitwiseOr, 4 },
            { TokenType::caret, BinaryOp::bitwiseXor, 5 },
            { TokenType::amp, BinaryOp::bitwiseAnd, 6 },
            { TokenType::eq, BinaryOp::equal, 7 },
            { TokenType::ne, BinaryOp::notEqual, 7 },
            { TokenType::lt, BinaryOp::lessThan, 8 },
            { TokenType::gt, BinaryOp::greaterThan, 8 },
            { TokenType::le, BinaryOp::lessEqual, 8 },
            { TokenType::ge, BinaryOp::greaterEqual, 8 },
            { TokenType::lshift, BinaryOp::shiftLeft, 9 },
            { TokenType::rshift, BinaryOp::shiftRight, 9 },
            { TokenType::plus, BinaryOp::add, 10 },
            { TokenType::minus, BinaryOp::sub, 10 },
            { TokenType::star, BinaryOp::mul, 11 },
            { TokenType::slash, BinaryOp::div, 11 },
            { TokenType::percent, BinaryOp::mod, 11 }
        };

        for (const auto& [token, binaryOp, precedence] : operators)
        {
            if (token == type)
            {
                op = binaryOp;
                return precedence;
            }
        }

        return 0;
    }

    /** Precedence climbing over all left-associative binary operators: a single frame per level of nesting. */
    Expr parseBinary (int minimumPrecedence)
    {
        auto left = parseUnary();

        for (int length = 0;;)
        {
            BinaryOp op {};
            const auto precedence = binaryPrecedence (lexer.peek().type, op);

            if (precedence == 0 || precedence < minimumPrecedence)
                return left;

            const SourceLocation l = loc();
            checkChainLength (length, l);
            lexer.advance();

            auto right = parseBinary (precedence + 1);
            left = makeBinary (l, std::move (left), op, std::move (right));
        }
    }

    // unary -> ('+'|'-'|'!'|'~'|'++'|'--') unary | postfix
    Expr parseUnary()
    {
        const SourceLocation l = loc();
        DepthGuard guard (*this, l);

        static const std::pair<TokenType, UnaryOp> prefixOps[] = {
            { TokenType::plus, UnaryOp::plus },
            { TokenType::minus, UnaryOp::minus },
            { TokenType::lnot, UnaryOp::logicalNot },
            { TokenType::bnot, UnaryOp::bitwiseNot },
            { TokenType::inc, UnaryOp::preInc },
            { TokenType::dec, UnaryOp::preDec }
        };

        for (const auto& [token, op] : prefixOps)
        {
            if (match (token))
                return makeUnary (l, op, parseUnary());
        }

        return parsePostfix();
    }

    // postfix -> primary ('[' expression ']' | '.' identifier | '(' arguments ')' | '++' | '--')*
    Expr parsePostfix()
    {
        auto left = parsePrimary();

        for (int length = 0;; checkChainLength (length, loc()))
        {
            const SourceLocation l = loc();

            if (match (TokenType::lBracket))
            {
                auto index = parseExpression();
                expect (TokenType::rBracket);
                left = makeExpr (l, ExprBracket { l, std::make_unique<Expr> (std::move (left)), std::make_unique<Expr> (std::move (index)) });
            }
            else if (match (TokenType::dot))
            {
                auto member = expectIdentifier ("member name");
                left = makeExpr (l, ExprDot { l, std::make_unique<Expr> (std::move (left)), member.text });
            }
            else if (match (TokenType::lParen))
            {
                ExprFunCall call;
                call.loc = l;
                call.callee = std::make_unique<Expr> (std::move (left));
                call.args = parseArguments();
                left = makeExpr (l, std::move (call));
            }
            else if (match (TokenType::inc))
            {
                left = makeUnary (l, UnaryOp::postInc, std::move (left));
            }
            else if (match (TokenType::dec))
            {
                left = makeUnary (l, UnaryOp::postDec, std::move (left));
            }
            else
            {
                return left;
            }
        }
    }

    /** Parses a call argument list after the opening parenthesis, including the closing one. */
    std::vector<Expr> parseArguments()
    {
        std::vector<Expr> args;

        if (lexer.peek().type != TokenType::rParen)
        {
            do
                args.push_back (parseAssignment());
            while (match (TokenType::comma));
        }

        expect (TokenType::rParen);
        return args;
    }

    Expr parsePrimary()
    {
        const SourceLocation l = loc();

        if (match (TokenType::lParen))
        {
            DepthGuard guard (*this, l);
            auto inner = parseExpression();
            expect (TokenType::rParen);
            return makeExpr (l, ExprParen { l, std::make_unique<Expr> (std::move (inner)) });
        }

        const auto& tok = lexer.peek();

        switch (tok.type)
        {
            case TokenType::intConst:
            {
                auto t = lexer.advance();
                return makeExpr (t.loc, ExprIntConst { t.loc, t.intValue });
            }

            case TokenType::uintConst:
            {
                auto t = lexer.advance();
                return makeExpr (t.loc, ExprUIntConst { t.loc, t.uintValue });
            }

            case TokenType::floatConst:
            {
                auto t = lexer.advance();
                return makeExpr (t.loc, ExprFloatConst { t.loc, t.floatValue, t.isDouble });
            }

            case TokenType::boolConst:
            {
                auto t = lexer.advance();
                return makeExpr (t.loc, ExprBoolConst { t.loc, t.boolValue });
            }

            default:
                break;
        }

        if (isTypeStart (tok))
        {
            ExprTypeConstructor ctor;
            ctor.loc = l;
            ctor.type = parseTypeSpecifier();

            if (! match (TokenType::lParen))
                throwError (l, "Expected '(' after type in constructor, got '" + describe (lexer.peek()) + "'");

            ctor.args = parseArguments();
            return makeExpr (l, std::move (ctor));
        }

        if (tok.type == TokenType::identifier)
        {
            auto t = lexer.advance();
            return makeExpr (t.loc, ExprVariable { t.loc, t.text });
        }

        throwError (l, "Expected expression, got '" + describe (tok) + "'");
    }

    //==========================================================================
    // Types and qualifiers
    //==========================================================================

    bool isTypeStart (const Token& tok) const
    {
        return tok.type == TokenType::identifier
            && (glslTypeNames().count (tok.text) > 0 || userStructNames.count (tok.text) > 0);
    }

    TypeSpecifier parseTypeSpecifier()
    {
        const SourceLocation l = loc();
        const auto& tok = lexer.peek();

        if (tok.type != TokenType::identifier)
            throwError (l, "Expected type, got '" + describe (tok) + "'");

        if (tok.text == "struct")
            throwError (l, "Embedded struct definitions are not supported");

        TypeSpecifier ts;

        if (auto found = glslTypeNames().find (tok.text); found != glslTypeNames().end())
            ts = TypeSpecifier::make (l, found->second);
        else if (userStructNames.count (tok.text) > 0)
            ts = TypeSpecifier::makeNamed (l, tok.text);
        else
            throwError (l, "Unknown type '" + tok.text + "'");

        lexer.advance();

        while (lexer.peek().type == TokenType::lBracket)
            ts.arraySpecifiers.push_back (parseArraySpecifier());

        return ts;
    }

    std::optional<TypeQualifier> parseTypeQualifier()
    {
        TypeQualifier q;
        q.loc = loc();
        bool hasQualifier = false;

        static const std::pair<const char*, StorageQualifier> storageKeywords[] = {
            { "const", StorageQualifier::constQual },
            { "in", StorageQualifier::in },
            { "out", StorageQualifier::out },
            { "inout", StorageQualifier::inout },
            { "uniform", StorageQualifier::uniform },
            { "buffer", StorageQualifier::buffer },
            { "shared", StorageQualifier::shared },
            { "centroid", StorageQualifier::centroid },
            { "sample", StorageQualifier::sample }
        };

        static const std::pair<const char*, InterpolationQualifier> interpolationKeywords[] = {
            { "flat", InterpolationQualifier::flat },
            { "smooth", InterpolationQualifier::smooth },
            { "noperspective", InterpolationQualifier::noPerspective }
        };

        static const std::pair<const char*, PrecisionQualifier> precisionKeywords[] = {
            { "lowp", PrecisionQualifier::lowp },
            { "mediump", PrecisionQualifier::mediump },
            { "highp", PrecisionQualifier::highp }
        };

        static const std::pair<const char*, MemoryQualifier> memoryKeywords[] = {
            { "readonly", MemoryQualifier::readonlyQual },
            { "writeonly", MemoryQualifier::writeonlyQual },
            { "coherent", MemoryQualifier::coherent },
            { "volatile", MemoryQualifier::volatileQual },
            { "restrict", MemoryQualifier::restrict }
        };

        const auto matchAny = [this] (const auto& table, auto& target)
        {
            for (const auto& [keyword, value] : table)
            {
                if (matchKeyword (keyword))
                {
                    target.push_back (value);
                    return true;
                }
            }

            return false;
        };

        for (;;)
        {
            if (matchKeyword ("layout"))
            {
                // Multiple layout(...) groups on one declaration accumulate
                auto layout = parseLayoutQualifier();

                if (q.layout == nullptr)
                    q.layout = std::make_unique<LayoutQualifier> (std::move (layout));
                else
                    for (auto& entry : layout.entries)
                        q.layout->entries.push_back (std::move (entry));
            }
            else if (matchKeyword ("invariant"))
            {
                q.invariant = true;
            }
            else if (matchKeyword ("precise"))
            {
                q.precise = true;
            }
            else if (! matchAny (storageKeywords, q.storage)
                     && ! matchAny (interpolationKeywords, q.interpolation)
                     && ! matchAny (precisionKeywords, q.precision)
                     && ! matchAny (memoryKeywords, q.memory))
            {
                break;
            }

            hasQualifier = true;
        }

        if (! hasQualifier)
            return std::nullopt;

        return q;
    }

    LayoutQualifier parseLayoutQualifier()
    {
        LayoutQualifier layout;
        layout.loc = loc();

        expect (TokenType::lParen);

        do
            layout.entries.push_back (parseLayoutQualifierEntry());
        while (match (TokenType::comma));

        expect (TokenType::rParen);
        return layout;
    }

    LayoutQualifierEntry parseLayoutQualifierEntry()
    {
        LayoutQualifierEntry entry;
        entry.loc = loc();

        const auto idTok = expectIdentifier ("layout qualifier");
        entry.name = idTok.text;

        if (const auto id = findLayoutQualifier (idTok.text))
            entry.id = *id;
        else if (isImageFormatName (idTok.text))
            entry.id = LayoutQualifierId::imageFormat;
        else
            throwError (idTok.loc, "Unknown layout qualifier '" + idTok.text + "'");

        // Use parseConditional so the comma separating layout ids isn't read as an operator
        if (match (TokenType::assign))
            entry.value = std::make_unique<Expr> (parseConditional());

        return entry;
    }

    static AssignmentOp tokenToAssignmentOp (TokenType tt)
    {
        switch (tt)
        {
            case TokenType::addAssign:
                return AssignmentOp::addAssign;
            case TokenType::subAssign:
                return AssignmentOp::subAssign;
            case TokenType::mulAssign:
                return AssignmentOp::mulAssign;
            case TokenType::divAssign:
                return AssignmentOp::divAssign;
            case TokenType::modAssign:
                return AssignmentOp::modAssign;
            case TokenType::lshiftAssign:
                return AssignmentOp::shiftLeftAssign;
            case TokenType::rshiftAssign:
                return AssignmentOp::shiftRightAssign;
            case TokenType::andAssign:
                return AssignmentOp::bitwiseAndAssign;
            case TokenType::xorAssign:
                return AssignmentOp::bitwiseXorAssign;
            case TokenType::orAssign:
                return AssignmentOp::bitwiseOrAssign;
            default:
                return AssignmentOp::assign;
        }
    }

    Lexer& lexer;
    std::unordered_set<std::string> userStructNames;
    int statementDepth = 0;
    int expressionDepth = 0;
    std::uintptr_t stackBase = 0;
    int anonymousStructCount = 0;
};

} // namespace

//==============================================================================
// GlslParser::parse()
//==============================================================================

ResultValue<TranslationUnit> GlslParser::parse (const String& source)
{
    try
    {
        std::string src = source.toStdString();
        Lexer lexer (src);
        Parser parser (lexer);
        return makeResultValueOk (parser.parseTranslationUnit());
    }
    catch (const std::exception& e)
    {
        return makeResultValueFail (String (e.what()));
    }
}

} // namespace wgsl
} // namespace yup
