/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

static bool runZenityDialog (const String& title, const String& startingPath, const String& filters, int flags, Array<File>& results)
{
    const bool isSave = (flags & FileChooser::saveMode) != 0;
    const bool canChooseDirectories = (flags & FileChooser::canSelectDirectories) != 0;
    const bool allowsMultiple = (flags & FileChooser::canSelectMultipleItems) != 0;

    String command = "zenity --file-selection";

    if (isSave)
        command += " --save";
    if (canChooseDirectories)
        command += " --directory";
    if (allowsMultiple && ! isSave)
        command += " --multiple";
    if (title.isNotEmpty())
        command += " --title=\"" + title + "\"";
    if (startingPath.isNotEmpty())
        command += " --filename=\"" + startingPath + "\"";

    // Add file filters for zenity
    if (filters.isNotEmpty() && ! canChooseDirectories)
    {
        StringArray extensions = StringArray::fromTokens (filters, ";,", String());
        for (const auto& ext : extensions)
        {
            String pattern = ext.trim();
            if (pattern.isNotEmpty())
            {
                if (! pattern.startsWith ("*"))
                    pattern = "*." + pattern;

                command += " --file-filter=\"" + pattern + "\"";
            }
        }
    }

    command += " 2>/dev/null";

    FILE* pipe = popen (command.toUTF8(), "r");
    if (pipe == nullptr)
        return false;

    char buffer[4096];
    String output;

    while (fgets (buffer, sizeof (buffer), pipe) != nullptr)
    {
        output += String::fromUTF8 (buffer);
    }

    int result = pclose (pipe);

    if (result == 0 && output.isNotEmpty())
    {
        output = output.trim();

        if (allowsMultiple && ! isSave)
        {
            StringArray paths = StringArray::fromTokens (output, "|", String());
            for (const auto& path : paths)
            {
                if (path.isNotEmpty())
                    results.add (File (path));
            }
        }
        else
        {
            results.add (File (output));
        }

        return results.size() > 0;
    }

    return false;
}

void FileChooser::showPlatformDialog (int flags, Component* previewComponent)
{
    const bool isSave = (flags & saveMode) != 0;
    const bool canChooseFiles = (flags & canSelectFiles) != 0;
    const bool canChooseDirectories = (flags & canSelectDirectories) != 0;
    const bool allowsMultiple = (flags & canSelectMultipleItems) != 0;

    // First try zenity (works on most Linux distributions)
    if (runZenityDialog (title, startingFile.getFullPathName(), filters, flags, results))
        return;
}

} // namespace yup
