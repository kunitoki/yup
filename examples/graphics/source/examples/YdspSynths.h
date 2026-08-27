/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#include <yup_dsp_jit/yup_dsp_jit.h>
#include <yup_audio_devices/yup_audio_devices.h>
#include <yup_audio_gui/yup_audio_gui.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

//==============================================================================
// Palette matching cmake/platforms/emscripten/shell.html's dark theme, so the
// native demo and the web shell around it read as one product.
namespace
{
constexpr yup::uint32 paletteVoidColor = 0xff07090e;
constexpr yup::uint32 paletteSurfaceColor = 0xff0e121a;
constexpr yup::uint32 paletteEdgeColor = 0xff1b2230;
constexpr yup::uint32 paletteInkColor = 0xffe6eaf2;
constexpr yup::uint32 paletteMutedColor = 0xff7c8798;
constexpr yup::uint32 paletteGlowColor = 0xff0a84ff;
constexpr yup::uint32 paletteGlowSoftColor = 0xff6fb6ff;
constexpr yup::uint32 paletteDangerColor = 0xffff6b5a;
} // namespace

//==============================================================================
/** A small waveform display fed from the audio thread. */
class YdspSynthOscilloscope : public yup::Component
{
public:
    YdspSynthOscilloscope()
        : Component ("YdspSynthOscilloscope")
    {
    }

    void setRenderData (const std::vector<float>& data)
    {
        renderData = data;
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::Color (paletteVoidColor));
        g.fillAll();

        auto bounds = getLocalBounds().reduced (4.0f);
        if (renderData.empty() || bounds.isEmpty())
            return;

        auto lineColor = yup::Color (paletteGlowColor);

        const float xSize = static_cast<float> (bounds.getWidth()) / static_cast<float> (renderData.size());
        const float halfHeight = static_cast<float> (bounds.getHeight()) * 0.5f;

        // Build the main waveform path
        path.clear();
        path.reserveSpace (static_cast<int> (renderData.size()));
        path.moveTo (bounds.getX(), bounds.getY() + (renderData[0] + 1.0f) * halfHeight);

        for (std::size_t i = 1; i < renderData.size(); ++i)
            path.lineTo (bounds.getX() + static_cast<float> (i) * xSize, bounds.getY() + (renderData[i] + 1.0f) * halfHeight);

        filledPath = path.createStrokePolygon (2.0f);

        g.setFillColor (lineColor);
        g.setFeather (4.0f);
        g.fillPath (filledPath);

        g.setFillColor (lineColor.brighter (0.2f));
        g.setFeather (2.0f);
        g.fillPath (filledPath);

        g.setStrokeColor (lineColor.withAlpha (0.8f));
        g.setStrokeWidth (1.0f);
        g.strokePath (path);

        g.setStrokeColor (lineColor.brighter (0.3f));
        g.setStrokeWidth (0.5f);
        g.strokePath (path);

        g.setStrokeColor (yup::Colors::white.withAlpha (0.9f));
        g.setStrokeWidth (0.3f);
        g.strokePath (path);
    }

private:
    std::vector<float> renderData;
    yup::Path path;
    yup::Path filledPath;
};

