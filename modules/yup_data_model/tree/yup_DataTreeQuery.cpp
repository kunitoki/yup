/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

struct DataTreeQuery::XPathParser
{
    struct Token
    {
        enum class Type
        {
            Slash,        // /
            DoubleSlash,  // //
            Identifier,   // NodeType, property names
            Star,         // *
            OpenBracket,  // [
            CloseBracket, // ]
            AtSign,       // @
            Equal,        // =
            NotEqual,     // !=
            Greater,      // >
            Less,         // <
            GreaterEqual, // >=
            LessEqual,    // <=
            String,       // 'value' or "value"
            Number,       // 123, 45.67
            And,          // and
            Or,           // or
            Not,          // not
            Function,     // first(), last(), position(), count()
            OpenParen,    // (
            CloseParen,   // )
            EndOfInput
        };

        Type type;
        String value;
        double numericValue = 0.0;
        int position = 0;

        Token (Type t, int pos = 0)
            : type (t)
            , position (pos)
        {
        }

        Token (Type t, const String& val, int pos = 0)
            : type (t)
            , value (val)
            , position (pos)
        {
        }

        Token (Type t, double val, int pos = 0)
            : type (t)
            , numericValue (val)
            , position (pos)
        {
        }
    };

    struct Predicate
    {
        enum Type
        {
            HasProperty,       // [@prop]
            PropertyEquals,    // [@prop='value']
            PropertyNotEquals, // [@prop!='value']
            PropertyGreater,   // [@prop > value]
            PropertyLess,      // [@prop < value]
            PropertyGreaterEqual, // [@prop >= value]
            PropertyLessEqual, // [@prop <= value]
            Position,          // [1], [2], etc.
            First,             // [first()]
            Last,              // [last()]
            And,               // predicate1 and predicate2
            Or,                // predicate1 or predicate2
            Not                // not(predicate)
        };

        Type type;
        String property;
        var value;
        int position = 0;
        std::unique_ptr<Predicate> left;
        std::unique_ptr<Predicate> right;

        Predicate (Type t)
            : type (t)
        {
        }

        Predicate (Type t, const String& prop)
            : type (t)
            , property (prop)
        {
        }

        Predicate (Type t, const String& prop, const var& val)
            : type (t)
            , property (prop)
            , value (val)
        {
        }

        Predicate (Type t, int pos)
            : type (t)
            , position (pos)
        {
        }
    };

    XPathParser (const String& xpath)
        : input (xpath)
    {
        tokenize();
    }

    const Result& getParseResult() const { return parseResult; }

    std::vector<DataTreeQuery::QueryOperation> parse()
    {
        std::vector<DataTreeQuery::QueryOperation> operations;

        if (tokens.empty() || tokens[0].type == Token::Type::EndOfInput)
            return operations;

        if (tokens[0].type == Token::Type::Slash || tokens[0].type == Token::Type::DoubleSlash)
            parseStep (operations);
        else
            parseStep (operations);

        while (currentToken < static_cast<int> (tokens.size()) && tokens[currentToken].type != Token::Type::EndOfInput)
            parseStep (operations);

        return operations;
    }

    bool parse (std::vector<DataTreeQuery::QueryOperation>& operations)
    {
        parseResult = Result::ok();
        operations.clear();

        if (tokens.empty() || tokens[0].type == Token::Type::EndOfInput)
            return true; // Empty query is valid

        // Handle absolute vs relative paths
        if (tokens[0].type == Token::Type::Slash || tokens[0].type == Token::Type::DoubleSlash)
        {
            parseStep (operations);
        }
        else
        {
            // Relative path, assume current context
            parseStep (operations);
        }

        while (currentToken < static_cast<int> (tokens.size()) && tokens[currentToken].type != Token::Type::EndOfInput && parseResult.wasOk())
        {
            parseStep (operations);
        }

        return parseResult.wasOk();
    }

private:
    void parseStep (std::vector<DataTreeQuery::QueryOperation>& operations)
    {
        if (currentToken >= static_cast<int> (tokens.size()))
            return;

        const auto& token = tokens[currentToken];

        if (token.type == Token::Type::Slash)
        {
            ++currentToken;
            parseNodeTest (operations, false); // Direct children
        }
        else if (token.type == Token::Type::DoubleSlash)
        {
            ++currentToken;
            parseNodeTest (operations, true); // All descendants
        }
        else
        {
            ++currentToken;
            parseNodeTest (operations, false); // Default to children
        }
    }

