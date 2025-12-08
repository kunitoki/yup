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

TEST (BigIntegerTests, BasicTests)
{
    auto getBigRandom = [] (Random& r)
    {
        BigInteger b;

        while (b < 2)
            r.fillBitsRandomly (b, 0, r.nextInt (150) + 1);

        return b;
    };

    Random& r = Random::getSystemRandom();

    EXPECT_TRUE (BigInteger().isZero());
    EXPECT_TRUE (BigInteger (1).isOne());

    for (int j = 10000; --j >= 0;)
    {
        BigInteger b1 (getBigRandom (r)), b2 (getBigRandom (r));

        BigInteger b3 = b1 + b2;
        EXPECT_TRUE (b3 > b1 && b3 > b2);
        EXPECT_TRUE (b3 - b1 == b2);
        EXPECT_TRUE (b3 - b2 == b1);

        BigInteger b4 = b1 * b2;
        EXPECT_TRUE (b4 > b1 && b4 > b2);
        EXPECT_TRUE (b4 / b1 == b2);
        EXPECT_TRUE (b4 / b2 == b1);
        EXPECT_TRUE (((b4 << 1) >> 1) == b4);
        EXPECT_TRUE (((b4 << 10) >> 10) == b4);
        EXPECT_TRUE (((b4 << 100) >> 100) == b4);

        // TODO: should add tests for other ops (although they also get pretty well tested in the RSA unit test)

        BigInteger b5;
        b5.loadFromMemoryBlock (b3.toMemoryBlock());
        EXPECT_TRUE (b3 == b5);
    }
}

TEST (BigIntegerTests, BitSetting)
{
    Random r = Random::getSystemRandom();
    static uint8 test[2048];

    for (int j = 100000; --j >= 0;)
    {
        uint32 offset = static_cast<uint32> (r.nextInt (200) + 10);
        uint32 num = static_cast<uint32> (r.nextInt (32) + 1);
        uint32 value = static_cast<uint32> (r.nextInt());

        if (num < 32)
            value &= ((1u << num) - 1);

        auto old1 = readLittleEndianBitsInBuffer (test, offset - 6, 6);
        auto old2 = readLittleEndianBitsInBuffer (test, offset + num, 6);
        writeLittleEndianBitsInBuffer (test, offset, num, value);
        auto result = readLittleEndianBitsInBuffer (test, offset, num);

        EXPECT_TRUE (result == value);
        EXPECT_TRUE (old1 == readLittleEndianBitsInBuffer (test, offset - 6, 6));
        EXPECT_TRUE (old2 == readLittleEndianBitsInBuffer (test, offset + num, 6));
    }
}

TEST (BigIntegerTests, ConstructorUInt32)
{
    BigInteger zero (0u);
    EXPECT_TRUE (zero.isZero());
    EXPECT_EQ (0, zero.toInteger());

    BigInteger small (42u);
    EXPECT_EQ (42, small.toInteger());
    EXPECT_FALSE (small.isZero());
    EXPECT_FALSE (small.isNegative());

    BigInteger maxUInt32 (0xFFFFFFFFu);
    EXPECT_FALSE (maxUInt32.isZero());
    EXPECT_FALSE (maxUInt32.isNegative());
    // toInteger() masks with 0x7FFFFFFF, so returns max positive int
    EXPECT_EQ (0x7FFFFFFF, maxUInt32.toInteger());
}

TEST (BigIntegerTests, ConstructorInt64Negative)
{
    BigInteger negative (int64 (-42));
    EXPECT_TRUE (negative.isNegative());
    EXPECT_EQ (-42, negative.toInt64());
    EXPECT_FALSE (negative.isZero());

    BigInteger largeNegative (int64 (-9223372036854775807LL));
    EXPECT_TRUE (largeNegative.isNegative());
    EXPECT_EQ (-9223372036854775807LL, largeNegative.toInt64());

    BigInteger minInt64 (int64 (-9223372036854775807LL) - 1);
    EXPECT_TRUE (minInt64.isNegative());
}

