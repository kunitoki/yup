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

namespace
{
AudioGraphNodeView* findNodeViewForEventSource (Component* source) noexcept
{
    if (source == nullptr)
        return nullptr;

    if (auto* nodeView = dynamic_cast<AudioGraphNodeView*> (source))
        return nodeView;

    return source->getParentComponentWithType<AudioGraphNodeView>();
}
} // namespace

//==============================================================================
const Identifier AudioGraphComponent::Style::backgroundColorId ("audioGraphBackground");
const Identifier AudioGraphComponent::Style::gridColorId ("audioGraphGrid");

//==============================================================================
AudioGraphComponent::AudioGraphComponent (std::shared_ptr<AudioGraphModel> modelIn)
    : model (std::move (modelIn))
{
    setOpaque (true);
    setWantsKeyboardFocus (true);
    setWantsMouseEvents (true, true);
    enableRenderingUnclipped (true);
}

AudioGraphComponent::AudioGraphComponent (std::shared_ptr<AudioGraphProcessor> graphIn)
    : AudioGraphComponent (graphIn != nullptr ? graphIn->getModel() : nullptr)
{
    graph = std::move (graphIn);
}

AudioGraphComponent::~AudioGraphComponent()
{
    for (auto& node : nodes)
    {
        if (node.view != nullptr)
            node.view->removeMouseListener (this);
    }
}

//==============================================================================
void AudioGraphComponent::addNodeView (AudioGraphNodeID nodeID, std::unique_ptr<AudioGraphNodeView> view, Point<float> initialCanvasPosition)
{
    if (view == nullptr)
        return;

    removeNodeView (nodeID);

    view->setViewScale (zoom);
    view->addMouseListener (this);
    addAndMakeVisible (*view);

    nodes.push_back ({ nodeID, std::move (view), initialCanvasPosition, NodeItem::Kind::processor });
    updateNodeBounds();
    repaint();
}

void AudioGraphComponent::setGraphInputView (std::unique_ptr<AudioGraphNodeView> view, Point<float> initialCanvasPosition)
{
    if (view == nullptr)
        return;

    auto* item = findNodeItemByKind (NodeItem::Kind::graphInput);
    if (item != nullptr)
    {
        if (item->view != nullptr)
        {
            item->view->removeMouseListener (this);
            removeChildComponent (item->view.get());
        }

        item->view = std::move (view);
        item->canvasPosition = initialCanvasPosition;
    }
    else
    {
        nodes.push_back ({ AudioGraphNodeID::invalid(), std::move (view), initialCanvasPosition, NodeItem::Kind::graphInput });
        item = &nodes.back();
    }

    item->view->setViewScale (zoom);
    item->view->addMouseListener (this);
    addAndMakeVisible (*item->view);
    updateNodeBounds();
    repaint();
}

void AudioGraphComponent::setGraphOutputView (std::unique_ptr<AudioGraphNodeView> view, Point<float> initialCanvasPosition)
{
    if (view == nullptr)
        return;

    auto* item = findNodeItemByKind (NodeItem::Kind::graphOutput);
    if (item != nullptr)
    {
        if (item->view != nullptr)
        {
            item->view->removeMouseListener (this);
            removeChildComponent (item->view.get());
        }

        item->view = std::move (view);
        item->canvasPosition = initialCanvasPosition;
    }
    else
    {
        nodes.push_back ({ AudioGraphNodeID::invalid(), std::move (view), initialCanvasPosition, NodeItem::Kind::graphOutput });
        item = &nodes.back();
    }

    item->view->setViewScale (zoom);
    item->view->addMouseListener (this);
    addAndMakeVisible (*item->view);
    updateNodeBounds();
    repaint();
}

void AudioGraphComponent::removeNodeView (AudioGraphNodeID nodeID)
{
    const auto iterator = std::find_if (nodes.begin(), nodes.end(), [nodeID] (const NodeItem& item)
    {
        return item.kind == NodeItem::Kind::processor && item.nodeID == nodeID;
    });

    if (iterator == nodes.end())
        return;

    if (iterator->view != nullptr)
    {
        iterator->view->removeMouseListener (this);
        removeChildComponent (iterator->view.get());
    }

    nodes.erase (iterator);
    repaint();
}

