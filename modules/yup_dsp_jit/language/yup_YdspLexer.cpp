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

namespace
{

constexpr std::pair<const char*, YdspTokenType> keywordTable[] = {
    { "processor", YdspTokenType::kwProcessor },
    { "graph", YdspTokenType::kwGraph },
    { "node", YdspTokenType::kwNode },
    { "connection", YdspTokenType::kwConnection },
    { "input", YdspTokenType::kwInput },
    { "output", YdspTokenType::kwOutput },
    { "value", YdspTokenType::kwValue },
    { "stream", YdspTokenType::kwStream },
    { "state", YdspTokenType::kwState },
    { "process", YdspTokenType::kwProcess },
    { "block", YdspTokenType::kwBlock },
    { "for", YdspTokenType::kwFor },
    { "if", YdspTokenType::kwIf },
    { "else", YdspTokenType::kwElse },
    { "let", YdspTokenType::kwLet },
    { "true", YdspTokenType::kwTrue },
    { "false", YdspTokenType::kwFalse },
    { "declare", YdspTokenType::kwDeclare },
    { "func", YdspTokenType::kwFunc },
    { "return", YdspTokenType::kwReturn },
    { "import", YdspTokenType::kwImport },
    { "struct", YdspTokenType::kwStruct },
    { "init", YdspTokenType::kwInit },
    { "event", YdspTokenType::kwEvent },
    { "emit", YdspTokenType::kwEmit },
};

YdspTokenType keywordType (StringRef text)
{
    for (const auto& [keyword, type] : keywordTable)
        if (text == StringRef (keyword))
            return type;

    return YdspTokenType::identifier;
}

bool isIdentifierStart (int c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool isIdentifierPart (int c)
{
    return isIdentifierStart (c) || (c >= '0' && c <= '9');
}

bool isDigit (int c)
{
    return c >= '0' && c <= '9';
}

bool isHexDigit (int c)
{
    return isDigit (c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

} // namespace

//==============================================================================

YdspLexer::YdspLexer (StringRef source, YdspDiagnostics& diagnostics)
    : source (String (source))
    , diagnostics (diagnostics)
{
}

int YdspLexer::peek (int offset) const noexcept
{
    const auto index = static_cast<int> (pos + offset);
    return index < source.length() ? static_cast<int> (source[index]) : -1;
}

int YdspLexer::current() const noexcept
{
    return peek (0);
}

void YdspLexer::advance() noexcept
{
    if (pos < static_cast<size_t> (source.length()))
    {
        if (source[static_cast<int> (pos)] == '\n')
        {
            ++line;
            column = 1;
        }
        else
        {
            ++column;
        }

        ++pos;
    }
}

void YdspLexer::skipWhitespaceAndComments()
{
    for (;;)
    {
        const auto c = current();

        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        {
            advance();
            continue;
        }

        if (c == '/' && peek (1) == '/')
        {
            while (current() != -1 && current() != '\n')
                advance();

            continue;
        }

        if (c == '/' && peek (1) == '*')
        {
            const auto startLine = line;
            const auto startColumn = column;

            advance();
            advance();

            bool closed = false;

            while (current() != -1)
            {
                if (current() == '*' && peek (1) == '/')
                {
                    advance();
                    advance();
                    closed = true;
                    break;
                }

                advance();
            }

            if (! closed)
                diagnostics.addError (startLine, startColumn, "Unterminated block comment");

            continue;
        }

        break;
    }
}

YdspToken YdspLexer::lexIdentifierOrKeyword()
{
    const auto startLine = line;
    const auto startColumn = column;
    const auto start = pos;

    while (isIdentifierPart (current()))
        advance();

    const auto text = source.substring (static_cast<int> (start), static_cast<int> (pos));

    // A standalone underscore is the wildcard token, not an identifier.
    if (text == "_")
        return { YdspTokenType::underscore, text, startLine, startColumn };

    return { keywordType (text), text, startLine, startColumn };
}

YdspToken YdspLexer::lexNumber()
{
    const auto startLine = line;
    const auto startColumn = column;
    const auto start = pos;

    const auto consumeDigits = [this] (auto isDigitLike)
    {
        while (isDigitLike (current()) || (current() == '_' && isDigitLike (peek (1))))
            advance();
    };

    if (current() == '0' && (peek (1) == 'x' || peek (1) == 'X'))
    {
        advance();
        advance();
        const auto digitsStart = pos;
        consumeDigits (isHexDigit);

        if (pos == digitsStart)
            diagnostics.addError (startLine, startColumn, "Hex literal has no digits after '0x'");

        return { YdspTokenType::intLiteral, source.substring (static_cast<int> (start), static_cast<int> (pos)), startLine, startColumn };
    }

    if (current() == '0' && (peek (1) == 'b' || peek (1) == 'B'))
    {
        advance();
        advance();
        const auto digitsStart = pos;
        consumeDigits ([] (int c)
        {
            return c == '0' || c == '1';
        });

        if (pos == digitsStart)
            diagnostics.addError (startLine, startColumn, "Binary literal has no digits after '0b'");

        return { YdspTokenType::intLiteral, source.substring (static_cast<int> (start), static_cast<int> (pos)), startLine, startColumn };
    }

    consumeDigits (isDigit);

    bool isFloat = false;

    if (current() == '.' && isDigit (peek (1)))
    {
        isFloat = true;
        advance();
        consumeDigits (isDigit);
    }
    else if (current() == '.' && peek (1) != '.' && ! isIdentifierStart (peek (1)))
    {
        isFloat = true;
        advance();
    }

    if ((current() == 'e' || current() == 'E') && (isDigit (peek (1)) || ((peek (1) == '+' || peek (1) == '-') && isDigit (peek (2)))))
    {
        isFloat = true;
        advance();

        if (current() == '+' || current() == '-')
            advance();

        consumeDigits (isDigit);
    }

    const auto text = source.substring (static_cast<int> (start), static_cast<int> (pos));

    return { isFloat ? YdspTokenType::floatLiteral : YdspTokenType::intLiteral, text, startLine, startColumn };
}

YdspToken YdspLexer::lexString()
{
    const auto startLine = line;
    const auto startColumn = column;

    advance(); // opening quote

    String text;

    while (current() != -1 && current() != '"')
    {
        if (current() == '\n')
        {
            diagnostics.addError (line, column, "Unterminated string literal");
            break;
        }

        if (current() == '\\')
        {
            const auto escapeLine = line;
            const auto escapeColumn = column;
            advance();

            if (current() == -1 || current() == '\n')
            {
                diagnostics.addError (escapeLine, escapeColumn, "Unterminated string literal");
                break;
            }

            switch (current())
            {
                case 'n':
                    text += '\n';
                    break;
                case 't':
                    text += '\t';
                    break;
                case 'r':
                    text += '\r';
                    break;
                case '\\':
                    text += '\\';
                    break;
                case '"':
                    text += '"';
                    break;
                default:
                    diagnostics.addError (escapeLine, escapeColumn, "Unknown string escape '\\" + String::charToString (static_cast<yup_wchar> (current())) + "'");
                    break;
            }

            advance();
            continue;
        }

        text += static_cast<yup_wchar> (current());
        advance();
    }

    if (current() == '"')
    {
        advance();
    }
    else if (current() == -1)
    {
        diagnostics.addError (startLine, startColumn, "Unterminated string literal");
    }

    return { YdspTokenType::stringLiteral, text, startLine, startColumn };
}

YdspToken YdspLexer::lexSymbol()
{
    const auto startLine = line;
    const auto startColumn = column;

    const auto c = current();
    const auto n = peek (1);

    advance();

    switch (c)
    {
        case '{':
            return { YdspTokenType::lBrace, "{", startLine, startColumn };
        case '}':
            return { YdspTokenType::rBrace, "}", startLine, startColumn };
        case '(':
            return { YdspTokenType::lParen, "(", startLine, startColumn };
        case ')':
            return { YdspTokenType::rParen, ")", startLine, startColumn };
        case ';':
            return { YdspTokenType::semi, ";", startLine, startColumn };
        case ',':
            return { YdspTokenType::comma, ",", startLine, startColumn };
        case '~':
            return { YdspTokenType::tilde, "~", startLine, startColumn };
        case '_':
            return { YdspTokenType::underscore, "_", startLine, startColumn };
        case '\'':
            return { YdspTokenType::apostrophe, "'", startLine, startColumn };
        case '@':
            return { YdspTokenType::at, "@", startLine, startColumn };
        case '?':
            return { YdspTokenType::question, "?", startLine, startColumn };
        case '=':
            if (n == '=')
            {
                advance();
                return { YdspTokenType::equal, "==", startLine, startColumn };
            }
            return { YdspTokenType::assign, "=", startLine, startColumn };
        case '!':
            if (n == '=')
            {
                advance();
                return { YdspTokenType::notEqual, "!=", startLine, startColumn };
            }
            return { YdspTokenType::not_, "!", startLine, startColumn };
        case '<':
            if (n == ':')
            {
                advance();
                return { YdspTokenType::lessColon, "<:", startLine, startColumn };
            }
            if (n == '<')
            {
                advance();
                if (current() == '=')
                {
                    advance();
                    return { YdspTokenType::shlEq, "<<=", startLine, startColumn };
                }
                return { YdspTokenType::shl, "<<", startLine, startColumn };
            }
            if (n == '=')
            {
                advance();
                return { YdspTokenType::lessEqual, "<=", startLine, startColumn };
            }
            return { YdspTokenType::less, "<", startLine, startColumn };
        case '>':
            if (n == '>')
            {
                advance();
                if (current() == '=')
                {
                    advance();
                    return { YdspTokenType::shrEq, ">>=", startLine, startColumn };
                }
                return { YdspTokenType::shr, ">>", startLine, startColumn };
            }
            if (n == '=')
            {
                advance();
                return { YdspTokenType::greaterEqual, ">=", startLine, startColumn };
            }
            return { YdspTokenType::greater, ">", startLine, startColumn };
        case '+':
            if (n == '=')
            {
                advance();
                return { YdspTokenType::plusEq, "+=", startLine, startColumn };
            }
            return { YdspTokenType::plus, "+", startLine, startColumn };
        case '-':
            if (n == '=')
            {
                advance();
                return { YdspTokenType::minusEq, "-=", startLine, startColumn };
            }
            if (n == '>')
            {
                advance();
                return { YdspTokenType::arrow, "->", startLine, startColumn };
            }
            return { YdspTokenType::minus, "-", startLine, startColumn };
        case '*':
            if (n == '=')
            {
                advance();
                return { YdspTokenType::starEq, "*=", startLine, startColumn };
            }
            return { YdspTokenType::star, "*", startLine, startColumn };
        case '/':
            if (n == '=')
            {
                advance();
                return { YdspTokenType::slashEq, "/=", startLine, startColumn };
            }
            return { YdspTokenType::slash, "/", startLine, startColumn };
        case '%':
            if (n == '=')
            {
                advance();
                return { YdspTokenType::percentEq, "%=", startLine, startColumn };
            }
            return { YdspTokenType::percent, "%", startLine, startColumn };
        case ':':
            if (n == '>')
            {
                advance();
                return { YdspTokenType::colonGreater, ":>", startLine, startColumn };
            }
            return { YdspTokenType::colon, ":", startLine, startColumn };
        case '.':
            if (n == '.')
            {
                advance();
                return { YdspTokenType::range, "..", startLine, startColumn };
            }
            return { YdspTokenType::dot, ".", startLine, startColumn };
        case '[':
            if (n == '[')
            {
                advance();
                return { YdspTokenType::lAnnotation, "[[", startLine, startColumn };
            }
            return { YdspTokenType::lBracket, "[", startLine, startColumn };
        case ']':
            if (n == ']')
            {
                advance();
                return { YdspTokenType::rAnnotation, "]]", startLine, startColumn };
            }
            return { YdspTokenType::rBracket, "]", startLine, startColumn };
        case '&':
            if (n == '&')
            {
                advance();
                return { YdspTokenType::andAnd, "&&", startLine, startColumn };
            }
            if (n == '=')
            {
                advance();
                return { YdspTokenType::ampersandEq, "&=", startLine, startColumn };
            }
            return { YdspTokenType::ampersand, "&", startLine, startColumn };
        case '|':
            if (n == '|')
            {
                advance();
                return { YdspTokenType::orOr, "||", startLine, startColumn };
            }
            if (n == '=')
            {
                advance();
                return { YdspTokenType::pipeEq, "|=", startLine, startColumn };
            }
            return { YdspTokenType::pipe, "|", startLine, startColumn };
        case '^':
            if (n == '=')
            {
                advance();
                return { YdspTokenType::caretEq, "^=", startLine, startColumn };
            }
            return { YdspTokenType::caret, "^", startLine, startColumn };
        default:
            break;
    }

    diagnostics.addError (startLine, startColumn, String ("Unexpected character '") + String::charToString (static_cast<yup_wchar> (c)) + "'");

    return { YdspTokenType::endOfFile, {}, startLine, startColumn };
}

std::vector<YdspToken> YdspLexer::tokenize()
{
    std::vector<YdspToken> tokens;

    for (;;)
    {
        skipWhitespaceAndComments();

        const auto c = current();

        if (c == -1)
        {
            tokens.push_back ({ YdspTokenType::endOfFile, {}, line, column });
            break;
        }

        if (isIdentifierStart (c))
        {
            tokens.push_back (lexIdentifierOrKeyword());
            continue;
        }

        if (isDigit (c) || (c == '.' && isDigit (peek (1))))
        {
            tokens.push_back (lexNumber());
            continue;
        }

        if (c == '"')
        {
            tokens.push_back (lexString());
            continue;
        }

        tokens.push_back (lexSymbol());
    }

    return tokens;
}

} // namespace yup
