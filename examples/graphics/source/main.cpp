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

//==============================================================================

class TextButton : public yup::Button
{
public:
    TextButton (yup::StringRef componentID, const yup::Font& font)
         : Button (componentID)
         , font (font)
    {
    }

    void paintButton (yup::Graphics& g, bool isButtonOver, bool isButtonDown) override
    {
        auto bounds = getLocalBounds().reduced (proportionOfWidth (0.01f));
        const auto center = bounds.getCenter();

        yup::Path backgroundPath;
        //backgroundPath.clear();
        backgroundPath.addRoundedRectangle (bounds.reduced (proportionOfWidth (0.045f)), 10.0f, 10.0f, 10.0f, 10.0f);
        g.setFillColor (isButtonDown ? yup::Color (0xff000000) : yup::Color (0xffffffff));
        g.fillPath (backgroundPath);

        yup::StyledText text;
        //text.clear();
        text.appendText (font, bounds.getHeight() * 0.5f, bounds.getHeight() * 0.5f, getComponentID().toRawUTF8());
        text.layout (bounds.reduced (0.0f, 10.0f), yup::StyledText::center);

        g.setStrokeColor (isButtonDown ? yup::Color (0xffffffff) : yup::Color (0xff000000));
        g.strokeFittedText (text, {});
    }

private:
    const yup::Font& font;
};

//==============================================================================

class CustomSlider : public yup::Component
{
public:
    CustomSlider (int index, const yup::Font& font)
         : index (index)
         , font (font)
    {
        setTitle (yup::String (index));
        setValue (0.0f);
    }

    void resized() override
    {
        updateRenderItems (true);
    }

    void mouseEnter (const yup::MouseEvent& event) override
    {
        isInside = true;

        repaint();
    }

    void mouseExit (const yup::MouseEvent& event) override
    {
        isInside = false;

        repaint();
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        origin = event.getPosition();

        takeFocus();

        repaint();
    }

    void mouseUp (const yup::MouseEvent& event) override
    {
    }

    void mouseDrag (const yup::MouseEvent& event) override
    {
        //auto [x, y] = event.getPosition();

        const float multiplier =
            (event.getModifiers().isShiftDown() || event.isRightButtonDown()) ? 0.0001f : 0.0025f;

        const float distance = -origin.verticalDistanceTo (event.getPosition()) * multiplier;

        origin = event.getPosition();

        setValue (value + distance);

        repaint();
    }

    void mouseWheel (const yup::MouseEvent& event, const yup::MouseWheelData& data) override
    {
        //auto [x, y] = event.getPosition();

        const float multiplier = event.getModifiers().isShiftDown() ? 0.001f : 0.0025f;
        const float distance = (data.getDeltaX() + data.getDeltaY()) * multiplier;

        origin = event.getPosition();

        setValue (value + distance);

        repaint();
    }

    void paint (yup::Graphics& g) override
    {
        auto bounds = getLocalBounds().reduced (proportionOfWidth (0.1f));

        g.setFillColor (yup::Color (0xff3d3d3d));
        g.fillPath (backgroundPath);

        g.setStrokeColor (yup::Color (0xff2b2b2b));
        g.setStrokeWidth (proportionOfWidth (0.0175f));
        g.strokePath (backgroundPath);

        g.setStrokeCap (yup::StrokeCap::Round);
        g.setStrokeColor (yup::Color (0xff636363));
        g.setStrokeWidth (proportionOfWidth (0.075f));
        g.strokePath (backgroundArc);

        g.setStrokeCap (yup::StrokeCap::Round);
        g.setStrokeColor (isInside ? yup::Color (0xff4ebfff).brighter (0.3f) : yup::Color (0xff4ebfff));
        g.setStrokeWidth (proportionOfWidth (0.075f));
        g.strokePath (foregroundArc);

        g.setStrokeCap (yup::StrokeCap::Round);
        g.setStrokeColor (yup::Color (0xffffffff));
        g.setStrokeWidth (proportionOfWidth (0.03f));
        g.strokePath (foregroundLine);

        g.setStrokeColor (yup::Color (0xffffffff));
        g.strokeFittedText (text, getLocalBounds().reduced (5).removeFromBottom (proportionOfWidth (0.1f)));

        //if (hasFocus())
        //{
        //    g.setStrokeColor (yup::Color (0xffff5f2b));
        //    g.strokeRect (getLocalBounds(), proportionOfWidth (0.0175f));
        //}
    }