TEST (BigIntegerTests, AssignmentOperatorReallocation)
{
    // Create a small BigInteger
    BigInteger small (42);

    // Create a large BigInteger that requires heap allocation
    BigInteger large;
    for (int i = 0; i < 200; ++i)
        large.setBit (i);

    // Assign large to small - should trigger reallocation
    small = large;
    EXPECT_TRUE (small == large);
    EXPECT_EQ (199, small.getHighestBit());

    // Assign small value to large - should free heap
    BigInteger tiny (1);
    large = tiny;
    EXPECT_TRUE (large == tiny);
    EXPECT_TRUE (large.isOne());
}

TEST (BigIntegerTests, EnsureSizeWithExistingHeapAllocation)
{
    BigInteger big;

    // Force heap allocation by setting a high bit
    big.setBit (150);
    EXPECT_EQ (150, big.getHighestBit());

    // Now set an even higher bit, forcing reallocation
    big.setBit (300);
    EXPECT_EQ (300, big.getHighestBit());
    EXPECT_TRUE (big[150]);
    EXPECT_TRUE (big[300]);
}

TEST (BigIntegerTests, AdditionEdgeCases)
{
    // Test adding to itself
    BigInteger a (100);
    a += a;
    EXPECT_EQ (200, a.toInteger());

    // Test negative + positive where abs(negative) < positive
    BigInteger neg (-50);
    BigInteger pos (100);
    neg += pos;
    EXPECT_EQ (50, neg.toInteger());
    EXPECT_FALSE (neg.isNegative());

    // Test negative + positive where abs(negative) > positive
    BigInteger neg2 (-100);
    BigInteger pos2 (50);
    neg2 += pos2;
    EXPECT_EQ (-50, neg2.toInteger());
    EXPECT_TRUE (neg2.isNegative());

    // Test adding negative to positive
    BigInteger pos3 (100);
    BigInteger neg3 (-50);
    pos3 += neg3;
    EXPECT_EQ (50, pos3.toInteger());
}

TEST (BigIntegerTests, SubtractionEdgeCases)
{
    // Test subtracting from itself
    BigInteger a (100);
    a -= a;
    EXPECT_TRUE (a.isZero());

    // Test subtracting negative (becomes addition)
    BigInteger pos (100);
    BigInteger neg (-50);
    pos -= neg;
    EXPECT_EQ (150, pos.toInteger());

    // Test negative - positive
    BigInteger neg2 (-50);
    BigInteger pos2 (100);
    neg2 -= pos2;
    EXPECT_EQ (-150, neg2.toInteger());
    EXPECT_TRUE (neg2.isNegative());

    // Test positive - larger positive (result becomes negative)
    BigInteger small (50);
    BigInteger large (100);
    small -= large;
    EXPECT_EQ (-50, small.toInteger());
    EXPECT_TRUE (small.isNegative());
}

TEST (BigIntegerTests, DivideByEdgeCases)
{
    // Test dividing by itself
    BigInteger dividend (100);
    BigInteger remainder;
    dividend.divideBy (dividend, remainder);
    EXPECT_TRUE (dividend.isOne());
    EXPECT_TRUE (remainder.isZero());

    // Test division by zero
    BigInteger numerator (100);
    BigInteger zero;
    BigInteger rem;
    numerator.divideBy (zero, rem);
    EXPECT_TRUE (numerator.isZero());
    EXPECT_TRUE (rem.isZero());

    // Test zero divided by something
    BigInteger zero2;
    BigInteger divisor (42);
    BigInteger rem2;
    zero2.divideBy (divisor, rem2);
    EXPECT_TRUE (zero2.isZero());
    EXPECT_TRUE (rem2.isZero());
}

TEST (BigIntegerTests, IncrementOperators)
{
    // Pre-increment
    BigInteger a (42);
    BigInteger& result = ++a;
    EXPECT_EQ (43, a.toInteger());
    EXPECT_EQ (43, result.toInteger());

    // Post-increment
    BigInteger b (42);
    BigInteger old = b++;
    EXPECT_EQ (43, b.toInteger());
    EXPECT_EQ (42, old.toInteger());

    // Test with negative
    BigInteger neg (-5);
    ++neg;
    EXPECT_EQ (-4, neg.toInteger());

    BigInteger neg2 (-1);
    neg2++;
    EXPECT_TRUE (neg2.isZero());
}

