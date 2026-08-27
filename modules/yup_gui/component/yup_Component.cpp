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

Component::Component()
    : optionsValue (0)
{
}

Component::Component (StringRef componentID)
    : componentID (componentID)
    , optionsValue (0)
{
}

Component::~Component()
{
    componentListeners.call (&ComponentListener::componentBeingDeleted, *this);

    if (options.onDesktop)
        removeFromDesktop();

    if (parentComponent != nullptr)
        parentComponent->removeChildComponent (this);

    for (auto component : children)
        component->parentComponent = nullptr;

    children.clear();

    masterReference.clear();
}

//==============================================================================

String Component::getComponentID() const
{
    return componentID;
}

//==============================================================================

bool Component::isEnabled() const
{
    return ! options.isDisabled && (parentComponent == nullptr || parentComponent->isEnabled());
}

void Component::setEnabled (bool shouldBeEnabled)
{
    if (options.isDisabled == ! shouldBeEnabled)
        return;

    options.isDisabled = ! shouldBeEnabled;

    //if (options.onDesktop && native != nullptr)
    //    native->setEnabled (shouldBeEnabled);

    enablementChanged();
}

void Component::enablementChanged() {}

//==============================================================================

bool Component::isVisible() const
{
    return options.isVisible;
}

void Component::setVisible (bool shouldBeVisible)
{
    if (options.isVisible == shouldBeVisible)
        return;

    const bool wasShowing = isShowing();

    options.isVisible = shouldBeVisible;

    auto bailOutChecker = BailOutChecker (this);

    if (options.onDesktop && native != nullptr)
        native->setVisible (shouldBeVisible);

    if (bailOutChecker.shouldBailOut())
        return;

    if (wasShowing != isShowing())
        internalVisibilityChanged();

    if (bailOutChecker.shouldBailOut())
        return;

    internalRepaint();
}

bool Component::isShowing() const
{
    if (! isVisible())
        return false;

    auto parent = getParentComponent();
    while (parent != nullptr)
    {
        if (! parent->isVisible())
            return false;

        parent = parent->getParentComponent();
    }

    return true;
}

void Component::visibilityChanged() {}

//==============================================================================

String Component::getTitle() const
{
    return componentTitle;
}

void Component::setTitle (const String& title)
{
    componentTitle = title;

    if (options.onDesktop && native != nullptr)
        native->setTitle (title);
}

//==============================================================================

Point<float> Component::getPosition() const
{
    return boundsInParent.getTopLeft();
}

void Component::setPosition (const Point<float>& newPosition)
{
    boundsInParent.setTopLeft (newPosition);

    if (options.onDesktop && native != nullptr)
        native->setPosition (newPosition.to<int>());

    sendMoved();
}

float Component::getX() const
{
    return boundsInParent.getX();
}

float Component::getY() const
{
    return boundsInParent.getY();
}

float Component::getLeft() const
{
    return boundsInParent.getX();
}

float Component::getTop() const
{
    return boundsInParent.getY();
}

float Component::getRight() const
{
    return boundsInParent.getX() + boundsInParent.getWidth();
}

float Component::getBottom() const
{
    return boundsInParent.getY() + boundsInParent.getHeight();
}

Point<float> Component::getTopLeft() const
{
    return boundsInParent.getTopLeft();
}

void Component::setTopLeft (const Point<float>& newTopLeft)
{
    boundsInParent.setTopLeft (newTopLeft);

    if (options.onDesktop && native != nullptr)
        native->setPosition (newTopLeft.to<int>());

    sendMoved();
}

Point<float> Component::getBottomLeft() const
{
    return boundsInParent.getBottomLeft();
}

void Component::setBottomLeft (const Point<float>& newBottomLeft)
{
    boundsInParent.setBottomLeft (newBottomLeft);

    if (options.onDesktop && native != nullptr)
        native->setPosition (newBottomLeft.translated (0.0f, -getHeight()).to<int>());

    sendMoved();
}

Point<float> Component::getTopRight() const
{
    return boundsInParent.getTopRight();
}

void Component::setTopRight (const Point<float>& newTopRight)
{
    boundsInParent.setTopRight (newTopRight);

    if (options.onDesktop && native != nullptr)
        native->setPosition (newTopRight.translated (-getWidth(), 0.0f).to<int>());

    sendMoved();
}

Point<float> Component::getBottomRight() const
{
    return boundsInParent.getBottomRight();
}

void Component::setBottomRight (const Point<float>& newBottomRight)
{
    boundsInParent.setBottomRight (newBottomRight);

    if (options.onDesktop && native != nullptr)
        native->setPosition (newBottomRight.translated (-getWidth(), -getHeight()).to<int>());

    sendMoved();
}

Point<float> Component::getCenter() const
{
    return boundsInParent.getCenter();
}

void Component::setCenter (const Point<float>& newCenter)
{
    boundsInParent.setCenter (newCenter);

    if (options.onDesktop && native != nullptr)
        native->setPosition (newCenter.translated (-getWidth() / 2.0f, -getHeight() / 2.0f).to<int>());

    sendMoved();
}

float Component::getCenterX() const
{
    return boundsInParent.getCenterX();
}

void Component::setCenterX (float newCenterX)
{
    boundsInParent.setCenterX (newCenterX);

    if (options.onDesktop && native != nullptr)
    {
        auto newCenter = boundsInParent.getCenter();
        native->setPosition (newCenter.translated (-getWidth() / 2.0f, 0.0f).to<int>());
    }

    sendMoved();
}

float Component::getCenterY() const
{
    return boundsInParent.getCenterY();
}

void Component::setCenterY (float newCenterY)
{
    boundsInParent.setCenterY (newCenterY);

    if (options.onDesktop && native != nullptr)
    {
        auto newCenter = boundsInParent.getCenter();
        native->setPosition (newCenter.translated (0.0f, -getHeight() / 2.0f).to<int>());
    }

    sendMoved();
}

void Component::moved() {}

void Component::sendMoved()
{
    moved();

    componentListeners.call ([this] (ComponentListener& listener)
    {
        listener.componentMoved (*this);
    });
}

