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

namespace yup
{

/** System clipboard.

    This class provides methods to copy and retrieve text from the system clipboard.

    @note This class is not thread-safe.
*/
class YUP_API SystemClipboard
{
public:
    /** Copies the given text to the system clipboard.

        @param text The text to copy to the clipboard.
    */
    static void copyTextToClipboard (const String& text);

    /** Retrieves the text from the system clipboard.

        @return The text from the clipboard.
    */
    static String getTextFromClipboard();
};

} // namespace yup
