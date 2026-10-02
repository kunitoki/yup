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

namespace yup
{

//==============================================================================

SceneComponent::SceneComponent (StringRef componentID)
    : Component (componentID)
{
    setOpaque (true);

    // Clicking takes the focus, so the mouse wheel reaches the scene and not a previously focused component
    setWantsKeyboardFocus (true);
}

SceneComponent::~SceneComponent() = default;

//==============================================================================

void SceneComponent::setScene (Scene::Ptr newScene)
{
    scene = std::move (newScene);
    repaint();
}

//==============================================================================

void SceneComponent::paint (Graphics& g)
{
    const auto bounds = getLocalBounds();
    const auto background = scene != nullptr ? scene->getBackgroundColor() : Color (0xff000000);

    g.setFillColor (background);
    g.fillAll();

    if (scene == nullptr)
        return;

    auto& context = g.getGraphicsContext();

    String error;
    if (! context.isGpuAvailable())
    {
        error = "GPU rendering is not available";
    }
    else
    {
        const auto width = roundToInt (bounds.getWidth() * g.getContextScale());
        const auto height = roundToInt (bounds.getHeight() * g.getContextScale());
        if (width < 1 || height < 1)
            return;

        if (auto texture = renderer.render (context.getGpuDevice(), *scene, width, height))
        {
            g.drawTexture (texture, bounds);
            return;
        }

        error = renderer.getLastError();
    }

    g.setFillColor (background.contrasting());
    g.fillFittedText (error, ApplicationTheme::getGlobalTheme()->getDefaultFont(), bounds.reduced (10.0f));
}

void SceneComponent::refreshDisplay (double lastFrameTimeSeconds)
{
    if (! continuousUpdates || scene == nullptr)
        return;

    scene->update (lastFrameTimeSeconds);
    repaint();
}

} // namespace yup
