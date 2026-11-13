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
struct Expression::Node
{
    Kind kind = Kind::constant;
    ExpressionOperator op = ExpressionOperator::none;
    String name;
    Value value;
    std::vector<Expression> inputs;
    int line = 0, column = 0;
};

namespace
{

enum ExpressionPrecedence
{
    precedenceTernary = 1,
    precedenceOr,
    precedenceAnd,
    precedenceEquality,
    precedenceComparison,
    precedenceAdditive,
    precedenceMultiplicative,
    precedenceUnary,
    precedencePower,
    precedencePostfix
};

int getBinaryPrecedence (ExpressionOperator op) noexcept
{
    switch (op)
    {
        case ExpressionOperator::logicalOr:
            return precedenceOr;
        case ExpressionOperator::logicalAnd:
            return precedenceAnd;
        case ExpressionOperator::equal:
        case ExpressionOperator::notEqual:
            return precedenceEquality;
        case ExpressionOperator::less:
        case ExpressionOperator::lessOrEqual:
        case ExpressionOperator::greater:
        case ExpressionOperator::greaterOrEqual:
            return precedenceComparison;
        case ExpressionOperator::add:
        case ExpressionOperator::subtract:
            return precedenceAdditive;
        case ExpressionOperator::multiply:
        case ExpressionOperator::divide:
        case ExpressionOperator::modulo:
            return precedenceMultiplicative;
        case ExpressionOperator::power:
            return precedencePower;
        default:
            return precedencePostfix;
    }
}

String formatExpressionFloat (double value, int maxPrecision)
{
    std::string text;

    for (int precision = 1; precision <= maxPrecision; ++precision)
    {
        std::ostringstream out;
        out.imbue (std::locale::classic());
        out << std::setprecision (precision) << value;
        text = out.str();

        std::istringstream in (text);
        in.imbue (std::locale::classic());

        double parsed = 0.0;
        in >> parsed;

        if (maxPrecision > 9 ? parsed == value : (float) parsed == (float) value)
            break;
    }

    if (text.find_first_of (".en") == std::string::npos)
        text += ".0";

    return String (text.c_str());
}

String quoteExpressionString (const String& text)
{
    return "\""
         + text.replace ("\\", "\\\\")
               .replace ("\"", "\\\"")
               .replace ("\n", "\\n")
               .replace ("\t", "\\t")
               .replace ("\r", "\\r")
         + "\"";
}

String getFirstSymbolSegment (const String& name)
{
    return name.upToFirstOccurrenceOf (".", false, false);
}

} // namespace

//==============================================================================
struct Expression::Parser
{
    struct Error
    {
        String message;
        int line, column, offset;
    };

    explicit Parser (StringRef source, int startOffset = 0)
        : text (source.text.getAddress())
        , position (text)
        , lineStart (text)
    {
        for (const auto* end = text + jlimit (0, (int) std::strlen (text), startOffset); position < end; ++position)
        {
            if (*position == '\n')
            {
                ++line;
                lineStart = position + 1;
            }
        }
    }

    Expression parseSingleExpression()
    {
        auto result = parseTernary();
        skipWhitespace();
        return result;
    }

    int getOffset() const noexcept
    {
        return (int) (position - text);
    }

    Expression parseProgram()
    {
        std::vector<Expression> statements;

        while (! isAtEnd())
        {
            const auto statementPosition = getPosition();

            if (matchKeyword ("let"))
            {
                const auto name = parseIdentifier ("Expected a name after 'let'");
                expect ("=");
                statements.push_back (make (Kind::let, ExpressionOperator::none, name, {}, { parseTernary() }, statementPosition));
            }
            else if (isOutputAssignment())
            {
                const auto name = parseIdentifier ("Expected a name");
                expect ("=");
                statements.push_back (make (Kind::output, ExpressionOperator::none, name, {}, { parseTernary() }, statementPosition));
            }
            else
            {
                statements.push_back (parseTernary());

                if (! match (";") && ! isAtEnd())
                    fail ("Expected ';' or the end of the expression");

                if (! isAtEnd())
                    fail ("Only the last statement can be a plain expression, use 'name = ...' or 'let name = ...'", statementPosition);

                break;
            }

            if (! match (";") && ! isAtEnd())
                fail ("Expected ';'");
        }

        if (statements.empty())
            fail ("Expected an expression");

        if (statements.size() == 1 && statements.front().getKind() != Kind::let && statements.front().getKind() != Kind::output)
            return statements.front();

        return make (Kind::block, ExpressionOperator::none, {}, {}, std::move (statements), { 1, 1 });
    }

private:
    using Position = std::pair<int, int>;

