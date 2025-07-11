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

namespace yup
{
extern String reduceLengthOfFloatString (const String& input);
extern String serialiseDouble (double input, int maxDecimalPlaces = 0);
} // namespace yup

class StringTests : public ::testing::Test
{
protected:
    template <class CharPointerType>
    struct TestUTFConversion
    {
        static void test()
        {
            Random r;

            String s (createRandomWideCharString (r));

            typename CharPointerType::CharType buffer[300];

            memset (buffer, 0xff, sizeof (buffer));
            CharPointerType (buffer).writeAll (s.toUTF32());
            EXPECT_EQ (String (CharPointerType (buffer)), s);

            memset (buffer, 0xff, sizeof (buffer));
            CharPointerType (buffer).writeAll (s.toUTF16());
            EXPECT_EQ (String (CharPointerType (buffer)), s);

            memset (buffer, 0xff, sizeof (buffer));
            CharPointerType (buffer).writeAll (s.toUTF8());
            EXPECT_EQ (String (CharPointerType (buffer)), s);

            EXPECT_TRUE (CharPointerType::isValidString (buffer, (int) strlen ((const char*) buffer)));
        }
    };

    static String createRandomWideCharString (Random& r)
    {
        yup_wchar buffer[50] = { 0 };

        for (int i = 0; i < numElementsInArray (buffer) - 1; ++i)
        {
            if (r.nextBool())
            {
                do
                {
                    buffer[i] = (yup_wchar) (1 + r.nextInt (0x10ffff - 1));
                } while (! CharPointer_UTF16::canRepresent (buffer[i]));
            }
            else
                buffer[i] = (yup_wchar) (1 + r.nextInt (0xff));
        }

        return CharPointer_UTF32 (buffer);
    }
};

TEST_F (StringTests, Basics)
{
    EXPECT_TRUE (String().length() == 0);
    EXPECT_TRUE (String() == String());
    String s1, s2 ("abcd");
    EXPECT_TRUE (s1.isEmpty() && ! s1.isNotEmpty());
    EXPECT_TRUE (s2.isNotEmpty() && ! s2.isEmpty());
    EXPECT_TRUE (s2.length() == 4);
    s1 = "abcd";
    EXPECT_TRUE (s2 == s1 && s1 == s2);
    EXPECT_TRUE (s1 == "abcd" && s1 == L"abcd");
    EXPECT_TRUE (String ("abcd") == String (L"abcd"));
    EXPECT_TRUE (String ("abcdefg", 4) == L"abcd");
    EXPECT_TRUE (String ("abcdefg", 4) == String (L"abcdefg", 4));
    EXPECT_TRUE (String::charToString ('x') == "x");
    EXPECT_TRUE (String::charToString (0) == String());
    EXPECT_TRUE (s2 + "e" == "abcde" && s2 + 'e' == "abcde");
    EXPECT_TRUE (s2 + L'e' == "abcde" && s2 + L"e" == "abcde");
    EXPECT_TRUE (s1.equalsIgnoreCase ("abcD") && s1 < "abce" && s1 > "abbb");
    EXPECT_TRUE (s1.startsWith ("ab") && s1.startsWith ("abcd") && ! s1.startsWith ("abcde"));
    EXPECT_TRUE (s1.startsWithIgnoreCase ("aB") && s1.endsWithIgnoreCase ("CD"));
    EXPECT_TRUE (s1.endsWith ("bcd") && ! s1.endsWith ("aabcd"));
    EXPECT_EQ (s1.indexOf (String()), 0);
    EXPECT_EQ (s1.indexOfIgnoreCase (String()), 0);
    EXPECT_TRUE (s1.startsWith (String()) && s1.endsWith (String()) && s1.contains (String()));
    EXPECT_TRUE (s1.contains ("cd") && s1.contains ("ab") && s1.contains ("abcd"));
    EXPECT_TRUE (s1.containsChar ('a'));
    EXPECT_TRUE (! s1.containsChar ('x'));
    EXPECT_TRUE (! s1.containsChar (0));
    EXPECT_TRUE (String ("abc foo bar").containsWholeWord ("abc") && String ("abc foo bar").containsWholeWord ("abc"));
}

