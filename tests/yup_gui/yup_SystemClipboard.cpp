/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

namespace
{

MemoryBlock makeMemoryBlock (const char* data, size_t size)
{
    return MemoryBlock (data, size);
}

class SystemClipboardTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_TRUE (SystemClipboard::clearClipboardData());
    }

    void TearDown() override
    {
        EXPECT_TRUE (SystemClipboard::clearClipboardData());
    }
};

} // namespace

TEST (ClipboardDataTests, DefaultConstructionCreatesEmptyData)
{
    ClipboardData data;

    EXPECT_TRUE (data.mimeType.isEmpty());
    EXPECT_EQ (data.data.getSize(), 0u);
}

TEST (ClipboardDataTests, ConstructionStoresMimeTypeAndPayload)
{
    constexpr char payload[] = { 'y', 'u', 'p' };
    ClipboardData data ("application/x-yup-test", makeMemoryBlock (payload, sizeof (payload)));

    EXPECT_EQ (data.mimeType, String ("application/x-yup-test"));
    EXPECT_EQ (data.data, makeMemoryBlock (payload, sizeof (payload)));
}

TEST_F (SystemClipboardTests, CopiesAndRetrievesText)
{
    SystemClipboard::copyTextToClipboard ("clipboard text");

    EXPECT_TRUE (SystemClipboard::hasClipboardText());
    EXPECT_EQ (SystemClipboard::getTextFromClipboard(), String ("clipboard text"));
}

TEST_F (SystemClipboardTests, EmptyTextIsReportedAsUnavailable)
{
    SystemClipboard::copyTextToClipboard (String());

    EXPECT_FALSE (SystemClipboard::hasClipboardText());
    EXPECT_TRUE (SystemClipboard::getTextFromClipboard().isEmpty());
}

TEST_F (SystemClipboardTests, CopiesAndRetrievesSingleMimeType)
{
    constexpr char payload[] = { 'y', '\0', 'u', 'p' };
    const String mimeType = "application/x-yup-test";
    const MemoryBlock expected (payload, sizeof (payload));

    ASSERT_TRUE (SystemClipboard::copyToClipboard ({ mimeType, expected }));

    EXPECT_TRUE (SystemClipboard::hasClipboardData (mimeType));
    EXPECT_TRUE (SystemClipboard::getClipboardMimeTypes().contains (mimeType));

    const auto result = SystemClipboard::getFromClipboard (mimeType);
    EXPECT_EQ (result.mimeType, mimeType);
    EXPECT_EQ (result.data, expected);
}

TEST_F (SystemClipboardTests, CopiesAndRetrievesMultipleMimeTypes)
{
    constexpr char firstPayload[] = { 'o', 'n', 'e' };
    constexpr char secondPayload[] = { 't', 'w', 'o' };
    const String firstMimeType = "application/x-yup-first";
    const String secondMimeType = "application/x-yup-second";
    Array<ClipboardData> items;
    items.add ({ firstMimeType, makeMemoryBlock (firstPayload, sizeof (firstPayload)) });
    items.add ({ secondMimeType, makeMemoryBlock (secondPayload, sizeof (secondPayload)) });

    ASSERT_TRUE (SystemClipboard::copyToClipboard (items));

    const auto mimeTypes = SystemClipboard::getClipboardMimeTypes();
    EXPECT_TRUE (mimeTypes.contains (firstMimeType));
    EXPECT_TRUE (mimeTypes.contains (secondMimeType));
    EXPECT_EQ (SystemClipboard::getFromClipboard (firstMimeType).data,
               makeMemoryBlock (firstPayload, sizeof (firstPayload)));
    EXPECT_EQ (SystemClipboard::getFromClipboard (secondMimeType).data,
               makeMemoryBlock (secondPayload, sizeof (secondPayload)));
}

TEST_F (SystemClipboardTests, MissingMimeTypeReturnsEmptyData)
{
    const String mimeType = "application/x-yup-missing";

    EXPECT_FALSE (SystemClipboard::hasClipboardData (mimeType));

    const auto result = SystemClipboard::getFromClipboard (mimeType);
    EXPECT_TRUE (result.mimeType.isEmpty());
    EXPECT_EQ (result.data.getSize(), 0u);
}

TEST_F (SystemClipboardTests, ClearingClipboardReleasesOwnedData)
{
    constexpr char payload[] = { 'y', 'u', 'p' };
    bool released = false;

    ASSERT_TRUE (SystemClipboard::copyToClipboard (
        { "application/x-yup-test", makeMemoryBlock (payload, sizeof (payload)) },
        [&released]
    {
        released = true;
    }));
    EXPECT_FALSE (released);

    EXPECT_TRUE (SystemClipboard::clearClipboardData());
    EXPECT_TRUE (released);
    EXPECT_TRUE (SystemClipboard::getClipboardMimeTypes().isEmpty());
}

TEST_F (SystemClipboardTests, CopiesAndRetrievesPrimarySelectionText)
{
    SystemClipboard::copyTextToPrimarySelection ("primary selection");

    EXPECT_TRUE (SystemClipboard::hasPrimarySelectionText());
    EXPECT_EQ (SystemClipboard::getTextFromPrimarySelection(), String ("primary selection"));

    SystemClipboard::copyTextToPrimarySelection (String());
    EXPECT_FALSE (SystemClipboard::hasPrimarySelectionText());
    EXPECT_TRUE (SystemClipboard::getTextFromPrimarySelection().isEmpty());
}
