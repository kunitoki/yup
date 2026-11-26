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

struct ComponentIds
{
    static inline const Identifier id { "id" };
    static inline const Identifier title { "title" };
    static inline const Identifier visible { "visible" };
    static inline const Identifier enabled { "enabled" };
    static inline const Identifier opacity { "opacity" };
    static inline const Identifier x { "x" };
    static inline const Identifier y { "y" };
    static inline const Identifier width { "width" };
    static inline const Identifier height { "height" };
    static inline const Identifier bounds { "bounds" };
    static inline const Identifier transform { "transform" };
    static inline const Identifier cached { "cached" };
    static inline const Identifier paintMargin { "paintMargin" };
    static inline const Identifier focused { "focused" };
    static inline const Identifier focusWithin { "focus-within" };
    static inline const Identifier hovered { "hovered" };
    static inline const Identifier pressed { "pressed" };
    static inline const Identifier palette { "palette" };
};

/** Returns the axis-aligned bounding box of the corners of a rectangle mapped through @a mapPoint. */
template <class MapPoint>
Rectangle<float> getMappedBounds (const Rectangle<float>& bounds, MapPoint&& mapPoint)
{
    const auto p1 = mapPoint (bounds.getTopLeft());
    const auto p2 = mapPoint (bounds.getTopRight());
    const auto p3 = mapPoint (bounds.getBottomLeft());
    const auto p4 = mapPoint (bounds.getBottomRight());

    const auto minX = jmin (p1.getX(), p2.getX(), p3.getX(), p4.getX());
    const auto maxX = jmax (p1.getX(), p2.getX(), p3.getX(), p4.getX());
    const auto minY = jmin (p1.getY(), p2.getY(), p3.getY(), p4.getY());
    const auto maxY = jmax (p1.getY(), p2.getY(), p3.getY(), p4.getY());

    return { minX, minY, maxX - minX, maxY - minY };
}

/** Returns the axis-aligned bounding box of a rectangle mapped through a transform. */
Rectangle<float> getTransformedBounds (const Rectangle<float>& bounds, const AffineTransform& transform)
{
    return getMappedBounds (bounds, [&transform] (Point<float> p)
    {
        return p.transformed (transform);
    });
}

/** Returns the rectangles of @a region that lie inside @a localBounds mapped through @a toTopLevel,
    rounded to whole pixels.

    The @a region rectangles live in the coordinate space of the top level component, so @a localBounds
    are mapped through the accumulated transform of the component and its ancestors to obtain the area
    it really covers there. For untransformed components that mapping is a plain offset, so the
    intersection reduces to a bounds check.

    The returned list never contains overlapping rectangles, so it can be used directly as a
    clip region.
*/
RectangleList<float> intersectRepaintRegion (const RectangleList<float>& region, const Rectangle<float>& localBounds, const AffineTransform& toTopLevel)
{
    const auto clipBounds = getTransformedBounds (localBounds, toTopLevel);

    RectangleList<float> result;

    for (const auto& rect : region.getRectangles())
    {
        const auto clipped = clipBounds.intersection (rect).smallestIntContainer();

        if (! clipped.isEmpty())
            result.add (clipped);
    }

    return result;
}

/** Clips subsequent drawing to the union of the rectangles in @a region. */
void setClipRegion (Graphics& g, const RectangleList<float>& region)
{
    if (region.getNumRectangles() == 1)
    {
        g.setClipPath (region.getRectangles()[0]);
        return;
    }

    Path path;

    for (const auto& rect : region.getRectangles())
        path.addRectangle (rect);

    g.setClipPath (path);
}

/** Returns the device-pixel size of an offscreen canvas covering @a logicalSize at @a scale. */
Size<int> getCanvasPixelSize (const Size<float>& logicalSize, float scale)
{
    return { jmax (1, roundToInt (logicalSize.getWidth() * scale)),
             jmax (1, roundToInt (logicalSize.getHeight() * scale)) };
}

/** Returns true if @a canvas exists and has exactly @a pixelSize. */
bool hasPixelSize (const GpuCanvas::Ptr& canvas, const Size<int>& pixelSize)
{
    return canvas != nullptr
        && canvas->getWidth() == pixelSize.getWidth()
        && canvas->getHeight() == pixelSize.getHeight();
}

/** Returns a new offscreen canvas of @a pixelSize, or nullptr where there is none to have: without a GPU,
    or for a size no texture of the device has.
*/
GpuCanvas::Ptr createOffscreenCanvas (GraphicsContext& context, const Size<int>& pixelSize)
{
    const auto device = context.getGpuDevice();
    const auto maxSize = device != nullptr ? static_cast<int> (device->getMaximumTextureSize (GpuTextureType::texture2D)) : 0;

    // A device that tells no limit is left to refuse the size itself
    if (maxSize > 0 && (pixelSize.getWidth() > maxSize || pixelSize.getHeight() > maxSize))
        return nullptr;

    return GpuCanvas::create (context, pixelSize.getWidth(), pixelSize.getHeight());
}

/** The part of a component an offscreen canvas holds: its bounds and a margin around them. */
struct OffscreenArea
{
    Rectangle<float> bounds;      ///< What the canvas holds, in the local space of the component.
    Rectangle<float> visible;     ///< The part of it the component paints in, up to whole pixels: what is drawn back.
    Rectangle<float> clip;        ///< What the component paints in, in the units the canvas is drawn in.
    Point<float> origin;          ///< Where the origin of the component is in the canvas.
    Size<int> pixelSize;          ///< The size of the canvas in device pixels.
    EdgeSpacing<int> pixelMargin; ///< The margin the canvas holds around the bounds, in device pixels.
};

/** Returns the area of an offscreen canvas holding a component of @a size and the @a margin around it.

    The margin is grown to the whole units direct painting is clipped to, then to whole device pixels,
    and added around the pixels the component has without one. So what is painted around the bounds
    shows as far as it does without a canvas, and what is inside them is rendered the same with or
    without a margin.

    A canvas that is kept, holding @a keptMargin pixels around the same bounds, keeps the room it has:
    a margin that shrinks, or moves from an edge to another, needs no new canvas. Only one that grows does.
*/
OffscreenArea getOffscreenArea (const Size<float>& size, const EdgeSpacing<float>& margin, float scale, const GpuCanvas::Ptr& keptCanvas = nullptr, const EdgeSpacing<int>& keptMargin = {})
{
    const auto inner = getCanvasPixelSize (size, scale);
    const auto innerWidth = static_cast<float> (inner.getWidth());
    const auto innerHeight = static_cast<float> (inner.getHeight());

    // Direct painting is clipped to the whole units around the paint bounds
    const auto toUnits = [] (float edge, float length = 0.0f)
    {
        return edge > 0.0f ? std::ceil (length + edge) - length : 0.0f;
    };

    const EdgeSpacing<float> reach (toUnits (margin.getLeft()),
                                    toUnits (margin.getTop()),
                                    toUnits (margin.getRight(), size.getWidth()),
                                    toUnits (margin.getBottom(), size.getHeight()));

    // A size that is no whole number of pixels leaves a part of a pixel to the margin after it, or takes it
    const auto toPixels = [scale] (float edge, float remainder = 0.0f)
    {
        return edge > 0.0f ? jmax (0, roundToInt (std::ceil (edge * scale + remainder - 0.001f))) : 0;
    };

    const EdgeSpacing<int> needed (toPixels (reach.getLeft()),
                                   toPixels (reach.getTop()),
                                   toPixels (reach.getRight(), size.getWidth() * scale - innerWidth),
                                   toPixels (reach.getBottom(), size.getHeight() * scale - innerHeight));

    const bool keepsRoom = hasPixelSize (keptCanvas, { inner.getWidth() + keptMargin.getHorizontal(), inner.getHeight() + keptMargin.getVertical() });

    const auto held = keepsRoom ? EdgeSpacing<int> (jmax (needed.getLeft(), keptMargin.getLeft()),
                                                    jmax (needed.getTop(), keptMargin.getTop()),
                                                    jmax (needed.getRight(), keptMargin.getRight()),
                                                    jmax (needed.getBottom(), keptMargin.getBottom()))
                                : needed;

    const Rectangle<float> bounds (Point<float>(), size);
    const Point<float> origin (static_cast<float> (held.getLeft()) / scale, static_cast<float> (held.getTop()) / scale);

    // A pixel of the canvas covers as much of the component as it does without a margin
    const auto grownBy = [&] (const EdgeSpacing<int>& pixels)
    {
        const auto unitX = size.getWidth() / innerWidth;
        const auto unitY = size.getHeight() / innerHeight;

        return bounds.enlarged (EdgeSpacing<float> (static_cast<float> (pixels.getLeft()) * unitX,
                                                    static_cast<float> (pixels.getTop()) * unitY,
                                                    static_cast<float> (pixels.getRight()) * unitX,
                                                    static_cast<float> (pixels.getBottom()) * unitY));
    };

    return {
        .bounds = grownBy (held),
        .visible = grownBy (needed),
        .clip = bounds.enlarged (reach).translated (origin),
        .origin = origin,
        .pixelSize = { inner.getWidth() + held.getHorizontal(), inner.getHeight() + held.getVertical() },
        .pixelMargin = held
    };
}

/** Returns the whole pixels of an offscreen canvas that @a dirtyRegion touches, to paint them again over what
    the canvas kept, or nothing if one of them is not fully inside the bounds of the component: what is painted
    over has to be covered in full, from an opaque bottom. A clip through a pixel would blend into what is kept.
*/
std::optional<RectangleList<float>> getPartialRepaintRegion (const RectangleList<float>& dirtyRegion, const OffscreenArea& area, const Size<float>& size, float scale)
{
    const auto toPixel = [scale] (float position, bool roundUp)
    {
        return static_cast<int> (roundUp ? std::ceil (position * scale - 0.001f) : std::floor (position * scale + 0.001f));
    };

    const auto left = toPixel (area.origin.getX(), false);
    const auto top = toPixel (area.origin.getY(), false);

    const Rectangle<int> canvas (0, 0, area.pixelSize.getWidth(), area.pixelSize.getHeight());
    const Rectangle<int> solid (left, top, toPixel (size.getWidth(), false), toPixel (size.getHeight(), false));

    RectangleList<float> region;

    for (const auto& rect : dirtyRegion.getRectangles())
    {
        const auto x = left + toPixel (rect.getX(), false);
        const auto y = top + toPixel (rect.getY(), false);
        const auto pixels = Rectangle<int> (x, y, left + toPixel (rect.getRight(), true) - x, top + toPixel (rect.getBottom(), true) - y).intersection (canvas);

        if (pixels.isEmpty())
            continue;

        if (! solid.contains (pixels))
            return std::nullopt;

        region.add (Rectangle<float> (pixels.getX() / scale, pixels.getY() / scale, pixels.getWidth() / scale, pixels.getHeight() / scale));
    }

    return region;
}

} // namespace

//==============================================================================

Component::Component()
    : flagsValue (0)
{
}

Component::Component (StringRef componentID)
    : componentID (componentID)
    , flagsValue (0)
{
}

Component::~Component()
{
    flags.isBeingDeleted = true;

    componentListeners.call (&ComponentListener::componentBeingDeleted, *this);

    while (! frameAttachments.isEmpty())
        frameAttachments.getLast()->detach();

    if (parentComponent != nullptr)
    {
        if (flags.isHovered)
            parentComponent->setHoveredChain (false);

        if (flags.isPressed)
            parentComponent->setPressedChain (false);
    }

    styleContext.reset();

    if (flags.onDesktop)
        removeFromDesktop();

    if (parentComponent != nullptr)
        parentComponent->removeChildComponent (this);

    for (auto component : children)
        component->parentComponent = nullptr;

    children.clear();

    // The effect may outlive the component, and tells no more of what it draws to one that is gone
    if (componentEffect != nullptr)
        componentEffect->components.removeFirstMatchingValue (this);

    masterReference.clear();
}

//==============================================================================

String Component::getComponentID() const
{
    return componentID;
}

void Component::setComponentID (StringRef newComponentID)
{
    if (componentID == newComponentID)
        return;

    componentID = newComponentID;

    notifyPropertyChanged (ComponentIds::id);
}

//==============================================================================

bool Component::isEnabled() const
{
    return ! flags.isDisabled && (parentComponent == nullptr || parentComponent->isEnabled());
}

