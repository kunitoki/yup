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
FlexBox::FlexBox (Direction d)
    : flexDirection (d)
{
}

FlexBox::FlexBox (Direction d, Wrap w, AlignItems ai, JustifyContent jc, AlignContent ac)
    : flexDirection (d)
    , flexWrap (w)
    , alignItems (ai)
    , justifyContent (jc)
    , alignContent (ac)
{
}

//==============================================================================
namespace
{

bool isRowDirection (FlexBox::Direction direction)
{
    return direction == FlexBox::Direction::row || direction == FlexBox::Direction::rowReverse;
}

bool isReverseDirection (FlexBox::Direction direction)
{
    return direction == FlexBox::Direction::rowReverse || direction == FlexBox::Direction::columnReverse;
}

/** Clamps a size against a min/max pair where a negative bound means "unset". */
float applyMinMax (float size, float minSize, float maxSize)
{
    if (minSize >= 0.0f)
        size = std::max (size, minSize);

    if (maxSize >= 0.0f)
        size = std::min (size, maxSize);

    return size;
}

//==============================================================================
/**
    The min-content and max-content sizes of an item along one axis.

    @see getIntrinsicSize
*/
struct IntrinsicSize
{
    float minContent = 0.0f;
    float maxContent = 0.0f;
};

/**
    The single point at which the flex algorithm asks an item how big its
    content is.

    CSS needs this query in several places - `flex-basis: auto`, an auto cross
    size, and the §4.5 automatic minimum size that floors flex-shrink - and YUP
    has no content-measurement hook on Component yet, so this is deliberately a
    stub with a designed-for shape rather than a real implementation.

    It reports:

    - `maxContent` as the component's current bounds, which is the established
      YUP behaviour for an unspecified size (measured/intrinsic sizing).
    - `minContent` as 0. A box whose content cannot be measured has no known
      lower bound, and 0 is the only safe floor. This matches a browser exactly
      whenever the real min-content size is 0 (an empty box), and under-reports
      otherwise - a divergence that is documented rather than papered over,
      because substituting the current size here would make a shrinking item
      refuse to shrink at all.

    When Component grows a `getContentSize()` hook, only this function changes:
    every caller in the file routes through it, so the freeze loop, the line
    breaker and the cross-size resolution do not need to be re-plumbed.
*/
IntrinsicSize getIntrinsicSize (const FlexItem& item, bool horizontal)
{
    const float measured = item.associatedComponent != nullptr
                             ? (horizontal ? item.associatedComponent->getWidth()
                                           : item.associatedComponent->getHeight())
                             : 0.0f;

    return { 0.0f, measured };
}

//==============================================================================
LayoutDistributionMode toDistribution (FlexBox::JustifyContent value)
{
    switch (value)
    {
        case FlexBox::JustifyContent::flexStart:    return LayoutDistributionMode::start;
        case FlexBox::JustifyContent::flexEnd:      return LayoutDistributionMode::end;
        case FlexBox::JustifyContent::center:       return LayoutDistributionMode::center;
        case FlexBox::JustifyContent::spaceBetween: return LayoutDistributionMode::spaceBetween;
        case FlexBox::JustifyContent::spaceAround:  return LayoutDistributionMode::spaceAround;
        case FlexBox::JustifyContent::spaceEvenly:  return LayoutDistributionMode::spaceEvenly;
    }

    return LayoutDistributionMode::start;
}

LayoutDistributionMode toDistribution (FlexBox::AlignContent value)
{
    switch (value)
    {
        case FlexBox::AlignContent::flexStart:    return LayoutDistributionMode::start;
        case FlexBox::AlignContent::flexEnd:      return LayoutDistributionMode::end;
        case FlexBox::AlignContent::center:       return LayoutDistributionMode::center;
        case FlexBox::AlignContent::spaceBetween: return LayoutDistributionMode::spaceBetween;
        case FlexBox::AlignContent::spaceAround:  return LayoutDistributionMode::spaceAround;
        case FlexBox::AlignContent::spaceEvenly:  return LayoutDistributionMode::spaceEvenly;
        case FlexBox::AlignContent::stretch:      return LayoutDistributionMode::stretch;
    }

    return LayoutDistributionMode::stretch;
}

//==============================================================================
/**
    An item with every input the algorithm needs resolved exactly once.

    Sizes and margins are expressed in the container's *logical* axes: main and
    cross, with "start" always meaning the direction the items flow in. The
    mapping back to physical left/top/width/height - including the mirroring
    that row-reverse, column-reverse and wrap-reverse need - happens in a single
    step at the very end, so no pass in between has to think about direction.
*/
struct ResolvedItem
{
    FlexItem* item = nullptr;
    int sourceOrder = 0;

