/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

#include <yup_core/yup_core.h>
#include <yup_events/yup_events.h>
#include <yup_graphics/yup_graphics.h>
#include <yup_gui/yup_gui.h>

#if YUP_MODULE_AVAILABLE_yup_audio_devices
#include <yup_audio_devices/yup_audio_devices.h>
#endif
#if YUP_MODULE_AVAILABLE_yup_dsp
#include <yup_dsp/yup_dsp.h>
#endif
#if YUP_MODULE_AVAILABLE_yup_audio_gui
#include <yup_audio_gui/yup_audio_gui.h>
#endif
#if YUP_MODULE_AVAILABLE_yup_animation
#include <yup_animation/yup_animation.h>
#endif
#if YUP_MODULE_AVAILABLE_yup_ai
#include <yup_ai/yup_ai.h>
#endif
#if YUP_MODULE_AVAILABLE_yup_python
#include <yup_python/yup_python.h>
#endif

#include <atomic>
#include <memory>
#include <functional>
#include <vector>
#include <cmath> // For sine wave generation

// Enable this to enable leak detection tools on windows
// #define YUP_ENABLE_WINDOWS_BREAK_ALLOC 277639

#if YUP_WINDOWS && YUP_ENABLE_WINDOWS_BREAK_ALLOC
#include <crtdbg.h>
#endif

//==============================================================================

inline yup::File getAssetPath (yup::StringRef subPath = {})
{
    yup::File basePath;

#if YUP_WASM
    basePath = yup::File ("/");
#elif YUP_MOBILE
    basePath = yup::File::getSpecialLocation (yup::File::bundleDirectory);
#else
    basePath = yup::File (__FILE__)
                   .getParentDirectory()
                   .getParentDirectory();
#endif

    if (! subPath.isEmpty())
        basePath = basePath.getChildFile (subPath);

    return basePath;
}

/** Loads a shader bundle precompiled by the SHADERS of a demo in CMakeLists.txt. */
inline yup::ResultValue<yup::ShaderBundle> loadShaderBundle (yup::StringRef name)
{
#if YUP_WASM || YUP_MOBILE
    const auto shadersPath = getAssetPath ("data/shaders");
#else
    const auto shadersPath = yup::File (YUP_EXAMPLE_GRAPHICS_SHADERS_PATH);
#endif

    return yup::ShaderBundle::loadFromFile (shadersPath.getChildFile (yup::String (name) + ".ysl"));
}

/** Compiles a render pipeline from a shader bundle loaded with loadShaderBundle(). */
inline yup::ResultValue<yup::GpuPipeline::Ptr> compilePipelineFromBundle (yup::GpuDevice::Ptr device,
                                                                          yup::StringRef name,
                                                                          const yup::GpuPipelineOptions& options = {})
{
    auto bundle = loadShaderBundle (name);
    if (bundle.failed())
        return yup::makeResultValueFail ("Shader bundle " + yup::String (name) + " failed to load: " + bundle.getErrorMessage());

    return yup::GpuPipeline::compileFromBundle (std::move (device), bundle.getReference(), options);
}

#if YUP_ENABLE_SHADER_TRANSPILER
/** Compiles a GLSL vertex + fragment pair for every target language and saves it as a .ysl shader bundle. */
inline yup::Result saveGlslShaderBundle (const yup::File& file, const yup::String& vertSource, const yup::String& fragSource)
{
    const std::vector<yup::ShaderLanguage> targetLanguages = {
        yup::ShaderLanguage::glsl,
        yup::ShaderLanguage::essl,
        yup::ShaderLanguage::hlsl,
        yup::ShaderLanguage::msl
    };

    auto makeEntry = [&] (yup::ShaderStage stage)
    {
        yup::ShaderBundleEntry entry;
        entry.stage = stage;
        entry.targetLanguages = targetLanguages;
        entry.options.entryPoint = "main";
        return entry;
    };

    yup::ShaderBundleCompiler compiler;

    yup::ShaderBundleCompileRequest vertRequest;
    vertRequest.source = vertSource;
    vertRequest.sourceLanguage = yup::ShaderLanguage::glsl;
    vertRequest.entries.push_back (makeEntry (yup::ShaderStage::vertex));

    auto vsBundle = compiler.compile (vertRequest);
    if (vsBundle.failed())
        return yup::Result::fail ("vertex compilation failed: " + vsBundle.getErrorMessage());

    yup::ShaderBundleCompileRequest fragRequest;
    fragRequest.source = fragSource;
    fragRequest.sourceLanguage = yup::ShaderLanguage::glsl;
    fragRequest.entries.push_back (makeEntry (yup::ShaderStage::fragment));

    auto fsBundle = compiler.compile (fragRequest);
    if (fsBundle.failed())
        return yup::Result::fail ("fragment compilation failed: " + fsBundle.getErrorMessage());

    yup::ShaderBundle bundle;
    for (const auto& info : vsBundle.getReference().getShaders())
        bundle.addShader (info);
    for (const auto& info : fsBundle.getReference().getShaders())
        bundle.addShader (info);

    return bundle.saveToFile (file);
}
#endif

