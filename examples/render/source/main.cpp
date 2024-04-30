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

#include "rive/math/simd.hpp"
#include "rive/artboard.hpp"
#include "rive/file.hpp"
#include "rive/layout.hpp"
#include "rive/animation/state_machine_instance.hpp"
#include "rive/static_scene.hpp"

#include <memory>

enum class API
{
    gl,
    metal,
    d3d,
    dawn,
};

//==============================================================================

class CustomWindow : public juce::DocumentWindow, public juce::Timer
{
public:
    CustomWindow()
    {
        switch (api)
        {
        case API::metal:
            fiddleContext = juce::LowLevelRenderContext::makeMetalPLS (options);
            break;

        case API::d3d:
            fiddleContext = juce::LowLevelRenderContext::makeD3DPLS (options);
            break;

        case API::dawn:
            fiddleContext = juce::LowLevelRenderContext::makeDawnPLS (options);
            break;

        case API::gl:
            fiddleContext = juce::LowLevelRenderContext::makeGLPLS();
            break;
        }

        if (!fiddleContext)
        {
            juce::Logger::outputDebugString ("Failed to create a fiddle context");

            juce::JUCEApplicationBase::getInstance()->systemRequestedQuit();
            return;
        }

        rive::Factory* factory = fiddleContext->factory();

#if JUCE_WASM
        juce::File riveFilePath = juce::File ("/data");
#else
        juce::File riveFilePath = juce::File (__FILE__).getParentDirectory().getSiblingFile("data");
#endif
        riveFilePath = riveFilePath.getChildFile("car_interface.riv");

        if (riveFilePath.existsAsFile())
        {
            if (auto is = riveFilePath.createInputStream(); is != nullptr && is->openedOk())
            {
                juce::MemoryBlock mb;
                is->readIntoMemoryBlock (mb);

                rivFile = rive::File::import( { static_cast<const uint8_t*> (mb.getData()), mb.getSize() }, factory);
            }
        }

        startTimerHz (static_cast<int> (framerate));
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (scenes.empty() || ! event.isLeftButtoDown())
            return;

        auto [x, y] = event.getPosition();

        float dpiScale = fiddleContext->dpiScale (nativeHandle());
        x *= dpiScale;
        y *= dpiScale;

        auto xy = viewTransform.invertOrIdentity() * rive::Vec2D (x, y);
        for (auto& scene : scenes)
            scene->pointerDown (xy);
    }

    void mouseUp (const juce::MouseEvent& event) override
    {
        if (scenes.empty() || ! event.isLeftButtoDown())
            return;

        auto [x, y] = event.getPosition();

        float dpiScale = fiddleContext->dpiScale (nativeHandle());
        x *= dpiScale;
        y *= dpiScale;

        auto xy = viewTransform.invertOrIdentity() * rive::Vec2D (x, y);
        for (auto& scene : scenes)
            scene->pointerUp (xy);
    }

    void mouseMove (const juce::MouseEvent& event) override
    {
        if (scenes.empty())
            return;

        auto [x, y] = event.getPosition();

        float dpiScale = fiddleContext->dpiScale (nativeHandle());
        x *= dpiScale;
        y *= dpiScale;

        const auto xy = viewTransform.invertOrIdentity() * rive::Vec2D (x, y);
        for (auto& scene : scenes)
            scene->pointerMove (xy);
    }

    void mouseDrag (const juce::MouseEvent& event) override
    {
        if (scenes.empty() || ! event.isLeftButtoDown())
            return;

        auto [x, y] = event.getPosition();

        float dpiScale = fiddleContext->dpiScale (nativeHandle());
        x *= dpiScale;
        y *= dpiScale;

        const auto xy = viewTransform.invertOrIdentity() * rive::Vec2D (x, y);
        for (auto& scene : scenes)
            scene->pointerMove (xy);
    }

