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

#include "SynthTuning.h"

namespace
{
const yup::Identifier nameProperty ("name");
const yup::Identifier presetProperty ("preset");
const yup::Identifier scaleProperty ("scale");
const yup::Identifier keyMapProperty ("keyMap");
const yup::Identifier tuningProperty ("tuning");
const yup::Identifier bendDownProperty ("bendDown");
const yup::Identifier bendUpProperty ("bendUp");
const yup::Identifier bendModeProperty ("bendMode");

/** Writes a Scala scale from its pitches, in cents or as ratios, the period last. */
yup::String makeScale (const yup::String& description, const yup::StringArray& pitches)
{
    return description + "\n" + yup::String (pitches.size()) + "\n" + pitches.joinIntoString ("\n") + "\n";
}

/** Divides a period, given in cents and as a ratio, into equal steps. */
yup::String makeEqualScale (const yup::String& description, int steps, double periodCents, const yup::String& period)
{
    yup::StringArray pitches;

    for (int step = 1; step < steps; ++step)
        pitches.add (yup::String (periodCents * step / steps, 6));

    pitches.add (period);
    return makeScale (description, pitches);
}

const char* const presetNames[] = {
    "12-TET",
    "Just Intonation",
    "Pythagorean",
    "Meantone 1/4 Comma",
    "Werckmeister III",
    "19-EDO",
    "22-EDO",
    "24-EDO",
    "31-EDO",
    "Bohlen-Pierce",
    "Harmonic 8-16"
};

/** Returns the Scala scale of a built-in tuning, empty for 12 tone equal temperament. */
yup::String getPresetScale (int index)
{
    switch (index)
    {
        case 1: return makeScale ("5-limit just intonation", { "16/15", "9/8", "6/5", "5/4", "4/3", "45/32", "3/2", "8/5", "5/3", "9/5", "15/8", "2/1" });
        case 2: return makeScale ("Pythagorean", { "256/243", "9/8", "32/27", "81/64", "4/3", "729/512", "3/2", "128/81", "27/16", "16/9", "243/128", "2/1" });
        case 3: return makeScale ("1/4-comma meantone", { "76.049", "193.157", "310.265", "386.314", "503.422", "579.471", "696.578", "772.627", "889.735", "1006.843", "1082.892", "2/1" });
        case 4: return makeScale ("Werckmeister III", { "90.225", "192.180", "294.135", "390.225", "498.045", "588.270", "696.090", "792.180", "888.270", "996.090", "1092.180", "2/1" });
        case 5: return makeEqualScale ("19 equal divisions of the octave", 19, 1200.0, "2/1");
        case 6: return makeEqualScale ("22 equal divisions of the octave", 22, 1200.0, "2/1");
        case 7: return makeEqualScale ("24 equal divisions of the octave", 24, 1200.0, "2/1");
        case 8: return makeEqualScale ("31 equal divisions of the octave", 31, 1200.0, "2/1");
        case 9: return makeEqualScale ("Bohlen-Pierce, 13 equal divisions of the tritave", 13, 1200.0 * std::log2 (3.0), "3/1");
        case 10: return makeScale ("Harmonics 8 to 16", { "9/8", "10/8", "11/8", "12/8", "13/8", "14/8", "15/8", "2/1" });
        default: break;
    }

    return {};
}
} // namespace

//==============================================================================
SynthTuningTable::SynthTuningTable() noexcept
{
    for (std::size_t key = 0; key < pitches.size(); ++key)
        pitches[key] = static_cast<double> (key);

    mapped.fill (true);
}

double SynthTuningTable::getPitch (double key) const noexcept
{
    if (key <= 0.0)
        return pitches[0] + key * (pitches[1] - pitches[0]);

    if (key >= 127.0)
        return pitches[127] + (key - 127.0) * (pitches[127] - pitches[126]);

    const auto index = static_cast<std::size_t> (key);
    const auto fraction = key - static_cast<double> (index);
    return pitches[index] + fraction * (pitches[index + 1] - pitches[index]);
}

//==============================================================================
const yup::Identifier SynthTuning::treeType ("Tuning");

SynthTuning::SynthTuning()
    : name (getPresetName (0))
{
}

int SynthTuning::getNumPresets() noexcept
{
    return yup::numElementsInArray (presetNames);
}

yup::String SynthTuning::getPresetName (int index)
{
    return yup::isPositiveAndBelow (index, getNumPresets()) ? yup::String (presetNames[index]) : yup::String();
}

void SynthTuning::setPreset (int index)
{
    if (! yup::isPositiveAndBelow (index, getNumPresets()))
        return;

    [[maybe_unused]] const auto result = apply ({ getPresetScale (index), {}, {} }, index, getPresetName (index));
    jassert (result.wasOk());
}

yup::Result SynthTuning::loadFile (const yup::File& file)
{
    if (! file.existsAsFile())
        return yup::Result::fail ("Cannot read " + file.getFullPathName());

    const auto extension = file.getFileExtension().toLowerCase();
    const auto text = file.loadFileAsString();
    const auto fileName = file.getFileNameWithoutExtension();
    auto newSource = source;

    if (extension == ".scl")
    {
        newSource.scale = text;
        newSource.tuning = {};
        return apply (newSource, -1, fileName);
    }

    if (extension == ".kbm")
    {
        newSource.keyMap = text;
        newSource.tuning = {};
        const auto scaleName = source.tuning.isEmpty() ? name.upToFirstOccurrenceOf (" / ", false, false) : getPresetName (0);
        return apply (newSource, -1, scaleName + " / " + fileName);
    }

    if (extension == ".tun" || extension == ".msf")
        return apply ({ {}, {}, text }, -1, fileName);

    return yup::Result::fail ("Not a tuning file: " + file.getFileName());
}

