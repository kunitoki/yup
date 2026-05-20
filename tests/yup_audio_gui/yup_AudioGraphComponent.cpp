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

#include <yup_audio_gui/yup_audio_gui.h>

#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

using namespace yup;

namespace
{
constexpr float pointTolerance = 0.001f;

AudioBusLayout stereoLayout()
{
    return AudioBusLayout ({ AudioBus ("Input", AudioBus::Type::Audio, AudioBus::Direction::Input, 2) },
                           { AudioBus ("Output", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) });
}

void expectPointNear (Point<float> actual, Point<float> expected, float tolerance = pointTolerance)
{
    EXPECT_NEAR (expected.getX(), actual.getX(), tolerance);
    EXPECT_NEAR (expected.getY(), actual.getY(), tolerance);
}

void expectRectangleInside (const Rectangle<float>& outer, const Rectangle<float>& inner)
{
    EXPECT_LE (outer.getLeft(), inner.getLeft());
    EXPECT_LE (outer.getTop(), inner.getTop());
    EXPECT_GE (outer.getRight(), inner.getRight());
    EXPECT_GE (outer.getBottom(), inner.getBottom());
}

class TestProcessor : public AudioProcessor
{
public:
    TestProcessor()
        : AudioProcessor ("Graph Component Test Processor", stereoLayout())
    {
    }

    void prepareToPlay (float, int) override {}

    void releaseResources() override {}

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override {}

    int getLatencySamples() override { return 0; }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    Result loadStateFromMemory (const MemoryBlock&) override { return Result::ok(); }

    Result saveStateIntoMemory (MemoryBlock&) override { return Result::ok(); }

    bool hasEditor() const override { return false; }
};

class TestNodeView : public AudioGraphNodeView
{
public:
    explicit TestNodeView (AudioGraphNodeID nodeID,
                           String titleToUse = "Node",
                           int inputsToUse = 1,
                           int outputsToUse = 1,
                           int parametersToUse = 0,
                           int preferredWidthToUse = 140)
        : AudioGraphNodeView (nodeID)
        , title (std::move (titleToUse))
        , inputs (inputsToUse)
        , outputs (outputsToUse)
        , parameters (parametersToUse)
        , preferredWidth (preferredWidthToUse)
    {
    }

    String getNodeTitle() const override { return title; }

    int getNumInputPorts() const override { return inputs; }

    int getNumOutputPorts() const override { return outputs; }

    int getNumParameterRows() const override { return parameters; }

    int getPreferredWidth() const override { return preferredWidth; }

    Color getNodeColor() const override { return nodeColor; }

    String getNodeSubtitle() const override { return subtitle; }

    PortInfo getInputPortInfo (int busIndex) const override
    {
        return { String ("input ") + String (busIndex), inputColor, PortKind::audio };
    }

    PortInfo getOutputPortInfo (int busIndex) const override
    {
        return { String ("output ") + String (busIndex), outputColor, PortKind::audio };
    }

    ParameterInfo getParameterInfo (int parameterIndex) const override
    {
        return { String ("parameter ") + String (parameterIndex),
                 String (parameterIndex),
                 parameterColor,
                 0.25f * static_cast<float> (parameterIndex + 1),
                 PortKind::parameter };
    }

    Color nodeColor = Color (0xff4466aa);
    Color inputColor = Color (0xff112233);
    Color outputColor = Color (0xff445566);
    Color parameterColor = Color (0xff778899);
    String subtitle = "Subtitle";

private:
    String title;
    int inputs = 1;
    int outputs = 1;
    int parameters = 0;
    int preferredWidth = 140;
};

class RecordingGraphListener : public AudioGraphComponent::Listener
{
public:
    void connectionAdded (const AudioGraphConnection& connection) override
    {
        addedConnections.push_back (connection);
    }

    void connectionRemoved (const AudioGraphConnection& connection) override
    {
        removedConnections.push_back (connection);
    }

    void nodeViewMoved (AudioGraphNodeID nodeID, Point<float> newCanvasPos) override
    {
        movedNodeID = nodeID;
        movedCanvasPosition = newCanvasPos;
        ++nodeMoveCount;
    }

