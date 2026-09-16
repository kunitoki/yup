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

TEST (DragAndDropDataTests, DefaultIsEmpty)
{
    DragAndDropData data;

    EXPECT_TRUE (data.isEmpty());
    EXPECT_FALSE (data.hasFiles());
    EXPECT_FALSE (data.hasText());
    EXPECT_FALSE (data.hasUris());
    EXPECT_TRUE (data.getFiles().isEmpty());
    EXPECT_TRUE (data.getText().isEmpty());
    EXPECT_TRUE (data.getUris().isEmpty());
}

TEST (DragAndDropDataTests, WithFilesSetsFiles)
{
    Array<File> files;
    files.add (File ("/tmp/one.txt"));
    files.add (File ("/tmp/two.txt"));

    auto data = DragAndDropData().withFiles (files);

    EXPECT_TRUE (data.hasFiles());
    EXPECT_FALSE (data.isEmpty());
    EXPECT_EQ (data.getFiles().size(), 2);
    EXPECT_EQ (data.getFiles()[0], File ("/tmp/one.txt"));
}

TEST (DragAndDropDataTests, WithTextSetsText)
{
    auto data = DragAndDropData().withText ("hello");

    EXPECT_TRUE (data.hasText());
    EXPECT_FALSE (data.isEmpty());
    EXPECT_EQ (data.getText(), String ("hello"));
}

TEST (DragAndDropDataTests, WithUrisSetsUris)
{
    StringArray uris;
    uris.add ("https://example.com");

    auto data = DragAndDropData().withUris (uris);

    EXPECT_TRUE (data.hasUris());
    EXPECT_FALSE (data.isEmpty());
    EXPECT_EQ (data.getUris().size(), 1);
    EXPECT_EQ (data.getUris()[0], String ("https://example.com"));
}

TEST (DragAndDropDataTests, BuildersAreImmutable)
{
    DragAndDropData original;
    auto withText = original.withText ("hello");

    EXPECT_TRUE (original.isEmpty());
    EXPECT_TRUE (withText.hasText());
}

TEST (DragAndDropDataTests, BuildersChainAndPreservePreviousValues)
{
    Array<File> files;
    files.add (File ("/tmp/one.txt"));

    auto data = DragAndDropData()
                    .withFiles (files)
                    .withText ("hello");

    EXPECT_TRUE (data.hasFiles());
    EXPECT_TRUE (data.hasText());
    EXPECT_EQ (data.getFiles().size(), 1);
    EXPECT_EQ (data.getText(), String ("hello"));
}

TEST (DragAndDropDataTests, EmptyTextDoesNotCountAsText)
{
    auto data = DragAndDropData().withText ("");

    EXPECT_FALSE (data.hasText());
    EXPECT_TRUE (data.isEmpty());
}

// =============================================================================
// MIME store
// =============================================================================

TEST (DragAndDropDataTests, RoundTripsArbitraryMimeData)
{
    const String payload ("custom-payload");
    auto data = DragAndDropData().withMimeData ("application/x-yup-test",
                                                MemoryBlock (payload.toRawUTF8(), payload.getNumBytesAsUTF8()));

    EXPECT_TRUE (data.hasMimeData ("application/x-yup-test"));
    EXPECT_TRUE (data.hasMimeData ("Application/X-Yup-Test")); // MIME types are case-insensitive
    EXPECT_FALSE (data.isEmpty());
    EXPECT_EQ (data.getMimeData ("application/x-yup-test").toString(), payload);
    EXPECT_EQ (data.getMimeTypes().size(), 1);
    EXPECT_EQ (data.getMimeTypes()[0], String ("application/x-yup-test"));
}

TEST (DragAndDropDataTests, GetAllMimeDataExposesEveryEntry)
{
    StringArray uris;
    uris.add ("https://example.com");

    auto data = DragAndDropData().withText ("hello").withUris (uris);

    auto all = data.getAllMimeData();
    ASSERT_EQ (all.size(), 2u);

    StringArray types;
    for (const auto& entry : all)
        types.add (entry.mimeType);

    EXPECT_TRUE (types.contains (DragAndDropData::mimeTypeText));
    EXPECT_TRUE (types.contains (DragAndDropData::mimeTypeUriList));
}

TEST (DragAndDropDataTests, WithMimeDataReplacesExistingEntry)
{
    auto data = DragAndDropData()
                    .withMimeData ("application/x-yup-test", MemoryBlock ("one", 3))
                    .withMimeData ("application/x-yup-test", MemoryBlock ("two", 3));

    EXPECT_EQ (data.getMimeTypes().size(), 1);
    EXPECT_EQ (data.getMimeData ("application/x-yup-test").toString(), String ("two"));
}

TEST (DragAndDropDataTests, EmptyMimeDataRemovesEntry)
{
    auto data = DragAndDropData().withMimeData ("application/x-yup-test", MemoryBlock ("abc", 3));
    EXPECT_TRUE (data.hasMimeData ("application/x-yup-test"));

    auto cleared = data.withMimeData ("application/x-yup-test", MemoryBlock());

    EXPECT_FALSE (cleared.hasMimeData ("application/x-yup-test"));
    EXPECT_TRUE (cleared.isEmpty());
}

