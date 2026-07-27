/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

namespace
{
const uint8 noLSBValueReceived = 0xff;
const Range<int> allChannels { 1, 17 };

template <typename Range, typename Value>
void mpeInstrumentFill (Range& range, const Value& value)
{
    std::fill (std::begin (range), std::end (range), value);
}
} // namespace

//==============================================================================
MPEInstrument::MPEInstrument() noexcept
{
    mpeInstrumentFill (lastPressureLowerBitReceivedOnChannel, noLSBValueReceived);
    mpeInstrumentFill (lastTimbreLowerBitReceivedOnChannel, noLSBValueReceived);
    mpeInstrumentFill (isMemberChannelSustained, false);

    pitchbendDimension.value = &MPENote::pitchbend;
    pressureDimension.value = &MPENote::pressure;
    timbreDimension.value = &MPENote::timbre;

    resetLastReceivedValues();

    legacyMode.channelRange = allChannels;
}

MPEInstrument::MPEInstrument (MPEZoneLayout layout)
    : MPEInstrument()
{
    setZoneLayout (layout);
}

MPEInstrument::~MPEInstrument() = default;

//==============================================================================
MPEZoneLayout MPEInstrument::getZoneLayout() const noexcept
{
    return zoneLayout;
}

void MPEInstrument::resetLastReceivedValues()
{
    struct Defaults
    {
        MPEDimension& dimension;
        MPEValue defaultValue;
    };

    // The default value for pressure is 0, for all other dimensions it is centre
    for (const auto& pair : { Defaults { pressureDimension, MPEValue::minValue() },
                              Defaults { pitchbendDimension, MPEValue::centreValue() },
                              Defaults { timbreDimension, MPEValue::centreValue() } })
    {
        mpeInstrumentFill (pair.dimension.lastValueReceivedOnChannel, pair.defaultValue);
    }
}

void MPEInstrument::setZoneLayout (MPEZoneLayout newLayout)
{
    releaseAllNotes();

    const ScopedLock sl (lock);
    legacyMode.isEnabled = false;

    if (zoneLayout != newLayout)
    {
        zoneLayout = newLayout;
        listeners.call ([=] (Listener& l)
        {
            l.zoneLayoutChanged();
        });
    }
}

//==============================================================================
void MPEInstrument::enableLegacyMode (int pitchbendRange, Range<int> channelRange)
{
    if (legacyMode.isEnabled)
        return;

    releaseAllNotes();

    const ScopedLock sl (lock);

    legacyMode.isEnabled = true;
    legacyMode.pitchbendRange = pitchbendRange;
    legacyMode.channelRange = channelRange;

    zoneLayout.clearAllZones();
    listeners.call ([=] (Listener& l)
    {
        l.zoneLayoutChanged();
    });
}

bool MPEInstrument::isLegacyModeEnabled() const noexcept
{
    return legacyMode.isEnabled;
}

Range<int> MPEInstrument::getLegacyModeChannelRange() const noexcept
{
    return legacyMode.channelRange;
}

void MPEInstrument::setLegacyModeChannelRange (Range<int> channelRange)
{
    jassert (allChannels.contains (channelRange));

    releaseAllNotes();
    const ScopedLock sl (lock);

    if (legacyMode.channelRange != channelRange)
    {
        legacyMode.channelRange = channelRange;
        listeners.call ([=] (Listener& l)
        {
            l.zoneLayoutChanged();
        });
    }
}

int MPEInstrument::getLegacyModePitchbendRange() const noexcept
{
    return legacyMode.pitchbendRange;
}

void MPEInstrument::setLegacyModePitchbendRange (int pitchbendRange)
{
    jassert (pitchbendRange >= 0 && pitchbendRange <= 96);

    releaseAllNotes();
    const ScopedLock sl (lock);

    if (legacyMode.pitchbendRange != pitchbendRange)
    {
        legacyMode.pitchbendRange = pitchbendRange;
        listeners.call ([=] (Listener& l)
        {
            l.zoneLayoutChanged();
        });
    }
}

//==============================================================================
void MPEInstrument::setPressureTrackingMode (TrackingMode modeToUse)
{
    pressureDimension.trackingMode = modeToUse;
}

