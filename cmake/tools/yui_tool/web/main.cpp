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

#include "../source/yup_YuiAgentTools.h"
#include "../source/yup_YuiCommands.h"
#include "../source/yup_YuiInspectCommand.h"
#include "../source/yup_YuiToolCheck.h"
#include "../source/yup_YuiToolGuides.h"
#include "../source/yup_YuiToolModels.h"
#include "../source/yup_YuiToolView.h"

#include <yup_audio_gui/yup_audio_gui.h>
#include <yup_gui_markup/yup_gui_markup.h>
#include <yup_renderer/yup_renderer.h>

#include <SDL3/SDL_hints.h>

#include <emscripten/emscripten.h>
#include <emscripten/eventloop.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace yup;

// Of the windowing of YUP for the web: draws the window now instead of at its next animation frame
extern "C" void yupRenderPrimaryWindow();

namespace
{

//==============================================================================
// The files the host gives, at the paths it names them with
File getFilesFolder()
{
    return File ("/yui");
}

// Tells the renderer where the module holds the template a material is compiled around and the library it includes:
// the folder they are embedded in, see CMakeLists.txt, in place of the one of the sources the module was built from
void useEmbeddedMaterialShaders()
{
#if YUP_ENABLE_SHADER_TRANSPILER
    MaterialShader::setSourceDirectory (File (YUP_YUI_TOOL_MATERIAL_SHADERS));
#endif
}

// Calls a function with a component, then with each of those it holds
template <class Function>
void forEachComponent (Component& component, Function&& call)
{
    call (component);

    for (int index = 0; index < component.getNumChildComponents(); ++index)
        forEachComponent (*component.getChildComponent (index), call);
}

// The files a document reads beside its texts, as it names them: those of its shaders, each after what a build compiles
// it into, which is read in its place, then the models and the images of the scenes its interface shows
StringArray findNamedFiles (YuiDocument& document)
{
    StringArray names;

    for (const auto& element : document.getTree())
    {
        if (element.getType().toString() != "shader")
            continue;

        if (const auto fragment = element.getProperty ("fragment").toString(); fragment.isNotEmpty())
        {
            if (! fragment.endsWithIgnoreCase (".ysl"))
                names.add (fragment + ".ysl");

            names.add (fragment);
        }

        if (const auto material = element.getProperty ("material").toString(); material.isNotEmpty())
        {
            names.add (material + ".raster.ysl");
            names.add (material + ".full.ysl");
            names.add (material);
        }
    }

    if (auto* root = document.getRoot())
    {
        forEachComponent (*root, [&names] (Component& component)
        {
            if (const auto* model = dynamic_cast<const SceneModelComponent*> (&component))
                names.add (model->getSource());

            if (dynamic_cast<const SceneMaterialComponent*> (&component) == nullptr)
                return;

            for (const auto* property : component.getTypeInfo().getAllProperties())
                if (property->name.toString().endsWith ("Texture"))
                    names.add (component.getProperty (property->name).toString());
        });
    }

    return names;
}

//==============================================================================
// How many of the last frames painted the statistics tell about
constexpr int statsFrames = 120;

/** What a component took to paint itself in the frames the statistics tell about. */
struct PaintCost
{
    String name;
    double micros = 0.0;
};

// Adds the components of an interface that painted themselves since a frame, with the time it took them
void collectPaintCosts (const Component& component, uint64 sinceFrame, std::vector<PaintCost>& costs)
{
    double micros = 0.0;

    if (const auto* stats = PaintProfiler::getInstance().getStatsForComponent (component))
        for (const auto& sample : stats->copySamples())
            if (sample.frameIndex >= sinceFrame)
                micros += sample.selfMicros;

    if (micros > 0.0)
        costs.push_back ({ describeYuiToolName (component), micros });

    for (int index = 0; index < component.getNumChildComponents(); ++index)
        collectPaintCosts (*component.getChildComponent (index), sinceFrame, costs);
}

String describePlatform (GpuPlatform platform)
{
    switch (platform)
    {
        case GpuPlatform::OpenGLES:
            return "WebGL 2";

        case GpuPlatform::WebGPU:
            return "WebGPU";

        default:
            return "no GPU";
    }
}

// How the renderer draws paths, which depends on what the GPU of the page can do
String describePathRendering (rive::gpu::InterlockMode mode)
{
    switch (mode)
    {
        case rive::gpu::InterlockMode::rasterOrdering:
            return "raster ordering";

        case rive::gpu::InterlockMode::atomics:
            return "atomics";

        case rive::gpu::InterlockMode::clockwise:
            return "clockwise";

        case rive::gpu::InterlockMode::clockwiseAtomic:
            return "clockwise atomics";

        case rive::gpu::InterlockMode::depthStencil:
            return "depth and stencil";
    }

    return "another way";
}

//==============================================================================
/** What the audio displays of an interface show where no application gives them a state: a signal made here.

    A document can't write the state of a `SpectrumAnalyzer`, a `Spectrogram` or a `KMeter`: an application gives
    it, and a tool showing the document has none to give, so they would stay still. While this lives, every such
    display of the tree it looks through that has no state is given one, fed with a chord swelling under a partial
    sweeping the spectrum: what a drawing or a shader makes of their levels is seen moving. A keyboard needs
    nothing, it shows the keys held on it.

    The displays are looked for again at every tick, so those a document makes when it is loaded again are fed too,
    and the ones that went are let go. A display still there when this goes is left without a state.
*/
class YuiToolSignals final : private yup::Timer
{
public:
    YuiToolSignals();
    ~YuiToolSignals() override;