/** Base for demos that take a while to open: the demo window shows a loading bar until all their loading steps have run.

    Register the steps in the constructor. They run in order, one per tick, starting once the first frame (showing the bar)
    has been drawn. Message thread steps run from a timer, render thread steps (GPU work) from pumpLoading(), which the
    demo must call in its paint().
*/
class LoadingDemo : private yup::Timer
{
public:
    enum class StepThread
    {
        message,
        render
    };

    ~LoadingDemo() override = default;

    /** Returns the loading progress from 0 to 1, safe to call from any thread. */
    double getLoadingProgress() const
    {
        return steps.empty() ? 1.0 : (double) stepsDone.load() / (double) steps.size();
    }

    /** Returns true until every step has run, or loading was finished early. */
    bool isLoading() const
    {
        return stepsDone.load() < (int) steps.size();
    }

protected:
    /** Adds a step that runs once on the given thread. */
    void addLoadingStep (std::function<void()> step, StepThread thread = StepThread::message)
    {
        addStep ([step = std::move (step)]
        {
            step();
            return true;
        }, thread);
    }

    /** Adds a step that completes once isReady returns true, polled on the message thread (e.g. work on another thread). */
    void addLoadingWait (std::function<bool()> isReady)
    {
        addStep (std::move (isReady), StepThread::message);
    }

    /** Skips the remaining steps, e.g. after a failure. */
    void finishLoading()
    {
        stepsDone = (int) steps.size();
    }

    /** Call first thing in paint(): runs the next render thread step, and returns true while still loading. */
    bool pumpLoading()
    {
        if (firstFrameDrawn.exchange (true))
            runStep (StepThread::render);

        return isLoading();
    }

private:
    struct Step
    {
        std::function<bool()> run;
        StepThread thread;
    };

    void addStep (std::function<bool()> run, StepThread thread)
    {
        jassert (stepsDone == 0);

        steps.push_back ({ std::move (run), thread });
        startTimerHz (60);
    }

    void runStep (StepThread thread)
    {
        const int index = stepsDone.load();
        if (index >= (int) steps.size() || steps[(size_t) index].thread != thread)
            return;

        if (steps[(size_t) index].run() && stepsDone.load() == index) // The step may have called finishLoading()
            stepsDone = index + 1;
    }

    void timerCallback() override
    {
        if (! isLoading())
            stopTimer();
        else if (firstFrameDrawn) // Rendering holds the message manager lock, so wait for a frame showing the bar
            runStep (StepThread::message);
    }

    std::vector<Step> steps;
    std::atomic<int> stepsDone { 0 };
    std::atomic<bool> firstFrameDrawn { false };
};

//==============================================================================