TEST_F (StringTests, Operations)
{
    String s ("012345678");
    EXPECT_TRUE (s.hashCode() != 0);
    EXPECT_TRUE (s.hashCode64() != 0);
    EXPECT_TRUE (s.hashCode() != (s + s).hashCode());
    EXPECT_TRUE (s.hashCode64() != (s + s).hashCode64());
    EXPECT_TRUE (s.compare (String ("012345678")) == 0);
    EXPECT_TRUE (s.compare (String ("012345679")) < 0);
    EXPECT_TRUE (s.compare (String ("012345676")) > 0);
    EXPECT_TRUE (String ("a").compareNatural ("A") == 0);
    EXPECT_TRUE (String ("A").compareNatural ("B") < 0);
    EXPECT_TRUE (String ("a").compareNatural ("B") < 0);
    EXPECT_TRUE (String ("10").compareNatural ("2") > 0);
    EXPECT_TRUE (String ("Abc 10").compareNatural ("aBC 2") > 0);
    EXPECT_TRUE (String ("Abc 1").compareNatural ("aBC 2") < 0);
    EXPECT_TRUE (s.substring (2, 3) == String::charToString (s[2]));
    EXPECT_TRUE (s.substring (0, 1) == String::charToString (s[0]));
    EXPECT_TRUE (s.getLastCharacter() == s[s.length() - 1]);
    EXPECT_TRUE (String::charToString (s.getLastCharacter()) == s.getLastCharacters (1));
    EXPECT_TRUE (s.substring (0, 3) == L"012");
    EXPECT_TRUE (s.substring (0, 100) == s);
    EXPECT_TRUE (s.substring (-1, 100) == s);
    EXPECT_TRUE (s.substring (3) == "345678");
    EXPECT_TRUE (s.indexOf (String (L"45")) == 4);
    EXPECT_TRUE (String ("444445").indexOf ("45") == 4);
    EXPECT_TRUE (String ("444445").lastIndexOfChar ('4') == 4);
    EXPECT_TRUE (String ("45454545x").lastIndexOf (String (L"45")) == 6);
    EXPECT_TRUE (String ("45454545x").lastIndexOfAnyOf ("456") == 7);
    EXPECT_TRUE (String ("45454545x").lastIndexOfAnyOf (String (L"456x")) == 8);
    EXPECT_TRUE (String ("abABaBaBa").lastIndexOfIgnoreCase ("aB") == 6);
    EXPECT_TRUE (s.indexOfChar (L'4') == 4);
    EXPECT_TRUE (s + s == "012345678012345678");
    EXPECT_TRUE (s.startsWith (s));
    EXPECT_TRUE (s.startsWith (s.substring (0, 4)));
    EXPECT_TRUE (s.startsWith (s.dropLastCharacters (4)));
    EXPECT_TRUE (s.endsWith (s.substring (5)));
    EXPECT_TRUE (s.endsWith (s));
    EXPECT_TRUE (s.contains (s.substring (3, 6)));
    EXPECT_TRUE (s.contains (s.substring (3)));
    EXPECT_TRUE (s.startsWithChar (s[0]));
    EXPECT_TRUE (s.endsWithChar (s.getLastCharacter()));
    EXPECT_TRUE (s[s.length()] == 0);
    EXPECT_TRUE (String ("abcdEFGH").toLowerCase() == String ("abcdefgh"));
    EXPECT_TRUE (String ("abcdEFGH").toUpperCase() == String ("ABCDEFGH"));

    EXPECT_TRUE (String (StringRef ("abc")) == "abc");
    EXPECT_TRUE (String (StringRef ("abc")) == StringRef ("abc"));
    EXPECT_TRUE (String ("abc") + StringRef ("def") == "abcdef");

    EXPECT_TRUE (String ("0x00").getHexValue32() == 0);
    EXPECT_TRUE (String ("0x100").getHexValue32() == 256);

    String s2 ("123");
    s2 << ((int) 4) << ((short) 5) << "678" << L"9" << '0';
    s2 += "xyz";
    EXPECT_TRUE (s2 == "1234567890xyz");
    s2 += (int) 123;
    EXPECT_TRUE (s2 == "1234567890xyz123");
    s2 += (int64) 123;
    EXPECT_TRUE (s2 == "1234567890xyz123123");
    s2 << StringRef ("def");
    EXPECT_TRUE (s2 == "1234567890xyz123123def");

    // int16
    {
        String numStr (std::numeric_limits<int16>::max());
        EXPECT_TRUE (numStr == "32767");
    }
    {
        String numStr (std::numeric_limits<int16>::min());
        EXPECT_TRUE (numStr == "-32768");
    }
    {
        String numStr;
        numStr << std::numeric_limits<int16>::max();
        EXPECT_TRUE (numStr == "32767");
    }
    {
        String numStr;
        numStr << std::numeric_limits<int16>::min();
        EXPECT_TRUE (numStr == "-32768");
    }
    // int32
    {
        String numStr (std::numeric_limits<int32>::max());
        EXPECT_TRUE (numStr == "2147483647");
    }
    {
        String numStr (std::numeric_limits<int32>::min());
        EXPECT_TRUE (numStr == "-2147483648");
    }
    {
        String numStr;
        numStr << std::numeric_limits<int32>::max();
        EXPECT_TRUE (numStr == "2147483647");
    }
    {
        String numStr;
        numStr << std::numeric_limits<int32>::min();
        EXPECT_TRUE (numStr == "-2147483648");
    }
    // uint32
    {
        String numStr (std::numeric_limits<uint32>::max());
        EXPECT_TRUE (numStr == "4294967295");
    }
    {
        String numStr (std::numeric_limits<uint32>::min());
        EXPECT_TRUE (numStr == "0");
    }
    // int64
    {
        String numStr (std::numeric_limits<int64>::max());
        EXPECT_TRUE (numStr == "9223372036854775807");
    }
    {
        String numStr (std::numeric_limits<int64>::min());
        EXPECT_TRUE (numStr == "-9223372036854775808");
    }
    {
        String numStr;
        numStr << std::numeric_limits<int64>::max();
        EXPECT_TRUE (numStr == "9223372036854775807");
    }
    {
        String numStr;
        numStr << std::numeric_limits<int64>::min();
        EXPECT_TRUE (numStr == "-9223372036854775808");
    }
    // uint64
    {
        String numStr (std::numeric_limits<uint64>::max());
        EXPECT_TRUE (numStr == "18446744073709551615");
    }
    {
        String numStr (std::numeric_limits<uint64>::min());
        EXPECT_TRUE (numStr == "0");
    }
    {
        String numStr;
        numStr << std::numeric_limits<uint64>::max();
        EXPECT_TRUE (numStr == "18446744073709551615");
    }
    {
        String numStr;
        numStr << std::numeric_limits<uint64>::min();
        EXPECT_TRUE (numStr == "0");
    }
    // size_t
    {
        String numStr (std::numeric_limits<size_t>::min());
        EXPECT_TRUE (numStr == "0");
    }
}

