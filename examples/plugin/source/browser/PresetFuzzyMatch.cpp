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

#include "PresetFuzzyMatch.h"

#include <string>

namespace PresetFuzzyMatch
{
namespace
{
constexpr int letterScore = 1;
constexpr int wordStartBonus = 8;
constexpr int consecutiveBonus = 5;
constexpr int skipPenalty = 1;

std::u32string toLowerCodePoints (const yup::String& text)
{
    const auto lower = text.toLowerCase();
    std::u32string result;

    for (auto p = lower.getCharPointer(); ! p.isEmpty();)
        result.push_back (static_cast<char32_t> (p.getAndAdvance()));

    return result;
}

bool startsWord (const std::u32string& text, std::size_t index)
{
    return index == 0 || ! yup::CharacterFunctions::isLetterOrDigit (static_cast<yup::yup_wchar> (text[index - 1]));
}

std::optional<int> matchFrom (const std::u32string& pattern, const std::u32string& text, std::size_t start)
{
    int score = 0;
    auto previous = start;

    for (std::size_t p = 0, t = start; p < pattern.size(); ++p, ++t)
    {
        while (t < text.size() && text[t] != pattern[p])
            ++t;

        if (t == text.size())
            return std::nullopt;

        score += letterScore;

        if (startsWord (text, t))
            score += wordStartBonus;

        if (p > 0)
        {
            if (t == previous + 1)
                score += consecutiveBonus;
            else
                score -= skipPenalty * static_cast<int> (t - previous - 1);
        }

        previous = t;
    }

    return score;
}
} // namespace

std::optional<int> match (const yup::String& pattern, const yup::String& text)
{
    if (pattern.isEmpty())
        return 0;

    const auto lowerPattern = toLowerCodePoints (pattern);
    const auto lowerText = toLowerCodePoints (text);
    std::optional<int> best;

    for (std::size_t start = 0; start < lowerText.size(); ++start)
    {
        if (lowerText[start] != lowerPattern.front())
            continue;

        const auto score = matchFrom (lowerPattern, lowerText, start);

        if (! score.has_value())
            break;

        if (! best.has_value() || *score > *best)
            best = score;
    }

    return best;
}
} // namespace PresetFuzzyMatch
