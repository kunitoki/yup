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

// Each level of elements is a call inside the one around it, for the parser and for what reads its trees: this keeps
// the stack of any platform
inline constexpr int maxYuiNestingDepth = 128;

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
