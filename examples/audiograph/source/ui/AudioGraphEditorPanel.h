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
    AudioGraphEditorPanel (std::shared_ptr<yup::AudioGraphProcessor> graphIn,
                           NodeRegistry& nodeRegistryIn,
                           yup::StringRef endpointSubtitleIn)
        : graph (std::move (graphIn))
        , nodeRegistry (nodeRegistryIn)
        , endpointSubtitle (endpointSubtitleIn)
    {
        graphComponent = std::make_unique<yup::AudioGraphComponent> (graph);
        graphComponent->onCanvasContextMenu = [this] (yup::Point<float> canvasPos)
        {
            showAddNodeMenu (canvasPos);
        };
        graphComponent->onNodeContextMenu = [this] (yup::AudioGraphNodeID id, yup::Point<float>)
        {
            removeNode (id);
        };
        graphComponent->onNodeDoubleClicked = [this] (yup::AudioGraphNodeID id)
        {
            if (onNodeDoubleClicked != nullptr)
                onNodeDoubleClicked (*this, id);
        };
        graphComponent->addListener (this);
        addAndMakeVisible (*graphComponent);

        reloadViews();
    }

    ~AudioGraphEditorPanel() override
    {
        graphComponent->removeListener (this);
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
        graph->setNodePosition (nodeID, newCanvasPos.getX(), newCanvasPos.getY());
    }

    std::shared_ptr<yup::AudioGraphProcessor> getGraph() const noexcept { return graph; }

    void reloadViews()
    {
        for (auto& [nodeID, _] : loadedNodes)
            graphComponent->removeNodeView (nodeID);

        loadedNodes.clear();

        for (auto nodeID : graph->getNodeIDs())
            addViewForNode (nodeID);

        graphComponent->setGraphInputView (std::make_unique<GraphInputNodeView> (graph, endpointSubtitle), { 40.0f, 200.0f });
        graphComponent->setGraphOutputView (std::make_unique<GraphOutputNodeView> (graph, endpointSubtitle), { 760.0f, 200.0f });
        graphComponent->zoomToFitNodes();
    }

    void refreshNodeView (yup::AudioGraphNodeID nodeID)
    {
        graphComponent->removeNodeView (nodeID);
        loadedNodes.erase (nodeID);
        addViewForNode (nodeID);
    }

    void addProcessorNode (std::unique_ptr<yup::AudioProcessor> processor,
                           yup::AudioGraphNodeProperties props,
                           yup::Point<float> canvasPos)
    {
        const auto identifier = props.identifier;
        const auto nodeID = graph->addNode (std::move (processor), std::move (props));
        if (! nodeID.isValid())
            return;

        graph->commitChanges();

        auto* rawProc = graph->getNodeProcessor (nodeID);
        auto view = nodeRegistry.createView (nodeID, identifier, rawProc, graph.get());
        if (view != nullptr)
        {
            graphComponent->addNodeView (nodeID, std::move (view), canvasPos);
            loadedNodes[nodeID] = identifier;
        }
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
            return;

        addProcessorNode (std::move (processorResult).getValue(), std::move (props), canvasPos);
    }

    std::function<void (AudioGraphEditorPanel&, yup::AudioGraphNodeID)> onNodeDoubleClicked;
    std::function<void (AudioGraphEditorPanel&, yup::AudioGraphNodeID)> onNodeWillBeRemoved;

#if YUP_DESKTOP
    std::function<void (AudioGraphEditorPanel&, const yup::AudioPluginDescription&, yup::Point<float>)> onPluginSelected;
    std::function<const std::vector<yup::AudioPluginDescription>&()> getDiscoveredPlugins;
#endif

private:
    void addViewForNode (yup::AudioGraphNodeID nodeID)
    {
        auto props = graph->getNodeProperties (nodeID);
        if (! props.has_value())
            return;

        auto* proc = graph->getNodeProcessor (nodeID);
        auto view = nodeRegistry.createView (nodeID, props->identifier, proc, graph.get());

        if (view != nullptr)
        {
            graphComponent->addNodeView (nodeID, std::move (view), { props->positionX, props->positionY });
            loadedNodes[nodeID] = props->identifier;
        }
    }

    void removeNode (yup::AudioGraphNodeID nodeID)
    {
        if (onNodeWillBeRemoved != nullptr)
            onNodeWillBeRemoved (*this, nodeID);

        graphComponent->removeNodeView (nodeID);
        graph->removeNode (nodeID);
        graph->commitChanges();
        loadedNodes.erase (nodeID);
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
                    pluginSubMenu->addItem (plugins[static_cast<size_t> (i)].name, 100 + i);

                activeMenu->addSubMenu ("Plugins", pluginSubMenu);
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
    NodeRegistry& nodeRegistry;
    yup::String endpointSubtitle;
    std::unique_ptr<yup::AudioGraphComponent> graphComponent;
    std::map<yup::AudioGraphNodeID, yup::String> loadedNodes;
    yup::PopupMenu::Ptr activeMenu;
    yup::Point<float> pendingMenuCanvasPos;
};
