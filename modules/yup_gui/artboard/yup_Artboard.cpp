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

Result Artboard::loadFromFile (const File& file, int defaultArtboardIndex, bool shouldUseStateMachines)
{
    if (! file.existsAsFile())
        return Result::fail ("Failed to find file to load");

    auto is = file.createInputStream();
    if (is == nullptr || ! is->openedOk())
        return Result::fail ("Failed to open file for reading");

    return loadFromStream (*is, defaultArtboardIndex, shouldUseStateMachines);
}

Result Artboard::loadFromStream (InputStream& is, int defaultArtboardIndex, bool shouldUseStateMachines)
{
    jassert (getNativeComponent() != nullptr); // Must be added to a NativeComponent !

    rive::Factory* factory = getNativeComponent()->getFactory();
    if (factory == nullptr)
        return Result::fail ("Failed to create a graphics context");

    yup::MemoryBlock mb;
    is.readIntoMemoryBlock (mb);

    rivFile = rive::File::import ({ static_cast<const uint8_t*> (mb.getData()), mb.getSize() }, factory);
    artboardIndex = jlimit (-1, static_cast<int> (rivFile->artboardCount()) - 1, defaultArtboardIndex);

    useStateMachines = shouldUseStateMachines;

    updateSceneFromFile();
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

void Artboard::advanceAndApply (float elapsedSeconds)
{
    if (scene == nullptr)
        return;

    scene->advanceAndApply (elapsedSeconds);
}

float Artboard::durationSeconds() const
{
    if (scene == nullptr)
        return 0.0f;

    return scene->durationSeconds();
}

//==============================================================================

void Artboard::setNumberInput (const String& name, double value)
{
    if (scene == nullptr)
        return;

    if (auto numberInput = scene->getNumber (name.toStdString()))
        numberInput->value (static_cast<float> (value));

    repaint();
}

//==============================================================================

void Artboard::paint (Graphics& g)
{
    if (rivFile == nullptr || scene == nullptr)
        return;

    if (! paused && getNativeComponent() != nullptr)
        scene->advanceAndApply (1.0f / getNativeComponent()->getDesiredFrameRate());

    auto* renderer = g.getRenderer();

    renderer->save();

    renderer->transform (viewTransform);

    scene->draw (renderer);

    renderer->restore();
}

//==============================================================================

void Artboard::resized()
{
    auto scaleDpi = getNativeComponent()->getScaleDpi();
    auto scaledBounds = getBounds() * scaleDpi;

    auto frameBounds = rive::AABB (
        scaledBounds.getX(),
        scaledBounds.getY(),
        scaledBounds.getX() + scaledBounds.getWidth(),
        scaledBounds.getY() + scaledBounds.getHeight());

    rive::AABB artboardBounds;
    if (artboard != nullptr)
        artboardBounds = artboard->bounds();

    viewTransform = rive::computeAlignment (
        rive::Fit::contain,
        rive::Alignment::center,
        frameBounds,
        artboardBounds);
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
    if (scene == nullptr || ! event.isLeftButtoDown())
        return;

    auto [x, y] = event.getPosition();

    auto xy = viewTransform.invertOrIdentity() * rive::Vec2D (x, y);
    scene->pointerDown (xy);

    repaint();
}

void Artboard::mouseUp (const MouseEvent& event)
{
    if (scene == nullptr)
        return;

    auto [x, y] = event.getPosition();

    auto xy = viewTransform.invertOrIdentity() * rive::Vec2D (x, y);
    scene->pointerUp (xy);

    repaint();
}

void Artboard::mouseMove (const MouseEvent& event)
{
    if (scene == nullptr)
        return;

    auto [x, y] = event.getPosition();

    const auto xy = viewTransform.invertOrIdentity() * rive::Vec2D (x, y);
    scene->pointerMove (xy);

    repaint();
}

void Artboard::mouseDrag (const MouseEvent& event)
{
    if (scene == nullptr || ! event.isLeftButtoDown())
        return;

    auto [x, y] = event.getPosition();

    const auto xy = viewTransform.invertOrIdentity() * rive::Vec2D (x, y);
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

void Artboard::updateSceneFromFile()
{
    jassert (rivFile != nullptr);

    artboard.reset();
    scene.reset();
    stateMachine = nullptr;
    eventProperties.clear();

    auto currentArtboard = (artboardIndex == -1)
                             ? rivFile->artboardDefault()
                             : rivFile->artboard (artboardIndex)->instance();

    std::unique_ptr<rive::Scene> currentScene;
    rive::StateMachineInstance* currentStateMachine = nullptr;

    if (useStateMachines)
    {
        if (yup::isPositiveAndBelow (stateMachineIndex, currentArtboard->stateMachineCount()))
        {
            auto machine = currentArtboard->stateMachineAt (stateMachineIndex);
            currentStateMachine = machine.get();
            currentScene = std::move (machine);
        }
        else if (currentArtboard->stateMachineCount() > 0)
        {
            auto machine = currentArtboard->defaultStateMachine();
            currentStateMachine = machine.get();
            currentScene = std::move (machine);
        }
    }
    else if (yup::isPositiveAndBelow (animationIndex, currentArtboard->animationCount()))
    {
        currentScene = currentArtboard->animationAt (animationIndex);
    }
    else if (currentArtboard->animationCount() > 0)
    {
        currentScene = currentArtboard->animationAt (0);
    }

    if (currentScene == nullptr)
        currentScene = std::make_unique<rive::StaticScene> (currentArtboard.get());

    currentScene->advanceAndApply (0.0f);

    artboard = std::move (currentArtboard);
    scene = std::move (currentScene);
    if (currentStateMachine)
        stateMachine = currentStateMachine;
}

//==============================================================================

void Artboard::pullEventsFromStateMachines()
{
    if (stateMachine == nullptr)
        return;

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

} // namespace yup
