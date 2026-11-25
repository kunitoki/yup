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
    A problem found in a tree that was parsed from text, such as a `.yui` document.

    A syntax error carries the position where parsing stopped. The problems found later, by
    the code reading the tree, carry the element they come from, and the property of that
    element when they are about a value, which a DataTreeSourceMap turns into a position in
    the text.

    @code
    DataTreeDiagnostic problem { .message = "Expected a number", .element = slider, .property = "value" };

    DBG (sourceMap.locate (problem).toString());   // "ui.yui:12:9: Expected a number"
    @endcode

    @see DataTreeSourceMap, DataTree
*/
struct YUP_API DataTreeDiagnostic
{
    /** Severity level of a diagnostic. */
    enum class Severity
    {
        error,  /**< Prevents what the tree describes from being loaded or applied. */
        warning /**< Advisory problem that does not block evaluation or loading. */
    };

    /** What went wrong. */
    String message;

    /** The element of the tree the problem comes from, if known. */
    DataTree element;

    /** The property of the element the problem is about, null when it is about the whole element. */
    Identifier property;

    /** The text the problem is in when the tree was parsed from several, such as an imported file, empty for the main text. */
    String source;

    /** The 1-based line of the problem, 0 when the position is unknown. */
    int line = 0;

    /** The 1-based column of the problem, counted in characters. */
    int column = 0;

    /** The severity of the diagnostic (`Severity::error` by default). */
    Severity severity = Severity::error;

    /** Returns true when @ref severity is `Severity::error`. */
    bool isError() const noexcept { return severity == Severity::error; }

    /** Returns true when @ref severity is `Severity::warning`. */
    bool isWarning() const noexcept { return severity == Severity::warning; }

    /** Returns the diagnostic as `source:line:column: [warning: ]message`, omitting the parts that are unknown. */
    String toString() const;

    /** Returns the diagnostics as strings, joined by a separator. */
    static String join (const std::vector<DataTreeDiagnostic>& diagnostics, StringRef separator = "\n");
};

} // namespace yup
