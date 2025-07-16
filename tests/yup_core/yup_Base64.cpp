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

class Base64Tests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup any common test data if needed
    }

    // Helper to create test data of specific size
    MemoryBlock createTestData (size_t size, uint8 pattern = 0)
    {
        MemoryBlock block (size);
        auto* data = static_cast<uint8*> (block.getData());
        for (size_t i = 0; i < size; ++i)
            data[i] = pattern == 0 ? (uint8) (i % 256) : pattern;
        return block;
    }
};

// Test known vectors from RFC 4648
TEST_F (Base64Tests, RFC4648TestVectors)
{
    // Test vectors from RFC 4648
    struct TestVector
    {
        const char* input;
        const char* expected;
    };

    const TestVector vectors[] = {
        { "", "" },
        { "f", "Zg==" },
        { "fo", "Zm8=" },
        { "foo", "Zm9v" },
        { "foob", "Zm9vYg==" },
        { "fooba", "Zm9vYmE=" },
        { "foobar", "Zm9vYmFy" },
        { "hello world", "aGVsbG8gd29ybGQ=" },
        { "Hello World!", "SGVsbG8gV29ybGQh" },
        { "The quick brown fox jumps over the lazy dog", "VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIHRoZSBsYXp5IGRvZw==" }
    };

    for (const auto& vector : vectors)
    {
        // Test encoding
        String encoded = Base64::toBase64 (vector.input, strlen (vector.input));
        EXPECT_EQ (encoded, String (vector.expected))
            << "Failed encoding: " << vector.input;

        // Test String-based encoding
        String stringEncoded = Base64::toBase64 (String (vector.input));
        EXPECT_EQ (stringEncoded, String (vector.expected))
            << "Failed string encoding: " << vector.input;

        // Test decoding
        MemoryOutputStream decoded;
        EXPECT_TRUE (Base64::convertFromBase64 (decoded, vector.expected))
            << "Failed decoding: " << vector.expected;

        String decodedString = decoded.toString();
        EXPECT_EQ (decodedString, String (vector.input))
            << "Decode mismatch for: " << vector.expected;
    }
}

TEST_F (Base64Tests, EmptyData)
{
    // Test empty data encoding
    String encoded = Base64::toBase64 (nullptr, 0);
    EXPECT_TRUE (encoded.isEmpty());

    // Test empty string encoding
    String emptyStringEncoded = Base64::toBase64 (String());
    EXPECT_TRUE (emptyStringEncoded.isEmpty());

    // Test empty data decoding
    MemoryOutputStream decoded;
    EXPECT_TRUE (Base64::convertFromBase64 (decoded, ""));
    EXPECT_EQ (decoded.getDataSize(), 0);
}

TEST_F (Base64Tests, SingleByteData)
{
    // Test all possible single byte values
    for (int i = 0; i < 256; ++i)
    {
        uint8 byte = (uint8) i;
        String encoded = Base64::toBase64 (&byte, 1);
        EXPECT_FALSE (encoded.isEmpty());
        EXPECT_TRUE (encoded.endsWith ("==")); // Single byte should have 2 padding chars

        // Decode and verify
        MemoryOutputStream decoded;
        EXPECT_TRUE (Base64::convertFromBase64 (decoded, encoded));
        EXPECT_EQ (decoded.getDataSize(), 1);
        EXPECT_EQ (((uint8*) decoded.getData())[0], byte);
    }
}

TEST_F (Base64Tests, TwoByteData)
{
    // Test two-byte combinations
    for (int i = 0; i < 256; i += 17) // Skip some to avoid excessive testing
    {
        uint8 bytes[2] = { (uint8) i, (uint8) (255 - i) };
        String encoded = Base64::toBase64 (bytes, 2);
        EXPECT_FALSE (encoded.isEmpty());
        EXPECT_TRUE (encoded.endsWith ("=")); // Two bytes should have 1 padding char

        // Decode and verify
        MemoryOutputStream decoded;
        EXPECT_TRUE (Base64::convertFromBase64 (decoded, encoded));
        EXPECT_EQ (decoded.getDataSize(), 2);
        EXPECT_EQ (((uint8*) decoded.getData())[0], bytes[0]);
        EXPECT_EQ (((uint8*) decoded.getData())[1], bytes[1]);
    }
}

TEST_F (Base64Tests, ThreeByteData)
{
    // Test three-byte combinations (no padding)
    for (int i = 0; i < 256; i += 23) // Skip some to avoid excessive testing
    {
        uint8 bytes[3] = { (uint8) i, (uint8) (127 + i), (uint8) (255 - i) };
        String encoded = Base64::toBase64 (bytes, 3);
        EXPECT_FALSE (encoded.isEmpty());
        EXPECT_FALSE (encoded.endsWith ("=")); // Three bytes should have no padding

        // Decode and verify
        MemoryOutputStream decoded;
        EXPECT_TRUE (Base64::convertFromBase64 (decoded, encoded));
        EXPECT_EQ (decoded.getDataSize(), 3);
        EXPECT_EQ (((uint8*) decoded.getData())[0], bytes[0]);
        EXPECT_EQ (((uint8*) decoded.getData())[1], bytes[1]);
        EXPECT_EQ (((uint8*) decoded.getData())[2], bytes[2]);
    }
}

TEST_F (Base64Tests, LargeData)
{
    // Test various large data sizes
    for (size_t size : { 1000, 2048, 4096, 10000 })
    {
        auto testData = createTestData (size);

        String encoded = Base64::toBase64 (testData.getData(), testData.getSize());
        EXPECT_FALSE (encoded.isEmpty());

        MemoryOutputStream decoded;
        EXPECT_TRUE (Base64::convertFromBase64 (decoded, encoded));

        auto decodedBlock = decoded.getMemoryBlock();
        EXPECT_EQ (decodedBlock, testData);
    }
}

