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

bool matchesAt (const String& text, StringRef pattern, int pos)
{
    if (pos + pattern.length() > text.length())
        return false;

    for (int i = 0; i < pattern.length(); ++i)
    {
        if (text[(pos) + (i)] != pattern[(i)])
            return false;
    }

    return true;
}

int findPattern (const String& text, StringRef pattern, int from)
{
    if (pattern.isEmpty())
        return -1;

    for (int i = jmax (0, from); i + pattern.length() <= text.length(); ++i)
    {
        if (matchesAt (text, pattern, i))
            return i;
    }

    return -1;
}

int findUnescaped (const String& text, StringRef close, int from, yup_wchar escapeCharacter)
{
    for (int i = jmax (0, from); i + close.length() <= text.length(); ++i)
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

int parseRawStringDelimiter (const String& text, int quotePos)
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

int findRawStringClose (const String& text, StringRef delimiter, int from)
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

uint8_t scanLine (const SyntaxDefinition& definition,
                  const String& text,
                  uint8_t stateBefore,
                  const String& continuationRawStringDelimiter,
                  std::vector<Token>& tokens,
                  String* outRawStringDelimiter)
{
    tokens.clear();

    const int length = text.length();
    int pos = 0;
    uint8_t state = stateBefore;

    auto push = [&tokens] (int start, int end, SyntaxDefinition::TokenType type)
    {
        if (end > start)
            tokens.push_back ({ start, end, type });
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
            push (0, length, SyntaxDefinition::TokenType::string);

            if (outRawStringDelimiter != nullptr)
                *outRawStringDelimiter = continuationRawStringDelimiter;

            return CodeTokeniser::inRawString;
        }

        const int endPos = closePos + 1 + static_cast<int> (continuationRawStringDelimiter.length()) + 1;
        push (0, endPos, SyntaxDefinition::TokenType::string);
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

    // A preprocessor directive consumes the whole line.
    if (state == CodeTokeniser::normal
        && pos == 0
        && preprocessorPrefix.isNotEmpty()
        && matchesAt (text, preprocessorPrefix, 0))
    {
        push (0, length, SyntaxDefinition::TokenType::preprocessor);
        return CodeTokeniser::normal;
    }

    while (pos < length)
    {
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
                        push (tokenStart, pos, SyntaxDefinition::TokenType::string);
                        continue;
                    }

                    push (tokenStart, length, SyntaxDefinition::TokenType::string);

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

            if (definition.isKeyword (word))
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

void CodeTokeniser::codeDocumentChanged (CodeDocument& document, int firstChangedLine, int lastChangedLine)
{
    if (definition == nullptr || cache.empty())
        return;

    const int newNumLines = document.getNumLines();
    const bool lineCountChanged = static_cast<int> (cache.size()) != newNumLines;

    if (lineCountChanged)
    {
        cache.resize (static_cast<size_t> (newNumLines));
        dirty.resize (static_cast<size_t> (newNumLines));
    }

    if (cache.empty())
        return;

    firstChangedLine = jlimit (0, static_cast<int> (cache.size()) - 1, firstChangedLine);
    lastChangedLine = jlimit (0, static_cast<int> (cache.size()) - 1, lastChangedLine);

    const int dirtyEnd = lineCountChanged ? static_cast<int> (cache.size()) - 1 : lastChangedLine;

    for (int i = firstChangedLine; i <= dirtyEnd; ++i)
        dirty[i] = true;

    std::vector<Token> scratch;
    for (int i = firstChangedLine; i < static_cast<int> (cache.size()) && cache[i].has_value(); ++i)
    {
        const uint8_t stateBefore = i == 0 ? LineState::normal : cache[i - 1]->stateAfter;

        const String continuationDelimiter = stateBefore == CodeTokeniser::inRawString && i > 0
                                               ? cache[i - 1]->rawStringDelimiter
                                               : String();

        String scratchDelimiter;
        const uint8_t stateAfter = scanLine (*definition, document.getLine (i), stateBefore, continuationDelimiter, scratch, &scratchDelimiter);

        const bool stateStable = i > lastChangedLine
                              && cache[i]->stateAfter == stateAfter
                              && cache[i]->stateBefore == stateBefore
                              && cache[i]->rawStringDelimiter == scratchDelimiter;

        if (stateStable)
            break;

        cache[i]->stateBefore = stateBefore;
        cache[i]->stateAfter = stateAfter;
        cache[i]->rawStringDelimiter = std::move (scratchDelimiter);
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

        TokenCache newCache;
        newCache.stateBefore = stateBefore;
        newCache.stateAfter = scanLine (*definition, document.getLine (line), stateBefore, continuationDelimiter, newCache.tokens, &newCache.rawStringDelimiter);

        const bool stateChanged = cache[line].has_value()
                               && (cache[line]->stateAfter != newCache.stateAfter
                                   || cache[line]->rawStringDelimiter != newCache.rawStringDelimiter);

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
