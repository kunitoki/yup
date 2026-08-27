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

//==============================================================================

StringRef toString (YdspPrimitiveType type) noexcept
{
    switch (type)
    {
        case YdspPrimitiveType::float32Type:
            return "float32";
        case YdspPrimitiveType::float64Type:
            return "float64";
        case YdspPrimitiveType::int32Type:
            return "int32";
        case YdspPrimitiveType::int64Type:
            return "int64";
        case YdspPrimitiveType::boolType:
            return "bool";
    }

    return "?";
}

//==============================================================================

namespace
{

String stripDigitSeparators (StringRef textRef)
{
    const String text (textRef);
    String result;

    for (int i = 0; i < text.length(); ++i)
        if (text[i] != '_')
            result += text[i];

    return result;
}

long long parseIntLiteralText (StringRef textRef)
{
    const auto text = stripDigitSeparators (textRef);

    if (text.startsWithIgnoreCase ("0x"))
        return static_cast<long long> (text.getHexValue64());

    if (text.startsWithIgnoreCase ("0b"))
    {
        long long value = 0;

        for (int i = 2; i < text.length(); ++i)
            value = value * 2 + (text[i] == '1' ? 1 : 0);

        return value;
    }

    return text.getLargeIntValue();
}

double parseFloatLiteralText (StringRef textRef)
{
    return stripDigitSeparators (textRef).getDoubleValue();
}

String getDottedPath (const YdspExpr& expr)
{
    if (expr.kind == YdspExprKind::identifier)
        return expr.text;

    if (expr.kind == YdspExprKind::member && ! expr.children.empty())
    {
        const auto base = getDottedPath (*expr.children[0]);

        if (base.isNotEmpty())
            return base + "." + expr.text;
    }

    return {};
}

} // namespace

//==============================================================================

std::optional<YdspPrimitiveType> YdspParser::parsePrimitiveType (const YdspToken& token) const noexcept
{
    if (token.type != YdspTokenType::identifier)
        return std::nullopt;

    if (token.text == "float" || token.text == "float32")
        return YdspPrimitiveType::float32Type;

    if (token.text == "float64")
        return YdspPrimitiveType::float64Type;

    if (token.text == "int" || token.text == "int32")
        return YdspPrimitiveType::int32Type;

    if (token.text == "int64")
        return YdspPrimitiveType::int64Type;

    if (token.text == "bool")
        return YdspPrimitiveType::boolType;

    return std::nullopt;
}

//==============================================================================

namespace YdspExprFactory
{

YdspExprPtr makeInt (YdspLocation location, long long value)
{
    auto expr = std::make_unique<YdspExpr>();
    expr->kind = YdspExprKind::intLiteral;
    expr->location = location;
    expr->number = static_cast<double> (value);
    return expr;
}

YdspExprPtr makeFloat (YdspLocation location, double value)
{
    auto expr = std::make_unique<YdspExpr>();
    expr->kind = YdspExprKind::floatLiteral;
    expr->location = location;
    expr->number = value;
    return expr;
}

YdspExprPtr makeBool (YdspLocation location, bool value)
{
    auto expr = std::make_unique<YdspExpr>();
    expr->kind = YdspExprKind::boolLiteral;
    expr->location = location;
    expr->flag = value;
    return expr;
}

YdspExprPtr makeIdentifier (YdspLocation location, String name)
{
    auto expr = std::make_unique<YdspExpr>();
    expr->kind = YdspExprKind::identifier;
    expr->location = location;
    expr->text = std::move (name);
    return expr;
}

YdspExprPtr makeBinary (YdspLocation location, YdspOperator op, YdspExprPtr lhs, YdspExprPtr rhs)
{
    auto expr = std::make_unique<YdspExpr>();
    expr->kind = YdspExprKind::binary;
    expr->location = location;
    expr->op = op;
    expr->children.push_back (std::move (lhs));
    expr->children.push_back (std::move (rhs));
    return expr;
}

YdspExprPtr makeUnary (YdspLocation location, YdspOperator op, YdspExprPtr operand)
{
    auto expr = std::make_unique<YdspExpr>();
    expr->kind = YdspExprKind::unary;
    expr->location = location;
    expr->op = op;
    expr->children.push_back (std::move (operand));
    return expr;
}

YdspExprPtr makeCall (YdspLocation location, String callee, std::vector<YdspExprPtr> args)
{
    auto expr = std::make_unique<YdspExpr>();
    expr->kind = YdspExprKind::call;
    expr->location = location;
    expr->text = std::move (callee);
    expr->children = std::move (args);
    return expr;
}

YdspExprPtr clone (const YdspExpr& expr)
{
    auto copy = std::make_unique<YdspExpr>();
    copy->kind = expr.kind;
    copy->location = expr.location;
    copy->text = expr.text;
    copy->number = expr.number;
    copy->flag = expr.flag;
    copy->op = expr.op;

    for (const auto& child : expr.children)
        copy->children.push_back (child != nullptr ? clone (*child) : nullptr);

    for (const auto& [name, value] : expr.overrides)
        copy->overrides.emplace_back (name, value != nullptr ? clone (*value) : nullptr);

    return copy;
}

} // namespace YdspExprFactory

//==============================================================================

YdspParser::YdspParser (std::vector<YdspToken> tokens, DspJitDiagnostics& diagnostics)
    : tokens (std::move (tokens))
    , diagnostics (diagnostics)
{
}

const YdspToken& YdspParser::current() const noexcept
{
    jassert (! tokens.empty());
    return tokens[std::min (index, tokens.size() - 1)];
}

const YdspToken& YdspParser::peekNext() const noexcept
{
    jassert (! tokens.empty());
    return tokens[std::min (index + 1, tokens.size() - 1)];
}

void YdspParser::advance() noexcept
{
    if (index < tokens.size() - 1)
        ++index;
}

bool YdspParser::at (YdspTokenType type) const noexcept
{
    return current().type == type;
}

bool YdspParser::atIdentifier (StringRef text) const noexcept
{
    return current().type == YdspTokenType::identifier && current().text == text;
}

bool YdspParser::match (YdspTokenType type)
{
    if (at (type))
    {
        advance();
        return true;
    }

    return false;
}

bool YdspParser::matchIdentifier (StringRef text)
{
    if (atIdentifier (text))
    {
        advance();
        return true;
    }

    return false;
}

const YdspToken& YdspParser::expect (YdspTokenType type, StringRef what)
{
    if (at (type))
    {
        const auto& token = tokens[index];

        if (index < tokens.size() - 1)
            ++index;

        return token;
    }

    errorCurrent (String ("Expected ") + what);

    synchronize();

    return tokens[index > 0 ? index - 1 : 0];
}

const YdspToken& YdspParser::expectMemberName (StringRef what)
{
    if (isKeywordToken (current().type))
    {
        const auto& token = tokens[index];

        if (index < tokens.size() - 1)
            ++index;

        return token;
    }

    return expectIdentifier (what);
}

