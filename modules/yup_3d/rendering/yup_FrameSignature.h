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
/** FNV-1a over the values a frame depends on, for the change tracking of SceneRenderer and
    ProbeBaker. Floats are canonicalized first, so 0 and -0, and NaNs of any payload, hash the same.

    @internal
*/
class FrameSignature
{
public:
    void addBits (uint64 value) noexcept
    {
        addBytes (value, 8);
    }

    void addFloat (float value) noexcept
    {
        auto bits = std::bit_cast<uint32> (value);

        if ((bits & 0x7f800000u) == 0x7f800000u && (bits & 0x007fffffu) != 0)
            bits = 0x7fc00000u;
        else if (bits == 0x80000000u)
            bits = 0;

        addBytes (bits, 4);
    }

    void addFloats (Span<const float> values) noexcept
    {
        for (const auto value : values)
            addFloat (value);
    }

    void addPointer (const void* pointer) noexcept
    {
        addBits (static_cast<uint64> (reinterpret_cast<uintptr_t> (pointer)));
    }

    void addVector (const Vector3<float>& vector) noexcept
    {
        addFloat (vector.getX());
        addFloat (vector.getY());
        addFloat (vector.getZ());
    }

    void addMatrix (const Matrix4& matrix) noexcept
    {
        addFloats (Span<const float> (matrix.getData(), 16));
    }

    uint64 get() const noexcept { return hash; }

    /** The hash through the splitmix64 finalizer. Sums of plain FNV-1a hashes of similar items
        cancel out (two primitives of one mesh changing together leave the sum unchanged), sums of
        finalized ones don't.
    */
    uint64 getFinalized() const noexcept
    {
        auto z = hash;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
        return z ^ (z >> 31);
    }

private:
    void addBytes (uint64 value, int count) noexcept
    {
        for (int i = 0; i < count; ++i)
        {
            hash ^= (value >> (i * 8)) & 0xff;
            hash *= 1099511628211ull;
        }
    }

    uint64 hash = 14695981039346656037ull;
};

} // namespace yup
