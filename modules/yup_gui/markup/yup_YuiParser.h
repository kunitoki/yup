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
    Parses `.yui` text into a document tree.

    The document is a DataTree of type `yui` holding one element per top-level block.
    The text maps onto the tree like this:

    - `drawing Knob { ... }` becomes a `drawing` element whose `name` is `Knob`;
    - `circle { ... }` becomes a `circle` child element;
    - `radius: r - 2;` becomes the property `radius` holding the expression text `r - 2`;
    - `let r: bounds.width / 2;` becomes a `let` element with `name` and `value`;
    - `repeat i in 0 .. 11 { ... }` becomes a `repeat` element with `name`, `from` and `to`.

    The parser checks the syntax only. Every value must be a single expression, but element
    types, property names and the meaning of expressions are checked by the code consuming
    the tree, such as DrawingProgram. Line and block comments can go anywhere whitespace can.

    This version reads `drawing` blocks only; the other top-level blocks of the language
    (`import`, `tokens`, `style`, `component` and the instance tree) are not supported yet.

    @code
    const auto result = YuiParser::parse (text);
    if (result.wasOk())
        auto style = DrawingStyle::create (knob, result.document.getChildWithName ("drawing"));
    else
        DBG (result.diagnostics.front().toString());
    @endcode

    @see DrawingProgram

    @tags{UI}
*/
class YUP_API YuiParser
{
public:
    //==============================================================================
    /** A problem found in the text, at a 1-based line and column. */
    struct Diagnostic
    {
        int line = 0;
        int column = 0;
        String message;

        /** Returns the diagnostic as `line:column: message`. */
        String toString() const;
    };

    /** The outcome of parsing a text. */
    struct ParseResult
    {
        /** The document, invalid when the text has errors. */
        DataTree document;

        /** The problems found, empty when the text was parsed. */
        std::vector<Diagnostic> diagnostics;

        /** Returns true if the text was parsed without errors. */
        bool wasOk() const noexcept { return diagnostics.empty(); }
    };

    //==============================================================================
    /** Parses a `.yui` text, stopping at the first error.

        @param text  The text to parse.

        @returns The document, or the diagnostic of the first error.
    */
    static ParseResult parse (const String& text);

    YuiParser() = delete;
};

} // namespace yup