const YdspToken& YdspParser::expectIdentifier (StringRef what)
{
    if (current().type == YdspTokenType::identifier)
    {
        const auto& token = tokens[index];

        if (index < tokens.size() - 1)
            ++index;

        return token;
    }

    errorCurrent (String ("Expected ") + what);

    synchronize();

    return tokens[index > 0 ? index - 1 : 0];
}

void YdspParser::error (const YdspToken& token, StringRef message)
{
    diagnostics.addError (token.line, token.column, message);
}

void YdspParser::errorCurrent (StringRef message)
{
    error (current(), message);
}

void YdspParser::synchronize()
{
    // Skip forward to a statement/declaration boundary to avoid cascading errors.
    while (! at (YdspTokenType::endOfFile) && ! at (YdspTokenType::semi) && ! at (YdspTokenType::rBrace))
        advance();

    if (at (YdspTokenType::semi) || at (YdspTokenType::rBrace))
        advance();
}

//==============================================================================

std::unique_ptr<YdspProgram> YdspParser::parseProgram()
{
    return parseProgramBody();
}

std::unique_ptr<YdspProgram> YdspParser::parseProgramBody()
{
    auto program = std::make_unique<YdspProgram>();

    while (! at (YdspTokenType::endOfFile))
    {
        if (match (YdspTokenType::kwDeclare))
        {
            parseDeclare (*program);
            continue;
        }

        if (match (YdspTokenType::kwImport))
        {
            parseImport (*program);
            continue;
        }

        if (match (YdspTokenType::kwLet))
        {
            parseProgramConstant (*program);
            continue;
        }

        if (match (YdspTokenType::kwFunc))
        {
            program->functions.push_back (parseFunction());
            continue;
        }

        if (match (YdspTokenType::kwProcessor))
        {
            program->processors.push_back (std::move (*parseProcessor()));
            continue;
        }

        if (match (YdspTokenType::kwGraph))
        {
            program->graphs.push_back (std::move (*parseGraph()));
            continue;
        }

        errorCurrent ("Expected 'declare', 'import', 'let', 'func', 'processor' or 'graph'");
        synchronize();
    }

    return program;
}

void YdspParser::parseDeclare (YdspProgram& program)
{
    const auto keyToken = expectIdentifier ("a metadata key after 'declare'");

    if (at (YdspTokenType::stringLiteral))
    {
        YdspDeclare declare;
        declare.key = keyToken.text;
        declare.value = current().text;
        declare.location = { keyToken.line, keyToken.column };

        advance();

        if (match (YdspTokenType::semi))
            program.declares.push_back (std::move (declare));
    }
    else
    {
        errorCurrent ("Expected a string literal metadata value after 'declare'");
        synchronize();
    }
}

void YdspParser::parseImport (YdspProgram& program)
{
    const auto& token = current();

    if (token.type == YdspTokenType::identifier)
    {
        YdspImportDecl importDecl;
        importDecl.path = token.text;
        importDecl.location = { token.line, token.column };

        advance();

        // Dotted module path: `import X.Y.Z` maps to the file `X/Y/Z.ydsp`.
        while (match (YdspTokenType::dot))
            importDecl.path += "." + expectIdentifier ("a module path segment after '.'").text;

        // Optional alias: `import X.Y.Z as W` forces access as `W.*`.
        if (matchIdentifier ("as"))
        {
            const auto& aliasToken = expectIdentifier ("an alias name after 'as'");
            importDecl.alias = aliasToken.text;
        }

        expect (YdspTokenType::semi, "';' after the import directive");

        program.imports.push_back (std::move (importDecl));
    }
    else
    {
        errorCurrent ("Expected a dotted module path after 'import' (e.g. 'import fx.Delay')");
        synchronize();
    }
}

void YdspParser::parseProgramConstant (YdspProgram& program)
{
    const auto& nameToken = expectIdentifier ("a constant name after 'let'");

    YdspLetDecl constant;
    constant.name = nameToken.text;
    constant.location = { nameToken.line, nameToken.column };

    expect (YdspTokenType::assign, "'=' after the constant name");

    constant.value = parseExpression();

    expect (YdspTokenType::semi, "';' after the constant declaration");

    program.constants.push_back (std::move (constant));
}

//==============================================================================

std::unique_ptr<YdspProcessorDecl> YdspParser::parseProcessor()
{
    auto processor = std::make_unique<YdspProcessorDecl>();

    const auto& nameToken = expectIdentifier ("a processor name");
    processor->name = nameToken.text;
    processor->location = { nameToken.line, nameToken.column };

    // Optional latency declaration: [[ latency: N ]]
    processor->annotations = parseAnnotations();

    expect (YdspTokenType::lBrace, "'{' after processor name");

    while (! at (YdspTokenType::rBrace) && ! at (YdspTokenType::endOfFile))
    {
        if (at (YdspTokenType::kwInput) || at (YdspTokenType::kwOutput))
        {
            for (auto& ep : parseEndpointWithKind())
                processor->endpoints.push_back (std::move (ep));
            continue;
        }

        if (match (YdspTokenType::kwState))
        {
            processor->states.push_back (parseState());
            continue;
        }

        if (match (YdspTokenType::kwStruct))
        {
            processor->structs.push_back (parseStruct());
            continue;
        }

        if (match (YdspTokenType::kwInit))
        {
            if (processor->init != nullptr)
            {
                errorCurrent ("A processor may only have one init block");
                synchronize();
                continue;
            }

            // Reuse the process-body parser; init runs once, before audio starts.
            auto initBody = parseProcess();
            initBody->mode = YdspProcessMode::block;
            processor->init = std::move (initBody);
            continue;
        }

        if (match (YdspTokenType::kwFunc))
        {
            processor->functions.push_back (parseFunction());
            continue;
        }

        if (match (YdspTokenType::kwEvent))
        {
            processor->eventHandlers.push_back (parseEventHandler());
            continue;
        }

        if (match (YdspTokenType::kwProcess))
        {
            if (processor->process != nullptr)
            {
                errorCurrent ("A processor may only have one process body");
                synchronize();
                continue;
            }

            processor->process = parseProcess();
            continue;
        }

        if (at (YdspTokenType::identifier))
        {
            // A struct-typed state declaration (`Voice mono;` / `Voice voices[4];`).
            processor->states.push_back (parseState());
            continue;
        }

        errorCurrent ("Expected an endpoint, 'state', 'func', 'event' or 'process' declaration");
        synchronize();
    }

    expect (YdspTokenType::rBrace, "'}' to close the processor");

    return processor;
}