    void setValue (float newValue)
    {
        value = yup::jlimit (0.0f, 1.0f, newValue);

        valueChanged();

        updateRenderItems (false);
    }

    virtual void valueChanged()
    {
    }

private:
    void updateRenderItems (bool forceAll)
    {
        auto bounds = getLocalBounds().reduced (proportionOfWidth (0.1f));
        const auto center = bounds.getCenter();

        constexpr auto fromRadians = yup::degreesToRadians(135.0f);
        constexpr auto toRadians = fromRadians + yup::degreesToRadians(270.0f);

        if (forceAll)
        {
            backgroundPath.clear();
            backgroundPath.addEllipse (bounds.reduced (proportionOfWidth (0.105f)));

            backgroundArc.clear();
            backgroundArc.addCenteredArc (center,
                                          bounds.getWidth() / 2.0f, bounds.getHeight() / 2.0f, 0.0f,
                                          fromRadians, toRadians, true);
        }

        const auto toCurrentRadians = fromRadians + yup::degreesToRadians(270.0f) * value;

        foregroundArc.clear();
        foregroundArc.addCenteredArc (center,
                                      bounds.getWidth() / 2.0f, bounds.getHeight() / 2.0f, 0.0f,
                                      fromRadians, toCurrentRadians, true);

        const auto reducedBounds = bounds.reduced (proportionOfWidth (0.175f));
        const auto pos = center.getPointOnCircumference (
            reducedBounds.getWidth() / 2.0f,
            reducedBounds.getHeight() / 2.0f,
            toCurrentRadians);

        foregroundLine.clear();
        foregroundLine.addLine (yup::Line<float> (pos, center).keepOnlyStart (0.25f));

        text.clear();
        text.appendText (font, proportionOfHeight(0.1f), proportionOfHeight(0.1f), yup::String (value, 3).toRawUTF8());
        text.layout (getLocalBounds().reduced (5).removeFromBottom (proportionOfWidth (0.1f)), yup::StyledText::center);
    }

    struct
    {
        yup::Path backgroundPath;
        yup::Path backgroundArc;
        yup::Path foregroundArc;
        yup::Path foregroundLine;
        yup::StyledText text;
    };

    yup::Point<float> origin;
    const yup::Font& font;
    float value = 0.0f;
    int index = 0;
    bool isInside = false;
};

//==============================================================================

