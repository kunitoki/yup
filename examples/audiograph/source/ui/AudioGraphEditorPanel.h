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

#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "../nodes/GraphEndpointNodeViews.h"
#include "../nodes/NodeRegistry.h"
#include "../nodes/SubgraphNode.h"

//==============================================================================
class AudioGraphEditorPanel final
    : public yup::Component
    , public yup::AudioGraphComponent::Listener
{
public:
    struct EndpointViews
    {
        std::function<std::unique_ptr<yup::AudioGraphNodeView>()> createInputView;
        std::function<std::unique_ptr<yup::AudioGraphNodeView>()> createOutputView;
        yup::Point<float> defaultInputPosition { 40.0f, 200.0f };
        yup::Point<float> defaultOutputPosition { 760.0f, 200.0f };
    };

    AudioGraphEditorPanel (std::shared_ptr<yup::AudioGraphProcessor> graphIn,
                           NodeRegistry& nodeRegistryIn,
                           yup::StringRef endpointSubtitleIn)
        : AudioGraphEditorPanel (graphIn,
                                 nodeRegistryIn,
                                 makeGraphEndpointViews (graphIn, endpointSubtitleIn))
    {
    }

    AudioGraphEditorPanel (std::shared_ptr<yup::AudioGraphProcessor> graphIn,
                           NodeRegistry& nodeRegistryIn,
                           EndpointViews endpointViewsIn)
        : graph (std::move (graphIn))
        , model (graph != nullptr ? graph->getModel() : nullptr)
        , nodeRegistry (nodeRegistryIn)
        , endpointViews (std::move (endpointViewsIn))
    {
        graphComponent = std::make_unique<yup::AudioGraphComponent> (graph, &undoManager);

        graphComponent->onConnectionRequested = [this] (const yup::AudioGraphConnection& connection)
        {
            return addConnection (connection);
        };

        graphComponent->onConnectionRemovalRequested = [this] (const yup::AudioGraphConnection& connection)
        {
            return removeConnection (connection);
        };

        graphComponent->onEndpointConnectionsRemovalRequested = [this] (const yup::AudioGraphEndpoint& endpoint)
        {
            return removeConnectionsForEndpoint (endpoint);
        };

        graphComponent->onNodeMoveRequested = [this] (yup::AudioGraphNodeID nodeID, yup::Point<float> oldCanvasPos, yup::Point<float> newCanvasPos)
        {
            return moveNode (nodeID, oldCanvasPos, newCanvasPos);
        };

        graphComponent->addListener (this);
        addAndMakeVisible (*graphComponent);

        reloadViews();
    }

    ~AudioGraphEditorPanel() override
    {
        graphComponent->removeListener (this);
    }

    std::shared_ptr<yup::AudioGraphProcessor> getGraph() const noexcept { return graph; }

    void clearUndoHistory()
    {
        undoManager.clear();
    }

    void zoomToFitNodes()
    {
        graphComponent->zoomToFitNodes();
    }

    void resized() override
    {
        graphComponent->setBounds (getLocalBounds());
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::Color (0xff0d1117));
        g.fillAll();
    }

    void nodeViewMoved (yup::AudioGraphNodeID nodeID, yup::Point<float> newCanvasPos) override
    {
        model->setNodePosition (nodeID, newCanvasPos.getX(), newCanvasPos.getY());
    }

    void nodeContextMenu (yup::AudioGraphNodeID id, yup::Point<float>) override
    {
        removeNode (id);
    }

    void nodeDoubleClicked (yup::AudioGraphNodeID id) override
    {
        if (onNodeDoubleClicked != nullptr)
            onNodeDoubleClicked (*this, id);
    }

    void canvasContextMenu (yup::Point<float> canvasPos) override
    {
        showAddNodeMenu (canvasPos);
    }

    void reloadViews (bool shouldZoomToFit = true)
    {
        clearNodeViews();

        for (auto nodeID : model->getNodeIDs())
            addViewForNode (nodeID);

        reloadBoundaryViews();

        if (shouldZoomToFit)
            graphComponent->zoomToFitNodes();
    }

    void clearNodeViews()
    {
        for (auto& [nodeID, _] : loadedNodes)
            graphComponent->removeNodeView (nodeID);

        loadedNodes.clear();
    }

    void refreshNodeView (yup::AudioGraphNodeID nodeID)
    {
        graphComponent->removeNodeView (nodeID);
        loadedNodes.erase (nodeID);
        addViewForNode (nodeID);
    }

    bool addProcessorNode (std::unique_ptr<yup::AudioProcessor> processor,
                           yup::AudioGraphNodeProperties props,
                           yup::Point<float> canvasPos)
    {
        const auto before = model->createSnapshot();
        const auto nodeID = addProcessorNodeWithoutUndo (std::move (processor), std::move (props), canvasPos);
        if (! nodeID.isValid())
            return false;

        addSnapshotUndoAction (before, model->createSnapshot(), "Add Audio Graph Node");
        return true;
    }

    void addInternalNode (const yup::String& identifier,
                          yup::Point<float> canvasPos,
                          yup::MemoryBlock creationData = {})
    {
        yup::AudioGraphNodeProperties props;
        props.identifier = identifier;
        props.name = identifier == NodeRegistry::subgraphIdentifier
                       ? SubgraphConfig::fromCreationData (creationData).getDisplayName()
                       : NodeRegistry::identifierToDisplayName (identifier);
        props.positionX = canvasPos.getX();
        props.positionY = canvasPos.getY();
        props.creationData = std::move (creationData);

        auto processorResult = nodeRegistry.makeProcessorFactory() (props);
        if (processorResult.failed())
        {
            sendStatusMessage ("Failed: " + processorResult.getErrorMessage());
            return;
        }

        addProcessorNode (std::move (processorResult).getValue(), std::move (props), canvasPos);
    }

    std::function<void (const yup::String&)> onStatusMessage;
    std::function<void (AudioGraphEditorPanel&, yup::AudioGraphNodeID)> onNodeDoubleClicked;
    std::function<void (AudioGraphEditorPanel&, yup::AudioGraphNodeID)> onNodeWillBeRemoved;