std::vector<YdspEndpointDecl> YdspParser::parseEndpointWithKind()
{
    const auto kindTokenLoc = YdspLocation { current().line, current().column };
    const bool isInput = current().type == YdspTokenType::kwInput;

    advance(); // 'input' or 'output'

    if (at (YdspTokenType::kwEvent))
    {
        advance(); // 'event'

        const auto& shapeToken = expectIdentifier ("an event shape name after 'event'");

        YdspEndpointDecl endpoint;
        endpoint.kind = isInput ? YdspEndpointKind::inputEvent : YdspEndpointKind::outputEvent;
        endpoint.name = shapeToken.text;
        endpoint.location = { shapeToken.line, shapeToken.column };

        expect (YdspTokenType::semi, "';' after the event endpoint declaration");

        std::vector<YdspEndpointDecl> endpoints;
        endpoints.push_back (std::move (endpoint));
        return endpoints;
    }

    YdspEndpointKind kind = isInput ? YdspEndpointKind::inputStream : YdspEndpointKind::outputStream;

    if (at (YdspTokenType::kwValue))
    {
        kind = isInput ? YdspEndpointKind::inputValue : YdspEndpointKind::outputValue;
        advance();
    }
    else if (at (YdspTokenType::kwStream))
    {
        advance();
    }
    else
    {
        errorCurrent ("Expected 'stream', 'value' or 'event' after 'input'/'output'");
    }

    return parseEndpointList (kind, kindTokenLoc);
}

std::vector<YdspEndpointDecl> YdspParser::parseEndpointList (YdspEndpointKind kind, const YdspLocation& location)
{
    std::vector<YdspEndpointDecl> endpoints;

    std::optional<YdspPrimitiveType> sharedType;
    if (current().type == YdspTokenType::identifier)
    {
        sharedType = parsePrimitiveType (current());

        if (sharedType.has_value())
            advance();
    }

    for (;;)
    {
        endpoints.push_back (parseEndpoint (kind, location, sharedType));

        if (! match (YdspTokenType::comma))
            break;
    }

    expect (YdspTokenType::semi, "';' after endpoint declaration");

    return endpoints;
}

YdspEndpointDecl YdspParser::parseEndpoint (YdspEndpointKind kind, const YdspLocation& location, std::optional<YdspPrimitiveType> sharedType)
{
    YdspEndpointDecl endpoint;
    endpoint.kind = kind;
    endpoint.location = location;

    const bool isValueKind = kind == YdspEndpointKind::inputValue || kind == YdspEndpointKind::outputValue;

    if (sharedType.has_value())
    {
        endpoint.type = *sharedType;
    }
    else if (isValueKind)
    {
        const auto& typeToken = expectIdentifier ("a type (float, int, float64, int64 or bool)");
        const auto type = parsePrimitiveType (typeToken);

        if (type.has_value())
            endpoint.type = *type;
        else
            error (typeToken, "Unknown type '" + typeToken.text + "' (expected float, int, float64, int64 or bool)");
    }

    const auto& nameToken = expectIdentifier ("an endpoint name");
    endpoint.name = nameToken.text;

    if (match (YdspTokenType::lBracket))
    {
        const auto& countToken = expect (YdspTokenType::intLiteral, "a channel count");
        endpoint.channelCount = static_cast<int> (parseIntLiteralText (countToken.text));

        expect (YdspTokenType::rBracket, "']' after channel count");
    }

    if (kind == YdspEndpointKind::inputValue && match (YdspTokenType::assign))
        endpoint.defaultValue = parseExpression();

    endpoint.annotations = parseAnnotations();

    return endpoint;
}

YdspStateDecl YdspParser::parseState()
{
    YdspStateDecl state;

    const auto& typeToken = expectIdentifier ("a type (float, int, float64, int64, bool) or a struct name");
    const auto type = parsePrimitiveType (typeToken);

    if (type.has_value())
    {
        state.type = *type;
    }
    else
    {
        state.structName = typeToken.text;
    }

    const auto& nameToken = expectIdentifier ("a state name");
    state.name = nameToken.text;
    state.location = { nameToken.line, nameToken.column };

    if (match (YdspTokenType::lBracket))
    {
        if (at (YdspTokenType::identifier))
        {
            state.arraySizeName = current().text;
            advance();

            while (match (YdspTokenType::dot))
                state.arraySizeName += "." + expectIdentifier ("a constant name after '.'").text;
        }
        else
        {
            const auto& sizeToken = expect (YdspTokenType::intLiteral, "a constant array size");
            state.arraySize = static_cast<int> (parseIntLiteralText (sizeToken.text));
        }

        expect (YdspTokenType::rBracket, "']' after the array size");
    }

    if (match (YdspTokenType::assign))
    {
        if (match (YdspTokenType::lBrace))
        {
            if (! at (YdspTokenType::rBrace))
            {
                for (;;)
                {
                    state.initialisers.push_back (parseExpression());

                    if (! match (YdspTokenType::comma))
                        break;

                    if (at (YdspTokenType::rBrace))
                        break;
                }
            }

            expect (YdspTokenType::rBrace, "'}' to close the state initialiser list");
        }
        else
        {
            state.initialisers.push_back (parseExpression());
        }
    }

    state.annotations = parseAnnotations();

    expect (YdspTokenType::semi, "';' after state declaration");

    return state;
}

YdspStructDecl YdspParser::parseStruct()
{
    YdspStructDecl structDecl;

    const auto& nameToken = expectIdentifier ("a struct name");
    structDecl.name = nameToken.text;
    structDecl.location = { nameToken.line, nameToken.column };

    expect (YdspTokenType::lBrace, "'{' after the struct name");

    while (! at (YdspTokenType::rBrace) && ! at (YdspTokenType::endOfFile))
    {
        const auto& typeToken = expectIdentifier ("a field type (float, int, float64, int64 or bool)");
        const auto type = parsePrimitiveType (typeToken);

        YdspStructField field;
        field.location = { typeToken.line, typeToken.column };

        if (type.has_value())
        {
            field.type = *type;
        }
        else
        {
            error (typeToken, "Unknown type '" + typeToken.text + "' (expected float, int, float64, int64 or bool)");
        }

        const auto& fieldNameToken = expectIdentifier ("a field name");
        field.name = fieldNameToken.text;

        if (match (YdspTokenType::lBracket))
        {
            const auto& sizeToken = expect (YdspTokenType::intLiteral, "a constant field array size");
            field.arraySize = static_cast<int> (parseIntLiteralText (sizeToken.text));

            expect (YdspTokenType::rBracket, "']' after the field array size");
        }

        expect (YdspTokenType::semi, "';' after the field declaration");

        structDecl.fields.push_back (std::move (field));
    }

    expect (YdspTokenType::rBrace, "'}' to close the struct");

    return structDecl;
}

std::unique_ptr<YdspProcessDecl> YdspParser::parseProcess()
{
    auto process = std::make_unique<YdspProcessDecl>();

    const auto& processToken = tokens[std::min (index - 1, tokens.size() - 1)];
    process->location = { processToken.line, processToken.column };

    if (match (YdspTokenType::kwBlock))
        process->mode = YdspProcessMode::block;

    expect (YdspTokenType::lBrace, "'{' after 'process'");

    while (! at (YdspTokenType::rBrace) && ! at (YdspTokenType::endOfFile))
    {
        auto stmt = parseStatement();

        if (stmt != nullptr)
            process->body.push_back (std::move (stmt));
    }

    expect (YdspTokenType::rBrace, "'}' to close the process body");

    return process;
}

