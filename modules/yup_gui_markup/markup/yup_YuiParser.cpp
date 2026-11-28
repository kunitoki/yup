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

String unquoteYuiText (const String& raw)
{
    const auto trimmed = raw.trim();

    if (trimmed.length() < 2)
        return raw;

    const auto quote = trimmed[0];
    if (quote != '"' && quote != '\'')
        return raw;

    String unescaped;
    auto ptr = trimmed.getCharPointer();
    ++ptr;

    while (! ptr.isEmpty())
    {
        const auto c = ptr.getAndAdvance();

        if (c == quote)
            return ptr.isEmpty() ? unescaped : raw;

        if (c != '\\')
        {
            unescaped << String::charToString (c);
            continue;
        }

        if (ptr.isEmpty())
            return raw;

        const auto escaped = ptr.getAndAdvance();

        switch (escaped)
        {
            case '\\': unescaped << '\\'; break;
            case '"':  unescaped << '"'; break;
            case '\'': unescaped << '\''; break;
            case 'n':  unescaped << '\n'; break;
            case 'r':  unescaped << '\r'; break;
            case 't':  unescaped << '\t'; break;
            default:   unescaped << '\\' << String::charToString (escaped); break;
        }
    }

    return raw;
}

//==============================================================================
class YuiTextParser
{
public:
    struct Error
    {
        int offset = 0;
        String message;
    };

    YuiTextParser (const String& textToParse, const String& sourceToReport)
        : source (textToParse)
        , sourceName (sourceToReport)
        , text (source.toRawUTF8())
    {
        lineStarts.push_back (0);

        for (int i = 0; text[i] != 0; ++i)
        {
            if (text[i] == '\n')
                lineStarts.push_back (i + 1);
        }
    }

    DataTree parseDocument()
    {
        struct BlockParser
        {
            const char* keyword;
            DataTree (YuiTextParser::*parse)();
        };

        static constexpr BlockParser blocks[] = {
            { "import", &YuiTextParser::parseImport },
            { "drawing", &YuiTextParser::parseDrawing },
            { "shader", &YuiTextParser::parseShader },
            { "palette", &YuiTextParser::parsePalette },
            { "style", &YuiTextParser::parseStyle },
            { "tokens", &YuiTextParser::parseTokens },
            { "component", &YuiTextParser::parseComponent }
        };

        DataTree document ("yui");
        auto transaction = document.beginTransaction();

        bool hasInstance = false;

        while (! isAtEnd())
        {
            const auto start = position;
            const auto keyword = parseIdentifier ("Expected a block or a component");
            DataTree block;

            for (const auto& entry : blocks)
            {
                if (keyword == entry.keyword)
                {
                    block = (this->*entry.parse)();
                    break;
                }
            }

            if (! block.isValid())
            {
                if (! detail::isYuiTypeName (keyword))
                {
                    StringArray expected;
                    for (const auto& entry : blocks)
                        expected.add ("'" + String (entry.keyword) + "'");

                    throw Error { start, "Unknown block '" + keyword + "', expected " + expected.joinIntoString (", ") + " or a component instance" };
                }

                if (hasInstance)
                    throw Error { start, "A document can only have one root component" };

                hasInstance = true;
                block = DataTree { Identifier (keyword) };
                parseBody (block);
            }

            transaction.addChild (mark (block, start));
        }

        transaction.commit();
        return document;
    }

    DataTreeDiagnostic locate (const Error& error) const
    {
        const auto nextLine = std::upper_bound (lineStarts.begin(), lineStarts.end(), error.offset);
        int column = 1;

        // Columns count characters, so the continuation bytes of UTF-8 sequences are skipped
        for (auto i = *(nextLine - 1); i < error.offset; ++i)
            column += (static_cast<uint8> (text[i]) & 0xc0) != 0x80 ? 1 : 0;

        return { .message = error.message, .source = sourceName, .line = static_cast<int> (nextLine - lineStarts.begin()), .column = column };
    }

    /** Returns where each element and property was written, as diagnostics without a message. */
    const std::vector<DataTreeDiagnostic>& getPositions() const noexcept { return positions; }

private:
    DataTree mark (const DataTree& element, int offset, const Identifier& property = {})
    {
        auto located = locate ({ offset, {} });
        located.element = element;
        located.property = property;
        positions.push_back (std::move (located));
        return element;
    }