    /** Sets the component whose tree is looked through for displays, or nullptr for none. */
    void setRoot (yup::Component* newRoot);

private:
    void timerCallback() override;
    void feed (yup::Component& component);
    void makeSignal();

    static constexpr double sampleRate = 48000.0;
    static constexpr int ticksPerSecond = 30;

    /** A display fed by a state of its own: two reading one state would take each other's samples. */
    struct Fed
    {
        yup::WeakReference<yup::Component> display;
        std::unique_ptr<yup::SpectrumAnalyzerState> state;
    };

    yup::WeakReference<yup::Component> root;
    std::vector<Fed> spectrums;
    std::vector<yup::WeakReference<yup::Component>> meters;
    yup::KMeterState meterState { sampleRate, 2 };
    double lowPhase = 0.0, midPhase = 0.0, highPhase = 0.0, sweepPhase = 0.0, swellPhase = 0.0;
};

YuiToolSignals::YuiToolSignals()
{
    startTimerHz (ticksPerSecond);
}

YuiToolSignals::~YuiToolSignals()
{
    stopTimer();

    // The states go with this: no display is left reading one
    for (auto& fed : spectrums)
    {
        if (auto* analyzer = dynamic_cast<SpectrumAnalyzerComponent*> (fed.display.get()))
            analyzer->setState (nullptr);
        else if (auto* spectrogram = dynamic_cast<SpectrogramComponent*> (fed.display.get()))
            spectrogram->setState (nullptr);
    }

    for (auto& display : meters)
        if (auto* meter = dynamic_cast<KMeterComponent*> (display.get()))
            meter->setState (nullptr);
}

void YuiToolSignals::setRoot (Component* newRoot)
{
    root = newRoot;
}

void YuiToolSignals::timerCallback()
{
    // The displays that went with their interface are let go, and those a new one brought are given a state
    std::erase_if (spectrums, [] (const Fed& fed)
    {
        return fed.display.get() == nullptr;
    });

    std::erase_if (meters, [] (const WeakReference<Component>& display)
    {
        return display.get() == nullptr;
    });

    if (auto* component = root.get())
        feed (*component);

    if (! spectrums.empty() || ! meters.empty())
        makeSignal();
}

void YuiToolSignals::feed (Component& component)
{
    if (auto* analyzer = dynamic_cast<SpectrumAnalyzerComponent*> (&component))
    {
        if (analyzer->getState() == nullptr)
        {
            auto& fed = spectrums.emplace_back (Fed { &component, std::make_unique<SpectrumAnalyzerState> (2048, 2048) });
            analyzer->setState (fed.state.get());
            analyzer->setSampleRate (sampleRate);
        }
    }
    else if (auto* spectrogram = dynamic_cast<SpectrogramComponent*> (&component))
    {
        if (spectrogram->getState() == nullptr)
        {
            auto& fed = spectrums.emplace_back (Fed { &component, std::make_unique<SpectrumAnalyzerState> (2048, 2048) });
            spectrogram->setState (fed.state.get());
            spectrogram->setSampleRate (sampleRate);
        }
    }
    else if (auto* meter = dynamic_cast<KMeterComponent*> (&component))
    {
        if (meter->getState() == nullptr)
        {
            meters.emplace_back (&component);
            meter->setState (&meterState);
        }
    }

    for (int index = 0; index < component.getNumChildComponents(); ++index)
        if (auto* child = component.getChildComponent (index))
            feed (*child);
}

void YuiToolSignals::makeSignal()
{
    static constexpr int numSamples = static_cast<int> (sampleRate) / ticksPerSecond;

    std::array<float, numSamples> left {};
    std::array<float, numSamples> right {};

    const auto advance = [] (double& phase, double frequency)
    {
        phase += MathConstants<double>::twoPi * frequency / sampleRate;

        if (phase > MathConstants<double>::twoPi)
            phase -= MathConstants<double>::twoPi;

        return std::sin (phase);
    };

    for (int index = 0; index < numSamples; ++index)
    {
        const auto swell = 0.5 + 0.5 * advance (swellPhase, 0.35);
        const auto sweep = 2400.0 + 2000.0 * advance (sweepPhase, 0.11);

        const auto sample = swell * (0.45 * advance (lowPhase, 220.0) + 0.3 * advance (midPhase, 880.0))
                          + 0.2 * advance (highPhase, sweep);

        left[static_cast<std::size_t> (index)] = static_cast<float> (sample);
        right[static_cast<std::size_t> (index)] = static_cast<float> (sample * 0.5);
    }

    for (auto& fed : spectrums)
        fed.state->pushSamples (left.data(), numSamples);

    const float* channels[2] = { left.data(), right.data() };
    meterState.pushSamples (channels, 2, numSamples);
    meterState.processPendingAudio();
}

//==============================================================================
/** The window the page shows in its canvas: an interface over the background of its palette. */
class PreviewWindow final : public DocumentWindow
{
public:
    // The page gives the window its size and the canvas the size it is seen at, which a window the browser
    // may resize would take back from it
    PreviewWindow()
        : DocumentWindow (ComponentNative::Options().withResizableWindow (false))
    {
        addChildComponent (guides);
    }