YdspEventHandlerDecl YdspParser::parseEventHandler()
{
    YdspEventHandlerDecl handler;

    const auto& inputToken = expectIdentifier ("an event input name after 'event'");
    handler.endpointName = inputToken.text;
    handler.location = { inputToken.line, inputToken.column };

    expect (YdspTokenType::lParen, "'(' after the event input name");

    const auto& paramToken = expectIdentifier ("an event parameter name");
    handler.paramName = paramToken.text;

    expect (YdspTokenType::colon, "':' after the event parameter name");

    const auto& shapeToken = expectIdentifier ("an event shape name after ':'");
    handler.shapeName = shapeToken.text;

    expect (YdspTokenType::rParen, "')' after the event shape name");

    expect (YdspTokenType::lBrace, "'{' to open the event handler body");

    while (! at (YdspTokenType::rBrace) && ! at (YdspTokenType::endOfFile))
    {
        auto stmt = parseStatement();

        if (stmt != nullptr)
            handler.body.push_back (std::move (stmt));
    }

    expect (YdspTokenType::rBrace, "'}' to close the event handler body");

    return handler;
}

std::vector<std::pair<String, String>> YdspParser::parseAnnotations()
{
    std::vector<std::pair<String, String>> annotations;

    if (! match (YdspTokenType::lAnnotation))
        return annotations;

    auto parseValue = [this]() -> String
    {
        if (current().type == YdspTokenType::identifier
            || current().type == YdspTokenType::stringLiteral
            || current().type == YdspTokenType::intLiteral
            || current().type == YdspTokenType::floatLiteral)
        {
            auto value = current().text;
            advance();
            return value;
        }

        if (current().type == YdspTokenType::minus || current().type == YdspTokenType::plus)
        {
            auto sign = current().text;
            advance();

            if (current().type == YdspTokenType::intLiteral || current().type == YdspTokenType::floatLiteral)
            {
                auto value = sign + current().text;
                advance();
                return value;
            }
        }

        errorCurrent ("Expected an annotation value");
        return {};
    };

    auto parseListValue = [this, &parseValue]() -> String
    {
        expect (YdspTokenType::lBrace, "'{' to open the annotation list value");

        String joined;

        for (;;)
        {
            const auto entry = parseValue();

            if (entry.isNotEmpty())
                joined += (joined.isNotEmpty() ? "," : "") + entry;

            if (! match (YdspTokenType::comma))
                break;
        }

        expect (YdspTokenType::rBrace, "'}' to close the annotation list value");

        return joined;
    };

    String key, value;

    for (;;)
    {
        key.clear();

        if (current().type == YdspTokenType::identifier
            || current().type == YdspTokenType::stringLiteral
            || isKeywordToken (current().type))
        {
            key = current().text;
            advance();
        }
        else
        {
            errorCurrent ("Expected an annotation key");
        }

        value.clear();

        if (match (YdspTokenType::colon))
            value = current().type == YdspTokenType::lBrace ? parseListValue() : parseValue();

        if (key.isNotEmpty())
            annotations.emplace_back (std::move (key), std::move (value));

        if (! match (YdspTokenType::comma))
            break;
    }

    expect (YdspTokenType::rAnnotation, "']]' to close the annotation block");

    return annotations;
}

//==============================================================================

std::unique_ptr<YdspGraphDecl> YdspParser::parseGraph()
{
    auto graph = std::make_unique<YdspGraphDecl>();

    const auto& nameToken = expectIdentifier ("a graph name");
    graph->name = nameToken.text;
    graph->location = { nameToken.line, nameToken.column };

    graph->annotations = parseAnnotations();

    expect (YdspTokenType::lBrace, "'{' after graph name");

    while (! at (YdspTokenType::rBrace) && ! at (YdspTokenType::endOfFile))
    {
        if (at (YdspTokenType::kwInput) || at (YdspTokenType::kwOutput))
        {
            for (auto& ep : parseEndpointWithKind())
                graph->endpoints.push_back (std::move (ep));
            continue;
        }

        if (match (YdspTokenType::kwNode))
        {
            graph->nodes.push_back (parseNode());
            continue;
        }

        if (match (YdspTokenType::kwConnection))
        {
            if (graph->bodyKind == YdspGraphBodyKind::algebra)
                errorCurrent ("A graph cannot have both a connection block and a 'process =' definition");

            graph->bodyKind = YdspGraphBodyKind::connections;
            parseConnectionBlock (*graph);
            continue;
        }

        if (match (YdspTokenType::kwProcess))
        {
            if (graph->bodyKind == YdspGraphBodyKind::connections)
                errorCurrent ("A graph cannot have both a connection block and a 'process =' definition");

            graph->bodyKind = YdspGraphBodyKind::algebra;
            parseAlgebraDefinition (*graph);
            continue;
        }

        errorCurrent ("Expected an endpoint, 'node', 'connection' or 'process' declaration");
        synchronize();
    }

    expect (YdspTokenType::rBrace, "'}' to close the graph");

    return graph;
}

YdspNodeDecl YdspParser::parseNode()
{
    YdspNodeDecl node;

    const auto& instanceToken = expectIdentifier ("a node instance name");
    node.instanceName = instanceToken.text;
    node.location = { instanceToken.line, instanceToken.column };

    expect (YdspTokenType::assign, "'=' after the node instance name");

    const auto& processorToken = expectIdentifier ("a processor name");
    node.processorName = processorToken.text;

    while (match (YdspTokenType::dot))
    {
        const auto& partToken = expectIdentifier ("a processor name after '.'");
        node.processorName += "." + partToken.text;
    }

    if (match (YdspTokenType::lBracket))
    {
        const auto& countToken = expect (YdspTokenType::intLiteral, "a voice count");
        node.voiceCount = static_cast<int> (parseIntLiteralText (countToken.text));

        expect (YdspTokenType::rBracket, "']' after the voice count");
    }

    if (match (YdspTokenType::lParen))
    {
        if (! at (YdspTokenType::rParen))
        {
            for (;;)
            {
                const auto& paramToken = expectIdentifier ("a parameter name in the override list");

                expect (YdspTokenType::assign, "'=' after the parameter name");

                auto value = parseExpression();

                node.overrides.emplace_back (paramToken.text, std::move (value));

                if (! match (YdspTokenType::comma))
                    break;
            }
        }

        expect (YdspTokenType::rParen, "')' to close the override list");
    }

    if (match (YdspTokenType::star))
    {
        const auto& multToken = expect (YdspTokenType::intLiteral, "an integer multiplier after '*'");
        node.rateMultiplier = static_cast<int> (parseIntLiteralText (multToken.text));
    }
    else if (match (YdspTokenType::slash))
    {
        const auto& divToken = expect (YdspTokenType::intLiteral, "an integer divider after '/'");
        node.rateDivider = static_cast<int> (parseIntLiteralText (divToken.text));
    }

    node.annotations = parseAnnotations();

    expect (YdspTokenType::semi, "';' after node declaration");

    return node;
}