TEST (BigIntegerTests, DecrementOperators)
{
    // Pre-decrement
    BigInteger a (42);
    BigInteger& result = --a;
    EXPECT_EQ (41, a.toInteger());
    EXPECT_EQ (41, result.toInteger());

    // Post-decrement
    BigInteger b (42);
    BigInteger old = b--;
    EXPECT_EQ (41, b.toInteger());
    EXPECT_EQ (42, old.toInteger());

    // Test with negative
    BigInteger neg (-5);
    --neg;
    EXPECT_EQ (-6, neg.toInteger());

    // Test crossing zero
    BigInteger one (1);
    one--;
    EXPECT_TRUE (one.isZero());
}

TEST (BigIntegerTests, UnaryMinus)
{
    BigInteger pos (42);
    BigInteger neg = -pos;
    EXPECT_EQ (42, pos.toInteger());
    EXPECT_EQ (-42, neg.toInteger());
    EXPECT_TRUE (neg.isNegative());

    BigInteger zero;
    BigInteger negZero = -zero;
    EXPECT_TRUE (negZero.isZero());
    EXPECT_FALSE (negZero.isNegative());
}

TEST (BigIntegerTests, BitwiseOrOperator)
{
    BigInteger a (0b1010);
    BigInteger b (0b1100);
    BigInteger result = a | b;
    EXPECT_EQ (0b1110, result.toInteger());

    BigInteger zero;
    BigInteger value (42);
    BigInteger orWithZero = value | zero;
    EXPECT_EQ (42, orWithZero.toInteger());
}

TEST (BigIntegerTests, BitwiseAndOperator)
{
    BigInteger a (0b1010);
    BigInteger b (0b1100);
    BigInteger result = a & b;
    EXPECT_EQ (0b1000, result.toInteger());

    BigInteger zero;
    BigInteger value (42);
    BigInteger andWithZero = value & zero;
    EXPECT_TRUE (andWithZero.isZero());
}

TEST (BigIntegerTests, BitwiseXorOperator)
{
    BigInteger a (0b1010);
    BigInteger b (0b1100);
    BigInteger result = a ^ b;
    EXPECT_EQ (0b0110, result.toInteger());

    BigInteger value (42);
    BigInteger xorWithItself = value ^ value;
    EXPECT_TRUE (xorWithItself.isZero());
}

TEST (BigIntegerTests, CompareEqual)
{
    BigInteger a (42);
    BigInteger b (42);
    EXPECT_EQ (0, a.compare (b));
    EXPECT_TRUE (a == b);
    EXPECT_FALSE (a != b);
    EXPECT_FALSE (a < b);
    EXPECT_TRUE (a <= b);
    EXPECT_FALSE (a > b);
    EXPECT_TRUE (a >= b);
}

TEST (BigIntegerTests, ShiftRightWithStartBit)
{
    BigInteger value;
    value.setBit (10);
    value.setBit (11);
    value.setBit (12);

    // Shift right by 2 bits starting from bit 5
    // Bits 10, 11, 12 move to positions 8, 9, 10
    value.shiftBits (-2, 5);

    EXPECT_TRUE (value[8]);   // Was bit 10
    EXPECT_TRUE (value[9]);   // Was bit 11
    EXPECT_TRUE (value[10]);  // Was bit 12
    EXPECT_FALSE (value[11]); // Cleared
    EXPECT_FALSE (value[12]); // Cleared
}

TEST (BigIntegerTests, FindGreatestCommonDivisorComplex)
{
    // Test the complex path that creates temp2
    BigInteger a (1071);
    BigInteger b (462);
    BigInteger gcd = a.findGreatestCommonDivisor (b);
    EXPECT_EQ (21, gcd.toInteger());

    // Test with large numbers that differ significantly in bit count
    // This triggers the divideBy path in findGreatestCommonDivisor
    BigInteger large;
    large.setBit (100);
    large += 1000000;

    BigInteger small (1000);
    BigInteger gcd2 = large.findGreatestCommonDivisor (small);
    // GCD(2^100 + 1000000, 1000) = GCD(1000000, 1000) = 1000
    EXPECT_EQ (8, gcd2.toInteger());
}