//==============================================================================

void Component::setSize (float width, float height)
{
    setSize ({ width, height });
}

void Component::setSize (const Size<float>& newSize)
{
    auto areaToRepaint = boundsInParent;
    boundsInParent = boundsInParent.withSize (newSize);
    areaToRepaint = areaToRepaint.unionWith (boundsInParent);

    if (options.onDesktop && native != nullptr)
        native->setSize (newSize.to<int>());

    sendResized();

    repaint (areaToRepaint);
}

Size<float> Component::getSize() const
{
    if (options.onDesktop && native != nullptr)
        return native->getSize().to<float>();

    return boundsInParent.getSize();
}

float Component::getWidth() const
{
    return boundsInParent.getWidth();
}

float Component::getHeight() const
{
    return boundsInParent.getHeight();
}

//==============================================================================

void Component::setBounds (float x, float y, float width, float height)
{
    setBounds ({ x, y, width, height });
}

void Component::setBounds (const Rectangle<float>& newBounds)
{
    repaint();

    boundsInParent = newBounds;

    if (options.onDesktop && native != nullptr)
        native->setBounds (newBounds.to<int>());

    auto bailOutChecker = BailOutChecker (this);

    sendResized();

    if (bailOutChecker.shouldBailOut())
        return;

    sendMoved();
}

Rectangle<float> Component::getBounds() const
{
    return boundsInParent;
}

Rectangle<float> Component::getLocalBounds() const
{
    return boundsInParent.withZeroPosition();
}

Rectangle<float> Component::getBoundsRelativeToTopLevelComponent() const
{
    auto bounds = boundsInParent;
    if (options.onDesktop || options.paintAsOffscreenRoot)
        return bounds.withZeroPosition();

    auto parent = getParentComponent();
    while (parent != nullptr && ! parent->options.onDesktop && ! parent->options.paintAsOffscreenRoot)
    {
        bounds.translate (parent->getPosition());
        parent = parent->getParentComponent();
    }

    return bounds;
}

float Component::proportionOfWidth (float proportion) const
{
    return getWidth() * proportion;
}

float Component::proportionOfHeight (float proportion) const
{
    return getHeight() * proportion;
}

void Component::resized() {}

void Component::sendResized()
{
    resized();

    componentListeners.call ([this] (ComponentListener& listener)
    {
        listener.componentResized (*this);
    });
}

//==============================================================================

void Component::setTransform (const AffineTransform& newTransform)
{
    if (transform == newTransform)
        return;

    transform = newTransform;

    transformChanged();
}

AffineTransform Component::getTransform() const
{
    return transform;
}

bool Component::isTransformed() const
{
    return ! transform.isIdentity();
}

void Component::transformChanged()
{
}

//==============================================================================

bool Component::isFullScreen() const
{
    return options.isFullScreen;
}

void Component::setFullScreen (bool shouldBeFullScreen)
{
    if (options.isFullScreen == shouldBeFullScreen)
        return;

    options.isFullScreen = shouldBeFullScreen;

    if (options.onDesktop && native != nullptr)
        native->setFullScreen (shouldBeFullScreen);
}

//==============================================================================

void Component::displayChanged() {}

//==============================================================================

float Component::getScaleDpi() const
{
    if (options.onDesktop && native != nullptr)
        return native->getScaleDpi();

    if (parentComponent == nullptr)
        return 1.0f;

    return parentComponent->getScaleDpi();
}

void Component::contentScaleChanged ([[maybe_unused]] float dpiScale) {}

//==============================================================================

void Component::setOpacity (float newOpacity)
{
    newOpacity = jlimit (0.0f, 1.0f, newOpacity);

    opacity = static_cast<uint8> (newOpacity * 255);

    if (options.onDesktop && native != nullptr)
        native->setOpacity (newOpacity);
}

float Component::getOpacity() const
{
    return opacity / 255.0f;
}

//==============================================================================

bool Component::isOpaque() const
{
    return ! options.isTransparent;
}

void Component::setOpaque (bool shouldBeOpaque)
{
    options.isTransparent = ! shouldBeOpaque;
}

//==============================================================================

void Component::enableRenderingUnclipped (bool shouldBeEnabled)
{
    options.unclippedRendering = shouldBeEnabled;
}

bool Component::isRenderingUnclipped() const
{
    return options.unclippedRendering;
}

void Component::setPaintProfilingDisabled (bool shouldBeDisabled)
{
    options.paintProfilingDisabled = shouldBeDisabled;
}

bool Component::isPaintProfilingDisabled() const
{
    return options.paintProfilingDisabled;
}

void Component::repaint()
{
    repaint (getLocalBounds());
}

void Component::repaint (float x, float y, float width, float height)
{
    repaint ({ x, y, width, height });
}

void Component::repaint (const Rectangle<float>& rect)
{
    jassert (! options.isRepainting); // You are likely repainting from paint !

    cachedTextureCanvas = nullptr;

    if (rect.isEmpty() || ! isShowing())
        return;

    internalRepaint (rect);
}

//==============================================================================

void* Component::getNativeHandle() const
{
    if (options.onDesktop && native != nullptr)
        return native->getNativeHandle();

    return nullptr;
}

//==============================================================================

ComponentNative* Component::getNativeComponent()
{
    if (native != nullptr)
        return native.get();

    if (parentComponent == nullptr)
        return nullptr;

    return parentComponent->getNativeComponent();
}

const ComponentNative* Component::getNativeComponent() const
{
    if (native != nullptr)
        return native.get();

    if (parentComponent == nullptr)
        return nullptr;

    return parentComponent->getNativeComponent();
}

void Component::attachedToNative() {}

void Component::detachedFromNative() {}

//==============================================================================

bool Component::isOnDesktop() const
{
    return options.onDesktop;
}

void Component::addToDesktop (const ComponentNative::Options& nativeOptions, void* parent)
{
    YUP_ASSERT_MESSAGE_MANAGER_IS_LOCKED

    if (options.onDesktop)
        removeFromDesktop();

    if (parentComponent != nullptr)
    {
        parentComponent->removeChildComponent (this);
        parentComponent = nullptr;
    }

    options.onDesktop = true;

    native = ComponentNative::createFor (*this, nativeOptions, parent);

    internalAttachedToNative();

    setBounds (getBounds()); // This is needed to update based on scaleDpi
}