    /** Shows a target of a file and tells the page what the load gave.

        The document is reloaded in place, keeping the state of its components, for as long as the
        file, the target and the models stay the same. The theme is switched under it.

        @param chosenUse The path of the component a drawing or a type shown in context is framed on,
                         the first one when empty or when it is no longer one of them.
    */
    void show (const File& input, const YuiToolTarget& target, const String& chosenUse, const File& modelsFile, bool lightTheme)
    {
        const auto modelsText = modelsFile.existsAsFile() ? modelsFile.loadFileAsString() : String();
        const auto key = StringArray { input.getFullPathName(), target.drawing, target.palette, target.hostType, modelsText }
                             .joinIntoString ("\n");

        if (shownLight != lightTheme)
        {
            shownLight = lightTheme;
            ApplicationTheme::getGlobalTheme()->setPalette (ThemePalette ({}, lightTheme ? ThemePalette::Mode::light : ThemePalette::Mode::dark));
        }

        if (session == nullptr || key != sessionKey)
        {
            guides.setDocument (nullptr);
            session.reset();
            sessionKey = key;

            session = std::make_unique<Session>();
            session->signals.setRoot (&session->view);

            if (modelsFile.existsAsFile())
            {
                if (const auto loaded = session->models.load (modelsFile.getFullPathName()); loaded.failed())
                    std::cerr << loaded.getErrorMessage() << "\n";
                else
                    session->models.attachTo (session->view.getDocument());
            }

            session->models.onValueChanged = [] (const String& model, const String& property, const var& value)
            {
                // clang-format off
                EM_ASM ({
                    if (Module.yuiPreviewModelChanged)
                        Module.yuiPreviewModelChanged (UTF8ToString ($0), UTF8ToString ($1), JSON.parse (UTF8ToString ($2)));
                }, model.toRawUTF8(), property.toRawUTF8(), JSON::toString (value, true).toRawUTF8());
                // clang-format on
            };

            addAndMakeVisible (session->view);
            guides.toFront (false);
            resized();
        }

        auto& document = session->view.getDocument();
        const auto shown = loadYuiToolTarget (document, session->view.getLoader(), input, target);

        shownInput = input;
        shownTarget = shown.target;

        const auto uses = shown.isInContext ? findYuiToolUses (document, shown.target) : std::vector<Component*>();
        auto* chosen = chosenUse.isNotEmpty() ? findYuiToolComponent (document, chosenUse) : nullptr;

        use = std::find (uses.begin(), uses.end(), chosen) != uses.end() ? chosen : (uses.empty() ? nullptr : uses.front());

        Array<var> usesTold;

        for (const auto* component : uses)
            usesTold.add (describeYuiToolComponent (document, *component));

        // The uses were looked for in the interface at the size it sets: it gets back the one it is shown at
        session->view.resized();

        if (showsStats)
            measurePainting();

        guides.setDocument (&document, input);

        Array<var> problems;

        for (const auto& problem : findYuiToolProblems (document, input, shown.result))
        {
            if (! problem.isError)
                continue;

            // The host knows the files by the paths it gave them
            auto record = problem.toJson();
            record.getDynamicObject()->setProperty ("file", problem.file.getRelativePathFrom (getFilesFolder()));
            problems.add (record);
        }

        const auto declaredSize = shown.status == YuiToolShown::Status::loaded ? findYuiToolDeclaredSize (document.getTree()) : Size<float>();

        auto* report = new DynamicObject();
        report->setProperty ("status", shown.status != YuiToolShown::Status::loaded ? "nothing" : (shown.result.failed() ? "failed" : "shown"));
        report->setProperty ("message", shown.status == YuiToolShown::Status::invalidName ? String ("the name of what to show is not valid") : shown.message);
        report->setProperty ("problems", var (problems));
        report->setProperty ("hasInterface", document.getRoot() != nullptr);
        report->setProperty ("width", declaredSize.getWidth());
        report->setProperty ("height", declaredSize.getHeight());
        report->setProperty ("defaultWidth", shown.defaultSize.getWidth());
        report->setProperty ("defaultHeight", shown.defaultSize.getHeight());
        report->setProperty ("inContext", shown.isInContext);
        report->setProperty ("uses", var (usesTold));
        report->setProperty ("use", use != nullptr ? describeYuiToolComponent (document, *use.get())["path"] : var());
        report->setProperty ("needsHost", shown.needsHost);
        report->setProperty ("models", session->models.toJson());

        repaint();

        // clang-format off
        EM_ASM ({
            if (Module.yuiPreviewShown)
                Module.yuiPreviewShown (JSON.parse (UTF8ToString ($0)));
        }, JSON::toString (var (report)).toRawUTF8());
        // clang-format on
    }

