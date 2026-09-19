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
/** A source span with 1-based positions and an exclusive end position.
    Zero positions denote a diagnostic without a source location. */
struct YdspSourceRange
{
    int startLine = 0;
    int startColumn = 0;
    int endLine = 0;
    int endColumn = 0;
    String sourceId; ///< File path or virtual source name; empty uses the current source.
};

/** A compilation diagnostic associated with a source range. */
struct YdspDiagnostic
{
    YdspSeverity severity = YdspSeverity::error;
    String sourceId;
    YdspSourceRange range;
    String code;
    String message;

    // Deprecated source-compatible accessors. New callers should use range.
    int line = 0;
    int column = 0;
};

//==============================================================================
/** The list of diagnostics produced while compiling a YDSP program.

    Diagnostics are source-range-annotated messages: syntax errors, type
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
    /** Sets the source text and its file path or virtual name. */
    void setSource (StringRef source, StringRef sourceId);

    /** Registers additional source text, for example an imported file. */
    void registerSource (StringRef sourceId, StringRef source);

    /** Names the current source and previously unnamed diagnostics. */
    void setSourceId (StringRef sourceId);

    /** Returns the current file path or virtual source name. */
    const String& getSourceId() const noexcept { return currentSourceId; }

    /** Adds an error at the given source range. */
    void addError (YdspSourceRange range, StringRef message);
    /** Adds a warning at the given source range. */
    void addWarning (YdspSourceRange range, StringRef message);
    /** Adds an informational diagnostic at the given source range. */
    void addInfo (YdspSourceRange range, StringRef message);

    /** Adds an error diagnostic at the given 1-based line/column.
        @deprecated Use the source-range overload. */
    void addError (int line, int column, StringRef message);

    /** Adds a warning diagnostic at the given 1-based line/column.
        @deprecated Use the source-range overload. */
    void addWarning (int line, int column, StringRef message);

    /** Adds an informational diagnostic at the given 1-based line/column.
        @deprecated Use the source-range overload. */
    void addInfo (int line, int column, StringRef message);

    //==============================================================================
    /** Appends a diagnostic, resolving an empty source identity to the current source. */
    void add (YdspDiagnostic diagnostic);

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
        followed by up to five source lines (two before and two after the issue),
        with a caret and underline at the range. Headers use path:line:column.
    */
    String toString() const;

private:
    std::vector<YdspDiagnostic> items;
    String currentSourceId;
    std::unordered_map<String, String> sources;
};

} // namespace yup
