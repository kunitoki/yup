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

//==============================================================================
/** Fills the image behind the interface with the background of the palette its root is drawn with. */
class Backdrop final : public Component
{
public:
    explicit Backdrop (Component& rootToShow)
        : root (rootToShow)
    {
        addAndMakeVisible (root);
    }

    void paint (Graphics& g) override
    {
        g.setFillColor (ApplicationTheme::getGlobalTheme()->getPalette (root).getColor (ThemePalette::Role::background));
        g.fillAll();
    }

    void resized() override
    {
        root.setBounds (getLocalBounds());
    }

private:
    Component& root;
};

//==============================================================================
// Drawing needs a device of the GPU but no window, which only some platforms give
std::unique_ptr<GraphicsContext> createGraphicsContext()
{
#if YUP_MAC
    return GraphicsContext::createContext (GpuPlatform::Metal, {});
#elif YUP_WINDOWS
    GraphicsContext::Options options;
    options.allowHeadlessRendering = true;

    return GraphicsContext::createContext (GpuPlatform::Direct3D, options);
#else
    return nullptr;
#endif
}

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
    const auto workingDirectory = File::getCurrentWorkingDirectory();
    File input;
    File output;
    String drawing;
    String palette;
    String hostType;
    Size<float> size;
    Size<float> fallbackSize;
    float scale = 2.0f;
    auto mode = ThemePalette::Mode::dark;
    YuiToolModels models;

    for (int i = 2; i < argc; ++i)
    {
        const String argument (argv[i]);
        const auto hasValue = i + 1 < argc;

        if (argument == "--output" && hasValue)
        {
            output = workingDirectory.getChildFile (argv[++i]);
        }
        else if (argument == "--models" && hasValue)
        {
            if (! models.load (argv[++i]))
                return 2;
        }
        else if (argument == "--drawing" && hasValue)
        {
            drawing = argv[++i];
        }
        else if (argument == "--palette" && hasValue)
        {
            palette = argv[++i];
        }
        else if (argument == "--as" && hasValue)
        {
            hostType = argv[++i];
        }
        else if ((argument == "--size" || argument == "--default-size") && hasValue)
        {
            const String value (argv[++i]);
            const Size<float> parsed { value.upToFirstOccurrenceOf ("x", false, true).getFloatValue(), value.fromFirstOccurrenceOf ("x", false, true).getFloatValue() };

            if (parsed.getWidth() < 1.0f || parsed.getHeight() < 1.0f)
            {
                std::cerr << "invalid size, expected <width>x<height>: " << value << "\n";
                return 2;
            }

            (argument == "--size" ? size : fallbackSize) = parsed;
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
        else if (argument == "--theme" && hasValue && (String (argv[i + 1]) == "dark" || String (argv[i + 1]) == "light"))
        {
            mode = String (argv[++i]) == "light" ? ThemePalette::Mode::light : ThemePalette::Mode::dark;
        }
        else if (input == File() && ! argument.startsWithChar ('-'))
        {
            input = workingDirectory.getChildFile (argument);
        }
        else
        {
            std::cerr << "invalid argument: " << argv[i] << "\n";
            return 2;
        }
    }

    if (input == File() || output == File() || (drawing.isNotEmpty() && palette.isNotEmpty()) || (hostType.isNotEmpty() && palette.isNotEmpty()))
    {
        std::cerr << "Rendering requires an input file and --output <image.png>, with --drawing or --palette but not both\n";
        return 2;
    }

    if (! input.existsAsFile())
    {
        std::cerr << "can't read '" << input.getFullPathName() << "'\n";
        return 2;
    }

    ApplicationTheme::getGlobalTheme()->setPalette (ThemePalette ({}, mode));

    YuiDocument document;
    YuiLoader loader (document);
    models.attachTo (document);

    const auto shown = loadYuiToolTarget (document, loader, input, { drawing, palette, hostType });

    if (shown.status == YuiToolShown::Status::nothing)
    {
        std::cerr << input.getFullPathName() << ": error: nothing to render, " << shown.message << "\n";
        return 1;
    }

    if (shown.status == YuiToolShown::Status::invalidName)
    {
        std::cerr << "invalid name given to --drawing, --palette or --as\n";
        return 2;
    }

    for (const auto& problem : findYuiToolProblems (document, input, shown.result))
        if (problem.isError)
            problem.print();

    if (shown.result.failed())
    {
        if (shown.target.drawing.isNotEmpty() && shown.target.hostType.isEmpty())
            std::cerr << "hint: a drawing reading the properties of a widget needs it as its host, for example --as Slider\n";

        return 1;
    }

    auto& root = *document.getRoot();

    // The size asked on the command line, else the one the root asks for, else a default one
    if (size.getWidth() < 1.0f)
    {
        if (root.getWidth() >= 1.0f && root.getHeight() >= 1.0f)
            size = root.getSize();
        else
            size = fallbackSize.getWidth() >= 1.0f ? fallbackSize : shown.defaultSize;
    }

    Backdrop backdrop (root);
    backdrop.setSize (size);

    const auto context = createGraphicsContext();
    const auto image = context != nullptr ? backdrop.snapshotToImage (*context, true, scale) : Image();

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
