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
#include <atomic>
#include <map>
#include <memory>
#include <vector>

#include "nodes/GraphEndpointNodeViews.h"
#include "nodes/NodeRegistry.h"
#include "nodes/SubgraphNode.h"
#include "ui/AudioGraphEditorPanel.h"
#include "ui/PluginEditorWindow.h"
#include "ui/SubgraphEditorWindow.h"

//==============================================================================

class AudioGraphApp final
    : public yup::Component
    , public yup::AudioIODeviceCallback
    , public yup::AudioGraphComponent::Listener
{
public:
    AudioGraphApp()
    {
        deviceManager.initialiseWithDefaultDevices (2, 2);

        model = std::make_shared<yup::AudioGraphModel>();
        graph = std::make_shared<yup::AudioGraphProcessor> (model);
        nodeRegistry.registerInternalNodes();

#if YUP_DESKTOP
        scanner = std::make_unique<yup::AudioPluginScanner>();

#if YUP_AUDIO_PLUGIN_HOST_ENABLE_VST3
        scanner->addFormat (std::make_unique<yup::VST3Format>());
#endif
#if YUP_AUDIO_PLUGIN_HOST_ENABLE_CLAP
        scanner->addFormat (std::make_unique<yup::CLAPFormat>());
#endif
#if YUP_AUDIO_PLUGIN_HOST_ENABLE_AU && YUP_MAC
        scanner->addFormat (std::make_unique<yup::AUv2Format>());
#endif

        nodeRegistry.registerPluginFormats (scanner.get(), makeHostContext());
#endif

        model->setNodeFactory (nodeRegistry.makeProcessorFactory());

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

        setupToolbar();
        resetToEmptyGraph();

#if YUP_DESKTOP
        startPluginScan();
#endif
    }

    ~AudioGraphApp() override
    {
#if YUP_DESKTOP
        scanLifetime->store (false);
#endif

        closePluginEditor();
        closeAllSubgraphEditors();

        if (audioCallbackRegistered)
        {
            deviceManager.removeAudioCallback (this);
            audioCallbackRegistered = false;
        }

        deviceManager.closeAudioDevice();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto toolbar = bounds.removeFromTop (36);

        newButton.setBounds (toolbar.removeFromLeft (70).reduced (4, 4));
        openButton.setBounds (toolbar.removeFromLeft (70).reduced (4, 4));
        saveButton.setBounds (toolbar.removeFromLeft (70).reduced (4, 4));
        toolbar.removeFromLeft (8);
        scanButton.setBounds (toolbar.removeFromLeft (100).reduced (4, 4));
        statusLabel.setBounds (toolbar.reduced (4, 4));

        graphComponent->setBounds (bounds);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::Color (0xff0d1117));
        g.fillAll();

        g.setFillColor (yup::Color (0xff161b22));
        g.fillRect (getLocalBounds().removeFromTop (36).to<float>());
    }

    void visibilityChanged() override
    {
        if (isVisible())
        {
            deviceManager.addAudioCallback (this);
            audioCallbackRegistered = true;
        }
        else if (audioCallbackRegistered)
        {
            deviceManager.removeAudioCallback (this);
            audioCallbackRegistered = false;
        }
    }

    //==============================================================================
    // AudioIODeviceCallback

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const yup::AudioIODeviceCallbackContext&) override
    {
        for (int ch = 0; ch < numOutputChannels; ++ch)
            yup::FloatVectorOperations::clear (outputChannelData[ch], numSamples);

        if (graph == nullptr || numOutputChannels <= 0 || numSamples <= 0)
            return;

        yup::AudioBuffer<float> outputBuffer (outputChannelData, numOutputChannels, numSamples);

        for (int ch = 0; ch < yup::jmin (numInputChannels, numOutputChannels); ++ch)
        {
            if (inputChannelData != nullptr && inputChannelData[ch] != nullptr)
                yup::FloatVectorOperations::copy (outputChannelData[ch], inputChannelData[ch], numSamples);
        }

        yup::MidiBuffer midi;
        graph->processBlock (outputBuffer, midi);
    }

    void audioDeviceAboutToStart (yup::AudioIODevice* device) override
    {
        if (graph == nullptr || device == nullptr)
            return;

#if YUP_DESKTOP
        yup::AudioPluginHostContext ctx;
        ctx.sampleRate = static_cast<float> (device->getCurrentSampleRate());
        ctx.maxBlockSize = device->getCurrentBufferSizeSamples();
        nodeRegistry.setPluginScanner (scanner.get(), ctx);
#endif

        graph->prepareToPlay (static_cast<float> (device->getCurrentSampleRate()),
                              device->getCurrentBufferSizeSamples());
    }

    void audioDeviceStopped() override
    {
        if (graph != nullptr)
            graph->releaseResources();
    }

    //==============================================================================
    // AudioGraphComponent::Listener
    void nodeContextMenu (yup::AudioGraphNodeID nodeID, yup::Point<float>) override
    {
        removeNode (nodeID);
    }

    void nodeDoubleClicked (yup::AudioGraphNodeID nodeID) override
    {
        openNodeEditor (graph, nodeID);
    }

    void canvasContextMenu (yup::Point<float> canvasPos) override
    {
        showAddNodeMenu (canvasPos);
    }

