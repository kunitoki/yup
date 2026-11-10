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

#include "AudioGraphApp.h"

//==============================================================================

AudioGraphApp::AudioGraphApp()
{
    const auto settings = loadSettings();
    const auto* deviceState = settings != nullptr ? settings->getChildByName ("DEVICESETUP") : nullptr;

    deviceManager.initialise (2, 2, deviceState, true);

    if (auto defaultMidiIn = yup::MidiInput::getDefaultDevice();
        deviceState == nullptr && defaultMidiIn != yup::MidiDeviceInfo())
    {
        deviceManager.setMidiInputDeviceEnabled (defaultMidiIn.identifier, true);
    }

    deviceManager.addMidiInputDeviceCallback ({}, &midiCollector);
    deviceManager.addChangeListener (this);

    model = std::make_shared<yup::AudioGraphModel>();
    graph = std::make_shared<yup::AudioGraphProcessor> (model);
    nodeRegistry.registerInternalNodes();

#if YUP_DESKTOP
    scanner = std::make_unique<yup::AudioPluginScanner>();

    {
        const auto databaseFile = yup::File::getSpecialLocation (yup::File::userApplicationDataDirectory)
                                      .getChildFile ("YUP/AudioGraph/plugins.db");
        databaseFile.getParentDirectory().createDirectory();

        auto pluginDatabase = std::make_shared<yup::AudioPluginDatabase>();
        if (pluginDatabase->open (databaseFile).wasOk())
            scanner->setDatabase (std::move (pluginDatabase));
    }

    setOutOfProcessScanning (settings == nullptr || settings->getBoolAttribute ("outOfProcessScanning", true));

#if YUP_AUDIO_PLUGIN_HOST_ENABLE_VST3
    scanner->addFormat (std::make_unique<yup::VST3Format>());
#endif
#if YUP_AUDIO_PLUGIN_HOST_ENABLE_CLAP
    scanner->addFormat (std::make_unique<yup::CLAPFormat>());
#endif
#if YUP_AUDIO_PLUGIN_HOST_ENABLE_AU && YUP_MAC
    scanner->addFormat (std::make_unique<yup::AUFormat>());
#endif
    nodeRegistry.registerPluginFormats ([this] (const yup::AudioPluginDescription& desc)
    {
        return loadPlugin (desc);
    });

    if (auto database = scanner->getDatabase())
        nodeRegistry.setDiscoveredPlugins (database->getPlugins());
#endif

    model->setNodeFactory (nodeRegistry.makeProcessorFactory());

    editorPanel = createMainPanel();

    setupToolbar();
    setupWorkspace();
    resetToEmptyGraph();

#if YUP_DESKTOP
    startPluginScan();
#endif
}

AudioGraphApp::~AudioGraphApp()
{
#if YUP_DESKTOP
    scanLifetime->store (false);
#endif

    deviceManager.removeChangeListener (this);
    deviceManager.removeMidiInputDeviceCallback ({}, &midiCollector);

    closePluginEditor();
    closeAllSubgraphEditors();
    closeSettings();

    if (audioCallbackRegistered)
    {
        deviceManager.removeAudioCallback (this);
        audioCallbackRegistered = false;
    }

    deviceManager.closeAudioDevice();
}

//==============================================================================

void AudioGraphApp::resized()
{
    auto bounds = getLocalBounds();
    auto toolbar = bounds.removeFromTop (36);

    newButton.setBounds (toolbar.removeFromLeft (70).reduced (4, 4));
    openButton.setBounds (toolbar.removeFromLeft (70).reduced (4, 4));
    saveButton.setBounds (toolbar.removeFromLeft (70).reduced (4, 4));
    settingsButton.setBounds (toolbar.removeFromRight (90).reduced (4, 4));
    toolbar.removeFromLeft (8);
    statusLabel.setBounds (toolbar.reduced (4, 4));

    browser->setBounds (bounds.removeFromLeft (260));
    workspace.setBounds (bounds);
}

void AudioGraphApp::paint (yup::Graphics& g)
{
    g.setFillColor (yup::Color (0xff0d1117));
    g.fillAll();

    g.setFillColor (yup::Color (0xff161b22));
    g.fillRect (getLocalBounds().removeFromTop (36).to<float>());
}

void AudioGraphApp::visibilityChanged()
{
    if (isVisible())
    {
        if (editorPanel)
            editorPanel->reloadViews();

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

void AudioGraphApp::audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                                      int numInputChannels,
                                                      float* const* outputChannelData,
                                                      int numOutputChannels,
                                                      int numSamples,
                                                      const yup::AudioIODeviceCallbackContext&)
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

    midiCollector.removeNextBlockOfMessages (midiBuffer, numSamples);

    yup::ParameterChangeBuffer emptyParams;
    yup::AudioProcessContext<float> ctx { outputBuffer, midiBuffer, emptyParams };
    graph->processBlock (ctx);
}