    struct DepthGuard
    {
        explicit DepthGuard (Parser& p)
            : parser (p)
        {
            if (++parser.depth > 256)
                parser.fail ("Expression is too deeply nested");
        }

        ~DepthGuard() { --parser.depth; }

        Parser& parser;
    };

    //==============================================================================
    Expression parseTernary()
    {
        DepthGuard guard (*this);

        const auto start = getPosition();
        auto condition = parseBinary (precedenceOr);

        if (! match ("?"))
            return condition;

        auto whenTrue = parseTernary();
        expect (":");
        auto whenFalse = parseTernary();

        return make (Kind::ternary, ExpressionOperator::none, {}, {}, { condition, whenTrue, whenFalse }, start);
    }

    Expression parseBinary (int precedence)
    {
        if (precedence > precedenceMultiplicative)
            return parseUnary();

        auto lhs = parseBinary (precedence + 1);

        for (;;)
        {
            const auto start = getPosition();
            const auto op = matchBinaryOperator (precedence);

            if (op == ExpressionOperator::none)
                return lhs;

            auto rhs = parseBinary (precedence + 1);
            lhs = make (Kind::binary, op, {}, {}, { lhs, rhs }, start);
        }
    }

    Expression parseUnary()
    {
        DepthGuard guard (*this);

        const auto start = getPosition();

        if (match ("!"))
            return make (Kind::unary, ExpressionOperator::logicalNot, {}, {}, { parseUnary() }, start);

        if (match ("+"))
            return parseUnary();

        if (! match ("-"))
            return parsePower();

        auto operand = parseUnary();

        if (operand.getKind() == Kind::constant)
        {
            const auto& value = operand.getValue();

            if (const auto* i = std::get_if<int> (&value); i != nullptr && *i != std::numeric_limits<int>::min())
                return make (Kind::constant, ExpressionOperator::none, {}, -*i, {}, start);

            if (const auto* f = std::get_if<float> (&value))
                return make (Kind::constant, ExpressionOperator::none, {}, -*f, {}, start);

            if (const auto* d = std::get_if<double> (&value))
                return make (Kind::constant, ExpressionOperator::none, {}, -*d, {}, start);
        }

        return make (Kind::unary, ExpressionOperator::negate, {}, {}, { operand }, start);
    }

    Expression parsePower()
    {
        auto base = parsePostfix();
        const auto start = getPosition();

        if (! match ("^"))
            return base;

        return make (Kind::binary, ExpressionOperator::power, {}, {}, { base, parseUnary() }, start);
    }

    Expression parsePostfix()
    {
        auto result = parsePrimary();

        for (;;)
        {
            const auto start = getPosition();

            if (match ("["))
            {
                auto index = parseTernary();
                expect ("]");
                result = make (Kind::index, ExpressionOperator::none, {}, {}, { result, index }, start);
            }
            else if (match ("."))
            {
                const auto name = parseIdentifier ("Expected a member name after '.'");

                if (match ("("))
                    result = make (Kind::method, ExpressionOperator::none, name, {}, parseArguments (result), start);
                else
                    result = make (Kind::member, ExpressionOperator::none, name, {}, { result }, start);
            }
            else
            {
                return result;
            }
        }
    }

