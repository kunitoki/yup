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
ProbeAtlas::ProbeAtlas (int widthIn, int maxHeightIn)
    : width (widthIn)
    , maxHeight (maxHeightIn)
{
    jassert (width > 0 && maxHeight > 0);
}

ProbeAtlas::~ProbeAtlas() = default;

//==============================================================================
std::optional<Rectangle<int>> ProbeAtlas::allocate (int blockWidth, int blockHeight, int alignment)
{
    jassert (blockWidth > 0 && blockHeight > 0 && alignment > 0);
    if (blockWidth <= 0 || blockHeight <= 0 || alignment <= 0 || blockWidth > width)
        return std::nullopt;

    const auto isAligned = [alignment] (int value)
    {
        return value % alignment == 0;
    };

    const auto alignUp = [alignment] (int value)
    {
        return (value + alignment - 1) / alignment * alignment;
    };

    const auto reusable = std::find_if (released.begin(), released.end(), [&] (const Rectangle<int>& block)
    {
        return block.getWidth() == blockWidth && block.getHeight() == blockHeight && isAligned (block.getX()) && isAligned (block.getY());
    });

    if (reusable != released.end())
    {
        const auto block = *reusable;
        released.erase (reusable);
        return block;
    }

    for (auto& shelf : shelves)
    {
        const auto x = alignUp (shelf.cursorX);
        if (shelf.height < blockHeight || ! isAligned (shelf.y) || x + blockWidth > width)
            continue;

        shelf.cursorX = x + blockWidth;
        return Rectangle<int> (x, shelf.y, blockWidth, blockHeight);
    }

    const auto y = alignUp (shelves.empty() ? 0 : shelves.back().y + shelves.back().height);
    if (y + blockHeight > maxHeight)
        return std::nullopt;

    shelves.push_back ({ y, blockHeight, blockWidth });
    updateRequiredHeight();

    return Rectangle<int> (0, y, blockWidth, blockHeight);
}

void ProbeAtlas::release (const Rectangle<int>& block)
{
    jassert (block.getX() >= 0 && block.getRight() <= width && block.getY() >= 0 && block.getBottom() <= maxHeight);
    released.push_back (block);
}

void ProbeAtlas::clear()
{
    shelves.clear();
    released.clear();
    updateRequiredHeight();
}

//==============================================================================
void ProbeAtlas::updateRequiredHeight()
{
    const auto height = nextPowerOfTwo (shelves.empty() ? 0 : shelves.back().y + shelves.back().height);
    if (height == requiredHeight)
        return;

    requiredHeight = height;
    ++generation;
}

} // namespace yup