    bool nextMember()
    {
        if (match ("}"))
            return false;

        if (isAtEnd())
            throw Error { position, "Expected '}'" };

        return true;
    }

    DataTree parseImport()
    {
        skipWhitespace();

        const auto start = position;
        const auto path = unquoteYuiText (parseRawValue()).trim();

        if (path.isEmpty())
            throw Error { start, "Expected the path of the import" };

        expect (";");
        return DataTree ("import", { { "path", path } });
    }

    DataTree parseDrawing()
    {
        DataTree drawing = lookingAt ("{")
                             ? DataTree ("drawing")
                             : DataTree ("drawing", { { "name", parseIdentifier ("Expected the name of the drawing") } });

        // The elements of a drawing can be named blocks, see parseBody
        const ScopedValueSetter<bool> scope (insideDrawing, true);

        parseBody (drawing);
        return drawing;
    }

    // After `drawing`, `pattern`, `marker`, `use` or `part` inside a drawing: the name it defines or refers to, then its body
    DataTree parseNamedDrawingBlock (const String& type)
    {
        const auto name = parseIdentifier ("Expected a name after '" + type + "'");

        DataTree block (Identifier (type), { { type == "use" ? Identifier ("drawing") : Identifier ("name"), name } });
        parseBody (block);
        return block;
    }

    static bool isNamedDrawingBlock (const String& name)
    {
        return name == "drawing" || name == "pattern" || name == "marker" || name == "use" || name == "part";
    }

    DataTree parsePalette()
    {
        DataTree palette ("palette");
        auto transaction = palette.beginTransaction();

        transaction.setProperty ("name", parseIdentifier ("Expected the name of the palette"));
        expect ("{");

        StringArray properties;

        while (nextMember())
        {
            const auto start = position;
            const auto name = parseIdentifier ("Expected a palette property");

            if (properties.contains (name))
                throw Error { start, "The property '" + name + "' is already set" };

            expect (":");
            properties.add (name);
            transaction.setProperty (name, unquoteYuiText (parseRawValue()));
            mark (palette, start, name);
            expect (";");
        }

        if (! properties.contains ("colors"))
            throw Error { position - 1, "The palette needs 'colors'" };

        transaction.commit();
        return palette;
    }

    DataTree parseShader()
    {
        DataTree shader ("shader");
        auto transaction = shader.beginTransaction();

        transaction.setProperty ("name", parseIdentifier ("Expected the name of the shader"));
        expect ("{");

        // The name is the one written after `shader`, which a member can't set again
        StringArray properties { "name" };
        StringArray declared;

        while (nextMember())
        {
            const auto start = position;
            const auto name = parseIdentifier ("Expected a shader property");

            if (name == "property" && ! lookingAt (":"))
            {
                transaction.addChild (parsePropertyDeclaration (declared, false));
                continue;
            }

            if (properties.contains (name))
                throw Error { start, "The property '" + name + "' is already set" };

            expect (":");
            properties.add (name);
            transaction.setProperty (name, unquoteYuiText (parseRawValue()));
            mark (shader, start, name);
            expect (";");
        }

        transaction.commit();
        return shader;
    }

    DataTree parseStyle()
    {
        DataTree style ("style");
        auto transaction = style.beginTransaction();

        expect ("{");

        while (nextMember())
        {
            if (match ("@"))
            {
                const auto start = position;

                if (parseIdentifier ("Expected 'keyframes'") != "keyframes")
                    throw Error { start, "Expected 'keyframes'" };

                transaction.addChild (mark (parseKeyframes(), start));
                continue;
            }

            const auto start = position;
            const auto selector = parseRawUntil ('{');

            if (selector.isEmpty())
                throw Error { start, "Expected a selector" };

            DataTree rule ("rule", { { "selector", selector } });
            parseDeclarations (rule, true);
            transaction.addChild (mark (rule, start));
        }

        transaction.commit();
        return style;
    }