AudioGraphNodeView* AudioGraphComponent::getNodeView (AudioGraphNodeID nodeID) const noexcept
{
    const auto iterator = std::find_if (nodes.begin(), nodes.end(), [nodeID] (const NodeItem& item)
    {
        return item.kind == NodeItem::Kind::processor && item.nodeID == nodeID;
    });

    return iterator != nodes.end() ? iterator->view.get() : nullptr;
}

//==============================================================================
void AudioGraphComponent::setZoom (float newZoom)
{
    const auto clampedZoom = jlimit (minZoom, maxZoom, newZoom);
    if (zoom == clampedZoom)
        return;

    needsInitialReset = false;
    needsInitialZoomToFit = false;
    zoom = clampedZoom;
    updateNodeBounds();
    repaint();
}

void AudioGraphComponent::setMinZoom (float newMinZoom)
{
    const auto clampedMinZoom = jmax (0.001f, newMinZoom);
    if (minZoom == clampedMinZoom)
        return;

    minZoom = clampedMinZoom;

    if (maxZoom < minZoom)
        maxZoom = minZoom;

    setZoom (zoom);
}

void AudioGraphComponent::setMaxZoom (float newMaxZoom)
{
    const auto clampedMaxZoom = jmax (0.001f, newMaxZoom);
    if (maxZoom == clampedMaxZoom)
        return;

    maxZoom = clampedMaxZoom;

    if (minZoom > maxZoom)
        minZoom = maxZoom;

    setZoom (zoom);
}

void AudioGraphComponent::setWireHitDistance (float newWireHitDistance)
{
    wireHitDistance = jmax (0.0f, newWireHitDistance);
}

void AudioGraphComponent::setDragWireThreshold (float newDragWireThreshold)
{
    dragWireThreshold = jmax (0.0f, newDragWireThreshold);
}

void AudioGraphComponent::setCanvasOffset (Point<float> offset)
{
    needsInitialReset = false;
    needsInitialZoomToFit = false;
    canvasOffset = offset;
    updateNodeBounds();
    repaint();
}

void AudioGraphComponent::resetView()
{
    if (getWidth() <= 0.0f || getHeight() <= 0.0f)
    {
        needsInitialReset = true;
        needsInitialZoomToFit = false;
        return;
    }

    needsInitialReset = false;
    needsInitialZoomToFit = false;
    zoom = jlimit (minZoom, maxZoom, 1.0f);

    centerViewOnNodes();
}

void AudioGraphComponent::centerViewOnNodes()
{
    if (getWidth() <= 0.0f || getHeight() <= 0.0f)
    {
        needsInitialReset = true;
        needsInitialZoomToFit = false;
        return;
    }

    needsInitialReset = false;
    needsInitialZoomToFit = false;

    const auto bounds = getNodesCanvasBounds();
    if (! bounds.has_value())
    {
        canvasOffset = getLocalBounds().getCenter();
        updateNodeBounds();
        repaint();
        return;
    }

    canvasOffset = getLocalBounds().getCenter() - (bounds->getCenter() * zoom);
    updateNodeBounds();
    repaint();
}

void AudioGraphComponent::zoomToFitNodes (float padding, float maximumZoom)
{
    if (getWidth() <= 0.0f || getHeight() <= 0.0f)
    {
        needsInitialReset = false;
        needsInitialZoomToFit = true;
        pendingZoomToFitPadding = padding;
        pendingZoomToFitMaximumZoom = maximumZoom;
        return;
    }

    needsInitialReset = false;
    needsInitialZoomToFit = false;
    const auto upperZoom = jlimit (minZoom, maxZoom, maximumZoom);

    const auto bounds = getNodesCanvasBounds();
    if (! bounds.has_value())
    {
        zoom = jlimit (minZoom, upperZoom, 1.0f);
        canvasOffset = getLocalBounds().getCenter();
        updateNodeBounds();
        repaint();
        return;
    }

    const auto safePadding = jmax (0.0f, padding);
    const auto availableWidth = jmax (1.0f, static_cast<float> (getWidth()) - (safePadding * 2.0f));
    const auto availableHeight = jmax (1.0f, static_cast<float> (getHeight()) - (safePadding * 2.0f));
    const auto fitZoom = jmin (availableWidth / jmax (1.0f, bounds->getWidth()),
                               availableHeight / jmax (1.0f, bounds->getHeight()));

    zoom = jlimit (minZoom, upperZoom, fitZoom);
    canvasOffset = getLocalBounds().getCenter() - (bounds->getCenter() * zoom);
    updateNodeBounds();
    repaint();
}

