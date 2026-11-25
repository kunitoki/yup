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

#include "Figures.h"

#include <yup_gui_markup/yup_gui_markup.h>
#include <yup_renderer/yup_renderer.h>

#include <algorithm>
#include <iostream>

namespace
{

yup::String figureFailure;

/** The most a figure is drawn for the scenes it shows to settle: four times the frames the renderer accumulates. */
constexpr int maximumFrames = 64;

//==============================================================================
/** The scenes a component shows, in the order of its tree. */
void collectScenes (yup::Component& component, std::vector<yup::SceneComponent*>& scenes)
{
    if (auto* scene = dynamic_cast<yup::SceneComponent*> (&component))
        scenes.push_back (scene);

    for (int index = 0; index < component.getNumChildComponents(); ++index)
        collectScenes (*component.getChildComponent (index), scenes);
}

//==============================================================================

struct PaintedFigure : yup::Component
{
    explicit PaintedFigure (Figures::PaintFunction functionToUse)
        : paintFunction (std::move (functionToUse))
    {
        setOpaque (false);
    }

    void paint (yup::Graphics& g) override
    {
        paintFunction (g, getLocalBounds());
    }

    Figures::PaintFunction paintFunction;
};

//==============================================================================
/** Fills the background of a figure and keeps its component inside the padding. */
struct Frame : yup::Component
{
    Frame (const Figure& figure, std::unique_ptr<yup::Component> componentToShow)
        : content (std::move (componentToShow))
    {
        setSize (figure.size);
        addAndMakeVisible (*content);
        content->setBounds (getLocalBounds().reduced (figure.padding));
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (role (Role::background));
        g.fillAll();
    }

    std::unique_ptr<yup::Component> content;
};

//==============================================================================
/** Adds the `.yui` files of the figures folder as figures named by their path.

    A file starting with an underscore is only there to be imported, and a first line
    such as `// size: 480 160` gives a figure another size than the default one.
*/
void addYuiFigures (Figures& f)
{
    const auto folder = docsimgFile ("figures");

    for (const auto& file : folder.findChildFiles (yup::File::findFiles, true, "*.yui", yup::File::FollowSymlinks::no))
    {
        if (file.getFileName().startsWithChar ('_'))
            continue;

        const auto firstLine = file.loadFileAsString().upToFirstOccurrenceOf ("\n", false, false);
        const auto values = yup::StringArray::fromTokens (firstLine.fromFirstOccurrenceOf ("size:", false, false).trim(), false);

        const auto size = values.size() == 2
                            ? yup::Size<float> (values[0].getFloatValue(), values[1].getFloatValue())
                            : Figures::defaultSize;

        const auto id = file.getRelativePathFrom (folder).upToLastOccurrenceOf (".", false, false).replaceCharacter ('\\', '/');

        f.component (id, size, [file]() -> std::unique_ptr<yup::Component>
        {
            auto view = std::make_unique<yup::YuiView>();
            const auto result = view->getLoader().loadFile (file);
            const auto& diagnostics = view->getDocument().getDiagnostics();

            if (result.failed())
                std::cerr << result.getErrorMessage() << std::endl;

            for (const auto& diagnostic : diagnostics)
                std::cerr << diagnostic.toString() << std::endl;

            if (result.failed() || ! diagnostics.empty())
                return nullptr;

            return view;
        });
    }
}

} // namespace

//==============================================================================

Figure& Figures::paint (const yup::String& id, yup::Size<float> size, PaintFunction paintFunction)
{
    return component (id, size, [paintFunction = std::move (paintFunction)]
    {
        return std::make_unique<PaintedFigure> (paintFunction);
    });
}

Figure& Figures::component (const yup::String& id, yup::Size<float> size, Figure::Factory create)
{
    return figures.emplace_back (Figure { id, size, std::move (create) });
}

//==============================================================================

void failFigure (const yup::String& reason)
{
    figureFailure = reason;
}

void labelled (yup::Graphics& g, yup::Rectangle<float> area, std::initializer_list<const char*> names, int columns, const std::function<void (int, yup::Rectangle<float>)>& draw)
{
    const auto rows = (static_cast<int> (names.size()) + columns - 1) / columns;
    const auto width = area.getWidth() / static_cast<float> (columns);
    const auto height = area.getHeight() / static_cast<float> (rows);

    int index = 0;
    for (const auto* name : names)
    {
        const auto column = static_cast<float> (index % columns);
        const auto row = static_cast<float> (index / columns);
        auto cell = yup::Rectangle<float> (area.getX() + column * width, area.getY() + row * height, width, height).reduced (0.0f, 2.0f);

        g.setFillColor (role (Role::textMuted));
        g.fillFittedText (name, font (12.0f), cell.removeFromBottom (18.0f), yup::Justification::center);

        const auto saved = g.saveState();
        draw (index++, cell.reduced (6.0f, 0.0f));
    }
}

