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

rive::Fit toRiveFit (std::optional<Fitting> fitting)
{
    // No fitting of our own means the artboard resolves its own layout.
    if (! fitting.has_value())
        return rive::Fit::layout;

    switch (*fitting)
    {
        case Fitting::fill:
            return rive::Fit::fill;
        case Fitting::scaleToFit:
            return rive::Fit::contain;
        case Fitting::scaleToFill:
            return rive::Fit::cover;
        case Fitting::fitWidth:
            return rive::Fit::fitWidth;
        case Fitting::fitHeight:
            return rive::Fit::fitHeight;
        case Fitting::none:
            return rive::Fit::none;
        case Fitting::centerInside:
            return rive::Fit::scaleDown;

        // Rive has no equivalent for these, so they degrade to a uniform fit.
        case Fitting::tile:
        case Fitting::centerCrop:
        case Fitting::stretchWidth:
        case Fitting::stretchHeight:
            return rive::Fit::contain;
    }

    return rive::Fit::contain;
}

rive::Alignment toRiveAlignment (Justification justification)
{
    const bool left = justification.testFlags (Justification::left);
    const bool right = ! left && justification.testFlags (Justification::right);

    const bool top = justification.testFlags (Justification::top);
    const bool bottom = ! top && justification.testFlags (Justification::bottom);

    if (top)
        return left ? rive::Alignment::topLeft : right ? rive::Alignment::topRight
                                                       : rive::Alignment::topCenter;

    if (bottom)
        return left ? rive::Alignment::bottomLeft : right ? rive::Alignment::bottomRight
                                                          : rive::Alignment::bottomCenter;

    return left ? rive::Alignment::centerLeft : right ? rive::Alignment::centerRight
                                                      : rive::Alignment::center;
}

constexpr float kNodeBoundsEpsilon = 0.01f;

bool areNodeBoundsEqual (const Rectangle<float>& a, const Rectangle<float>& b)
{
    const auto isClose = [] (float x, float y)
    {
        return x - y < kNodeBoundsEpsilon && y - x < kNodeBoundsEpsilon;
    };

    return isClose (a.getX(), b.getX())
        && isClose (a.getY(), b.getY())
        && isClose (a.getWidth(), b.getWidth())
        && isClose (a.getHeight(), b.getHeight());
}

void expandNodeWorldBounds (rive::Component* component, rive::AABB& result, bool& found)
{
    if (component == nullptr)
        return;

    if (component->is<rive::Shape>())
    {
        const auto bounds = component->as<rive::Shape>()->worldBounds();
        if (! bounds.isEmptyOrNaN())
        {
            if (! found)
                result = bounds;
            else
                result.expand (bounds);

            found = true;
        }
    }

    if (component->is<rive::ContainerComponent>())
        for (auto* child : component->as<rive::ContainerComponent>()->children())
            expandNodeWorldBounds (child, result, found);
}

std::optional<rive::AABB> collectNodeWorldBounds (rive::Component* node)
{
    if (node == nullptr)
        return std::nullopt;

    rive::AABB result = rive::AABB::forExpansion();
    bool found = false;

    expandNodeWorldBounds (node, result, found);

    return found ? std::optional<rive::AABB> (result) : std::nullopt;
}
} // namespace

//==============================================================================

Artboard::Artboard (StringRef componentID)
    : Component (componentID)
{
    setOpaque (true);
}

Artboard::Artboard (StringRef componentID, std::shared_ptr<ArtboardFile> file)
    : Artboard (componentID)
{
    setFile (std::move (file));
}

Artboard::~Artboard()
{
    detachAllComponents();
}

//==============================================================================

void Artboard::setFile (std::shared_ptr<ArtboardFile> file, StringRef artboardName)
{
    clear();

    artboardFile = std::move (file);
    selectedArtboardName = artboardName;

    updateSceneFromFile();
}

//==============================================================================

void Artboard::clear()
{
    ++nodeEpoch;

    stateMachine = nullptr;
    boundViewModelInstance = nullptr;

    eventProperties.clear();
    viewTransform = rive::Mat2D();
    selectedArtboardName.clear();

    lastNodeBounds.clear();
    lastNodeViewTransforms.clear();
    cachedNodeHandles.clear();

    artboardFile.reset();

    scene.reset();
    artboard.reset();
}

//==============================================================================

void Artboard::setFitting (std::optional<Fitting> newFitting)
{
    if (fitting == newFitting)
        return;

    fitting = newFitting;
    updateNodeBounds();
    repaint();
}