    void keyDown (const juce::KeyPress& keys, double x, double y) override
    {
        const bool shift = keys.getModifiers().isShiftDown();

        switch (keys.getKey())
        {
        case juce::KeyPress::escapeKey:
            close();
            break;

        case juce::KeyPress::textAKey:
            forceAtomicMode = !forceAtomicMode;
            fpsLastTime = 0;
            fpsFrames = 0;
            needsTitleUpdate = true;
            break;

        case juce::KeyPress::textDKey:
            printf ("static float scale = %f;\n", scale);
            printf ("static float2 translate = {%f, %f};\n", translate.x, translate.y);
            fflush(stdout);
            break;

        case juce::KeyPress::textWKey:
            wireframe = !wireframe;
            break;

        case juce::KeyPress::textPKey:
            paused = !paused;
            break;

        case juce::KeyPress::textHKey:
            if (!shift)
                ++horzRepeat;
            else if (horzRepeat > 0)
                --horzRepeat;
            break;

        case juce::KeyPress::textKKey:
            if (!shift)
                ++upRepeat;
            else if (upRepeat > 0)
                --upRepeat;
            break;

        case juce::KeyPress::textJKey:
            if (!shift)
                ++downRepeat;
            else if (downRepeat > 0)
                --downRepeat;
            break;

        case juce::KeyPress::upKey:
        {
            float oldScale = scale;
            scale *= 1.25;
            rive::float2 cursorPos = rive::float2 { (float)x, (float)y } * fiddleContext->dpiScale (nativeHandle());
            translate = cursorPos + (translate - cursorPos) * scale / oldScale;
            break;
        }

        case juce::KeyPress::downKey:
        {
            float oldScale = scale;
            scale /= 1.25;
            rive::float2 cursorPos = rive::float2 { (float)x, (float)y } * fiddleContext->dpiScale (nativeHandle());
            translate = cursorPos + (translate - cursorPos) * scale / oldScale;
            break;
        }
        }
    }

    void updateWindowTitle (double fps, int instances, int width, int height)
    {
        juce::String title;

        if (fps != 0)
            title << "[" << fps << " FPS]";

        if (instances > 1)
            title << " (x" << instances << " instances)";

        title << " | " << "YUP On Rive Renderer";

        if (forceAtomicMode)
            title << " (atomic)";

        title << " | " << width << " x " << height;

        setWindowTitle (title);
    }

private:
    void timerCallback() override
    {
        if (shouldClose())
        {
            stopTimer();

            juce::MessageManager::callAsync ([this] { juce::JUCEApplication::getInstance()->systemRequestedQuit(); });
            return;
        }

        mainLoop (juce::Time::getMillisecondCounterHiRes() / 1000.0);

        fiddleContext->tick();
    }

    void updateScenesFromFile (std::size_t count)
    {
        jassert (rivFile != nullptr);

        artboards.clear();
        scenes.clear();
        for (size_t i = 0; i < count; ++i)
        {
            auto artboard = rivFile->artboardDefault();

            std::unique_ptr<rive::Scene> scene;

            //scene = artboard->defaultStateMachine();
            if (scene == nullptr)
            {
                if (stateMachine >= 0)
                {
                    scene = artboard->stateMachineAt(stateMachine);
                }
                else if (animation >= 0)
                {
                    scene = artboard->animationAt(animation);
                }
                else
                {
                    scene = artboard->animationAt(0);
                }
            }

            if (scene == nullptr)
            {
                // This is a riv without any animations or state machines. Just draw the artboard.
                scene = std::make_unique<rive::StaticScene>(artboard.get());
            }

            scene->advanceAndApply(scene->durationSeconds() * i / count);

            artboards.push_back(std::move(artboard));
            scenes.push_back(std::move(scene));
        }
    }