void Component::setEnabled (bool shouldBeEnabled)
{
    if (flags.isDisabled == ! shouldBeEnabled)
        return;

    flags.isDisabled = ! shouldBeEnabled;

    // Inside a disabled parent the effective state of the subtree doesn't change
    if (parentComponent != nullptr && ! parentComponent->isEnabled())
        return;

    repaint();

    //if (flags.onDesktop && native != nullptr)
    //    native->setEnabled (shouldBeEnabled);

    if (auto* nativeComponent = getNativeComponent())
    {
        auto* hovered = nativeComponent->getPointerTarget();

        for (auto* component = hovered; component != nullptr; component = component->getParentComponent())
        {
            if (component == this)
            {
                hovered->updateMouseCursor();
                break;
            }
        }
    }

    sendEnablementChanged();
}

void Component::sendEnablementChanged()
{
    const WeakReference<Component> self (this);

    // What the component keeps of its painting shows the state it had: the repaint of the one that state was set on
    // does not reach it
    flags.cachedTextureStale = true;
    markEffectCanvasDirty();

    notifyPropertyChanged (ComponentIds::enabled);
    if (self == nullptr)
        return;

    enablementChanged();
    if (self == nullptr)
        return;

    // The descendants disabled on their own stay disabled, so they and their subtrees see no change
    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        if (auto* child = getChildComponent (i); child != nullptr && ! child->flags.isDisabled)
            child->sendEnablementChanged();

        if (self == nullptr)
            return;
    }
}

void Component::enablementChanged() {}

//==============================================================================

bool Component::isVisible() const
{
    return flags.isVisible;
}

void Component::setVisible (bool shouldBeVisible)
{
    if (flags.isVisible == shouldBeVisible)
        return;

    const bool wasShowing = isShowing();

    flags.isVisible = shouldBeVisible;

    auto bailOutChecker = BailOutChecker (this);

    if (flags.onDesktop && native != nullptr)
        native->setVisible (shouldBeVisible);

    if (bailOutChecker.shouldBailOut())
        return;

    if (wasShowing != isShowing())
        internalVisibilityChanged();

    if (bailOutChecker.shouldBailOut())
        return;

    if (parentComponent != nullptr)
        parentComponent->childLayoutChanged (*this);

    if (bailOutChecker.shouldBailOut())
        return;

    notifyPropertyChanged (ComponentIds::visible);

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
    if (componentTitle == title)
        return;

    componentTitle = title;

    if (flags.onDesktop && native != nullptr)
        native->setTitle (title);

    notifyPropertyChanged (ComponentIds::title);
}

//==============================================================================

void Component::applyPositionChange (const Rectangle<float>& oldBounds, Point<int> nativePosition)
{
    if (flags.onDesktop && native != nullptr)
        native->setPosition (nativePosition);

    const BailOutChecker bailOutChecker (this);

    sendMoved();

    if (! bailOutChecker.shouldBailOut())
        notifyBoundsPropertiesChanged (oldBounds);
}

void Component::notifyBoundsPropertiesChanged (const Rectangle<float>& oldBounds)
{
    const BailOutChecker bailOutChecker (this);

    if (oldBounds.getX() != boundsInParent.getX())
    {
        notifyPropertyChanged (ComponentIds::x);
        if (bailOutChecker.shouldBailOut())
            return;
    }

    if (oldBounds.getY() != boundsInParent.getY())
    {
        notifyPropertyChanged (ComponentIds::y);
        if (bailOutChecker.shouldBailOut())
            return;
    }

    if (oldBounds.getWidth() != boundsInParent.getWidth())
    {
        notifyPropertyChanged (ComponentIds::width);
        if (bailOutChecker.shouldBailOut())
            return;
    }

    if (oldBounds.getHeight() != boundsInParent.getHeight())
    {
        notifyPropertyChanged (ComponentIds::height);
        if (bailOutChecker.shouldBailOut())
            return;
    }

    if (oldBounds != boundsInParent)
        notifyPropertyChanged (ComponentIds::bounds);
}

Point<float> Component::getPosition() const
{
    return boundsInParent.getTopLeft();
}

void Component::setPosition (const Point<float>& newPosition)
{
    const auto oldBounds = boundsInParent;
    boundsInParent.setTopLeft (newPosition);
    applyPositionChange (oldBounds, newPosition.to<int>());
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
    const auto oldBounds = boundsInParent;
    boundsInParent.setTopLeft (newTopLeft);
    applyPositionChange (oldBounds, newTopLeft.to<int>());
}

Point<float> Component::getBottomLeft() const
{
    return boundsInParent.getBottomLeft();
}

void Component::setBottomLeft (const Point<float>& newBottomLeft)
{
    const auto oldBounds = boundsInParent;
    boundsInParent.setBottomLeft (newBottomLeft);
    applyPositionChange (oldBounds, newBottomLeft.translated (0.0f, -getHeight()).to<int>());
}

Point<float> Component::getTopRight() const
{
    return boundsInParent.getTopRight();
}

void Component::setTopRight (const Point<float>& newTopRight)
{
    const auto oldBounds = boundsInParent;
    boundsInParent.setTopRight (newTopRight);
    applyPositionChange (oldBounds, newTopRight.translated (-getWidth(), 0.0f).to<int>());
}

Point<float> Component::getBottomRight() const
{
    return boundsInParent.getBottomRight();
}

void Component::setBottomRight (const Point<float>& newBottomRight)
{
    const auto oldBounds = boundsInParent;
    boundsInParent.setBottomRight (newBottomRight);
    applyPositionChange (oldBounds, newBottomRight.translated (-getWidth(), -getHeight()).to<int>());
}

Point<float> Component::getCenter() const
{
    return boundsInParent.getCenter();
}

void Component::setCenter (const Point<float>& newCenter)
{
    const auto oldBounds = boundsInParent;
    boundsInParent.setCenter (newCenter);
    applyPositionChange (oldBounds, newCenter.translated (-getWidth() / 2.0f, -getHeight() / 2.0f).to<int>());
}

float Component::getCenterX() const
{
    return boundsInParent.getCenterX();
}

void Component::setCenterX (float newCenterX)
{
    const auto oldBounds = boundsInParent;
    boundsInParent.setCenterX (newCenterX);
    applyPositionChange (oldBounds, boundsInParent.getCenter().translated (-getWidth() / 2.0f, 0.0f).to<int>());
}

float Component::getCenterY() const
{
    return boundsInParent.getCenterY();
}

void Component::setCenterY (float newCenterY)
{
    const auto oldBounds = boundsInParent;
    boundsInParent.setCenterY (newCenterY);
    applyPositionChange (oldBounds, boundsInParent.getCenter().translated (0.0f, -getHeight() / 2.0f).to<int>());
}

void Component::moved() {}

void Component::sendMoved()
{
    moved();

    auto bailOutChecker = BailOutChecker (this);

    componentListeners.call ([this] (ComponentListener& listener)
    {
        listener.componentMoved (*this);
    });

    if (bailOutChecker.shouldBailOut())
        return;

    sendChildBoundsChangedToParent();
}

//==============================================================================

void Component::setSize (float width, float height)
{
    setSize ({ width, height });
}

void Component::setSize (const Size<float>& newSize)
{
    if (boundsInParent.getSize() == newSize)
        return;

    const auto oldBounds = boundsInParent;
    auto areaToRepaint = getPaintBounds();
    boundsInParent = boundsInParent.withSize (newSize);
    areaToRepaint = areaToRepaint.unionWith (getPaintBounds());

    if (flags.onDesktop && native != nullptr)
        native->setSize (newSize.to<int>());

    const BailOutChecker bailOutChecker (this);

    sendResized();

    if (bailOutChecker.shouldBailOut())
        return;

    notifyBoundsPropertiesChanged (oldBounds);

    if (bailOutChecker.shouldBailOut())
        return;

    repaint (areaToRepaint);
}

Size<float> Component::getSize() const
{
    if (flags.onDesktop && native != nullptr)
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
    if (newBounds == boundsInParent)
        return;

    const auto oldBounds = boundsInParent;

    repaint();

    boundsInParent = newBounds;

    repaint();

    if (flags.onDesktop && native != nullptr)
        native->setBounds (newBounds.to<int>());

    auto bailOutChecker = BailOutChecker (this);

    suppressChildBoundsChanged = true;

    sendResized();

    if (bailOutChecker.shouldBailOut())
        return;

    sendMoved();

    if (bailOutChecker.shouldBailOut())
        return;

    suppressChildBoundsChanged = false;

    sendChildBoundsChangedToParent();

    if (bailOutChecker.shouldBailOut())
        return;

    notifyBoundsPropertiesChanged (oldBounds);
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
    if (flags.onDesktop || flags.paintAsOffscreenRoot)
        return bounds.withPosition (offscreenOrigin);

    auto parent = getParentComponent();
    while (parent != nullptr && ! parent->flags.onDesktop && ! parent->flags.paintAsOffscreenRoot)
    {
        bounds.translate (parent->getPosition());
        parent = parent->getParentComponent();
    }

    return parent != nullptr ? bounds.translated (parent->offscreenOrigin) : bounds;
}

AffineTransform Component::getTransformToTopLevelComponent() const
{
    AffineTransform toTopLevel;
    auto comp = this;

    for (; comp != nullptr && ! comp->flags.onDesktop && ! comp->flags.paintAsOffscreenRoot; comp = comp->getParentComponent())
    {
        if (comp->isTransformed())
            toTopLevel = toTopLevel.followedBy (comp->getCombinedTransform());

        toTopLevel = toTopLevel.translated (comp->getPosition());
    }

    // An offscreen root painted with its margin sits inside its canvas, not at its corner
    return comp != nullptr ? toTopLevel.translated (comp->offscreenOrigin) : toTopLevel;
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

    auto bailOutChecker = BailOutChecker (this);

    componentListeners.call ([this] (ComponentListener& listener)
    {
        listener.componentResized (*this);
    });

    if (bailOutChecker.shouldBailOut())
        return;

    for (int index = children.size(); --index >= 0;)
    {
        children.getUnchecked (index)->parentSizeChanged();

        if (bailOutChecker.shouldBailOut())
            return;

        index = jmin (index, children.size());
    }

    sendChildBoundsChangedToParent();
}

void Component::sendChildBoundsChangedToParent()
{
    if (suppressChildBoundsChanged || parentComponent == nullptr)
        return;

    parentComponent->childBoundsChanged (this);
}

void Component::parentSizeChanged() {}

void Component::childBoundsChanged ([[maybe_unused]] Component* child) {}

//==============================================================================

void Component::setTransform (const AffineTransform& newTransform)
{
    if (transform == newTransform)
        return;

    // The pixels move, so both what the component covered and what it covers now repaint
    repaint();
    transform = newTransform;
    repaint();

    const BailOutChecker bailOutChecker (this);

    transformChanged();

    if (! bailOutChecker.shouldBailOut())
        notifyPropertyChanged (ComponentIds::transform);
}

void Component::setStyleTransform (const AffineTransform& newTransform)
{
    const AffineTransform current = styleState != nullptr ? styleState->getStyleTransform() : AffineTransform();
    if (current == newTransform)
        return;

    repaint();
    getOrCreateStyleState().setStyleTransform (newTransform);
    repaint();

    const BailOutChecker bailOutChecker (this);

    transformChanged();

    if (! bailOutChecker.shouldBailOut())
        notifyPropertyChanged (ComponentIds::transform);
}

AffineTransform Component::getTransform() const
{
    return transform;
}

AffineTransform Component::getCombinedTransform() const
{
    if (styleState == nullptr || ! styleState->hasStyleTransform())
        return transform;

    if (transform.isIdentity())
        return styleState->getStyleTransform();

    return styleState->getStyleTransform().followedBy (transform);
}

bool Component::isTransformed() const
{
    return ! transform.isIdentity() || (styleState != nullptr && styleState->hasStyleTransform());
}

void Component::transformChanged()
{
}

//==============================================================================

bool Component::isFullScreen() const
{
    return flags.isFullScreen;
}

void Component::setFullScreen (bool shouldBeFullScreen)
{
    if (flags.isFullScreen == shouldBeFullScreen)
        return;

    flags.isFullScreen = shouldBeFullScreen;

    if (flags.onDesktop && native != nullptr)
        native->setFullScreen (shouldBeFullScreen);
}

void Component::setWindowAspectRatio (std::optional<float> aspectRatio)
{
    windowAspectRatio = aspectRatio;

    if (flags.onDesktop && native != nullptr)
        native->setAspectRatio (aspectRatio);
}