    void paint (Graphics& g) override
    {
        const auto* root = session != nullptr ? session->view.getDocument().getRoot() : nullptr;
        const auto& theme = *ApplicationTheme::getGlobalTheme();

        g.setFillColor ((root != nullptr ? theme.getPalette (*root) : theme.getPalette (*this)).getColor (ThemePalette::Role::background));
        g.fillAll();
    }

    void resized() override
    {
        guides.setBounds (getLocalBounds());

        if (session == nullptr)
            return;

        // Until the page says the size of the interface, it is the one of the window
        if (interfaceSize.getWidth() < 1.0f)
            session->view.setBounds (getLocalBounds());
        else
            session->view.setBounds (Rectangle<float> (-frame.getX(), -frame.getY(), interfaceSize.getWidth(), interfaceSize.getHeight()));
    }

    /** Lays the interface out at a size and gives the window the size of what is shown of it: the
        component a drawing or a type shown in context is framed on, else all of it.

        The page is told that part through Module.yuiPreviewFramed before the window takes its size,
        which is when the density of the display is read again: the page changes it there to zoom,
        and answers how many times larger the interface is then seen. Once the window drew,
        Module.yuiPreviewDrawn is told why a scene of the interface could not be drawn, or nothing.
    */
    void setView (Size<int> size)
    {
        interfaceSize = size.to<float>();
        frame = Rectangle<float> (0.0f, 0.0f, interfaceSize.getWidth(), interfaceSize.getHeight());

        if (session != nullptr)
        {
            session->view.setSize (interfaceSize);

            const auto* root = session->view.getDocument().getRoot();
            const auto area = use != nullptr && root != nullptr ? findYuiToolPaintArea (*use.get(), *root) : frame;

            // A use that lost its size since it was found leaves all of the interface to show
            if (area.getWidth() >= 1.0f && area.getHeight() >= 1.0f)
                frame = area;
        }

        // clang-format off
        const auto zoom = EM_ASM_DOUBLE ({
            return (Module.yuiPreviewFramed && Module.yuiPreviewFramed ($0, $1, $2, $3)) || 1;
        }, frame.getX(), frame.getY(), frame.getWidth(), frame.getHeight());
        // clang-format on

        guides.setView (frame.getPosition(), static_cast<float> (zoom));

        const auto isSameSize = getWidth() == frame.getWidth() && getHeight() == frame.getHeight();

        setSize (frame.getSize());
        resized();

        // A window keeping its size is given the pixels of its canvas again, as their density may have changed
        if (auto* native = isSameSize ? getNativeComponent() : nullptr)
            native->setBounds (frame.withZeroPosition().to<int>());

        repaint();

        // The browser cleared the canvas if it got other pixels, and would show it empty until the next frame
        yupRenderPrimaryWindow();

        // A scene that could not be drawn shows why in its place: the page lists it with the problems of the document
        // clang-format off
        EM_ASM ({
            if (Module.yuiPreviewDrawn)
                Module.yuiPreviewDrawn (UTF8ToString ($0));
        }, findSceneProblem().toRawUTF8());
        // clang-format on
    }

    /** Shows or hides the guides over the interface. */
    void setGuidesShown (bool shouldShow)
    {
        guides.setVisible (shouldShow);
    }

    /** Tells the guides the place of the file shown the caret of the editor is at. */
    void setCaret (YuiToolPlace caret)
    {
        guides.setCaret (caret);
    }

    /** Starts or stops measuring how long the components take to paint, which describeStats() tells.

        Measuring costs a little for each component painted, so nothing is measured until this is on.
    */
    void setStatsShown (bool shouldShow)
    {
        showsStats = shouldShow;

        if (shouldShow)
            measurePainting();
        else
            PaintProfiler::getInstance().disableSubtree (*this);
    }

