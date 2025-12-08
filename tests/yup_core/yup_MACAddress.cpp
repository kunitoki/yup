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

#include <gtest/gtest.h>

#include <yup_core/yup_core.h>

using namespace yup;

class MACAddressTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

// =============================================================================
// Constructor Tests
// =============================================================================

TEST_F (MACAddressTests, DefaultConstructor)
{
    MACAddress addr;
    EXPECT_TRUE (addr.isNull());
    EXPECT_EQ (addr.toInt64(), 0);
    EXPECT_EQ (addr.toString(), "00-00-00-00-00-00");
}

TEST_F (MACAddressTests, CopyConstructor)
{
    uint8 bytes[6] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
    MACAddress addr1 (bytes);
    MACAddress addr2 (addr1);

    EXPECT_EQ (addr1, addr2);
    EXPECT_EQ (addr2.toString(), addr1.toString());
}

TEST_F (MACAddressTests, AssignmentOperator)
{
    uint8 bytes[6] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
    MACAddress addr1 (bytes);
    MACAddress addr2;

    addr2 = addr1;
    EXPECT_EQ (addr1, addr2);
    EXPECT_EQ (addr2.toString(), "11-22-33-44-55-66");
}

TEST_F (MACAddressTests, ConstructorFromBytes)
{
    uint8 bytes[6] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
    MACAddress addr (bytes);

    EXPECT_FALSE (addr.isNull());
    EXPECT_EQ (addr.toString(), "aa-bb-cc-dd-ee-ff");
}

TEST_F (MACAddressTests, ConstructorFromValidHexString)
{
    MACAddress addr ("112233445566");
    EXPECT_FALSE (addr.isNull());
    EXPECT_EQ (addr.toString(), "11-22-33-44-55-66");
}

TEST_F (MACAddressTests, ConstructorFromHexStringWithDashes)
{
    MACAddress addr ("11-22-33-44-55-66");
    EXPECT_FALSE (addr.isNull());
    EXPECT_EQ (addr.toString(), "11-22-33-44-55-66");
}

TEST_F (MACAddressTests, ConstructorFromHexStringWithColons)
{
    MACAddress addr ("11:22:33:44:55:66");
    EXPECT_FALSE (addr.isNull());
    EXPECT_EQ (addr.toString(), "11-22-33-44-55-66");
}

TEST_F (MACAddressTests, ConstructorFromInvalidHexStringTooShort)
{
    MACAddress addr ("1122334455");
    EXPECT_TRUE (addr.isNull());
}

TEST_F (MACAddressTests, ConstructorFromInvalidHexStringTooLong)
{
    MACAddress addr ("11223344556677");
    EXPECT_TRUE (addr.isNull());
}

TEST_F (MACAddressTests, ConstructorFromEmptyString)
{
    MACAddress addr ("");
    EXPECT_TRUE (addr.isNull());
}

TEST_F (MACAddressTests, ConstructorFromInvalidCharacters)
{
    // MemoryBlock::loadFromHexString treats invalid hex chars as 0
    // So "GGHHIIJJKKLL" becomes 12 chars but with invalid hex resulting in non-null
    MACAddress addr ("GGHHIIJJKKLL");
    // The behavior depends on MemoryBlock implementation, which may parse partial hex
    // This test verifies the constructor doesn't crash with invalid input
    EXPECT_NO_THROW (addr.toString());
}

// =============================================================================
// toString Tests
// =============================================================================

TEST_F (MACAddressTests, ToStringDefault)
{
    uint8 bytes[6] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB };
    MACAddress addr (bytes);
    EXPECT_EQ (addr.toString(), "01-23-45-67-89-ab");
}

TEST_F (MACAddressTests, ToStringWithCustomSeparator)
{
    uint8 bytes[6] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB };
    MACAddress addr (bytes);
    EXPECT_EQ (addr.toString (":"), "01:23:45:67:89:ab");
}

TEST_F (MACAddressTests, ToStringWithEmptySeparator)
{
    uint8 bytes[6] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB };
    MACAddress addr (bytes);
    EXPECT_EQ (addr.toString (""), "0123456789ab");
}

TEST_F (MACAddressTests, ToStringWithMultiCharSeparator)
{
    uint8 bytes[6] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
    MACAddress addr (bytes);
    EXPECT_EQ (addr.toString ("::"), "aa::bb::cc::dd::ee::ff");
}

TEST_F (MACAddressTests, ToStringPadsZeros)
{
    uint8 bytes[6] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 };
    MACAddress addr (bytes);
    EXPECT_EQ (addr.toString(), "00-01-02-03-04-05");
}

// =============================================================================
// toInt64 Tests
// =============================================================================

TEST_F (MACAddressTests, ToInt64Zero)
{
    MACAddress addr;
    EXPECT_EQ (addr.toInt64(), 0);
}

