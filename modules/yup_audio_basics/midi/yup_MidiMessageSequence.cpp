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

MidiMessageSequence::MidiEventHolder::MidiEventHolder (const MidiMessage& mm)
    : message (mm)
{
}

MidiMessageSequence::MidiEventHolder::MidiEventHolder (MidiMessage&& mm)
    : message (std::move (mm))
{
}

//==============================================================================
MidiMessageSequence::MidiMessageSequence()
{
}

MidiMessageSequence::MidiMessageSequence (const MidiMessageSequence& other)
{
    list.addCopiesOf (other.list);

    for (int i = 0; i < list.size(); ++i)
    {
        auto noteOffIndex = other.getIndexOfMatchingKeyUp (i);

        if (noteOffIndex >= 0)
            list.getUnchecked (i)->noteOffObject = list.getUnchecked (noteOffIndex);
    }
}

MidiMessageSequence& MidiMessageSequence::operator= (const MidiMessageSequence& other)
{
    MidiMessageSequence otherCopy (other);
    swapWith (otherCopy);
    return *this;
}

MidiMessageSequence::MidiMessageSequence (MidiMessageSequence&& other) noexcept
    : list (std::move (other.list))
{
}

MidiMessageSequence& MidiMessageSequence::operator= (MidiMessageSequence&& other) noexcept
{
    list = std::move (other.list);
    return *this;
}

void MidiMessageSequence::swapWith (MidiMessageSequence& other) noexcept
{
    list.swapWith (other.list);
}

void MidiMessageSequence::clear()
{
    list.clear();
}

int MidiMessageSequence::getNumEvents() const noexcept
{
    return list.size();
}

MidiMessageSequence::MidiEventHolder* MidiMessageSequence::getEventPointer (int index) const noexcept
{
    return list[index];
}

MidiMessageSequence::MidiEventHolder** MidiMessageSequence::begin() noexcept { return list.begin(); }

MidiMessageSequence::MidiEventHolder* const* MidiMessageSequence::begin() const noexcept { return list.begin(); }

MidiMessageSequence::MidiEventHolder** MidiMessageSequence::end() noexcept { return list.end(); }

MidiMessageSequence::MidiEventHolder* const* MidiMessageSequence::end() const noexcept { return list.end(); }

double MidiMessageSequence::getTimeOfMatchingKeyUp (int index) const noexcept
{
    if (auto* meh = list[index])
        if (auto* noteOff = meh->noteOffObject)
            return noteOff->message.getTimeStamp();

    return 0;
}

int MidiMessageSequence::getIndexOfMatchingKeyUp (int index) const noexcept
{
    if (auto* meh = list[index])
    {
        if (auto* noteOff = meh->noteOffObject)
        {
            for (int i = index; i < list.size(); ++i)
                if (list.getUnchecked (i) == noteOff)
                    return i;

            jassertfalse; // we've somehow got a pointer to a note-off object that isn't in the sequence
        }
    }

    return -1;
}

int MidiMessageSequence::getIndexOf (const MidiEventHolder* event) const noexcept
{
    return list.indexOf (event);
}

int MidiMessageSequence::getNextIndexAtTime (double timeStamp) const noexcept
{
    auto numEvents = list.size();
    int i;

    for (i = 0; i < numEvents; ++i)
        if (list.getUnchecked (i)->message.getTimeStamp() >= timeStamp)
            break;

    return i;
}

//==============================================================================
double MidiMessageSequence::getStartTime() const noexcept
{
    return getEventTime (0);
}

double MidiMessageSequence::getEndTime() const noexcept
{
    return getEventTime (list.size() - 1);
}

double MidiMessageSequence::getEventTime (const int index) const noexcept
{
    if (auto* meh = list[index])
        return meh->message.getTimeStamp();

    return 0;
}

//==============================================================================
MidiMessageSequence::MidiEventHolder* MidiMessageSequence::addEvent (MidiEventHolder* newEvent, double timeAdjustment)
{
    newEvent->message.addToTimeStamp (timeAdjustment);
    auto time = newEvent->message.getTimeStamp();
    int i;

    for (i = list.size(); --i >= 0;)
        if (list.getUnchecked (i)->message.getTimeStamp() <= time)
            break;

    list.insert (i + 1, newEvent);
    return newEvent;
}

MidiMessageSequence::MidiEventHolder* MidiMessageSequence::addEvent (const MidiMessage& newMessage, double timeAdjustment)
{
    return addEvent (new MidiEventHolder (newMessage), timeAdjustment);
}

MidiMessageSequence::MidiEventHolder* MidiMessageSequence::addEvent (MidiMessage&& newMessage, double timeAdjustment)
{
    return addEvent (new MidiEventHolder (std::move (newMessage)), timeAdjustment);
}