Point<float> AudioGraphComponent::canvasToScreen (Point<float> canvasPos) const
{
    return (canvasPos * zoom) + canvasOffset;
}

Point<float> AudioGraphComponent::screenToCanvas (Point<float> screenPos) const
{
    return (screenPos - canvasOffset) / zoom;
}

void AudioGraphComponent::addListener (Listener* listener)
{
    listeners.add (listener);
}

void AudioGraphComponent::removeListener (Listener* listener)
{
    listeners.remove (listener);
}

//==============================================================================
void AudioGraphComponent::resized()
{
    if (needsInitialZoomToFit)
        zoomToFitNodes (pendingZoomToFitPadding, pendingZoomToFitMaximumZoom);
    else if (needsInitialReset || canvasOffset.isOrigin())
        resetView();
    else
        updateNodeBounds();
}

void AudioGraphComponent::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

//==============================================================================
void AudioGraphComponent::mouseDown (const MouseEvent& event)
{
    takeKeyboardFocus();

    const auto screenPos = eventPositionInThisComponent (event);
    mouseDownScreen = screenPos;
    lastMouseScreen = screenPos;

    if (event.getButtons() == MouseEvent::rightButton)
    {
        if (auto endpoint = hitTestEndpoint (screenPos))
        {
            removeConnectionsForEndpoint (endpoint->endpoint);
            return;
        }

        if (auto connection = hitTestConnection (screenPos))
        {
            requestConnectionRemoval (*connection);
            return;
        }

        for (const auto& node : nodes)
        {
            if (node.view == nullptr || ! node.nodeID.isValid())
                continue;

            const auto localPos = node.view->getLocalPoint (this, screenPos);

            if (node.view->getLocalBounds().contains (localPos))
            {
                listeners.call (&Listener::nodeContextMenu, node.nodeID, screenToCanvas (screenPos));
                return;
            }
        }

        listeners.call (&Listener::canvasContextMenu, screenToCanvas (screenPos));
        return;
    }

    if (spacebarDown || event.getButtons() == MouseEvent::middleButton)
    {
        interaction = Interaction::panningCanvas;
        panStartOffset = canvasOffset;
        setMouseCursor (MouseCursor::ResizeAll);
        return;
    }

    if (auto* clickedNodeView = findNodeViewForEventSource (event.getSourceComponent()))
        clickedNodeView->toFront (true);

    if (auto endpoint = hitTestEndpoint (screenPos))
    {
        if (interaction == Interaction::armedPort && activeEndpoint.has_value())
        {
            if (tryConnect (*activeEndpoint, endpoint->endpoint))
            {
                interaction = Interaction::idle;
                activeEndpoint.reset();
            }
            else
            {
                activeEndpoint = endpoint->endpoint;
                interaction = Interaction::armedPort;
            }
        }
        else
        {
            activeEndpoint = endpoint->endpoint;
            interaction = Interaction::armedPort;
        }

        pendingWireEnd = screenPos;
        repaint();
        return;
    }

    if (interaction == Interaction::armedPort)
    {
        interaction = Interaction::idle;
        activeEndpoint.reset();
        repaint();
        return;
    }

    auto* source = event.getSourceComponent();
    if (auto* nodeView = dynamic_cast<AudioGraphNodeView*> (source))
    {
        if (auto* item = findNodeItemForView (nodeView))
        {
            draggedNodeIndex = static_cast<int> (item - nodes.data());
            dragStartCanvasMouse = screenToCanvas (screenPos);
            dragStartNodePosition = item->canvasPosition;
            interaction = Interaction::draggingNode;
        }
    }
}