class CustomWindow
    : public yup::DocumentWindow
    , public yup::Timer
    , public yup::AudioIODeviceCallback
{
public:
    CustomWindow()
        : yup::DocumentWindow (yup::ComponentNative::defaultFlags, yup::Color (0xff404040))
    {
        rive::Factory* factory = getNativeComponent()->getFactory();
        if (factory == nullptr)
        {
            yup::Logger::outputDebugString ("Failed to create a graphics context");

            yup::YUPApplication::getInstance()->systemRequestedQuit();
            return;
        }

#if JUCE_WASM
        yup::File dataPath = yup::File ("/data");
#else
        yup::File dataPath = yup::File (__FILE__).getParentDirectory().getSiblingFile ("data");
#endif

        yup::File fontFilePath = dataPath.getChildFile ("Roboto-Regular.ttf");

        if (auto result = font.loadFromFile (fontFilePath, factory); result.failed())
            yup::Logger::outputDebugString (result.getErrorMessage());

        setTitle ("main");

        for (int i = 0; i < totalRows * totalColumns; ++i)
            addAndMakeVisible (sliders.add (std::make_unique<CustomSlider> (i, font)));

        button = std::make_unique<TextButton> ("xyz", font);
        addAndMakeVisible (*button);

        //deviceManager.addAudioCallback (this);
        //deviceManager.initialiseWithDefaultDevices (1, 0);

        startTimerHz (1);
    }

    ~CustomWindow() override
    {
        inputReady.signal();
        renderReady.signal();

        //deviceManager.closeAudioDevice();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (100);
        auto width = bounds.getWidth() / totalColumns;
        auto height = bounds.getHeight() / totalRows;

        for (int i = 0; i < totalRows && sliders.size(); ++i)
        {
            auto row = bounds.removeFromTop (height);
            for (int j = 0; j < totalColumns; ++j)
            {
                auto col = row.removeFromLeft (width);
                sliders.getUnchecked (i * totalRows + j)->setBounds (col.largestFittingSquare());
            }
        }

        if (button != nullptr)
            button->setBounds (getLocalBounds().removeFromTop (80).reduced (proportionOfWidth(0.4f), 0.0f));
    }

    void paint (yup::Graphics& g) override
    {
        yup::DocumentWindow::paint (g);

        /*
        yup::String svgPathData =
            "M 250,0 "
            "L 309,181 "
            "L 500,181 "
            "L 351,293 "
            "L 405,475 "
            "L 250,375 "
            "L 95,475 "
            "L 149,293 "
            "L 0,181 "
            "L 191,181 "
            "Z";

        yup::Path yupPath;

        if (yupPath.parsePathData (svgPathData))
        {
            g.setColor (0xff4b4bff);
            g.drawPath (yupPath, 5.0f);
        }
        */

        /*
        //inputReady.wait();
        //std::swap (inputData, renderData);
        //renderReady.signal();

        float xSize = getWidth() / float (renderData.size());

        yup::Path path;
        path.moveTo (0.0f, (renderData[0] + 1.0f) * 0.5f * getHeight());

        for (size_t i = 1; i < renderData.size(); ++i)
            path.lineTo (i * xSize, (renderData[i] + 1.0f) * 0.5f * getHeight());

        g.setColor (0xff4b4bff);
        g.drawPath (path, 5.0f);
        */
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        takeFocus();
    }

    void mouseExit (const yup::MouseEvent& event) override
    {
    }

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
            getNativeComponent()->enableAtomicMode (!getNativeComponent()->isAtomicModeEnabled());
            break;

        case yup::KeyPress::textWKey:
            getNativeComponent()->enableWireframe (!getNativeComponent()->isWireframeEnabled());
            break;

        case yup::KeyPress::textZKey:
            setFullScreen (!isFullScreen());
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

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const yup::AudioIODeviceCallbackContext& context) override
    {
        int copiedSamples = 0;
        while (copiedSamples < numSamples)
        {
            renderData[readPos % renderData.size()] = inputChannelData[0][copiedSamples];

            ++copiedSamples;
            ++readPos;

            readPos %= renderData.size();
        }

        //inputReady.signal();
        //renderReady.wait();
    }

    void audioDeviceAboutToStart (yup::AudioIODevice* device) override
    {
        DBG ("audioDeviceAboutToStart");

        inputData.resize (device->getDefaultBufferSize());
        renderData.resize (device->getDefaultBufferSize());
        readPos = 0;
    }

    void audioDeviceStopped() override
    {
        DBG ("audioDeviceStopped");
    }

private:
    void updateWindowTitle()
    {
        yup::String title;

        title << "[" << yup::String (getNativeComponent()->getCurrentFrameRate(), 1) << " FPS]";
        title << " | " << "YUP On Rive Renderer";

        if (getNativeComponent()->isAtomicModeEnabled())
            title << " (atomic)";

        setTitle (title);
    }

    yup::AudioDeviceManager deviceManager;

    std::vector<float> inputData;
    yup::WaitableEvent inputReady;
    std::vector<float> renderData;
    yup::WaitableEvent renderReady;
    int readPos = 0;

    yup::OwnedArray<CustomSlider> sliders;
    int totalRows = 4;
    int totalColumns = 4;

    std::unique_ptr<TextButton> button;

    yup::Font font;
    yup::StyledText styleText;
};

//==============================================================================

struct Application : yup::YUPApplication
{
    Application() = default;

    const yup::String getApplicationName() override
    {
        return "yup graphics!";
    }

    const yup::String getApplicationVersion() override
    {
        return "1.0";
    }

    void initialise (const yup::String& commandLineParameters) override
    {
        yup::Logger::outputDebugString ("Starting app " + commandLineParameters);

        window = std::make_unique<CustomWindow>();
        window->centreWithSize ({ 800, 800 });
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
