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
#include "yup_YuiToolModels.h"
#include "yup_YuiToolView.h"

#include <algorithm>
#include <iostream>
#include <string_view>
#include <utility>

using namespace yup;

std::optional<int> checkYuiToolCommand (int argc, char** argv)
{
    const std::string_view mode (argc > 1 ? argv[1] : "");

    const auto asksForHelp = [] (const std::string_view argument)
    {
        return argument == "--help" || argument == "-h";
    };

    auto isCommand = mode == "verify" || mode == "lint" || mode == "schema" || mode == "import-svg" || mode == "render" || mode == "inspect"
                  || mode == "outline" || mode == "complete" || mode == "definition" || mode == "hover";

#if ! YUP_WASM
    isCommand = isCommand || mode == "mcp";
#endif

    // A command asked for help is told the usage of them all
    const auto isHelp = asksForHelp (mode) || (isCommand && std::any_of (argv + 2, argv + argc, asksForHelp));

    if (isCommand && ! isHelp)
        return std::nullopt;

    if (mode == "--version")
    {
        std::cout << "yup_yui_tool " << YUP_YUI_TOOL_VERSION << "\n";
        return 0;
    }

    // The usage that was asked for is the output of the tool, the one of a mistake is an error
    auto& usage = isHelp ? std::cout : std::cerr;

    usage << "yup_yui_tool verify [--json] [--models <models.json>] [--embedded-base <dir> --embedded <list file>] <file.yui>...\n"
          << "yup_yui_tool lint [--json] [--strict] [--models <models.json>] <file.yui>...\n"
          << "yup_yui_tool lint [--json] [--strict] [--models <models.json>] --stdin --path <file.yui>\n"
          << "yup_yui_tool schema [--output <file.json>]\n"
          << "yup_yui_tool import-svg <input.svg> [--output <output.yui>] [--name <Name>] [--color-properties]\n"
          << "yup_yui_tool render <file.yui> --output <image.png> [--size <width>x<height>] [--default-size <width>x<height>]\n"
          << "  [--scale <factor>] [--theme dark|light] [--models <models.json>] [--drawing <Name>] [--as <Type>] [--palette <Name>]\n"
          << "  [--element <id|path> [--margin <pixels>]] [--region <x>,<y>,<width>,<height>] [--guides <id|path>[,<id|path>...]]\n"
          << "yup_yui_tool inspect <file.yui> [--size <width>x<height>] [--default-size <width>x<height>]\n"
          << "  [--theme dark|light] [--models <models.json>] [--drawing <Name>] [--as <Type>] [--palette <Name>] [--element <id|path>]\n"
          << "yup_yui_tool outline <file.yui>\n"
          << "yup_yui_tool complete <file.yui> --at <line>:<column> [--models <models.json>]\n"
          << "yup_yui_tool definition <file.yui> --at <line>:<column>\n"
          << "yup_yui_tool hover <file.yui> --at <line>:<column>\n"
#if ! YUP_WASM
          << "yup_yui_tool mcp\n"
#endif
          << "yup_yui_tool [<command>] --help\n"
          << "yup_yui_tool --version\n";

    return isHelp ? 0 : 2;
}

int runYuiToolCommand (int argc, char** argv)
{
    const std::string_view mode (argv[1]);

    if (mode == "verify")
        return runYuiVerifyCommand (argc, argv, false);

    if (mode == "lint")
        return runYuiVerifyCommand (argc, argv, true);

    if (mode == "schema")
        return runYuiSchemaCommand (argc, argv);

    if (mode == "render")
        return runYuiRenderCommand (argc, argv);

    if (mode == "inspect")
        return runYuiInspectCommand (argc, argv);

    if (mode == "outline")
        return runYuiOutlineCommand (argc, argv);

    if (mode == "complete")
        return runYuiCompleteCommand (argc, argv);

    if (mode == "definition")
        return runYuiDefinitionCommand (argc, argv);

    if (mode == "hover")
        return runYuiHoverCommand (argc, argv);

#if ! YUP_WASM
    if (mode == "mcp")
        return runYuiMcpCommand (argc, argv);
#endif

    return runYuiImportSvgCommand (argc, argv);
}

//==============================================================================

int refuseYuiToolCommand (const Result& reason)
{
    std::cerr << reason.getErrorMessage() << "\n";
    return 2;
}

const char* readYuiToolOption (int argc, char** argv, int& index, const char* option)
{
    return std::string_view (argv[index]) == option && index + 1 < argc ? argv[++index] : nullptr;
}

Result readYuiToolViewOption (int argc, char** argv, int& index, YuiToolViewOptions& options, YuiToolModels& models)
{
    const String argument (argv[index]);
    const auto hasValue = index + 1 < argc;

    if (const auto* path = readYuiToolOption (argc, argv, index, "--models"))
    {
        options.hasModels = true;
        return models.load (path);
    }

    const std::pair<const char*, String*> named[] { { "--drawing", &options.target.drawing },
                                                    { "--palette", &options.target.palette },
                                                    { "--as", &options.target.hostType },
                                                    { "--element", &options.element } };

    for (const auto& [option, name] : named)
    {
        if (const auto* given = readYuiToolOption (argc, argv, index, option))
        {
            *name = given;
            return Result::ok();
        }
    }

    if ((argument == "--size" || argument == "--default-size") && hasValue)
    {
        const String value (argv[++index]);
        const auto size = parseYuiToolSize (value);

        if (! size.has_value())
            return Result::fail ("invalid size, expected <width>x<height> in logical pixels, each from 1 to " + String (static_cast<int> (maximumYuiToolSize)) + ": " + value);

        (argument == "--size" ? options.size : options.fallbackSize) = *size;
        return Result::ok();
    }

    if (argument == "--theme" && hasValue && (String (argv[index + 1]) == "dark" || String (argv[index + 1]) == "light"))
    {
        options.mode = String (argv[++index]) == "light" ? ThemePalette::Mode::light : ThemePalette::Mode::dark;
        return Result::ok();
    }

    if (options.input == File() && ! argument.startsWithChar ('-'))
    {
        options.input = File::getCurrentWorkingDirectory().getChildFile (argument);
        return Result::ok();
    }

    return Result::fail ("invalid argument: " + String::fromUTF8 (argv[index]));
}
