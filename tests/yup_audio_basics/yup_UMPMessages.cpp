/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#include <yup_audio_basics/yup_audio_basics.h>

#include <gtest/gtest.h>

using namespace yup;
using namespace yup::ump;

TEST (UMPMessagesTests, UtilityMessageConstructors)
{
    {
        UtilityMessage m;

        EXPECT_EQ (m.data[0], 0u);
        EXPECT_EQ (m.data[1], 0u);
        EXPECT_EQ (m.data[2], 0u);
        EXPECT_EQ (m.data[3], 0u);
        EXPECT_EQ (m.getType(), PacketType::utility);
        EXPECT_EQ (m.getStatus(), 0u);
        EXPECT_EQ (m.getSize(), 1u);
    }

    {
        UtilityMessage m { Status (UtilityStatus::jitterClock) };

        EXPECT_EQ (m.data[0], 0x00100000u);
        EXPECT_EQ (m.data[1], 0u);
        EXPECT_EQ (m.data[2], 0u);
        EXPECT_EQ (m.data[3], 0u);
        EXPECT_EQ (m.getType(), PacketType::utility);
        EXPECT_EQ (m.getStatus(), Status (UtilityStatus::jitterClock));
        EXPECT_EQ (m.getSize(), 1u);
    }

    {
        UtilityMessage m { Status (UtilityStatus::jitterTimestamp), 0xabcd };

        EXPECT_EQ (m.data[0], 0x0020abcd);
        EXPECT_EQ (m.data[1], 0u);
        EXPECT_EQ (m.data[2], 0u);
        EXPECT_EQ (m.data[3], 0u);
        EXPECT_EQ (m.getType(), PacketType::utility);
        EXPECT_EQ (m.getStatus(), Status (UtilityStatus::jitterTimestamp));
        EXPECT_EQ (m.getSize(), 1u);
    }
}

TEST (UMPMessagesTests, UtilityMessageView)
{
    {
        UtilityMessage m {};
        UtilityMessageView view { m };

        EXPECT_EQ (view.getStatus(), Status (UtilityStatus::noop));
        EXPECT_EQ (view.getPayload(), 0u);
    }

    {
        UtilityMessage m { Status (UtilityStatus::jitterTimestamp), 0xabcd };
        UtilityMessageView view { m };

        EXPECT_EQ (view.getStatus(), Status (UtilityStatus::jitterTimestamp));
        EXPECT_EQ (view.getPayload(), 0xabcd);
    }
}

TEST (UMPMessagesTests, MakeUtilityMessage)
{
    {
        const auto m = makeUtilityMessage (Status (UtilityStatus::jitterClock), 0xf499);
        EXPECT_EQ (m.data[0], 0x0010f499u);

        const UtilityMessageView view { m };
        EXPECT_EQ (view.getStatus(), Status (UtilityStatus::jitterClock));
        EXPECT_EQ (view.getPayload(), 0xf499);
    }

    {
        const auto m = makeUtilityMessage (Status (UtilityStatus::jitterTimestamp), 0x17cc);
        EXPECT_EQ (m.data[0], 0x002017ccu);

        const UtilityMessageView view { m };
        EXPECT_EQ (view.getStatus(), Status (UtilityStatus::jitterTimestamp));
        EXPECT_EQ (view.getPayload(), 0x17ccu);
    }
}

