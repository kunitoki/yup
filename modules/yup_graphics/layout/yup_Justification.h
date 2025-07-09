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
enum class Justification
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

constexpr Justification operator| (Justification lhs, Justification rhs) noexcept
{
    return static_cast<Justification> (static_cast<int> (lhs) | static_cast<int> (rhs));
}

constexpr Justification operator& (Justification lhs, Justification rhs) noexcept
{
    return static_cast<Justification> (static_cast<int> (lhs) & static_cast<int> (rhs));
}

constexpr Justification operator~(Justification lhs) noexcept
{
    return static_cast<Justification> (~static_cast<int> (lhs));
}

} // namespace yup
