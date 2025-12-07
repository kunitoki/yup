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
using namespace yup::detail;

class MimeTypeTableTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Register some custom MIME types for testing
        MimeTypeTable::registerCustomMimeTypeForFileExtension ("application/x-test", "test");
        MimeTypeTable::registerCustomMimeTypeForFileExtension ("text/x-custom", "custom");
    }
};

TEST_F (MimeTypeTableTests, RegisterCustomMimeType)
{
    MimeTypeTable::registerCustomMimeTypeForFileExtension ("application/x-example", "example");

    auto mimeTypes = MimeTypeTable::getMimeTypesForFileExtension ("example");
    EXPECT_GT (mimeTypes.size(), 0);
    EXPECT_TRUE (mimeTypes.contains ("application/x-example"));
}

TEST_F (MimeTypeTableTests, GetMimeTypesForFileExtension)
{
    auto mimeTypes = MimeTypeTable::getMimeTypesForFileExtension ("test");
    EXPECT_GT (mimeTypes.size(), 0);
    EXPECT_TRUE (mimeTypes.contains ("application/x-test"));
}

TEST_F (MimeTypeTableTests, GetMimeTypesForCommonExtensions)
{
    // Test common file extensions
    auto jpgTypes = MimeTypeTable::getMimeTypesForFileExtension ("jpg");
    EXPECT_GT (jpgTypes.size(), 0);
    EXPECT_TRUE (jpgTypes.contains ("image/jpeg"));

    auto pngTypes = MimeTypeTable::getMimeTypesForFileExtension ("png");
    EXPECT_GT (pngTypes.size(), 0);
    EXPECT_TRUE (pngTypes.contains ("image/png"));

    auto txtTypes = MimeTypeTable::getMimeTypesForFileExtension ("txt");
    EXPECT_GT (txtTypes.size(), 0);
    EXPECT_TRUE (txtTypes.contains ("text/plain"));

    auto htmlTypes = MimeTypeTable::getMimeTypesForFileExtension ("html");
    EXPECT_GT (htmlTypes.size(), 0);
    EXPECT_TRUE (htmlTypes.contains ("text/html"));
}

TEST_F (MimeTypeTableTests, GetFileExtensionsForMimeType)
{
    auto extensions = MimeTypeTable::getFileExtensionsForMimeType ("application/x-test");
    EXPECT_GT (extensions.size(), 0);
    EXPECT_TRUE (extensions.contains ("test"));
}

TEST_F (MimeTypeTableTests, GetFileExtensionsForCommonMimeTypes)
{
    auto jpegExtensions = MimeTypeTable::getFileExtensionsForMimeType ("image/jpeg");
    EXPECT_GT (jpegExtensions.size(), 0);
    EXPECT_TRUE (jpegExtensions.contains ("jpg") || jpegExtensions.contains ("jpeg"));

    auto pngExtensions = MimeTypeTable::getFileExtensionsForMimeType ("image/png");
    EXPECT_GT (pngExtensions.size(), 0);
    EXPECT_TRUE (pngExtensions.contains ("png"));

    auto textExtensions = MimeTypeTable::getFileExtensionsForMimeType ("text/plain");
    EXPECT_GT (textExtensions.size(), 0);
    EXPECT_TRUE (textExtensions.contains ("txt"));
}

TEST_F (MimeTypeTableTests, UnknownExtensionReturnsEmptyArray)
{
    auto mimeTypes = MimeTypeTable::getMimeTypesForFileExtension ("unknownextension12345");
    EXPECT_EQ (mimeTypes.size(), 0);
}

TEST_F (MimeTypeTableTests, UnknownMimeTypeReturnsEmptyArray)
{
    auto extensions = MimeTypeTable::getFileExtensionsForMimeType ("application/x-unknown-mime-type-12345");
    EXPECT_EQ (extensions.size(), 0);
}

TEST_F (MimeTypeTableTests, CaseInsensitiveExtensionLookup)
{
    auto mimeTypesLower = MimeTypeTable::getMimeTypesForFileExtension ("jpg");
    auto mimeTypesUpper = MimeTypeTable::getMimeTypesForFileExtension ("JPG");
    auto mimeTypesMixed = MimeTypeTable::getMimeTypesForFileExtension ("JpG");

    EXPECT_GT (mimeTypesLower.size(), 0);
    EXPECT_EQ (mimeTypesLower.size(), mimeTypesUpper.size());
    EXPECT_EQ (mimeTypesLower.size(), mimeTypesMixed.size());
}

TEST_F (MimeTypeTableTests, MultipleExtensionsForSameMimeType)
{
    // JPEG can have multiple extensions
    auto jpegExtensions = MimeTypeTable::getFileExtensionsForMimeType ("image/jpeg");
    EXPECT_GT (jpegExtensions.size(), 0);

    // Verify we can look up MIME type from different extensions
    auto jpgTypes = MimeTypeTable::getMimeTypesForFileExtension ("jpg");
    auto jpegTypes = MimeTypeTable::getMimeTypesForFileExtension ("jpeg");

    EXPECT_TRUE (jpgTypes.contains ("image/jpeg"));
    EXPECT_TRUE (jpegTypes.contains ("image/jpeg"));
}

TEST_F (MimeTypeTableTests, RegisterDuplicateDoesNotCauseDuplicates)
{
    MimeTypeTable::registerCustomMimeTypeForFileExtension ("application/x-duplicate", "dup");
    MimeTypeTable::registerCustomMimeTypeForFileExtension ("application/x-duplicate", "dup");

    auto mimeTypes = MimeTypeTable::getMimeTypesForFileExtension ("dup");
    EXPECT_GT (mimeTypes.size(), 0);

    int count = 0;
    for (const auto& type : mimeTypes)
    {
        if (type == "application/x-duplicate")
            ++count;
    }

    EXPECT_EQ (count, 1);
}