    DataTree parseTokens()
    {
        DataTree tokens ("tokens");
        auto transaction = tokens.beginTransaction();
        StringArray declared;

        expect ("{");

        while (nextMember())
        {
            expectKeyword ("property");
            transaction.addChild (parsePropertyDeclaration (declared, false));
        }

        transaction.commit();
        return tokens;
    }

    DataTree parseComponent()
    {
        DataTree component ("component");
        auto transaction = component.beginTransaction();

        transaction.setProperty ("name", parseTypeName ("Expected the name of the component"));
        expect (":");
        skipWhitespace();

        const auto baseStart = position;
        const auto base = parseTypeName ("Expected the base type of the component");
        transaction.setProperty ("base", base);

        // The body configures an instance of the base type, the declarations belong to the component
        DataTree instance { Identifier (base) };
        parseBody (instance, &transaction);
        transaction.addChild (mark (instance, baseStart));

        transaction.commit();
        return component;
    }

    // After `property`: either `alias name <=> target;` or a property declaration
    DataTree parseMemberDeclaration (StringArray& declared)
    {
        skipWhitespace();

        const auto start = position;

        if (parseIdentifier ("Expected the name of the property") == "alias" && ! lookingAt (":"))
        {
            const auto name = parseDeclaredName (declared, "Expected the name of the alias");
            expect ("<=>");
            const auto target = parsePath ("Expected the property the alias stands for");
            expect (";");
            return mark (DataTree ("alias", { { "name", name }, { "target", target } }), start);
        }

        position = start;
        return parsePropertyDeclaration (declared, false);
    }

    // After `property`: `name: type = value;`, the value being optional
    DataTree parsePropertyDeclaration (StringArray& declared, bool verbatimValue)
    {
        skipWhitespace();

        DataTree property = mark (DataTree ("property"), position);
        auto transaction = property.beginTransaction();

        transaction.setProperty ("name", parseDeclaredName (declared, "Expected the name of the property"));
        expect (":");
        transaction.setProperty ("type", parseType());

        if (match ("="))
        {
            skipWhitespace();

            const auto start = position;
            const auto value = verbatimValue ? parseRawValue() : parseExpressionText();

            if (value.isEmpty())
                throw Error { start, "Expected the default value of the property" };

            transaction.setProperty ("value", value);
        }

        expect (";");

        transaction.commit();
        return property;
    }

    // After `signal`: `name;` or `name (argument: type, ...);`
    DataTree parseSignal (StringArray& declared)
    {
        skipWhitespace();

        DataTree signal = mark (DataTree ("signal"), position);
        auto transaction = signal.beginTransaction();

        transaction.setProperty ("name", parseDeclaredName (declared, "Expected the name of the signal"));

        if (match ("(") && ! match (")"))
        {
            StringArray arguments;

            do
            {
                const auto argName = parseDeclaredName (arguments, "Expected the name of the argument");
                expect (":");
                transaction.addChild (DataTree ("argument", { { "name", argName }, { "type", parseType() } }));
            } while (match (","));

            expect (")");
        }

        expect (";");

        transaction.commit();
        return signal;
    }

    // After `name <=>`: the target of a two-way binding
    DataTree parseTwoWayBinding (const String& name)
    {
        auto target = parsePath ("Expected the property to bind to");

        // An element of a list, at an index that is an expression
        if (match ("["))
        {
            target << "[" << parseExpressionText() << "]";
            expect ("]");
        }

        expect (";");
        return DataTree ("bind", { { "property", name }, { "target", target } });
    }

    String parseDeclaredName (StringArray& declared, const String& error)
    {
        skipWhitespace();

        const auto start = position;
        const auto name = parseIdentifier (error);

        if (declared.contains (name))
            throw Error { start, "'" + name + "' is already declared" };

        declared.add (name);
        return name;
    }

    String parseType()
    {
        const auto& types = ComponentProperty::getDeclaredTypeNames();

        skipWhitespace();

        const auto start = position;
        const auto type = parseIdentifier ("Expected a type");

        if (! types.contains (type))
            throw Error { start, "Unknown type '" + type + "', expected one of " + types.joinIntoString (", ") };

        if (! match ("["))
            return type;

        expect ("]");

        if (ComponentProperty::getElementTypeFromName (type + "[]") == ComponentProperty::Type::any)
            throw Error { start, "'" + type + "' has no array type" };

        return type + "[]";
    }

