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
namespace
{
rive::Fit toRiveFit (Artboard::Layout layout)
{
    switch (layout)
    {
        case Artboard::Layout::fill:
            return rive::Fit::fill;
        case Artboard::Layout::contain:
            return rive::Fit::contain;
        case Artboard::Layout::cover:
            return rive::Fit::cover;
        case Artboard::Layout::fitWidth:
            return rive::Fit::fitWidth;
        case Artboard::Layout::fitHeight:
            return rive::Fit::fitHeight;
        case Artboard::Layout::none:
            return rive::Fit::none;
        case Artboard::Layout::scaleDown:
            return rive::Fit::scaleDown;
        case Artboard::Layout::layout:
            return rive::Fit::layout;
    }

    return rive::Fit::contain;
}

rive::Alignment toRiveAlignment (Artboard::Alignment alignment)
{
    switch (alignment)
    {
        case Artboard::Alignment::topLeft:
            return rive::Alignment::topLeft;
        case Artboard::Alignment::topCenter:
            return rive::Alignment::topCenter;
        case Artboard::Alignment::topRight:
            return rive::Alignment::topRight;
        case Artboard::Alignment::centerLeft:
            return rive::Alignment::centerLeft;
        case Artboard::Alignment::center:
            return rive::Alignment::center;
        case Artboard::Alignment::centerRight:
            return rive::Alignment::centerRight;
        case Artboard::Alignment::bottomLeft:
            return rive::Alignment::bottomLeft;
        case Artboard::Alignment::bottomCenter:
            return rive::Alignment::bottomCenter;
        case Artboard::Alignment::bottomRight:
            return rive::Alignment::bottomRight;
    }

    return rive::Alignment::center;
}
} // namespace

//==============================================================================

Artboard::Artboard (StringRef componentID)
    : Component (componentID)
{
    setOpaque (true);
}

Artboard::Artboard (StringRef componentID, std::shared_ptr<ArtboardFile> file)
    : Component (componentID)
{
    setFile (std::move (file));
}

//==============================================================================

void Artboard::setFile (std::shared_ptr<ArtboardFile> file)
{
    clear();

    artboardFile = std::move (file);

    updateSceneFromFile();
}

//==============================================================================

void Artboard::clear()
{
    artboardFile.reset();

    artboard.reset();
    scene.reset();

    stateMachine = nullptr;

    eventProperties.clear();
    viewTransform = rive::Mat2D();
}

//==============================================================================

void Artboard::setLayout (Layout newLayout)
{
    if (layout == newLayout)
        return;

    layout = newLayout;
    updateViewTransform();
    repaint();
}

Artboard::Layout Artboard::getLayout() const
{
    return layout;
}

void Artboard::setAlignment (Alignment newAlignment)
{
    if (alignment == newAlignment)
        return;

    alignment = newAlignment;
    updateViewTransform();
    repaint();
}