void AudioGraphApp::audioDeviceAboutToStart (yup::AudioIODevice* device)
{
    if (graph == nullptr || device == nullptr)
        return;

    const auto sampleRate = device->getCurrentSampleRate();
    midiCollector.reset (sampleRate);

    midiBuffer.ensureSize (4096);

    const auto spec = yup::AudioSpec (
        static_cast<float> (device->getCurrentSampleRate()),
        device->getCurrentBufferSizeSamples());
    graph->prepareToPlay (spec);
}

void AudioGraphApp::audioDeviceStopped()
{
    if (graph != nullptr)
        graph->releaseResources();
}

//==============================================================================

void AudioGraphApp::setupToolbar()
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

    settingsButton.setButtonText ("Settings");
    settingsButton.onClick = [this]
    {
        showSettings();
    };
    addAndMakeVisible (settingsButton);

    statusLabel.setText ("Ready.", yup::dontSendNotification);
    addAndMakeVisible (statusLabel);
}

void AudioGraphApp::setupWorkspace()
{
    browser = std::make_unique<NodeBrowser> (nodeRegistry);
    browser->onItemChosen = [this] (const yup::String& key)
    {
        auto& panel = getActiveEditorPanel();
        panel.addBrowserItem (key, panel.getVisibleCanvasCenter());
    };

#if YUP_DESKTOP
    browser->isPluginSandboxed = [this] (const yup::AudioPluginDescription& desc)
    {
        auto database = scanner->getDatabase();
        return database != nullptr && database->shouldRunSandboxed (desc);
    };

    browser->setPluginSandboxed = [this] (const yup::AudioPluginDescription& desc, bool shouldRunSandboxed)
    {
        if (auto database = scanner->getDatabase())
            database->setRunSandboxed (desc, shouldRunSandboxed);

        browser->refresh();
        statusLabel.setText (desc.name + (shouldRunSandboxed ? " will run sandboxed" : " will run in process")
                                 + " when added next time.",
                             yup::dontSendNotification);
    };

    browser->refresh();
#endif

    addAndMakeVisible (*browser);

    workspace.addTab ("graph", "Graph", *editorPanel);
    workspace.getTabBar().onTabCloseRequested = [this] (const yup::Identifier& tabId)
    {
        if (tabId == yup::Identifier ("settings"))
            closeSettings();
        else
            closeSubgraphTab (tabId);
    };
    addAndMakeVisible (workspace);
}

void AudioGraphApp::showSettings()
{
    if (settingsPanel == nullptr)
    {
        auto panel = std::make_unique<SettingsPanel> (deviceManager);

#if YUP_DESKTOP
        auto pluginPage = std::make_unique<PluginSettingsPage> (scanner->getDatabase());
        pluginPage->onRescanRequested = [this]
        {
            startPluginScan();
        };
        pluginPage->isOutOfProcessScanning = [this]
        {
            return scanner->getOutOfProcessScanning().has_value();
        };
        pluginPage->onOutOfProcessScanningChanged = [this] (bool shouldScanOutOfProcess)
        {
            setOutOfProcessScanning (shouldScanOutOfProcess);
            saveSettings();
        };
        pluginPage->onPluginsChanged = [this]
        {
            refreshPluginLists();
        };

        pluginSettingsPage = pluginPage.get();
        panel->addSection ("plugins", "Plugins", std::move (pluginPage));
#endif

        settingsPanel = panel.get();
        workspace.addTab ("settings", "Settings", std::move (panel)).setClosable (true);
    }

    workspace.getTabBar().setSelectedTab ("settings");
}

void AudioGraphApp::closeSettings()
{
    if (settingsPanel == nullptr)
        return;

#if YUP_DESKTOP
    pluginSettingsPage = nullptr;
#endif
    settingsPanel = nullptr;
    workspace.removeTab ("settings");
}

//==============================================================================

yup::File AudioGraphApp::getSettingsFile() const
{
    return yup::File::getSpecialLocation (yup::File::userApplicationDataDirectory)
        .getChildFile ("YUP/AudioGraph/settings.xml");
}

std::unique_ptr<yup::XmlElement> AudioGraphApp::loadSettings() const
{
    const auto file = getSettingsFile();
    return file.existsAsFile() ? yup::parseXMLIfTagMatches (file, "AudioGraphSettings") : nullptr;
}

