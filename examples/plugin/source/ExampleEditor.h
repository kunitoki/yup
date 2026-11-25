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

#include <yup_audio_gui/yup_audio_gui.h>

#include "ExamplePlugin.h"

#include <array>
#include <memory>

//==============================================================================
/** The plugin editor: four rotary controls over a playable keyboard.

    Each control is bound to its parameter with an AudioParameterAttachment, so the
    knobs follow host automation and host automation follows the knobs. The keyboard
    plays the shared MidiKeyboardState, which the processor merges into the audio
    block, so notes played here sound through the instrument.

    @see ExamplePlugin
*/
class ExampleEditor final : public yup::AudioProcessorEditor
{
public:
    explicit ExampleEditor (ExamplePlugin& processorToEdit);
    ~ExampleEditor() override;

    //==============================================================================
    bool isResizable() const override { return true; }

    yup::Size<int> getPreferredSize() const override { return { 520, 320 }; }

    bool shouldPreserveAspectRatio() const override { return true; }

    //==============================================================================
    void resized() override;
    void paint (yup::Graphics& g) override;

private:
    //==============================================================================
    struct Knob
    {
        yup::Slider slider { yup::Slider::Rotary };
        std::unique_ptr<yup::AudioParameterAttachment> attachment;
        yup::AudioParameter::Ptr parameter;
        yup::Label caption;
        yup::String name;
    };

    void bindKnob (Knob& knob, const yup::String& parameterID, const yup::String& caption);
    void updateCaption (Knob& knob);
    float getScaleFactor() const;

    ExamplePlugin& processor;
    std::array<Knob, 4> knobs;
    yup::Label title;
    yup::MidiKeyboardComponent keyboard;
    yup::Font font;
    bool updatingFromParameter = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExampleEditor)
};
