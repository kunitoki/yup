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
    bytes. The members are described once and never change: the copies share
    that description, and each has bytes of its own, so a copy is cheap and each
    one can be used by another thread.

    Setting a member by name looks for it among the members. What fills the same
    uniforms again and again, as at every frame, finds each member once and sets
    it with the handle it got, with no search:

    @code
    const auto intensity = uniforms.findMember ("intensity");   // once

    uniforms.set (intensity, { 0.5f });                         // at every frame
    @endcode

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
    /** A member of the block that set() can write: its name, the type of its scalars
        and where its bytes are, as the reflection of the shader tells them.

        Each scalar takes 4 bytes. The one at the row `r` of the column `c` of the
        element `e` starts at `offset + e * elementStride + c * columnStride + r * 4`.

        @see getMembers
    */
    struct Member
    {
        /** The name the shader declares the member with. */
        String name;

        /** The type of its scalars: ShaderReflection::BaseType::float32, int32, uint32 or boolean. */
        ShaderReflection::BaseType type = ShaderReflection::BaseType::unknown;

        /** Where the member starts in the block, in bytes. */
        uint32 offset = 0;

        /** The number of elements of an array, 1 for a member that is not one. */
        uint32 numElements = 1;

        /** The bytes from an element of an array to the next one. */
        uint32 elementStride = 0;

        /** The number of columns of a matrix, 1 for a scalar and for a vector. */
        uint32 numColumns = 1;

        /** The bytes from a column of a matrix to the next one. */
        uint32 columnStride = 0;

        /** The number of scalars of a vector or of a column of a matrix, 1 for a scalar. */
        uint32 numRows = 1;

        /** Returns the number of scalars the member holds, the most values set() writes
            into it: its elements times its columns times its rows. */
        int getNumComponents() const noexcept { return static_cast<int> (numElements * numColumns * numRows); }

        /** Returns true if both describe the same member at the same place. */
        bool operator== (const Member&) const = default;
    };

    //==============================================================================
    /** Tells which member of a block to set, once findMember() found it by name.

        It is the index of the member in getMembers(), so it tells the same member in
        all the uniforms laid out the same way (see hasSameLayoutAs): the copies of the
        ones it was found in, and every uniforms a shader creates (see
        GpuImageShader::createUniforms, GpuImageShader::getUniformLayout). In uniforms
        laid out another way it tells another member, or none.

        @see findMember
    */
    class MemberHandle
    {
    public:
        /** Creates the handle of no member, as findMember() returns for a name the block does not have. */
        constexpr MemberHandle() noexcept = default;

        /** Returns true if the handle tells a member. Setting through one that does not writes nothing. */
        constexpr bool isValid() const noexcept { return index >= 0; }

        /** Returns the index of the member in getMembers(), or -1 when isValid() is false. */
        constexpr int getIndex() const noexcept { return index; }

    private:
        friend class ShaderUniforms;

        constexpr explicit MemberHandle (int memberIndex) noexcept
            : index (memberIndex)
        {
        }

        int index = -1;
    };

    //==============================================================================
    /** Creates the uniforms of no block: empty, with no member. */
    ShaderUniforms() = default;

    /** Creates the zeroed uniforms of a reflected uniform block.

        A block larger than maxSizeInBytes gives empty uniforms, as a block of no size
        does: a reflection read from a file can declare any size. So is a member left
        out when it reaches past the block, or is an array declaring no stride.

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

    /** Sets every byte back to zero, as the uniforms were created. The block keeps
        its size and its members.

        This is how uniforms kept from one frame to the next start being filled again,
        when the values that are not set have to be zero rather than what they were.
    */
    void clear() noexcept { std::fill (bytes.begin(), bytes.end(), uint8 (0)); }

    //==============================================================================
    /** Returns the members of the block that set() can write, in the order the
        reflection of the block lists them: what a tool lists, or what a test reads the
        bytes of a member with.

        The members of a type that can't be set are left out (see the class
        description). The memory is the one the copies of the uniforms share: it stays
        valid while any of them is alive.
    */
    Span<const Member> getMembers() const noexcept;

    /** Finds a member by its name, to set it again and again with no search.

        @param name  The name of the member as the shader declares it, case sensitive.

        @returns The handle of the member, for the set() overloads taking one, or a
                 handle that is not valid when hasMember() is false.
    */
    MemberHandle findMember (StringRef name) const noexcept;

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
    bool set (StringRef name, Span<const float> values) { return set (findMember (name), values); }

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
    bool set (StringRef name, const var& value) { return set (findMember (name), value); }

    //==============================================================================
    /** Sets the scalars of a member found by findMember(), as the overload taking its
        name does, with no search.

        @param member  The handle of the member, found in these uniforms or in any laid
                       out the same way.
        @param values  The values, as many as the member holds to set all of it.

        @returns False, having written nothing, when the handle is not valid or is not
                 one of these uniforms.
    */
    bool set (MemberHandle member, Span<const float> values);

    /** Sets the scalars of a member found by findMember() from an array or from a list
        in braces, as the Span overload does.

        @code
        uniforms.set (resolution, { 640.0f, 480.0f });
        @endcode
    */
    template <size_t numValues>
    bool set (MemberHandle member, const float (&values)[numValues])
    {
        return set (member, Span<const float> (values, numValues));
    }

    /** Sets a member found by findMember() from a var, as the overload taking its name
        does, with no search.

        @param member  The handle of the member.
        @param value   A number or a boolean for the first scalar of the member, or an
                       array of numbers and booleans for as many of them.

        @returns False, having written nothing, when the handle is not valid or the var
                 holds something else (a string, an object, an array of those).
    */
    bool set (MemberHandle member, const var& value);

    //==============================================================================
    /** Returns true if both are the uniforms of the same block: the same members, laid
        out the same way in as many bytes, whatever the bytes hold.

        GpuImageShader::draw() takes the uniforms of its shader only. Those of another
        block of the same size would fill its members with values meant for others.

        Uniforms that are copies of each other, as all the ones a shader creates are,
        are told at once. The members are only compared for the ones made apart, each
        from the reflection of a block.

        @param other  The uniforms to compare the layout with.
    */
    bool hasSameLayoutAs (const ShaderUniforms& other) const noexcept;

    /** Returns true if both hold the same members and the same bytes. */
    bool operator== (const ShaderUniforms& other) const noexcept;

private:
    const Member* getMember (MemberHandle member) const noexcept;
    void write (const Member& member, size_t index, float value) noexcept;

    // Never modified once made, so the copies share it, on any thread
    std::shared_ptr<const std::vector<Member>> layout;
    std::vector<uint8> bytes;
};

} // namespace yup
