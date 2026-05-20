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

#include <atomic>
#include <map>
#include <memory>
#include <vector>

//==============================================================================

class SoundCardInputNodeView final : public yup::AudioGraphNodeView
{
public:
    SoundCardInputNodeView()
        : AudioGraphNodeView (yup::AudioGraphNodeID::invalid())
    {
    }

    yup::String getNodeTitle() const override { return "INPUT"; }

    yup::String getNodeSubtitle() const override { return "sound card"; }

    int getNumInputPorts() const override { return 0; }

    int getNumOutputPorts() const override { return 1; }

    int getPreferredWidth() const override { return 150; }

    yup::Color getNodeColor() const override { return yup::Color (0xfff97316); }

    PortInfo getOutputPortInfo (int) const override
    {
        return { "audio", getPortKindColor (PortKind::audio), PortKind::audio };
    }
};

class SoundCardOutputNodeView final : public yup::AudioGraphNodeView
{
public:
    SoundCardOutputNodeView()
        : AudioGraphNodeView (yup::AudioGraphNodeID::invalid())
    {
    }

    yup::String getNodeTitle() const override { return "OUTPUT"; }

    yup::String getNodeSubtitle() const override { return "sound card"; }

    int getNumInputPorts() const override { return 1; }

    int getNumOutputPorts() const override { return 0; }

    int getPreferredWidth() const override { return 150; }

    yup::Color getNodeColor() const override { return yup::Color (0xff06b6d4); }

    PortInfo getInputPortInfo (int) const override
    {
        return { "audio", getPortKindColor (PortKind::audio), PortKind::audio };
    }
};

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

        graph = std::make_shared<yup::AudioGraphProcessor>();
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

        graph->setNodeFactory (nodeRegistry.makeProcessorFactory());

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
            openPluginEditor (id);
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

    void nodeViewMoved (yup::AudioGraphNodeID nodeID, yup::Point<float> newCanvasPos) override
    {
        graph->setNodePosition (nodeID, newCanvasPos.getX(), newCanvasPos.getY());
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

    //==============================================================================

    void resetToEmptyGraph()
    {
        closePluginEditor();

        for (auto& [nodeID, _] : loadedNodes)
            graphComponent->removeNodeView (nodeID);

        loadedNodes.clear();
        graph->clear();
        graph->commitChanges();
        currentFilePath = yup::File();

        graphComponent->setGraphInputView (std::make_unique<SoundCardInputNodeView>(), { 40.0f, 200.0f });
        graphComponent->setGraphOutputView (std::make_unique<SoundCardOutputNodeView>(), { 760.0f, 200.0f });
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

        for (auto& [nodeID, _] : loadedNodes)
            graphComponent->removeNodeView (nodeID);
        loadedNodes.clear();

        graph->clear();

        const auto result = graph->loadStateFromMemory (mb);
        if (result.failed())
        {
            statusLabel.setText ("Load failed: " + result.getErrorMessage(), yup::dontSendNotification);
            graph->commitChanges();
            return;
        }

        graph->commitChanges();
        currentFilePath = file;

        for (auto nodeID : graph->getNodeIDs())
        {
            auto props = graph->getNodeProperties (nodeID);
            if (! props.has_value())
                continue;

            auto* proc = graph->getNodeProcessor (nodeID);
            auto view = nodeRegistry.createView (nodeID, props->identifier, proc, graph.get());

            if (view != nullptr)
            {
                graphComponent->addNodeView (nodeID, std::move (view), { props->positionX, props->positionY });
                loadedNodes[nodeID] = props->identifier;
            }
        }

        graphComponent->setGraphInputView (std::make_unique<SoundCardInputNodeView>(), { 40.0f, 200.0f });
        graphComponent->setGraphOutputView (std::make_unique<SoundCardOutputNodeView>(), { 760.0f, 200.0f });
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

        activeMenu = yup::PopupMenu::create (options);
        activeMenu->addSubMenu ("Internal Nodes", internalSubMenu);

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

    void addInternalNode (const yup::String& identifier, yup::Point<float> canvasPos)
    {
        yup::AudioGraphNodeProperties props;
        props.identifier = identifier;
        props.name = NodeRegistry::identifierToDisplayName (identifier);
        props.positionX = canvasPos.getX();
        props.positionY = canvasPos.getY();

        auto processorResult = nodeRegistry.makeProcessorFactory() (props);

        if (processorResult.failed())
        {
            statusLabel.setText ("Failed: " + processorResult.getErrorMessage(), yup::dontSendNotification);
            return;
        }

        const auto nodeID = graph->addNode (std::move (processorResult).getValue(), props);
        if (! nodeID.isValid())
        {
            statusLabel.setText ("Failed to add node to graph.", yup::dontSendNotification);
            return;
        }

        graph->commitChanges();

        auto* rawProc = graph->getNodeProcessor (nodeID);
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

        const auto nodeID = graph->addNode (std::move (result).getValue(), props);
        if (! nodeID.isValid())
        {
            statusLabel.setText ("Failed to add plugin to graph.", yup::dontSendNotification);
            return;
        }

        graph->commitChanges();

        auto* rawProc = graph->getNodeProcessor (nodeID);
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
        if (activePluginEditorNodeID == nodeID)
        {
            closePluginEditor();
        }

        graphComponent->removeNodeView (nodeID);
        graph->removeNode (nodeID);
        graph->commitChanges();
        loadedNodes.erase (nodeID);
    }

    //==============================================================================

    void openPluginEditor (yup::AudioGraphNodeID nodeID)
    {
        if (pluginEditorWindow != nullptr && activePluginEditorNodeID == nodeID)
        {
            pluginEditorWindow->toFront (true);
            return;
        }

        auto* proc = graph->getNodeProcessor (nodeID);
        if (proc == nullptr || ! proc->hasEditor())
            return;

        auto editor = std::unique_ptr<yup::AudioProcessorEditor> (proc->createEditor());
        if (editor == nullptr)
            return;

        closePluginEditor();
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
        activePluginEditorNodeID = yup::AudioGraphNodeID::invalid();
    }

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
    std::unique_ptr<yup::AudioGraphComponent> graphComponent;
    NodeRegistry nodeRegistry;
    std::map<yup::AudioGraphNodeID, yup::String> loadedNodes;

    yup::File currentFilePath;
    yup::FileChooser::Ptr fileChooser;

    std::unique_ptr<PluginEditorWindow> pluginEditorWindow;
    yup::AudioGraphNodeID activePluginEditorNodeID { yup::AudioGraphNodeID::invalid() };

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