#if YUP_EXAMPLE_GRAPHICS_DEMO_AI
#include "examples/AI.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Artboard
#include "examples/Artboard.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ArtboardLayout
#include "examples/ArtboardLayout.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_AudioFile
#include "examples/AudioFile.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Clipboard
#include "examples/Clipboard.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ColorLab
#include "examples/ColorLab.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Component3D
#include "examples/Component3D.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ComponentEffects
#include "examples/ComponentEffects.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ComputeParticles
#include "examples/ComputeParticles.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Convolution
#include "examples/Convolution.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Crossover
#include "examples/Crossover.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_CodeEditor
#include "examples/CodeEditor.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_DragAndDrop
#include "examples/DragAndDrop.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Emoji
#include "examples/Emoji.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_FileChooser
#include "examples/FileChooser.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Filter
#include "examples/Filter.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_FluidSimulation
#include "examples/FluidSimulation.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_GpuAudio
#include "examples/GpuAudioProcessing.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Images
#include "examples/Images.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ImageMesh
#include "examples/ImageMesh.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Layout
#include "examples/Layout.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ListBox
#include "examples/ListBox.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Lottie
#include "examples/Lottie.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_OffscreenRender
#include "examples/OffscreenRender.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Opaque
#include "examples/Opaque.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_PaintProfiler
#include "examples/PaintProfiler.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Paths
#include "examples/Paths.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Pbr
#include "examples/Pbr.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_PopupMenu
#include "examples/PopupMenu.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Scene3D
#include "examples/Scene3D.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ScrollBar
#include "examples/ScrollBar.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Shaders && YUP_ENABLE_SHADER_TRANSPILER
#include "examples/Shaders.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Sliders
#include "examples/Slider.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_SpectrumAnalyzer
#include "examples/SpectrumAnalyzer.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_SpinningCube
#include "examples/SpinningCube.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Svg
#include "examples/Svg.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_TextEditor
#include "examples/TextEditor.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Themes
#include "examples/Themes.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ToastNotification
#include "examples/ToastNotification.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_TouchTrails
#include "examples/TouchTrails.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_TreeView
#include "examples/TreeView.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_VariableFonts
#include "examples/VariableFonts.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Widgets
#include "examples/Widgets.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_YdspSynths && YUP_MODULE_AVAILABLE_yup_dsp_jit
#include "examples/YdspSynths.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Python && YUP_MODULE_AVAILABLE_yup_python
#include "examples/Python.h"
#endif

//==============================================================================

class DemoListModel : public yup::ListBoxModel
{
public:
    DemoListModel (yup::Array<yup::String> names)
        : demoNames (std::move (names))
    {
    }

    int getNumRows() override
    {
        return demoNames.size();
    }

    yup::String getRowText (int rowIndex) override
    {
        if (rowIndex >= 0 && rowIndex < demoNames.size())
            return demoNames[rowIndex];
        return {};
    }

    void selectedRowsChanged (const yup::Array<int>& selectedRows) override
    {
        if (onSelectionChanged && ! selectedRows.isEmpty())
            onSelectionChanged (selectedRows[0]);
    }

    std::function<void (int)> onSelectionChanged;

private:
    yup::Array<yup::String> demoNames;
};

//==============================================================================

/** Covers the demo area with a progress bar while a LoadingDemo opens.

    Polls the demo on the message thread. Not opaque, so the demo underneath keeps painting
    (and loading) while hidden.
*/
class LoadingOverlay
    : public yup::Component
    , private yup::Timer
{
public:
    LoadingOverlay()
        : yup::Component ("loadingOverlay")
    {
        setOpaque (false);
        addAndMakeVisible (bar);
    }

    /** Shows the overlay until the demo has loaded; nullptr (a demo that doesn't load) hides it. */
    void track (const LoadingDemo* demoToTrack)
    {
        demo = demoToTrack;
        update();
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (yup::ThemePalette::Role::background));
        g.fillAll();
    }

    void resized() override
    {
        const auto bounds = getLocalBounds();
        bar.setBounds (bounds.withSizeKeepingCenter (yup::jmin (560.0f, bounds.getWidth() * 0.7f), 20.0f));
    }

private:
    void timerCallback() override
    {
        update();
    }

    void update()
    {
        const double progress = demo != nullptr ? demo->getLoadingProgress() : 1.0;
        const bool isLoading = progress < 1.0;

        if (isLoading)
            bar.setProgress (progress, yup::dontSendNotification);

        setVisible (isLoading);

        if (! isLoading)
            stopTimer();
        else if (! isTimerRunning())
            startTimerHz (30);
    }

    const LoadingDemo* demo = nullptr;
    yup::ProgressBar bar { "loadingBar" };
};

//==============================================================================