    Expression parsePrimary()
    {
        skipWhitespace();

        const auto start = getPosition();

        if (match ("("))
        {
            auto result = parseTernary();
            expect (")");
            return result;
        }

        if (*position == '"')
            return make (Kind::constant, ExpressionOperator::none, {}, parseString(), {}, start);

        if (CharacterFunctions::isDigit (*position) || (*position == '.' && CharacterFunctions::isDigit (position[1])))
            return parseNumber();

        if (*position == '#')
            return parseColor();

        if (! isIdentifierStart (*position))
            fail (isAtEnd() ? "Unexpected end of expression" : "Unexpected character '" + String::charToString ((yup_wchar) (uint8) *position) + "'");

        if (matchKeyword ("true"))
            return make (Kind::constant, ExpressionOperator::none, {}, true, {}, start);

        if (matchKeyword ("false"))
            return make (Kind::constant, ExpressionOperator::none, {}, false, {}, start);

        auto name = parseIdentifier ({});

        while (*position == '.' && isIdentifierStart (position[1]))
        {
            ++position;
            name << "." << parseIdentifier ({});
        }

        if (! match ("("))
            return make (Kind::symbol, ExpressionOperator::none, name, {}, {}, start);

        if (! name.containsChar ('.'))
            return make (Kind::call, ExpressionOperator::none, name, {}, parseArguments ({}), start);

        auto object = make (Kind::symbol, ExpressionOperator::none, name.upToLastOccurrenceOf (".", false, false), {}, {}, start);
        return make (Kind::method, ExpressionOperator::none, name.fromLastOccurrenceOf (".", false, false), {}, parseArguments (object), start);
    }

    std::vector<Expression> parseArguments (std::optional<Expression> object)
    {
        std::vector<Expression> arguments;

        if (object)
            arguments.push_back (*object);

        if (match (")"))
            return arguments;

        do
        {
            arguments.push_back (parseTernary());
        } while (match (","));

        expect (")");
        return arguments;
    }

    Expression parseNumber()
    {
        const auto start = getPosition();
        const auto* begin = position;

        if (position[0] == '0' && (position[1] == 'x' || position[1] == 'X'))
        {
            position += 2;
            uint64 value = 0;
            int numDigits = 0;

            for (; CharacterFunctions::getHexDigitValue ((yup_wchar) (uint8) *position) >= 0; ++position, ++numDigits)
            {
                value = (value << 4) | (uint64) CharacterFunctions::getHexDigitValue ((yup_wchar) (uint8) *position);

                if (value > 0xffffffffu)
                    fail ("Hexadecimal literal is too large", start);
            }

            if (numDigits == 0 || isIdentifierPart (*position))
                fail ("Invalid hexadecimal literal", start);

            return make (Kind::constant, ExpressionOperator::none, {}, (int) (uint32) value, {}, start);
        }

        bool isFloating = false;

        while (CharacterFunctions::isDigit (*position))
            ++position;

        if (*position == '.' && CharacterFunctions::isDigit (position[1]))
        {
            isFloating = true;
            ++position;

            while (CharacterFunctions::isDigit (*position))
                ++position;
        }

        if ((*position == 'e' || *position == 'E')
            && (CharacterFunctions::isDigit (position[1]) || ((position[1] == '+' || position[1] == '-') && CharacterFunctions::isDigit (position[2]))))
        {
            isFloating = true;
            position += 2;

            while (CharacterFunctions::isDigit (*position))
                ++position;
        }

        const auto literal = String (begin, (size_t) (position - begin));
        const bool isFloatSuffix = *position == 'f' && ! isIdentifierPart (position[1]);

        if (isFloatSuffix)
            ++position;

        const auto unit = isFloatSuffix ? String() : parseUnit (start);

        if (isFloatSuffix)
            return make (Kind::constant, ExpressionOperator::none, {}, (float) literal.getDoubleValue(), {}, start);

        if (unit == "deg")
            return make (Kind::constant, ExpressionOperator::none, {}, degreesToRadians (literal.getDoubleValue()), {}, start);

        if (unit == "ms")
            return make (Kind::constant, ExpressionOperator::none, {}, literal.getDoubleValue() / 1000.0, {}, start);

        if (unit == "%")
            return make (Kind::constant, ExpressionOperator::none, {}, literal.getDoubleValue() / 100.0, {}, start);

        if (isFloating)
            return make (Kind::constant, ExpressionOperator::none, {}, literal.getDoubleValue(), {}, start);

        const auto value = literal.getLargeIntValue();

        if (literal.length() > 10 || value > std::numeric_limits<int>::max())
            fail ("Integer literal is too large", start);

        return make (Kind::constant, ExpressionOperator::none, {}, (int) value, {}, start);
    }