void AudioGraphComponent::mouseDrag (const MouseEvent& event)
{
    const auto screenPos = eventPositionInThisComponent (event);
    lastMouseScreen = screenPos;

    switch (interaction)
    {
        case Interaction::panningCanvas:
            setCanvasOffset (panStartOffset + (screenPos - mouseDownScreen));
            break;

        case Interaction::draggingNode:
        {
            if (isPositiveAndBelow (draggedNodeIndex, static_cast<int> (nodes.size())))
            {
                nodes[static_cast<size_t> (draggedNodeIndex)].canvasPosition = dragStartNodePosition + (screenToCanvas (screenPos) - dragStartCanvasMouse);
                updateNodeBounds();
                repaint();
            }

            break;
        }

        case Interaction::armedPort:
            if (mouseDownScreen.distanceTo (screenPos) > dragWireThreshold)
                interaction = Interaction::draggingWire;

            pendingWireEnd = screenPos;
            repaint();
            break;

        case Interaction::draggingWire:
            pendingWireEnd = screenPos;
            repaint();
            break;

        case Interaction::idle:
            break;
    }
}

void AudioGraphComponent::mouseUp (const MouseEvent& event)
{
    const auto screenPos = eventPositionInThisComponent (event);
    lastMouseScreen = screenPos;

    switch (interaction)
    {
        case Interaction::draggingWire:
            if (activeEndpoint.has_value())
            {
                if (auto endpoint = hitTestEndpoint (screenPos))
                    tryConnect (*activeEndpoint, endpoint->endpoint);
            }

            activeEndpoint.reset();
            interaction = Interaction::idle;
            repaint();
            break;

        case Interaction::draggingNode:
        {
            if (isPositiveAndBelow (draggedNodeIndex, static_cast<int> (nodes.size())))
            {
                const auto& item = nodes[static_cast<size_t> (draggedNodeIndex)];

                if (item.kind == NodeItem::Kind::processor)
                {
                    auto& draggedItem = nodes[static_cast<size_t> (draggedNodeIndex)];
                    bool accepted = true;

                    if (onNodeMoveRequested != nullptr)
                        accepted = onNodeMoveRequested (draggedItem.nodeID, dragStartNodePosition, draggedItem.canvasPosition);

                    if (! accepted)
                    {
                        draggedItem.canvasPosition = dragStartNodePosition;
                        updateNodeBounds();
                        repaint();
                        interaction = Interaction::idle;
                        draggedNodeIndex = -1;
                        break;
                    }

                    listeners.call (&Listener::nodeViewMoved, draggedItem.nodeID, draggedItem.canvasPosition);
                }
            }

            interaction = Interaction::idle;
            draggedNodeIndex = -1;
            break;
        }

        case Interaction::panningCanvas:
            interaction = Interaction::idle;
            setMouseCursor (MouseCursor::Default);
            break;

        case Interaction::armedPort:
        case Interaction::idle:
            break;
    }
}

void AudioGraphComponent::mouseDoubleClick (const MouseEvent& event)
{
    if (listeners.isEmpty())
        return;

    const auto screenPos = eventPositionInThisComponent (event);

    for (const auto& node : nodes)
    {
        if (node.view == nullptr || ! node.nodeID.isValid())
            continue;

        const auto localPos = node.view->getLocalPoint (this, screenPos);

        if (node.view->getLocalBounds().contains (localPos))
        {
            listeners.call (&Listener::nodeDoubleClicked, node.nodeID);
            return;
        }
    }
}

void AudioGraphComponent::mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData)
{
    const auto screenPos = eventPositionInThisComponent (event);
    const auto canvasPos = screenToCanvas (screenPos);
    const auto zoomFactor = std::pow (1.12f, wheelData.getDeltaY());
    const auto newZoom = jlimit (minZoom, maxZoom, zoom * zoomFactor);

    if (newZoom == zoom)
        return;

    needsInitialReset = false;
    needsInitialZoomToFit = false;
    zoom = newZoom;
    canvasOffset = screenPos - (canvasPos * zoom);
    updateNodeBounds();
    repaint();
}

void AudioGraphComponent::keyDown (const KeyPress& keys, const Point<float>&)
{
    if (keys.getKey() == KeyPress::spaceKey)
    {
        spacebarDown = true;
        setMouseCursor (MouseCursor::ResizeAll);
        return;
    }

    if (keys.getKey() == KeyPress::textFKey || keys.getKey() == KeyPress::homeKey)
    {
        zoomToFitNodes();
        return;
    }

    if (keys.getKey() == KeyPress::number0Key)
        resetView();
}

