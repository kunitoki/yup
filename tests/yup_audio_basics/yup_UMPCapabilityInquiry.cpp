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
#include <string>
#include <string_view>

using namespace yup;
using namespace yup::ump;
using namespace yup::ump::ci;

TEST (UMPCapabilityInquiryProfileTests, ProfileIdEquality)
{
    ProfileId a;
    EXPECT_EQ (a.byte1, 0x7e);
    EXPECT_EQ (a.byte2, 0x00);
    EXPECT_EQ (a.byte3, 0x00);
    EXPECT_EQ (a.byte4, 0x00);
    EXPECT_EQ (a.byte5, 0x00);

    ProfileId b { 0x7e, 0x02, 0x03, 0x04, 0x05 };
    ProfileId c { 0x00, 0x21, 0x09, 0x7f, 0x01 };
    ProfileId d { b };

    EXPECT_TRUE (a == a);
    EXPECT_FALSE (a == b);
    EXPECT_FALSE (a == c);
    EXPECT_FALSE (a == d);

    EXPECT_FALSE (b == a);
    EXPECT_TRUE (b == b);
    EXPECT_FALSE (b == c);
    EXPECT_TRUE (b == d);

    EXPECT_FALSE (c == a);
    EXPECT_FALSE (c == b);
    EXPECT_TRUE (c == c);
    EXPECT_FALSE (c == d);

    EXPECT_FALSE (d == a);
    EXPECT_TRUE (d == b);
    EXPECT_FALSE (d == c);
    EXPECT_TRUE (d == d);
}

TEST (UMPCapabilityInquiryProfileTests, ProfileInquiryViewAndMessage)
{
    SysEx7 sx { Manufacturer::universalNonRealtime,
                { 0x7f, 0x0d, Subtype::profileInquiry, 0x02, 0x78, 0x56, 0x34, 0x12, 0x77, 0x55, 0x33, 0x11 } };

    EXPECT_TRUE (ProfileInquiryView::validate (sx));
    EXPECT_TRUE (isCapabilityInquiryMessage (sx));

    auto mut = makeProfileInquiryMessage (0x7665544, 0x24d2b78, 0x08);
    EXPECT_TRUE (ProfileInquiryView::validate (mut));
    EXPECT_EQ (mut.data.size(), 12u);
}

TEST (UMPCapabilityInquiryProfileTests, ProfileInquiryReplyProfiles)
{
    SysEx7 sx { Manufacturer::universalNonRealtime,
                { 0x7f, 0x0d, Subtype::profileInquiryReply, 0x00, 0x78, 0x56, 0x34, 0x12, 0x44, 0x33, 0x22, 0x11, 0x02, 0x00, 0x00, 0x21, 0x09, 42, 7, 0x7e, 0x03, 0x02, 0x01, 0x00, 0x01, 0x00, 0x7e, 0x01, 0x02, 0x03, 0x04, 0x00, 0x00 } };

    EXPECT_TRUE (ProfileInquiryReplyView::validate (sx));

    const auto view = ProfileInquiryReplyView { sx };
    const auto enabled = view.getEnabledProfiles();
    const auto disabled = view.getDisabledProfiles();

    ASSERT_EQ (enabled.size(), 2u);
    EXPECT_EQ (enabled[0].byte1, 0x00);
    EXPECT_EQ (enabled[0].byte2, 0x21);
    EXPECT_EQ (enabled[0].byte3, 0x09);
    EXPECT_EQ (enabled[0].byte4, 42);
    EXPECT_EQ (enabled[0].byte5, 7);

    ASSERT_EQ (disabled.size(), 1u);
    EXPECT_EQ (disabled[0].byte1, 0x7e);
    EXPECT_EQ (disabled[0].byte2, 0x01);
    EXPECT_EQ (disabled[0].byte3, 0x02);
    EXPECT_EQ (disabled[0].byte4, 0x03);
    EXPECT_EQ (disabled[0].byte5, 0x04);
}

TEST (UMPCapabilityInquiryProfileTests, ProfileInquiryReplyBuilder)
{
    const std::vector<ProfileId> enabled { { 0x7e, 0x02, 0x03, 0x04, 0x05 } };
    const std::vector<ProfileId> disabled { { 0x00, 0x21, 0x09, 0x7f, 0x01 } };

    auto sx = makeProfileInquiryReply (0x7665544, 0x4711, enabled, disabled, 0x08);
    EXPECT_TRUE (ProfileInquiryReplyView::validate (sx));

    const auto view = ProfileInquiryReplyView { sx };
    EXPECT_EQ (view.getNumEnabledProfiles(), 1u);
    EXPECT_EQ (view.getNumDisabledProfiles(), 1u);
}