    std::vector<AudioGraphConnection> addedConnections;
    std::vector<AudioGraphConnection> removedConnections;
    AudioGraphNodeID movedNodeID;
    Point<float> movedCanvasPosition;
    int nodeMoveCount = 0;
};

class AudioGraphComponentTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        model = std::make_shared<AudioGraphModel>();
        graph = std::make_shared<AudioGraphProcessor> (model);
        component = std::make_unique<AudioGraphComponent> (graph);
        component->setBounds (0.0f, 0.0f, 800.0f, 600.0f);
        component->onConnectionRequested = [this] (const AudioGraphConnection& connection)
        {
            return model->addConnection (connection).wasOk() && graph->commitChanges().wasOk();
        };
        component->onConnectionRemovalRequested = [this] (const AudioGraphConnection& connection)
        {
            return model->removeConnection (connection) && graph->commitChanges().wasOk();
        };
        component->onEndpointConnectionsRemovalRequested = [this] (const AudioGraphEndpoint& endpoint)
        {
            auto connections = model->getConnections();
            bool removedAny = false;

            for (const auto& connection : connections)
                if (connection.source == endpoint || connection.destination == endpoint)
                    removedAny = model->removeConnection (connection) || removedAny;

            return removedAny && graph->commitChanges().wasOk();
        };
        component->onNodeMoveRequested = [this] (AudioGraphNodeID nodeID, Point<float>, Point<float> newCanvasPos)
        {
            return model->setNodePosition (nodeID, newCanvasPos.getX(), newCanvasPos.getY());
        };
    }

    void TearDown() override
    {
        component.reset();
        model.reset();
        graph.reset();
    }

    AudioGraphNodeID addProcessorNode()
    {
        return model->addNode (std::make_unique<TestProcessor>());
    }

    TestNodeView* addNodeView (AudioGraphNodeID nodeID,
                               Point<float> canvasPosition,
                               int inputs = 1,
                               int outputs = 1,
                               int parameters = 0,
                               int preferredWidth = 140)
    {
        auto view = std::make_unique<TestNodeView> (nodeID, "Node", inputs, outputs, parameters, preferredWidth);
        auto* rawView = view.get();
        component->addNodeView (nodeID, std::move (view), canvasPosition);
        return rawView;
    }

    static MouseEvent mouseEventForComponent (MouseEvent::Buttons buttons, Point<float> position)
    {
        return MouseEvent (buttons, KeyModifiers(), position);
    }

    static MouseEvent mouseEventForSource (MouseEvent::Buttons buttons, Component* source, Point<float> positionInSource)
    {
        return MouseEvent (buttons, KeyModifiers(), positionInSource, source);
    }

    std::shared_ptr<AudioGraphProcessor> graph;
    std::shared_ptr<AudioGraphModel> model;
    std::unique_ptr<AudioGraphComponent> component;
};

} // namespace

//==============================================================================
// AudioGraphNodeView Tests
//==============================================================================

TEST (AudioGraphNodeViewTests, DefaultsDescribeBasicAudioNode)
{
    TestNodeView view (AudioGraphNodeID (1));

    EXPECT_EQ (AudioGraphNodeID (1), view.getNodeID());
    EXPECT_EQ (String ("Node"), view.getNodeTitle());
    EXPECT_EQ (String ("Subtitle"), view.getNodeSubtitle());
    EXPECT_EQ (1, view.getNumInputPorts());
    EXPECT_EQ (1, view.getNumOutputPorts());
    EXPECT_EQ (0, view.getNumParameterRows());
    EXPECT_EQ (140, view.getPreferredWidth());
    EXPECT_EQ (82, view.getPreferredHeight());
    EXPECT_EQ (Color (0xff4466aa), view.getNodeColor());
}

TEST (AudioGraphNodeViewTests, PreferredHeightAccountsForPortsAndParameterRows)
{
    TestNodeView onePortNoParameters (AudioGraphNodeID (1), "One", 0, 0, 0);
    TestNodeView threePortsTwoParameters (AudioGraphNodeID (2), "Three", 3, 2, 2);
    TestNodeView twoOutputsFourParameters (AudioGraphNodeID (3), "Outputs", 1, 2, 4);

    EXPECT_EQ (82, onePortNoParameters.getPreferredHeight());
    EXPECT_EQ (180, threePortsTwoParameters.getPreferredHeight());
    EXPECT_EQ (206, twoOutputsFourParameters.getPreferredHeight());
}

TEST (AudioGraphNodeViewTests, PortCentersUseScaledLocalGeometry)
{
    TestNodeView view (AudioGraphNodeID (1), "Scaled", 2, 2, 0);
    view.setBounds (0.0f, 0.0f, 140.0f, static_cast<float> (view.getPreferredHeight()));

    expectPointNear (view.getInputPortCenter (0), { 5.0f, 66.0f });
    expectPointNear (view.getInputPortCenter (1), { 5.0f, 90.0f });
    expectPointNear (view.getOutputPortCenter (0), { 135.0f, 66.0f });
    expectPointNear (view.getOutputPortCenter (1), { 135.0f, 90.0f });

    view.setViewScale (2.0f);
    view.setBounds (0.0f, 0.0f, 280.0f, static_cast<float> (view.getPreferredHeight()) * 2.0f);

    EXPECT_FLOAT_EQ (12.0f, view.getPortRadius());
    expectPointNear (view.getInputPortCenter (0), { 10.0f, 132.0f });
    expectPointNear (view.getOutputPortCenter (1), { 270.0f, 180.0f });
}

