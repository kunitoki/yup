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

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <yup_graphics/yup_graphics.h>
#include <yup_gui/yup_gui.h>

#include <memory>

//==============================================================================

class CustomWindow
    : public yup::DocumentWindow
    , public yup::Timer
{
public:
    CustomWindow()
        // Fluid and continuous animations needs continuous repainting
        : yup::DocumentWindow (yup::ComponentNative::defaultFlags | yup::ComponentNative::renderContinuous, {}, 60.0f)
        , artboard ("Artboard")
    {
        // Set title
        setTitle ("main");

        // Intantiate and load the artboard
        //addAndMakeVisible (artboard);

#if JUCE_WASM
        yup::File riveFilePath = yup::File ("/data");
#else
        yup::File riveFilePath = yup::File (__FILE__).getParentDirectory().getSiblingFile("data");
#endif
        riveFilePath = riveFilePath.getChildFile("mixer_table.riv");

        //artboard.loadFromFile (riveFilePath, 2);
        //artboard.setNumberInput ("rotation", 90.0);

        // Setup artboards
        for (int i = 0; i < totalRows * totalColumns; ++i)
        {
            auto art = artboards.add (std::make_unique<yup::Artboard> (yup::String ("art") + yup::String (i)));
            addAndMakeVisible (art);

            art->loadFromFile (riveFilePath, 2);
        }

        // Update titlebar
        startTimerHz(1);
    }

    void resized() override
    {
        // artboard.setBounds (getLocalBounds());

        auto bounds = getLocalBounds().reduced (100);
        auto width = bounds.getWidth() / totalColumns;
        auto height = bounds.getHeight() / totalRows;

        for (int i = 0; i < totalRows && artboards.size(); ++i)
        {
            auto row = bounds.removeFromTop (height);
            for (int j = 0; j < totalColumns; ++j)
            {
                auto col = row.removeFromLeft (width);
                artboards.getUnchecked (i * totalRows + j)->setBounds (col.largestFittingSquare());
            }
        }
    }

    void keyDown (const yup::KeyPress& keys, const yup::Point<float>& position) override
    {
        const bool shift = keys.getModifiers().isShiftDown();

        switch (keys.getKey())
        {
        case yup::KeyPress::escapeKey:
            userTriedToCloseWindow();
            break;

        case yup::KeyPress::textAKey:
            getNativeComponent()->enableAtomicMode (!getNativeComponent()->isAtomicModeEnabled());
            break;

        case yup::KeyPress::textWKey:
            getNativeComponent()->enableWireframe (!getNativeComponent()->isWireframeEnabled());
            break;

        case yup::KeyPress::textPKey:
            //artboard.setPaused (!artboard.isPaused());
            break;

        case yup::KeyPress::textHKey:
            //artboard.addHorizontalRepeats (shift ? -1 : 1);
            break;

        case yup::KeyPress::textJKey:
            //artboard.addVerticalRepeats (shift ? -1 : 1);
            break;

        case yup::KeyPress::textZKey:
            setFullScreen (!isFullScreen());
            break;

        case yup::KeyPress::upKey:
            //artboard.multiplyScale (1.25, position);
            break;

        case yup::KeyPress::downKey:
            //artboard.multiplyScale (1.0 / 1.25, position);
            break;
        }
    }

    void userTriedToCloseWindow() override
    {
        yup::YUPApplication::getInstance()->systemRequestedQuit();
    }

    void timerCallback() override
    {
        updateWindowTitle();
    }

private:
    void updateWindowTitle ()
    {
        yup::String title;

        auto currentFps = getNativeComponent()->getCurrentFrameRate();
        title << "[" << yup::String (currentFps, 1) << " FPS]";

        auto instances = artboard.getNumInstances();
        title << " (x" << instances << " instances)";
        title << " | " << "YUP On Rive Renderer";

        if (getNativeComponent()->isAtomicModeEnabled())
            title << " (atomic)";

        auto [width, height] = getContentSize();
        title << " | " << width << " x " << height;

        setTitle (title);
    }

    yup::Artboard artboard;
    yup::OwnedArray<yup::Artboard> artboards;
    int totalRows = 4;
    int totalColumns = 4;
};

//==============================================================================

struct Application : yup::YUPApplication
{
    Application() = default;

    const yup::String getApplicationName() override
    {
        return "yup!";
    }

    const yup::String getApplicationVersion() override
    {
        return "1.0";
    }

    void initialise (const yup::String& commandLineParameters) override
    {
        yup::Logger::outputDebugString ("Starting app " + commandLineParameters);

        window = std::make_unique<CustomWindow>();
        window->centreWithSize ({ 1280, 866 });
        window->setVisible (true);
    }

    void shutdown() override
    {
        yup::Logger::outputDebugString ("Shutting down");

        window.reset();
    }

private:
    std::unique_ptr<CustomWindow> window;
};

START_JUCE_APPLICATION(Application)