void YdspParser::parseConnectionBlock (YdspGraphDecl& graph)
{
    expect (YdspTokenType::lBrace, "'{' after 'connection'");

    while (! at (YdspTokenType::rBrace) && ! at (YdspTokenType::endOfFile))
    {
        YdspConnection connection;

        const auto& sourceToken = expectIdentifier ("a connection source");
        connection.sourcePath = sourceToken.text;
        connection.location = { sourceToken.line, sourceToken.column };

        if (match (YdspTokenType::dot))
        {
            const auto& memberToken = expectIdentifier ("an endpoint name after '.'");
            connection.sourcePath += "." + memberToken.text;
        }

        expect (YdspTokenType::arrow, "'->' in connection");

        if (match (YdspTokenType::lBracket))
        {
            const auto& delayToken = expect (YdspTokenType::intLiteral, "a delay amount in samples");
            connection.delaySamples = static_cast<int> (parseIntLiteralText (delayToken.text));

            expect (YdspTokenType::rBracket, "']' after delay amount");

            expect (YdspTokenType::arrow, "'->' after inline delay");
        }

        const auto& destToken = expectIdentifier ("a connection destination");
        connection.destPath = destToken.text;

        if (match (YdspTokenType::dot))
        {
            const auto& memberToken = expectIdentifier ("an endpoint name after '.'");
            connection.destPath += "." + memberToken.text;
        }

        expect (YdspTokenType::semi, "';' after connection");

        graph.connections.push_back (std::move (connection));
    }

    expect (YdspTokenType::rBrace, "'}' to close the connection block");
}

void YdspParser::parseAlgebraDefinition (YdspGraphDecl& graph)
{
    expect (YdspTokenType::assign, "'=' after 'process'");

    graph.algebraRoot = parseAlgebra();

    expect (YdspTokenType::semi, "';' after the process definition");
}

//==============================================================================

YdspStmtPtr YdspParser::parseStatement()
{
    YdspRecursionGuard guard (recursionDepth);

    if (guard.exceeded())
    {
        errorCurrent ("Statement nested too deeply");
        synchronize();
        return nullptr;
    }

    if (at (YdspTokenType::lBrace))
        return parseBlockStatement();

    if (at (YdspTokenType::kwIf))
        return parseIfStatement();

    if (at (YdspTokenType::kwFor))
        return parseForStatement();

    if (at (YdspTokenType::kwReturn))
        return parseReturnStatement();

    if (at (YdspTokenType::kwLet))
        return parseLocalDeclaration();

    if (at (YdspTokenType::kwEmit))
        return parseEmitStatement();

    if (at (YdspTokenType::identifier))
    {
        if (peekNext().type == YdspTokenType::identifier)
            return parseLocalDeclaration();

        return parseAssignment();
    }

    errorCurrent ("Expected a statement");

    synchronize();

    return nullptr;
}

YdspStmtPtr YdspParser::parseBlockStatement()
{
    auto stmt = std::make_unique<YdspStmt>();
    stmt->kind = YdspStmtKind::block;
    stmt->location = { current().line, current().column };

    expect (YdspTokenType::lBrace, "'{'");

    while (! at (YdspTokenType::rBrace) && ! at (YdspTokenType::endOfFile))
    {
        auto child = parseStatement();

        if (child != nullptr)
            stmt->children.push_back (std::move (child));
    }

    expect (YdspTokenType::rBrace, "'}'");

    return stmt;
}

YdspStmtPtr YdspParser::parseIfStatement()
{
    auto stmt = std::make_unique<YdspStmt>();
    stmt->kind = YdspStmtKind::ifStmt;
    stmt->location = { current().line, current().column };

    advance(); // 'if'

    expect (YdspTokenType::lParen, "'(' after 'if'");

    stmt->cond = parseExpression();

    expect (YdspTokenType::rParen, "')' after the if condition");

    stmt->thenStmt = parseStatement();

    if (match (YdspTokenType::kwElse))
        stmt->elseStmt = parseStatement();

    return stmt;
}

YdspStmtPtr YdspParser::parseForStatement()
{
    auto stmt = std::make_unique<YdspStmt>();
    stmt->kind = YdspStmtKind::forStmt;
    stmt->location = { current().line, current().column };

    advance(); // 'for'

    const auto& nameToken = expectIdentifier ("a loop variable name");
    stmt->name = nameToken.text;

    if (! matchIdentifier ("in"))
        errorCurrent ("Expected 'in' after the loop variable");

    stmt->startExpr = parseExpression();

    expect (YdspTokenType::range, "'..' between the loop bounds");

    stmt->endExpr = parseExpression();

    stmt->body = parseStatement();

    return stmt;
}

YdspStmtPtr YdspParser::parseLocalDeclaration()
{
    auto stmt = std::make_unique<YdspStmt>();
    stmt->kind = YdspStmtKind::localDecl;
    stmt->location = { current().line, current().column };

    if (match (YdspTokenType::kwLet))
    {
        stmt->isLet = true;

        const auto& nameToken = expectIdentifier ("a variable name after 'let'");
        stmt->name = nameToken.text;

        expect (YdspTokenType::assign, "'=' after the variable name");

        stmt->value = parseExpression();
    }
    else
    {
        const auto& typeToken = expectIdentifier ("a type name (float, int, float64, int64 or bool)");
        const auto type = parsePrimitiveType (typeToken);

        if (type.has_value())
            stmt->declType = *type;
        else
            error (typeToken, "Unknown type '" + typeToken.text + "' (expected float, int, float64, int64 or bool)");

        stmt->hasDeclType = true;

        const auto& nameToken = expectIdentifier ("a variable name");
        stmt->name = nameToken.text;

        if (match (YdspTokenType::assign))
            stmt->value = parseExpression();
    }

    expect (YdspTokenType::semi, "';' after the local declaration");

    return stmt;
}

YdspStmtPtr YdspParser::parseEmitStatement()
{
    auto stmt = std::make_unique<YdspStmt>();
    stmt->kind = YdspStmtKind::emitStmt;
    stmt->location = { current().line, current().column };

    advance(); // 'emit'

    const auto& shapeToken = expectIdentifier ("an event shape name after 'emit'");
    stmt->shapeName = shapeToken.text;

    expect (YdspTokenType::lParen, "'(' after the shape name");

    if (! at (YdspTokenType::rParen))
    {
        for (;;)
        {
            const auto& fieldToken = expectIdentifier ("a field name in the emit field list");

            expect (YdspTokenType::colon, "':' after the field name");

            auto value = parseExpression();

            stmt->emitFields.emplace_back (fieldToken.text, std::move (value));

            if (! match (YdspTokenType::comma))
                break;
        }
    }

    expect (YdspTokenType::rParen, "')' to close the emit field list");

    expect (YdspTokenType::arrow, "'->' after the emit field list");

    const auto& endpointToken = expectIdentifier ("an endpoint name after '->'");
    stmt->endpointName = endpointToken.text;

    expect (YdspTokenType::semi, "';' after the emit statement");

    return stmt;
}

