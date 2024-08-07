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

namespace juce
{

MPEZoneLayout::MPEZoneLayout (MPEZone lower, MPEZone upper)
    : lowerZone (lower)
    , upperZone (upper)
{
}

MPEZoneLayout::MPEZoneLayout (MPEZone zone)
    : lowerZone (zone.isLowerZone() ? zone : MPEZone())
    , upperZone (! zone.isLowerZone() ? zone : MPEZone())
{
}

MPEZoneLayout::MPEZoneLayout (const MPEZoneLayout& other)
    : lowerZone (other.lowerZone)
    , upperZone (other.upperZone)
{
}

MPEZoneLayout& MPEZoneLayout::operator= (const MPEZoneLayout& other)
{
    lowerZone = other.lowerZone;
    upperZone = other.upperZone;

    sendLayoutChangeMessage();

    return *this;
}

void MPEZoneLayout::sendLayoutChangeMessage()
{
    listeners.call ([this] (Listener& l)
                    {
                        l.zoneLayoutChanged (*this);
                    });
}

//==============================================================================
void MPEZoneLayout::setZone (bool isLower, int numMemberChannels, int perNotePitchbendRange, int masterPitchbendRange) noexcept
{
    checkAndLimitZoneParameters (0, 15, numMemberChannels);
    checkAndLimitZoneParameters (0, 96, perNotePitchbendRange);
    checkAndLimitZoneParameters (0, 96, masterPitchbendRange);

    if (isLower)
        lowerZone = { MPEZone::Type::lower, numMemberChannels, perNotePitchbendRange, masterPitchbendRange };
    else
        upperZone = { MPEZone::Type::upper, numMemberChannels, perNotePitchbendRange, masterPitchbendRange };

    if (numMemberChannels > 0)
    {
        auto totalChannels = lowerZone.numMemberChannels + upperZone.numMemberChannels;

        if (totalChannels >= 15)
        {
            if (isLower)
                upperZone.numMemberChannels = 14 - numMemberChannels;
            else
                lowerZone.numMemberChannels = 14 - numMemberChannels;
        }
    }

    sendLayoutChangeMessage();
}

void MPEZoneLayout::setLowerZone (int numMemberChannels, int perNotePitchbendRange, int masterPitchbendRange) noexcept
{
    setZone (true, numMemberChannels, perNotePitchbendRange, masterPitchbendRange);
}

void MPEZoneLayout::setUpperZone (int numMemberChannels, int perNotePitchbendRange, int masterPitchbendRange) noexcept
{
    setZone (false, numMemberChannels, perNotePitchbendRange, masterPitchbendRange);
}

void MPEZoneLayout::clearAllZones()
{
    lowerZone = { MPEZone::Type::lower, 0 };
    upperZone = { MPEZone::Type::upper, 0 };

    sendLayoutChangeMessage();
}

//==============================================================================
void MPEZoneLayout::processNextMidiEvent (const MidiMessage& message)
{
    if (! message.isController())
        return;

    if (auto parsed = rpnDetector.tryParse (message.getChannel(),
                                            message.getControllerNumber(),
                                            message.getControllerValue()))
    {
        processRpnMessage (*parsed);
    }
}

void MPEZoneLayout::processRpnMessage (MidiRPNMessage rpn)
{
    if (rpn.parameterNumber == MPEMessages::zoneLayoutMessagesRpnNumber)
        processZoneLayoutRpnMessage (rpn);
    else if (rpn.parameterNumber == 0)
        processPitchbendRangeRpnMessage (rpn);
}

void MPEZoneLayout::processZoneLayoutRpnMessage (MidiRPNMessage rpn)
{
    if (rpn.value < 16)
    {
        if (rpn.channel == 1)
            setLowerZone (rpn.value);
        else if (rpn.channel == 16)
            setUpperZone (rpn.value);
    }
}

void MPEZoneLayout::updateMasterPitchbend (MPEZone& zone, int value)
{
    if (zone.masterPitchbendRange != value)
    {
        checkAndLimitZoneParameters (0, 96, zone.masterPitchbendRange);
        zone.masterPitchbendRange = value;
        sendLayoutChangeMessage();
    }
}

void MPEZoneLayout::updatePerNotePitchbendRange (MPEZone& zone, int value)
{
    if (zone.perNotePitchbendRange != value)
    {
        checkAndLimitZoneParameters (0, 96, zone.perNotePitchbendRange);
        zone.perNotePitchbendRange = value;
        sendLayoutChangeMessage();
    }
}

void MPEZoneLayout::processPitchbendRangeRpnMessage (MidiRPNMessage rpn)
{
    if (rpn.channel == 1)
    {
        updateMasterPitchbend (lowerZone, rpn.value);
    }
    else if (rpn.channel == 16)
    {
        updateMasterPitchbend (upperZone, rpn.value);
    }
    else
    {
        if (lowerZone.isUsingChannelAsMemberChannel (rpn.channel))
            updatePerNotePitchbendRange (lowerZone, rpn.value);
        else if (upperZone.isUsingChannelAsMemberChannel (rpn.channel))
            updatePerNotePitchbendRange (upperZone, rpn.value);
    }
}

void MPEZoneLayout::processNextMidiBuffer (const MidiBuffer& buffer)
{
    for (const auto metadata : buffer)
        processNextMidiEvent (metadata.getMessage());
}

//==============================================================================
void MPEZoneLayout::addListener (Listener* const listenerToAdd) noexcept
{
    listeners.add (listenerToAdd);
}

void MPEZoneLayout::removeListener (Listener* const listenerToRemove) noexcept
{
    listeners.remove (listenerToRemove);
}

//==============================================================================
void MPEZoneLayout::checkAndLimitZoneParameters (int minValue, int maxValue, int& valueToCheckAndLimit) noexcept
{
    if (valueToCheckAndLimit < minValue || valueToCheckAndLimit > maxValue)
    {
        // if you hit this, one of the parameters you supplied for this zone
        // was not within the allowed range!
        // we fit this back into the allowed range here to maintain a valid
        // state for the zone, but probably the resulting zone is not what you
        // wanted it to be!
        jassertfalse;

        valueToCheckAndLimit = jlimit (minValue, maxValue, valueToCheckAndLimit);
    }
}

//==============================================================================
//==============================================================================
#if JUCE_UNIT_TESTS

class MPEZoneLayoutTests final : public UnitTest
{
public:
    MPEZoneLayoutTests()
        : UnitTest ("MPEZoneLayout class", UnitTestCategories::midi)
    {
    }