void MPEInstrument::setPitchbendTrackingMode (TrackingMode modeToUse)
{
    pitchbendDimension.trackingMode = modeToUse;
}

void MPEInstrument::setTimbreTrackingMode (TrackingMode modeToUse)
{
    timbreDimension.trackingMode = modeToUse;
}

//==============================================================================
void MPEInstrument::addListener (Listener* listenerToAdd)
{
    listeners.add (listenerToAdd);
}

void MPEInstrument::removeListener (Listener* listenerToRemove)
{
    listeners.remove (listenerToRemove);
}

//==============================================================================
void MPEInstrument::processNextMidiEvent (const MidiMessage& message)
{
    zoneLayout.processNextMidiEvent (message);

    if (message.isNoteOn (true))
        processMidiNoteOnMessage (message);
    else if (message.isNoteOff (false))
        processMidiNoteOffMessage (message);
    else if (message.isResetAllControllers()
             || message.isAllNotesOff())
        processMidiResetAllControllersMessage (message);
    else if (message.isPitchWheel())
        processMidiPitchWheelMessage (message);
    else if (message.isChannelPressure())
        processMidiChannelPressureMessage (message);
    else if (message.isController())
        processMidiControllerMessage (message);
    else if (message.isAftertouch())
        processMidiAfterTouchMessage (message);
}

//==============================================================================
void MPEInstrument::processMidiNoteOnMessage (const MidiMessage& message)
{
    // Note: If a note-on with velocity = 0 is used to convey a note-off,
    // then the actual note-off velocity is not known. In this case,
    // the MPE convention is to use note-off velocity = 64.

    if (message.getVelocity() == 0)
    {
        noteOff (message.getChannel(),
                 message.getNoteNumber(),
                 MPEValue::from7BitInt (64));
    }
    else
    {
        noteOn (message.getChannel(),
                message.getNoteNumber(),
                MPEValue::from7BitInt (message.getVelocity()));
    }
}

//==============================================================================
void MPEInstrument::processMidiNoteOffMessage (const MidiMessage& message)
{
    noteOff (message.getChannel(),
             message.getNoteNumber(),
             MPEValue::from7BitInt (message.getVelocity()));
}

//==============================================================================
void MPEInstrument::processMidiPitchWheelMessage (const MidiMessage& message)
{
    pitchbend (message.getChannel(),
               MPEValue::from14BitInt (message.getPitchWheelValue()));
}

//==============================================================================
void MPEInstrument::processMidiChannelPressureMessage (const MidiMessage& message)
{
    pressure (message.getChannel(),
              MPEValue::from7BitInt (message.getChannelPressureValue()));
}

//==============================================================================
void MPEInstrument::processMidiControllerMessage (const MidiMessage& message)
{
    switch (message.getControllerNumber())
    {
        case 64:
            sustainPedal (message.getChannel(), message.isSustainPedalOn());
            break;
        case 66:
            sostenutoPedal (message.getChannel(), message.isSostenutoPedalOn());
            break;
        case 70:
            handlePressureMSB (message.getChannel(), message.getControllerValue());
            break;
        case 74:
            handleTimbreMSB (message.getChannel(), message.getControllerValue());
            break;
        case 102:
            handlePressureLSB (message.getChannel(), message.getControllerValue());
            break;
        case 106:
            handleTimbreLSB (message.getChannel(), message.getControllerValue());
            break;
        default:
            break;
    }
}