void Component::removeFromDesktop()
{
    YUP_ASSERT_MESSAGE_MANAGER_IS_LOCKED

    if (! options.onDesktop)
        return;

    options.onDesktop = false;

    native.reset();

    internalDetachedFromNative();
}

//==============================================================================

void Component::toFront (bool shouldGainKeyboardFocus)
{
    if (options.onDesktop && native != nullptr)
        native->toFront();

    if (parentComponent == nullptr)
        return;

    parentComponent->addChildComponent (this, parentComponent->getNumChildComponents());

    if (shouldGainKeyboardFocus && options.wantsKeyboardFocus)
        takeKeyboardFocus();
}

void Component::toBack()
{
    if (parentComponent == nullptr)
        return;

    parentComponent->addChildComponent (this, 0);
}

void Component::raiseAbove (Component* component)
{
    if (parentComponent == nullptr)
        return;

    auto indexOfComponent = parentComponent->getIndexOfChildComponent (component);
    if (indexOfComponent < 0)
        return;

    indexOfComponent = jmin (indexOfComponent + 1, parentComponent->getNumChildComponents());

    parentComponent->addChildComponent (this, indexOfComponent);
}

void Component::lowerBelow (Component* component)
{
    if (parentComponent == nullptr)
        return;

    auto indexOfComponent = parentComponent->getIndexOfChildComponent (component);
    if (indexOfComponent < 0)
        return;

    indexOfComponent = jmax (indexOfComponent - 1, 0);

    parentComponent->addChildComponent (this, indexOfComponent);
}

void Component::raiseBy (int indexToRaise)
{
    if (parentComponent == nullptr)
        return;

    const int currentIndex = parentComponent->getIndexOfChildComponent (this);
    const int newIndex = jmin (currentIndex + indexToRaise, parentComponent->getNumChildComponents());

    if (currentIndex != newIndex)
        parentComponent->addChildComponent (this, newIndex);
}

void Component::lowerBy (int indexToLower)
{
    const int currentIndex = parentComponent->getIndexOfChildComponent (this);
    const int newIndex = jmax (currentIndex - indexToLower, 0);

    if (currentIndex != newIndex)
        parentComponent->addChildComponent (this, newIndex);
}

//==============================================================================

bool Component::hasParent() const
{
    return parentComponent != nullptr;
}

Component* Component::getParentComponent()
{
    return parentComponent;
}

const Component* Component::getParentComponent() const
{
    return parentComponent;
}

//==============================================================================

void Component::addChildComponent (Component& component, int index)
{
    addChildComponent (&component, index);
}

void Component::addChildComponent (Component* component, int index)
{
    jassert (component != nullptr);

    component->parentComponent = this;

    const int currentIndex = children.indexOf (component);

    if (isPositiveAndBelow (currentIndex, children.size()))
    {
        if (currentIndex != index)
        {
            children.move (currentIndex, index);

            auto bailOutChecker = BailOutChecker (this);

            component->internalHierarchyChanged();

            if (bailOutChecker.shouldBailOut())
                return;

            childrenChanged();
        }
    }
    else
    {
        children.insert (index, component);

        auto bailOutChecker = BailOutChecker (this);

        if (getNativeComponent() != nullptr)
        {
            component->internalAttachedToNative();

            if (bailOutChecker.shouldBailOut())
                return;
        }

        component->internalHierarchyChanged();

        if (bailOutChecker.shouldBailOut())
            return;

        childrenChanged();
    }
}

void Component::addAndMakeVisible (Component& component, int index)
{
    addAndMakeVisible (&component, index);
}

void Component::addAndMakeVisible (Component* component, int index)
{
    addChildComponent (component, index);

    component->setVisible (true);
}

void Component::removeChildComponent (Component& component)
{
    removeChildComponent (&component);
}

void Component::removeChildComponent (Component* component)
{
    jassert (component != nullptr);

    auto indexToRemove = children.indexOf (component);
    removeChildComponent (indexToRemove);
}

void Component::removeChildComponent (int index)
{
    if (! isPositiveAndBelow (index, children.size()))
        return;

    auto component = children.removeAndReturn (index);

    if (component->isShowing())
        repaint (component->getBounds());

    component->parentComponent = nullptr;

    auto bailOutChecker = BailOutChecker (this);

    if (getNativeComponent() != nullptr)
    {
        component->internalDetachedFromNative();

        if (bailOutChecker.shouldBailOut())
            return;
    }

    component->internalHierarchyChanged();

    if (bailOutChecker.shouldBailOut())
        return;

    childrenChanged();
}

void Component::removeAllChildren()
{
    while (! children.isEmpty())
        removeChildComponent (children.size() - 1);
}

void Component::internalHierarchyChanged()
{
    parentHierarchyChanged();

    auto bailOutChecker = BailOutChecker (this);

    for (int index = children.size(); --index >= 0;)
    {
        auto child = children.getUnchecked (index);

        if (bailOutChecker.shouldBailOut())
        {
            jassertfalse; // Deleting a parent component when notifying its children!
            return;
        }

        child->internalHierarchyChanged();

        index = jmin (index, children.size());
    }
}

void Component::parentHierarchyChanged() {}

void Component::childrenChanged() {}

//==============================================================================

int Component::getNumChildComponents() const
{
    return children.size();
}

Component* Component::getChildComponent (int index) const
{
    return children.getUnchecked (index);
}

int Component::getIndexOfChildComponent (Component* component) const
{
    return children.indexOf (component);
}

Component* Component::findComponentAt (const Point<float>& p)
{
    if (! options.isVisible || ! boundsInParent.withZeroPosition().contains (p))
        return nullptr;

    for (int index = children.size(); --index >= 0;)
    {
        auto child = children.getUnchecked (index);
        if (! child->isVisible() || ! child->boundsInParent.contains (p))
            continue;

        child = child->findComponentAt (p - child->boundsInParent.getPosition());
        if (child != nullptr)
            return child;
    }

    return this;
}