    String parseTypeName (const String& error)
    {
        skipWhitespace();

        const auto start = position;
        const auto name = parseIdentifier (error);

        if (! detail::isYuiTypeName (name))
            throw Error { start, "Type names start with an upper case letter" };

        return name;
    }

    // A dotted property path such as `knob.value`
    String parsePath (const String& error)
    {
        auto path = parseIdentifier (error);

        while (match ("."))
            path << "." << parseIdentifier ("Expected a name after '.'");

        return path;
    }

    DataTree parseKeyframes()
    {
        DataTree keyframes ("keyframes");
        auto transaction = keyframes.beginTransaction();

        transaction.setProperty ("name", parseIdentifier ("Expected the name of the keyframes"));
        expect ("{");

        while (nextMember())
        {
            const auto start = position;
            const auto label = parseRawUntil ('{');
            const auto percentage = label.endsWithChar ('%') ? label.dropLastCharacters (1).trim() : String();
            double at = 0.0;

            // A percentage is digits, around one point at most
            if (label == "from")
                at = 0.0;
            else if (label == "to")
                at = 1.0;
            else if (percentage.containsAnyOf ("0123456789") && percentage.containsOnly ("0123456789.") && percentage.indexOfChar ('.') == percentage.lastIndexOfChar ('.'))
                at = percentage.getDoubleValue() / 100.0;
            else
                throw Error { start, "Expected 'from', 'to' or a percentage" };

            if (at < 0.0 || at > 1.0)
                throw Error { start, "The keyframe position must be between 0% and 100%" };

            DataTree frame ("keyframe", { { "at", at } });
            parseDeclarations (frame, false);
            transaction.addChild (mark (frame, start));
        }

        transaction.commit();
        return keyframes;
    }

    void parseDeclarations (DataTree& element, bool allowPropertyDeclarations)
    {
        expect ("{");

        auto transaction = element.beginTransaction();
        StringArray declared;

        while (nextMember())
        {
            const auto start = position;
            auto name = parseIdentifier ("Expected a declaration");

            if (allowPropertyDeclarations && name == "property" && ! lookingAt (":"))
            {
                transaction.addChild (parsePropertyDeclaration (declared, true));
                continue;
            }

            // Qualified names like `Slider.thumbColor` cascade to the descendants of that type
            if (match ("."))
                name << "." << parseIdentifier ("Expected a property name after '.'");

            expect (":");

            const auto value = parseRawValue();

            if (value.isEmpty())
                throw Error { start, "The declaration '" + name + "' needs a value" };

            expect (";");
            transaction.addChild (mark (DataTree ("declaration", { { "name", name }, { "value", value } }), start));
        }
    }

    // Style values are kept verbatim up to the closing ';', since `transition:` or `drawing:` aren't expressions
    String parseRawValue()
    {
        return parseRawUntil (';');
    }

    String parseRawUntil (char terminator)
    {
        skipWhitespace();

        const auto start = position;
        std::string out;
        int chunkStart = position;
        int depth = 0;

        for (;;)
        {
            const auto c = text[position];

            if (c == 0)
                throw Error { position, "Expected '" + String::charToString (terminator) + "'" };

            if (c == '"' || c == '\'')
            {
                for (++position; text[position] != c; ++position)
                {
                    if (text[position] == 0 || text[position] == '\n')
                        throw Error { start, "Unterminated string" };

                    if (text[position] == '\\' && text[position + 1] != 0)
                        ++position;
                }

                ++position;
                continue;
            }

            if (c == '/' && text[position + 1] == '/')
            {
                out.append (text + chunkStart, static_cast<size_t> (position - chunkStart));
                position += 2;

                while (text[position] != 0 && text[position] != '\n')
                    ++position;

                chunkStart = position;
                continue;
            }

            if (c == '/' && text[position + 1] == '*')
            {
                out.append (text + chunkStart, static_cast<size_t> (position - chunkStart));

                const auto* end = std::strstr (text + position + 2, "*/");
                if (end == nullptr)
                    throw Error { position, "Unterminated comment" };

                position = static_cast<int> (end - text) + 2;
                const auto next = text[position];
                const bool prevIsSpace = out.empty() || out.back() == ' ' || out.back() == '\t' || out.back() == '\n' || out.back() == '\r';
                const bool nextIsSpace = next == 0 || next == ' ' || next == '\t' || next == '\n' || next == '\r' || next == terminator;
                const bool selectorAttachment = terminator == '{' && (next == '.' || next == '#' || next == ':' || next == '[');

                if (! prevIsSpace && ! nextIsSpace && ! selectorAttachment)
                    out.push_back (' ');

                chunkStart = position;
                continue;
            }

            if (depth == 0 && c == terminator)
                break;

            if (c == '(' || c == '[')
                ++depth;
            else if ((c == ')' || c == ']') && depth > 0)
                --depth;
            else if (depth == 0 && (c == '{' || c == '}' || c == ';'))
                throw Error { position, "Expected '" + String::charToString (terminator) + "'" };

            ++position;
        }

        out.append (text + chunkStart, static_cast<size_t> (position - chunkStart));
        return String::fromUTF8 (out.data(), static_cast<int> (out.size())).trim();
    }