    String parseUnit (Position start)
    {
        // A '%' followed by an operand is the modulo operator, as in `10%3`.
        if (*position == '%')
        {
            const auto next = position[1];

            if (isIdentifierPart (next) || (next != 0 && std::strchr ("(.-+!\"#", next) != nullptr))
                return {};

            ++position;
            return "%";
        }

        if (! isIdentifierStart (*position))
            return {};

        const auto* begin = position;

        while (isIdentifierPart (*position))
            ++position;

        const auto unit = String (begin, (size_t) (position - begin));

        if (unit != "deg" && unit != "rad" && unit != "ms" && unit != "s" && unit != "px")
            fail ("Unknown unit '" + unit + "', expected one of %, px, deg, rad, s or ms", start);

        return unit;
    }

    Expression parseColor()
    {
        const auto start = getPosition();
        const auto* begin = ++position;

        while (CharacterFunctions::getHexDigitValue ((yup_wchar) (uint8) *position) >= 0)
            ++position;

        const auto numDigits = (int) (position - begin);

        if (isIdentifierPart (*position) || (numDigits != 3 && numDigits != 4 && numDigits != 6 && numDigits != 8))
            fail ("Invalid color literal, expected #rgb, #rgba, #rrggbb or #rrggbbaa", start);

        uint32 rgba = 0;

        for (int i = 0; i < numDigits; ++i)
        {
            const auto digit = (uint32) CharacterFunctions::getHexDigitValue ((yup_wchar) (uint8) begin[i]);
            rgba = numDigits <= 4 ? ((rgba << 8) | (digit * 17)) : ((rgba << 4) | digit);
        }

        if (numDigits == 3 || numDigits == 6)
            rgba = (rgba << 8) | 0xffu;

        const auto argb = (rgba >> 8) | (rgba << 24);
        const auto value = make (Kind::constant, ExpressionOperator::none, {}, (int) argb, {}, start);
        return make (Kind::call, ExpressionOperator::none, "color", {}, { value }, start);
    }

    String parseString()
    {
        const auto start = getPosition();
        std::string result;

        for (++position;; ++position)
        {
            const auto c = *position;

            if (c == 0 || c == '\n')
                fail ("Unterminated string literal", start);

            if (c == '"')
                break;

            if (c != '\\')
            {
                result += c;
                continue;
            }

            switch (*++position)
            {
                case '\\':
                    result += '\\';
                    break;
                case '"':
                    result += '"';
                    break;
                case 'n':
                    result += '\n';
                    break;
                case 't':
                    result += '\t';
                    break;
                case 'r':
                    result += '\r';
                    break;
                default:
                    fail ("Invalid escape sequence in string literal");
            }
        }

        ++position;
        return String::fromUTF8 (result.data(), (int) result.size());
    }

    String parseIdentifier (const String& errorMessage)
    {
        skipWhitespace();

        if (! isIdentifierStart (*position))
            fail (errorMessage);

        const auto* begin = position;

        while (isIdentifierPart (*position))
            ++position;

        return String (begin, (size_t) (position - begin));
    }

