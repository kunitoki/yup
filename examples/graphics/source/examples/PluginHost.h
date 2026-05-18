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
#include <cmath>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <yup_audio_basics/yup_audio_basics.h>
#include <yup_audio_devices/yup_audio_devices.h>
#include <yup_audio_formats/yup_audio_formats.h>
#include <yup_audio_gui/yup_audio_gui.h>
#include <yup_audio_plugin_host/yup_audio_plugin_host.h>
#include <yup_audio_processors/yup_audio_processors.h>
#include <yup_core/yup_core.h>
#include <yup_dsp/yup_dsp.h>
#include <yup_gui/yup_gui.h>

//==============================================================================

/**
    Demonstrates scanning for audio plugins, listing results, loading one,
    and running a drum loop through it — similar to CrossoverDemo.
*/
class PluginHostDemo : public yup::Component
    , public yup::AudioIODeviceCallback
    , public yup::Timer
{
    class PluginEditorWindow : public yup::DocumentWindow
    {
    public:
        PluginEditorWindow (yup::StringRef windowTitle,
                            yup::AudioProcessorEditor* editorToOwn,
                            std::function<void()> onCloseCallback)
            : yup::DocumentWindow (makeWindowOptions (editorToOwn), yup::Color (0xff101417))
            , editor (editorToOwn)
            , onClose (std::move (onCloseCallback))
        {
            setTitle (windowTitle);

            addAndMakeVisible (*editor);
            // DocumentWindow is already native here, so late children need this hook manually.
            editor->attachedToNative();
            takeKeyboardFocus();
        }

        void resized() override
        {
            editor->setBounds (getLocalBounds());
        }

        void userTriedToCloseWindow() override
        {
            if (onClose != nullptr)
                yup::MessageManager::callAsync (onClose);
        }

    private:
        static yup::ComponentNative::Options makeWindowOptions (yup::AudioProcessorEditor* editor)
        {
            return yup::ComponentNative::Options()
                .withResizableWindow (editor != nullptr && editor->isResizable())
                .withRenderContinuous (editor != nullptr && editor->shouldRenderContinuous());
        }

        std::unique_ptr<yup::AudioProcessorEditor> editor;
        std::function<void()> onClose;
    };

    struct ParameterRow : public yup::Component
    {
        ParameterRow()
            : slider (yup::Slider::LinearHorizontal)
        {
            setOpaque (true);
            setWantsMouseEvents (false, true);

            const auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f);
            nameLabel.setFont (font);
            valueLabel.setFont (font);
            valueLabel.setJustification (yup::Justification::right);

            nameLabel.setColor (yup::Label::Style::textFillColorId, yup::Color (0xffe8ecef));
            valueLabel.setColor (yup::Label::Style::textFillColorId, yup::Color (0xffb7c1c8));

            slider.setColor (yup::Slider::Style::backgroundColorId, yup::Color (0xff343b40));
            slider.setColor (yup::Slider::Style::trackColorId, yup::Color (0xff6d7d88));
            slider.setColor (yup::Slider::Style::thumbColorId, yup::Color (0xffe0a84d));
            slider.setColor (yup::Slider::Style::thumbOverColorId, yup::Color (0xfff0ba60));
            slider.setColor (yup::Slider::Style::thumbDownColorId, yup::Color (0xffc78f36));

            addAndMakeVisible (nameLabel);
            addAndMakeVisible (valueLabel);
            addAndMakeVisible (slider);

            slider.onDragStart = [this] (const yup::MouseEvent&)
            {
                if (parameter != nullptr)
                    parameter->beginChangeGesture();
            };

            slider.onValueChanged = [this] (double value)
            {
                if (parameter == nullptr)
                    return;

                parameter->setValueNotifyingHost (static_cast<float> (value));
                updateValueLabel();
            };

            slider.onDragEnd = [this] (const yup::MouseEvent&)
            {
                if (parameter != nullptr)
                    parameter->endChangeGesture();
            };
        }

        void setParameter (yup::AudioParameter::Ptr newParameter)
        {
            parameter = std::move (newParameter);

            if (parameter == nullptr)
            {
                nameLabel.setText ({}, yup::dontSendNotification);
                valueLabel.setText ({}, yup::dontSendNotification);
                return;
            }

            nameLabel.setText (parameter->getName(), yup::dontSendNotification);
            slider.setRange (static_cast<double> (parameter->getMinimumValue()),
                             static_cast<double> (parameter->getMaximumValue()));
            slider.setDefaultValue (static_cast<double> (parameter->getDefaultValue()));
            refreshValue();
        }

        void refreshValue()
        {
            if (parameter == nullptr)
                return;

            slider.setValue (static_cast<double> (parameter->getValue()), yup::dontSendNotification);
            updateValueLabel();
        }

        void paint (yup::Graphics& g) override
        {
            g.setFillColor (yup::Color (0xff20262a));
            g.fillAll();

            g.setStrokeColor (yup::Color (0xff31383e));
            g.setStrokeWidth (1.0f);
            g.strokeLine (0.0f, getHeight() - 0.5f, getWidth(), getHeight() - 0.5f);
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced (8, 4);
            auto labelArea = bounds.removeFromLeft (yup::jmin (180.0f, bounds.getWidth() * 0.35f));
            nameLabel.setBounds (labelArea.removeFromTop (bounds.getHeight()));

            bounds.removeFromLeft (8);
            valueLabel.setBounds (bounds.removeFromRight (84));
            bounds.removeFromRight (8);
            slider.setBounds (bounds);
        }

    private:
        void updateValueLabel()
        {
            if (parameter != nullptr)
                valueLabel.setText (parameter->convertToString (parameter->getValue()), yup::dontSendNotification);
        }

        yup::AudioParameter::Ptr parameter;
        yup::Label nameLabel;
        yup::Label valueLabel;
        yup::Slider slider;
    };

    struct ParameterListModel : public yup::ListBoxModel
    {
        int getNumRows() override
        {
            return static_cast<int> (parameters.size());
        }

        yup::Component* refreshComponentForRow (int rowIndex, yup::Component* existingComponent) override
        {
            auto* row = dynamic_cast<ParameterRow*> (existingComponent);
            if (row == nullptr)
                row = new ParameterRow();

            if (rowIndex >= 0 && rowIndex < static_cast<int> (parameters.size()))
                row->setParameter (parameters[static_cast<std::size_t> (rowIndex)]);
            else
                row->setParameter (nullptr);

            return row;
        }

        void setParameters (std::vector<yup::AudioParameter::Ptr> newParameters)
        {
            parameters = std::move (newParameters);
        }

        void clear()
        {
            parameters.clear();
        }

    private:
        std::vector<yup::AudioParameter::Ptr> parameters;
    };

