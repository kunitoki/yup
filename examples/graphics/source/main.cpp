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

#if YUP_MOBILE
#include <BinaryData.h>
#endif

//==============================================================================

inline yup::File getAssetPath (yup::StringRef subPath = {})
{
    yup::File basePath;

#if YUP_WASM
    basePath = yup::File ("/");
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

#include "examples/Artboard.h"
#include "examples/AI.h"
#include "examples/Audio.h"
#include "examples/AudioFileDemo.h"
#include "examples/ClipboardDemo.h"
#include "examples/ColorLab.h"
#include "examples/ComponentEffectsDemo.h"
#include "examples/ComputeParticlesDemo.h"
#include "examples/ConvolutionDemo.h"
#include "examples/CrossoverDemo.h"
#include "examples/FileChooser.h"
#include "examples/FilterDemo.h"
#include "examples/GpuAudioProcessingDemo.h"
#include "examples/Images.h"
#include "examples/Layout.h"
#include "examples/LayoutFonts.h"
#include "examples/LottieDemo.h"
#include "examples/OffscreenRenderDemo.h"
#include "examples/OpaqueDemo.h"
#include "examples/PaintProfilerDemo.h"
#include "examples/Paths.h"
#include "examples/PopupMenu.h"
#include "examples/ScrollBarDemo.h"
#include "examples/SliderDemo.h"
#include "examples/SpectrumAnalyzer.h"
#include "examples/SpinningCubeDemo.h"
#include "examples/Svg.h"
#include "examples/TextEditor.h"
#include "examples/VariableFonts.h"
#include "examples/Widgets.h"
#if YUP_MODULE_AVAILABLE_yup_python
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
#if YUP_WASM
        auto baseFilePath = yup::File ("/data");
#else
        auto baseFilePath = yup::File (__FILE__).getParentDirectory().getSiblingFile ("data");
#endif
        {
            yup::MemoryBlock mb;
            auto imageFile = baseFilePath.getChildFile ("logo.png");
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
        addDemo ("AI", [] { return std::make_unique<AIDemo>(); });
        addDemo ("Artboard", [] { return std::make_unique<ArtboardDemo>(); });
        addDemo ("Audio", [] { return std::make_unique<AudioExample>(); });
        addDemo ("Audio File", [] { return std::make_unique<AudioFileDemo>(); });
        addDemo ("Clipboard", [] { return std::make_unique<ClipboardDemo>(); });
        addDemo ("Color Lab", [] { return std::make_unique<ColorLabDemo>(); });
        addDemo ("Component Effects", [] { return std::make_unique<ComponentEffectsDemo>(); });
        addDemo ("Compute Particles", [] { return std::make_unique<ComputeParticlesDemo>(); });
        addDemo ("Convolution Demo", [] { return std::make_unique<ConvolutionDemo>(); });
        addDemo ("Crossover Demo", [] { return std::make_unique<CrossoverDemo>(); });
        addDemo ("File Chooser", [] { return std::make_unique<FileChooserDemo>(); });
        addDemo ("Filter Demo", [] { return std::make_unique<FilterDemo>(); });
        addDemo ("GPU Audio", [] { return std::make_unique<GpuAudioProcessingDemo>(); });
        addDemo ("Images", [] { return std::make_unique<ImagesDemo>(); });
        addDemo ("Layout", [] { return std::make_unique<LayoutExample>(); });
        addDemo ("Layout Fonts", [] { return std::make_unique<LayoutFontsExample>(); });
        addDemo ("Lottie", [] { return std::make_unique<LottieDemo>(); });
        addDemo ("Offscreen Render", [] { return std::make_unique<OffscreenRenderDemo>(); });
        addDemo ("Opaque Demo", [] { return std::make_unique<OpaqueDemo>(); });
        addDemo ("Paint Profiler", [] { return std::make_unique<PaintProfilerDemo>(); });
        addDemo ("Paths", [] { return std::make_unique<PathsExample>(); });
        addDemo ("Popup Menu", [] { return std::make_unique<PopupMenuDemo>(); });
        addDemo ("ScrollBar", [] { return std::make_unique<ScrollBarDemo>(); });
        addDemo ("Sliders", [] { return std::make_unique<SliderDemo>(); });
        addDemo ("FFT Analyzer", [] { return std::make_unique<SpectrumAnalyzerDemo>(); });
        addDemo ("Spinning Cube", [] { return std::make_unique<SpinningCubeDemo>(); });
        addDemo ("SVG", [] { return std::make_unique<SvgDemo>(); });
        addDemo ("Text Editor", [] { return std::make_unique<TextEditorDemo>(); });
        addDemo ("Variable Fonts", [] { return std::make_unique<VariableFontsExample>(); });
        addDemo ("Widgets", [] { return std::make_unique<WidgetsDemo>(); });
#if YUP_MODULE_AVAILABLE_yup_python
        addDemo ("Python", [] { return std::make_unique<PythonDemo>(); });
#endif
        // clang-format on

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
        for (auto* component : components)
        {
            if (component != nullptr)
                component->setVisible (false);
        }

        if (components[index] == nullptr)
        {
            components.set (index, demoFactories[index]().release());
            addChildComponent (components[index]);
        }

        resized(); // Ensure the newly created component is sized correctly
        components[index]->setVisible (true);
    }

private:
    void updateWindowTitle()
    {
        yup::String title;

        auto currentFps = getNativeComponent()->getCurrentFrameRate();
        title << "[" << yup::String (currentFps, 1) << " FPS]";
        title << " | YUP On Rive Renderer";

        if (getNativeComponent()->isAtomicModeEnabled())
            title << " (atomic)";

        auto [width, height] = getNativeComponent()->getContentSize();
        title << " | " << width << " x " << height;

        setTitle (title);
    }

    yup::Array<yup::String> demoNames;
    std::vector<std::function<std::unique_ptr<yup::Component>()>> demoFactories;
    std::unique_ptr<DemoListModel> listModel;
    std::unique_ptr<yup::ListBox> listBox;
    yup::OwnedArray<yup::Component> components;
    yup::Image image;
};

//==============================================================================

struct Application : yup::YUPApplication
{
    Application() = default;

    yup::String getApplicationName() override
    {
        return "yup! graphics";
    }

    yup::String getApplicationVersion() override
    {
        return "1.0";
    }

    void initialise (const yup::String& commandLineParameters) override
    {
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