YdspStmtPtr YdspParser::parseAssignment()
{
    auto stmt = std::make_unique<YdspStmt>();
    stmt->kind = YdspStmtKind::assign;
    stmt->location = { current().line, current().column };

    auto target = std::make_unique<YdspExpr>();
    target->kind = YdspExprKind::identifier;
    target->location = { current().line, current().column };
    target->text = current().text;

    advance();

    for (;;)
    {
        if (match (YdspTokenType::lBracket))
        {
            auto indexExpr = parseExpression();

            expect (YdspTokenType::rBracket, "']' after the index");

            auto indexed = std::make_unique<YdspExpr>();
            indexed->kind = YdspExprKind::index;
            indexed->location = target->location;
            indexed->children.push_back (std::move (target));
            indexed->children.push_back (std::move (indexExpr));

            target = std::move (indexed);
            continue;
        }

        if (match (YdspTokenType::dot))
        {
            const auto& memberToken = expectMemberName ("a member name after '.'");

            auto memberExpr = std::make_unique<YdspExpr>();
            memberExpr->kind = YdspExprKind::member;
            memberExpr->location = target->location;
            memberExpr->text = memberToken.text;
            memberExpr->children.push_back (std::move (target));

            target = std::move (memberExpr);
            continue;
        }

        break;
    }

    stmt->target = std::move (target);

    YdspOperator compoundOp = YdspOperator::none;
    if (match (YdspTokenType::plusEq))
        compoundOp = YdspOperator::add;
    else if (match (YdspTokenType::minusEq))
        compoundOp = YdspOperator::sub;
    else if (match (YdspTokenType::starEq))
        compoundOp = YdspOperator::mul;
    else if (match (YdspTokenType::slashEq))
        compoundOp = YdspOperator::div;
    else if (match (YdspTokenType::percentEq))
        compoundOp = YdspOperator::mod;
    else if (match (YdspTokenType::ampersandEq))
        compoundOp = YdspOperator::bitAnd;
    else if (match (YdspTokenType::pipeEq))
        compoundOp = YdspOperator::bitOr;
    else if (match (YdspTokenType::caretEq))
        compoundOp = YdspOperator::bitXor;
    else if (match (YdspTokenType::shlEq))
        compoundOp = YdspOperator::shl;
    else if (match (YdspTokenType::shrEq))
        compoundOp = YdspOperator::shr;
    else
        expect (YdspTokenType::assign, "'=' in assignment");

    stmt->value = parseExpression();

    if (compoundOp != YdspOperator::none)
    {
        auto targetExpr = YdspExprFactory::clone (*stmt->target);
        stmt->value = YdspExprFactory::makeBinary (stmt->location, compoundOp, std::move (targetExpr), std::move (stmt->value));
    }

    expect (YdspTokenType::semi, "';' after the assignment");

    return stmt;
}

//==============================================================================

YdspStmtPtr YdspParser::parseReturnStatement()
{
    auto stmt = std::make_unique<YdspStmt>();
    stmt->kind = YdspStmtKind::returnStmt;
    stmt->location = { current().line, current().column };

    advance(); // 'return'

    if (! at (YdspTokenType::semi))
        stmt->returnExpr = parseExpression();

    expect (YdspTokenType::semi, "';' after the return statement");

    return stmt;
}

YdspFuncDecl YdspParser::parseFunction()
{
    YdspFuncDecl func;

    const auto& nameToken = expectIdentifier ("a function name");
    func.name = nameToken.text;
    func.location = { nameToken.line, nameToken.column };

    expect (YdspTokenType::lParen, "'(' after function name");

    if (! at (YdspTokenType::rParen))
    {
        for (;;)
        {
            const auto& nameToken = expectIdentifier ("a parameter name");

            expect (YdspTokenType::colon, "':' after the parameter name");

            const auto& typeToken = expectIdentifier ("a parameter type (float, int, float64, int64 or bool)");

            YdspPrimitiveType paramType = YdspPrimitiveType::float32Type;
            const auto type = parsePrimitiveType (typeToken);

            if (type.has_value())
                paramType = *type;
            else
                error (typeToken, "Unknown type '" + typeToken.text + "' (expected float, int, float64, int64 or bool)");

            func.params.emplace_back (nameToken.text, paramType);

            if (! match (YdspTokenType::comma))
                break;
        }
    }

    expect (YdspTokenType::rParen, "')' after function parameters");

    if (match (YdspTokenType::colon))
    {
        const auto& retTypeToken = expectIdentifier ("a return type (float, int, float64, int64 or bool)");
        const auto retType = parsePrimitiveType (retTypeToken);

        func.hasReturnType = true;

        if (retType.has_value())
            func.returnType = *retType;
        else
            error (retTypeToken, "Unknown type '" + retTypeToken.text + "' (expected float, int, float64, int64 or bool)");
    }

    expect (YdspTokenType::lBrace, "'{' to start the function body");

    while (! at (YdspTokenType::rBrace) && ! at (YdspTokenType::endOfFile))
    {
        auto stmt = parseStatement();

        if (stmt != nullptr)
            func.body.push_back (std::move (stmt));
    }

    expect (YdspTokenType::rBrace, "'}' to close the function body");

    return func;
}

//==============================================================================

YdspExprPtr YdspParser::parseExpression()
{
    YdspRecursionGuard guard (recursionDepth);

    if (guard.exceeded())
    {
        errorCurrent ("Expression nested too deeply");
        return std::make_unique<YdspExpr>();
    }

    return parseTernary();
}

YdspExprPtr YdspParser::parseTernary()
{
    auto cond = parseOr();

    if (match (YdspTokenType::question))
    {
        auto expr = std::make_unique<YdspExpr>();
        expr->kind = YdspExprKind::ternary;
        expr->location = cond->location;

        auto thenExpr = parseExpression();

        expect (YdspTokenType::colon, "':' in the ternary expression");

        auto elseExpr = parseExpression();

        expr->children.push_back (std::move (cond));
        expr->children.push_back (std::move (thenExpr));
        expr->children.push_back (std::move (elseExpr));

        return expr;
    }

    return cond;
}

YdspExprPtr YdspParser::parseOr()
{
    auto lhs = parseAnd();

    while (at (YdspTokenType::orOr))
    {
        const auto& opToken = tokens[index++];
        auto rhs = parseAnd();
        lhs = YdspExprFactory::makeBinary ({ opToken.line, opToken.column }, YdspOperator::orL, std::move (lhs), std::move (rhs));
    }

    return lhs;
}

YdspExprPtr YdspParser::parseAnd()
{
    auto lhs = parseBitOr();

    while (at (YdspTokenType::andAnd))
    {
        const auto& opToken = tokens[index++];
        auto rhs = parseBitOr();
        lhs = YdspExprFactory::makeBinary ({ opToken.line, opToken.column }, YdspOperator::andL, std::move (lhs), std::move (rhs));
    }

    return lhs;
}

YdspExprPtr YdspParser::parseBitOr()
{
    auto lhs = parseBitXor();

    while (at (YdspTokenType::pipe))
    {
        const auto& opToken = tokens[index++];
        auto rhs = parseBitXor();
        lhs = YdspExprFactory::makeBinary ({ opToken.line, opToken.column }, YdspOperator::bitOr, std::move (lhs), std::move (rhs));
    }

    return lhs;
}

