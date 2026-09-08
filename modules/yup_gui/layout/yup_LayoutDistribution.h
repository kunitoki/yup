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

namespace yup
{

//==============================================================================
/**
    How a run of boxes shares out the space left over on one axis.

    FlexBox uses this for `justify-content` (items along the main axis) and
    `align-content` (lines along the cross axis); Grid uses it for
    `justify-content` and `align-content` over its tracks. Keeping one
    definition of what each mode means is what stops the four of them drifting
    apart - historically `spaceAround` was implemented as `space-evenly` on two
    of them and correctly on neither.

    @see LayoutDistribution

    @tags{GUI}
*/
enum class LayoutDistributionMode
{
    start,        /**< Pack at the start, leaving the free space at the end. */
    end,          /**< Pack at the end, leaving the free space at the start. */
    center,       /**< Pack in the middle, splitting the free space evenly. */
    spaceBetween, /**< Even space between boxes, none at the edges. */
    spaceAround,  /**< A full space between boxes and a half space at each edge. */
    spaceEvenly,  /**< An equal space between boxes and at both edges. */
    stretch       /**< Grow every box equally to consume the free space. */
};

//==============================================================================
/**
    The result of sharing free space out over a run of boxes.

    @see LayoutDistribution

    @tags{GUI}
*/
struct LayoutDistribution
{
    float leading = 0.0f;  /**< Space before the first box. */
    float between = 0.0f;  /**< Space between two adjacent boxes, gap included. */
    float growEach = 0.0f; /**< Extra size handed to every box (stretch only). */

    //==============================================================================
    /**
        Shares `freeSpace` out over `count` boxes separated by `gap`.

        Negative free space (the content overflows) is handled the way the CSS
        box alignment module requires rather than by scaling the overflow:
        `spaceBetween` falls back to `start` and both `spaceAround` and
        `spaceEvenly` fall back to `center`, so an overflowing run stays
        predictable instead of interleaving its boxes.

        @param mode       which alignment mode to apply.
        @param freeSpace  the space left over, which may be negative.
        @param count      how many boxes share the space.
        @param gap        the fixed gap already required between two boxes.

        @returns the leading offset, the spacing to insert between boxes, and
                 the extra size each box should grow by.
    */
    static LayoutDistribution calculate (LayoutDistributionMode mode, float freeSpace, int count, float gap)
    {
        if (count <= 0)
            return { 0.0f, gap, 0.0f };

        const auto n = static_cast<float> (count);

        switch (mode)
        {
            case LayoutDistributionMode::start:
                return { 0.0f, gap, 0.0f };

            case LayoutDistributionMode::end:
                return { freeSpace, gap, 0.0f };

            case LayoutDistributionMode::center:
                return { freeSpace / 2.0f, gap, 0.0f };

            case LayoutDistributionMode::spaceBetween:
                if (count < 2 || freeSpace < 0.0f)
                    return { 0.0f, gap, 0.0f };

                return { 0.0f, gap + freeSpace / (n - 1.0f), 0.0f };

            case LayoutDistributionMode::spaceAround:
                if (freeSpace < 0.0f)
                    return { freeSpace / 2.0f, gap, 0.0f };

                return { freeSpace / (2.0f * n), gap + freeSpace / n, 0.0f };

            case LayoutDistributionMode::spaceEvenly:
                if (freeSpace < 0.0f)
                    return { freeSpace / 2.0f, gap, 0.0f };

                return { freeSpace / (n + 1.0f), gap + freeSpace / (n + 1.0f), 0.0f };

            case LayoutDistributionMode::stretch:
                if (freeSpace <= 0.0f)
                    return { 0.0f, gap, 0.0f };

                return { 0.0f, gap, freeSpace / n };
        }

        return { 0.0f, gap, 0.0f };
    }
};

} // namespace yup
