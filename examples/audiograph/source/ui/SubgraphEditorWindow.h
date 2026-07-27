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
#include <memory>

#include "AudioGraphEditorPanel.h"

//==============================================================================
class SubgraphEditorWindow final : public yup::DocumentWindow
{
public:
    SubgraphEditorWindow (yup::StringRef title,
                          std::unique_ptr<AudioGraphEditorPanel> panelToOwn,
                          const SubgraphConfig& config,
                          std::function<void (int)> onPresetChangedIn,
                          std::function<void()> onClosedIn)
        : yup::DocumentWindow (yup::ComponentNative::Options().withAllowedHighDensityDisplay (true).withResizableWindow (true),
                               yup::Color (0xff0d1117))
        , panel (std::move (panelToOwn))
        , onPresetChanged (std::move (onPresetChangedIn))
        , onClosed (std::move (onClosedIn))
    {
        setTitle (title);
        configureCombo (config);
        addAndMakeVisible (ioLabel);
        addAndMakeVisible (ioCombo);
        addAndMakeVisible (*panel);
    }

    void setEditorPanel (std::unique_ptr<AudioGraphEditorPanel> panelToOwn, const SubgraphConfig& config)
    {
        removeChildComponent (panel.get());
        panel = std::move (panelToOwn);
        addAndMakeVisible (*panel);
        configureCombo (config);
        resized();
    }

    AudioGraphEditorPanel* getEditorPanel() const noexcept { return panel.get(); }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto toolbar = bounds.removeFromTop (36);

        ioLabel.setBounds (toolbar.removeFromLeft (64).reduced (4, 4));
        ioCombo.setBounds (toolbar.removeFromLeft (190).reduced (4, 4));
        panel->setBounds (bounds);
    }

    void keyDown (const yup::KeyPress& keys, const yup::Point<float>&) override
    {
        if (keys.getKey() == yup::KeyPress::escapeKey)
            userTriedToCloseWindow();
    }

    void userTriedToCloseWindow() override
    {
        if (onClosed != nullptr)
            onClosed();
    }

private:
    void configureCombo (const SubgraphConfig& config)
    {
        ioLabel.setText ("I/O", yup::dontSendNotification);

        ioCombo.onSelectedItemChanged = nullptr;
        ioCombo.clear();
        ioCombo.addItem ("Mono", SubgraphConfig::mono);
        ioCombo.addItem ("Stereo", SubgraphConfig::stereo);
        ioCombo.addItem ("Mono + MIDI", SubgraphConfig::monoMidi);
        ioCombo.addItem ("Stereo + MIDI", SubgraphConfig::stereoMidi);
        ioCombo.setSelectedId (config.getPresetID(), yup::dontSendNotification);
        ioCombo.onSelectedItemChanged = [this]
        {
            if (onPresetChanged != nullptr)
                onPresetChanged (ioCombo.getSelectedId());
        };
    }

    std::unique_ptr<AudioGraphEditorPanel> panel;
    yup::Label ioLabel;
    yup::ComboBox ioCombo;
    std::function<void (int)> onPresetChanged;
    std::function<void()> onClosed;
};