    //==============================================================================
    ExpressionOperator matchBinaryOperator (int precedence)
    {
        static const std::pair<const char*, ExpressionOperator> operators[] = {
            { "||", ExpressionOperator::logicalOr },
            { "&&", ExpressionOperator::logicalAnd },
            { "==", ExpressionOperator::equal },
            { "!=", ExpressionOperator::notEqual },
            { "<=", ExpressionOperator::lessOrEqual },
            { ">=", ExpressionOperator::greaterOrEqual },
            { "<", ExpressionOperator::less },
            { ">", ExpressionOperator::greater },
            { "+", ExpressionOperator::add },
            { "-", ExpressionOperator::subtract },
            { "*", ExpressionOperator::multiply },
            { "/", ExpressionOperator::divide },
            { "%", ExpressionOperator::modulo }
        };

        for (const auto& [symbol, op] : operators)
            if (getBinaryPrecedence (op) == precedence && match (symbol))
                return op;

        return ExpressionOperator::none;
    }

    bool isOutputAssignment()
    {
        skipWhitespace();

        if (! isIdentifierStart (*position))
            return false;

        const auto* p = position;

        while (isIdentifierPart (*p))
            ++p;

        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
            ++p;

        return p[0] == '=' && p[1] != '=';
    }

    bool match (const char* token)
    {
        skipWhitespace();

        const auto length = std::strlen (token);

        if (std::strncmp (position, token, length) != 0)
            return false;

        position += length;
        return true;
    }

    bool matchKeyword (const char* keyword)
    {
        skipWhitespace();

        const auto length = std::strlen (keyword);

        if (std::strncmp (position, keyword, length) != 0 || isIdentifierPart (position[length]))
            return false;

        position += length;
        return true;
    }

    void expect (const char* token)
    {
        if (! match (token))
            fail ("Expected '" + String (token) + "'");
    }

    bool isAtEnd()
    {
        skipWhitespace();
        return *position == 0;
    }

    void skipWhitespace()
    {
        for (;;)
        {
            const auto c = *position;

            if (c == '\n')
            {
                ++line;
                lineStart = ++position;
            }
            else if (c == ' ' || c == '\t' || c == '\r')
            {
                ++position;
            }
            else if (c == '/' && position[1] == '/')
            {
                while (*position != 0 && *position != '\n')
                    ++position;
            }
            else if (c == '/' && position[1] == '*')
            {
                const auto start = getPosition();

                for (position += 2; ! (position[0] == '*' && position[1] == '/'); ++position)
                {
                    if (*position == 0)
                        fail ("Unterminated comment", start);

                    if (*position == '\n')
                    {
                        ++line;
                        lineStart = position + 1;
                    }
                }

                position += 2;
            }
            else
            {
                return;
            }
        }
    }

    static bool isIdentifierStart (char c) noexcept
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }

    static bool isIdentifierPart (char c) noexcept
    {
        return isIdentifierStart (c) || (c >= '0' && c <= '9');
    }

    Position getPosition() const noexcept
    {
        return { line, (int) (position - lineStart) + 1 };
    }

    [[noreturn]] void fail (const String& message)
    {
        skipWhitespaceSafely();
        fail (message, getPosition());
    }

    [[noreturn]] void fail (const String& message, Position where)
    {
        const auto* p = text;

        for (int l = 1; l < where.first && *p != 0; ++p)
            if (*p == '\n')
                ++l;

        throw Error { message, where.first, where.second, (int) (p - text) + where.second - 1 };
    }

    void skipWhitespaceSafely() noexcept
    {
        while (*position == ' ' || *position == '\t' || *position == '\r' || *position == '\n')
        {
            if (*position == '\n')
            {
                ++line;
                lineStart = position + 1;
            }

            ++position;
        }
    }

    static Expression make (Kind kind, ExpressionOperator op, const String& name, Value value, std::vector<Expression> inputs, Position where)
    {
        return Expression::make (kind, op, name, std::move (value), std::move (inputs), where.first, where.second);
    }

    const char* text;
    const char* position;
    const char* lineStart;
    int line = 1, depth = 0;
};

//==============================================================================
Expression::Expression()
    : Expression (0)
{
}

Expression::Expression (int value)
    : Expression (make (Kind::constant, ExpressionOperator::none, {}, value, {}))
{
}

