/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

namespace yup
{

//==============================================================================
/** Defines types of stroke used in graphical contexts.

    This class encapsulates the type of stroke used in graphical contexts.
    It includes the width of the stroke, the cap type, and the join type.
*/
class YUP_API StrokeType
{
public:
    //==============================================================================
    /** Constructs a default stroke type with width 1.0, cap butt, and join miter. */
    StrokeType() = default;

    /** Constructs a stroke type with the given width.

        @param width The width of the stroke.
    */
    StrokeType (float width) noexcept
        : width (width)
    {
    }

    /** Constructs a stroke type with the given width and join.

        @param width The width of the stroke.
        @param join The join type of the stroke.
    */
    StrokeType (float width, StrokeJoin join) noexcept
        : width (width)
        , join (join)
    {
    }

    /** Constructs a stroke type with the given width and cap.

        @param width The width of the stroke.
        @param cap The cap type of the stroke.
    */
    StrokeType (float width, StrokeCap cap) noexcept
        : width (width)
        , cap (cap)
    {
    }

    /** Constructs a stroke type with the given width, join, and cap.

        @param width The width of the stroke.
        @param join The join type of the stroke.
        @param cap The cap type of the stroke.
    */
    StrokeType (float width, StrokeJoin join, StrokeCap cap) noexcept
        : width (width)
        , join (join)
        , cap (cap)
    {
    }

    //==============================================================================
    /** Copy constructor. */
    StrokeType (const StrokeType& other) noexcept = default;

    /** Move constructor. */
    StrokeType (StrokeType&& other) noexcept = default;

    /** Copy assignment operator. */
    StrokeType& operator= (const StrokeType& other) noexcept = default;

    /** Move assignment operator. */
    StrokeType& operator= (StrokeType&& other) noexcept = default;

    //==============================================================================
    /** Returns the width of the stroke. */
    float getWidth() const noexcept
    {
        return width;
    }

    /** Returns a new stroke type with the given width. */
    StrokeType withWidth (float newWidth) const noexcept
    {
        return StrokeType (newWidth, join, cap);
    }

    //==============================================================================
    /** Returns the cap type of the stroke. */
    StrokeCap getCap() const noexcept
    {
        return cap;
    }

    /** Returns a new stroke type with the given cap. */
    StrokeType withCap (StrokeCap newCap) const noexcept
    {
        return StrokeType (width, join, newCap);
    }

    //==============================================================================
    /** Returns the join type of the stroke. */
    StrokeJoin getJoin() const noexcept
    {
        return join;
    }

    /** Returns a new stroke type with the given join. */
    StrokeType withJoin (StrokeJoin newJoin) const noexcept
    {
        return StrokeType (width, newJoin, cap);
    }

    //==============================================================================
    /** Returns true if the stroke types are equal. */
    bool operator== (const StrokeType& other) const noexcept
    {
        return width == other.width && cap == other.cap && join == other.join;
    }

    /** Returns true if the stroke types are not equal. */
    bool operator!= (const StrokeType& other) const noexcept
    {
        return ! operator== (other);
    }

private:
    float width = 1.0f;
    StrokeCap cap = StrokeCap::Butt;
    StrokeJoin join = StrokeJoin::Miter;
};

} // namespace yup
