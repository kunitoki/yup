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
#include "yup_YuiInspectCommand.h"
#include "yup_YuiToolGuides.h"
#include "yup_YuiToolModels.h"
#include "yup_YuiToolResources.h"
#include "yup_YuiToolView.h"

#include <iostream>

using namespace yup;

namespace
{

// The file is replaced once the image is whole, so one that can't be written leaves what was there
bool writePng (const Image& image, const File& file)
{
    MemoryBlock data;

    return encodeYuiToolPng (image, data) && file.replaceWithData (data.getData(), data.getSize());
}

// Reads a part of an interface written as `<x>,<y>,<width>,<height>`
std::optional<Rectangle<float>> parseRegion (const String& text)
{
    const auto numbers = StringArray::fromTokens (text, ",", "");

    if (numbers.size() != 4)
        return std::nullopt;

    for (const auto& number : numbers)
        if (! isYuiToolNumber (number.trim()))
            return std::nullopt;

    return Rectangle<float> (numbers[0].getFloatValue(), numbers[1].getFloatValue(), numbers[2].getFloatValue(), numbers[3].getFloatValue());
}

} // namespace

//==============================================================================

int runYuiRenderCommand (int argc, char** argv)
{
    File output;
    float scale = 2.0f;
    float margin = 8.0f;
    std::optional<Rectangle<float>> region;
    StringArray guided;
    YuiToolViewOptions options;
    YuiToolModels models;

    for (int i = 2; i < argc; ++i)
    {
        if (const auto* path = readYuiToolOption (argc, argv, i, "--output"))
        {
            output = File::getCurrentWorkingDirectory().getChildFile (path);
        }
        else if (const auto* factor = readYuiToolOption (argc, argv, i, "--scale"))
        {
            scale = String (factor).getFloatValue();

            if (scale <= 0.0f || scale > 8.0f)
            {
                std::cerr << "invalid scale, expected a factor up to 8: " << factor << "\n";
                return 2;
            }
        }
        else if (const auto* pixels = readYuiToolOption (argc, argv, i, "--margin"))
        {
            const String value (pixels);

            if (! isYuiToolNumber (value))
            {
                std::cerr << "invalid margin, expected logical pixels around the element: " << value << "\n";
                return 2;
            }

            margin = value.getFloatValue();
        }
        else if (const auto* names = readYuiToolOption (argc, argv, i, "--guides"))
        {
            guided.addTokens (names, ",", "");
        }
        else if (const auto* area = readYuiToolOption (argc, argv, i, "--region"))
        {
            region = parseRegion (area);

            if (! region.has_value())
            {
                std::cerr << "invalid region, expected <x>,<y>,<width>,<height>: " << area << "\n";
                return 2;
            }
        }
        else if (const auto read = readYuiToolViewOption (argc, argv, i, options, models); read.failed())
        {
            return refuseYuiToolCommand (read);
        }
    }

    const auto& target = options.target;

    if (options.input == File() || output == File() || (target.drawing.isNotEmpty() && target.palette.isNotEmpty()) || (target.hostType.isNotEmpty() && target.palette.isNotEmpty()))
    {
        std::cerr << "Rendering requires an input file and --output <image.png>, with --drawing or --palette but not both\n";
        return 2;
    }

    if (options.element.isNotEmpty() && region.has_value())
    {
        std::cerr << "--element and --region both say what to draw: give one of them\n";
        return 2;
    }

    if (! options.input.existsAsFile())
    {
        std::cerr << "can't read '" << options.input.getFullPathName() << "'\n";
        return 2;
    }

    // Without models given, those next to the file are used
    if (const auto beside = findYuiToolModelsFile (options.input); ! options.hasModels && beside.existsAsFile())
        if (const auto loaded = models.load (beside.getFullPathName()); loaded.failed())
            return refuseYuiToolCommand (loaded);

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

    const auto problems = view.findProblems();

    // What is wrong with the document is told whether loading it or drawing it found it, its warnings included
    for (const auto& problem : problems)
        problem.print();

    if (shown.needsHost)
        std::cerr << "hint: a drawing reading the properties of a widget needs it as its host, for example --as Slider\n";

    if (! view.hasInterface())
        return 1;

    // The interface was styled before it had its size
    view.settle();

    // All of the interface, or the part asked for, or the one a drawing or a type shown in context is framed on
    const auto whole = view.getComponent()->getLocalBounds();
    auto part = view.getFrame();

    if (options.element.isNotEmpty())
    {
        const auto* component = findYuiToolElement (view.getDocument(), options.element);

        if (component == nullptr)
        {
            std::cerr << options.input.getFullPathName() << ": error: nothing to render, there is no component with the id or at the path '" << options.element << "'\n";
            return 1;
        }

        part = clipYuiToolRegion (component->getLocalBounds().translated (findYuiToolOrigin (*component, *view.getRoot())).enlarged (margin), whole);
    }
    else if (region.has_value())
    {
        part = clipYuiToolRegion (*region, whole);
    }

    if (part.getWidth() < 1.0f || part.getHeight() < 1.0f)
    {
        std::cerr << options.input.getFullPathName() << ": error: nothing to render, there is nothing of the interface there: it is "
                  << roundToInt (whole.getWidth()) << "x" << roundToInt (whole.getHeight()) << " logical pixels\n";
        return 1;
    }

    // The components the guides are drawn over
    std::vector<Component*> marked;

    for (const auto& named : guided)
    {
        auto* component = findYuiToolElement (view.getDocument(), named.trim());

        if (component == nullptr)
        {
            std::cerr << options.input.getFullPathName() << ": error: nothing to render, there is no component with the id or at the path '" << named.trim() << "'\n";
            return 1;
        }

        marked.push_back (component);
    }

    const auto context = createYuiToolGraphicsContext();
    auto drawn = ResultValue<Image>::ok (Image());

    // A scene only shows its final picture after some frames, which are drawn until it does
    const auto draw = [&context, scale] (Component& component)
    {
        return drawYuiToolImage (component, [&]
        {
            return component.snapshotToImage (*context, true, scale);
        });
    };

    if (context != nullptr && part == whole && marked.empty())
    {
        drawn = draw (*view.getComponent());
    }
    else if (context != nullptr)
    {
        // Moving the interface into the frame may restyle it
        YuiToolFrame frame (*view.getComponent(), part);
        YuiToolGuides guides;

        if (! marked.empty())
        {
            guides.setDocument (&view.getDocument());
            guides.setView (part.getPosition(), 1.0f);
            guides.setPicked (marked);

            frame.addAndMakeVisible (guides);
            guides.setBounds (frame.getLocalBounds());
        }

        view.settle();

        drawn = draw (frame);
    }

    // What drawing found: what a scene left to its first frame, and one still changing at its last
    const auto found = view.findProblems();

    for (auto index = jmin (problems.size(), found.size()); index < found.size(); ++index)
        found[index].print();

    if (drawn.failed())
    {
        std::cerr << options.input.getFullPathName() << ": error: " << drawn.getErrorMessage() << "\n";
        return 1;
    }

    const auto image = drawn.getValue();

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
