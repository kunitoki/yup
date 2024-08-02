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

namespace yup
{

//==============================================================================

Artboard::Artboard (StringRef componentID)
    : Component (componentID)
{
}

//==============================================================================

Result Artboard::loadFromFile (const juce::File& file, int defaultArtboardIndex, bool shouldUseStateMachines)
{
    jassert (getNativeComponent() != nullptr); // Must be added to a NativeComponent !

    rive::Factory* factory = getNativeComponent()->getFactory();
    if (factory == nullptr)
        return Result::fail ("Failed to create a graphics context");

    if (! file.existsAsFile())
        return Result::fail ("Failed to find file to load");

    auto is = file.createInputStream();
    if (is == nullptr || ! is->openedOk())
        return Result::fail ("Failed to open file for reading");

    yup::MemoryBlock mb;
    is->readIntoMemoryBlock (mb);

    rivFile = rive::File::import ({ static_cast<const uint8_t*> (mb.getData()), mb.getSize() }, factory);
    artboardIndex = jlimit (-1, static_cast<int> (rivFile->artboardCount()) - 1, defaultArtboardIndex);

    horzRepeat = 0;
    vertRepeat = 0;
    useStateMachines = shouldUseStateMachines;

    updateScenesFromFile (getNumInstances());
    repaint();

    return Result::ok();
}

//==============================================================================

bool Artboard::isPaused() const
{
    return paused;
}

void Artboard::setPaused (bool shouldPause)
{
    paused = shouldPause;

    repaint();
}

//==============================================================================

void Artboard::setBoolInput (const String& name, bool value)
{
    for (const auto& scene : scenes)
    {
        if (auto boolInput = scene->getBool (name.toStdString()))
            boolInput->value (value);
    }

    repaint();
}

void Artboard::setNumberInput (const String& name, double value)
{
    for (const auto& scene : scenes)
    {
        if (auto numberInput = scene->getNumber (name.toStdString()))
            numberInput->value (static_cast<float> (value));
    }

    repaint();
}

void Artboard::triggerInput (const String& name)
{
    for (const auto& scene : scenes)
    {
        if (auto triggerInput = scene->getTrigger (name.toStdString()))
            triggerInput->fire();
    }

    repaint();
}

//==============================================================================

void Artboard::addHorizontalRepeats (int repeatsToAdd)
{
    horzRepeat += repeatsToAdd;
    horzRepeat = jmax (0, horzRepeat);

    repaint();
}

void Artboard::addVerticalRepeats (int repeatsToAdd)
{
    vertRepeat += repeatsToAdd;
    vertRepeat = jmax (0, vertRepeat);

    repaint();
}

int Artboard::getNumInstances() const
{
    return (1 + horzRepeat * 2) * (1 + vertRepeat * 2);
}

//==============================================================================

void Artboard::multiplyScale (float factor)
{
    scale *= factor;

    repaint();
}

//==============================================================================

void Artboard::paint (Graphics& g)
{
    jassert (getNativeComponent() != nullptr); // Must be added to a NativeComponent !

    double frameRate = getNativeComponent()->getDesiredFrameRate();
    double time = Time::getMillisecondCounterHiRes() / 1000.0;

    if (rivFile == nullptr)
        return;

    auto bounds = getBounds().withPosition (g.getDrawingArea().getTopLeft());
    auto* renderer = g.getRenderer();

    const int instances = getNumInstances();
    if (artboards.size() != instances || scenes.size() != instances)
    {
        updateScenesFromFile (instances);
    }
    else if (! paused)
    {
        for (const auto& scene : scenes)
            scene->advanceAndApply (1.0f / frameRate);
    }

    if (scenes.empty() || artboards.empty())
        return;

    rive::Mat2D m = rive::computeAlignment (rive::Fit::contain,
                                            rive::Alignment::center,
                                            rive::AABB (bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight()),
                                            artboards.front()->bounds());

    auto x = (bounds.getWidth() - bounds.getWidth() * scale) * 0.5f;
    auto y = (bounds.getHeight() - bounds.getHeight() * scale) * 0.5f;

    m = rive::Mat2D (scale, 0, 0, scale, bounds.getX() + x, bounds.getY() + y) * m;
    viewTransform = m;

    renderer->transform (m);

    const float spacing = 200 / m.findMaxScale();

    auto scene = scenes.begin();
    for (int j = 0; j < vertRepeat + 1; ++j)
    {
        renderer->save();

        renderer->transform (rive::Mat2D::fromTranslate (-spacing * horzRepeat, (j - (vertRepeat / 2)) * spacing));

        for (int i = 0; i < horzRepeat * 2 + 1; ++i)
        {
            (*scene++)->draw (renderer);

            renderer->transform (rive::Mat2D::fromTranslate (spacing, 0));
        }

        renderer->restore();
    }
}

//==============================================================================

void Artboard::mouseEnter (const MouseEvent& event)
{
    repaint();
}

void Artboard::mouseExit (const MouseEvent& event)
{
    repaint();
}

void Artboard::mouseDown (const MouseEvent& event)
{
    if (scenes.empty() || ! event.isLeftButtoDown())
        return;

    auto [x, y] = event.getPosition();

    auto xy = viewTransform.invertOrIdentity() * rive::Vec2D (x, y);
    for (auto& scene : scenes)
        scene->pointerDown (xy);

    repaint();
}

void Artboard::mouseUp (const MouseEvent& event)
{
    if (scenes.empty())
        return;

    auto [x, y] = event.getPosition();

    auto xy = viewTransform.invertOrIdentity() * rive::Vec2D (x, y);
    for (auto& scene : scenes)
        scene->pointerUp (xy);

    repaint();
}

void Artboard::mouseMove (const MouseEvent& event)
{
    if (scenes.empty())
        return;

    auto [x, y] = event.getPosition();

    const auto xy = viewTransform.invertOrIdentity() * rive::Vec2D (x, y);
    for (auto& scene : scenes)
        scene->pointerMove (xy);

    repaint();
}

void Artboard::mouseDrag (const MouseEvent& event)
{
    if (scenes.empty() || ! event.isLeftButtoDown())
        return;

    auto [x, y] = event.getPosition();

    const auto xy = viewTransform.invertOrIdentity() * rive::Vec2D (x, y);
    for (auto& scene : scenes)
        scene->pointerMove (xy);

    pullEventsFromStateMachines();

    repaint();
}

//==============================================================================

void Artboard::propertyChanged (const String& eventName, const String& propertyName, const var& oldValue, const var& newValue)
{
    DBG (eventName << " (" << propertyName << ") = " << newValue.toString() << " (" << oldValue.toString() << ")");
}

//==============================================================================

void Artboard::updateScenesFromFile (std::size_t count)
{
    jassert (rivFile != nullptr);

    artboards.clear();
    scenes.clear();
    stateMachines.clear();

    eventProperties.clear();

    for (size_t i = 0; i < count; ++i)
    {
        auto currentArtboard = (artboardIndex == -1)
                                 ? rivFile->artboardDefault()
                                 : rivFile->artboard (artboardIndex)->instance();

        std::unique_ptr<rive::Scene> scene;
        rive::StateMachineInstance* stateMachine = nullptr;

        if (useStateMachines)
        {
            if (yup::isPositiveAndBelow (stateMachineIndex, currentArtboard->stateMachineCount()))
            {
                auto machine = currentArtboard->stateMachineAt (stateMachineIndex);
                stateMachine = machine.get();
                scene = std::move (machine);
            }
            else if (currentArtboard->stateMachineCount() > 0)
            {
                auto machine = currentArtboard->defaultStateMachine();
                stateMachine = machine.get();
                scene = std::move (machine);
            }
        }
        else if (yup::isPositiveAndBelow (animationIndex, currentArtboard->animationCount()))
        {
            scene = currentArtboard->animationAt (animationIndex);
        }
        else if (currentArtboard->animationCount() > 0)
        {
            scene = currentArtboard->animationAt (0);
        }

        if (scene == nullptr)
            scene = std::make_unique<rive::StaticScene> (currentArtboard.get());

        scene->advanceAndApply (scene->durationSeconds() * i / count);

        artboards.push_back (std::move (currentArtboard));
        scenes.push_back (std::move (scene));

        if (stateMachine)
            stateMachines.push_back (stateMachine);
    }
}

//==============================================================================

void Artboard::pullEventsFromStateMachines()
{
    for (const auto& stateMachine : stateMachines)
    {
        for (std::size_t eventIndex = 0; eventIndex < stateMachine->reportedEventCount(); ++eventIndex)
        {
            auto event = stateMachine->reportedEventAt (eventIndex).event();
            if (event == nullptr)
                continue;

            const auto eventName = String (event->name());

            for (const auto& child : event->children())
            {
                var newValue;
                if (child->is<rive::CustomPropertyNumber>())
                    newValue = child->as<rive::CustomPropertyNumber>()->propertyValue();

                else if (child->is<rive::CustomPropertyString>())
                    newValue = String (child->as<rive::CustomPropertyString>()->propertyValue());

                else if (child->is<rive::CustomPropertyBoolean>())
                    newValue = child->as<rive::CustomPropertyBoolean>()->propertyValue();

                else
                    continue;

                var oldValue = eventProperties[eventName];
                if (oldValue == newValue)
                    continue;

                eventProperties.set (eventName, newValue);
                propertyChanged (eventName, String (child->name()), oldValue, newValue);
            }
        }
    }
}

} // namespace yup
