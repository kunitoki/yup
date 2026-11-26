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

class Component;

//==============================================================================
/**
    Describes a part of what a component shows, that whoever paints the component can place.

    Some of what a widget shows can't be told as shapes: the text it laid out, its caret and its
    selection, the icon it was given. A widget declares each as a part: its name, where the widget
    puts it and whether it can go elsewhere. Painting a part is up to the theme, as painting the
    widget is (see ApplicationTheme::setComponentPart), so a part has one painter, which the
    painter of the widget calls and a `.yui` drawing places with a `part` element.

    A part is painted with the paint set on the Graphics: a text is filled with the fill color, a
    text outline is stroked with the stroke color and width. What has colors of its own, as an
    image or the tokens of a code editor, ignores them.

    @code
    // The type of the widget declares the part
    .fixedPart ("text", &TextButton::getTextBounds, "The text of the button, filled with the fill color.")

    // The theme paints it
    theme->setComponentPart<TextButton> ("text", [] (Graphics& g, const ApplicationTheme&, const TextButton& button, const Rectangle<float>& area)
    {
        g.fillFittedText (button.getStyledText(), area);
    });
    @endcode

    @see ComponentTypeInfo::findPart, ComponentTypeBuilder::part, ComponentTypeBuilder::fixedPart,
         ApplicationTheme::setComponentPart, Component::paintPart

    @tags{UI}
*/
struct ComponentPart
{
    //==============================================================================
    /** Returns where a component puts the part itself, in its own coordinates. */
    using AreaGetter = std::function<Rectangle<float> (const Component& component)>;

    //==============================================================================
    /** The name of the part, as a `.yui` drawing names it: `part text { }`. */
    Identifier name;

    /** Where the component puts the part itself. Null for the bounds of the component. */
    AreaGetter area;

    /** True when the part is painted in whatever area it is given, so a drawing can move it.

        False for a part the component has to lay out itself, such as the text of an editor, whose
        caret and hit testing follow where it is. Such a part is always painted where @ref area tells.
    */
    bool isPlaceable = true;

    /** A human readable description, surfaced by the schema and tooling. */
    String description;

    //==============================================================================
    /** Returns where a component puts the part itself: what @ref area tells, or its bounds. */
    Rectangle<float> getArea (const Component& component) const;
};

} // namespace yup
