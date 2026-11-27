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

#pragma once

namespace yup
{

//==============================================================================
/** A text as whoever paints a widget laid it out, kept with what it was laid out from.

    A widget showing a text holds one of these and never looks into it: the theme painting the widget lays the text
    out its own way, in its own font, and keeps the result here to lay it out again only when it is given another
    text, another font or another size.

    @code
    auto& kept = item.getTextLayoutCache();

    if (kept.text != text || kept.font != font || kept.styledText.getMaxSize() != area.getSize())
    {
        auto modifier = kept.styledText.startUpdate();
        modifier.clear();
        modifier.setMaxSize (area.getSize());
        modifier.appendText (text, font);

        kept.text = text;
        kept.font = font;
    }

    g.fillFittedText (kept.styledText, area);
    @endcode

    @see StyledText, ApplicationTheme
*/
struct TextLayoutCache
{
    TextLayoutCache() = default;

    /** The text as it was laid out, which tells the size it was given and how it was aligned. */
    StyledText styledText;

    /** The text that was laid out. */
    String text;

    /** The font it was laid out in. */
    Font font;

private:
    YUP_DECLARE_NON_COPYABLE (TextLayoutCache)
};

} // namespace yup