void AudioGraphComponent::keyUp (const KeyPress& keys, const Point<float>&)
{
    if (keys.getKey() == KeyPress::spaceKey)
    {
        spacebarDown = false;
        setMouseCursor (MouseCursor::Default);
    }
}

//==============================================================================
void AudioGraphComponent::updateNodeBounds()
{
    for (auto& node : nodes)
    {
        if (node.view == nullptr)
            continue;

        node.view->setViewScale (zoom);

        const auto canvasBounds = getNodeCanvasBounds (node);
        const auto screenTopLeft = canvasToScreen (canvasBounds.getPosition());
        node.view->setBounds ({ screenTopLeft, canvasBounds.getSize() * zoom });
    }
}

Rectangle<float> AudioGraphComponent::getNodeCanvasBounds (const NodeItem& item) const
{
    if (item.view == nullptr)
        return {};

    return { item.canvasPosition,
             static_cast<float> (item.view->getPreferredWidth()),
             static_cast<float> (item.view->getPreferredHeight()) };
}

std::optional<Rectangle<float>> AudioGraphComponent::getNodesCanvasBounds() const
{
    std::optional<Rectangle<float>> result;

    for (const auto& node : nodes)
    {
        if (node.view == nullptr)
            continue;

        const auto bounds = getNodeCanvasBounds (node);
        if (bounds.isEmpty())
            continue;

        if (! result.has_value())
        {
            result = bounds;
            continue;
        }

        const auto left = jmin (result->getLeft(), bounds.getLeft());
        const auto top = jmin (result->getTop(), bounds.getTop());
        const auto right = jmax (result->getRight(), bounds.getRight());
        const auto bottom = jmax (result->getBottom(), bounds.getBottom());
        result = Rectangle<float> { left, top, right - left, bottom - top };
    }

    return result;
}

Point<float> AudioGraphComponent::eventPositionInThisComponent (const MouseEvent& event) const
{
    if (event.getSourceComponent() == this || event.getSourceComponent() == nullptr)
        return event.getPosition();

    return getLocalPoint (event.getSourceComponent(), event.getPosition());
}

AudioGraphComponent::NodeItem* AudioGraphComponent::findNodeItemForView (const AudioGraphNodeView* view) noexcept
{
    const auto iterator = std::find_if (nodes.begin(), nodes.end(), [view] (const NodeItem& item)
    {
        return item.view.get() == view;
    });

    return iterator != nodes.end() ? &*iterator : nullptr;
}

const AudioGraphComponent::NodeItem* AudioGraphComponent::findNodeItemForView (const AudioGraphNodeView* view) const noexcept
{
    const auto iterator = std::find_if (nodes.begin(), nodes.end(), [view] (const NodeItem& item)
    {
        return item.view.get() == view;
    });

    return iterator != nodes.end() ? &*iterator : nullptr;
}

AudioGraphComponent::NodeItem* AudioGraphComponent::findNodeItemByKind (NodeItem::Kind kind) noexcept
{
    const auto iterator = std::find_if (nodes.begin(), nodes.end(), [kind] (const NodeItem& item)
    {
        return item.kind == kind;
    });

    return iterator != nodes.end() ? &*iterator : nullptr;
}

const AudioGraphComponent::NodeItem* AudioGraphComponent::findNodeItemByKind (NodeItem::Kind kind) const noexcept
{
    const auto iterator = std::find_if (nodes.begin(), nodes.end(), [kind] (const NodeItem& item)
    {
        return item.kind == kind;
    });

    return iterator != nodes.end() ? &*iterator : nullptr;
}

std::optional<AudioGraphComponent::EndpointHit> AudioGraphComponent::hitTestEndpoint (Point<float> screenPos) const
{
    for (const auto& node : nodes)
    {
        if (node.view == nullptr)
            continue;

        const auto localPos = node.view->getLocalPoint (this, screenPos);
        if (auto port = node.view->hitTestPort (localPos))
        {
            AudioGraphEndpoint endpoint;

            if (node.kind == NodeItem::Kind::graphInput)
                endpoint = AudioGraphEndpoint::graphInput (port->busIndex);
            else if (node.kind == NodeItem::Kind::graphOutput)
                endpoint = AudioGraphEndpoint::graphOutput (port->busIndex);
            else
                endpoint = port->isInput
                             ? AudioGraphEndpoint::nodeInput (node.nodeID, port->busIndex)
                             : AudioGraphEndpoint::nodeOutput (node.nodeID, port->busIndex);

            return EndpointHit { endpoint, *port, node.view.get() };
        }
    }

    return {};
}

