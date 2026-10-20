/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>

//==============================================================================
/**
    Base of the website demos.

    Fills the whole area with the site field color, keeps the running time, and
    tracks the pointer over the demo, including over its widgets: it listens to the
    mouse events of every descendant, so the pointer stays known while the events go
    to the widget under it. The listeners are refreshed every frame to pick up
    children created later, such as list rows and popups.
*/
class Demo : public yup::Component
{
public:
    //==============================================================================
    static constexpr yup::Color fieldColor { 0xff06173a };
    static constexpr yup::Color blueColor { 0xff005aff };
    static constexpr yup::Color paleColor { 0xff9cc2ff };

    //==============================================================================
    explicit Demo (const yup::String& name)
        : yup::Component (name)
    {
        setOpaque (true);
    }

    //==============================================================================
    void refreshDisplay (double lastFrameTimeSeconds) override
    {
        if (! isVisible())
            return;

        listenToDescendants (*this);

        // Crossing from one widget to another (or into a gap of a 3D surface) sends an exit before
        // the next enter, sometimes a few events later: only forget the pointer if nothing follows.
        if (framesSinceExit >= 0 && ++framesSinceExit > exitGraceFrames)
        {
            pointer.reset();
            framesSinceExit = -1;
        }

        const auto deltaSeconds = static_cast<float> (yup::jlimit (0.0, 0.1, lastFrameTimeSeconds));
        time += deltaSeconds;

        listen (deltaSeconds);
        advance (deltaSeconds);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (fieldColor);
        g.fillAll();
    }

    /** Panels on 3D surfaces are cached textures, so a demo shown again repaints all of them,
        picking up palette changes made in another demo meanwhile. */
    void visibilityChanged() override
    {
        if (isVisible())
            repaintDescendants (*this);
    }

    //==============================================================================
    /** The widget that takes the keys when the demo is opened full screen, or nullptr. */
    virtual yup::Component* getFocusTarget() { return nullptr; }

    /** Gives the keyboard focus to getFocusTarget(), called when the page opens the demo. */
    void focusMainComponent()
    {
        if (auto* target = getFocusTarget())
        {
            target->setWantsKeyboardFocus (true);
            target->takeKeyboardFocus();
        }
    }

    /** Whether the page shows the demo open, full screen, which is when it makes sound. */
    void setOpen (bool shouldBeOpen) { open = shouldBeOpen; }

    bool isOpen() const { return open; }

    /** What the demo plays while it is open, or nullptr for a silent demo. */
    virtual SoundSource* getSoundSource() { return nullptr; }

    /** Where the demo hears its own sound from, to react to it. */
    void setAnalysis (const AudioAnalysis* newAnalysis) { analysis = newAnalysis; }

    //==============================================================================
    void mouseEnter (const yup::MouseEvent& event) override { trackPointer (event); }

    void mouseMove (const yup::MouseEvent& event) override { trackPointer (event); }

    void mouseDown (const yup::MouseEvent& event) override
    {
        trackPointer (event);
        dragging = true;
        pressed (screenToLocal (event.getScreenPosition()));
    }

    void mouseDrag (const yup::MouseEvent& event) override { trackPointer (event); }

    void mouseUp (const yup::MouseEvent& event) override
    {
        dragging = false;
        trackPointer (event);
    }

    void mouseExit (const yup::MouseEvent&) override
    {
        if (! dragging)
            framesSinceExit = 0;
    }

protected:
    //==============================================================================
    /** Steps the demo once per frame while it is visible. */
    virtual void advance (float /*deltaSeconds*/) {}

    /** Called when a button goes down anywhere on the demo, at that point. */
    virtual void pressed (yup::Point<float> /*position*/) {}

    /** True (the default) to stop following the pointer while a widget is dragged, so the surface
        under it (a turning solid, a bending list) holds still until the drag ends. */
    virtual bool holdsStillWhileDragging() const { return true; }

    /** How much bigger than its design size the demo's content is drawn, so it fills large
        screens and still fits on phones. Demos are designed for a 900 x 640 area. */
    float contentScale() const
    {
        return yup::jlimit (0.6f, 2.2f, yup::jmin (getWidth() / 900.0f, getHeight() / 640.0f));
    }

    /** A rectangle of the given design size, scaled by contentScale() and centered in the demo. */
    yup::Rectangle<float> centeredContent (float designWidth, float designHeight) const
    {
        return centeredContent (designWidth, designHeight, contentScale());
    }