YdspExprPtr YdspParser::parseBitXor()
{
    auto lhs = parseBitAnd();

    while (at (YdspTokenType::caret))
    {
        const auto& opToken = tokens[index++];
        auto rhs = parseBitAnd();
        lhs = YdspExprFactory::makeBinary ({ opToken.line, opToken.column }, YdspOperator::bitXor, std::move (lhs), std::move (rhs));
    }

    return lhs;
}

YdspExprPtr YdspParser::parseBitAnd()
{
    auto lhs = parseEquality();

    while (at (YdspTokenType::ampersand))
    {
        const auto& opToken = tokens[index++];
        auto rhs = parseEquality();
        lhs = YdspExprFactory::makeBinary ({ opToken.line, opToken.column }, YdspOperator::bitAnd, std::move (lhs), std::move (rhs));
    }

    return lhs;
}

YdspExprPtr YdspParser::parseEquality()
{
    auto lhs = parseRelational();

    for (;;)
    {
        YdspOperator op = YdspOperator::none;

        if (at (YdspTokenType::equal))
            op = YdspOperator::eq;
        else if (at (YdspTokenType::notEqual))
            op = YdspOperator::ne;

        if (op == YdspOperator::none)
            break;

        const auto& opToken = tokens[index++];
        auto rhs = parseRelational();
        lhs = YdspExprFactory::makeBinary ({ opToken.line, opToken.column }, op, std::move (lhs), std::move (rhs));
    }

    return lhs;
}

YdspExprPtr YdspParser::parseRelational()
{
    auto lhs = parseShift();

    for (;;)
    {
        YdspOperator op = YdspOperator::none;

        if (at (YdspTokenType::less))
            op = YdspOperator::lt;
        else if (at (YdspTokenType::lessEqual))
            op = YdspOperator::le;
        else if (at (YdspTokenType::greater))
            op = YdspOperator::gt;
        else if (at (YdspTokenType::greaterEqual))
            op = YdspOperator::ge;

        if (op == YdspOperator::none)
            break;

        const auto& opToken = tokens[index++];
        auto rhs = parseShift();
        lhs = YdspExprFactory::makeBinary ({ opToken.line, opToken.column }, op, std::move (lhs), std::move (rhs));
    }

    return lhs;
}

YdspExprPtr YdspParser::parseShift()
{
    auto lhs = parseDelay();

    for (;;)
    {
        YdspOperator op = YdspOperator::none;

        if (at (YdspTokenType::shl))
            op = YdspOperator::shl;
        else if (at (YdspTokenType::shr))
            op = YdspOperator::shr;

        if (op == YdspOperator::none)
            break;

        const auto& opToken = tokens[index++];
        auto rhs = parseDelay();
        lhs = YdspExprFactory::makeBinary ({ opToken.line, opToken.column }, op, std::move (lhs), std::move (rhs));
    }

    return lhs;
}

YdspExprPtr YdspParser::parseDelay()
{
    auto lhs = parseAdditive();

    while (at (YdspTokenType::at))
    {
        const auto& opToken = tokens[index++];
        auto rhs = parseAdditive();

        auto expr = std::make_unique<YdspExpr>();
        expr->kind = YdspExprKind::delay;
        expr->location = { opToken.line, opToken.column };
        expr->children.push_back (std::move (lhs));
        expr->children.push_back (std::move (rhs));
        lhs = std::move (expr);
    }

    return lhs;
}

YdspExprPtr YdspParser::parseAdditive()
{
    auto lhs = parseMultiplicative();

    for (;;)
    {
        YdspOperator op = YdspOperator::none;

        if (at (YdspTokenType::plus))
            op = YdspOperator::add;
        else if (at (YdspTokenType::minus))
            op = YdspOperator::sub;

        if (op == YdspOperator::none)
            break;

        const auto& opToken = tokens[index++];
        auto rhs = parseMultiplicative();
        lhs = YdspExprFactory::makeBinary ({ opToken.line, opToken.column }, op, std::move (lhs), std::move (rhs));
    }

    return lhs;
}

YdspExprPtr YdspParser::parseMultiplicative()
{
    auto lhs = parseUnary();

    for (;;)
    {
        YdspOperator op = YdspOperator::none;

        if (at (YdspTokenType::star))
            op = YdspOperator::mul;
        else if (at (YdspTokenType::slash))
            op = YdspOperator::div;
        else if (at (YdspTokenType::percent))
            op = YdspOperator::mod;

        if (op == YdspOperator::none)
            break;

        const auto& opToken = tokens[index++];
        auto rhs = parseUnary();
        lhs = YdspExprFactory::makeBinary ({ opToken.line, opToken.column }, op, std::move (lhs), std::move (rhs));
    }

    return lhs;
}

YdspExprPtr YdspParser::parseUnary()
{
    YdspRecursionGuard guard (recursionDepth);

    if (guard.exceeded())
    {
        errorCurrent ("Expression nested too deeply");
        return std::make_unique<YdspExpr>();
    }

    if (at (YdspTokenType::minus))
    {
        const auto& opToken = tokens[index++];
        auto operand = parseUnary();
        return YdspExprFactory::makeUnary ({ opToken.line, opToken.column }, YdspOperator::neg, std::move (operand));
    }

    if (at (YdspTokenType::not_))
    {
        const auto& opToken = tokens[index++];
        auto operand = parseUnary();
        return YdspExprFactory::makeUnary ({ opToken.line, opToken.column }, YdspOperator::notL, std::move (operand));
    }

    if (at (YdspTokenType::tilde))
    {
        const auto& opToken = tokens[index++];
        auto operand = parseUnary();
        return YdspExprFactory::makeUnary ({ opToken.line, opToken.column }, YdspOperator::notI, std::move (operand));
    }

    return parsePostfix();
}

YdspExprPtr YdspParser::parsePostfix()
{
    auto expr = parsePrimary();

    for (;;)
    {
        if (match (YdspTokenType::apostrophe))
        {
            auto prevExpr = std::make_unique<YdspExpr>();
            prevExpr->kind = YdspExprKind::prev;
            prevExpr->location = expr->location;
            prevExpr->children.push_back (std::move (expr));
            expr = std::move (prevExpr);
            continue;
        }

        if (at (YdspTokenType::lBracket))
        {
            advance();

            auto indexExpr = parseExpression();

            expect (YdspTokenType::rBracket, "']' after the index");

            auto indexed = std::make_unique<YdspExpr>();
            indexed->kind = YdspExprKind::index;
            indexed->location = expr->location;
            indexed->children.push_back (std::move (expr));
            indexed->children.push_back (std::move (indexExpr));

            expr = std::move (indexed);
            continue;
        }

        if (match (YdspTokenType::dot))
        {
            const auto& memberToken = expectMemberName ("a member name after '.'");

            if (at (YdspTokenType::lParen))
            {
                auto dottedName = getDottedPath (*expr);

                if (dottedName.isNotEmpty())
                {
                    dottedName += "." + memberToken.text;
                    advance(); // '('

                    std::vector<YdspExprPtr> args;

                    if (! at (YdspTokenType::rParen))
                    {
                        for (;;)
                        {
                            args.push_back (parseExpression());

                            if (! match (YdspTokenType::comma))
                                break;
                        }
                    }

                    expect (YdspTokenType::rParen, "')' to close the call");

                    return YdspExprFactory::makeCall (expr->location, dottedName, std::move (args));
                }
            }

            auto memberExpr = std::make_unique<YdspExpr>();
            memberExpr->kind = YdspExprKind::member;
            memberExpr->location = expr->location;
            memberExpr->text = memberToken.text;
            memberExpr->children.push_back (std::move (expr));

            expr = std::move (memberExpr);
            continue;
        }

        break;
    }

    return expr;
}