TEST (UMPMessagesTests, SystemMessageConstructors)
{
    {
        SystemMessage m;

        EXPECT_TRUE (isSystemMessage (m));
        EXPECT_EQ (m.data[0], 0x10000000u);
        EXPECT_EQ (m.data[1], 0u);
        EXPECT_EQ (m.data[2], 0u);
        EXPECT_EQ (m.data[3], 0u);
        EXPECT_EQ (m.getType(), PacketType::system);
        EXPECT_EQ (m.getStatus(), 0u);
        EXPECT_EQ (m.getGroup(), 0u);
        EXPECT_EQ (m.getSize(), 1u);
    }

    {
        SystemMessage m { 4, Status (SystemStatus::clock) };

        EXPECT_TRUE (isSystemMessage (m));
        EXPECT_EQ (m.data[0], 0x14f80000u);
        EXPECT_EQ (m.data[1], 0u);
        EXPECT_EQ (m.data[2], 0u);
        EXPECT_EQ (m.data[3], 0u);
        EXPECT_EQ (m.getStatus(), Status (SystemStatus::clock));
        EXPECT_EQ (m.getGroup(), 4u);
        EXPECT_EQ (m.getSize(), 1u);
    }

    {
        SystemMessage m { 9, Status (SystemStatus::mtcQuarterFrame), 0x46 };

        EXPECT_TRUE (isSystemMessage (m));
        EXPECT_EQ (m.data[0], 0x19f14600u);
        EXPECT_EQ (m.data[1], 0u);
        EXPECT_EQ (m.data[2], 0u);
        EXPECT_EQ (m.data[3], 0u);
        EXPECT_EQ (m.getStatus(), Status (SystemStatus::mtcQuarterFrame));
        EXPECT_EQ (m.getGroup(), 9u);
        EXPECT_EQ (m.getSize(), 1u);
    }

    {
        SystemMessage m { 12, Status (SystemStatus::songSelect), 66 };

        EXPECT_TRUE (isSystemMessage (m));
        EXPECT_EQ (m.data[0], 0x1cf34200u);
        EXPECT_EQ (m.data[1], 0u);
        EXPECT_EQ (m.data[2], 0u);
        EXPECT_EQ (m.data[3], 0u);
        EXPECT_EQ (m.getStatus(), Status (SystemStatus::songSelect));
        EXPECT_EQ (m.getGroup(), 12u);
        EXPECT_EQ (m.getSize(), 1u);
    }
}

TEST (UMPMessagesTests, SystemMessageView)
{
    {
        SystemMessage m {};
        EXPECT_TRUE (isSystemMessage (m));
        EXPECT_TRUE (asSystemMessageView (m));

        const auto view = SystemMessageView { m };
        EXPECT_EQ (view.getGroup(), 0u);
        EXPECT_EQ (view.getStatus(), 0u);
        EXPECT_EQ (view.getDataByte1(), 0u);
        EXPECT_EQ (view.getDataByte2(), 0u);
        EXPECT_EQ (view.getSongPosition(), 0u);
    }

    {
        const auto m = SystemMessage { 5, Status (SystemStatus::clock) };
        EXPECT_TRUE (isSystemMessage (m));
        EXPECT_TRUE (asSystemMessageView (m));

        const auto view = SystemMessageView { m };
        EXPECT_EQ (view.getGroup(), 5u);
        EXPECT_EQ (view.getStatus(), Status (SystemStatus::clock));
        EXPECT_EQ (view.getDataByte1(), 0u);
        EXPECT_EQ (view.getDataByte2(), 0u);
        EXPECT_EQ (view.getSongPosition(), 0u);
    }

    {
        const auto m = SystemMessage { 11, Status (SystemStatus::mtcQuarterFrame), 0x46 };
        EXPECT_TRUE (isSystemMessage (m));
        EXPECT_TRUE (asSystemMessageView (m));

        const auto view = SystemMessageView { m };
        EXPECT_EQ (view.getGroup(), 11u);
        EXPECT_EQ (view.getStatus(), Status (SystemStatus::mtcQuarterFrame));
        EXPECT_EQ (view.getDataByte1(), 0x46u);
        EXPECT_EQ (view.getDataByte2(), 0u);
        EXPECT_EQ (view.getSongPosition(), 0u);
    }

    {
        SystemMessage m { 12, Status (SystemStatus::songSelect), 66 };
        EXPECT_TRUE (isSystemMessage (m));
        EXPECT_TRUE (asSystemMessageView (m));

        auto view = SystemMessageView { m };
        EXPECT_EQ (view.getGroup(), 12u);
        EXPECT_EQ (view.getStatus(), Status (SystemStatus::songSelect));
        EXPECT_EQ (view.getDataByte1(), 66u);
        EXPECT_EQ (view.getDataByte2(), 0u);
        EXPECT_EQ (view.getSongPosition(), 0u);
    }

    {
        SystemMessage m { 3, Status (SystemStatus::songPosition), 0x34u, 0x24u };
        EXPECT_TRUE (isSystemMessage (m));
        EXPECT_TRUE (asSystemMessageView (m));

        const auto view = SystemMessageView { m };
        EXPECT_EQ (view.getGroup(), 3u);
        EXPECT_EQ (view.getStatus(), Status (SystemStatus::songPosition));
        EXPECT_EQ (view.getDataByte1(), 0x34u);
        EXPECT_EQ (view.getDataByte2(), 0x24u);
        EXPECT_EQ (view.getSongPosition(), 0x1234u);
    }

    EXPECT_FALSE (asSystemMessageView (UniversalPacket { 0x21112233u }));
}

