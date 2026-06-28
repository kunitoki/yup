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

TEST (ImageFormatWriterTests, WriterHasCorrectFormatName)
{
    PpmImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);

    EXPECT_EQ (writer.getFormatName(), String ("PPM/PGM/PBM Image"));
}

TEST (ImageFormatWriterTests, WriterFlushReturnsTrue)
{
    PpmImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);

    EXPECT_TRUE (writer.flush());
}

TEST (ImageFormatWriterTests, WriterReturnsCorrectPixelFormat)
{
    PpmImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);

    EXPECT_EQ (writer.getPixelFormat(), PixelFormat::RGB);
}

TEST (ImageFormatWriterTests, WriterReturnsCorrectGrayscalePixelFormat)
{
    PpmImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::Grayscale);

    EXPECT_EQ (writer.getPixelFormat(), PixelFormat::Grayscale);
}
