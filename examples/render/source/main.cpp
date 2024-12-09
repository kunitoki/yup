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

#if JUCE_ANDROID
#include <BinaryData.h>
#endif

//==============================================================================

class CustomWindow
    : public yup::DocumentWindow
    , public yup::Timer
{
public:
    CustomWindow()
        // Fluid and continuous animations needs continuous repainting
        : yup::DocumentWindow (yup::ComponentNative::Options()
            .withFlags(yup::ComponentNative::defaultFlags | yup::ComponentNative::renderContinuous), {})
    {
        // Set title
        setTitle ("main");

#if JUCE_WASM
        yup::File riveFilePath = yup::File ("/data")
            .getChildFile ("artboard.riv");
#else
        yup::File riveFilePath = yup::File (__FILE__)
            .getParentDirectory()
            .getSiblingFile ("data")
            .getChildFile ("alien.riv");
#endif

        // Setup artboards
        for (int i = 0; i < totalRows * totalColumns; ++i)
        {
            auto art = artboards.add (std::make_unique<yup::Artboard> (yup::String ("art") + yup::String (i)));
            addAndMakeVisible (art);

#if JUCE_ANDROID
            yup::MemoryInputStream is(yup::RiveFile_data, yup::RiveFile_size, false);
            art->loadFromStream (is, 0, true);
#else
            art->loadFromFile (riveFilePath, 0, true);
#endif

            art->advanceAndApply (i * 1.0f);
        }

        // Grab focus
        takeFocus();

        // Update titlebar
        startTimerHz (1);
    }

    void resized() override
    {
        //for (int i = 0; i < totalRows * totalColumns; ++i)
        //    artboards.getUnchecked (i)->setBounds (getLocalBounds().reduced (100.0f));

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
                getNativeComponent()->enableAtomicMode (! getNativeComponent()->isAtomicModeEnabled());
                break;

            case yup::KeyPress::textWKey:
                getNativeComponent()->enableWireframe (! getNativeComponent()->isWireframeEnabled());
                break;

            case yup::KeyPress::textPKey:
                for (int i = 0; i < totalRows * totalColumns; ++i)
                    artboards[i]->setPaused (! artboards[i]->isPaused());
                break;

            case yup::KeyPress::textZKey:
                setFullScreen (! isFullScreen());
                break;
        }
    }

    void paint (yup::Graphics& g) override
    {
        //g.setStrokeColor (0xffffffff);

        //for (const auto& artboard : artboards)
        //    g.strokeRect (artboard->getBounds());
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
    void updateWindowTitle()
    {
        yup::String title;

        auto currentFps = getNativeComponent()->getCurrentFrameRate();
        title << "[" << yup::String (currentFps, 1) << " FPS]";

        title << " (x" << (totalRows * totalColumns) << " instances)";
        title << " | "
              << "YUP Renderer";

        if (getNativeComponent()->isAtomicModeEnabled())
            title << " (atomic)";

        auto [width, height] = getContentSize();
        title << " | " << width << " x " << height;

        setTitle (title);
    }

    yup::OwnedArray<yup::Artboard> artboards;
    int totalRows = 2;
    int totalColumns = 2;
};

//==============================================================================

struct Application : yup::YUPApplication
{
    Application() = default;

    const yup::String getApplicationName() override
    {
        return "yup! render";
    }

    const yup::String getApplicationVersion() override
    {
        return "1.0";
    }

    void initialise (const yup::String& commandLineParameters) override
    {
        YUP_PROFILE_START();

        yup::Logger::outputDebugString ("Starting app " + commandLineParameters);

        window = std::make_unique<CustomWindow>();
        window->centreWithSize ({ 1280, 866 });
        window->setVisible (true);
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

START_JUCE_APPLICATION (Application)