TEST_F (MACAddressTests, ToInt64Simple)
{
    uint8 bytes[6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
    MACAddress addr (bytes);

    // Little-endian: bytes are stored with first byte in LSB
    int64 expected = 0x060504030201LL;
    EXPECT_EQ (addr.toInt64(), expected);
}

TEST_F (MACAddressTests, ToInt64AllFF)
{
    uint8 bytes[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    MACAddress addr (bytes);
    EXPECT_EQ (addr.toInt64(), 0xFFFFFFFFFFFFLL);
}

TEST_F (MACAddressTests, ToInt64Alternating)
{
    uint8 bytes[6] = { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 };
    MACAddress addr (bytes);

    int64 expected = 0x55AA55AA55AALL;
    EXPECT_EQ (addr.toInt64(), expected);
}

// =============================================================================
// getBytes Tests
// =============================================================================

TEST_F (MACAddressTests, GetBytes)
{
    uint8 bytes[6] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
    MACAddress addr (bytes);

    const uint8* retrieved = addr.getBytes();
    for (int i = 0; i < 6; ++i)
    {
        EXPECT_EQ (retrieved[i], bytes[i]);
    }
}

// =============================================================================
// isNull Tests
// =============================================================================

TEST_F (MACAddressTests, IsNullForDefaultConstructed)
{
    MACAddress addr;
    EXPECT_TRUE (addr.isNull());
}

TEST_F (MACAddressTests, IsNullForZeroBytes)
{
    uint8 bytes[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    MACAddress addr (bytes);
    EXPECT_TRUE (addr.isNull());
}

TEST_F (MACAddressTests, IsNotNullForNonZeroAddress)
{
    uint8 bytes[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
    MACAddress addr (bytes);
    EXPECT_FALSE (addr.isNull());
}

TEST_F (MACAddressTests, IsNotNullForValidAddress)
{
    MACAddress addr ("112233445566");
    EXPECT_FALSE (addr.isNull());
}

// =============================================================================
// Equality Tests
// =============================================================================

TEST_F (MACAddressTests, EqualityOperator)
{
    uint8 bytes[6] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
    MACAddress addr1 (bytes);
    MACAddress addr2 (bytes);

    EXPECT_TRUE (addr1 == addr2);
}

TEST_F (MACAddressTests, EqualityOperatorDifferent)
{
    uint8 bytes1[6] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
    uint8 bytes2[6] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x67 };
    MACAddress addr1 (bytes1);
    MACAddress addr2 (bytes2);

    EXPECT_FALSE (addr1 == addr2);
}

TEST_F (MACAddressTests, InequalityOperator)
{
    uint8 bytes1[6] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
    uint8 bytes2[6] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
    MACAddress addr1 (bytes1);
    MACAddress addr2 (bytes2);

    EXPECT_TRUE (addr1 != addr2);
}

TEST_F (MACAddressTests, InequalityOperatorSame)
{
    uint8 bytes[6] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
    MACAddress addr1 (bytes);
    MACAddress addr2 (bytes);

    EXPECT_FALSE (addr1 != addr2);
}

TEST_F (MACAddressTests, EqualityForNullAddresses)
{
    MACAddress addr1;
    MACAddress addr2;

    EXPECT_TRUE (addr1 == addr2);
}

// =============================================================================
// getAllAddresses Tests
// =============================================================================

TEST_F (MACAddressTests, GetAllAddresses)
{
    auto addresses = MACAddress::getAllAddresses();

    // Should return an array (may be empty on some systems)
    // At least verify it doesn't crash and returns valid data
    for (const auto& addr : addresses)
    {
        // Each address should have a valid string representation
        String str = addr.toString();
        EXPECT_EQ (str.length(), 17); // Format: "XX-XX-XX-XX-XX-XX"
    }
}

// =============================================================================
// Round-trip Tests
// =============================================================================

TEST_F (MACAddressTests, RoundTripThroughString)
{
    uint8 bytes[6] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
    MACAddress addr1 (bytes);

    String str = addr1.toString ("");
    MACAddress addr2 (str);

    EXPECT_EQ (addr1, addr2);
}

TEST_F (MACAddressTests, RoundTripThroughInt64)
{
    uint8 bytes[6] = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC };
    MACAddress addr1 (bytes);

    int64 value = addr1.toInt64();

    // Reconstruct from int64 (note: there's no direct constructor, so we use bytes)
    uint8 reconstructed[6];
    for (int i = 0; i < 6; ++i)
    {
        reconstructed[i] = (value >> (i * 8)) & 0xFF;
    }

    MACAddress addr2 (reconstructed);
    EXPECT_EQ (addr1, addr2);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F (MACAddressTests, MaxValueAddress)
{
    uint8 bytes[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    MACAddress addr (bytes);

    EXPECT_FALSE (addr.isNull());
    EXPECT_EQ (addr.toString(), "ff-ff-ff-ff-ff-ff");
    EXPECT_EQ (addr.toInt64(), 0xFFFFFFFFFFFFLL);
}

TEST_F (MACAddressTests, AlternatingBitPattern)
{
    uint8 bytes[6] = { 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA };
    MACAddress addr (bytes);

    EXPECT_FALSE (addr.isNull());
    EXPECT_EQ (addr.toString(), "55-aa-55-aa-55-aa");
}

TEST_F (MACAddressTests, SequentialBytes)
{
    uint8 bytes[6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
    MACAddress addr (bytes);

    const uint8* retrieved = addr.getBytes();
    for (int i = 0; i < 6; ++i)
    {
        EXPECT_EQ (retrieved[i], i + 1);
    }
}

TEST_F (MACAddressTests, CopyPreservesValue)
{
    uint8 bytes[6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE };
    MACAddress original (bytes);
    MACAddress copy = original;

    // Modify original (by reassignment)
    uint8 newBytes[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    original = MACAddress (newBytes);

    // Copy should still have old value
    EXPECT_EQ (copy.toString(), "de-ad-be-ef-ca-fe");
    EXPECT_NE (original, copy);
}

TEST_F (MACAddressTests, SelfAssignment)
{
    uint8 bytes[6] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
    MACAddress addr (bytes);

    addr = addr; // Self-assignment

    EXPECT_EQ (addr.toString(), "11-22-33-44-55-66");
}

TEST_F (MACAddressTests, MixedCaseHexString)
{
    MACAddress addr1 ("aAbBcCdDeEfF");
    MACAddress addr2 ("AABBCCDDEEFF");

    EXPECT_EQ (addr1, addr2);
    EXPECT_EQ (addr1.toString(), "aa-bb-cc-dd-ee-ff");
}