//==============================================================================
void MPEInstrument::processMidiResetAllControllersMessage (const MidiMessage& message)
{
    // in MPE mode, "reset all controllers" is per-zone and expected on the master channel;
    // in legacy mode, it is per MIDI channel (within the channel range used).

    if (legacyMode.isEnabled && legacyMode.channelRange.contains (message.getChannel()))
    {
        for (int i = notes.size(); --i >= 0;)
        {
            auto& note = notes.getReference (i);

            if (note.midiChannel == message.getChannel())
            {
                note.keyState = MPENote::off;
                note.noteOffVelocity = MPEValue::from7BitInt (64); // some reasonable number
                listeners.call ([&] (Listener& l)
                {
                    l.noteReleased (note);
                });
                notes.remove (i);
            }
        }
    }
    else if (isMasterChannel (message.getChannel()))
    {
        auto zone = (message.getChannel() == 1 ? zoneLayout.getLowerZone()
                                               : zoneLayout.getUpperZone());

        for (int i = notes.size(); --i >= 0;)
        {
            auto& note = notes.getReference (i);

            if (zone.isUsing (note.midiChannel))
            {
                note.keyState = MPENote::off;
                note.noteOffVelocity = MPEValue::from7BitInt (64); // some reasonable number
                listeners.call ([&] (Listener& l)
                {
                    l.noteReleased (note);
                });
                notes.remove (i);
            }
        }
    }
}

void MPEInstrument::processMidiAfterTouchMessage (const MidiMessage& message)
{
    if (! isMasterChannel (message.getChannel()))
        return;

    polyAftertouch (message.getChannel(), message.getNoteNumber(), MPEValue::from7BitInt (message.getAfterTouchValue()));
}

//==============================================================================
void MPEInstrument::handlePressureMSB (int midiChannel, int value) noexcept
{
    auto lsb = lastPressureLowerBitReceivedOnChannel[midiChannel - 1];

    pressure (midiChannel, lsb == noLSBValueReceived ? MPEValue::from7BitInt (value) : MPEValue::from14BitInt (lsb + (value << 7)));
}

void MPEInstrument::handlePressureLSB (int midiChannel, int value) noexcept
{
    lastPressureLowerBitReceivedOnChannel[midiChannel - 1] = uint8 (value);
}

void MPEInstrument::handleTimbreMSB (int midiChannel, int value) noexcept
{
    auto lsb = lastTimbreLowerBitReceivedOnChannel[midiChannel - 1];

    timbre (midiChannel, lsb == noLSBValueReceived ? MPEValue::from7BitInt (value) : MPEValue::from14BitInt (lsb + (value << 7)));
}

void MPEInstrument::handleTimbreLSB (int midiChannel, int value) noexcept
{
    lastTimbreLowerBitReceivedOnChannel[midiChannel - 1] = uint8 (value);
}

//==============================================================================
void MPEInstrument::noteOn (int midiChannel,
                            int midiNoteNumber,
                            MPEValue midiNoteOnVelocity)
{
    if (! isUsingChannel (midiChannel))
        return;

    MPENote newNote (midiChannel,
                     midiNoteNumber,
                     midiNoteOnVelocity,
                     getInitialValueForNewNote (midiChannel, pitchbendDimension),
                     getInitialValueForNewNote (midiChannel, pressureDimension),
                     getInitialValueForNewNote (midiChannel, timbreDimension),
                     isMemberChannelSustained[midiChannel - 1] ? MPENote::keyDownAndSustained : MPENote::keyDown);

    const ScopedLock sl (lock);
    updateNoteTotalPitchbend (newNote);

    if (auto* alreadyPlayingNote = getNotePtr (midiChannel, midiNoteNumber))
    {
        // pathological case: second note-on received for same note -> retrigger it
        alreadyPlayingNote->keyState = MPENote::off;
        alreadyPlayingNote->noteOffVelocity = MPEValue::from7BitInt (64); // some reasonable number
        listeners.call ([=] (Listener& l)
        {
            l.noteReleased (*alreadyPlayingNote);
        });
        notes.remove (alreadyPlayingNote);
    }

    notes.add (newNote);
    listeners.call ([&] (Listener& l)
    {
        l.noteAdded (newNote);
    });
}

