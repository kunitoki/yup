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
#include <yup_gui/yup_gui.h>

#include "../dsp/SynthTuning.h"
#include "SynthIconButton.h"
#include "SynthSegmented.h"

//==============================================================================
/** The strip above the keys: the pitch wheel ranges, what they count, the octave the
    computer keyboard plays, and the tuning.

    The ranges bend down and up separately, each set by dragging its box up or down (a
    double-click returns it to its default), in semitones or in steps of the tuning. The
    tuning is picked from the built-in ones in a list or with the arrows beside it, or
    loaded from a Scala (.scl, .kbm) or AnaMark (.tun, .msf) file; a loaded file is listed
    by its name until another tuning is picked.

    @see SynthTuning
*/
class SynthKeyboardToolbar
    : public yup::Component
    , private yup::ChangeListener
{
public:
    /** Creates the toolbar.

        @param tuningToEdit   the tuning and pitch wheel ranges it edits
        @param keyboard       the keyboard whose computer-key base octave it sets
    */
    SynthKeyboardToolbar (SynthTuning& tuningToEdit, yup::MidiKeyboardComponent& keyboard);
    ~SynthKeyboardToolbar() override;

    /** Asks for a tuning file and loads it. */
    void loadTuningFile();

    void resized() override;
    void paint (yup::Graphics& g) override;

private:
    static constexpr float arrowWidth = 22.0f;
    static constexpr float rangeWidth = 52.0f;
    static constexpr float modeWidth = 92.0f;
    static constexpr float captionWidth = 50.0f;
    static constexpr float keysCaptionWidth = 36.0f;
    static constexpr float comboBoxChrome = 16.0f + 20.0f; // ComboBox text padding and arrow
    static constexpr int maxKeyPressOctave = 8;
    static constexpr float gap = 6.0f;
    static constexpr int customTuningId = 1000;

    /** A sunken box showing a whole number, stepped by dragging up or down. */
    class ValueBox : public yup::Component
    {
    public:
        ValueBox (int minimum, int maximum, int defaultValue, std::function<yup::String (int)> format, std::function<void (int)> onChange);

        void setValue (int newValue);

        void paint (yup::Graphics& g) override;
        void mouseDown (const yup::MouseEvent& event) override;
        void mouseDrag (const yup::MouseEvent& event) override;
        void mouseDoubleClick (const yup::MouseEvent& event) override;

    private:
        static constexpr float pixelsPerStep = 6.0f;

        void changeTo (int newValue);

        int minimum, maximum, defaultValue;
        std::function<yup::String (int)> format;
        std::function<void (int)> onChange;
        int value = defaultValue;
        int dragStartValue = defaultValue;
        yup::Font valueFont;
    };

    void changeListenerCallback (yup::ChangeBroadcaster* source) override;
    void syncFromTuning();
    void stepPreset (int delta);
    float getTuningBoxWidth() const;

    SynthTuning& tuning;
    yup::MidiKeyboardComponent& keyboard;
    ValueBox bendDownBox { 0, SynthTuning::maxBendRange, 2, [] (int range) { return "-" + yup::String (range); }, [this] (int range) { tuning.setBendDown (range); } };
    ValueBox bendUpBox { 0, SynthTuning::maxBendRange, 2, [] (int range) { return "+" + yup::String (range); }, [this] (int range) { tuning.setBendUp (range); } };
    ValueBox octaveBox { 0, maxKeyPressOctave, 3, [this] (int octave) { return yup::MidiMessage::getMidiNoteName (octave * 12, true, true, keyboard.getOctaveForMiddleC()); }, [this] (int octave) { keyboard.setKeyPressBaseOctave (octave); } };
    SynthSegmented modeSelector { std::vector<SynthSegmented::Segment> { { "Semi" }, { "Step" } } };
    yup::ComboBox tuningBox { "Tuning" };
    SynthIconButton previousButton { "PreviousTuning", YUP_ICON_CHEVRON_LEFT, 11.0f, false };
    SynthIconButton nextButton { "NextTuning", YUP_ICON_CHEVRON_RIGHT, 11.0f, false };
    SynthIconButton loadButton { "LoadTuning", YUP_ICON_FOLDER_OPEN, 12.0f, false };
    yup::Rectangle<float> bendCaption;
    yup::Rectangle<float> keysCaption;
    yup::Rectangle<float> tuningCaption;
    yup::Font captionFont;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthKeyboardToolbar)
};
