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
/**
    A problem found in a `.yui` document.

    Syntax errors carry the position where parsing stopped. The problems found later, by the
    builder or the stylesheet, carry the element of the document tree they come from, and the
    property of that element when they are about a value, which a YuiSourceMap turns into a
    position in the text.

    @see YuiParser, YuiSourceMap, YuiDocument

    @tags{UI}
*/
struct YUP_API YuiDiagnostic
{
    /** What went wrong. */
    String message;

    /** The element of the document tree the problem comes from, if known. */
    DataTree element;

    /** The property of the element the problem is about, null when it is about the whole element. */
    Identifier property;

    /** The imported file the problem is in, empty for the main text. */
    String source;

    /** The 1-based line of the problem, 0 when the position is unknown. */
    int line = 0;

    /** The 1-based column of the problem, counted in characters. */
    int column = 0;

    /** Returns the diagnostic as `source:line:column: message`, omitting the parts that are unknown. */
    String toString() const;

    /** Returns the diagnostics as strings, joined by a separator. */
    static String join (const std::vector<YuiDiagnostic>& diagnostics, StringRef separator = "\n");
};

//==============================================================================
/**
    Maps the elements of a parsed document to their position in the text.

    YuiParser records where each element and each property was written. Positions are keyed by
    the place of the element in its document rather than by the element itself, so they also
    apply to a copy of the document or to a tree patched to match it with YuiTreeDiff.

    @see YuiParser::ParseResult, YuiDiagnostic

    @tags{UI}
*/
class YUP_API YuiSourceMap
{
public:
    //==============================================================================
    /** Records where an element or one of its properties was written.

        @param element   An element, already part of the document it will be looked up in.
        @param property  The property of the element, or null for the element itself.
        @param line      The 1-based line.
        @param column    The 1-based column, in characters.
        @param source    The imported file the element comes from, empty for the main text.
    */
    void add (const DataTree& element, const Identifier& property, int line, int column, const String& source = {});

    /** Fills in the position of a diagnostic from its element and property.

        The position of the property is used when known, otherwise the one of the element, or
        of its closest ancestor with a known position. A diagnostic which already has a
        position, or has no element, is returned unchanged.
    */
    YuiDiagnostic locate (YuiDiagnostic diagnostic) const;

private:
    struct Position
    {
        String source;
        int line = 0;
        int column = 0;
    };

    using Key = std::pair<std::vector<int>, String>;

    static std::vector<int> getPath (const DataTree& element);

    std::map<Key, Position> positions;
};

} // namespace yup