    float baseMainSize = 0.0f;         /**< The flex base size. */
    float hypotheticalMainSize = 0.0f; /**< Base size clamped by min/max. */
    float mainSize = 0.0f;             /**< Final size, after the freeze loop. */

    float hypotheticalCrossSize = 0.0f; /**< Cross size before stretching. */
    float crossSize = 0.0f;             /**< Final cross size. */

    float mainMarginStart = 0.0f;
    float mainMarginEnd = 0.0f;
    float crossMarginStart = 0.0f;
    float crossMarginEnd = 0.0f;

    bool mainMarginStartIsAuto = false;
    bool mainMarginEndIsAuto = false;
    bool crossMarginStartIsAuto = false;
    bool crossMarginEndIsAuto = false;

    float minMainSize = -1.0f;
    float maxMainSize = -1.0f;
    float minCrossSize = -1.0f;
    float maxCrossSize = -1.0f;

    float flexGrow = 0.0f;
    float flexShrink = 1.0f;

    FlexItem::AlignSelf align = FlexItem::AlignSelf::flexStart;
    bool crossSizeIsAuto = false;

    bool frozen = false;
    float violation = 0.0f;

    float mainPosition = 0.0f;
    float crossPosition = 0.0f;

    /** The item's outer (margin box) size along the main axis. */
    float outerMainSize() const noexcept { return mainSize + mainMarginStart + mainMarginEnd; }

    /** The item's outer (margin box) size along the cross axis, before stretching. */
    float outerHypotheticalCrossSize() const noexcept { return hypotheticalCrossSize + crossMarginStart + crossMarginEnd; }

    /** The distance from the item's margin-box cross start to its baseline. */
    float baselineOffset() const noexcept { return item->baseline >= 0.0f ? item->baseline : crossSize; }