std::optional<float> Component::getWindowAspectRatio() const
{
    return windowAspectRatio;
}

//==============================================================================

void Component::displayChanged() {}

//==============================================================================

float Component::getScaleDpi() const
{
    if (flags.onDesktop && native != nullptr)
        return native->getScaleDpi();

    if (parentComponent == nullptr)
        return 1.0f;

    return parentComponent->getScaleDpi();
}

void Component::contentScaleChanged ([[maybe_unused]] float dpiScale) {}

//==============================================================================

void Component::setOpacity (float newOpacity)
{
    auto clampedOpacity = static_cast<uint8> (jlimit (0.0f, 1.0f, newOpacity) * 255);
    if (opacity == clampedOpacity)
        return;

    opacity = clampedOpacity;

    if (flags.onDesktop && native != nullptr)
        native->setOpacity (newOpacity);
    else
        repaint();

    const BailOutChecker bailOutChecker (this);

    opacityChanged();

    if (! bailOutChecker.shouldBailOut())
        notifyPropertyChanged (ComponentIds::opacity);
}

float Component::getOpacity() const
{
    return opacity / 255.0f;
}

void Component::opacityChanged() {}

//==============================================================================

bool Component::isOpaque() const
{
    return ! flags.isTransparent;
}

void Component::setOpaque (bool shouldBeOpaque)
{
    flags.isTransparent = ! shouldBeOpaque;
}

//==============================================================================

void Component::enableRenderingUnclipped (bool shouldBeEnabled)
{
    flags.unclippedRendering = shouldBeEnabled;
}

bool Component::isRenderingUnclipped() const
{
    return flags.unclippedRendering;
}

//==============================================================================

void Component::setPaintMargin (float left, float top, float right, float bottom)
{
    jassert (left >= 0.0f && top >= 0.0f && right >= 0.0f && bottom >= 0.0f);

    const EdgeSpacing<float> newMargin { jmax (0.0f, left), jmax (0.0f, top), jmax (0.0f, right), jmax (0.0f, bottom) };
    const auto hasChanged = paintMargin != newMargin;

    repaint();
    paintMargin = newMargin;
    repaint();

    if (hasChanged)
        notifyPropertyChanged (ComponentIds::paintMargin);
}

void Component::setPaintMargin (float margin)
{
    setPaintMargin (margin, margin, margin, margin);
}

void Component::setPaintMargin (const EdgeSpacing<float>& margin)
{
    setPaintMargin (margin.getLeft(), margin.getTop(), margin.getRight(), margin.getBottom());
}

EdgeSpacing<float> Component::getPaintMargin() const
{
    return paintMargin;
}

Rectangle<float> Component::getPaintBounds() const
{
    return getLocalBounds().enlarged (paintMargin);
}

EdgeSpacing<float> Component::getOffscreenMargin() const
{
    return flags.onDesktop ? EdgeSpacing<float>() : paintMargin;
}

Rectangle<float> Component::getEffectBounds (float scale) const
{
    return getOffscreenArea (getSize(), getOffscreenMargin(), scale, effectOffscreenCanvas, offscreenCanvasMargin).bounds;
}

void Component::setExcludedFromParentLayout (bool shouldBeExcluded)
{
    if (flags.excludedFromParentLayout == shouldBeExcluded)
        return;

    flags.excludedFromParentLayout = shouldBeExcluded;

    if (auto* parent = getParentComponent())
        parent->childLayoutChanged (*this);
}

bool Component::isExcludedFromParentLayout() const
{
    return flags.excludedFromParentLayout;
}

void Component::setPaintProfilingDisabled (bool shouldBeDisabled)
{
    flags.paintProfilingDisabled = shouldBeDisabled;
}

bool Component::isPaintProfilingDisabled() const
{
    return flags.paintProfilingDisabled;
}

void Component::repaint()
{
    repaint (getPaintBounds());
}

void Component::repaint (float x, float y, float width, float height)
{
    repaint ({ x, y, width, height });
}

void Component::repaint (const Rectangle<float>& rect)
{
    jassert (! isRepainting.load (std::memory_order_relaxed)); // You are likely repainting from paint !

    flags.cachedTextureStale = true;

    if (rect.isEmpty() || ! isShowing())
        return;

    internalRepaint (rect);
}

//==============================================================================

void* Component::getNativeHandle() const
{
    if (flags.onDesktop && native != nullptr)
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
    return flags.onDesktop;
}

void Component::addToDesktop (const ComponentNative::Options& nativeOptions, void* parent)
{
    YUP_ASSERT_MESSAGE_MANAGER_IS_LOCKED

    if (flags.onDesktop)
        removeFromDesktop();

    if (parentComponent != nullptr)
    {
        parentComponent->removeChildComponent (this);
        parentComponent = nullptr;
    }

    flags.onDesktop = true;

    native = ComponentNative::createFor (*this, nativeOptions, parent);

    if (windowAspectRatio.has_value() && native != nullptr)
        native->setAspectRatio (windowAspectRatio);

    internalAttachedToNative();

    setBounds (getBounds()); // This is needed to update based on scaleDpi
}

void Component::removeFromDesktop()
{
    YUP_ASSERT_MESSAGE_MANAGER_IS_LOCKED

    if (! flags.onDesktop)
        return;

    flags.onDesktop = false;

    // What the subtree keeps of the window is released while its rendering context is there to release it with
    releaseGpuResources (true);

    native.reset();

    internalDetachedFromNative();
}

//==============================================================================

