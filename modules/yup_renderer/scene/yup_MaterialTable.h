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
/** The materials a frame draws, numbered for the G-buffer and packed for the GPU.

    The G-buffer stores a material id per texel, which the lighting pass uses to read the packed
    GpuMaterial back from getEntries(). Ids are assigned in order of first use within a frame,
    starting from 1: id 0 is always the default Material.

    The id is stored in a half float, exact up to maxMaterials. Materials beyond that alias the
    default material for the rest of the frame, and a debug message is logged once per frame.

    @internal
*/
class YUP_API MaterialTable
{
public:
    //==============================================================================
    /** The number of entries a frame can hold, the default included: exact integers in a half float. */
    static constexpr int maxMaterials = 2048;

    //==============================================================================
    /** Creates a table holding only the default material, at id 0. */
    MaterialTable();

    //==============================================================================
    /** Starts a frame: forgets the previous assignments, keeping the allocated memory. */
    void clear();

    /** Returns the id of a material in this frame's table, adding it on first use.

        Materials are told apart by address. Once the table holds maxMaterials entries, new
        materials get 0, the default material.
    */
    int getId (const Material& material);

    /** Returns the packed table, indexed by id. Entry 0 is always the default material. */
    Span<const GpuMaterial> getEntries() const noexcept { return entries; }

private:
    std::unordered_map<const Material*, int> ids;
    std::vector<GpuMaterial> entries;
    GpuMaterial defaultEntry;
    bool overflowLogged = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MaterialTable)
};

} // namespace yup
