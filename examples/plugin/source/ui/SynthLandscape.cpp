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

#include "SynthLandscape.h"
#include "SynthSettings.h"
#include "SynthTheme.h"

SynthLandscape::SynthLandscape (std::shared_ptr<SynthWaveformShader> sharedShader)
    : shader (std::move (sharedShader))
    , samples (static_cast<std::size_t> (SynthWaveformShader::sampleCount), 0.0f)
{
    look.scrolls = false;
}

bool SynthLandscape::isEnabled()
{
    return SynthSettings::get().landscapes;
}

void SynthLandscape::setProfile (float newWidth, float height, const std::function<float (float)>& yAtX)
{
    width = newWidth;

    const auto last = static_cast<float> (SynthWaveformShader::sampleCount - 1);

    for (int index = 0; index < SynthWaveformShader::sampleCount; ++index)
    {
        const auto sample = SynthWaveformShader::sampleForY (yAtX (width * static_cast<float> (index) / last), height);
        auto& slot = samples[static_cast<std::size_t> (index)];

        dirty = dirty || sample != slot;
        slot = sample;
    }
}

void SynthLandscape::setPlayhead (float x, float trailWidth)
{
    const auto playhead = x >= 0.0f && width > 0.0f ? x / width : -1.0f;
    const auto trail = width > 0.0f ? trailWidth / width : 0.0f;

    dirty = dirty || playhead != look.playhead || trail != look.trail;
    look.playhead = playhead;
    look.trail = trail;
}

bool SynthLandscape::advance (double lastFrameTimeSeconds)
{
    if (! animating || ! isEnabled())
    {
        sinceLastFrame = 0.0;
        return false;
    }

    sinceLastFrame += lastFrameTimeSeconds;

    if (sinceLastFrame < animationInterval)
        return false;

    time += static_cast<float> (sinceLastFrame);
    sinceLastFrame = 0.0;
    dirty = true;
    return true;
}

bool SynthLandscape::paint (yup::Graphics& g, const yup::Component& scope, yup::Rectangle<float> bounds, yup::Color accent)
{
    if (shader == nullptr || ! isEnabled())
        return false;

    const auto ground = SynthTheme::wellColor (scope);
    const auto isLight = ! SynthTheme::isDark (scope);
    const auto pixelWidth = yup::roundToInt (bounds.getWidth());
    const auto pixelHeight = yup::roundToInt (bounds.getHeight());
    const auto resized = target == nullptr || target->getWidth() != pixelWidth || target->getHeight() != pixelHeight;

    if (dirty || resized || cached == nullptr || accent != look.accent || ground != look.ground || isLight != look.isLight)
    {
        look.accent = accent;
        look.ground = ground;
        look.isLight = isLight;
        cached = shader->render (g.getGraphicsContext(), target, pixelWidth, pixelHeight, samples, time, look);
        dirty = false;
    }

    if (cached == nullptr)
        return false;

    {
        const auto state = g.saveState();

        // The clip follows the border's centerline so the texture's antialiased edge stays under it.
        yup::Path clip;
        clip.addRoundedRectangle (bounds.reduced (0.5f), SynthTheme::displayCorner);
        g.setClipPath (clip);
        g.drawTexture (cached, bounds);
    }

    SynthTheme::paintDisplayBorder (g, scope, bounds);
    return true;
}
