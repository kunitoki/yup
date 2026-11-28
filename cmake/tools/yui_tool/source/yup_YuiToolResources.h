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

#include <yup_gui_markup/yup_gui_markup.h>

#include <functional>

//==============================================================================
/** Waits for the files the scenes of an interface are loading: no message loop brings them in while a command runs. */
void finishLoadingYuiToolScenes (yup::Component& component);

/** Checks what a document needs that only shows once it is used or shown, for one that is checked with no window: its
    shaders are loaded whether a component uses them or not, and its scenes run the frame a window showing them starts with.
*/
void checkYuiToolResources (yup::YuiDocument& document);

/** Draws a component with no window, as a window showing it ends up drawing it.

    A window draws frame after frame, and a `Scene3D` only shows its final picture after some of
    them: it builds what it left to its first frame, then refines what it shows over the next ones.
    So the component is drawn a frame at a time until every scene in it is final, for a bounded
    number of frames. They are given no time, so what is animated stands where it starts. A scene
    still changing after them is reported as a warning of its document. A component holding no
    scene is drawn once.

    @param component What to draw: the component of a view, or a frame showing a part of it.
    @param snapshot  Draws the component once, into an image that is not valid where nothing can draw.

    @returns The image, which is not valid where nothing can draw, or why a scene could not be
             drawn: it then shows that in its place, which is not the picture asked for.
*/
yup::ResultValue<yup::Image> drawYuiToolImage (yup::Component& component, const std::function<yup::Image()>& snapshot);