TEST_F (StringTests, NumericConversions)
{
    String s ("012345678");

    EXPECT_TRUE (String().getIntValue() == 0);
    EXPECT_EQ (String().getDoubleValue(), 0.0);
    EXPECT_EQ (String().getFloatValue(), 0.0f);
    EXPECT_TRUE (s.getIntValue() == 12345678);
    EXPECT_TRUE (s.getLargeIntValue() == (int64) 12345678);
    EXPECT_EQ (s.getDoubleValue(), 12345678.0);
    EXPECT_EQ (s.getFloatValue(), 12345678.0f);
    EXPECT_TRUE (String (-1234).getIntValue() == -1234);
    EXPECT_TRUE (String ((int64) -1234).getLargeIntValue() == -1234);
    EXPECT_EQ (String (-1234.56).getDoubleValue(), -1234.56);
    EXPECT_EQ (String (-1234.56f).getFloatValue(), -1234.56f);
    EXPECT_TRUE (String (std::numeric_limits<int>::max()).getIntValue() == std::numeric_limits<int>::max());
    EXPECT_TRUE (String (std::numeric_limits<int>::min()).getIntValue() == std::numeric_limits<int>::min());
    EXPECT_TRUE (String (std::numeric_limits<int64>::max()).getLargeIntValue() == std::numeric_limits<int64>::max());
    EXPECT_TRUE (String (std::numeric_limits<int64>::min()).getLargeIntValue() == std::numeric_limits<int64>::min());
    EXPECT_TRUE (("xyz" + s).getTrailingIntValue() == s.getIntValue());
    EXPECT_TRUE (String ("xyz-5").getTrailingIntValue() == -5);
    EXPECT_TRUE (String ("-12345").getTrailingIntValue() == -12345);
    EXPECT_TRUE (s.getHexValue32() == 0x12345678);
    EXPECT_TRUE (s.getHexValue64() == (int64) 0x12345678);
    EXPECT_TRUE (String::toHexString (0x1234abcd).equalsIgnoreCase ("1234abcd"));
    EXPECT_TRUE (String::toHexString ((int64) 0x1234abcd).equalsIgnoreCase ("1234abcd"));
    EXPECT_TRUE (String::toHexString ((short) 0x12ab).equalsIgnoreCase ("12ab"));
    EXPECT_TRUE (String::toHexString ((size_t) 0x12ab).equalsIgnoreCase ("12ab"));
    EXPECT_TRUE (String::toHexString ((long) 0x12ab).equalsIgnoreCase ("12ab"));
    EXPECT_TRUE (String::toHexString ((int8) -1).equalsIgnoreCase ("ff"));
    EXPECT_TRUE (String::toHexString ((int16) -1).equalsIgnoreCase ("ffff"));
    EXPECT_TRUE (String::toHexString ((int32) -1).equalsIgnoreCase ("ffffffff"));
    EXPECT_TRUE (String::toHexString ((int64) -1).equalsIgnoreCase ("ffffffffffffffff"));

    unsigned char data[] = { 1, 2, 3, 4, 0xa, 0xb, 0xc, 0xd };
    EXPECT_TRUE (String::toHexString (data, 8, 0).equalsIgnoreCase ("010203040a0b0c0d"));
    EXPECT_TRUE (String::toHexString (data, 8, 1).equalsIgnoreCase ("01 02 03 04 0a 0b 0c 0d"));
    EXPECT_TRUE (String::toHexString (data, 8, 2).equalsIgnoreCase ("0102 0304 0a0b 0c0d"));

    EXPECT_EQ (String (12345.67, 4), String ("12345.6700"));
    EXPECT_EQ (String (12345.67, 6), String ("12345.670000"));
    EXPECT_EQ (String (2589410.5894, 7), String ("2589410.5894000"));
    EXPECT_EQ (String (12345.67, 8), String ("12345.67000000"));
    EXPECT_EQ (String (1e19, 4), String ("10000000000000000000.0000"));
    EXPECT_EQ (String (1e-34, 36), String ("0.000000000000000000000000000000000100"));
    EXPECT_EQ (String (1.39, 1), String ("1.4"));

    EXPECT_EQ (String (12345.67, 4, true), String ("1.2346e+04"));
    EXPECT_EQ (String (12345.67, 6, true), String ("1.234567e+04"));
    EXPECT_EQ (String (2589410.5894, 7, true), String ("2.5894106e+06"));
    EXPECT_EQ (String (12345.67, 8, true), String ("1.23456700e+04"));
    EXPECT_EQ (String (1e19, 4, true), String ("1.0000e+19"));
    EXPECT_EQ (String (1e-34, 5, true), String ("1.00000e-34"));
    EXPECT_EQ (String (1.39, 1, true), String ("1.4e+00"));
}