TEST (AudioGraphNodeViewTests, ViewScaleIsClamped)
{
    TestNodeView view (AudioGraphNodeID (1));

    view.setViewScale (0.001f);
    EXPECT_FLOAT_EQ (0.1f, view.getViewScale());
    EXPECT_FLOAT_EQ (0.6f, view.getPortRadius());

    view.setViewScale (12.0f);
    EXPECT_FLOAT_EQ (4.0f, view.getViewScale());
    EXPECT_FLOAT_EQ (24.0f, view.getPortRadius());
}

TEST (AudioGraphNodeViewTests, HitTestPortReturnsMatchingInputOrOutput)
{
    TestNodeView view (AudioGraphNodeID (1), "Ports", 2, 2, 0);
    view.setBounds (0.0f, 0.0f, 140.0f, static_cast<float> (view.getPreferredHeight()));

    auto inputHit = view.hitTestPort (view.getInputPortCenter (1));
    ASSERT_TRUE (inputHit.has_value());
    EXPECT_EQ (1, inputHit->busIndex);
    EXPECT_TRUE (inputHit->isInput);

    auto outputHit = view.hitTestPort (view.getOutputPortCenter (0));
    ASSERT_TRUE (outputHit.has_value());
    EXPECT_EQ (0, outputHit->busIndex);
    EXPECT_FALSE (outputHit->isInput);

    EXPECT_FALSE (view.hitTestPort ({ 70.0f, 12.0f }).has_value());
}

TEST (AudioGraphNodeViewTests, InvalidPortCentersReturnOrigin)
{
    TestNodeView view (AudioGraphNodeID (1), "Ports", 1, 1, 0);
    view.setBounds (0.0f, 0.0f, 140.0f, static_cast<float> (view.getPreferredHeight()));

    expectPointNear (view.getInputPortCenter (-1), Point<float>());
    expectPointNear (view.getInputPortCenter (1), Point<float>());
    expectPointNear (view.getOutputPortCenter (-1), Point<float>());
    expectPointNear (view.getOutputPortCenter (1), Point<float>());
}

TEST (AudioGraphNodeViewTests, PortKindColorsAreStable)
{
    EXPECT_EQ (Color (0xffffc43b), AudioGraphNodeView::getPortKindColor (AudioGraphNodeView::PortKind::audio));
    EXPECT_EQ (Color (0xffff4d67), AudioGraphNodeView::getPortKindColor (AudioGraphNodeView::PortKind::midi));
    EXPECT_EQ (Color (0xff2f8cff), AudioGraphNodeView::getPortKindColor (AudioGraphNodeView::PortKind::parameter));
}

//==============================================================================
// AudioGraphComponent View Management Tests
//==============================================================================

TEST_F (AudioGraphComponentTests, ConstructorStoresGraphAndInitialSettings)
{
    EXPECT_EQ (graph.get(), component->getGraphProcessor());
    EXPECT_FLOAT_EQ (1.0f, component->getZoom());
    EXPECT_FLOAT_EQ (0.1f, component->getMinZoom());
    EXPECT_FLOAT_EQ (4.0f, component->getMaxZoom());
    EXPECT_FLOAT_EQ (8.0f, component->getWireHitDistance());
    EXPECT_FLOAT_EQ (4.0f, component->getDragWireThreshold());
}

TEST_F (AudioGraphComponentTests, AddNodeViewAddsChildAndAppliesCanvasTransform)
{
    const auto nodeID = addProcessorNode();
    auto* view = addNodeView (nodeID, { 100.0f, 50.0f });

    ASSERT_EQ (view, component->getNodeView (nodeID));
    EXPECT_EQ (1, component->getNumChildComponents());
    EXPECT_EQ (component.get(), view->getParentComponent());
    expectPointNear (view->getBounds().getPosition(), component->canvasToScreen ({ 100.0f, 50.0f }));
    EXPECT_FLOAT_EQ (static_cast<float> (view->getPreferredWidth()), view->getWidth());
    EXPECT_FLOAT_EQ (static_cast<float> (view->getPreferredHeight()), view->getHeight());
}

TEST_F (AudioGraphComponentTests, AddNodeViewReplacesExistingViewForNode)
{
    const auto nodeID = addProcessorNode();
    auto* firstView = addNodeView (nodeID, { 100.0f, 50.0f });
    ASSERT_EQ (firstView, component->getNodeView (nodeID));

    auto replacement = std::make_unique<TestNodeView> (nodeID, "Replacement", 2, 2, 1, 180);
    auto* replacementView = replacement.get();
    component->addNodeView (nodeID, std::move (replacement), { 20.0f, 30.0f });

    EXPECT_EQ (replacementView, component->getNodeView (nodeID));
    EXPECT_EQ (1, component->getNumChildComponents());
    EXPECT_EQ (component.get(), replacementView->getParentComponent());
    expectPointNear (replacementView->getBounds().getPosition(), component->canvasToScreen ({ 20.0f, 30.0f }));
}