Expression::Expression (double value)
    : Expression (make (Kind::constant, ExpressionOperator::none, {}, value, {}))
{
}

Expression::Expression (float value)
    : Expression (make (Kind::constant, ExpressionOperator::none, {}, value, {}))
{
}

Expression::Expression (bool value)
    : Expression (make (Kind::constant, ExpressionOperator::none, {}, value, {}))
{
}

Expression::Expression (std::shared_ptr<const Node> n)
    : node (std::move (n))
{
}

Expression Expression::make (Kind kind, ExpressionOperator op, const String& name, Value value, std::vector<Expression> inputs, int line, int column)
{
    auto n = std::make_shared<Node>();
    n->kind = kind;
    n->op = op;
    n->name = name;
    n->value = std::move (value);
    n->inputs = std::move (inputs);
    n->line = line;
    n->column = column;
    return Expression (std::move (n));
}

Expression Expression::text (const String& value)
{
    return make (Kind::constant, ExpressionOperator::none, {}, value, {});
}

Expression Expression::symbol (const String& name)
{
    return make (Kind::symbol, ExpressionOperator::none, name, {}, {});
}

Expression Expression::call (const String& functionName, std::vector<Expression> arguments)
{
    return make (Kind::call, ExpressionOperator::none, functionName, {}, std::move (arguments));
}

//==============================================================================
ResultValue<Expression> Expression::parse (StringRef text)
{
    try
    {
        Parser parser (text);
        return makeResultValueOk (parser.parseProgram());
    }
    catch (const Parser::Error& error)
    {
        return makeResultValueFail (String (error.line) + ":" + String (error.column) + ": " + error.message);
    }
}

ResultValue<Expression> Expression::parseExpression (StringRef text, int& offset)
{
    try
    {
        Parser parser (text, offset);
        auto result = parser.parseSingleExpression();
        offset = parser.getOffset();
        return makeResultValueOk (std::move (result));
    }
    catch (const Parser::Error& error)
    {
        offset = error.offset;
        return makeResultValueFail (String (error.line) + ":" + String (error.column) + ": " + error.message);
    }
}

//==============================================================================
Expression::Kind Expression::getKind() const noexcept { return node->kind; }

const String& Expression::getName() const noexcept { return node->name; }

ExpressionOperator Expression::getOperator() const noexcept { return node->op; }

const Expression::Value& Expression::getValue() const noexcept { return node->value; }

int Expression::getNumInputs() const noexcept { return (int) node->inputs.size(); }

std::pair<int, int> Expression::getPosition() const noexcept { return { node->line, node->column }; }

Expression Expression::getInput (int index) const
{
    jassert (isPositiveAndBelow (index, getNumInputs()));
    return isPositiveAndBelow (index, getNumInputs()) ? node->inputs[(size_t) index] : Expression();
}

var Expression::getConstantValue() const
{
    if (node->kind != Kind::constant)
        return {};

    return std::visit ([] (const auto& value) -> var
    {
        using T = std::decay_t<decltype (value)>;

        if constexpr (std::is_same_v<T, float>)
            return (double) value;
        else
            return value;
    },
                       node->value);
}

//==============================================================================
StringRef Expression::getOperatorSymbol (ExpressionOperator op) noexcept
{
    switch (op)
    {
        case ExpressionOperator::add:
            return "+";
        case ExpressionOperator::subtract:
        case ExpressionOperator::negate:
            return "-";
        case ExpressionOperator::multiply:
            return "*";
        case ExpressionOperator::divide:
            return "/";
        case ExpressionOperator::modulo:
            return "%";
        case ExpressionOperator::power:
            return "^";
        case ExpressionOperator::logicalNot:
            return "!";
        case ExpressionOperator::less:
            return "<";
        case ExpressionOperator::lessOrEqual:
            return "<=";
        case ExpressionOperator::greater:
            return ">";
        case ExpressionOperator::greaterOrEqual:
            return ">=";
        case ExpressionOperator::equal:
            return "==";
        case ExpressionOperator::notEqual:
            return "!=";
        case ExpressionOperator::logicalAnd:
            return "&&";
        case ExpressionOperator::logicalOr:
            return "||";
        case ExpressionOperator::none:
            break;
    }

    return "";
}