TEST_F (StringTests, Subsections)
{
    String s3;
    s3 = "abcdeFGHIJ";
    EXPECT_TRUE (s3.equalsIgnoreCase ("ABCdeFGhiJ"));
    EXPECT_TRUE (s3.compareIgnoreCase (L"ABCdeFGhiJ") == 0);
    EXPECT_TRUE (s3.containsIgnoreCase (s3.substring (3)));
    EXPECT_TRUE (s3.indexOfAnyOf ("xyzf", 2, true) == 5);
    EXPECT_TRUE (s3.indexOfAnyOf (String (L"xyzf"), 2, false) == -1);
    EXPECT_TRUE (s3.indexOfAnyOf ("xyzF", 2, false) == 5);
    EXPECT_TRUE (s3.containsAnyOf (String (L"zzzFs")));
    EXPECT_TRUE (s3.startsWith ("abcd"));
    EXPECT_TRUE (s3.startsWithIgnoreCase (String (L"abCD")));
    EXPECT_TRUE (s3.startsWith (String()));
    EXPECT_TRUE (s3.startsWithChar ('a'));
    EXPECT_TRUE (s3.endsWith (String ("HIJ")));
    EXPECT_TRUE (s3.endsWithIgnoreCase (String (L"Hij")));
    EXPECT_TRUE (s3.endsWith (String()));
    EXPECT_TRUE (s3.endsWithChar (L'J'));
    EXPECT_TRUE (s3.indexOf ("HIJ") == 7);
    EXPECT_TRUE (s3.indexOf (String (L"HIJK")) == -1);
    EXPECT_TRUE (s3.indexOfIgnoreCase ("hij") == 7);
    EXPECT_TRUE (s3.indexOfIgnoreCase (String (L"hijk")) == -1);
    EXPECT_TRUE (s3.toStdString() == s3.toRawUTF8());

    String s4 (s3);
    s4.append (String ("xyz123"), 3);
    EXPECT_TRUE (s4 == s3 + "xyz");

    EXPECT_TRUE (String (1234) < String (1235));
    EXPECT_TRUE (String (1235) > String (1234));
    EXPECT_TRUE (String (1234) >= String (1234));
    EXPECT_TRUE (String (1234) <= String (1234));
    EXPECT_TRUE (String (1235) >= String (1234));
    EXPECT_TRUE (String (1234) <= String (1235));

    String s5 ("word word2 word3");
    EXPECT_TRUE (s5.containsWholeWord (String ("word2")));
    EXPECT_TRUE (s5.indexOfWholeWord ("word2") == 5);
    EXPECT_TRUE (s5.containsWholeWord (String (L"word")));
    EXPECT_TRUE (s5.containsWholeWord ("word3"));
    EXPECT_TRUE (s5.containsWholeWord (s5));
    EXPECT_TRUE (s5.containsWholeWordIgnoreCase (String (L"Word2")));
    EXPECT_TRUE (s5.indexOfWholeWordIgnoreCase ("Word2") == 5);
    EXPECT_TRUE (s5.containsWholeWordIgnoreCase (String (L"Word")));
    EXPECT_TRUE (s5.containsWholeWordIgnoreCase ("Word3"));
    EXPECT_TRUE (! s5.containsWholeWordIgnoreCase (String (L"Wordx")));
    EXPECT_TRUE (! s5.containsWholeWordIgnoreCase ("xWord2"));
    EXPECT_TRUE (s5.containsNonWhitespaceChars());
    EXPECT_TRUE (s5.containsOnly ("ordw23 "));
    EXPECT_TRUE (! String (" \n\r\t").containsNonWhitespaceChars());

    EXPECT_TRUE (s5.matchesWildcard (String (L"wor*"), false));
    EXPECT_TRUE (s5.matchesWildcard ("wOr*", true));
    EXPECT_TRUE (s5.matchesWildcard (String (L"*word3"), true));
    EXPECT_TRUE (s5.matchesWildcard ("*word?", true));
    EXPECT_TRUE (s5.matchesWildcard (String (L"Word*3"), true));
    EXPECT_TRUE (! s5.matchesWildcard (String (L"*34"), true));
    EXPECT_TRUE (String ("xx**y").matchesWildcard ("*y", true));
    EXPECT_TRUE (String ("xx**y").matchesWildcard ("x*y", true));
    EXPECT_TRUE (String ("xx**y").matchesWildcard ("xx*y", true));
    EXPECT_TRUE (String ("xx**y").matchesWildcard ("xx*", true));
    EXPECT_TRUE (String ("xx?y").matchesWildcard ("x??y", true));
    EXPECT_TRUE (String ("xx?y").matchesWildcard ("xx?y", true));
    EXPECT_TRUE (! String ("xx?y").matchesWildcard ("xx?y?", true));
    EXPECT_TRUE (String ("xx?y").matchesWildcard ("xx??", true));

    EXPECT_EQ (s5.fromFirstOccurrenceOf (String(), true, false), s5);
    EXPECT_EQ (s5.fromFirstOccurrenceOf ("xword2", true, false), s5.substring (100));
    EXPECT_EQ (s5.fromFirstOccurrenceOf (String (L"word2"), true, false), s5.substring (5));
    EXPECT_EQ (s5.fromFirstOccurrenceOf ("Word2", true, true), s5.substring (5));
    EXPECT_EQ (s5.fromFirstOccurrenceOf ("word2", false, false), s5.getLastCharacters (6));
    EXPECT_EQ (s5.fromFirstOccurrenceOf ("Word2", false, true), s5.getLastCharacters (6));

    EXPECT_EQ (s5.fromLastOccurrenceOf (String(), true, false), s5);
    EXPECT_EQ (s5.fromLastOccurrenceOf ("wordx", true, false), s5);
    EXPECT_EQ (s5.fromLastOccurrenceOf ("word", true, false), s5.getLastCharacters (5));
    EXPECT_EQ (s5.fromLastOccurrenceOf ("worD", true, true), s5.getLastCharacters (5));
    EXPECT_EQ (s5.fromLastOccurrenceOf ("word", false, false), s5.getLastCharacters (1));
    EXPECT_EQ (s5.fromLastOccurrenceOf ("worD", false, true), s5.getLastCharacters (1));

    EXPECT_TRUE (s5.upToFirstOccurrenceOf (String(), true, false).isEmpty());
    EXPECT_EQ (s5.upToFirstOccurrenceOf ("word4", true, false), s5);
    EXPECT_EQ (s5.upToFirstOccurrenceOf ("word2", true, false), s5.substring (0, 10));
    EXPECT_EQ (s5.upToFirstOccurrenceOf ("Word2", true, true), s5.substring (0, 10));
    EXPECT_EQ (s5.upToFirstOccurrenceOf ("word2", false, false), s5.substring (0, 5));
    EXPECT_EQ (s5.upToFirstOccurrenceOf ("Word2", false, true), s5.substring (0, 5));

    EXPECT_EQ (s5.upToLastOccurrenceOf (String(), true, false), s5);
    EXPECT_EQ (s5.upToLastOccurrenceOf ("zword", true, false), s5);
    EXPECT_EQ (s5.upToLastOccurrenceOf ("word", true, false), s5.dropLastCharacters (1));
    EXPECT_EQ (s5.dropLastCharacters (1).upToLastOccurrenceOf ("word", true, false), s5.dropLastCharacters (1));
    EXPECT_EQ (s5.upToLastOccurrenceOf ("Word", true, true), s5.dropLastCharacters (1));
    EXPECT_EQ (s5.upToLastOccurrenceOf ("word", false, false), s5.dropLastCharacters (5));
    EXPECT_EQ (s5.upToLastOccurrenceOf ("Word", false, true), s5.dropLastCharacters (5));

    EXPECT_EQ (s5.replace ("word", "xyz", false), String ("xyz xyz2 xyz3"));
    EXPECT_TRUE (s5.replace ("Word", "xyz", true) == "xyz xyz2 xyz3");
    EXPECT_TRUE (s5.dropLastCharacters (1).replace ("Word", String ("xyz"), true) == L"xyz xyz2 xyz");
    EXPECT_TRUE (s5.replace ("Word", "", true) == " 2 3");
    EXPECT_EQ (s5.replace ("Word2", "xyz", true), String ("word xyz word3"));
    EXPECT_TRUE (s5.replaceCharacter (L'w', 'x') != s5);
    EXPECT_EQ (s5.replaceCharacter ('w', L'x').replaceCharacter ('x', 'w'), s5);
    EXPECT_TRUE (s5.replaceCharacters ("wo", "xy") != s5);
    EXPECT_EQ (s5.replaceCharacters ("wo", "xy").replaceCharacters ("xy", "wo"), s5);
    EXPECT_EQ (s5.retainCharacters ("1wordxya"), String ("wordwordword"));
    EXPECT_TRUE (s5.retainCharacters (String()).isEmpty());
    EXPECT_TRUE (s5.removeCharacters ("1wordxya") == " 2 3");
    EXPECT_EQ (s5.removeCharacters (String()), s5);
    EXPECT_TRUE (s5.initialSectionContainingOnly ("word") == L"word");
    EXPECT_TRUE (String ("word").initialSectionContainingOnly ("word") == L"word");
    EXPECT_EQ (s5.initialSectionNotContaining (String ("xyz ")), String ("word"));
    EXPECT_EQ (s5.initialSectionNotContaining (String (";[:'/")), s5);
    EXPECT_TRUE (! s5.isQuotedString());
    EXPECT_TRUE (s5.quoted().isQuotedString());
    EXPECT_TRUE (! s5.quoted().unquoted().isQuotedString());
    EXPECT_TRUE (! String ("x'").isQuotedString());
    EXPECT_TRUE (String ("'x").isQuotedString());

    String s6 (" \t xyz  \t\r\n");
    EXPECT_EQ (s6.trim(), String ("xyz"));
    EXPECT_TRUE (s6.trim().trim() == "xyz");
    EXPECT_EQ (s5.trim(), s5);
    EXPECT_EQ (s6.trimStart().trimEnd(), s6.trim());
    EXPECT_EQ (s6.trimStart().trimEnd(), s6.trimEnd().trimStart());
    EXPECT_EQ (s6.trimStart().trimStart().trimEnd().trimEnd(), s6.trimEnd().trimStart());
    EXPECT_TRUE (s6.trimStart() != s6.trimEnd());
    EXPECT_EQ (("\t\r\n " + s6 + "\t\n \r").trim(), s6.trim());
    EXPECT_TRUE (String::repeatedString ("xyz", 3) == L"xyzxyzxyz");
}