Component* Component::findComponentAtForMouseEvent (const Point<float>& p)
{
    if (! options.isVisible || ! boundsInParent.withZeroPosition().contains (p))
        return nullptr;

    if (doesWantChildrenMouseEvents())
    {
        for (int index = children.size(); --index >= 0;)
        {
            auto child = children.getUnchecked (index);
            if (! child->isVisible() || ! child->boundsInParent.contains (p))
                continue;

            if (auto* hit = child->findComponentAtForMouseEvent (p - child->boundsInParent.getPosition()))
                return hit;
        }
    }

    return doesWantSelfMouseEvents() ? this : nullptr;
}

Component* Component::getTopLevelComponent()
{
    auto currentComponent = this;

    auto parent = getParentComponent();
    while (parent != nullptr)
    {
        currentComponent = parent;
        parent = currentComponent->getParentComponent();
    }

    return currentComponent;
}

//==============================================================================

void Component::setMouseCursor (const MouseCursor& cursorType)
{
    mouseCursor = cursorType;

    if (auto nativeComponent = getNativeComponent())
    {
        if (nativeComponent->getFocusedComponent() == this)
            updateMouseCursor();
    }
}

MouseCursor Component::getMouseCursor() const
{
    return mouseCursor;
}

//==============================================================================

void Component::setWantsKeyboardFocus (bool wantsFocus)
{
    options.wantsKeyboardFocus = wantsFocus;
}

bool Component::getWantsKeyboardFocus() const
{
    return options.wantsKeyboardFocus;
}

void Component::setClickingGrabFocus (bool shouldGrabFocus)
{
    options.clickingDoesNotGrabFocus = ! shouldGrabFocus;
}

bool Component::getClickingGrabFocus() const
{
    return ! options.clickingDoesNotGrabFocus;
}

void Component::takeKeyboardFocus()
{
    if (! options.wantsKeyboardFocus || ! isEnabled())
        return;

    if (auto nativeComponent = getNativeComponent())
        nativeComponent->setFocusedComponent (this);
}

void Component::leaveKeyboardFocus()
{
    if (auto nativeComponent = getNativeComponent())
    {
        if (nativeComponent->getFocusedComponent() == this)
            nativeComponent->setFocusedComponent (nullptr);
    }
}

bool Component::hasKeyboardFocus() const
{
    if (! options.wantsKeyboardFocus || ! isEnabled())
        return false;

    if (auto nativeComponent = getNativeComponent())
        return nativeComponent->getFocusedComponent() == this;

    return false;
}

void Component::focusGained() {}

void Component::focusLost() {}

//==============================================================================

void Component::handleKeyboardFocusFromClick()
{
    for (auto* component = this; component != nullptr; component = component->parentComponent)
    {
        if (component->options.wantsKeyboardFocus && ! component->options.clickingDoesNotGrabFocus)
        {
            component->takeKeyboardFocus();
            return;
        }
    }
}

//==============================================================================

NamedValueSet& Component::getProperties()
{
    return properties;
}

const NamedValueSet& Component::getProperties() const
{
    return properties;
}

//==============================================================================

void Component::paint (Graphics& g)
{
    jassert (! isOpaque()); // If your component is opaque, you need to paint it !
}

void Component::paintOverChildren (Graphics& g) {}

void Component::refreshDisplay (double lastFrameTimeSeconds) {}

//==============================================================================

void Component::setWantsMouseEvents (bool allowSelfMouseEvents, bool allowChildrenMouseEvents)
{
    options.blockSelfMouseEvents = ! allowSelfMouseEvents;
    options.blockChildrenMouseEvents = ! allowChildrenMouseEvents;
}

bool Component::doesWantSelfMouseEvents() const
{
    return ! options.blockSelfMouseEvents;
}

bool Component::doesWantChildrenMouseEvents() const
{
    return ! options.blockChildrenMouseEvents;
}

//==============================================================================

void Component::mouseEnter (const MouseEvent& event) {}

void Component::mouseExit (const MouseEvent& event) {}

void Component::mouseDown (const MouseEvent& event) {}

void Component::mouseMove (const MouseEvent& event) {}

void Component::mouseDrag (const MouseEvent& event) {}

void Component::mouseUp (const MouseEvent& event) {}

void Component::mouseDoubleClick (const MouseEvent& event) {}

void Component::mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) {}

bool Component::isInterestedInDrag (const DragAndDropData& data) { return false; }

bool Component::itemsDropped (const Point<float>& position, const DragAndDropData& data) { return false; }

void Component::itemDragEnter (const DragAndDropData& data, const Point<float>& position) {}

void Component::itemDragMove (const DragAndDropData& data, const Point<float>& position) {}

void Component::itemDragExit (const DragAndDropData& data) {}

void Component::keyDown (const KeyPress& keys, const Point<float>& position) {}

void Component::keyUp (const KeyPress& keys, const Point<float>& position) {}

void Component::textInput (const String& text) {}

//==============================================================================

void Component::addMouseListener (MouseListener* listener)
{
    mouseListeners.add (listener);
}

void Component::removeMouseListener (MouseListener* listener)
{
    mouseListeners.remove (listener);
}

//==============================================================================

void Component::addComponentListener (ComponentListener* listener)
{
    componentListeners.add (listener);
}

void Component::removeComponentListener (ComponentListener* listener)
{
    componentListeners.remove (listener);
}

//==============================================================================

void Component::setStyle (ComponentStyle::Ptr newStyle)
{
    if (style == newStyle)
        return;

    style = std::move (newStyle);

    auto bailOutChecker = BailOutChecker (this);

    styleChanged();

    if (bailOutChecker.shouldBailOut())
        return;

    repaint();
}

ComponentStyle::Ptr Component::getStyle() const
{
    return style;
}

void Component::styleChanged() {}

//==============================================================================

