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
#if YUP_MODULE_AVAILABLE_yup_python
#include <yup_python/yup_python.h>
#endif

#include <memory>
#include <cmath> // For sine wave generation

#if YUP_ANDROID
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
#include "examples/Audio.h"
#include "examples/AudioFileDemo.h"
#include "examples/ColorLab.h"
#include "examples/ConvolutionDemo.h"
#include "examples/CrossoverDemo.h"
#include "examples/FileChooser.h"
#include "examples/FilterDemo.h"
#include "examples/Images.h"
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
        int counter = 0;
        registerDemo<ArtboardDemo> ("Artboard", counter++);
        registerDemo<AudioExample> ("Audio", counter++);
        registerDemo<AudioFileDemo> ("Audio File", counter++);
        registerDemo<ColorLabDemo> ("Color Lab", counter++);
        registerDemo<ConvolutionDemo> ("Convolution Demo", counter++);
        registerDemo<CrossoverDemo> ("Crossover Demo", counter++);
        registerDemo<FileChooserDemo> ("File Chooser", counter++);
        registerDemo<FilterDemo> ("Filter Demo", counter++);
        registerDemo<ImagesDemo> ("Images", counter++);
        registerDemo<LayoutFontsExample> ("Layout Fonts", counter++);
        registerDemo<LottieDemo> ("Lottie", counter++);
        registerDemo<OffscreenRenderDemo> ("Offscreen Render", counter++);
        registerDemo<OpaqueDemo> ("Opaque Demo", counter++);
        registerDemo<PaintProfilerDemo> ("Paint Profiler", counter++);
        registerDemo<PathsExample> ("Paths", counter++);
        registerDemo<PopupMenuDemo> ("Popup Menu", counter++);
        registerDemo<ScrollBarDemo> ("ScrollBar", counter++);
        registerDemo<SliderDemo> ("Sliders", counter++);
        registerDemo<SpectrumAnalyzerDemo> ("FFT Analyzer", counter++);
        registerDemo<SpinningCubeDemo> ("Spinning Cube", counter++);
        registerDemo<SvgDemo> ("SVG", counter++);
        registerDemo<TextEditorDemo> ("Text Editor", counter++);
        registerDemo<VariableFontsExample> ("Variable Fonts", counter++);
        registerDemo<WidgetsDemo> ("Widgets", counter++);
#if YUP_MODULE_AVAILABLE_yup_python
        registerDemo<PythonDemo> ("Python", counter++);
#endif

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

        auto bounds = getLocalBounds().reduced (margin);
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
        for (auto& component : components)
            component->setBounds (bounds);
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
        for (auto& component : components)
            component->setVisible (false);

        components[index]->setVisible (true);
    }

private:
    template <class Demo>
    void registerDemo (const yup::String& name, int counter)
    {
        demoNames.add (name);

        auto demo = std::make_unique<Demo>();
        auto& demoInstance = *demo.get();

        components.add (std::move (demo));
        addChildComponent (components.getLast());
    }

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

#if YUP_IOS
            window->centreWithSize ({ 320, 480 });
#elif YUP_ANDROID
            window->centreWithSize ({ 1080, 2400 });
            // window->setFullScreen(true);
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
