/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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
class FileChooser::FileChooserImpl : public Thread
{
public:
    FileChooserImpl (CompletionCallback callback, StringArray arguments, bool allowsMultiple, bool isSave)
        : Thread ("FileChooser")
        , callback (std::move (callback))
        , arguments (std::move (arguments))
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

        ChildProcess process;
        if (process.start (arguments, ChildProcess::wantStdOut))
        {
            MemoryOutputStream outputStream;
            char buffer[4096];

            while (! threadShouldExit())
            {
                const int numBytesRead = process.readProcessOutput (buffer, (int) sizeof (buffer));
                if (numBytesRead > 0)
                    outputStream.write (buffer, (size_t) numBytesRead);
                else if (! process.isRunning())
                    break;
            }

            if (threadShouldExit())
            {
                process.kill();
                result = 1;
            }
            else
            {
                result = (int) process.getExitCode();
                String output = outputStream.toUTF8();

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
            }
        }

        MessageManager::callAsync ([callback = std::move (callback), result, results = std::move (results)]
        {
            callback (result == 0 && results.size() > 0, results);
        });
    }

private:
    CompletionCallback callback;
    StringArray arguments;
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

    StringArray arguments { "zenity", "--file-selection" };

    if (isSave)
        arguments.add ("--save");

    if (canChooseDirectories)
        arguments.add ("--directory");

    if (allowsMultiple && ! isSave)
        arguments.add ("--multiple");

    if (title.isNotEmpty())
        arguments.add ("--title=" + title);

    // A trailing separator makes zenity open inside a directory instead of selecting it in its parent
    if (auto path = startingFile.isDirectory() ? File::addTrailingSeparator (startingFile.getFullPathName())
                                               : startingFile.getFullPathName();
        path.isNotEmpty())
        arguments.add ("--filename=" + path);

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

                arguments.add ("--file-filter=" + pattern);
            }
        }
    }

    impl = std::make_unique<FileChooserImpl> (std::move (callback), std::move (arguments), allowsMultiple, isSave);
    impl->startThread();
}

} // namespace yup
