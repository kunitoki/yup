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

using namespace yup;

namespace
{

bool writePng (const Image& image, const File& file)
{
    file.deleteFile();

    auto stream = std::make_unique<FileOutputStream> (file);
    if (stream->failedToOpen())
        return false;

    PngImageFormatWriter writer (stream.release(), image.getPixelFormat());
    return writer.writeImage (image);
}

} // namespace

//==============================================================================

int runYuiRenderCommand (int argc, char** argv)
{
    File output;
    float scale = 2.0f;
    YuiToolViewOptions options;
    YuiToolModels models;

    for (int i = 2; i < argc; ++i)
    {
        const String argument (argv[i]);
        const auto hasValue = i + 1 < argc;

        if (argument == "--output" && hasValue)
        {
            output = File::getCurrentWorkingDirectory().getChildFile (argv[++i]);
        }
        else if (argument == "--scale" && hasValue)
        {
            scale = String (argv[++i]).getFloatValue();

            if (scale <= 0.0f || scale > 8.0f)
            {
                std::cerr << "invalid scale, expected a factor up to 8: " << argv[i] << "\n";
                return 2;
            }
        }
        else if (const auto isValid = readYuiToolViewOption (argc, argv, i, options, models))
        {
            if (! *isValid)
                return 2;
        }
        else
        {
            std::cerr << "invalid argument: " << argv[i] << "\n";
            return 2;
        }
    }

    const auto& target = options.target;

    if (options.input == File() || output == File() || (target.drawing.isNotEmpty() && target.palette.isNotEmpty()) || (target.hostType.isNotEmpty() && target.palette.isNotEmpty()))
    {
        std::cerr << "Rendering requires an input file and --output <image.png>, with --drawing or --palette but not both\n";
        return 2;
    }

    if (! options.input.existsAsFile())
    {
        std::cerr << "can't read '" << options.input.getFullPathName() << "'\n";
        return 2;
    }

    YuiToolView view (options, models);
    const auto& shown = view.getShown();

    if (shown.status == YuiToolShown::Status::nothing)
    {
        std::cerr << options.input.getFullPathName() << ": error: nothing to render, " << shown.message << "\n";
        return 1;
    }

    if (shown.status == YuiToolShown::Status::invalidName)
    {
        std::cerr << "invalid name given to --drawing, --palette or --as\n";
        return 2;
    }

    for (const auto& problem : view.findProblems())
        if (problem.isError)
            problem.print();

    if (! view.hasInterface())
    {
        if (shown.target.drawing.isNotEmpty() && shown.target.hostType.isEmpty())
            std::cerr << "hint: a drawing reading the properties of a widget needs it as its host, for example --as Slider\n";

        return 1;
    }

    const auto context = createYuiToolGraphicsContext();
    const auto image = context != nullptr ? view.getComponent()->snapshotToImage (*context, true, scale) : Image();

    if (! image.isValid())
    {
        std::cerr << "no graphics device to render with\n";
        return 3;
    }

    if (! writePng (image, output))
    {
        std::cerr << "can't write '" << output.getFullPathName() << "'\n";
        return 2;
    }

    std::cout << output.getFullPathName() << " " << image.getWidth() << "x" << image.getHeight() << "\n";
    return 0;
}