    /** Tells how the interface is drawn, in three lines: how often and for how long, at what size and with
        what, and the components that take the longest to paint themselves.

        The times are those of the last frames painted while setStatsShown() is on, and tell what painting
        asks of the processor: what the GPU then does with it is not in them.
    */
    String describeStats()
    {
        auto* native = getNativeComponent();

        if (native == nullptr)
            return {};

        // The window holds all that is painted, so the time it took is the one of the frame
        const auto* painted = PaintProfiler::getInstance().getStatsForComponent (*this);
        const auto samples = painted != nullptr ? painted->copySamples() : std::vector<PaintProfileSample>();
        const auto numFrames = static_cast<double> (samples.size());
        double total = 0.0;
        double longest = 0.0;

        for (const auto& sample : samples)
        {
            total += sample.totalMicros;
            longest = jmax (longest, sample.totalMicros);
        }

        String told;
        told << roundToInt (native->getCurrentFrameRate()) << " fps of " << roundToInt (native->getDesiredFrameRate());

        if (samples.empty())
            told << " | nothing painted yet";
        else
            told << " | paint " << String (total / (1000.0 * numFrames), 2) << " ms, " << String (longest / 1000.0, 2) << " at most";

        // What the last frame had to paint again of the window
        const auto bounds = getLocalBounds();
        const auto& areas = native->getFrameRepaintAreas();
        float repainted = 0.0f;

        for (const auto& area : areas.getRectangles())
            repainted += area.intersection (bounds).area();

        if (bounds.area() > 0.0f)
            told << " | repainted " << roundToInt (jmin (100.0f, 100.0f * repainted / bounds.area())) << "% in "
                 << areas.getNumRectangles() << (areas.getNumRectangles() == 1 ? " area" : " areas");

        const auto pixels = native->getContentSize();
        told << "\n" << pixels.getWidth() << " x " << pixels.getHeight() << " px at " << String (native->getScaleDpi(), 2) << "x";

        if (auto* context = native->getGraphicsContext())
        {
            told << " | " << describePlatform (context->getPlatform());

            if (const auto* renderer = context->getRenderContext())
                told << " | paths by " << describePathRendering (renderer->frameInterlockMode());
        }

        // The components of the interface by what painting them asks of a frame, the tool's own left out
        std::vector<PaintCost> costs;

        if (const auto* root = session != nullptr ? session->view.getDocument().getRoot() : nullptr; root != nullptr && ! samples.empty())
            collectPaintCosts (*root, samples.front().frameIndex, costs);

        std::sort (costs.begin(), costs.end(), [] (const PaintCost& first, const PaintCost& second)
        {
            return first.micros > second.micros;
        });

        StringArray slowest;

        for (size_t index = 0; index < jmin (costs.size(), size_t (5)); ++index)
            slowest.add (costs[index].name + " " + String (costs[index].micros / (1000.0 * numFrames), 2) + " ms");

        told << "\nslowest: " << (slowest.isEmpty() ? String ("-") : slowest.joinIntoString (" | "));

        return told;
    }

    // The page owns the window
    void userTriedToCloseWindow() override {}

    /** Changes a value of the models of the interface shown. */
    bool setModelValue (const String& model, const String& property, const var& value)
    {
        return session != nullptr && session->models.setValue (model, property, value);
    }

    /** Returns the current values of the models of the interface shown. */
    var getModels() const
    {
        return session != nullptr ? session->models.toJson() : var (new DynamicObject());
    }

    /** Captures the interface as it is shown now, for an agent: its image, with what it is an image of. */
    YuiAgentResult capture (const var& arguments)
    {
        if (session == nullptr || session->view.getDocument().getRoot() == nullptr)
            return YuiAgentResult::fail ("the preview shows no interface: open a .yui document and run 'YUI: Open Preview'");

        double scale = 1.0;

        if (const auto read = readYuiAgentScale (arguments, scale); read.failed())
            return YuiAgentResult::fail (read.getErrorMessage());

        if (jmax (getWidth(), getHeight()) * scale > maximumYuiAgentImageSide)
            return YuiAgentResult::fail ("the image would be more than " + String (maximumYuiAgentImageSide) + " pixels a side: lower 'scale'");

        auto* native = getNativeComponent();
        auto* context = native != nullptr ? native->getGraphicsContext() : nullptr;
        const auto image = context != nullptr ? snapshotToImage (*context, true, static_cast<float> (scale)) : Image();

        YuiAgentResult result;

        if (! encodeYuiToolPng (image, result.image))
            return YuiAgentResult::fail ("the preview has no graphics device to capture with");

        auto* captured = new DynamicObject();
        captured->setProperty ("file", shownInput.getFullPathName());

        if (shownTarget.drawing.isNotEmpty())
            captured->setProperty ("drawing", shownTarget.drawing);

        if (shownTarget.palette.isNotEmpty())
            captured->setProperty ("palette", shownTarget.palette);

        if (shownTarget.hostType.isNotEmpty())
            captured->setProperty ("as", shownTarget.hostType);

        if (use != nullptr)
            captured->setProperty ("use", describeYuiToolComponent (session->view.getDocument(), *use.get()));

        captured->setProperty ("width", image.getWidth());
        captured->setProperty ("height", image.getHeight());
        captured->setProperty ("scale", scale);
        captured->setProperty ("theme", shownLight.value_or (false) ? "light" : "dark");
        captured->setProperty ("models", summarizeYuiAgentModels (session->models.toJson()));

        result.text = JSON::toString (var (captured));
        return result;
    }

private:
    // Why the frame drawn last could not draw a scene of the interface, nothing when it drew them all
    String findSceneProblem()
    {
        auto* root = session != nullptr ? session->view.getDocument().getRoot() : nullptr;
        auto* native = getNativeComponent();
        auto* context = native != nullptr ? native->getGraphicsContext() : nullptr;
        String problem;

        if (root == nullptr || context == nullptr)
            return problem;

        forEachComponent (*root, [&problem, context] (Component& component)
        {
            if (auto* scene = dynamic_cast<SceneComponent*> (&component); scene != nullptr && problem.isEmpty())
                problem = context->isGpuAvailable() ? scene->getRenderer().getLastError() : String ("GPU rendering is not available");
        });

        return problem;
    }

