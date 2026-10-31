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

#include "SynthWaveformShader.h"

#include <functional>
#include <memory>
#include <vector>

//==============================================================================
/** The shader landscape behind a curve display, its ridge following the curve drawn.

    A display traces its curve through setProfile() in its own coordinates, and paints
    the landscape in place of its well; the ridge on the far wall sits under the curve, so
    the points and handles drawn on top line up with it. A playhead set with setPlayhead()
    lights the ridge as it runs. The landscape shimmers while animating, and is rendered
    again only when something it shows changed.

    It is drawn while the landscape displays setting is on and a GPU path is available;
    otherwise paint() leaves the display to draw itself.

    @see SynthWaveformShader
*/
class SynthLandscape
{
public:
    explicit SynthLandscape (std::shared_ptr<SynthWaveformShader> sharedShader);

    /** True while the landscape displays setting is on. */
    static bool isEnabled();

    /** Samples the curve across the display.

        @param width   The width of the display
        @param height  The height of the display
        @param yAtX    Returns the curve's vertical position at a horizontal one, both in the display's coordinates
    */
    void setProfile (float width, float height, const std::function<float (float)>& yAtX);

    /** Sets where the playhead lights the ridge, in the display's coordinates, or a negative
        position for none, with the trail reaching that far behind it. */
    void setPlayhead (float x, float trailWidth);

    /** Lets the landscape shimmer, while something plays. */
    void setAnimating (bool shouldAnimate) noexcept { animating = shouldAnimate; }

    /** Moves the animation on; call it from the display's refreshDisplay().

        @returns true when the display should repaint.
    */
    bool advance (double lastFrameTimeSeconds);

    /** Paints the landscape over the display's bounds, clipped and bordered like the waveform's.

        @param g       The graphics the display paints with
        @param scope   The display, for its theme
        @param bounds  The display's bounds
        @param accent  The color of the ridges
        @returns false when it painted nothing, and the display should paint its well.
    */
    bool paint (yup::Graphics& g, const yup::Component& scope, yup::Rectangle<float> bounds, yup::Color accent);

private:
    static constexpr double animationInterval = 1.0 / 30.0;

    std::shared_ptr<SynthWaveformShader> shader;
    yup::GpuTarget::Ptr target;
    yup::GpuTexture::Ptr cached;
    std::vector<float> samples;
    SynthWaveformShader::Look look;
    float width = 0.0f;
    float time = 0.0f;
    double sinceLastFrame = 0.0;
    bool animating = false;
    bool dirty = true;
};
