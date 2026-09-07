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

#include <gtest/gtest.h>

#include <yup_core/yup_core.h>

using namespace yup;

class MimeTypeTableTests : public ::testing::Test
{
};

TEST_F (MimeTypeTableTests, KnownExtensionReturnsExpectedType)
{
    const auto types = MimeTypeTable::getMimeTypesForFileExtension ("txt");

    EXPECT_TRUE (types.contains ("text/plain"));
}

TEST_F (MimeTypeTableTests, KnownExtensionWithMultipleTypesReturnsAll)
{
    // "aif" maps to two MIME types, so this exercises the equal_range/for_each
    // loop that collects more than one match.
    const auto types = MimeTypeTable::getMimeTypesForFileExtension ("aif");

    EXPECT_EQ (2, types.size());
    EXPECT_TRUE (types.contains ("audio/aiff"));
    EXPECT_TRUE (types.contains ("audio/x-aiff"));
}

TEST_F (MimeTypeTableTests, KnownTypeReturnsExpectedExtensions)
{
    const auto extensions = MimeTypeTable::getFileExtensionsForMimeType ("image/png");

    EXPECT_TRUE (extensions.contains ("png"));
}

TEST_F (MimeTypeTableTests, UnknownExtensionReturnsEmptyArray)
{
    EXPECT_TRUE (MimeTypeTable::getMimeTypesForFileExtension ("not-a-real-extension").isEmpty());
}

TEST_F (MimeTypeTableTests, UnknownTypeReturnsEmptyArray)
{
    EXPECT_TRUE (MimeTypeTable::getFileExtensionsForMimeType ("not/a-real-type").isEmpty());
}

TEST_F (MimeTypeTableTests, RegisterCustomMimeTypeIsQueryableBothWays)
{
    // Use a distinctive extension/type pair, since the table is a process-wide
    // singleton shared with the ~700 built-in entries and with other tests.
    MimeTypeTable::registerCustomMimeTypeForFileExtension ("application/x-yup-test", "yuptest");

    EXPECT_TRUE (MimeTypeTable::getMimeTypesForFileExtension ("yuptest").contains ("application/x-yup-test"));
    EXPECT_TRUE (MimeTypeTable::getFileExtensionsForMimeType ("application/x-yup-test").contains ("yuptest"));
}
