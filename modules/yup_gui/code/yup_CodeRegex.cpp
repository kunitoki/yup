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

/** Replaces the line start anchors of a pattern with a never matching group, or returns an empty string if it has none. */
std::string disableLineStartAnchors (const std::string& pattern)
{
    std::string result;
    bool inClass = false;
    bool replaced = false;

    for (size_t i = 0; i < pattern.size(); ++i)
    {
        const char character = pattern[i];

        if (character == '\\' && i + 1 < pattern.size())
        {
            result += character;
            result += pattern[++i];
            continue;
        }

        if (inClass)
            inClass = character != ']';
        else if (character == '[')
            inClass = true;
        else if (character == '^')
        {
            result += "(?!)";
            replaced = true;
            continue;
        }

        result += character;

        if (character == '[' && i + 1 < pattern.size() && pattern[i + 1] == '^')
            result += pattern[++i];
    }

    return replaced ? result : std::string();
}

} // namespace

//==============================================================================

struct CodeRegex::Impl
{
    std::regex regex;

    // Used past the start of the text: some standard libraries let '^' match there
    // when the preceding character is made available to the matcher.
    std::optional<std::regex> regexPastLineStart;
};

//==============================================================================

CodeRegex::Subject::Subject (const String& text)
    : utf8 (text.toStdString())
{
    auto p = text.getCharPointer();
    const auto* const begin = p.getAddress();

    while (! p.isEmpty())
    {
        characterOffsets.push_back (static_cast<int> (p.getAddress() - begin));
        ++p;
    }

    numCharacters = static_cast<int> (characterOffsets.size());

    // Pure ASCII text maps characters to bytes one to one, no table needed.
    if (numCharacters == static_cast<int> (utf8.size()))
        characterOffsets.clear();
    else
        characterOffsets.push_back (static_cast<int> (utf8.size()));
}

int CodeRegex::Subject::getLength() const noexcept
{
    return numCharacters;
}

int CodeRegex::Subject::byteToCharacter (int byteOffset) const noexcept
{
    if (characterOffsets.empty())
        return byteOffset;

    const auto found = std::lower_bound (characterOffsets.begin(), characterOffsets.end(), byteOffset);
    return static_cast<int> (std::distance (characterOffsets.begin(), found));
}

int CodeRegex::Subject::characterToByte (int characterIndex) const noexcept
{
    if (characterOffsets.empty())
        return characterIndex;

    return characterOffsets[static_cast<size_t> (characterIndex)];
}

//==============================================================================

ResultValue<CodeRegex> CodeRegex::compile (StringRef pattern, bool caseSensitive)
{
    auto flags = std::regex::ECMAScript | std::regex::optimize;
    if (! caseSensitive)
        flags |= std::regex::icase;

    CodeRegex result;
    result.pattern = String (pattern);

    try
    {
        const auto source = result.pattern.toStdString();

        Impl impl { std::regex (source, flags), std::nullopt };

        if (const auto withoutAnchors = disableLineStartAnchors (source); ! withoutAnchors.empty())
            impl.regexPastLineStart.emplace (withoutAnchors, flags);

        result.impl = std::make_shared<const Impl> (std::move (impl));
    }
    catch (const std::regex_error& error)
    {
        return makeResultValueFail ("Invalid regular expression \"" + result.pattern + "\": " + error.what());
    }

    return makeResultValueOk (std::move (result));
}

bool CodeRegex::isValid() const noexcept
{
    return impl != nullptr;
}

const String& CodeRegex::getPattern() const noexcept
{
    return pattern;
}

int CodeRegex::getNumGroups() const noexcept
{
    return impl != nullptr ? static_cast<int> (impl->regex.mark_count()) : 0;
}

//==============================================================================

std::optional<CodeRegex::Match> CodeRegex::matchAt (const Subject& subject, int position) const
{
    return find (subject, position, true);
}

std::optional<CodeRegex::Match> CodeRegex::findNext (const Subject& subject, int startPosition) const
{
    return find (subject, startPosition, false);
}

std::optional<CodeRegex::Match> CodeRegex::find (const Subject& subject, int position, bool anchored) const
{
    if (impl == nullptr || position < 0 || position > subject.getLength())
        return std::nullopt;

    const auto begin = subject.utf8.cbegin();
    const auto start = begin + subject.characterToByte (position);

    auto flags = std::regex_constants::match_default;
    if (anchored)
        flags |= std::regex_constants::match_continuous;
    if (position > 0)
        flags |= std::regex_constants::match_prev_avail;

    const auto& regex = position > 0 && impl->regexPastLineStart.has_value() ? *impl->regexPastLineStart : impl->regex;

    std::smatch results;

    try
    {
        if (! std::regex_search (start, subject.utf8.cend(), results, regex, flags))
            return std::nullopt;
    }
    catch (const std::regex_error&)
    {
        return std::nullopt;
    }

    Match match;
    match.groups.reserve (results.size());

    for (size_t i = 0; i < results.size(); ++i)
    {
        const auto& group = results[i];

        if (! group.matched)
        {
            match.groups.push_back ({ -1, -1 });
            match.texts.add ({});
            continue;
        }

        const auto groupStart = subject.byteToCharacter (static_cast<int> (group.first - begin));
        const auto groupEnd = subject.byteToCharacter (static_cast<int> (group.second - begin));

        match.groups.push_back ({ groupStart, groupEnd });
        match.texts.add (group.length() > 0 ? String::fromUTF8 (&*group.first, static_cast<int> (group.length())) : String());
    }

    return match;
}

//==============================================================================

String CodeRegex::escape (StringRef text)
{
    static const String specialCharacters ("\\^$.|?*+()[]{}");

    String escaped;
    escaped.preallocateBytes (text.length() * 2);

    for (auto p = text.text; ! p.isEmpty(); ++p)
    {
        const auto character = *p;

        if (specialCharacters.containsChar (character))
            escaped << '\\';

        escaped << String::charToString (character);
    }

    return escaped;
}

} // namespace yup