public:
    PluginHostDemo()
        : dryWetSlider (yup::Slider::LinearHorizontal)
    {
        // Load the drum loop audio file
        loadAudioFile();

        // Audio device manager
        audioDeviceManager.initialiseWithDefaultDevices (0, 2);

        // Initialize the plugin scanner
        scanner = std::make_unique<yup::AudioPluginScanner>();

        // Register available format backends
#if YUP_AUDIO_PLUGIN_HOST_ENABLE_VST3
        scanner->addFormat (std::make_unique<yup::VST3Format>());
#endif
#if YUP_AUDIO_PLUGIN_HOST_ENABLE_CLAP
        scanner->addFormat (std::make_unique<yup::CLAPFormat>());
#endif
#if YUP_AUDIO_PLUGIN_HOST_ENABLE_AU && YUP_MAC
        scanner->addFormat (std::make_unique<yup::AUv2Format>());
#endif

        // Build UI
        createUI();

        startTimerHz (20);
    }

    ~PluginHostDemo() override
    {
        scanLifetime->store (false);
        if (scanner != nullptr)
            scanner->cancelPendingScans();

        audioDeviceManager.removeAudioCallback (this);
        audioDeviceManager.closeAudioDevice();
        unloadPlugin();
        cleanupRetiredPlugins (true);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (8);

        // Top bar: scan button and status
        auto topBar = bounds.removeFromTop (32);
        scanButton.setBounds (topBar.removeFromLeft (140));
        topBar.removeFromLeft (8);
        statusLabel.setBounds (topBar);

        bounds.removeFromTop (8);

        // Plugin list takes left portion
        auto listArea = bounds.removeFromLeft (220);
        scanResultsLabel.setBounds (listArea.removeFromTop (22));
        pluginList.setBounds (listArea.reduced (0, 4));

        bounds.removeFromLeft (8);

        // Plugin info + load/unload controls
        auto infoArea = bounds.removeFromTop (126);

        auto infoLeft = infoArea.removeFromLeft (infoArea.getWidth() / 2);
        nameLabel.setBounds (infoLeft.removeFromTop (20));
        vendorLabel.setBounds (infoLeft.removeFromTop (18));
        formatLabel.setBounds (infoLeft.removeFromTop (18));
        infoLeft.removeFromTop (8);
        presetLabel.setBounds (infoLeft.removeFromTop (18));
        presetCombo.setBounds (infoLeft.removeFromTop (28).reduced (0, 2));

        auto infoRight = infoArea;
        infoRight.removeFromTop (4);
        loadButton.setBounds (infoRight.removeFromTop (28).reduced (2, 0));
        infoRight.removeFromTop (6);
        unloadButton.setBounds (infoRight.removeFromTop (28).reduced (2, 0));
        infoRight.removeFromTop (6);
        openEditorButton.setBounds (infoRight.removeFromTop (28).reduced (2, 0));
        infoRight.removeFromTop (6);
        dryWetLabel.setBounds (infoRight.removeFromTop (20));

        bounds.removeFromTop (8);

        // Dry/wet slider
        auto sliderRow = bounds.removeFromTop (36);
        dryWetSlider.setBounds (sliderRow);

        bounds.removeFromTop (8);

        // Parameter controls area (generic fallback UI)
        parameterList.setBounds (bounds);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::Color (0xff1b2024));
        g.fillAll();
    }

    void visibilityChanged() override
    {
        if (! isVisible())
            audioDeviceManager.removeAudioCallback (this);
        else
            audioDeviceManager.addAudioCallback (this);
    }

    //==============================================================================
    // AudioIODeviceCallback

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const yup::AudioIODeviceCallbackContext& context) override
    {
        // Zero outputs
        for (int ch = 0; ch < numOutputChannels; ++ch)
            yup::FloatVectorOperations::clear (outputChannelData[ch], numSamples);

        // Read from drum loop buffer into a temp buffer for processing
        for (int i = 0; i < numSamples; ++i)
        {
            // Read drum loop (looping)
            float sample = 0.0f;
            if (audioBufferLoaded)
            {
                const int numChannels = audioBuffer.getNumChannels();
                const int totalSamples = audioBuffer.getNumSamples();

                for (int ch = 0; ch < yup::jmin (2, numChannels); ++ch)
                    sample += audioBuffer.getSample (ch, readPosition) * 0.5f;
                sample /= yup::jmin (2, numChannels);

                readPosition++;
                if (readPosition >= totalSamples)
                    readPosition = 0;
            }

            // Write as stereo
            processBuffer.setSample (0, i, sample);
            processBuffer.setSample (1, i, sample);
            dryBuffer.setSample (0, i, sample);
            dryBuffer.setSample (1, i, sample);
        }

        // If a plugin is loaded, process through it
        yup::MidiBuffer midi;

        if (auto plugin = getLoadedPlugin())
            plugin->processBlock (processBuffer, midi);

        // Mix dry/wet into output
        const float wet = dryWetMix.getNextValue();
        const float dry = 1.0f - wet;

        for (int i = 0; i < numSamples; ++i)
        {
            for (int ch = 0; ch < yup::jmin (2, numOutputChannels); ++ch)
            {
                outputChannelData[ch][i] = dryBuffer.getSample (ch, i) * dry
                                         + processBuffer.getSample (ch, i) * wet;
            }
        }
    }

    void audioDeviceAboutToStart (yup::AudioIODevice* device) override
    {
        const auto sr = device->getCurrentSampleRate();
        const auto bs = device->getCurrentBufferSizeSamples();

        readPosition = 0;
        dryWetMix.reset (sr, 0.02);

        processBuffer.setSize (2, bs);
        dryBuffer.setSize (2, bs);

        if (auto plugin = getLoadedPlugin())
            plugin->prepareToPlay (sr, bs);
    }

    void audioDeviceStopped() override
    {
        if (auto plugin = getLoadedPlugin())
            plugin->releaseResources();
    }

    //==============================================================================
    // Timer

    void timerCallback() override
    {
        updateStatusLine();
        cleanupRetiredPlugins();

        // Update visible parameter rows from plugin values
        if (getLoadedPlugin() != nullptr)
            refreshVisibleParameterRows();
    }