void AudioGraphApp::saveSettings()
{
    yup::XmlElement settings ("AudioGraphSettings");

#if YUP_DESKTOP
    settings.setAttribute ("outOfProcessScanning", scanner->getOutOfProcessScanning().has_value());
#endif

    if (auto deviceState = deviceManager.createStateXml())
        settings.addChildElement (deviceState.release());

    const auto file = getSettingsFile();
    file.getParentDirectory().createDirectory();
    settings.writeTo (file);
}

void AudioGraphApp::changeListenerCallback (yup::ChangeBroadcaster*)
{
    saveSettings();
}

//==============================================================================

void AudioGraphApp::resetToEmptyGraph()
{
    closePluginEditor();
    closeAllSubgraphEditors();

    editorPanel->clearNodeViews();
    model->clear();
    model->setNodePosition (yup::AudioGraphModel::getGraphInputNodeID(), 40.0f, 200.0f);
    model->setNodePosition (yup::AudioGraphModel::getGraphOutputNodeID(), 760.0f, 200.0f);
    graph->commitChanges();
    editorPanel->clearUndoHistory();
    currentFilePath = yup::File();

    editorPanel->reloadViews();
}

void AudioGraphApp::openGraph()
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

void AudioGraphApp::saveGraph()
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

void AudioGraphApp::saveGraphToFile (const yup::File& file)
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

void AudioGraphApp::loadGraphFromMemory (const yup::MemoryBlock& mb, const yup::File& file)
{
    closePluginEditor();
    closeAllSubgraphEditors();

    editorPanel->clearNodeViews();

    model->clear();

    const auto result = graph->loadStateFromMemory (mb);
    if (result.failed())
    {
        statusLabel.setText ("Load failed: " + result.getErrorMessage(), yup::dontSendNotification);
        graph->commitChanges();
        return;
    }

    graph->commitChanges();
    editorPanel->clearUndoHistory();
    currentFilePath = file;

    editorPanel->reloadViews();

    statusLabel.setText ("Loaded: " + file.getFileName(), yup::dontSendNotification);
}

//==============================================================================

void AudioGraphApp::openNodeEditor (std::shared_ptr<yup::AudioGraphProcessor> ownerGraph, yup::AudioGraphNodeID nodeID)
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