class CustomWindow
    : public yup::DocumentWindow
    , public yup::Timer
{
public:
    explicit CustomWindow (const std::optional<yup::String>& initialDemo)
        : yup::DocumentWindow (yup::ComponentNative::Options().withVSync (true))
    {
        setTitle ("main");

        // The logo, white for dark themes and black for bright ones
        auto loadImage = [] (const char* path, yup::Image& destination)
        {
            yup::MemoryBlock mb;
            if (! getAssetPath (path).loadFileAsData (mb))
            {
                yup::Logger::outputDebugString ("Unable to load requested image " + yup::String (path));
                return;
            }

            auto loadedImage = yup::Image::loadFromData (mb.asBytes());
            if (loadedImage.wasOk())
                destination = std::move (loadedImage.getReference());
        };

        loadImage ("data/logo.png", logo);
        loadImage ("data/logo_black.png", logoBlack);

        // Setup examples
        auto addDemo = [&] (yup::StringRef id, yup::StringRef name, auto factory)
        {
            demoIds.add (id);
            demoNames.add (name);
            demoFactories.push_back (std::move (factory));
            components.add (nullptr);
        };

        // clang-format off
#if YUP_EXAMPLE_GRAPHICS_DEMO_AI
        addDemo ("AI", "AI", [] { return std::make_unique<AIDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Artboard
        addDemo ("Artboard", "Artboard", [] { return std::make_unique<ArtboardDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ArtboardLayout
        addDemo ("ArtboardLayout", "Artboard Layout", [] { return std::make_unique<ArtboardLayoutDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_AudioFile
        addDemo ("AudioFile", "Audio File", [] { return std::make_unique<AudioFileDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Clipboard
        addDemo ("Clipboard", "Clipboard", [] { return std::make_unique<ClipboardDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ColorLab
        addDemo ("ColorLab", "Color Lab", [] { return std::make_unique<ColorLabDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Component3D
        addDemo ("Component3D", "Component 3D", [] { return std::make_unique<Component3DDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ComponentEffects
        addDemo ("ComponentEffects", "Component Effects", [] { return std::make_unique<ComponentEffectsDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ComputeParticles
        addDemo ("ComputeParticles", "Compute Particles", [] { return std::make_unique<ComputeParticlesDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Convolution
        addDemo ("Convolution", "Convolution Demo", [] { return std::make_unique<ConvolutionDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Crossover
        addDemo ("Crossover", "Crossover Demo", [] { return std::make_unique<CrossoverDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_CodeEditor
        addDemo ("CodeEditor", "Code Editor", [] { return std::make_unique<CodeEditorDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_DragAndDrop
        addDemo ("DragAndDrop", "Drag and Drop", [] { return std::make_unique<DragAndDropDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Emoji
        addDemo ("Emoji", "Emoji", [] { return std::make_unique<EmojiDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_FileChooser
        addDemo ("FileChooser", "File Chooser", [] { return std::make_unique<FileChooserDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Filter
        addDemo ("Filter", "Filter Demo", [] { return std::make_unique<FilterDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_FluidSimulation
        addDemo ("FluidSimulation", "Fluid Simulation", [] { return std::make_unique<FluidSimulationDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_GpuAudio
        addDemo ("GpuAudio", "GPU Audio", [] { return std::make_unique<GpuAudioProcessingDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Images
        addDemo ("Images", "Images", [] { return std::make_unique<ImagesDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ImageMesh
        addDemo ("ImageMesh", "Image Mesh", [] { return std::make_unique<ImageMeshDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Layout
        addDemo ("Layout", "Layout", [] { return std::make_unique<LayoutExample>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ListBox
        addDemo ("ListBox", "List Box", [] { return std::make_unique<ListBoxDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Lottie
        addDemo ("Lottie", "Lottie", [] { return std::make_unique<LottieDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_OffscreenRender
        addDemo ("OffscreenRender", "Offscreen Render", [] { return std::make_unique<OffscreenRenderDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Opaque
        addDemo ("Opaque", "Opaque Demo", [] { return std::make_unique<OpaqueDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_PaintProfiler
        addDemo ("PaintProfiler", "Paint Profiler", [] { return std::make_unique<PaintProfilerDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Paths
        addDemo ("Paths", "Paths", [] { return std::make_unique<PathsExample>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Pbr
        addDemo ("Pbr", "PBR IBL", [] { return std::make_unique<PbrDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_PopupMenu
        addDemo ("PopupMenu", "Popup Menu", [] { return std::make_unique<PopupMenuDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Scene3D
        addDemo ("Scene3D", "Scene 3D", [] { return std::make_unique<Scene3DDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ScrollBar
        addDemo ("ScrollBar", "ScrollBar", [] { return std::make_unique<ScrollBarDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Shaders && YUP_ENABLE_SHADER_TRANSPILER
        addDemo ("Shaders", "Shaders", [] { return std::make_unique<ShadersDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Sliders
        addDemo ("Sliders", "Sliders", [] { return std::make_unique<SliderDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_SpectrumAnalyzer
        addDemo ("SpectrumAnalyzer", "FFT Analyzer", [] { return std::make_unique<SpectrumAnalyzerDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_SpinningCube
        addDemo ("SpinningCube", "Spinning Cube", [] { return std::make_unique<SpinningCubeDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Svg
        addDemo ("Svg", "SVG", [] { return std::make_unique<SvgDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_TextEditor
        addDemo ("TextEditor", "Text Editor", [] { return std::make_unique<TextEditorDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Themes
        addDemo ("Themes", "Themes", [] { return std::make_unique<ThemesDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ToastNotification
        addDemo ("ToastNotification", "Toast Notifications", [] { return std::make_unique<ToastNotificationDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_TouchTrails
        addDemo ("TouchTrails", "Touch Trails", [] { return std::make_unique<TouchTrailsDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_TreeView
        addDemo ("TreeView", "Tree View", [] { return std::make_unique<TreeViewDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_VariableFonts
        addDemo ("VariableFonts", "Variable Fonts", [] { return std::make_unique<VariableFontsExample>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Widgets
        addDemo ("Widgets", "Widgets", [] { return std::make_unique<WidgetsDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_YdspSynths && YUP_MODULE_AVAILABLE_yup_dsp_jit
        addDemo ("YdspSynths", "YDSP Synths", [] { return std::make_unique<YdspSynthDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Python && YUP_MODULE_AVAILABLE_yup_python
        addDemo ("Python", "Python", [] { return std::make_unique<PythonDemo>(); });
#endif
        // clang-format on

        // A single-demo build (see YUP_EXAMPLE_GRAPHICS_DEMO in CMakeLists.txt) only registers one
        // demo, so skip the picker entirely and let that demo fill the whole window.
        singleDemoMode = (demoNames.size() == 1);

        const int initialIndex = singleDemoMode ? 0 : findDemoIndex (initialDemo.value_or(""));

        if (! singleDemoMode)
        {
            // Create the ListBox with the demo names
            listModel = std::make_unique<DemoListModel> (demoNames);
            listModel->onSelectionChanged = [this] (int index)
            {
                selectComponent (index);
            };

            listBox = std::make_unique<yup::ListBox>();
            listBox->setModel (listModel.get());
            listBox->setRowSize (30);
            listBox->setHorizontalScrollBarVisibility (yup::ScrollBar::VisibilityMode::alwaysHidden);
            listBox->setVerticalScrollBarVisibility (yup::ScrollBar::VisibilityMode::autoHide);
            listBox->selectRow (initialIndex, false, yup::dontSendNotification);
            addChildComponent (listBox.get());

            // Takes the list's place when the window is too narrow for it, demos past the end in its menu
            tabBar = std::make_unique<yup::TabBar> ("demoTabs");
            tabBar->setVariant (yup::TabBar::Variant::underline);
            tabBar->setOverflow (yup::TabBar::Overflow::menu);
            for (int i = 0; i < demoIds.size(); ++i)
                tabBar->addTab (yup::Identifier (demoIds[i]), demoNames[i]);
            tabBar->setSelectedTab (yup::Identifier (demoIds[initialIndex]), yup::dontSendNotification);
            tabBar->onSelectionChanged = [this] (const yup::Identifier& tabId)
            {
                selectComponent (tabBar->indexOfTab (tabId));
            };
            addChildComponent (tabBar.get());

            // The list has no size yet: scroll to the selection on its first layout
            scrollToSelectedDemo = true;
        }

        addChildComponent (loadingOverlay);

        selectComponent (initialIndex);

        startTimerHz (1);
    }

    ~CustomWindow() override
    {
    }

    void resized() override
    {
        constexpr auto margin = 5;
        constexpr auto listBoxWidth = 200;
        constexpr auto tabBarHeight = 40;
        constexpr auto logoHeight = 64;
        constexpr auto minimumDemoWidth = 480;

        auto bounds = getSafeAreaBounds().reduced (margin);
        auto width = bounds.getWidth();

        if (! singleDemoMode)
        {
            // The list on the left while it leaves the demo enough room, tabs along the top otherwise,
            // the logo heading either of them
            const auto useTabs = width - listBoxWidth - margin < minimumDemoWidth;
            listBox->setVisible (! useTabs);
            tabBar->setVisible (useTabs);

            if (useTabs)
            {
                auto tabRow = bounds.removeFromTop (tabBarHeight);
                logoBounds = tabRow.removeFromLeft (tabBarHeight).reduced (6);
                tabRow.removeFromLeft (margin);
                tabBar->setBounds (tabRow);

                bounds.removeFromTop (margin);
            }
            else
            {
                auto sidebar = bounds.removeFromLeft (listBoxWidth);
                logoBounds = sidebar.removeFromTop (logoHeight).withSizeKeepingCenter (logoHeight - 12, logoHeight - 12);
                listBox->setBounds (sidebar);

                bounds.removeFromLeft (margin);
            }

            if (scrollToSelectedDemo && currentDemo >= 0 && ! listBox->getLocalBounds().isEmpty())
            {
                listBox->scrollToRow (currentDemo);
                scrollToSelectedDemo = false;
            }
        }

        // Demo components take the remaining space
        for (auto* component : components)
        {
            if (component != nullptr)
                component->setBounds (bounds);
        }

        loadingOverlay.setBounds (bounds);
    }

    void paint (yup::Graphics& g) override
    {
        yup::DocumentWindow::paint (g);

        if (singleDemoMode)
            return;

        const auto background = yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (yup::ThemePalette::Role::background);
        const auto& image = background.getLuminance() > 0.5f ? logoBlack : logo;
        if (image.isValid())
            g.drawImage (image, logoBounds);
    }

    bool keyDown (const yup::KeyPress& keys, const yup::Point<float>& position) override
    {
        switch (keys.getKey())
        {
            case yup::KeyPress::escapeKey:
                userTriedToCloseWindow();
                break;

            case yup::KeyPress::textAKey:
                getNativeComponent()->enableAtomicMode (! getNativeComponent()->isAtomicModeEnabled());
                break;

            case yup::KeyPress::textWKey:
                getNativeComponent()->enableWireframe (! getNativeComponent()->isWireframeEnabled());
                break;

            case yup::KeyPress::textZKey:
                setFullScreen (! isFullScreen());
                break;

            default:
                return false;
        }

        return true;
    }

    void timerCallback() override
    {
        updateWindowTitle();
    }

    void userTriedToCloseWindow() override
    {
        yup::YUPApplication::getInstance()->systemRequestedQuit();
    }

    void selectComponent (int index)
    {
        if (! yup::isPositiveAndBelow (index, components.size()))
            return;

        if (components[index] == nullptr)
        {
            components.set (index, demoFactories[index]().release());
            addChildComponent (components[index]);
            loadingOverlay.toFront (false);
        }

        for (int i = 0; i < components.size(); ++i)
        {
            if (i == index)
                continue;

            if (components[i] != nullptr)
            {
                components[i]->setVisible (false);
                components.set (i, nullptr);
            }
        }

        currentDemo = index;

        // Whichever of the list and the tabs is hidden follows the one that was used
        if (listBox != nullptr && listBox->getSelectedRow() != index)
            listBox->selectRow (index, false, yup::dontSendNotification);
        if (tabBar != nullptr && tabBar->getSelectedTabIndex() != index)
            tabBar->setSelectedTab (yup::Identifier (demoIds[index]), yup::dontSendNotification);

        resized(); // Ensure the newly created component is sized correctly

        components[index]->setVisible (true);

        loadingOverlay.track (dynamic_cast<const LoadingDemo*> (components[index]));
    }

private:
    int findDemoIndex (const yup::String& name) const
    {
        if (name.isEmpty())
            return 0;

        auto normalize = [] (const yup::String& s)
        {
            return s.removeCharacters (" ").toLowerCase();
        };

        const auto wanted = normalize (name);

        for (int i = 0; i < demoIds.size(); ++i)
        {
            if (normalize (demoIds[i]) == wanted || normalize (demoNames[i]) == wanted)
                return i;
        }

        yup::Logger::outputDebugString ("Unknown demo '" + name + "', available: " + demoIds.joinIntoString (", "));
        return 0;
    }

    void updateWindowTitle()
    {
        yup::String title;
        auto nativeComponent = getNativeComponent();

        auto currentFps = nativeComponent ? nativeComponent->getCurrentFrameRate() : 0.0f;
        title << "[" << yup::String (currentFps, 1) << " FPS]";
        title << " | " << yup::YUPApplication::getInstance()->getApplicationName() << " ";

        if (nativeComponent)
        {
            if (auto context = nativeComponent->getGraphicsContext())
            {
                switch (context->getPlatform())
                {
                    case yup::GpuPlatform::Direct3D:
                        title << " | D3D11";
                        break;

                    case yup::GpuPlatform::Metal:
                        title << " | Metal";
                        break;

                    case yup::GpuPlatform::OpenGL:
                        title << " | OpenGL 4.x";
                        break;

                    case yup::GpuPlatform::OpenGLES:
                        title << " | OpenGLES 3.x";
                        break;

                    case yup::GpuPlatform::WebGPU:
                        title << " | WebGPU";
                        break;

                    case yup::GpuPlatform::Vulkan:
                        title << " | Vulkan";
                        break;

                    case yup::GpuPlatform::Headless:
                        title << " | Headless";
                        break;
                }
            }

            if (nativeComponent->isAtomicModeEnabled())
                title << " (atomic)";

            auto [width, height] = nativeComponent->getContentSize();
            title << " | " << width << " x " << height;
        }

        setTitle (title);
    }

    yup::StringArray demoIds;
    yup::Array<yup::String> demoNames;
    std::vector<std::function<std::unique_ptr<yup::Component>()>> demoFactories;
    std::unique_ptr<DemoListModel> listModel;
    std::unique_ptr<yup::ListBox> listBox;
    std::unique_ptr<yup::TabBar> tabBar;
    yup::OwnedArray<yup::Component> components;
    LoadingOverlay loadingOverlay;
    yup::Image logo, logoBlack;
    yup::Rectangle<float> logoBounds;
    int currentDemo = -1;
    bool singleDemoMode = false;
    bool scrollToSelectedDemo = false;
};

//==============================================================================

struct Application : yup::YUPApplication, yup::Timer
{
    Application() = default;

    yup::String getApplicationName() override
    {
        return "YUP! demos";
    }

    yup::String getApplicationVersion() override
    {
        return "1.0";
    }

    void initialise (const yup::String& commandLineParameters) override
    {
#if YUP_WINDOWS && YUP_ENABLE_WINDOWS_BREAK_ALLOC
        _CrtSetBreakAlloc (YUP_ENABLE_WINDOWS_BREAK_ALLOC);
#endif

        YUP_PROFILE_START();

        yup::Logger::outputDebugString ("Starting app " + commandLineParameters);

        std::optional<yup::String> initialDemo;
        std::optional<int> exitAfter;
        
        auto executable = yup::File::getSpecialLocation (yup::File::currentApplicationFile)
            .getFileNameWithoutExtension();

        const auto args = yup::ArgumentList (std::move (executable), yup::YUPApplication::getCommandLineParameterArray());

        if (auto demoArg = args.getValueForOption ("--demo"); demoArg.has_value() && demoArg->isNotEmpty())
            initialDemo = std::move (demoArg);

        if (const auto exitAfterArg = args.getIntValueForOption ("--exit", 0); exitAfterArg > 0)
            exitAfter = exitAfterArg;

        yup::MessageManager::callAsync ([this, initialDemo, exitAfter]
        {
            yup::Process::makeForegroundProcess();

            window = std::make_unique<CustomWindow> (initialDemo);

#if YUP_MOBILE
            window->centreWithSize ({ 720, 1280 });
#else
            window->centreWithSize ({ 1024, 768 });
#endif

            window->setVisible (true);

            if (exitAfter.has_value())
                startTimer (*exitAfter);
        });
    }

    void shutdown() override
    {
        yup::Logger::outputDebugString ("Shutting down");

        window.reset();

        YUP_PROFILE_STOP();
    }

private:
    void timerCallback() override
    {
        yup::YUPApplication::getInstance()->systemRequestedQuit();

        stopTimer();
    }

    std::unique_ptr<CustomWindow> window;
};

START_YUP_APPLICATION (Application)