TEST_F (StringTests, UTFConversions)
{
    TestUTFConversion<CharPointer_UTF32>::test();
    TestUTFConversion<CharPointer_UTF8>::test();
    TestUTFConversion<CharPointer_UTF16>::test();
}

TEST_F (StringTests, StringArray)
{
    StringArray s;
    s.addTokens ("4,3,2,1,0", ";,", "x");
    EXPECT_EQ (s.size(), 5);

    EXPECT_EQ (s.joinIntoString ("-"), String ("4-3-2-1-0"));
    s.remove (2);
    EXPECT_EQ (s.joinIntoString ("--"), String ("4--3--1--0"));
    EXPECT_EQ (s.joinIntoString (StringRef()), String ("4310"));
    s.clear();
    EXPECT_EQ (s.joinIntoString ("x"), String());

    StringArray toks;
    toks.addTokens ("x,,", ";,", "");
    EXPECT_EQ (toks.size(), 3);
    EXPECT_EQ (toks.joinIntoString ("-"), String ("x--"));
    toks.clear();

    toks.addTokens (",x,", ";,", "");
    EXPECT_EQ (toks.size(), 3);
    EXPECT_EQ (toks.joinIntoString ("-"), String ("-x-"));
    toks.clear();

    toks.addTokens ("x,'y,z',", ";,", "'");
    EXPECT_EQ (toks.size(), 3);
    EXPECT_EQ (toks.joinIntoString ("-"), String ("x-'y,z'-"));
}

TEST_F (StringTests, Variant)
{
    var v1 = 0;
    var v2 = 0.16;
    var v3 = "0.16";
    var v4 = (int64) 0;
    var v5 = 0.0;
    EXPECT_TRUE (! v2.equals (v1));
    EXPECT_TRUE (! v1.equals (v2));
    EXPECT_TRUE (v2.equals (v3));
    EXPECT_TRUE (! v3.equals (v1));
    EXPECT_TRUE (! v1.equals (v3));
    EXPECT_TRUE (v1.equals (v4));
    EXPECT_TRUE (v4.equals (v1));
    EXPECT_TRUE (v5.equals (v4));
    EXPECT_TRUE (v4.equals (v5));
    EXPECT_TRUE (! v2.equals (v4));
    EXPECT_TRUE (! v4.equals (v2));
}

TEST_F (StringTests, SignificantFigures)
{
    // Integers

    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (13, 1), String ("10"));
    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (13, 2), String ("13"));
    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (13, 3), String ("13.0"));
    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (13, 4), String ("13.00"));

    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (19368, 1), String ("20000"));
    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (19348, 3), String ("19300"));

    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (-5, 1), String ("-5"));
    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (-5, 3), String ("-5.00"));

    // Zero

    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (0, 1), String ("0"));
    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (0, 2), String ("0.0"));
    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (0, 3), String ("0.00"));

    // Floating point

    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (19.0, 1), String ("20"));
    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (19.0, 2), String ("19"));
    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (19.0, 3), String ("19.0"));
    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (19.0, 4), String ("19.00"));

    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (-5.45, 1), String ("-5"));
    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (-5.45, 3), String ("-5.45"));

    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (12345.6789, 9), String ("12345.6789"));
    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (12345.6789, 8), String ("12345.679"));
    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (12345.6789, 5), String ("12346"));

    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (0.00028647, 6), String ("0.000286470"));
    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (0.0028647, 6), String ("0.00286470"));
    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (2.8647, 6), String ("2.86470"));

    EXPECT_EQ (String::toDecimalStringWithSignificantFigures (-0.0000000000019, 1), String ("-0.000000000002"));
}

