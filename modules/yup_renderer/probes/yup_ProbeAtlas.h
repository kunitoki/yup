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
/** Packs the blocks of the probe atlases into a texture of fixed width that grows downwards.

    A shelf allocator in texels, top-left origin. It serves square tiles aligned to their size
    (the reflection atlas, so mip n of a tile is exactly the tile shifted right by n) and
    rectangular blocks (the irradiance and visibility atlases). A released block is kept and given
    back to the next request of the same size; its space is never merged with its neighbours.

    @code
    ProbeAtlas atlas (1024, 4096);

    if (auto tile = atlas.allocate (128, 128, 128))
        bakeInto (*tile);

    if (atlas.getGeneration() != textureGeneration)
        reallocateTextureAndRebake (atlas.getWidth(), atlas.getRequiredHeight());
    @endcode

    @internal
*/
class YUP_API ProbeAtlas
{
public:
    //==============================================================================
    /** An atlas @p width texels wide that may grow up to @p maxHeight texels. */
    ProbeAtlas (int width, int maxHeight);

    /** Destructor. */
    ~ProbeAtlas();

    //==============================================================================
    /** Reserves a block, its x and y multiples of @p alignment.

        Reuses a released block of the same size and alignment first. Otherwise the block goes on
        the first shelf tall enough whose top is aligned and which has room left, or on a new shelf
        opened at the aligned bottom of the last one.

        @returns The block in texels, or std::nullopt when the atlas is full, the block is wider
                 than the atlas, or a size or the alignment is not positive.
    */
    std::optional<Rectangle<int>> allocate (int blockWidth, int blockHeight, int alignment = 1);

    /** Returns a block to the atlas, for a later allocate() of the same size. */
    void release (const Rectangle<int>& block);

    /** Releases every block. */
    void clear();

    //==============================================================================
    /** The texture height the blocks need: the bottom of the lowest shelf, rounded up to a power of two (at least 1). */
    int getRequiredHeight() const noexcept { return requiredHeight; }

    /** The atlas width in texels. */
    int getWidth() const noexcept { return width; }

    /** Increases whenever getRequiredHeight() changed, so callers know to reallocate and rebake. */
    uint32 getGeneration() const noexcept { return generation; }

private:
    //==============================================================================
    struct Shelf
    {
        int y = 0;
        int height = 0;
        int cursorX = 0;
    };

    void updateRequiredHeight();

    int width = 0;
    int maxHeight = 0;
    int requiredHeight = 1;
    uint32 generation = 0;

    std::vector<Shelf> shelves;
    std::vector<Rectangle<int>> released;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProbeAtlas)
};

} // namespace yup
