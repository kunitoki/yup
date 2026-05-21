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

#include <yup_core/yup_core.h>
#include <yup_audio_basics/yup_audio_basics.h>
#include <yup_audio_devices/yup_audio_devices.h>
#include <yup_audio_formats/yup_audio_formats.h>
#include <yup_audio_processors/yup_audio_processors.h>
#include <yup_audio_graph/yup_audio_graph.h>
#include <yup_events/yup_events.h>
#include <yup_graphics/yup_graphics.h>
#include <yup_gui/yup_gui.h>
#include <yup_audio_gui/yup_audio_gui.h>

#if YUP_DESKTOP
#include <yup_audio_plugin_host/yup_audio_plugin_host.h>
#endif

#include <algorithm>
#include <atomic>
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
{
public:
    AudioGraphApp();
    ~AudioGraphApp() override;

    // Component
    void resized() override;
    void paint (yup::Graphics& g) override;
    void visibilityChanged() override;

    // AudioIODeviceCallback
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const yup::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart (yup::AudioIODevice* device) override;
    void audioDeviceStopped() override;

private:
    //==============================================================================
    void setupToolbar();

    //==============================================================================
    void resetToEmptyGraph();

    void openGraph();
    void saveGraph();
    void saveGraphToFile (const yup::File& file);

    void loadGraphFromMemory (const yup::MemoryBlock& mb, const yup::File& file);

    //==============================================================================
    void openNodeEditor (std::shared_ptr<yup::AudioGraphProcessor> ownerGraph, yup::AudioGraphNodeID nodeID);
    void openPluginEditor (std::shared_ptr<yup::AudioGraphProcessor> ownerGraph, yup::AudioGraphNodeID nodeID);
    void closePluginEditor();

    //==============================================================================
    struct SubgraphEditorRecord
    {
        const yup::AudioGraphProcessor* ownerGraphRaw = nullptr;
        yup::AudioGraphNodeID nodeID;
        std::shared_ptr<yup::AudioGraphProcessor> ownerGraph;
        std::unique_ptr<SubgraphEditorWindow> window;
    };

    std::unique_ptr<AudioGraphEditorPanel> createMainPanel();
    std::unique_ptr<AudioGraphEditorPanel> createSubgraphPanel (std::shared_ptr<yup::AudioGraphProcessor> childGraph);

    void configurePanelCallbacks (AudioGraphEditorPanel& panel);
    void openSubgraphEditor (std::shared_ptr<yup::AudioGraphProcessor> ownerGraph,
                             yup::AudioGraphNodeID nodeID,
                             SubgraphProcessor& processor);
    void reconfigureSubgraph (std::shared_ptr<yup::AudioGraphProcessor> ownerGraph,
                              yup::AudioGraphNodeID nodeID,
                              const SubgraphConfig& config);
    void refreshNodeViewsForGraph (const yup::AudioGraphProcessor* graphToRefresh, yup::AudioGraphNodeID nodeID);
    void closeSubgraphEditorForNode (std::shared_ptr<yup::AudioGraphProcessor> ownerGraph, yup::AudioGraphNodeID nodeID);
    void closeSubgraphEditorsForGraph (const yup::AudioGraphProcessor* graphToClose);
    void closeAllSubgraphEditors();
    SubgraphEditorRecord* findSubgraphEditorRecord (const yup::AudioGraphProcessor* ownerGraph, yup::AudioGraphNodeID nodeID);

    //==============================================================================

#if YUP_DESKTOP
    void startPluginScan();
    void addPluginNodeToPanel (AudioGraphEditorPanel& editor, const yup::AudioPluginDescription& desc, yup::Point<float> canvasPos);
    yup::AudioPluginHostContext makeHostContext() const;
#endif

    //==============================================================================

    yup::AudioDeviceManager deviceManager;
    std::shared_ptr<yup::AudioGraphProcessor> graph;
    std::shared_ptr<yup::AudioGraphModel> model;
    NodeRegistry nodeRegistry;
    std::unique_ptr<AudioGraphEditorPanel> editorPanel;

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

#if YUP_DESKTOP
    std::shared_ptr<std::atomic<bool>> scanLifetime { std::make_shared<std::atomic<bool>> (true) };
    std::atomic<bool> scanInProgress { false };
    std::unique_ptr<yup::AudioPluginScanner> scanner;
#endif

    YUP_DECLARE_WEAK_REFERENCEABLE (AudioGraphApp)
};