void MidiMessageSequence::deleteEvent (int index, bool deleteMatchingNoteUp)
{
    if (isPositiveAndBelow (index, list.size()))
    {
        if (deleteMatchingNoteUp)
            deleteEvent (getIndexOfMatchingKeyUp (index), false);

        list.remove (index);
    }
}

void MidiMessageSequence::addSequence (const MidiMessageSequence& other, double timeAdjustment)
{
    for (auto* m : other)
    {
        auto newOne = new MidiEventHolder (m->message);
        newOne->message.addToTimeStamp (timeAdjustment);
        list.add (newOne);
    }

    sort();
}

void MidiMessageSequence::addSequence (const MidiMessageSequence& other,
                                       double timeAdjustment,
                                       double firstAllowableTime,
                                       double endOfAllowableDestTimes)
{
    for (auto* m : other)
    {
        auto t = m->message.getTimeStamp() + timeAdjustment;

        if (t >= firstAllowableTime && t < endOfAllowableDestTimes)
        {
            auto newOne = new MidiEventHolder (m->message);
            newOne->message.setTimeStamp (t);
            list.add (newOne);
        }
    }

    sort();
}

void MidiMessageSequence::sort() noexcept
{
    std::stable_sort (list.begin(), list.end(), [] (const MidiEventHolder* a, const MidiEventHolder* b)
    {
        return a->message.getTimeStamp() < b->message.getTimeStamp();
    });
}

void MidiMessageSequence::updateMatchedPairs() noexcept
{
    for (int i = 0; i < list.size(); ++i)
    {
        auto* meh = list.getUnchecked (i);
        auto& m1 = meh->message;

        if (m1.isNoteOn())
        {
            meh->noteOffObject = nullptr;
            auto note = m1.getNoteNumber();
            auto chan = m1.getChannel();
            auto len = list.size();

            for (int j = i + 1; j < len; ++j)
            {
                auto* meh2 = list.getUnchecked (j);
                auto& m = meh2->message;

                if (m.getNoteNumber() == note && m.getChannel() == chan)
                {
                    if (m.isNoteOff())
                    {
                        meh->noteOffObject = meh2;
                        break;
                    }

                    if (m.isNoteOn())
                    {
                        auto newEvent = new MidiEventHolder (MidiMessage::noteOff (chan, note));
                        list.insert (j, newEvent);
                        newEvent->message.setTimeStamp (m.getTimeStamp());
                        meh->noteOffObject = newEvent;
                        break;
                    }
                }
            }
        }
    }
}

void MidiMessageSequence::addTimeToMessages (double delta) noexcept
{
    if (! approximatelyEqual (delta, 0.0))
        for (auto* m : list)
            m->message.addToTimeStamp (delta);
}

//==============================================================================
void MidiMessageSequence::extractMidiChannelMessages (const int channelNumberToExtract,
                                                      MidiMessageSequence& destSequence,
                                                      const bool alsoIncludeMetaEvents) const
{
    for (auto* meh : list)
        if (meh->message.isForChannel (channelNumberToExtract)
            || (alsoIncludeMetaEvents && meh->message.isMetaEvent()))
            destSequence.addEvent (meh->message);
}

void MidiMessageSequence::extractSysExMessages (MidiMessageSequence& destSequence) const
{
    for (auto* meh : list)
        if (meh->message.isSysEx())
            destSequence.addEvent (meh->message);
}

void MidiMessageSequence::deleteMidiChannelMessages (const int channelNumberToRemove)
{
    for (int i = list.size(); --i >= 0;)
        if (list.getUnchecked (i)->message.isForChannel (channelNumberToRemove))
            list.remove (i);
}

void MidiMessageSequence::deleteSysExMessages()
{
    for (int i = list.size(); --i >= 0;)
        if (list.getUnchecked (i)->message.isSysEx())
            list.remove (i);
}

//==============================================================================
class OptionalPitchWheel
{
    std::optional<int> value;

public:
    void emit (int channel, Array<MidiMessage>& out) const
    {
        if (value.has_value())
            out.add (MidiMessage::pitchWheel (channel, *value));
    }

    void set (int v)
    {
        value = v;
    }
};

class OptionalControllerValues
{
    std::optional<char> values[128];

public:
    void emit (int channel, Array<MidiMessage>& out) const
    {
        for (auto it = std::begin (values); it != std::end (values); ++it)
            if (it->has_value())
                out.add (MidiMessage::controllerEvent (channel, (int) std::distance (std::begin (values), it), **it));
    }

    void set (int controller, int value)
    {
        values[controller] = (char) value;
    }
};

