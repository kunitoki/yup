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

float AnimationLayer::localFrame (float compFrame) const noexcept
{
    if (timeRemap.has_value())
        return timeRemap->getValueAt (compFrame);

    if (std::abs (timeStretch) <= 1.0e-6f)
        return compFrame - startFrame;

    return (compFrame - startFrame) / timeStretch;
}

bool AnimationLayer::isVisibleAt (float compFrame) const noexcept
{
    return ! hidden && compFrame >= inFrame && compFrame < outFrame;
}

} // namespace yup