    /** A rectangle of the given design size at the given scale, centered in the demo. */
    yup::Rectangle<float> centeredContent (float designWidth, float designHeight, float scale) const
    {
        return getLocalBounds().withSizeKeepingCenter (designWidth * scale, designHeight * scale);
    }

    /** Like contentScale() on large screens, but on small ones content of the given design size is
        not shrunk below its full size while that still fits the demo, and only as much as needed
        when it doesn't: phones keep it readable. */
    float readableScale (float designWidth, float designHeight) const
    {
        constexpr float margin = 16.0f;
        const auto fit = yup::jmin ((getWidth() - 2.0f * margin) / designWidth, (getHeight() - 2.0f * margin) / designHeight);

        return yup::jmax (0.1f, yup::jmin (fit, yup::jmax (contentScale(), 1.0f)));
    }

    /** What the demo hears of its sound, smoothed for the eye, all from 0 to 1 and all 0 while closed. */
    struct Hearing
    {
        float low = 0.0f, mid = 0.0f, high = 0.0f;
        float hit = 0.0f;        // jumps to 1 on every hit and dies away in a fraction of a second
        float lowAccent = 0.0f;  // how far the low level stands above its recent average: 0 for a steady
        float highAccent = 0.0f; // bass or treble, rising with every pulse or burst
        std::array<float, AudioAnalysis::numNotes> notes {}; // how loud each note from C3 up sounds
    };

    /** Moves a value toward a target, quickly on the way up and slowly on the way down, so what it
        drives eases in and lets go gently instead of flickering. */
    static float easeToward (float value, float target, float deltaSeconds, float riseSeconds, float fallSeconds)
    {
        const auto seconds = target > value ? riseSeconds : fallSeconds;
        return value + (target - value) * (1.0f - std::exp (-deltaSeconds / seconds));
    }

    /** True while a button is held down on the demo or one of its widgets. */
    bool isDragging() const { return dragging; }

    float time = 0.0f;
    std::optional<yup::Point<float>> pointer; // in demo coordinates
    Hearing hearing;

private:
    //==============================================================================
    void trackPointer (const yup::MouseEvent& event)
    {
        framesSinceExit = -1;

        if (! dragging || ! holdsStillWhileDragging())
            pointer = screenToLocal (event.getScreenPosition());
    }

    /** Follows the analysis: levels rise at once and fall gently, hits flash and fade. */
    void listen (float deltaSeconds)
    {
        const auto levels = open && analysis != nullptr ? analysis->read() : AudioAnalysis::Levels {};
        const auto fall = 1.0f - std::exp (-deltaSeconds / 0.15f);

        const auto follow = [fall] (float& value, float target)
        {
            value = target > value ? target : value + (target - value) * fall;
        };

        follow (hearing.low, levels.low);
        follow (hearing.mid, levels.mid);
        follow (hearing.high, levels.high);
        for (std::size_t i = 0; i < hearing.notes.size(); ++i)
            follow (hearing.notes[i], levels.notes[i]);

        // The hit count runs on while the demo is closed: only count hits heard since it opened.
        const auto slowFollow = 1.0f - std::exp (-deltaSeconds / 1.0f);
        averageLow += (hearing.low - averageLow) * slowFollow;
        averageHigh += (hearing.high - averageHigh) * slowFollow;
        hearing.lowAccent = std::clamp ((hearing.low - averageLow) * 4.0f, 0.0f, 1.0f);
        hearing.highAccent = std::clamp ((hearing.high - averageHigh) * 4.0f, 0.0f, 1.0f);

        hearing.hit *= std::exp (-deltaSeconds / 0.18f);
        if (open && listening && levels.hits != lastHits)
            hearing.hit = 1.0f;

        listening = open && analysis != nullptr;
        lastHits = levels.hits;
    }

    static void repaintDescendants (yup::Component& parent)
    {
        for (int i = 0; i < parent.getNumChildComponents(); ++i)
        {
            if (auto* child = parent.getChildComponent (i))
            {
                child->repaint();
                repaintDescendants (*child);
            }
        }
    }

    void listenToDescendants (yup::Component& parent)
    {
        for (int i = 0; i < parent.getNumChildComponents(); ++i)
        {
            if (auto* child = parent.getChildComponent (i))
            {
                child->addMouseListener (this);
                listenToDescendants (*child);
            }
        }
    }

    static constexpr int exitGraceFrames = 4;
    int framesSinceExit = -1;
    bool dragging = false;
    bool open = false;
    const AudioAnalysis* analysis = nullptr;
    std::uint32_t lastHits = 0;
    float averageLow = 0.0f, averageHigh = 0.0f;
    bool listening = false;
};
