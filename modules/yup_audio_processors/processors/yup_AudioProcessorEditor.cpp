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

namespace yup
{

//==============================================================================

AudioProcessorEditor::AudioProcessorEditor()
{
}

AudioProcessorEditor::~AudioProcessorEditor()
{
}

//==============================================================================

Size<int> AudioProcessorEditor::getConstrainedSize (Size<int> requestedSize) const
{
    if (! isResizable())
        return getPreferredSize();

    const auto width = jmax (1, requestedSize.getWidth());
    const auto height = jmax (1, requestedSize.getHeight());

    const auto ratioSize = getPreferredAspectRatioSize();
    if (! shouldPreserveAspectRatio() || ratioSize.getWidth() <= 0 || ratioSize.getHeight() <= 0)
        return { width, height };

    const auto ratio = static_cast<double> (ratioSize.getWidth()) / static_cast<double> (ratioSize.getHeight());

    // Independent of the current size, so hosts repeating unconstrained sizes while dragging always get the same answer
    if (static_cast<double> (width) / static_cast<double> (height) >= ratio)
        return { width, jmax (1, roundToInt (width / ratio)) };

    return { jmax (1, roundToInt (height * ratio)), height };
}

} // namespace yup
