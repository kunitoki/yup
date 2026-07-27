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
*/
#include <gtest/gtest.h>

#include <yup_core/yup_core.h>

using namespace yup;

class LocalisedStringsTests : public ::testing::Test
{
protected:
    static const inline String validTranslationFile = String::fromUTF8 (
        "language: English\n"
        "countries: us gb au\n"
        "\"hello\" = \"hello\"\n"
        "\"goodbye\" = \"goodbye\"\n"
        "\"yes\" = \"yes\"\n");

    static const inline String translationFileWithEscapedQuotes =
        String::fromUTF8 ("\"a \\\"quoted\\\" string\" = \"une \\\"chaîne\\\" citée\"\n");

    static const inline String invalidTranslationFile = String::fromUTF8 (
        "invalid content\n"
        "\"hello\" different");

    void SetUp() override
    {
        LocalisedStrings::setCurrentMappings (nullptr); // Ensure no residual mappings.
    }
};

TEST_F (LocalisedStringsTests, ConstructFromFileContents)
{
    LocalisedStrings translations (validTranslationFile, false);
    EXPECT_EQ (translations.getLanguageName(), "English");
    EXPECT_EQ (translations.getCountryCodes(), (StringArray { "us", "gb", "au" }));
    EXPECT_EQ (translations.translate ("hello"), "hello");
    EXPECT_EQ (translations.translate ("nonexistent"), "nonexistent");
}

TEST_F (LocalisedStringsTests, ConstructFromFileWithEscapedQuotes)
{
    LocalisedStrings translations (translationFileWithEscapedQuotes, false);
    EXPECT_EQ (translations.translate ("a \"quoted\" string"), "une \"chaîne\" citée");
}

TEST_F (LocalisedStringsTests, ConstructFromFileIgnoresInvalidEntries)
{
    LocalisedStrings translations (invalidTranslationFile, false);
    EXPECT_EQ (translations.translate ("hello"), "hello"); // Not translated.
    EXPECT_EQ (translations.getMappings().size(), 0);      // No valid mappings.
}

TEST_F (LocalisedStringsTests, IgnoreCaseOfKeys)
{
    LocalisedStrings translations (validTranslationFile, true);
    EXPECT_EQ (translations.translate ("HELLO"), "hello");
    EXPECT_EQ (translations.translate ("goodBYE"), "goodbye");
}

TEST_F (LocalisedStringsTests, AddStringsMergesTranslations)
{
    LocalisedStrings translations1 ("language: English\n\"hello\" = \"hi\"\n", false);
    LocalisedStrings translations2 ("language: English\n\"goodbye\" = \"bye\"\n", false);
    translations1.addStrings (translations2);

    EXPECT_EQ (translations1.translate ("hello"), "hi");
    EXPECT_EQ (translations1.translate ("goodbye"), "bye");
}

TEST_F (LocalisedStringsTests, DISABLED_AddStringsWithConflictingLanguageThrows)
{
    /*
    LocalisedStrings translations1 ("language: English\n\"hello\" = \"hi\"\n", false);
    LocalisedStrings translations2 ("language: French\n\"bonjour\" = \"hello\"\n", false);
    EXPECT_DEATH (translations1.addStrings (translations2), ".*");
    */
}

TEST_F (LocalisedStringsTests, SetAndGetCurrentMappings)
{
    auto* translations = new LocalisedStrings (validTranslationFile, false);
    LocalisedStrings::setCurrentMappings (translations);
    EXPECT_EQ (LocalisedStrings::getCurrentMappings(), translations);
    EXPECT_EQ (LocalisedStrings::translateWithCurrentMappings ("hello"), "hello");
    EXPECT_EQ (LocalisedStrings::translateWithCurrentMappings ("nonexistent"), "nonexistent");
}