    // The declarations of a component (properties, aliases and signals) go to its own transaction
    void parseBody (DataTree& element, DataTree::Transaction* declarations = nullptr)
    {
        const ScopedValueSetter<int> nesting (nestingDepth, nestingDepth + 1);

        if (nestingDepth > detail::maxYuiNestingDepth)
            throw Error { position, "Elements are nested more than " + String (detail::maxYuiNestingDepth) + " deep" };

        expect ("{");

        auto transaction = element.beginTransaction();
        StringArray properties;
        StringArray declared;
        StringArray watched;

        while (nextMember())
        {
            const auto start = position;
            const auto name = parseIdentifier ("Expected a property or an element");

            if (declarations != nullptr && name == "property" && ! lookingAt (":"))
            {
                declarations->addChild (parseMemberDeclaration (declared));
            }
            else if (declarations == nullptr && element.getType() == Identifier ("drawing") && name == "property" && ! lookingAt (":"))
            {
                transaction.addChild (parsePropertyDeclaration (declared, false));
            }
            else if (declarations != nullptr && name == "signal" && ! lookingAt (":"))
            {
                declarations->addChild (parseSignal (declared));
            }
            else if (name == "let")
            {
                transaction.addChild (mark (parseLet(), start));
            }
            else if (name == "repeat")
            {
                transaction.addChild (mark (parseRepeat(), start));
            }
            else if (insideDrawing && isNamedDrawingBlock (name) && lookingAtIdentifier())
            {
                transaction.addChild (mark (parseNamedDrawingBlock (name), start));
            }
            else if (lookingAt ("{"))
            {
                DataTree child { Identifier (name) };
                parseBody (child);
                transaction.addChild (mark (child, start));
            }
            else if (name == "changed" && isIdentifierStart (text[position]))
            {
                transaction.addChild (mark (parseChangedHandler (watched), start));
            }
            else
            {
                if (properties.contains (name))
                    throw Error { start, "The property '" + name + "' is already set" };

                if (match ("<=>"))
                {
                    properties.add (name);
                    transaction.addChild (mark (parseTwoWayBinding (name), start));
                    continue;
                }

                expect (":");
                properties.add (name);
                transaction.setProperty (name, parseValueText());
                mark (element, start, name);
            }
        }
    }

    // After `changed`: `property: handler;`, run when the property changes
    DataTree parseChangedHandler (StringArray& watched)
    {
        const auto start = position;
        const auto property = parseIdentifier ("Expected the name of the property");

        if (watched.contains (property))
            throw Error { start, "A changed handler for '" + property + "' is already set" };

        watched.add (property);
        expect (":");
        return DataTree ("changed", { { "property", property }, { "handler", parseValueText() } });
    }

