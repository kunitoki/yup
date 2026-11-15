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
    A component style that paints a component from a drawing tree.

    A DrawingStyle wraps a DrawingProgram as a ComponentStyle, so a data-driven drawing
    can be installed on a component with Component::setStyle and used like any other
    style. Every paint feeds the component's live bounds and reflected properties into
    the drawing, so it reflects the component's current state.

    @code
    auto style = DrawingStyle::create (slider, drawing);
    if (style.wasOk())
        slider.setStyle (style.getReference());
    @endcode

    The style is bound to the component it was created for: install it on that component
    only. The drawing resolves and feeds that component's properties, so painting a
    different component through the same style is not supported.

    @see DrawingProgram, DrawingScope, ComponentStyle

    @tags{UI}
*/
class YUP_API DrawingStyle : public ComponentStyle
{
public:
    //==============================================================================
    /** A pointer to a DrawingStyle object. */
    using Ptr = ReferenceCountedObjectPtr<DrawingStyle>;

    //==============================================================================
    /** Creates a style that paints a component from a drawing tree.

        @param target     The component the style is created for and installed on.
        @param drawing    The drawing tree (see DrawingProgram::create).

        @return A style on success, or a failure carrying the compiler message.
    */
    static ResultValue<DrawingStyle::Ptr> create (Component& target,
                                                  const DataTree& drawing,
                                                  DrawingScope::ComponentResolver componentResolver = nullptr,
                                                  DrawingScope::ModelResolver modelResolver = nullptr);

    //==============================================================================
    /** Returns the properties the drawing declares (see DrawingProgram::getProperties). */
    const std::vector<ComponentProperty>& getProperties() const noexcept;

    //==============================================================================
    /** @internal */
    void paint (Graphics& g, const ApplicationTheme& theme, const Component& component) override;

private:
    //==============================================================================
    explicit DrawingStyle (std::unique_ptr<DrawingProgram> programToUse);

    std::unique_ptr<DrawingProgram> program;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrawingStyle)
};

} // namespace yup