TEST_F (AudioGraphComponentTests, RemoveNodeViewRemovesOnlyProcessorViews)
{
    const auto firstNodeID = addProcessorNode();
    const auto secondNodeID = addProcessorNode();

    addNodeView (firstNodeID, { 0.0f, 0.0f });
    auto* secondView = addNodeView (secondNodeID, { 200.0f, 0.0f });
    ASSERT_EQ (2, component->getNumChildComponents());

    component->removeNodeView (firstNodeID);

    EXPECT_EQ (nullptr, component->getNodeView (firstNodeID));
    EXPECT_EQ (secondView, component->getNodeView (secondNodeID));
    EXPECT_EQ (1, component->getNumChildComponents());

    component->removeNodeView (AudioGraphNodeID (999));
    EXPECT_EQ (1, component->getNumChildComponents());
}

TEST_F (AudioGraphComponentTests, GraphInputAndOutputViewsAreReplaceable)
{
    auto input = std::make_unique<TestNodeView> (AudioGraphNodeID::invalid(), "Input", 0, 2);
    auto* inputView = input.get();
    component->setGraphInputView (std::move (input), { -200.0f, 10.0f });

    auto output = std::make_unique<TestNodeView> (AudioGraphNodeID::invalid(), "Output", 2, 0);
    auto* outputView = output.get();
    component->setGraphOutputView (std::move (output), { 400.0f, 10.0f });

    EXPECT_EQ (2, component->getNumChildComponents());
    EXPECT_EQ (component.get(), inputView->getParentComponent());
    EXPECT_EQ (component.get(), outputView->getParentComponent());

    auto replacement = std::make_unique<TestNodeView> (AudioGraphNodeID::invalid(), "Input Replacement", 0, 1);
    auto* replacementView = replacement.get();
    component->setGraphInputView (std::move (replacement), { -120.0f, 20.0f });

    EXPECT_EQ (2, component->getNumChildComponents());
    EXPECT_EQ (component.get(), replacementView->getParentComponent());
    expectPointNear (replacementView->getBounds().getPosition(), component->canvasToScreen ({ -120.0f, 20.0f }));
}

//==============================================================================
// AudioGraphComponent Viewport Tests
//==============================================================================

TEST_F (AudioGraphComponentTests, ZoomAndOffsetRoundTripCanvasCoordinates)
{
    component->setCanvasOffset ({ 20.0f, -40.0f });
    component->setZoom (2.5f);

    const Point<float> canvasPoint { 32.0f, 48.0f };
    const auto screenPoint = component->canvasToScreen (canvasPoint);

    expectPointNear (screenPoint, { 100.0f, 80.0f });
    expectPointNear (component->screenToCanvas (screenPoint), canvasPoint);
}

TEST_F (AudioGraphComponentTests, ZoomAndThresholdSettersClampValues)
{
    component->setMinZoom (0.5f);
    component->setMaxZoom (2.0f);

    component->setZoom (0.1f);
    EXPECT_FLOAT_EQ (0.5f, component->getZoom());

    component->setZoom (8.0f);
    EXPECT_FLOAT_EQ (2.0f, component->getZoom());

    component->setMinZoom (3.0f);
    EXPECT_FLOAT_EQ (3.0f, component->getMinZoom());
    EXPECT_FLOAT_EQ (3.0f, component->getMaxZoom());
    EXPECT_FLOAT_EQ (3.0f, component->getZoom());

    component->setWireHitDistance (-4.0f);
    component->setDragWireThreshold (-9.0f);

    EXPECT_FLOAT_EQ (0.0f, component->getWireHitDistance());
    EXPECT_FLOAT_EQ (0.0f, component->getDragWireThreshold());
}

TEST_F (AudioGraphComponentTests, CenterViewOnNodesCentersSingleNode)
{
    const auto nodeID = addProcessorNode();
    auto* view = addNodeView (nodeID, { 100.0f, 50.0f });

    component->centerViewOnNodes();

    expectPointNear (view->getBounds().getCenter(), component->getLocalBounds().getCenter());
}

TEST_F (AudioGraphComponentTests, CenterViewOnEmptyGraphUsesComponentCenterAsOffset)
{
    component->setCanvasOffset ({ 10.0f, 20.0f });
    component->centerViewOnNodes();

    expectPointNear (component->getCanvasOffset(), component->getLocalBounds().getCenter());
}

TEST_F (AudioGraphComponentTests, ResetViewRestoresUnitZoomAndCentersNodes)
{
    const auto nodeID = addProcessorNode();
    auto* view = addNodeView (nodeID, { 150.0f, 75.0f });

    component->setZoom (2.0f);
    component->setCanvasOffset ({ 0.0f, 0.0f });
    component->resetView();

    EXPECT_FLOAT_EQ (1.0f, component->getZoom());
    expectPointNear (view->getBounds().getCenter(), component->getLocalBounds().getCenter());
}