// =============================================================================
// File / URI round-tripping
// =============================================================================

TEST (DragAndDropDataTests, FilesRoundTripThroughUriList)
{
    Array<File> files;
    files.add (File ("/tmp/one.txt"));
    files.add (File ("/tmp/two.txt"));

    auto data = DragAndDropData().withFiles (files);

    EXPECT_TRUE (data.hasFiles());
    EXPECT_TRUE (data.hasUris());
    EXPECT_TRUE (data.hasMimeData (DragAndDropData::mimeTypeUriList));
    EXPECT_EQ (data.getFiles().size(), 2);
    EXPECT_EQ (data.getFiles()[0], File ("/tmp/one.txt"));
    EXPECT_EQ (data.getFiles()[1], File ("/tmp/two.txt"));
    EXPECT_EQ (data.getUris().size(), 2);
}

TEST (DragAndDropDataTests, UriListIgnoresCommentsAndBlankLines)
{
    const String list ("# this is a comment\r\n\r\nhttps://a.example/\r\nhttps://b.example/\r\n");
    auto data = DragAndDropData().withMimeData (DragAndDropData::mimeTypeUriList,
                                                MemoryBlock (list.toRawUTF8(), list.getNumBytesAsUTF8()));

    auto uris = data.getUris();
    ASSERT_EQ (uris.size(), 2);
    EXPECT_EQ (uris[0], String ("https://a.example/"));
    EXPECT_EQ (uris[1], String ("https://b.example/"));

    // Non-file URIs must not be reported as files.
    EXPECT_FALSE (data.hasFiles());
}

// =============================================================================
// Same-process native object
// =============================================================================

TEST (DragAndDropDataTests, NativeObjectIsVoidByDefault)
{
    DragAndDropData data;

    EXPECT_FALSE (data.hasNativeObject());
    EXPECT_TRUE (data.getNativeObject().isVoid());
}

TEST (DragAndDropDataTests, NativeObjectRoundTrips)
{
    auto data = DragAndDropData().withNativeObject (var (42));

    EXPECT_TRUE (data.hasNativeObject());
    EXPECT_FALSE (data.isEmpty());
    EXPECT_TRUE (data.getNativeObject().isInt());
    EXPECT_EQ (static_cast<int> (data.getNativeObject()), 42);
}

TEST (DragAndDropDataTests, NativeObjectAloneIsNotEmptyButMimeStoreIsEmpty)
{
    auto data = DragAndDropData().withNativeObject (var ("track"));

    EXPECT_FALSE (data.isEmpty());
    EXPECT_TRUE (data.getMimeTypes().isEmpty());
    EXPECT_TRUE (data.getAllMimeData().empty());
}

// =============================================================================
// Image payload
// =============================================================================

#if YUP_IMAGE_FORMAT_PNG
TEST (DragAndDropDataTests, ImageRoundTripsThroughPng)
{
    Image image (4, 3, PixelFormat::RGBA);
    image.fill (0xff3366aau);

    auto data = DragAndDropData().withImage (image);

    EXPECT_TRUE (data.hasImage());
    EXPECT_TRUE (data.hasMimeData (DragAndDropData::mimeTypePng));

    auto png = data.getMimeData (DragAndDropData::mimeTypePng);
    ASSERT_GE (png.getSize(), 8u);

    const auto* bytes = static_cast<const uint8*> (png.getData());
    EXPECT_EQ (bytes[0], 0x89);
    EXPECT_EQ (bytes[1], 'P');
    EXPECT_EQ (bytes[2], 'N');
    EXPECT_EQ (bytes[3], 'G');

    auto decoded = data.getImage();
    ASSERT_TRUE (decoded.isValid());
    EXPECT_EQ (decoded.getWidth(), 4);
    EXPECT_EQ (decoded.getHeight(), 3);
}
#endif

// =============================================================================
// Edge cases of the fluent builders
// =============================================================================

TEST (DragAndDropDataTests, WithImageIgnoresAnInvalidImage)
{
    // There is nothing to encode, so the payload comes back untouched rather than gaining an
    // empty PNG entry that would claim the drag carries an image.
    EXPECT_TRUE (DragAndDropData{}.withImage (Image()).isEmpty());
}

TEST (DragAndDropDataTests, WithMimeDataAcceptsAClipboardEntry)
{
    const String payload ("clipboard-payload");
    const auto bytes = MemoryBlock (payload.toRawUTF8(), payload.getNumBytesAsUTF8());

    auto data = DragAndDropData{}.withMimeData (ClipboardData ("application/x-yup-clipboard", bytes));

    EXPECT_TRUE (data.hasMimeData ("application/x-yup-clipboard"));
    EXPECT_EQ (bytes, data.getMimeData ("application/x-yup-clipboard"));
}

TEST (DragAndDropDataTests, GetMimeDataReturnsAnEmptyBlockForAnUnknownType)
{
    auto data = DragAndDropData{}.withText ("hello");

    // Asking for something the payload does not carry is not an error, it is simply empty.
    EXPECT_EQ (0u, data.getMimeData ("application/x-absent").getSize());
}