TEST (UMPMessagesTests, MakeSystemMessage)
{
    {
        const auto m = makeSystemMessage (9, Status (SystemStatus::songPosition), 0x74, 0x69);
        EXPECT_EQ (m.data[0], 0x19f27469u);
        EXPECT_TRUE (isSystemMessage (m));
        EXPECT_TRUE (asSystemMessageView (m));

        const SystemMessageView view { m };
        EXPECT_EQ (view.getGroup(), 9u);
        EXPECT_EQ (view.getStatus(), Status (SystemStatus::songPosition));
        EXPECT_EQ (view.getDataByte1(), 0x74u);
        EXPECT_EQ (view.getDataByte2(), 0x69u);
        EXPECT_EQ (view.getSongPosition(), 0x34f4u);
    }

    {
        const auto m = makeSystemMessage (4, Status (SystemStatus::mtcQuarterFrame), 0x43);
        EXPECT_EQ (m.data[0], 0x14f14300u);
        EXPECT_TRUE (isSystemMessage (m));
        EXPECT_TRUE (asSystemMessageView (m));

        const auto view = SystemMessageView { m };
        EXPECT_EQ (view.getGroup(), 4u);
        EXPECT_EQ (view.getStatus(), Status (SystemStatus::mtcQuarterFrame));
        EXPECT_EQ (view.getDataByte1(), 0x43u);
        EXPECT_EQ (view.getDataByte2(), 0u);
        EXPECT_EQ (view.getSongPosition(), 0u);
    }

    {
        const auto m = makeSystemMessage (12, Status (SystemStatus::clock));
        EXPECT_EQ (m.data[0], 0x1cf80000u);
        EXPECT_TRUE (isSystemMessage (m));
        EXPECT_TRUE (asSystemMessageView (m));

        const auto view = SystemMessageView { m };
        EXPECT_EQ (view.getGroup(), 12u);
        EXPECT_EQ (view.getStatus(), Status (SystemStatus::clock));
        EXPECT_EQ (view.getDataByte1(), 0u);
        EXPECT_EQ (view.getDataByte2(), 0u);
        EXPECT_EQ (view.getSongPosition(), 0u);
    }
}

TEST (UMPMessagesTests, MakeSongPositionMessage)
{
    {
        const auto m = makeSongPositionMessage (3, 0x1234);
        EXPECT_EQ (m.data[0], 0x13f23424u);
        EXPECT_TRUE (isSystemMessage (m));
        EXPECT_TRUE (asSystemMessageView (m));

        const auto view = SystemMessageView { m };
        EXPECT_EQ (view.getGroup(), 3u);
        EXPECT_EQ (view.getStatus(), Status (SystemStatus::songPosition));
        EXPECT_EQ (view.getDataByte1(), 0x34u);
        EXPECT_EQ (view.getDataByte2(), 0x24u);
        EXPECT_EQ (view.getSongPosition(), 0x1234u);
    }

    {
        const auto m = makeSongPositionMessage (13, 0xfafa);
        EXPECT_EQ (m.data[0], 0x1df27a75u);
        EXPECT_TRUE (isSystemMessage (m));
        EXPECT_TRUE (asSystemMessageView (m));

        const auto view = SystemMessageView { m };
        EXPECT_EQ (view.getGroup(), 13u);
        EXPECT_EQ (view.getStatus(), Status (SystemStatus::songPosition));
        EXPECT_EQ (view.getDataByte1(), 0x7au);
        EXPECT_EQ (view.getDataByte2(), 0x75u);
        EXPECT_EQ (view.getSongPosition(), 0x3afau);
    }
}