    // A value, or a handler: optional argument names `(a, b) =>`, then a call or a block of calls `{ a (); b (); }`
    String parseValueText()
    {
        skipWhitespace();

        const auto start = position;
        const auto hasArgumentNames = text[position] == '(' && skipArgumentNames();

        if (! hasArgumentNames)
            position = start;

        skipWhitespace();

        if (text[position] != '{')
        {
            const auto value = parseExpressionText();
            const auto result = hasArgumentNames ? String::fromUTF8 (text + start, position - start).trimEnd() : value;
            expect (";");
            return result;
        }

        ++position;

        while (nextMember())
        {
            parseExpressionText();
            expect (";");
        }

        const auto result = String::fromUTF8 (text + start, position - start);
        match (";");
        return result;
    }

    // Skips `(name, ...) =>`, returning false when the text isn't that
    bool skipArgumentNames()
    {
        ++position;
        skipWhitespace();

        if (text[position] != ')')
        {
            for (;;)
            {
                skipWhitespace();

                if (! isIdentifierStart (text[position]))
                    return false;

                parseIdentifier ({});
                skipWhitespace();

                if (text[position] != ',')
                    break;

                ++position;
            }
        }

        if (text[position] != ')')
            return false;

        ++position;
        return match ("=>");
    }

    DataTree parseLet()
    {
        const auto name = parseIdentifier ("Expected the name of the let");
        expect (":");
        const auto value = parseExpressionText();
        expect (";");
        return DataTree ("let", { { "name", name }, { "value", value } });
    }

    DataTree parseRepeat()
    {
        const auto name = parseIdentifier ("Expected the name of the repeat index");
        expectKeyword ("in");
        const auto from = parseExpressionText();
        expect ("..");
        const auto to = parseExpressionText();

        DataTree repeat ("repeat", { { "name", name }, { "from", from }, { "to", to } });
        parseBody (repeat);
        return repeat;
    }