std::optional<Fitting> Artboard::getFitting() const
{
    return fitting;
}

void Artboard::setJustification (Justification newJustification)
{
    if (justification == newJustification)
        return;

    justification = newJustification;
    updateNodeBounds();
    repaint();
}

Justification Artboard::getJustification() const
{
    return justification;
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
    advanceScene (elapsedSeconds);

    notifyNodeBoundsChanged();
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
        else if (dynamic_cast<rive::SMITrigger*> (inputObject) != nullptr)
        {
            object->setProperty ("type", "trigger");
        }

        stateMachineInputs.add (var (object.get()));
    }

    return stateMachineInputs;
}

void Artboard::setAllInputs (const var& value)
{
    if (stateMachine == nullptr)
        return;

    auto* inputs = value.getArray();
    if (inputs == nullptr)
        return;

    for (const auto& input : *inputs)
    {
        auto* object = input.getDynamicObject();
        if (object == nullptr)
            continue;

        const auto inputValue = object->getProperty ("value");
        if (inputValue.isVoid())
            continue;

        setInput (object->getProperty ("id").toString(), inputValue);
    }
}

void Artboard::setInput (const String& name, const var& value)
{
    if (stateMachine == nullptr)
        return;

    for (std::size_t inputIndex = 0; inputIndex < stateMachine->inputCount(); ++inputIndex)
    {
        auto inputObject = stateMachine->input (inputIndex);

        if (StringRef (inputObject->name()) != name)
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

void Artboard::setNodeBoundsListener (StringRef nodeName, NodeBoundsCallback callback)
{
    const String name (nodeName);

    if (callback)
    {
        nodeBoundsListeners.set (name, std::move (callback));

        if (artboard != nullptr)
        {
            if (auto* node = artboard->find<rive::Component> (name.toStdString()))
            {
                lastNodeBounds.set (name, computeNodeBounds (node));
                lastNodeViewTransforms.set (name, computeNodeViewTransform (node));
            }
        }
    }
    else
    {
        nodeBoundsListeners.remove (name);
    }
}

void Artboard::clearNodeBoundsListener (StringRef nodeName)
{
    nodeBoundsListeners.remove (String (nodeName));
}

void Artboard::clearAllNodeBoundsListeners()
{
    nodeBoundsListeners.clear();
}

Rectangle<float> Artboard::getNodeBounds (StringRef nodeName) const
{
    if (artboard == nullptr)
        return {};

    auto* node = artboard->find<rive::Component> (String (nodeName).toStdString());
    if (node == nullptr)
        return {};

    return computeNodeBounds (node);
}

//==============================================================================

bool Artboard::attachComponentToNode (StringRef nodeName, Component* component, NodeAttachmentOptions options)
{
    if (component == nullptr || artboard == nullptr)
        return false;

    const String name (nodeName);

    auto* node = artboard->find<rive::Component> (name.toStdString());
    if (node == nullptr)
        return false;

    detachComponentEverywhere (component);

    NodeAttachment attachment;
    attachment.component = component;
    attachment.options = options;
    attachedComponents.set (name, attachment);

    component->addComponentListener (this);

    const auto bounds = computeNodeBounds (node);
    applyNodeAttachment (attachment, node, bounds);
    lastNodeBounds.set (name, bounds);
    lastNodeViewTransforms.set (name, computeNodeViewTransform (node));

    return true;
}

bool Artboard::detachComponentFromNode (StringRef nodeName, Component* component)
{
    const String name (nodeName);

    auto* existing = attachedComponents.getPointer (name);
    if (existing == nullptr || existing->component != component)
        return false;

    component->removeComponentListener (this);
    attachedComponents.remove (name);

    return true;
}

void Artboard::detachAllComponents()
{
    for (auto it = attachedComponents.begin(); it != attachedComponents.end(); ++it)
        if (auto* component = it.getValue().component)
            component->removeComponentListener (this);

    attachedComponents.clear();
}

void Artboard::detachComponentEverywhere (Component* component)
{
    if (component == nullptr)
        return;

    String attachedNodeName;
    bool found = false;

    for (auto it = attachedComponents.begin(); it != attachedComponents.end(); ++it)
    {
        if (it.getValue().component == component)
        {
            attachedNodeName = it.getKey();
            found = true;
            break;
        }
    }

    if (! found)
        return;

    component->removeComponentListener (this);
    attachedComponents.remove (attachedNodeName);
}

void Artboard::componentBeingDeleted (Component& component)
{
    detachComponentEverywhere (std::addressof (component));
}

void Artboard::componentResized (Component& component)
{
    if (applyingNodeAttachment)
        return;

    reapplyNodeAttachment (std::addressof (component));
}

void Artboard::reapplyNodeAttachment (Component* component)
{
    if (component == nullptr || artboard == nullptr)
        return;

    for (auto it = attachedComponents.begin(); it != attachedComponents.end(); ++it)
    {
        if (it.getValue().component != component)
            continue;

        if (auto* node = artboard->find<rive::Component> (it.getKey().toStdString()))
            applyNodeAttachment (it.getValue(), node, computeNodeBounds (node));

        return;
    }
}

//==============================================================================

ArtboardNode::Ptr Artboard::findNode (StringRef nodeName) const
{
    if (artboard == nullptr)
        return nullptr;

    const String name (nodeName);

    auto* node = artboard->find<rive::Component> (name.toStdString());
    if (node == nullptr)
        return nullptr;

    return nodeHandleFor (name, node);
}

ArtboardNode::Ptr Artboard::nodeHandleFor (const String& nodeName, rive::Component* node) const
{
    if (auto* cached = cachedNodeHandles.getPointer (nodeName); cached != nullptr && (*cached)->isValid())
        return *cached;

    auto handle = ArtboardNode::Ptr (new ArtboardNode (*const_cast<Artboard*> (this), node));
    cachedNodeHandles.set (nodeName, handle);
    return handle;
}

//==============================================================================

String Artboard::getViewModelName() const
{
    if (artboard == nullptr || artboardFile == nullptr)
        return {};

    auto* rivFile = artboardFile->getRiveFile();
    if (rivFile == nullptr)
        return {};

    const auto viewModelIndex = artboard->viewModelId();
    if (viewModelIndex >= rivFile->viewModelCount())
        return {};

    if (auto* viewModel = rivFile->viewModel (viewModelIndex))
        return String (viewModel->name());

    return {};
}

bool Artboard::bindViewModelInstance (const ArtboardViewModelInstance::Ptr& model)
{
    if (model == nullptr || artboard == nullptr || scene == nullptr)
        return false;

    if (artboardFile == nullptr || model->getArtboardFile() != artboardFile.get())
        return false;

    auto* riveInstance = model->internalRiveInstance();
    if (riveInstance == nullptr)
        return false;

    riveInstance->ref();

    if (stateMachine != nullptr)
    {
        stateMachine->bindViewModelInstance (rive::rcp<rive::ViewModelInstance> (riveInstance));
    }
    else
    {
        artboard->unbind();
        artboard->bindViewModelInstance (rive::rcp<rive::ViewModelInstance> (riveInstance));
    }

    boundViewModelInstance = model;

    advanceScene (0.0f);
    repaint();

    return true;
}

void Artboard::unbindViewModelInstance()
{
    if (stateMachine != nullptr)
        stateMachine->clearDataContext();

    if (artboard != nullptr)
        artboard->unbind();

    boundViewModelInstance = nullptr;

    repaint();
}

ArtboardViewModelInstance::Ptr Artboard::getBoundViewModelInstance() const noexcept
{
    return boundViewModelInstance;
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
    updateNodeBounds();
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

    pullEventsFromStateMachines();

    repaint();
}

void Artboard::mouseExit (const MouseEvent& event)
{
    if (scene == nullptr)
        return;

    auto [x, y] = transformPoint (event.getPosition());
    scene->pointerExit (rive::Vec2D (x, y));

    pullEventsFromStateMachines();

    repaint();
}

void Artboard::mouseDown (const MouseEvent& event)
{
    if (scene == nullptr || ! event.isLeftButtonDown())
        return;

    auto [x, y] = transformPoint (event.getPosition());
    scene->pointerDown (rive::Vec2D (x, y));

    pullEventsFromStateMachines();

    repaint();
}

void Artboard::mouseUp (const MouseEvent& event)
{
    if (scene == nullptr)
        return;

    auto [x, y] = transformPoint (event.getPosition());
    scene->pointerUp (rive::Vec2D (x, y));

    pullEventsFromStateMachines();

    repaint();
}

void Artboard::mouseMove (const MouseEvent& event)
{
    if (scene == nullptr)
        return;

    auto [x, y] = transformPoint (event.getPosition());
    scene->pointerMove (rive::Vec2D (x, y));

    pullEventsFromStateMachines();

    repaint();
}

void Artboard::mouseDrag (const MouseEvent& event)
{
    if (scene == nullptr || ! event.isLeftButtonDown())
        return;

    auto [x, y] = transformPoint (event.getPosition());
    scene->pointerMove (rive::Vec2D (x, y));

    pullEventsFromStateMachines();

    repaint();
}

//==============================================================================

void Artboard::propertyChanged (const String& eventName, const String& propertyName, const var& oldValue, const var& newValue)
{
    ignoreUnused (eventName, propertyName, oldValue, newValue);
}

//==============================================================================

void Artboard::updateSceneFromFile()
{
    scene.reset();
    artboard.reset();
    stateMachine = nullptr;

    if (artboardFile == nullptr)
        return;

    auto rivFile = artboardFile->getRiveFile();
    if (rivFile == nullptr)
        return;

    auto currentArtboard = selectedArtboardName.isEmpty()
                               ? rivFile->artboardDefault()
                               : rivFile->artboardNamed (selectedArtboardName.toStdString());
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

    updateNodeBounds();
    repaint();
}

//==============================================================================

void Artboard::advanceScene (float elapsedSeconds)
{
    if (scene == nullptr)
        return;

    scene->advanceAndApply (elapsedSeconds);

    pullEventsFromStateMachines();
}

//==============================================================================

void Artboard::pullEventsFromStateMachines()
{
    if (stateMachine == nullptr || drainingEvents)
        return;

    const auto reportedCount = stateMachine->reportedEventCount();
    if (reportedCount == 0)
        return;

    const ScopedValueSetter<bool> draining (drainingEvents, true);

    Array<rive::Event*> reportedEvents;
    reportedEvents.ensureStorageAllocated (static_cast<int> (reportedCount));

    for (std::size_t eventIndex = 0; eventIndex < reportedCount; ++eventIndex)
        if (auto* event = stateMachine->reportedEventAt (eventIndex).event())
            reportedEvents.add (event);

    for (auto* event : reportedEvents)
    {
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

void Artboard::updateNodeBounds()
{
    if (artboard == nullptr)
        return;

    if (! fitting.has_value() || *fitting == Fitting::fill)
    {
        const auto bounds = getLocalBounds();

        artboard->width (bounds.getWidth());
        artboard->height (bounds.getHeight());
    }
    else
    {
        if (artboard->originalWidth() > 0.0f && artboard->originalHeight() > 0.0f)
            artboard->resetSize();
    }

    advanceScene (0.0f);

    updateViewTransform();
    notifyNodeBoundsChanged();
}

//==============================================================================

void Artboard::notifyNodeBoundsChanged()
{
    if (artboard == nullptr || (nodeBoundsListeners.isEmpty() && attachedComponents.isEmpty()) || notifyingNodeBounds)
        return;

    const ScopedValueSetter<bool> notifying (notifyingNodeBounds, true);

    nodeNames.clear();

    for (auto it = nodeBoundsListeners.begin(); it != nodeBoundsListeners.end(); ++it)
        nodeNames.addIfNotAlreadyThere (it.getKey());

    for (auto it = attachedComponents.begin(); it != attachedComponents.end(); ++it)
        nodeNames.addIfNotAlreadyThere (it.getKey());

    for (const auto& nodeName : nodeNames)
    {
        if (artboard == nullptr)
            break;

        if (auto* node = artboard->find<rive::Component> (nodeName.toStdString()))
            checkNodeBounds (nodeName, node);
    }
}

//==============================================================================

void Artboard::checkNodeBounds (const String& nodeName, rive::Component* node)
{
    const auto newBounds = computeNodeBounds (node);
    const auto newViewTransform = computeNodeViewTransform (node);

    const auto* cachedBounds = lastNodeBounds.getPointer (nodeName);
    const auto* cachedViewTransform = lastNodeViewTransforms.getPointer (nodeName);

    const bool boundsChanged = cachedBounds == nullptr || ! areNodeBoundsEqual (*cachedBounds, newBounds);
    const bool viewTransformChanged = cachedViewTransform == nullptr || ! newViewTransform.approximatelyEqualTo (*cachedViewTransform);

    if (! boundsChanged && ! viewTransformChanged)
        return;

    lastNodeBounds.set (nodeName, newBounds);
    lastNodeViewTransforms.set (nodeName, newViewTransform);

    if (auto* attachment = attachedComponents.getPointer (nodeName))
    {
        const bool followsTransform = attachment->options.applyTransform;
        if (boundsChanged || (followsTransform && viewTransformChanged))
            applyNodeAttachment (*attachment, node, newBounds);
    }

    if (auto* callback = nodeBoundsListeners.getPointer (nodeName))
    {
        if (*callback)
            (*callback) (*this, nodeName, nodeHandleFor (nodeName, node));
    }
}

//==============================================================================

void Artboard::applyNodeAttachment (NodeAttachment attachment, rive::Component* node, const Rectangle<float>& nodeBounds)
{
    auto* component = attachment.component;
    if (component == nullptr)
        return;

    const ScopedValueSetter<bool> applying (applyingNodeAttachment, true);

    Rectangle<float> bounds = nodeBounds;
    Point<float> rotationPivot = { bounds.getWidth() * 0.5f, bounds.getHeight() * 0.5f };

    if (attachment.options.mode == NodeAttachmentOptions::Mode::trackPosition)
    {
        const auto width = component->getWidth();
        const auto height = component->getHeight();

        const auto justificationOffsets = [] (Justification justification, float w, float h) -> Point<float>
        {
            const auto offsetX = justification.testFlags (Justification::horizontalCenter)
                ? w * 0.5f
                : justification.testFlags (Justification::right)
                    ? w
                    : 0.0f;

            const auto offsetY = justification.testFlags (Justification::verticalCenter)
                ? h * 0.5f
                : justification.testFlags (Justification::bottom)
                    ? h
                    : 0.0f;

            return { offsetX, offsetY };
        };

        const auto anchor = justificationOffsets (attachment.options.anchor,
                                                  nodeBounds.getWidth(),
                                                  nodeBounds.getHeight());

        const auto pivot = justificationOffsets (attachment.options.pivot,
                                                 width,
                                                 height);

        bounds = { nodeBounds.getX() + anchor.getX() - pivot.getX(),
                   nodeBounds.getY() + anchor.getY() - pivot.getY(),
                   width,
                   height };

        rotationPivot = pivot;
    }

    component->setBounds (bounds);

    if (attachment.options.applyTransform)
    {
        component->setTransform (AffineTransform::rotation (computeNodeRotation (node),
                                                           rotationPivot.getX(),
                                                           rotationPivot.getY()));
    }
    else
    {
        component->setTransform (AffineTransform());
    }
}

//==============================================================================

float Artboard::computeNodeRotation (rive::Component* node) const
{
    if (node != nullptr && node->is<rive::TransformComponent>())
        return (viewTransform * node->as<rive::TransformComponent>()->worldTransform()).decompose().rotation();

    return 0.0f;
}

AffineTransform Artboard::computeNodeViewTransform (rive::Component* node) const
{
    if (node != nullptr && node->is<rive::TransformComponent>())
        return AffineTransform (viewTransform * node->as<rive::TransformComponent>()->worldTransform());

    return {};
}

//==============================================================================

Rectangle<float> Artboard::computeNodeBounds (rive::Component* node) const
{
    if (node == nullptr)
        return {};

    if (node->is<rive::LayoutComponent>())
    {
        auto* layoutNode = node->as<rive::LayoutComponent>();
        const auto nodeRect = rive::AABB::fromLTWH (0.0f, 0.0f, layoutNode->layoutWidth(), layoutNode->layoutHeight());
        const auto mapped = (viewTransform * layoutNode->worldTransform()).mapBoundingBox (nodeRect);
        return { mapped.left(), mapped.top(), mapped.width(), mapped.height() };
    }

    if (auto worldBounds = collectNodeWorldBounds (node))
    {
        const auto mapped = viewTransform.mapBoundingBox (*worldBounds);
        return { mapped.left(), mapped.top(), mapped.width(), mapped.height() };
    }

    const auto nodeRect = rive::AABB::fromLTWH (0.0f, 0.0f, 1.0f, 1.0f);

    rive::Mat2D transform = viewTransform;
    if (node->is<rive::TransformComponent>())
        transform = viewTransform * node->as<rive::TransformComponent>()->worldTransform();

    const auto mapped = transform.mapBoundingBox (nodeRect);

    return { mapped.left(), mapped.top(), mapped.width(), mapped.height() };
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
        toRiveFit (fitting),
        toRiveAlignment (justification),
        getLocalBounds().toAABB(),
        artboard->bounds());
}

} // namespace yup