std::optional<AudioGraphConnection> AudioGraphComponent::hitTestConnection (Point<float> screenPos) const
{
    if (model == nullptr)
        return {};

    for (const auto& connection : model->getConnections())
    {
        const auto start = getEndpointScreenPosition (connection.source);
        const auto end = getEndpointScreenPosition (connection.destination);
        const auto controlOffset = jmax (60.0f, std::abs (end.getX() - start.getX()) * 0.5f);
        const auto cp1 = Point<float> { start.getX() + controlOffset, start.getY() };
        const auto cp2 = Point<float> { end.getX() - controlOffset, end.getY() };

        auto previous = start;
        const auto approximateLength = jmax (40.0f, start.distanceTo (end));
        const auto numSteps = jmax (8, roundToInt (approximateLength / 10.0f));

        for (int i = 1; i <= numSteps; ++i)
        {
            const auto t = static_cast<float> (i) / static_cast<float> (numSteps);
            const auto current = cubicPoint (start, cp1, cp2, end, t);

            if (distanceToSegment (screenPos, previous, current) <= wireHitDistance)
                return connection;

            previous = current;
        }
    }

    return {};
}

Point<float> AudioGraphComponent::getEndpointScreenPosition (const AudioGraphEndpoint& endpoint) const
{
    if (endpoint.getKind() == AudioGraphEndpoint::Kind::graphInput || endpoint.getKind() == AudioGraphEndpoint::Kind::graphOutput)
    {
        if (const auto* item = findNodeItemByKind (endpoint.getKind() == AudioGraphEndpoint::Kind::graphInput ? NodeItem::Kind::graphInput : NodeItem::Kind::graphOutput))
        {
            if (item->view != nullptr)
            {
                const auto local = endpoint.getKind() == AudioGraphEndpoint::Kind::graphInput
                                     ? item->view->getOutputPortCenter (endpoint.getBusIndex())
                                     : item->view->getInputPortCenter (endpoint.getBusIndex());

                return getLocalPoint (item->view.get(), local);
            }
        }

        auto left = 0.0f;
        auto right = 720.0f;
        auto top = 96.0f;

        if (! nodes.empty())
        {
            left = std::numeric_limits<float>::max();
            right = std::numeric_limits<float>::lowest();
            top = std::numeric_limits<float>::max();

            for (const auto& node : nodes)
            {
                const auto bounds = getNodeCanvasBounds (node);
                left = jmin (left, bounds.getLeft());
                right = jmax (right, bounds.getRight());
                top = jmin (top, bounds.getTop());
            }
        }

        const auto x = endpoint.getKind() == AudioGraphEndpoint::Kind::graphInput ? left - 96.0f : right + 96.0f;
        const auto y = top + 60.0f + static_cast<float> (endpoint.getBusIndex()) * 36.0f;
        return canvasToScreen ({ x, y });
    }

    if (auto* view = getNodeView (endpoint.getNodeID()))
    {
        const auto local = endpoint.getKind() == AudioGraphEndpoint::Kind::nodeInput
                             ? view->getInputPortCenter (endpoint.getBusIndex())
                             : view->getOutputPortCenter (endpoint.getBusIndex());

        return getLocalPoint (view, local);
    }

    return {};
}