    void parseNodeTest (std::vector<DataTreeQuery::QueryOperation>& operations, bool descendants)
    {
        if (currentToken >= static_cast<int> (tokens.size()))
            return;

        const auto& token = tokens[currentToken];

        if (token.type == Token::Type::Star)
        {
            // Any node type
            if (descendants)
                operations.emplace_back (QueryOperation::Descendants);
            else
                operations.emplace_back (QueryOperation::Children);

            ++currentToken;
        }
        else if (token.type == Token::Type::Identifier)
        {
            // Specific node type
            if (descendants)
                operations.emplace_back (QueryOperation::DescendantsOfType, token.value);
            else
                operations.emplace_back (QueryOperation::ChildrenOfType, token.value);

            ++currentToken;
        }
        else if (token.type == Token::Type::AtSign)
        {
            // Property selection
            ++currentToken;
            if (currentToken < static_cast<int> (tokens.size()) && tokens[currentToken].type == Token::Type::Identifier)
            {
                operations.emplace_back (QueryOperation::Property, tokens[currentToken].value);
                ++currentToken;
            }
            else
            {
                // Error: @ not followed by identifier
                parseResult = Result::fail("Expected property name after '@' in node test");
                return;
            }

            return; // Property selection terminates node traversal
        }
        else if (token.type == Token::Type::Function && token.value == "text")
        {
            // text() function - select text property
            ++currentToken;
            
            // Skip parentheses for text()
            if (currentToken < static_cast<int> (tokens.size()) && tokens[currentToken].type == Token::Type::OpenParen)
            {
                ++currentToken;
                if (currentToken < static_cast<int> (tokens.size()) && tokens[currentToken].type == Token::Type::CloseParen)
                    ++currentToken;
            }
            
            operations.emplace_back (QueryOperation::Property, "text");
            return; // text() selection terminates node traversal
        }
        else if (token.type == Token::Type::OpenBracket)
        {
            // Unexpected bracket without node test
            parseResult = Result::fail("Unexpected '[' without preceding node selector");
            return;
        }

        // Parse predicates
        while (currentToken < static_cast<int> (tokens.size()) && tokens[currentToken].type == Token::Type::OpenBracket)
        {
            parsePredicate (operations);
        }
    }

    void parsePredicate (std::vector<DataTreeQuery::QueryOperation>& operations)
    {
        if (currentToken >= static_cast<int> (tokens.size()) || tokens[currentToken].type != Token::Type::OpenBracket)
            return;

        ++currentToken; // Skip '['

        auto predicate = parsePredicateExpression();
        if (predicate)
            addPredicateOperation (operations, std::move (predicate));
        else
        {
            // Error parsing predicate expression
            parseResult = Result::fail("Invalid predicate expression inside brackets");
            return;
        }

        // Skip ']' - this must be present
        if (currentToken < static_cast<int> (tokens.size()) && tokens[currentToken].type == Token::Type::CloseBracket)
            ++currentToken;
        else
        {
            // Error: missing closing bracket
            parseResult = Result::fail("Missing closing bracket ']' in predicate");
            return;
        }
    }

    std::unique_ptr<Predicate> parsePredicateExpression()
    {
        return parseOrExpression();
    }

    std::unique_ptr<Predicate> parseOrExpression()
    {
        auto left = parseAndExpression();

        while (currentToken < static_cast<int> (tokens.size()) && tokens[currentToken].type == Token::Type::Or)
        {
            ++currentToken;
            auto right = parseAndExpression();

            auto orPredicate = std::make_unique<Predicate> (Predicate::Or);
            orPredicate->left = std::move (left);
            orPredicate->right = std::move (right);
            left = std::move (orPredicate);
        }

        return left;
    }

    std::unique_ptr<Predicate> parseAndExpression()
    {
        auto left = parseNotExpression();

        while (currentToken < static_cast<int> (tokens.size()) && tokens[currentToken].type == Token::Type::And)
        {
            ++currentToken;
            auto right = parseNotExpression();

            auto andPredicate = std::make_unique<Predicate> (Predicate::And);
            andPredicate->left = std::move (left);
            andPredicate->right = std::move (right);
            left = std::move (andPredicate);
        }

        return left;
    }

    std::unique_ptr<Predicate> parseNotExpression()
    {
        if (currentToken < static_cast<int> (tokens.size()) && tokens[currentToken].type == Token::Type::Not)
        {
            ++currentToken;

            // Skip optional '('
            if (currentToken < static_cast<int> (tokens.size()) && tokens[currentToken].type == Token::Type::OpenParen)
                ++currentToken;

            auto inner = parsePrimaryExpression();

            // Skip optional ')'
            if (currentToken < static_cast<int> (tokens.size()) && tokens[currentToken].type == Token::Type::CloseParen)
                ++currentToken;

            auto notPredicate = std::make_unique<Predicate> (Predicate::Not);
            notPredicate->left = std::move (inner);
            return notPredicate;
        }

        return parsePrimaryExpression();
    }

