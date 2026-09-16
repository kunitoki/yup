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

    Returning false means the gesture was not exported - either the platform has no implementation,
    or the payload cannot be represented on the system clipboard. The caller keeps the drag in flight
    in that case, so a drag that cannot be exported still behaves as an in-app one.

    Only what can be expressed on the system clipboard travels: files, text and URIs today. The `var`
    native object in a `DragAndDropData` is same-process only and is never exported, and an image-only
    payload is not exported yet either.

    @param sourceComponent  The component the drag started from, which the platform may snapshot for
                            the native drag image.
    @param data             The payload to place on the system clipboard.
    @param onComplete       Called on the message thread once the drag is over, with the action the
                            platform reported (`DragAndDropAction::none` when nothing was performed).

    @returns True when a native drag was started; false when the gesture could not be exported, in
             which case @a onComplete is not called and the caller keeps the drag as an in-app one.

    @note A platform drag runs its own event loop. A blocking implementation (macOS) returns only once
          the drag is over, while Windows starts the drag on a worker thread and returns immediately,
          so the outcome always arrives through @a onComplete.

    @see DragAndDropSource, DragAndDropManager
*/
bool performNativeDrag (Component& sourceComponent,
                        const DragAndDropData& data,
                        std::function<void (std::optional<DragAndDropAction>)> onComplete);

} // namespace yup