private:
    //==============================================================================

    void loadAudioFile()
    {
#if YUP_WASM
        auto dataDir = yup::File ("/data");
#else
        auto dataDir = yup::File (__FILE__)
                           .getParentDirectory()
                           .getParentDirectory()
                           .getParentDirectory()
                           .getChildFile ("data");
#endif

        yup::File audioFile = dataDir.getChildFile ("break_boomblastic_92bpm.mp3");
        if (! audioFile.existsAsFile())
        {
            statusText = "Could not find drum loop file";
            return;
        }

        yup::AudioFormatManager formatManager;
        formatManager.registerDefaultFormats();

        if (auto reader = formatManager.createReaderFor (audioFile))
        {
            audioBuffer.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);
            reader->read (&audioBuffer, 0, (int) reader->lengthInSamples, 0, true, true);
            audioBufferLoaded = true;

            std::cout << "Loaded: " << audioFile.getFileName() << std::endl;
            std::cout << "Sample rate: " << reader->sampleRate << " Hz, "
                      << reader->numChannels << " ch, "
                      << reader->lengthInSamples << " samples" << std::endl;
        }
        else
        {
            statusText = "Failed to read drum loop";
        }
    }

    void createUI()
    {
        setOpaque (false);

        // Scan button
        scanButton.setButtonText ("Scan for Plugins");
        scanButton.onClick = [this]
        {
            performScan();
        };
        addAndMakeVisible (scanButton);

        // Status label
        statusLabel.setFont (yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f));
        statusLabel.setText ("Ready. Click 'Scan for Plugins' to begin.", yup::dontSendNotification);
        addAndMakeVisible (statusLabel);

        // Scan results label
        scanResultsLabel.setFont (yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f));
        scanResultsLabel.setText ("Plugins (0 found)", yup::dontSendNotification);
        addAndMakeVisible (scanResultsLabel);

        // Plugin list box
        pluginList.setRowHeight (24);
        pluginList.setModel (&pluginListModel);
        pluginListModel.onSelectedRowChanged = [this] (int row)
        {
            if (row >= 0 && row < static_cast<int> (discoveredPlugins.size()))
            {
                selectedPluginIndex = row;
                updateInfoForSelectedPlugin();
            }
        };
        addAndMakeVisible (pluginList);

        // Info labels
        auto infoFont = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (13.0f);
        nameLabel.setFont (infoFont);
        nameLabel.setText ("No plugin selected", yup::dontSendNotification);
        addAndMakeVisible (nameLabel);

        vendorLabel.setFont (infoFont);
        vendorLabel.setText ("", yup::dontSendNotification);
        addAndMakeVisible (vendorLabel);

        formatLabel.setFont (infoFont);
        formatLabel.setText ("", yup::dontSendNotification);
        addAndMakeVisible (formatLabel);

        // Load / Unload buttons
        loadButton.setButtonText ("Load Plugin");
        loadButton.onClick = [this]
        {
            loadSelectedPlugin();
        };
        addAndMakeVisible (loadButton);

        unloadButton.setButtonText ("Unload Plugin");
        unloadButton.onClick = [this]
        {
            unloadPlugin();
        };
        addAndMakeVisible (unloadButton);

        openEditorButton.setButtonText ("Open Editor");
        openEditorButton.onClick = [this]
        {
            openPluginEditor();
        };
        openEditorButton.setEnabled (false);
        addAndMakeVisible (openEditorButton);

        presetLabel.setFont (infoFont);
        presetLabel.setText ("Preset", yup::dontSendNotification);
        addAndMakeVisible (presetLabel);

        presetCombo.setTextWhenNothingSelected ("No presets");
        presetCombo.setEnabled (false);
        presetCombo.onSelectedItemChanged = [this]
        {
            changeSelectedPreset();
        };
        addAndMakeVisible (presetCombo);

        // Dry/Wet
        dryWetLabel.setFont (infoFont);
        dryWetLabel.setText ("Dry / Wet Mix", yup::dontSendNotification);
        addAndMakeVisible (dryWetLabel);

        dryWetSlider.setRange (0.0, 1.0);
        dryWetSlider.setValue (0.5);
        dryWetSlider.onValueChanged = [this] (double value)
        {
            dryWetMix.setTargetValue ((float) value);
        };
        addAndMakeVisible (dryWetSlider);

        dryWetMix.setCurrentAndTargetValue (0.5f);

        // Parameter list area
        parameterList.setRowHeight (40);
        parameterList.setSelectionMode (yup::ListBox::SelectionMode::none);
        parameterList.setColor (yup::ListBox::Style::backgroundColorId, yup::Color (0xff20262a));
        parameterList.setColor (yup::ListBox::Style::outlineColorId, yup::Color (0xff31383e));
        parameterList.setColor (yup::ListBox::Style::rowBackgroundColorId, yup::Color (0xff20262a));
        parameterList.setColor (yup::ListBox::Style::selectedRowBackgroundColorId, yup::Color (0xff20262a));
        parameterList.setColor (yup::ListBox::Style::hoveredRowBackgroundColorId, yup::Color (0xff242b30));
        parameterList.setModel (&parameterListModel);
        addAndMakeVisible (parameterList);
        parameterList.setVisible (false);
    }

    void performScan()
    {
        if (scanInProgress.exchange (true))
        {
            statusText = "Scan already in progress.";
            return;
        }

        discoveredPlugins.clear();
        pluginListModel.clear();
        pluginList.updateContent();
        selectedPluginIndex = -1;
        clearPresetCombo();
        updateEditorButtonState();

        if (scanner == nullptr || scanner->getNumFormats() == 0)
        {
            scanInProgress = false;
            statusText = "No plugin formats registered.";
            return;
        }

        statusText = "Scanning...";
        scanButton.setEnabled (false);
        scanResultsLabel.setText ("Plugins (scanning...)", yup::dontSendNotification);

        const auto lifetime = scanLifetime;
        scanner->scanDefaultsAsync ([this, lifetime] (yup::AudioPluginScanner::ScanResult result) mutable
        {
            if (! lifetime->load())
                return;

            yup::MessageManager::callAsync ([this, lifetime, result = std::move (result)]() mutable
            {
                if (! lifetime->load())
                    return;

                handleScanFinished (std::move (result));
            });
        });
    }

    void handleScanFinished (yup::AudioPluginScanner::ScanResult result)
    {
        discoveredPlugins = std::move (result.discovered);
        statusText = yup::String ("Found ") + yup::String (discoveredPlugins.size()) + " plugins.";

        if (! result.failedPaths.empty())
            statusText += " (" + yup::String ((int) result.failedPaths.size()) + " failed)";

        // Populate list model
        yup::Array<yup::String> names;
        for (const auto& desc : discoveredPlugins)
            names.add (desc.name + " [" + formatTypeToString (desc.formatType) + "]");

        pluginListModel.setItems (std::move (names));
        pluginList.updateContent();
        scanResultsLabel.setText ("Plugins (" + yup::String (discoveredPlugins.size()) + " found)",
                                  yup::dontSendNotification);
        scanButton.setEnabled (true);
        scanInProgress = false;
    }

    void updateInfoForSelectedPlugin()
    {
        if (selectedPluginIndex < 0 || selectedPluginIndex >= static_cast<int> (discoveredPlugins.size()))
            return;

        const auto& desc = discoveredPlugins[static_cast<std::size_t> (selectedPluginIndex)];

        nameLabel.setText (desc.name, yup::dontSendNotification);
        vendorLabel.setText ("Vendor: " + desc.vendor, yup::dontSendNotification);

        auto formatText = "Format: " + formatTypeToString (desc.formatType);
        if (desc.numInputChannels > 0 || desc.numOutputChannels > 0)
        {
            formatText += " | " + yup::String (desc.numInputChannels) + " in / "
                        + yup::String (desc.numOutputChannels) + " out";
        }

        formatLabel.setText (formatText, yup::dontSendNotification);
    }

    void loadSelectedPlugin()
    {
        if (selectedPluginIndex < 0 || selectedPluginIndex >= static_cast<int> (discoveredPlugins.size()))
        {
            statusText = "No plugin selected.";
            return;
        }

        const auto& desc = discoveredPlugins[static_cast<std::size_t> (selectedPluginIndex)];

        // Find the right format backend
        auto* format = scanner->getFormatForType (desc.formatType);
        if (format == nullptr)
        {
            statusText = "No format backend for: " + formatTypeToString (desc.formatType);
            return;
        }

        yup::AudioPluginHostContext hostCtx;
        hostCtx.sampleRate = audioDeviceManager.getCurrentAudioDevice()
                               ? audioDeviceManager.getCurrentAudioDevice()->getCurrentSampleRate()
                               : 44100.0f;
        hostCtx.maxBlockSize = audioDeviceManager.getCurrentAudioDevice()
                                 ? audioDeviceManager.getCurrentAudioDevice()->getCurrentBufferSizeSamples()
                                 : 512;

        auto result = format->loadPlugin (desc, hostCtx);

        if (result.failed())
        {
            statusText = "Failed to load: " + result.getErrorMessage();
            return;
        }

        closePluginEditor();
        clearParameterSliders();
        clearPresetCombo();

        auto plugin = std::shared_ptr<yup::AudioPluginInstance> (std::move (result).getValue());
        plugin->prepareToPlay (hostCtx.sampleRate, hostCtx.maxBlockSize);
        retirePlugin (std::atomic_exchange (&loadedPlugin, std::move (plugin)));

        buildParameterSliders();
        populatePresetCombo();
        updateEditorButtonState();

        if (statusText.isEmpty() || ! statusText.startsWith ("Loaded: "))
            statusText = "Loaded: " + desc.name;
    }

    void unloadPlugin()
    {
        closePluginEditor();
        clearParameterSliders();
        clearPresetCombo();

        unloadPluginInternal();

        updateEditorButtonState();
        statusText = "Plugin unloaded.";
    }

    void unloadPluginInternal()
    {
        retirePlugin (std::atomic_exchange (&loadedPlugin, std::shared_ptr<yup::AudioPluginInstance>()));
    }

    void openPluginEditor()
    {
        if (pluginEditorWindow != nullptr)
        {
            pluginEditorWindow->toFront (true);
            return;
        }

        yup::AudioProcessorEditor* editor = nullptr;
        yup::String pluginName;

        auto plugin = getLoadedPlugin();

        if (plugin == nullptr)
        {
            statusText = "No plugin loaded.";
            return;
        }

        if (! plugin->hasEditor())
        {
            statusText = "Loaded plugin has no editor.";
            return;
        }

        pluginName = plugin->getDescription().name;
        editor = plugin->createEditor();

        if (editor == nullptr)
        {
            statusText = "Could not create plugin editor.";
            updateEditorButtonState();
            return;
        }

        const auto preferredSize = editor->getPreferredSize();
        yup::WeakReference<yup::Component> weakThis (this);

        pluginEditorWindow = std::make_unique<PluginEditorWindow> (
            pluginName,
            editor,
            [weakThis]
        {
            if (auto* component = weakThis.get())
                if (auto* demo = dynamic_cast<PluginHostDemo*> (component))
                    demo->closePluginEditor();
        });

        pluginEditorWindow->centreWithSize (preferredSize);
        pluginEditorWindow->setVisible (true);
        pluginEditorWindow->toFront (true);

        updateEditorButtonState();
    }

    void closePluginEditor()
    {
        pluginEditorWindow.reset();
        updateEditorButtonState();
    }

    void updateEditorButtonState()
    {
        bool canOpenEditor = false;

        if (auto plugin = getLoadedPlugin())
            canOpenEditor = plugin->hasEditor();

        openEditorButton.setEnabled (canOpenEditor);
        openEditorButton.setButtonText (pluginEditorWindow != nullptr ? "Show Editor" : "Open Editor");
    }

    void populatePresetCombo()
    {
        clearPresetCombo();

        auto plugin = getLoadedPlugin();
        if (plugin == nullptr)
            return;

        const auto numPresets = plugin->getNumPresets();
        if (numPresets <= 0)
            return;

        for (int i = 0; i < numPresets; ++i)
        {
            auto name = plugin->getPresetName (i);
            if (name.isEmpty())
                name = "Preset " + yup::String (i + 1);

            presetCombo.addItem (name, i + 1);
        }

        const auto currentPreset = plugin->getCurrentPreset();
        if (yup::isPositiveAndBelow (currentPreset, numPresets))
            presetCombo.setSelectedId (currentPreset + 1, yup::dontSendNotification);

        presetCombo.setEnabled (true);
    }

    void clearPresetCombo()
    {
        presetCombo.clear();
        presetCombo.setEnabled (false);
    }

    void changeSelectedPreset()
    {
        const auto selectedPreset = presetCombo.getSelectedId() - 1;
        if (selectedPreset < 0)
            return;

        yup::String presetName;

        auto plugin = getLoadedPlugin();
        if (plugin == nullptr)
            return;

        plugin->setCurrentPreset (selectedPreset);
        presetName = plugin->getPresetName (selectedPreset);

        statusText = "Preset: " + (presetName.isNotEmpty() ? presetName : yup::String (selectedPreset + 1));
        refreshVisibleParameterRows();
    }

    void buildParameterSliders()
    {
        clearParameterSliders();

        auto plugin = getLoadedPlugin();
        if (plugin == nullptr)
            return;

        const auto params = plugin->getParameters();
        if (params.empty())
        {
            statusText = "Loaded: " + plugin->getDescription().name + " (no adjustable parameters)";
            return;
        }

        std::vector<yup::AudioParameter::Ptr> parameters;
        parameters.reserve (params.size());

        for (auto& param : params)
            parameters.push_back (param);

        parameterListModel.setParameters (std::move (parameters));
        parameterList.updateContent();
        parameterList.setVisible (true);

        resized();
    }

    void clearParameterSliders()
    {
        parameterListModel.clear();
        parameterList.updateContent();
        parameterList.setVisible (false);
        resized();
    }

    std::shared_ptr<yup::AudioPluginInstance> getLoadedPlugin()
    {
        return std::atomic_load (&loadedPlugin);
    }

    void retirePlugin (std::shared_ptr<yup::AudioPluginInstance> plugin)
    {
        if (plugin != nullptr)
            retiredPlugins.push_back ({ std::move (plugin), 20 });
    }

    void cleanupRetiredPlugins (bool force = false)
    {
        for (auto iterator = retiredPlugins.begin(); iterator != retiredPlugins.end();)
        {
            if (! force && --iterator->timerTicksRemaining > 0)
            {
                ++iterator;
                continue;
            }

            iterator->plugin->releaseResources();
            iterator = retiredPlugins.erase (iterator);
        }
    }

    void refreshVisibleParameterRows()
    {
        if (parameterListModel.getNumRows() <= 0)
            return;

        const auto visibleRows = parameterList.getVisibleRowRange();
        for (int row = visibleRows.getStart(); row < visibleRows.getEnd(); ++row)
            if (auto* parameterRow = dynamic_cast<ParameterRow*> (parameterList.getComponentForRow (row)))
                parameterRow->refreshValue();
    }

    void updateStatusLine()
    {
        statusLabel.setText (statusText, yup::dontSendNotification);
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
                return "Unknown";
        }
    }

    //==============================================================================
    // Plugin list model

    struct PluginListModel : public yup::ListBoxModel
    {
        int getNumRows() override
        {
            return static_cast<int> (items.size());
        }

        yup::String getRowText (int rowIndex) override
        {
            if (rowIndex >= 0 && rowIndex < static_cast<int> (items.size()))
                return items[rowIndex];
            return {};
        }

        void selectedRowsChanged (const yup::Array<int>& selectedRows) override
        {
            if (onSelectedRowChanged && ! selectedRows.isEmpty())
                onSelectedRowChanged (selectedRows[0]);
        }

        void setItems (yup::Array<yup::String> items)
        {
            this->items = std::move (items);
        }

        void clear()
        {
            items.clear();
        }

        std::function<void (int)> onSelectedRowChanged;

    private:
        yup::Array<yup::String> items;
    };

    struct RetiredPlugin
    {
        std::shared_ptr<yup::AudioPluginInstance> plugin;
        int timerTicksRemaining = 0;
    };

    //==============================================================================

    // Audio
    yup::AudioDeviceManager audioDeviceManager;
    yup::AudioBuffer<float> audioBuffer;
    yup::AudioBuffer<float> processBuffer;
    yup::AudioBuffer<float> dryBuffer;

    bool audioBufferLoaded = false;
    int readPosition = 0;

    // Plugin hosting
    std::shared_ptr<std::atomic<bool>> scanLifetime { std::make_shared<std::atomic<bool>> (true) };
    std::atomic<bool> scanInProgress { false };
    std::unique_ptr<yup::AudioPluginScanner> scanner;
    std::vector<yup::AudioPluginDescription> discoveredPlugins;
    int selectedPluginIndex = -1;
    std::shared_ptr<yup::AudioPluginInstance> loadedPlugin;
    std::vector<RetiredPlugin> retiredPlugins;
    std::unique_ptr<PluginEditorWindow> pluginEditorWindow;

    // Dry/wet mix
    yup::SmoothedValue<float> dryWetMix;

    // UI
    yup::TextButton scanButton;
    yup::Label statusLabel;
    yup::Label scanResultsLabel;
    yup::ListBox pluginList;
    PluginListModel pluginListModel;

    yup::Label nameLabel;
    yup::Label vendorLabel;
    yup::Label formatLabel;

    yup::TextButton loadButton;
    yup::TextButton unloadButton;
    yup::TextButton openEditorButton;

    yup::Label presetLabel;
    yup::ComboBox presetCombo;

    yup::Label dryWetLabel;
    yup::Slider dryWetSlider;

    yup::ListBox parameterList;
    ParameterListModel parameterListModel;

    yup::String statusText;
};
