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

#include <yup_events/yup_events.h>

#include <optional>

//==============================================================================
/** Loose, in-order letter matching for the preset search.

    @see PresetQuery
*/
namespace PresetFuzzyMatch
{
/** Matches the pattern's letters, in order, anywhere in the text, ignoring case.

    Letters that start a word or directly follow the previous matched letter score more,
    and letters skipped between two matched ones score less; letters before the first
    match are free. The best scoring start in the text is used.

    @param pattern  the letters to look for. An empty pattern matches with a score of 0.
    @param text     the text to look in.

    @returns the score, higher for better matches, or nullopt when the letters are not all
             found in order.
*/
std::optional<int> match (const yup::String& pattern, const yup::String& text);
} // namespace PresetFuzzyMatch