//==============================================================================
void MPEInstrument::noteOff (int midiChannel,
                             int midiNoteNumber,
                             MPEValue midiNoteOffVelocity)
{
    const ScopedLock sl (lock);

    if (notes.isEmpty() || ! isUsingChannel (midiChannel))
        return;

    if (auto* note = getNotePtr (midiChannel, midiNoteNumber))
    {
        note->keyState = (note->keyState == MPENote::keyDownAndSustained) ? MPENote::sustained : MPENote::off;
        note->noteOffVelocity = midiNoteOffVelocity;

        // If no more notes are playing on this channel in mpe mode, reset the dimension values
        if (! legacyMode.isEnabled && getLastNotePlayedPtr (midiChannel) == nullptr)
        {
            pressureDimension.lastValueReceivedOnChannel[midiChannel - 1] = MPEValue::minValue();
            pitchbendDimension.lastValueReceivedOnChannel[midiChannel - 1] = MPEValue::centreValue();
            timbreDimension.lastValueReceivedOnChannel[midiChannel - 1] = MPEValue::centreValue();
        }

        if (note->keyState == MPENote::off)
        {
            listeners.call ([=] (Listener& l)
            {
                l.noteReleased (*note);
            });
            notes.remove (note);
        }
        else
        {
            listeners.call ([=] (Listener& l)
            {
                l.noteKeyStateChanged (*note);
            });
        }
    }
}

//==============================================================================
void MPEInstrument::pitchbend (int midiChannel, MPEValue value)
{
    const ScopedLock sl (lock);
    updateDimension (midiChannel, pitchbendDimension, value);
}

void MPEInstrument::pressure (int midiChannel, MPEValue value)
{
    const ScopedLock sl (lock);
    updateDimension (midiChannel, pressureDimension, value);
}

void MPEInstrument::timbre (int midiChannel, MPEValue value)
{
    const ScopedLock sl (lock);
    updateDimension (midiChannel, timbreDimension, value);
}

void MPEInstrument::polyAftertouch (int midiChannel, int midiNoteNumber, MPEValue value)
{
    const ScopedLock sl (lock);

    for (int i = notes.size(); --i >= 0;)
    {
        auto& note = notes.getReference (i);

        if (note.midiChannel == midiChannel
            && note.initialNote == midiNoteNumber
            && pressureDimension.getValue (note) != value)
        {
            pressureDimension.getValue (note) = value;
            callListenersDimensionChanged (note, pressureDimension);
        }
    }
}

MPEValue MPEInstrument::getInitialValueForNewNote (int midiChannel, MPEDimension& dimension) const
{
    if (! legacyMode.isEnabled && getLastNotePlayedPtr (midiChannel) != nullptr)
        return &dimension == &pressureDimension ? MPEValue::minValue() : MPEValue::centreValue();

    return dimension.lastValueReceivedOnChannel[midiChannel - 1];
}

//==============================================================================
void MPEInstrument::updateDimension (int midiChannel, MPEDimension& dimension, MPEValue value)
{
    dimension.lastValueReceivedOnChannel[midiChannel - 1] = value;

    if (notes.isEmpty())
        return;

    if (isMemberChannel (midiChannel))
    {
        if (dimension.trackingMode == allNotesOnChannel)
        {
            for (int i = notes.size(); --i >= 0;)
            {
                auto& note = notes.getReference (i);

                if (note.midiChannel == midiChannel)
                    updateDimensionForNote (note, dimension, value);
            }
        }
        else
        {
            if (auto* note = getNotePtr (midiChannel, dimension.trackingMode))
                updateDimensionForNote (*note, dimension, value);
        }
    }
    else if (isMasterChannel (midiChannel))
    {
        updateDimensionMaster (midiChannel == 1, dimension, value);
    }
}

//==============================================================================
void MPEInstrument::updateDimensionMaster (bool isLowerZone, MPEDimension& dimension, MPEValue value)
{
    auto zone = (isLowerZone ? zoneLayout.getLowerZone()
                             : zoneLayout.getUpperZone());

    if (! zone.isActive())
        return;

    for (int i = notes.size(); --i >= 0;)
    {
        auto& note = notes.getReference (i);

        if (! zone.isUsing (note.midiChannel))
            continue;

        if (&dimension == &pitchbendDimension)
        {
            // master pitchbend is a special case: we don't change the note's own pitchbend,
            // instead we have to update its total (master + note) pitchbend.
            updateNoteTotalPitchbend (note);
            listeners.call ([&] (Listener& l)
            {
                l.notePitchbendChanged (note);
            });
        }
        else if (dimension.getValue (note) != value)
        {
            dimension.getValue (note) = value;
            callListenersDimensionChanged (note, dimension);
        }
    }
}

