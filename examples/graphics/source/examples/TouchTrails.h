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

#include <cmath>
#include <vector>

//==============================================================================
/**
    Demonstrates multitouch input by drawing a colored trail for every finger
    that touches the screen.

    On any platform with touch hardware (mobile, Emscripten in a mobile
    browser, and desktop touchscreens) each finger is delivered through the
    regular mouse callbacks - mouseDown, mouseDrag and mouseUp with the left
    button held - and can be told apart with MouseEvent::isTouch() and
    MouseEvent::getTouchIndex(). Every contact starts a new trail in its own
    hue-spaced color. A mouse maps to a single contact, so the demo can also
    be exercised on the desktop.

    Trails do not depend on the wall clock: every point carries an age measured
    in timer ticks, and its size and opacity are derived from that age - the
    newest point is at full size and opacity while older points shrink and fade
    as they age - so a stroke always looks identical no matter when a frame
    happens to be rendered. This is what keeps it flicker-free on render-thread
    backends, where a fade sampled from the clock would advance unevenly if a
    frame is delivered late. The fade timer also runs while a pointer is down:
    points behind the fingertip dissolve after a fixed number of ticks, and if
    the finger is held still the whole trail fades away until only the point
    under the finger remains. Once the finger is lifted that point fades out as
    well, so a stroke always shrinks and fades to nothing instead of popping.
*/
class TouchTrailsDemo
    : public yup::Component
    , public yup::Timer
{
public:
    //==============================================================================
    TouchTrailsDemo()
        : yup::Component ("TouchTrailsDemo")
    {
        setOpaque (true);

        hintFont = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (13.0f);
        statusFont = hintFont.withHeight (11.0f);
    }

    //==============================================================================
    void paint (yup::Graphics& g) override
    {
        g.setFillColor (backgroundColor);
        g.fillAll();

        drawInfoText (g);

        for (const auto& contact : contacts)
            drawTrail (g, contact);
    }

    //==============================================================================
    void mouseDown (const yup::MouseEvent& event) override
    {
        if (! event.isLeftButtonDown())
            return;

        if (findActiveContactFor (event) != nullptr)
            return; // Safety net: ignore a spurious second down for the same pointer.

        TouchContact contact;
        contact.pointerKey = pointerKeyFor (event);
        contact.color = colorForPointer (contact.pointerKey);
        contact.strokeWidth = yup::jlimit (8.0f, 24.0f, 10.0f + event.getPressure() * 14.0f);
        contact.isDown = true;
        contact.points.push_back ({ event.getPosition() });

        contacts.push_back (std::move (contact));

        repaint();
    }

    void mouseDrag (const yup::MouseEvent& event) override
    {
        if (! event.isLeftButtonDown())
            return;

        if (auto* contact = findActiveContactFor (event))
        {
            appendPoint (*contact, event.getPosition());
            updateFadeTimer();
            repaint();
        }
    }

    void mouseUp (const yup::MouseEvent& event) override
    {
        if (auto* contact = findActiveContactFor (event))
        {
            appendPoint (*contact, event.getPosition());
            contact->isDown = false;

            updateFadeTimer();
            repaint();
        }
    }

    //==============================================================================
    void timerCallback() override
    {
        advanceTrails();
        updateFadeTimer();
        repaint();
    }

private:
    //==============================================================================
    /** The color of every stroke drawn by a single pointer, in ARGB. */
    static constexpr yup::Color backgroundColor = yup::Color (0xff14161c);

    /** How many trailing points a moving pointer may accumulate at most. */
    static constexpr int maxTrailPoints = 128;

    /** How many fade ticks a point needs to fade out completely. */
    static constexpr int maxAgeTicks = 120;

    /** Rate of the fade timer, in Hz. */
    static constexpr int fadeTimerHz = 60;

    //==============================================================================
    struct TrailPoint
    {
        yup::Point<float> position;
        int ageTicks = 0; // ticks spent fading since it stopped being the tip
    };

    struct TouchContact
    {
        int pointerKey = 0; // -1 for the mouse, the touch index otherwise
        yup::Color color;
        float strokeWidth = 10.0f;
        bool isDown = false;
        std::vector<TrailPoint> points;
    };

    //==============================================================================
    int pointerKeyFor (const yup::MouseEvent& event) const
    {
        return event.isTouch() ? event.getTouchIndex() : -1;
    }

    static yup::Color colorForPointer (int pointerKey)
    {
        const float hue = std::fmod (static_cast<float> (pointerKey + 1) * 0.61803398875f, 1.0f);
        return yup::Color::fromHSL (hue, 0.8f, 0.62f);
    }

    static float alphaForAge (int ageTicks)
    {
        if (ageTicks >= maxAgeTicks)
            return 0.0f;

        const float life = 1.0f - static_cast<float> (ageTicks) / static_cast<float> (maxAgeTicks);
        return life * life;
    }

    static void appendPoint (TouchContact& contact, const yup::Point<float>& position)
    {
        if (! contact.points.empty())
        {
            const auto delta = position - contact.points.back().position;
            if (delta.getX() * delta.getX() + delta.getY() * delta.getY() < 1.0f)
                return;
        }

        contact.points.push_back ({ position });

        while (contact.points.size() > static_cast<std::size_t> (maxTrailPoints))
            contact.points.erase (contact.points.begin());
    }

    TouchContact* findActiveContactFor (const yup::MouseEvent& event)
    {
        const auto key = pointerKeyFor (event);

        for (auto& contact : contacts)
        {
            if (contact.isDown && contact.pointerKey == key)
                return &contact;
        }

        return nullptr;
    }

    void advanceTrails()
    {
        for (auto& contact : contacts)
        {
            const auto numPoints = static_cast<int> (contact.points.size());

            for (int i = 0; i < numPoints; ++i)
            {
                if (contact.isDown && i == numPoints - 1)
                    continue; // the point under the finger is kept while pressed

                ++contact.points[static_cast<std::size_t> (i)].ageTicks;
            }

            while (contact.points.size() > (contact.isDown ? 1u : 0u)
                   && contact.points.front().ageTicks >= maxAgeTicks)
            {
                contact.points.erase (contact.points.begin());
            }
        }

        for (int i = static_cast<int> (contacts.size()) - 1; i >= 0; --i)
        {
            if (! contacts[static_cast<std::size_t> (i)].isDown
                && contacts[static_cast<std::size_t> (i)].points.empty())
            {
                contacts.erase (contacts.begin() + i);
            }
        }
    }

    void updateFadeTimer()
    {
        bool hasPointsToFade = false;
        for (const auto& contact : contacts)
        {
            const auto keepWhileDown = contact.isDown ? 1u : 0u;
            if (contact.points.size() > keepWhileDown)
            {
                hasPointsToFade = true;
                break;
            }
        }

        if (hasPointsToFade)
        {
            if (! isTimerRunning())
                startTimerHz (fadeTimerHz);
        }
        else if (isTimerRunning())
        {
            stopTimer();
        }
    }

    //==============================================================================
    void drawTrail (yup::Graphics& g, const TouchContact& contact) const
    {
        const auto& points = contact.points;
        if (points.empty())
            return;

        for (std::size_t i = 1; i < points.size(); ++i)
        {
            const float fade = alphaForAge (points[i].ageTicks);
            g.setStrokeWidth (contact.strokeWidth * fade);
            g.setStrokeColor (contact.color.withAlpha (fade));
            g.strokeLine (points[i - 1].position, points[i].position);
        }

        const float tipFade = alphaForAge (points.back().ageTicks);
        g.setFillColor (contact.color.withAlpha (tipFade));
        const float radius = contact.strokeWidth * 0.5f * tipFade;
        g.fillEllipse (points.back().position.getX() - radius,
                       points.back().position.getY() - radius,
                       radius * 2.0f,
                       radius * 2.0f);
    }

    void drawInfoText (yup::Graphics& g) const
    {
        auto area = getLocalBounds().reduced (12.0f, 10.0f);

        int activePointers = 0;
        for (const auto& contact : contacts)
        {
            if (contact.isDown)
                ++activePointers;
        }

        g.setFillColor (yup::Colors::white.withAlpha (0.6f));
        g.fillFittedText ("Touch and drag with one or more fingers - every finger draws in its own color.", hintFont, area.removeFromTop (18.0f), yup::Justification::left);

        g.setFillColor (yup::Colors::white.withAlpha (0.4f));
        const auto statusText = (activePointers > 0)
                                    ? "Active pointers: " + yup::String (activePointers)
                                    : yup::String ("Trails fade out after the finger is lifted.");
        g.fillFittedText (statusText, statusFont, area.removeFromTop (25.0f), yup::Justification::left);
    }

    //==============================================================================
    yup::Font hintFont;
    yup::Font statusFont;
    std::vector<TouchContact> contacts;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TouchTrailsDemo)
};