TEST_F (AudioGraphComponentTests, ZoomToFitNodesFitsAllViewsInsidePadding)
{
    const auto firstNodeID = addProcessorNode();
    const auto secondNodeID = addProcessorNode();
    auto* firstView = addNodeView (firstNodeID, { 0.0f, 0.0f });
    auto* secondView = addNodeView (secondNodeID, { 600.0f, 420.0f }, 2, 2, 1, 180);

    component->zoomToFitNodes (50.0f, 1.0f);

    const Rectangle<float> paddedViewport = component->getLocalBounds().reduced (50.0f);
    expectRectangleInside (paddedViewport, firstView->getBounds());
    expectRectangleInside (paddedViewport, secondView->getBounds());
    EXPECT_LE (component->getZoom(), 1.0f);
}

TEST_F (AudioGraphComponentTests, DeferredViewportOperationsRunOnResize)
{
    auto deferredComponent = std::make_unique<AudioGraphComponent> (graph);
    const auto nodeID = addProcessorNode();
    auto view = std::make_unique<TestNodeView> (nodeID);
    auto* rawView = view.get();

    deferredComponent->addNodeView (nodeID, std::move (view), { 100.0f, 100.0f });
    deferredComponent->zoomToFitNodes (24.0f, 0.75f);
    deferredComponent->setBounds (0.0f, 0.0f, 320.0f, 240.0f);

    EXPECT_LE (deferredComponent->getZoom(), 0.75f);
    EXPECT_EQ (deferredComponent.get(), rawView->getParentComponent());
}

TEST_F (AudioGraphComponentTests, MouseWheelZoomsAroundCursor)
{
    const auto cursor = Point<float> { 280.0f, 220.0f };
    const auto canvasBefore = component->screenToCanvas (cursor);

    component->mouseWheel (mouseEventForComponent (MouseEvent::noButtons, cursor), MouseWheelData (0.0f, 2.0f));

    EXPECT_GT (component->getZoom(), 1.0f);
    expectPointNear (component->screenToCanvas (cursor), canvasBefore);
}

TEST_F (AudioGraphComponentTests, SpacebarDragPansCanvas)
{
    const auto originalOffset = component->getCanvasOffset();

    component->keyDown (KeyPress (KeyPress::spaceKey), Point<float>());
    component->mouseDown (mouseEventForComponent (MouseEvent::leftButton, { 100.0f, 100.0f }));
    component->mouseDrag (mouseEventForComponent (MouseEvent::leftButton, { 130.0f, 145.0f }));
    component->mouseUp (mouseEventForComponent (MouseEvent::leftButton, { 130.0f, 145.0f }));
    component->keyUp (KeyPress (KeyPress::spaceKey), Point<float>());

    expectPointNear (component->getCanvasOffset(), originalOffset + Point<float> (30.0f, 45.0f));
}

//==============================================================================
// AudioGraphComponent Endpoint Tests
//==============================================================================

TEST_F (AudioGraphComponentTests, EndpointScreenPositionUsesNodePortCenters)
{
    const auto nodeID = addProcessorNode();
    auto* view = addNodeView (nodeID, { 100.0f, 50.0f }, 2, 2);

    expectPointNear (component->getEndpointScreenPosition (AudioGraphEndpoint::nodeInput (nodeID, 1)),
                     component->getLocalPoint (view, view->getInputPortCenter (1)));
    expectPointNear (component->getEndpointScreenPosition (AudioGraphEndpoint::nodeOutput (nodeID, 0)),
                     component->getLocalPoint (view, view->getOutputPortCenter (0)));
}

TEST_F (AudioGraphComponentTests, EndpointScreenPositionUsesGraphBoundaryViewsWhenPresent)
{
    auto input = std::make_unique<TestNodeView> (AudioGraphNodeID::invalid(), "Input", 0, 2);
    auto* inputView = input.get();
    component->setGraphInputView (std::move (input), { -200.0f, 10.0f });

    auto output = std::make_unique<TestNodeView> (AudioGraphNodeID::invalid(), "Output", 2, 0);
    auto* outputView = output.get();
    component->setGraphOutputView (std::move (output), { 400.0f, 10.0f });

    expectPointNear (component->getEndpointScreenPosition (AudioGraphEndpoint::graphInput (1)),
                     component->getLocalPoint (inputView, inputView->getOutputPortCenter (1)));
    expectPointNear (component->getEndpointScreenPosition (AudioGraphEndpoint::graphOutput (0)),
                     component->getLocalPoint (outputView, outputView->getInputPortCenter (0)));
}

