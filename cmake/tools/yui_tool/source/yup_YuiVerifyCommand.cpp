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
#include "yup_YuiToolCheck.h"
#include "yup_YuiToolModels.h"

#include <iostream>
#include <iterator>
#include <optional>
#include <string>

using namespace yup;

int runYuiVerifyCommand (int argc, char** argv, bool includeWarnings)
{
    StringArray paths;
    String unsavedPath;
    String embeddedBase;
    String embeddedList;
    YuiToolModels models;
    bool hasModels = false;
    bool json = false;
    bool strict = false;
    bool readStdin = false;

    for (int i = 2; i < argc; ++i)
    {
        const String argument (argv[i]);

        if (argument == "--json")
        {
            json = true;
        }
        else if (const auto* given = readYuiToolOption (argc, argv, i, "--models"))
        {
            if (const auto loaded = models.load (given); loaded.failed())
                return refuseYuiToolCommand (loaded);

            hasModels = true;
        }
        else if (const auto* folder = includeWarnings ? nullptr : readYuiToolOption (argc, argv, i, "--embedded-base"))
        {
            embeddedBase = folder;
        }
        else if (const auto* list = includeWarnings ? nullptr : readYuiToolOption (argc, argv, i, "--embedded"))
        {
            embeddedList = list;
        }
        else if (includeWarnings && argument == "--strict")
        {
            strict = true;
        }
        else if (includeWarnings && argument == "--stdin")
        {
            readStdin = true;
        }
        else if (const auto* unsaved = includeWarnings ? readYuiToolOption (argc, argv, i, "--path") : nullptr)
        {
            unsavedPath = unsaved;
        }
        else if (argument.startsWithChar ('-'))
        {
            std::cerr << "invalid argument: " << argv[i] << "\n";
            return 2;
        }
        else
        {
            paths.add (argument);
        }
    }

    if (readStdin ? (unsavedPath.isEmpty() || ! paths.isEmpty()) : (unsavedPath.isNotEmpty() || paths.isEmpty()))
    {
        std::cerr << "Checking requires .yui files, or --stdin with --path <file.yui>\n";
        return 2;
    }

    if (embeddedBase.isEmpty() != embeddedList.isEmpty())
    {
        std::cerr << "--embedded-base <dir> and --embedded <list file> go together\n";
        return 2;
    }

    const auto workingDirectory = File::getCurrentWorkingDirectory();
    std::optional<String> unsavedText;
    std::optional<YuiToolEmbedded> embedded;

    // The files a build embeds: one path a line, from the folder they are embedded from
    if (embeddedList.isNotEmpty())
    {
        const auto listFile = workingDirectory.getChildFile (embeddedList);

        if (! listFile.existsAsFile() || ! workingDirectory.getChildFile (embeddedBase).isDirectory())
        {
            std::cerr << "can't read '" << (listFile.existsAsFile() ? embeddedBase : embeddedList) << "'\n";
            return 2;
        }

        embedded = YuiToolEmbedded { workingDirectory.getChildFile (embeddedBase), StringArray::fromLines (listFile.loadFileAsString()) };
        embedded->paths.trim();
        embedded->paths.removeEmptyStrings();
    }

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
        const auto file = workingDirectory.getChildFile (path);

        // Without models given, each file is checked with those next to it
        YuiToolModels beside;

        if (const auto modelsFile = findYuiToolModelsFile (file); ! hasModels && modelsFile.existsAsFile())
            if (const auto loaded = beside.load (modelsFile.getFullPathName()); loaded.failed())
                return refuseYuiToolCommand (loaded);

        for (const auto& problem : findYuiToolFileProblems (file, unsavedText, hasModels ? models : beside, embedded.has_value() ? &*embedded : nullptr))
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
        std::cout << JSON::toString (var::object ({ { "diagnostics", var (records) } })).toStdString() << "\n";

    return numErrors > 0 || (strict && numWarnings > 0) ? 1 : 0;
}