    std::unique_ptr<Predicate> parsePrimaryExpression()
    {
        if (currentToken >= static_cast<int> (tokens.size()))
            return nullptr;

        const auto& token = tokens[currentToken];

        if (token.type == Token::Type::Number)
        {
            ++currentToken;
            return std::make_unique<Predicate> (Predicate::Position, static_cast<int> (token.numericValue));
        }
        else if (token.type == Token::Type::Function)
        {
            ++currentToken;

            if (token.value == "first")
            {
                return std::make_unique<Predicate> (Predicate::First);
            }
            else if (token.value == "last")
            {
                return std::make_unique<Predicate> (Predicate::Last);
            }
            else if (token.value == "position")
            {
                // Skip parentheses for position()
                if (currentToken < static_cast<int> (tokens.size()) && tokens[currentToken].type == Token::Type::OpenParen)
                {
                    ++currentToken;
                    if (currentToken < static_cast<int> (tokens.size()) && tokens[currentToken].type == Token::Type::CloseParen)
                        ++currentToken;
                }

                // For now, treat position() as position(1) - could be enhanced
                return std::make_unique<Predicate> (Predicate::Position, 1);
            }
        }
        else if (token.type == Token::Type::AtSign)
        {
            ++currentToken;
            if (currentToken < static_cast<int> (tokens.size()) && tokens[currentToken].type == Token::Type::Identifier)
            {
                String propertyName = tokens[currentToken].value;
                ++currentToken;

                // Check for equality/inequality
                if (currentToken < static_cast<int> (tokens.size()))
                {
                    if (tokens[currentToken].type == Token::Type::Equal)
                    {
                        ++currentToken;
                        auto value = parseValue();
                        if (!isValidValue(value))
                        {
                            parseResult = Result::fail("Expected value after comparison operator");
                            return nullptr;
                        }
                        return std::make_unique<Predicate> (Predicate::PropertyEquals, propertyName, value);
                    }
                    else if (tokens[currentToken].type == Token::Type::NotEqual)
                    {
                        ++currentToken;
                        auto value = parseValue();
                        if (!isValidValue(value))
                        {
                            parseResult = Result::fail("Expected value after comparison operator");
                            return nullptr;
                        }
                        return std::make_unique<Predicate> (Predicate::PropertyNotEquals, propertyName, value);
                    }
                    else if (tokens[currentToken].type == Token::Type::Greater)
                    {
                        ++currentToken;
                        auto value = parseValue();
                        if (!isValidValue(value))
                        {
                            parseResult = Result::fail("Expected value after comparison operator");
                            return nullptr;
                        }
                        return std::make_unique<Predicate> (Predicate::PropertyGreater, propertyName, value);
                    }
                    else if (tokens[currentToken].type == Token::Type::Less)
                    {
                        ++currentToken;
                        auto value = parseValue();
                        if (!isValidValue(value))
                        {
                            parseResult = Result::fail("Expected value after comparison operator");
                            return nullptr;
                        }
                        return std::make_unique<Predicate> (Predicate::PropertyLess, propertyName, value);
                    }
                    else if (tokens[currentToken].type == Token::Type::GreaterEqual)
                    {
                        ++currentToken;
                        auto value = parseValue();
                        if (!isValidValue(value))
                        {
                            parseResult = Result::fail("Expected value after comparison operator");
                            return nullptr;
                        }
                        return std::make_unique<Predicate> (Predicate::PropertyGreaterEqual, propertyName, value);
                    }
                    else if (tokens[currentToken].type == Token::Type::LessEqual)
                    {
                        ++currentToken;
                        auto value = parseValue();
                        if (!isValidValue(value))
                        {
                            parseResult = Result::fail("Expected value after comparison operator");
                            return nullptr;
                        }
                        return std::make_unique<Predicate> (Predicate::PropertyLessEqual, propertyName, value);
                    }
                }

                // Just checking for property existence
                return std::make_unique<Predicate> (Predicate::HasProperty, propertyName);
            }
            else
            {
                // Error: @ not followed by identifier in predicate
                parseResult = Result::fail("Expected property name after '@' in predicate");
                return nullptr;
            }
        }

        return nullptr;
    }

    var parseValue()
    {
        if (currentToken >= static_cast<int> (tokens.size()))
            return {};

        const auto& token = tokens[currentToken];

        if (token.type == Token::Type::String)
        {
            ++currentToken;

            return var (token.value);
        }
        else if (token.type == Token::Type::Number)
        {
            ++currentToken;

            return var (token.numericValue);
        }
        else if (token.type == Token::Type::Identifier)
        {
            ++currentToken;

            // Handle boolean literals
            if (token.value == "true")
                return var (true);
            else if (token.value == "false")
                return var (false);
            else
                return var (token.value);
        }

        return {};
    }
    