TEST_F (AudioGraphComponentTests, EndpointScreenPositionFallsBackForMissingGraphBoundaryViews)
{
    const auto nodeID = addProcessorNode();
    addNodeView (nodeID, { 100.0f, 50.0f });

    expectPointNear (component->getEndpointScreenPosition (AudioGraphEndpoint::graphInput (0)),
                     component->canvasToScreen ({ 4.0f, 110.0f }));
    expectPointNear (component->getEndpointScreenPosition (AudioGraphEndpoint::graphOutput (1)),
                     component->canvasToScreen ({ 336.0f, 146.0f }));
}

TEST_F (AudioGraphComponentTests, EndpointColorUsesPortMetadataAndFallback)
{
    const auto nodeID = addProcessorNode();
    auto* view = addNodeView (nodeID, { 100.0f, 50.0f });
    view->inputColor = Color (0xff010203);
    view->outputColor = Color (0xff040506);

    EXPECT_EQ (Color (0xff010203), component->getEndpointColor (AudioGraphEndpoint::nodeInput (nodeID, 0)));
    EXPECT_EQ (Color (0xff040506), component->getEndpointColor (AudioGraphEndpoint::nodeOutput (nodeID, 0)));
    EXPECT_EQ (AudioGraphNodeView::getPortKindColor (AudioGraphNodeView::PortKind::audio),
               component->getEndpointColor (AudioGraphEndpoint::nodeOutput (AudioGraphNodeID (999), 0)));
}

TEST_F (AudioGraphComponentTests, GraphBoundaryEndpointColorUsesOppositeSidePortMetadata)
{
    auto input = std::make_unique<TestNodeView> (AudioGraphNodeID::invalid(), "Input", 0, 1);
    auto* inputView = input.get();
    inputView->outputColor = Color (0xff101112);
    component->setGraphInputView (std::move (input), { -200.0f, 10.0f });

    auto output = std::make_unique<TestNodeView> (AudioGraphNodeID::invalid(), "Output", 1, 0);
    auto* outputView = output.get();
    outputView->inputColor = Color (0xff131415);
    component->setGraphOutputView (std::move (output), { 400.0f, 10.0f });

    EXPECT_EQ (Color (0xff101112), component->getEndpointColor (AudioGraphEndpoint::graphInput (0)));
    EXPECT_EQ (Color (0xff131415), component->getEndpointColor (AudioGraphEndpoint::graphOutput (0)));
}

TEST_F (AudioGraphComponentTests, MouseDownOnEndpointArmsPendingWire)
{
    const auto nodeID = addProcessorNode();
    addNodeView (nodeID, { 100.0f, 50.0f });

    const auto endpoint = AudioGraphEndpoint::nodeOutput (nodeID, 0);
    const auto screenPosition = component->getEndpointScreenPosition (endpoint);

    EXPECT_FALSE (component->isPendingWireVisible());

    component->mouseDown (mouseEventForComponent (MouseEvent::leftButton, screenPosition));

    ASSERT_TRUE (component->isPendingWireVisible());
    ASSERT_TRUE (component->getPendingWireEndpoint().has_value());
    EXPECT_EQ (endpoint, *component->getPendingWireEndpoint());
    expectPointNear (component->getPendingWireEndPosition(), screenPosition);
}

TEST_F (AudioGraphComponentTests, DragPastThresholdTurnsPendingWireIntoDraggedWire)
{
    const auto nodeID = addProcessorNode();
    addNodeView (nodeID, { 100.0f, 50.0f });

    const auto start = component->getEndpointScreenPosition (AudioGraphEndpoint::nodeOutput (nodeID, 0));
    const auto end = start + Point<float> (30.0f, 10.0f);

    component->mouseDown (mouseEventForComponent (MouseEvent::leftButton, start));
    component->mouseDrag (mouseEventForComponent (MouseEvent::leftButton, end));

    EXPECT_TRUE (component->isPendingWireVisible());
    expectPointNear (component->getPendingWireEndPosition(), end);
}

TEST_F (AudioGraphComponentTests, ClickingAwayClearsArmedPendingWire)
{
    const auto nodeID = addProcessorNode();
    addNodeView (nodeID, { 100.0f, 50.0f });

    const auto endpointPosition = component->getEndpointScreenPosition (AudioGraphEndpoint::nodeOutput (nodeID, 0));
    component->mouseDown (mouseEventForComponent (MouseEvent::leftButton, endpointPosition));
    ASSERT_TRUE (component->isPendingWireVisible());

    component->mouseDown (mouseEventForComponent (MouseEvent::leftButton, { 20.0f, 20.0f }));

    EXPECT_FALSE (component->isPendingWireVisible());
    EXPECT_FALSE (component->getPendingWireEndpoint().has_value());
}

//==============================================================================
// AudioGraphComponent Interaction Tests
//==============================================================================