private:
    //==============================================================================

    void setupToolbar()
    {
        newButton.setButtonText ("New");
        newButton.onClick = [this]
        {
            resetToEmptyGraph();
            statusLabel.setText ("New graph.", yup::dontSendNotification);
        };
        addAndMakeVisible (newButton);

        openButton.setButtonText ("Open");
        openButton.onClick = [this]
        {
            openGraph();
        };
        addAndMakeVisible (openButton);

        saveButton.setButtonText ("Save");
        saveButton.onClick = [this]
        {
            saveGraph();
        };
        addAndMakeVisible (saveButton);

        scanButton.setButtonText ("Scan Plugins");
        scanButton.onClick = [this]
        {
            startPluginScan();
        };
        addAndMakeVisible (scanButton);

        statusLabel.setText ("Ready.", yup::dontSendNotification);
        addAndMakeVisible (statusLabel);
    }

    void clearNodeViews()
    {
        for (auto& [nodeID, _] : loadedNodes)
            graphComponent->removeNodeView (nodeID);

        loadedNodes.clear();
    }

    void reloadGraphViewsFromModel()
    {
        clearNodeViews();

        for (auto nodeID : model->getNodeIDs())
        {
            auto props = model->getNodeProperties (nodeID);
            if (! props.has_value())
                continue;

            auto* proc = model->getNodeProcessor (nodeID);
            auto view = nodeRegistry.createView (nodeID, props->identifier, proc, graph.get());

            if (view != nullptr)
            {
                graphComponent->addNodeView (nodeID, std::move (view), { props->positionX, props->positionY });
                loadedNodes[nodeID] = props->identifier;
            }
        }
    }

    void reloadGraphBoundaryViewsFromModel()
    {
        const auto input = model->getNodeProperties (yup::AudioGraphModel::getGraphInputNodeID())
                               .value_or (yup::AudioGraphNodeProperties {});
        graphComponent->setGraphInputView (std::make_unique<SoundCardInputNodeView>(), { input.positionX, input.positionY });

        const auto output = model->getNodeProperties (yup::AudioGraphModel::getGraphOutputNodeID())
                                .value_or (yup::AudioGraphNodeProperties {});
        graphComponent->setGraphOutputView (std::make_unique<SoundCardOutputNodeView>(), { output.positionX, output.positionY });
    }

    //==============================================================================

    void resetToEmptyGraph()
    {
        closePluginEditor();
        closeAllSubgraphEditors();

        clearNodeViews();
        model->clear();
        model->setNodePosition (yup::AudioGraphModel::getGraphInputNodeID(), 40.0f, 200.0f);
        model->setNodePosition (yup::AudioGraphModel::getGraphOutputNodeID(), 760.0f, 200.0f);
        graph->commitChanges();
        undoManager.clear();
        currentFilePath = yup::File();

        reloadGraphBoundaryViewsFromModel();
        graphComponent->zoomToFitNodes();
    }

    //==============================================================================

    void openGraph()
    {
        fileChooser = yup::FileChooser::create (
            "Open Audio Graph",
            currentFilePath.existsAsFile() ? currentFilePath.getParentDirectory()
                                           : yup::File::getSpecialLocation (yup::File::userDocumentsDirectory),
            "*.yug");

        fileChooser->browseForFileToOpen ([this] (bool success, const yup::Array<yup::File>& results)
        {
            if (! success || results.isEmpty())
                return;

            const auto& file = results[0];

            yup::MemoryBlock mb;
            if (! file.loadFileAsData (mb))
            {
                statusLabel.setText ("Failed to read file.", yup::dontSendNotification);
                return;
            }

            loadGraphFromMemory (mb, file);
        });
    }

    void saveGraph()
    {
        if (currentFilePath.existsAsFile())
        {
            saveGraphToFile (currentFilePath);
            return;
        }

        fileChooser = yup::FileChooser::create (
            "Save Audio Graph",
            yup::File::getSpecialLocation (yup::File::userDocumentsDirectory).getChildFile ("Untitled.yug"),
            "*.yug");

        fileChooser->browseForFileToSave ([this] (bool success, const yup::Array<yup::File>& results)
        {
            if (! success || results.isEmpty())
                return;

            auto file = results[0];
            if (file.getFileExtension().isEmpty())
                file = file.withFileExtension ("yug");

            saveGraphToFile (file);
        },
                                          true);
    }

    void saveGraphToFile (const yup::File& file)
    {
        yup::MemoryBlock mb;
        const auto result = graph->saveStateIntoMemory (mb);

        if (result.failed())
        {
            statusLabel.setText ("Save failed: " + result.getErrorMessage(), yup::dontSendNotification);
            return;
        }

        if (! file.replaceWithData (mb.getData(), mb.getSize()))
        {
            statusLabel.setText ("Could not write: " + file.getFileName(), yup::dontSendNotification);
            return;
        }

        currentFilePath = file;
        statusLabel.setText ("Saved: " + file.getFileName(), yup::dontSendNotification);
    }

    void loadGraphFromMemory (const yup::MemoryBlock& mb, const yup::File& file)
    {
        closePluginEditor();
        closeAllSubgraphEditors();

        clearNodeViews();

        model->clear();

        const auto result = graph->loadStateFromMemory (mb);
        if (result.failed())
        {
            statusLabel.setText ("Load failed: " + result.getErrorMessage(), yup::dontSendNotification);
            graph->commitChanges();
            return;
        }

        model->setNodePosition (yup::AudioGraphModel::getGraphInputNodeID(), 40.0f, 200.0f);
        model->setNodePosition (yup::AudioGraphModel::getGraphOutputNodeID(), 760.0f, 200.0f);
        graph->commitChanges();
        undoManager.clear();
        currentFilePath = file;

        reloadGraphViewsFromModel();
        reloadGraphBoundaryViewsFromModel();
        graphComponent->zoomToFitNodes();

        statusLabel.setText ("Loaded: " + file.getFileName(), yup::dontSendNotification);
    }

    //==============================================================================

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
        const auto& plugins = nodeRegistry.getDiscoveredPlugins();

        if (! plugins.empty())
        {
            auto pluginSubMenu = yup::PopupMenu::create();

            for (int i = 0; i < static_cast<int> (plugins.size()); ++i)
            {
                const auto& desc = plugins[static_cast<size_t> (i)];
                pluginSubMenu->addItem (desc.name + " [" + formatTypeToString (desc.formatType) + "]",
                                        100 + i);
            }

            activeMenu->addSubMenu ("Plugins", pluginSubMenu);
        }
        else
        {
            activeMenu->addItem ("No plugins (click Scan)", -1, false);
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
            else if (selectedID >= 100)
            {
                const auto& plugins = nodeRegistry.getDiscoveredPlugins();

                const auto index = static_cast<size_t> (selectedID - 100);
                if (index < plugins.size())
                    addPluginNode (plugins[index], pendingMenuCanvasPos);
            }
#endif
        });
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
            statusLabel.setText ("Failed: " + processorResult.getErrorMessage(), yup::dontSendNotification);
            return;
        }

        const auto nodeID = model->addNode (std::move (processorResult).getValue(), props);
        if (! nodeID.isValid())
        {
            statusLabel.setText ("Failed to add node to graph.", yup::dontSendNotification);
            return;
        }

        graph->commitChanges();

        auto* rawProc = model->getNodeProcessor (nodeID);
        auto view = nodeRegistry.createView (nodeID, identifier, rawProc, graph.get());
        if (view != nullptr)
        {
            graphComponent->addNodeView (nodeID, std::move (view), canvasPos);
            loadedNodes[nodeID] = identifier;
        }
    }