#if YUP_DESKTOP
    std::function<void (AudioGraphEditorPanel&, const yup::AudioPluginDescription&, yup::Point<float>)> onPluginSelected;
    std::function<const std::vector<yup::AudioPluginDescription>&()> getDiscoveredPlugins;
    std::function<yup::String (const yup::AudioPluginDescription&)> getPluginMenuItemText;
#endif

private:
    static EndpointViews makeGraphEndpointViews (std::shared_ptr<yup::AudioGraphProcessor> graph,
                                                 yup::String endpointSubtitle)
    {
        EndpointViews views;
        views.createInputView = [graph, endpointSubtitle]
        {
            return std::make_unique<GraphInputNodeView> (graph, endpointSubtitle);
        };
        views.createOutputView = [graph, endpointSubtitle]
        {
            return std::make_unique<GraphOutputNodeView> (graph, endpointSubtitle);
        };
        return views;
    }

    void reloadBoundaryViews()
    {
        if (endpointViews.createInputView != nullptr)
        {
            const auto input = model->getNodeProperties (yup::AudioGraphModel::getGraphInputNodeID())
                                   .value_or (makeEndpointProperties (endpointViews.defaultInputPosition));
            graphComponent->setGraphInputView (endpointViews.createInputView(),
                                               getBoundaryPositionOrDefault (input, endpointViews.defaultInputPosition));
        }

        if (endpointViews.createOutputView != nullptr)
        {
            const auto output = model->getNodeProperties (yup::AudioGraphModel::getGraphOutputNodeID())
                                    .value_or (makeEndpointProperties (endpointViews.defaultOutputPosition));
            graphComponent->setGraphOutputView (endpointViews.createOutputView(),
                                                getBoundaryPositionOrDefault (output, endpointViews.defaultOutputPosition));
        }
    }

    static yup::AudioGraphNodeProperties makeEndpointProperties (yup::Point<float> position)
    {
        yup::AudioGraphNodeProperties props;
        props.positionX = position.getX();
        props.positionY = position.getY();
        return props;
    }

    static yup::Point<float> getBoundaryPositionOrDefault (const yup::AudioGraphNodeProperties& props,
                                                           yup::Point<float> defaultPosition)
    {
        if (props.positionX == 0.0f && props.positionY == 0.0f)
            return defaultPosition;

        return { props.positionX, props.positionY };
    }

    void sendStatusMessage (const yup::String& message)
    {
        if (onStatusMessage != nullptr)
            onStatusMessage (message);
    }

    struct SnapshotAction final : public yup::UndoableAction
    {
        SnapshotAction (AudioGraphEditorPanel& panelToUse,
                        yup::AudioGraphModel::Snapshot beforeToUse,
                        yup::AudioGraphModel::Snapshot afterToUse)
            : panel (&panelToUse)
            , before (std::move (beforeToUse))
            , after (std::move (afterToUse))
        {
        }

        bool isValid() const override
        {
            return panel != nullptr;
        }

        bool perform (yup::UndoableActionState stateToPerform) override
        {
            if (panel == nullptr)
                return false;

            if (stateToPerform == yup::UndoableActionState::Redo && skipNextRedo)
            {
                skipNextRedo = false;
                return true;
            }

            return panel->restoreSnapshotForUndo (stateToPerform == yup::UndoableActionState::Redo ? after : before);
        }

    private:
        AudioGraphEditorPanel* panel = nullptr;
        yup::AudioGraphModel::Snapshot before;
        yup::AudioGraphModel::Snapshot after;
        bool skipNextRedo = true;
    };

    void addSnapshotUndoAction (const yup::AudioGraphModel::Snapshot& before,
                                const yup::AudioGraphModel::Snapshot& after,
                                yup::StringRef transactionName)
    {
        yup::UndoManager::ScopedTransaction transaction (undoManager, transactionName);
        undoManager.perform (new SnapshotAction (*this, before, after));
    }

    bool restoreSnapshotForUndo (const yup::AudioGraphModel::Snapshot& snapshot)
    {
        notifyNodesRemovedBySnapshot (snapshot);

        model->restoreSnapshot (snapshot);
        const auto result = graph->commitChanges();
        reloadViews (false);

        return result.wasOk();
    }

    void notifyNodesRemovedBySnapshot (const yup::AudioGraphModel::Snapshot& snapshot)
    {
        if (onNodeWillBeRemoved == nullptr)
            return;

        for (auto nodeID : model->getNodeIDs())
        {
            const auto willStillExist = std::any_of (snapshot.nodes.begin(), snapshot.nodes.end(), [nodeID] (const yup::AudioGraphModel::NodeSnapshot& node)
            {
                return node.kind == yup::AudioGraphModel::NodeKind::processor && node.id == nodeID;
            });

            if (! willStillExist)
                onNodeWillBeRemoved (*this, nodeID);
        }
    }

    yup::AudioGraphNodeID addProcessorNodeWithoutUndo (std::unique_ptr<yup::AudioProcessor> processor,
                                                       yup::AudioGraphNodeProperties props,
                                                       yup::Point<float> canvasPos)
    {
        const auto identifier = props.identifier;
        const auto nodeID = model->addNode (std::move (processor), std::move (props));
        if (! nodeID.isValid())
        {
            sendStatusMessage ("Failed to add node to graph.");
            return yup::AudioGraphNodeID::invalid();
        }

        const auto commitResult = graph->commitChanges();
        if (commitResult.failed())
        {
            model->removeNode (nodeID);
            graph->commitChanges();
            sendStatusMessage ("Failed: " + commitResult.getErrorMessage());
            return yup::AudioGraphNodeID::invalid();
        }

        auto* rawProc = model->getNodeProcessor (nodeID);
        auto view = nodeRegistry.createView (nodeID, identifier, rawProc, graph.get());
        if (view != nullptr)
        {
            graphComponent->addNodeView (nodeID, std::move (view), canvasPos);
            loadedNodes[nodeID] = identifier;
        }

        return nodeID;
    }

    void addViewForNode (yup::AudioGraphNodeID nodeID)
    {
        auto props = model->getNodeProperties (nodeID);
        if (! props.has_value())
            return;

        auto* proc = model->getNodeProcessor (nodeID);
        auto view = nodeRegistry.createView (nodeID, props->identifier, proc, graph.get());

        if (view != nullptr)
        {
            graphComponent->addNodeView (nodeID, std::move (view), { props->positionX, props->positionY });
            loadedNodes[nodeID] = props->identifier;
        }
    }

    void removeNode (yup::AudioGraphNodeID nodeID)
    {
        if (model->getNodeProcessor (nodeID) == nullptr)
            return;

        if (onNodeWillBeRemoved != nullptr)
            onNodeWillBeRemoved (*this, nodeID);

        const auto before = model->createSnapshot();

        graphComponent->removeNodeView (nodeID);
        if (! model->removeNode (nodeID))
            return;

        const auto commitResult = graph->commitChanges();
        if (commitResult.failed())
        {
            model->restoreSnapshot (before);
            graph->commitChanges();
            refreshNodeView (nodeID);
            sendStatusMessage ("Failed: " + commitResult.getErrorMessage());
            return;
        }

        loadedNodes.erase (nodeID);
        addSnapshotUndoAction (before, model->createSnapshot(), "Remove Audio Graph Node");
    }

    bool addConnection (const yup::AudioGraphConnection& connection)
    {
        if (model->addConnection (connection).failed())
            return false;

        if (graph->commitChanges().wasOk())
            return true;

        model->removeConnection (connection);
        graph->commitChanges();
        return false;
    }

    bool removeConnection (const yup::AudioGraphConnection& connection)
    {
        if (! model->removeConnection (connection))
            return false;

        if (graph->commitChanges().wasOk())
            return true;

        model->addConnection (connection);
        graph->commitChanges();
        return false;
    }

    bool removeConnectionsForEndpoint (const yup::AudioGraphEndpoint& endpoint)
    {
        const auto connections = model->getConnections();
        std::vector<yup::AudioGraphConnection> removedConnections;

        for (const auto& connection : connections)
        {
            if (connection.source == endpoint || connection.destination == endpoint)
            {
                if (model->removeConnection (connection))
                    removedConnections.push_back (connection);
            }
        }

        if (removedConnections.empty())
            return false;

        if (graph->commitChanges().wasOk())
            return true;

        for (const auto& connection : removedConnections)
            model->addConnection (connection);

        graph->commitChanges();
        return false;
    }

    bool moveNode (yup::AudioGraphNodeID nodeID, yup::Point<float>, yup::Point<float> newCanvasPos)
    {
        return model->setNodePosition (nodeID, newCanvasPos.getX(), newCanvasPos.getY());
    }

    void showAddNodeMenu (yup::Point<float> canvasPos)
    {
        pendingMenuCanvasPos = canvasPos;

        const auto screenPos = graphComponent->localToScreen (graphComponent->canvasToScreen (canvasPos));
        const auto options = yup::PopupMenu::Options {}
                                 .withFocusComponent (graphComponent.get())
                                 .withPosition (screenPos, yup::Justification::topLeft);

        auto internalSubMenu = yup::PopupMenu::create();

        auto internalNodes = nodeRegistry.getInternalNodeIdentifiers();
        for (const auto& identifier : internalNodes)
        {
            const auto displayName = NodeRegistry::identifierToDisplayName (identifier);
            internalSubMenu->addItem (displayName, static_cast<int> (internalSubMenu->getNumItems() + 1));
        }

        auto subgraphSubMenu = yup::PopupMenu::create();
        subgraphSubMenu->addItem ("Mono Subgraph", 10);
        subgraphSubMenu->addItem ("Stereo Subgraph", 11);
        subgraphSubMenu->addItem ("Mono Subgraph + MIDI", 12);
        subgraphSubMenu->addItem ("Stereo Subgraph + MIDI", 13);

        activeMenu = yup::PopupMenu::create (options);
        activeMenu->addSubMenu ("Internal Nodes", internalSubMenu);
        activeMenu->addSubMenu ("Subgraphs", subgraphSubMenu);

#if YUP_DESKTOP
        if (getDiscoveredPlugins != nullptr)
        {
            const auto& plugins = getDiscoveredPlugins();

            if (! plugins.empty())
            {
                auto pluginSubMenu = yup::PopupMenu::create();

                for (int i = 0; i < static_cast<int> (plugins.size()); ++i)
                {
                    const auto& desc = plugins[static_cast<size_t> (i)];
                    const auto menuText = getPluginMenuItemText != nullptr ? getPluginMenuItemText (desc) : desc.name;
                    pluginSubMenu->addItem (menuText, 100 + i);
                }

                activeMenu->addSubMenu ("Plugins", pluginSubMenu);
            }
            else
            {
                activeMenu->addItem ("No plugins (click Scan)", -1, false);
            }
        }
#endif

        activeMenu->show ([this, internalNodes = std::move (internalNodes)] (int selectedID)
        {
            if (selectedID <= 0)
                return;

            if (selectedID >= 1 && selectedID <= static_cast<int> (internalNodes.size()))
            {
                addInternalNode (internalNodes[selectedID - 1], pendingMenuCanvasPos);
            }
            else if (selectedID >= 10 && selectedID <= 13)
            {
                const auto preset = selectedID == 10 ? SubgraphConfig::mono
                                  : selectedID == 11 ? SubgraphConfig::stereo
                                  : selectedID == 12 ? SubgraphConfig::monoMidi
                                                     : SubgraphConfig::stereoMidi;
                const auto config = SubgraphConfig::fromPreset (preset);
                addInternalNode (NodeRegistry::subgraphIdentifier,
                                 pendingMenuCanvasPos,
                                 SubgraphConfig::toCreationData (config));
            }
#if YUP_DESKTOP
            else if (selectedID >= 100 && onPluginSelected != nullptr && getDiscoveredPlugins != nullptr)
            {
                const auto& plugins = getDiscoveredPlugins();
                const auto index = static_cast<size_t> (selectedID - 100);

                if (index < plugins.size())
                    onPluginSelected (*this, plugins[index], pendingMenuCanvasPos);
            }
#endif
        });
    }

    std::shared_ptr<yup::AudioGraphProcessor> graph;
    std::shared_ptr<yup::AudioGraphModel> model;
    NodeRegistry& nodeRegistry;
    EndpointViews endpointViews;
    yup::UndoManager undoManager;
    std::unique_ptr<yup::AudioGraphComponent> graphComponent;
    std::map<yup::AudioGraphNodeID, yup::String> loadedNodes;
    yup::PopupMenu::Ptr activeMenu;
    yup::Point<float> pendingMenuCanvasPos;
};