Artboard::Alignment Artboard::getAlignment() const
{
    return alignment;
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

bool Artboard::isPausingWhenHidden() const
{
    return pauseWhenHidden;
}

void Artboard::shouldPauseWhenHidden (bool shouldPause)
{
    pauseWhenHidden = shouldPause;
}

//==============================================================================

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

bool Artboard::hasBoolInput (const String& name) const
{
    if (scene == nullptr)
        return false;

    return scene->getBool (name.toStdString()) != nullptr;
}

void Artboard::setBoolInput (const String& name, bool value)
{
    if (scene == nullptr)
        return;

    if (auto sceneInput = scene->getBool (name.toStdString()))
    {
        sceneInput->value (value);

        repaint();
    }
}

bool Artboard::hasNumberInput (const String& name) const
{
    if (scene == nullptr)
        return false;

    return scene->getNumber (name.toStdString()) != nullptr;
}

void Artboard::setNumberInput (const String& name, double value)
{
    if (scene == nullptr)
        return;

    if (auto sceneInput = scene->getNumber (name.toStdString()))
    {
        sceneInput->value (static_cast<float> (value));

        repaint();
    }
}

bool Artboard::hasTriggerInput (const String& name) const
{
    if (scene == nullptr)
        return false;

    return scene->getTrigger (name.toStdString()) != nullptr;
}

void Artboard::triggerInput (const String& name)
{
    if (scene == nullptr)
        return;

    if (auto sceneInput = scene->getTrigger (name.toStdString()))
    {
        sceneInput->fire();

        repaint();
    }
}

//==============================================================================

var Artboard::getAllInputs() const
{
    if (stateMachine == nullptr)
        return {};

    Array<var> stateMachineInputs;
    stateMachineInputs.ensureStorageAllocated (static_cast<int> (stateMachine->inputCount()));

    for (std::size_t inputIndex = 0; inputIndex < stateMachine->inputCount(); ++inputIndex)
    {
        auto inputObject = stateMachine->input (inputIndex);

        DynamicObject::Ptr object = new DynamicObject;
        object->setProperty ("id", String (inputObject->name()));

        if (auto number = dynamic_cast<rive::SMINumber*> (inputObject))
        {
            object->setProperty ("type", "number");
            object->setProperty ("value", number->value());
        }
        else if (auto boolean = dynamic_cast<rive::SMIBool*> (inputObject))
        {
            object->setProperty ("type", "boolean");
            object->setProperty ("value", boolean->value());
        }
        else if (auto trigger = dynamic_cast<rive::SMITrigger*> (inputObject))
        {
            object->setProperty ("type", "trigger");
        }

        stateMachineInputs.add (var (object.get()));
    }

    return stateMachineInputs;
}

void Artboard::setAllInputs (const var& value)
{
}

void Artboard::setInput (const String& inputName, const var& value)
{
    if (stateMachine == nullptr)
        return;

    for (std::size_t inputIndex = 0; inputIndex < stateMachine->inputCount(); ++inputIndex)
    {
        auto inputObject = stateMachine->input (inputIndex);

        if (StringRef (inputObject->name()) != inputName)
            continue;

        if (auto trigger = dynamic_cast<rive::SMITrigger*> (inputObject))
        {
            trigger->fire();
            break;
        }
        else if (auto boolean = dynamic_cast<rive::SMIBool*> (inputObject))
        {
            jassert (value.isBool());

            boolean->value (static_cast<bool> (value));
            break;
        }
        else if (auto number = dynamic_cast<rive::SMINumber*> (inputObject))
        {
            jassert (value.isDouble() || value.isInt() || value.isInt64());

            number->value (static_cast<float> (value));
            break;
        }
    }
}

//==============================================================================

void Artboard::refreshDisplay (double lastFrameTimeSeconds)
{
    if (paused || (pauseWhenHidden && ! isShowing()))
        return;

    advanceAndApply (static_cast<float> (lastFrameTimeSeconds));
}

//==============================================================================

void Artboard::paint (Graphics& g)
{
    if (scene == nullptr)
        return;

    auto* renderer = g.getRenderer();

    auto transform = g.getTransform()
                         .translated (g.getDrawingArea().getX(), g.getDrawingArea().getY())
                         .scaled (g.getContextScale());

    renderer->save();
    renderer->transform (transform.toMat2D());
    renderer->transform (viewTransform);

    scene->draw (renderer);
    renderer->restore();
}

//==============================================================================

void Artboard::resized()
{
    updateViewTransform();
}

//==============================================================================

void Artboard::contentScaleChanged (float dpiScale)
{
    resized();
}

//==============================================================================

void Artboard::mouseEnter (const MouseEvent& event)
{
    if (scene == nullptr)
        return;

    auto [x, y] = transformPoint (event.getPosition());
    scene->pointerMove (rive::Vec2D (x, y));

    repaint();
}

void Artboard::mouseExit (const MouseEvent& event)
{
    if (scene == nullptr)
        return;

    auto [x, y] = transformPoint (event.getPosition());
    scene->pointerExit (rive::Vec2D (x, y));

    repaint();
}

void Artboard::mouseDown (const MouseEvent& event)
{
    if (scene == nullptr || ! event.isLeftButtoDown())
        return;

    auto [x, y] = transformPoint (event.getPosition());
    scene->pointerDown (rive::Vec2D (x, y));

    repaint();
}

void Artboard::mouseUp (const MouseEvent& event)
{
    if (scene == nullptr)
        return;

    auto [x, y] = transformPoint (event.getPosition());
    scene->pointerUp (rive::Vec2D (x, y));

    repaint();
}

void Artboard::mouseMove (const MouseEvent& event)
{
    if (scene == nullptr)
        return;

    auto [x, y] = transformPoint (event.getPosition());
    scene->pointerMove (rive::Vec2D (x, y));

    repaint();
}

void Artboard::mouseDrag (const MouseEvent& event)
{
    if (scene == nullptr || ! event.isLeftButtoDown())
        return;

    auto [x, y] = transformPoint (event.getPosition());
    scene->pointerMove (rive::Vec2D (x, y));

    pullEventsFromStateMachines();

    repaint();
}

//==============================================================================

void Artboard::propertyChanged (const String& eventName, const String& propertyName, const var& oldValue, const var& newValue)
{
    // YUP_DBG (eventName << " (" << propertyName << ") = " << newValue.toString() << " (" << oldValue.toString() << ")");
}

//==============================================================================

void Artboard::updateSceneFromFile()
{
    artboard.reset();
    scene.reset();
    stateMachine = nullptr;

    auto rivFile = artboardFile->getRiveFile();
    if (rivFile == nullptr)
        return;

    auto currentArtboard = rivFile->artboardDefault();
    if (currentArtboard == nullptr)
        return;

    std::unique_ptr<rive::Scene> currentScene;
    rive::StateMachineInstance* currentStateMachine = nullptr;

    if (currentArtboard->stateMachineCount() > 0)
    {
        auto machine = currentArtboard->defaultStateMachine();
        currentStateMachine = machine.get();
        currentScene = std::move (machine);
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

    stateMachine = currentStateMachine;

    updateViewTransform();
    repaint();
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

            if (onPropertyChanged)
                onPropertyChanged (*this, eventName, String (child->name()), oldValue, newValue);
        }
    }
}

//==============================================================================

Point<float> Artboard::transformPoint (Point<float> point) const
{
    const auto xy = viewTransform.invertOrIdentity() * rive::Vec2D (point.getX(), point.getY());
    return { xy.x, xy.y };
}

//==============================================================================

void Artboard::updateViewTransform()
{
    if (artboard == nullptr)
    {
        viewTransform = rive::Mat2D();
        return;
    }

    viewTransform = rive::computeAlignment (
        toRiveFit (layout),
        toRiveAlignment (alignment),
        getLocalBounds().toAABB(),
        artboard->bounds());
}

} // namespace yup