    // Values are kept as written, the expression parser only finds where they end. A value starting
    // with '[' is an array literal, whose elements are expressions of their own, unless it is read at
    // an index, which makes it part of an expression.
    String parseExpressionText()
    {
        skipWhitespace();

        const auto start = position;

        if (text[position] == '[')
        {
            StringArray elements;
            int offset = 0;

            if (auto result = PropertyExpression::splitArrayLiteral (StringRef (CharPointer_UTF8 (text + start)), elements, offset); result.failed())
                throw Error { start + offset, result.getErrorMessage() };

            position = start + offset;
            skipWhitespace();

            if (text[position] != '[')
            {
                position = start + offset;
                return String::fromUTF8 (text + start, offset);
            }
        }

        // The expression parser counts lines from the start of what it gets, so it only gets the rest of the text
        int length = 0;
        const auto parsed = Expression::parseExpression (StringRef (CharPointer_UTF8 (text + start)), length);

        if (parsed.failed())
            throw Error { start + length, parsed.getErrorMessage().fromFirstOccurrenceOf (": ", false, false) };

        position = start + length;
        return String::fromUTF8 (text + start, length).trimEnd();
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

    bool lookingAtIdentifier()
    {
        skipWhitespace();
        return isIdentifierStart (text[position]);
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

    const String source;
    const String sourceName;
    const char* text;
    std::vector<int> lineStarts;
    std::vector<DataTreeDiagnostic> positions;
    int position = 0;
    int nestingDepth = 0;
    bool insideDrawing = false;
};

//==============================================================================
struct ParsedText
{
    YuiParser::ParseResult result;
    std::vector<DataTreeDiagnostic> positions;
};

ParsedText parseText (const String& text, const String& sourceName)
{
    YuiTextParser parser (text, sourceName);

    try
    {
        auto document = parser.parseDocument();
        return { { .document = document }, parser.getPositions() };
    }
    catch (const YuiTextParser::Error& error)
    {
        return { { .diagnostics = { parser.locate (error) } }, {} };
    }
}

// Only the positions of elements still in the document are kept, the imports are dropped by the merge
DataTreeSourceMap makeSourceMap (const std::vector<DataTreeDiagnostic>& positions, const DataTree& document)
{
    DataTreeSourceMap sourceMap;

    for (const auto& position : positions)
    {
        if (position.element.getRoot() == document)
            sourceMap.add (position.element, position.property, position.line, position.column, position.source);
    }

    return sourceMap;
}

// Where an element was written, as a diagnostic without a message, which has no position for an element that was not marked
DataTreeDiagnostic findPosition (const std::vector<DataTreeDiagnostic>& positions, const DataTree& element)
{
    const auto found = std::find_if (positions.begin(), positions.end(), [&element] (const DataTreeDiagnostic& position)
    {
        return position.element == element && ! position.property.isValid();
    });

    if (found == positions.end())
        return {};

    return { .source = found->source, .line = found->line, .column = found->column };
}

} // namespace

//==============================================================================
YuiParser::ParseResult YuiParser::parse (const String& text)
{
    auto parsed = parseText (text, {});
    parsed.result.sourceMap = makeSourceMap (parsed.positions, parsed.result.document);
    return std::move (parsed.result);
}

YuiParser::ParseResult YuiParser::parse (const String& text, const ImportResolver& resolveImport)
{
    return parse (text, resolveImport, {});
}

YuiParser::ParseResult YuiParser::parse (const String& text, const ImportResolver& resolveImport, const String& mainPath)
{
    auto parsed = parseText (text, {});

    if (! parsed.result.wasOk())
        return std::move (parsed.result);

    StringArray imported;
    std::vector<DataTree> elements;
    auto positions = std::move (parsed.positions);

    // The text itself is a file imported already, for the ones importing it back
    if (const auto ownPath = detail::normalizeYuiPath (mainPath); ownPath.isNotEmpty())
        imported.add (ownPath);

    std::function<std::optional<DataTreeDiagnostic> (const DataTree&, const String&)> collect = [&] (const DataTree& document, const String& currentSource) -> std::optional<DataTreeDiagnostic>
    {
        for (const auto& element : document)
        {
            if (element.getType().toString() != "import")
            {
                elements.push_back (element);
                continue;
            }

            const auto rawPath = element.getProperty ("path").toString();
            const auto path = detail::resolveRelativeYuiPath (currentSource, rawPath);

            // Each file is imported once, which also breaks import cycles
            if (imported.contains (path))
                continue;

            imported.add (path);

            const auto importedText = resolveImport != nullptr ? resolveImport (path) : std::nullopt;

            if (! importedText.has_value())
            {
                // Every block is marked, so the import has a position
                auto diagnostic = findPosition (positions, element);
                jassert (diagnostic.line > 0);

                diagnostic.message = "Can't import '" + path + "'";
                return diagnostic;
            }

            auto importedResult = parseText (*importedText, path);

            if (! importedResult.result.wasOk())
                return importedResult.result.diagnostics.front();

            positions.insert (positions.end(), importedResult.positions.begin(), importedResult.positions.end());

            if (auto diagnostic = collect (importedResult.result.document, path))
                return diagnostic;
        }

        return std::nullopt;
    };

    if (auto diagnostic = collect (parsed.result.document, {}))
        return { .diagnostics = { *diagnostic } };

    // Each text was checked to have one root as it was parsed, which leaves the roots of different texts
    const auto isRoot = [] (const DataTree& element)
    {
        return detail::isYuiComponentElement (element);
    };

    const auto firstRoot = std::find_if (elements.begin(), elements.end(), isRoot);
    const auto secondRoot = firstRoot != elements.end() ? std::find_if (firstRoot + 1, elements.end(), isRoot) : elements.end();

    if (secondRoot != elements.end())
    {
        const auto firstSource = findPosition (positions, *firstRoot).source;

        auto diagnostic = findPosition (positions, *secondRoot);
        diagnostic.message = "A document can only have one root component: "
                           + (firstSource.isNotEmpty() ? "'" + firstSource + "'" : String ("the main text"))
                           + " has one already";

        return { .diagnostics = { diagnostic } };
    }

    // The elements move to the merged document, keeping the identities their positions refer to
    DataTree merged ("yui");
    auto transaction = merged.beginTransaction();

    for (const auto& element : elements)
        transaction.addChild (element);

    transaction.commit();
    return { .document = merged, .sourceMap = makeSourceMap (positions, merged) };
}

ResultValue<StyleSheet::Ptr> YuiParser::parseStyleSheet (const String& text)
{
    const auto parsed = parse (text);

    if (! parsed.wasOk())
        return makeResultValueFail (parsed.diagnostics.front().toString());

    return StyleSheet::fromTree (parsed.document);
}

} // namespace yup