void Component::setColor (const Identifier& colorId, const std::optional<Color>& color)
{
    if (color)
        properties.set (colorId, static_cast<int64> (color->getARGB()));
    else
        properties.remove (colorId);

    styleChanged();
}

std::optional<Color> Component::getColor (const Identifier& colorId) const
{
    if (auto color = properties.getVarPointer (colorId); color != nullptr && color->isInt64())
        return Color (static_cast<uint32> (static_cast<int64> (*color)));

    return std::nullopt;
}

std::optional<Color> Component::findColor (const Identifier& colorId) const
{
    if (auto color = getColor (colorId))
        return color;

    if (parentComponent != nullptr)
        return parentComponent->findColor (colorId);

    return std::nullopt;
}

//==============================================================================

void Component::setMetric (const Identifier& metricId, const std::optional<float>& metric)
{
    if (metric)
        properties.set (metricId, static_cast<double> (*metric));
    else
        properties.remove (metricId);

    styleChanged();
}

std::optional<float> Component::getMetric (const Identifier& metricId) const
{
    if (auto value = properties.getVarPointer (metricId); value != nullptr && value->isDouble())
        return static_cast<float> (static_cast<double> (*value));

    return std::nullopt;
}

std::optional<float> Component::findMetric (const Identifier& metricId) const
{
    if (auto metric = getMetric (metricId))
        return metric;

    if (parentComponent != nullptr)
        return parentComponent->findMetric (metricId);

    return std::nullopt;
}

//==============================================================================

//==============================================================================

void Component::setComponentEffect (ComponentEffect::Ptr effect)
{
    componentEffect = std::move (effect);

    if (componentEffect == nullptr)
        effectOffscreenCanvas = nullptr;

    repaint();
}

ComponentEffect::Ptr Component::getComponentEffect() const
{
    return componentEffect;
}

void Component::setCachedToTexture (bool shouldCache)
{
    if (options.cachedToTexture == shouldCache)
        return;

    options.cachedToTexture = shouldCache;
    cachedTextureCanvas = nullptr;
    repaint();
}

bool Component::isCachedToTexture() const
{
    return options.cachedToTexture;
}

GpuCanvas::Ptr Component::renderSnapshotOffscreen (GraphicsContext& ctx, bool includeEffects)
{
    if (getWidth() <= 0.0f || getHeight() <= 0.0f)
        return nullptr;

    auto canvas = renderSubtreeOffscreen (ctx, getOpacity(), false);
    if (canvas == nullptr)
        return nullptr;

    if (! includeEffects || componentEffect == nullptr)
        return canvas;

    auto texture = canvas->asTexture();

    auto effectCanvas = GpuCanvas::create (ctx, canvas->getWidth(), canvas->getHeight());
    if (effectCanvas == nullptr)
        return canvas;

    auto& g = effectCanvas->beginDraw();
    auto localBounds = getLocalBounds();
    g.setDrawingArea (localBounds);
    componentEffect->apply (g, texture, localBounds);

    return effectCanvas;
}

Image Component::snapshotToImage (GraphicsContext& ctx, bool includeEffects)
{
    auto canvas = renderSnapshotOffscreen (ctx, includeEffects);
    if (canvas == nullptr)
        return {};

    return canvas->asImage();
}

GpuTexture::Ptr Component::snapshotToTexture (GraphicsContext& ctx, bool includeEffects)
{
    auto canvas = renderSnapshotOffscreen (ctx, includeEffects);
    if (canvas == nullptr)
        return nullptr;

    return canvas->asTexture();
}

//==============================================================================

void Component::userTriedToCloseWindow() {}

//==============================================================================

bool Component::hasOpaqueChildCoveringArea (const Rectangle<float>& area)
{
    // Check only direct children - no recursive hierarchy traversal
    for (int childIndex = children.size(); --childIndex >= 0;)
    {
        auto child = children.getUnchecked (childIndex);
        if (! child->isVisible() || ! child->isOpaque() || child->options.unclippedRendering || child->isTransformed())
            continue;

        auto childBounds = child->getBoundsRelativeToTopLevelComponent();
        if (childBounds.contains (area))
            return true;
    }

    return false;
}

void Component::internalRefreshDisplay (double lastFrameTimeSeconds)
{
    refreshDisplay (lastFrameTimeSeconds);

    for (auto child : children)
        child->internalRefreshDisplay (lastFrameTimeSeconds);
}

//==============================================================================

void Component::internalRepaint()
{
    internalRepaint (getLocalBounds());
}

void Component::internalRepaint (const Rectangle<float>& rect)
{
    if (rect.isEmpty())
        return;

    if (auto nativeComponent = getNativeComponent())
        nativeComponent->repaint (rect.translated (getBoundsRelativeToTopLevelComponent().getTopLeft()));
}

//==============================================================================

void Component::paintChildrenAndOverChildren (Graphics& g, const Rectangle<float>& clipArea, bool renderContinuous)
{
    for (auto child : children)
        child->internalPaint (g, clipArea, renderContinuous);

    paintOverChildren (g);
}

GpuCanvas::Ptr Component::renderSubtreeOffscreen (GraphicsContext& ctx, float opacity, bool renderContinuous, GpuCanvas::Ptr reuseCanvas)
{
    if (getWidth() <= 0.0f || getHeight() <= 0.0f)
        return nullptr;

    const auto w = static_cast<int> (getWidth());
    const auto h = static_cast<int> (getHeight());

    GpuCanvas::Ptr canvas;
    if (reuseCanvas != nullptr && reuseCanvas->getWidth() == w && reuseCanvas->getHeight() == h)
    {
        canvas = std::move (reuseCanvas);
    }
    else
    {
        reuseCanvas = nullptr;
        canvas = GpuCanvas::create (ctx, w, h);
    }

    if (canvas == nullptr)
        return nullptr;

    auto& offscreenG = canvas->beginDraw();

    options.paintAsOffscreenRoot = true;

    auto localBounds = getLocalBounds();
    paintSubtree (offscreenG, localBounds, localBounds, opacity, renderContinuous);

    options.paintAsOffscreenRoot = false;

    canvas->commit();
    return canvas;
}

//==============================================================================

