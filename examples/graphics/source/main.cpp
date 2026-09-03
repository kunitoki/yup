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
#include <yup_audio_devices/yup_audio_devices.h>
#include <yup_events/yup_events.h>
#include <yup_graphics/yup_graphics.h>
#include <yup_animation/yup_animation.h>
#include <yup_gui/yup_gui.h>
#include <yup_audio_gui/yup_audio_gui.h>
#include <yup_ai/yup_ai.h>
#if YUP_MODULE_AVAILABLE_yup_python
#include <yup_python/yup_python.h>
#endif

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

//==============================================================================

#if YUP_EXAMPLE_GRAPHICS_DEMO_Artboard
#include "examples/Artboard.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_AI
#include "examples/AI.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Audio
#include "examples/Audio.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_AudioFile
#include "examples/AudioFileDemo.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Clipboard
#include "examples/ClipboardDemo.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ColorLab
#include "examples/ColorLab.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ComponentEffects
#include "examples/ComponentEffectsDemo.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ComputeParticles
#include "examples/ComputeParticlesDemo.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Convolution
#include "examples/ConvolutionDemo.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Crossover
#include "examples/CrossoverDemo.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_CodeEditor
#include "examples/CodeEditor.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_FileChooser
#include "examples/FileChooser.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_FilterDemo
#include "examples/FilterDemo.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_GpuAudio
#include "examples/GpuAudioProcessingDemo.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Images
#include "examples/Images.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_LayoutFonts
#include "examples/LayoutFonts.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Lottie
#include "examples/LottieDemo.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_OffscreenRender
#include "examples/OffscreenRenderDemo.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_OpaqueDemo
#include "examples/OpaqueDemo.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_PaintProfiler
#include "examples/PaintProfilerDemo.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Paths
#include "examples/Paths.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_PopupMenu
#include "examples/PopupMenu.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ScrollBar
#include "examples/ScrollBarDemo.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Sliders
#include "examples/SliderDemo.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_SpectrumAnalyzer
#include "examples/SpectrumAnalyzer.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_SpinningCube
#include "examples/SpinningCubeDemo.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Svg
#include "examples/Svg.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_TextEditor
#include "examples/TextEditor.h"
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ToastNotificationDemo
#include "examples/ToastNotificationDemo.h"
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