void AudioGraphApp::openPluginEditor (std::shared_ptr<yup::AudioGraphProcessor> ownerGraph, yup::AudioGraphNodeID nodeID)
{
    if (pluginEditorWindow != nullptr && activePluginEditorGraph == ownerGraph.get() && activePluginEditorNodeID == nodeID)
    {
        pluginEditorWindow->toFront (true);
        return;
    }

    auto ownerModel = ownerGraph->getModel();
    auto* proc = ownerModel->getNodeProcessor (nodeID);

#if YUP_DESKTOP
    if (auto* remote = dynamic_cast<yup::RemoteAudioPluginInstance*> (proc))
    {
        if (! remote->hasCrashed())
        {
            remote->showEditor();
            return;
        }

        const auto result = remote->reload();
        statusLabel.setText (result.wasOk() ? "Reloaded: " + proc->getName() : "Reload failed: " + result.getErrorMessage(),
                             yup::dontSendNotification);
        return;
    }
#endif

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

void AudioGraphApp::closePluginEditor()
{
    pluginEditorWindow.reset();
    activePluginEditorGraph = nullptr;
    activePluginEditorNodeID = yup::AudioGraphNodeID::invalid();
}

//==============================================================================

std::unique_ptr<AudioGraphEditorPanel> AudioGraphApp::createMainPanel()
{
    AudioGraphEditorPanel::EndpointViews endpointViews;
    endpointViews.createInputView = [this]
    {
        return std::make_unique<SoundCardInputNodeView> (graph, "sound card");
    };
    endpointViews.createOutputView = [this]
    {
        return std::make_unique<SoundCardOutputNodeView> (graph, "sound card");
    };

    auto panel = std::make_unique<AudioGraphEditorPanel> (graph, nodeRegistry, std::move (endpointViews));
    configurePanelCallbacks (*panel);
    return panel;
}

std::unique_ptr<AudioGraphEditorPanel> AudioGraphApp::createSubgraphPanel (std::shared_ptr<yup::AudioGraphProcessor> childGraph)
{
    auto panel = std::make_unique<AudioGraphEditorPanel> (std::move (childGraph), nodeRegistry, "parent graph");
    configurePanelCallbacks (*panel);
    return panel;
}

AudioGraphEditorPanel& AudioGraphApp::getActiveEditorPanel()
{
    const auto selectedTab = workspace.getTabBar().getSelectedTabId();

    for (auto& record : subgraphEditors)
    {
        if (record.tabId == selectedTab && record.page != nullptr)
            return *record.page->getEditorPanel();
    }

    return *editorPanel;
}

void AudioGraphApp::configurePanelCallbacks (AudioGraphEditorPanel& panel)
{
    panel.onStatusMessage = [this] (const yup::String& message)
    {
        statusLabel.setText (message, yup::dontSendNotification);
    };

    panel.onNodeDoubleClicked = [this] (AudioGraphEditorPanel& editor, yup::AudioGraphNodeID nodeID)
    {
        openNodeEditor (editor.getGraph(), nodeID);
    };

    panel.onNodeWillBeRemoved = [this] (AudioGraphEditorPanel& editor, yup::AudioGraphNodeID nodeID)
    {
        if (activePluginEditorGraph == editor.getGraph().get() && activePluginEditorNodeID == nodeID)
            closePluginEditor();

        closeSubgraphEditorForNode (editor.getGraph(), nodeID);
    };

#if YUP_DESKTOP
    panel.onPluginRequested = [this] (AudioGraphEditorPanel& editor,
                                      const yup::AudioPluginDescription& desc,
                                      yup::Point<float> canvasPos)
    {
        addPluginNodeToPanel (editor, desc, canvasPos);
    };
#endif
}

void AudioGraphApp::openSubgraphEditor (std::shared_ptr<yup::AudioGraphProcessor> ownerGraph,
                                        yup::AudioGraphNodeID nodeID,
                                        SubgraphProcessor& processor)
{
    if (auto* record = findSubgraphEditorRecord (ownerGraph.get(), nodeID))
    {
        workspace.getTabBar().setSelectedTab (record->tabId);
        return;
    }

    yup::WeakReference<AudioGraphApp> weakThis (this);
    auto page = std::make_unique<SubgraphEditorPage> (
        createSubgraphPanel (processor.getGraph()),
        processor.getConfig(),
        [weakThis, ownerGraph, nodeID] (int presetID)
    {
        if (auto* self = weakThis.get())
            self->reconfigureSubgraph (ownerGraph, nodeID, SubgraphConfig::fromPreset (presetID));
    });

    const auto tabId = yup::Identifier ("subgraph" + yup::String (++nextSubgraphTabNumber));
    auto* pagePtr = page.get();

    workspace.addTab (tabId, processor.getConfig().getDisplayName(), std::move (page)).setClosable (true);
    subgraphEditors.push_back ({ ownerGraph.get(), nodeID, ownerGraph, tabId, pagePtr });
    workspace.getTabBar().setSelectedTab (tabId);
}

void AudioGraphApp::reconfigureSubgraph (std::shared_ptr<yup::AudioGraphProcessor> ownerGraph,
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
        if (auto* tab = workspace.getTabBar().getTabButton (record->tabId))
            tab->setText (config.getDisplayName());

        record->page->setEditorPanel (createSubgraphPanel (newProcessor->getGraph()), config);
    }
}

void AudioGraphApp::refreshNodeViewsForGraph (const yup::AudioGraphProcessor* graphToRefresh, yup::AudioGraphNodeID nodeID)
{
    if (graphToRefresh == graph.get())
        editorPanel->refreshNodeView (nodeID);

    for (auto& record : subgraphEditors)
    {
        if (record.page == nullptr)
            continue;

        auto* panel = record.page->getEditorPanel();
        if (panel != nullptr && panel->getGraph().get() == graphToRefresh)
            panel->refreshNodeView (nodeID);
    }
}

void AudioGraphApp::closeSubgraphEditorForNode (std::shared_ptr<yup::AudioGraphProcessor> ownerGraph, yup::AudioGraphNodeID nodeID)
{
    if (ownerGraph == nullptr)
        return;

    auto ownerModel = ownerGraph->getModel();
    if (auto* subgraph = dynamic_cast<SubgraphProcessor*> (ownerModel->getNodeProcessor (nodeID)))
        closeSubgraphEditorsForGraph (subgraph->getGraph().get());

    removeSubgraphEditors ([ownerGraphRaw = ownerGraph.get(), nodeID] (const SubgraphEditorRecord& record)
    {
        return record.ownerGraphRaw == ownerGraphRaw && record.nodeID == nodeID;
    });
}

