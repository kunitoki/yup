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
/** Severity of a YDSP compilation diagnostic. */
enum class YdspSeverity
{
    error,
    warning,
    info
};

//==============================================================================
/** A single YDSP compilation diagnostic (message + source location). */
struct YdspDiagnostic
{
    YdspSeverity severity = YdspSeverity::error;
    int line = 0;
    int column = 0;
    String message;
};

//==============================================================================
/** The list of diagnostics produced while compiling a YDSP program.

    Diagnostics are line/column-annotated messages: syntax errors, type
    errors, realtime-safety violations, and informational notes. A compile
    only produces a runnable YdspAudioGraph when no error diagnostics are
    present.
*/
class YdspDiagnostics
{
public:
    /** Default constructor. */
    YdspDiagnostics() = default;

    //==============================================================================
    /** Returns true if any error diagnostics are present. */
    bool hasErrors() const noexcept;

    /** Returns the number of diagnostics. */
    int getCount() const noexcept;

    /** Returns the diagnostic at the given index. */
    const YdspDiagnostic& getItem (int index) const noexcept;

    //==============================================================================
    /** Stores the source text so that toString() can render source lines with
        a caret marker at the diagnostic position. */
    void setSource (StringRef source);

    /** Adds an error diagnostic at the given 1-based line/column. */
    void addError (int line, int column, StringRef message);

    /** Adds a warning diagnostic at the given 1-based line/column. */
    void addWarning (int line, int column, StringRef message);

    /** Adds an informational diagnostic at the given 1-based line/column. */
    void addInfo (int line, int column, StringRef message);

    //==============================================================================
    /** Returns a marker for the current number of diagnostics.

        Paired with rollbackTo(), this lets a *speculative* transform - one that
        is entitled to decide not to apply - analyze something it synthesized
        and then discard whatever that reported. Without it, a bug in such a
        transform would fail the user's compile citing a construct they never
        wrote. */
    int mark() const noexcept;

    /** Discards every diagnostic added after `marker`, which must come from a
        previous mark() on this object. */
    void rollbackTo (int marker);

    /** Returns a human-readable, multi-line rendering of all diagnostics.
        When source text has been set via setSource(), each diagnostic is
        followed by the offending source line and a caret (^) marker.
    */
    String toString() const;

private:
    std::vector<YdspDiagnostic> items;
    String sourceText;
};

} // namespace yup