//==============================================================================
void MPEInstrument::updateDimensionForNote (MPENote& note, MPEDimension& dimension, MPEValue value)
{
    if (dimension.getValue (note) != value)
    {
        dimension.getValue (note) = value;

        if (&dimension == &pitchbendDimension)
            updateNoteTotalPitchbend (note);

        callListenersDimensionChanged (note, dimension);
    }
}

//==============================================================================
void MPEInstrument::callListenersDimensionChanged (const MPENote& note, const MPEDimension& dimension)
{
    if (&dimension == &pressureDimension)
    {
        listeners.call ([&] (Listener& l)
        {
            l.notePressureChanged (note);
        });
        return;
    }
    if (&dimension == &timbreDimension)
    {
        listeners.call ([&] (Listener& l)
        {
            l.noteTimbreChanged (note);
        });
        return;
    }
    if (&dimension == &pitchbendDimension)
    {
        listeners.call ([&] (Listener& l)
        {
            l.notePitchbendChanged (note);
        });
        return;
    }
}

//==============================================================================
void MPEInstrument::updateNoteTotalPitchbend (MPENote& note)
{
    if (legacyMode.isEnabled)
    {
        note.totalPitchbendInSemitones = note.pitchbend.asSignedFloat() * (float) legacyMode.pitchbendRange;
    }
    else
    {
        auto zone = zoneLayout.getLowerZone();

        if (! zone.isActive() || ! zone.isUsing (note.midiChannel))
        {
            auto upperZone = zoneLayout.getUpperZone();

            if (upperZone.isActive() && upperZone.isUsing (note.midiChannel))
            {
                zone = upperZone;
            }
            else
            {
                // this note doesn't belong to any zone!
                jassertfalse;
                return;
            }
        }

        auto notePitchbendInSemitones = 0.0f;

        if (zone.isUsingChannelAsMemberChannel (note.midiChannel))
            notePitchbendInSemitones = note.pitchbend.asSignedFloat() * (float) zone.perNotePitchbendRange;

        auto masterPitchbendInSemitones = pitchbendDimension.lastValueReceivedOnChannel[zone.getMasterChannel() - 1]
                                              .asSignedFloat()
                                        * (float) zone.masterPitchbendRange;

        note.totalPitchbendInSemitones = notePitchbendInSemitones + masterPitchbendInSemitones;
    }
}

//==============================================================================
void MPEInstrument::sustainPedal (int midiChannel, bool isDown)
{
    const ScopedLock sl (lock);
    handleSustainOrSostenuto (midiChannel, isDown, false);
}

void MPEInstrument::sostenutoPedal (int midiChannel, bool isDown)
{
    const ScopedLock sl (lock);
    handleSustainOrSostenuto (midiChannel, isDown, true);
}

//==============================================================================
void MPEInstrument::handleSustainOrSostenuto (int midiChannel, bool isDown, bool isSostenuto)
{
    // in MPE mode, sustain/sostenuto is per-zone and expected on the master channel;
    // in legacy mode, sustain/sostenuto is per MIDI channel (within the channel range used).

    if (legacyMode.isEnabled ? (! legacyMode.channelRange.contains (midiChannel)) : (! isMasterChannel (midiChannel)))
        return;

    auto zone = (midiChannel == 1 ? zoneLayout.getLowerZone()
                                  : zoneLayout.getUpperZone());

    for (int i = notes.size(); --i >= 0;)
    {
        auto& note = notes.getReference (i);

        if (legacyMode.isEnabled ? (note.midiChannel == midiChannel) : zone.isUsing (note.midiChannel))
        {
            if (note.keyState == MPENote::keyDown && isDown)
                note.keyState = MPENote::keyDownAndSustained;
            else if (note.keyState == MPENote::sustained && ! isDown)
                note.keyState = MPENote::off;
            else if (note.keyState == MPENote::keyDownAndSustained && ! isDown)
                note.keyState = MPENote::keyDown;

            if (note.keyState == MPENote::off)
            {
                listeners.call ([&] (Listener& l)
                {
                    l.noteReleased (note);
                });
                notes.remove (i);
            }
            else
            {
                listeners.call ([&] (Listener& l)
                {
                    l.noteKeyStateChanged (note);
                });
            }
        }
    }

    if (! isSostenuto)
    {
        isMemberChannelSustained[midiChannel - 1] = isDown;

        if (! legacyMode.isEnabled)
        {
            if (zone.isLowerZone())
            {
                for (int i = zone.getFirstMemberChannel(); i <= zone.getLastMemberChannel(); ++i)
                    isMemberChannelSustained[i - 1] = isDown;
            }
            else
            {
                for (int i = zone.getFirstMemberChannel(); i >= zone.getLastMemberChannel(); --i)
                    isMemberChannelSustained[i - 1] = isDown;
            }
        }
    }
}

