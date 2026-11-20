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
/**
    Writes a `.yui` document tree back to text.

    It is the reverse of YuiParser: parsing the written text gives back an equivalent tree,
    so tools can load a document, edit the tree and save it. Every block and element is
    written with four spaces of indentation per level, the properties of an element before
    its children. Comments aren't part of the tree, so they aren't preserved, and neither
    are the original layout and the order of properties relative to children.

    @code
    const auto result = YuiParser::parse (text);
    if (result.wasOk())
        file.replaceWithText (YuiWriter::write (result.document));
    @endcode

    @see YuiParser

    @tags{UI}
*/
class YUP_API YuiWriter
{
public:
    //==============================================================================
    /** Writes a document, or a single block or element of one, as `.yui` text.

        @param tree  A `yui` document as made by YuiParser, or any element of one.

        @returns The text, ending with a new line.
    */
    static String write (const DataTree& tree);

    /** Deleted constructor to prevent instantiation of this utility class. */
    YuiWriter() = delete;
};

} // namespace yup
