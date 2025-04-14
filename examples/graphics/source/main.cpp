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
#include <juce_audio_devices/juce_audio_devices.h>
#include <yup_graphics/yup_graphics.h>
#include <yup_gui/yup_gui.h>

#include <memory>
#include <cmath> // For sine wave generation

#if JUCE_ANDROID
#include <BinaryData.h>
#endif

#include "examples/Audio.h"
#include "examples/LayoutFonts.h"
#include "examples/VariableFonts.h"
#include "examples/Paths.h"

//==============================================================================

class CustomWindow
    : public yup::DocumentWindow
    , public yup::Timer
{
public:
    CustomWindow()
        : yup::DocumentWindow ({}, yup::Color (0xff404040))
    {
        setTitle ("main");

#if JUCE_WASM
        auto baseFilePath = yup::File ("/data");
#else
        auto baseFilePath = yup::File (__FILE__).getParentDirectory().getSiblingFile ("data");
#endif

        // Load the font
        {
#if JUCE_ANDROID
            yup::MemoryBlock mb (yup::RobotoFont_data, yup::RobotoFont_size);
            if (auto result = font.loadFromData (mb); result.failed())
                yup::Logger::outputDebugString (result.getErrorMessage());
#else
            auto fontFilePath = baseFilePath.getChildFile ("RobotoFlex-VariableFont.ttf");
            if (auto result = font.loadFromFile (fontFilePath); result.failed())
                yup::Logger::outputDebugString (result.getErrorMessage());
#endif
        }

        /*
        // Load an image
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
        */

        // Add the demos
        int demo = 2;

        if (demo == 0)
        {
            components.add (std::make_unique<AudioExample> (font));
            addAndMakeVisible (components.getLast());
        }

        if (demo == 1)
        {
            components.add (std::make_unique<LayoutFontsExample> (font));
            addAndMakeVisible (components.getLast());
        }

        if (demo == 2)
        {
            components.add (std::make_unique<VariableFontsExample> (font));
            addAndMakeVisible (components.getLast());
        }

        if (demo == 3)
        {
            components.add (std::make_unique<PathsExample>());
            addAndMakeVisible (components.getLast());
        }

        // Timer
        startTimerHz (10);
    }

    ~CustomWindow() override
    {
    }

    void resized() override
    {
        for (auto& component : components)
            component->setBounds (getLocalBounds());
    }

    void paint (yup::Graphics& g) override
    {
        yup::DocumentWindow::paint (g);

        //g.drawImageAt (image, getLocalBounds().getCenter());
    }

    /*
    void paintOverChildren (yup::Graphics& g) override
    {
        if (! image.isValid())
            return;

        g.setBlendMode (yup::BlendMode::ColorDodge);
        g.setOpacity (1.0f);
        g.drawImageAt (image, getLocalBounds().getCenter());
    }
    */

    void keyDown (const yup::KeyPress& keys, const yup::Point<float>& position) override
    {
        switch (keys.getKey())
        {
            case yup::KeyPress::textQKey:
                std::cout << 'a';
                break;

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

    yup::OwnedArray<yup::Component> components;

    yup::Font font;

    yup::Image image;
};

//==============================================================================

struct Application : yup::YUPApplication
{
    Application() = default;

    const yup::String getApplicationName() override
    {
        return "yup! graphics";
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

#if JUCE_IOS
        window->centreWithSize ({ 320, 480 });
#elif JUCE_ANDROID
        window->centreWithSize ({ 1080, 2400 });
        // window->setFullScreen(true);
#else
        window->centreWithSize ({ 600, 800 });
#endif

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
