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
/**
    Zoomable and pannable editor canvas for an AudioGraphProcessor.

    The component owns only the visual node views. The AudioGraphProcessor remains
    the source of truth for nodes and connections.

    @see AudioGraphNodeView, AudioGraphProcessor
*/
class YUP_API AudioGraphComponent : public Component
{
public:
    /** Style identifiers for theme customization. */
    struct Style
    {
        /** Canvas background color. */
        static const Identifier backgroundColorId;

        /** Grid dot color. */
        static const Identifier gridColorId;
    };

    /** Receives notifications for graph editor gestures that committed changes. */
    struct Listener
    {
        virtual ~Listener() = default;

        /** Called after a connection has been added and committed. */
        virtual void connectionAdded (const AudioGraphConnection&) {}

        /** Called after a connection has been removed and committed. */
        virtual void connectionRemoved (const AudioGraphConnection&) {}

        /** Called when a node view is released after dragging. */
        virtual void nodeViewMoved (AudioGraphNodeID, Point<float> newCanvasPos) {}
    };

    /** Creates an editor for graph. */
    explicit AudioGraphComponent (std::shared_ptr<AudioGraphProcessor> graph);

    /** Destructor. */
    ~AudioGraphComponent() override;

    /** Adds or replaces the view for a node at an initial canvas position. */
    void addNodeView (AudioGraphNodeID nodeID,
                      std::unique_ptr<AudioGraphNodeView> view,
                      Point<float> initialCanvasPosition);

    /** Adds or replaces the view that represents the graph input buses. */
    void setGraphInputView (std::unique_ptr<AudioGraphNodeView> view,
                            Point<float> initialCanvasPosition);

    /** Adds or replaces the view that represents the graph output buses. */
    void setGraphOutputView (std::unique_ptr<AudioGraphNodeView> view,
                             Point<float> initialCanvasPosition);

    /** Removes the view for a node. */
    void removeNodeView (AudioGraphNodeID nodeID);

    /** Returns the view for a node, or nullptr. */
    AudioGraphNodeView* getNodeView (AudioGraphNodeID nodeID) const noexcept;

    /** @internal Returns the graph processor currently being edited. */
    const AudioGraphProcessor* getGraphProcessor() const noexcept { return graph.get(); }

    /** Sets the zoom factor, clamped to [getMinZoom(), getMaxZoom()]. */
    void setZoom (float zoom);

    /** Returns the zoom factor. */
    float getZoom() const noexcept { return zoom; }

    /** Sets the minimum zoom factor. */
    void setMinZoom (float newMinZoom);

    /** Returns the minimum zoom factor. */
    float getMinZoom() const noexcept { return minZoom; }

    /** Sets the maximum zoom factor. */
    void setMaxZoom (float newMaxZoom);

    /** Returns the maximum zoom factor. */
    float getMaxZoom() const noexcept { return maxZoom; }

    /** Sets the screen-space hit distance used for selecting connection wires. */
    void setWireHitDistance (float newWireHitDistance);

    /** Returns the screen-space hit distance used for selecting connection wires. */
    float getWireHitDistance() const noexcept { return wireHitDistance; }

    /** Sets the drag distance needed before an armed port starts dragging a wire. */
    void setDragWireThreshold (float newDragWireThreshold);

    /** Returns the drag distance needed before an armed port starts dragging a wire. */
    float getDragWireThreshold() const noexcept { return dragWireThreshold; }

    /** Sets the canvas-to-screen offset. */
    void setCanvasOffset (Point<float> offset);

    /** Returns the canvas-to-screen offset. */
    Point<float> getCanvasOffset() const noexcept { return canvasOffset; }

    /** Resets zoom and centers the current node views. */
    void resetView();

    /** Centers the current node views without changing the zoom factor. */
    void centerViewOnNodes();

    /** Zooms and centers the current node views so they fit in the viewport. */
    void zoomToFitNodes (float padding = 48.0f, float maximumZoom = 1.0f);

    /** Converts a canvas-space point to component-local screen space. */
    Point<float> canvasToScreen (Point<float> canvasPos) const;

    /** Converts component-local screen space to canvas space. */
    Point<float> screenToCanvas (Point<float> screenPos) const;

    /** @internal Returns the screen position for an endpoint. */
    Point<float> getEndpointScreenPosition (const AudioGraphEndpoint& endpoint) const;

    /** @internal Returns the display color for an endpoint. */
    Color getEndpointColor (const AudioGraphEndpoint& endpoint) const;

    /** @internal Returns true when a pending connection wire should be painted. */
    bool isPendingWireVisible() const noexcept;

    /** @internal Returns the endpoint that started the pending connection wire. */
    std::optional<AudioGraphEndpoint> getPendingWireEndpoint() const;

