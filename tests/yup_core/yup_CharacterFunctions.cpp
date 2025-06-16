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

#define QUOTE(x) #x
#define STR(value) QUOTE (value)
#define ASYM_CHARPTR_DOUBLE_PAIR(str, value) std::pair<const char*, double> (STR (str), value)
#define CHARPTR_DOUBLE_PAIR(value) ASYM_CHARPTR_DOUBLE_PAIR (value, value)
#define CHARPTR_DOUBLE_PAIR_COMBOS(value)                \
    CHARPTR_DOUBLE_PAIR (value),                         \
        CHARPTR_DOUBLE_PAIR (-value),                    \
        ASYM_CHARPTR_DOUBLE_PAIR (+value, value),        \
        ASYM_CHARPTR_DOUBLE_PAIR (000000##value, value), \
        ASYM_CHARPTR_DOUBLE_PAIR (+000##value, value),   \
        ASYM_CHARPTR_DOUBLE_PAIR (-0##value, -value)

namespace
{

template <class CharPointerType>
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

template <class FromCharPointerType, class ToCharPointerType>
MemoryBlock convert (const MemoryBlock& source, bool removeNullTerminator = false)
{
    using ToCharType = typename ToCharPointerType ::CharType;
    using FromCharType = typename FromCharPointerType::CharType;

    FromCharPointerType sourcePtr { (FromCharType*) source.getData() };

    std::vector<yup_wchar> sourceChars;
    size_t requiredSize = 0;
    yup_wchar c;

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

template <class CharPointerType>
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

template <class CharPointerType, class StorageType>
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

template <class TestFunction>
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

template <class CharPointerType>
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
        const std::pair<const char*, double> trials[] = {
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
using CharacterPointerTypes = ::testing::Types<yup::CharPointer_ASCII, yup::CharPointer_UTF8, yup::CharPointer_UTF16, yup::CharPointer_UTF32>;
TYPED_TEST_SUITE (CharacterFunctionsTests, CharacterPointerTypes);

TYPED_TEST (CharacterFunctionsTests, ReadDoubleValue)
{
    this->testReadDoubleValue();
}

// Additional tests for all CharacterFunctions

TEST (CharacterFunctionsGeneralTests, ToUpperCase)
{
    // Basic ASCII
    EXPECT_EQ (CharacterFunctions::toUpperCase ('a'), 'A');
    EXPECT_EQ (CharacterFunctions::toUpperCase ('z'), 'Z');
    EXPECT_EQ (CharacterFunctions::toUpperCase ('A'), 'A');
    EXPECT_EQ (CharacterFunctions::toUpperCase ('Z'), 'Z');
    EXPECT_EQ (CharacterFunctions::toUpperCase ('0'), '0');
    EXPECT_EQ (CharacterFunctions::toUpperCase ('!'), '!');

    // Latin-1 Supplement
    EXPECT_EQ (CharacterFunctions::toUpperCase (L'à'), L'À');
    EXPECT_EQ (CharacterFunctions::toUpperCase (L'é'), L'É');
    EXPECT_EQ (CharacterFunctions::toUpperCase (L'ñ'), L'Ñ');
    EXPECT_EQ (CharacterFunctions::toUpperCase (L'ÿ'), L'Ÿ');

    // Latin Extended
    EXPECT_EQ (CharacterFunctions::toUpperCase (L'ā'), L'Ā');
    EXPECT_EQ (CharacterFunctions::toUpperCase (L'ě'), L'Ě');
    EXPECT_EQ (CharacterFunctions::toUpperCase (L'ő'), L'Ő');

    // Greek
    EXPECT_EQ (CharacterFunctions::toUpperCase (L'α'), L'Α');
    EXPECT_EQ (CharacterFunctions::toUpperCase (L'ω'), L'Ω');

    // Cyrillic
    EXPECT_EQ (CharacterFunctions::toUpperCase (L'а'), L'А');
    EXPECT_EQ (CharacterFunctions::toUpperCase (L'я'), L'Я');

    // Special case: dotless i
    EXPECT_EQ (CharacterFunctions::toUpperCase (L'ı'), L'I');
}

TEST (CharacterFunctionsGeneralTests, ToLowerCase)
{
    // Basic ASCII
    EXPECT_EQ (CharacterFunctions::toLowerCase ('A'), 'a');
    EXPECT_EQ (CharacterFunctions::toLowerCase ('Z'), 'z');
    EXPECT_EQ (CharacterFunctions::toLowerCase ('a'), 'a');
    EXPECT_EQ (CharacterFunctions::toLowerCase ('z'), 'z');
    EXPECT_EQ (CharacterFunctions::toLowerCase ('0'), '0');
    EXPECT_EQ (CharacterFunctions::toLowerCase ('!'), '!');

    // Latin-1 Supplement
    EXPECT_EQ (CharacterFunctions::toLowerCase (L'À'), L'à');
    EXPECT_EQ (CharacterFunctions::toLowerCase (L'É'), L'é');
    EXPECT_EQ (CharacterFunctions::toLowerCase (L'Ñ'), L'ñ');
    EXPECT_EQ (CharacterFunctions::toLowerCase (L'Ÿ'), L'ÿ');

    // Latin Extended
    EXPECT_EQ (CharacterFunctions::toLowerCase (L'Ā'), L'ā');
    EXPECT_EQ (CharacterFunctions::toLowerCase (L'Ě'), L'ě');
    EXPECT_EQ (CharacterFunctions::toLowerCase (L'Ő'), L'ő');

    // Greek
    EXPECT_EQ (CharacterFunctions::toLowerCase (L'Α'), L'α');
    EXPECT_EQ (CharacterFunctions::toLowerCase (L'Ω'), L'ω');

    // Cyrillic
    EXPECT_EQ (CharacterFunctions::toLowerCase (L'А'), L'а');
    EXPECT_EQ (CharacterFunctions::toLowerCase (L'Я'), L'я');

    // Special case: capital I to dotted i
    EXPECT_EQ (CharacterFunctions::toLowerCase (L'İ'), L'i');
}

TEST (CharacterFunctionsGeneralTests, IsUpperCase)
{
    // ASCII uppercase
    EXPECT_TRUE (CharacterFunctions::isUpperCase ('A'));
    EXPECT_TRUE (CharacterFunctions::isUpperCase ('Z'));
    EXPECT_FALSE (CharacterFunctions::isUpperCase ('a'));
    EXPECT_FALSE (CharacterFunctions::isUpperCase ('z'));
    EXPECT_FALSE (CharacterFunctions::isUpperCase ('0'));
    EXPECT_FALSE (CharacterFunctions::isUpperCase ('!'));

    // Extended characters
    EXPECT_TRUE (CharacterFunctions::isUpperCase (L'À'));
    EXPECT_TRUE (CharacterFunctions::isUpperCase (L'Ñ'));
    EXPECT_TRUE (CharacterFunctions::isUpperCase (L'Ω'));
    EXPECT_TRUE (CharacterFunctions::isUpperCase (L'Я'));

    EXPECT_FALSE (CharacterFunctions::isUpperCase (L'à'));
    EXPECT_FALSE (CharacterFunctions::isUpperCase (L'ñ'));
    EXPECT_FALSE (CharacterFunctions::isUpperCase (L'ω'));
    EXPECT_FALSE (CharacterFunctions::isUpperCase (L'я'));
}

TEST (CharacterFunctionsGeneralTests, IsLowerCase)
{
    // ASCII lowercase
    EXPECT_TRUE (CharacterFunctions::isLowerCase ('a'));
    EXPECT_TRUE (CharacterFunctions::isLowerCase ('z'));
    EXPECT_FALSE (CharacterFunctions::isLowerCase ('A'));
    EXPECT_FALSE (CharacterFunctions::isLowerCase ('Z'));
    EXPECT_FALSE (CharacterFunctions::isLowerCase ('0'));
    EXPECT_FALSE (CharacterFunctions::isLowerCase ('!'));

    // Extended characters
    EXPECT_TRUE (CharacterFunctions::isLowerCase (L'à'));
    EXPECT_TRUE (CharacterFunctions::isLowerCase (L'ñ'));
    EXPECT_TRUE (CharacterFunctions::isLowerCase (L'ω'));
    EXPECT_TRUE (CharacterFunctions::isLowerCase (L'я'));

    EXPECT_FALSE (CharacterFunctions::isLowerCase (L'À'));
    EXPECT_FALSE (CharacterFunctions::isLowerCase (L'Ñ'));
    EXPECT_FALSE (CharacterFunctions::isLowerCase (L'Ω'));
    EXPECT_FALSE (CharacterFunctions::isLowerCase (L'Я'));
}

TEST (CharacterFunctionsGeneralTests, IsWhitespace)
{
    // char version
    EXPECT_TRUE (CharacterFunctions::isWhitespace (' '));
    EXPECT_TRUE (CharacterFunctions::isWhitespace ('\t'));
    EXPECT_TRUE (CharacterFunctions::isWhitespace ('\n'));
    EXPECT_TRUE (CharacterFunctions::isWhitespace ('\r'));
    EXPECT_TRUE (CharacterFunctions::isWhitespace ('\v'));
    EXPECT_TRUE (CharacterFunctions::isWhitespace ('\f'));
    EXPECT_FALSE (CharacterFunctions::isWhitespace ('a'));
    EXPECT_FALSE (CharacterFunctions::isWhitespace ('0'));
    EXPECT_FALSE (CharacterFunctions::isWhitespace ('!'));

    // yup_wchar version
    EXPECT_TRUE (CharacterFunctions::isWhitespace (yup_wchar (L' ')));
    EXPECT_TRUE (CharacterFunctions::isWhitespace (yup_wchar (L'\t')));
    EXPECT_TRUE (CharacterFunctions::isWhitespace (yup_wchar (L'\n')));
    EXPECT_TRUE (CharacterFunctions::isWhitespace (yup_wchar (L'\r')));
    EXPECT_TRUE (CharacterFunctions::isWhitespace (yup_wchar (L'\v')));
    EXPECT_TRUE (CharacterFunctions::isWhitespace (yup_wchar (L'\f')));
    //EXPECT_TRUE (CharacterFunctions::isWhitespace (yup_wchar (L'\u00A0'))); // Non-breaking space
    //EXPECT_TRUE (CharacterFunctions::isWhitespace (yup_wchar (L'\u2000'))); // En quad
    //EXPECT_TRUE (CharacterFunctions::isWhitespace (yup_wchar (L'\u2001'))); // Em quad
    EXPECT_FALSE (CharacterFunctions::isWhitespace (yup_wchar (L'a')));
    EXPECT_FALSE (CharacterFunctions::isWhitespace (yup_wchar (L'0')));
}

TEST (CharacterFunctionsGeneralTests, IsDigit)
{
    // char version
    for (char c = '0'; c <= '9'; ++c)
        EXPECT_TRUE (CharacterFunctions::isDigit (c));

    EXPECT_FALSE (CharacterFunctions::isDigit ('a'));
    EXPECT_FALSE (CharacterFunctions::isDigit ('A'));
    EXPECT_FALSE (CharacterFunctions::isDigit (' '));
    EXPECT_FALSE (CharacterFunctions::isDigit ('!'));

    // yup_wchar version
    for (auto c = yup_wchar (L'0'); c <= yup_wchar (L'9'); ++c)
        EXPECT_TRUE (CharacterFunctions::isDigit (c));

    EXPECT_FALSE (CharacterFunctions::isDigit (yup_wchar (L'a')));
    EXPECT_FALSE (CharacterFunctions::isDigit (yup_wchar (L'A')));
    EXPECT_FALSE (CharacterFunctions::isDigit (yup_wchar (L' ')));

    // Unicode digits from other scripts (should return true if iswdigit supports them)
    // Note: The behavior may vary depending on the locale and platform
}

TEST (CharacterFunctionsGeneralTests, IsLetter)
{
    // char version
    for (char c = 'a'; c <= 'z'; ++c)
        EXPECT_TRUE (CharacterFunctions::isLetter (c));

    for (char c = 'A'; c <= 'Z'; ++c)
        EXPECT_TRUE (CharacterFunctions::isLetter (c));

    EXPECT_FALSE (CharacterFunctions::isLetter ('0'));
    EXPECT_FALSE (CharacterFunctions::isLetter ('9'));
    EXPECT_FALSE (CharacterFunctions::isLetter (' '));
    EXPECT_FALSE (CharacterFunctions::isLetter ('!'));

    // yup_wchar version
    for (auto c = yup_wchar (L'a'); c <= yup_wchar (L'z'); ++c)
        EXPECT_TRUE (CharacterFunctions::isLetter (c));

    for (auto c = yup_wchar (L'A'); c <= yup_wchar (L'Z'); ++c)
        EXPECT_TRUE (CharacterFunctions::isLetter (c));

    // Extended characters
    //EXPECT_TRUE (CharacterFunctions::isLetter (yup_wchar (L'á')));
    //EXPECT_TRUE (CharacterFunctions::isLetter (yup_wchar (L'Ñ')));
    //EXPECT_TRUE (CharacterFunctions::isLetter (yup_wchar (L'ω')));
    //EXPECT_TRUE (CharacterFunctions::isLetter (yup_wchar (L'Я')));

    EXPECT_FALSE (CharacterFunctions::isLetter (yup_wchar (L'0')));
    EXPECT_FALSE (CharacterFunctions::isLetter (yup_wchar (L' ')));
}

TEST (CharacterFunctionsGeneralTests, IsLetterOrDigit)
{
    // char version
    for (char c = 'a'; c <= 'z'; ++c)
        EXPECT_TRUE (CharacterFunctions::isLetterOrDigit (c));

    for (char c = 'A'; c <= 'Z'; ++c)
        EXPECT_TRUE (CharacterFunctions::isLetterOrDigit (c));

    for (char c = '0'; c <= '9'; ++c)
        EXPECT_TRUE (CharacterFunctions::isLetterOrDigit (c));

    EXPECT_FALSE (CharacterFunctions::isLetterOrDigit (' '));
    EXPECT_FALSE (CharacterFunctions::isLetterOrDigit ('!'));
    EXPECT_FALSE (CharacterFunctions::isLetterOrDigit ('@'));

    // yup_wchar version
    for (auto c = yup_wchar (L'a'); c <= yup_wchar (L'z'); ++c)
        EXPECT_TRUE (CharacterFunctions::isLetterOrDigit (c));

    for (auto c = yup_wchar (L'A'); c <= yup_wchar (L'Z'); ++c)
        EXPECT_TRUE (CharacterFunctions::isLetterOrDigit (c));

    for (auto c = yup_wchar (L'0'); c <= yup_wchar (L'9'); ++c)
        EXPECT_TRUE (CharacterFunctions::isLetterOrDigit (c));

    // Extended characters
    //EXPECT_TRUE (CharacterFunctions::isLetterOrDigit (yup_wchar (L'á')));
    //EXPECT_TRUE (CharacterFunctions::isLetterOrDigit (yup_wchar (L'Ω')));

    EXPECT_FALSE (CharacterFunctions::isLetterOrDigit (yup_wchar (L' ')));
    EXPECT_FALSE (CharacterFunctions::isLetterOrDigit (yup_wchar (L'!')));
}

TEST (CharacterFunctionsGeneralTests, IsPrintable)
{
    // char version
    for (char c = ' '; c <= '~'; ++c)
        EXPECT_TRUE (CharacterFunctions::isPrintable (c));

    EXPECT_FALSE (CharacterFunctions::isPrintable ('\0'));
    EXPECT_FALSE (CharacterFunctions::isPrintable ('\n'));
    EXPECT_FALSE (CharacterFunctions::isPrintable ('\t'));
    EXPECT_FALSE (CharacterFunctions::isPrintable ('\r'));
    //EXPECT_FALSE (CharacterFunctions::isPrintable (0x7F)); // DEL

    // yup_wchar version
    for (auto c = yup_wchar (L' '); c <= yup_wchar (L'~'); ++c)
        EXPECT_TRUE (CharacterFunctions::isPrintable (c));

    //EXPECT_TRUE (CharacterFunctions::isPrintable (yup_wchar (L'á')));
    //EXPECT_TRUE (CharacterFunctions::isPrintable (yup_wchar (L'€')));
    //EXPECT_TRUE (CharacterFunctions::isPrintable (yup_wchar (L'♪')));

    EXPECT_FALSE (CharacterFunctions::isPrintable (yup_wchar (L'\0')));
    EXPECT_FALSE (CharacterFunctions::isPrintable (yup_wchar (L'\n')));
    EXPECT_FALSE (CharacterFunctions::isPrintable (yup_wchar (L'\t')));
}

TEST (CharacterFunctionsGeneralTests, GetHexDigitValue)
{
    // Valid hex digits
    EXPECT_EQ (CharacterFunctions::getHexDigitValue ('0'), 0);
    EXPECT_EQ (CharacterFunctions::getHexDigitValue ('1'), 1);
    EXPECT_EQ (CharacterFunctions::getHexDigitValue ('5'), 5);
    EXPECT_EQ (CharacterFunctions::getHexDigitValue ('9'), 9);
    EXPECT_EQ (CharacterFunctions::getHexDigitValue ('a'), 10);
    EXPECT_EQ (CharacterFunctions::getHexDigitValue ('A'), 10);
    EXPECT_EQ (CharacterFunctions::getHexDigitValue ('b'), 11);
    EXPECT_EQ (CharacterFunctions::getHexDigitValue ('B'), 11);
    EXPECT_EQ (CharacterFunctions::getHexDigitValue ('f'), 15);
    EXPECT_EQ (CharacterFunctions::getHexDigitValue ('F'), 15);

    // Invalid hex digits
    EXPECT_EQ (CharacterFunctions::getHexDigitValue ('g'), -1);
    EXPECT_EQ (CharacterFunctions::getHexDigitValue ('G'), -1);
    EXPECT_EQ (CharacterFunctions::getHexDigitValue ('z'), -1);
    EXPECT_EQ (CharacterFunctions::getHexDigitValue ('!'), -1);
    EXPECT_EQ (CharacterFunctions::getHexDigitValue (' '), -1);

    // Wide characters
    EXPECT_EQ (CharacterFunctions::getHexDigitValue (L'0'), 0);
    EXPECT_EQ (CharacterFunctions::getHexDigitValue (L'9'), 9);
    EXPECT_EQ (CharacterFunctions::getHexDigitValue (L'a'), 10);
    EXPECT_EQ (CharacterFunctions::getHexDigitValue (L'F'), 15);
    EXPECT_EQ (CharacterFunctions::getHexDigitValue (L'€'), -1);
}

TEST (CharacterFunctionsGeneralTests, GetUnicodeCharFromWindows1252Codepage)
{
    // Values below 0x80 should pass through unchanged
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x00), 0x00);
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x41), 0x41); // 'A'
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x7F), 0x7F);

    // Values from 0xA0 and above should also pass through
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0xA0), 0xA0);
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0xFF), 0xFF);

    // Special Windows-1252 mappings (0x80-0x9F)
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x80), 0x20AC); // Euro sign
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x82), 0x201A); // Single low-9 quotation mark
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x83), 0x0192); // Latin small letter f with hook
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x84), 0x201E); // Double low-9 quotation mark
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x85), 0x2026); // Horizontal ellipsis
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x86), 0x2020); // Dagger
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x87), 0x2021); // Double dagger
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x88), 0x02C6); // Modifier letter circumflex accent
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x89), 0x2030); // Per mille sign
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x8A), 0x0160); // Latin capital letter S with caron
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x8B), 0x2039); // Single left-pointing angle quotation mark
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x8C), 0x0152); // Latin capital ligature OE
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x8E), 0x017D); // Latin capital letter Z with caron
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x91), 0x2018); // Left single quotation mark
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x92), 0x2019); // Right single quotation mark
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x93), 0x201C); // Left double quotation mark
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x94), 0x201D); // Right double quotation mark
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x95), 0x2022); // Bullet
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x96), 0x2013); // En dash
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x97), 0x2014); // Em dash
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x98), 0x02DC); // Small tilde
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x99), 0x2122); // Trade mark sign
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x9A), 0x0161); // Latin small letter s with caron
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x9B), 0x203A); // Single right-pointing angle quotation mark
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x9C), 0x0153); // Latin small ligature oe
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x9E), 0x017E); // Latin small letter z with caron
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x9F), 0x0178); // Latin capital letter Y with diaeresis

    // Undefined characters (0x81, 0x8D, 0x8F, 0x90, 0x9D) should map to 0x0007
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x81), 0x0007);
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x8D), 0x0007);
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x8F), 0x0007);
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x90), 0x0007);
    EXPECT_EQ (CharacterFunctions::getUnicodeCharFromWindows1252Codepage (0x9D), 0x0007);
}

// Test case conversion consistency
TEST (CharacterFunctionsGeneralTests, CaseConversionConsistency)
{
    // Test that converting to upper and then to lower returns the original for lowercase letters
    for (auto c = yup_wchar (L'a'); c <= yup_wchar (L'z'); ++c)
    {
        auto upper = CharacterFunctions::toUpperCase (c);
        auto lower = CharacterFunctions::toLowerCase (upper);
        EXPECT_EQ (lower, c);
    }

    // Test that converting to lower and then to upper returns the original for uppercase letters
    for (auto c = yup_wchar (L'A'); c <= yup_wchar (L'Z'); ++c)
    {
        auto lower = CharacterFunctions::toLowerCase (c);
        auto upper = CharacterFunctions::toUpperCase (lower);
        EXPECT_EQ (upper, c);
    }

    // Test some extended characters
    const yup_wchar testChars[] = { L'à', L'é', L'ñ', L'α', L'ω', L'а', L'я' };
    for (auto c : testChars)
    {
        auto upper = CharacterFunctions::toUpperCase (c);
        auto lower = CharacterFunctions::toLowerCase (upper);
        EXPECT_EQ (lower, c);
    }
}