    // Measures the components the window holds now: those a load makes are measured once it made them
    void measurePainting()
    {
        PaintProfileOptions options;
        options.sampleCapacity = statsFrames;

        PaintProfiler::getInstance().enableSubtree (*this, options);
    }

    // The models outlive the document they are set on, and what the audio displays of the interface show outlives them
    struct Session
    {
        YuiToolModels models;
        YuiToolSignals signals;
        YuiView view;
    };

    std::unique_ptr<Session> session;
    String sessionKey;
    File shownInput;
    YuiToolTarget shownTarget;
    std::optional<bool> shownLight;
    WeakReference<Component> use;
    Size<float> interfaceSize;
    Rectangle<float> frame;
    YuiToolGuides guides;
    bool showsStats = false;
};

PreviewWindow* previewWindow = nullptr;
MemoryBlock pendingFile;

// The files the documents loaded since the host last asked name without having been given them, see yuiToolMissingFiles
StringArray missingFiles;

// Keeps the text an entry point returns, which its host reads before it calls the module again
const char* keep (const String& text)
{
    static std::string kept;
    kept = text.toStdString();
    return kept.c_str();
}

//==============================================================================
// Sets the module up to work on the files of its host with no page to open a window in: once, as an
// application is, default theme included
bool prepareToRunWithoutPage()
{
    static const bool hasDriver = SDL_SetHint (SDL_HINT_VIDEO_DRIVER, "dummy");
    static const ScopedYupInitialiser_GUI gui;
    static const ScopedYupInitialiser_Windowing windowing;

    if (! hasDriver || ApplicationTheme::getGlobalTheme() == nullptr)
        return false;

    // A command runs to its end, so the loop the message manager started has nothing to do but keep the
    // host awake: it is stopped, and the runtime that would end with it is kept for the next commands.
    // The loop of a running application is left alone
    static const bool isLoopStopped = []
    {
        if (YUPApplicationBase::getInstance() != nullptr)
            return false;

        emscripten_runtime_keepalive_push();
        emscripten_cancel_main_loop();
        return true;
    }();

    ignoreUnused (isLoopStopped);

    useEmbeddedMaterialShaders();

    getFilesFolder().createDirectory();
    getFilesFolder().setAsCurrentWorkingDirectory();

    return true;
}

// What the tools of an agent get here: the files of the host and, in a page, the window it shows to draw with
YuiAgentHost makeAgentHost()
{
    YuiAgentHost host;
    host.baseFolder = getFilesFolder();

    if (previewWindow == nullptr)
        return host;

    host.snapshot = [] (Component& component, double scale)
    {
        Image image;
        auto* native = previewWindow != nullptr ? previewWindow->getNativeComponent() : nullptr;

        if (auto* context = native != nullptr ? native->getGraphicsContext() : nullptr)
        {
            native->runWithGraphicsContext ([&]
            {
                image = component.snapshotToImage (*context, true, static_cast<float> (scale));
            });
        }

        return image;
    };

    host.capturePreview = [] (const var& arguments)
    {
        return previewWindow != nullptr ? previewWindow->capture (arguments) : YuiAgentResult::fail ("the preview is closed");
    };

    return host;
}

//==============================================================================
class PreviewApplication final : public YUPApplication
{
public:
    PreviewApplication() = default;

    String getApplicationName() override { return "yup_yui_tool"; }

    String getApplicationVersion() override { return YUP_YUI_TOOL_VERSION; }

