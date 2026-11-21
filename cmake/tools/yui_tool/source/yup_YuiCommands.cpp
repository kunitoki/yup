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

#include <yup_core/yup_core.h>

#include <iostream>
#include <string_view>

std::optional<int> checkYuiToolCommand (int argc, char** argv)
{
    const std::string_view mode (argc > 1 ? argv[1] : "");
    const auto isHelp = mode == "--help" || mode == "-h";

    if (mode == "verify" || mode == "lint" || mode == "schema" || mode == "import-svg" || mode == "render" || mode == "inspect")
        return std::nullopt;

#if ! YUP_WASM
    if (mode == "mcp")
        return std::nullopt;
#endif

    std::cerr << "yup_yui_tool verify [--json] [--models <models.json>] <file.yui>...\n"
              << "yup_yui_tool lint [--json] [--strict] [--models <models.json>] <file.yui>...\n"
              << "yup_yui_tool lint [--json] [--strict] [--models <models.json>] --stdin --path <file.yui>\n"
              << "yup_yui_tool schema [--output <file.json>]\n"
              << "yup_yui_tool import-svg <input.svg> [--output <output.yui>] [--name <Name>] [--color-properties]\n"
              << "yup_yui_tool render <file.yui> --output <image.png> [--size <width>x<height>] [--default-size <width>x<height>]\n"
              << "  [--scale <factor>] [--theme dark|light] [--models <models.json>] [--drawing <Name>] [--as <Type>] [--palette <Name>]\n"
              << "yup_yui_tool inspect <file.yui> [--size <width>x<height>] [--default-size <width>x<height>]\n"
              << "  [--theme dark|light] [--models <models.json>] [--drawing <Name>] [--as <Type>] [--palette <Name>]\n"
#if ! YUP_WASM
              << "yup_yui_tool mcp\n"
#endif
        ;

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

#if ! YUP_WASM
    if (mode == "mcp")
        return runYuiMcpCommand (argc, argv);
#endif

    return runYuiImportSvgCommand (argc, argv);
}
