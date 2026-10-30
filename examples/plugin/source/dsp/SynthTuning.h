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

#include <yup_audio_basics/yup_audio_basics.h>
#include <yup_data_model/yup_data_model.h>
#include <yup_events/yup_events.h>

#include <array>
#include <atomic>

//==============================================================================
/** How the pitch wheel bends a tuned note. */
enum class SynthBendMode
{
    semitones, /**< By equal tempered semitones, whatever the tuning. */
    steps      /**< By keys of the tuning, so a bend lands on its notes. */
};

//==============================================================================
/** The pitch of every MIDI key under a tuning, as a fractional MIDI note number (69 is
    A4 at 440 Hz), which the voices read.

    Keys the tuning leaves unmapped get a pitch interpolated between their mapped
    neighbors, so a glide or a bend crossing them stays smooth, but they start no note.
*/
struct SynthTuningTable
{
    std::array<double, 128> pitches {};
    std::array<bool, 128> mapped {};

    /** Creates the 12 tone equal temperament table. */
    SynthTuningTable() noexcept;

    /** Returns the pitch at a fractional key, interpolated between keys and continued at
        the slope of the outermost keys beyond the MIDI range. */
    double getPitch (double key) const noexcept;

    /** True when a key starts a note. */
    bool isMapped (int key) const noexcept { return yup::isPositiveAndBelow (key, 128) && mapped[static_cast<std::size_t> (key)]; }
};

//==============================================================================
/** The tuning and the pitch wheel ranges: patch state, not parameters.

    The tuning is a built-in preset or a tuning file loaded by the user, a Scala scale
    (.scl) with an optional key map (.kbm), or an AnaMark tuning (.tun, .msf). The file
    contents are kept and saved with the patch, so a patch plays the same anywhere.

    The message thread changes the tuning, rebuilds the table under a spin lock and bumps
    the generation; the audio thread copies the table with a try-lock once the generation
    moves, like SynthCurveState. The bend ranges and mode are atomics read every block.

    Every change is broadcast, asynchronously, to the controls showing it.

    @see yup::TuningMap, SynthTuningTable
*/
class SynthTuning : public yup::ChangeBroadcaster
{
public:
    /** Creates 12 tone equal temperament with a two semitone bend each way. */
    SynthTuning();

    //==============================================================================
    /** Returns how many built-in tunings there are; the first is 12 tone equal temperament. */
    static int getNumPresets() noexcept;

    /** Returns the name of a built-in tuning, or empty for an index out of range. */
    static yup::String getPresetName (int index);

    /** Plays a built-in tuning. Message thread. */
    void setPreset (int index);

    /** Returns the built-in tuning playing, or -1 when a file was loaded. */
    int getPresetIndex() const noexcept { return presetIndex; }

    /** Loads a tuning file, by its extension: .scl and .kbm replace the scale or the key
        map of a Scala tuning, keeping the other, and .tun and .msf load an AnaMark tuning.

        A file that fails to load leaves the tuning as it was. Message thread.
    */
    yup::Result loadFile (const yup::File& file);

    /** Returns the name the tuning is shown with. */
    yup::String getName() const { return name; }

    //==============================================================================
    /** The largest bend range, in semitones or steps. */
    static constexpr int maxBendRange = 48;

    /** Returns how far the wheel bends down, in semitones or steps. */
    int getBendDown() const noexcept { return bendDown.load(); }

    /** Returns how far the wheel bends up, in semitones or steps. */
    int getBendUp() const noexcept { return bendUp.load(); }

    /** Returns what the bend ranges count. */
    SynthBendMode getBendMode() const noexcept { return bendMode.load(); }

    /** Sets how far the wheel bends down, limited to 0 to maxBendRange. Message thread. */
    void setBendDown (int range);

    /** Sets how far the wheel bends up, limited to 0 to maxBendRange. Message thread. */
    void setBendUp (int range);

    /** Sets what the bend ranges count. Message thread. */
    void setBendMode (SynthBendMode mode);

    /** Returns a pitch wheel position, 0 to 16383, as -1 to 1 with the center at 0. */
    static double normalizePitchWheel (int wheelValue) noexcept
    {
        const auto offset = static_cast<double> (wheelValue - 8192);
        return yup::jlimit (-1.0, 1.0, offset / (offset < 0.0 ? 8192.0 : 8191.0));
    }

    //==============================================================================
    /** Copies the table when the generation moved since seenGeneration. Audio thread.

        @returns True when the table was copied, in which case seenGeneration is updated.
    */
    bool copyIfChanged (SynthTuningTable& destination, int& seenGeneration) const noexcept;

    //==============================================================================
    /** Writes the tuning, its file contents and the bend settings into a "Tuning" tree. */
    yup::DataTree createTree() const;

    /** Replaces everything from a "Tuning" tree; a missing or unreadable tree gives the
        defaults of the constructor. Message thread. */
    void readFrom (const yup::DataTree& tree);

    /** The type of the tree createTree() returns. */
    static const yup::Identifier treeType;

private:
    /** The texts a tuning is built from: an AnaMark tuning, or a scale and a key map. */
    struct Source
    {
        yup::String scale;
        yup::String keyMap;
        yup::String tuning;
    };

    yup::Result apply (const Source& newSource, int newPresetIndex, const yup::String& newName);

    Source source;
    int presetIndex = 0;
    yup::String name;

    mutable yup::SpinLock lock;
    SynthTuningTable table;
    std::atomic<int> generation { 0 };

    std::atomic<int> bendDown { 2 };
    std::atomic<int> bendUp { 2 };
    std::atomic<SynthBendMode> bendMode { SynthBendMode::semitones };

    YUP_DECLARE_NON_COPYABLE (SynthTuning)
};
