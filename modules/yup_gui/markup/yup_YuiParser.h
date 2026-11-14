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
    - `repeat i in 0 .. 11 { ... }` becomes a `repeat` element with `name`, `from` and `to`;
    - `import "knob.yui";` becomes an `import` element with a `path`;
    - `palette Ember { colors: "..."; accent: #ff9a1f; }` becomes a `palette` element whose
      properties are the values as written, with quotes removed;
    - `style { ... }` becomes a `style` element, where `Slider:hover { thumbColor: c; }`
      becomes a `rule` with a `selector` and one `declaration` (`name`, `value`) per entry,
      and `@keyframes flash { from { ... } 50% { ... } to { ... } }` becomes a `keyframes`
      element holding one `keyframe` per step, its `at` position going from 0 to 1;
    - the root component, a capitalized type such as `Component { Slider { id: knob; } }`,
      becomes an element of that type, its properties being expression texts and its
      children nested elements, as YuiBuilder expects them. A document has one at most.

    The parser checks the syntax only. Every drawing and component value must be a single expression, while
    style and palette values are kept verbatim up to their `;`, since some of them (such as
    `transition:` or `drawing:`) are not expressions. Element types, property names and the
    meaning of values are checked by the code consuming the tree, such as DrawingProgram and
    StyleSheet. Line and block comments can go anywhere whitespace can.

    The `tokens` and `component` blocks of the language are not supported yet.

    @code
    const auto result = YuiParser::parse (text);
    if (result.wasOk())
        auto style = DrawingStyle::create (knob, result.document.getChildWithName ("drawing"));
    else
        DBG (result.diagnostics.front().toString());
    @endcode

    @see DrawingProgram, StyleSheet

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

        /** The imported file the problem is in, empty for the text passed to the parser. */
        String source;

        /** Returns the diagnostic as `line:column: message`, prefixed by the source if any. */
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

    /** Loads the text of an imported file, or returns nothing if it can't be found. */
    using ImportResolver = std::function<std::optional<String> (const String& path)>;

    /** Parses a `.yui` text along with the files it imports.

        Every `import "path";` is replaced by the blocks of the imported text, recursively, so
        the document holds everything in import order and no `import` element. Each path is
        imported once, the first time it is met, which also makes import cycles harmless.
        Paths are passed to the resolver as written, so it decides what they are relative to.

        @param text           The text to parse.
        @param resolveImport  Returns the text of an imported path.

        @returns The merged document, or the diagnostic of the first error, whose source is
                 the import path when the error is in an imported file.
    */
    static ParseResult parse (const String& text, const ImportResolver& resolveImport);

    YuiParser() = delete;
};

} // namespace yup