//==============================================================================
bool MPEInstrument::isMemberChannel (int midiChannel) const noexcept
{
    if (legacyMode.isEnabled)
        return legacyMode.channelRange.contains (midiChannel);

    return zoneLayout.getLowerZone().isUsingChannelAsMemberChannel (midiChannel)
        || zoneLayout.getUpperZone().isUsingChannelAsMemberChannel (midiChannel);
}

bool MPEInstrument::isMasterChannel (int midiChannel) const noexcept
{
    if (legacyMode.isEnabled)
        return false;

    const auto lowerZone = zoneLayout.getLowerZone();
    const auto upperZone = zoneLayout.getUpperZone();

    return (lowerZone.isActive() && midiChannel == lowerZone.getMasterChannel())
        || (upperZone.isActive() && midiChannel == upperZone.getMasterChannel());
}

bool MPEInstrument::isUsingChannel (int midiChannel) const noexcept
{
    if (legacyMode.isEnabled)
        return legacyMode.channelRange.contains (midiChannel);

    return zoneLayout.getLowerZone().isUsing (midiChannel)
        || zoneLayout.getUpperZone().isUsing (midiChannel);
}

//==============================================================================
int MPEInstrument::getNumPlayingNotes() const noexcept
{
    return notes.size();
}

MPENote MPEInstrument::getNote (int midiChannel, int midiNoteNumber) const noexcept
{
    if (auto* note = getNotePtr (midiChannel, midiNoteNumber))
        return *note;

    return {};
}

MPENote MPEInstrument::getNote (int index) const noexcept
{
    return notes[index];
}

MPENote MPEInstrument::getNoteWithID (uint16 noteID) const noexcept
{
    const ScopedLock sl (lock);

    for (auto& note : notes)
        if (note.noteID == noteID)
            return note;

    return {};
}

//==============================================================================
MPENote MPEInstrument::getMostRecentNote (int midiChannel) const noexcept
{
    if (auto* note = getLastNotePlayedPtr (midiChannel))
        return *note;

    return {};
}

MPENote MPEInstrument::getMostRecentNoteOtherThan (MPENote otherThanThisNote) const noexcept
{
    for (auto i = notes.size(); --i >= 0;)
    {
        auto& note = notes.getReference (i);

        if (note != otherThanThisNote)
            return note;
    }

    return {};
}

//==============================================================================
const MPENote* MPEInstrument::getNotePtr (int midiChannel, int midiNoteNumber) const noexcept
{
    for (int i = 0; i < notes.size(); ++i)
    {
        auto& note = notes.getReference (i);

        if (note.midiChannel == midiChannel && note.initialNote == midiNoteNumber)
            return &note;
    }

    return nullptr;
}

MPENote* MPEInstrument::getNotePtr (int midiChannel, int midiNoteNumber) noexcept
{
    return const_cast<MPENote*> (static_cast<const MPEInstrument&> (*this).getNotePtr (midiChannel, midiNoteNumber));
}

