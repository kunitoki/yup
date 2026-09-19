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

// YdspDiagnostics

bool YdspDiagnostics::hasErrors() const noexcept
{
    for (const auto& item : items)
        if (item.severity == YdspSeverity::error)
            return true;

    return false;
}

int YdspDiagnostics::getCount() const noexcept
{
    return static_cast<int> (items.size());
}

const YdspDiagnostic& YdspDiagnostics::getItem (int index) const noexcept
{
    jassert (static_cast<size_t> (index) < items.size());
    return items[static_cast<size_t> (index)];
}

void YdspDiagnostics::setSource (StringRef source)
{
    registerSource (currentSourceId, source);
}

void YdspDiagnostics::setSource (StringRef source, StringRef sourceId)
{
    setSourceId (sourceId);
    setSource (source);
}

void YdspDiagnostics::registerSource (StringRef sourceId, StringRef source)
{
    sources[String (sourceId)] = String (source);
}

void YdspDiagnostics::setSourceId (StringRef sourceId)
{
    const auto previous = sources.find (currentSourceId);
    if (previous != sources.end())
        sources[String (sourceId)] = previous->second;
    currentSourceId = sourceId;
    for (auto& item : items)
        if (item.sourceId.isEmpty())
        {
            item.sourceId = sourceId;
            item.range.sourceId = sourceId;
        }
}

void YdspDiagnostics::addError (YdspSourceRange range, StringRef message)
{
    add ({ YdspSeverity::error, {}, std::move (range), {}, String (message) });
}

void YdspDiagnostics::addWarning (YdspSourceRange range, StringRef message)
{
    add ({ YdspSeverity::warning, {}, std::move (range), {}, String (message) });
}

void YdspDiagnostics::addInfo (YdspSourceRange range, StringRef message)
{
    add ({ YdspSeverity::info, {}, std::move (range), {}, String (message) });
}

void YdspDiagnostics::addError (int line, int column, StringRef message)
{
    addError ({ line, column, line, column }, message);
}

void YdspDiagnostics::addWarning (int line, int column, StringRef message)
{
    addWarning ({ line, column, line, column }, message);
}

void YdspDiagnostics::addInfo (int line, int column, StringRef message)
{
    addInfo ({ line, column, line, column }, message);
}

void YdspDiagnostics::add (YdspDiagnostic diagnostic)
{
    if (diagnostic.range.sourceId.isNotEmpty())
        diagnostic.sourceId = diagnostic.range.sourceId;
    else if (diagnostic.sourceId.isEmpty())
        diagnostic.sourceId = currentSourceId;
    diagnostic.range.sourceId = diagnostic.sourceId;
    diagnostic.line = diagnostic.range.startLine;
    diagnostic.column = diagnostic.range.startColumn;
    items.push_back (std::move (diagnostic));
}

int YdspDiagnostics::mark() const noexcept
{
    return static_cast<int> (items.size());
}

void YdspDiagnostics::rollbackTo (int marker)
{
    if (marker >= 0 && static_cast<size_t> (marker) < items.size())
        items.resize (static_cast<size_t> (marker));
}

String YdspDiagnostics::toString() const
{
    String result;
    for (const auto& item : items)
    {
        if (result.isNotEmpty())
            result += "\n";
        const auto& range = item.range;
        const auto path = item.sourceId.isNotEmpty() ? item.sourceId : String ("<memory>");
        const auto severity = item.severity == YdspSeverity::error ? "error"
                            : item.severity == YdspSeverity::warning ? "warning" : "info";
        result += path;
        if (range.startLine > 0)
            result += ":" + String (range.startLine) + ":" + String (range.startColumn);
        result += ": " + String (severity) + ": " + item.message;

        const auto source = sources.find (item.sourceId);
        if (range.startLine <= 0 || source == sources.end() || source->second.isEmpty())
            continue;
        const auto lines = StringArray::fromLines (source->second);
        if (range.startLine > lines.size())
            continue;
        const int first = jmax (1, range.startLine - 2);
        const int last = jmin (lines.size(), range.startLine + 2);
        const int width = String (last).length();
        for (int line = first; line <= last; ++line)
        {
            const auto& text = lines[line - 1];
            result += "\n  " + String (line).paddedLeft (' ', width) + " | " + text;
            if (line != range.startLine || range.startColumn <= 0)
                continue;
            const int column = jmin (range.startColumn - 1, text.length());
            String padding;
            for (int i = 0; i < column; ++i)
                padding += text[i] == '\t' ? "\t" : " ";
            const int end = range.endLine > line ? text.length()
                          : range.endLine == line ? jmin (text.length(), range.endColumn - 1) : column + 1;
            result += "\n  " + String::repeatedString (" ", width + 3) + padding
                    + "^" + String::repeatedString ("~", jmax (0, end - column - 1));
        }
    }
    return result;
}

} // namespace yup
