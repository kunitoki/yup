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

namespace
{

bool isDigit (yup_wchar c)
{
    return c >= '0' && c <= '9';
}

bool isHexDigit (yup_wchar c)
{
    return isDigit (c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

/** String indexing is linear on UTF-8, so a scanned line is decoded once and then indexed in constant time. */
class LineText
{
public:
    explicit LineText (const String& lineText)
        : text (lineText)
    {
        characters.reserve (text.getCharPointer().sizeInBytes() / sizeof (String::CharPointerType::CharType));

        for (auto p = text.getCharPointer(); ! p.isEmpty();)
            characters.push_back (p.getAndAdvance());

        characters.push_back (0);
    }

    const String& getString() const noexcept
    {
        return text;
    }

    int length() const noexcept
    {
        return static_cast<int> (characters.size()) - 1;
    }

    /** Returns the character at an index, or the terminating null at the end of the line. */
    yup_wchar operator[] (int index) const noexcept
    {
        return characters[static_cast<size_t> (index)];
    }

    String substring (int start, int end) const
    {
        if (end <= start)
            return {};

        return String (CharPointer_UTF32 (characters.data() + start), CharPointer_UTF32 (characters.data() + end));
    }

private:
    const String text;
    std::vector<yup_wchar> characters;
};

bool matchesAt (const LineText& text, StringRef pattern, int pos)
{
    for (auto p = pattern.text; ! p.isEmpty(); ++pos)
    {
        if (pos >= text.length() || text[pos] != p.getAndAdvance())
            return false;
    }

    return true;
}

int findPattern (const LineText& text, StringRef pattern, int from)
{
    if (pattern.isEmpty())
        return -1;

    const int lastStart = text.length() - pattern.length();

    for (int i = jmax (0, from); i <= lastStart; ++i)
    {
        if (matchesAt (text, pattern, i))
            return i;
    }

    return -1;
}

int findUnescaped (const LineText& text, StringRef close, int from, yup_wchar escapeCharacter)
{
    const int lastStart = text.length() - close.length();

    for (int i = jmax (0, from); i <= lastStart; ++i)
    {
        if (escapeCharacter != 0 && text[(i)] == escapeCharacter)
        {
            i += 1;
            continue;
        }

        if (matchesAt (text, close, i))
            return i;
    }

    return -1;
}

int parseRawStringDelimiter (const LineText& text, int quotePos)
{
    const int length = text.length();
    const int delimiterStart = quotePos + 1;

    int i = delimiterStart;
    while (i < length && i - delimiterStart < 16)
    {
        const auto character = text[(i)];

        if (character == '(')
            return i - delimiterStart;

        if (character == ')' || character == '\\' || CharacterFunctions::isWhitespace (character))
            return -1;

        ++i;
    }

    return -1;
}

int findRawStringClose (const LineText& text, StringRef delimiter, int from)
{
    const int length = text.length();
    const int closingLength = 1 + delimiter.length() + 1;

    for (int i = jmax (0, from); i + closingLength <= length; ++i)
    {
        if (text[(i)] != ')')
            continue;

        if (matchesAt (text, delimiter, i + 1) && text[(i + 1 + delimiter.length())] == '"')
            return i;
    }

    return -1;
}

using Token = CodeTokeniser::Token;
using StateFrame = CodeTokeniser::StateFrame;

const Identifier& getTokenTypeScope (SyntaxDefinition::TokenType type)
{
    static const auto scopes = []
    {
        std::array<Identifier, static_cast<size_t> (SyntaxDefinition::TokenType::other) + 1> result;
        for (size_t i = 0; i < result.size(); ++i)
            result[i] = Identifier (SyntaxDefinition::tokenTypeToString (static_cast<SyntaxDefinition::TokenType> (i)));

        return result;
    }();

    return scopes[static_cast<size_t> (type)];
}

void pushScopedToken (std::vector<Token>& tokens, int start, int end, const Identifier& scope, bool mergeWithPrevious = false)
{
    if (end <= start)
        return;

    if (mergeWithPrevious && ! tokens.empty() && tokens.back().end == start && tokens.back().scope == scope)
    {
        tokens.back().end = end;
        return;
    }

    tokens.push_back ({ start, end, SyntaxDefinition::scopeToTokenType (scope), scope });
}

constexpr int maxRegexLineLength = 10000;

void addMatchTokens (std::vector<Token>& tokens,
                     const CodeRegex::Match& match,
                     const SyntaxDefinition::Rule& rule,
                     const Identifier& baseScope)
{
    const auto matched = match.groups[0];

    std::vector<std::pair<Range<int>, Identifier>> captures;
    for (const auto& [group, scope] : rule.captures)
    {
        if (isPositiveAndBelow (group, static_cast<int> (match.groups.size())) && match.groups[static_cast<size_t> (group)].getStart() >= 0)
            captures.emplace_back (match.groups[static_cast<size_t> (group)], scope);
    }

    std::sort (captures.begin(), captures.end(), [] (const auto& a, const auto& b)
    {
        return a.first.getStart() < b.first.getStart();
    });

    int cursor = matched.getStart();
    for (const auto& [range, scope] : captures)
    {
        if (range.getStart() < cursor)
            continue;

        pushScopedToken (tokens, cursor, range.getStart(), baseScope);
        pushScopedToken (tokens, range.getStart(), range.getEnd(), scope);
        cursor = range.getEnd();
    }

    pushScopedToken (tokens, cursor, matched.getEnd(), baseScope);
}

//==============================================================================
/** Applies the regular expression rules of the SyntaxDefinition states to a line. */
class RuleMatcher
{
public:
    RuleMatcher (const SyntaxDefinition& definition,
                 const LineText& text,
                 std::vector<StateFrame>& stack,
                 std::unordered_map<String, CodeRegex>& expandedRules)
        : definition (definition)
        , text (text)
        , stack (stack)
        , expandedRules (expandedRules)
        , rootState (definition.findState ("root"))
    {
    }

    bool isInPushedState() const noexcept
    {
        return ! stack.empty();
    }

    /** Matches the rules of the active state at a position, consuming the match. */
    bool apply (int& pos, std::vector<Token>& tokens)
    {
        const int stateIndex = stack.empty() ? rootState : stack.back().state;
        if (stateIndex < 0 || text.length() > maxRegexLineLength)
            return false;

        if (pos == zeroWidthPosition && zeroWidthCount >= maxZeroWidthMatches)
            return false;

        if (! subject.has_value())
            subject.emplace (text.getString());

        const auto& state = definition.getState (stateIndex);

        for (int ruleIndex = 0; ruleIndex < static_cast<int> (state.rules.size()); ++ruleIndex)
        {
            const auto& rule = state.rules[static_cast<size_t> (ruleIndex)];

            const auto* regex = definition.getCompiledRule (stateIndex, ruleIndex);
            if (regex == nullptr)
                regex = expandPushCaptures (rule.match);

            if (regex == nullptr)
                continue;

            const auto match = regex->matchAt (*subject, pos);
            if (! match.has_value())
                continue;

            const int end = match->groups[0].getEnd();
            const bool changesState = rule.pop || rule.push.isNotEmpty();

            if (end == pos && ! changesState)
                continue;

            if (rule.skipWords && definition.isKnownWord (match->texts[0]))
                continue;

            addMatchTokens (tokens, *match, rule, getBaseScope (rule, state));

            if (rule.pop && ! stack.empty())
                stack.pop_back();

            if (const int pushed = rule.push.isNotEmpty() ? definition.findState (rule.push) : -1; pushed >= 0 && stack.size() < maxStackDepth)
                stack.push_back ({ pushed, match->texts });

            if (end == pos)
            {
                zeroWidthCount = pos == zeroWidthPosition ? zeroWidthCount + 1 : 1;
                zeroWidthPosition = pos;
            }

            pos = end;
            return true;
        }

        return false;
    }

    /** Consumes one character as the content of the active pushed state. */
    void consumeStateContent (int& pos, std::vector<Token>& tokens) const
    {
        const auto& scope = definition.getState (stack.back().state).scope;
        pushScopedToken (tokens, pos, pos + 1, scope.isValid() ? scope : getTokenTypeScope (SyntaxDefinition::TokenType::other), true);
        ++pos;
    }

private:
    static constexpr int maxZeroWidthMatches = 8;
    static constexpr size_t maxStackDepth = 32;

    static const Identifier& getBaseScope (const SyntaxDefinition::Rule& rule, const SyntaxDefinition::State& state)
    {
        if (rule.scope.isValid())
            return rule.scope;

        if (state.scope.isValid())
            return state.scope;

        return getTokenTypeScope (SyntaxDefinition::TokenType::other);
    }

    const CodeRegex* expandPushCaptures (const String& pattern)
    {
        const auto& captures = stack.empty() ? StringArray() : stack.back().captures;

        String expanded;
        for (int i = 0; i < pattern.length(); ++i)
        {
            const auto character = pattern[i];

            if (character == '\\' && i + 1 < pattern.length())
            {
                expanded << String::charToString (character) << String::charToString (pattern[++i]);
                continue;
            }

            if (character == '$' && i + 1 < pattern.length() && pattern[i + 1] >= '1' && pattern[i + 1] <= '9')
            {
                expanded << CodeRegex::escape (captures[static_cast<int> (pattern[++i] - '0')]);
                continue;
            }

            expanded << String::charToString (character);
        }

        auto found = expandedRules.find (expanded);
        if (found == expandedRules.end())
            found = expandedRules.emplace (expanded, CodeRegex::compile (expanded).valueOr (CodeRegex())).first;

        return found->second.isValid() ? &found->second : nullptr;
    }

    const SyntaxDefinition& definition;
    const LineText& text;
    std::vector<StateFrame>& stack;
    std::unordered_map<String, CodeRegex>& expandedRules;
    const int rootState;

    std::optional<CodeRegex::Subject> subject;
    int zeroWidthPosition = -1;
    int zeroWidthCount = 0;
};

//==============================================================================

std::optional<CodeRegex::Match> findInjectedMatch (const SyntaxDefinition& definition,
                                                   const CodeRegex& regex,
                                                   const SyntaxDefinition::Rule& rule,
                                                   const CodeRegex::Subject& subject,
                                                   int start,
                                                   int end)
{
    while (start < end)
    {
        auto match = regex.findNext (subject, start);
        if (! match.has_value() || match->groups[0].getStart() >= end)
            return std::nullopt;

        const auto matched = match->groups[0];
        if (! matched.isEmpty() && matched.getEnd() <= end && ! (rule.skipWords && definition.isKnownWord (match->texts[0])))
            return match;

        start = matched.getStart() + 1;
    }

    return std::nullopt;
}

/** Splits a token with the rules of an injected state, the earliest match winning (rule order on ties). */
void injectToken (const SyntaxDefinition& definition,
                  int stateIndex,
                  const CodeRegex::Subject& subject,
                  const Token& token,
                  std::vector<Token>& output)
{
    const auto& state = definition.getState (stateIndex);
    const auto numRules = state.rules.size();

    std::vector<std::optional<CodeRegex::Match>> pending (numRules);
    std::vector<bool> exhausted (numRules, false);

    int pos = token.start;
    while (pos < token.end)
    {
        int best = -1;

        for (size_t i = 0; i < numRules; ++i)
        {
            if (exhausted[i])
                continue;

            auto& candidate = pending[i];
            if (! candidate.has_value() || candidate->groups[0].getStart() < pos)
            {
                const auto* regex = definition.getCompiledRule (stateIndex, static_cast<int> (i));
                candidate = regex != nullptr ? findInjectedMatch (definition, *regex, state.rules[i], subject, pos, token.end) : std::nullopt;

                if (! candidate.has_value())
                {
                    exhausted[i] = true;
                    continue;
                }
            }

            if (best < 0 || candidate->groups[0].getStart() < pending[static_cast<size_t> (best)]->groups[0].getStart())
                best = static_cast<int> (i);
        }

        if (best < 0)
            break;

        const auto& match = *pending[static_cast<size_t> (best)];
        const auto& rule = state.rules[static_cast<size_t> (best)];

        pushScopedToken (output, pos, match.groups[0].getStart(), token.scope);
        addMatchTokens (output, match, rule, rule.scope.isValid() ? rule.scope : (state.scope.isValid() ? state.scope : token.scope));
        pos = match.groups[0].getEnd();
    }

    pushScopedToken (output, pos, token.end, token.scope);
}

/** Applies the injected states of the definition to the plain comment, string, ... tokens of a line. */
void applyInjections (const SyntaxDefinition& definition, const LineText& text, std::vector<Token>& tokens)
{
    if (text.length() > maxRegexLineLength)
        return;

    const auto injectedState = [&definition] (const Token& token)
    {
        return token.scope == getTokenTypeScope (token.type) ? definition.getInjectionState (token.type) : -1;
    };

    if (std::none_of (tokens.begin(), tokens.end(), [&] (const Token& token) { return injectedState (token) >= 0; }))
        return;

    const CodeRegex::Subject subject (text.getString());

    std::vector<Token> injected;
    injected.reserve (tokens.size() * 2);

    for (const auto& token : tokens)
    {
        if (const int stateIndex = injectedState (token); stateIndex >= 0)
            injectToken (definition, stateIndex, subject, token, injected);
        else
            injected.push_back (token);
    }

    tokens = std::move (injected);
}

//==============================================================================

uint8_t scanLine (const SyntaxDefinition& definition,
                  const LineText& text,
                  uint8_t stateBefore,
                  const String& continuationRawStringDelimiter,
                  std::vector<Token>& tokens,
                  String* outRawStringDelimiter,
                  RuleMatcher& rules)
{
    tokens.clear();

    const int length = text.length();
    int pos = 0;
    uint8_t state = stateBefore;

    auto push = [&tokens] (int start, int end, SyntaxDefinition::TokenType type)
    {
        if (end > start)
            tokens.push_back ({ start, end, type, getTokenTypeScope (type) });
    };

    // Raw strings carry their own scope, so string injections (escapes) skip them
    static const Identifier rawStringScope ("string.raw");
    auto pushRawString = [&tokens] (int start, int end)
    {
        pushScopedToken (tokens, start, end, rawStringScope);
    };

    // Continuation of a block comment started on a previous line.
    if (state == CodeTokeniser::inBlockComment)
    {
        const auto& blockComment = definition.getBlockComment();
        if (blockComment.has_value())
        {
            const int closePos = findPattern (text, blockComment->end, 0);
            if (closePos < 0)
            {
                push (0, length, SyntaxDefinition::TokenType::comment);
                return CodeTokeniser::inBlockComment;
            }

            push (0, closePos + blockComment->end.length(), SyntaxDefinition::TokenType::comment);
            pos = closePos + static_cast<int> (blockComment->end.length());
            state = CodeTokeniser::normal;
        }
        else
        {
            state = CodeTokeniser::normal;
        }
    }
    // Continuation of a C++ raw string literal started on a previous line.
    else if (state == CodeTokeniser::inRawString)
    {
        const int closePos = findRawStringClose (text, continuationRawStringDelimiter, 0);

        if (closePos < 0)
        {
            pushRawString (0, length);

            if (outRawStringDelimiter != nullptr)
                *outRawStringDelimiter = continuationRawStringDelimiter;

            return CodeTokeniser::inRawString;
        }

        const int endPos = closePos + 1 + static_cast<int> (continuationRawStringDelimiter.length()) + 1;
        pushRawString (0, endPos);
        pos = endPos;
        state = CodeTokeniser::normal;
    }
    // Continuation of a multi-line string started on a previous line.
    else if (state >= CodeTokeniser::inMultiLineString)
    {
        const int delimiterIndex = state - CodeTokeniser::inMultiLineString;
        const auto& multiLineDelimiters = definition.getMultiLineStringDelimiters();

        if (delimiterIndex < static_cast<int> (multiLineDelimiters.size()))
        {
            const auto& close = multiLineDelimiters[(delimiterIndex)];
            const int closePos = findUnescaped (text, close, 0, definition.getEscapeCharacter());

            if (closePos < 0)
            {
                push (0, length, SyntaxDefinition::TokenType::string);
                return state;
            }

            push (0, closePos + close.length(), SyntaxDefinition::TokenType::string);
            pos = closePos + static_cast<int> (close.length());
            state = CodeTokeniser::normal;
        }
        else
        {
            state = CodeTokeniser::normal;
        }
    }

    const auto& lineCommentPrefix = definition.getLineCommentPrefix();
    const auto& blockComment = definition.getBlockComment();
    const auto& stringDelimiters = definition.getStringDelimiters();
    const auto& multiLineDelimiters = definition.getMultiLineStringDelimiters();
    const yup_wchar escapeCharacter = definition.getEscapeCharacter();
    const bool multiLineStrings = definition.areStringsMultiLine();
    const auto& preprocessorPrefix = definition.getPreprocessorPrefix();

    while (pos < length)
    {
        if (rules.apply (pos, tokens))
            continue;

        if (rules.isInPushedState())
        {
            rules.consumeStateContent (pos, tokens);
            continue;
        }

        // A preprocessor directive consumes the whole line.
        if (state == CodeTokeniser::normal
            && pos == 0
            && preprocessorPrefix.isNotEmpty()
            && matchesAt (text, preprocessorPrefix, 0))
        {
            push (0, length, SyntaxDefinition::TokenType::preprocessor);
            return CodeTokeniser::normal;
        }

        const yup_wchar c = text[(pos)];

        // Whitespace
        if (CharacterFunctions::isWhitespace (c))
        {
            const int tokenStart = pos;
            while (pos < length && CharacterFunctions::isWhitespace (text[(pos)]))
                ++pos;

            push (tokenStart, pos, SyntaxDefinition::TokenType::whitespace);
            continue;
        }

        // Line comment consumes the rest of the line.
        if (lineCommentPrefix.isNotEmpty() && matchesAt (text, lineCommentPrefix, pos))
        {
            push (pos, length, SyntaxDefinition::TokenType::comment);
            return state;
        }

        // Block comment.
        if (blockComment.has_value() && matchesAt (text, blockComment->start, pos))
        {
            const int tokenStart = pos;
            const int closePos = findPattern (text, blockComment->end, pos + static_cast<int> (blockComment->start.length()));

            if (closePos < 0)
            {
                push (tokenStart, length, SyntaxDefinition::TokenType::comment);
                return CodeTokeniser::inBlockComment;
            }

            pos = closePos + static_cast<int> (blockComment->end.length());
            push (tokenStart, pos, SyntaxDefinition::TokenType::comment);
            continue;
        }

        // Multi-line string delimiters (checked before single-line ones, e.g. """ vs ").
        bool consumed = false;
        for (int i = 0; i < static_cast<int> (multiLineDelimiters.size()); ++i)
        {
            const auto& delim = multiLineDelimiters[(i)];
            if (! matchesAt (text, delim, pos))
                continue;

            const int tokenStart = pos;
            const int closePos = findUnescaped (text, delim, pos + static_cast<int> (delim.length()), escapeCharacter);

            if (closePos < 0)
            {
                push (tokenStart, length, SyntaxDefinition::TokenType::string);

                if (multiLineStrings)
                    return static_cast<uint8_t> (CodeTokeniser::inMultiLineString + i);

                pos = length;
            }
            else
            {
                pos = closePos + static_cast<int> (delim.length());
                push (tokenStart, pos, SyntaxDefinition::TokenType::string);
            }

            consumed = true;
            break;
        }

        if (consumed)
            continue;

        // Single-line string delimiters.
        for (const auto& delim : stringDelimiters)
        {
            if (! matchesAt (text, delim, pos))
                continue;

            const int tokenStart = pos;
            const int closePos = findUnescaped (text, delim, pos + static_cast<int> (delim.length()), escapeCharacter);

            pos = closePos < 0 ? length : closePos + static_cast<int> (delim.length());
            push (tokenStart, pos, SyntaxDefinition::TokenType::string);

            consumed = true;
            break;
        }

        if (consumed)
            continue;

        // Numbers.
        if (isDigit (c))
        {
            const int tokenStart = pos;

            if (definition.numbersAllowHex() && pos + 2 <= length && c == '0' && (text[(pos) + 1] == 'x' || text[(pos) + 1] == 'X'))
            {
                pos += 2;
                while (pos < length && isHexDigit (text[(pos)]))
                    ++pos;
            }
            else if (definition.numbersAllowBinary() && pos + 2 <= length && c == '0' && (text[(pos) + 1] == 'b' || text[(pos) + 1] == 'B'))
            {
                pos += 2;
                while (pos < length && (text[(pos)] == '0' || text[(pos)] == '1'))
                    ++pos;
            }
            else
            {
                while (pos < length && isDigit (text[(pos)]))
                    ++pos;
            }

            if (definition.numbersAllowFloat() && pos + 1 < length && text[(pos)] == '.' && isDigit (text[(pos) + 1]))
            {
                ++pos;
                while (pos < length && isDigit (text[(pos)]))
                    ++pos;
            }

            if (definition.numbersAllowExponent() && pos < length && (text[(pos)] == 'e' || text[(pos)] == 'E'))
            {
                int exponent = pos + 1;
                if (exponent < length && (text[(exponent)] == '+' || text[(exponent)] == '-'))
                    ++exponent;

                if (exponent < length && isDigit (text[(exponent)]))
                {
                    pos = exponent;
                    while (pos < length && isDigit (text[(pos)]))
                        ++pos;
                }
            }

            if (definition.numbersAllowSuffix())
            {
                while (pos < length)
                {
                    const auto suffix = text[(pos)];
                    if (suffix != 'u' && suffix != 'U' && suffix != 'l' && suffix != 'L' && suffix != 'f' && suffix != 'F')
                        break;

                    ++pos;
                }
            }

            push (tokenStart, pos, SyntaxDefinition::TokenType::number);
            continue;
        }

        // Identifiers / keywords / types.
        if (definition.isIdentifierStart (c))
        {
            const int tokenStart = pos;
            while (pos < length && definition.isIdentifierPart (text[(pos)]))
                ++pos;

            const String word = text.substring (tokenStart, pos);

            // C++ raw string literal: R"delim(...)delim" (also u8R, uR, UR, LR prefixes).
            if (definition.supportsRawStrings()
                && pos < length
                && text[(pos)] == '"'
                && definition.isRawStringPrefix (word))
            {
                const int delimiterLength = parseRawStringDelimiter (text, pos);
                if (delimiterLength >= 0)
                {
                    const String delimiter = text.substring (pos + 1, pos + 1 + delimiterLength);
                    const int closePos = findRawStringClose (text, delimiter, pos + 1 + delimiterLength + 1);

                    if (closePos >= 0)
                    {
                        pos = closePos + 1 + delimiterLength + 1;
                        pushRawString (tokenStart, pos);
                        continue;
                    }

                    pushRawString (tokenStart, length);

                    if (outRawStringDelimiter != nullptr)
                        *outRawStringDelimiter = delimiter;

                    return CodeTokeniser::inRawString;
                }
            }

            // String/character literal with an encoding or format prefix
            if (pos < length
                && (text[(pos)] == '"' || text[(pos)] == '\'')
                && definition.isStringPrefix (word))
            {
                // Prefixed multi-line strings (e.g. Python f"""...""").
                bool prefixedConsumed = false;
                for (int i = 0; i < static_cast<int> (multiLineDelimiters.size()); ++i)
                {
                    const auto& delim = multiLineDelimiters[(i)];
                    if (! matchesAt (text, delim, pos))
                        continue;

                    const int closePos = findUnescaped (text, delim, pos + static_cast<int> (delim.length()), escapeCharacter);

                    if (closePos < 0)
                    {
                        push (tokenStart, length, SyntaxDefinition::TokenType::string);

                        if (multiLineStrings)
                            return static_cast<uint8_t> (CodeTokeniser::inMultiLineString + i);

                        pos = length;
                    }
                    else
                    {
                        pos = closePos + static_cast<int> (delim.length());
                        push (tokenStart, pos, SyntaxDefinition::TokenType::string);
                    }

                    prefixedConsumed = true;
                    break;
                }

                if (prefixedConsumed)
                    continue;

                // Single-line prefixed string/character literal.
                const yup_wchar quote = text[(pos)];

                int scanPos = pos + 1;
                while (scanPos < length)
                {
                    const auto scanned = text[(scanPos)];
                    if (escapeCharacter != 0 && scanned == escapeCharacter && scanPos + 1 < length)
                    {
                        scanPos += 2;
                        continue;
                    }

                    if (scanned == quote)
                    {
                        ++scanPos;
                        break;
                    }

                    ++scanPos;
                }

                pos = scanPos;
                push (tokenStart, pos, SyntaxDefinition::TokenType::string);
                continue;
            }

            if (const auto wordScope = definition.getWordScope (word); wordScope.isValid())
                pushScopedToken (tokens, tokenStart, pos, wordScope);
            else if (definition.isKeyword (word))
                push (tokenStart, pos, SyntaxDefinition::TokenType::keyword);
            else if (definition.isType (word))
                push (tokenStart, pos, SyntaxDefinition::TokenType::type);
            else
                push (tokenStart, pos, SyntaxDefinition::TokenType::identifier);

            continue;
        }

        // Operators: longest match first (3, 2, then 1 characters).
        bool matchedOperator = false;
        for (int lengthToTry = 3; lengthToTry >= 1; --lengthToTry)
        {
            if (pos + lengthToTry > length)
                continue;

            const String op = text.substring (pos, pos + lengthToTry);
            if (definition.isOperator (op))
            {
                pos += lengthToTry;
                push (pos - lengthToTry, pos, SyntaxDefinition::TokenType::operator_);
                matchedOperator = true;
                break;
            }
        }

        if (matchedOperator)
            continue;

        // Anything else.
        ++pos;
        push (pos - 1, pos, SyntaxDefinition::TokenType::other);
    }

    // Zero-width rules (e.g. "$") may still change the state at the end of the line.
    while (pos == length && rules.apply (pos, tokens))
    {
    }

    return state;
}

} // namespace

//==============================================================================

CodeTokeniser::~CodeTokeniser()
{
    if (attachedDocument != nullptr)
        attachedDocument->removeListener (this);
}

//==============================================================================

void CodeTokeniser::setSyntaxDefinition (const SyntaxDefinition& definition)
{
    this->definition = &definition;
    expandedRules.clear();
    clear();
}

const SyntaxDefinition& CodeTokeniser::getSyntaxDefinition() const
{
    static const SyntaxDefinition inert;
    return definition != nullptr ? *definition : inert;
}

bool CodeTokeniser::hasSyntaxDefinition() const
{
    return definition != nullptr;
}

//==============================================================================

Span<const CodeTokeniser::Token> CodeTokeniser::getTokens (const CodeDocument& document, int lineIndex)
{
    if (definition == nullptr)
        return {};

    attachToDocument (document);

    const int numLines = document.getNumLines();
    lineIndex = jlimit (0, numLines - 1, lineIndex);

    if (static_cast<int> (cache.size()) < numLines)
    {
        const int oldSize = static_cast<int> (cache.size());
        cache.resize ((numLines));
        dirty.resize ((numLines));

        for (int i = oldSize; i < numLines; ++i)
        {
            cache[i] = std::nullopt;
            dirty[i] = true;
        }
    }

    return tokenize (document, lineIndex).tokens;
}

void CodeTokeniser::invalidateLines (int firstLine, int lastLine)
{
    if (cache.empty())
        return;

    firstLine = jlimit (0, static_cast<int> (cache.size()) - 1, firstLine);
    lastLine = jlimit (0, static_cast<int> (cache.size()) - 1, lastLine);

    for (int i = firstLine; i <= lastLine; ++i)
        dirty[i] = true;
}

void CodeTokeniser::clear()
{
    cache.clear();
    dirty.clear();
}

void CodeTokeniser::codeDocumentChanged (CodeDocument& document, const CodeDocument::Change& change)
{
    if (definition == nullptr || cache.empty())
        return;

    const int oldNumLines = static_cast<int> (cache.size());
    const int firstChangedLine = jlimit (0, oldNumLines, change.firstLine);
    const int numRemoved = jlimit (0, oldNumLines - firstChangedLine, change.numLinesRemoved);
    const int numInserted = jmax (0, change.numLinesInserted);

    // Lines after the block keep their tokens: only the replaced block is dropped
    cache.erase (cache.begin() + firstChangedLine, cache.begin() + firstChangedLine + numRemoved);
    cache.insert (cache.begin() + firstChangedLine, static_cast<size_t> (numInserted), std::nullopt);
    dirty.erase (dirty.begin() + firstChangedLine, dirty.begin() + firstChangedLine + numRemoved);
    dirty.insert (dirty.begin() + firstChangedLine, static_cast<size_t> (numInserted), true);

    if (static_cast<int> (cache.size()) != document.getNumLines())
    {
        // Out of sync with the document (a missed notification): start over
        cache.assign (static_cast<size_t> (document.getNumLines()), std::nullopt);
        dirty.assign (cache.size(), true);
        return;
    }

    if (cache.empty())
        return;

    const int lastChangedLine = jmin (static_cast<int> (cache.size()) - 1, firstChangedLine + numInserted - 1);

    // Only cached lines after the block can go stale; everything else is tokenized lazily
    if (lastChangedLine + 1 >= static_cast<int> (cache.size()) || ! cache[lastChangedLine + 1].has_value())
        return;

    std::vector<Token> scratch;
    for (int i = firstChangedLine; i < static_cast<int> (cache.size()); ++i)
    {
        if ((i > 0 && ! cache[i - 1].has_value()) || (i > lastChangedLine && ! cache[i].has_value()))
            break;

        const uint8_t stateBefore = i == 0 ? LineState::normal : cache[i - 1]->stateAfter;

        const String continuationDelimiter = stateBefore == CodeTokeniser::inRawString && i > 0
                                               ? cache[i - 1]->rawStringDelimiter
                                               : String();

        const LineText lineText (document.getLine (i));
        auto stack = i == 0 ? std::vector<StateFrame>() : cache[i - 1]->stackAfter;
        RuleMatcher rules (*definition, lineText, stack, expandedRules);

        String scratchDelimiter;
        const uint8_t stateAfter = scanLine (*definition, lineText, stateBefore, continuationDelimiter, scratch, &scratchDelimiter, rules);

        const bool stateStable = i > lastChangedLine
                              && cache[i]->stateAfter == stateAfter
                              && cache[i]->stateBefore == stateBefore
                              && cache[i]->rawStringDelimiter == scratchDelimiter
                              && cache[i]->stackAfter == stack;

        if (stateStable)
            break;

        if (! cache[i].has_value())
            cache[i].emplace();

        cache[i]->stateBefore = stateBefore;
        cache[i]->stateAfter = stateAfter;
        cache[i]->rawStringDelimiter = std::move (scratchDelimiter);
        cache[i]->stackAfter = std::move (stack);
        dirty[i] = true;
    }
}

//==============================================================================

const CodeTokeniser::TokenCache& CodeTokeniser::tokenize (const CodeDocument& document, int lineIndex)
{
    int first = lineIndex;
    while (first > 0 && (dirty[first] || ! cache[first].has_value()))
        --first;

    for (int line = first; line <= lineIndex; ++line)
    {
        if (! dirty[line] && cache[line].has_value())
            continue;

        const uint8_t stateBefore = line == 0
                                      ? LineState::normal
                                      : cache[line - 1]->stateAfter;

        const String continuationDelimiter = stateBefore == CodeTokeniser::inRawString && line > 0
                                               ? cache[line - 1]->rawStringDelimiter
                                               : String();

        const LineText lineText (document.getLine (line));

        TokenCache newCache;
        newCache.stateBefore = stateBefore;
        newCache.stackAfter = line == 0 ? std::vector<StateFrame>() : cache[line - 1]->stackAfter;

        RuleMatcher rules (*definition, lineText, newCache.stackAfter, expandedRules);
        newCache.stateAfter = scanLine (*definition, lineText, stateBefore, continuationDelimiter, newCache.tokens, &newCache.rawStringDelimiter, rules);
        applyInjections (*definition, lineText, newCache.tokens);

        const bool stateChanged = cache[line].has_value()
                               && (cache[line]->stateAfter != newCache.stateAfter
                                   || cache[line]->rawStringDelimiter != newCache.rawStringDelimiter
                                   || cache[line]->stackAfter != newCache.stackAfter);

        cache[line] = std::move (newCache);
        dirty[line] = false;

        if (stateChanged && line + 1 < static_cast<int> (cache.size()))
            dirty[line + 1] = true;
    }

    return *cache[lineIndex];
}

void CodeTokeniser::attachToDocument (const CodeDocument& document)
{
    if (attachedDocument == &document)
        return;

    if (attachedDocument != nullptr)
        attachedDocument->removeListener (this);

    attachedDocument = const_cast<CodeDocument*> (&document);
    attachedDocument->addListener (this);
}

} // namespace yup
