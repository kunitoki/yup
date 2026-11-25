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
    Maps the elements of a parsed tree to their position in the text.

    A parser records where each element and each property was written. Positions are keyed by
    the place of the element in its tree rather than by the element itself, so they also apply
    to a copy of the tree, or to a tree patched to match it.

    @code
    DataTreeSourceMap sourceMap;
    sourceMap.add (slider, "value", 12, 9, "ui.yui");

    const auto located = sourceMap.locate ({ .message = "Expected a number", .element = slider, .property = "value" });
    @endcode

    @see DataTreeDiagnostic, DataTree
*/
class YUP_API DataTreeSourceMap
{
public:
    //==============================================================================
    /** Records where an element or one of its properties was written.

        @param element   An element, already part of the tree it will be looked up in.
        @param property  The property of the element, or null for the element itself.
        @param line      The 1-based line.
        @param column    The 1-based column, in characters.
        @param source    The text the element comes from when the tree was parsed from several, empty for the main text.
    */
    void add (const DataTree& element, const Identifier& property, int line, int column, const String& source = {});

    /** Fills in the position of a diagnostic from its element and property.

        The position of the property is used when known, otherwise the one of the element, or
        of its closest ancestor with a known position. A diagnostic which already has a
        position, or has no element, is returned unchanged.
    */
    DataTreeDiagnostic locate (DataTreeDiagnostic diagnostic) const;

    /** Returns true when the two maps hold the same positions: those of a text, and of that text
        parsed again.
    */
    bool operator== (const DataTreeSourceMap& other) const;

private:
    struct Position
    {
        String source;
        int line = 0;
        int column = 0;

        bool operator== (const Position& other) const = default;
    };

    using Key = std::pair<std::vector<int>, String>;

    static std::vector<int> getPath (const DataTree& element);

    std::map<Key, Position> positions;
};

} // namespace yup