TEST_F (AudioGraphComponentTests, ClickingCompatibleEndpointsAddsConnectionAndNotifiesListener)
{
    const auto nodeID = addProcessorNode();
    addNodeView (nodeID, { 100.0f, 50.0f });

    auto inputView = std::make_unique<TestNodeView> (AudioGraphNodeID::invalid(), "Graph Input", 0, 1);
    component->setGraphInputView (std::move (inputView), { -120.0f, 50.0f });

    RecordingGraphListener listener;
    component->addListener (&listener);

    const auto source = AudioGraphEndpoint::graphInput (0);
    const auto destination = AudioGraphEndpoint::nodeInput (nodeID, 0);

    component->mouseDown (mouseEventForComponent (MouseEvent::leftButton, component->getEndpointScreenPosition (source)));
    component->mouseDown (mouseEventForComponent (MouseEvent::leftButton, component->getEndpointScreenPosition (destination)));

    const auto connections = model->getConnections();
    ASSERT_EQ (1u, connections.size());
    EXPECT_EQ (AudioGraphConnection (source, destination), connections.front());

    ASSERT_EQ (1u, listener.addedConnections.size());
    EXPECT_EQ (AudioGraphConnection (source, destination), listener.addedConnections.front());

    component->removeListener (&listener);
}

TEST_F (AudioGraphComponentTests, DraggingCompatibleEndpointsAddsConnectionAndClearsPendingWire)
{
    const auto nodeID = addProcessorNode();
    addNodeView (nodeID, { 100.0f, 50.0f });

    auto inputView = std::make_unique<TestNodeView> (AudioGraphNodeID::invalid(), "Graph Input", 0, 1);
    component->setGraphInputView (std::move (inputView), { -120.0f, 50.0f });

    const auto source = AudioGraphEndpoint::graphInput (0);
    const auto destination = AudioGraphEndpoint::nodeInput (nodeID, 0);
    const auto sourcePosition = component->getEndpointScreenPosition (source);
    const auto destinationPosition = component->getEndpointScreenPosition (destination);

    component->mouseDown (mouseEventForComponent (MouseEvent::leftButton, sourcePosition));
    component->mouseDrag (mouseEventForComponent (MouseEvent::leftButton, sourcePosition + Point<float> (20.0f, 0.0f)));
    component->mouseUp (mouseEventForComponent (MouseEvent::leftButton, destinationPosition));

    const auto connections = model->getConnections();
    ASSERT_EQ (1u, connections.size());
    EXPECT_EQ (AudioGraphConnection (source, destination), connections.front());
    EXPECT_FALSE (component->isPendingWireVisible());
    EXPECT_FALSE (component->getPendingWireEndpoint().has_value());
}

TEST_F (AudioGraphComponentTests, IncompatibleEndpointPairKeepsNewEndpointArmedAndDoesNotConnect)
{
    const auto firstNodeID = addProcessorNode();
    const auto secondNodeID = addProcessorNode();
    addNodeView (firstNodeID, { 100.0f, 50.0f });
    addNodeView (secondNodeID, { 320.0f, 50.0f });

    const auto firstInput = AudioGraphEndpoint::nodeInput (firstNodeID, 0);
    const auto secondInput = AudioGraphEndpoint::nodeInput (secondNodeID, 0);

    component->mouseDown (mouseEventForComponent (MouseEvent::leftButton, component->getEndpointScreenPosition (firstInput)));
    component->mouseDown (mouseEventForComponent (MouseEvent::leftButton, component->getEndpointScreenPosition (secondInput)));

    EXPECT_TRUE (model->getConnections().empty());
    ASSERT_TRUE (component->getPendingWireEndpoint().has_value());
    EXPECT_EQ (secondInput, *component->getPendingWireEndpoint());
}

TEST_F (AudioGraphComponentTests, RightClickEndpointRemovesAllConnectionsForEndpoint)
{
    const auto nodeID = addProcessorNode();
    addNodeView (nodeID, { 100.0f, 50.0f });

    auto inputView = std::make_unique<TestNodeView> (AudioGraphNodeID::invalid(), "Graph Input", 0, 1);
    component->setGraphInputView (std::move (inputView), { -120.0f, 50.0f });

    const AudioGraphConnection connection { AudioGraphEndpoint::graphInput (0),
                                            AudioGraphEndpoint::nodeInput (nodeID, 0) };
    ASSERT_TRUE (model->addConnection (connection).wasOk());
    ASSERT_TRUE (graph->commitChanges().wasOk());

    RecordingGraphListener listener;
    component->addListener (&listener);

    component->mouseDown (mouseEventForComponent (MouseEvent::rightButton, component->getEndpointScreenPosition (connection.source)));

    EXPECT_TRUE (model->getConnections().empty());
    ASSERT_EQ (1u, listener.removedConnections.size());
    EXPECT_EQ (connection, listener.removedConnections.front());

    component->removeListener (&listener);
}