#if YUP_DESKTOP
    void addPluginNode (const yup::AudioPluginDescription& desc, yup::Point<float> canvasPos)
    {
        auto* format = scanner->getFormatForType (desc.formatType);
        if (format == nullptr)
        {
            statusLabel.setText ("No format backend for: " + desc.name, yup::dontSendNotification);
            return;
        }

        auto result = format->loadPlugin (desc, makeHostContext());
        if (result.failed())
        {
            statusLabel.setText ("Load failed: " + result.getErrorMessage(), yup::dontSendNotification);
            return;
        }

        yup::AudioGraphNodeProperties props;
        props.identifier = NodeRegistry::identifierForDescription (desc);
        props.name = desc.name;
        props.positionX = canvasPos.getX();
        props.positionY = canvasPos.getY();
        props.creationData = NodeRegistry::descriptionToCreationData (desc);

        const auto nodeID = model->addNode (std::move (result).getValue(), props);
        if (! nodeID.isValid())
        {
            statusLabel.setText ("Failed to add plugin to graph.", yup::dontSendNotification);
            return;
        }

        graph->commitChanges();

        auto* rawProc = model->getNodeProcessor (nodeID);
        auto view = nodeRegistry.createView (nodeID, props.identifier, rawProc, graph.get());
        if (view != nullptr)
        {
            graphComponent->addNodeView (nodeID, std::move (view), canvasPos);
            loadedNodes[nodeID] = props.identifier;
        }

        statusLabel.setText ("Loaded: " + desc.name, yup::dontSendNotification);
    }