    bool isValidValue(const var& value) const
    {
        // Check if the value is valid (not empty/null in meaningful way)
        // For XPath parsing, any parsed value should be valid
        // But if parseValue() was called and no value was found, it returns empty var
        return !value.isVoid();
    }

    void addPredicateOperation (std::vector<DataTreeQuery::QueryOperation>& operations,
                                std::unique_ptr<Predicate> predicate)
    {
        QueryOperation op (QueryOperation::Where);

        auto predicatePtr = std::shared_ptr<Predicate> (std::move (predicate));
        
        // Store the predicate for position-aware evaluation
        op.xpathPredicate = predicatePtr;
        
        operations.push_back (std::move (op));
    }


public:
    static bool evaluatePredicate (const Predicate& predicate, const DataTree& node, int position, int totalCount)
    {
        switch (predicate.type)
        {
            case Predicate::HasProperty:
                return node.hasProperty (predicate.property);

            case Predicate::PropertyEquals:
                return node.hasProperty (predicate.property) && node.getProperty (predicate.property) == predicate.value;

            case Predicate::PropertyNotEquals:
                return ! node.hasProperty (predicate.property) || node.getProperty (predicate.property) != predicate.value;

            case Predicate::PropertyGreater:
                if (!node.hasProperty (predicate.property))
                    return false;
                return node.getProperty (predicate.property) > predicate.value;

            case Predicate::PropertyLess:
                if (!node.hasProperty (predicate.property))
                    return false;
                return node.getProperty (predicate.property) < predicate.value;

            case Predicate::PropertyGreaterEqual:
                if (!node.hasProperty (predicate.property))
                    return false;
                return node.getProperty (predicate.property) >= predicate.value;

            case Predicate::PropertyLessEqual:
                if (!node.hasProperty (predicate.property))
                    return false;
                return node.getProperty (predicate.property) <= predicate.value;

            case Predicate::Position:
                return position == predicate.position - 1; // XPath is 1-indexed

            case Predicate::First:
                return position == 0;

            case Predicate::Last:
                return position == totalCount - 1;

            case Predicate::And:
                return predicate.left && predicate.right && evaluatePredicate (*predicate.left, node, position, totalCount) && evaluatePredicate (*predicate.right, node, position, totalCount);

            case Predicate::Or:
                return predicate.left && predicate.right && (evaluatePredicate (*predicate.left, node, position, totalCount) || evaluatePredicate (*predicate.right, node, position, totalCount));

            case Predicate::Not:
                return predicate.left && ! evaluatePredicate (*predicate.left, node, position, totalCount);
        }

        return false;
    }

private:
    void tokenize()
    {
        pos = 0;
        parseResult = Result::ok();

        while (pos < input.length() && parseResult.wasOk())
        {
            skipWhitespace();

            if (pos >= input.length())
                break;

            char ch = input[pos];
            int tokenStart = pos;

            switch (ch)
            {
                case '/':
                    if (pos + 1 < input.length() && input[pos + 1] == '/')
                    {
                        tokens.emplace_back (Token::Type::DoubleSlash, tokenStart);
                        pos += 2;
                    }
                    else
                    {
                        tokens.emplace_back (Token::Type::Slash, tokenStart);
                        ++pos;
                    }
                    break;

                case '*':
                    tokens.emplace_back (Token::Type::Star, tokenStart);
                    ++pos;
                    break;

                case '[':
                    tokens.emplace_back (Token::Type::OpenBracket, tokenStart);
                    ++pos;
                    break;

                case ']':
                    tokens.emplace_back (Token::Type::CloseBracket, tokenStart);
                    ++pos;
                    break;

                case '@':
                    tokens.emplace_back (Token::Type::AtSign, tokenStart);
                    ++pos;
                    break;

                case '=':
                    tokens.emplace_back (Token::Type::Equal, tokenStart);
                    ++pos;
                    break;

                case '!':
                    if (pos + 1 < input.length() && input[pos + 1] == '=')
                    {
                        tokens.emplace_back (Token::Type::NotEqual, tokenStart);
                        pos += 2;
                    }
                    else
                    {
                        ++pos; // Skip invalid character
                    }
                    break;

                case '>':
                    if (pos + 1 < input.length() && input[pos + 1] == '=')
                    {
                        tokens.emplace_back (Token::Type::GreaterEqual, tokenStart);
                        pos += 2;
                    }
                    else
                    {
                        tokens.emplace_back (Token::Type::Greater, tokenStart);
                        ++pos;
                    }
                    break;

                case '<':
                    if (pos + 1 < input.length() && input[pos + 1] == '=')
                    {
                        tokens.emplace_back (Token::Type::LessEqual, tokenStart);
                        pos += 2;
                    }
                    else
                    {
                        tokens.emplace_back (Token::Type::Less, tokenStart);
                        ++pos;
                    }
                    break;

                case '(':
                    tokens.emplace_back (Token::Type::OpenParen, tokenStart);
                    ++pos;
                    break;

                case ')':
                    tokens.emplace_back (Token::Type::CloseParen, tokenStart);
                    ++pos;
                    break;

                case '\'':
                case '"':
                    tokenizeString();
                    break;

                default:
                    if (std::isdigit (ch))
                        tokenizeNumber();
                    else if (std::isalpha (ch) || ch == '_')
                        tokenizeIdentifier();
                    else
                        ++pos; // Skip unknown character
                    break;
            }
        }

        if (parseResult.wasOk())
            tokens.emplace_back (Token::Type::EndOfInput, pos);
    }