TEST_F (AudioGraphComponentTests, RightClickEndpointRemovesMultipleConnectionsForEndpoint)
{
    const auto firstNodeID = addProcessorNode();
    const auto secondNodeID = addProcessorNode();
    addNodeView (firstNodeID, { 100.0f, 50.0f });
    addNodeView (secondNodeID, { 100.0f, 190.0f });

    auto inputView = std::make_unique<TestNodeView> (AudioGraphNodeID::invalid(), "Graph Input", 0, 1);
    component->setGraphInputView (std::move (inputView), { -120.0f, 50.0f });

    const AudioGraphConnection firstConnection { AudioGraphEndpoint::graphInput (0),
                                                 AudioGraphEndpoint::nodeInput (firstNodeID, 0) };
    const AudioGraphConnection secondConnection { AudioGraphEndpoint::graphInput (0),
                                                  AudioGraphEndpoint::nodeInput (secondNodeID, 0) };

    ASSERT_TRUE (model->addConnection (firstConnection).wasOk());
    ASSERT_TRUE (model->addConnection (secondConnection).wasOk());
    ASSERT_TRUE (graph->commitChanges().wasOk());

    RecordingGraphListener listener;
    component->addListener (&listener);

    component->mouseDown (mouseEventForComponent (MouseEvent::rightButton, component->getEndpointScreenPosition (firstConnection.source)));

    EXPECT_TRUE (model->getConnections().empty());
    ASSERT_EQ (2u, listener.removedConnections.size());
    EXPECT_EQ (firstConnection, listener.removedConnections[0]);
    EXPECT_EQ (secondConnection, listener.removedConnections[1]);

    component->removeListener (&listener);
}

TEST_F (AudioGraphComponentTests, RightClickConnectionRemovesSingleConnection)
{
    const auto nodeID = addProcessorNode();
    addNodeView (nodeID, { 100.0f, 50.0f });

    auto inputView = std::make_unique<TestNodeView> (AudioGraphNodeID::invalid(), "Graph Input", 0, 1);
    component->setGraphInputView (std::move (inputView), { -120.0f, 50.0f });

    const AudioGraphConnection connection { AudioGraphEndpoint::graphInput (0),
                                            AudioGraphEndpoint::nodeInput (nodeID, 0) };
    ASSERT_TRUE (model->addConnection (connection).wasOk());
    ASSERT_TRUE (graph->commitChanges().wasOk());

    const auto midpoint = (component->getEndpointScreenPosition (connection.source)
                           + component->getEndpointScreenPosition (connection.destination))
                        * 0.5f;
    component->setWireHitDistance (64.0f);

    component->mouseDown (mouseEventForComponent (MouseEvent::rightButton, midpoint));

    EXPECT_TRUE (model->getConnections().empty());
}

TEST_F (AudioGraphComponentTests, DraggingNodeUpdatesCanvasPositionAndNotifiesListener)
{
    const auto nodeID = addProcessorNode();
    auto* view = addNodeView (nodeID, { 100.0f, 50.0f });
    RecordingGraphListener listener;
    component->addListener (&listener);

    const auto localStart = view->getLocalBounds().getCenter();
    component->mouseDown (mouseEventForSource (MouseEvent::leftButton, view, localStart));
    component->mouseDrag (mouseEventForSource (MouseEvent::leftButton, view, localStart + Point<float> (25.0f, 35.0f)));
    component->mouseUp (mouseEventForComponent (MouseEvent::leftButton, component->getLocalBounds().getCenter()));

    EXPECT_EQ (1, listener.nodeMoveCount);
    EXPECT_EQ (nodeID, listener.movedNodeID);
    expectPointNear (listener.movedCanvasPosition, { 125.0f, 85.0f });
    expectPointNear (view->getBounds().getPosition(), component->canvasToScreen ({ 125.0f, 85.0f }));

    component->removeListener (&listener);
}

TEST_F (AudioGraphComponentTests, DraggingGraphBoundaryViewDoesNotNotifyNodeMoveListener)
{
    auto graphInput = std::make_unique<TestNodeView> (AudioGraphNodeID::invalid(), "Graph Input", 0, 1);
    auto* graphInputView = graphInput.get();
    component->setGraphInputView (std::move (graphInput), { -120.0f, 50.0f });

    RecordingGraphListener listener;
    component->addListener (&listener);

    const auto localStart = graphInputView->getLocalBounds().getCenter();
    component->mouseDown (mouseEventForSource (MouseEvent::leftButton, graphInputView, localStart));
    component->mouseDrag (mouseEventForSource (MouseEvent::leftButton, graphInputView, localStart + Point<float> (25.0f, 35.0f)));
    component->mouseUp (mouseEventForComponent (MouseEvent::leftButton, component->getLocalBounds().getCenter()));

    EXPECT_EQ (0, listener.nodeMoveCount);
    expectPointNear (graphInputView->getBounds().getPosition(), component->canvasToScreen ({ -95.0f, 85.0f }));

    component->removeListener (&listener);
}
