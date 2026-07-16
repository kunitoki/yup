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
    switch (tt)
    {
        case TokenType::identifier:
            return "identifier";
        case TokenType::intConst:
            return "intConst";
        case TokenType::uintConst:
            return "uintConst";
        case TokenType::floatConst:
            return "floatConst";
        case TokenType::boolConst:
            return "boolConst";
        case TokenType::semicolon:
            return ";";
        case TokenType::comma:
            return ",";
        case TokenType::lParen:
            return "(";
        case TokenType::rParen:
            return ")";
        case TokenType::lBrace:
            return "{";
        case TokenType::rBrace:
            return "}";
        case TokenType::lBracket:
            return "[";
        case TokenType::rBracket:
            return "]";
        case TokenType::dot:
            return ".";
        case TokenType::question:
            return "?";
        case TokenType::colon:
            return ":";
        case TokenType::hash:
            return "#";
        case TokenType::plus:
            return "+";
        case TokenType::minus:
            return "-";
        case TokenType::star:
            return "*";
        case TokenType::slash:
            return "/";
        case TokenType::percent:
            return "%";
        case TokenType::lt:
            return "<";
        case TokenType::gt:
            return ">";
        case TokenType::le:
            return "<=";
        case TokenType::ge:
            return ">=";
        case TokenType::eq:
            return "==";
        case TokenType::ne:
            return "!=";
        case TokenType::amp:
            return "&";
        case TokenType::caret:
            return "^";
        case TokenType::pipe:
            return "|";
        case TokenType::land:
            return "&&";
        case TokenType::lor:
            return "||";
        case TokenType::lnot:
            return "!";
        case TokenType::bnot:
            return "~";
        case TokenType::lshift:
            return "<<";
        case TokenType::rshift:
            return ">>";
        case TokenType::inc:
            return "++";
        case TokenType::dec:
            return "--";
        case TokenType::assign:
            return "=";
        case TokenType::addAssign:
            return "+=";
        case TokenType::subAssign:
            return "-=";
        case TokenType::mulAssign:
            return "*=";
        case TokenType::divAssign:
            return "/=";
        case TokenType::modAssign:
            return "%=";
        case TokenType::lshiftAssign:
            return "<<=";
        case TokenType::rshiftAssign:
            return ">>=";
        case TokenType::andAssign:
            return "&=";
        case TokenType::xorAssign:
            return "^=";
        case TokenType::orAssign:
            return "|=";
        case TokenType::endOfFile:
            return "EOF";
    }
    return "?";
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
    int intValue = 0;
    unsigned int uintValue = 0;
    double floatValue = 0.0;
    bool boolValue = false;
};

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
    }

    Token next()
    {
        skipWhitespaceAndComments();

        currentLoc = loc;

        if (pos >= src.size())
            return makeToken (TokenType::endOfFile);

        char c = src[pos];

        // Identifier or keyword
        if (isAlpha (c) || c == '_')
            return lexIdentifier();

        // Numbers
        if (isDigit (c))
            return lexNumber();

        // Punctuation and operators
        switch (c)
        {
            case ';':
                advanceChar();
                return makeToken (TokenType::semicolon);
            case ',':
                advanceChar();
                return makeToken (TokenType::comma);
            case '(':
                advanceChar();
                return makeToken (TokenType::lParen);
            case ')':
                advanceChar();
                return makeToken (TokenType::rParen);
            case '{':
                advanceChar();
                return makeToken (TokenType::lBrace);
            case '}':
                advanceChar();
                return makeToken (TokenType::rBrace);
            case '[':
                advanceChar();
                return makeToken (TokenType::lBracket);
            case ']':
                advanceChar();
                return makeToken (TokenType::rBracket);
            case '.':
                advanceChar();
                return makeToken (TokenType::dot);
            case '?':
                advanceChar();
                return makeToken (TokenType::question);
            case ':':
                advanceChar();
                return makeToken (TokenType::colon);
            case '#':
                advanceChar();
                skipLineComment();
                return next();
            case '~':
                advanceChar();
                return makeToken (TokenType::bnot);

            case '+':
                advanceChar();
                if (matchCh ('+'))
                    return makeToken (TokenType::inc);
                if (matchCh ('='))
                    return makeToken (TokenType::addAssign);
                return makeToken (TokenType::plus);

            case '-':
                advanceChar();
                if (matchCh ('-'))
                    return makeToken (TokenType::dec);
                if (matchCh ('='))
                    return makeToken (TokenType::subAssign);
                return makeToken (TokenType::minus);

            case '*':
                advanceChar();
                if (matchCh ('='))
                    return makeToken (TokenType::mulAssign);
                return makeToken (TokenType::star);

            case '/':
                advanceChar();
                if (matchCh ('/'))
                {
                    skipLineComment();
                    return next();
                }
                if (matchCh ('*'))
                {
                    skipBlockComment();
                    return next();
                }
                if (matchCh ('='))
                    return makeToken (TokenType::divAssign);
                return makeToken (TokenType::slash);

            case '%':
                advanceChar();
                if (matchCh ('='))
                    return makeToken (TokenType::modAssign);
                return makeToken (TokenType::percent);

            case '<':
                advanceChar();
                if (matchCh ('<'))
                {
                    if (matchCh ('='))
                        return makeToken (TokenType::lshiftAssign);
                    return makeToken (TokenType::lshift);
                }
                if (matchCh ('='))
                    return makeToken (TokenType::le);
                return makeToken (TokenType::lt);

            case '>':
                advanceChar();
                if (matchCh ('>'))
                {
                    if (matchCh ('='))
                        return makeToken (TokenType::rshiftAssign);
                    return makeToken (TokenType::rshift);
                }
                if (matchCh ('='))
                    return makeToken (TokenType::ge);
                return makeToken (TokenType::gt);

            case '=':
                advanceChar();
                if (matchCh ('='))
                    return makeToken (TokenType::eq);
                return makeToken (TokenType::assign);

            case '!':
                advanceChar();
                if (matchCh ('='))
                    return makeToken (TokenType::ne);
                return makeToken (TokenType::lnot);

            case '&':
                advanceChar();
                if (matchCh ('&'))
                    return makeToken (TokenType::land);
                if (matchCh ('='))
                    return makeToken (TokenType::andAssign);
                return makeToken (TokenType::amp);

            case '|':
                advanceChar();
                if (matchCh ('|'))
                    return makeToken (TokenType::lor);
                if (matchCh ('='))
                    return makeToken (TokenType::orAssign);
                return makeToken (TokenType::pipe);

            case '^':
                advanceChar();
                if (matchCh ('='))
                    return makeToken (TokenType::xorAssign);
                return makeToken (TokenType::caret);

            default:
            {
                String err = String::formatted ("%d:%d: Unexpected character '%c' (0x%02x)",
                                                loc.line,
                                                loc.column,
                                                c,
                                                static_cast<unsigned> (c));
                throw std::runtime_error (err.toStdString());
            }
        }
    }

    const Token& peek()
    {
        if (! lookahead.has_value())
            lookahead = next();
        return *lookahead;
    }

    Token advance()
    {
        if (lookahead.has_value())
        {
            Token t = std::move (*lookahead);
            lookahead.reset();
            return t;
        }
        return next();
    }