    void skipWhitespace()
    {
        while (pos < input.length() && std::isspace (input[pos]))
            ++pos;
    }

    void tokenizeString()
    {
        char quote = input[pos];
        int start = pos++;
        String value;

        while (pos < input.length() && input[pos] != quote)
            value += input[pos++];

        if (pos < input.length())
        {
            ++pos; // Skip closing quote
            tokens.emplace_back (Token::Type::String, value, start);
        }
        else
        {
            // Error: Unmatched quote
            parseResult = Result::fail("Unmatched quote in string literal");
        }
    }

    void tokenizeNumber()
    {
        int start = pos;
        String number;

        while (pos < input.length() && (std::isdigit (input[pos]) || input[pos] == '.'))
            number += input[pos++];

        tokens.emplace_back (Token::Type::Number, number.getDoubleValue(), start);
    }

    void tokenizeIdentifier()
    {
        int start = pos;
        String identifier;

        while (pos < input.length() && (std::isalnum (input[pos]) || input[pos] == '_'))
            identifier += input[pos++];

        if (identifier == "and")
            tokens.emplace_back (Token::Type::And, start);

        else if (identifier == "or")
            tokens.emplace_back (Token::Type::Or, start);

        else if (identifier == "not")
            tokens.emplace_back (Token::Type::Not, start);

        else if (identifier == "first" || identifier == "last" || identifier == "position" || identifier == "count" || identifier == "text")
            tokens.emplace_back (Token::Type::Function, identifier, start);

        else
            tokens.emplace_back (Token::Type::Identifier, identifier, start);
    }

    String input;
    int pos = 0;
    std::vector<Token> tokens;
    int currentToken = 0;
    Result parseResult = Result::ok();
};

//==============================================================================

DataTreeQuery::QueryResult::QueryResult()
    : evaluated (true)
{
}

DataTreeQuery::QueryResult::QueryResult (std::vector<DataTree> nodes)
    : cachedNodes (std::move (nodes))
    , evaluated (true)
{
}

DataTreeQuery::QueryResult::QueryResult (std::vector<var> properties)
    : cachedProperties (std::move (properties))
    , evaluated (true)
{
}

DataTreeQuery::QueryResult::QueryResult (std::function<std::vector<DataTree>()> evaluator)
    : evaluator (std::move (evaluator))
    , evaluated (false)
{
}

int DataTreeQuery::QueryResult::size() const
{
    ensureEvaluated();

    return static_cast<int> (cachedNodes.size());
}

const DataTree& DataTreeQuery::QueryResult::getNode (int index) const
{
    ensureEvaluated();

    jassert (index >= 0 && index < static_cast<int> (cachedNodes.size()));

    return cachedNodes[static_cast<size_t> (index)];
}

const var& DataTreeQuery::QueryResult::getProperty (int index) const
{
    jassert (index >= 0 && index < static_cast<int> (cachedProperties.size()));

    return cachedProperties[static_cast<size_t> (index)];
}

std::vector<DataTree> DataTreeQuery::QueryResult::nodes() const
{
    ensureEvaluated();

    return cachedNodes;
}

DataTree DataTreeQuery::QueryResult::node() const
{
    ensureEvaluated();

    return cachedNodes.empty() ? DataTree() : cachedNodes[0];
}

std::vector<var> DataTreeQuery::QueryResult::properties() const
{
    if (! cachedProperties.empty())
        return cachedProperties;

    ensureEvaluated();

    return cachedProperties;
}

StringArray DataTreeQuery::QueryResult::strings() const
{
    auto props = properties();

    StringArray result;
    result.ensureStorageAllocated (static_cast<int> (props.size()));

    for (const auto& prop : props)
        result.add (prop.toString());

    return result;
}