void Component::paintSubtree (Graphics& g, const Rectangle<float>& drawingArea, const Rectangle<float>& clipArea, float opacity, bool renderContinuous)
{
    options.isRepainting = true;

    const ErasedScopeGuard resetIsRepainting ([&]
    {
        options.isRepainting = false;
    });

    {
        const bool shouldMeasurePaint = ! options.paintProfilingDisabled && ! componentListeners.isEmpty();

        ComponentPaintMetrics metrics;
        int64 totalStartTicks = 0;
        int64 selfStartTicks = 0;

        if (shouldMeasurePaint)
        {
            totalStartTicks = Time::getHighResolutionTicks();
            metrics.repaintArea = drawingArea;
            metrics.componentBounds = drawingArea;
            metrics.renderContinuous = renderContinuous;
        }

        const auto globalState = g.saveState();

        g.setOpacity (opacity);
        g.setDrawingArea (drawingArea);
        if (! options.unclippedRendering)
            g.setClipPath (clipArea);
        g.setTransform (transform);

        bool canSkipPaint = false;
        if (! options.unclippedRendering && ! isTransformed())
            canSkipPaint = hasOpaqueChildCoveringArea (clipArea);

        if (! canSkipPaint)
        {
            const auto paintState = g.saveState();

            if (shouldMeasurePaint)
            {
                selfStartTicks = Time::getHighResolutionTicks();
                paint (g);
                metrics.selfTicks += Time::getHighResolutionTicks() - selfStartTicks;
            }
            else
            {
                paint (g);
            }
        }
        else
        {
            if (shouldMeasurePaint)
                metrics.selfPaintSkipped = true;
        }

        if (shouldMeasurePaint)
        {
            const int64 childrenStartTicks = Time::getHighResolutionTicks();

            for (auto child : children)
                child->internalPaint (g, clipArea, renderContinuous);

            metrics.childrenTicks += Time::getHighResolutionTicks() - childrenStartTicks;

            selfStartTicks = Time::getHighResolutionTicks();

            paintOverChildren (g);

            const int64 selfEndTicks = Time::getHighResolutionTicks();

            metrics.selfTicks += selfEndTicks - selfStartTicks;
            metrics.totalTicks = selfEndTicks - totalStartTicks;

            componentListeners.call (&ComponentListener::componentPaintCompleted, *this, metrics);
        }
        else
        {
            paintChildrenAndOverChildren (g, clipArea, renderContinuous);
        }
    }
}

//==============================================================================

void Component::internalPaint (Graphics& g, const Rectangle<float>& repaintArea, bool renderContinuous)
{
    if (! isVisible() || getWidth() <= 0.0f || getHeight() <= 0.0f)
        return;

    auto bounds = getBoundsRelativeToTopLevelComponent();

    auto boundsToRedraw = bounds
                              .intersection (repaintArea)
                              .roundToInt()
                              .to<float>();

    if (! renderContinuous && boundsToRedraw.isEmpty())
        return;

    const auto selfOpacity = (! options.onDesktop && native == nullptr) ? getOpacity() : 1.0f;
    const auto opacity = g.getOpacity() * selfOpacity;
    if (opacity <= 0.0f)
        return;

    // Effect path: render full subtree offscreen, apply effect, composite
    if (componentEffect != nullptr)
    {
        auto canvas = renderSubtreeOffscreen (g.getGraphicsContext(), opacity, renderContinuous, std::move (effectOffscreenCanvas));
        if (canvas == nullptr)
            return;

        auto texture = canvas->asTexture();

        {
            const auto saved = g.saveState();
            g.setOpacity (opacity);
            g.setDrawingArea (bounds);
            if (! options.unclippedRendering)
                g.setClipPath (boundsToRedraw);
            g.setTransform (transform);

            componentEffect->apply (g, texture, getLocalBounds());
        }

        effectOffscreenCanvas = canvas;

        if (options.cachedToTexture)
            cachedTextureCanvas = canvas;

        return;
    }

    // Cache path: cache own paint(), children paint on top
    if (options.cachedToTexture)
    {
        if (cachedTextureCanvas == nullptr)
        {
            auto canvas = GpuCanvas::create (g.getGraphicsContext(),
                                             static_cast<int> (getWidth()),
                                             static_cast<int> (getHeight()));
            if (canvas != nullptr)
            {
                auto& offscreenG = canvas->beginDraw();
                auto localBounds = getLocalBounds();
                offscreenG.setOpacity (opacity);
                offscreenG.setDrawingArea (localBounds);
                offscreenG.setTransform (transform);
                paint (offscreenG);
                canvas->commit();
                cachedTextureCanvas = canvas;
            }
        }

        options.isRepainting = true;

        const ErasedScopeGuard resetIsRepainting ([&]
        {
            options.isRepainting = false;
        });

        {
            const auto saved = g.saveState();
            g.setOpacity (opacity);
            g.setDrawingArea (bounds);
            if (! options.unclippedRendering)
                g.setClipPath (boundsToRedraw);
            g.setTransform (transform);

            if (cachedTextureCanvas != nullptr)
                g.drawTexture (cachedTextureCanvas->asTexture(), getLocalBounds());
            else
                paint (g);
        }

        paintChildrenAndOverChildren (g, boundsToRedraw, renderContinuous);
        return;
    }

    // Normal paint path
    paintSubtree (g, bounds, boundsToRedraw, opacity, renderContinuous);

#if YUP_ENABLE_COMPONENT_PAINT_DEBUGGING
    g.setFillColor (debugColor);
    g.setOpacity (0.2f);
    g.fillAll();

    if (--counter == 0)
    {
        counter = 2;
        debugColor = Color::opaqueRandom();
    }
#endif
}

//==============================================================================

void Component::internalMouseEnter (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    auto bailOutChecker = BailOutChecker (this);

    mouseEnter (event);

    if (bailOutChecker.shouldBailOut())
        return;

    mouseListeners.callChecked (bailOutChecker, &MouseListener::mouseEnter, event);
}

//==============================================================================

