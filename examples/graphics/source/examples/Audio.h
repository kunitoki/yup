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

#include "audio/SynthSettings.h"
#include "audio/SynthEngine.h"
#include "audio/SynthPanels.h"
#include "audio/SynthModulationPage.h"

#include <array>
#include <atomic>
#include <cmath>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

//==============================================================================
/** The PRISM logo, with the viewBox cropped to the drawing so it fills the header slot. */
inline constexpr const char* synthLogoSvg = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" width="236" height="122" viewBox="10 66 236 122" fill="none">
  <g stroke="#FFFFFF" stroke-linejoin="miter" stroke-linecap="square" transform="translate(0 37)">
    <path d="M14 100 q4 -7 8 0 t8 0 t8 0 t8 0 t8 0 t8 0 t8 0 t8 0 t8 0 L91 100" stroke-width="2.2" stroke-linecap="round"/>
    <path d="M128 36 L191.5 146 L64.5 146 Z" stroke-width="3"/>
    <path d="M91 100 L160.3 92" stroke-width="1.5" stroke-opacity="0.5"/>
    <g stroke-linecap="butt">
      <path d="M160.3 92 L242 66"  stroke-width="2.5" stroke-opacity="1"/>
      <path d="M160.3 92 L242 80"  stroke-width="2.2" stroke-opacity="0.8"/>
      <path d="M160.3 92 L242 94"  stroke-width="2"   stroke-opacity="0.6"/>
      <path d="M160.3 92 L242 108" stroke-width="1.7" stroke-opacity="0.42"/>
      <path d="M160.3 92 L242 122" stroke-width="1.5" stroke-opacity="0.26"/>
    </g>
  </g>
</svg>
)svg";

//==============================================================================
/** A page of the instrument: paints nothing and hands clicks on its background back. */
class SynthPage : public yup::Component
{
public:
    /** Construct a page. */
    SynthPage()
    {
        setOpaque (false);
    }

    /** Called when the page itself, not one of its children, is clicked. */
    std::function<void()> onMouseDown;

    void mouseDown (const yup::MouseEvent&) override
    {
        if (onMouseDown != nullptr)
            onMouseDown();
    }
};

