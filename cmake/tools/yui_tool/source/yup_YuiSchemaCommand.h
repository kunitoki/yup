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
/** Describes a component type as `schema` does.

    @param includeInherited True to list the properties and signals of its bases too, each with
                            the type it comes from.
*/
yup::var describeYuiToolComponentType (const yup::ComponentTypeInfo& info, bool includeInherited);

/** Describes what a document can name beyond its component types, as the code checking documents
    knows it: `drawing` for the elements of a drawing with their properties, `styles` for what a
    rule and a selector take, `expressions` for the functions, members and enumerations, `shaders`
    for how a shader is declared, written and used and, in a tool that has the 3D component types,
    `scenes` for how a scene is written.

    @returns The description, or nothing for a name that is none of these topics.
*/
yup::var describeYuiToolSchemaTopic (const yup::String& topic);

/** Returns the names of the topics describeYuiToolSchemaTopic() describes in this tool. */
yup::StringArray getYuiToolSchemaTopics();
