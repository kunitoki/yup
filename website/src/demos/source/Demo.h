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

    //==============================================================================
    void mouseEnter (const yup::MouseEvent& event) override { trackPointer (event); }

    void mouseMove (const yup::MouseEvent& event) override { trackPointer (event); }

    void mouseDown (const yup::MouseEvent& event) override
    {
        trackPointer (event);
        dragging = true;
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

    /** How much bigger than its design size the demo's content is drawn, so it fills large
        screens and still fits on phones. Demos are designed for a 900 x 640 area. */
    float contentScale() const
    {
        return yup::jlimit (0.6f, 2.2f, yup::jmin (getWidth() / 900.0f, getHeight() / 640.0f));
    }

    /** A rectangle of the given design size, scaled by contentScale() and centered in the demo. */
    yup::Rectangle<float> centeredContent (float designWidth, float designHeight) const
    {
        const auto scale = contentScale();
        return getLocalBounds().withSizeKeepingCenter (designWidth * scale, designHeight * scale);
    }

    float time = 0.0f;
    std::optional<yup::Point<float>> pointer; // in demo coordinates

private:
    //==============================================================================
    /** While a widget is dragged the demo stops following the pointer, so the surface under it
        (a turning solid, a bending list) holds still until the drag ends. */
    void trackPointer (const yup::MouseEvent& event)
    {
        framesSinceExit = -1;

        if (! dragging)
            pointer = screenToLocal (event.getScreenPosition());
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
};