TEST_F (StringTests, FloatTrimming)
{
    {
        StringPairArray tests;
        tests.set ("1", "1");
        tests.set ("1.0", "1.0");
        tests.set ("-1", "-1");
        tests.set ("-100", "-100");
        tests.set ("110", "110");
        tests.set ("9090", "9090");
        tests.set ("1000.0", "1000.0");
        tests.set ("1.0", "1.0");
        tests.set ("-1.00", "-1.0");
        tests.set ("1.20", "1.2");
        tests.set ("1.300", "1.3");
        tests.set ("1.301", "1.301");
        tests.set ("1e", "1");
        tests.set ("-1e+", "-1");
        tests.set ("1e-", "1");
        tests.set ("1e0", "1");
        tests.set ("1e+0", "1");
        tests.set ("1e-0", "1");
        tests.set ("1e000", "1");
        tests.set ("1e+000", "1");
        tests.set ("-1e-000", "-1");
        tests.set ("1e100", "1e100");
        tests.set ("100e100", "100e100");
        tests.set ("100.0e0100", "100.0e100");
        tests.set ("-1e1", "-1e1");
        tests.set ("1e10", "1e10");
        tests.set ("-1e+10", "-1e10");
        tests.set ("1e-10", "1e-10");
        tests.set ("1e0010", "1e10");
        tests.set ("1e-0010", "1e-10");
        tests.set ("1e-1", "1e-1");
        tests.set ("-1.0e1", "-1.0e1");
        tests.set ("1.0e-1", "1.0e-1");
        tests.set ("1.00e-1", "1.0e-1");
        tests.set ("1.001e1", "1.001e1");
        tests.set ("1.010e+1", "1.01e1");
        tests.set ("-1.1000e1", "-1.1e1");

        for (auto& input : tests.getAllKeys())
            EXPECT_EQ (reduceLengthOfFloatString (input), tests[input]);
    }

    {
        std::map<double, String> tests;
        tests[1] = "1.0";
        tests[1.1] = "1.1";
        tests[1.01] = "1.01";
        tests[0.76378] = "7.6378e-1";
        tests[-10] = "-1.0e1";
        tests[10.01] = "1.001e1";
        tests[10691.01] = "1.069101e4";
        tests[0.0123] = "1.23e-2";
        tests[-3.7e-27] = "-3.7e-27";
        tests[1e+40] = "1.0e40";

        for (auto& test : tests)
            EXPECT_EQ (reduceLengthOfFloatString (String (test.first, 15, true)), test.second);
    }
}

TEST_F (StringTests, Serialisation)
{
    std::map<double, String> tests;

    tests[364] = "364.0";
    tests[1e7] = "1.0e7";
    tests[12345678901] = "1.2345678901e10";

    tests[1234567890123456.7] = "1.234567890123457e15";
    tests[12345678.901234567] = "1.234567890123457e7";
    tests[1234567.8901234567] = "1.234567890123457e6";
    tests[123456.78901234567] = "123456.7890123457";
    tests[12345.678901234567] = "12345.67890123457";
    tests[1234.5678901234567] = "1234.567890123457";
    tests[123.45678901234567] = "123.4567890123457";
    tests[12.345678901234567] = "12.34567890123457";
    tests[1.2345678901234567] = "1.234567890123457";
    tests[0.12345678901234567] = "0.1234567890123457";
    tests[0.012345678901234567] = "0.01234567890123457";
    tests[0.0012345678901234567] = "0.001234567890123457";
    tests[0.00012345678901234567] = "0.0001234567890123457";
    tests[0.000012345678901234567] = "0.00001234567890123457";
    tests[0.0000012345678901234567] = "1.234567890123457e-6";
    tests[0.00000012345678901234567] = "1.234567890123457e-7";

    for (auto& test : tests)
    {
        EXPECT_EQ (serialiseDouble (test.first), test.second);
        EXPECT_EQ (serialiseDouble (-test.first), "-" + test.second);
    }
}

TEST_F (StringTests, Loops)
{
    String str (CharPointer_UTF8 ("\xc2\xaf\\_(\xe3\x83\x84)_/\xc2\xaf"));
    std::vector<yup_wchar> parts { 175, 92, 95, 40, 12484, 41, 95, 47, 175 };
    size_t index = 0;

    for (auto c : str)
        EXPECT_EQ (c, parts[index++]);
}

TEST_F (StringTests, LineManipulation)
{
    String multiline ("line1\nline2\n  line3\n\nline4");

    // Test indentLines
    String indented = multiline.indentLines ("  ");
    EXPECT_EQ (indented, String ("  line1\n  line2\n    line3\n\n  line4"));

    String indentedWithBlanks = multiline.indentLines (">>", true);
    EXPECT_EQ (indentedWithBlanks, String (">>line1\n>>line2\n>>  line3\n>>\n>>line4"));

    // Test dedentLines
    String indentedText ("    line1\n    line2\n      line3\n    line4");
    String dedented = indentedText.dedentLines();
    EXPECT_EQ (dedented, String ("line1\nline2\n  line3\nline4"));

    // Test dedentLines with mixed indentation
    String mixedIndent ("  \tline1\n  \tline2\n  \t  line3");
    String dedentedMixed = mixedIndent.dedentLines();
    EXPECT_EQ (dedentedMixed, String ("line1\nline2\n  line3"));
}

TEST_F (StringTests, PaddingMethods)
{
    String short_str ("abc");

    // Test paddedLeft
    EXPECT_EQ (short_str.paddedLeft (' ', 10), String ("       abc"));
    EXPECT_EQ (short_str.paddedLeft ('*', 5), String ("**abc"));
    EXPECT_EQ (short_str.paddedLeft ('-', 3), String ("abc")); // No padding needed
    EXPECT_EQ (short_str.paddedLeft ('0', 1), String ("abc")); // Shorter than minimum

    // Test paddedRight
    EXPECT_EQ (short_str.paddedRight (' ', 10), String ("abc       "));
    EXPECT_EQ (short_str.paddedRight ('*', 5), String ("abc**"));
    EXPECT_EQ (short_str.paddedRight ('-', 3), String ("abc")); // No padding needed
    EXPECT_EQ (short_str.paddedRight ('0', 1), String ("abc")); // Shorter than minimum
}

TEST_F (StringTests, CharacterTrimming)
{
    String test_str ("...Hello World!!!");

    // Test trimCharactersAtStart
    EXPECT_EQ (test_str.trimCharactersAtStart ("."), String ("Hello World!!!"));
    EXPECT_EQ (test_str.trimCharactersAtStart (".*"), String ("Hello World!!!"));
    EXPECT_EQ (String ("  \t  text").trimCharactersAtStart (" \t"), String ("text"));

    // Test trimCharactersAtEnd
    EXPECT_EQ (test_str.trimCharactersAtEnd ("!"), String ("...Hello World"));
    EXPECT_EQ (test_str.trimCharactersAtEnd ("!."), String ("...Hello World"));
    EXPECT_EQ (String ("text  \t  ").trimCharactersAtEnd (" \t"), String ("text"));

    // Test with empty string
    EXPECT_EQ (String().trimCharactersAtStart ("abc"), String());
    EXPECT_EQ (String().trimCharactersAtEnd ("abc"), String());
}