//==============================================================================
class AudioExample
    : public yup::Component
    , public yup::AudioIODeviceCallback
{
public:
    AudioExample()
        : Component ("AudioExample")
        , keyboardComponent (keyboardState, yup::MidiKeyboardComponent::horizontalKeyboard)
        , pitchWheelComponent (keyboardState, "PitchWheel")
        , modWheelComponent (keyboardState, "ModWheel")
    {
        audioDeviceError = deviceManager.initialiseWithDefaultDevices (0, 2);

        // The keyboard state is pumped into the synth by processNextMidiBuffer(), so no note
        // listener is registered here: listening as well would trigger every note twice.
        keyboardComponent.setAvailableRange (36, 84); // C2 to C6
        keyboardComponent.setLowestVisibleKey (48);   // Start from C3
        keyboardComponent.setMidiChannel (1);
        keyboardComponent.setVelocity (0.7f);
        keyboardComponent.setColor (yup::MidiKeyboardComponent::Style::whiteKeyColorId, yup::Color (0xffd7dde3));
        keyboardComponent.setColor (yup::MidiKeyboardComponent::Style::whiteKeyPressedColorId, SynthTheme::accent);
        keyboardComponent.setColor (yup::MidiKeyboardComponent::Style::blackKeyColorId, yup::Color (0xff191d21));
        keyboardComponent.setColor (yup::MidiKeyboardComponent::Style::blackKeyPressedColorId, SynthTheme::accentDim);
        keyboardComponent.setColor (yup::MidiKeyboardComponent::Style::keyOutlineColorId, SynthTheme::panelBorder);
        mainPage.addAndMakeVisible (keyboardComponent);

        // Like the keyboard, the wheels follow keyboardState, which the audio callback
        // updates from the hardware input too; their own moves go in through the collector.
        pitchWheelComponent.onValueChanged = [this] (double value)
        {
            sendWheelMessage (yup::MidiMessage::pitchWheel (1, 8192 + static_cast<int> (value * 8191.0)));
        };
        modWheelComponent.onValueChanged = [this] (double value)
        {
            sendWheelMessage (yup::MidiMessage::controllerEvent (1, 1, static_cast<int> (value * 127.0)));
        };

        const auto addWheel = [this] (auto& wheel)
        {
            using Style = typename std::decay_t<decltype (wheel)>::Style;

            wheel.setColor (Style::bodyTopColorId, SynthTheme::panelBackground);
            wheel.setColor (Style::bodyBottomColorId, SynthTheme::displayBackground);
            wheel.setColor (Style::outlineColorId, SynthTheme::panelBorder);
            wheel.setColor (Style::gripColorId, SynthTheme::accentDim);
            wheel.setColor (Style::gripOverColorId, SynthTheme::accent);
            wheel.setColor (Style::gripDownColorId, SynthTheme::accent);
            wheel.setClickingGrabFocus (false);
            mainPage.addAndMakeVisible (wheel);
        };
        addWheel (pitchWheelComponent);
        addWheel (modWheelComponent);

        logo.parseSVG (synthLogoSvg);

        const auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont();

        titleLabel.setText ("P R I S M   /   SPECTRAL SYNTH", yup::dontSendNotification);
        titleLabel.setFont (font.withHeight (20.0f));
        titleLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::textPrimary);
        addAndMakeVisible (titleLabel);

        subtitleLabel.setText ("Sculpt harmonics. Scatter phases. Play the spectrum.", yup::dontSendNotification);
        subtitleLabel.setFont (font.withHeight (12.0f));
        subtitleLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::textSecondary);
        addAndMakeVisible (subtitleLabel);

        loadLabel.setFont (font.withHeight (11.0f));
        loadLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::textSecondary);
        mainPage.addAndMakeVisible (loadLabel);

        voiceLabel.setText ("", yup::dontSendNotification);
        voiceLabel.setFont (font.withHeight (11.0f));
        voiceLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::accent);
        addAndMakeVisible (voiceLabel);

        for (int index = 0; index < SynthExample::oscillatorCount; ++index)
        {
            auto panel = std::make_unique<SynthOscillatorPanel> (
                yup::String ("OSC ") + yup::String (index + 1),
                synth.getOscillatorSettings (index),
                synth.getResources(),
                waveformShader,
                font.withHeight (10.0f));

            mainPage.addAndMakeVisible (*panel);
            oscillatorPanels[static_cast<std::size_t> (index)] = std::move (panel);
        }

        filterPanel = std::make_unique<SynthFilterPanel> (synth.getFilterSettings(), font.withHeight (10.0f));
        mainPage.addAndMakeVisible (*filterPanel);

        for (int index = 0; index < SynthExample::envelopeCount; ++index)
        {
            auto& panel = envelopePanels[static_cast<std::size_t> (index)];
            panel = std::make_unique<SynthEnvelopePanel> (yup::String ("ENV ") + yup::String (index + 1),
                                                          synth.getEnvelopeSettings (index),
                                                          font.withHeight (10.0f));
            mainPage.addAndMakeVisible (*panel);
        }

        for (int index = 0; index < SynthExample::lfoCount; ++index)
        {
            auto& panel = lfoPanels[static_cast<std::size_t> (index)];
            panel = std::make_unique<SynthLFOPanel> (yup::String ("LFO ") + yup::String (index + 1),
                                                     synth.getLFOSettings (index),
                                                     font.withHeight (10.0f));
            mainPage.addAndMakeVisible (*panel);
        }

        modulationPage = std::make_unique<SynthModulationPage> (synth.getModulationSettings(), font.withHeight (10.0f));
        addChildComponent (*modulationPage);

        mainPage.onMouseDown = [this] { takeKeyboardFocus(); };
        addAndMakeVisible (mainPage);

        for (auto* button : { &mainPageButton, &modulationPageButton })
        {
            button->setColor (yup::ToggleButton::Style::backgroundColorId, SynthTheme::panelBackground);
            button->setColor (yup::ToggleButton::Style::backgroundToggledColorId, SynthTheme::accentDim);
            button->setColor (yup::ToggleButton::Style::textColorId, SynthTheme::textSecondary);
            button->setColor (yup::ToggleButton::Style::textToggledColorId, SynthTheme::textPrimary);
            button->setColor (yup::ToggleButton::Style::borderColorId, SynthTheme::panelBorder);
            button->setColor (yup::ToggleButton::Style::borderToggledColorId, SynthTheme::accent);
            addAndMakeVisible (*button);
        }

        mainPageButton.setButtonText ("MAIN");
        modulationPageButton.setButtonText ("MOD");
        mainPageButton.onClick = [this] { showModulationPage (false); };
        modulationPageButton.onClick = [this] { showModulationPage (true); };
        showModulationPage (false);

        randomizeButton.setColor (yup::TextButton::Style::backgroundColorId, SynthTheme::panelBackground);
        randomizeButton.setColor (yup::TextButton::Style::textColorId, SynthTheme::textPrimary);
        randomizeButton.setColor (yup::TextButton::Style::outlineColorId, SynthTheme::panelBorder);
        randomizeButton.onClick = [this] { randomizeVoice(); };
        addAndMakeVisible (randomizeButton);

        clearButton.setColor (yup::TextButton::Style::backgroundColorId, SynthTheme::panelBackground);
        clearButton.setColor (yup::TextButton::Style::textColorId, SynthTheme::textPrimary);
        clearButton.setColor (yup::TextButton::Style::outlineColorId, SynthTheme::panelBorder);
        clearButton.onClick = [this]
        {
            keyboardState.allNotesOff (0); // Turn off all notes on all channels
            synth.requestAllNotesOff();
        };
        addAndMakeVisible (clearButton);

        volumeKnob = std::make_unique<KnobControl> ("VOLUME", 0.0, 1.0, 0.001, 0.5, font.withHeight (10.0f));
        volumeKnob->formatValue = SynthFormat::percent;
        volumeKnob->onChange = [this] (double value) { masterVolume = static_cast<float> (value); };
        addAndMakeVisible (*volumeKnob);

        modeChoice = std::make_unique<ChoiceControl> ("VOICE MODE", yup::StringArray { "Poly / 8 voices", "Mono / retrigger", "Legato / glide" }, font.withHeight (10.0f));
        modeChoice->getComboBox().setSelectedId (1, yup::dontSendNotification);
        modeChoice->onChange = [this] (int id)
        {
            synth.playMode = id - 1;
            glideKnob->setEnabled (id != 1);
        };
        mainPage.addAndMakeVisible (*modeChoice);
        glideKnob = std::make_unique<KnobControl> ("GLIDE", 0.0, 2000.0, 1.0, 120.0, font.withHeight (10.0f));
        glideKnob->formatValue = [] (double value) { return SynthFormat::milliseconds (value * 0.001); };
        glideKnob->onChange = [this] (double value) { synth.portamento = static_cast<float> (value * 0.001); };
        glideKnob->setEnabled (false);
        mainPage.addAndMakeVisible (*glideKnob);
        midiDevices = yup::MidiInput::getAvailableDevices();
        yup::StringArray midiNames { "No MIDI input" };
        for (const auto& device : midiDevices)
            midiNames.add (device.name);
        midiChoice = std::make_unique<ChoiceControl> ("MIDI INPUT", midiNames, font.withHeight (10.0f));
        midiChoice->getComboBox().setSelectedId (midiDevices.isEmpty() ? 1 : 2, yup::dontSendNotification);
        midiChoice->onChange = [this] (int)
        {
            closeMidiInput();
            synth.requestAllNotesOff();
            if (isVisible())
                openMidiInput();
        };
        mainPage.addAndMakeVisible (*midiChoice);
        renderData.resize (SynthExample::maxBlockSize);
        mainPage.addAndMakeVisible (oscilloscope);
    }

    ~AudioExample() override
    {
        closeMidiInput();

        deviceManager.removeAudioCallback (this);
        deviceManager.closeAudioDevice();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (outerInset);

        auto header = bounds.removeFromTop (headerHeight);

        volumeKnob->setBounds (header.removeFromRight (64.0f));
        header.removeFromRight (spacing);
        clearButton.setBounds (header.removeFromRight (actionButtonWidth).reduced (0.0f, buttonInset));
        header.removeFromRight (spacing);
        randomizeButton.setBounds (header.removeFromRight (actionButtonWidth).reduced (0.0f, buttonInset));
        header.removeFromRight (spacing * 2.0f);
        voiceLabel.setBounds (header.removeFromRight (110.0f));
        header.removeFromRight (spacing);
        modulationPageButton.setBounds (header.removeFromRight (pageButtonWidth).reduced (0.0f, buttonInset));
        header.removeFromRight (spacing);
        mainPageButton.setBounds (header.removeFromRight (pageButtonWidth).reduced (0.0f, buttonInset));

        const auto logoHeight = header.getHeight() - 8.0f;
        logoArea = header.removeFromLeft (logoHeight * logoAspect).withSizeKeepingCenter (logoHeight * logoAspect, logoHeight);
        header.removeFromLeft (spacing * 1.5f);

        const auto textArea = header.reduced (0.0f, 6.0f);
        titleLabel.setBounds (textArea.withHeight (textArea.getHeight() * 0.55f));
        subtitleLabel.setBounds (textArea.withTrimmedTop (textArea.getHeight() * 0.55f));

        bounds.removeFromTop (spacing);

        mainPage.setBounds (bounds);
        modulationPage->setBounds (bounds);

        layoutMainPage();
    }

    /** Lays the main page out from the bottom up: keyboard, performance row, LFOs, shaping, oscillators. */
    void layoutMainPage()
    {
        auto bounds = mainPage.getLocalBounds();

        auto keyboardRow = bounds.removeFromBottom (yup::jmin (keyboardHeight, mainPage.proportionOfHeight (0.12f)));
        pitchWheelComponent.setBounds (keyboardRow.removeFromLeft (wheelWidth).reduced (0.0f, 4.0f));
        keyboardRow.removeFromLeft (spacing);
        modWheelComponent.setBounds (keyboardRow.removeFromLeft (wheelWidth).reduced (0.0f, 4.0f));
        keyboardRow.removeFromLeft (spacing * 2.0f);
        keyboardComponent.setBounds (keyboardRow);
        bounds.removeFromBottom (spacing);

        auto performance = bounds.removeFromBottom (58.0f);
        modeChoice->setBounds (performance.removeFromLeft (190.0f).reduced (4.0f, 9.0f));
        glideKnob->setBounds (performance.removeFromLeft (78.0f));
        performance.removeFromLeft (spacing);
        midiChoice->setBounds (performance.removeFromLeft (210.0f).reduced (4.0f, 9.0f));
        performance.removeFromLeft (spacing);
        loadLabel.setBounds (performance);
        bounds.removeFromBottom (spacing);

        // The oscillator panels take whatever the fixed-height rows below leave, and
        // their waveform editors need most of it. The LFO and shaping rows share columns.
        const auto columnWidth = (bounds.getWidth() - spacing * 2.0f) / 3.0f;

        auto lfoRow = bounds.removeFromBottom (lfoRowHeight);
        lfoPanels[0]->setBounds (lfoRow.removeFromLeft (columnWidth));
        lfoRow.removeFromLeft (spacing);
        lfoPanels[1]->setBounds (lfoRow.removeFromLeft (columnWidth));
        lfoRow.removeFromLeft (spacing);
        oscilloscope.setBounds (lfoRow);
        bounds.removeFromBottom (spacing);

        auto shapingRow = bounds.removeFromBottom (yup::jmin (shapingRowHeight, bounds.getHeight() * 0.3f));
        filterPanel->setBounds (shapingRow.removeFromLeft (columnWidth));
        shapingRow.removeFromLeft (spacing);
        envelopePanels[0]->setBounds (shapingRow.removeFromLeft (columnWidth));
        shapingRow.removeFromLeft (spacing);
        envelopePanels[1]->setBounds (shapingRow);
        bounds.removeFromBottom (spacing);

        const auto panelWidth = (bounds.getWidth() - spacing) / static_cast<float> (SynthExample::oscillatorCount);
        for (auto& panel : oscillatorPanels)
        {
            panel->setBounds (bounds.removeFromLeft (panelWidth));
            bounds.removeFromLeft (spacing);
        }
    }

    /** Switches between the main page and the modulation matrix. */
    void showModulationPage (bool show)
    {
        mainPageButton.setToggleState (! show, yup::dontSendNotification);
        modulationPageButton.setToggleState (show, yup::dontSendNotification);
        mainPage.setVisible (! show);
        modulationPage->setVisible (show);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (SynthTheme::windowBackground);
        g.fillAll();

        logo.paint (g, logoArea);
    }

    void mouseDown (const yup::MouseEvent&) override
    {
        takeKeyboardFocus();
    }

    void refreshDisplay (double) override
    {
        if (scopeReady.load (std::memory_order_acquire))
        {
            renderData.assign (scopeSamples.begin(), scopeSamples.begin() + scopeCount);
            scopeReady.store (false, std::memory_order_release);
            oscilloscope.setRenderData (renderData);
        }

        if (oscilloscope.isVisible())
            oscilloscope.repaint();

        // One generation bump per frame, however many mouse events the drag produced.
        for (auto& panel : oscillatorPanels)
            if (panel != nullptr)
                panel->commitPendingEdits();

        for (int index = 0; index < SynthExample::lfoCount; ++index)
            lfoPanels[static_cast<std::size_t> (index)]->setPhase (synth.getLFOPhase (index));

        const auto activeVoices = synth.getNumActiveVoices();

        const auto status = audioDeviceError.isNotEmpty() ? audioDeviceError
                          : midiInputError.isNotEmpty() ? midiInputError
                          : yup::String (loadMeasurer.getLoadAsPercentage(), 1) + "% AUDIO / "
                                + yup::String (loadMeasurer.getXRunCount()) + " OVERRUNS / "
                                + yup::String (receivedNoteOns.load()) + " NOTES IN";
        loadLabel.setText (status, yup::dontSendNotification);
        voiceLabel.setText (yup::String (activeVoices) + " / 8 VOICES",
                            yup::dontSendNotification);
    }

    void audioDeviceAboutToStart (yup::AudioIODevice* device) override
    {
        const auto maxBlockSize = yup::jmax (device->getDefaultBufferSize(), SynthExample::maxBlockSize);

        synth.prepare (device->getCurrentSampleRate(), maxBlockSize);

        renderBuffer.setSize (2, maxBlockSize, false, true, true);
        loadMeasurer.reset (device->getCurrentSampleRate(), device->getDefaultBufferSize());
        midiBuffer.ensureSize (16384);
        outputGain.reset (device->getCurrentSampleRate(), 0.02);
        outputGain.setCurrentAndTargetValue (masterVolume.load());

        midiCollector.reset (device->getCurrentSampleRate());
        midiCollector.ensureStorageAllocated (midiQueueBytes);
    }

    void audioDeviceStopped() override
    {
    }

    void audioDeviceIOCallbackWithContext (const float* const*,
                                           int,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const yup::AudioIODeviceCallbackContext&) override
    {
        if (numSamples <= 0)
            return;
        const yup::ScopedNoDenormals noDenormals;
        const yup::AudioProcessLoadMeasurer::ScopedTimer renderTimer (loadMeasurer, numSamples);
        if (numSamples <= 0 || numSamples > renderBuffer.getNumSamples())
        {
            for (int channel = 0; channel < numOutputChannels; ++channel)
                if (outputChannelData[channel] != nullptr)
                    yup::FloatVectorOperations::clear (outputChannelData[channel], numSamples);

            return;
        }

        for (int channel = 0; channel < renderBuffer.getNumChannels(); ++channel)
            yup::FloatVectorOperations::clear (renderBuffer.getWritePointer (channel), numSamples);

        midiBuffer.clear();

        // processNextMidiBuffer() reads whatever is already in the buffer before injecting
        // the on-screen keyboard's own events, so collecting the hardware input first is
        // what lights up the drawn keys as well as playing the notes.
        midiCollector.removeNextBlockOfMessages (midiBuffer, numSamples);
        keyboardState.processNextMidiBuffer (midiBuffer, 0, numSamples, true);
        for (const auto metadata : midiBuffer)
            if (metadata.getMessage().isNoteOn())
                receivedNoteOns.fetch_add (1, std::memory_order_relaxed);
        synth.renderNextBlock (renderBuffer, midiBuffer, 0, numSamples);

        outputGain.setTargetValue (masterVolume.load());
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto gain = outputGain.getNextValue();
            for (int channel = 0; channel < numOutputChannels; ++channel)
            {
                const auto sourceChannel = yup::jmin (channel, renderBuffer.getNumChannels() - 1);
                if (outputChannelData[channel] != nullptr)
                    outputChannelData[channel][sample] = renderBuffer.getSample (sourceChannel, sample) * gain;
            }
            renderBuffer.setSample (0, sample, renderBuffer.getSample (0, sample) * gain);
        }

        if (! scopeReady.load (std::memory_order_acquire))
        {
            scopeCount = yup::jmin (numSamples, SynthExample::maxBlockSize);
            std::copy_n (renderBuffer.getReadPointer (0), scopeCount, scopeSamples.begin());
            scopeReady.store (true, std::memory_order_release);
        }
    }

    void visibilityChanged() override
    {
        if (! isVisible())
        {
            closeMidiInput();
            deviceManager.removeAudioCallback (this);
        }
        else
        {
            deviceManager.addAudioCallback (this);
            openMidiInput();
        }
    }