void DataTreeQuery::QueryResult::ensureEvaluated() const
{
    if (! evaluated && evaluator)
    {
        cachedNodes = evaluator();
        evaluated = true;
    }
}

//==============================================================================

DataTreeQuery::DataTreeQuery() = default;

DataTreeQuery DataTreeQuery::from (const DataTree& root)
{
    DataTreeQuery query;
    query.rootNode = root;
    return query;
}

DataTreeQuery::QueryResult DataTreeQuery::xpath (const DataTree& root, const String& query)
{
    return DataTreeQuery::from (root).xpath (query).execute();
}

DataTreeQuery& DataTreeQuery::root (const DataTree& newRoot)
{
    operations.clear();
    rootNode = newRoot;
    return *this;
}

DataTreeQuery& DataTreeQuery::xpath (const String& query)
{
    std::vector<QueryOperation> xpathOps;

    XPathParser parser (query);
    if (! parser.parse (xpathOps))
    {
        operations.clear();
        rootNode = DataTree();
        return *this;
    }
    
    for (auto& op : xpathOps)
        operations.push_back (std::move (op));

    return *this;
}

DataTreeQuery& DataTreeQuery::children()
{
    return addOperation (QueryOperation (QueryOperation::Children));
}

DataTreeQuery& DataTreeQuery::children (const Identifier& type)
{
    return addOperation (QueryOperation (QueryOperation::ChildrenOfType, type.toString()));
}

DataTreeQuery& DataTreeQuery::descendants()
{
    return addOperation (QueryOperation (QueryOperation::Descendants));
}

DataTreeQuery& DataTreeQuery::descendants (const Identifier& type)
{
    return addOperation (QueryOperation (QueryOperation::DescendantsOfType, type.toString()));
}

DataTreeQuery& DataTreeQuery::parent()
{
    return addOperation (QueryOperation (QueryOperation::Parent));
}

DataTreeQuery& DataTreeQuery::ancestors()
{
    return addOperation (QueryOperation (QueryOperation::Ancestors));
}

DataTreeQuery& DataTreeQuery::siblings()
{
    return addOperation (QueryOperation (QueryOperation::Siblings));
}

DataTreeQuery& DataTreeQuery::ofType (const Identifier& type)
{
    return addOperation (QueryOperation (QueryOperation::OfType, type.toString()));
}

DataTreeQuery& DataTreeQuery::hasProperty (const Identifier& propertyName)
{
    return addOperation (QueryOperation (QueryOperation::HasProperty, propertyName.toString()));
}

DataTreeQuery& DataTreeQuery::propertyEquals (const Identifier& propertyName, const var& value)
{
    return addOperation (QueryOperation (QueryOperation::PropertyEquals, propertyName.toString(), value));
}

DataTreeQuery& DataTreeQuery::propertyNotEquals (const Identifier& propertyName, const var& value)
{
    return addOperation (QueryOperation (QueryOperation::PropertyNotEquals, propertyName.toString(), value));
}

DataTreeQuery& DataTreeQuery::property (const Identifier& propertyName)
{
    return addOperation (QueryOperation (QueryOperation::Property, propertyName.toString()));
}

DataTreeQuery& DataTreeQuery::properties (const std::initializer_list<Identifier>& propertyNames)
{
    StringArray names;
    for (const auto& name : propertyNames)
        names.add (name.toString());

    return addOperation (QueryOperation (QueryOperation::Properties, var (names)));
}

DataTreeQuery& DataTreeQuery::take (int count)
{
    return addOperation (QueryOperation (QueryOperation::Take, count));
}

DataTreeQuery& DataTreeQuery::skip (int count)
{
    return addOperation (QueryOperation (QueryOperation::Skip, count));
}

DataTreeQuery& DataTreeQuery::at (const std::initializer_list<int>& positions)
{
    Array<var> posArray;
    for (int pos : positions)
        posArray.add (var (pos));

    return addOperation (QueryOperation (QueryOperation::At, var (posArray)));
}

DataTreeQuery& DataTreeQuery::first()
{
    return addOperation (QueryOperation (QueryOperation::First));
}

DataTreeQuery& DataTreeQuery::last()
{
    return addOperation (QueryOperation (QueryOperation::Last));
}

DataTreeQuery& DataTreeQuery::orderByProperty (const Identifier& propertyName)
{
    return addOperation (QueryOperation (QueryOperation::OrderByProperty, propertyName.toString()));
}

DataTreeQuery& DataTreeQuery::reverse()
{
    return addOperation (QueryOperation (QueryOperation::Reverse));
}

