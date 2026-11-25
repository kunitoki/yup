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

#include "yup_YuiCommands.h"
#include "yup_YuiToolSupport.h"

#include <iostream>
#include <iterator>
#include <optional>
#include <string>

using namespace yup;

int runYuiVerifyCommand (int argc, char** argv, bool includeWarnings)
{
    StringArray paths;
    String unsavedPath;
    YuiToolModels models;
    bool json = false;
    bool strict = false;
    bool readStdin = false;

    for (int i = 2; i < argc; ++i)
    {
        const String argument (argv[i]);

        if (argument == "--json")
        {
            json = true;
            continue;
        }

        if (argument == "--models" && i + 1 < argc)
        {
            if (! models.load (argv[++i]))
                return 2;

            continue;
        }

        if (includeWarnings && argument == "--strict")
        {
            strict = true;
            continue;
        }

        if (includeWarnings && argument == "--stdin")
        {
            readStdin = true;
            continue;
        }

        if (includeWarnings && argument == "--path" && i + 1 < argc)
        {
            unsavedPath = argv[++i];
            continue;
        }

        if (argument.startsWithChar ('-'))
        {
            std::cerr << "invalid argument: " << argv[i] << "\n";
            return 2;
        }

        paths.add (argument);
    }

    if (readStdin ? (unsavedPath.isEmpty() || ! paths.isEmpty()) : (unsavedPath.isNotEmpty() || paths.isEmpty()))
    {
        std::cerr << "Checking requires .yui files, or --stdin with --path <file.yui>\n";
        return 2;
    }

    const auto workingDirectory = File::getCurrentWorkingDirectory();
    std::optional<String> unsavedText;

    if (readStdin)
    {
        const std::string input { std::istreambuf_iterator<char> (std::cin), std::istreambuf_iterator<char>() };

        unsavedText = String::fromUTF8 (input.data(), static_cast<int> (input.size()));
        paths.add (unsavedPath);
    }
    else
    {
        for (const auto& path : paths)
        {
            if (! workingDirectory.getChildFile (path).existsAsFile())
            {
                std::cerr << "can't read '" << path << "'\n";
                return 2;
            }
        }
    }

    Array<var> records;
    int numErrors = 0;
    int numWarnings = 0;

    for (const auto& path : paths)
    {
        for (const auto& problem : findYuiToolFileProblems (workingDirectory.getChildFile (path), unsavedText, models))
        {
            if (! problem.isError && ! includeWarnings)
                continue;

            ++(problem.isError ? numErrors : numWarnings);

            if (json)
                records.add (problem.toJson());
            else
                problem.print();
        }
    }

    if (json)
    {
        auto* result = new DynamicObject();
        result->setProperty ("diagnostics", var (records));

        std::cout << JSON::toString (var (result)).toStdString() << "\n";
    }

    return numErrors > 0 || (strict && numWarnings > 0) ? 1 : 0;
}
