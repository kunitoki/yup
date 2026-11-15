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

String normalizeYuiImportPath (const String& rawPath)
{
    const auto normalizedSeparators = rawPath.trim().replaceCharacter ('\\', '/');
    if (normalizedSeparators.isEmpty())
        return {};

    StringArray segments;

    for (const auto& token : StringArray::fromTokens (normalizedSeparators, "/", ""))
    {
        if (token.isEmpty() || token == ".")
            continue;

        if (token == ".." && ! segments.isEmpty() && segments[segments.size() - 1] != "..")
        {
            segments.remove (segments.size() - 1);
            continue;
        }

        segments.add (token);
    }

    return segments.joinIntoString ("/");
}

String resolveRelativeYuiImportPath (const String& importerPath, const String& importedPath)
{
    const auto cleanedImport = importedPath.trim().replaceCharacter ('\\', '/');
    if (cleanedImport.startsWithChar ('/'))
        return normalizeYuiImportPath (cleanedImport.substring (1));

    const auto normalizedImporter = normalizeYuiImportPath (importerPath);
    const auto slash = normalizedImporter.lastIndexOfChar ('/');

    if (slash < 0)
        return normalizeYuiImportPath (cleanedImport);

    return normalizeYuiImportPath (normalizedImporter.substring (0, slash + 1) + cleanedImport);
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
        DataTree document ("yui");
        auto transaction = document.beginTransaction();

        bool hasInstance = false;

        while (! isAtEnd())
        {
            const auto start = position;
            const auto keyword = parseIdentifier ("Expected a block or a component");
            DataTree block;

            if (keyword == "import")
            {
                block = parseImport();
            }
            else if (keyword == "drawing")
            {
                block = parseDrawing();
            }
            else if (keyword == "palette")
            {
                block = parsePalette();
            }
            else if (keyword == "style")
            {
                block = parseStyle();
            }
            else if (keyword == "tokens")
            {
                block = parseTokens();
            }
            else if (keyword == "component")
            {
                block = parseComponent();
            }
            else if (isTypeName (keyword))
            {
                if (hasInstance)
                    throw Error { start, "A document can only have one root component" };

                hasInstance = true;

                block = DataTree { Identifier (keyword) };
                parseBody (block);
            }
            else
            {
                throw Error { start, "Unknown block '" + keyword + "', expected 'import', 'drawing', 'palette', 'style', 'tokens', 'component' or a component instance" };
            }

            transaction.addChild (mark (block, start));
        }

        transaction.commit();
        return document;
    }

    YuiDiagnostic locate (const Error& error) const
    {
        const auto nextLine = std::upper_bound (lineStarts.begin(), lineStarts.end(), error.offset);
        int column = 1;

        // Columns count characters, so the continuation bytes of UTF-8 sequences are skipped
        for (auto i = *(nextLine - 1); i < error.offset; ++i)
            column += (static_cast<uint8> (text[i]) & 0xc0) != 0x80 ? 1 : 0;

        return { .message = error.message, .source = sourceName, .line = static_cast<int> (nextLine - lineStarts.begin()), .column = column };
    }

    /** Returns where each element and property was written, as diagnostics without a message. */
    const std::vector<YuiDiagnostic>& getPositions() const noexcept { return positions; }

