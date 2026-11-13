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

//==============================================================================
class YuiTextParser
{
public:
    struct Error
    {
        int offset = 0;
        String message;
    };

    explicit YuiTextParser (const String& textToParse)
        : source (textToParse)
        , text (textToParse.toRawUTF8())
    {
    }

    DataTree parseDocument()
    {
        DataTree document ("yui");
        auto transaction = document.beginTransaction();

        while (! isAtEnd())
        {
            const auto start = position;
            const auto keyword = parseIdentifier ("Expected a 'drawing' block");

            if (keyword != "drawing")
                throw Error { start, "Unknown block '" + keyword + "', expected 'drawing'" };

            transaction.addChild (parseDrawing());
        }

        transaction.commit();
        return document;
    }

    YuiParser::Diagnostic locate (const Error& error) const
    {
        int line = 1;
        int lineStart = 0;

        for (int i = 0; i < error.offset; ++i)
        {
            if (text[i] == '\n')
            {
                ++line;
                lineStart = i + 1;
            }
        }

        return { line, String::fromUTF8 (text + lineStart, error.offset - lineStart).length() + 1, error.message };
    }

private:
    DataTree parseDrawing()
    {
        DataTree drawing ("drawing");

        if (! lookingAt ("{"))
            drawing.beginTransaction().setProperty ("name", parseIdentifier ("Expected the name of the drawing"));

        parseBody (drawing);
        return drawing;
    }

    void parseBody (DataTree& element)
    {
        expect ("{");

        auto transaction = element.beginTransaction();
        StringArray properties;

        while (! match ("}"))
        {
            if (isAtEnd())
                throw Error { position, "Expected '}'" };

            const auto start = position;
            const auto name = parseIdentifier ("Expected a property or an element");

            if (name == "let")
            {
                transaction.addChild (parseLet());
            }
            else if (name == "repeat")
            {
                transaction.addChild (parseRepeat());
            }
            else if (lookingAt ("{"))
            {
                DataTree child { Identifier (name) };
                parseBody (child);
                transaction.addChild (child);
            }
            else
            {
                if (properties.contains (name))
                    throw Error { start, "The property '" + name + "' is already set" };

                expect (":");
                properties.add (name);
                transaction.setProperty (name, parseExpressionText());
                expect (";");
            }
        }
    }

    DataTree parseLet()
    {
        DataTree let ("let");
        auto transaction = let.beginTransaction();

        transaction.setProperty ("name", parseIdentifier ("Expected the name of the let"));
        expect (":");
        transaction.setProperty ("value", parseExpressionText());
        expect (";");

        transaction.commit();
        return let;
    }

    DataTree parseRepeat()
    {
        DataTree repeat ("repeat");

        {
            auto transaction = repeat.beginTransaction();

            transaction.setProperty ("name", parseIdentifier ("Expected the name of the repeat index"));
            expectKeyword ("in");
            transaction.setProperty ("from", parseExpressionText());
            expect ("..");
            transaction.setProperty ("to", parseExpressionText());
        }

        parseBody (repeat);
        return repeat;
    }

    // Values are kept as written, the expression parser only finds where they end
    String parseExpressionText()
    {
        skipWhitespace();

        const auto start = position;
        auto end = position;
        const auto parsed = Expression::parseExpression (source, end);

        if (parsed.failed())
            throw Error { end, parsed.getErrorMessage().fromFirstOccurrenceOf (": ", false, false) };

        position = end;
        return String::fromUTF8 (text + start, end - start).trimEnd();
    }

    //==============================================================================
    String parseIdentifier (const String& error)
    {
        skipWhitespace();

        const auto start = position;
        if (! isIdentifierStart (text[position]))
            throw Error { position, error };

        while (isIdentifierStart (text[position]) || CharacterFunctions::isDigit (text[position]))
            ++position;

        return String::fromUTF8 (text + start, position - start);
    }

    void expectKeyword (const String& keyword)
    {
        skipWhitespace();

        const auto start = position;
        const auto error = "Expected '" + keyword + "'";

        if (parseIdentifier (error) != keyword)
            throw Error { start, error };
    }

    void expect (const char* token)
    {
        if (! match (token))
            throw Error { position, "Expected '" + String (token) + "'" };
    }

    bool match (const char* token)
    {
        if (! lookingAt (token))
            return false;

        position += (int) std::strlen (token);
        return true;
    }

    bool lookingAt (const char* token)
    {
        skipWhitespace();
        return std::strncmp (text + position, token, std::strlen (token)) == 0;
    }

    bool isAtEnd()
    {
        skipWhitespace();
        return text[position] == 0;
    }

    void skipWhitespace()
    {
        for (;;)
        {
            const auto c = text[position];

            if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            {
                ++position;
            }
            else if (c == '/' && text[position + 1] == '/')
            {
                while (text[position] != 0 && text[position] != '\n')
                    ++position;
            }
            else if (c == '/' && text[position + 1] == '*')
            {
                const auto* end = std::strstr (text + position + 2, "*/");
                if (end == nullptr)
                    throw Error { position, "Unterminated comment" };

                position = (int) (end - text) + 2;
            }
            else
            {
                return;
            }
        }
    }

    static bool isIdentifierStart (char c) noexcept
    {
        return CharacterFunctions::isLetter (c) || c == '_';
    }

    const String& source;
    const char* text;
    int position = 0;
};

} // namespace

//==============================================================================
String YuiParser::Diagnostic::toString() const
{
    return String (line) + ":" + String (column) + ": " + message;
}

YuiParser::ParseResult YuiParser::parse (const String& text)
{
    YuiTextParser parser (text);

    try
    {
        return { parser.parseDocument(), {} };
    }
    catch (const YuiTextParser::Error& error)
    {
        return { {}, { parser.locate (error) } };
    }
}

} // namespace yup
