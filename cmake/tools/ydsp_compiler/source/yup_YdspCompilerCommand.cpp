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

#include "yup_YdspCommands.h"
#include <yup_dsp_jit/yup_dsp_jit.h>

#include <iostream>

using namespace yup;

static std::optional<YdspTargetTriple> parseTarget (const String& value)
{
    if (value == "macos-arm64")
        return YdspTargetTriple { YdspTargetOperatingSystem::macosTarget, YdspTargetArchitecture::arm64 };

    if (value == "macos-x64")
        return YdspTargetTriple { YdspTargetOperatingSystem::macosTarget, YdspTargetArchitecture::x64 };

    if (value == "linux-arm64")
        return YdspTargetTriple { YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::arm64 };

    if (value == "linux-x64")
        return YdspTargetTriple { YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::x64 };

    if (value == "windows-arm64")
        return YdspTargetTriple { YdspTargetOperatingSystem::windowsTarget, YdspTargetArchitecture::arm64 };

    if (value == "windows-x64")
        return YdspTargetTriple { YdspTargetOperatingSystem::windowsTarget, YdspTargetArchitecture::x64 };

    return {};
}

int runYdspCompilerCommand (int argc, char** argv)
{
    File input;
    File output;
    bool fastMath = false;
    String mainOverride;
    std::vector<YdspTargetTriple> targets;

    for (int i = 1; i < argc; ++i)
    {
        const String argument (argv[i]);

        if (argument == "--fast-math")
        {
            fastMath = true;
            continue;
        }

        if (argument == "--target" && i + 1 < argc)
        {
            const auto target = parseTarget (argv[++i]);
            if (! target)
            {
                std::cerr << "invalid target triple\n";
                return 2;
            }

            targets.push_back (*target);
            continue;
        }

        if (argument == "--main" && i + 1 < argc)
        {
            mainOverride = argv[++i];
            continue;
        }

        if (argument == "--output" && i + 1 < argc)
        {
            output = File::getCurrentWorkingDirectory().getChildFile (argv[++i]);
            continue;
        }

        if (input == File() && ! argument.startsWithChar ('-'))
        {
            input = File::getCurrentWorkingDirectory().getChildFile (argument);
        }
        else
        {
            std::cerr << "invalid argument: " << argv[i] << "\n";
            return 2;
        }
    }

    if (input == File() || output == File())
    {
        std::cerr << "Compilation requires an input file and --output <bundle.ydsb>\n";
        return 2;
    }

    YdspCompiler compiler;

    YdspBundleCompileOptions options;
    options.fastMath = fastMath;
    options.nativeTargets = std::move (targets);

    if (! input.existsAsFile() || (! input.hasFileExtension ("ydsp;ydsp-project"))
        || (mainOverride.isNotEmpty() && ! input.hasFileExtension ("ydsp-project")))
    {
        std::cerr << "Expected an existing .ydsp or .ydsp-project file; --main requires a project\n";
        return 2;
    }
    const auto result = input.hasFileExtension ("ydsp-project")
                          ? compiler.compileProjectBundle (input, options, mainOverride)
                          : compiler.compileBundle (input.loadFileAsString(), options, input.getFullPathName());
    if (! result)
    {
        std::cerr << (compiler.getDiagnostics().hasErrors() ? compiler.getDiagnostics().toString() : result.getErrorMessage()).toStdString() << "\n";
        return 1;
    }

    const auto saved = result.getReference().saveToFile (output);
    if (saved.failed())
    {
        std::cerr << saved.getErrorMessage().toStdString() << "\n";
        return 3;
    }

    return 0;
}