class OptionalProgramChange
{
    std::optional<char> value, bankLSB, bankMSB;

public:
    void emit (int channel, double time, Array<MidiMessage>& out) const
    {
        if (! value.has_value())
            return;

        if (bankLSB.has_value() && bankMSB.has_value())
        {
            out.add (MidiMessage::controllerEvent (channel, 0x00, *bankMSB).withTimeStamp (time));
            out.add (MidiMessage::controllerEvent (channel, 0x20, *bankLSB).withTimeStamp (time));
        }

        out.add (MidiMessage::programChange (channel, *value).withTimeStamp (time));
    }

    // Returns true if this is a bank number change, and false otherwise.
    bool trySetBank (int controller, int v)
    {
        switch (controller)
        {
            case 0x00:
                bankMSB = (char) v;
                return true;
            case 0x20:
                bankLSB = (char) v;
                return true;
        }

        return false;
    }

    void setProgram (int v) { value = (char) v; }
};

class ParameterNumberState
{
    enum class Kind
    {
        rpn,
        nrpn
    };

    std::optional<char> newestRpnLsb, newestRpnMsb, newestNrpnLsb, newestNrpnMsb, lastSentLsb, lastSentMsb;
    Kind lastSentKind = Kind::rpn, newestKind = Kind::rpn;

public:
    // If the effective parameter number has changed since the last time this function was called,
    // this will emit the current parameter in full (MSB and LSB).
    // This should be called before each data message (entry, increment, decrement: 0x06, 0x26, 0x60, 0x61)
    // to ensure that the data message operates on the correct parameter number.
    void sendIfNecessary (int channel, double time, Array<MidiMessage>& out)
    {
        const auto newestMsb = newestKind == Kind::rpn ? newestRpnMsb : newestNrpnMsb;
        const auto newestLsb = newestKind == Kind::rpn ? newestRpnLsb : newestNrpnLsb;

        auto lastSent = std::tie (lastSentKind, lastSentMsb, lastSentLsb);
        const auto newest = std::tie (newestKind, newestMsb, newestLsb);

        if (lastSent == newest || ! newestMsb.has_value() || ! newestLsb.has_value())
            return;

        out.add (MidiMessage::controllerEvent (channel, newestKind == Kind::rpn ? 0x65 : 0x63, *newestMsb).withTimeStamp (time));
        out.add (MidiMessage::controllerEvent (channel, newestKind == Kind::rpn ? 0x64 : 0x62, *newestLsb).withTimeStamp (time));

        lastSent = newest;
    }

    // Returns true if this is a parameter number change, and false otherwise.
    bool trySetProgramNumber (int controller, int value)
    {
        switch (controller)
        {
            case 0x65:
                newestRpnMsb = (char) value;
                newestKind = Kind::rpn;
                return true;
            case 0x64:
                newestRpnLsb = (char) value;
                newestKind = Kind::rpn;
                return true;
            case 0x63:
                newestNrpnMsb = (char) value;
                newestKind = Kind::nrpn;
                return true;
            case 0x62:
                newestNrpnLsb = (char) value;
                newestKind = Kind::nrpn;
                return true;
        }

        return false;
    }
};

void MidiMessageSequence::createControllerUpdatesForTime (int channel, double time, Array<MidiMessage>& dest)
{
    OptionalProgramChange programChange;
    OptionalControllerValues controllers;
    OptionalPitchWheel pitchWheel;
    ParameterNumberState parameterNumberState;

    for (const auto& item : list)
    {
        const auto& mm = item->message;

        if (! (mm.isForChannel (channel) && mm.getTimeStamp() <= time))
            continue;

        if (mm.isController())
        {
            const auto num = mm.getControllerNumber();

            if (parameterNumberState.trySetProgramNumber (num, mm.getControllerValue()))
                continue;

            if (programChange.trySetBank (num, mm.getControllerValue()))
                continue;

            constexpr int passthroughs[] { 0x06, 0x26, 0x60, 0x61 };

            if (std::find (std::begin (passthroughs), std::end (passthroughs), num) != std::end (passthroughs))
            {
                parameterNumberState.sendIfNecessary (channel, mm.getTimeStamp(), dest);
                dest.add (mm);
            }
            else
            {
                controllers.set (num, mm.getControllerValue());
            }
        }
        else if (mm.isProgramChange())
        {
            programChange.setProgram (mm.getProgramChangeNumber());
        }
        else if (mm.isPitchWheel())
        {
            pitchWheel.set (mm.getPitchWheelValue());
        }
    }

    pitchWheel.emit (channel, dest);
    controllers.emit (channel, dest);

    // Also emits bank change messages if necessary.
    programChange.emit (channel, time, dest);

    // Set the parameter number to its final state.
    parameterNumberState.sendIfNecessary (channel, time, dest);
}

} // namespace yup
