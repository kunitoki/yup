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

#include <yup_gui/yup_gui.h>

#include <iostream>

using namespace yup;

int runYuiImportSvgCommand (int argc, char** argv)
{
    const auto workingDirectory = File::getCurrentWorkingDirectory();
    File input;
    File output;
    YuiSvgImporter::Options options;

    for (int i = 2; i < argc; ++i)
    {
        const String argument (argv[i]);

        if (argument == "--output" && i + 1 < argc)
        {
            output = workingDirectory.getChildFile (argv[++i]);
            continue;
        }

        if (argument == "--name" && i + 1 < argc)
        {
            options.name = argv[++i];
            continue;
        }

        if (argument == "--color-properties")
        {
            options.colorProperties = true;
            continue;
        }

        if (input == File() && ! argument.startsWithChar ('-'))
        {
            input = workingDirectory.getChildFile (argument);
        }
        else
        {
            std::cerr << "invalid argument: " << argv[i] << "\n";
            return 2;
        }
    }

    if (input == File())
    {
        std::cerr << "Importing requires an input file: import-svg <input.svg>\n";
        return 2;
    }

    if (! input.existsAsFile())
    {
        std::cerr << "can't read '" << input.getFullPathName() << "'\n";
        return 2;
    }

    const auto imported = YuiSvgImporter::importSvg (input, options);

    if (! imported.wasOk())
    {
        std::cerr << input.getFullPathName() << ": error: not an SVG\n";
        return 1;
    }

    for (const auto& warning : imported.warnings)
        std::cerr << input.getFullPathName() << ": warning: " << warning << "\n";

    const auto text = imported.toText();

    if (output == File())
    {
        std::cout << text.toStdString();
        return 0;
    }

    if (! output.replaceWithText (text, false, false, "\n"))
    {
        std::cerr << "can't write '" << output.getFullPathName() << "'\n";
        return 2;
    }

    return 0;
}
