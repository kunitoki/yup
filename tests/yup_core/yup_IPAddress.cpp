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

#include <gtest/gtest.h>

#include <yup_core/yup_core.h>

using namespace yup;

TEST (IPAddressTests, Constructors)
{
    // Default IPAdress should be null
    IPAddress defaultConstructed;
    EXPECT_TRUE (defaultConstructed.isNull());

    auto local = IPAddress::local();
    EXPECT_FALSE (local.isNull());

    IPAddress ipv4 { 1, 2, 3, 4 };
    EXPECT_FALSE (ipv4.isNull());
    EXPECT_FALSE (ipv4.isIPv6);
    EXPECT_EQ (ipv4.toString(), "1.2.3.4");
}

TEST (IPAddressTests, FindAllAddresses)
{
    Array<IPAddress> ipv4Addresses;
    Array<IPAddress> allAddresses;

    IPAddress::findAllAddresses (ipv4Addresses, false);
    IPAddress::findAllAddresses (allAddresses, true);

    EXPECT_GE (allAddresses.size(), ipv4Addresses.size());

    for (auto& a : ipv4Addresses)
    {
        EXPECT_FALSE (a.isNull());
        EXPECT_FALSE (a.isIPv6);
    }

    for (auto& a : allAddresses)
    {
        EXPECT_FALSE (a.isNull());
    }
}

TEST (IPAddressTests, FindBroadcastAddress)
{
    Array<IPAddress> addresses;

    // Only IPv4 interfaces have broadcast
    IPAddress::findAllAddresses (addresses, false);

    for (auto& a : addresses)
    {
        EXPECT_FALSE (a.isNull());

        auto broadcastAddress = IPAddress::getInterfaceBroadcastAddress (a);

        // If we retrieve an address, it should be an IPv4 address
        if (! broadcastAddress.isNull())
        {
            EXPECT_FALSE (a.isIPv6);
        }
    }

    // Expect to fail to find a broadcast for this address
    IPAddress address { 1, 2, 3, 4 };
    EXPECT_TRUE (IPAddress::getInterfaceBroadcastAddress (address).isNull());
}