    void initialise (const String&) override
    {
        useEmbeddedMaterialShaders();

        MessageManager::callAsync ([this]
        {
            // The page has controls of its own, so the keys only go to the interface while its canvas has the focus
            SDL_SetHint (SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");

            window = std::make_unique<PreviewWindow>();
            window->centreWithSize ({ 640, 400 });
            window->setVisible (true);

            previewWindow = window.get();

            // clang-format off
            EM_ASM ({
                if (Module.yuiPreviewReady)
                    Module.yuiPreviewReady();
            });
            // clang-format on
        });
    }

    void shutdown() override
    {
        previewWindow = nullptr;
        window.reset();
    }

private:
    std::unique_ptr<PreviewWindow> window;
};

} // namespace

//==============================================================================
// Called by the tool with each document it loads from a file, see yup_YuiToolCheck.h. The files are those the host
// gave: the ones the document names that are not among them are kept for the host to ask, by their path from the
// folder of the files as the document names them from its own
void noteYuiToolMissingFiles (YuiDocument& document)
{
    const auto folder = document.getBaseDirectory();

    if (folder == File())
        return;

    for (const auto& name : findNamedFiles (document))
    {
        // An absolute path is no file of the host, which gives them by where they are from the document
        if (name.isEmpty() || File::isAbsolutePath (name) || folder.getChildFile (name).existsAsFile())
            continue;

        missingFiles.addIfNotAlreadyThere (folder.getRelativePathFrom (getFilesFolder()) + "/" + name);
    }
}

//==============================================================================
// Called by the host of the module, which gives it the files to work on.
extern "C"
{
    // Returns the memory the host copies the bytes of a file to, before naming it with yuiToolWriteFile
    void* EMSCRIPTEN_KEEPALIVE yuiToolAllocate (int size)
    {
        pendingFile.setSize (static_cast<std::size_t> (jmax (0, size)));
        return pendingFile.getData();
    }

    void EMSCRIPTEN_KEEPALIVE yuiToolWriteFile (const char* path)
    {
        const auto file = getFilesFolder().getChildFile (String::fromUTF8 (path));

        file.getParentDirectory().createDirectory();

        file.replaceWithData (pendingFile.getData(), pendingFile.getSize());

        // Replacing a file with nothing deletes it, and an empty file is still a file
        if (pendingFile.isEmpty())
            file.create();

        pendingFile.reset();
    }

    // Forgets a file the host no longer has
    void EMSCRIPTEN_KEEPALIVE yuiToolRemoveFile (const char* path)
    {
        getFilesFolder().getChildFile (String::fromUTF8 (path)).deleteFile();
    }

    // Returns the files the documents loaded since this was last called name without having been given them, and
    // forgets them: a JSON array of paths from the folder of the files, each written as the folder of its document
    // followed by the path the document names it by. They are what a document reads beside its texts: its shaders,
    // each with what a build compiles it into, and the models and the images of its scenes. The host gives those it
    // has, then runs again what it ran
    const char* EMSCRIPTEN_KEEPALIVE yuiToolMissingFiles()
    {
        return keep (JSON::toString (var (std::exchange (missingFiles, {})), true));
    }

    // Runs a command of the tool in the folder of the files. The arguments are a JSON array of strings. A host
    // that only runs commands loads the module with noInitialRun, which leaves the application unstarted
    int EMSCRIPTEN_KEEPALIVE yuiToolRun (const char* arguments)
    {
        const auto parsed = JSON::parse (String::fromUTF8 (arguments));

        StringArray words ("yup_yui_tool");

        if (const auto* items = parsed.getArray())
            for (const auto& item : *items)
                words.add (item.toString());

        std::vector<char*> argv;

        for (auto& word : words)
            argv.push_back (const_cast<char*> (word.toRawUTF8()));

        const auto argc = static_cast<int> (argv.size());

        if (const auto exitCode = checkYuiToolCommand (argc, argv.data()))
            return *exitCode;

        if (! prepareToRunWithoutPage())
        {
            std::cerr << "yup_yui_tool: can't initialise the windowing system\n";
            return 2;
        }

        const auto exitCode = runYuiToolCommand (argc, argv.data());

        std::cout.flush();
        std::cerr.flush();

        return exitCode;
    }

    // Shows a file in the window of the application, once Module.yuiPreviewReady was called. The names are
    // empty for what is not asked, `use` is the path of the component a drawing or a type shown in context is
    // framed on, and Module.yuiPreviewShown is called with the outcome
    void EMSCRIPTEN_KEEPALIVE yuiPreviewShow (const char* path, const char* drawing, const char* palette, const char* hostType, const char* use, const char* modelsPath, int lightTheme)
    {
        if (previewWindow == nullptr)
            return;

        const auto models = String::fromUTF8 (modelsPath);

        previewWindow->show (getFilesFolder().getChildFile (String::fromUTF8 (path)),
                             { String::fromUTF8 (drawing), String::fromUTF8 (palette), String::fromUTF8 (hostType) },
                             String::fromUTF8 (use),
                             models.isNotEmpty() ? getFilesFolder().getChildFile (models) : File(),
                             lightTheme != 0);
    }

    // Lays the interface shown out at a size in logical pixels, and gives the window the size of what is shown
    // of it, which Module.yuiPreviewFramed (x, y, width, height) is called with first. Once it drew,
    // Module.yuiPreviewDrawn (problem) is called with why a scene of the interface could not be drawn, empty when it was
    void EMSCRIPTEN_KEEPALIVE yuiPreviewSetView (int width, int height)
    {
        if (previewWindow != nullptr && width > 0 && height > 0)
            previewWindow->setView ({ width, height });
    }

    // Shows or hides the guides over the interface shown: the bounds of the components picked by clicking them,
    // or else of the one the caret of the editor is in, with their paint margin and their distances to what
    // holds them. While they show, the pointer picks components instead of reaching the interface
    void EMSCRIPTEN_KEEPALIVE yuiPreviewSetGuides (int shouldShow)
    {
        if (previewWindow != nullptr)
            previewWindow->setGuidesShown (shouldShow != 0);
    }

    // Tells the place of the file shown the caret of the editor is at, a line and a column from 1, or 0 for none
    void EMSCRIPTEN_KEEPALIVE yuiPreviewSetCaret (int line, int column)
    {
        if (previewWindow != nullptr)
            previewWindow->setCaret ({ line, column });
    }

    // Starts or stops measuring how long the components of the interface shown take to paint. It costs a little
    // for each of them, so it is off until asked for
    void EMSCRIPTEN_KEEPALIVE yuiPreviewSetStats (int shouldMeasure)
    {
        if (previewWindow != nullptr)
            previewWindow->setStatsShown (shouldMeasure != 0);
    }

    // Returns how the interface shown is drawn, in three lines of text: the frames a second and the time painting
    // one takes, the pixels and the GPU it is drawn with, and the components taking the longest to paint
    const char* EMSCRIPTEN_KEEPALIVE yuiPreviewStats()
    {
        return keep (previewWindow != nullptr ? previewWindow->describeStats() : String());
    }

    // Changes a value of a model of the interface shown, given as JSON. Values the interface changes itself are
    // reported to Module.yuiPreviewModelChanged (model, property, value)
    int EMSCRIPTEN_KEEPALIVE yuiPreviewSetModelValue (const char* model, const char* property, const char* valueJson)
    {
        if (previewWindow == nullptr)
            return 0;

        const auto value = JSON::fromString (String::fromUTF8 (valueJson));
        return previewWindow->setModelValue (String::fromUTF8 (model), String::fromUTF8 (property), value) ? 1 : 0;
    }

    // Returns the current values of the models as JSON, in the shape of the models file
    const char* EMSCRIPTEN_KEEPALIVE yuiPreviewModels()
    {
        return keep (previewWindow != nullptr ? JSON::toString (previewWindow->getModels(), true) : String ("{}"));
    }

    // Returns the tools of an agent as JSON: `tools`, each with its name, description and inputSchema, `drawing`,
    // the names of those only a page can run as they draw, and `instructions`, how they are meant to be used
    const char* EMSCRIPTEN_KEEPALIVE yuiAgentTools()
    {
        const auto tools = getYuiAgentTools (true);
        Array<var> drawing;

        for (const auto& tool : *tools.getArray())
            if (doesYuiAgentToolDraw (tool["name"].toString()))
                drawing.add (tool["name"]);

        return keep (JSON::toString (var::object ({ { "tools", tools }, { "drawing", var (drawing) }, { "instructions", getYuiAgentInstructions() } }), true));
    }

    // Runs a tool of an agent on the files given, with its arguments as a JSON object. The answer is JSON too:
    // `text`, `isError`, and `image`, a PNG in base64 which is empty when the tool gives none
    const char* EMSCRIPTEN_KEEPALIVE yuiAgentCall (const char* name, const char* arguments)
    {
        // A page set itself up as the application it runs
        const auto isReady = YUPApplicationBase::getInstance() != nullptr || prepareToRunWithoutPage();
        const auto answer = isReady ? callYuiAgentTool (String::fromUTF8 (name), JSON::fromString (String::fromUTF8 (arguments)), makeAgentHost())
                                    : YuiAgentResult::fail ("the windowing system can't be initialised");

        const auto told = var::object ({ { "text", answer.text },
                                         { "isError", answer.isError },
                                         { "image", answer.image.isEmpty() ? String() : Base64::toBase64 (answer.image.getData(), answer.image.getSize()) } });

        return keep (JSON::toString (told, true));
    }
}

START_YUP_APPLICATION (PreviewApplication)