    void runTest() override
    {
        beginTest ("initialisation");
        {
            MPEZoneLayout layout;
            expect (! layout.getLowerZone().isActive());
            expect (! layout.getUpperZone().isActive());
        }

        beginTest ("adding zones");
        {
            MPEZoneLayout layout;

            layout.setLowerZone (7);

            expect (layout.getLowerZone().isActive());
            expect (! layout.getUpperZone().isActive());
            expectEquals (layout.getLowerZone().getMasterChannel(), 1);
            expectEquals (layout.getLowerZone().numMemberChannels, 7);

            layout.setUpperZone (7);

            expect (layout.getLowerZone().isActive());
            expect (layout.getUpperZone().isActive());
            expectEquals (layout.getLowerZone().getMasterChannel(), 1);
            expectEquals (layout.getLowerZone().numMemberChannels, 7);
            expectEquals (layout.getUpperZone().getMasterChannel(), 16);
            expectEquals (layout.getUpperZone().numMemberChannels, 7);

            layout.setLowerZone (3);

            expect (layout.getLowerZone().isActive());
            expect (layout.getUpperZone().isActive());
            expectEquals (layout.getLowerZone().getMasterChannel(), 1);
            expectEquals (layout.getLowerZone().numMemberChannels, 3);
            expectEquals (layout.getUpperZone().getMasterChannel(), 16);
            expectEquals (layout.getUpperZone().numMemberChannels, 7);

            layout.setUpperZone (3);

            expect (layout.getLowerZone().isActive());
            expect (layout.getUpperZone().isActive());
            expectEquals (layout.getLowerZone().getMasterChannel(), 1);
            expectEquals (layout.getLowerZone().numMemberChannels, 3);
            expectEquals (layout.getUpperZone().getMasterChannel(), 16);
            expectEquals (layout.getUpperZone().numMemberChannels, 3);

            layout.setLowerZone (15);

            expect (layout.getLowerZone().isActive());
            expect (! layout.getUpperZone().isActive());
            expectEquals (layout.getLowerZone().getMasterChannel(), 1);
            expectEquals (layout.getLowerZone().numMemberChannels, 15);
        }

        beginTest ("clear all zones");
        {
            MPEZoneLayout layout;

            expect (! layout.getLowerZone().isActive());
            expect (! layout.getUpperZone().isActive());

            layout.setLowerZone (7);
            layout.setUpperZone (2);

            expect (layout.getLowerZone().isActive());
            expect (layout.getUpperZone().isActive());

            layout.clearAllZones();

            expect (! layout.getLowerZone().isActive());
            expect (! layout.getUpperZone().isActive());
        }

        beginTest ("process MIDI buffers");
        {
            MPEZoneLayout layout;
            MidiBuffer buffer;

            buffer = MPEMessages::setLowerZone (7);
            layout.processNextMidiBuffer (buffer);

            expect (layout.getLowerZone().isActive());
            expect (! layout.getUpperZone().isActive());
            expectEquals (layout.getLowerZone().getMasterChannel(), 1);
            expectEquals (layout.getLowerZone().numMemberChannels, 7);

            buffer = MPEMessages::setUpperZone (7);
            layout.processNextMidiBuffer (buffer);

            expect (layout.getLowerZone().isActive());
            expect (layout.getUpperZone().isActive());
            expectEquals (layout.getLowerZone().getMasterChannel(), 1);
            expectEquals (layout.getLowerZone().numMemberChannels, 7);
            expectEquals (layout.getUpperZone().getMasterChannel(), 16);
            expectEquals (layout.getUpperZone().numMemberChannels, 7);

            {
                buffer = MPEMessages::setLowerZone (10);
                layout.processNextMidiBuffer (buffer);

                expect (layout.getLowerZone().isActive());
                expect (layout.getUpperZone().isActive());
                expectEquals (layout.getLowerZone().getMasterChannel(), 1);
                expectEquals (layout.getLowerZone().numMemberChannels, 10);
                expectEquals (layout.getUpperZone().getMasterChannel(), 16);
                expectEquals (layout.getUpperZone().numMemberChannels, 4);

                buffer = MPEMessages::setLowerZone (10, 33, 44);
                layout.processNextMidiBuffer (buffer);

                expectEquals (layout.getLowerZone().numMemberChannels, 10);
                expectEquals (layout.getLowerZone().perNotePitchbendRange, 33);
                expectEquals (layout.getLowerZone().masterPitchbendRange, 44);
            }

            {
                buffer = MPEMessages::setUpperZone (10);
                layout.processNextMidiBuffer (buffer);

                expect (layout.getLowerZone().isActive());
                expect (layout.getUpperZone().isActive());
                expectEquals (layout.getLowerZone().getMasterChannel(), 1);
                expectEquals (layout.getLowerZone().numMemberChannels, 4);
                expectEquals (layout.getUpperZone().getMasterChannel(), 16);
                expectEquals (layout.getUpperZone().numMemberChannels, 10);

                buffer = MPEMessages::setUpperZone (10, 33, 44);

                layout.processNextMidiBuffer (buffer);

                expectEquals (layout.getUpperZone().numMemberChannels, 10);
                expectEquals (layout.getUpperZone().perNotePitchbendRange, 33);
                expectEquals (layout.getUpperZone().masterPitchbendRange, 44);
            }

            buffer = MPEMessages::clearAllZones();
            layout.processNextMidiBuffer (buffer);

            expect (! layout.getLowerZone().isActive());
            expect (! layout.getUpperZone().isActive());
        }

        beginTest ("process individual MIDI messages");
        {
            MPEZoneLayout layout;

            layout.processNextMidiEvent ({ 0x80, 0x59, 0xd0 }); // unrelated note-off msg
            layout.processNextMidiEvent ({ 0xb0, 0x64, 0x06 }); // RPN part 1
            layout.processNextMidiEvent ({ 0xb0, 0x65, 0x00 }); // RPN part 2
            layout.processNextMidiEvent ({ 0xb8, 0x0b, 0x66 }); // unrelated CC msg
            layout.processNextMidiEvent ({ 0xb0, 0x06, 0x03 }); // RPN part 3
            layout.processNextMidiEvent ({ 0x90, 0x60, 0x00 }); // unrelated note-on msg

            expect (layout.getLowerZone().isActive());
            expect (! layout.getUpperZone().isActive());
            expectEquals (layout.getLowerZone().getMasterChannel(), 1);
            expectEquals (layout.getLowerZone().numMemberChannels, 3);
            expectEquals (layout.getLowerZone().perNotePitchbendRange, 48);
            expectEquals (layout.getLowerZone().masterPitchbendRange, 2);

            const auto masterPitchBend = 0x0c;
            layout.processNextMidiEvent ({ 0xb0, 0x64, 0x00 });
            layout.processNextMidiEvent ({ 0xb0, 0x06, masterPitchBend });

            expectEquals (layout.getLowerZone().masterPitchbendRange, masterPitchBend);

            const auto newPitchBend = 0x0d;
            layout.processNextMidiEvent ({ 0xb0, 0x06, newPitchBend });

            expectEquals (layout.getLowerZone().masterPitchbendRange, newPitchBend);
        }
    }
};

static MPEZoneLayoutTests MPEZoneLayoutUnitTests;

#endif

} // namespace juce