    /** @internal Returns the current screen endpoint for the pending connection wire. */
    Point<float> getPendingWireEndPosition() const noexcept;

    /** Adds a listener. */
    void addListener (Listener* listener);

    /** Removes a listener. */
    void removeListener (Listener* listener);

    /**
        Called when the user right-clicks on an empty area of the canvas.
        canvasPos is the click position in canvas coordinates.
    */
    std::function<void (Point<float> canvasPos)> onCanvasContextMenu;

    /**
        Called when the user right-clicks on a node body (not a port or wire).
        nodeID identifies the node; canvasPos is the position in canvas coordinates.
    */
    std::function<void (AudioGraphNodeID nodeID, Point<float> canvasPos)> onNodeContextMenu;

    /**
        Called when the user double-clicks on a node body.
        nodeID identifies the clicked node.
    */
    std::function<void (AudioGraphNodeID nodeID)> onNodeDoubleClicked;

    /** @internal */
    void resized() override;
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void mouseDrag (const MouseEvent& event) override;
    /** @internal */
    void mouseUp (const MouseEvent& event) override;
    /** @internal */
    void mouseDoubleClick (const MouseEvent& event) override;
    /** @internal */
    void mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) override;
    /** @internal */
    void keyDown (const KeyPress& keys, const Point<float>& position) override;
    /** @internal */
    void keyUp (const KeyPress& keys, const Point<float>& position) override;

private:
    struct NodeItem
    {
        enum class Kind
        {
            processor,
            graphInput,
            graphOutput
        };

        AudioGraphNodeID nodeID;
        std::unique_ptr<AudioGraphNodeView> view;
        Point<float> canvasPosition;
        Kind kind = Kind::processor;
    };

    struct EndpointHit
    {
        AudioGraphEndpoint endpoint;
        AudioGraphNodeView::PortHit port;
        AudioGraphNodeView* view = nullptr;
    };

    enum class Interaction
    {
        idle,
        panningCanvas,
        draggingNode,
        draggingWire,
        armedPort
    };

    void updateNodeBounds();
    Rectangle<float> getNodeCanvasBounds (const NodeItem& item) const;
    std::optional<Rectangle<float>> getNodesCanvasBounds() const;

    Point<float> eventPositionInThisComponent (const MouseEvent& event) const;
    NodeItem* findNodeItemForView (const AudioGraphNodeView* view) noexcept;
    const NodeItem* findNodeItemForView (const AudioGraphNodeView* view) const noexcept;
    NodeItem* findNodeItemByKind (NodeItem::Kind kind) noexcept;
    const NodeItem* findNodeItemByKind (NodeItem::Kind kind) const noexcept;
    std::optional<EndpointHit> hitTestEndpoint (Point<float> screenPos) const;
    std::optional<AudioGraphConnection> hitTestConnection (Point<float> screenPos) const;

    bool tryConnect (const AudioGraphEndpoint& first, const AudioGraphEndpoint& second);
    bool removeConnectionAndCommit (const AudioGraphConnection& connection);
    void removeConnectionsForEndpoint (const AudioGraphEndpoint& endpoint);

    bool isCompatiblePair (const AudioGraphEndpoint& first, const AudioGraphEndpoint& second) const noexcept;
    AudioGraphConnection makeConnection (const AudioGraphEndpoint& first, const AudioGraphEndpoint& second) const noexcept;

    Point<float> cubicPoint (Point<float> p0, Point<float> p1, Point<float> p2, Point<float> p3, float t) const noexcept;
    float distanceToSegment (Point<float> point, Point<float> start, Point<float> end) const noexcept;

    std::shared_ptr<AudioGraphProcessor> graph;
    std::vector<NodeItem> nodes;
    ListenerList<Listener> listeners;

    Interaction interaction = Interaction::idle;
    float minZoom = 0.1f;
    float maxZoom = 4.0f;
    float wireHitDistance = 8.0f;
    float dragWireThreshold = 4.0f;
    float zoom = 1.0f;
    Point<float> canvasOffset { 0.0f, 0.0f };
    bool spacebarDown = false;
    bool needsInitialReset = true;
    bool needsInitialZoomToFit = false;
    float pendingZoomToFitPadding = 48.0f;
    float pendingZoomToFitMaximumZoom = 1.0f;

    Point<float> mouseDownScreen;
    Point<float> lastMouseScreen;
    Point<float> panStartOffset;
    Point<float> dragStartCanvasMouse;
    Point<float> dragStartNodePosition;
    int draggedNodeIndex = -1;

    std::optional<AudioGraphEndpoint> activeEndpoint;
    Point<float> pendingWireEnd;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioGraphComponent)
};

} // namespace yup