private:
    DataTree mark (const DataTree& element, int offset, const Identifier& property = {})
    {
        auto located = locate ({ offset, {} });
        located.element = element;
        located.property = property;
        positions.push_back (std::move (located));
        return element;
    }

    DataTree parseImport()
    {
        skipWhitespace();

        const auto start = position;
        const auto path = unquoteYuiText (parseRawValue()).trim();

        if (path.isEmpty())
            throw Error { start, "Expected the path of the import" };

        expect (";");

        DataTree import ("import");
        import.beginTransaction().setProperty ("path", path);
        return import;
    }

    DataTree parseDrawing()
    {
        DataTree drawing ("drawing");

        if (! lookingAt ("{"))
            drawing.beginTransaction().setProperty ("name", parseIdentifier ("Expected the name of the drawing"));

        parseBody (drawing);
        return drawing;
    }

    DataTree parsePalette()
    {
        DataTree palette ("palette");
        auto transaction = palette.beginTransaction();

        transaction.setProperty ("name", parseIdentifier ("Expected the name of the palette"));
        expect ("{");

        StringArray properties;

        while (! match ("}"))
        {
            if (isAtEnd())
                throw Error { position, "Expected '}'" };

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

    DataTree parseStyle()
    {
        DataTree style ("style");
        auto transaction = style.beginTransaction();

        expect ("{");

        while (! match ("}"))
        {
            if (isAtEnd())
                throw Error { position, "Expected '}'" };

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

            DataTree rule ("rule");
            rule.beginTransaction().setProperty ("selector", selector);
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

        while (! match ("}"))
        {
            if (isAtEnd())
                throw Error { position, "Expected '}'" };

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
            DataTree alias ("alias");
            auto transaction = alias.beginTransaction();

            transaction.setProperty ("name", parseDeclaredName (declared, "Expected the name of the alias"));
            expect ("<=>");
            transaction.setProperty ("target", parsePath ("Expected the property the alias stands for"));
            expect (";");

            transaction.commit();
            return mark (alias, start);
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
                DataTree argument ("argument");
                auto argumentTransaction = argument.beginTransaction();

                argumentTransaction.setProperty ("name", parseDeclaredName (arguments, "Expected the name of the argument"));
                expect (":");
                argumentTransaction.setProperty ("type", parseType());
                argumentTransaction.commit();

                transaction.addChild (argument);
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
        DataTree binding ("bind");
        auto transaction = binding.beginTransaction();

        transaction.setProperty ("property", name);
        transaction.setProperty ("target", parsePath ("Expected the property to bind to"));
        expect (";");

        transaction.commit();
        return binding;
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
        static const StringArray types { "number", "integer", "boolean", "string", "color", "point", "rectangle", "font", "drawing", "any" };

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

        if (! isTypeName (name))
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

        while (! match ("}"))
        {
            if (isAtEnd())
                throw Error { position, "Expected '}'" };

            const auto start = position;
            const auto label = parseRawUntil ('{');
            double at = 0.0;

            if (label == "from")
                at = 0.0;
            else if (label == "to")
                at = 1.0;
            else if (label.endsWithChar ('%') && label.dropLastCharacters (1).trim().containsOnly ("0123456789."))
                at = label.dropLastCharacters (1).getDoubleValue() / 100.0;
            else
                throw Error { start, "Expected 'from', 'to' or a percentage" };

            if (at < 0.0 || at > 1.0)
                throw Error { start, "The keyframe position must be between 0% and 100%" };

            DataTree frame ("keyframe");
            frame.beginTransaction().setProperty ("at", at);
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

        while (! match ("}"))
        {
            if (isAtEnd())
                throw Error { position, "Expected '}'" };

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

            DataTree declaration ("declaration");
            auto declarationTransaction = declaration.beginTransaction();
            declarationTransaction.setProperty ("name", name);
            declarationTransaction.setProperty ("value", value);
            declarationTransaction.commit();

            transaction.addChild (mark (declaration, start));
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

        return String::fromUTF8 (text + start, position - start).trim();
    }

    // The declarations of a component (properties, aliases and signals) go to its own transaction
    void parseBody (DataTree& element, DataTree::Transaction* declarations = nullptr)
    {
        expect ("{");

        auto transaction = element.beginTransaction();
        StringArray properties;
        StringArray declared;

        while (! match ("}"))
        {
            if (isAtEnd())
                throw Error { position, "Expected '}'" };

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
            else if (lookingAt ("{"))
            {
                DataTree child { Identifier (name) };
                parseBody (child);
                transaction.addChild (mark (child, start));
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
                transaction.setProperty (name, parseExpressionText());
                mark (element, start, name);
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

    // Values are kept as written, the expression parser only finds where they end. A value starting
    // with '[' is an array literal, whose elements are expressions of their own.
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
            return String::fromUTF8 (text + start, offset);
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

    // Block keywords are lower case, component types are capitalized like `Column`
    static bool isTypeName (const String& name)
    {
        return CharacterFunctions::isUpperCase (static_cast<yup_wchar> (*name.getCharPointer()));
    }

    const String source;
    const String sourceName;
    const char* text;
    std::vector<int> lineStarts;
    std::vector<YuiDiagnostic> positions;
    int position = 0;
};

//==============================================================================
struct ParsedText
{
    YuiParser::ParseResult result;
    std::vector<YuiDiagnostic> positions;
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
YuiSourceMap makeSourceMap (const std::vector<YuiDiagnostic>& positions, const DataTree& document)
{
    YuiSourceMap sourceMap;

    for (const auto& position : positions)
    {
        if (position.element.getRoot() == document)
            sourceMap.add (position.element, position.property, position.line, position.column, position.source);
    }

    return sourceMap;
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
    auto parsed = parseText (text, {});

    if (! parsed.result.wasOk())
        return std::move (parsed.result);

    StringArray imported;
    std::vector<DataTree> elements;
    auto positions = std::move (parsed.positions);

    std::function<std::optional<YuiDiagnostic> (const DataTree&, const String&)> collect = [&] (const DataTree& document, const String& currentSource) -> std::optional<YuiDiagnostic>
    {
        for (const auto& element : document)
        {
            if (element.getType().toString() != "import")
            {
                elements.push_back (element);
                continue;
            }

            const auto rawPath = element.getProperty ("path").toString();
            const auto path = resolveRelativeYuiImportPath (currentSource, rawPath);

            // Each file is imported once, which also breaks import cycles
            if (imported.contains (path))
                continue;

            imported.add (path);

            const auto importedText = resolveImport != nullptr ? resolveImport (path) : std::nullopt;

            if (! importedText.has_value())
            {
                // Every block is marked, so the import has a position
                const auto importPosition = std::find_if (positions.begin(), positions.end(), [&] (const auto& position)
                {
                    return position.element == element;
                });

                jassert (importPosition != positions.end());
                return YuiDiagnostic { .message = "Can't import '" + path + "'", .source = importPosition->source, .line = importPosition->line, .column = importPosition->column };
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

    // The elements move to the merged document, keeping the identities their positions refer to
    DataTree merged ("yui");
    auto transaction = merged.beginTransaction();

    for (const auto& element : elements)
        transaction.addChild (element);

    transaction.commit();
    return { .document = merged, .sourceMap = makeSourceMap (positions, merged) };
}

} // namespace yup