//==============================================================================
/** Renders the figures of the documentation and quits.

    Every figure matching `--figure` (a wildcard on its id, all of them by default) is drawn
    offscreen in the dark and in the light theme, at twice its size, and written as
    `<id>_dark.jpg` and `<id>_light.jpg` under the folder given by `--out`.
*/
class Application
    : public yup::YUPApplication
    , private yup::Timer
{
public:
    yup::String getApplicationName() override { return "docsimg"; }

    yup::String getApplicationVersion() override { return "1.0"; }

    void initialise (const yup::String&) override
    {
        const yup::ArgumentList args (getApplicationName(), getCommandLineParameterArray());
        const auto filter = args.getValueForOption ("--figure").value_or ("*");

        outputFolder = yup::File::getCurrentWorkingDirectory().getChildFile (args.getValueForOption ("--out").value_or ("docs/_static/images"));

        addGraphicsFigures (figures);
        addImagingFigures (figures);
        addLayoutFigures (figures);
        addWidgetFigures (figures);
        addDspFigures (figures);
        addAudioFigures (figures);
        addRhiFigures (figures);
        addScene3DFigures (figures);
        addYuiFigures (figures);

        for (const auto& figure : figures.getAll())
        {
            if (! figure.id.matchesWildcard (filter, true))
                continue;

            jobs.push_back ({ &figure, Mode::dark, figure.themed ? "_dark" : "" });

            if (figure.themed)
                jobs.push_back ({ &figure, Mode::light, "_light" });
        }

        context = yup::GraphicsContext::createContext (yup::GpuPlatform::Metal, {});

        if (context == nullptr)
            return finish ("No GPU context to draw with");

        if (jobs.empty())
            return finish ("No figure matches " + filter);

        startTimer (1);
    }

    void shutdown() override
    {
        frame.reset();
        context.reset();
    }

private:
    using Mode = yup::ThemePalette::Mode;

    struct Job
    {
        const Figure* figure;
        Mode mode;
        const char* suffix;
    };

    void timerCallback() override
    {
        if (frame == nullptr)
        {
            if (nextJob == jobs.size())
                return finish (failures == 0 ? yup::String() : yup::String (failures) + " figures failed");

            begin (jobs[nextJob]);
        }
        else if (yup::Time::getMillisecondCounter() >= captureTime)
        {
            capture (jobs[nextJob]);
            frame.reset();
            ++nextJob;
        }
    }

    void begin (const Job& job)
    {
        yup::ApplicationTheme::getGlobalTheme()->setPalette (yup::ThemePalette ({}, job.mode));

        if (auto content = job.figure->create())
        {
            frame = std::make_unique<Frame> (*job.figure, std::move (content));
            captureTime = yup::Time::getMillisecondCounter() + static_cast<yup::uint32> (job.figure->settleMs);
            return;
        }

        fail (job, "cannot be created");
        ++nextJob;
    }

    void capture (const Job& job)
    {
        std::vector<yup::SceneComponent*> scenes;
        collectScenes (*frame, scenes);

        const auto isSettled = [&scenes]
        {
            return std::all_of (scenes.begin(), scenes.end(), [] (yup::SceneComponent* scene)
            {
                return scene->getRenderer().isConverged();
            });
        };

        // A scene refines its picture over frames, so it is drawn until it has settled. Each time it runs the frame a
        // window runs before painting, with no time elapsed: what is animated stands where it starts, in every run
        yup::Image image;
        for (int i = 0; i < job.figure->frames || (i < maximumFrames && ! isSettled()); ++i)
        {
            for (auto* scene : scenes)
                scene->refreshDisplay (0.0);

            image = frame->snapshotToImage (*context, true, 2.0f);
        }

        if (! image.isValid())
            return fail (job, "cannot be drawn");

        if (figureFailure.isNotEmpty())
            return fail (job, std::exchange (figureFailure, {}));

        // A scene that could not be rendered shows why in its place, which is not the figure
        for (auto* scene : scenes)
            if (const auto& error = scene->getRenderer().getLastError(); error.isNotEmpty())
                return fail (job, "shows a scene that cannot be rendered: " + error);

        const auto file = outputFolder.getChildFile (job.figure->id + job.suffix + ".jpg");
        file.getParentDirectory().createDirectory();
        file.deleteFile();

        auto stream = std::make_unique<yup::FileOutputStream> (file);
        if (stream->failedToOpen())
            return fail (job, "cannot be written to " + file.getFullPathName());

        yup::JpegImageFormatWriter writer (stream.release(), image.getPixelFormat(), 0);
        if (! writer.writeImage (image))
            return fail (job, "cannot be encoded");

        std::cout << file.getFullPathName() << " " << image.getWidth() << "x" << image.getHeight() << std::endl;
    }

    void fail (const Job& job, const yup::String& reason)
    {
        std::cerr << job.figure->id << job.suffix << " " << reason << std::endl;
        ++failures;
    }

    void finish (const yup::String& error)
    {
        stopTimer();

        if (error.isNotEmpty())
        {
            std::cerr << error << std::endl;
            setApplicationReturnValue (1);
        }

        quit();
    }

    Figures figures;
    std::vector<Job> jobs;
    std::size_t nextJob = 0;
    int failures = 0;

    yup::File outputFolder;
    std::unique_ptr<yup::GraphicsContext> context;
    std::unique_ptr<Frame> frame;
    yup::uint32 captureTime = 0;
};

START_YUP_APPLICATION (Application)
