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

} // namespace yup::detail