void Component::internalMouseExit (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    auto bailOutChecker = BailOutChecker (this);

    mouseExit (event);

    if (bailOutChecker.shouldBailOut())
        return;

    mouseListeners.callChecked (bailOutChecker, &MouseListener::mouseExit, event);
}

//==============================================================================

void Component::internalMouseDown (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    auto bailOutChecker = BailOutChecker (this);

    handleKeyboardFocusFromClick();

    if (bailOutChecker.shouldBailOut())
        return;

    mouseDown (event);

    if (bailOutChecker.shouldBailOut())
        return;

    mouseListeners.callChecked (bailOutChecker, &MouseListener::mouseDown, event);
}

//==============================================================================

void Component::internalMouseMove (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    auto bailOutChecker = BailOutChecker (this);

    mouseMove (event);

    if (bailOutChecker.shouldBailOut())
        return;

    mouseListeners.callChecked (bailOutChecker, &MouseListener::mouseMove, event);
}

//==============================================================================

void Component::internalMouseDrag (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    auto bailOutChecker = BailOutChecker (this);

    mouseDrag (event);

    if (bailOutChecker.shouldBailOut())
        return;

    mouseListeners.callChecked (bailOutChecker, &MouseListener::mouseDrag, event);
}

//==============================================================================

void Component::internalMouseUp (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    auto bailOutChecker = BailOutChecker (this);

    mouseUp (event);

    if (bailOutChecker.shouldBailOut())
        return;

    mouseListeners.callChecked (bailOutChecker, &MouseListener::mouseUp, event);
}

//==============================================================================

void Component::internalMouseDoubleClick (const MouseEvent& event)
{
    if (! isVisible())
        return;

    auto bailOutChecker = BailOutChecker (this);

    mouseDoubleClick (event);

    if (bailOutChecker.shouldBailOut())
        return;

    mouseListeners.callChecked (bailOutChecker, &MouseListener::mouseDoubleClick, event);
}

//==============================================================================

void Component::internalMouseWheel (const MouseEvent& event, const MouseWheelData& wheelData)
{
    if (! isVisible())
        return;

    auto bailOutChecker = BailOutChecker (this);

    mouseWheel (event, wheelData);

    if (bailOutChecker.shouldBailOut())
        return;

    mouseListeners.callChecked (bailOutChecker, &MouseListener::mouseWheel, event, wheelData);
}

//==============================================================================

bool Component::internalItemsDropped (const DragAndDropData& data, const Point<float>& windowPosition)
{
    // Convert the window (root) position into this component's local coordinates,
    // mirroring MouseEvent::withRelativePositionTo.
    auto localPosition = windowPosition;
    for (Component* current = this; current != nullptr && current->getParentComponent() != nullptr; current = current->getParentComponent())
        localPosition = localPosition - current->getBounds().getPosition();

    for (Component* current = this; current != nullptr; current = current->getParentComponent())
    {
        if (current->isVisible() && current->isEnabled() && current->isInterestedInDrag (data))
        {
            if (current->itemsDropped (localPosition, data))
                return true;
        }

        // Ascend to the parent: the parent-local position adds back this component's offset.
        if (current->getParentComponent() != nullptr)
            localPosition = localPosition + current->getBounds().getPosition();
    }

    return false;
}

//==============================================================================

void Component::internalItemDragEnter (const DragAndDropData& data, const Point<float>& windowPosition)
{
    auto localPosition = windowPosition;
    for (Component* current = this; current != nullptr && current->getParentComponent() != nullptr; current = current->getParentComponent())
        localPosition = localPosition - current->getBounds().getPosition();

    for (Component* current = this; current != nullptr; current = current->getParentComponent())
    {
        if (current->isVisible() && current->isEnabled() && current->isInterestedInDrag (data))
            current->itemDragEnter (data, localPosition);

        if (current->getParentComponent() != nullptr)
            localPosition = localPosition + current->getBounds().getPosition();
    }
}

void Component::internalItemDragMove (const DragAndDropData& data, const Point<float>& windowPosition)
{
    auto localPosition = windowPosition;
    for (Component* current = this; current != nullptr && current->getParentComponent() != nullptr; current = current->getParentComponent())
        localPosition = localPosition - current->getBounds().getPosition();

    for (Component* current = this; current != nullptr; current = current->getParentComponent())
    {
        if (current->isVisible() && current->isEnabled() && current->isInterestedInDrag (data))
            current->itemDragMove (data, localPosition);

        if (current->getParentComponent() != nullptr)
            localPosition = localPosition + current->getBounds().getPosition();
    }
}

void Component::internalItemDragExit (const DragAndDropData& data)
{
    for (Component* current = this; current != nullptr; current = current->getParentComponent())
    {
        if (current->isVisible() && current->isEnabled() && current->isInterestedInDrag (data))
            current->itemDragExit (data);
    }
}

//==============================================================================

void Component::internalKeyDown (const KeyPress& keys, const Point<float>& position)
{
    if (! isVisible() || ! isEnabled())
        return;

    keyDown (keys, position);
}

//==============================================================================

void Component::internalKeyUp (const KeyPress& keys, const Point<float>& position)
{
    if (! isVisible() || ! isEnabled())
        return;

    keyUp (keys, position);
}

//==============================================================================

void Component::internalTextInput (const String& text)
{
    if (! options.wantsKeyboardFocus || ! isVisible() || ! isEnabled())
        return;

    textInput (text);
}

//==============================================================================

void Component::internalResized (int width, int height)
{
    const auto newBounds = boundsInParent.withSize (Size<int> (width, height).to<float>());

    if (newBounds != boundsInParent)
    {
        boundsInParent = newBounds;

        sendResized();
    }
}

//==============================================================================

void Component::internalMoved (int xpos, int ypos)
{
    const auto newBounds = boundsInParent.withPosition (Point<int> (xpos, ypos).to<float>());

    if (newBounds != boundsInParent)
    {
        boundsInParent = newBounds;

        sendMoved();
    }
}

//==============================================================================

void Component::internalFocusChanged (bool gotFocus)
{
    if (gotFocus)
        focusGained();
    else
        focusLost();
}

//==============================================================================

