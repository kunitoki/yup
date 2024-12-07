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

#include <juce_core/juce_core.h>

using namespace juce;

#define QUOTE(x) #x
#define STR(value) QUOTE (value)
#define ASYM_CHARPTR_DOUBLE_PAIR(str, value) std::pair<const char*, double> (STR (str), value)
#define CHARPTR_DOUBLE_PAIR(value) ASYM_CHARPTR_DOUBLE_PAIR (value, value)
#define CHARPTR_DOUBLE_PAIR_COMBOS(value)            \
CHARPTR_DOUBLE_PAIR (value),                         \
    CHARPTR_DOUBLE_PAIR (-value),                    \
    ASYM_CHARPTR_DOUBLE_PAIR (+value, value),        \
    ASYM_CHARPTR_DOUBLE_PAIR (000000##value, value), \
    ASYM_CHARPTR_DOUBLE_PAIR (+000##value, value),   \
    ASYM_CHARPTR_DOUBLE_PAIR (-0##value, -value)

namespace
{

template <typename CharPointerType>
MemoryBlock memoryBlockFromCharPtr (const typename CharPointerType::CharType* charPtr)
{
    using CharType = typename CharPointerType::CharType;

    MemoryBlock result;
    CharPointerType source (charPtr);

    result.setSize (CharPointerType::getBytesRequiredFor (source) + sizeof (CharType));
    CharPointerType dest { (CharType*) result.getData() };
    dest.writeAll (source);
    return result;
}

template <typename FromCharPointerType, typename ToCharPointerType>
MemoryBlock convert (const MemoryBlock& source, bool removeNullTerminator = false)
{
    using ToCharType = typename ToCharPointerType ::CharType;
    using FromCharType = typename FromCharPointerType::CharType;

    FromCharPointerType sourcePtr { (FromCharType*) source.getData() };

    std::vector<juce_wchar> sourceChars;
    size_t requiredSize = 0;
    juce_wchar c;

    while ((c = sourcePtr.getAndAdvance()) != '\0')
    {
        requiredSize += ToCharPointerType::getBytesRequiredFor (c);
        sourceChars.push_back (c);
    }

    if (! removeNullTerminator)
        requiredSize += sizeof (ToCharType);

    MemoryBlock result;
    result.setSize (requiredSize);

    ToCharPointerType dest { (ToCharType*) result.getData() };

    for (auto wc : sourceChars)
        dest.write (wc);

    if (! removeNullTerminator)
        dest.writeNull();

    return result;
}

struct SeparatorStrings
{
    std::vector<MemoryBlock> terminals, nulls;
};

template <typename CharPointerType>
SeparatorStrings getSeparators()
{
    jassertfalse;
    return {};
}

template <>
SeparatorStrings getSeparators<CharPointer_ASCII>()
{
    SeparatorStrings result;

    const CharPointer_ASCII::CharType* terminalCharPtrs[] = {
        "", "-", "+", "e", "e+", "E-", "f", " ", ",", ";", "<", "'", "\"", "_", "k", " +", " -", " -e", "-In ", " +n", "n", "  r"
    };

    for (auto ptr : terminalCharPtrs)
        result.terminals.push_back (memoryBlockFromCharPtr<CharPointer_ASCII> (ptr));

    const CharPointer_ASCII::CharType* nullCharPtrs[] = { "." };

    result.nulls = result.terminals;

    for (auto ptr : nullCharPtrs)
        result.nulls.push_back (memoryBlockFromCharPtr<CharPointer_ASCII> (ptr));

    return result;
}

template <>
SeparatorStrings getSeparators<CharPointer_UTF8>()
{
    auto result = getSeparators<CharPointer_ASCII>();

    const CharPointer_UTF8::CharType* terminalCharPtrs[] = {
        "\xe2\x82\xac",                    // €
        "\xf0\x90\x90\xB7",                // 𐐷
        "\xf0\x9f\x98\x83",                // 😃
        "\xf0\x9f\x8f\x81\xF0\x9F\x9A\x97" // 🏁🚗
    };

    for (auto ptr : terminalCharPtrs)
    {
        auto block = memoryBlockFromCharPtr<CharPointer_UTF8> (ptr);

        for (auto vec : { &result.terminals, &result.nulls })
            vec->push_back (block);
    }

    return result;
}

template <typename CharPointerType, typename StorageType>
SeparatorStrings prefixWithAsciiSeparators (const std::vector<std::vector<StorageType>>& terminalCharPtrs)
{
    auto asciiSeparators = getSeparators<CharPointer_ASCII>();

    SeparatorStrings result;

    for (const auto& block : asciiSeparators.terminals)
        result.terminals.push_back (convert<CharPointer_ASCII, CharPointerType> (block));

    for (const auto& block : asciiSeparators.nulls)
        result.nulls.push_back (convert<CharPointer_ASCII, CharPointerType> (block));

    for (auto& t : terminalCharPtrs)
    {
        const auto block = memoryBlockFromCharPtr<CharPointerType> ((typename CharPointerType::CharType*) t.data());

        for (auto vec : { &result.terminals, &result.nulls })
            vec->push_back (block);
    }

    return result;
}

template <>
SeparatorStrings getSeparators<CharPointer_UTF16>()
{
    const std::vector<std::vector<char16_t>> terminalCharPtrs {
        { 0x0 },
        { 0x0076, 0x0 },                        // v
        { 0x20ac, 0x0 },                        // €
        { 0xd801, 0xdc37, 0x0 },                // 𐐷
        { 0x0065, 0xd83d, 0xde03, 0x0 },        // e😃
        { 0xd83c, 0xdfc1, 0xd83d, 0xde97, 0x0 } // 🏁🚗
    };

    return prefixWithAsciiSeparators<CharPointer_UTF16> (terminalCharPtrs);
}

template <>
SeparatorStrings getSeparators<CharPointer_UTF32>()
{
    const std::vector<std::vector<char32_t>> terminalCharPtrs = {
        { 0x00000076, 0x0 },             // v
        { 0x000020aC, 0x0 },             // €
        { 0x00010437, 0x0 },             // 𐐷
        { 0x00000065, 0x0001f603, 0x0 }, // e😃
        { 0x0001f3c1, 0x0001f697, 0x0 }  // 🏁🚗
    };

    return prefixWithAsciiSeparators<CharPointer_UTF32> (terminalCharPtrs);
}

template <typename TestFunction>
void withAllPrefixesAndSuffixes (const std::vector<MemoryBlock>& prefixes,
                                 const std::vector<MemoryBlock>& suffixes,
                                 const std::vector<MemoryBlock>& testValues,
                                 TestFunction&& test)
{
    for (const auto& prefix : prefixes)
    {
        for (const auto& testValue : testValues)
        {
            MemoryBlock testBlock = prefix;
            testBlock.append (testValue.getData(), testValue.getSize());

            for (const auto& suffix : suffixes)
            {
                MemoryBlock data = testBlock;
                data.append (suffix.getData(), suffix.getSize());

                test (data, suffix);
            }
        }
    }
}

} // namespace

// Specialized versions for UTF8, UTF16, and UTF32 are implemented similarly...

template <typename CharPointerType>
class CharacterFunctionsTests : public ::testing::Test
{
protected:
    using CharType = typename CharPointerType::CharType;

    static SeparatorStrings separators;

    static SeparatorStrings initSeparators()
    {
        return getSeparators<CharPointerType>();
    }

    void testReadDoubleValue()
    {
        const std::pair<const char*, double> trials[] =
        {
            // Integers
            CHARPTR_DOUBLE_PAIR_COMBOS (0),
            CHARPTR_DOUBLE_PAIR_COMBOS (3),
            CHARPTR_DOUBLE_PAIR_COMBOS (4931),
            CHARPTR_DOUBLE_PAIR_COMBOS (5000),
            CHARPTR_DOUBLE_PAIR_COMBOS (9862097),

            // Floating point numbers
            CHARPTR_DOUBLE_PAIR_COMBOS (0.),
            CHARPTR_DOUBLE_PAIR_COMBOS (9.),
            CHARPTR_DOUBLE_PAIR_COMBOS (7.000),
            CHARPTR_DOUBLE_PAIR_COMBOS (0.2),
            CHARPTR_DOUBLE_PAIR_COMBOS (.298630),
            CHARPTR_DOUBLE_PAIR_COMBOS (1.118),
            CHARPTR_DOUBLE_PAIR_COMBOS (0.9000),
            CHARPTR_DOUBLE_PAIR_COMBOS (0.0000001),
            CHARPTR_DOUBLE_PAIR_COMBOS (500.0000001),
            CHARPTR_DOUBLE_PAIR_COMBOS (9862098.2398604),

            // Exponents
            CHARPTR_DOUBLE_PAIR_COMBOS (0e0),
            CHARPTR_DOUBLE_PAIR_COMBOS (0.e0),
            CHARPTR_DOUBLE_PAIR_COMBOS (0.00000e0),
            CHARPTR_DOUBLE_PAIR_COMBOS (.0e7),
            CHARPTR_DOUBLE_PAIR_COMBOS (0e-5),
            CHARPTR_DOUBLE_PAIR_COMBOS (2E0),
            CHARPTR_DOUBLE_PAIR_COMBOS (4.E0),
            CHARPTR_DOUBLE_PAIR_COMBOS (1.2000000E0),
            CHARPTR_DOUBLE_PAIR_COMBOS (1.2000000E6),
            CHARPTR_DOUBLE_PAIR_COMBOS (.398e3),
            CHARPTR_DOUBLE_PAIR_COMBOS (10e10),
            CHARPTR_DOUBLE_PAIR_COMBOS (1.4962e+2),
            CHARPTR_DOUBLE_PAIR_COMBOS (3198693.0973e4),
            CHARPTR_DOUBLE_PAIR_COMBOS (10973097.2087E-4),
            CHARPTR_DOUBLE_PAIR_COMBOS (1.3986e00006),
            CHARPTR_DOUBLE_PAIR_COMBOS (2087.3087e+00006),
            CHARPTR_DOUBLE_PAIR_COMBOS (6.0872e-00006),

            CHARPTR_DOUBLE_PAIR_COMBOS (1.7976931348623157e+308),
            CHARPTR_DOUBLE_PAIR_COMBOS (2.2250738585072014e-308),

            // Too many sig figs. The parsing routine on MinGW gets the last
            // significant figure wrong.
            CHARPTR_DOUBLE_PAIR_COMBOS (17654321098765432.9),
            CHARPTR_DOUBLE_PAIR_COMBOS (183456789012345678.9),
            CHARPTR_DOUBLE_PAIR_COMBOS (1934567890123456789.9),
            CHARPTR_DOUBLE_PAIR_COMBOS (20345678901234567891.9),
            CHARPTR_DOUBLE_PAIR_COMBOS (10000000000000000303786028427003666890752.000000),
            CHARPTR_DOUBLE_PAIR_COMBOS (10000000000000000303786028427003666890752e3),
            CHARPTR_DOUBLE_PAIR_COMBOS (10000000000000000303786028427003666890752e100),
            CHARPTR_DOUBLE_PAIR_COMBOS (10000000000000000303786028427003666890752.000000e-5),
            CHARPTR_DOUBLE_PAIR_COMBOS (10000000000000000303786028427003666890752.000005e-40),

            CHARPTR_DOUBLE_PAIR_COMBOS (1.23456789012345678901234567890),
            CHARPTR_DOUBLE_PAIR_COMBOS (1.23456789012345678901234567890e-111),
        };

        auto asciiToMemoryBlock = [] (const char* asciiPtr, bool removeNullTerminator)
        {
            auto block = memoryBlockFromCharPtr<CharPointer_ASCII> (asciiPtr);
            return convert<CharPointer_ASCII, CharPointerType> (block, removeNullTerminator);
        };

        const auto separators = getSeparators<CharPointerType>();

        for (const auto& trial : trials)
        {
            for (const auto& terminal : separators.terminals)
            {
                MemoryBlock data { asciiToMemoryBlock (trial.first, true) };
                data.append (terminal.getData(), terminal.getSize());

                CharPointerType charPtr { (CharType*) data.getData() };
                EXPECT_EQ (CharacterFunctions::readDoubleValue (charPtr), trial.second);
                EXPECT_TRUE (*charPtr == *(CharPointerType ((CharType*) terminal.getData())));
            }
        }

        auto asciiToMemoryBlocks = [&] (const std::vector<const char*>& asciiPtrs, bool removeNullTerminator)
        {
            std::vector<MemoryBlock> result;

            for (auto* ptr : asciiPtrs)
                result.push_back (asciiToMemoryBlock (ptr, removeNullTerminator));

            return result;
        };

        std::vector<const char*> prefixCharPtrs = { "", "+", "-" };
        const auto prefixes = asciiToMemoryBlocks (prefixCharPtrs, true);

        {
            std::vector<const char*> nanCharPtrs = { "NaN", "nan", "NAN", "naN" };
            auto nans = asciiToMemoryBlocks (nanCharPtrs, true);

            withAllPrefixesAndSuffixes (prefixes, separators.terminals, nans, [this] (const MemoryBlock& data, const MemoryBlock& suffix)
                                        {
                                            CharPointerType charPtr { (CharType*) data.getData() };
                                            EXPECT_TRUE (std::isnan (CharacterFunctions::readDoubleValue (charPtr)));
                                            EXPECT_TRUE (*charPtr == *(CharPointerType ((CharType*) suffix.getData())));
                                        });
        }

        {
            std::vector<const char*> infCharPtrs = { "Inf", "inf", "INF", "InF", "1.0E1024", "1.23456789012345678901234567890e123456789" };
            auto infs = asciiToMemoryBlocks (infCharPtrs, true);

            withAllPrefixesAndSuffixes (prefixes, separators.terminals, infs, [this] (const MemoryBlock& data, const MemoryBlock& suffix)
                                        {
                                            CharPointerType charPtr { (CharType*) data.getData() };
                                            auto expected = charPtr[0] == '-' ? -std::numeric_limits<double>::infinity()
                                                                              : std::numeric_limits<double>::infinity();
                                            EXPECT_EQ (CharacterFunctions::readDoubleValue (charPtr), expected);
                                            EXPECT_TRUE (*charPtr == *(CharPointerType ((CharType*) suffix.getData())));
                                        });
        }

        {
            std::vector<const char*> zeroCharPtrs = { "1.0E-400", "1.23456789012345678901234567890e-123456789" };
            auto zeros = asciiToMemoryBlocks (zeroCharPtrs, true);

            withAllPrefixesAndSuffixes (prefixes, separators.terminals, zeros, [this] (const MemoryBlock& data, const MemoryBlock& suffix)
                                        {
                                            CharPointerType charPtr { (CharType*) data.getData() };
                                            auto expected = charPtr[0] == '-' ? -0.0 : 0.0;
                                            EXPECT_EQ (CharacterFunctions::readDoubleValue (charPtr), expected);
                                            EXPECT_TRUE (*charPtr == *(CharPointerType ((CharType*) suffix.getData())));
                                        });
        }

        {
            for (const auto& n : separators.nulls)
            {
                MemoryBlock data { n.getData(), n.getSize() };
                CharPointerType charPtr { (CharType*) data.getData() };
                EXPECT_EQ (CharacterFunctions::readDoubleValue (charPtr), 0.0);
                EXPECT_TRUE (charPtr == CharPointerType { (CharType*) data.getData() }.findEndOfWhitespace());
            }
        }
    }
};

template <typename CharPointerType>
SeparatorStrings CharacterFunctionsTests<CharPointerType>::separators = CharacterFunctionsTests<CharPointerType>::initSeparators();

// Register tests for all character pointer types
using CharacterPointerTypes = ::testing::Types<juce::CharPointer_ASCII, juce::CharPointer_UTF8, juce::CharPointer_UTF16, juce::CharPointer_UTF32>;
TYPED_TEST_SUITE(CharacterFunctionsTests, CharacterPointerTypes);

TYPED_TEST(CharacterFunctionsTests, ReadDoubleValue)
{
    this->testReadDoubleValue();
}