TEST_F (StringTests, SectionReplacement)
{
    String base ("Hello World");

    // Test replaceSection
    EXPECT_EQ (base.replaceSection (0, 5, "Hi"), String ("Hi World"));
    EXPECT_EQ (base.replaceSection (6, 5, "Universe"), String ("Hello Universe"));
    EXPECT_EQ (base.replaceSection (5, 1, ""), String ("HelloWorld"));
    EXPECT_EQ (base.replaceSection (0, 0, "Well, "), String ("Well, Hello World"));

    // Test replaceFirstOccurrenceOf
    String repeated ("abc abc abc");
    EXPECT_EQ (repeated.replaceFirstOccurrenceOf ("abc", "xyz"), String ("xyz abc abc"));
    EXPECT_EQ (repeated.replaceFirstOccurrenceOf ("abc", "xyz", true), String ("xyz abc abc"));
    EXPECT_EQ (repeated.replaceFirstOccurrenceOf ("ABC", "xyz", true), String ("xyz abc abc"));
    EXPECT_EQ (repeated.replaceFirstOccurrenceOf ("ABC", "xyz", false), String ("abc abc abc"));
    EXPECT_EQ (repeated.replaceFirstOccurrenceOf ("def", "xyz"), String ("abc abc abc"));
}

TEST_F (StringTests, StringReversing)
{
    // Test reversed with basic ASCII strings
    EXPECT_EQ (String ("hello").reversed(), String ("olleh"));
    EXPECT_EQ (String ("a").reversed(), String ("a"));
    EXPECT_EQ (String().reversed(), String());
    EXPECT_EQ (String ("12345").reversed(), String ("54321"));

    // Test with Unicode characters - this is the critical test for UTF-8 handling
    String unicode_str (L"café");
    String reversed_unicode = unicode_str.reversed();
    EXPECT_EQ (reversed_unicode, String (L"éfac")); // Should correctly reverse Unicode characters

    // Test with more complex Unicode strings
    String unicode_complex (CharPointer_UTF8 ("Hello, 世界!"));
    String reversed_complex = unicode_complex.reversed();
    EXPECT_EQ (reversed_complex, String (CharPointer_UTF8 ("!界世 ,olleH")));

    // Test with emojis and other complex Unicode
    String emoji_str (CharPointer_UTF8 ("🌟⭐"));
    String reversed_emoji = emoji_str.reversed();
    EXPECT_EQ (reversed_emoji, String (CharPointer_UTF8 ("⭐🌟")));

    // Test reversibility (reversing twice should give original)
    String original (CharPointer_UTF8 ("Test string with UTF-8: café"));
    String double_reversed = original.reversed().reversed();
    EXPECT_EQ (double_reversed, original);

    // Test with mixed ASCII and Unicode
    String mixed (CharPointer_UTF8 ("abc世界def"));
    String reversed_mixed = mixed.reversed();
    EXPECT_EQ (reversed_mixed, String (CharPointer_UTF8 ("fed界世cba")));
}

TEST_F (StringTests, RepeatedString)
{
    // Test repeatedString
    EXPECT_EQ (String::repeatedString ("abc", 3), String ("abcabcabc"));
    EXPECT_EQ (String::repeatedString ("x", 5), String ("xxxxx"));
    EXPECT_EQ (String::repeatedString ("hello", 0), String());
    EXPECT_EQ (String::repeatedString ("", 10), String());
    EXPECT_EQ (String::repeatedString ("test", 1), String ("test"));
}

TEST_F (StringTests, BufferCopyMethods)
{
    String test_str (L"Hello, 世界!");

    // Test copyToUTF8
    char utf8_buffer[100];
    size_t utf8_bytes = test_str.copyToUTF8 (utf8_buffer, sizeof (utf8_buffer));
    EXPECT_GT (utf8_bytes, 0);
    EXPECT_EQ (String::fromUTF8 (utf8_buffer), test_str);

    // Test getNumBytesAsUTF8
    size_t required_bytes = test_str.getNumBytesAsUTF8();
    EXPECT_GT (required_bytes, test_str.length()); // Should be more due to unicode chars

    // Test copyToUTF16
    CharPointer_UTF16::CharType utf16_buffer[100];
    size_t utf16_bytes = test_str.copyToUTF16 (utf16_buffer, sizeof (utf16_buffer));
    EXPECT_GT (utf16_bytes, 0);

    // Test copyToUTF32
    CharPointer_UTF32::CharType utf32_buffer[100];
    size_t utf32_bytes = test_str.copyToUTF32 (utf32_buffer, sizeof (utf32_buffer));
    EXPECT_GT (utf32_bytes, 0);

    // Test with null buffer (should return required size)
    size_t utf8_required = test_str.copyToUTF8 (nullptr, 0);
    EXPECT_GT (utf8_required, 0);
}

TEST_F (StringTests, PreallocationAndReferenceCounting)
{
    String str1 ("test");
    String str2 = str1; // This should share the same data

    // Test getReferenceCount
    EXPECT_EQ (str1.getReferenceCount(), str2.getReferenceCount());
    EXPECT_GE (str1.getReferenceCount(), 2);

    // Test preallocateBytes
    String growing_str;
    growing_str.preallocateBytes (1000);
    // After preallocation, adding strings should be more efficient
    for (int i = 0; i < 10; ++i)
        growing_str += "some text ";
    EXPECT_GT (growing_str.length(), 0);
}

TEST_F (StringTests, FormattedStrings)
{
    // Test formatted (basic cases)
    String formatted_str = String::formatted ("Hello %s", "World");
    EXPECT_TRUE (formatted_str.contains ("Hello"));
    EXPECT_TRUE (formatted_str.contains ("World"));

    String formatted_int = String::formatted ("Number: %d", 42);
    EXPECT_TRUE (formatted_int.contains ("Number"));
    EXPECT_TRUE (formatted_int.contains ("42"));

    String formatted_float = String::formatted ("Value: %.2f", 3.14159);
    EXPECT_TRUE (formatted_float.contains ("Value"));
    EXPECT_TRUE (formatted_float.contains ("3.14"));
}

