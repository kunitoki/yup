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

TEST (UMPStreamMessagesTests, ConstructorsAndFormat)
{
    StreamMessage m;
    EXPECT_TRUE (isStreamMessage (m));
    EXPECT_EQ (m.getFormat(), PacketFormat::complete);
    EXPECT_EQ (m.getType(), PacketType::stream);

    StreamMessage info { Status (StreamStatus::endpointInfo) };
    EXPECT_EQ (info.getFormat(), PacketFormat::complete);
    EXPECT_EQ (info.getStatus(), Status (StreamStatus::endpointInfo));

    StreamMessage cont { Status (StreamStatus::endpointName), PacketFormat::cont };
    EXPECT_EQ (cont.getFormat(), PacketFormat::cont);

    StreamMessage start { Status (StreamStatus::productInstanceId), PacketFormat::start };
    EXPECT_EQ (start.getFormat(), PacketFormat::start);

    StreamMessage end { Status (StreamStatus::endpointName), PacketFormat::end };
    EXPECT_EQ (end.getFormat(), PacketFormat::end);
}

TEST (UMPStreamMessagesTests, SetFormat)
{
    StreamMessage m;
    m.setFormat (PacketFormat::end);
    EXPECT_EQ (m.getFormat(), PacketFormat::end);
}

TEST (UMPStreamMessagesTests, EndpointDiscoveryView)
{
    auto msg = makeEndpointDiscoveryMessage (StreamDiscoveryFilter::endpointAll, 1, 2);
    auto view = EndpointDiscoveryView { msg };

    EXPECT_EQ (view.getUmpVersionMajor(), 1u);
    EXPECT_EQ (view.getUmpVersionMinor(), 2u);
    EXPECT_TRUE (view.requestsInfo());
    EXPECT_TRUE (view.requestsDeviceIdentity());
}

TEST (UMPStreamMessagesTests, EndpointInfoView)
{
    auto msg = makeEndpointInfoMessage (3,
                                        true,
                                        uint8_t (StreamProtocol::midi2),
                                        uint8_t (StreamExtensions::jitterReductionTransmit));
    auto view = EndpointInfoView { msg };

    EXPECT_EQ (view.getNumFunctionBlocks(), 3u);
    EXPECT_TRUE (view.hasStaticFunctionBlocks());
    EXPECT_EQ (view.getProtocols(), 0x2u);
}

TEST (UMPStreamMessagesTests, DeviceIdentityView)
{
    const DeviceIdentity identity { Manufacturer::nativeInstruments, 0x1234, 0x1678, 0x0abcdef0 };
    auto msg = makeDeviceIdentityMessage (identity);
    auto view = DeviceIdentityView { msg };

    const auto parsed = view.getIdentity();
    EXPECT_EQ (parsed.manufacturer, identity.manufacturer);
    EXPECT_EQ (parsed.family, identity.family);
    EXPECT_EQ (parsed.model, identity.model);
    EXPECT_EQ (parsed.revision, identity.revision);
}

TEST (UMPStreamMessagesTests, EndpointNamePayload)
{
    auto msg = makeEndpointNameMessage (PacketFormat::complete, "Endpoint");
    auto view = EndpointNameView { msg };
    EXPECT_EQ (view.getPayload(), "Endpoint");
}

TEST (UMPStreamMessagesTests, IsStreamMessageReturnsTrueForAllStreamTypes)
{
    EXPECT_TRUE (isStreamMessage (StreamMessage {}));
    EXPECT_TRUE (isStreamMessage (StreamMessage { Status (StreamStatus::endpointInfo) }));
    EXPECT_TRUE (isStreamMessage (StreamMessage { Status (StreamStatus::endpointName), PacketFormat::start }));
}

TEST (UMPStreamMessagesTests, EndpointInfoExtensionsField)
{
    auto msg = makeEndpointInfoMessage (2,
                                        false,
                                        uint8_t (StreamProtocol::midi1),
                                        uint8_t (StreamExtensions::jitterReductionReceive));
    auto view = EndpointInfoView { msg };
    EXPECT_EQ (view.getNumFunctionBlocks(), 2u);
    EXPECT_FALSE (view.hasStaticFunctionBlocks());
}

TEST (UMPStreamMessagesTests, DeviceIdentityRoundTripWithDifferentManufacturer)
{
    const DeviceIdentity identity { Manufacturer::moog, 0xABCD, 0x1234, 0x00FF0000 };
    auto msg = makeDeviceIdentityMessage (identity);
    auto view = DeviceIdentityView { msg };

    const auto parsed = view.getIdentity();
    EXPECT_EQ (parsed.manufacturer, identity.manufacturer);
    EXPECT_EQ (parsed.revision, identity.revision);
}