String Expression::getOperatorFunctionName (ExpressionOperator op)
{
    return "operator" + String (getOperatorSymbol (op));
}

//==============================================================================
String Expression::toString() const
{
    String result;
    write (result, 0);
    return result;
}

void Expression::write (String& out, int parentPrecedence) const
{
    const auto& n = *node;

    const auto writeList = [&out] (const std::vector<Expression>& items, size_t first, const char* separator)
    {
        for (auto i = first; i < items.size(); ++i)
        {
            if (i > first)
                out << separator;

            items[i].write (out, 0);
        }
    };

    const auto writeWrapped = [&] (int precedence, auto&& body)
    {
        const bool needsParentheses = precedence < parentPrecedence;

        if (needsParentheses)
            out << "(";

        body();

        if (needsParentheses)
            out << ")";
    };

    switch (n.kind)
    {
        case Kind::constant:
        {
            String literal;

            if (const auto* b = std::get_if<bool> (&n.value))
                literal = *b ? "true" : "false";
            else if (const auto* i = std::get_if<int> (&n.value))
                literal = String (*i);
            else if (const auto* f = std::get_if<float> (&n.value))
                literal = formatExpressionFloat (*f, 9) + "f";
            else if (const auto* d = std::get_if<double> (&n.value))
                literal = formatExpressionFloat (*d, 17);
            else
                literal = quoteExpressionString (std::get<String> (n.value));

            writeWrapped (literal.startsWithChar ('-') ? (int) precedenceUnary : (int) precedencePostfix, [&]
            {
                out << literal;
            });
            break;
        }

        case Kind::symbol:
            out << n.name;
            break;

        case Kind::unary:
            writeWrapped (precedenceUnary, [&]
            {
                out << getOperatorSymbol (n.op);
                n.inputs[0].write (out, precedenceUnary);
            });
            break;

        case Kind::binary:
        {
            const auto precedence = getBinaryPrecedence (n.op);
            const bool isPower = n.op == ExpressionOperator::power;

            writeWrapped (precedence, [&]
            {
                n.inputs[0].write (out, isPower ? (int) precedencePostfix : precedence);
                out << " " << getOperatorSymbol (n.op) << " ";
                n.inputs[1].write (out, isPower ? (int) precedenceUnary : precedence + 1);
            });
            break;
        }

        case Kind::ternary:
            writeWrapped (precedenceTernary, [&]
            {
                n.inputs[0].write (out, precedenceOr);
                out << " ? ";
                n.inputs[1].write (out, precedenceTernary);
                out << " : ";
                n.inputs[2].write (out, precedenceTernary);
            });
            break;

        case Kind::call:
            if (const auto* argb = n.name == "color" && n.inputs.size() == 1 && n.inputs[0].node->kind == Kind::constant
                                     ? std::get_if<int> (&n.inputs[0].node->value)
                                     : nullptr)
            {
                const auto value = (uint32) *argb;
                out << "#" << String::toHexString ((int) (value & 0xffffffu)).paddedLeft ('0', 6);

                if ((value >> 24) != 0xffu)
                    out << String::toHexString ((int) (value >> 24)).paddedLeft ('0', 2);

                break;
            }

            out << n.name << " (";
            writeList (n.inputs, 0, ", ");
            out << ")";
            break;

        case Kind::method:
            n.inputs[0].write (out, precedencePostfix);
            out << "." << n.name << " (";
            writeList (n.inputs, 1, ", ");
            out << ")";
            break;

        case Kind::member:
            n.inputs[0].write (out, precedencePostfix);
            out << "." << n.name;
            break;

        case Kind::index:
            n.inputs[0].write (out, precedencePostfix);
            out << "[";
            n.inputs[1].write (out, 0);
            out << "]";
            break;

        case Kind::let:
            out << "let " << n.name << " = ";
            n.inputs[0].write (out, 0);
            break;

        case Kind::output:
            out << n.name << " = ";
            n.inputs[0].write (out, 0);
            break;

        case Kind::block:
            writeList (n.inputs, 0, "; ");
            break;
    }
}

