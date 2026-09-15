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
/** Hands a drag that has left every YUP window over to the operating system.

    This is the seam between the in-app drag machinery and each platform's own drag-and-drop
    implementation. `DragAndDropManager` calls it once per drag, at the point where the pointer is
    over none of our windows and the source asked for external drags, and the platform runs the
    native drag from there.

    Returning `std::nullopt` means the gesture was not exported - either the platform has no
    implementation, or the payload cannot be represented on the system clipboard. The caller keeps
    the drag in flight in that case, so a drag that cannot be exported still behaves as an in-app one.

    Only what can be expressed on the system clipboard travels: files, text and URIs today. The `var`
    native object in a `DragAndDropData` is same-process only and is never exported, and an image-only
    payload is not exported yet either.

    @param sourceComponent  The component the drag started from, which the platform may snapshot for
                            the native drag image.
    @param data             The payload to place on the system clipboard.

    @returns The action the platform reported, or `std::nullopt` when the drag was not exported.

    @note macOS returns `DragAndDropAction::none` once it has handed the gesture to AppKit, because the
          session runs inside AppKit's own event handling and the operation the destination chose only
          arrives in the session's "ended" callback. Reporting that operation back to the source is
          the next step for that platform; until then the source is told nothing was performed.

    @see DragAndDropSource, DragAndDropManager
*/
std::optional<DragAndDropAction> performNativeDrag (Component& sourceComponent, const DragAndDropData& data);

} // namespace yup