TEST_F (Base64Tests, StreamBasedEncoding)
{
    // Test stream-based encoding
    String testString = "Hello, World! This is a test of stream-based Base64 encoding.";

    MemoryOutputStream encodedStream;
    EXPECT_TRUE (Base64::convertToBase64 (encodedStream, testString.toRawUTF8(), strlen (testString.toRawUTF8())));

    String encodedResult = encodedStream.toString();
    EXPECT_FALSE (encodedResult.isEmpty());

    // Verify it matches direct encoding
    String directEncoded = Base64::toBase64 (testString);
    EXPECT_EQ (encodedResult, directEncoded);
}

TEST_F (Base64Tests, ErrorHandling)
{
    MemoryOutputStream decoded;

    // Test invalid characters
    EXPECT_FALSE (Base64::convertFromBase64 (decoded, "Zm9v@")); // @ is invalid
    EXPECT_FALSE (Base64::convertFromBase64 (decoded, "Zm9v#")); // # is invalid
    EXPECT_FALSE (Base64::convertFromBase64 (decoded, "Zm9v$")); // $ is invalid
    EXPECT_FALSE (Base64::convertFromBase64 (decoded, "Zm9v%")); // % is invalid

    // Test invalid padding
    EXPECT_FALSE (Base64::convertFromBase64 (decoded, "Z===")); // Too many padding chars
    EXPECT_FALSE (Base64::convertFromBase64 (decoded, "=Zg=")); // Padding at start

    // Test incomplete data
    EXPECT_FALSE (Base64::convertFromBase64 (decoded, "Z"));  // Single char
    EXPECT_FALSE (Base64::convertFromBase64 (decoded, "Zg")); // Two chars without padding
}

TEST_F (Base64Tests, PaddingVariants)
{
    // Test various padding scenarios
    MemoryOutputStream decoded;

    // Valid padding scenarios
    EXPECT_TRUE (Base64::convertFromBase64 (decoded, "Zg==")); // Single byte with padding
    EXPECT_TRUE (Base64::convertFromBase64 (decoded, "Zm8=")); // Two bytes with padding
    EXPECT_TRUE (Base64::convertFromBase64 (decoded, "Zm9v")); // Three bytes, no padding
    EXPECT_TRUE (Base64::convertFromBase64 (decoded, "Zg=a")); // Non-padding after padding

    // Invalid padding placement
    EXPECT_FALSE (Base64::convertFromBase64 (decoded, "Z=g=")); // Padding in middle
    EXPECT_FALSE (Base64::convertFromBase64 (decoded, "=Zg=")); // Padding at start
}

TEST_F (Base64Tests, BinaryData)
{
    // Test with binary data containing all byte values
    MemoryBlock binaryData (256);
    auto* data = static_cast<uint8*> (binaryData.getData());
    for (int i = 0; i < 256; ++i)
        data[i] = (uint8) i;

    String encoded = Base64::toBase64 (binaryData.getData(), binaryData.getSize());
    EXPECT_FALSE (encoded.isEmpty());

    MemoryOutputStream decoded;
    EXPECT_TRUE (Base64::convertFromBase64 (decoded, encoded));

    auto decodedBlock = decoded.getMemoryBlock();
    EXPECT_EQ (decodedBlock, binaryData);
}

TEST_F (Base64Tests, RandomData)
{
    Random random;

    auto createRandomData = [&]
    {
        MemoryOutputStream m;
        for (int i = random.nextInt (400); --i >= 0;)
            m.writeByte (static_cast<char> (random.nextInt (256)));
        return m.getMemoryBlock();
    };

    for (int i = 100; --i >= 0;) // Reduced from 1000 to speed up tests
    {
        auto original = createRandomData();
        auto asBase64 = Base64::toBase64 (original.getData(), original.getSize());

        MemoryOutputStream out;
        EXPECT_TRUE (Base64::convertFromBase64 (out, asBase64));

        auto result = out.getMemoryBlock();
        EXPECT_EQ (result, original);
    }
}

TEST_F (Base64Tests, UnicodeStringEncoding)
{
    // Test encoding of unicode strings
    String unicodeString = L"Hello 世界! 🌍 Test";
    String encoded = Base64::toBase64 (unicodeString);
    EXPECT_FALSE (encoded.isEmpty());

    MemoryOutputStream decoded;
    EXPECT_TRUE (Base64::convertFromBase64 (decoded, encoded));

    String decodedString = String::fromUTF8 (static_cast<const char*> (decoded.getData()), (int) decoded.getDataSize());
    EXPECT_EQ (decodedString, unicodeString);
}

TEST_F (Base64Tests, LongLines)
{
    // Test very long data that would result in long base64 lines
    auto longData = createTestData (5000, 0xAB);
    String encoded = Base64::toBase64 (longData.getData(), longData.getSize());

    MemoryOutputStream decoded;
    EXPECT_TRUE (Base64::convertFromBase64 (decoded, encoded));

    auto decodedBlock = decoded.getMemoryBlock();
    EXPECT_EQ (decodedBlock, longData);
}

TEST_F (Base64Tests, SpecialCharacters)
{
    // Test data containing special characters used in Base64
    String specialData = "++//==";
    String encoded = Base64::toBase64 (specialData);

    MemoryOutputStream decoded;
    EXPECT_TRUE (Base64::convertFromBase64 (decoded, encoded));

    String decodedString = decoded.toString();
    EXPECT_EQ (decodedString, specialData);
}
