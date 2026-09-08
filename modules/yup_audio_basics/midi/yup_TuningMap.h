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

#include <array>
#include <vector>

namespace yup
{

//==============================================================================
/**
    Maps MIDI note numbers to frequencies using an arbitrary scale and key map.

    A TuningMap combines two independent pieces of state:

      - A scale, i.e. the ordered set of pitch intervals the tuning uses within
        each octave. The last scale interval is the octave itself, so a scale
        with N intervals defines N pitch classes per octave (the tonic being
        interval 0) together with the ratio that repeats them at higher octaves.

      - A key map, which describes how each MIDI key relates to the scale
        degrees. Keys are mapped cyclically: after mapSize consecutive keys the
        mapping repeats, each pass advancing by an adjustable number of scale
        degrees, which is what allows an instrument to be laid out so that, for
        example, one octave of keys always spans one octave of the tuning.

    The default configuration is the usual 12 tone equal temperament: the scale
    holds the twelve chromatic intervals, every key maps to the corresponding
    consecutive scale degree, and MIDI note 69 (A4) sounds at 440 Hz.

    Scales and key maps can be loaded from the two text formats used by Scala:

      - loadScale() reads a .scl file describing the scale intervals, either as
        rational frequency ratios (e.g. "5/4") or as values in cents (any value
        containing a decimal point).

      - loadKeyMap() reads a .kbm file describing how the MIDI keys map onto the
        scale, including which notes to retune, which note carries the reference
        frequency, and how the mapping repeats. Keys can be excluded from the
        map with "x" entries, and the optional "&lt; first last" range lines
        declare which notes are considered playable (see isNoteActive()).

    Loading never leaves the tuning half-modified: if a file fails to parse, a
    failed yup::Result is returned and the previously loaded scale/key map stays
    in effect.

    noteToPitch() performs no allocation and can be called from real-time
    threads.

    @tags{Audio}
*/
class YUP_API TuningMap
{
public:
    //==============================================================================
    /** Creates a tuning using a 12 tone equal temperament scale and a
        chromatic key map of all 128 MIDI notes, with A4 (MIDI note 69)
        tuned to 440 Hz.

        @see noteToPitch
    */
    TuningMap();

    /** Destructor. */
    ~TuningMap() noexcept = default;

    //==============================================================================
    /** Returns the frequency, in Hz, of a MIDI note under the current scale
        and key map.

        Notes that the current key map excludes (an "x" entry in a .kbm file)
        are not audible: this method returns a negative value for them.

        @param note    the MIDI note number to look up, in the range 0 to 127

        @returns       the frequency in Hz, or a negative value if the note is
                       unmapped by the current key map (or out of range)

        @see isNoteMapped, loadScale, loadKeyMap
    */
    double noteToPitch (int note) const noexcept;

    /** Returns true if the given MIDI note is mapped to an audible scale
        degree by the current key map.

        @param note    the MIDI note number to look up, in the range 0 to 127

        @see noteToPitch
    */
    bool isNoteMapped (int note) const noexcept;

    /** Returns true if the given MIDI note falls inside the active note range
        declared by the current key map.

        This reflects the optional "&lt; first last" range lines of a .kbm
        file. When a key map declares no range at all, every note is
        considered active.

        @param note    the MIDI note number to look up, in the range 0 to 127

        @see loadKeyMap
    */
    bool isNoteActive (int note) const noexcept;

    //==============================================================================
    /** Loads a scale from a Scala .scl file.

        The file must contain a description line, the number of intervals in
        the scale, and that many intervals, one per line. Comments ("!" lines)
        and blank lines are ignored. An interval is either a rational ratio
        such as "5/4", or a number of cents written with a decimal point (e.g.
        "386.313714").

        If the file cannot be read or does not describe the declared number of
        intervals, a failed yup::Result is returned and the previously loaded
        scale stays in effect.

        @param file    the .scl file to read

        @see loadKeyMap, noteToPitch, getScaleFile
    */
    Result loadScale (const File& file);

    /** Loads a key map from a Scala .kbm file.

        The file describes the size of the key map, the range of notes to
        retune, the note whose frequency is fixed by the reference pitch, and
        the mapping from keys to scale degrees. A "x" entry unmaps its key,
        and the optional "&lt; first last" lines declare the active note range
        (see isNoteActive()).

        A key map size of 0 selects the automatic linear layout, where every
        key is mapped to the consecutive scale degree. Keys listed after the
        header fields map to the given scale degrees, an "x" entry unmaps its
        key, keys that are never listed are treated as unmapped, and any
        entries beyond the declared map size are ignored.

        If the file cannot be read, ends before declaring all of the header
        fields, or leaves the reference note unmapped, a failed yup::Result is
        returned and the previously loaded key map stays in effect.

        @param file    the .kbm file to read

        @see loadScale, noteToPitch, getKeyMapFile
    */
    Result loadKeyMap (const File& file);

    //==============================================================================
    /** Returns the MIDI note number whose scale degree is considered the
        tonic of the current key map, i.e. the key the mapping is anchored to.

        @see getReferenceNote, getKeyMapRepeatIncrement
    */
    int getZeroNote() const noexcept;

    /** Returns the MIDI note whose frequency is fixed by the reference pitch.

        For the default tuning this is 69 (A4).

        @see getReferencePitch
    */
    int getReferenceNote() const noexcept;

    /** Returns the frequency, in Hz, of getReferenceNote() under the current
        scale and key map.

        For the default tuning this is 440 Hz.

        @see getReferenceNote, noteToPitch
    */
    double getReferencePitch() const noexcept;

    /** Returns how many scale degrees the mapping advances every time it
        cycles through the full set of keys in the key map.

        @see getZeroNote
    */
    int getKeyMapRepeatIncrement() const noexcept;

    /** Returns the number of intervals stored in the current scale, i.e. the
        number of pitch classes defined within each octave (the tonic
        included).

        @see noteToPitch, loadScale
    */
    int getNumberOfScaleDegrees() const noexcept;

    /** Returns the path of the .scl file most recently loaded with loadScale(),
        or an empty string if no scale file has been loaded yet.

        @see loadScale
    */
    String getScaleFile() const noexcept;

    /** Returns the path of the .kbm file most recently loaded with
        loadKeyMap(), or an empty string if no key map file has been loaded
        yet.

        @see loadKeyMap
    */
    String getKeyMapFile() const noexcept;

private:
    //==============================================================================
    void activateRange (int firstNote, int lastNote) noexcept;
    void updateBasePitch() noexcept;

    //==============================================================================
    std::vector<double> scale;
    std::vector<int> mapping;
    std::array<bool, 128> activeRange {};

    String scaleFile;
    String keyMapFile;

    int zeroNote = 0;
    int refNote = 69;
    int mapRepeatInc = 1;
    double refPitch = 440.0;
    double basePitch = 1.0;

    YUP_LEAK_DETECTOR (TuningMap)
};

} // namespace yup