void AudioGraphApp::closeSubgraphEditorsForGraph (const yup::AudioGraphProcessor* graphToClose)
{
    bool removed = true;

    while (removed)
    {
        removed = false;

        for (auto& record : subgraphEditors)
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

void AudioGraphApp::closeAllSubgraphEditors()
{
    removeSubgraphEditors ([] (const SubgraphEditorRecord&)
    {
        return true;
    });
}

void AudioGraphApp::closeSubgraphTab (const yup::Identifier& tabId)
{
    for (const auto& record : subgraphEditors)
    {
        if (record.tabId == tabId)
        {
            closeSubgraphEditorForNode (record.ownerGraph, record.nodeID);
            return;
        }
    }
}

void AudioGraphApp::removeSubgraphEditors (std::function<bool (const SubgraphEditorRecord&)> shouldRemove)
{
    std::vector<yup::Identifier> tabsToRemove;

    std::erase_if (subgraphEditors, [&] (const SubgraphEditorRecord& record)
    {
        if (! shouldRemove (record))
            return false;

        tabsToRemove.push_back (record.tabId);
        return true;
    });

    for (const auto& tabId : tabsToRemove)
        workspace.removeTab (tabId);
}

AudioGraphApp::SubgraphEditorRecord* AudioGraphApp::findSubgraphEditorRecord (const yup::AudioGraphProcessor* ownerGraph, yup::AudioGraphNodeID nodeID)
{
    const auto iterator = std::find_if (subgraphEditors.begin(), subgraphEditors.end(), [ownerGraph, nodeID] (const SubgraphEditorRecord& record)
    {
        return record.ownerGraphRaw == ownerGraph && record.nodeID == nodeID;
    });

    return iterator != subgraphEditors.end() ? &*iterator : nullptr;
}

#if YUP_DESKTOP
void AudioGraphApp::addPluginNodeToPanel (AudioGraphEditorPanel& editor,
                                          const yup::AudioPluginDescription& desc,
                                          yup::Point<float> canvasPos)
{
    auto result = loadPlugin (desc);
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

    if (editor.addProcessorNode (std::move (result).getValue(), std::move (props), canvasPos))
        statusLabel.setText ("Loaded: " + desc.name, yup::dontSendNotification);
}

yup::ResultValue<std::unique_ptr<yup::AudioPluginInstance>> AudioGraphApp::loadPlugin (const yup::AudioPluginDescription& desc)
{
    if (auto database = scanner->getDatabase(); database != nullptr && database->shouldRunSandboxed (desc))
    {
        yup::RemoteAudioPluginInstance::Options options;
        options.executable = yup::File::getSpecialLocation (yup::File::currentExecutableFile);

        auto result = yup::RemoteAudioPluginInstance::load (desc, makeHostContext(), options);
        if (result.failed())
            return yup::makeResultValueFail ("Sandboxed load failed: " + result.getErrorMessage());

        auto instance = std::move (result.getReference());

        yup::WeakReference<AudioGraphApp> weakThis (this);
        instance->onCrashed = [weakThis, name = desc.name]
        {
            if (auto* self = weakThis.get())
                self->statusLabel.setText (name + " crashed. Double-click the node to reload it.", yup::dontSendNotification);
        };

        return yup::makeResultValueOk (std::unique_ptr<yup::AudioPluginInstance> (std::move (instance)));
    }

    auto* format = scanner->getFormatForType (desc.formatType);
    if (format == nullptr)
        return yup::makeResultValueFail ("No format backend for: " + desc.name);

    return format->loadPlugin (desc, makeHostContext());
}
#endif

//==============================================================================

#if YUP_DESKTOP
void AudioGraphApp::startPluginScan()
{
    if (scanInProgress.exchange (true))
        return;

    statusLabel.setText ("Scanning for plugins...", yup::dontSendNotification);

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
            browser->refresh();

            if (pluginSettingsPage != nullptr)
                pluginSettingsPage->refresh();

            const auto count = static_cast<int> (nodeRegistry.getDiscoveredPlugins().size());
            statusLabel.setText ("Found " + yup::String (count) + " plugins.", yup::dontSendNotification);
            scanInProgress = false;
        });
    });
}

void AudioGraphApp::setOutOfProcessScanning (bool shouldScanOutOfProcess)
{
    if (! shouldScanOutOfProcess)
    {
        scanner->setOutOfProcessScanning (std::nullopt);
        return;
    }

    yup::AudioPluginScanner::OutOfProcessOptions scanOptions;
    scanOptions.executable = yup::File::getSpecialLocation (yup::File::currentExecutableFile);
    scanner->setOutOfProcessScanning (scanOptions);
}

void AudioGraphApp::refreshPluginLists()
{
    if (auto database = scanner->getDatabase())
        nodeRegistry.setDiscoveredPlugins (database->getPlugins());

    browser->refresh();
}

yup::AudioPluginHostContext AudioGraphApp::makeHostContext() const
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
#endif
