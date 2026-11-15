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

inline bool splitTopLevel (const String& text, bool commaOnly, StringArray& outTokens)
{
    outTokens.clear();

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

inline std::vector<StringArray> tokenizeStyleValue (const String& value)
{
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

inline String normalizeYuiPath (const String& rawPath)
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

inline String resolveRelativeYuiPath (const String& baseFilePath, const String& relativePath)
{
    const auto cleanedRelative = relativePath.trim().replaceCharacter ('\\', '/');
    if (cleanedRelative.startsWithChar ('/'))
        return normalizeYuiPath (cleanedRelative.substring (1));

    const auto normalizedBase = normalizeYuiPath (baseFilePath);
    const auto slash = normalizedBase.lastIndexOfChar ('/');

    if (slash < 0)
        return normalizeYuiPath (cleanedRelative);

    return normalizeYuiPath (normalizedBase.substring (0, slash + 1) + cleanedRelative);
}

inline bool isYuiTypeName (const String& name)
{
    return CharacterFunctions::isUpperCase (static_cast<yup_wchar> (*name.getCharPointer()));
}

inline bool isYuiComponentElement (const DataTree& element)
{
    return isYuiTypeName (element.getType().toString());
}

inline String getYuiElementId (const DataTree& element)
{
    static const Identifier id ("id");
    return element.getProperty (id).toString().unquoted().trim();
}

inline std::optional<std::pair<String, Identifier>> splitYuiPath (const String& path)
{
    const auto dot = path.indexOfChar ('.');
    if (dot <= 0 || dot + 1 >= path.length())
        return std::nullopt;

    return std::make_pair (path.substring (0, dot), Identifier (path.substring (dot + 1)));
}

} // namespace yup::detail