TEST_F (LocalisedStringsTests, FallbackTranslations)
{
    LocalisedStrings primary ("language: English\n\"hello\" = \"hi\"\n", false);
    auto* fallback = new LocalisedStrings ("language: English\n\"goodbye\" = \"bye\"\n", false);
    primary.setFallback (fallback);

    EXPECT_EQ (primary.translate ("hello"), "hi");    // From primary
    EXPECT_EQ (primary.translate ("goodbye"), "bye"); // From fallback
    EXPECT_EQ (primary.translate ("nonexistent"), "nonexistent");
}

TEST_F (LocalisedStringsTests, TranslateWithResultIfNotFound)
{
    LocalisedStrings translations ("language: English\n\"hello\" = \"hi\"\n", false);

    EXPECT_EQ (translations.translate ("hello", "not found"), "hi");              // Found translation
    EXPECT_EQ (translations.translate ("nonexistent", "not found"), "not found"); // Not found
}

TEST_F (LocalisedStringsTests, ConstructFromFile)
{
    auto tempDir = File::getSpecialLocation (File::SpecialLocationType::tempDirectory);
    auto testFile = tempDir.getChildFile ("test_translations.txt");

    testFile.replaceWithText (validTranslationFile);

    LocalisedStrings translations (testFile, false);

    EXPECT_EQ (translations.getLanguageName(), "English");
    EXPECT_EQ (translations.translate ("hello"), "hello");
    EXPECT_EQ (translations.translate ("goodbye"), "goodbye");

    testFile.deleteFile();
}

TEST_F (LocalisedStringsTests, CopyConstructor)
{
    LocalisedStrings original (validTranslationFile, false);
    LocalisedStrings copy (original);

    EXPECT_EQ (copy.getLanguageName(), original.getLanguageName());
    EXPECT_EQ (copy.getCountryCodes(), original.getCountryCodes());
    EXPECT_EQ (copy.translate ("hello"), "hello");
    EXPECT_EQ (copy.translate ("goodbye"), "goodbye");
}

TEST_F (LocalisedStringsTests, AssignmentOperator)
{
    LocalisedStrings original (validTranslationFile, false);
    LocalisedStrings assigned ("language: French\n", false);

    assigned = original;

    EXPECT_EQ (assigned.getLanguageName(), original.getLanguageName());
    EXPECT_EQ (assigned.getCountryCodes(), original.getCountryCodes());
    EXPECT_EQ (assigned.translate ("hello"), "hello");
    EXPECT_EQ (assigned.translate ("goodbye"), "goodbye");
}

TEST_F (LocalisedStringsTests, TranslateWithCurrentMappingsStaticMethod)
{
    auto* translations = new LocalisedStrings (validTranslationFile, false);
    LocalisedStrings::setCurrentMappings (translations);

    EXPECT_EQ (LocalisedStrings::translateWithCurrentMappings ("hello"), "hello");
    EXPECT_EQ (LocalisedStrings::translateWithCurrentMappings ("goodbye"), "goodbye");
    EXPECT_EQ (LocalisedStrings::translateWithCurrentMappings ("nonexistent"), "nonexistent");
}

TEST_F (LocalisedStringsTests, GlobalTranslateFunctionWithCharPointerUTF8)
{
    auto* translations = new LocalisedStrings (validTranslationFile, false);
    LocalisedStrings::setCurrentMappings (translations);

    CharPointer_UTF8 text1 ("hello");
    CharPointer_UTF8 text2 ("nonexistent");

    EXPECT_EQ (yup::translate (text1), "hello");
    EXPECT_EQ (yup::translate (text2), "nonexistent");
}

TEST_F (LocalisedStringsTests, TranslateUsesResultIfNotFoundWhenKeyMissing)
{
    LocalisedStrings translations ("language: English\n\"hello\" = \"hi\"\n", false);

    // Test the fallback behavior when translation is not found
    EXPECT_EQ (translations.translate ("missing", "fallback"), "fallback");
    EXPECT_EQ (translations.translate ("another_missing", "custom fallback"), "custom fallback");
}