void Component::toFront (bool shouldGainKeyboardFocus)
{
    if (flags.onDesktop && native != nullptr)
        native->toFront();

    if (parentComponent == nullptr)
        return;

    parentComponent->addChildComponent (this, parentComponent->getNumChildComponents());

    if (shouldGainKeyboardFocus && flags.wantsKeyboardFocus)
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

    if (auto* previousParent = component->parentComponent; previousParent != nullptr && previousParent != this)
        previousParent->removeChildComponent (component);

    component->parentComponent = this;

    const int currentIndex = children.indexOf (component);

    if (isPositiveAndBelow (currentIndex, children.size()))
    {
        if (currentIndex != index)
        {
            children.move (currentIndex, index);

            auto bailOutChecker = BailOutChecker (this);

            if (const int newIndex = children.indexOf (component); newIndex != currentIndex)
            {
                component->indexInParentChildrenChanged (currentIndex, newIndex);

                if (bailOutChecker.shouldBailOut())
                    return;
            }

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
        repaint (component->getPaintBounds().translated (component->getPosition()));

    auto* formerContext = component->styleContext == nullptr ? findStyleContext() : nullptr;

    component->parentComponent = nullptr;

    auto bailOutChecker = BailOutChecker (this);

    if (formerContext != nullptr)
    {
        if (component->flags.isBeingDeleted)
        {
            formerContext->forgetComponent (*component);
        }
        else
        {
            auto childBailOutChecker = BailOutChecker (component);
            formerContext->detachComponent (*component);

            if (bailOutChecker.shouldBailOut() || childBailOutChecker.shouldBailOut())
                return;
        }
    }

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
    if (auto* context = findStyleContext())
        context->markDirty (*this);

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

void Component::childLayoutChanged ([[maybe_unused]] Component& child) {}

void Component::layoutPropertyChanged() {}

void Component::indexInParentChildrenChanged ([[maybe_unused]] int oldIndex, [[maybe_unused]] int newIndex) {}

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

bool Component::hitTest (float x, float y)
{
    return getLocalBounds().contains (x, y);
}

Component* Component::findComponentAt (const Point<float>& p)
{
    if (! flags.isVisible || ! hitTest (p.getX(), p.getY()))
        return nullptr;

    for (int index = children.size(); --index >= 0;)
    {
        auto child = children.getUnchecked (index);
        if (! child->isVisible())
            continue;

        const auto childPoint = child->getLocalPointFromParent (p);
        if (! childPoint || ! child->getLocalBounds().contains (*childPoint))
            continue;

        if (auto found = child->findComponentAt (*childPoint))
            return found;
    }

    return this;
}

Component* Component::findComponentAtForMouseEvent (const Point<float>& p)
{
    if (! flags.isVisible || ! boundsInParent.withZeroPosition().contains (p) || ! hitTest (p.getX(), p.getY()))
        return nullptr;

    if (doesWantChildrenMouseEvents())
    {
        for (int index = children.size(); --index >= 0;)
        {
            auto child = children.getUnchecked (index);
            if (! child->isVisible())
                continue;

            const auto childPoint = child->getLocalPointFromParent (p);
            if (! childPoint)
                continue;

            if (auto* hit = child->findComponentAtForMouseEvent (*childPoint))
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

Component* Component::getPopupParentComponent()
{
    auto* popupParent = this;

    for (auto* ancestor = parentComponent; ancestor != nullptr; ancestor = ancestor->parentComponent)
    {
        popupParent = ancestor;

        if (ancestor->isTransformed() || ancestor->flags.manuallyComposited || ancestor->flags.popupContainer)
            break;
    }

    return popupParent;
}

//==============================================================================

void Component::setMouseCursor (const MouseCursor& cursorType)
{
    mouseCursor = cursorType;

    if (auto nativeComponent = getNativeComponent())
    {
        if (nativeComponent->getPointerTarget() == this)
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
    flags.wantsKeyboardFocus = wantsFocus;
}

bool Component::getWantsKeyboardFocus() const
{
    return flags.wantsKeyboardFocus;
}

void Component::setClickingGrabFocus (bool shouldGrabFocus)
{
    flags.clickingDoesNotGrabFocus = ! shouldGrabFocus;
}

bool Component::getClickingGrabFocus() const
{
    return ! flags.clickingDoesNotGrabFocus;
}

void Component::takeKeyboardFocus()
{
    takeKeyboardFocus (FocusChangeType::focusChangedDirectly);
}

void Component::takeKeyboardFocus (FocusChangeType cause)
{
    if (! flags.wantsKeyboardFocus || ! isEnabled())
        return;

    if (auto nativeComponent = getNativeComponent())
        nativeComponent->setFocusedComponent (this, cause);
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
    if (! flags.wantsKeyboardFocus || ! isEnabled())
        return false;

    if (auto nativeComponent = getNativeComponent())
        return nativeComponent->getFocusedComponent() == this;

    return false;
}

void Component::focusGained() {}

void Component::focusLost() {}

void Component::focusOfChildComponentChanged ([[maybe_unused]] Component* child, [[maybe_unused]] FocusChangeType cause) {}

//==============================================================================

void Component::handleKeyboardFocusFromClick()
{
    for (auto* component = this; component != nullptr; component = component->parentComponent)
    {
        if (component->flags.wantsKeyboardFocus && ! component->flags.clickingDoesNotGrabFocus)
        {
            component->takeKeyboardFocus (FocusChangeType::focusChangedByMouseClick);
            return;
        }
    }
}

//==============================================================================

const NamedValueSet& Component::getProperties() const
{
    return properties;
}

//==============================================================================

void Component::paint (Graphics& g)
{
    if (auto style = getStyle())
    {
        if (auto theme = ApplicationTheme::getGlobalTheme())
            style->paint (g, *theme, *this);

        return;
    }

    jassert (! isOpaque()); // If your component is opaque, you need to paint it !
}

void Component::paintOverChildren ([[maybe_unused]] Graphics& g) {}

//==============================================================================

void Component::setWantsMouseEvents (bool allowSelfMouseEvents, bool allowChildrenMouseEvents)
{
    flags.blockSelfMouseEvents = ! allowSelfMouseEvents;
    flags.blockChildrenMouseEvents = ! allowChildrenMouseEvents;
}

bool Component::doesWantSelfMouseEvents() const
{
    return ! flags.blockSelfMouseEvents;
}

bool Component::doesWantChildrenMouseEvents() const
{
    return ! flags.blockChildrenMouseEvents;
}

//==============================================================================

void Component::mouseEnter ([[maybe_unused]] const MouseEvent& event) {}

void Component::mouseExit ([[maybe_unused]] const MouseEvent& event) {}

void Component::mouseDown ([[maybe_unused]] const MouseEvent& event) {}

void Component::mouseMove ([[maybe_unused]] const MouseEvent& event) {}

void Component::mouseDrag ([[maybe_unused]] const MouseEvent& event) {}

void Component::mouseUp ([[maybe_unused]] const MouseEvent& event) {}

void Component::mouseDoubleClick ([[maybe_unused]] const MouseEvent& event) {}

bool Component::mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData)
{
    ignoreUnused (event, wheelData);
    return false;
}

//==============================================================================

void Component::setWantedGestures (GestureType gestureTypes)
{
    wantedGestures = gestureTypes;
}

GestureType Component::getWantedGestures() const
{
    return wantedGestures;
}

Component* Component::findGestureTargetAt (const Point<float>& position, GestureType gestureType)
{
    auto* target = findComponentAtForMouseEvent (position);

    while (target != nullptr && ! hasBitValueSet (target->wantedGestures, gestureType))
        target = (target == this) ? nullptr : target->getParentComponent();

    return target;
}

void Component::gestureTap ([[maybe_unused]] const GestureEvent& event) {}

void Component::gestureLongPress ([[maybe_unused]] const GestureEvent& event) {}

void Component::gesturePinch ([[maybe_unused]] const GestureEvent& event) {}

void Component::gestureRotate ([[maybe_unused]] const GestureEvent& event) {}

void Component::gesturePan ([[maybe_unused]] const GestureEvent& event) {}

void Component::gestureSwipe ([[maybe_unused]] const GestureEvent& event) {}

//==============================================================================

bool Component::keyDown ([[maybe_unused]] const KeyPress& keys, [[maybe_unused]] const Point<float>& position) { return false; }

bool Component::keyUp ([[maybe_unused]] const KeyPress& keys, [[maybe_unused]] const Point<float>& position) { return false; }

void Component::textInput ([[maybe_unused]] const String& text) {}

bool Component::keyStateChanged ([[maybe_unused]] const KeyPress& key, [[maybe_unused]] bool isDown) { return false; }

void Component::modifierKeysChanged (const KeyModifiers& modifiers)
{
    if (parentComponent != nullptr)
        parentComponent->modifierKeysChanged (modifiers);
}

//==============================================================================

void Component::addMouseListener (MouseListener* listener, bool wantsEventsForAllNestedChildComponents)
{
    removeMouseListener (listener);

    if (wantsEventsForAllNestedChildComponents)
        nestedMouseListeners.add (listener);
    else
        mouseListeners.add (listener);
}

void Component::removeMouseListener (MouseListener* listener)
{
    mouseListeners.remove (listener);
    nestedMouseListeners.remove (listener);
}

void Component::addGestureListener (GestureListener* listener)
{
    gestureListeners.add (listener);
}

void Component::removeGestureListener (GestureListener* listener)
{
    gestureListeners.remove (listener);
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

void Component::notifyPropertyChanged (const Identifier& name)
{
    const BailOutChecker bailOutChecker (this);

    componentListeners.callChecked (bailOutChecker, &ComponentListener::componentPropertyChanged, *this, name);

    if (bailOutChecker.shouldBailOut())
        return;

    if (auto* context = findStyleContext())
        context->propertyChanged (*this, name);
}

//==============================================================================

void Component::addStyleClass (const Identifier& styleClass)
{
    if (styleClasses.contains (styleClass))
        return;

    styleClasses.add (styleClass);

    markStyleDirty();
}

void Component::removeStyleClass (const Identifier& styleClass)
{
    if (! styleClasses.contains (styleClass))
        return;

    styleClasses.removeAllInstancesOf (styleClass);

    markStyleDirty();
}

bool Component::hasStyleClass (const Identifier& styleClass) const
{
    return styleClasses.contains (styleClass);
}

void Component::setStyleClasses (const Array<Identifier>& newStyleClasses)
{
    if (styleClasses == newStyleClasses)
        return;

    styleClasses = newStyleClasses;

    markStyleDirty();
}

const Array<Identifier>& Component::getStyleClasses() const noexcept
{
    return styleClasses;
}

//==============================================================================

bool Component::isHovered() const noexcept
{
    return flags.isHovered;
}

bool Component::isPressed() const noexcept
{
    return flags.isPressed;
}

void Component::setHovered (bool shouldBeHovered)
{
    setHoveredChain (shouldBeHovered);
}

void Component::setPressed (bool shouldBePressed)
{
    setPressedChain (shouldBePressed);
}

bool Component::isFocused() const noexcept
{
    return flags.isFocused;
}

bool Component::hasFocusWithin() const
{
    if (flags.isFocused)
        return true;

    if (auto* nativeComponent = getNativeComponent())
    {
        auto* focused = nativeComponent->getFocusedComponent();
        if (focused == nullptr || ! focused->isFocused())
            return false;

        for (auto* ancestor = focused->parentComponent; ancestor != nullptr; ancestor = ancestor->parentComponent)
            if (ancestor == this)
                return true;

        return false;
    }

    for (const auto* child : children)
        if (child->hasFocusWithin())
            return true;

    return false;
}

void Component::setHoveredChain (bool isNowHovered)
{
    for (auto* component = this; component != nullptr; component = component->parentComponent)
    {
        if (component->flags.isHovered == isNowHovered)
            break;

        component->flags.isHovered = isNowHovered;
        component->notifyPropertyChanged (ComponentIds::hovered);
    }
}

void Component::setPressedChain (bool isNowPressed)
{
    for (auto* component = this; component != nullptr; component = component->parentComponent)
    {
        if (component->flags.isPressed == isNowPressed)
            break;

        component->flags.isPressed = isNowPressed;
        component->notifyPropertyChanged (ComponentIds::pressed);
    }
}

void Component::clearHoveredInSubtree()
{
    if (! flags.isHovered)
        return;

    flags.isHovered = false;
    notifyPropertyChanged (ComponentIds::hovered);

    for (auto* child : children)
        child->clearHoveredInSubtree();
}

void Component::clearPressedInSubtree()
{
    if (! flags.isPressed)
        return;

    flags.isPressed = false;
    notifyPropertyChanged (ComponentIds::pressed);

    for (auto* child : children)
        child->clearPressedInSubtree();
}

//==============================================================================

void Component::setStyleSheet (ReferenceCountedObjectPtr<StyleSheet> newStyleSheet)
{
    if (newStyleSheet == nullptr)
    {
        if (styleContext == nullptr)
            return;

        styleContext->detach();
        styleContext.reset();
        markStyleDirty();
        return;
    }

    if (styleContext == nullptr)
    {
        if (parentComponent != nullptr)
            if (auto* outerContext = parentComponent->findStyleContext())
                outerContext->releaseSubtree (*this);

        styleContext = std::make_unique<StyleContext> (*this, std::move (newStyleSheet));
    }
    else
    {
        styleContext->setStyleSheet (std::move (newStyleSheet));
    }
}

ReferenceCountedObjectPtr<StyleSheet> Component::getStyleSheet() const
{
    return styleContext != nullptr ? styleContext->getStyleSheet() : nullptr;
}

StyleContext* Component::getStyleContext() const noexcept
{
    return styleContext.get();
}

StyleContext* Component::findStyleContext() const noexcept
{
    for (auto* component = this; component != nullptr; component = component->parentComponent)
        if (component->styleContext != nullptr)
            return component->styleContext.get();

    return nullptr;
}

const StyleState* Component::getStyleState() const noexcept
{
    return styleState.get();
}

StyleState& Component::getOrCreateStyleState()
{
    if (styleState == nullptr)
        styleState = std::make_unique<StyleState>();

    return *styleState;
}

void Component::markStyleDirty()
{
    if (auto* context = findStyleContext())
        context->markDirty (*this);
}

//==============================================================================

const ComponentTypeInfo& Component::getTypeInfo() const
{
    return instanceTypeInfo != nullptr ? *instanceTypeInfo : getDefaultTypeInfo();
}

void Component::setTypeInfo (const ComponentTypeInfo* newTypeInfo)
{
    const bool extendsTheClass = newTypeInfo == nullptr || newTypeInfo->isSubtypeOf (getDefaultTypeInfo());

    jassert (extendsTheClass); // The accessors of a type info take the component for one of the class it describes
    if (! extendsTheClass)
        return;

    if (instanceTypeInfo == newTypeInfo)
        return;

    instanceTypeInfo = newTypeInfo;

    markStyleDirty();
}

bool Component::paintPart (Graphics& g, const Identifier& partName) const
{
    const auto* part = getTypeInfo().findPart (partName);

    return part != nullptr && ApplicationTheme::paintComponentPart (g, *this, partName, part->getArea (*this));
}

bool Component::paintPart (Graphics& g, const Identifier& partName, const Rectangle<float>& area) const
{
    const auto* part = getTypeInfo().findPart (partName);

    return part != nullptr && ApplicationTheme::paintComponentPart (g, *this, partName, part->isPlaceable ? area : part->getArea (*this));
}

const ComponentTypeInfo& Component::getClassTypeInfo()
{
    using Type = ComponentProperty::Type;
    using LayoutEffect = ComponentProperty::LayoutEffect;
    const auto& selfAlignments = detail::layoutSelfAlignmentNames();

    static const auto info = ComponentTypeBuilder<Component> (Identifier ("Component"))
                                 .factory ([]
                                 {
                                     // A plain component paints nothing, so a created one must not claim to be opaque
                                     auto component = std::make_unique<Component>();
                                     component->setOpaque (false);
                                     return component;
                                 })
                                 .readOnly (ComponentIds::id, &Component::getComponentID)
                                 .property (ComponentIds::title, &Component::getTitle, &Component::setTitle)
                                 .styleableProperty (ComponentIds::visible, &Component::isVisible, &Component::setVisible, true)
                                 .styleableProperty (ComponentIds::enabled, [] (const Component& c) { return ! c.flags.isDisabled; }, &Component::setEnabled, true)
                                 .styleableProperty (ComponentIds::opacity, &Component::getOpacity, &Component::setOpacity, 1.0)
                                 .styleableProperty (ComponentIds::transform, &Component::getTransform, &Component::setTransform, VariantConverter<AffineTransform>::toVar (AffineTransform()))
                                 .styleableProperty (ComponentIds::cached, &Component::isCachedToTexture, &Component::setCachedToTexture, false, "Keeps what the component paints in a texture, painted again only when the component is repainted.")
                                 .addProperty (ComponentProperty {
                                     .name = ComponentIds::paintMargin,
                                     .type = Type::any,
                                     .defaultValue = 0.0,
                                     .getter = [] (const Component& c)
                                     {
                                         return VariantConverter<EdgeSpacing<float>>::toVar (c.getPaintMargin());
                                     },
                                     .setter = [] (Component& c, const var& v)
                                     {
                                         c.setPaintMargin (LayoutComponent::parseSpacing (v));
                                     },
                                     .isInherited = false,
                                     .description = "How far the component may paint outside its bounds, for a glow or a shadow it draws itself: a number, or a string of 1, 2 or 4 values (top right bottom left)." })
                                 .property (ComponentIds::x, &Component::getX, [] (Component& c, float v) { c.setBounds (v, c.getY(), c.getWidth(), c.getHeight()); })
                                 .property (ComponentIds::y, &Component::getY, [] (Component& c, float v) { c.setBounds (c.getX(), v, c.getWidth(), c.getHeight()); })
                                 .property (ComponentIds::width, &Component::getWidth, [] (Component& c, float v) { c.setBounds (c.getX(), c.getY(), v, c.getHeight()); })
                                 .property (ComponentIds::height, &Component::getHeight, [] (Component& c, float v) { c.setBounds (c.getX(), c.getY(), c.getWidth(), v); })
                                 .property (ComponentIds::bounds, &Component::getBounds, [] (Component& c, const Rectangle<float>& v) { c.setBounds (v); })
                                 .state (ComponentIds::focused, &Component::isFocused, false)
                                 .state (ComponentIds::hovered, &Component::isHovered, false)
                                 .state (ComponentIds::pressed, &Component::isPressed, false)
                                 .layoutProperty ("flexGrow", Type::number, 0.0, LayoutEffect::parent, "How much of the free space along the main axis the item takes.")
                                 .layoutProperty ("flexShrink", Type::number, 1.0, LayoutEffect::parent, "How much the item shrinks when the main axis overflows.")
                                 .layoutProperty ("flexBasis", Type::number, -1.0, LayoutEffect::parent, "The initial main size of the item, negative for automatic.")
                                 .layoutProperty ("order", Type::integer, 0, LayoutEffect::parent, "The order of the item in a flex container.")
                                 .layoutProperty ("alignSelf", Type::enumeration, "auto", LayoutEffect::parent, "The cross axis alignment of the item, overriding the container one.", selfAlignments)
                                 .layoutProperty ("justifySelf", Type::enumeration, "auto", LayoutEffect::parent, "The horizontal alignment of the item in a grid or stack cell.", selfAlignments)
                                 .layoutProperty ("margin", Type::any, 0.0, LayoutEffect::parent, "The outer spacing of the item: a number, or 1, 2 or 4 values (top right bottom left).")
                                 .layoutProperty ("preferredWidth", Type::number, -1.0, LayoutEffect::parent, "The width the layout gives the item, negative for automatic.")
                                 .layoutProperty ("preferredHeight", Type::number, -1.0, LayoutEffect::parent, "The height the layout gives the item, negative for automatic.")
                                 .layoutProperty ("minWidth", Type::number, -1.0, LayoutEffect::parent, "The minimum width of the item, negative for none.")
                                 .layoutProperty ("maxWidth", Type::number, -1.0, LayoutEffect::parent, "The maximum width of the item, negative for none.")
                                 .layoutProperty ("minHeight", Type::number, -1.0, LayoutEffect::parent, "The minimum height of the item, negative for none.")
                                 .layoutProperty ("maxHeight", Type::number, -1.0, LayoutEffect::parent, "The maximum height of the item, negative for none.")
                                 .layoutProperty ("gridArea", Type::string, String(), LayoutEffect::parent, "The named grid area the item occupies.")
                                 .layoutProperty ("gridColumn", Type::integer, -1, LayoutEffect::parent, "The zero based grid column of the item, negative for automatic placement.")
                                 .layoutProperty ("gridRow", Type::integer, -1, LayoutEffect::parent, "The zero based grid row of the item, negative for automatic placement.")
                                 .layoutProperty ("gridColumnSpan", Type::integer, 1, LayoutEffect::parent, "The number of grid columns the item spans.")
                                 .layoutProperty ("gridRowSpan", Type::integer, 1, LayoutEffect::parent, "The number of grid rows the item spans.")
                                 .build();

    return info;
}

//==============================================================================

struct Component::ResolvedProperty
{
    const ComponentProperty* descriptor = nullptr;
    Identifier name;
    Identifier storageKey;
};

Component::ResolvedProperty Component::resolveProperty (const Identifier& name) const
{
    const auto* descriptor = getTypeInfo().findProperty (name);
    return { descriptor, name, descriptor != nullptr ? descriptor->getStorageKey() : name };
}

Component::ResolvedProperty Component::resolveColorProperty (const Identifier& colorId) const
{
    const auto& info = getTypeInfo();
    const auto* descriptor = info.findPropertyByStorageKey (colorId);

    if (descriptor == nullptr)
    {
        if (const auto* byName = info.findProperty (colorId); byName != nullptr && byName->type == ComponentProperty::Type::color)
            descriptor = byName;
    }

    if (descriptor != nullptr)
        return { descriptor, descriptor->name, descriptor->getStorageKey() };

    return { nullptr, colorId, colorId };
}

const var* Component::findStoredValue (const Identifier& storageKey, bool isInherited) const
{
    for (auto* component = this; component != nullptr; component = component->parentComponent)
    {
        if (const auto* value = component->properties.getVarPointer (storageKey))
            return value;

        if (component->styleState != nullptr)
            if (const auto* styled = component->styleState->getStyleValue (storageKey))
                return styled;

        if (! isInherited)
            break;
    }

    return nullptr;
}

void Component::applyStoredPropertyChange (const ResolvedProperty& resolved, const var* valueToWrite)
{
    const bool changed = valueToWrite != nullptr
                           ? properties.set (resolved.storageKey, *valueToWrite)
                           : properties.remove (resolved.storageKey);

    if (! changed)
        return;

    const BailOutChecker bailOutChecker (this);

    if (resolved.descriptor != nullptr)
    {
        if (resolved.descriptor->layoutEffect == ComponentProperty::LayoutEffect::own)
        {
            layoutPropertyChanged();
            if (bailOutChecker.shouldBailOut())
                return;
        }
        else if (resolved.descriptor->layoutEffect == ComponentProperty::LayoutEffect::parent && parentComponent != nullptr)
        {
            parentComponent->childLayoutChanged (*this);
            if (bailOutChecker.shouldBailOut())
                return;
        }
    }

    styleChanged();
    if (bailOutChecker.shouldBailOut())
        return;

    notifyPropertyChanged (resolved.name);
    if (bailOutChecker.shouldBailOut())
        return;

    repaint();
}

void Component::setProperty (const Identifier& name, const var& value)
{
    const auto resolved = resolveProperty (name);

    if (resolved.descriptor != nullptr && resolved.descriptor->isStateBacked())
    {
        if (resolved.descriptor->setter != nullptr)
            resolved.descriptor->setter (*this, value);
        else
            jassertfalse; // Trying to write a read-only property.

        return;
    }

    applyStoredPropertyChange (resolved, &value);
}

void Component::removeProperty (const Identifier& name)
{
    const auto resolved = resolveProperty (name);

    if (resolved.descriptor != nullptr && resolved.descriptor->isStateBacked())
        return; // State-backed properties cannot be removed.

    applyStoredPropertyChange (resolved, nullptr);
}

var Component::getProperty (const Identifier& name) const
{
    const auto resolved = resolveProperty (name);

    if (resolved.descriptor != nullptr && resolved.descriptor->isStateBacked())
        return resolved.descriptor->getter (*this);

    if (const auto* value = properties.getVarPointer (resolved.storageKey))
        return *value;

    return {};
}

var Component::findProperty (const Identifier& name) const
{
    if (name == ComponentIds::transform)
        return VariantConverter<AffineTransform>::toVar (getCombinedTransform());

    if (name == ComponentIds::enabled)
        return isEnabled();

    const auto resolved = resolveProperty (name);

    if (resolved.descriptor != nullptr && resolved.descriptor->isStateBacked())
        return resolved.descriptor->getter (*this);

    const bool isInherited = resolved.descriptor == nullptr || resolved.descriptor->isInherited;
    if (const auto* stored = findStoredValue (resolved.storageKey, isInherited))
        return *stored;

    if (auto theme = ApplicationTheme::getGlobalTheme())
    {
        // Theme colors are kept apart from the other defaults, together with the palette ones
        if (resolved.descriptor != nullptr && resolved.descriptor->type == ComponentProperty::Type::color)
            if (auto color = theme->findDefaultColor (*this, resolved.storageKey))
                return VariantConverter<Color>::toVar (*color);

        if (auto themed = theme->findProperty (*this, resolved.storageKey))
            return *themed;
    }

    if (resolved.descriptor != nullptr)
        return resolved.descriptor->defaultValue;

    return {};
}

Component::PropertySource Component::findPropertySource (const Identifier& name) const
{
    PropertySource source;

    if (name == ComponentIds::transform)
    {
        source.layer = PropertySource::Layer::stateBacked;
        source.component = this;
        source.storageKey = name;
        source.value = VariantConverter<AffineTransform>::toVar (getCombinedTransform());
        return source;
    }

    const auto resolved = resolveProperty (name);
    source.descriptor = resolved.descriptor;
    source.storageKey = resolved.storageKey;

    if (resolved.descriptor != nullptr && resolved.descriptor->isStateBacked())
    {
        source.layer = PropertySource::Layer::stateBacked;
        source.component = this;
        source.value = name == ComponentIds::enabled ? var (isEnabled()) : resolved.descriptor->getter (*this);
        return source;
    }

    const bool isInherited = resolved.descriptor == nullptr || resolved.descriptor->isInherited;

    for (const auto* component = this; component != nullptr; component = component->parentComponent)
    {
        if (const auto* value = component->properties.getVarPointer (resolved.storageKey))
        {
            source.layer = PropertySource::Layer::explicitly;
            source.component = component;
            source.value = *value;
            return source;
        }

        if (component->styleState != nullptr)
        {
            if (const auto* styled = component->styleState->getStyleValue (resolved.storageKey))
            {
                source.layer = PropertySource::Layer::styled;
                source.component = component;
                source.value = *styled;
                return source;
            }
        }

        if (! isInherited)
            break;
    }

    if (auto theme = ApplicationTheme::getGlobalTheme())
    {
        if (resolved.descriptor != nullptr && resolved.descriptor->type == ComponentProperty::Type::color)
        {
            if (auto color = theme->findDefaultColor (*this, resolved.storageKey))
            {
                source.layer = PropertySource::Layer::theme;
                source.value = VariantConverter<Color>::toVar (*color);
                return source;
            }
        }

        if (auto themed = theme->findProperty (*this, resolved.storageKey))
        {
            source.layer = PropertySource::Layer::theme;
            source.value = *themed;
            return source;
        }
    }

    if (resolved.descriptor != nullptr)
    {
        source.layer = PropertySource::Layer::defaulted;
        source.value = resolved.descriptor->defaultValue;
    }

    return source;
}

bool Component::hasProperty (const Identifier& name) const
{
    const auto resolved = resolveProperty (name);

    if (resolved.descriptor != nullptr && resolved.descriptor->isStateBacked())
        return true;

    return properties.contains (resolved.storageKey);
}

//==============================================================================

void Component::setColor (const Identifier& colorId, const std::optional<Color>& color)
{
    const auto resolved = resolveColorProperty (colorId);

    if (color.has_value())
    {
        const var encoded (static_cast<int64> (color->getARGB()));
        applyStoredPropertyChange (resolved, &encoded);
    }
    else
    {
        applyStoredPropertyChange (resolved, nullptr);
    }
}

std::optional<Color> Component::getColor (const Identifier& colorId) const
{
    const auto resolved = resolveColorProperty (colorId);

    if (const auto* color = properties.getVarPointer (resolved.storageKey); color != nullptr && color->isInt64())
        return Color (static_cast<uint32> (static_cast<int64> (*color)));

    return std::nullopt;
}

std::optional<Color> Component::findColor (const Identifier& colorId) const
{
    const auto resolved = resolveColorProperty (colorId);
    const bool isInherited = resolved.descriptor == nullptr || resolved.descriptor->isInherited;

    if (const auto* stored = findStoredValue (resolved.storageKey, isInherited); stored != nullptr && stored->isInt64())
        return Color (static_cast<uint32> (static_cast<int64> (*stored)));

    return std::nullopt;
}

//==============================================================================

void Component::setPalette (std::optional<ThemePalette> newPalette)
{
    palette = newPalette.has_value() ? std::make_unique<ThemePalette> (std::move (*newPalette)) : nullptr;

    markStyleDirty();
    sendPaletteChanged();
}

void Component::sendPaletteChanged()
{
    const BailOutChecker bailOutChecker (this);

    styleChanged();
    if (bailOutChecker.shouldBailOut())
        return;

    notifyPropertyChanged (ComponentIds::palette);
    if (bailOutChecker.shouldBailOut())
        return;

    repaint();
    if (bailOutChecker.shouldBailOut())
        return;

    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        auto* child = getChildComponent (i);
        if (child == nullptr || child->palette != nullptr)
            continue;

        if (child->styleState != nullptr && child->styleState->getStylePalette() != nullptr)
            continue;

        child->sendPaletteChanged();
        if (bailOutChecker.shouldBailOut())
            return;
    }
}

const ThemePalette* Component::getPalette() const
{
    return palette.get();
}

const ThemePalette* Component::findPalette() const
{
    for (auto* component = this; component != nullptr; component = component->parentComponent)
    {
        if (component->palette != nullptr)
            return component->palette.get();

        if (component->styleState != nullptr)
            if (auto* stylePalette = component->styleState->getStylePalette())
                return stylePalette;
    }

    return nullptr;
}

//==============================================================================

void Component::setComponentEffect (ComponentEffect::Ptr effect)
{
    // An effect tells the components it is set on when what it draws changed
    if (componentEffect != nullptr)
        componentEffect->components.removeFirstMatchingValue (this);

    componentEffect = std::move (effect);

    if (componentEffect != nullptr)
        componentEffect->components.add (this);

    releaseUnusedEffectCanvases();
    repaint();
}

ComponentEffect::Ptr Component::getComponentEffect() const
{
    return componentEffect;
}

ComponentEffect* Component::getActiveComponentEffect() const
{
    return (componentEffect != nullptr && componentEffect->isActive()) ? componentEffect.get() : nullptr;
}

bool Component::hasActiveEffect() const noexcept
{
    return getActiveComponentEffect() != nullptr || (styleState != nullptr && styleState->getFilterEffect() != nullptr);
}

void Component::releaseUnusedEffectCanvases() noexcept
{
    if (componentEffect == nullptr || styleState == nullptr || styleState->getFilterEffect() == nullptr)
        filterIntermediateCanvas = nullptr;

    if (! hasActiveEffect())
        effectOffscreenCanvas = nullptr;
}

void Component::releaseGpuResources (bool withChildren)
{
    // The canvases, and what the effects and the drawing keep, belong to the device of the window: kept past it they
    // would hold that device alive, and be released without its rendering context. The next paint makes them again
    cachedTextureCanvas = nullptr;
    effectOffscreenCanvas = nullptr;
    filterIntermediateCanvas = nullptr;
    presentedCanvas = nullptr;

    if (componentEffect != nullptr)
        componentEffect->releaseResources();

    if (styleState != nullptr && styleState->getFilterEffect() != nullptr)
        styleState->getFilterEffect()->releaseResources();

    if (auto* drawing = dynamic_cast<DrawingStyle*> (style.get()))
        drawing->releaseResources();

    if (! withChildren)
        return;

    for (auto* child : children)
        child->releaseGpuResources (true);
}

void Component::applyActiveEffects (Graphics& g, GpuTexture::Ptr texture, Rectangle<float> bounds, float scale)
{
    auto* filterEffect = styleState != nullptr ? styleState->getFilterEffect().get() : nullptr;
    auto* effect = getActiveComponentEffect();

    if (effect != nullptr && filterEffect != nullptr && texture != nullptr)
    {
        const int width = texture->getWidth();
        const int height = texture->getHeight();

        if (filterIntermediateCanvas == nullptr
            || filterIntermediateCanvas->getWidth() != width
            || filterIntermediateCanvas->getHeight() != height)
        {
            filterIntermediateCanvas = GpuCanvas::create (g.getGraphicsContext(), width, height);
        }

        if (filterIntermediateCanvas != nullptr)
        {
            // The canvas starts where the bounds do, so the effect gets the bounds it maps input over
            auto& intermediateG = filterIntermediateCanvas->beginDraw ({}, scale);
            intermediateG.setDrawingArea (getLocalBounds().withPosition (-bounds.getPosition()));
            effect->apply (intermediateG, texture, bounds);
            filterIntermediateCanvas->commit();

            if (auto postEffectTexture = filterIntermediateCanvas->asTexture())
            {
                filterEffect->apply (g, std::move (postEffectTexture), bounds);
                return;
            }
        }
    }
    else
    {
        filterIntermediateCanvas = nullptr;
    }

    if (effect != nullptr)
        effect->apply (g, std::move (texture), bounds);
    else if (filterEffect != nullptr)
        filterEffect->apply (g, std::move (texture), bounds);
}

void Component::setCachedToTexture (bool shouldCache)
{
    if (flags.cachedToTexture == shouldCache)
        return;

    flags.cachedToTexture = shouldCache;
    cachedTextureCanvas = nullptr;
    repaint();

    notifyPropertyChanged (ComponentIds::cached);
}

bool Component::isCachedToTexture() const
{
    return flags.cachedToTexture;
}

//==============================================================================

void Component::setManuallyComposited (bool shouldBeManuallyComposited)
{
    if (flags.manuallyComposited == shouldBeManuallyComposited)
        return;

    flags.manuallyComposited = shouldBeManuallyComposited;

    if (! shouldBeManuallyComposited)
        presentedCanvas = nullptr;

    repaint();
}

bool Component::isManuallyComposited() const
{
    return flags.manuallyComposited;
}

void Component::setPopupContainer (bool shouldBePopupContainer)
{
    flags.popupContainer = shouldBePopupContainer;
}

bool Component::isPopupContainer() const
{
    return flags.popupContainer;
}

GpuTexture::Ptr Component::renderToTexture (GraphicsContext& ctx, float scale)
{
    invalidateCanvasesIfThemeChanged();

    const bool sizeChanged = ! hasPixelSize (presentedCanvas, getCanvasPixelSize (getSize(), scale));

    if (! subtreeDirty.exchange (false) && ! sizeChanged)
        return presentedCanvas->asTexture();

    presentedCanvas = renderSnapshotOffscreen (ctx, true, scale, std::move (presentedCanvas));
    if (presentedCanvas == nullptr)
    {
        subtreeDirty = true;
        return nullptr;
    }

    return presentedCanvas->asTexture();
}

//==============================================================================

GpuCanvas::Ptr Component::renderSnapshotOffscreen (GraphicsContext& ctx, bool includeEffects, float scale, GpuCanvas::Ptr reuseCanvas)
{
    if (getWidth() <= 0.0f || getHeight() <= 0.0f)
        return nullptr;

    const auto renderSnapshot = [&] () -> GpuCanvas::Ptr
    {
        const bool applyEffect = includeEffects && hasActiveEffect();

        auto canvas = renderSubtreeOffscreen (ctx, getOpacity(), false, scale, applyEffect ? nullptr : std::move (reuseCanvas));
        if (canvas == nullptr)
            return nullptr;

        if (! applyEffect)
            return canvas;

        auto texture = canvas->asTexture();

        auto effectCanvas = std::move (reuseCanvas);
        if (effectCanvas == nullptr || effectCanvas->getWidth() != canvas->getWidth() || effectCanvas->getHeight() != canvas->getHeight())
            effectCanvas = GpuCanvas::create (ctx, canvas->getWidth(), canvas->getHeight());

        if (effectCanvas == nullptr)
            return canvas;

        auto& g = effectCanvas->beginDraw ({}, scale);
        auto localBounds = getLocalBounds();
        g.setDrawingArea (localBounds);
        applyActiveEffects (g, texture, localBounds, scale);

        return effectCanvas;
    };

    if (auto* nativeComponent = getNativeComponent())
    {
        GpuCanvas::Ptr canvas;
        nativeComponent->runWithGraphicsContext ([&] { canvas = renderSnapshot(); });
        return canvas;
    }

    return renderSnapshot();
}

Image Component::snapshotToImage (GraphicsContext& ctx, bool includeEffects, float scale)
{
    jassert (scale > 0.0f);
    if (scale <= 0.0f)
        return {};

    Image result;
    const auto takeSnapshot = [&]
    {
        auto canvas = renderSnapshotOffscreen (ctx, includeEffects, scale);
        if (canvas == nullptr)
            return;

        result = canvas->asImage();
    };

    if (auto* nativeComponent = getNativeComponent())
        nativeComponent->runWithGraphicsContext (takeSnapshot);
    else
        takeSnapshot();

    return result;
}

GpuTexture::Ptr Component::snapshotToTexture (GraphicsContext& ctx, bool includeEffects)
{
    GpuTexture::Ptr result;
    const auto takeSnapshot = [&]
    {
        auto canvas = renderSnapshotOffscreen (ctx, includeEffects, 1.0f);
        if (canvas == nullptr)
            return;

        result = canvas->asTexture();
    };

    if (auto* nativeComponent = getNativeComponent())
        nativeComponent->runWithGraphicsContext (takeSnapshot);
    else
        takeSnapshot();

    return result;
}

//==============================================================================

void Component::userTriedToCloseWindow() {}

//==============================================================================

bool Component::isOpaqueForPainting() const noexcept
{
    // A style set on the component paints what it likes of it, so what is behind has to be painted
    return ! flags.isTransparent && style == nullptr;
}

bool Component::hasOpaqueChildCoveringArea (const Rectangle<float>& area)
{
    for (int childIndex = children.size(); --childIndex >= 0;)
    {
        auto child = children.getUnchecked (childIndex);
        if (! child->isVisible() || ! child->isOpaqueForPainting() || child->getOpacity() < 1.0f || child->hasActiveEffect() || child->flags.unclippedRendering || child->flags.manuallyComposited || child->isTransformed())
            continue;

        auto childBounds = child->getBoundsRelativeToTopLevelComponent();
        if (childBounds.contains (area))
            return true;
    }

    return false;
}

//==============================================================================

void Component::internalRepaint()
{
    internalRepaint (getPaintBounds());
}

void Component::internalRepaint (const Rectangle<float>& rect, bool contentChanged)
{
    if (rect.isEmpty())
        return;

    auto dirtyArea = rect;

    for (auto* component = this;; component = component->parentComponent)
    {
        component->subtreeDirty = true;

        const bool hasEffect = component->hasActiveEffect();

        // What changed underneath an effect is kept, so only that is painted again in what the component keeps of its
        // subtree: also while its effect has nothing to apply, which it may have again before the next paint lets go
        // of what was kept. An effect that is only applied again leaves what its own component painted as it is
        if ((hasEffect || component->effectOffscreenCanvas != nullptr) && (contentChanged || component != this))
            component->effectDirtyRegion.add (dirtyArea);

        // A warp can move pixels anywhere inside the component, so the whole of it is presented again
        if (hasEffect)
            dirtyArea = component->getPaintBounds();

        auto* parent = component->parentComponent;

        if (parent != nullptr && component->flags.manuallyComposited)
            dirtyArea = parent->getLocalBounds();
        else if (! component->flags.onDesktop)
            dirtyArea = getTransformedBounds (dirtyArea, component->getCombinedTransform().translated (component->getPosition()));

        if (parent == nullptr)
        {
            if (component->native != nullptr)
                component->native->repaint (dirtyArea);

            return;
        }
    }
}

void Component::repaintEffect()
{
    // An effect that has nothing to apply draws nothing again: the component is painted as it is
    if (! hasActiveEffect())
    {
        repaint();
        return;
    }

    jassert (! isRepainting.load (std::memory_order_relaxed)); // An effect can't repaint from apply() !

    if (isShowing())
        internalRepaint (getPaintBounds(), false);
}

//==============================================================================

void Component::paintChildrenAndOverChildren (Graphics& g,
                                              const RectangleList<float>& clipRegion,
                                              bool renderContinuous,
                                              const RectangleList<float>* activeClipRegion)
{
    for (auto child : children)
        child->internalPaint (g, clipRegion, renderContinuous, activeClipRegion);

    paintOverChildren (g);
}

GpuCanvas::Ptr Component::renderSubtreeOffscreen (GraphicsContext& ctx, float opacity, bool renderContinuous, float scale, GpuCanvas::Ptr reuseCanvas, const std::optional<RectangleList<float>>& dirtyRegion, bool holdPaintMargin)
{
    if (getWidth() <= 0.0f || getHeight() <= 0.0f)
        return nullptr;

    const auto renderOffscreen = [&] () -> GpuCanvas::Ptr
    {
        const auto margin = holdPaintMargin ? getOffscreenMargin() : EdgeSpacing<float>();
        const auto area = holdPaintMargin ? getOffscreenArea (getSize(), margin, scale, reuseCanvas, offscreenCanvasMargin)
                                          : getOffscreenArea (getSize(), margin, scale);
        const auto reused = hasPixelSize (reuseCanvas, area.pixelSize);

        GpuCanvas::Ptr canvas;
        if (reused)
        {
            canvas = std::move (reuseCanvas);
        }
        else
        {
            reuseCanvas = nullptr;
            canvas = createOffscreenCanvas (ctx, area.pixelSize);
        }

        if (canvas == nullptr)
            return nullptr;

        std::optional<RectangleList<float>> dirtyPixels;

        // What was painted around the bounds is not painted over, so a new margin is painted in full. So is what
        // was painted at another scale, which can take the same pixels
        if (reused && dirtyRegion.has_value() && margin == offscreenPaintedMargin && scale == offscreenPaintedScale)
            dirtyPixels = getPartialRepaintRegion (*dirtyRegion, area, getSize(), scale);

        if (holdPaintMargin)
        {
            offscreenCanvasMargin = area.pixelMargin;
            offscreenPaintedMargin = margin;
            offscreenPaintedScale = scale;
        }

        if (dirtyPixels.has_value() && dirtyPixels->isEmpty())
            return canvas;

        GpuFrameDescriptor frame;

        if (dirtyPixels.has_value())
            frame.loadOp = GpuLoadOp::load;

        auto& offscreenG = canvas->beginDraw (frame, scale);

        const auto clipRegion = dirtyPixels.value_or (RectangleList<float> { area.clip });

        flags.paintAsOffscreenRoot = true;
        offscreenOrigin = area.origin;

        paintSubtree (offscreenG, clipRegion, opacity, renderContinuous);

        offscreenOrigin = {};
        flags.paintAsOffscreenRoot = false;

        canvas->commit();
        return canvas;
    };

    if (auto* nativeComponent = getNativeComponent())
    {
        GpuCanvas::Ptr canvas;
        nativeComponent->runWithGraphicsContext ([&] { canvas = renderOffscreen(); });
        return canvas;
    }

    return renderOffscreen();
}

void Component::markEffectCanvasDirty()
{
    if (hasActiveEffect() || effectOffscreenCanvas != nullptr)
        effectDirtyRegion.add (getPaintBounds());
}

void Component::invalidateCanvasesIfThemeChanged()
{
    const auto theme = ApplicationTheme::getGlobalTheme();
    const auto generation = theme != nullptr ? theme->getPaletteGeneration() : 0;

    if (std::exchange (canvasThemeGeneration, generation) == generation)
        return;

    // What the component keeps of its painting was painted with the theme as it was
    flags.cachedTextureStale = true;
    subtreeDirty = true;
    markEffectCanvasDirty();
}

std::optional<RectangleList<float>> Component::takeEffectDirtyRegion (bool renderContinuous)
{
    RectangleList<float> region;
    std::swap (region, effectDirtyRegion);

    // A window rendering continuously paints everything at every frame, and no repaint is kept of a component
    // that is not showing: nothing tells what changed in it
    if (renderContinuous || ! isShowing())
        return std::nullopt;

    // With nothing to paint again what was kept is still good, whatever the component
    if (region.isEmpty())
        return region;

    // What is painted over what was kept has to cover it: a component that is not opaque would blend
    // into it, and one that is not clipped paints past what is dirty
    if (! isOpaqueForPainting() || flags.unclippedRendering)
        return std::nullopt;

    const auto bounds = getLocalBounds();
    const bool hasMargin = getOffscreenMargin() != EdgeSpacing<float>();
    RectangleList<float> clipped;

    for (const auto& rect : region.getRectangles())
    {
        if (hasMargin && ! bounds.contains (rect))
            return std::nullopt;

        const auto inside = rect.intersection (bounds).smallestIntContainer();

        if (! inside.isEmpty())
            clipped.add (inside);
    }

    if (clipped.getNumRectangles() > maximumDirtyRectangles)
    {
        const auto box = clipped.getBoundingBox();
        clipped.clearQuick();
        clipped.add (box);
    }

    return clipped;
}

//==============================================================================

void Component::applyPaintState (Graphics& g, const RectangleList<float>& clipRegion, const RectangleList<float>* activeClipRegion) const
{
    const auto toTopLevel = getTransformToTopLevelComponent();

    // The clip region is in top-level coordinates, while the parent's drawing area is still set
    g.setDrawingArea ({});
    g.setTransform (AffineTransform::identity());

    if (! flags.unclippedRendering && (activeClipRegion == nullptr || *activeClipRegion != clipRegion))
        setClipRegion (g, clipRegion);

    // The translation goes in the drawing area and only the linear part in the transform, so an
    // untransformed hierarchy keeps painting in exactly the same state it always had
    g.setDrawingArea (getLocalBounds().withPosition (toTopLevel.getTranslation()));
    g.setTransform (toTopLevel.withAbsoluteTranslation (0.0f, 0.0f));

    // Under rotation or shear the clip region only holds the bounding box, so clip to the real
    // outline too. Children paint inside this state, so they inherit it.
    const bool isAxisAligned = approximatelyEqual (toTopLevel.getShearX(), 0.0f) && approximatelyEqual (toTopLevel.getShearY(), 0.0f);
    if (! flags.unclippedRendering && ! isAxisAligned)
        g.setClipPath (getPaintBounds());
}

void Component::paintStyleShadows (Graphics& g) const
{
    if (styleState == nullptr || (styleState->getShadows().empty() && styleState->getGlows().empty()))
        return;

    static const Identifier cornerRadiusId ("cornerRadius");
    styleState->paintShadows (g, getLocalBounds(), findPropertyAs<float> (cornerRadiusId));
}

void Component::paintSubtree (Graphics& g,
                              const RectangleList<float>& clipRegion,
                              float opacity,
                              bool renderContinuous,
                              const RectangleList<float>* activeClipRegion)
{
    isRepainting.store (true, std::memory_order_relaxed);

    const ErasedScopeGuard clearRepaintingFlag ([this]
    {
        isRepainting.store (false, std::memory_order_relaxed);
    });

    {
        const bool shouldMeasurePaint = ! flags.paintProfilingDisabled && ! componentListeners.isEmpty();
        const auto toTopLevel = getTransformToTopLevelComponent();

        ComponentPaintMetrics metrics;
        int64 totalStartTicks = 0;
        int64 selfStartTicks = 0;

        if (shouldMeasurePaint)
        {
            const auto topLevelBounds = getTransformedBounds (getLocalBounds(), toTopLevel);

            totalStartTicks = Time::getHighResolutionTicks();
            metrics.repaintArea = topLevelBounds;
            metrics.componentBounds = topLevelBounds;
            metrics.renderContinuous = renderContinuous;
        }

        const auto globalState = g.saveState();

        g.setOpacity (opacity);
        applyPaintState (g, clipRegion, activeClipRegion);

        const auto* subtreeActiveClip = flags.unclippedRendering ? activeClipRegion : &clipRegion;

        bool canSkipPaint = false;
        if (! flags.unclippedRendering && toTopLevel.isOnlyTranslation() && clipRegion.getNumRectangles() == 1)
            canSkipPaint = hasOpaqueChildCoveringArea (clipRegion.getRectangles()[0]);

        if (! canSkipPaint)
        {
            const auto paintState = g.saveState();

            paintStyleShadows (g);

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
                child->internalPaint (g, clipRegion, renderContinuous, subtreeActiveClip);

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
            paintChildrenAndOverChildren (g, clipRegion, renderContinuous, subtreeActiveClip);
        }
    }
}

//==============================================================================

void Component::internalPaint (Graphics& g,
                               const RectangleList<float>& repaintRegions,
                               bool renderContinuous,
                               const RectangleList<float>* activeClipRegion)
{
    if (! isVisible() || flags.manuallyComposited || getWidth() <= 0.0f || getHeight() <= 0.0f)
        return;

    const auto toTopLevel = getTransformToTopLevelComponent();

    auto boundsToRedraw = intersectRepaintRegion (repaintRegions, getPaintBounds(), toTopLevel);

    if (boundsToRedraw.isEmpty())
    {
        if (! renderContinuous)
            return;

        boundsToRedraw.addWithoutMerge (getTransformedBounds (getPaintBounds(), toTopLevel));
    }

    const auto selfOpacity = (! flags.onDesktop && native == nullptr) ? getOpacity() : 1.0f;
    const auto opacity = g.getOpacity() * selfOpacity;
    if (opacity <= 0.0f)
        return;

    const auto getCanvasRegion = [&] (const Rectangle<float>& canvasBounds)
    {
        auto region = intersectRepaintRegion (repaintRegions, canvasBounds, toTopLevel);
        return region.isEmpty() ? boundsToRedraw : region;
    };

    if (hasActiveEffect())
    {
        invalidateCanvasesIfThemeChanged();

        const auto scale = g.getContextScale();

        auto canvas = renderSubtreeOffscreen (g.getGraphicsContext(),
                                              1.0f,
                                              renderContinuous,
                                              scale,
                                              std::move (effectOffscreenCanvas),
                                              takeEffectDirtyRegion (renderContinuous),
                                              true);

        // Where there is no canvas to have, without a GPU or for a component larger than a texture
        // can be, the component is painted below as it is without an effect
        if (canvas != nullptr)
        {
            const auto area = getOffscreenArea (getSize(), getOffscreenMargin(), scale, canvas, offscreenCanvasMargin);

            // A canvas gives another texture once it was drawn in again, and the same one until then: that is how
            // an effect keeping what it made of the last one, as ShaderEffect does, tells whether it still holds
            auto texture = canvas->asTexture();

            {
                const auto saved = g.saveState();
                g.setOpacity (opacity);
                applyPaintState (g, getCanvasRegion (area.visible), activeClipRegion);
                applyActiveEffects (g, texture, area.bounds, scale);
            }

            effectOffscreenCanvas = canvas;

            if (flags.cachedToTexture)
                cachedTextureCanvas = canvas;

            return;
        }
    }
    else if (effectOffscreenCanvas != nullptr)
    {
        // An effect that is not active any more keeps no canvas, and what the component cached is of its subtree
        releaseUnusedEffectCanvases();
        flags.cachedTextureStale = true;
    }

    if (flags.cachedToTexture)
    {
        invalidateCanvasesIfThemeChanged();

        const auto scale = g.getContextScale();
        const auto area = getOffscreenArea (getSize(), getOffscreenMargin(), scale, cachedTextureCanvas, offscreenCanvasMargin);

        const auto sizeMatches = hasPixelSize (cachedTextureCanvas, area.pixelSize);

        if (! sizeMatches || flags.cachedTextureStale)
        {
            // A stale canvas of the right size is drawn over again, sparing a new GPU target.
            auto canvas = sizeMatches ? std::move (cachedTextureCanvas)
                                      : createOffscreenCanvas (g.getGraphicsContext(), area.pixelSize);

            cachedTextureCanvas = nullptr;
            flags.cachedTextureStale = false;

            if (canvas != nullptr)
            {
                auto& offscreenG = canvas->beginDraw ({}, scale);
                offscreenG.setClipPath (area.clip);
                offscreenG.setDrawingArea (getLocalBounds().translated (area.origin));

                flags.paintAsOffscreenRoot = true;
                offscreenOrigin = area.origin;
                paint (offscreenG);
                offscreenOrigin = {};
                flags.paintAsOffscreenRoot = false;

                canvas->commit();
                cachedTextureCanvas = canvas;
                offscreenCanvasMargin = area.pixelMargin;
            }
        }

        isRepainting.store (true, std::memory_order_relaxed);

        {
            const auto saved = g.saveState();
            g.setOpacity (opacity);
            applyPaintState (g, getCanvasRegion (area.visible), activeClipRegion);
            paintStyleShadows (g);

            if (cachedTextureCanvas != nullptr)
                g.drawTexture (cachedTextureCanvas->asTexture(), area.bounds);
            else
                paint (g);
        }

        isRepainting.store (false, std::memory_order_relaxed);

        paintChildrenAndOverChildren (g, boundsToRedraw, renderContinuous, activeClipRegion);
        return;
    }

    paintSubtree (g, boundsToRedraw, opacity, renderContinuous, activeClipRegion);

#if YUP_ENABLE_COMPONENT_PAINT_DEBUGGING
    paintDebugOverlay (g, boundsToRedraw);
#endif
}

#if YUP_ENABLE_COMPONENT_PAINT_DEBUGGING
void Component::paintDebugOverlay (Graphics& g, const RectangleList<float>& boundsToRedraw)
{
    const auto saved = g.saveState();

    applyPaintState (g, boundsToRedraw);
    g.setFillColor (debugColor.withMultipliedAlpha (0.2f));
    g.fillRect (getLocalBounds());

    if (--counter == 0)
    {
        counter = 2;
        debugColor = Color::opaqueRandom();
    }
}
#endif

//==============================================================================

void Component::internalMouseEnter (const MouseEvent& event)
{
    if (! isVisible())
        return;

    setHoveredChain (true);

    updateMouseCursor();

    if (! isEnabled())
        return;

    auto bailOutChecker = BailOutChecker (this);

    mouseEnter (event);

    if (bailOutChecker.shouldBailOut())
        return;

    callMouseListeners (bailOutChecker, &MouseListener::mouseEnter, event);
}

//==============================================================================

void Component::internalMouseExit (const MouseEvent& event)
{
    if (! isVisible())
        return;

    clearHoveredInSubtree();

    if (parentComponent != nullptr)
        parentComponent->setHoveredChain (false);

    updateMouseCursor();

    auto bailOutChecker = BailOutChecker (this);

    mouseExit (event);

    if (bailOutChecker.shouldBailOut())
        return;

    callMouseListeners (bailOutChecker, &MouseListener::mouseExit, event);
}

//==============================================================================

void Component::internalMouseDown (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    if (! isEnabled())
        return;

    setPressedChain (true);

    auto bailOutChecker = BailOutChecker (this);

    handleKeyboardFocusFromClick();

    if (bailOutChecker.shouldBailOut())
        return;

    mouseDown (event);

    if (bailOutChecker.shouldBailOut())
        return;

    callMouseListeners (bailOutChecker, &MouseListener::mouseDown, event);
}

//==============================================================================

void Component::internalMouseMove (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    if (! isEnabled())
        return;

    auto bailOutChecker = BailOutChecker (this);

    mouseMove (event);

    if (bailOutChecker.shouldBailOut())
        return;

    callMouseListeners (bailOutChecker, &MouseListener::mouseMove, event);
}

//==============================================================================

void Component::internalMouseDrag (const MouseEvent& event)
{
    if (! isVisible())
        return;

    updateMouseCursor();

    if (! isEnabled())
        return;

    auto bailOutChecker = BailOutChecker (this);

    mouseDrag (event);

    if (bailOutChecker.shouldBailOut())
        return;

    callMouseListeners (bailOutChecker, &MouseListener::mouseDrag, event);
}

//==============================================================================

void Component::internalMouseUp (const MouseEvent& event)
{
    getTopLevelComponent()->clearPressedInSubtree();

    // A component hidden while it was pressed still gets the release, which ends what its press began
    if (isVisible())
        updateMouseCursor();

    auto bailOutChecker = BailOutChecker (this);

    mouseUp (event);

    if (bailOutChecker.shouldBailOut())
        return;

    callMouseListeners (bailOutChecker, &MouseListener::mouseUp, event);
}

//==============================================================================

void Component::internalMouseDoubleClick (const MouseEvent& event)
{
    if (! isVisible() || ! isEnabled())
        return;

    auto bailOutChecker = BailOutChecker (this);

    mouseDoubleClick (event);

    if (bailOutChecker.shouldBailOut())
        return;

    callMouseListeners (bailOutChecker, &MouseListener::mouseDoubleClick, event);
}

//==============================================================================

void Component::internalMouseWheel (const MouseEvent& event, const MouseWheelData& wheelData)
{
    if (! isVisible())
        return;

    auto bailOutChecker = BailOutChecker (this);

    for (auto* component = this; component != nullptr; component = component->parentComponent)
    {
        if (! component->isVisible() || ! component->isEnabled())
            continue;

        auto componentBailOutChecker = BailOutChecker (component);
        const auto componentEvent = event.withPosition (component->getLocalPoint (this, event.getPosition()))
                                         .withSourceComponent (component);

        if (component->mouseWheel (componentEvent, wheelData)
            || componentBailOutChecker.shouldBailOut()
            || bailOutChecker.shouldBailOut())
            break;
    }

    if (bailOutChecker.shouldBailOut())
        return;

    callMouseWheelListeners (bailOutChecker, event, wheelData);
}

//==============================================================================

void Component::internalGesture (const GestureEvent& event)
{
    if (! isVisible() || ! isEnabled())
        return;

    void (GestureListener::*callback) (const GestureEvent&) = nullptr;

    switch (event.getType())
    {
        case GestureType::tap:
            callback = &GestureListener::gestureTap;
            break;
        case GestureType::longPress:
            callback = &GestureListener::gestureLongPress;
            break;
        case GestureType::pinch:
            callback = &GestureListener::gesturePinch;
            break;
        case GestureType::rotate:
            callback = &GestureListener::gestureRotate;
            break;
        case GestureType::pan:
            callback = &GestureListener::gesturePan;
            break;
        case GestureType::swipe:
            callback = &GestureListener::gestureSwipe;
            break;
        default:
            return;
    }

    auto bailOutChecker = BailOutChecker (this);

    (this->*callback) (event);

    if (bailOutChecker.shouldBailOut())
        return;

    gestureListeners.callChecked (bailOutChecker, callback, event);
}

//==============================================================================

bool Component::internalKeyDown (const KeyPress& keys, const Point<float>& position)
{
    return dispatchKey (keys, position, &Component::keyDown);
}

//==============================================================================

bool Component::internalKeyUp (const KeyPress& keys, const Point<float>& position)
{
    return dispatchKey (keys, position, &Component::keyUp);
}

//==============================================================================

void Component::internalTextInput (const String& text)
{
    if (! flags.wantsKeyboardFocus || ! isVisible() || ! isEnabled())
        return;

    textInput (text);
}

//==============================================================================

bool Component::internalKeyStateChanged (const KeyPress& keys, bool isDown)
{
    for (auto* component = this; component != nullptr; component = component->parentComponent)
    {
        if (! component->isVisible() || ! component->isEnabled())
            continue;

        auto bailOutChecker = BailOutChecker (component);

        if (component->keyStateChanged (keys, isDown) || bailOutChecker.shouldBailOut())
            return true;
    }

    return false;
}

//==============================================================================

void Component::internalModifierKeysChanged (const KeyModifiers& modifiers)
{
    if (! isVisible() || ! isEnabled())
        return;

    modifierKeysChanged (modifiers);
}

//==============================================================================

void Component::internalResized (int width, int height)
{
    const auto newBounds = boundsInParent.withSize (Size<int> (width, height).to<float>());

    if (newBounds != boundsInParent)
    {
        const auto oldBounds = boundsInParent;
        boundsInParent = newBounds;

        const BailOutChecker bailOutChecker (this);

        sendResized();

        if (! bailOutChecker.shouldBailOut())
            notifyBoundsPropertiesChanged (oldBounds);
    }
}

//==============================================================================

void Component::internalMoved (int xpos, int ypos)
{
    const auto newBounds = boundsInParent.withPosition (Point<int> (xpos, ypos).to<float>());

    if (newBounds != boundsInParent)
    {
        const auto oldBounds = boundsInParent;
        boundsInParent = newBounds;

        const BailOutChecker bailOutChecker (this);

        sendMoved();

        if (! bailOutChecker.shouldBailOut())
            notifyBoundsPropertiesChanged (oldBounds);
    }
}

//==============================================================================

void Component::internalFocusChanged (bool gotFocus)
{
    const WeakReference<Component> self (this);

    flags.isFocused = gotFocus;

    notifyPropertyChanged (ComponentIds::focused);
    if (self == nullptr)
        return;

    for (WeakReference<Component> component (this); component != nullptr;)
    {
        const WeakReference<Component> next (component->parentComponent);
        component->notifyPropertyChanged (ComponentIds::focusWithin);
        component = next;
    }

    if (self == nullptr)
        return;

    if (gotFocus)
        focusGained();
    else
        focusLost();
}

//==============================================================================

void Component::internalFocusOfComponentChanged (FocusChangeType cause)
{
    auto bailOutChecker = BailOutChecker (this);

    for (auto* ancestor = parentComponent; ancestor != nullptr;)
    {
        auto ancestorBailOutChecker = BailOutChecker (ancestor);

        ancestor->focusOfChildComponentChanged (this, cause);

        if (bailOutChecker.shouldBailOut() || ancestorBailOutChecker.shouldBailOut())
            return;

        ancestor = ancestor->parentComponent;
    }
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

void Component::updateFrameAttachments()
{
    for (auto* attachment : frameAttachments)
        attachment->updateNative();
}

void Component::internalThemeChanged()
{
    // The stylesheets of the window follow the theme in the frame that is starting
    if (styleContext != nullptr)
        styleContext->themeChanged();

    for (auto* child : children)
        child->internalThemeChanged();
}

void Component::internalAttachedToNative()
{
    updateFrameAttachments();

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
    updateFrameAttachments();

    // The children are told below, each letting go of what it keeps of the window
    releaseGpuResources (false);

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
    // What changed while the component was not showing asked for no repaint, so what it keeps of its subtree is painted again
    markEffectCanvasDirty();

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
    Desktop::getInstance()->setMouseCursor (isEnabled() ? mouseCursor : MouseCursor (MouseCursor::Default));
}

//==============================================================================

Point<float> Component::getScreenPosition() const
{
    return localToScreen (Point<float>());
}

//==============================================================================

Rectangle<float> Component::getScreenBounds() const
{
    return localToScreen (getLocalBounds());
}

//==============================================================================

Rectangle<float> Component::getSafeAreaBounds() const
{
    if (flags.onDesktop && native != nullptr)
        return native->getSafeAreaBounds().to<float>();

    if (parentComponent == nullptr)
        return getLocalBounds();

    return parentComponent->getSafeAreaBounds()
        .translated (-getPosition())
        .intersection (getLocalBounds());
}

void Component::safeAreaChanged() {}

//==============================================================================

std::optional<Point<float>> Component::getChildPointFromLocal (const Component& child, Point<float> localPoint) const
{
    jassert (child.parentComponent == this);

    const auto childPoint = localPoint - child.getPosition();
    if (! child.isTransformed())
        return childPoint;

    const auto combined = child.getCombinedTransform();
    if (approximatelyEqual (combined.getDeterminant(), 0.0f))
        return std::nullopt;

    return childPoint.transformed (combined.inverted());
}

std::optional<Point<float>> Component::getLocalPointFromChild (const Component& child, Point<float> childPoint) const
{
    jassert (child.parentComponent == this);

    return childPoint.transformed (child.getCombinedTransform()) + child.getPosition();
}

std::optional<Point<float>> Component::getLocalPointFromParent (Point<float> parentPoint) const
{
    if (parentComponent == nullptr)
        return std::nullopt;

    auto localPoint = parentComponent->getChildPointFromLocal (*this, parentPoint);
    const auto* effect = getActiveComponentEffect();

    if (localPoint && effect != nullptr)
        localPoint = effect->displayToContent (*localPoint, getEffectBounds (getScaleDpi()));

    return localPoint;
}

std::optional<Point<float>> Component::getParentPointFromLocal (Point<float> localPoint) const
{
    if (parentComponent == nullptr)
        return std::nullopt;

    std::optional<Point<float>> displayPoint = localPoint;
    if (const auto* effect = getActiveComponentEffect())
        displayPoint = effect->contentToDisplay (localPoint, getEffectBounds (getScaleDpi()));

    if (! displayPoint)
        return std::nullopt;

    return parentComponent->getLocalPointFromChild (*this, *displayPoint);
}

Point<float> Component::getLocalPointFromTopLevel (Point<float> topLevelPoint) const
{
    if (parentComponent == nullptr)
        return topLevelPoint;

    const auto parentPoint = parentComponent->getLocalPointFromTopLevel (topLevelPoint);

    return getLocalPointFromParent (parentPoint).value_or (parentPoint - getPosition());
}

Point<float> Component::getTopLevelScreenOrigin() const
{
    auto topLevel = this;
    while (topLevel->parentComponent != nullptr)
        topLevel = topLevel->parentComponent;

    if (topLevel->flags.onDesktop && topLevel->native != nullptr)
        return topLevel->native->getPosition().to<float>();

    return topLevel->getPosition();
}

Point<float> Component::localToScreen (const Point<float>& localPoint) const
{
    auto point = localPoint;

    for (auto component = this; component->parentComponent != nullptr; component = component->parentComponent)
        point = component->getParentPointFromLocal (point).value_or (point + component->getPosition());

    return point + getTopLevelScreenOrigin();
}

Point<float> Component::screenToLocal (const Point<float>& screenPoint) const
{
    return getLocalPointFromTopLevel (screenPoint - getTopLevelScreenOrigin());
}

Rectangle<float> Component::localToScreen (const Rectangle<float>& localRectangle) const
{
    return getMappedBounds (localRectangle, [this] (Point<float> p)
    {
        return localToScreen (p);
    });
}

Rectangle<float> Component::screenToLocal (const Rectangle<float>& screenRectangle) const
{
    return getMappedBounds (screenRectangle, [this] (Point<float> p)
    {
        return screenToLocal (p);
    });
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

    for (auto comp = this; comp->parentComponent != nullptr; comp = comp->parentComponent)
    {
        if (comp->isTransformed())
            transform = transform.followedBy (comp->getCombinedTransform());

        transform = transform.translated (comp->getPosition());
    }

    return transform.translated (getTopLevelScreenOrigin());
}

//==============================================================================

bool Component::dispatchKey (const KeyPress& keys, const Point<float>& position, KeyHandler handler)
{
    for (auto* component = this; component != nullptr; component = component->parentComponent)
    {
        if (! component->isVisible() || ! component->isEnabled())
            continue;

        auto bailOutChecker = BailOutChecker (component);

        if ((component->*handler) (keys, position) || bailOutChecker.shouldBailOut())
            return true;
    }

    return false;
}

} // namespace yup
