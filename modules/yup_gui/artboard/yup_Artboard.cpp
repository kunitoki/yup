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

std::optional<rive::AABB> collectNodeWorldBounds (rive::Component* node)
{
    if (node == nullptr)
        return std::nullopt;

    rive::AABB result = rive::AABB::forExpansion();
    bool found = false;

    std::function<void (rive::Component*)> visit = [&] (rive::Component* component)
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
                visit (child);
    };

    visit (node);

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
    : Component (componentID)
{
    setFile (std::move (file));
}

Artboard::~Artboard()
{
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

void Artboard::setLayout (Layout newLayout)
{
    if (layout == newLayout)
        return;

    layout = newLayout;
    updateNodeBounds();
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
    updateNodeBounds();
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

void Artboard::setNodeBoundsListener (StringRef nodeName, NodeBoundsCallback callback)
{
    const String name (nodeName);

    if (callback)
    {
        nodeBoundsListeners.set (name, std::move (callback));

        // Cache the node's current state so the listener only fires on actual changes.
        if (artboard != nullptr)
            if (auto* node = artboard->find<rive::Component> (name.toStdString()))
            {
                lastNodeBounds.set (name, computeNodeBounds (node));
                lastNodeViewTransforms.set (name, computeNodeViewTransform (node));
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

    NodeAttachment attachment;
    attachment.component = component;
    attachment.options = options;
    attachedComponents.set (name, attachment);

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

    attachedComponents.remove (name);

    return true;
}

void Artboard::detachAllComponents()
{
    attachedComponents.clear();
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

    if (auto* cached = cachedNodeHandles.getPointer (name); cached != nullptr && (*cached)->isValid())
        return *cached;

    auto handle = ArtboardNode::Ptr (new ArtboardNode (*const_cast<Artboard*> (this), node));
    cachedNodeHandles.set (name, handle);
    return handle;
}

//==============================================================================

String Artboard::getViewModelName()
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

    // The instance must originate from the same Rive file as this artboard.
    if (artboardFile == nullptr || model->getArtboardFile() != artboardFile.get())
        return false;

    auto* riveInstance = static_cast<rive::ViewModelInstance*> (model->internalRiveInstance());
    if (riveInstance == nullptr)
        return false;

    // Bind through the state machine when one drives this artboard, so both the
    // scene and the artboard share the data context; otherwise bind the artboard.
    if (stateMachine != nullptr)
    {
        stateMachine->clearDataContext();

        riveInstance->ref();
        stateMachine->bindViewModelInstance (rive::rcp<rive::ViewModelInstance> (riveInstance));
    }
    else
    {
        artboard->unbind();

        riveInstance->ref();
        artboard->bindViewModelInstance (rive::rcp<rive::ViewModelInstance> (riveInstance));
    }

    boundViewModelInstance = model;

    scene->advanceAndApply (0.0f);
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
    if (scene == nullptr || ! event.isLeftButtonDown())
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
    // YUP_DBG (eventName << " (" << propertyName << ") = " << newValue.toString() << " (" << oldValue.toString() << ")");
}

//==============================================================================

void Artboard::updateSceneFromFile()
{
    scene.reset();
    artboard.reset();
    stateMachine = nullptr;

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

void Artboard::updateNodeBounds()
{
    if (artboard == nullptr)
        return;

    if (layout == Layout::layout || layout == Layout::fill)
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

    if (scene != nullptr)
        scene->advanceAndApply (0.0f);

    updateViewTransform();
    notifyNodeBoundsChanged();
}

//==============================================================================

void Artboard::notifyNodeBoundsChanged()
{
    if (nodeBoundsListeners.isEmpty() && attachedComponents.isEmpty())
        return;

    nodeNames.clear();

    for (auto it = nodeBoundsListeners.begin(); it != nodeBoundsListeners.end(); ++it)
        nodeNames.addIfNotAlreadyThere (it.getKey());

    for (auto it = attachedComponents.begin(); it != attachedComponents.end(); ++it)
        nodeNames.addIfNotAlreadyThere (it.getKey());

    for (const auto& nodeName : nodeNames)
        checkNodeBounds (nodeName);
}

//==============================================================================

void Artboard::checkNodeBounds (const String& nodeName)
{
    if (artboard == nullptr)
        return;

    auto* node = artboard->find<rive::Component> (nodeName.toStdString());
    if (node == nullptr)
        return;

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
            (*callback) (*this, nodeName, findNode (nodeName));
    }
}

//==============================================================================

void Artboard::applyNodeAttachment (const NodeAttachment& attachment, rive::Component* node, const Rectangle<float>& nodeBounds)
{
    auto* component = attachment.component;
    if (component == nullptr)
        return;

    Rectangle<float> bounds = nodeBounds;
    Point<float> rotationPivot = { bounds.getWidth() * 0.5f, bounds.getHeight() * 0.5f };

    if (attachment.options.mode == NodeAttachmentOptions::Mode::trackPosition)
    {
        const auto width = component->getWidth();
        const auto height = component->getHeight();

        // Offsets of a justification point within a rect of the given size.
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

        // Where on the node the component's pivot is anchored.
        const auto anchor = justificationOffsets (attachment.options.anchor,
                                                  nodeBounds.getWidth(),
                                                  nodeBounds.getHeight());

        // Which point of the component is the pivot placed on the anchor.
        const auto pivot = justificationOffsets (attachment.options.pivot,
                                                 width,
                                                 height);

        bounds = { nodeBounds.getX() + anchor.getX() - pivot.getX(),
                   nodeBounds.getY() + anchor.getY() - pivot.getY(),
                   width,
                   height };

        // Rotate around the anchored point (the pivot point within the component,
        // in its local coordinates), so the anchor does not drift while the
        // component follows the rotation.
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

    // Layout nodes report their laid-out size.
    if (node->is<rive::LayoutComponent>())
    {
        auto* layoutNode = node->as<rive::LayoutComponent>();
        const auto nodeRect = rive::AABB::fromLTWH (0.0f, 0.0f, layoutNode->layoutWidth(), layoutNode->layoutHeight());
        const auto mapped = (viewTransform * layoutNode->worldTransform()).mapBoundingBox (nodeRect);
        return { mapped.left(), mapped.top(), mapped.width(), mapped.height() };
    }

    // Other nodes report their real geometry (shapes, or anything under them).
    if (auto worldBounds = collectNodeWorldBounds (node))
    {
        const auto mapped = viewTransform.mapBoundingBox (*worldBounds);
        return { mapped.left(), mapped.top(), mapped.width(), mapped.height() };
    }

    // Fall back to a unit rect at the node's origin.
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
        toRiveFit (layout),
        toRiveAlignment (alignment),
        getLocalBounds().toAABB(),
        artboard->bounds());
}

} // namespace yup