yup::Result SynthTuning::apply (const Source& newSource, int newPresetIndex, const yup::String& newName)
{
    yup::TuningMap map;

    if (newSource.tuning.isNotEmpty())
    {
        if (auto result = map.loadTuningFromString (newSource.tuning); result.failed())
            return result;
    }
    else
    {
        if (newSource.scale.isNotEmpty())
            if (auto result = map.loadScaleFromString (newSource.scale); result.failed())
                return result;

        if (newSource.keyMap.isNotEmpty())
            if (auto result = map.loadKeyMapFromString (newSource.keyMap); result.failed())
                return result;
    }

    SynthTuningTable newTable;
    auto& pitches = newTable.pitches;

    for (int key = 0; key < 128; ++key)
    {
        const auto index = static_cast<std::size_t> (key);
        const auto frequency = map.noteToPitch (key);

        newTable.mapped[index] = map.isNoteMapped (key) && map.isNoteActive (key) && frequency > 0.0 && std::isfinite (frequency);

        if (newTable.mapped[index])
            pitches[index] = 69.0 + 12.0 * std::log2 (frequency / 440.0);
    }

    // Unmapped keys lie between their mapped neighbors, a semitone apart past the outermost ones.
    int previous = -1;

    for (int key = 0; key < 128; ++key)
    {
        const auto index = static_cast<std::size_t> (key);

        if (! newTable.mapped[index])
            continue;

        for (int gap = previous + 1; gap < key; ++gap)
        {
            pitches[static_cast<std::size_t> (gap)] = previous < 0
                                                        ? pitches[index] - (key - gap)
                                                        : pitches[static_cast<std::size_t> (previous)] + (pitches[index] - pitches[static_cast<std::size_t> (previous)]) * (gap - previous) / (key - previous);
        }

        previous = key;
    }

    if (previous < 0)
        return yup::Result::fail ("The tuning maps no key");

    for (int gap = previous + 1; gap < 128; ++gap)
        pitches[static_cast<std::size_t> (gap)] = pitches[static_cast<std::size_t> (previous)] + (gap - previous);

    source = newSource;
    presetIndex = newPresetIndex;
    name = newName;

    {
        const yup::SpinLock::ScopedLockType scopedLock (lock);
        table = newTable;
    }

    generation.fetch_add (1);
    sendChangeMessage();
    return yup::Result::ok();
}

//==============================================================================
void SynthTuning::setBendDown (int range)
{
    bendDown.store (yup::jlimit (0, maxBendRange, range));
    sendChangeMessage();
}

void SynthTuning::setBendUp (int range)
{
    bendUp.store (yup::jlimit (0, maxBendRange, range));
    sendChangeMessage();
}

void SynthTuning::setBendMode (SynthBendMode mode)
{
    bendMode.store (mode);
    sendChangeMessage();
}

//==============================================================================
bool SynthTuning::copyIfChanged (SynthTuningTable& destination, int& seenGeneration) const noexcept
{
    const auto current = generation.load();

    if (current == seenGeneration)
        return false;

    const yup::SpinLock::ScopedTryLockType scopedLock (lock);

    if (! scopedLock.isLocked())
        return false;

    destination = table;
    seenGeneration = current;
    return true;
}

//==============================================================================
yup::DataTree SynthTuning::createTree() const
{
    yup::DataTree tree (treeType);
    auto transaction = tree.beginTransaction();
    transaction.setProperty (nameProperty, name);
    transaction.setProperty (presetProperty, presetIndex);
    transaction.setProperty (scaleProperty, source.scale);
    transaction.setProperty (keyMapProperty, source.keyMap);
    transaction.setProperty (tuningProperty, source.tuning);
    transaction.setProperty (bendDownProperty, getBendDown());
    transaction.setProperty (bendUpProperty, getBendUp());
    transaction.setProperty (bendModeProperty, static_cast<int> (getBendMode()));
    return tree;
}

void SynthTuning::readFrom (const yup::DataTree& tree)
{
    const auto valid = tree.isValid() && tree.getType() == treeType;
    const auto read = [&] (const yup::Identifier& property, const yup::var& fallback)
    {
        return valid ? tree.getProperty (property, fallback) : fallback;
    };

    bendDown.store (yup::jlimit (0, maxBendRange, static_cast<int> (read (bendDownProperty, 2))));
    bendUp.store (yup::jlimit (0, maxBendRange, static_cast<int> (read (bendUpProperty, 2))));
    bendMode.store (static_cast<int> (read (bendModeProperty, 0)) == 1 ? SynthBendMode::steps : SynthBendMode::semitones);

    const Source newSource { read (scaleProperty, {}).toString(), read (keyMapProperty, {}).toString(), read (tuningProperty, {}).toString() };
    const auto newPreset = static_cast<int> (read (presetProperty, 0));
    const auto newName = read (nameProperty, {}).toString();

    if (valid && apply (newSource, yup::isPositiveAndBelow (newPreset, getNumPresets()) ? newPreset : -1, newName.isNotEmpty() ? newName : getPresetName (0)).wasOk())
        return;

    setPreset (0);
}