TEST_F (StringTests, StringCreationFromData)
{
    // Test createStringFromData
    const char* ascii_data = "Hello World";
    String from_ascii = String::createStringFromData (ascii_data, (int) strlen (ascii_data));
    EXPECT_EQ (from_ascii, String ("Hello World"));

    // Test with UTF-8 data
    const char* utf8_data = "Hello, 世界!";
    String from_utf8 = String::createStringFromData (utf8_data, (int) strlen (utf8_data));
    EXPECT_TRUE (from_utf8.contains ("Hello"));
    EXPECT_TRUE (from_utf8.contains ("世界"));

    // Test with zero size
    String empty_from_data = String::createStringFromData (ascii_data, 0);
    EXPECT_TRUE (empty_from_data.isEmpty());
}

TEST_F (StringTests, FromUTF8)
{
    // Test fromUTF8
    const char* utf8_text = "Hello, 世界!";
    String from_utf8 = String::fromUTF8 (utf8_text);
    EXPECT_TRUE (from_utf8.contains ("Hello"));
    EXPECT_TRUE (from_utf8.contains ("世界"));

    // Test with explicit length
    String partial_utf8 = String::fromUTF8 (utf8_text, 5);
    EXPECT_EQ (partial_utf8, String ("Hello"));

    // Test with null input
    String null_utf8 = String::fromUTF8 (nullptr);
    EXPECT_TRUE (null_utf8.isEmpty());
}

TEST_F (StringTests, NaturalComparison)
{
    // Test compareNatural
    EXPECT_EQ (String ("file1.txt").compareNatural ("file1.txt"), 0);
    EXPECT_LT (String ("file1.txt").compareNatural ("file10.txt"), 0);
    EXPECT_GT (String ("file10.txt").compareNatural ("file2.txt"), 0);
    EXPECT_EQ (String ("abc").compareNatural ("ABC", false), 0); // Case insensitive
    EXPECT_NE (String ("abc").compareNatural ("ABC", true), 0);  // Case sensitive

    // Test with numbers
    EXPECT_LT (String ("version1.2").compareNatural ("version1.10"), 0);
    EXPECT_GT (String ("version2.0").compareNatural ("version1.10"), 0);
}

TEST_F (StringTests, StandardLibraryIntegration)
{
    // Test std::string integration
    std::string std_str = "Hello from std::string";
    String yup_str (std_str);
    EXPECT_EQ (yup_str.toStdString(), std_str);

    // Test std::wstring integration
    std::wstring wide_str = L"Hello from std::wstring";
    String yup_wide_str (wide_str);
    EXPECT_TRUE (yup_wide_str.contains ("Hello"));

    // Test std::string_view integration
    std::string_view string_view = "Hello from string_view";
    String yup_view_str (string_view);
    EXPECT_EQ (yup_view_str, String ("Hello from string_view"));

    // Test std::wstring_view integration
    std::wstring_view wide_view = L"Hello from wstring_view";
    String yup_wide_view_str (wide_view);
    EXPECT_TRUE (yup_wide_view_str.contains ("Hello"));
}

TEST_F (StringTests, CaseConversionEdgeCases)
{
    // Test toUpperCase with edge cases
    String mixed_case (L"Hello, 世界! 123");
    String upper_case = mixed_case.toUpperCase();
    EXPECT_EQ (upper_case, String (L"HELLO, 世界! 123"));

    // Test toLowerCase with edge cases
    String lower_case = mixed_case.toLowerCase();
    EXPECT_EQ (lower_case, String (L"hello, 世界! 123"));

    // Test with empty string
    EXPECT_EQ (String().toUpperCase(), String());
    EXPECT_EQ (String().toLowerCase(), String());

    // Test with numbers only
    String numbers ("12345");
    EXPECT_EQ (numbers.toUpperCase(), String ("12345"));
    EXPECT_EQ (numbers.toLowerCase(), String ("12345"));
}

TEST_F (StringTests, AdditionalUtilityMethods)
{
    // Test swapWith
    String str1 ("Hello");
    String str2 ("World");
    String original1 = str1;
    String original2 = str2;

    str1.swapWith (str2);
    EXPECT_EQ (str1, original2);
    EXPECT_EQ (str2, original1);

    // Test hash method
    String hash_test ("test string");
    size_t hash1 = hash_test.hash();
    size_t hash2 = String ("test string").hash();
    size_t hash3 = String ("different string").hash();

    EXPECT_EQ (hash1, hash2); // Same strings should have same hash
    EXPECT_NE (hash1, hash3); // Different strings should have different hashes

    // Test begin/end iterators
    String iter_test ("abc");
    auto it = iter_test.begin();
    EXPECT_EQ (*it, 'a');
    ++it;
    EXPECT_EQ (*it, 'b');
    ++it;
    EXPECT_EQ (*it, 'c');
    ++it;
    EXPECT_EQ (it, iter_test.end());
}

TEST_F (StringTests, EdgeCasesAndBoundaryConditions)
{
    // Test with very long strings
    String long_str = String::repeatedString ("a", 10000);
    EXPECT_EQ (long_str.length(), 10000);
    EXPECT_TRUE (long_str.startsWith ("aaa"));
    EXPECT_TRUE (long_str.endsWith ("aaa"));

    // Test with single character
    String single_char ("x");
    EXPECT_EQ (single_char.length(), 1);
    EXPECT_EQ (single_char[0], 'x');
    EXPECT_EQ (single_char.getLastCharacter(), 'x');

    // Test boundary conditions for substring
    String boundary_test ("hello");
    EXPECT_EQ (boundary_test.substring (-5, 10), String ("hello"));
    EXPECT_EQ (boundary_test.substring (0, 0), String());
    EXPECT_EQ (boundary_test.substring (5, 5), String());
    EXPECT_EQ (boundary_test.substring (100, 200), String());

    // Test with null characters
    String null_char_test = String::charToString (0);
    EXPECT_EQ (null_char_test, String());

    // Test very large numbers
    String large_num (std::numeric_limits<long long>::max());
    EXPECT_GT (large_num.length(), 0);
    EXPECT_EQ (large_num.getLargeIntValue(), std::numeric_limits<long long>::max());
}