#endif

    void removeNode (yup::AudioGraphNodeID nodeID)
    {
        if (model->getNodeProcessor (nodeID) == nullptr)
            return;

        if (activePluginEditorGraph == graph.get() && activePluginEditorNodeID == nodeID)
            closePluginEditor();

        closeSubgraphEditorForNode (graph, nodeID);

        graphComponent->removeNodeView (nodeID);
        model->removeNode (nodeID);
        graph->commitChanges();
        loadedNodes.erase (nodeID);
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

    //==============================================================================

    void openNodeEditor (std::shared_ptr<yup::AudioGraphProcessor> ownerGraph, yup::AudioGraphNodeID nodeID)
    {
        if (ownerGraph == nullptr)
            return;

        auto ownerModel = ownerGraph->getModel();
        auto* proc = ownerModel->getNodeProcessor (nodeID);
        if (auto* subgraph = dynamic_cast<SubgraphProcessor*> (proc))
        {
            openSubgraphEditor (ownerGraph, nodeID, *subgraph);
            return;
        }

        openPluginEditor (ownerGraph, nodeID);
    }

    void openPluginEditor (std::shared_ptr<yup::AudioGraphProcessor> ownerGraph, yup::AudioGraphNodeID nodeID)
    {
        if (pluginEditorWindow != nullptr && activePluginEditorGraph == ownerGraph.get() && activePluginEditorNodeID == nodeID)
        {
            pluginEditorWindow->toFront (true);
            return;
        }

        auto ownerModel = ownerGraph->getModel();
        auto* proc = ownerModel->getNodeProcessor (nodeID);
        if (proc == nullptr || ! proc->hasEditor())
            return;

        auto editor = std::unique_ptr<yup::AudioProcessorEditor> (proc->createEditor());
        if (editor == nullptr)
            return;

        closePluginEditor();
        activePluginEditorGraph = ownerGraph.get();
        activePluginEditorNodeID = nodeID;

        const auto preferredSize = editor->getPreferredSize();

        yup::WeakReference<AudioGraphApp> weakThis (this);

        pluginEditorWindow = std::make_unique<PluginEditorWindow> (
            proc->getName(),
            std::move (editor),
            [weakThis]()
        {
            if (auto* self = weakThis.get())
                self->closePluginEditor();
        });

        pluginEditorWindow->centreWithSize (preferredSize);
        pluginEditorWindow->setVisible (true);
        pluginEditorWindow->toFront (true);
    }

    void closePluginEditor()
    {
        pluginEditorWindow.reset();
        activePluginEditorGraph = nullptr;
        activePluginEditorNodeID = yup::AudioGraphNodeID::invalid();
    }

    struct SubgraphEditorRecord
    {
        const yup::AudioGraphProcessor* ownerGraphRaw = nullptr;
        yup::AudioGraphNodeID nodeID;
        std::shared_ptr<yup::AudioGraphProcessor> ownerGraph;
        std::unique_ptr<SubgraphEditorWindow> window;
    };

    std::unique_ptr<AudioGraphEditorPanel> createSubgraphPanel (std::shared_ptr<yup::AudioGraphProcessor> childGraph)
    {
        auto panel = std::make_unique<AudioGraphEditorPanel> (std::move (childGraph), nodeRegistry, "parent graph");

        panel->onNodeDoubleClicked = [this] (AudioGraphEditorPanel& editor, yup::AudioGraphNodeID nodeID)
        {
            openNodeEditor (editor.getGraph(), nodeID);
        };

        panel->onNodeWillBeRemoved = [this] (AudioGraphEditorPanel& editor, yup::AudioGraphNodeID nodeID)
        {
            if (activePluginEditorGraph == editor.getGraph().get() && activePluginEditorNodeID == nodeID)
                closePluginEditor();

            closeSubgraphEditorForNode (editor.getGraph(), nodeID);
        };

#if YUP_DESKTOP
        panel->getDiscoveredPlugins = [this]() -> const std::vector<yup::AudioPluginDescription>&
        {
            return nodeRegistry.getDiscoveredPlugins();
        };
        panel->onPluginSelected = [this] (AudioGraphEditorPanel& editor,
                                          const yup::AudioPluginDescription& desc,
                                          yup::Point<float> canvasPos)
        {
            addPluginNodeToPanel (editor, desc, canvasPos);
        };
#endif

        return panel;
    }

    void openSubgraphEditor (std::shared_ptr<yup::AudioGraphProcessor> ownerGraph,
                             yup::AudioGraphNodeID nodeID,
                             SubgraphProcessor& processor)
    {
        if (auto* record = findSubgraphEditorRecord (ownerGraph.get(), nodeID))
        {
            record->window->toFront (true);
            return;
        }

        auto childGraph = processor.getGraph();
        auto panel = createSubgraphPanel (childGraph);

        yup::WeakReference<AudioGraphApp> weakThis (this);
        auto window = std::make_unique<SubgraphEditorWindow> (
            processor.getConfig().getDisplayName(),
            std::move (panel),
            processor.getConfig(),
            [weakThis, ownerGraph, nodeID] (int presetID)
        {
            if (auto* self = weakThis.get())
                self->reconfigureSubgraph (ownerGraph, nodeID, SubgraphConfig::fromPreset (presetID));
        },
            [weakThis, ownerGraph, nodeID]
        {
            if (auto* self = weakThis.get())
                self->closeSubgraphEditorForNode (ownerGraph, nodeID);
        });

        subgraphEditorWindows.push_back ({ ownerGraph.get(), nodeID, ownerGraph, std::move (window) });
        auto& record = subgraphEditorWindows.back();
        record.window->centreWithSize ({ 900, 620 });
        record.window->setVisible (true);
        record.window->toFront (true);
    }

    void reconfigureSubgraph (std::shared_ptr<yup::AudioGraphProcessor> ownerGraph,
                              yup::AudioGraphNodeID nodeID,
                              const SubgraphConfig& config)
    {
        if (ownerGraph == nullptr)
            return;

        auto ownerModel = ownerGraph->getModel();
        auto* oldProcessor = dynamic_cast<SubgraphProcessor*> (ownerModel->getNodeProcessor (nodeID));
        if (oldProcessor == nullptr || oldProcessor->getConfig().getPresetID() == config.getPresetID())
            return;

        closePluginEditor();
        closeSubgraphEditorsForGraph (oldProcessor->getGraph().get());

        yup::MemoryBlock oldState;
        if (const auto saveResult = oldProcessor->saveStateIntoMemory (oldState); saveResult.failed())
        {
            statusLabel.setText ("Subgraph save failed: " + saveResult.getErrorMessage(), yup::dontSendNotification);
            return;
        }

        auto props = ownerModel->getNodeProperties (nodeID).value_or (yup::AudioGraphNodeProperties());
        props.identifier = NodeRegistry::subgraphIdentifier;
        props.name = config.getDisplayName();
        props.creationData = SubgraphConfig::toCreationData (config);

        auto replacement = std::make_unique<SubgraphProcessor> (config);
        auto replacementModel = replacement->getModel();
        replacementModel->setNodeFactory (nodeRegistry.makeProcessorFactory());

        if (const auto loadResult = replacement->loadStateFromMemory (oldState); loadResult.failed())
        {
            statusLabel.setText ("Subgraph reload failed: " + loadResult.getErrorMessage(), yup::dontSendNotification);
            return;
        }

        if (const auto replaceResult = ownerModel->replaceNode (nodeID, std::move (replacement), props); replaceResult.failed())
        {
            statusLabel.setText ("Subgraph reconfigure failed: " + replaceResult.getErrorMessage(), yup::dontSendNotification);
            return;
        }

        if (const auto commitResult = ownerGraph->commitChanges(); commitResult.failed())
        {
            statusLabel.setText ("Subgraph commit failed: " + commitResult.getErrorMessage(), yup::dontSendNotification);
            return;
        }

        refreshNodeViewsForGraph (ownerGraph.get(), nodeID);

        auto* newProcessor = dynamic_cast<SubgraphProcessor*> (ownerModel->getNodeProcessor (nodeID));
        auto* record = findSubgraphEditorRecord (ownerGraph.get(), nodeID);

        if (newProcessor != nullptr && record != nullptr)
        {
            record->window->setTitle (config.getDisplayName());
            record->window->setEditorPanel (createSubgraphPanel (newProcessor->getGraph()), config);
        }
    }

    void refreshNodeViewsForGraph (const yup::AudioGraphProcessor* graphToRefresh, yup::AudioGraphNodeID nodeID)
    {
        if (graphToRefresh == graph.get())
        {
            graphComponent->removeNodeView (nodeID);
            loadedNodes.erase (nodeID);

            if (auto props = model->getNodeProperties (nodeID))
            {
                auto* proc = model->getNodeProcessor (nodeID);
                auto view = nodeRegistry.createView (nodeID, props->identifier, proc, graph.get());

                if (view != nullptr)
                {
                    graphComponent->addNodeView (nodeID, std::move (view), { props->positionX, props->positionY });
                    loadedNodes[nodeID] = props->identifier;
                }
            }
        }

        for (auto& record : subgraphEditorWindows)
            if (record.ownerGraphRaw == graphToRefresh && record.window != nullptr && record.window->getEditorPanel() != nullptr)
                record.window->getEditorPanel()->refreshNodeView (nodeID);
    }

    void closeSubgraphEditorForNode (std::shared_ptr<yup::AudioGraphProcessor> ownerGraph, yup::AudioGraphNodeID nodeID)
    {
        if (ownerGraph == nullptr)
            return;

        auto ownerModel = ownerGraph->getModel();
        if (auto* subgraph = dynamic_cast<SubgraphProcessor*> (ownerModel->getNodeProcessor (nodeID)))
            closeSubgraphEditorsForGraph (subgraph->getGraph().get());

        subgraphEditorWindows.erase (std::remove_if (subgraphEditorWindows.begin(), subgraphEditorWindows.end(), [ownerGraphRaw = ownerGraph.get(), nodeID] (SubgraphEditorRecord& record)
        {
            return record.ownerGraphRaw == ownerGraphRaw && record.nodeID == nodeID;
        }),
                                     subgraphEditorWindows.end());
    }

    void closeSubgraphEditorsForGraph (const yup::AudioGraphProcessor* graphToClose)
    {
        bool removed = true;

        while (removed)
        {
            removed = false;

            for (auto& record : subgraphEditorWindows)
            {
                if (record.ownerGraphRaw == graphToClose)
                {
                    closeSubgraphEditorForNode (record.ownerGraph, record.nodeID);
                    removed = true;
                    break;
                }
            }
        }
    }

    void closeAllSubgraphEditors()
    {
        subgraphEditorWindows.clear();
    }

    SubgraphEditorRecord* findSubgraphEditorRecord (const yup::AudioGraphProcessor* ownerGraph, yup::AudioGraphNodeID nodeID)
    {
        const auto iterator = std::find_if (subgraphEditorWindows.begin(), subgraphEditorWindows.end(), [ownerGraph, nodeID] (const SubgraphEditorRecord& record)
        {
            return record.ownerGraphRaw == ownerGraph && record.nodeID == nodeID;
        });

        return iterator != subgraphEditorWindows.end() ? &*iterator : nullptr;
    }

#if YUP_DESKTOP
    void addPluginNodeToPanel (AudioGraphEditorPanel& editor,
                               const yup::AudioPluginDescription& desc,
                               yup::Point<float> canvasPos)
    {
        auto* format = scanner->getFormatForType (desc.formatType);
        if (format == nullptr)
            return;

        auto result = format->loadPlugin (desc, makeHostContext());
        if (result.failed())
        {
            statusLabel.setText ("Load failed: " + result.getErrorMessage(), yup::dontSendNotification);
            return;
        }

        yup::AudioGraphNodeProperties props;
        props.identifier = NodeRegistry::identifierForDescription (desc);
        props.name = desc.name;
        props.positionX = canvasPos.getX();
        props.positionY = canvasPos.getY();
        props.creationData = NodeRegistry::descriptionToCreationData (desc);

        editor.addProcessorNode (std::move (result).getValue(), std::move (props), canvasPos);
    }
#endif

    //==============================================================================

#if YUP_DESKTOP
    void startPluginScan()
    {
        if (scanInProgress.exchange (true))
            return;

        statusLabel.setText ("Scanning for plugins...", yup::dontSendNotification);
        scanButton.setEnabled (false);

        auto lifetime = scanLifetime;

        scanner->scanDefaultsAsync ([this, lifetime] (yup::AudioPluginScanner::ScanResult result) mutable
        {
            if (! lifetime->load())
                return;

            yup::MessageManager::callAsync ([this, lifetime, result = std::move (result)]() mutable
            {
                if (! lifetime->load())
                    return;

                nodeRegistry.setDiscoveredPlugins (std::move (result.discovered));

                const auto count = static_cast<int> (nodeRegistry.getDiscoveredPlugins().size());
                statusLabel.setText ("Found " + yup::String (count) + " plugins.", yup::dontSendNotification);
                scanButton.setEnabled (true);
                scanInProgress = false;
            });
        });
    }

    yup::AudioPluginHostContext makeHostContext() const
    {
        yup::AudioPluginHostContext ctx;
        ctx.sampleRate = 44100.0f;
        ctx.maxBlockSize = 512;

        if (auto* dev = deviceManager.getCurrentAudioDevice())
        {
            ctx.sampleRate = static_cast<float> (dev->getCurrentSampleRate());
            ctx.maxBlockSize = dev->getCurrentBufferSizeSamples();
        }

        return ctx;
    }

    static yup::String formatTypeToString (yup::AudioPluginFormatType type)
    {
        switch (type)
        {
            case yup::AudioPluginFormatType::vst3:
                return "VST3";
            case yup::AudioPluginFormatType::clap:
                return "CLAP";
            case yup::AudioPluginFormatType::audioUnit:
                return "AU";
            default:
                return "?";
        }
    }
#endif

    //==============================================================================

    yup::AudioDeviceManager deviceManager;
    std::shared_ptr<yup::AudioGraphProcessor> graph;
    std::shared_ptr<yup::AudioGraphModel> model;
    yup::UndoManager undoManager;
    std::unique_ptr<yup::AudioGraphComponent> graphComponent;
    NodeRegistry nodeRegistry;
    std::map<yup::AudioGraphNodeID, yup::String> loadedNodes;

    yup::File currentFilePath;
    yup::FileChooser::Ptr fileChooser;

    std::unique_ptr<PluginEditorWindow> pluginEditorWindow;
    const yup::AudioGraphProcessor* activePluginEditorGraph = nullptr;
    yup::AudioGraphNodeID activePluginEditorNodeID { yup::AudioGraphNodeID::invalid() };
    std::vector<SubgraphEditorRecord> subgraphEditorWindows;

    yup::TextButton newButton;
    yup::TextButton openButton;
    yup::TextButton saveButton;
    yup::TextButton scanButton;
    yup::Label statusLabel;

    bool audioCallbackRegistered = false;

    yup::PopupMenu::Ptr activeMenu;
    yup::Point<float> pendingMenuCanvasPos;

#if YUP_DESKTOP
    std::shared_ptr<std::atomic<bool>> scanLifetime { std::make_shared<std::atomic<bool>> (true) };
    std::atomic<bool> scanInProgress { false };
    std::unique_ptr<yup::AudioPluginScanner> scanner;
#endif

    YUP_DECLARE_WEAK_REFERENCEABLE (AudioGraphApp)
};