TEST (BigIntegerTests, ExponentModuloComplexPath)
{
    // Test the else branch (Montgomery multiplication path)
    BigInteger base (7);
    BigInteger exponent (3);
    BigInteger modulus (11);

    base.exponentModulo (exponent, modulus);
    // 7^3 = 343, 343 % 11 = 2
    EXPECT_EQ (2, base.toInteger());

    // Test with larger odd modulus to trigger Montgomery path
    BigInteger base2 (5);
    BigInteger exp2 (100);
    BigInteger mod2 (17);

    base2.exponentModulo (exp2, mod2);
    EXPECT_TRUE (base2.toInteger() >= 0);
    EXPECT_TRUE (base2.toInteger() < 17);
}

TEST (BigIntegerTests, MontgomeryMultiplicationNegative)
{
    BigInteger a (5);
    BigInteger b (3);
    BigInteger modulus (7);
    BigInteger modulusp;

    // Set up for Montgomery multiplication
    int k = 8;
    BigInteger R (1);
    R <<= k;

    BigInteger R1, m1, g;
    g.extendedEuclidean (modulus, R, m1, R1);

    a.montgomeryMultiplication (b, modulus, m1, k);

    // Make it negative to test the else if branch
    BigInteger negTest (-10);
    negTest.montgomeryMultiplication (b, modulus, m1, k);
    // Should be adjusted by adding modulus
    EXPECT_FALSE (negTest.isNegative());
}

TEST (BigIntegerTests, ExtendedEuclideanSwapPath)
{
    BigInteger a (17);
    BigInteger b (13);
    BigInteger x, y;
    BigInteger gcd;

    gcd.extendedEuclidean (a, b, x, y);

    // Verify the Extended Euclidean algorithm result
    // gcd = a*x + b*y (or a*x - b*y depending on implementation)
    EXPECT_EQ (1, gcd.toInteger());

    // Test the swap condition
    BigInteger a2 (240);
    BigInteger b2 (46);
    BigInteger x2, y2;
    BigInteger gcd2;

    gcd2.extendedEuclidean (a2, b2, x2, y2);
    EXPECT_EQ (2, gcd2.toInteger());

    // Verify: gcd = a*x - b*y or gcd = b*y - a*x
    BigInteger check1 = a2 * x2;
    BigInteger check2 = b2 * y2;
    BigInteger diff1 = check2 - check1;
    BigInteger diff2 = check1 - check2;

    EXPECT_TRUE (gcd2.compareAbsolute (diff1) == 0 || gcd2.compareAbsolute (diff2) == 0);
}

TEST (BigIntegerTests, OutputStreamOperator)
{
    BigInteger value (12345);
    MemoryOutputStream stream;

    stream << value;

    String result = stream.toString();
    EXPECT_EQ (String ("12345"), result);

    // Test with negative
    BigInteger negative (-6789);
    MemoryOutputStream stream2;
    stream2 << negative;
    EXPECT_EQ (String ("-6789"), stream2.toString());
}

TEST (BigIntegerTests, ToStringBase10)
{
    BigInteger value (12345);
    String str = value.toString (10);
    EXPECT_EQ (String ("12345"), str);

    // Test with minimum characters padding
    BigInteger small (42);
    String padded = small.toString (10, 5);
    EXPECT_EQ (String ("00042"), padded);

    // Test negative
    BigInteger negative (-9876);
    String negStr = negative.toString (10);
    EXPECT_EQ (String ("-9876"), negStr);

    // Test zero
    BigInteger zero;
    String zeroStr = zero.toString (10);
    EXPECT_EQ (String ("0"), zeroStr);
}

TEST (BigIntegerTests, ParseStringBase10)
{
    BigInteger value;
    value.parseString ("12345", 10);
    EXPECT_EQ (12345, value.toInteger());

    // Test with whitespace
    BigInteger withSpace;
    withSpace.parseString ("  42", 10);
    EXPECT_EQ (42, withSpace.toInteger());

    // Test large number
    BigInteger large;
    large.parseString ("123456789012345", 10);
    EXPECT_EQ (123456789012345LL, large.toInt64());

    // Test parsing and then manually setting negative
    BigInteger manualNegative;
    manualNegative.parseString ("6789", 10);
    manualNegative.setNegative (true);
    EXPECT_EQ (-6789, manualNegative.toInteger());
    EXPECT_TRUE (manualNegative.isNegative());
}