DataTreeQuery& DataTreeQuery::distinct()
{
    return addOperation (QueryOperation (QueryOperation::Distinct));
}

DataTreeQuery::QueryResult DataTreeQuery::execute() const
{
    // Capture data by value to avoid lifetime issues
    auto capturedOperations = operations;
    auto capturedRootNode = rootNode;

    return QueryResult ([capturedOperations, capturedRootNode]()
    {
        std::vector<DataTree> result;

        // Start with the root node if available
        if (capturedRootNode.isValid())
            result.push_back (capturedRootNode);

        // If we have no root node and no operations, return empty
        if (result.empty() && capturedOperations.empty())
            return result;

        // Execute operations sequentially using captured data
        for (const auto& op : capturedOperations)
            result = DataTreeQuery::applyOperation (op, result, capturedRootNode);

        return result;
    });
}

DataTreeQuery& DataTreeQuery::addOperation (QueryOperation operation)
{
    operations.push_back (std::move (operation));
    return *this;
}

std::vector<DataTree> DataTreeQuery::executeOperations() const
{
    std::vector<DataTree> result;

    // Start with the root node if available
    if (rootNode.isValid())
        result.push_back (rootNode);

    // If we have no root node and no operations, return empty
    if (result.empty() && operations.empty())
        return result;

    // Execute operations sequentially
    for (const auto& op : operations)
        result = applyOperation (op, result, rootNode);

    return result;
}


