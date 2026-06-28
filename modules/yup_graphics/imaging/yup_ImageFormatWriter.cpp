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

namespace yup
{

ImageFormatWriter::ImageFormatWriter (OutputStream* destStream,
                                      const String& formatName_,
                                      PixelFormat pixelFormat_)
    : output (destStream)
    , formatName (formatName_)
    , pixelFormat (pixelFormat_)
{
}

ImageFormatWriter::~ImageFormatWriter() = default;

bool ImageFormatWriter::flush()
{
    return true;
}

bool ImageFormatWriter::beginAnimation (int /*loopCount*/)
{
    jassertfalse;
    return false;
}

bool ImageFormatWriter::writeFrame (const Image& /*frame*/, int /*delayMs*/)
{
    jassertfalse;
    return false;
}

bool ImageFormatWriter::endAnimation()
{
    jassertfalse;
    return false;
}

} // namespace yup
