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

ParameterChangeQueue::ParameterChangeQueue (int numParametersToHold)
{
    resize (numParametersToHold);
}

void ParameterChangeQueue::resize (int numParametersToHold)
{
    numParameters = jmax (0, numParametersToHold);
    words = std::vector<std::atomic<uint32>> ((static_cast<std::size_t> (numParameters) + bitsPerWord - 1) / bitsPerWord);
}

void ParameterChangeQueue::ensureSize (int numParametersToHold)
{
    if (numParametersToHold <= numParameters)
        return;

    const auto requiredWords = (static_cast<std::size_t> (numParametersToHold) + bitsPerWord - 1) / bitsPerWord;

    if (requiredWords > words.size())
    {
        std::vector<std::atomic<uint32>> grownWords (jmax (requiredWords, words.size() * 2));

        for (std::size_t word = 0; word < words.size(); ++word)
            grownWords[word].store (words[word].load (std::memory_order_relaxed), std::memory_order_relaxed);

        words.swap (grownWords);
    }

    numParameters = numParametersToHold;
}

void ParameterChangeQueue::markChanged (int parameterIndex) noexcept
{
    if (! isPositiveAndBelow (parameterIndex, numParameters))
        return;

    const auto bit = uint32 (1) << (parameterIndex % bitsPerWord);
    words[static_cast<std::size_t> (parameterIndex / bitsPerWord)].fetch_or (bit, std::memory_order_release);
}

} // namespace yup