    /** How many of the item's cross-axis margins are auto. */
    int numAutoCrossMargins() const noexcept
    {
        return (crossMarginStartIsAuto ? 1 : 0) + (crossMarginEndIsAuto ? 1 : 0);
    }
};

/** A run of items sharing one flex line, as a range into the resolved array. */
struct FlexLine
{
    int firstItem = 0;
    int numItems = 0;
    float crossSize = 0.0f;
    float crossPosition = 0.0f;
};

//==============================================================================
/**
    Resolves the flexible lengths of one line per CSS Flexbox §9.7.

    A single proportional pass is not enough: an item whose grown or shrunk size
    violates its own min/max must be frozen at the clamped value and the space
    it could not take redistributed over the items that are still flexible. That
    loop is what makes `max-width` on a growing item behave, and what makes a
    `min-width` floor on a shrinking item push the deficit onto its siblings.
*/
void resolveFlexibleLengths (ResolvedItem* lineItems, int numItems, float containerMainSize, float gap)
{
    if (numItems <= 0)
        return;

    const float totalGap = gap * static_cast<float> (std::max (0, numItems - 1));

    float totalMargins = 0.0f;
    float totalHypothetical = 0.0f;

    for (int i = 0; i < numItems; ++i)
    {
        totalMargins += lineItems[i].mainMarginStart + lineItems[i].mainMarginEnd;
        totalHypothetical += lineItems[i].hypotheticalMainSize;
    }

    const float initialFreeSpace = containerMainSize - totalMargins - totalGap - totalHypothetical;
    const bool isGrowing = initialFreeSpace > 0.0f;

    // Freeze the items that cannot flex in the direction we need, and the ones
    // whose base size was already clamped away from that direction.
    for (int i = 0; i < numItems; ++i)
    {
        auto& r = lineItems[i];
        const float factor = isGrowing ? r.flexGrow : r.flexShrink;

        if (factor <= 0.0f
            || (isGrowing && r.baseMainSize > r.hypotheticalMainSize)
            || (! isGrowing && r.baseMainSize < r.hypotheticalMainSize))
        {
            r.frozen = true;
            r.mainSize = r.hypotheticalMainSize;
        }
        else
        {
            r.frozen = false;
            r.mainSize = r.baseMainSize;
        }
    }

    // Each iteration freezes at least one item, so numItems passes is an upper
    // bound; the extra one lets the final all-satisfied pass run.
    for (int pass = 0; pass <= numItems; ++pass)
    {
        int numUnfrozen = 0;
        float totalGrow = 0.0f;
        float totalShrink = 0.0f;
        float totalScaledShrink = 0.0f;
        float used = 0.0f;

        for (int i = 0; i < numItems; ++i)
        {
            const auto& r = lineItems[i];
            used += r.frozen ? r.mainSize : r.baseMainSize;

            if (r.frozen)
                continue;

            ++numUnfrozen;
            totalGrow += r.flexGrow;
            totalShrink += r.flexShrink;
            totalScaledShrink += r.flexShrink * r.baseMainSize;
        }

        if (numUnfrozen == 0)
            break;

        const float remainingFreeSpace = containerMainSize - totalMargins - totalGap - used;

        if (isGrowing)
        {
            if (totalGrow <= 0.0f)
                break;

            // §9.7.4b: flex factors summing to less than one only claim that
            // fraction of the original free space, so `flex-grow: 0.5` grows by
            // half the slack rather than absorbing all of it.
            float distributable = remainingFreeSpace;

            if (totalGrow < 1.0f && std::abs (initialFreeSpace * totalGrow) < std::abs (remainingFreeSpace))
                distributable = initialFreeSpace * totalGrow;

            for (int i = 0; i < numItems; ++i)
            {
                auto& r = lineItems[i];

                if (! r.frozen)
                    r.mainSize = r.baseMainSize + distributable * r.flexGrow / totalGrow;
            }
        }
        else
        {
            if (totalScaledShrink <= 0.0f)
                break;

            float distributable = remainingFreeSpace;

            if (totalShrink < 1.0f && std::abs (initialFreeSpace * totalShrink) < std::abs (remainingFreeSpace))
                distributable = initialFreeSpace * totalShrink;

            // Weighted by flexShrink * base size, so a large item gives up more
            // than a small one at the same shrink factor.
            for (int i = 0; i < numItems; ++i)
            {
                auto& r = lineItems[i];

                if (! r.frozen)
                    r.mainSize = r.baseMainSize + distributable * (r.flexShrink * r.baseMainSize) / totalScaledShrink;
            }
        }

        // Clamp, and total up which way the clamps pushed.
        float totalViolation = 0.0f;

        for (int i = 0; i < numItems; ++i)
        {
            auto& r = lineItems[i];

            if (r.frozen)
                continue;

            const float unclamped = r.mainSize;

            r.mainSize = applyMinMax (r.mainSize, r.minMainSize, r.maxMainSize);

            // The §4.5 automatic minimum size, via the D1 seam: with no
            // explicit min the floor is the item's min-content size.
            if (r.minMainSize < 0.0f)
                r.mainSize = std::max (r.mainSize, 0.0f);

            r.violation = r.mainSize - unclamped;
            totalViolation += r.violation;
        }

        // An absolute epsilon, not a relative one: the violations are pixel
        // sizes summed over the line, so comparing the total against zero
        // relatively (which is what approximatelyEqual would do) never
        // succeeds once rounding has crept in.
        constexpr float violationEpsilon = 1.0e-4f;

        bool frozeAnyItem = false;

        auto freeze = [&] (ResolvedItem& item)
        {
            item.frozen = true;
            frozeAnyItem = true;
        };

        if (std::abs (totalViolation) < violationEpsilon)
        {
            for (int i = 0; i < numItems; ++i)
                freeze (lineItems[i]);
        }
        else if (totalViolation > 0.0f)
        {
            for (int i = 0; i < numItems; ++i)
                if (lineItems[i].violation > 0.0f)
                    freeze (lineItems[i]);
        }
        else
        {
            for (int i = 0; i < numItems; ++i)
                if (lineItems[i].violation < 0.0f)
                    freeze (lineItems[i]);
        }

        // Every pass must make progress. One that freezes nothing would
        // otherwise repeat until the iteration bound and leave the sizes a
        // partial distribution happened to produce.
        if (! frozeAnyItem)
            for (int i = 0; i < numItems; ++i)
                lineItems[i].frozen = true;
    }

    for (int i = 0; i < numItems; ++i)
        lineItems[i].mainSize = std::max (0.0f, lineItems[i].mainSize);
}

} // namespace

