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

namespace wgsl
{

//==============================================================================
/**
    Gives uniform, storage and workgroup memory the byte layout GLSL specifies.

    GLSL lays out uniform blocks with std140 and storage blocks with std430, while
    WGSL uses its own natural layout and forbids some types in host-shareable memory.
    This pass computes the GLSL offsets of every block member and makes WGSL match:

    - members get @size attributes wherever GLSL leaves padding WGSL wouldn't
    - std140 arrays of scalars or vec2, and std140 matrices with two rows, use
      16-byte wrapper structs so their stride matches
    - bool members are stored as u32
    - members and workgroup variables used with GLSL atomic functions become atomic<T>

    Accesses to such memory are rewritten accordingly: reads convert back to the GLSL
    type, writes convert to the stored type and atomics load and store explicitly.
    Runs after WgslTypeLegalizer and before WgslStatementLowering.
*/
class WgslHostLayout
{
public:
    /** Rewrites the program in place, throwing LoweringError for layouts WGSL can't reproduce. */
    static void apply (TranslationUnit& ast, WgslLoweringContext& context);
};

} // namespace wgsl
} // namespace yup
