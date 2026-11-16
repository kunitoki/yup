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

String YuiDiagnostic::toString() const
{
    String prefix;

    if (source.isNotEmpty())
        prefix << source << ":";

    if (line > 0)
        prefix << line << ":" << column << ":";

    const String body = severity == Severity::warning ? ("warning: " + message) : message;

    return prefix.isEmpty() ? body : prefix + " " + body;
}

String YuiDiagnostic::join (const std::vector<YuiDiagnostic>& diagnostics, StringRef separator)
{
    StringArray lines;

    for (const auto& diagnostic : diagnostics)
        lines.add (diagnostic.toString());

    return lines.joinIntoString (separator);
}

//==============================================================================

void YuiSourceMap::add (const DataTree& element, const Identifier& property, int line, int column, const String& source)
{
    positions[{ getPath (element), property.toString() }] = { source, line, column };
}

YuiDiagnostic YuiSourceMap::locate (YuiDiagnostic diagnostic) const
{
    if (diagnostic.line > 0 || ! diagnostic.element.isValid())
        return diagnostic;

    // The path goes from the element up, so dropping its front moves to the parent
    Key key { getPath (diagnostic.element), diagnostic.property.toString() };

    for (;;)
    {
        if (const auto found = positions.find (key); found != positions.end())
        {
            diagnostic.source = found->second.source;
            diagnostic.line = found->second.line;
            diagnostic.column = found->second.column;
            return diagnostic;
        }

        if (key.second.isNotEmpty())
            key.second = {};
        else if (! key.first.empty())
            key.first.erase (key.first.begin());
        else
            return diagnostic;
    }
}

std::vector<int> YuiSourceMap::getPath (const DataTree& element)
{
    std::vector<int> path;

    for (auto child = element, parent = element.getParent(); parent.isValid(); child = parent, parent = parent.getParent())
        path.push_back (parent.indexOf (child));

    return path;
}

} // namespace yup
