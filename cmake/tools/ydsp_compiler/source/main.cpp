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

#include <iostream>
#include <string_view>

static void printHelp()
{
    std::cerr << "yup_dsp_compiler <input.ydsp|input.ydsp-project> --output <output.ydsb> [--main NAME] [--target <os>-<arch>]... [--fast-math]\n"
              << "yup_dsp_compiler --inspect <bundle.ydsb> [--list]\n"
              << "yup_dsp_compiler devices [--json]\n"
              << "yup_dsp_compiler run <input.ydsp|input.ydsp-project> [--main NAME] [--hotreload]\n"
              << "  [--audio-type TYPE] [--audio-input NAME|none] [--audio-output NAME]\n"
              << "  [--midi-input ID|NAME|none] [--midi-output ID|NAME|none]\n"
              << "  [--sample-rate HZ] [--block-size SAMPLES] [--verbose] [--test-note 0..127]\n"
              << "yup_dsp_compiler --lsp\n";
}

int main (int argc, char** argv)
{
    if (argc < 2)
    {
        printHelp();
        return 2;
    }
    const std::string_view mode (argv[1]);
    if (mode == "--help" || mode == "-h")
    {
        printHelp();
        return 0;
    }
    if (mode == "--lsp")
        return runYdspLspCommand();
    if (mode == "--inspect")
        return runYdspInspectorCommand (argc, argv);
    if (mode == "devices")
        return runYdspDevicesCommand (argc, argv);
    if (mode == "run")
        return runYdspPlayerCommand (argc, argv);
    return runYdspCompilerCommand (argc, argv);
}