//==============================================================================
void FlexBox::setPadding (float newPadding) noexcept
{
    paddingLeft = paddingRight = paddingTop = paddingBottom = newPadding;
}

void FlexBox::setPadding (float horizontal, float vertical) noexcept
{
    paddingLeft = paddingRight = horizontal;
    paddingTop = paddingBottom = vertical;
}

//==============================================================================
void FlexBox::performLayout (Rectangle<float> targetArea)
{
    if (items.isEmpty())
        return;

    const bool isRow = isRowDirection (flexDirection);
    const bool isReverseMain = isReverseDirection (flexDirection);
    const bool isReverseCross = (flexWrap == Wrap::wrapReverse);
    const bool isSingleLine = (flexWrap == Wrap::noWrap);

    jassert (gap >= 0.0f); // a negative gap is meaningless and is clamped away
    jassert (paddingLeft >= 0.0f && paddingRight >= 0.0f);
    jassert (paddingTop >= 0.0f && paddingBottom >= 0.0f);

    // Padding shrinks the content box the items are laid out in, so every
    // pass below works in content-box coordinates and only the final mapping
    // adds the origin back.
    const auto paddedArea = targetArea.reduced (std::max (0.0f, paddingLeft),
                                                std::max (0.0f, paddingTop),
                                                std::max (0.0f, paddingRight),
                                                std::max (0.0f, paddingBottom));

    // Padding larger than the area would otherwise invert the rectangle.
    const Rectangle<float> contentArea (paddedArea.getX(),
                                        paddedArea.getY(),
                                        std::max (0.0f, paddedArea.getWidth()),
                                        std::max (0.0f, paddedArea.getHeight()));

    const float containerMainSize = isRow ? contentArea.getWidth() : contentArea.getHeight();
    const float containerCrossSize = isRow ? contentArea.getHeight() : contentArea.getWidth();
    const float containerMainStart = isRow ? contentArea.getX() : contentArea.getY();
    const float containerCrossStart = isRow ? contentArea.getY() : contentArea.getX();

    // row-gap and column-gap fall back to the `gap` shorthand when unset. In a
    // row container the columns separate items and the rows separate lines; in
    // a column container it is the other way round.
    const float shorthandGap = std::max (0.0f, gap);
    const float usedRowGap = rowGap >= 0.0f ? rowGap : shorthandGap;
    const float usedColumnGap = columnGap >= 0.0f ? columnGap : shorthandGap;

    const float mainGap = isRow ? usedColumnGap : usedRowGap;
    const float crossGap = isRow ? usedRowGap : usedColumnGap;

    //==============================================================================
    // Pass 1: resolve every input once, in logical (main/cross) terms.

    Array<ResolvedItem> resolved;
    resolved.ensureStorageAllocated (items.size());

    for (int i = 0; i < items.size(); ++i)
    {
        auto& item = items.getReference (i);

        ResolvedItem r;
        r.item = &item;
        r.sourceOrder = item.order;

        jassert (item.flexGrow >= 0.0f);
        jassert (item.flexShrink >= 0.0f);
        r.flexGrow = std::max (0.0f, item.flexGrow);
        r.flexShrink = std::max (0.0f, item.flexShrink);

        // Margins follow the logical axes. In a reversed direction the item
        // flows from the far edge, so its *leading* margin is the physically
        // trailing one - which is why mirroring the final rectangle instead
        // makes marginLeft behave as a right margin in row-reverse.
        if (isRow)
        {
            r.mainMarginStart = isReverseMain ? item.marginRight : item.marginLeft;
            r.mainMarginEnd = isReverseMain ? item.marginLeft : item.marginRight;
            r.crossMarginStart = isReverseCross ? item.marginBottom : item.marginTop;
            r.crossMarginEnd = isReverseCross ? item.marginTop : item.marginBottom;

            r.mainMarginStartIsAuto = isReverseMain ? item.marginRightAuto : item.marginLeftAuto;
            r.mainMarginEndIsAuto = isReverseMain ? item.marginLeftAuto : item.marginRightAuto;
            r.crossMarginStartIsAuto = isReverseCross ? item.marginBottomAuto : item.marginTopAuto;
            r.crossMarginEndIsAuto = isReverseCross ? item.marginTopAuto : item.marginBottomAuto;

            r.minMainSize = item.minWidth;
            r.maxMainSize = item.maxWidth;
            r.minCrossSize = item.minHeight;
            r.maxCrossSize = item.maxHeight;
        }
        else
        {
            r.mainMarginStart = isReverseMain ? item.marginBottom : item.marginTop;
            r.mainMarginEnd = isReverseMain ? item.marginTop : item.marginBottom;
            r.crossMarginStart = isReverseCross ? item.marginRight : item.marginLeft;
            r.crossMarginEnd = isReverseCross ? item.marginLeft : item.marginRight;

            r.mainMarginStartIsAuto = isReverseMain ? item.marginBottomAuto : item.marginTopAuto;
            r.mainMarginEndIsAuto = isReverseMain ? item.marginTopAuto : item.marginBottomAuto;
            r.crossMarginStartIsAuto = isReverseCross ? item.marginRightAuto : item.marginLeftAuto;
            r.crossMarginEndIsAuto = isReverseCross ? item.marginLeftAuto : item.marginRightAuto;

            r.minMainSize = item.minHeight;
            r.maxMainSize = item.maxHeight;
            r.minCrossSize = item.minWidth;
            r.maxCrossSize = item.maxWidth;
        }

        // Flex base size: flex-basis, then a percentage, then a fixed size,
        // then the item's max-content size through the D1 seam.
        const float mainPercent = isRow ? item.widthPercent : item.heightPercent;
        const float mainFixed = isRow ? item.width : item.height;

        if (item.flexBasisPercent >= 0.0f)
            r.baseMainSize = containerMainSize * item.flexBasisPercent / 100.0f;
        else if (item.flexBasis >= 0.0f)
            r.baseMainSize = item.flexBasis;
        else if (mainPercent >= 0.0f)
            r.baseMainSize = containerMainSize * mainPercent / 100.0f;
        else if (mainFixed >= 0.0f)
            r.baseMainSize = mainFixed;
        else
            r.baseMainSize = getIntrinsicSize (item, isRow).maxContent;

        r.hypotheticalMainSize = applyMinMax (r.baseMainSize, r.minMainSize, r.maxMainSize);

        // Cross size: a percentage, then a fixed size, then max-content. Only
        // an item with none of those is "auto", and only an auto item is
        // resized by align-items: stretch.
        const float crossPercent = isRow ? item.heightPercent : item.widthPercent;
        const float crossFixed = isRow ? item.height : item.width;

        r.crossSizeIsAuto = (crossPercent < 0.0f && crossFixed < 0.0f);

        if (crossPercent >= 0.0f)
            r.hypotheticalCrossSize = containerCrossSize * crossPercent / 100.0f;
        else if (crossFixed >= 0.0f)
            r.hypotheticalCrossSize = crossFixed;
        else
            r.hypotheticalCrossSize = getIntrinsicSize (item, ! isRow).maxContent;

        r.hypotheticalCrossSize = applyMinMax (r.hypotheticalCrossSize, r.minCrossSize, r.maxCrossSize);

        // Resolve align-self against the container's align-items.
        r.align = item.alignSelf;

        if (r.align == FlexItem::AlignSelf::autoAlign)
        {
            switch (alignItems)
            {
                case AlignItems::flexStart: r.align = FlexItem::AlignSelf::flexStart; break;
                case AlignItems::flexEnd:   r.align = FlexItem::AlignSelf::flexEnd; break;
                case AlignItems::center:    r.align = FlexItem::AlignSelf::center; break;
                case AlignItems::stretch:   r.align = FlexItem::AlignSelf::stretch; break;
                case AlignItems::baseline:  r.align = FlexItem::AlignSelf::baseline; break;
            }
        }

        // In a column container the cross axis is the inline axis, where a box
        // with no text has no baseline to share, so CSS aligns it as
        // flex-start instead.
        if (r.align == FlexItem::AlignSelf::baseline && ! isRow)
            r.align = FlexItem::AlignSelf::flexStart;

        resolved.add (r);
    }

    // CSS orders by the `order` property but keeps source order within a group,
    // so this must be a stable sort.
    std::stable_sort (resolved.begin(), resolved.end(), [] (const ResolvedItem& a, const ResolvedItem& b)
    {
        return a.sourceOrder < b.sourceOrder;
    });

    //==============================================================================
    // Pass 2: collect items into lines.

    Array<FlexLine> lines;

    {
        FlexLine current;
        current.firstItem = 0;
        float lineMainSize = 0.0f;

        for (int i = 0; i < resolved.size(); ++i)
        {
            const auto& r = resolved.getReference (i);

            // CSS breaks on the hypothetical main size, and the gap that would
            // be inserted before this item counts against the line just like
            // the item does.
            const float outer = r.hypotheticalMainSize + r.mainMarginStart + r.mainMarginEnd;

            if (! isSingleLine && current.numItems > 0
                && lineMainSize + mainGap + outer > containerMainSize)
            {
                lines.add (current);

                current = {};
                current.firstItem = i;
                lineMainSize = 0.0f;
            }

            lineMainSize += (current.numItems > 0 ? mainGap : 0.0f) + outer;
            ++current.numItems;
        }

        if (current.numItems > 0)
            lines.add (current);
    }

    //==============================================================================
    // Pass 3: resolve flexible lengths, one line at a time.

    for (const auto& line : lines)
        resolveFlexibleLengths (resolved.begin() + line.firstItem, line.numItems, containerMainSize, mainGap);

    //==============================================================================
    // Pass 4: size the lines on the cross axis.

    for (auto& line : lines)
    {
        if (isSingleLine)
        {
            // A single-line container has exactly one line and it IS the
            // container on the cross axis - align-content does not apply. This
            // is what makes the default align-items: stretch actually stretch.
            line.crossSize = containerCrossSize;
            continue;
        }

        float maxOuter = 0.0f;
        float maxAscent = 0.0f;
        float maxDescent = 0.0f;
        bool hasBaselineItem = false;

        for (int i = 0; i < line.numItems; ++i)
        {
            const auto& r = resolved.getReference (line.firstItem + i);
            maxOuter = std::max (maxOuter, r.outerHypotheticalCrossSize());

            if (r.align != FlexItem::AlignSelf::baseline)
                continue;

            // A baseline-aligned item can force a taller line than the tallest
            // item on its own, because the shared baseline pushes items down.
            const float offset = r.item->baseline >= 0.0f ? r.item->baseline : r.hypotheticalCrossSize;

            hasBaselineItem = true;
            maxAscent = std::max (maxAscent, r.crossMarginStart + offset);
            maxDescent = std::max (maxDescent, r.hypotheticalCrossSize + r.crossMarginEnd - offset);
        }

        line.crossSize = hasBaselineItem ? std::max (maxOuter, maxAscent + maxDescent) : maxOuter;
    }

    //==============================================================================
    // Pass 5: place the lines along the cross axis (align-content).

    {
        float totalLinesCrossSize = crossGap * static_cast<float> (std::max (0, lines.size() - 1));

        for (const auto& line : lines)
            totalLinesCrossSize += line.crossSize;

        // align-content never applies to a single-line container; it does apply
        // to a wrapping container that happens to produce only one line.
        const auto distributed = isSingleLine
                                   ? LayoutDistribution { 0.0f, crossGap, 0.0f }
                                   : LayoutDistribution::calculate (toDistribution (alignContent),
                                                                    containerCrossSize - totalLinesCrossSize,
                                                                    lines.size(),
                                                                    crossGap);

        float crossPosition = distributed.leading;

        for (auto& line : lines)
        {
            line.crossSize += distributed.growEach;
            line.crossPosition = crossPosition;
            crossPosition += line.crossSize + distributed.between;
        }
    }

    //==============================================================================
    // Pass 6: place the items within each line, then map back to physical
    // coordinates and apply the bounds.

    for (const auto& line : lines)
    {
        auto* lineItems = resolved.begin() + line.firstItem;

        // The free space justify-content gets to play with is what is left
        // AFTER the flexible lengths were resolved. Reusing the pre-flex figure
        // hands the same space out twice and pushes items past the container.
        float usedMainSize = mainGap * static_cast<float> (std::max (0, line.numItems - 1));

        for (int i = 0; i < line.numItems; ++i)
            usedMainSize += lineItems[i].outerMainSize();

        float freeMainSpace = containerMainSize - usedMainSize;

        // Auto margins on the main axis take ALL the positive free space and
        // split it equally, before justify-content is consulted - which is why
        // `marginLeftAuto` beats a `justifyContent` of center.
        int numAutoMainMargins = 0;

        for (int i = 0; i < line.numItems; ++i)
            numAutoMainMargins += (lineItems[i].mainMarginStartIsAuto ? 1 : 0)
                                + (lineItems[i].mainMarginEndIsAuto ? 1 : 0);

        if (numAutoMainMargins > 0 && freeMainSpace > 0.0f)
        {
            const float share = freeMainSpace / static_cast<float> (numAutoMainMargins);

            for (int i = 0; i < line.numItems; ++i)
            {
                auto& r = lineItems[i];

                if (r.mainMarginStartIsAuto)
                    r.mainMarginStart = share;

                if (r.mainMarginEndIsAuto)
                    r.mainMarginEnd = share;
            }

            freeMainSpace = 0.0f;
        }

        const auto justified = LayoutDistribution::calculate (toDistribution (justifyContent),
                                                              freeMainSpace,
                                                              line.numItems,
                                                              mainGap);

        // Stretch resizes only auto-sized items; an explicit cross size wins,
        // and so does an auto cross margin - an item whose margin is going to
        // absorb the leftover cannot also be sized to fill it.
        for (int i = 0; i < line.numItems; ++i)
        {
            auto& r = lineItems[i];

            if (r.align == FlexItem::AlignSelf::stretch
                && r.crossSizeIsAuto
                && r.numAutoCrossMargins() == 0)
            {
                r.crossSize = applyMinMax (std::max (0.0f, line.crossSize - r.crossMarginStart - r.crossMarginEnd),
                                           r.minCrossSize,
                                           r.maxCrossSize);
            }
            else
            {
                r.crossSize = r.hypotheticalCrossSize;
            }

            // Auto cross margins share whatever room is left in the line.
            if (const int numAuto = r.numAutoCrossMargins(); numAuto > 0)
            {
                const float leftover = line.crossSize - r.crossSize - r.crossMarginStart - r.crossMarginEnd;

                if (leftover > 0.0f)
                {
                    const float share = leftover / static_cast<float> (numAuto);

                    if (r.crossMarginStartIsAuto)
                        r.crossMarginStart = share;

                    if (r.crossMarginEndIsAuto)
                        r.crossMarginEnd = share;
                }
            }
        }

        // The line's shared baseline, measured from each margin box's start so
        // that a leading cross margin shifts an item without being counted
        // twice when the position is derived from it below.
        float lineBaseline = 0.0f;

        for (int i = 0; i < line.numItems; ++i)
        {
            const auto& r = lineItems[i];

            if (r.align == FlexItem::AlignSelf::baseline)
                lineBaseline = std::max (lineBaseline, r.crossMarginStart + r.baselineOffset());
        }

        float mainPosition = justified.leading;

        for (int i = 0; i < line.numItems; ++i)
        {
            auto& r = lineItems[i];

            if (i > 0)
                mainPosition += justified.between;

            mainPosition += r.mainMarginStart;
            r.mainPosition = mainPosition;
            mainPosition += r.mainSize + r.mainMarginEnd;

            // An item with an auto cross margin has already been positioned by
            // that margin; align-items must not move it again.
            if (r.numAutoCrossMargins() > 0)
            {
                r.crossPosition = line.crossPosition + r.crossMarginStart;
            }
            else switch (r.align)
            {
                case FlexItem::AlignSelf::flexEnd:
                    r.crossPosition = line.crossPosition + line.crossSize - r.crossMarginEnd - r.crossSize;
                    break;

                case FlexItem::AlignSelf::center:
                    // The margin box is what gets centred, so asymmetric cross
                    // margins shift the border box off the line's midpoint.
                    r.crossPosition = line.crossPosition
                                    + r.crossMarginStart
                                    + (line.crossSize - r.crossMarginStart - r.crossMarginEnd - r.crossSize) / 2.0f;
                    break;

                case FlexItem::AlignSelf::baseline:
                    r.crossPosition = line.crossPosition + lineBaseline - r.baselineOffset();
                    break;

                case FlexItem::AlignSelf::autoAlign:
                case FlexItem::AlignSelf::flexStart:
                case FlexItem::AlignSelf::stretch:
                default:
                    r.crossPosition = line.crossPosition + r.crossMarginStart;
                    break;
            }

            if (r.item->associatedComponent == nullptr)
                continue;

            // Mirror within the container, not around the final rectangle, so
            // margins and gaps keep the meaning they were resolved with.
            const float mainPos = isReverseMain
                                    ? containerMainStart + containerMainSize - r.mainPosition - r.mainSize
                                    : containerMainStart + r.mainPosition;


            const float crossPos = isReverseCross
                                     ? containerCrossStart + containerCrossSize - r.crossPosition - r.crossSize
                                     : containerCrossStart + r.crossPosition;

            // Component::setBounds takes floats, so rounding here would only
            // lose precision and make adjacent items disagree about the edge
            // they share.
            const auto bounds = isRow
                                  ? Rectangle<float> (mainPos, crossPos, r.mainSize, r.crossSize)
                                  : Rectangle<float> (crossPos, mainPos, r.crossSize, r.mainSize);

            r.item->associatedComponent->setBounds (bounds);
        }
    }
}

void FlexBox::performLayout (Rectangle<int> targetArea)
{
    performLayout (targetArea.to<float>());
}

} // namespace yup
