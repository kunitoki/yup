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

#pragma once

namespace yup::detail
{

inline String stripOptionalPxSuffix (const String& text)
{
    const auto trimmed = text.trim();
    return trimmed.endsWithIgnoreCase ("px") ? trimmed.dropLastCharacters (2).trimEnd() : trimmed;
}

inline std::optional<double> parseStrictDouble (const String& text, bool allowNegative = true) noexcept
{
    auto cursor = text.getCharPointer().findEndOfWhitespace();
    if (cursor.isEmpty())
        return std::nullopt;

    if (! allowNegative && *cursor == '-')
        return std::nullopt;

    const double value = CharacterFunctions::readDoubleValue (cursor);
    if (! cursor.findEndOfWhitespace().isEmpty() || ! std::isfinite (value))
        return std::nullopt;

    return value;
}

inline std::optional<float> parseStrictFloat (const String& text, bool allowNegative = true, bool allowPxSuffix = false)
{
    const auto parsed = parseStrictDouble (allowPxSuffix ? stripOptionalPxSuffix (text) : text, allowNegative);
    if (! parsed.has_value())
        return std::nullopt;

    const auto value = static_cast<float> (*parsed);
    if (! std::isfinite (value))
        return std::nullopt;

    return value;
}

template <class Value, std::size_t N>
inline std::optional<Value> findNamed (const std::pair<const char*, Value> (&table)[N], const String& name)
{
    for (const auto& [candidate, value] : table)
        if (name == candidate)
            return value;

    return std::nullopt;
}

template <class EnumType>
struct EnumNames
{
    StringArray names;
    std::vector<EnumType> values;

    EnumNames (std::initializer_list<std::pair<const char*, EnumType>> entries)
    {
        values.reserve (entries.size());

        for (const auto& [name, value] : entries)
        {
            names.add (name);
            values.push_back (value);
        }
    }

    std::optional<EnumType> find (const String& name) const
    {
        const auto index = names.indexOf (name);
        return index >= 0 ? std::optional<EnumType> (values[static_cast<std::size_t> (index)]) : std::nullopt;
    }

    EnumType toEnum (const String& name, EnumType fallback) const
    {
        return find (name).value_or (fallback);
    }

    String toString (EnumType value) const
    {
        for (std::size_t i = 0; i < values.size(); ++i)
            if (values[i] == value)
                return names[static_cast<int> (i)];

        return {};
    }
};

inline const StringArray& layoutSelfAlignmentNames()
{
    static const StringArray names { "auto", "start", "end", "center", "stretch", "baseline" };
    return names;
}

inline ResultValue<String> stripStyleComments (StringRef rawText)
{
    const auto* utf8 = rawText.text.getAddress();
    if (utf8 == nullptr)
        return makeResultValueOk (String());

    std::string out;
    int pos = 0;
    int chunkStart = 0;

    while (utf8[pos] != 0)
    {
        const auto ch = utf8[pos];

        if (ch == '"' || ch == '\'')
        {
            ++pos;
            while (utf8[pos] != 0 && utf8[pos] != ch)
            {
                if (utf8[pos] == '\\' && utf8[pos + 1] != 0)
                    ++pos;

                ++pos;
            }

            if (utf8[pos] == ch)
                ++pos;

            continue;
        }

        if (ch == '/' && utf8[pos + 1] == '/')
        {
            out.append (utf8 + chunkStart, static_cast<size_t> (pos - chunkStart));
            pos += 2;

            while (utf8[pos] != 0 && utf8[pos] != '\n')
                ++pos;

            chunkStart = pos;
            continue;
        }

        if (ch == '/' && utf8[pos + 1] == '*')
        {
            out.append (utf8 + chunkStart, static_cast<size_t> (pos - chunkStart));

            const auto* end = std::strstr (utf8 + pos + 2, "*/");
            if (end == nullptr)
                return makeResultValueFail ("Unterminated comment");

            pos = static_cast<int> (end - utf8) + 2;
            const auto next = utf8[pos];
            const bool prevIsSpace = out.empty() || out.back() == ' ' || out.back() == '\t' || out.back() == '\n' || out.back() == '\r';
            const bool nextIsSpace = next == 0 || next == ' ' || next == '\t' || next == '\n' || next == '\r';

            if (! prevIsSpace && ! nextIsSpace)
                out.push_back (' ');

            chunkStart = pos;
            continue;
        }

        ++pos;
    }

    out.append (utf8 + chunkStart, static_cast<size_t> (pos - chunkStart));
    return makeResultValueOk (String::fromUTF8 (out.data(), static_cast<int> (out.size())));
}

inline bool splitTopLevel (const String& rawText, bool commaOnly, StringArray& outTokens)
{
    outTokens.clear();

    const auto stripped = stripStyleComments (rawText);
    if (stripped.failed())
        return false;

    const auto& text = stripped.getReference();

    int depth = 0;
    yup_wchar quote = 0;
    int tokenStart = 0;
    const int length = text.length();

    for (int i = 0; i < length; ++i)
    {
        const auto ch = text[i];

        if (quote != 0)
        {
            if (ch == '\\' && i + 1 < length)
                ++i;
            else if (ch == quote)
                quote = 0;

            continue;
        }

        if (ch == '"' || ch == '\'')
        {
            quote = ch;
            continue;
        }

        if (ch == '(' || ch == '[')
        {
            ++depth;
            continue;
        }

        if (ch == ')' || ch == ']')
        {
            if (--depth < 0)
                return false;

            continue;
        }

        if (depth == 0)
        {
            const bool isSeparator = commaOnly
                                       ? (ch == ',')
                                       : (ch == ',' || CharacterFunctions::isWhitespace (ch));

            if (isSeparator)
            {
                const auto segment = text.substring (tokenStart, i).trim();

                if (commaOnly || segment.isNotEmpty())
                    outTokens.add (segment);

                tokenStart = i + 1;
            }
        }
    }

    if (quote != 0 || depth != 0)
        return false;

    const auto tail = text.substring (tokenStart).trim();
    if (commaOnly || tail.isNotEmpty())
        outTokens.add (tail);

    return true;
}

inline std::vector<StringArray> tokenizeStyleValue (const String& rawValue)
{
    const auto stripped = stripStyleComments (rawValue);
    const String& value = stripped.wasOk() ? stripped.getReference() : rawValue;

    std::vector<StringArray> entries (1);
    String token;
    int depth = 0;
    yup_wchar quote = 0;

    const auto flush = [&]
    {
        if (token.isEmpty())
            return;

        auto& tokens = entries.back();

        if (token.startsWithChar ('(') && ! tokens.isEmpty())
            tokens.set (tokens.size() - 1, tokens[tokens.size() - 1] + token);
        else
            tokens.add (token);

        token.clear();
    };

    for (auto p = value.getCharPointer(); ! p.isEmpty(); ++p)
    {
        const auto c = *p;

        if (quote != 0)
        {
            token += c;

            if (c == '\\')
            {
                ++p;
                if (p.isEmpty())
                    break;

                token += *p;
            }
            else if (c == quote)
            {
                quote = 0;
            }

            continue;
        }

        if (depth == 0 && (c == ',' || CharacterFunctions::isWhitespace (c)))
        {
            flush();

            if (c == ',')
                entries.emplace_back();

            continue;
        }

        if (c == '"' || c == '\'')
            quote = c;
        else if (c == '(' || c == '[')
            ++depth;
        else if (c == ')' || c == ']')
            depth = jmax (0, depth - 1);

        token += c;
    }

    flush();
    return entries;
}

} // namespace yup::detail
