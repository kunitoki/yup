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
    A declarative window container for `.yui` markup trees.

    `WindowComponent` is registered in @ref ComponentTypeRegistry under the `Window` type name.
    Unlike @ref DocumentWindow, constructing a `WindowComponent` does not open a native desktop
    window, so `.yui` documents rooted at `Window { ... }` can be embedded inside @ref YuiView,
    plugin editors, previews and headless tests while stacking and laying out their children across
    the window bounds and optionally painting a `backgroundColor`.

    @see StackComponent, DocumentWindow, YuiView

    @tags{GUI}
*/
class YUP_API WindowComponent : public StackComponent
{
public:
    //==============================================================================
    /** Creates an empty window container. */
    WindowComponent();

    /** Creates an empty window container with an identifier. */
    explicit WindowComponent (StringRef componentID);

    /** Destructor. */
    ~WindowComponent() override;

    //==============================================================================
    /** Style identifiers for theme and stylesheet customization. */
    struct Style
    {
        static const Identifier backgroundColorId; /**< Optional fill color painted behind the window's children. */
    };

    //==============================================================================
    /** Returns the static type info describing the Window type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WindowComponent)
};

} // namespace yup