TEST (UMPCapabilityInquiryProcessTests, ProcessInquiryCapabilities)
{
    auto inquiry = makeProcessInquiryCapabilitiesInquiry (0x7665544, 0x24d2b78, 0x08);
    EXPECT_TRUE (CapabilityInquiryView::validate (inquiry));
    EXPECT_EQ (inquiry.data.size(), 12u);

    auto reply = makeProcessInquiryCapabilitiesReply (0x7665544, 0x24d2b78, 19, 0x08);
    EXPECT_TRUE (ProcessInquiryCapabilitiesReplyView::validate (reply));
    EXPECT_EQ (reply.data.size(), 13u);

    const auto view = ProcessInquiryCapabilitiesReplyView { reply };
    EXPECT_EQ (view.getSupportedFeatures(), 19);
}

TEST (UMPCapabilityInquiryProcessTests, MidiMessageReportMessages)
{
    auto inquiry = makeMidiMessageReportInquiry (0x7665544, 0x24d2b78, 0x12, 0x34, 0x56, 0x78, 0x04);
    EXPECT_TRUE (MidiMessageReportInquiryView::validate (inquiry));
    EXPECT_EQ (inquiry.data.size(), 16u);

    auto reply = makeMidiMessageReportReply (0x7665544, 0x76, 0x54, 0x32, 0x0a);
    EXPECT_TRUE (MidiMessageReportReplyView::validate (reply));
    EXPECT_EQ (reply.data.size(), 15u);

    auto end = makeMidiMessageReportEnd (0x7665544, 0x03);
    EXPECT_TRUE (CapabilityInquiryView::validate (end));
    EXPECT_EQ (end.data.size(), 12u);
}

TEST (UMPCapabilityInquiryPropertyExchangeTests, CapabilitiesView)
{
    SysEx7 sx { Manufacturer::universalNonRealtime,
                { 0x7f, 0x0d, Subtype::propertyExchangeCapabilitiesInquiry, 0x01, 0x78, 0x56, 0x34, 0x12, 0x77, 0x55, 0x33, 0x11, 2 } };

    EXPECT_TRUE (PropertyExchangeCapabilitiesView::validate (sx));
    const auto view = PropertyExchangeCapabilitiesView { sx };
    EXPECT_EQ (view.getMaximumNumberOfRequests(), 2);
    EXPECT_EQ (view.getMajorVersion(), 0);
    EXPECT_EQ (view.getMinorVersion(), 0);

    SysEx7 missingVersion { Manufacturer::universalNonRealtime,
                            { 0x7f, 0x0d, Subtype::propertyExchangeCapabilitiesReply, 0x02, 0x78, 0x56, 0x34, 0x12, 0x77, 0x55, 0x33, 0x11, 2 } };
    EXPECT_FALSE (PropertyExchangeCapabilitiesView::validate (missingVersion));
}

TEST (UMPCapabilityInquiryPropertyExchangeTests, PropertyDataMessageValidation)
{
    const auto headerJson = propertyExchange::makeRjson (propertyExchange::Tags::resource,
                                                         "ResourceList");
    const auto header = propertyExchange::Header { std::string_view { headerJson } };
    const auto chunk = propertyExchange::Chunk { std::string_view { "ABC" } };

    auto sx = propertyExchange::makePropertyDataMessage (Subtype::getPropertyDataInquiry,
                                                         0x1234567,
                                                         0x4332211,
                                                         header,
                                                         1,
                                                         1,
                                                         chunk,
                                                         0x11,
                                                         0x0a);
    EXPECT_TRUE (propertyExchange::PropertyDataMessageView::validate (sx));
    EXPECT_TRUE (GetPropertyDataView::validate (sx));

    SysEx7 invalid { Manufacturer::universalNonRealtime,
                     { 0x7f, 0x0d, Subtype::getPropertyDataInquiry, 0x02, 0x78, 0x56, 0x34, 0x12, 0x77, 0x55, 0x33, 0x11, 0x01, 0x7f, 0x7f } };
    EXPECT_FALSE (propertyExchange::PropertyDataMessageView::validate (invalid));

    SysEx7 invalidChunk { Manufacturer::universalNonRealtime,
                          { 0x7f, 0x0d, Subtype::getPropertyDataInquiry, 0x02, 0x78, 0x56, 0x34, 0x12, 0x77, 0x55, 0x33, 0x11, 0x01, 0x01, 0x00, 0x7f, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00 } };
    EXPECT_FALSE (propertyExchange::PropertyDataMessageView::validate (invalidChunk));
}