private:
    //==============================================================================
    /** Opens the selected hardware input and reports device-open failures in the UI. */
    void openMidiInput()
    {
        if (midiInputIdentifier.isNotEmpty() || midiChoice == nullptr)
            return;

        midiInputError.clear();
        const auto index = midiChoice->getComboBox().getSelectedId() - 2;
        if (! yup::isPositiveAndBelow (index, midiDevices.size()))
            return;

        const auto identifier = midiDevices[index].identifier;
        deviceManager.setMidiInputDeviceEnabled (identifier, true);
        if (! deviceManager.isMidiInputDeviceEnabled (identifier))
        {
            midiInputError = "Cannot open MIDI input: " + midiDevices[index].name;
            return;
        }
        midiInputIdentifier = identifier;
        deviceManager.addMidiInputDeviceCallback (midiInputIdentifier, &midiCollector);
    }

    /** Releases the input again, so a hidden demo does not hold the device open. */
    void closeMidiInput()
    {
        if (midiInputIdentifier.isEmpty())
            return;

        deviceManager.removeMidiInputDeviceCallback (midiInputIdentifier, &midiCollector);
        deviceManager.setMidiInputDeviceEnabled (midiInputIdentifier, false);

        midiInputIdentifier.clear();
    }

    /** Queues a message from one of the on-screen wheels, timestamped as the collector expects. */
    void sendWheelMessage (yup::MidiMessage message)
    {
        // The collector is only reset, and so only ready, once an audio device has started.
        if (deviceManager.getCurrentAudioDevice() == nullptr)
            return;

        message.setTimeStamp (yup::Time::getMillisecondCounterHiRes() * 0.001);
        midiCollector.addMessageToQueue (message);
    }

    //==============================================================================
    /** Randomizes everything a voice is made of; volume, voice mode, glide and MIDI input stay. */
    void randomizeVoice()
    {
        auto& random = yup::Random::getSystemRandom();

        for (int index = 0; index < SynthExample::oscillatorCount; ++index)
        {
            auto& settings = synth.getOscillatorSettings (index);

            settings.waveform = random.nextInt (6);
            settings.syncMode = random.nextInt (4);
            settings.syncRatio = 1.0f + random.nextFloat() * 3.0f;
            settings.level = 0.2f + random.nextFloat() * 0.8f;
            settings.octave = random.nextInt (3) - 1;
            settings.detuneSemitones = (random.nextFloat() - 0.5f) * 0.3f;
            settings.ridgeSpacing = 0.5f + random.nextFloat() * 3.0f;
            settings.color = random.nextFloat();
            settings.dispersion = 0.1f + random.nextFloat() * 0.8f;
            settings.squeeze = random.nextBool() ? 0.0f : random.nextFloat() * 0.5f;
            settings.squash = 0.4f + random.nextFloat() * 1.6f;
            settings.tilt = (random.nextFloat() - 0.5f) * 3.0f;
            settings.oddEven = 0.2f + random.nextFloat() * 0.6f;
            settings.formant = (random.nextFloat() - 0.4f) * 5.0f;
            settings.formantPosition = random.nextFloat() * 6.0f;
            settings.scatter = random.nextBool() ? 0.0f : random.nextFloat() * 0.6f;
            settings.unisonVoices = 1 + random.nextInt (SynthExample::maxUnisonVoices);
            settings.unisonDetune = random.nextFloat() * 0.5f;
            settings.unisonSpread = random.nextFloat();

            // A randomized waveform is only audible once the edited partials are dropped.
            settings.usesCustomSeries = false;
            settings.harmonicGeneration.fetch_add (1);

            if (auto& panel = oscillatorPanels[static_cast<std::size_t> (index)]; panel != nullptr)
                panel->refresh();
        }

        auto& filter = synth.getFilterSettings();
        filter.type = 1 + random.nextInt (6);
        filter.cutoff = 200.0f * std::exp2 (random.nextFloat() * 6.0f);
        filter.resonance = random.nextFloat() * 0.8f;
        filter.drive = random.nextBool() ? 0.0f : random.nextFloat() * 0.6f;
        filter.keytrack = random.nextBool() ? 0.0f : 1.0f;
        filterPanel->refresh();

        // The amplitude envelope keeps a short attack more often than not, so the patch
        // still speaks when played; the modulation envelope is free to be slow.
        for (int index = 0; index < SynthExample::envelopeCount; ++index)
        {
            auto& envelope = synth.getEnvelopeSettings (index);
            const auto slow = index > 0 || random.nextInt (4) == 0;

            envelope.delay = random.nextInt (4) == 0 ? random.nextFloat() * 0.3f : 0.0f;
            envelope.attack = 0.003f + random.nextFloat() * (slow ? 1.5f : 0.15f);
            envelope.hold = random.nextBool() ? 0.0f : random.nextFloat() * 0.3f;
            envelope.decay = 0.05f + random.nextFloat() * 1.5f;
            envelope.sustain = random.nextFloat();
            envelope.release = 0.05f + random.nextFloat() * 1.5f;

            envelopePanels[static_cast<std::size_t> (index)]->refresh();
        }

        for (int index = 0; index < SynthExample::lfoCount; ++index)
        {
            auto& lfo = synth.getLFOSettings (index);

            lfo.shape = random.nextInt (5);
            lfo.rate = 0.1f * std::exp2 (random.nextFloat() * 6.0f);
            lfo.phase = random.nextBool() ? 0.0f : random.nextFloat();
            lfo.retrigger = random.nextBool();

            lfoPanels[static_cast<std::size_t> (index)]->refresh();
        }

        // A few live routes, never to the oscillator levels, so a random patch cannot
        // fall silent; the rest of the slots are cleared.
        auto& modulation = synth.getModulationSettings();
        const auto liveRoutes = 1 + random.nextInt (4);

        for (int index = 0; index < SynthExample::modulationSlots; ++index)
        {
            auto& slot = modulation.slots[static_cast<std::size_t> (index)];

            if (index >= liveRoutes)
            {
                slot.destination = static_cast<int> (SynthModulationDestination::none);
                slot.depth = 0.0f;
                continue;
            }

            auto destination = SynthModulationDestination::none;

            do
            {
                destination = static_cast<SynthModulationDestination> (1 + random.nextInt (static_cast<int> (SynthModulationDestination::count) - 1));
            } while (destination == SynthModulationDestination::osc1Level || destination == SynthModulationDestination::osc2Level);

            slot.source = random.nextInt (4);
            slot.destination = static_cast<int> (destination);
            slot.depth = (random.nextFloat() - 0.5f) * (random.nextBool() ? 2.0f : 1.0f);
        }

        modulationPage->refresh();
    }

    //==============================================================================
    static constexpr std::size_t midiQueueBytes = 2048;

    static constexpr float outerInset = 10.0f;
    static constexpr float headerHeight = 60.0f;
    static constexpr float spacing = 8.0f;
    static constexpr float buttonInset = 12.0f;
    static constexpr float pageButtonWidth = 76.0f;
    static constexpr float actionButtonWidth = 120.0f;
    static constexpr float logoAspect = 236.0f / 122.0f;
    static constexpr float wheelWidth = 28.0f;
    static constexpr float keyboardHeight = 72.0f;
    static constexpr float lfoRowHeight = 104.0f;
    static constexpr float shapingRowHeight = 150.0f;

    //==============================================================================
    yup::AudioDeviceManager deviceManager;
    HarmonicSynthEngine synth;

    // MIDI keyboard components
    yup::MidiKeyboardState keyboardState;
    yup::MidiKeyboardComponent keyboardComponent;
    yup::PitchWheelComponent pitchWheelComponent;
    yup::ModWheelComponent modWheelComponent;
    yup::MidiMessageCollector midiCollector;
    yup::String midiInputIdentifier;
    yup::String audioDeviceError;
    yup::String midiInputError;
    yup::Array<yup::MidiDeviceInfo> midiDevices;
    std::atomic<int> receivedNoteOns { 0 };

    yup::AudioBuffer<float> renderBuffer;
    yup::MidiBuffer midiBuffer;
    std::vector<float> renderData;
    std::array<float, SynthExample::maxBlockSize> scopeSamples {};
    std::atomic<bool> scopeReady { false };
    int scopeCount = 0;
    yup::SmoothedValue<float> outputGain;
    yup::AudioProcessLoadMeasurer loadMeasurer;

    // UI Components
    yup::Drawable logo;
    yup::Rectangle<float> logoArea;
    yup::Label titleLabel;
    yup::Label subtitleLabel;
    yup::Label voiceLabel;
    yup::Label loadLabel;

    SynthPage mainPage;
    std::shared_ptr<SynthWaveformShader> waveformShader = std::make_shared<SynthWaveformShader>();
    std::array<std::unique_ptr<SynthOscillatorPanel>, SynthExample::oscillatorCount> oscillatorPanels;
    std::unique_ptr<SynthFilterPanel> filterPanel;
    std::array<std::unique_ptr<SynthEnvelopePanel>, SynthExample::envelopeCount> envelopePanels;
    std::array<std::unique_ptr<SynthLFOPanel>, SynthExample::lfoCount> lfoPanels;
    std::unique_ptr<SynthModulationPage> modulationPage;
    yup::ToggleButton mainPageButton;
    yup::ToggleButton modulationPageButton;

    yup::TextButton randomizeButton { "RANDOMIZE" };
    yup::TextButton clearButton { "ALL NOTES OFF" };
    std::unique_ptr<KnobControl> volumeKnob;
    std::unique_ptr<ChoiceControl> modeChoice;
    std::unique_ptr<ChoiceControl> midiChoice;
    std::unique_ptr<KnobControl> glideKnob;
    Oscilloscope oscilloscope;

    std::atomic<float> masterVolume { 0.5f };
};
