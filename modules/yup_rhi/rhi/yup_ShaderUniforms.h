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
/** The bytes of one uniform block of a shader, set member by member.

    A shader reads its parameters from a uniform block, which reaches the GPU as
    raw bytes laid out the way GLSL lays the block out (std140). ShaderUniforms
    holds those bytes with the layout of the members, taken from the reflection of
    the block, so values are set by member name rather than at offsets computed by
    hand, and converted to the type the shader declares.

    @code
    // layout(set = 0, binding = 0) uniform Parameters
    // {
    //     float intensity;
    //     vec4 tint;
    // } params;
    ShaderUniforms uniforms (fragment->reflection.uniformBuffers.front());

    uniforms.set ("intensity", 0.5f);
    uniforms.set ("tint", { 1.0f, 0.5f, 0.25f, 1.0f });

    pass.setUniformBuffer (0, 0, uniforms.getData(), uniforms.getSizeInBytes());
    @endcode

    It is a plain value: it can be copied, kept from one draw to the next and
    compared. Two of them are equal when they hold the same members and the same
    bytes.

    A member can be a scalar, a vector of 2 to 4 components or a matrix of up to 4
    columns and rows, made of `float`, `int`, `uint` or `bool`, or a one
    dimensional array of those. A GLSL `bool` is stored as a 32 bit integer that
    is true when not zero, and the shader compiler reflects it as a `uint`.
    Matrices are column major, the GLSL default: a block declared `row_major` is
    not supported. The block keeps the room of the members of any other type
    (structures, arrays of arrays, `double`, and the 8, 16 and 64 bit types),
    zeroed, but they are not members that can be set.

    It knows nothing about what the values mean: colors, sizes or times are
    converted to numbers by the caller.

    @see GpuImageShader, GpuRenderPass::setUniformBuffer, ShaderReflection
*/
class YUP_API ShaderUniforms
{
public:
    //==============================================================================
    /** The largest uniform block there can be uniforms of, in bytes: 64 KiB, the most
        Direct3D binds as one constant buffer and WebGPU as one uniform buffer by default,
        so the most a block meant for every graphics API can take. */
    static constexpr size_t maxSizeInBytes = 64 * 1024;

    //==============================================================================
    /** Creates the uniforms of no block: empty, with no member. */
    ShaderUniforms() = default;

    /** Creates the zeroed uniforms of a reflected uniform block.

        A block larger than maxSizeInBytes gives empty uniforms, as a block of no size
        does: a reflection read from a file can declare any size.

        @param block  The block, one of ShaderReflection::uniformBuffers. Its members
                      and its size are read, not its name nor where it is bound.
    */
    explicit ShaderUniforms (const ShaderReflection::ResourceBinding& block);

    //==============================================================================
    /** Returns true if there are no bytes: the uniforms of no block. */
    bool isEmpty() const noexcept { return bytes.empty(); }

    /** Returns the number of bytes of the block, a multiple of 16: std140 gives a
        block the alignment of a `vec4`, and backends take no less than that. */
    size_t getSizeInBytes() const noexcept { return bytes.size(); }

    /** Returns the bytes of the block, to pass to GpuRenderPass::setUniformBuffer()
        with getSizeInBytes(). There is nothing to read from it when isEmpty(). */
    const uint8* getData() const noexcept { return bytes.data(); }

    //==============================================================================
    /** Returns true if the block has a member of this name that set() can write.

        @param name  The name of the member as the shader declares it, case sensitive.
    */
    bool hasMember (StringRef name) const noexcept;

    /** Returns the type of the scalars a member is made of.

        @param name  The name of the member.

        @returns ShaderReflection::BaseType::float32, int32, uint32 or boolean, or
                 unknown when hasMember() is false. A `bool` of a shader compiled
                 from GLSL is a uint32.
    */
    ShaderReflection::BaseType getMemberType (StringRef name) const noexcept;

    /** Returns the number of scalars a member holds, the most values set() writes
        into it: 1 for a `float`, 3 for a `vec3`, 16 for a `mat4`, 8 for a `vec2[4]`.

        @param name  The name of the member.

        @returns The number of scalars, or 0 when hasMember() is false.
    */
    int getNumComponents (StringRef name) const noexcept;

    //==============================================================================
    /** Sets the scalars of a member.

        The values fill the components of a vector in order, the columns of a matrix
        one after the other, and the elements of an array one after the other. Each
        is converted to the type of the member: rounded to the nearest integer for
        `int` and `uint` (and kept inside their range), true when not zero for `bool`.
        Values go through `float`, so integers above 16777216 are not exact.

        Fewer values than the member holds leave the rest of it as it is, and the
        values past its end are ignored.

        @param name    The name of the member.
        @param values  The values, as many as getNumComponents() to set all of it.

        @returns False, having written nothing, when hasMember() is false: a caller
                 can set a member a shader may or may not declare.
    */
    bool set (StringRef name, Span<const float> values);

    /** Sets the scalars of a member from an array or from a list in braces, as the
        Span overload does.

        @code
        uniforms.set ("resolution", { 640.0f, 480.0f });
        @endcode
    */
    template <size_t numValues>
    bool set (StringRef name, const float (&values)[numValues])
    {
        return set (name, Span<const float> (values, numValues));
    }

    /** Sets a member from a var, through the Span overload.

        @param name   The name of the member.
        @param value  A number or a boolean for the first scalar of the member, or an
                      array of numbers and booleans for as many of them.

        @returns False, having written nothing, when hasMember() is false or the var
                 holds something else (a string, an object, an array of those).
    */
    bool set (StringRef name, const var& value);

    //==============================================================================
    /** Returns true if both hold the same members and the same bytes. */
    bool operator== (const ShaderUniforms&) const = default;

private:
    struct Member
    {
        String name;
        ShaderReflection::BaseType type = ShaderReflection::BaseType::unknown;
        uint32 offset = 0;
        uint32 numElements = 1;
        uint32 elementStride = 0;
        uint32 numColumns = 1;
        uint32 columnStride = 0;
        uint32 numRows = 1;

        bool operator== (const Member&) const = default;
    };

    const Member* findMember (StringRef name) const noexcept;

    std::vector<Member> members;
    std::vector<uint8> bytes;
};

} // namespace yup