YdspExprPtr YdspParser::parsePrimary()
{
    const auto& token = current();

    if (token.type == YdspTokenType::intLiteral)
    {
        advance();
        return YdspExprFactory::makeInt ({ token.line, token.column }, parseIntLiteralText (token.text));
    }

    if (token.type == YdspTokenType::floatLiteral)
    {
        advance();
        return YdspExprFactory::makeFloat ({ token.line, token.column }, parseFloatLiteralText (token.text));
    }

    if (token.type == YdspTokenType::kwTrue || token.type == YdspTokenType::kwFalse)
    {
        advance();
        return YdspExprFactory::makeBool ({ token.line, token.column }, token.type == YdspTokenType::kwTrue);
    }

    if (token.type == YdspTokenType::identifier)
    {
        advance();

        if (at (YdspTokenType::lParen))
        {
            advance();

            std::vector<YdspExprPtr> args;

            if (! at (YdspTokenType::rParen))
            {
                for (;;)
                {
                    args.push_back (parseExpression());

                    if (! match (YdspTokenType::comma))
                        break;
                }
            }

            expect (YdspTokenType::rParen, "')' to close the call");

            return YdspExprFactory::makeCall ({ token.line, token.column }, token.text, std::move (args));
        }

        return YdspExprFactory::makeIdentifier ({ token.line, token.column }, token.text);
    }

    if (token.type == YdspTokenType::lParen)
        return parseParenthesized();

    errorCurrent ("Expected an expression");

    synchronize();

    return YdspExprFactory::makeInt ({ token.line, token.column }, 0);
}

YdspExprPtr YdspParser::parseParenthesized()
{
    advance(); // '('

    auto expr = parseExpression();

    expect (YdspTokenType::rParen, "')'");

    return expr;
}

//==============================================================================

YdspExprPtr YdspParser::parseAlgebra()
{
    YdspRecursionGuard guard (recursionDepth);

    if (guard.exceeded())
    {
        errorCurrent ("Graph expression nested too deeply");
        return std::make_unique<YdspExpr>();
    }

    return parseAlgebraParallel();
}

YdspExprPtr YdspParser::parseAlgebraParallel()
{
    auto lhs = parseAlgebraSequential();

    while (at (YdspTokenType::comma))
    {
        const auto& opToken = tokens[index++];
        auto rhs = parseAlgebraSequential();

        auto expr = std::make_unique<YdspExpr>();
        expr->kind = YdspExprKind::graphOp;
        expr->op = YdspOperator::par;
        expr->location = { opToken.line, opToken.column };
        expr->children.push_back (std::move (lhs));
        expr->children.push_back (std::move (rhs));
        lhs = std::move (expr);
    }

    return lhs;
}

YdspExprPtr YdspParser::parseAlgebraSequential()
{
    auto lhs = parseAlgebraRecursive();

    for (;;)
    {
        YdspOperator op = YdspOperator::none;

        if (at (YdspTokenType::colon))
            op = YdspOperator::seq;
        else if (at (YdspTokenType::lessColon))
            op = YdspOperator::split;
        else if (at (YdspTokenType::colonGreater))
            op = YdspOperator::merge;

        if (op == YdspOperator::none)
            break;

        const auto& opToken = tokens[index++];
        auto rhs = parseAlgebraRecursive();

        auto expr = std::make_unique<YdspExpr>();
        expr->kind = YdspExprKind::graphOp;
        expr->op = op;
        expr->location = { opToken.line, opToken.column };
        expr->children.push_back (std::move (lhs));
        expr->children.push_back (std::move (rhs));
        lhs = std::move (expr);
    }

    return lhs;
}

YdspExprPtr YdspParser::parseAlgebraRecursive()
{
    YdspRecursionGuard guard (recursionDepth);

    if (guard.exceeded())
    {
        errorCurrent ("Graph expression nested too deeply");
        return std::make_unique<YdspExpr>();
    }

    auto lhs = parseAlgebraPrimary();

    while (at (YdspTokenType::tilde))
    {
        const auto& opToken = tokens[index++];
        auto rhs = parseAlgebraRecursive();

        auto expr = std::make_unique<YdspExpr>();
        expr->kind = YdspExprKind::graphOp;
        expr->op = YdspOperator::recurse;
        expr->location = { opToken.line, opToken.column };
        expr->children.push_back (std::move (lhs));
        expr->children.push_back (std::move (rhs));
        lhs = std::move (expr);
    }

    return lhs;
}

YdspExprPtr YdspParser::parseAlgebraPrimary()
{
    const auto& token = current();

    if (token.type == YdspTokenType::underscore)
    {
        advance();

        auto expr = std::make_unique<YdspExpr>();
        expr->kind = YdspExprKind::graphLeaf;
        expr->text = "_";
        expr->location = { token.line, token.column };
        return expr;
    }

    if (token.type == YdspTokenType::identifier)
    {
        advance();

        auto expr = std::make_unique<YdspExpr>();
        expr->kind = YdspExprKind::graphLeaf;
        expr->text = token.text;
        expr->location = { token.line, token.column };

        while (at (YdspTokenType::dot))
        {
            advance();

            const auto& partToken = expectIdentifier ("a processor name after '.'");
            expr->text += "." + partToken.text;
        }

        if (at (YdspTokenType::lParen))
        {
            advance();

            if (! at (YdspTokenType::rParen))
            {
                for (;;)
                {
                    const auto& paramToken = expectIdentifier ("a parameter name in the override list");

                    expect (YdspTokenType::assign, "'=' after the parameter name");

                    auto value = parseExpression();

                    expr->overrides.emplace_back (paramToken.text, std::move (value));

                    if (! match (YdspTokenType::comma))
                        break;
                }
            }

            expect (YdspTokenType::rParen, "')' to close the override list");
        }

        return expr;
    }

    if (token.type == YdspTokenType::lParen)
    {
        advance();

        auto expr = parseAlgebra();

        expect (YdspTokenType::rParen, "')'");

        return expr;
    }

    errorCurrent ("Expected a graph expression (a name, '_' or '(')");

    synchronize();

    auto expr = std::make_unique<YdspExpr>();
    expr->kind = YdspExprKind::graphLeaf;
    expr->text = "_";
    expr->location = { token.line, token.column };
    return expr;
}

} // namespace yup
