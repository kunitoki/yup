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

#include <yup_graphics/yup_graphics.h>

using namespace yup;

TEST (ImageFormatReaderTests, ReaderHasCorrectFormatName)
{
    // Minimal valid P6 stream: "P6\n1 1\n255\n" + 3 RGB bytes
    const char data[] = "P6\n1 1\n255\n\xFF\x00\x00";
    auto* stream = new MemoryInputStream (data, sizeof (data) - 1, false);

    PpmImageFormatReader reader (stream);

    EXPECT_EQ (reader.getFormatName(), String ("PPM/PGM/PBM Image"));
}

TEST (ImageFormatReaderTests, ReaderSetsWidthAndHeightFromHeader)
{
    // P6 header declaring a 4x8 image (no pixel data needed — we only test header parsing)
    const char data[] = "P6\n4 8\n255\n";
    auto* stream = new MemoryInputStream (data, sizeof (data) - 1, false);

    PpmImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 4);
    EXPECT_EQ (reader.height, 8);
    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGB);
}

TEST (ImageFormatReaderTests, ReaderHasZeroWidthHeightForInvalidStream)
{
    // Invalid magic number — reader should leave width/height at zero
    const char data[] = "XX\n4 8\n255\n";
    auto* stream = new MemoryInputStream (data, sizeof (data) - 1, false);

    PpmImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 0);
    EXPECT_EQ (reader.height, 0);
}
