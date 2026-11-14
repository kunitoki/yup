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
    The base of the components that lay their children out with a layout algorithm.

    A layout component positions its visible children whenever it is resized, its children change,
    a child's layout item property changes, or a stylesheet restyles it. Subclasses implement
    @ref performLayout and read each child's layout item properties (`flexGrow`, `margin`,
    `gridArea`, ...) through the helpers here.

    Its own properties are `padding`, the space kept inside its bounds, and `gap`, the space kept
    between the children.

    @see FlexBoxComponent, GridComponent, StackComponent

    @tags{GUI}
*/
class YUP_API LayoutComponent : public Component
{
public:
    //==============================================================================
    /** The names of the layout properties, those of the containers and those of their items. */
    struct YUP_API Properties
    {
        //! Container properties
        static const Identifier paddingId;
        static const Identifier gapId;
        static const Identifier directionId;
        static const Identifier wrapId;
        static const Identifier justifyId;
        static const Identifier alignId;
        static const Identifier columnsId;
        static const Identifier rowsId;
        static const Identifier areasId;

        //! Item properties
        static const Identifier flexGrowId;
        static const Identifier flexShrinkId;
        static const Identifier flexBasisId;
        static const Identifier orderId;
        static const Identifier alignSelfId;
        static const Identifier justifySelfId;
        static const Identifier marginId;
        static const Identifier preferredWidthId;
        static const Identifier preferredHeightId;
        static const Identifier minWidthId;
        static const Identifier maxWidthId;
        static const Identifier minHeightId;
        static const Identifier maxHeightId;
        static const Identifier gridAreaId;
        static const Identifier gridColumnId;
        static const Identifier gridRowId;
        static const Identifier gridColumnSpanId;
        static const Identifier gridRowSpanId;
    };

    //==============================================================================
    /** Creates a layout component. */
    LayoutComponent()
    {
        setOpaque (false);
    }

    /** Creates a layout component with an identifier. */
    explicit LayoutComponent (StringRef componentID)
        : Component (componentID)
    {
        setOpaque (false);
    }

    //==============================================================================
    /** Sets the space kept inside the bounds of the container, in pixels. */
    void setPadding (const EdgeSpacing<float>& newPadding);

    /** Returns the space kept inside the bounds of the container. */
    EdgeSpacing<float> getPadding() const;

    /** Sets the space kept between the children, in pixels. */
    void setGap (float newGap);

    /** Returns the space kept between the children. */
    float getGap() const;

    //==============================================================================
    /** Lays the children out again. Called automatically when anything affecting the layout changes. */
    void updateLayout();

    //==============================================================================
    /** Returns the static type info describing the Layout type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getTypeInfo() const override;

    /** @internal */
    void resized() override;

    /** @internal */
    void childrenChanged() override;

    /** @internal */
    void childLayoutChanged (Component& child) override;

    /** @internal */
    void styleChanged() override;

protected:
    //==============================================================================
    /** Positions the children inside @p area, the bounds of the container minus its padding. */
    virtual void performLayout (Rectangle<float> area) = 0;

    //==============================================================================
    /** Returns the visible children, in the order they must be laid out in. */
    std::vector<Component*> getLayoutChildren() const;

    /** Reads a layout item property of a child as a number. */
    static float getItemNumber (const Component& child, const Identifier& name);

    /** Reads a layout item property of a child as an integer. */
    static int getItemInteger (const Component& child, const Identifier& name);

    /** Reads a layout item property of a child as a string. */
    static String getItemString (const Component& child, const Identifier& name);

    /** Reads the `margin` property of a child. */
    static EdgeSpacing<float> getItemMargin (const Component& child);

    /** Converts a number, or a string of 1, 2 or 4 values in CSS order, into an edge spacing. */
    static EdgeSpacing<float> parseSpacing (const var& value);

    /** Splits a track or area list on whitespace and commas. */
    static StringArray splitTokens (const String& text);

private:
    bool isPerformingLayout = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LayoutComponent)
};

} // namespace yup
