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

MidiBuffer MPEMessages::setLowerZone (int numMemberChannels, int perNotePitchbendRange, int masterPitchbendRange)
{
    auto buffer = MidiRPNGenerator::generate (1, zoneLayoutMessagesRpnNumber, numMemberChannels, false, false);

    buffer.addEvents (setLowerZonePerNotePitchbendRange (perNotePitchbendRange), 0, -1, 0);
    buffer.addEvents (setLowerZoneMasterPitchbendRange (masterPitchbendRange), 0, -1, 0);

    return buffer;
}

MidiBuffer MPEMessages::setUpperZone (int numMemberChannels, int perNotePitchbendRange, int masterPitchbendRange)
{
    auto buffer = MidiRPNGenerator::generate (16, zoneLayoutMessagesRpnNumber, numMemberChannels, false, false);

    buffer.addEvents (setUpperZonePerNotePitchbendRange (perNotePitchbendRange), 0, -1, 0);
    buffer.addEvents (setUpperZoneMasterPitchbendRange (masterPitchbendRange), 0, -1, 0);

    return buffer;
}

MidiBuffer MPEMessages::setLowerZonePerNotePitchbendRange (int perNotePitchbendRange)
{
    return MidiRPNGenerator::generate (2, 0, perNotePitchbendRange, false, false);
}

MidiBuffer MPEMessages::setUpperZonePerNotePitchbendRange (int perNotePitchbendRange)
{
    return MidiRPNGenerator::generate (15, 0, perNotePitchbendRange, false, false);
}

MidiBuffer MPEMessages::setLowerZoneMasterPitchbendRange (int masterPitchbendRange)
{
    return MidiRPNGenerator::generate (1, 0, masterPitchbendRange, false, false);
}

MidiBuffer MPEMessages::setUpperZoneMasterPitchbendRange (int masterPitchbendRange)
{
    return MidiRPNGenerator::generate (16, 0, masterPitchbendRange, false, false);
}

MidiBuffer MPEMessages::clearLowerZone()
{
    return MidiRPNGenerator::generate (1, zoneLayoutMessagesRpnNumber, 0, false, false);
}

MidiBuffer MPEMessages::clearUpperZone()
{
    return MidiRPNGenerator::generate (16, zoneLayoutMessagesRpnNumber, 0, false, false);
}

MidiBuffer MPEMessages::clearAllZones()
{
    MidiBuffer buffer;

    buffer.addEvents (clearLowerZone(), 0, -1, 0);
    buffer.addEvents (clearUpperZone(), 0, -1, 0);

    return buffer;
}

MidiBuffer MPEMessages::setZoneLayout (MPEZoneLayout layout)
{
    MidiBuffer buffer;

    buffer.addEvents (clearAllZones(), 0, -1, 0);

    auto lowerZone = layout.getLowerZone();
    if (lowerZone.isActive())
        buffer.addEvents (setLowerZone (lowerZone.numMemberChannels,
                                        lowerZone.perNotePitchbendRange,
                                        lowerZone.masterPitchbendRange),
                          0,
                          -1,
                          0);

    auto upperZone = layout.getUpperZone();
    if (upperZone.isActive())
        buffer.addEvents (setUpperZone (upperZone.numMemberChannels,
                                        upperZone.perNotePitchbendRange,
                                        upperZone.masterPitchbendRange),
                          0,
                          -1,
                          0);

    return buffer;
}

} // namespace yup
