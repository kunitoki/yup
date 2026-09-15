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

//==============================================================================
#if !YUP_MAC && !YUP_WINDOWS
/** No implementation on this platform yet, so nothing is exported - which is what tells the manager
    to keep treating the gesture as an in-app drag.

    macOS has its own in `native/yup_NativeDragAndDrop_mac.mm` and Windows in
    `native/yup_NativeDragAndDrop_windows.cpp`; X11 will add theirs, and each one takes its platform
    out of this guard as it lands, so exactly one definition is ever compiled on any given platform. */
std::optional<DragAndDropAction> performNativeDrag (Component&, const DragAndDropData&)
{
    return std::nullopt;
}
#endif

} // namespace yup