    void mainLoop (double time)
    {
        auto [width, height] = getSize();
        if (lastWidth != width || lastHeight != height)
        {
            DBG ("size changed to " << width << "x" << height << "\n");

            lastWidth = width;
            lastHeight = height;

            fiddleContext->onSizeChanged (nativeHandle(), width, height, 0);
            renderer = fiddleContext->makeRenderer (width, height);

            needsTitleUpdate = true;
        }

        if (needsTitleUpdate)
        {
            updateWindowTitle (0, 1, width, height);
            needsTitleUpdate = false;
        }

        fiddleContext->begin ({
            .renderTargetWidth = static_cast<uint32_t> (width),
            .renderTargetHeight = static_cast<uint32_t> (height),
            .clearColor = 0xff404040,
            .msaaSampleCount = 0,
            .disableRasterOrdering = forceAtomicMode,
            .wireframe = wireframe,
            .fillsDisabled = disableFill,
            .strokesDisabled = disableStroke,
        });

        int instances = 1;
        if (rivFile)
        {
            instances = (1 + horzRepeat * 2) * (1 + upRepeat + downRepeat);
            if (artboards.size() != instances || scenes.size() != instances)
            {
                updateScenesFromFile (instances);
            }
            else if (!paused)
            {
                for (const auto& scene : scenes)
                {
                    scene->advanceAndApply (1 / framerate);
                }
            }

            rive::Mat2D m = computeAlignment (rive::Fit::contain,
                                              rive::Alignment::center,
                                              rive::AABB (0, 0, width, height),
                                              artboards.front()->bounds());
            renderer->save();

            m = rive::Mat2D (scale, 0, 0, scale, translate.x, translate.y) * m;
            viewTransform = m;

            renderer->transform (m);

            const float spacing = 200 / m.findMaxScale();

            auto scene = scenes.begin();
            for (int j = 0; j < upRepeat + 1 + downRepeat; ++j)
            {
                renderer->save();
                renderer->transform(
                    rive::Mat2D::fromTranslate (-spacing * horzRepeat, (j - upRepeat) * spacing));

                for (int i = 0; i < horzRepeat * 2 + 1; ++i)
                {
                    (*scene++)->draw (renderer.get());
                    renderer->transform (rive::Mat2D::fromTranslate (spacing, 0));
                }

                renderer->restore();
            }
            renderer->restore();
        }

        fiddleContext->end (nativeHandle());

        updateFrameTime (time, width, height);
    }

    void updateFrameTime (double time, int width, int height)
    {
        ++fpsFrames;

        double fpsElapsed = time - fpsLastTime;
        if (fpsElapsed > 2)
        {
            int instances = (1 + horzRepeat * 2) * (1 + upRepeat + downRepeat);
            double fps = fpsLastTime == 0 ? 0 : fpsFrames / fpsElapsed;

            updateWindowTitle (fps, instances, width, height);

            fpsFrames = 0;
            fpsLastTime = time;
        }
    }

    juce::LowLevelRenderContext::Options options;
    bool forceAtomicMode = false;
    bool wireframe = false;
    bool disableFill = false;
    bool disableStroke = false;

    float framerate = 30.0f;

    API api =
    #if JUCE_MAC || JUCE_IOS
        API::metal
    #elif JUCE_WINDOWS
        API::d3d
    #else
        API::gl
    #endif
    ;

    std::unique_ptr<juce::LowLevelRenderContext> fiddleContext;
    std::unique_ptr<rive::Renderer> renderer;

    std::unique_ptr<rive::File> rivFile;
    std::vector<std::unique_ptr<rive::Artboard>> artboards;
    std::vector<std::unique_ptr<rive::Scene>> scenes;

    rive::Mat2D viewTransform;
    rive::float2 translate;
    float scale = 1.0f;

    bool paused = false;

    int animation = -1;
    int stateMachine = -1;

    int horzRepeat = 0;
    int upRepeat = 0;
    int downRepeat = 0;

    int lastWidth = 0, lastHeight = 0;
    double fpsLastTime = 0;
    int fpsFrames = 0;
    bool needsTitleUpdate = false;
};

//==============================================================================

struct Application : juce::JUCEApplication
{
    Application() = default;

    const juce::String getApplicationName() override
    {
        return "yup!";
    }

    const juce::String getApplicationVersion() override
    {
        return "1.0";
    }

    void initialise (const juce::String& commandLineParameters) override
    {
        juce::Logger::outputDebugString ("Starting app " + commandLineParameters);

        window = std::make_unique<CustomWindow>();
        window->setSize (1280, 866);
        window->setVisible (true);
    }

    void shutdown() override
    {
        juce::Logger::outputDebugString ("Shutting down");

        window.reset();
    }

private:
    std::unique_ptr<CustomWindow> window;
};

START_JUCE_APPLICATION(Application)
