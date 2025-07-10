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
/**
    Specifies the positioning of an item relative to its target area.
*/
class Justification
{
public:
    //==============================================================================

    /** Flags for the justification. */
    enum Flags
    {
        left = 1 << 0,             /**< Aligns the content to the left. */
        right = 1 << 1,            /**< Aligns the content to the right. */
        horizontalCenter = 1 << 2, /**< Centers the content horizontally. */

        top = 1 << 3,            /**< Aligns the content to the top. */
        bottom = 1 << 4,         /**< Aligns the content to the bottom. */
        verticalCenter = 1 << 5, /**< Centers the content vertically. */

        topLeft = left | top,         /**< Aligns the content to the top left corner. */
        topRight = right | top,       /**< Aligns the content to the top right corner. */
        bottomLeft = left | bottom,   /**< Aligns the content to the bottom left corner. */
        bottomRight = right | bottom, /**< Aligns the content to the bottom right corner. */

        centerLeft = left | verticalCenter,         /**< Aligns the content to the left and centers it vertically. */
        centerTop = horizontalCenter | top,         /**< Centers the content horizontally and aligns it to the top. */
        center = horizontalCenter | verticalCenter, /**< Centers the content both horizontally and vertically. */
        centerRight = right | verticalCenter,       /**< Aligns the content to the right and centers it vertically. */
        centerBottom = horizontalCenter | bottom    /**< Centers the content horizontally and aligns it to the bottom. */
    };

    //==============================================================================

    /** Constructor. */
    constexpr Justification (Flags flags) noexcept
        : flags (flags)
    {
    }

    /** Copy constructor. */
    constexpr Justification (const Justification& other) noexcept
        : flags (other.flags)
    {
    }

    /** Assignment operator. */
    constexpr Justification& operator= (const Justification& other) noexcept
    {
        flags = other.flags;
        return *this;
    }

    //==============================================================================

    /** Get the flags. */
    constexpr Flags getFlags() const noexcept
    {
        return flags;
    }

    //==============================================================================

    /** Test if the flags contain the specified flags. */
    constexpr bool testFlags (Flags flagsToTest) const noexcept
    {
        return (static_cast<int> (flags) & static_cast<int> (flagsToTest)) == static_cast<int> (flagsToTest);
    }

    /** Test if the flags contain the specified flags. */
    constexpr bool testFlags (Justification flagsToTest) const noexcept
    {
        return (static_cast<int> (flags) & static_cast<int> (flagsToTest.flags)) == static_cast<int> (flagsToTest.flags);
    }

    //==============================================================================

    /** Add the specified flags to the current flags. */
    constexpr Justification withAddedFlags (Flags newFlags) const noexcept
    {
        return static_cast<Flags> (static_cast<int> (flags) | static_cast<int> (newFlags));
    }

    /** Remove the specified flags from the current flags. */
    constexpr Justification withRemovedFlags (Flags newFlags) const noexcept
    {
        return static_cast<Flags> (static_cast<int> (flags) & ~static_cast<int> (newFlags));
    }

    //==============================================================================

    /** Bitwise OR operator. */
    friend constexpr Justification operator| (Justification lhs, Justification rhs) noexcept
    {
        return static_cast<Flags> (static_cast<int> (lhs.flags) | static_cast<int> (rhs.flags));
    }

    /** Bitwise AND operator. */
    friend constexpr Justification operator& (Justification lhs, Justification rhs) noexcept
    {
        return static_cast<Flags> (static_cast<int> (lhs.flags) & static_cast<int> (rhs.flags));
    }

    /** Bitwise NOT operator. */
    friend constexpr Justification operator~(Justification lhs) noexcept
    {
        return static_cast<Flags> (~static_cast<int> (lhs.flags));
    }

    //==============================================================================

    /** Equality operator. */
    friend constexpr bool operator== (Justification lhs, Justification rhs) noexcept
    {
        return lhs.flags == rhs.flags;
    }

    /** Inequality operator. */
    friend constexpr bool operator!= (Justification lhs, Justification rhs) noexcept
    {
        return lhs.flags != rhs.flags;
    }

    /** Equality operator. */
    friend constexpr bool operator== (Justification lhs, Flags rhs) noexcept
    {
        return lhs.flags == rhs;
    }

    /** Inequality operator. */
    friend constexpr bool operator!= (Justification lhs, Flags rhs) noexcept
    {
        return lhs.flags != rhs;
    }

    /** Equality operator. */
    friend constexpr bool operator== (Flags lhs, Justification rhs) noexcept
    {
        return lhs == rhs.flags;
    }

    /** Inequality operator. */
    friend constexpr bool operator!= (Flags lhs, Justification rhs) noexcept
    {
        return lhs != rhs.flags;
    }

private:
    Flags flags;
};

} // namespace yup