private:
    void skipWhitespaceAndComments()
    {
        while (pos < src.size())
        {
            char c = src[pos];
            if (c == ' ' || c == '\t' || c == '\r')
            {
                ++pos;
                ++loc.column;
            }
            else if (c == '\n')
            {
                ++pos;
                ++loc.line;
                loc.column = 1;
            }
            else if (c == '/' && pos + 1 < src.size())
            {
                if (src[pos + 1] == '/')
                {
                    pos += 2;
                    loc.column += 2;
                    skipLineComment();
                }
                else if (src[pos + 1] == '*')
                {
                    pos += 2;
                    loc.column += 2;
                    skipBlockComment();
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }

    void skipLineComment()
    {
        while (pos < src.size() && src[pos] != '\n')
        {
            ++pos;
            ++loc.column;
        }
    }

    void skipBlockComment()
    {
        while (pos < src.size())
        {
            if (src[pos] == '*' && pos + 1 < src.size() && src[pos + 1] == '/')
            {
                pos += 2;
                loc.column += 2;
                return;
            }

            if (src[pos] == '\n')
            {
                ++loc.line;
                loc.column = 1;
            }
            else
            {
                ++loc.column;
            }
            ++pos;
        }
    }

    bool matchCh (char c)
    {
        if (pos < src.size() && src[pos] == c)
        {
            ++pos;
            ++loc.column;
            return true;
        }
        return false;
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

    static bool isOctalDigit (char c)
    {
        return c >= '0' && c <= '7';
    }

    Token lexIdentifier()
    {
        size_t start = pos;
        while (pos < src.size() && (isAlpha (src[pos]) || isDigit (src[pos]) || src[pos] == '_'))
        {
            ++pos;
            ++loc.column;
        }

        std::string text = src.substr (start, pos - start);
        Token tok = makeToken (TokenType::identifier);
        tok.text = text;

        // Check for keywords
        if (text == "true")
        {
            tok.type = TokenType::boolConst;
            tok.boolValue = true;
        }
        else if (text == "false")
        {
            tok.type = TokenType::boolConst;
            tok.boolValue = false;
        }

        return tok;
    }

    Token lexNumber()
    {
        size_t start = pos;

        // Check for hex or octal prefix
        if (src[pos] == '0' && pos + 1 < src.size())
        {
            char next = src[pos + 1];
            if (next == 'x' || next == 'X')
            {
                pos += 2;
                loc.column += 2;
                return lexHexNumber();
            }
            // Numbers starting with 0 are decimal unless followed by octal digits only,
            // but in GLSL, 0-prefixed integers without x/X are interpreted as decimal
        }

        // Decimal integer or float
        return lexDecimalNumber();
    }

    Token lexHexNumber()
    {
        size_t start = pos;

        while (pos < src.size() && isHexDigit (src[pos]))
        {
            ++pos;
            ++loc.column;
        }

        std::string digits = src.substr (start, pos - start);

        bool isUnsigned = false;
        if (pos < src.size() && (src[pos] == 'u' || src[pos] == 'U'))
        {
            isUnsigned = true;
            ++pos;
            ++loc.column;
        }

        Token tok = makeToken (isUnsigned ? TokenType::uintConst : TokenType::intConst);
        tok.text = src.substr (start - 2, pos - (start - 2));

        unsigned long long val = std::stoull (digits, nullptr, 16);

        if (isUnsigned)
            tok.uintValue = static_cast<unsigned int> (val);
        else
            tok.intValue = static_cast<int> (val);

        return tok;
    }

    Token lexDecimalNumber()
    {
        size_t begin = pos;
        bool isFloat = false;

        // Integer part
        while (pos < src.size() && isDigit (src[pos]))
        {
            ++pos;
            ++loc.column;
        }

        // Fractional part
        if (pos < src.size() && src[pos] == '.')
        {
            // Check it's not a swizzle (e.g., .xyz) or method call (.length())
            if (pos + 1 < src.size() && isDigit (src[pos + 1]))
            {
                isFloat = true;
                ++pos;
                ++loc.column;
                while (pos < src.size() && isDigit (src[pos]))
                {
                    ++pos;
                    ++loc.column;
                }
            }
            else if (pos + 1 < src.size() && src[pos + 1] == '.')
            {
                // Float literal ending with ".." - malformed, treat as integer + dot
                // but this shouldn't happen in valid GLSL
            }
        }

        // Exponent
        if (pos < src.size() && (src[pos] == 'e' || src[pos] == 'E'))
        {
            isFloat = true;
            ++pos;
            ++loc.column;
            if (pos < src.size() && (src[pos] == '+' || src[pos] == '-'))
            {
                ++pos;
                ++loc.column;
            }
            while (pos < src.size() && isDigit (src[pos]))
            {
                ++pos;
                ++loc.column;
            }
        }

        // Suffix
        bool isUnsigned = false;
        if (pos < src.size())
        {
            char sfx = src[pos];
            if (sfx == 'f' || sfx == 'F')
            {
                isFloat = true;
                ++pos;
                ++loc.column;
            }
            else if (sfx == 'l' || sfx == 'L')
            {
                ++pos;
                ++loc.column;
                if (pos < src.size() && (src[pos] == 'f' || src[pos] == 'F'))
                {
                    isFloat = true;
                    ++pos;
                    ++loc.column;
                }
            }
            else if (sfx == 'u' || sfx == 'U')
            {
                isUnsigned = true;
                ++pos;
                ++loc.column;
            }
        }

        std::string text = src.substr (begin, pos - begin);

        Token tok;
        tok.loc = currentLoc;
        tok.text = text;

        if (isFloat)
        {
            tok.type = TokenType::floatConst;
            tok.floatValue = std::stod (text);
        }
        else if (isUnsigned)
        {
            tok.type = TokenType::uintConst;
            tok.uintValue = static_cast<unsigned int> (std::stoull (text));
        }
        else
        {
            tok.type = TokenType::intConst;
            tok.intValue = static_cast<int> (std::stoll (text));
        }

        return tok;
    }

    Token makeToken (TokenType tt)
    {
        Token tok;
        tok.type = tt;
        tok.loc = currentLoc;
        return tok;
    }

    const std::string& src;
    size_t pos = 0;
    SourceLocation loc;
    SourceLocation currentLoc;
    std::optional<Token> lookahead;
};

//==============================================================================
// Keyword check helpers (against raw token text)
//==============================================================================

static bool isKeyword (const Token& tok, const char* kw)
{
    return tok.type == TokenType::identifier && tok.text == kw;
}

//==============================================================================
// GLSL 4.50 Recursive Descent Parser
//==============================================================================

class Parser
{
public:
    explicit Parser (Lexer& lex)
        : lexer (lex)
    {
    }

    ResultValue<TranslationUnit> parseTranslationUnit()
    {
        TranslationUnit unit;
        unit.loc = loc();

        while (lexer.peek().type != TokenType::endOfFile)
        {
            // Skip stray semicolons at top level
            if (lexer.peek().type == TokenType::semicolon)
            {
                lexer.advance();
                continue;
            }

            auto decl = parseExternalDeclaration();

            if (! decl)
                return makeResultValueFail (decl.getErrorMessage());

            unit.declarations.push_back (std::move (decl).getValue());
        }

        return makeResultValueOk (std::move (unit));
    }

private:
    //==========================================================================
    SourceLocation loc() const
    {
        return lexer.peek().loc;
    }

    //==========================================================================
    bool match (TokenType tt)
    {
        if (lexer.peek().type == tt)
        {
            lexer.advance();
            return true;
        }
        return false;
    }

    bool matchKeyword (const char* kw)
    {
        if (isKeyword (lexer.peek(), kw))
        {
            lexer.advance();
            return true;
        }
        return false;
    }

    Token expect (TokenType tt)
    {
        if (lexer.peek().type == tt)
            return lexer.advance();

        String msg = String::formatted ("%d:%d: Expected '%s', got '%s'",
                                        lexer.peek().loc.line,
                                        lexer.peek().loc.column,
                                        toString (tt),
                                        lexer.peek().text.c_str());
        throw std::runtime_error (msg.toStdString());
    }

    Token expectKeyword (const char* kw)
    {
        if (isKeyword (lexer.peek(), kw))
            return lexer.advance();

        String msg = String::formatted ("%d:%d: Expected keyword '%s', got '%s'",
                                        lexer.peek().loc.line,
                                        lexer.peek().loc.column,
                                        kw,
                                        lexer.peek().text.c_str());
        throw std::runtime_error (msg.toStdString());
    }

    //==========================================================================
    // External declarations (top-level)
    //==========================================================================

    ResultValue<ExternalDeclaration> parseExternalDeclaration()
    {
        SourceLocation l = loc();

        // Skip stray semicolons at top level
        while (lexer.peek().type == TokenType::semicolon)
        {
            lexer.advance();
            l = loc();
        }

        if (lexer.peek().type == TokenType::endOfFile)
            return makeResultValueFail ("Unexpected end of file");

        // Struct definition: struct Name { ... } ;
        if (isKeyword (lexer.peek(), "struct"))
        {
            lexer.advance();

            StructSpecifier ss;
            ss.loc = l;

            if (lexer.peek().type == TokenType::identifier)
                ss.name = lexer.advance().text;

            expect (TokenType::lBrace);
            while (lexer.peek().type != TokenType::rBrace && lexer.peek().type != TokenType::endOfFile)
            {
                auto field = parseStructFieldSpecifier();
                if (! field)
                    return makeResultValueFail (field.getErrorMessage());
                ss.fields.push_back (std::move (field).getValue());
            }
            expect (TokenType::rBrace);
            expect (TokenType::semicolon);

            Declaration decl;
            decl.loc = l;
            decl.structSpecifier = std::make_unique<StructSpecifier> (std::move (ss));
            return makeResultValueOk (ExternalDeclaration { std::move (decl) });
        }

        // Parse qualifiers, type, and identifier
        auto qualifier = parseTypeQualifier();
        std::optional<ResultValue<TypeSpecifier>> type;

        // Standalone qualifier declaration: layout(local_size_x = 8) in;
        if (qualifier && lexer.peek().type == TokenType::semicolon)
        {
            lexer.advance();
            Declaration decl;
            decl.loc = l;
            decl.qualifier = std::make_unique<TypeQualifier> (std::move (qualifier).value());
            return makeResultValueOk (ExternalDeclaration { std::move (decl) });
        }

        // Check for interface block: uniform/buffer followed by { or IDENT {
        std::unique_ptr<StructSpecifier> blockStruct;
        if (qualifier
            && (qualifier->hasStorage (StorageQualifier::uniform) || qualifier->hasStorage (StorageQualifier::buffer)))
        {
            // Anonymous block: layout(...) uniform { fields } instanceName;
            if (lexer.peek().type == TokenType::lBrace)
            {
                blockStruct = std::make_unique<StructSpecifier> (std::move (parseStructSpecifierDeclaration (l)).getValue());
                type = makeResultValueOk (TypeSpecifier::make (l, TypeKind::voidType));
            }
            // Named block: layout(...) uniform BlockName { fields } instanceName;
            else if (lexer.peek().type == TokenType::identifier)
            {
                std::string savedName = lexer.peek().text;
                lexer.advance();

                if (lexer.peek().type == TokenType::lBrace)
                {
                    auto ss = parseStructSpecifierDeclaration (l);
                    if (! ss)
                        return makeResultValueFail (ss.getErrorMessage());
                    blockStruct = std::make_unique<StructSpecifier> (std::move (ss).getValue());
                    blockStruct->name = savedName;
                    type = makeResultValueOk (TypeSpecifier::makeNamed (l, savedName));
                }
                else
                {
                    // Not a block — regular uniform variable. savedName is the type keyword.
                    // We already consumed it, so synthesize the TypeSpecifier from the name
                    // and continue to parse the variable name.
                    type = makeResultValueOk (TypeSpecifier::make (l, typeNameToKind (savedName)));
                    // Fall through to regular declaration parsing below
                }
            }
        }

        // Parse the type specifier (if not already set by block logic above)
        if (! type.has_value())
            type = parseTypeSpecifier();

        if (! type)
            return makeResultValueFail (type.value().getErrorMessage());

        // Must have identifier
        if (lexer.peek().type != TokenType::identifier)
        {
            auto msg = String::formatted ("%d:%d: Expected identifier after type specifier",
                                          lexer.peek().loc.line,
                                          lexer.peek().loc.column);
            return makeResultValueFail (msg);
        }

        Token nameTok = lexer.advance();
        std::string name = nameTok.text;

        // Branch: function definition/prototype vs declaration
        if (! blockStruct && lexer.peek().type == TokenType::lParen)
        {
            return parseFunctionDefinitionOrPrototype (std::move (qualifier), std::move (type.value()).getValue(), std::move (name), l);
        }

        auto singleDecl = parseSingleDeclarationRest (type.value().getValue(), std::move (name));
        if (! singleDecl)
            return makeResultValueFail (singleDecl.getErrorMessage());

        InitDeclaratorList initList;
        initList.loc = l;
        if (qualifier)
            initList.qualifier = std::make_unique<TypeQualifier> (std::move (*qualifier));
        initList.type = std::move (type.value()).getValue();
        initList.declarations.push_back (std::move (singleDecl).getValue());

        while (match (TokenType::comma))
        {
            Token nextName = expect (TokenType::identifier);
            auto nextDecl = parseSingleDeclarationRest (initList.type, std::string (nextName.text));
            if (! nextDecl)
                return makeResultValueFail (nextDecl.getErrorMessage());
            initList.declarations.push_back (std::move (nextDecl).getValue());
        }

        expect (TokenType::semicolon);

        Declaration decl;
        decl.loc = l;
        decl.initDeclaratorList = std::make_unique<InitDeclaratorList> (std::move (initList));
        decl.structSpecifier = std::move (blockStruct);
        return makeResultValueOk (ExternalDeclaration { std::move (decl) });
    }

    //==========================================================================
    // Function definition parsing
    //==========================================================================

    ResultValue<ExternalDeclaration> parseFunctionDefinitionOrPrototype (
        std::optional<TypeQualifier> qualifier,
        TypeSpecifier returnType,
        std::string name,
        SourceLocation l)
    {
        FunctionPrototype proto;
        proto.loc = l;
        proto.returnType = std::move (returnType);
        proto.name = std::move (name);

        expect (TokenType::lParen);

        // Parse parameters
        if (lexer.peek().type != TokenType::rParen)
        {
            auto params = parseFunctionParameters();
            if (! params)
                return makeResultValueFail (params.getErrorMessage());
            proto.parameters = std::move (params).getValue();
        }

        expect (TokenType::rParen);

        // Check if this is a function definition (has body) or prototype (ends with ';')
        if (match (TokenType::semicolon))
        {
            // Function prototype/declaration only - wrap as a declaration
            // For now, we treat prototypes as declarations
            Declaration decl;
            decl.loc = l;
            return makeResultValueOk (ExternalDeclaration { std::move (decl) });
        }

        // Parse body
        auto body = parseStatement();
        if (! body)
            return makeResultValueFail (body.getErrorMessage());

        FunctionDefinition funcDef;
        funcDef.loc = l;
        funcDef.prototype = std::move (proto);
        funcDef.body = std::make_unique<Statement> (std::move (body).getValue());

        return makeResultValueOk (ExternalDeclaration { std::move (funcDef) });
    }

    ResultValue<std::vector<FunctionParameterDeclaration>> parseFunctionParameters()
    {
        std::vector<FunctionParameterDeclaration> params;

        auto param = parseFunctionParameter();
        if (! param)
            return makeResultValueFail (param.getErrorMessage());
        params.push_back (std::move (param).getValue());

        while (match (TokenType::comma))
        {
            auto nextParam = parseFunctionParameter();
            if (! nextParam)
                return makeResultValueFail (nextParam.getErrorMessage());
            params.push_back (std::move (nextParam).getValue());
        }

        return makeResultValueOk (std::move (params));
    }

    ResultValue<FunctionParameterDeclaration> parseFunctionParameter()
    {
        SourceLocation l = loc();
        FunctionParameterDeclaration param;
        param.loc = l;

        auto qualifier = parseParameterQualifier();
        if (qualifier)
            param.qualifier = std::make_unique<TypeQualifier> (std::move (*qualifier));

        auto type = parseTypeSpecifier();
        if (! type)
            return makeResultValueFail (type.getErrorMessage());
        param.type = std::move (type).getValue();

        // Optional name
        if (lexer.peek().type == TokenType::identifier)
        {
            param.name = lexer.advance().text;
        }

        // Array specifiers (e.g., float arr[4])
        while (lexer.peek().type == TokenType::lBracket)
            param.arraySpecifiers.push_back (parseArraySpecifier());

        return makeResultValueOk (std::move (param));
    }

    std::optional<TypeQualifier> parseParameterQualifier()
    {
        TypeQualifier q;
        q.loc = loc();
        bool hasQual = false;

        if (matchKeyword ("in"))
        {
            q.storage.push_back (StorageQualifier::in);
            hasQual = true;
        }
        else if (matchKeyword ("out"))
        {
            q.storage.push_back (StorageQualifier::out);
            hasQual = true;
        }
        else if (matchKeyword ("inout"))
        {
            q.storage.push_back (StorageQualifier::inout);
            hasQual = true;
        }

        return hasQual ? std::optional<TypeQualifier> (std::move (q)) : std::nullopt;
    }

    //==========================================================================
    //==========================================================================
    // Statements
    //==========================================================================

    ResultValue<Statement> parseStatement()
    {
        SourceLocation l = loc();

        // Compound statement
        if (lexer.peek().type == TokenType::lBrace)
        {
            Statement stmt = parseCompoundStatement();
            return makeResultValueOk (std::move (stmt));
        }

        // Selection (if/else)
        if (matchKeyword ("if"))
            return parseIfStatement (l);

        // Switch
        if (matchKeyword ("switch"))
            return parseSwitchStatement (l);

        // Iteration
        if (matchKeyword ("for"))
            return parseForStatement (l);
        if (matchKeyword ("while"))
            return parseWhileStatement (l);
        if (matchKeyword ("do"))
            return parseDoWhileStatement (l);

        // Jump
        if (matchKeyword ("return"))
            return parseReturnStatement (l);
        if (matchKeyword ("break"))
        {
            expect (TokenType::semicolon);
            StmtJump jump;
            jump.loc = l;
            jump.kind = JumpKind::breakJump;
            Statement s;
            s.loc = l;
            s.value = std::move (jump);
            return makeResultValueOk (std::move (s));
        }
        if (matchKeyword ("continue"))
        {
            expect (TokenType::semicolon);
            StmtJump jump;
            jump.loc = l;
            jump.kind = JumpKind::continueJump;
            Statement s;
            s.loc = l;
            s.value = std::move (jump);
            return makeResultValueOk (std::move (s));
        }
        if (matchKeyword ("discard"))
        {
            expect (TokenType::semicolon);
            StmtJump jump;
            jump.loc = l;
            jump.kind = JumpKind::discardJump;
            Statement s;
            s.loc = l;
            s.value = std::move (jump);
            return makeResultValueOk (std::move (s));
        }

        // Case label
        if (matchKeyword ("case") || matchKeyword ("default"))
            return parseCaseLabel (l); // Note: keyword already consumed above, need to handle differently

        // Declaration or expression statement
        // Try to parse as declaration first (starts with type keyword or qualifier)
        auto isDeclarationStart = [&]()
        {
            if (isTypeStart (lexer.peek()))
                return true;
            // Precision/interpolation/storage qualifiers can precede type in declarations
            return isKeyword (lexer.peek(), "highp") || isKeyword (lexer.peek(), "mediump") || isKeyword (lexer.peek(), "lowp")
                || isKeyword (lexer.peek(), "flat") || isKeyword (lexer.peek(), "smooth") || isKeyword (lexer.peek(), "noperspective")
                || isKeyword (lexer.peek(), "const");
        };

        if (isDeclarationStart())
        {
            auto decl = parseDeclaration (false);
            if (! decl)
                return makeResultValueFail (decl.getErrorMessage());

            Statement s;
            s.loc = l;
            s.value = StmtDeclaration { l, std::move (decl).getValue() };

            expect (TokenType::semicolon);
            return makeResultValueOk (std::move (s));
        }

        // Semicolon-only statement
        if (match (TokenType::semicolon))
        {
            return makeResultValueOk (Statement::makeEmpty (l));
        }

        // Expression statement
        auto expr = parseExpression();
        if (! expr)
            return makeResultValueFail (expr.getErrorMessage());

        expect (TokenType::semicolon);

        Statement s;
        s.loc = l;
        s.value = StmtExpr { l, std::make_unique<Expr> (std::move (expr).getValue()) };
        return makeResultValueOk (std::move (s));
    }

    Statement parseCompoundStatement()
    {
        SourceLocation l = loc();
        expect (TokenType::lBrace);

        std::vector<Statement> stmts;

        while (lexer.peek().type != TokenType::rBrace && lexer.peek().type != TokenType::endOfFile)
        {
            auto stmt = parseStatement();
            if (stmt)
                stmts.push_back (std::move (stmt).getValue());
            else
                break; // Error already thrown
        }

        expect (TokenType::rBrace);

        StmtCompound comp;
        comp.loc = l;
        comp.statements = std::move (stmts);

        Statement s;
        s.loc = l;
        s.value = std::move (comp);
        return s;
    }

    ResultValue<Statement> parseIfStatement (SourceLocation l)
    {
        StmtSelection sel;
        sel.loc = l;

        expect (TokenType::lParen);
        auto cond = parseExpression();
        if (! cond)
            return makeResultValueFail (cond.getErrorMessage());
        sel.condition = std::make_unique<Expr> (std::move (cond).getValue());
        expect (TokenType::rParen);

        auto thenBody = parseStatement();
        if (! thenBody)
            return makeResultValueFail (thenBody.getErrorMessage());
        sel.thenBranch = std::make_unique<Statement> (std::move (thenBody).getValue());

        if (matchKeyword ("else"))
        {
            auto elseBody = parseStatement();
            if (! elseBody)
                return makeResultValueFail (elseBody.getErrorMessage());
            sel.elseBranch = std::make_unique<Statement> (std::move (elseBody).getValue());
        }

        Statement s;
        s.loc = l;
        s.value = std::move (sel);
        return makeResultValueOk (std::move (s));
    }

    ResultValue<Statement> parseSwitchStatement (SourceLocation l)
    {
        StmtSwitch sw;
        sw.loc = l;

        expect (TokenType::lParen);
        auto sel = parseExpression();
        if (! sel)
            return makeResultValueFail (sel.getErrorMessage());
        sw.selector = std::make_unique<Expr> (std::move (sel).getValue());
        expect (TokenType::rParen);

        expect (TokenType::lBrace);

        while (lexer.peek().type != TokenType::rBrace && lexer.peek().type != TokenType::endOfFile)
        {
            if (isKeyword (lexer.peek(), "case") || isKeyword (lexer.peek(), "default"))
            {
                SourceLocation caseLoc = loc();
                bool isDefault = false;

                if (matchKeyword ("default"))
                    isDefault = true;
                else
                    matchKeyword ("case");

                StmtCaseLabel caseLabel;
                caseLabel.loc = caseLoc;

                if (! isDefault)
                {
                    auto labelExpr = parseExpression();
                    if (! labelExpr)
                        return makeResultValueFail (labelExpr.getErrorMessage());
                    caseLabel.label = std::make_unique<Expr> (std::move (labelExpr).getValue());
                }

                expect (TokenType::colon);

                Statement caseStmt;
                caseStmt.loc = caseLoc;
                caseStmt.value = std::move (caseLabel);
                sw.body.push_back (std::move (caseStmt));
            }
            else
            {
                auto stmt = parseStatement();
                if (stmt)
                    sw.body.push_back (std::move (stmt).getValue());
                else
                    break;
            }
        }

        expect (TokenType::rBrace);

        Statement s;
        s.loc = l;
        s.value = std::move (sw);
        return makeResultValueOk (std::move (s));
    }

    ResultValue<Statement> parseCaseLabel (SourceLocation l)
    {
        StmtCaseLabel caseLabel;
        caseLabel.loc = l;
        // keyword already consumed by caller

        Statement s;
        s.loc = l;
        s.value = std::move (caseLabel);
        return makeResultValueOk (std::move (s));
    }

    ResultValue<Statement> parseForStatement (SourceLocation l)
    {
        StmtFor forStmt;
        forStmt.loc = l;

        expect (TokenType::lParen);

        // Init statement (declaration, expression, or empty)
        if (! match (TokenType::semicolon))
        {
            auto init = parseStatement();
            if (init)
                forStmt.init = std::make_unique<Statement> (std::move (init).getValue());
        }

        // Condition
        if (! match (TokenType::semicolon))
        {
            auto cond = parseExpression();
            if (cond)
                forStmt.condition = std::make_unique<Expr> (std::move (cond).getValue());
            expect (TokenType::semicolon);
        }

        // Update
        if (! match (TokenType::rParen))
        {
            auto update = parseExpression();
            if (update)
                forStmt.update = std::make_unique<Expr> (std::move (update).getValue());
            expect (TokenType::rParen);
        }

        auto body = parseStatement();
        if (! body)
            return makeResultValueFail (body.getErrorMessage());
        forStmt.body = std::make_unique<Statement> (std::move (body).getValue());

        Statement s;
        s.loc = l;
        s.value = std::move (forStmt);
        return makeResultValueOk (std::move (s));
    }

    ResultValue<Statement> parseWhileStatement (SourceLocation l)
    {
        StmtWhile whileStmt;
        whileStmt.loc = l;

        expect (TokenType::lParen);
        auto cond = parseExpression();
        if (! cond)
            return makeResultValueFail (cond.getErrorMessage());
        whileStmt.condition = std::make_unique<Expr> (std::move (cond).getValue());
        expect (TokenType::rParen);

        auto body = parseStatement();
        if (! body)
            return makeResultValueFail (body.getErrorMessage());
        whileStmt.body = std::make_unique<Statement> (std::move (body).getValue());

        Statement s;
        s.loc = l;
        s.value = std::move (whileStmt);
        return makeResultValueOk (std::move (s));
    }

    ResultValue<Statement> parseDoWhileStatement (SourceLocation l)
    {
        StmtDoWhile doStmt;
        doStmt.loc = l;

        auto body = parseStatement();
        if (! body)
            return makeResultValueFail (body.getErrorMessage());
        doStmt.body = std::make_unique<Statement> (std::move (body).getValue());

        expectKeyword ("while");
        expect (TokenType::lParen);
        auto cond = parseExpression();
        if (! cond)
            return makeResultValueFail (cond.getErrorMessage());
        doStmt.condition = std::make_unique<Expr> (std::move (cond).getValue());
        expect (TokenType::rParen);
        expect (TokenType::semicolon);

        Statement s;
        s.loc = l;
        s.value = std::move (doStmt);
        return makeResultValueOk (std::move (s));
    }

    ResultValue<Statement> parseReturnStatement (SourceLocation l)
    {
        StmtJump jump;
        jump.loc = l;
        jump.kind = JumpKind::returnJump;

        if (lexer.peek().type != TokenType::semicolon)
        {
            auto expr = parseExpression();
            if (! expr)
                return makeResultValueFail (expr.getErrorMessage());
            jump.returnValue = std::make_unique<Expr> (std::move (expr).getValue());
        }

        expect (TokenType::semicolon);

        Statement s;
        s.loc = l;
        s.value = std::move (jump);
        return makeResultValueOk (std::move (s));
    }

    //==========================================================================
    // Expressions - full GLSL precedence ladder
    //==========================================================================

    // Comma → Assignment (',' Assignment)*
    ResultValue<Expr> parseComma()
    {
        SourceLocation l = loc();
        auto left = parseAssignment();
        if (! left)
            return left;

        while (match (TokenType::comma))
        {
            auto right = parseAssignment();
            if (! right)
                return makeResultValueFail (right.getErrorMessage());

            ExprComma comma;
            comma.loc = l;
            comma.left = std::make_unique<Expr> (std::move (left).getValue());
            comma.right = std::make_unique<Expr> (std::move (right).getValue());

            Expr e;
            e.loc = l;
            e.value = std::move (comma);

            left = makeResultValueOk (std::move (e));
        }

        return left;
    }

    ResultValue<Expr> parseExpression()
    {
        return parseComma();
    }

    // Assignment → unary (AssignmentOp unary)?
    ResultValue<Expr> parseAssignment()
    {
        SourceLocation l = loc();
        auto left = parseConditional();
        if (! left)
            return left;

        // Check for assignment operator
        if (isAssignmentOp (lexer.peek().type))
        {
            AssignmentOp op = tokenToAssignmentOp (lexer.peek().type);
            lexer.advance();

            auto right = parseAssignment();
            if (! right)
                return makeResultValueFail (right.getErrorMessage());

            ExprAssignment assign;
            assign.loc = l;
            assign.op = op;
            assign.lhs = std::make_unique<Expr> (std::move (left).getValue());
            assign.rhs = std::make_unique<Expr> (std::move (right).getValue());

            Expr e;
            e.loc = l;
            e.value = std::move (assign);
            return makeResultValueOk (std::move (e));
        }

        return left;
    }

    // Conditional → logicalOr ('?' expr ':' conditional)?
    ResultValue<Expr> parseConditional()
    {
        SourceLocation l = loc();
        auto cond = parseLogicalOr();
        if (! cond)
            return cond;

        if (match (TokenType::question))
        {
            auto trueBranch = parseExpression();
            if (! trueBranch)
                return makeResultValueFail (trueBranch.getErrorMessage());

            expect (TokenType::colon);

            auto falseBranch = parseConditional();
            if (! falseBranch)
                return makeResultValueFail (falseBranch.getErrorMessage());

            ExprTernary ternary;
            ternary.loc = l;
            ternary.condition = std::make_unique<Expr> (std::move (cond).getValue());
            ternary.trueBranch = std::make_unique<Expr> (std::move (trueBranch).getValue());
            ternary.falseBranch = std::make_unique<Expr> (std::move (falseBranch).getValue());

            Expr e;
            e.loc = l;
            e.value = std::move (ternary);
            return makeResultValueOk (std::move (e));
        }

        return cond;
    }

    // logicalOr → logicalAnd ('||' logicalAnd)*
    ResultValue<Expr> parseLogicalOr()
    {
        return parseBinaryLeft (TokenType::lor, [this]()
        {
            return parseLogicalAnd();
        },
                                BinaryOp::logicalOr);
    }

    // logicalAnd → bitwiseOr ('&&' bitwiseOr)*
    ResultValue<Expr> parseLogicalAnd()
    {
        return parseBinaryLeft (TokenType::land, [this]()
        {
            return parseBitwiseOr();
        },
                                BinaryOp::logicalAnd);
    }

    // bitwiseOr → bitwiseXor ('|' bitwiseXor)*
    ResultValue<Expr> parseBitwiseOr()
    {
        return parseBinaryLeft (TokenType::pipe, [this]()
        {
            return parseBitwiseXor();
        },
                                BinaryOp::bitwiseOr);
    }

    // bitwiseXor → bitwiseAnd ('^' bitwiseAnd)*
    ResultValue<Expr> parseBitwiseXor()
    {
        return parseBinaryLeft (TokenType::caret, [this]()
        {
            return parseBitwiseAnd();
        },
                                BinaryOp::bitwiseXor);
    }

    // bitwiseAnd → equality ('&' equality)*
    ResultValue<Expr> parseBitwiseAnd()
    {
        return parseBinaryLeft (TokenType::amp, [this]()
        {
            return parseEquality();
        },
                                BinaryOp::bitwiseAnd);
    }

    // equality → relational (('=='|'!=') relational)*
    ResultValue<Expr> parseEquality()
    {
        auto left = parseRelational();
        if (! left)
            return left;

        while (lexer.peek().type == TokenType::eq || lexer.peek().type == TokenType::ne)
        {
            BinaryOp op = (lexer.peek().type == TokenType::eq) ? BinaryOp::equal : BinaryOp::notEqual;
            SourceLocation l = loc();
            lexer.advance();

            auto right = parseRelational();
            if (! right)
                return right;

            left = makeBinaryExpr (l, std::move (left).getValue(), op, std::move (right).getValue());
        }

        return left;
    }

    // relational → shift (('<'|'>'|'<='|'>=') shift)*
    ResultValue<Expr> parseRelational()
    {
        auto left = parseShift();
        if (! left)
            return left;

        while (lexer.peek().type == TokenType::lt || lexer.peek().type == TokenType::gt || lexer.peek().type == TokenType::le || lexer.peek().type == TokenType::ge)
        {
            BinaryOp op;
            switch (lexer.peek().type)
            {
                case TokenType::lt:
                    op = BinaryOp::lessThan;
                    break;
                case TokenType::gt:
                    op = BinaryOp::greaterThan;
                    break;
                case TokenType::le:
                    op = BinaryOp::lessEqual;
                    break;
                case TokenType::ge:
                    op = BinaryOp::greaterEqual;
                    break;
                default:
                    op = BinaryOp::lessThan;
                    break;
            }

            SourceLocation l = loc();
            lexer.advance();

            auto right = parseShift();
            if (! right)
                return right;

            left = makeBinaryExpr (l, std::move (left).getValue(), op, std::move (right).getValue());
        }

        return left;
    }

    // shift → additive (('<<'|'>>') additive)*
    ResultValue<Expr> parseShift()
    {
        return parseBinaryLeft (TokenType::lshift, [this]()
        {
            return parseAdditive();
        },
                                BinaryOp::shiftLeft,
                                TokenType::rshift,
                                BinaryOp::shiftRight);
    }

    // additive → multiplicative (('+'|'-') multiplicative)*
    ResultValue<Expr> parseAdditive()
    {
        auto left = parseMultiplicative();
        if (! left)
            return left;

        while (lexer.peek().type == TokenType::plus || lexer.peek().type == TokenType::minus)
        {
            BinaryOp op = (lexer.peek().type == TokenType::plus) ? BinaryOp::add : BinaryOp::sub;
            SourceLocation l = loc();
            lexer.advance();

            auto right = parseMultiplicative();
            if (! right)
                return right;

            left = makeBinaryExpr (l, std::move (left).getValue(), op, std::move (right).getValue());
        }

        return left;
    }

    // multiplicative → unary (('*'|'/'|'%') unary)*
    ResultValue<Expr> parseMultiplicative()
    {
        auto left = parseUnary();
        if (! left)
            return left;

        while (lexer.peek().type == TokenType::star || lexer.peek().type == TokenType::slash || lexer.peek().type == TokenType::percent)
        {
            BinaryOp op;
            switch (lexer.peek().type)
            {
                case TokenType::star:
                    op = BinaryOp::mul;
                    break;
                case TokenType::slash:
                    op = BinaryOp::div;
                    break;
                case TokenType::percent:
                    op = BinaryOp::mod;
                    break;
                default:
                    op = BinaryOp::mul;
                    break;
            }

            SourceLocation l = loc();
            lexer.advance();

            auto right = parseUnary();
            if (! right)
                return right;

            left = makeBinaryExpr (l, std::move (left).getValue(), op, std::move (right).getValue());
        }

        return left;
    }

    // unary → ('+'|'-'|'!'|'~'|'++'|'--') unary | postfix
    ResultValue<Expr> parseUnary()
    {
        SourceLocation l = loc();

        if (lexer.peek().type == TokenType::plus)
        {
            lexer.advance();
            auto operand = parseUnary();
            if (! operand)
                return operand;
            return makeUnaryExpr (l, UnaryOp::plus, std::move (operand).getValue());
        }
        if (lexer.peek().type == TokenType::minus)
        {
            lexer.advance();
            auto operand = parseUnary();
            if (! operand)
                return operand;
            return makeUnaryExpr (l, UnaryOp::minus, std::move (operand).getValue());
        }
        if (lexer.peek().type == TokenType::lnot)
        {
            lexer.advance();
            auto operand = parseUnary();
            if (! operand)
                return operand;
            return makeUnaryExpr (l, UnaryOp::logicalNot, std::move (operand).getValue());
        }
        if (lexer.peek().type == TokenType::bnot)
        {
            lexer.advance();
            auto operand = parseUnary();
            if (! operand)
                return operand;
            return makeUnaryExpr (l, UnaryOp::bitwiseNot, std::move (operand).getValue());
        }
        if (lexer.peek().type == TokenType::inc)
        {
            lexer.advance();
            auto operand = parseUnary();
            if (! operand)
                return operand;
            return makeUnaryExpr (l, UnaryOp::preInc, std::move (operand).getValue());
        }
        if (lexer.peek().type == TokenType::dec)
        {
            lexer.advance();
            auto operand = parseUnary();
            if (! operand)
                return operand;
            return makeUnaryExpr (l, UnaryOp::preDec, std::move (operand).getValue());
        }

        return parsePostfix();
    }

    // postfix → primary ('[' expr ']' | '.' identifier | '(' args ')' | '++' | '--')*
    ResultValue<Expr> parsePostfix()
    {
        auto left = parsePrimary();
        if (! left)
            return left;

        while (true)
        {
            SourceLocation l = loc();

            if (match (TokenType::lBracket))
            {
                auto index = parseExpression();
                if (! index)
                    return makeResultValueFail (index.getErrorMessage());
                expect (TokenType::rBracket);

                ExprBracket bracket;
                bracket.loc = l;
                bracket.base = std::make_unique<Expr> (std::move (left).getValue());
                bracket.index = std::make_unique<Expr> (std::move (index).getValue());

                Expr e;
                e.loc = l;
                e.value = std::move (bracket);
                left = makeResultValueOk (std::move (e));
            }
            else if (match (TokenType::dot))
            {
                Token memberTok = expect (TokenType::identifier);

                ExprDot dot;
                dot.loc = l;
                dot.base = std::make_unique<Expr> (std::move (left).getValue());
                dot.member = memberTok.text;

                Expr e;
                e.loc = l;
                e.value = std::move (dot);
                left = makeResultValueOk (std::move (e));
            }
            else if (lexer.peek().type == TokenType::lParen)
            {
                // Function call
                lexer.advance();

                ExprFunCall call;
                call.loc = l;
                call.callee = std::make_unique<Expr> (std::move (left).getValue());

                if (lexer.peek().type != TokenType::rParen)
                {
                    auto args = parseExpressionList();
                    if (! args)
                        return makeResultValueFail (args.getErrorMessage());
                    call.args = std::move (args).getValue();
                }

                expect (TokenType::rParen);

                Expr e;
                e.loc = l;
                e.value = std::move (call);
                left = makeResultValueOk (std::move (e));
            }
            else if (match (TokenType::inc))
            {
                left = makeUnaryExpr (l, UnaryOp::postInc, std::move (left).getValue());
            }
            else if (match (TokenType::dec))
            {
                left = makeUnaryExpr (l, UnaryOp::postDec, std::move (left).getValue());
            }
            else
            {
                break;
            }
        }

        return left;
    }

    // primary → variable | intConst | uintConst | floatConst | boolConst | '(' expr ')' | type_constructor
    ResultValue<Expr> parsePrimary()
    {
        SourceLocation l = loc();

        // Parenthesized expression
        if (match (TokenType::lParen))
        {
            auto expr = parseExpression();
            if (! expr)
                return makeResultValueFail (expr.getErrorMessage());
            expect (TokenType::rParen);

            ExprParen paren;
            paren.loc = l;
            paren.expr = std::make_unique<Expr> (std::move (expr).getValue());

            Expr e;
            e.loc = l;
            e.value = std::move (paren);
            return makeResultValueOk (std::move (e));
        }

        // Integer literal
        if (lexer.peek().type == TokenType::intConst)
        {
            Token tok = lexer.advance();
            ExprIntConst ic;
            ic.loc = tok.loc;
            ic.value = tok.intValue;

            Expr e;
            e.loc = tok.loc;
            e.value = std::move (ic);
            return makeResultValueOk (std::move (e));
        }

        // Unsigned integer literal
        if (lexer.peek().type == TokenType::uintConst)
        {
            Token tok = lexer.advance();
            ExprUIntConst uc;
            uc.loc = tok.loc;
            uc.value = tok.uintValue;

            Expr e;
            e.loc = tok.loc;
            e.value = std::move (uc);
            return makeResultValueOk (std::move (e));
        }

        // Float literal
        if (lexer.peek().type == TokenType::floatConst)
        {
            Token tok = lexer.advance();
            ExprFloatConst fc;
            fc.loc = tok.loc;
            fc.value = tok.floatValue;

            Expr e;
            e.loc = tok.loc;
            e.value = std::move (fc);
            return makeResultValueOk (std::move (e));
        }

        // Boolean literal
        if (lexer.peek().type == TokenType::boolConst)
        {
            Token tok = lexer.advance();
            ExprBoolConst bc;
            bc.loc = tok.loc;
            bc.value = tok.boolValue;

            Expr e;
            e.loc = tok.loc;
            e.value = std::move (bc);
            return makeResultValueOk (std::move (e));
        }

        // Type constructor (type '(' args ')') - check for type keyword
        if (isTypeStart (lexer.peek()))
        {
            auto type = parseTypeSpecifier();
            if (! type)
                return makeResultValueFail (type.getErrorMessage());

            if (lexer.peek().type == TokenType::lParen)
            {
                lexer.advance();

                ExprTypeConstructor ctor;
                ctor.loc = l;
                ctor.type = std::move (type).getValue();

                if (lexer.peek().type != TokenType::rParen)
                {
                    auto args = parseExpressionList();
                    if (! args)
                        return makeResultValueFail (args.getErrorMessage());
                    ctor.args = std::move (args).getValue();
                }

                expect (TokenType::rParen);

                Expr e;
                e.loc = l;
                e.value = std::move (ctor);
                return makeResultValueOk (std::move (e));
            }

            return makeResultValueFail (String::formatted ("%d:%d: Expected '(' after type in constructor",
                                                           l.line,
                                                           l.column));
        }

        // Identifier
        if (lexer.peek().type == TokenType::identifier)
        {
            Token tok = lexer.advance();
            ExprVariable var;
            var.loc = tok.loc;
            var.name = tok.text;

            Expr e;
            e.loc = tok.loc;
            e.value = std::move (var);
            return makeResultValueOk (std::move (e));
        }

        return makeResultValueFail (String::formatted ("%d:%d: Expected expression, got '%s'",
                                                       l.line,
                                                       l.column,
                                                       toString (lexer.peek().type)));
    }

    //==========================================================================
    // Type specifier parsing
    //==========================================================================

    bool isTypeStart (const Token& tok)
    {
        if (tok.type != TokenType::identifier)
            return false;

        const std::string& t = tok.text;

        // Quick check: all GLSL type keywords
        static const std::unordered_set<std::string> typeNames = {
            "void", "float", "int", "uint", "bool", "double", "vec2", "vec3", "vec4", "ivec2", "ivec3", "ivec4", "uvec2", "uvec3", "uvec4", "bvec2", "bvec3", "bvec4", "dvec2", "dvec3", "dvec4", "mat2", "mat3", "mat4", "mat2x2", "mat2x3", "mat2x4", "mat3x2", "mat3x3", "mat3x4", "mat4x2", "mat4x3", "mat4x4", "dmat2", "dmat3", "dmat4", "dmat2x2", "dmat2x3", "dmat2x4", "dmat3x2", "dmat3x3", "dmat3x4", "dmat4x2", "dmat4x3", "dmat4x4", "sampler1D", "sampler2D", "sampler3D", "samplerCube", "sampler1DShadow", "sampler2DShadow", "samplerCubeShadow", "sampler1DArray", "sampler2DArray", "sampler1DArrayShadow", "sampler2DArrayShadow", "sampler2DRect", "sampler2DRectShadow", "samplerBuffer", "sampler2DMS", "sampler2DMSArray", "isampler1D", "isampler2D", "isampler3D", "isamplerCube", "isampler1DArray", "isampler2DArray", "isampler2DRect", "isamplerBuffer", "isampler2DMS", "isampler2DMSArray", "usampler1D", "usampler2D", "usampler3D", "usamplerCube", "usampler1DArray", "usampler2DArray", "usampler2DRect", "usamplerBuffer", "usampler2DMS", "usampler2DMSArray", "image1D", "image2D", "image3D", "imageCube", "image1DArray", "image2DArray", "image2DRect", "imageBuffer", "image2DMS", "image2DMSArray", "iimage1D", "iimage2D", "iimage3D", "iimageCube", "iimage1DArray", "iimage2DArray", "iimage2DRect", "iimageBuffer", "iimage2DMS", "iimage2DMSArray", "uimage1D", "uimage2D", "uimage3D", "uimageCube", "uimage1DArray", "uimage2DArray", "uimage2DRect", "uimageBuffer", "uimage2DMS", "uimage2DMSArray", "atomic_uint",

            // Vulkan separate texture / sampler types
            "texture1D",
            "texture2D",
            "texture3D",
            "textureCube",
            "texture1DArray",
            "texture2DArray",
            "texture2DRect",
            "textureBuffer",
            "texture2DMS",
            "texture2DMSArray",
            "sampler",
            "samplerShadow",
            "subpassInput",
            "subpassInputMS"
        };

        return typeNames.find (t) != typeNames.end();
    }

    ResultValue<TypeSpecifier> parseTypeSpecifier()
    {
        SourceLocation l = loc();
        Token tok = lexer.peek();

        if (tok.type != TokenType::identifier)
        {
            String msg = String::formatted ("%d:%d: Expected type keyword, got '%s' (%s)",
                                            l.line,
                                            l.column,
                                            tok.text.empty() ? toString (tok.type) : tok.text.c_str(),
                                            toString (tok.type));
            return makeResultValueFail (msg);
        }

        const std::string& name = tok.text;

        // Built-in type name
        TypeKind kind = typeNameToKind (name);

        if (kind == TypeKind::namedStruct)
        {
            // Could be a user-defined struct name
            if (name == "struct")
            {
                // Anonymous struct: struct { fields } name
                lexer.advance();
                return parseStructSpecifierAsType (l);
            }

            // Named struct reference
            lexer.advance();
            auto ts = TypeSpecifier::makeNamed (l, name);

            // Array specifiers
            while (lexer.peek().type == TokenType::lBracket)
                ts.arraySpecifiers.push_back (parseArraySpecifier());

            return makeResultValueOk (std::move (ts));
        }

        lexer.advance();

        TypeSpecifier ts = TypeSpecifier::make (l, kind);

        // Array specifiers
        while (lexer.peek().type == TokenType::lBracket)
            ts.arraySpecifiers.push_back (parseArraySpecifier());

        return makeResultValueOk (std::move (ts));
    }

    ResultValue<TypeSpecifier> parseStructSpecifierAsType (SourceLocation l)
    {
        StructSpecifier ss;
        ss.loc = l;

        // Optional struct name
        if (lexer.peek().type == TokenType::identifier)
            ss.name = lexer.advance().text;

        expect (TokenType::lBrace);

        // Parse fields
        while (lexer.peek().type != TokenType::rBrace && lexer.peek().type != TokenType::endOfFile)
        {
            auto field = parseStructFieldSpecifier();
            if (! field)
                return makeResultValueFail (field.getErrorMessage());
            ss.fields.push_back (std::move (field).getValue());
        }

        expect (TokenType::rBrace);

        // Return type specifier referencing this struct
        TypeSpecifier ts;
        ts.loc = l;
        ts.kind = TypeKind::namedStruct;
        ts.structName = ss.name;
        return makeResultValueOk (std::move (ts));
    }

    ResultValue<StructFieldSpecifier> parseStructFieldSpecifier()
    {
        SourceLocation l = loc();
        StructFieldSpecifier field;
        field.loc = l;

        auto qualifier = parseTypeQualifier();
        if (qualifier)
            field.qualifier = std::make_unique<TypeQualifier> (std::move (*qualifier));

        auto type = parseTypeSpecifier();
        if (! type)
            return makeResultValueFail (type.getErrorMessage());
        field.type = std::move (type).getValue();

        field.name = expect (TokenType::identifier).text;

        // Array specifiers on field
        while (lexer.peek().type == TokenType::lBracket)
            field.type.arraySpecifiers.push_back (parseArraySpecifier());

        expect (TokenType::semicolon);
        return makeResultValueOk (std::move (field));
    }

    static TypeKind typeNameToKind (const std::string& name)
    {
        if (name == "void")
            return TypeKind::voidType;
        if (name == "float")
            return TypeKind::floatType;
        if (name == "int")
            return TypeKind::intType;
        if (name == "uint")
            return TypeKind::uintType;
        if (name == "bool")
            return TypeKind::boolType;
        if (name == "double")
            return TypeKind::doubleType;
        if (name == "vec2")
            return TypeKind::vec2;
        if (name == "vec3")
            return TypeKind::vec3;
        if (name == "vec4")
            return TypeKind::vec4;
        if (name == "ivec2")
            return TypeKind::ivec2;
        if (name == "ivec3")
            return TypeKind::ivec3;
        if (name == "ivec4")
            return TypeKind::ivec4;
        if (name == "uvec2")
            return TypeKind::uvec2;
        if (name == "uvec3")
            return TypeKind::uvec3;
        if (name == "uvec4")
            return TypeKind::uvec4;
        if (name == "bvec2")
            return TypeKind::bvec2;
        if (name == "bvec3")
            return TypeKind::bvec3;
        if (name == "bvec4")
            return TypeKind::bvec4;
        if (name == "dvec2")
            return TypeKind::dvec2;
        if (name == "dvec3")
            return TypeKind::dvec3;
        if (name == "dvec4")
            return TypeKind::dvec4;
        if (name == "mat2")
            return TypeKind::mat2;
        if (name == "mat3")
            return TypeKind::mat3;
        if (name == "mat4")
            return TypeKind::mat4;
        if (name == "mat2x2")
            return TypeKind::mat2x2;
        if (name == "mat2x3")
            return TypeKind::mat2x3;
        if (name == "mat2x4")
            return TypeKind::mat2x4;
        if (name == "mat3x2")
            return TypeKind::mat3x2;
        if (name == "mat3x3")
            return TypeKind::mat3x3;
        if (name == "mat3x4")
            return TypeKind::mat3x4;
        if (name == "mat4x2")
            return TypeKind::mat4x2;
        if (name == "mat4x3")
            return TypeKind::mat4x3;
        if (name == "mat4x4")
            return TypeKind::mat4x4;
        if (name == "dmat2")
            return TypeKind::dmat2;
        if (name == "dmat3")
            return TypeKind::dmat3;
        if (name == "dmat4")
            return TypeKind::dmat4;
        if (name == "dmat2x2")
            return TypeKind::dmat2x2;
        if (name == "dmat2x3")
            return TypeKind::dmat2x3;
        if (name == "dmat2x4")
            return TypeKind::dmat2x4;
        if (name == "dmat3x2")
            return TypeKind::dmat3x2;
        if (name == "dmat3x3")
            return TypeKind::dmat3x3;
        if (name == "dmat3x4")
            return TypeKind::dmat3x4;
        if (name == "dmat4x2")
            return TypeKind::dmat4x2;
        if (name == "dmat4x3")
            return TypeKind::dmat4x3;
        if (name == "dmat4x4")
            return TypeKind::dmat4x4;
        if (name == "sampler1D")
            return TypeKind::sampler1D;
        if (name == "sampler2D")
            return TypeKind::sampler2D;
        if (name == "sampler3D")
            return TypeKind::sampler3D;
        if (name == "samplerCube")
            return TypeKind::samplerCube;
        if (name == "sampler1DShadow")
            return TypeKind::sampler1DShadow;
        if (name == "sampler2DShadow")
            return TypeKind::sampler2DShadow;
        if (name == "samplerCubeShadow")
            return TypeKind::samplerCubeShadow;
        if (name == "sampler1DArray")
            return TypeKind::sampler1DArray;
        if (name == "sampler2DArray")
            return TypeKind::sampler2DArray;
        if (name == "sampler1DArrayShadow")
            return TypeKind::sampler1DArrayShadow;
        if (name == "sampler2DArrayShadow")
            return TypeKind::sampler2DArrayShadow;
        if (name == "sampler2DRect")
            return TypeKind::sampler2DRect;
        if (name == "sampler2DRectShadow")
            return TypeKind::sampler2DRectShadow;
        if (name == "samplerBuffer")
            return TypeKind::samplerBuffer;
        if (name == "sampler2DMS")
            return TypeKind::sampler2DMS;
        if (name == "sampler2DMSArray")
            return TypeKind::sampler2DMSArray;
        if (name == "isampler1D")
            return TypeKind::isampler1D;
        if (name == "isampler2D")
            return TypeKind::isampler2D;
        if (name == "isampler3D")
            return TypeKind::isampler3D;
        if (name == "isamplerCube")
            return TypeKind::isamplerCube;
        if (name == "isampler1DArray")
            return TypeKind::isampler1DArray;
        if (name == "isampler2DArray")
            return TypeKind::isampler2DArray;
        if (name == "isampler2DRect")
            return TypeKind::isampler2DRect;
        if (name == "isamplerBuffer")
            return TypeKind::isamplerBuffer;
        if (name == "isampler2DMS")
            return TypeKind::isampler2DMS;
        if (name == "isampler2DMSArray")
            return TypeKind::isampler2DMSArray;
        if (name == "usampler1D")
            return TypeKind::usampler1D;
        if (name == "usampler2D")
            return TypeKind::usampler2D;
        if (name == "usampler3D")
            return TypeKind::usampler3D;
        if (name == "usamplerCube")
            return TypeKind::usamplerCube;
        if (name == "usampler1DArray")
            return TypeKind::usampler1DArray;
        if (name == "usampler2DArray")
            return TypeKind::usampler2DArray;
        if (name == "usampler2DRect")
            return TypeKind::usampler2DRect;
        if (name == "usamplerBuffer")
            return TypeKind::usamplerBuffer;
        if (name == "usampler2DMS")
            return TypeKind::usampler2DMS;
        if (name == "usampler2DMSArray")
            return TypeKind::usampler2DMSArray;
        if (name == "image1D")
            return TypeKind::image1D;
        if (name == "image2D")
            return TypeKind::image2D;
        if (name == "image3D")
            return TypeKind::image3D;
        if (name == "imageCube")
            return TypeKind::imageCube;
        if (name == "image1DArray")
            return TypeKind::image1DArray;
        if (name == "image2DArray")
            return TypeKind::image2DArray;
        if (name == "image2DRect")
            return TypeKind::image2DRect;
        if (name == "imageBuffer")
            return TypeKind::imageBuffer;
        if (name == "image2DMS")
            return TypeKind::image2DMS;
        if (name == "image2DMSArray")
            return TypeKind::image2DMSArray;
        if (name == "iimage1D")
            return TypeKind::iimage1D;
        if (name == "iimage2D")
            return TypeKind::iimage2D;
        if (name == "iimage3D")
            return TypeKind::iimage3D;
        if (name == "iimageCube")
            return TypeKind::iimageCube;
        if (name == "iimage1DArray")
            return TypeKind::iimage1DArray;
        if (name == "iimage2DArray")
            return TypeKind::iimage2DArray;
        if (name == "iimage2DRect")
            return TypeKind::iimage2DRect;
        if (name == "iimageBuffer")
            return TypeKind::iimageBuffer;
        if (name == "iimage2DMS")
            return TypeKind::iimage2DMS;
        if (name == "iimage2DMSArray")
            return TypeKind::iimage2DMSArray;
        if (name == "uimage1D")
            return TypeKind::uimage1D;
        if (name == "uimage2D")
            return TypeKind::uimage2D;
        if (name == "uimage3D")
            return TypeKind::uimage3D;
        if (name == "uimageCube")
            return TypeKind::uimageCube;
        if (name == "uimage1DArray")
            return TypeKind::uimage1DArray;
        if (name == "uimage2DArray")
            return TypeKind::uimage2DArray;
        if (name == "uimage2DRect")
            return TypeKind::uimage2DRect;
        if (name == "uimageBuffer")
            return TypeKind::uimageBuffer;
        if (name == "uimage2DMS")
            return TypeKind::uimage2DMS;
        if (name == "uimage2DMSArray")
            return TypeKind::uimage2DMSArray;
        if (name == "atomic_uint")
            return TypeKind::atomicUint;

        // Vulkan separate texture / sampler types
        if (name == "texture1D")
            return TypeKind::texture1D;
        if (name == "texture2D")
            return TypeKind::texture2D;
        if (name == "texture3D")
            return TypeKind::texture3D;
        if (name == "textureCube")
            return TypeKind::textureCube;
        if (name == "texture1DArray")
            return TypeKind::texture1DArray;
        if (name == "texture2DArray")
            return TypeKind::texture2DArray;
        if (name == "texture2DRect")
            return TypeKind::texture2DRect;
        if (name == "textureBuffer")
            return TypeKind::textureBuffer;
        if (name == "texture2DMS")
            return TypeKind::texture2DMS;
        if (name == "texture2DMSArray")
            return TypeKind::texture2DMSArray;
        if (name == "sampler")
            return TypeKind::samplerType;
        if (name == "samplerShadow")
            return TypeKind::samplerShadow;
        if (name == "subpassInput")
            return TypeKind::subpassInput;
        if (name == "subpassInputMS")
            return TypeKind::subpassInputMS;

        return TypeKind::namedStruct; // user-defined struct type
    }

    //==========================================================================
    // Type qualifier parsing
    //==========================================================================

    std::optional<TypeQualifier> parseTypeQualifier()
    {
        TypeQualifier q;
        q.loc = loc();
        bool hasQualifier = false;

        for (;;)
        {
            // Layout qualifier
            if (matchKeyword ("layout"))
            {
                q.layout = std::make_unique<LayoutQualifier> (parseLayoutQualifier());
                hasQualifier = true;
                continue;
            }

            // Storage qualifiers
            if (matchKeyword ("const"))
            {
                q.storage.push_back (StorageQualifier::constQual);
                hasQualifier = true;
                continue;
            }
            if (matchKeyword ("in"))
            {
                q.storage.push_back (StorageQualifier::in);
                hasQualifier = true;
                continue;
            }
            if (matchKeyword ("out"))
            {
                q.storage.push_back (StorageQualifier::out);
                hasQualifier = true;
                continue;
            }
            if (matchKeyword ("inout"))
            {
                q.storage.push_back (StorageQualifier::inout);
                hasQualifier = true;
                continue;
            }
            if (matchKeyword ("uniform"))
            {
                q.storage.push_back (StorageQualifier::uniform);
                hasQualifier = true;
                continue;
            }
            if (matchKeyword ("buffer"))
            {
                q.storage.push_back (StorageQualifier::buffer);
                hasQualifier = true;
                continue;
            }
            if (matchKeyword ("shared"))
            {
                q.storage.push_back (StorageQualifier::shared);
                hasQualifier = true;
                continue;
            }
            if (matchKeyword ("centroid"))
            {
                q.storage.push_back (StorageQualifier::centroid);
                hasQualifier = true;
                continue;
            }
            if (matchKeyword ("sample"))
            {
                q.storage.push_back (StorageQualifier::sample);
                hasQualifier = true;
                continue;
            }

            // Interpolation
            if (matchKeyword ("flat"))
            {
                q.interpolation.push_back (InterpolationQualifier::flat);
                hasQualifier = true;
                continue;
            }
            if (matchKeyword ("smooth"))
            {
                q.interpolation.push_back (InterpolationQualifier::smooth);
                hasQualifier = true;
                continue;
            }
            if (matchKeyword ("noperspective"))
            {
                q.interpolation.push_back (InterpolationQualifier::noPerspective);
                hasQualifier = true;
                continue;
            }

            // Precision
            if (matchKeyword ("lowp"))
            {
                q.precision.push_back (PrecisionQualifier::lowp);
                hasQualifier = true;
                continue;
            }
            if (matchKeyword ("mediump"))
            {
                q.precision.push_back (PrecisionQualifier::mediump);
                hasQualifier = true;
                continue;
            }
            if (matchKeyword ("highp"))
            {
                q.precision.push_back (PrecisionQualifier::highp);
                hasQualifier = true;
                continue;
            }

            // Invariant / precise
            if (matchKeyword ("invariant"))
            {
                q.invariant = true;
                hasQualifier = true;
                continue;
            }
            if (matchKeyword ("precise"))
            {
                q.precise = true;
                hasQualifier = true;
                continue;
            }

            break;
        }

        return hasQualifier ? std::optional<TypeQualifier> (std::move (q)) : std::nullopt;
    }

    LayoutQualifier parseLayoutQualifier()
    {
        SourceLocation l = loc();
        LayoutQualifier layout;
        layout.loc = l;

        expect (TokenType::lParen);

        layout.entries.push_back (parseLayoutQualifierEntry());

        while (match (TokenType::comma))
            layout.entries.push_back (parseLayoutQualifierEntry());

        expect (TokenType::rParen);
        return layout;
    }

    LayoutQualifierEntry parseLayoutQualifierEntry()
    {
        SourceLocation l = loc();
        LayoutQualifierEntry entry;
        entry.loc = l;

        // Parse ID
        Token idTok = expect (TokenType::identifier);
        entry.id = layoutIdFromString (idTok.text);

        // Optional = value
        if (match (TokenType::assign))
        {
            auto val = parseExpression();
            if (val)
                entry.value = std::make_unique<Expr> (std::move (val).getValue());
        }

        return entry;
    }

    static LayoutQualifierId layoutIdFromString (const std::string& s)
    {
        if (s == "location")
            return LayoutQualifierId::location;
        if (s == "binding")
            return LayoutQualifierId::binding;
        if (s == "set")
            return LayoutQualifierId::descriptorSet;
        if (s == "component")
            return LayoutQualifierId::component;
        if (s == "local_size_x")
            return LayoutQualifierId::localSizeX;
        if (s == "local_size_y")
            return LayoutQualifierId::localSizeY;
        if (s == "local_size_z")
            return LayoutQualifierId::localSizeZ;
        if (s == "vertices")
            return LayoutQualifierId::vertices;
        if (s == "invocations")
            return LayoutQualifierId::invocations;
        if (s == "spacing")
            return LayoutQualifierId::tessSpacing;
        if (s == "vertex_spacing")
            return LayoutQualifierId::tessVertices;
        if (s == "output_topology")
            return LayoutQualifierId::tessOutputTopology;
        if (s == "input_topology")
            return LayoutQualifierId::tessInputMode;
        if (s == "points")
            return LayoutQualifierId::points;
        if (s == "lines")
            return LayoutQualifierId::lines;
        if (s == "line_strip")
            return LayoutQualifierId::lineStrip;
        if (s == "lines_adjacency")
            return LayoutQualifierId::linesAdjacency;
        if (s == "triangles")
            return LayoutQualifierId::triangles;
        if (s == "triangle_strip")
            return LayoutQualifierId::triangleStrip;
        if (s == "triangles_adjacency")
            return LayoutQualifierId::trianglesAdjacency;
        if (s == "fractional_even_spacing")
            return LayoutQualifierId::fractionalEvenSpacing;
        if (s == "fractional_odd_spacing")
            return LayoutQualifierId::fractionalOddSpacing;
        if (s == "equal_spacing")
            return LayoutQualifierId::equalSpacing;
        if (s == "cw")
            return LayoutQualifierId::cw;
        if (s == "ccw")
            return LayoutQualifierId::ccw;
        if (s == "isolines")
            return LayoutQualifierId::isolines;
        if (s == "quads")
            return LayoutQualifierId::quads;
        if (s == "xfb_buffer")
            return LayoutQualifierId::xfbBuffer;
        if (s == "xfb_stride")
            return LayoutQualifierId::xfbStride;
        if (s == "xfb_offset")
            return LayoutQualifierId::xfbOffset;
        if (s == "input_attachment_index")
            return LayoutQualifierId::inputAttachmentIndex;
        if (s == "std140")
            return LayoutQualifierId::std140;
        if (s == "std430")
            return LayoutQualifierId::std430;
        if (s == "column_major")
            return LayoutQualifierId::columnMajor;
        if (s == "row_major")
            return LayoutQualifierId::rowMajor;
        if (s == "early_fragment_tests")
            return LayoutQualifierId::earlyFragmentTests;
        if (s == "depth_greater")
            return LayoutQualifierId::depthGreater;
        if (s == "depth_less")
            return LayoutQualifierId::depthLess;
        if (s == "depth_unchanged")
            return LayoutQualifierId::depthUnchanged;
        if (s == "depth_any")
            return LayoutQualifierId::depthAny;
        return LayoutQualifierId::location; // fallback; should not happen with valid input
    }

    //==========================================================================
    // Declaration parsing
    //==========================================================================

    ResultValue<Declaration> parseDeclaration (bool isGlobal)
    {
        SourceLocation l = loc();

        // Check for struct definition
        if (matchKeyword ("struct"))
        {
            auto ss = parseStructSpecifierDeclaration (l);
            if (! ss)
                return makeResultValueFail (ss.getErrorMessage());

            Declaration decl;
            decl.loc = l;
            decl.structSpecifier = std::make_unique<StructSpecifier> (std::move (ss).getValue());
            return makeResultValueOk (std::move (decl));
        }

        // Parse type qualifiers
        auto qualifier = parseTypeQualifier();

        // Parse type
        auto type = parseTypeSpecifier();
        if (! type)
            return makeResultValueFail (type.getErrorMessage());

        // Parse init declarator list
        InitDeclaratorList initList;
        initList.loc = l;
        if (qualifier)
            initList.qualifier = std::make_unique<TypeQualifier> (std::move (*qualifier));
        initList.type = std::move (type).getValue();

        // First declarator
        Token nameTok = expect (TokenType::identifier);
        auto firstDecl = parseSingleDeclarationRest (initList.type, std::string (nameTok.text));
        if (! firstDecl)
            return makeResultValueFail (firstDecl.getErrorMessage());
        initList.declarations.push_back (std::move (firstDecl).getValue());

        // Additional declarators
        while (match (TokenType::comma))
        {
            Token nextTok = expect (TokenType::identifier);
            auto nextDecl = parseSingleDeclarationRest (initList.type, std::string (nextTok.text));
            if (! nextDecl)
                return makeResultValueFail (nextDecl.getErrorMessage());
            initList.declarations.push_back (std::move (nextDecl).getValue());
        }

        Declaration decl;
        decl.loc = l;
        decl.initDeclaratorList = std::make_unique<InitDeclaratorList> (std::move (initList));
        return makeResultValueOk (std::move (decl));
    }

    ResultValue<SingleDeclaration> parseSingleDeclarationRest (const TypeSpecifier& type,
                                                               std::string name)
    {
        SourceLocation l = loc();
        SingleDeclaration decl;
        decl.loc = l;
        decl.name = name;

        // Array specifiers
        while (lexer.peek().type == TokenType::lBracket)
            decl.arraySpecifiers.push_back (parseArraySpecifier());

        // Optional initializer
        if (match (TokenType::assign))
        {
            auto init = parseInitializer();
            if (! init)
                return makeResultValueFail (init.getErrorMessage());
            decl.initializer = std::make_unique<Initializer> (std::move (init).getValue());
        }

        return makeResultValueOk (std::move (decl));
    }

    ArraySpecifier parseArraySpecifier()
    {
        SourceLocation l = loc();
        ArraySpecifier arr;
        arr.loc = l;

        expect (TokenType::lBracket);

        if (lexer.peek().type != TokenType::rBracket)
        {
            auto sizeExpr = parseExpression();
            if (sizeExpr)
                arr.sizeExpr = std::make_unique<Expr> (std::move (sizeExpr).getValue());
            else
                arr.isUnsized = true;
        }
        else
        {
            arr.isUnsized = true;
        }

        expect (TokenType::rBracket);
        return arr;
    }

    ResultValue<Initializer> parseInitializer()
    {
        SourceLocation l = loc();

        // Aggregate initializer: { ... }
        if (match (TokenType::lBrace))
        {
            Initializer init;
            init.loc = l;

            if (lexer.peek().type != TokenType::rBrace)
            {
                auto first = parseInitializer();
                if (! first)
                    return makeResultValueFail (first.getErrorMessage());
                init.aggregate.push_back (std::move (first).getValue());

                while (match (TokenType::comma))
                {
                    auto next = parseInitializer();
                    if (next)
                        init.aggregate.push_back (std::move (next).getValue());
                }
            }

            expect (TokenType::rBrace);
            return makeResultValueOk (std::move (init));
        }

        // Expression initializer
        auto expr = parseAssignment();
        if (! expr)
            return makeResultValueFail (expr.getErrorMessage());

        Initializer init;
        init.loc = l;
        init.expr = std::make_unique<Expr> (std::move (expr).getValue());
        return makeResultValueOk (std::move (init));
    }

    ResultValue<StructSpecifier> parseStructSpecifierDeclaration (SourceLocation l)
    {
        StructSpecifier ss;
        ss.loc = l;

        // Name
        if (lexer.peek().type == TokenType::identifier)
            ss.name = lexer.advance().text;

        expect (TokenType::lBrace);

        while (lexer.peek().type != TokenType::rBrace && lexer.peek().type != TokenType::endOfFile)
        {
            auto field = parseStructFieldSpecifier();
            if (! field)
                return makeResultValueFail (field.getErrorMessage());
            ss.fields.push_back (std::move (field).getValue());
        }

        expect (TokenType::rBrace);
        return makeResultValueOk (std::move (ss));
    }

    //==========================================================================
    // Helper methods
    //==========================================================================

    ResultValue<Expr> parseBinaryLeft (TokenType tt,
                                       std::function<ResultValue<Expr>()> subParse,
                                       BinaryOp op)
    {
        auto left = subParse();
        if (! left)
            return left;

        while (match (tt))
        {
            SourceLocation l = loc();
            auto right = subParse();
            if (! right)
                return right;
            left = makeBinaryExpr (l, std::move (left).getValue(), op, std::move (right).getValue());
        }

        return left;
    }

    ResultValue<Expr> parseBinaryLeft (TokenType tt1,
                                       std::function<ResultValue<Expr>()> subParse,
                                       BinaryOp op1,
                                       TokenType tt2,
                                       BinaryOp op2)
    {
        auto left = subParse();
        if (! left)
            return left;

        while (lexer.peek().type == tt1 || lexer.peek().type == tt2)
        {
            BinaryOp op = (lexer.peek().type == tt1) ? op1 : op2;
            SourceLocation l = loc();
            lexer.advance();

            auto right = subParse();
            if (! right)
                return right;
            left = makeBinaryExpr (l, std::move (left).getValue(), op, std::move (right).getValue());
        }

        return left;
    }

    ResultValue<std::vector<Expr>> parseExpressionList()
    {
        std::vector<Expr> exprs;

        auto first = parseAssignment();
        if (! first)
            return makeResultValueFail (first.getErrorMessage());
        exprs.push_back (std::move (first).getValue());

        while (match (TokenType::comma))
        {
            auto next = parseAssignment();
            if (! next)
                return makeResultValueFail (next.getErrorMessage());
            exprs.push_back (std::move (next).getValue());
        }

        return makeResultValueOk (std::move (exprs));
    }

    static ResultValue<Expr> makeBinaryExpr (SourceLocation l, Expr left, BinaryOp op, Expr right)
    {
        ExprBinary bin;
        bin.loc = l;
        bin.op = op;
        bin.left = std::make_unique<Expr> (std::move (left));
        bin.right = std::make_unique<Expr> (std::move (right));

        Expr e;
        e.loc = l;
        e.value = std::move (bin);
        return makeResultValueOk (std::move (e));
    }

    static ResultValue<Expr> makeUnaryExpr (SourceLocation l, UnaryOp op, Expr operand)
    {
        ExprUnary un;
        un.loc = l;
        un.op = op;
        un.operand = std::make_unique<Expr> (std::move (operand));

        Expr e;
        e.loc = l;
        e.value = std::move (un);
        return makeResultValueOk (std::move (e));
    }

    static AssignmentOp tokenToAssignmentOp (TokenType tt)
    {
        switch (tt)
        {
            case TokenType::assign:
                return AssignmentOp::assign;
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
        return parser.parseTranslationUnit();
    }
    catch (const std::runtime_error& e)
    {
        return makeResultValueFail (String (e.what()));
    }
    catch (const std::exception& e)
    {
        return makeResultValueFail (String ("GLSL parse error: ") + e.what());
    }
}

} // namespace wgsl
} // namespace yup