//==============================================================================
/** Plays the YDSP synth patches found in `data/synths/` with extension `.ydsp`.

    Each patch is compiled lazily with yup::DspJitCompiler. Its `input value`
    endpoints drive the on-screen sliders, using the `[[ name, min, max ]]`
    annotations (and the declared default / type) so a patch can describe its
    own UI. The MIDI keyboard feeds note events into the JIT graph, which
    voice-allocates them sample-accurately.
*/
class YdspSynthDemo
    : public yup::Component
    , public yup::AudioIODeviceCallback
    , public yup::MidiInputCallback
{
public:
    YdspSynthDemo()
        : Component ("YdspSynthDemo")
        , keyboardComponent (keyboardState, yup::MidiKeyboardComponent::horizontalKeyboard)
        , pitchWheelComponent (keyboardState, "PitchWheel")
        , modWheelComponent (keyboardState, "ModWheel")
        , codeEditor (codeDocument)
    {
        deviceManager.initialiseWithDefaultDevices (0, 2);

        setColor (yup::DocumentWindow::Style::backgroundColorId, yup::Color (paletteVoidColor));

        {
            yup::MemoryBlock mb;
            auto imageFile = getAssetPath ("data/logo.png");
            if (imageFile.loadFileAsData (mb))
            {
                auto loadedImage = yup::Image::loadFromData (mb.asBytes());
                if (loadedImage.wasOk())
                    logoImage = std::move (loadedImage.getReference());
            }
        }

        midiCollector.reset (44100.0);
        keyboardState.addListener (&midiCollector);

        keyboardComponent.setAvailableRange (36, 84);
        keyboardComponent.setLowestVisibleKey (48);
        keyboardComponent.setMidiChannel (1);
        keyboardComponent.setVelocity (0.7f);
        addAndMakeVisible (keyboardComponent);
        keyboardComponent.takeKeyboardFocus();

        pitchWheelComponent.setClickingGrabFocus (false);
        pitchWheelComponent.onValueChanged = [this] (double value)
        {
            sendExpressionMessage (yup::MidiMessage::pitchWheel (1, 8192 + static_cast<int> (value * 8191.0)));
        };
        addAndMakeVisible (pitchWheelComponent);

        modWheelComponent.setClickingGrabFocus (false);
        modWheelComponent.onValueChanged = [this] (double value)
        {
            sendExpressionMessage (yup::MidiMessage::controllerEvent (1, 1, static_cast<int> (value * 127.0)));
        };
        addAndMakeVisible (modWheelComponent);

        auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont();

        performanceTabButton = std::make_unique<yup::TextButton> ("Performance");
        performanceTabButton->onClick = [this]
        {
            setEditorTabActive (false);
        };
        addAndMakeVisible (*performanceTabButton);

        editorTabButton = std::make_unique<yup::TextButton> ("Editor");
        editorTabButton->onClick = [this]
        {
            setEditorTabActive (true);
        };
        addAndMakeVisible (*editorTabButton);

        brandLabel = std::make_unique<yup::Label> ("Brand");
        brandLabel->setText ("YUP! Synths");
        brandLabel->setColor (yup::Label::Style::textFillColorId, yup::Color (paletteInkColor));
        brandLabel->setJustification (yup::Justification::centerLeft);
        {
            auto brandFont = font.withHeight (21.0f);
            if (brandFont.getAxisDescription ("wght").has_value())
                brandFont = brandFont.withAxisValue ("wght", 700.0f);
            brandLabel->setFont (brandFont);
        }
        addAndMakeVisible (*brandLabel);

        synthCombo = std::make_unique<yup::ComboBox> ("Synth");
        synthCombo->onSelectedItemChanged = [this]
        {
            selectSynth (synthCombo->getSelectedId() - 1);
        };
        addAndMakeVisible (*synthCombo);

        volumeSlider = std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal, "Volume");
        volumeSlider->setRange ({ 0.0, 1.0 });
        volumeSlider->setValue (0.5);
        volumeSlider->setClickingGrabFocus (false);
        volumeSlider->onValueChanged = [this] (double value)
        {
            masterVolume = static_cast<float> (value);
        };
        addAndMakeVisible (*volumeSlider);

        clearButton = std::make_unique<yup::TextButton> ("All Notes Off");
        clearButton->onClick = [this]
        {
            keyboardState.allNotesOff (0);
        };
        addAndMakeVisible (*clearButton);

#if YUP_WASM
        yup::String dumpText = "Dump Wasm";
#else
        yup::String dumpText = "Dump Asm";
#endif
        dumpAsmButton = std::make_unique<yup::TextButton> (dumpText);
        dumpAsmButton->onClick = [this]
        {
            dumpGeneratedAssembly();
        };
        addChildComponent (*dumpAsmButton); // lives in the editor tab, next to Compile - hidden until that tab is active

        createMidiInputCombo();
        createExpressionControls();

        midiDeviceListConnection = yup::MidiDeviceListConnection::make ([this]
        {
            refreshMidiInputCombo();
        });

        addAndMakeVisible (oscilloscope);

        codeEditor.setSyntaxDefinition ("ydsp");
        codeEditor.setScheme (yup::CodeEditorScheme::getBuiltIn ("onedark"));
        addChildComponent (codeEditor);

        compileButton = std::make_unique<yup::TextButton> ("Compile");
        compileButton->onClick = [this]
        {
            compileEditorSource();
        };
        addChildComponent (*compileButton);

        diagnosticsEditor = std::make_unique<yup::TextEditor> ("diagnostics");
        diagnosticsEditor->setMultiLine (true);
        diagnosticsEditor->setReadOnly (true);
        diagnosticsEditor->setColor (yup::TextEditor::Style::textColorId, yup::Color (paletteDangerColor));
        diagnosticsEditor->setFont (yup::ApplicationTheme::getGlobalTheme()->getDefaultMonospaceFont());
        addChildComponent (*diagnosticsEditor);

        loadSynthSources();
        selectSynth (0);
    }

    ~YdspSynthDemo() override
    {
        setEnabledMidiInput ({});
        keyboardState.removeListener (&midiCollector);

        deviceManager.removeAudioCallback (this);
        deviceManager.closeAudioDevice();
    }

    //==============================================================================
    // yup::Component

    // Measures the wordmark's text width, so its box grows with whatever it
    // says instead of clipping - same measurement PopupMenu uses to size its
    // own items.
    float measureBrandLabelWidth() const
    {
        auto font = brandLabel->getFont();
        if (! font.has_value())
            return proportionOfWidth (0.05f);

        yup::StyledText styledText;
        {
            auto modifier = styledText.startUpdate();
            modifier.setWrap (yup::StyledText::noWrap);
            modifier.appendText (brandLabel->getText(), *font);
        }

        return styledText.getComputedTextBounds().getWidth();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Row 2 (MIDI selector + expression sliders) only exists in the
        // Performance tab - the editor tab gives that height back to the
        // code editor and its buttons instead of leaving it empty.
        const auto tabBarHeight = proportionOfHeight (0.06f);
        const auto topBarHeight = showingEditorTab ? 0.0f : proportionOfHeight (0.07f);

        railBounds = bounds.removeFromTop (tabBarHeight + topBarHeight);
        auto rail = railBounds;

        auto tabBar = rail.removeFromTop (tabBarHeight);

        const auto logoSize = tabBar.getHeight() * 0.7f;
        logoBounds = tabBar.removeFromLeft (logoSize + 8.0f).withSizeKeepingCenter (logoSize, logoSize);

        brandLabel->setBounds (tabBar.removeFromLeft (measureBrandLabelWidth() + 12.0f));
        tabBar.removeFromLeft (18.0f); // breathing room before the first dropdown

        const auto comboLeftEdge = tabBar.getX();
        synthCombo->setBounds (tabBar.removeFromLeft (proportionOfWidth (0.14f)).reduced (6));
        tabBar.removeFromLeft (10.0f);

        const auto slotWidth = tabBar.getWidth() / 5.0f;
        performanceTabButton->setBounds (tabBar.removeFromLeft (slotWidth).reduced (4.0f, 6.0f));
        editorTabButton->setBounds (tabBar.removeFromLeft (slotWidth).reduced (4.0f, 6.0f));
        volumeSlider->setBounds (tabBar.removeFromLeft (slotWidth).reduced (4.0f, 6.0f));
        oscilloscope.setBounds (tabBar.removeFromLeft (slotWidth).reduced (4.0f, 6.0f));

        if (clearButton != nullptr)
            clearButton->setBounds (tabBar.removeFromLeft (slotWidth).reduced (4.0f, 6.0f));

        if (showingEditorTab)
        {
            layoutEditorTab (bounds);
            return;
        }

        auto topBar = rail;
        topBar.removeFromLeft (comboLeftEdge - topBar.getX());

        midiInputCombo->setBounds (topBar.removeFromRight (slotWidth * 2.0f).reduced (6));

        layoutExpressionBar (topBar);

        auto keyboardHeight = proportionOfHeight (0.18f);
        keyboardRailBounds = bounds.removeFromBottom (keyboardHeight);
        auto keyboardRow = keyboardRailBounds.reduced (proportionOfWidth (0.015f), proportionOfHeight (0.015f));

        const auto wheelWidth = proportionOfWidth (0.025f);
        pitchWheelComponent.setBounds (keyboardRow.removeFromLeft (wheelWidth).reduced (2.0f, 0.0f));
        modWheelComponent.setBounds (keyboardRow.removeFromLeft (wheelWidth).reduced (2.0f, 0.0f));

        keyboardComponent.setBounds (keyboardRow.withTrimmedLeft (proportionOfWidth (0.01f))); // gap before the keyboard

        if (! meters.empty())
            layoutMeters (bounds.removeFromBottom (proportionOfHeight (0.06f)).reduced (proportionOfWidth (0.04f), 0.0f));

        layoutParamSliders (bounds);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Color (paletteVoidColor)));
        g.fillAll();

        paintRail (g, railBounds, true);

        if (logoImage.isValid())
            g.drawImage (logoImage, logoBounds);

        if (! showingEditorTab)
        {
            paintRail (g, keyboardRailBounds, false);
            paintParamCards (g);
            paintMeters (g);
        }
    }

    void paintRail (yup::Graphics& g, yup::Rectangle<float> bounds, bool edgeAtBottom)
    {
        if (bounds.isEmpty())
            return;

        g.setFillColor (yup::Color (paletteSurfaceColor));
        g.fillRect (bounds);

        g.setStrokeColor (yup::Color (paletteEdgeColor));
        g.setStrokeWidth (1.0f);

        const auto edgeY = edgeAtBottom ? bounds.getBottom() : bounds.getY();
        g.strokeLine (yup::Point<float> (bounds.getX(), edgeY), yup::Point<float> (bounds.getRight(), edgeY));
    }

    void mouseDown (const yup::MouseEvent&) override
    {
        if (! showingEditorTab)
            keyboardComponent.takeKeyboardFocus();
        else
            takeKeyboardFocus();
    }

    void refreshDisplay (double) override
    {
        {
            const AudioLockType::ScopedLockType sl (renderMutex);
            oscilloscope.setRenderData (renderData);
        }

        oscilloscope.repaint();

        if (! meters.empty() && ! showingEditorTab)
        {
            if (auto graph = getCurrentGraph())
                for (auto& meter : meters)
                    meter.value = graph->getOutputValue (meter.name);

            repaint();
        }
    }

    void visibilityChanged() override
    {
        if (! isVisible())
            deviceManager.removeAudioCallback (this);
        else
            deviceManager.addAudioCallback (this);
    }

    //==============================================================================
    // yup::AudioIODeviceCallback

    void audioDeviceAboutToStart (yup::AudioIODevice* device) override
    {
        deviceSampleRate = device->getCurrentSampleRate();
        deviceBufferSize = device->getCurrentBufferSizeSamples();

        renderBuffer.assign (static_cast<size_t> (deviceBufferSize), 0.0f);
        renderBufferRight.assign (static_cast<size_t> (deviceBufferSize), 0.0f);
        renderData.assign (static_cast<size_t> (deviceBufferSize), 0.0f);

        midiMessages.ensureSize (4096);

        midiCollector.reset (deviceSampleRate);
        midiCollector.ensureStorageAllocated (4096);

        if (auto graph = getCurrentGraph())
            graph->prepare (deviceSampleRate, deviceBufferSize);
    }

    void audioDeviceStopped() override
    {
    }

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const yup::AudioIODeviceCallbackContext&) override
    {
        (void) inputChannelData;
        (void) numInputChannels;

        auto graph = getCurrentGraph();

        const auto graphOutputs = graph != nullptr ? graph->getOutputStreamCount() : 0;

        if (graph == nullptr || ! graph->isValid() || graphOutputs < 1 || graphOutputs > 2)
        {
            for (int sample = 0; sample < numSamples; ++sample)
                for (int channel = 0; channel < numOutputChannels; ++channel)
                    outputChannelData[channel][sample] = 0.0f;

            return;
        }

        static thread_local const yup::DspJitGraph* wasmPrewarmedGraph = nullptr;
        if (wasmPrewarmedGraph != graph.get())
        {
            graph->prewarmKernels();
            wasmPrewarmedGraph = graph.get();
        }

        midiMessages.clear();
        midiCollector.removeNextBlockOfMessages (midiMessages, numSamples);

        if (renderBuffer.size() < static_cast<size_t> (numSamples)
            || renderBufferRight.size() < static_cast<size_t> (numSamples))
        {
            for (int sample = 0; sample < numSamples; ++sample)
                for (int channel = 0; channel < numOutputChannels; ++channel)
                    outputChannelData[channel][sample] = 0.0f;

            return;
        }

        yup::DspJitOutputBuffer outputBuffers[] = {
            yup::Span<float> (renderBuffer.data(), static_cast<size_t> (numSamples)),
            yup::Span<float> (renderBufferRight.data(), static_cast<size_t> (numSamples))
        };

        graph->process ({}, yup::Span<yup::DspJitOutputBuffer> (outputBuffers, static_cast<size_t> (graphOutputs)), numSamples, &midiMessages, nullptr, 0);

        const AudioLockType::ScopedLockType sl (renderMutex);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto left = std::tanh (renderBuffer[static_cast<size_t> (sample)] * masterVolume);
            const auto right = graphOutputs > 1
                                 ? std::tanh (renderBufferRight[static_cast<size_t> (sample)] * masterVolume)
                                 : left;

            for (int channel = 0; channel < numOutputChannels; ++channel)
                outputChannelData[channel][sample] = (channel % 2) == 0 ? left : right;

            renderData[static_cast<size_t> (sample)] = graphOutputs > 1 ? (left + right) * 0.5f : left;
        }
    }

    //==============================================================================
    // yup::MidiInputCallback

    void handleIncomingMidiMessage (yup::MidiInput*, const yup::MidiMessage& message) override
    {
        keyboardState.processNextMidiEvent (message);

        if (! message.isNoteOnOrOff())
            midiCollector.addMessageToQueue (message);
    }

