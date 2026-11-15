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
    A layout component laying its children out on a grid.

    The container properties are `columns` and `rows`, which take either a track count or a track
    list (`"1fr 80px auto 20%"`), `areas`, a list of rows of area names separated by commas
    (`"head head, side main"`), and `justify` and `align`, on top of the `padding` and `gap` of
    @ref LayoutComponent.

    Each child is placed with its `gridColumn`, `gridRow`, `gridColumnSpan` and `gridRowSpan`, or
    with `gridArea` when the container declares areas, and sized with `preferredWidth`,
    `preferredHeight`, the minimum and maximum sizes, `margin`, `justifySelf` and `alignSelf`.

    @see Grid, FlexBoxComponent

    @tags{GUI}
*/
class YUP_API GridComponent : public LayoutComponent
{
public:
    //==============================================================================
    /** Creates an empty grid. */
    GridComponent() = default;

    /** Creates a grid with an identifier. */
    explicit GridComponent (StringRef componentID)
        : LayoutComponent (componentID)
    {
    }

    //==============================================================================
    /** Sets the column tracks, as a count or a track list. */
    void setColumns (const var& columns);

    /** Sets the row tracks, as a count or a track list. */
    void setRows (const var& rows);

    //==============================================================================
    /** Parses a track list such as `"1fr 80px auto 20%"`, `"minmax(80px, 1fr)"`, `"fit-content(120px)"`
        or `"repeat(3, 1fr)"`, or a plain track count. Returns an empty array when invalid. */
    static Array<Grid::TrackInfo> parseTracks (const var& value);

    //==============================================================================
    /** Returns the static type info describing the Grid type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override;

protected:
    /** @internal */
    void performLayout (Rectangle<float> area) override;

    /** @internal */
    bool usesOnlyStandardLayoutProperties() const noexcept override { return true; }

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GridComponent)
};

} // namespace yup
