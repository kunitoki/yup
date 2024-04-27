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
#include <yup_gui/yup_gui.h>

#include "rive/math/simd.hpp"
#include "rive/artboard.hpp"
#include "rive/file.hpp"
#include "rive/layout.hpp"
#include "rive/animation/state_machine_instance.hpp"
#include "rive/static_scene.hpp"

#include "fiddle_context.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

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
            if (skia)
                fprintf(stderr, "Skia not supported on Metal yet.\n");
            else
                fiddleContext = FiddleContext::MakeMetalPLS (options);
            break;

        case API::d3d:
            if (skia)
                fprintf(stderr, "Skia not supported on d3d yet.\n");
            else
                fiddleContext = FiddleContext::MakeD3DPLS (options);
            break;

        case API::dawn:
            if (skia)
                fprintf(stderr, "Skia not supported on dawn yet.\n");
            else
                fiddleContext = FiddleContext::MakeDawnPLS (options);
            break;

        case API::gl:
            if (skia)
                fiddleContext = FiddleContext::MakeGLSkia();
            else
                fiddleContext = FiddleContext::MakeGLPLS();
            break;
        }

        if (!fiddleContext)
        {
            fprintf(stderr, "Failed to create a fiddle context.\n");

            juce::JUCEApplicationBase::getInstance()->systemRequestedQuit();
            return;
        }

        rive::Factory* factory = fiddleContext->factory();

        auto rivName = juce::File (__FILE__)
            .getParentDirectory()
            .getSiblingFile("data")
            .getChildFile("seasynth.riv");

        if (rivName.existsAsFile())
        {
            if (auto is = rivName.createInputStream(); is != nullptr && is->openedOk())
            {
                juce::MemoryBlock mb;
                is->readIntoMemoryBlock (mb);

                rivFile = rive::File::import( { static_cast<const uint8_t*> (mb.getData()), mb.getSize() }, factory);
            }
        }

        startTimerHz (static_cast<int> (framerate));
    }

    void mouseDown(int button, int mods, double x, double y) override
    {
        float dpiScale = fiddleContext->dpiScale (nativeHandle());
        x *= dpiScale;
        y *= dpiScale;

        if (scenes.empty())
        {
            dragLastPos = rive::float2 { (float)x, (float)y };
            if (button == GLFW_MOUSE_BUTTON_LEFT)
            {
                dragIdx = -1;
                if (rivFile != nullptr)
                    return;

                for (int i = 0; i < kNumInteractivePts; ++i)
                {
                    if (rive::simd::all (rive::simd::abs (dragLastPos - (pts[i] + translate)) < 100))
                    {
                        dragIdx = i;
                        break;
                    }
                }
            }
        }
        else
        {
            auto xy = viewTransform.invertOrIdentity() * rive::Vec2D (x, y);
            for (auto& scene : scenes)
                scene->pointerDown (xy);
        }
    }

    void mouseUp (int button, int mods, double x, double y) override
    {
        float dpiScale = fiddleContext->dpiScale (nativeHandle());
        x *= dpiScale;
        y *= dpiScale;

        if (scenes.empty())
            return;

        auto xy = viewTransform.invertOrIdentity() * rive::Vec2D (x, y);
        for (auto& scene : scenes)
            scene->pointerUp (xy);
    }

    void mouseMove (int button, int mods, double x, double y) override
    {
        float dpiScale = fiddleContext->dpiScale (nativeHandle());
        x *= dpiScale;
        y *= dpiScale;

        if (scenes.empty())
            return;

        const auto xy = viewTransform.invertOrIdentity() * rive::Vec2D (x, y);
        for (auto& scene : scenes)
            scene->pointerMove (xy);
    }

    void mouseDrag (int button, int mods, double x, double y) override
    {
        float dpiScale = fiddleContext->dpiScale (nativeHandle());
        x *= dpiScale;
        y *= dpiScale;

        if (scenes.empty())
        {
            if (button == GLFW_MOUSE_BUTTON_LEFT)
            {
                rive::float2 pos = rive::float2 { (float)x, (float)y };
                if (dragIdx >= 0)
                    pts[dragIdx] += (pos - dragLastPos);
                else
                    translate += (pos - dragLastPos);
                dragLastPos = pos;
            }
        }
        else
        {
            const auto xy = viewTransform.invertOrIdentity() * rive::Vec2D (x, y);
            for (auto& scene : scenes)
                scene->pointerMove (xy);
        }
    }

    void keyDown (int key, int scancode, int mods, double x, double y) override
    {
        const bool shift = mods & GLFW_MOD_SHIFT;

        switch (key)
        {
        case GLFW_KEY_ESCAPE:
            close();
            break;

        case GLFW_KEY_A:
            forceAtomicMode = !forceAtomicMode;
            fpsLastTime = 0;
            fpsFrames = 0;
            needsTitleUpdate = true;
            break;

        case GLFW_KEY_D:
            printf ("static float scale = %f;\n", scale);
            printf ("static float2 translate = {%f, %f};\n", translate.x, translate.y);
            printf ("static float2 pts[] = {");
            for (int i = 0; i < kNumInteractivePts; i++)
            {
                printf("{%g, %g}", pts[i].x, pts[i].y);
                if (i < kNumInteractivePts - 1)
                    printf(", ");
                else
                    printf("};\n");
            }
            fflush(stdout);
            break;

        case GLFW_KEY_Z:
            fiddleContext->toggleZoomWindow();
            break;

        case GLFW_KEY_1:
            strokeWidth /= 1.5f;
            break;

        case GLFW_KEY_2:
            strokeWidth *= 1.5f;
            break;

        case GLFW_KEY_W:
            wireframe = !wireframe;
            break;

        case GLFW_KEY_C:
            cap = static_cast<rive::StrokeCap> ((static_cast<int> (cap) + 1) % 3);
            break;

        case GLFW_KEY_O:
            doClose = !doClose;
            break;

        case GLFW_KEY_S:
            disableStroke = !disableStroke;
            break;

        case GLFW_KEY_F:
            disableFill = !disableFill;
            break;

        case GLFW_KEY_P:
            paused = !paused;
            break;

        case GLFW_KEY_H:
            if (!shift)
                ++horzRepeat;
            else if (horzRepeat > 0)
                --horzRepeat;
            break;

        case GLFW_KEY_K:
            if (!shift)
                ++upRepeat;
            else if (upRepeat > 0)
                --upRepeat;
            break;

        case GLFW_KEY_J:
            if (!rivFile)
                join = static_cast<rive::StrokeJoin> ((static_cast<int> (join) + 1) % 3);
            else if (!shift)
                ++downRepeat;
            else if (downRepeat > 0)
                --downRepeat;
            break;

        case GLFW_KEY_UP:
        {
            float oldScale = scale;
            scale *= 1.25;
            rive::float2 cursorPos = rive::float2 { (float)x, (float)y } * fiddleContext->dpiScale (nativeHandle());
            translate = cursorPos + (translate - cursorPos) * scale / oldScale;
            break;
        }

        case GLFW_KEY_DOWN:
        {
            float oldScale = scale;
            scale /= 1.25;
            rive::float2 cursorPos = rive::float2 { (float)x, (float)y } * fiddleContext->dpiScale (nativeHandle());
            translate = cursorPos + (translate - cursorPos) * scale / oldScale;
            break;
        }
        }
    }

    void updateWindowTitle(double fps, int instances, int width, int height)
    {
        juce::String title;

        if (fps != 0)
            title << "[" << fps << " FPS]";

        if (instances > 1)
            title << " (x" << instances << " instances)";

        title << " | " << (skia ? "Skia" : "Yup") << " Renderer";

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

            juce::MessageManager::callAsync([this] { juce::JUCEApplicationBase::getInstance()->systemRequestedQuit(); });
            return;
        }

        glfwPollEvents(); // TODO - remove

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
            printf ("size changed to %ix%i\n", width, height);

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
        else
        {
            rive::float2 p[9];
            for (int i = 0; i < 9; ++i)
                p[i] = pts[i] + translate;

            rive::RawPath rawPath;
            rawPath.moveTo (p[0].x, p[0].y);
            rawPath.cubicTo (p[1].x, p[1].y, p[2].x, p[2].y, p[3].x, p[3].y);
            rive::float2 c0 = rive::simd::mix (p[3], p[4], rive::float2 (2 / 3.f));
            rive::float2 c1 = rive::simd::mix (p[5], p[4], rive::float2 (2 / 3.f));
            rawPath.cubicTo (c0.x, c0.y, c1.x, c1.y, p[5].x, p[5].y);
            rawPath.cubicTo (p[6].x, p[6].y, p[7].x, p[7].y, p[8].x, p[8].y);
            if (doClose)
                rawPath.close();

            rive::Factory* factory = fiddleContext->factory();
            auto path = factory->makeRenderPath (rawPath, rive::FillRule::nonZero);

            auto fillPaint = factory->makeRenderPaint();
            fillPaint->style (rive::RenderPaintStyle::fill);
            fillPaint->color (-1);

            auto strokePaint = factory->makeRenderPaint();
            strokePaint->style (rive::RenderPaintStyle::stroke);
            strokePaint->color (0x8000ffff);
            strokePaint->thickness (strokeWidth);
            strokePaint->join (join);
            strokePaint->cap (cap);

            renderer->drawPath (path.get(), fillPaint.get());
            renderer->drawPath (path.get(), strokePaint.get());

            // Draw the interactive points.
            auto pointPaint = factory->makeRenderPaint();
            pointPaint->style (rive::RenderPaintStyle::stroke);
            pointPaint->color (0xff0000ff);
            pointPaint->thickness (14);
            pointPaint->cap (rive::StrokeCap::round);

            auto pointPath = factory->makeEmptyRenderPath();
            for (int i : { 1, 2, 4, 6, 7 })
            {
                rive::float2 pt = pts[i] + translate;
                pointPath->moveTo (pt.x, pt.y);
            }

            renderer->drawPath (pointPath.get(), pointPaint.get());
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

    FiddleContextOptions options;
    bool forceAtomicMode = false;
    bool wireframe = false;
    bool disableFill = false;
    bool disableStroke = false;
    float framerate = 30.0f;

    std::unique_ptr<FiddleContext> fiddleContext;

    rive::float2 pts[9] = {
        {260 + 2 * 100, 60 + 2 * 500},
        {260 + 2 * 257, 60 + 2 * 233},
        {260 + 2 * -100, 60 + 2 * 300},
        {260 + 2 * 100, 60 + 2 * 200},
        {260 + 2 * 250, 60 + 2 * 0},
        {260 + 2 * 400, 60 + 2 * 200},
        {260 + 2 * 213, 60 + 2 * 200},
        {260 + 2 * 213, 60 + 2 * 300},
        {260 + 2 * 391, 60 + 2 * 480}
    };

    static constexpr int kNumInteractivePts = sizeof(pts) / sizeof(*pts);

    float strokeWidth = 70;

    rive::float2 translate;
    float scale = 1;

    rive::StrokeJoin join = rive::StrokeJoin::miter;
    rive::StrokeCap cap = rive::StrokeCap::butt;

    bool doClose = false;
    bool paused = false;

    int dragIdx = -1;
    rive::float2 dragLastPos;

    int animation = -1;
    int stateMachine = -1;
    int horzRepeat = 0;
    int upRepeat = 0;
    int downRepeat = 0;

    rive::Mat2D viewTransform;

    std::unique_ptr<rive::File> rivFile;
    std::vector<std::unique_ptr<rive::Artboard>> artboards;
    std::vector<std::unique_ptr<rive::Scene>> scenes;

    bool skia = false;
    bool angle = false;

    API api =
    #if defined(JUCE_MAC) || defined JUCE_IOS
        API::metal
    #elif defined(JUCE_WINDOWS)
        API::d3d
    #else
        API::gl
    #endif
    ;

    std::unique_ptr<rive::Renderer> renderer;

    int lastWidth = 0, lastHeight = 0;
    double fpsLastTime = 0;
    int fpsFrames = 0;
    bool needsTitleUpdate = false;
};

//==============================================================================

struct Application : juce::JUCEApplicationBase
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

    bool moreThanOneInstanceAllowed() override
    {
        return false;
    }

    void initialise (const juce::String& commandLineParameters) override
    {
        DBG("Starting app " << commandLineParameters);

        window = std::make_unique<CustomWindow>();
        window->setSize (1280, 866);
        window->setVisible (true);
    }

    void shutdown() override
    {
        DBG("Shutting down");

        window.reset();
    }

    void anotherInstanceStarted (const juce::String& commandLine) override
    {
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void suspended() override
    {
    }

    void resumed() override
    {
    }

    void unhandledException (const std::exception*,
                             const juce::String& sourceFilename,
                             int lineNumber) override
    {
    }

private:
    std::unique_ptr<CustomWindow> window;
};

START_JUCE_APPLICATION(Application)