private:
    //==============================================================================
    // MIDI input and expression controls

    void createMidiInputCombo()
    {
        midiInputCombo = std::make_unique<yup::ComboBox> ("MIDI In");
        midiInputCombo->onSelectedItemChanged = [this]
        {
            const int index = midiInputCombo->getSelectedId() - 1;

            setEnabledMidiInput (index > 0 && index <= midiInputDevices.size()
                                     ? midiInputDevices.getReference (index - 1).identifier
                                     : yup::String());
        };
        addAndMakeVisible (*midiInputCombo);

        refreshMidiInputCombo();
    }

    void refreshMidiInputCombo()
    {
        const auto previouslyEnabled = enabledMidiInput;

        midiInputDevices = yup::MidiInput::getAvailableDevices();

        midiInputCombo->clear();
        midiInputCombo->addItem ("MIDI In: None", 1);

        int selectedIndex = 0;

        for (int i = 0; i < midiInputDevices.size(); ++i)
        {
            midiInputCombo->addItem (midiInputDevices.getReference (i).name, i + 2);

            if (midiInputDevices.getReference (i).identifier == previouslyEnabled)
                selectedIndex = i + 1;
        }

        // Nothing enabled yet and devices are available - enable the first one,
        // e.g. at startup or when devices first appear after the WASM permission
        // grant.
        if (selectedIndex == 0 && ! midiInputDevices.isEmpty())
        {
            setEnabledMidiInput (midiInputDevices.getReference (0).identifier);
            selectedIndex = 1;
        }

        midiInputCombo->setSelectedItemIndex (selectedIndex, yup::dontSendNotification);
    }

    void setEnabledMidiInput (const yup::String& deviceIdentifier)
    {
        if (enabledMidiInput.isNotEmpty())
        {
            deviceManager.removeMidiInputDeviceCallback (enabledMidiInput, this);
            deviceManager.setMidiInputDeviceEnabled (enabledMidiInput, false);
        }

        enabledMidiInput = deviceIdentifier;

        if (enabledMidiInput.isNotEmpty())
        {
            deviceManager.setMidiInputDeviceEnabled (enabledMidiInput, true);
            deviceManager.addMidiInputDeviceCallback (enabledMidiInput, this);
        }
    }

    void sendExpressionMessage (yup::MidiMessage message)
    {
        message.setTimeStamp (yup::Time::getMillisecondCounterHiRes() * 0.001);

        midiCollector.addMessageToQueue (message);
    }

    void createExpressionControls()
    {
        auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont();

        const auto addControl = [&] (const char* caption,
                                     double minValue,
                                     double maxValue,
                                     double initialValue,
                                     double stepSize)
        {
            auto* label = expressionLabels.add (std::make_unique<yup::Label> (yup::String ("ExprLabel") + caption));
            label->setText (caption, yup::dontSendNotification);
            label->setColor (yup::Label::Style::textFillColorId, yup::Color (paletteMutedColor));
            label->setFont (font.withHeight (13.0f));
            label->setJustification (yup::Justification::centerLeft);
            label->setWantsMouseEvents (false, false);
            addAndMakeVisible (*label);

            auto* slider = expressionSliders.add (std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal, caption));
            slider->setRange (minValue, maxValue, stepSize);
            slider->setValue (initialValue, yup::dontSendNotification);
            slider->setDefaultValue (initialValue);
            slider->setClickingGrabFocus (false);
            addAndMakeVisible (*slider);

            return slider;
        };

        addControl ("Press", 0.0, 1.0, 0.0, 0.0)->onValueChanged = [this] (double value)
        {
            sendExpressionMessage (yup::MidiMessage::channelPressureChange (1, static_cast<int> (value * 127.0)));
        };

        addControl ("Slide", 0.0, 1.0, 0.5, 0.0)->onValueChanged = [this] (double value)
        {
            sendExpressionMessage (yup::MidiMessage::controllerEvent (1, 74, static_cast<int> (value * 127.0)));
        };

        addControl ("Prog", 0.0, 7.0, 0.0, 1.0)->onValueChanged = [this] (double value)
        {
            sendExpressionMessage (yup::MidiMessage::programChange (1, static_cast<int> (value)));
        };
    }

    //==============================================================================
    // Meters

    struct MeterDisplay
    {
        yup::String name;
        float value = 0.0f;
        yup::Rectangle<float> bounds;
    };

    void rebuildMeters (const yup::DspJitGraph& graph)
    {
        meters.clear();

        for (int i = 0; i < graph.getOutputValueCount(); ++i)
            meters.push_back ({ graph.getOutputValueName (i), 0.0f, {} });
    }

    void layoutMeters (yup::Rectangle<float> bounds)
    {
        if (meters.empty())
            return;

        const float cellWidth = bounds.getWidth() / static_cast<float> (meters.size());

        for (size_t i = 0; i < meters.size(); ++i)
            meters[i].bounds = yup::Rectangle<float> (bounds.getX() + static_cast<float> (i) * cellWidth,
                                                      bounds.getY(),
                                                      cellWidth,
                                                      bounds.getHeight())
                                   .reduced (6.0f, 2.0f);
    }

    void paintMeters (yup::Graphics& g)
    {
        auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultMonospaceFont().withHeight (12.0f);

        for (const auto& meter : meters)
        {
            auto track = meter.bounds;
            auto caption = track.removeFromTop (track.getHeight() * 0.5f);

            g.setFillColor (yup::Color (paletteMutedColor));
            g.fillFittedText (meter.name, font, caption, yup::Justification::centerLeft);

            g.setFillColor (yup::Color (paletteEdgeColor));
            g.fillRoundedRect (track, 3.0f);

            const auto filled = std::clamp (meter.value, 0.0f, 1.0f);
            if (filled > 0.0f)
            {
                g.setFillColor (yup::Color (paletteGlowColor));
                g.fillRoundedRect (track.withWidth (track.getWidth() * filled), 3.0f);
            }
        }
    }

    //==============================================================================
    // Synth loading

    struct SynthSource
    {
        std::string name;
        std::string path;
        std::string source;
    };

    void loadSynthSources()
    {
        synthSources.clear();

        auto synthsFolder = getAssetPath ("data/synths");
        if (synthsFolder.isDirectory())
        {
            for (yup::DirectoryEntry entry : yup::RangedDirectoryIterator (synthsFolder, false, "*.ydsp", yup::File::findFiles))
            {
                if (! entry.isDirectory() && ! entry.isHidden())
                {
                    auto patchFile = entry.getFile();
                    synthSources.push_back ({ patchFile.getFileNameWithoutExtension().toStdString(),
                                              patchFile.getFullPathName().toStdString(),
                                              patchFile.loadFileAsString().toStdString() });
                }
            }
        }

        std::sort (synthSources.begin(), synthSources.end(), [] (const SynthSource& a, const SynthSource& b)
        {
            return a.name < b.name;
        });

        compiledSynths.resize (synthSources.size());

        synthCombo->clear();

        for (int i = 0; i < static_cast<int> (synthSources.size()); ++i)
            synthCombo->addItem (yup::String (synthSources[static_cast<size_t> (i)].name), i + 1);

        if (synthCombo->getNumItems() > 0)
            synthCombo->setSelectedItemIndex (0, yup::dontSendNotification);
    }

    void selectSynth (int index)
    {
        if (index < 0 || index >= static_cast<int> (synthSources.size()))
            return;

        codeEditor.setText (yup::String (synthSources[static_cast<size_t> (index)].source));
        hideCompileError();

        std::shared_ptr<yup::DspJitGraph> graph;

        auto& cached = compiledSynths[static_cast<size_t> (index)];

        if (cached == nullptr)
        {
            YUP_DBG ("Compiling '" << synthSources[static_cast<size_t> (index)].name << "'...");

            yup::DspJitCompiler compiler;
            auto result = compiler.compile (synthSources[static_cast<size_t> (index)].source,
                                            synthSources[static_cast<size_t> (index)].path);
            if (! result.wasOk())
            {
                showCompileError (compiler.getDiagnostics().toString());
                return;
            }

            auto compiled = std::move (result).getValue();

            cached = std::make_shared<yup::DspJitGraph> (std::move (compiled));
        }

        graph = cached;

        if (deviceBufferSize > 0)
            graph->prepare (deviceSampleRate, deviceBufferSize);

        {
            const AudioLockType::ScopedLockType sl (graphLock);
            currentGraph = graph;
        }

        YUP_DBG (synthSources[static_cast<size_t> (index)].name << " - " << graph->getParameterCount() << " parameter(s)");

        rebuildParamUI (*graph);
    }

    std::shared_ptr<yup::DspJitGraph> getCurrentGraph() const
    {
        const AudioLockType::ScopedLockType sl (graphLock);
        return currentGraph;
    }

    //==============================================================================
    // Performance / Editor tab switching

    void setEditorTabActive (bool active)
    {
        if (showingEditorTab == active)
            return;

        showingEditorTab = active;

        updatePerformanceControlsVisible();
        updateEditorControlsVisible();

        resized();
        repaint();
    }

    void updatePerformanceControlsVisible()
    {
        const bool visible = ! showingEditorTab;

        keyboardComponent.setVisible (visible);
        pitchWheelComponent.setVisible (visible);
        modWheelComponent.setVisible (visible);
        midiInputCombo->setVisible (visible); // row 2 only exists in the Performance tab

        for (auto* slider : expressionSliders)
            slider->setVisible (visible);

        for (auto* label : expressionLabels)
            label->setVisible (visible);

        for (auto* slider : paramSliders)
            slider->setVisible (visible);

        for (auto* label : paramLabels)
            label->setVisible (visible);

        for (auto* valueLabel : paramValueLabels)
            valueLabel->setVisible (visible);
    }

    void updateEditorControlsVisible()
    {
        codeEditor.setVisible (showingEditorTab);
        compileButton->setVisible (showingEditorTab);

        if (dumpAsmButton != nullptr)
            dumpAsmButton->setVisible (showingEditorTab);

        diagnosticsEditor->setVisible (showingEditorTab && hasCompileError);
    }

    //==============================================================================
    // Live compilation

    void compileEditorSource()
    {
        const int index = synthCombo->getSelectedId() - 1;
        if (index < 0 || index >= static_cast<int> (synthSources.size()))
            return;

        const auto source = codeEditor.getText();

        yup::DspJitCompiler compiler;
        auto result = compiler.compile (source, synthSources[static_cast<size_t> (index)].path);

        if (! result.wasOk())
        {
            showCompileError (compiler.getDiagnostics().toString());
            return;
        }

        auto graph = std::make_shared<yup::DspJitGraph> (std::move (result).getValue());

        if (deviceBufferSize > 0)
            graph->prepare (deviceSampleRate, deviceBufferSize);

        retiredGraphs.push_back (compiledSynths[static_cast<size_t> (index)]);

        {
            const AudioLockType::ScopedLockType sl (graphLock);
            retiredGraphs.push_back (currentGraph);
            currentGraph = graph;
        }

        compiledSynths[static_cast<size_t> (index)] = graph;
        synthSources[static_cast<size_t> (index)].source = source.toStdString();

        rebuildParamUI (*graph);
        hideCompileError();
    }

    void showCompileError (const yup::String& diagnostics)
    {
        diagnosticsEditor->setText (diagnostics, yup::dontSendNotification);
        hasCompileError = true;
        updateEditorControlsVisible();
        resized();
    }

    void hideCompileError()
    {
        hasCompileError = false;
        updateEditorControlsVisible();
        resized();
    }

    //==============================================================================
    // Parameter sliders

    void rebuildParamUI (const yup::DspJitGraph& graph)
    {
        paramSliders.clear();
        paramLabels.clear();
        paramValueLabels.clear();

        rebuildMeters (graph);

        auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont();
        auto monospaceFont = yup::ApplicationTheme::getGlobalTheme()->getDefaultMonospaceFont();

        const auto accentColor = yup::Color (paletteGlowColor);

        for (int i = 0; i < graph.getParameterCount(); ++i)
        {
            const auto& info = graph.getParameterInfo (i);
            if (info.type != yup::DspJitElementType::float32)
                continue;

            const auto step = info.isDiscrete()
                                ? (info.maxValue - info.minValue) / static_cast<double> (info.discreteValues.size() - 1)
                                : 0.0;

            const auto sliderStep = info.isDiscrete() ? step : info.stepSize;

            const auto valueString = info.isDiscrete()
                                       ? info.labelForValue (info.defaultValue)
                                       : yup::String (info.defaultValue, 3);

            auto slider = paramSliders.add (std::make_unique<yup::Slider> (yup::Slider::RotaryVerticalDrag));
            slider->setRange (info.minValue, info.maxValue, sliderStep);
            slider->setDefaultValue (info.defaultValue);
            slider->setNumDecimalPlacesToDisplay (3);
            slider->setValue (graph.getParameter (info.name), yup::dontSendNotification);
            slider->setClickingGrabFocus (false);
            slider->setColor (yup::Slider::Style::trackColorId, yup::Color (paletteEdgeColor));
            slider->setColor (yup::Slider::Style::thumbColorId, accentColor);
            slider->setColor (yup::Slider::Style::thumbOverColorId, yup::Color (paletteGlowSoftColor));
            slider->setColor (yup::Slider::Style::thumbDownColorId, accentColor.brighter (0.3f));
            addAndMakeVisible (slider);

            auto label = paramLabels.add (std::make_unique<yup::Label> (yup::String ("ParamLabel") + yup::String (i)));
            label->setText (yup::String (info.displayName), yup::dontSendNotification);
            label->setColor (yup::Label::Style::textFillColorId, yup::Color (paletteMutedColor));
            label->setFont (font.withHeight (14.0f));
            label->setJustification (yup::Justification::center);
            label->setWantsMouseEvents (false, false);
            addAndMakeVisible (*label);

            auto valueLabel = paramValueLabels.add (std::make_unique<yup::Label> (yup::String ("ParamValue") + yup::String (i)));
            valueLabel->setText (valueString, yup::dontSendNotification);
            valueLabel->setColor (yup::Label::Style::textFillColorId, yup::Color (paletteInkColor));
            valueLabel->setFont (monospaceFont.withHeight (13.0f));
            valueLabel->setJustification (yup::Justification::center);
            valueLabel->setWantsMouseEvents (false, false);
            addAndMakeVisible (*valueLabel);

            slider->onValueChanged = [this, name = info.name, valueLabel, &info, step] (double value)
            {
                if (auto graph = getCurrentGraph())
                    graph->setParameter (name, static_cast<float> (value));

                valueLabel->setText (step > 0.0 ? info.labelForValue (value)
                                                : yup::String (value, 3),
                                     yup::dontSendNotification);
            };
        }

        updatePerformanceControlsVisible();

        resized();
        repaint();
    }

    void paintParamCards (yup::Graphics& g)
    {
        for (const auto& card : paramCardBounds)
        {
            g.setFillColor (yup::Color (paletteSurfaceColor));
            g.fillRoundedRect (card, 14.0f);

            g.setStrokeColor (yup::Color (paletteEdgeColor));
            g.setStrokeWidth (1.0f);
            g.strokeRoundedRect (card, 14.0f);
        }
    }

    void layoutParamSliders (yup::Rectangle<float> bounds)
    {
        paramCardBounds.clear();

        const int count = paramSliders.size();
        if (count == 0)
            return;

        auto grid = bounds.reduced (proportionOfWidth (0.04f), proportionOfHeight (0.02f));

        constexpr float maxCellSize = 200.0f;
        int columns = 1;
        float cellSize = 0.0f;

        for (int c = 1; c <= count; ++c)
        {
            const int rows = (count + c - 1) / c;
            const float candidate = std::min ({ grid.getWidth() / static_cast<float> (c),
                                                grid.getHeight() / static_cast<float> (rows),
                                                maxCellSize });

            if (candidate > cellSize)
            {
                cellSize = candidate;
                columns = c;
            }
        }

        const int rows = (count + columns - 1) / columns;
        const float gridWidth = cellSize * static_cast<float> (columns);
        const float gridHeight = cellSize * static_cast<float> (rows);
        const float originX = grid.getX() + (grid.getWidth() - gridWidth) * 0.5f;
        const float originY = grid.getY() + (grid.getHeight() - gridHeight) * 0.5f;

        paramCardBounds.reserve (static_cast<size_t> (count));

        for (int i = 0; i < count; ++i)
        {
            const float col = static_cast<float> (i % columns);
            const float row = static_cast<float> (i / columns);

            auto card = yup::Rectangle<float> (originX + col * cellSize, originY + row * cellSize, cellSize, cellSize)
                            .reduced (9.0f);
            paramCardBounds.push_back (card);

            auto cell = card.reduced (10.0f);

            const float captionHeight = std::clamp (cell.getHeight() * 0.13f, 12.0f, 18.0f);
            auto labelBounds = cell.removeFromTop (captionHeight);
            auto valueBounds = cell.removeFromBottom (captionHeight);

            auto knobArea = cell.largestFittingSquare();
            paramSliders.getUnchecked (i)->setBounds (knobArea);

            paramLabels.getUnchecked (i)->setBounds (labelBounds);
            paramValueLabels.getUnchecked (i)->setBounds (valueBounds);
        }
    }

    void layoutExpressionBar (yup::Rectangle<float> bounds)
    {
        const int count = expressionSliders.size();
        if (count == 0)
            return;

        const float cellWidth = bounds.getWidth() / static_cast<float> (count);

        for (int i = 0; i < count; ++i)
        {
            auto cell = bounds.removeFromLeft (cellWidth).reduced (4.0f, 4.0f);

            expressionLabels.getUnchecked (i)->setBounds (cell.removeFromLeft (cell.getWidth() * 0.38f));
            expressionSliders.getUnchecked (i)->setBounds (cell);
        }
    }

    //==============================================================================
    // Layout tabs

    void layoutEditorTab (yup::Rectangle<float> bounds)
    {
        auto compileRow = bounds.removeFromTop (proportionOfHeight (0.06f));
        compileButton->setBounds (compileRow.removeFromLeft (proportionOfWidth (0.15f)).reduced (4));

        if (dumpAsmButton != nullptr)
            dumpAsmButton->setBounds (compileRow.removeFromLeft (proportionOfWidth (0.15f)).reduced (4));

        if (hasCompileError)
            diagnosticsEditor->setBounds (bounds.removeFromBottom (proportionOfHeight (0.2f)).reduced (4));

        codeEditor.setBounds (bounds.reduced (4));
    }

    //==============================================================================
    // Prints the asmjit assembly listing of every compiled kernel of the current patch to the console

    void dumpGeneratedAssembly()
    {
        auto graph = getCurrentGraph();
        if (graph == nullptr)
            return;

        yup::String asmType =
#if YUP_WASM
            "wasm";
#else
            "asm";
#endif

        const int index = synthCombo->getSelectedId() - 1;
        yup::Logger::outputDebugString ("--- YDSP generated " + asmType + ": " + yup::String (synthSources[static_cast<size_t> (index)].name) + " ---");

        const auto& diagnostics = graph->getDiagnostics();

        for (int i = 0; i < diagnostics.getCount(); ++i)
        {
            const auto& item = diagnostics.getItem (i);

            if (item.severity == yup::DspJitSeverity::info)
                yup::Logger::outputDebugString (item.message);
        }
    }

    //==============================================================================

    yup::AudioDeviceManager deviceManager;
    double deviceSampleRate = 0.0;
    int deviceBufferSize = 0;
    float masterVolume = 0.5f;

    // The patch currently being processed
    mutable AudioLockType graphLock;
    std::shared_ptr<yup::DspJitGraph> currentGraph;

    // Lazily compiled patches, cached per combo index
    std::vector<std::shared_ptr<yup::DspJitGraph>> compiledSynths;
    std::vector<SynthSource> synthSources;

    // Graphs retired by a live recompile
    std::vector<std::shared_ptr<yup::DspJitGraph>> retiredGraphs;

    // MIDI keyboard, hardware input and the on-screen expression controls, all
    // merged into one collector that the audio callback drains.
    yup::MidiKeyboardState keyboardState;
    yup::MidiKeyboardComponent keyboardComponent;
    yup::PitchWheelComponent pitchWheelComponent;
    yup::ModWheelComponent modWheelComponent;
    yup::MidiMessageCollector midiCollector;
    yup::Array<yup::MidiDeviceInfo> midiInputDevices;
    yup::String enabledMidiInput;
    yup::MidiDeviceListConnection midiDeviceListConnection;

    // Editor tab: live-editable YDSP source bound to the selected patch
    yup::CodeDocument codeDocument;
    yup::CodeEditor codeEditor;

    // Audio/Midi rendering
    std::vector<float> renderBuffer;
    std::vector<float> renderBufferRight;
    std::vector<float> renderData;
    AudioLockType renderMutex;
    yup::MidiBuffer midiMessages;

    // UI
    yup::Image logoImage;
    yup::Rectangle<float> logoBounds;
    yup::Rectangle<float> railBounds;
    yup::Rectangle<float> keyboardRailBounds;
    std::unique_ptr<yup::Label> brandLabel;
    std::unique_ptr<yup::TextButton> performanceTabButton;
    std::unique_ptr<yup::TextButton> editorTabButton;
    std::unique_ptr<yup::ComboBox> synthCombo;
    std::unique_ptr<yup::Slider> volumeSlider;
    std::unique_ptr<yup::TextButton> clearButton;
    std::unique_ptr<yup::TextButton> dumpAsmButton;
    std::unique_ptr<yup::ComboBox> midiInputCombo;
    yup::OwnedArray<yup::Slider> expressionSliders;
    yup::OwnedArray<yup::Label> expressionLabels;
    std::vector<MeterDisplay> meters;
    yup::OwnedArray<yup::Slider> paramSliders;
    yup::OwnedArray<yup::Label> paramLabels;
    yup::OwnedArray<yup::Label> paramValueLabels;
    std::vector<yup::Rectangle<float>> paramCardBounds;
    YdspSynthOscilloscope oscilloscope;

    // Editor tab
    std::unique_ptr<yup::TextButton> compileButton;
    std::unique_ptr<yup::TextEditor> diagnosticsEditor;
    bool showingEditorTab = false;
    bool hasCompileError = false;
};