void Component::internalDisplayChanged() {}

//==============================================================================

void Component::internalContentScaleChanged (float dpiScale)
{
    if (contentScale != dpiScale)
    {
        contentScale = dpiScale;

        contentScaleChanged (dpiScale);
    }
}

//==============================================================================

void Component::internalSafeAreaChanged()
{
    auto bailOutChecker = BailOutChecker (this);

    safeAreaChanged();

    if (bailOutChecker.shouldBailOut())
        return;

    for (auto child : children)
    {
        child->internalSafeAreaChanged();

        if (bailOutChecker.shouldBailOut())
            return;
    }
}

//==============================================================================

void Component::internalUserTriedToCloseWindow()
{
    userTriedToCloseWindow();
}

//==============================================================================

void Component::internalAttachedToNative()
{
    auto bailOutChecker = BailOutChecker (this);

    attachedToNative();

    if (bailOutChecker.shouldBailOut())
        return;

    for (auto child : children)
    {
        child->internalAttachedToNative();

        if (bailOutChecker.shouldBailOut())
            return;
    }
}

void Component::internalDetachedFromNative()
{
    auto bailOutChecker = BailOutChecker (this);

    detachedFromNative();

    if (bailOutChecker.shouldBailOut())
        return;

    for (auto child : children)
    {
        child->internalDetachedFromNative();

        if (bailOutChecker.shouldBailOut())
            return;
    }
}

//==============================================================================

void Component::internalVisibilityChanged()
{
    visibilityChanged();

    auto bailOutChecker = BailOutChecker (this);

    for (int index = children.size(); --index >= 0;)
    {
        auto child = children.getUnchecked (index);

        if (bailOutChecker.shouldBailOut())
            return;

        if (child->isVisible())
            child->internalVisibilityChanged();

        index = jmin (index, children.size());
    }
}

//==============================================================================

void Component::updateMouseCursor()
{
    Desktop::getInstance()->setMouseCursor (mouseCursor);
}

//==============================================================================

Point<float> Component::getScreenPosition() const
{
    return localToScreen (getPosition());
}

//==============================================================================

Rectangle<float> Component::getScreenBounds() const
{
    return localToScreen (getLocalBounds());
}

//==============================================================================

Rectangle<float> Component::getSafeAreaBounds() const
{
    if (options.onDesktop && native != nullptr)
        return native->getSafeAreaBounds().to<float>();

    if (parentComponent == nullptr)
        return getLocalBounds();

    return parentComponent->getSafeAreaBounds()
        .translated (-getPosition())
        .intersection (getLocalBounds());
}

void Component::safeAreaChanged() {}

//==============================================================================

Point<float> Component::localToScreen (const Point<float>& localPoint) const
{
    if (options.onDesktop && native != nullptr)
        return native->getPosition().to<float>() + localPoint;

    auto screenPos = localPoint + getPosition();
    auto parent = getParentComponent();

    while (parent != nullptr)
    {
        if (parent->options.onDesktop && parent->native != nullptr)
        {
            screenPos += parent->native->getPosition().to<float>();
            break;
        }
        else
        {
            screenPos += parent->getPosition();
        }

        parent = parent->getParentComponent();
    }

    return screenPos;
}

Point<float> Component::screenToLocal (const Point<float>& screenPoint) const
{
    return screenPoint - localToScreen (Point<float> (0.0f, 0.0f));
}

Rectangle<float> Component::localToScreen (const Rectangle<float>& localRectangle) const
{
    return Rectangle<float> (localToScreen (localRectangle.getPosition()), localRectangle.getSize());
}

Rectangle<float> Component::screenToLocal (const Rectangle<float>& screenRectangle) const
{
    return Rectangle<float> (screenToLocal (screenRectangle.getPosition()), screenRectangle.getSize());
}

//==============================================================================

Point<float> Component::getLocalPoint (const Component* sourceComponent, Point<float> pointInSource) const
{
    if (sourceComponent == nullptr || sourceComponent == this)
        return pointInSource;

    return screenToLocal (sourceComponent->localToScreen (pointInSource));
}

Rectangle<float> Component::getLocalArea (const Component* sourceComponent, Rectangle<float> rectangleInSource) const
{
    if (sourceComponent == nullptr || sourceComponent == this)
        return rectangleInSource;

    return screenToLocal (sourceComponent->localToScreen (rectangleInSource));
}

//==============================================================================

Point<float> Component::getRelativePoint (const Component* targetComponent, Point<float> localPoint) const
{
    if (targetComponent == nullptr || targetComponent == this)
        return localPoint;

    return targetComponent->screenToLocal (localToScreen (localPoint));
}

Rectangle<float> Component::getRelativeArea (const Component* targetComponent, Rectangle<float> localRectangle) const
{
    if (targetComponent == nullptr || targetComponent == this)
        return localRectangle;

    return targetComponent->screenToLocal (localToScreen (localRectangle));
}

//==============================================================================

AffineTransform Component::getTransformToComponent (const Component* targetComponent) const
{
    if (targetComponent == nullptr || targetComponent == this)
        return AffineTransform();

    AffineTransform transform;

    auto thisToScreen = getTransformToScreen();
    auto targetToScreen = targetComponent->getTransformToScreen();

    transform = thisToScreen.followedBy (targetToScreen.inverted());

    return transform;
}

AffineTransform Component::getTransformFromComponent (const Component* sourceComponent) const
{
    if (sourceComponent == nullptr)
        return AffineTransform();

    return sourceComponent->getTransformToComponent (this);
}

AffineTransform Component::getTransformToScreen() const
{
    AffineTransform transform;
    const Component* comp = this;

    while (comp != nullptr)
    {
        if (comp->isTransformed())
            transform = transform.followedBy (comp->getTransform());

        transform = transform.translated (comp->getPosition());

        if (comp->options.onDesktop)
        {
            if (comp->native != nullptr)
            {
                auto nativePos = comp->native->getPosition().to<float>();
                transform = transform.translated (nativePos);
            }

            break;
        }

        comp = comp->getParentComponent();
    }

    return transform;
}

} // namespace yup