Color AudioGraphComponent::getEndpointColor (const AudioGraphEndpoint& endpoint) const
{
    if (endpoint.getKind() == AudioGraphEndpoint::Kind::graphInput || endpoint.getKind() == AudioGraphEndpoint::Kind::graphOutput)
    {
        if (const auto* item = findNodeItemByKind (endpoint.getKind() == AudioGraphEndpoint::Kind::graphInput ? NodeItem::Kind::graphInput : NodeItem::Kind::graphOutput))
        {
            if (item->view != nullptr)
            {
                return endpoint.getKind() == AudioGraphEndpoint::Kind::graphInput
                         ? item->view->getOutputPortInfo (endpoint.getBusIndex()).color
                         : item->view->getInputPortInfo (endpoint.getBusIndex()).color;
            }
        }
    }

    if (auto* view = getNodeView (endpoint.getNodeID()))
    {
        if (endpoint.getKind() == AudioGraphEndpoint::Kind::nodeInput)
            return view->getInputPortInfo (endpoint.getBusIndex()).color;

        if (endpoint.getKind() == AudioGraphEndpoint::Kind::nodeOutput)
            return view->getOutputPortInfo (endpoint.getBusIndex()).color;
    }

    return AudioGraphNodeView::getPortKindColor (AudioGraphNodeView::PortKind::audio);
}

bool AudioGraphComponent::isPendingWireVisible() const noexcept
{
    return activeEndpoint.has_value() && (interaction == Interaction::armedPort || interaction == Interaction::draggingWire);
}

std::optional<AudioGraphEndpoint> AudioGraphComponent::getPendingWireEndpoint() const
{
    return activeEndpoint;
}

Point<float> AudioGraphComponent::getPendingWireEndPosition() const noexcept
{
    return interaction == Interaction::draggingWire ? pendingWireEnd : lastMouseScreen;
}

bool AudioGraphComponent::tryConnect (const AudioGraphEndpoint& first, const AudioGraphEndpoint& second)
{
    if (model == nullptr || ! isCompatiblePair (first, second))
        return false;

    const auto connection = makeConnection (first, second);

    if (onConnectionRequested == nullptr || ! onConnectionRequested (connection))
        return false;

    listeners.call (&Listener::connectionAdded, connection);

    repaint();
    return true;
}

bool AudioGraphComponent::requestConnectionRemoval (const AudioGraphConnection& connection)
{
    if (model == nullptr || onConnectionRemovalRequested == nullptr || ! onConnectionRemovalRequested (connection))
        return false;

    listeners.call (&Listener::connectionRemoved, connection);

    repaint();
    return true;
}

void AudioGraphComponent::removeConnectionsForEndpoint (const AudioGraphEndpoint& endpoint)
{
    if (model == nullptr || onEndpointConnectionsRemovalRequested == nullptr)
        return;

    const auto connections = model->getConnections();
    std::vector<AudioGraphConnection> removedConnections;

    for (const auto& connection : connections)
        if (connection.source == endpoint || connection.destination == endpoint)
            removedConnections.push_back (connection);

    if (removedConnections.empty())
        return;

    if (! onEndpointConnectionsRemovalRequested (endpoint))
        return;

    for (const auto& connection : removedConnections)
        listeners.call (&Listener::connectionRemoved, connection);

    repaint();
}

bool AudioGraphComponent::isCompatiblePair (const AudioGraphEndpoint& first, const AudioGraphEndpoint& second) const noexcept
{
    return (first.isSource() && second.isDestination()) || (second.isSource() && first.isDestination());
}

AudioGraphConnection AudioGraphComponent::makeConnection (const AudioGraphEndpoint& first, const AudioGraphEndpoint& second) const noexcept
{
    return first.isSource() ? AudioGraphConnection { first, second } : AudioGraphConnection { second, first };
}

Point<float> AudioGraphComponent::cubicPoint (Point<float> p0, Point<float> p1, Point<float> p2, Point<float> p3, float t) const noexcept
{
    const auto u = 1.0f - t;
    return (p0 * (u * u * u))
         + (p1 * (3.0f * u * u * t))
         + (p2 * (3.0f * u * t * t))
         + (p3 * (t * t * t));
}

float AudioGraphComponent::distanceToSegment (Point<float> point, Point<float> start, Point<float> end) const noexcept
{
    const auto segment = end - start;
    const auto lengthSquared = segment.distanceToSquared ({});

    if (lengthSquared <= 0.0f)
        return point.distanceTo (start);

    const auto pointVector = point - start;
    const auto t = jlimit (0.0f, 1.0f, ((pointVector.getX() * segment.getX()) + (pointVector.getY() * segment.getY())) / lengthSquared);
    return point.distanceTo (start + (segment * t));
}

} // namespace yup