//==============================================================================
StringArray Expression::getReferencedSymbols() const
{
    StringArray symbols, bound;
    collectSymbols (symbols, bound);
    return symbols;
}

bool Expression::referencesSymbol (StringRef name) const
{
    return getReferencedSymbols().contains (name);
}

void Expression::collectSymbols (StringArray& symbols, StringArray& bound) const
{
    const auto& n = *node;

    if (n.kind == Kind::symbol)
    {
        if (! bound.contains (getFirstSymbolSegment (n.name)))
            symbols.addIfNotAlreadyThere (n.name);

        return;
    }

    for (const auto& input : n.inputs)
        input.collectSymbols (symbols, bound);

    if (n.kind == Kind::let || n.kind == Kind::output)
        bound.add (n.name);
}

Expression Expression::withRenamedSymbol (StringRef oldName, const String& newName) const
{
    StringArray bound;
    return renamed (oldName, newName, bound);
}

Expression Expression::renamed (StringRef oldName, const String& newName, StringArray& bound) const
{
    const auto& n = *node;

    if (n.kind == Kind::symbol)
    {
        if (bound.contains (getFirstSymbolSegment (n.name)))
            return *this;

        if (n.name == oldName)
            return make (n.kind, n.op, newName, n.value, {}, n.line, n.column);

        if (n.name.startsWith (String (oldName) + "."))
            return make (n.kind, n.op, newName + n.name.substring (oldName.length()), n.value, {}, n.line, n.column);

        return *this;
    }

    if (n.inputs.empty())
        return *this;

    std::vector<Expression> inputs;
    inputs.reserve (n.inputs.size());

    for (const auto& input : n.inputs)
        inputs.push_back (input.renamed (oldName, newName, bound));

    if (n.kind == Kind::let || n.kind == Kind::output)
        bound.add (n.name);

    return make (n.kind, n.op, n.name, n.value, std::move (inputs), n.line, n.column);
}

//==============================================================================
ResultValue<double> Expression::evaluate() const
{
    return evaluate (ExpressionContext());
}

ResultValue<double> Expression::evaluate (const ExpressionContext& context) const
{
    auto program = CompiledExpression::compile (*this, context);

    if (program.failed())
        return makeResultValueFail (program.getErrorMessage());

    auto outputIndex = program.getReference().findOutput ("result");

    if (outputIndex < 0)
        outputIndex = 0;

    ExpressionState state (program.getReference(), 1);
    state.evaluate();

    const auto value = state.getVar (outputIndex);

    if (! (value.isBool() || value.isInt() || value.isDouble()))
        return makeResultValueFail ("The result of type '" + program.getReference().getOutputType (outputIndex)->getName() + "' is not numeric");

    return makeResultValueOk ((double) value);
}

//==============================================================================
Expression Expression::operator+ (const Expression& other) const { return make (Kind::binary, ExpressionOperator::add, {}, {}, { *this, other }); }

Expression Expression::operator- (const Expression& other) const { return make (Kind::binary, ExpressionOperator::subtract, {}, {}, { *this, other }); }

Expression Expression::operator* (const Expression& other) const { return make (Kind::binary, ExpressionOperator::multiply, {}, {}, { *this, other }); }

Expression Expression::operator/ (const Expression& other) const { return make (Kind::binary, ExpressionOperator::divide, {}, {}, { *this, other }); }

Expression Expression::operator% (const Expression& other) const { return make (Kind::binary, ExpressionOperator::modulo, {}, {}, { *this, other }); }

Expression Expression::operator-() const { return make (Kind::unary, ExpressionOperator::negate, {}, {}, { *this }); }

} // namespace yup
