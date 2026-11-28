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
    The parser checks the syntax only. Every drawing and component value must be a single
    expression, while style and palette values and shader paths are kept verbatim up to their `;`, since some
    of them (such as `transition:` or `drawing:`) are not expressions. Element types, property
    names and the meaning of values are checked by the code consuming the tree, such as
    DrawingStyle and StyleSheet, whose diagnostics can be located in the text with the
    source map of the result. Line and block comments can go anywhere whitespace can.

    @code
    const auto result = YuiParser::parse (text);
    if (result.wasOk())
        auto style = DrawingStyle::create (knob, result.document.getChildWithName ("drawing"));
    else
        DBG (result.diagnostics.front().toString());
    @endcode

    @see DrawingStyle, StyleSheet, YuiWriter

    @tags{UI}
*/
class YUP_API YuiParser
{
public:
    //==============================================================================
    /** The outcome of parsing a text. */
    struct ParseResult
    {
        /** The document, invalid when the text has errors. */
        DataTree document;

        /** The problems found, at their line and column, empty when the text was parsed. */
        std::vector<DataTreeDiagnostic> diagnostics;

        /** Where each element of the document, and each of their properties, was written. */
        DataTreeSourceMap sourceMap;

        /** Returns true if the text was parsed without errors. */
        bool wasOk() const noexcept { return diagnostics.empty(); }
    };

    //==============================================================================
    /** Parses a `.yui` text, stopping at the first error.

        The files the text imports are not read: each `import` stays an element of the document,
        which YuiBuilder reports as unresolved.

        @param text  The text to parse.

        @returns The document, or the diagnostic of the first error.
    */
    static ParseResult parse (const String& text);

    /** Loads the text of an imported file, or returns nothing if it can't be found. */
    using ImportResolver = std::function<std::optional<String> (const String& path)>;

    /** Parses a `.yui` text along with the files it imports.

        Every `import "path";` is replaced by the blocks of the imported text, recursively, so
        the document holds everything in import order and no `import` element. Each path is
        imported once, the first time it is met, which also makes the cycles between imported
        files harmless. The text itself has no path here: a file importing it back imports it
        once more, as any other file (see the overload taking its path).

        Nested import paths are normalized and resolved relative to the directory of the file
        importing them before being passed to @p resolveImport. A path starting with `/` is
        relative to the directory of the text parsed, whatever file imports it.

        The merged document has at most one root component: one in an imported file is an
        error when another text has one too.

        @param text           The text to parse.
        @param resolveImport  Returns the text of an imported path.

        @returns The merged document, or the diagnostic of the first error, whose source is
                 the import path when the error is in an imported file. The positions of
                 the source map carry the import path of the elements coming from imports.
    */
    static ParseResult parse (const String& text, const ImportResolver& resolveImport);

    /** Parses the text of a `.yui` file along with the files it imports, which can import it back.

        This is the parse above for a text that has a path among the files it imports from, as
        the main file a YuiLoader loads has: its path counts as imported already, so a file
        importing the text back imports nothing.

        @param text           The text to parse.
        @param resolveImport  Returns the text of an imported path.
        @param mainPath       The path a file next to the text imports it by, which is its file
                              name. An empty path is the text of no file.

        @returns The merged document, or the diagnostic of the first error.
    */
    static ParseResult parse (const String& text, const ImportResolver& resolveImport, const String& mainPath);

    /** Parses a `.yui` text and builds the stylesheet of its blocks.

        This gives a hand-written interface its stylesheet from text, with no document:

        @code
        auto sheet = YuiParser::parseStyleSheet (text);
        if (sheet.wasOk())
            editor.setStyleSheet (sheet.getReference());
        @endcode

        @param text  The text to parse.

        @returns The sheet, or a failure with the first diagnostic.

        @see StyleSheet::fromTree
    */
    static ResultValue<StyleSheet::Ptr> parseStyleSheet (const String& text);

    YuiParser() = delete;
};

} // namespace yup
