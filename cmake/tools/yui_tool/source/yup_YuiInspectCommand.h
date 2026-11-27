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

//==============================================================================
/** Describes the layout of a document as `inspect` prints it: its components with their place,
    and what looks wrong with them.

    @param document     A document loaded and laid out, as the one of a view is.
    @param input        The file the document was loaded from.
    @param withDrawings True to tell, for each component a drawing paints, where the elements of
                        the drawing that have an id land in the interface.
    @param only         A component of the interface to describe alone with what it holds, or
                        nullptr for the whole interface.

    @returns The description, or nothing for a document with no interface.
*/
yup::var describeYuiToolLayout (yup::YuiDocument& document, const yup::File& input, bool withDrawings = false, const yup::Component* only = nullptr);

/** Returns the component a description of the layout gives a path, or nullptr. */
yup::Component* findYuiToolComponent (yup::YuiDocument& document, const yup::String& path);

/** Returns the component of an interface named by its id, or else by the path a description of
    the layout gives it, or nullptr.
*/
yup::Component* findYuiToolElement (yup::YuiDocument& document, const yup::String& idOrPath);

/** Tells a component as a description of the layout names it: its type, its id when it has one
    and its path. A component the document does not describe, such as a part a widget made for
    itself, is told as the one of the document it belongs to.

    @returns The description, or nothing for a component that is not of the interface.
*/
yup::var describeYuiToolComponent (yup::YuiDocument& document, const yup::Component& component);
