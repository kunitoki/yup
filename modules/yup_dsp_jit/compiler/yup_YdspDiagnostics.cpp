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
    sourceText = String (source);
}

void YdspDiagnostics::addError (int line, int column, StringRef message)
{
    items.push_back ({ YdspSeverity::error, line, column, String (message) });
}

void YdspDiagnostics::addWarning (int line, int column, StringRef message)
{
    items.push_back ({ YdspSeverity::warning, line, column, String (message) });
}

void YdspDiagnostics::addInfo (int line, int column, StringRef message)
{
    items.push_back ({ YdspSeverity::info, line, column, String (message) });
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

        switch (item.severity)
        {
            case YdspSeverity::error:
                result += "error";
                break;
            case YdspSeverity::warning:
                result += "warning";
                break;
            case YdspSeverity::info:
                result += "info";
                break;
        }

        if (item.line > 0)
            result += ":" + String (item.line) + ":" + String (item.column);

        result += ": " + item.message;

        // Render the source line with a caret marker when source text is available.
        if (item.line > 0 && sourceText.isNotEmpty())
        {
            int lineStart = 0;

            for (int l = 1; l < item.line && lineStart < sourceText.length(); ++lineStart)
                if (sourceText[lineStart] == '\n')
                    ++l;

            const auto lineEnd = sourceText.indexOf (lineStart, "\n");
            const auto sourceLine = lineEnd >= 0 ? sourceText.substring (lineStart, lineEnd)
                                                 : sourceText.substring (lineStart);

            // Strip a trailing carriage return if present.
            auto displayLine = sourceLine;
            if (! displayLine.isEmpty() && displayLine.getLastCharacter() == '\r')
                displayLine = displayLine.dropLastCharacters (1);

            // Show line number and source.
            result += "\n  " + String (item.line) + " | " + displayLine;

            // Caret marker at the column (1-based).
            if (item.column > 0)
            {
                const auto padLength = String (item.line).length() + 3 + item.column - 1;
                result += "\n  " + String::repeatedString (" ", padLength) + "^";
            }
        }
    }

    return result;
}

} // namespace yup
