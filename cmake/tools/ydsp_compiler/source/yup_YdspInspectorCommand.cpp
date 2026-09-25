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

static int inspectBundle (const File& file, bool listSources)
{
    auto loaded = YdspBundle::loadFromFile (file);
    if (loaded.failed())
    {
        std::cerr << loaded.getErrorMessage().toStdString() << "\n";
        return 2;
    }

    const auto& bundle = loaded.getReference();
    std::cout << "sources: " << bundle.getSources().size() << "\n"
              << "diagnostics: " << bundle.getDiagnostics().getCount() << "\n"
              << "native targets: " << bundle.getNativeTargets().joinIntoString (", ").toStdString() << "\n"
              << "wasm kernels: " << bundle.getWasmModules().size() << "\n";

    for (size_t i = 0; i < bundle.getNativeArtifacts().size(); ++i)
    {
        const auto& artifact = bundle.getNativeArtifacts()[i];
        size_t bytes = 0;
        for (const auto& kernel : artifact.kernels)
            bytes += kernel.code.size();
        std::cout << bundle.getNativeTargets()[static_cast<int> (i)].toStdString()
                  << ": " << artifact.kernels.size() << " kernels, " << bytes << " bytes\n";
    }

    if (listSources)
    {
        for (const auto& source : bundle.getSources())
            std::cout << source.id.toStdString() << (source.isRoot ? " (root)" : "") << "\n";
    }

    return 0;
}

int runYdspInspectorCommand (int argc, char** argv)
{
    if (argc < 3 || argc > 4 || (argc == 4 && String (argv[3]) != "--list"))
    {
        std::cerr << "Usage: yup_dsp_compiler --inspect <bundle.ydsb> [--list]\n";
        return 2;
    }
    return inspectBundle (File::getCurrentWorkingDirectory().getChildFile (argv[2]), argc == 4);
}
