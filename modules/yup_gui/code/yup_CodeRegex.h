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

namespace yup
{

//==============================================================================
/** A compiled regular expression matched against single lines of code.

    CodeRegex wraps `std::regex` with the ECMAScript grammar. Patterns are matched on
    the UTF-8 bytes of the text, while every position passed in or returned is a
    character index, the same unit used by CodeDocument, tokens and selections.

    Matching never throws: a pattern that fails to compile is reported through the
    ResultValue returned by compile(), and a match that exceeds the engine limits is
    reported as no match.

    The ECMAScript grammar supports lookahead and backreferences, but not lookbehind,
    named groups or possessive quantifiers. `\w`, `\d` and `.` operate on bytes, so
    non-ASCII text needs explicit character classes.

    @code
    auto regex = CodeRegex::compile ("[A-Za-z_]\\w*");
    if (regex.wasOk())
    {
        CodeRegex::Subject subject ("int value = 1;");
        if (auto match = regex.getValue().matchAt (subject, 4))
            auto word = match->groups[0]; // { 4, 9 }
    }
    @endcode

    @see SyntaxDefinition
*/
class YUP_API CodeRegex
{
public:
    //==============================================================================
    /** The text a CodeRegex is matched against, converted once for repeated matching. */
    class YUP_API Subject
    {
    public:
        /** Prepares a text for matching. */
        explicit Subject (const String& text);

        /** Returns the length of the text in characters. */
        int getLength() const noexcept;

    private:
        friend class CodeRegex;

        int byteToCharacter (int byteOffset) const noexcept;
        int characterToByte (int characterIndex) const noexcept;

        std::string utf8;
        std::vector<int> characterOffsets;
        int numCharacters = 0;
    };

    //==============================================================================
    /** The result of a successful match. */
    struct Match
    {
        /** The character ranges of the whole match (index 0) and of each capture group.
            A group that did not take part in the match has a negative start.
        */
        std::vector<Range<int>> groups;

        /** The text of the whole match (index 0) and of each capture group. */
        StringArray texts;
    };

    //==============================================================================
    /** Creates an invalid regex that never matches. */
    CodeRegex() = default;

    /** Compiles a pattern.

        @param pattern       The ECMAScript regular expression.
        @param caseSensitive False to match ignoring case.
        @returns The compiled regex, or the error reported by the regex engine.
    */
    static ResultValue<CodeRegex> compile (StringRef pattern, bool caseSensitive = true);

    /** Returns true if this regex holds a compiled pattern. */
    bool isValid() const noexcept;

    /** Returns the source pattern. */
    const String& getPattern() const noexcept;

    /** Returns the number of capture groups in the pattern. */
    int getNumGroups() const noexcept;

    //==============================================================================
    /** Matches the pattern starting exactly at a position.

        @param subject  The text to match.
        @param position The character index the match must start at.
        @returns The match, or std::nullopt.
    */
    std::optional<Match> matchAt (const Subject& subject, int position) const;

    /** Finds the first match starting at or after a position.

        @param subject       The text to search.
        @param startPosition The character index to start searching from.
        @returns The match, or std::nullopt.
    */
    std::optional<Match> findNext (const Subject& subject, int startPosition) const;

    //==============================================================================
    /** Returns a pattern matching the given text literally. */
    static String escape (StringRef text);

private:
    struct Impl;

    std::optional<Match> find (const Subject& subject, int position, bool anchored) const;

    std::shared_ptr<const Impl> impl;
    String pattern;
};

} // namespace yup
