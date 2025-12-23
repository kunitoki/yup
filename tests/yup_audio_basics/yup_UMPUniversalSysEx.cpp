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

TEST (UMPUniversalSysExTests, UniversalSysExMessageBasics)
{
    {
        SysEx7 sx { Manufacturer::universalNonRealtime };
        EXPECT_FALSE (isUniversalSysExMessage (sx));
    }

    {
        SysEx7 sx { Manufacturer::moog, { 0x12, 0x34, 0x56 } };
        EXPECT_FALSE (isUniversalSysExMessage (sx));
    }

    {
        SysEx7 sx { Manufacturer::universalNonRealtime, { 0x7f, 0x01 } };
        EXPECT_TRUE (isUniversalSysExMessage (sx));

        const auto view = UniversalSysEx::MessageView { sx };
        EXPECT_EQ (view.getDeviceId(), 0x7f);
        EXPECT_EQ (view.getType(), UniversalSysEx::TypeId::sampleDumpHeader);
        EXPECT_EQ (view.getSubtype(), 0u);
    }
}

TEST (UMPUniversalSysExTests, SetDeviceId)
{
    UniversalSysEx::Message msg { Manufacturer::universalRealtime, { 0x04, 0x01 } };
    EXPECT_TRUE (isUniversalSysExMessage (msg));
    EXPECT_EQ (msg.getDeviceId(), 0x04);

    msg.setDeviceId (9);
    EXPECT_EQ (msg.getDeviceId(), 9);
}

TEST (UMPUniversalSysExTests, IdentityRequestAndReply)
{
    SysEx7 req { Manufacturer::universalNonRealtime, { 0x7f, 0x06, 0x01 } };
    EXPECT_TRUE (UniversalSysEx::isIdentityRequest (req));

    auto built = UniversalSysEx::IdentityRequest {};
    EXPECT_TRUE (UniversalSysEx::isIdentityRequest (built));

    auto reply = UniversalSysEx::IdentityReply { Manufacturer::google, 0x1234, 0x0678, 0x00abcdef };
    EXPECT_TRUE (UniversalSysEx::isIdentityReply (reply));

    const auto view = UniversalSysEx::IdentityReplyView { reply };
    const auto identity = view.getIdentity();
    EXPECT_EQ (identity.manufacturer, Manufacturer::google);
    EXPECT_EQ (identity.family, 0x1234);
    EXPECT_EQ (identity.model, 0x0678);
    EXPECT_EQ (identity.revision, 0x00abcdef);
}