class CustomWindow
    : public yup::DocumentWindow
    , public yup::Timer
{
public:
    CustomWindow()
        : yup::DocumentWindow (yup::ComponentNative::Options()
                                   .withAllowedHighDensityDisplay (true),
                               yup::Color (0xff404040))
    {
        setTitle ("main");

        // Load the logo image
        {
            yup::MemoryBlock mb;
            auto imageFile = getAssetPath ("data/logo.png");
            if (imageFile.loadFileAsData (mb))
            {
                auto loadedImage = yup::Image::loadFromData (mb.asBytes());
                if (loadedImage.wasOk())
                    image = std::move (loadedImage.getReference());
            }
            else
            {
                yup::Logger::outputDebugString ("Unable to load requested image");
            }
        }

        // Setup examples
        auto addDemo = [&] (yup::StringRef name, auto factory)
        {
            demoNames.add (name);
            demoFactories.push_back (std::move (factory));
            components.add (nullptr);
        };

        // clang-format off
#if YUP_EXAMPLE_GRAPHICS_DEMO_AI
        addDemo ("AI", [] { return std::make_unique<AIDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Artboard
        addDemo ("Artboard", [] { return std::make_unique<ArtboardDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Audio
        addDemo ("Audio", [] { return std::make_unique<AudioExample>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_AudioFile
        addDemo ("Audio File", [] { return std::make_unique<AudioFileDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Clipboard
        addDemo ("Clipboard", [] { return std::make_unique<ClipboardDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ColorLab
        addDemo ("Color Lab", [] { return std::make_unique<ColorLabDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ComponentEffects
        addDemo ("Component Effects", [] { return std::make_unique<ComponentEffectsDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ComputeParticles
        addDemo ("Compute Particles", [] { return std::make_unique<ComputeParticlesDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Convolution
        addDemo ("Convolution Demo", [] { return std::make_unique<ConvolutionDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Crossover
        addDemo ("Crossover Demo", [] { return std::make_unique<CrossoverDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_CodeEditor
        addDemo ("Code Editor", [] { return std::make_unique<CodeEditorDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_FileChooser
        addDemo ("File Chooser", [] { return std::make_unique<FileChooserDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_FilterDemo
        addDemo ("Filter Demo", [] { return std::make_unique<FilterDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_GpuAudio
        addDemo ("GPU Audio", [] { return std::make_unique<GpuAudioProcessingDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Images
        addDemo ("Images", [] { return std::make_unique<ImagesDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_LayoutFonts
        addDemo ("Layout Fonts", [] { return std::make_unique<LayoutFontsExample>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Lottie
        addDemo ("Lottie", [] { return std::make_unique<LottieDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_OffscreenRender
        addDemo ("Offscreen Render", [] { return std::make_unique<OffscreenRenderDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_OpaqueDemo
        addDemo ("Opaque Demo", [] { return std::make_unique<OpaqueDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_PaintProfiler
        addDemo ("Paint Profiler", [] { return std::make_unique<PaintProfilerDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Paths
        addDemo ("Paths", [] { return std::make_unique<PathsExample>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_PopupMenu
        addDemo ("Popup Menu", [] { return std::make_unique<PopupMenuDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ScrollBar
        addDemo ("ScrollBar", [] { return std::make_unique<ScrollBarDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Sliders
        addDemo ("Sliders", [] { return std::make_unique<SliderDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_SpectrumAnalyzer
        addDemo ("FFT Analyzer", [] { return std::make_unique<SpectrumAnalyzerDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_SpinningCube
        addDemo ("Spinning Cube", [] { return std::make_unique<SpinningCubeDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Svg
        addDemo ("SVG", [] { return std::make_unique<SvgDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_TextEditor
        addDemo ("Text Editor", [] { return std::make_unique<TextEditorDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_ToastNotificationDemo
        addDemo ("Toast Notifications", [] { return std::make_unique<ToastNotificationDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_VariableFonts
        addDemo ("Variable Fonts", [] { return std::make_unique<VariableFontsExample>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Widgets
        addDemo ("Widgets", [] { return std::make_unique<WidgetsDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_YdspSynths && YUP_MODULE_AVAILABLE_yup_dsp_jit
        addDemo ("YDSP Synths", [] { return std::make_unique<YdspSynthDemo>(); });
#endif
#if YUP_EXAMPLE_GRAPHICS_DEMO_Python && YUP_MODULE_AVAILABLE_yup_python
        addDemo ("Python", [] { return std::make_unique<PythonDemo>(); });
#endif
        // clang-format on

        // A single-demo build (see YUP_EXAMPLE_GRAPHICS_DEMO in CMakeLists.txt) only registers one
        // demo, so skip the picker entirely and let that demo fill the whole window.
        singleDemoMode = (demoNames.size() == 1);

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
            listBox->setRowHeight (30);
            listBox->setRowWidth (200);
            listBox->selectRow (0, false, yup::dontSendNotification);
            addAndMakeVisible (listBox.get());
        }

        selectComponent (0);

        startTimerHz (10);
    }

    ~CustomWindow() override
    {
    }

    void resized() override
    {
        constexpr auto margin = 5;
        constexpr auto listBoxWidth = 200;
        constexpr auto listBoxHeight = 40;

        auto bounds = getSafeAreaBounds().reduced (margin);
        auto width = bounds.getWidth();
        auto height = bounds.getHeight();

        if (! singleDemoMode)
        {
            // Landscape orientation (width > height): vertical ListBox on the left
            if (width > height)
            {
                listBox->setOrientation (yup::ListBox::Orientation::vertical);
                listBox->setRowHeight (30);
                listBox->setVerticalScrollBarVisibility (yup::ScrollBar::VisibilityMode::autoHide);
                listBox->setHorizontalScrollBarVisibility (yup::ScrollBar::VisibilityMode::alwaysHidden);

                auto listBoxBounds = bounds.removeFromLeft (listBoxWidth);
                listBox->setBounds (listBoxBounds);

                // Add margin between ListBox and demo components
                bounds.removeFromLeft (margin);
            }
            // Portrait orientation (width <= height): horizontal ListBox on top
            else
            {
                listBox->setOrientation (yup::ListBox::Orientation::horizontal);
                listBox->setRowWidth (80);
                listBox->setRowHeight (listBoxHeight);
                listBox->setVerticalScrollBarVisibility (yup::ScrollBar::VisibilityMode::alwaysHidden);
                listBox->setHorizontalScrollBarVisibility (yup::ScrollBar::VisibilityMode::autoHide);

                auto listBoxBounds = bounds.removeFromTop (listBoxHeight);
                listBox->setBounds (listBoxBounds);

                // Add margin between ListBox and demo components
                bounds.removeFromTop (margin);
            }
        }

        // Demo components take the remaining space
        for (auto* component : components)
        {
            if (component != nullptr)
                component->setBounds (bounds);
        }
    }

    void paint (yup::Graphics& g) override
    {
        yup::DocumentWindow::paint (g);
    }

    void keyDown (const yup::KeyPress& keys, const yup::Point<float>& position) override
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
        }
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

        resized(); // Ensure the newly created component is sized correctly

        components[index]->setVisible (true);
    }

private:
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

    yup::Array<yup::String> demoNames;
    std::vector<std::function<std::unique_ptr<yup::Component>()>> demoFactories;
    std::unique_ptr<DemoListModel> listModel;
    std::unique_ptr<yup::ListBox> listBox;
    yup::OwnedArray<yup::Component> components;
    yup::Image image;
    bool singleDemoMode = false;
};

//==============================================================================

struct Application : yup::YUPApplication
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

        yup::MessageManager::callAsync ([this]
        {
            yup::Process::makeForegroundProcess();

            window = std::make_unique<CustomWindow>();

#if YUP_MOBILE
            window->centreWithSize ({ 720, 1280 });
#else
            window->centreWithSize ({ 1024, 768 });
#endif

            window->setVisible (true);
        });
    }

    void shutdown() override
    {
        yup::Logger::outputDebugString ("Shutting down");

        window.reset();

        YUP_PROFILE_STOP();
    }

private:
    std::unique_ptr<CustomWindow> window;
};

START_YUP_APPLICATION (Application)