std::vector<DataTree> DataTreeQuery::applyOperation (const QueryOperation& op, const std::vector<DataTree>& input, const DataTree& rootNode)
{
    std::vector<DataTree> result;

    switch (op.type)
    {
        case QueryOperation::Root:
        {
            result = input;
            break;
        }

        case QueryOperation::Children:
        {
            for (const auto& node : input)
            {
                for (int i = 0; i < node.getNumChildren(); ++i)
                    result.push_back (node.getChild (i));
            }

            break;
        }

        case QueryOperation::ChildrenOfType:
        {
            Identifier type (op.parameter1.toString());
            for (const auto& node : input)
            {
                for (int i = 0; i < node.getNumChildren(); ++i)
                {
                    auto child = node.getChild (i);
                    if (child.getType() == type)
                        result.push_back (child);
                }
            }

            break;
        }

        case QueryOperation::Descendants:
        {
            for (const auto& node : input)
            {
                std::function<void(const DataTree&)> traverse = [&](const DataTree& current)
                {
                    const int numChildren = current.getNumChildren();
                    for (int i = 0; i < numChildren; ++i)
                    {
                        auto child = current.getChild(i);
                        if (child.isValid())
                        {
                            result.push_back(child);
                            traverse(child);  // Recursively process child
                        }
                    }
                };

                traverse(node);
            }

            break;
        }

        case QueryOperation::DescendantsOfType:
        {
            Identifier type (op.parameter1.toString());
            
            for (const auto& node : input)
            {
                std::function<void(const DataTree&)> traverse = [&](const DataTree& current)
                {
                    const int numChildren = current.getNumChildren();
                    for (int i = 0; i < numChildren; ++i)
                    {
                        auto child = current.getChild(i);
                        if (child.isValid())
                        {
                            if (child.getType() == type)
                                result.push_back(child);
                            traverse(child);  // Recursively process child
                        }
                    }
                };

                traverse(node);
            }

            break;
        }

        case QueryOperation::Parent:
        {
            for (const auto& node : input)
            {
                auto parent = node.getParent();
                if (parent.isValid())
                    result.push_back (parent);
            }

            break;
        }

        case QueryOperation::Ancestors:
        {
            for (const auto& node : input)
            {
                auto parent = node.getParent();
                while (parent.isValid())
                {
                    result.push_back(parent);
                    parent = parent.getParent();
                }
            }

            break;
        }

        case QueryOperation::Siblings:
        {
            for (const auto& node : input)
            {
                auto parent = node.getParent();
                if (parent.isValid())
                {
                    for (int i = 0; i < parent.getNumChildren(); ++i)
                    {
                        auto sibling = parent.getChild (i);
                        if (sibling != node)
                            result.push_back (sibling);
                    }
                }
            }

            break;
        }

        case QueryOperation::Where:
        {
            if (op.xpathPredicate)
            {
                // XPath predicate with position information
                auto predicate = std::static_pointer_cast<XPathParser::Predicate>(op.xpathPredicate);
                int totalCount = static_cast<int>(input.size());
                for (int i = 0; i < static_cast<int>(input.size()); ++i)
                {
                    const auto& node = input[i];
                    if (XPathParser::evaluatePredicate(*predicate, node, i, totalCount))
                        result.push_back (node);
                }
            }
            else if (op.predicate)
            {
                // Regular predicate (from fluent API)
                for (const auto& node : input)
                {
                    if (op.predicate (node))
                        result.push_back (node);
                }
            }
            else
            {
                result = input;
            }

            break;
        }

        case QueryOperation::OfType:
        {
            Identifier type (op.parameter1.toString());
            for (const auto& node : input)
            {
                if (node.getType() == type)
                    result.push_back (node);
            }

            break;
        }

        case QueryOperation::HasProperty:
        {
            Identifier propertyName (op.parameter1.toString());
            for (const auto& node : input)
            {
                if (node.hasProperty (propertyName))
                    result.push_back (node);
            }

            break;
        }

        case QueryOperation::PropertyEquals:
        {
            Identifier propertyName (op.parameter1.toString());
            const var& value = op.parameter2;

            for (const auto& node : input)
            {
                if (node.hasProperty (propertyName) && node.getProperty (propertyName) == value)
                    result.push_back (node);
            }

            break;
        }

        case QueryOperation::PropertyNotEquals:
        {
            Identifier propertyName (op.parameter1.toString());
            const var& value = op.parameter2;

            for (const auto& node : input)
            {
                if (! node.hasProperty (propertyName) || node.getProperty (propertyName) != value)
                    result.push_back (node);
            }

            break;
        }

        case QueryOperation::PropertyWhere:
        {
            if (op.predicate)
            {
                for (const auto& node : input)
                {
                    if (op.predicate (node))
                        result.push_back (node);
                }
            }
            else
            {
                result = input;
            }

            break;
        }

        case QueryOperation::Property:
        {
            // Property operations are handled differently - they don't return DataTree nodes
            // For now, just pass through unchanged (this case should be handled at a higher level)
            result = input;
            break;
        }

        case QueryOperation::Properties:
        {
            // Properties operations are handled differently - they don't return DataTree nodes
            // For now, just pass through unchanged (this case should be handled at a higher level)
            result = input;
            break;
        }

        case QueryOperation::Select:
        {
            // Select operations transform nodes but for DataTree queries we pass through
            result = input;
            break;
        }

        case QueryOperation::At:
        {
            Array<var> positions = *op.parameter1.getArray();
            for (auto& posVar : positions)
            {
                int pos = static_cast<int> (posVar);
                if (pos >= 0 && pos < static_cast<int> (input.size()))
                    result.push_back (input[static_cast<size_t> (pos)]);
            }
            break;
        }

        case QueryOperation::OrderBy:
        {
            // OrderBy with custom transformer - for DataTree queries just pass through
            result = input;
            break;
        }

        case QueryOperation::Take:
        {
            int count = static_cast<int> (op.parameter1);
            if (count >= 0 && count < static_cast<int> (input.size()))
                result.assign (input.begin(), input.begin() + count);
            else
                result = input;

            break;
        }

        case QueryOperation::Skip:
        {
            int count = static_cast<int> (op.parameter1);
            if (count >= 0 && count < static_cast<int> (input.size()))
                result.assign (input.begin() + count, input.end());
            else if (count <= 0)
                result = input;

            break;
        }

        case QueryOperation::First:
        {
            if (! input.empty())
                result.push_back (input.front());

            break;
        }

        case QueryOperation::Last:
        {
            if (! input.empty())
                result.push_back (input.back());

            break;
        }

        case QueryOperation::Reverse:
        {
            result = input;
            std::reverse (result.begin(), result.end());

            break;
        }

        case QueryOperation::Distinct:
        {
            std::vector<DataTree> seen;
            for (const auto& node : input)
            {
                // Use DataTree equality comparison to check for duplicates
                auto it = std::find (seen.begin(), seen.end(), node);
                if (it == seen.end())
                {
                    seen.push_back (node);
                    result.push_back (node);
                }
            }

            break;
        }

        case QueryOperation::OrderByProperty:
        {
            result = input;
            Identifier propertyName (op.parameter1.toString());

            std::sort (result.begin(), result.end(), [&propertyName] (const DataTree& a, const DataTree& b)
            {
                auto valueA = a.getProperty (propertyName);
                auto valueB = b.getProperty (propertyName);

                // Handle comparison based on type
                if (valueA.isString() && valueB.isString())
                    return valueA.toString() < valueB.toString();
                else if (valueA.isDouble() && valueB.isDouble())
                    return static_cast<double> (valueA) < static_cast<double> (valueB);
                else if (valueA.isInt() && valueB.isInt())
                    return static_cast<int> (valueA) < static_cast<int> (valueB);
                else
                    return valueA.toString() < valueB.toString();
            });

            break;
        }

        default:
        {
            result = input;
            break;
        }
    }

    return result;
}

} // namespace yup
