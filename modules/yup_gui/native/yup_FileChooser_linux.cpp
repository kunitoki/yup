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

//==============================================================================
class FileChooserImpl : public Thread
{
public:
    FileChooserImpl (CompletionCallback callback, String command, bool allowsMultiple, bool isSave)
        : Thread ("FileChooser")
        , callback (std::move (callback))
        , command (std::move (command))
        , allowsMultiple (allowsMultiple)
        , isSave (isSave)
    {
    }

    ~FileChooserImpl() override
    {
        stopThread (2000);
    }

    void run() override
    {
        int result = 1;

        if (FILE* pipe = popen (command.toUTF8(), "r"))
        {
            int fd = fileno (pipe);
            int flags = fcntl (fd, F_GETFL, 0);
            flags |= O_NONBLOCK;
            fcntl (fd, F_SETFL, flags);

            char buffer[4096];
            String output;

            while (! threadShouldExit())
            {
                while (fgets (buffer, sizeof (buffer), pipe) != nullptr && ! threadShouldExit())
                    output += String::fromUTF8 (buffer);

                result = pclose (pipe);
                if (threadShouldExit())
                {
                    result = 1;
                    break;
                }

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
                }

                break;
            }
        }

        MessageManager::callAsync ([callback = std::move (callback), result, std::move (results)]
        {
            callback (result == 0 && results.size() > 0, results);
        });
    }

private:
    CompletionCallback callback;
    String command;
    bool allowsMultiple;
    bool isSave;
    Array<File> results;
};

//==============================================================================
void FileChooser::showPlatformDialog (CompletionCallback callback, int flags)
{
    const bool isSave = (flags & saveMode) != 0;
    const bool canChooseFiles = (flags & canSelectFiles) != 0;
    const bool canChooseDirectories = (flags & canSelectDirectories) != 0;
    const bool allowsMultiple = (flags & canSelectMultipleItems) != 0;

    Array<File> results;

    String command = "zenity --file-selection";

    if (isSave)
        command += " --save";

    if (canChooseDirectories)
        command += " --directory";

    if (allowsMultiple && ! isSave)
        command += " --multiple";

    if (title.isNotEmpty())
        command += " --title=\"" + title + "\"";

    if (auto path = startingFile.getFullPathName(); path.isNotEmpty())
        command += " --filename=\"" + path + "\"";

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

    impl = std::make_unique<FileChooserImpl> (std::move (callback), command, allowsMultiple, isSave);
    impl->startThread();
}

} // namespace yup