//==============================================================================
const MPENote* MPEInstrument::getNotePtr (int midiChannel, TrackingMode mode) const noexcept
{
    // for the "all notes" tracking mode, this method can never possibly
    // work because it returns 0 or 1 note but there might be more than one!
    jassert (mode != allNotesOnChannel);

    if (mode == lastNotePlayedOnChannel)
        return getLastNotePlayedPtr (midiChannel);
    if (mode == lowestNoteOnChannel)
        return getLowestNotePtr (midiChannel);
    if (mode == highestNoteOnChannel)
        return getHighestNotePtr (midiChannel);

    return nullptr;
}

MPENote* MPEInstrument::getNotePtr (int midiChannel, TrackingMode mode) noexcept
{
    return const_cast<MPENote*> (static_cast<const MPEInstrument&> (*this).getNotePtr (midiChannel, mode));
}

//==============================================================================
const MPENote* MPEInstrument::getLastNotePlayedPtr (int midiChannel) const noexcept
{
    const ScopedLock sl (lock);

    for (auto i = notes.size(); --i >= 0;)
    {
        auto& note = notes.getReference (i);

        if (note.midiChannel == midiChannel
            && (note.keyState == MPENote::keyDown || note.keyState == MPENote::keyDownAndSustained))
            return &note;
    }

    return nullptr;
}

MPENote* MPEInstrument::getLastNotePlayedPtr (int midiChannel) noexcept
{
    return const_cast<MPENote*> (static_cast<const MPEInstrument&> (*this).getLastNotePlayedPtr (midiChannel));
}

//==============================================================================
const MPENote* MPEInstrument::getHighestNotePtr (int midiChannel) const noexcept
{
    int initialNoteMax = -1;
    const MPENote* result = nullptr;

    for (auto i = notes.size(); --i >= 0;)
    {
        auto& note = notes.getReference (i);

        if (note.midiChannel == midiChannel
            && (note.keyState == MPENote::keyDown || note.keyState == MPENote::keyDownAndSustained)
            && note.initialNote > initialNoteMax)
        {
            result = &note;
            initialNoteMax = note.initialNote;
        }
    }

    return result;
}

MPENote* MPEInstrument::getHighestNotePtr (int midiChannel) noexcept
{
    return const_cast<MPENote*> (static_cast<const MPEInstrument&> (*this).getHighestNotePtr (midiChannel));
}

const MPENote* MPEInstrument::getLowestNotePtr (int midiChannel) const noexcept
{
    int initialNoteMin = 128;
    const MPENote* result = nullptr;

    for (auto i = notes.size(); --i >= 0;)
    {
        auto& note = notes.getReference (i);

        if (note.midiChannel == midiChannel
            && (note.keyState == MPENote::keyDown || note.keyState == MPENote::keyDownAndSustained)
            && note.initialNote < initialNoteMin)
        {
            result = &note;
            initialNoteMin = note.initialNote;
        }
    }

    return result;
}

MPENote* MPEInstrument::getLowestNotePtr (int midiChannel) noexcept
{
    return const_cast<MPENote*> (static_cast<const MPEInstrument&> (*this).getLowestNotePtr (midiChannel));
}

//==============================================================================
void MPEInstrument::releaseAllNotes()
{
    const ScopedLock sl (lock);

    for (auto i = notes.size(); --i >= 0;)
    {
        auto& note = notes.getReference (i);
        note.keyState = MPENote::off;
        note.noteOffVelocity = MPEValue::from7BitInt (64); // some reasonable number
        listeners.call ([&] (Listener& l)
        {
            l.noteReleased (note);
        });
    }

    notes.clear();
}

//==============================================================================
void MPEInstrument::Listener::noteAdded ([[maybe_unused]] MPENote newNote) {}

void MPEInstrument::Listener::notePressureChanged ([[maybe_unused]] MPENote changedNote) {}

void MPEInstrument::Listener::notePitchbendChanged ([[maybe_unused]] MPENote changedNote) {}

void MPEInstrument::Listener::noteTimbreChanged ([[maybe_unused]] MPENote changedNote) {}

void MPEInstrument::Listener::noteKeyStateChanged ([[maybe_unused]] MPENote changedNote) {}

void MPEInstrument::Listener::noteReleased ([[maybe_unused]] MPENote finishedNote) {}

void MPEInstrument::Listener::zoneLayoutChanged() {}

} // namespace yup
