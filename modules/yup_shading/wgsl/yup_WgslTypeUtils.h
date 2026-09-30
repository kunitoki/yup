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
/** GLSL built-in type names and the TypeKind each one denotes. */
inline const std::unordered_map<std::string, TypeKind>& glslTypeNames()
{
    static const std::unordered_map<std::string, TypeKind> names = {
        { "void", TypeKind::voidType },
        { "float", TypeKind::floatType },
        { "int", TypeKind::intType },
        { "uint", TypeKind::uintType },
        { "bool", TypeKind::boolType },
        { "double", TypeKind::doubleType },
        { "vec2", TypeKind::vec2 },
        { "vec3", TypeKind::vec3 },
        { "vec4", TypeKind::vec4 },
        { "ivec2", TypeKind::ivec2 },
        { "ivec3", TypeKind::ivec3 },
        { "ivec4", TypeKind::ivec4 },
        { "uvec2", TypeKind::uvec2 },
        { "uvec3", TypeKind::uvec3 },
        { "uvec4", TypeKind::uvec4 },
        { "bvec2", TypeKind::bvec2 },
        { "bvec3", TypeKind::bvec3 },
        { "bvec4", TypeKind::bvec4 },
        { "dvec2", TypeKind::dvec2 },
        { "dvec3", TypeKind::dvec3 },
        { "dvec4", TypeKind::dvec4 },
        { "mat2", TypeKind::mat2 },
        { "mat3", TypeKind::mat3 },
        { "mat4", TypeKind::mat4 },
        { "mat2x2", TypeKind::mat2x2 },
        { "mat2x3", TypeKind::mat2x3 },
        { "mat2x4", TypeKind::mat2x4 },
        { "mat3x2", TypeKind::mat3x2 },
        { "mat3x3", TypeKind::mat3x3 },
        { "mat3x4", TypeKind::mat3x4 },
        { "mat4x2", TypeKind::mat4x2 },
        { "mat4x3", TypeKind::mat4x3 },
        { "mat4x4", TypeKind::mat4x4 },
        { "dmat2", TypeKind::dmat2 },
        { "dmat3", TypeKind::dmat3 },
        { "dmat4", TypeKind::dmat4 },
        { "dmat2x2", TypeKind::dmat2x2 },
        { "dmat2x3", TypeKind::dmat2x3 },
        { "dmat2x4", TypeKind::dmat2x4 },
        { "dmat3x2", TypeKind::dmat3x2 },
        { "dmat3x3", TypeKind::dmat3x3 },
        { "dmat3x4", TypeKind::dmat3x4 },
        { "dmat4x2", TypeKind::dmat4x2 },
        { "dmat4x3", TypeKind::dmat4x3 },
        { "dmat4x4", TypeKind::dmat4x4 },
        { "sampler1D", TypeKind::sampler1D },
        { "sampler2D", TypeKind::sampler2D },
        { "sampler3D", TypeKind::sampler3D },
        { "samplerCube", TypeKind::samplerCube },
        { "sampler1DShadow", TypeKind::sampler1DShadow },
        { "sampler2DShadow", TypeKind::sampler2DShadow },
        { "samplerCubeShadow", TypeKind::samplerCubeShadow },
        { "samplerCubeArray", TypeKind::samplerCubeArray },
        { "samplerCubeArrayShadow", TypeKind::samplerCubeArrayShadow },
        { "sampler1DArray", TypeKind::sampler1DArray },
        { "sampler2DArray", TypeKind::sampler2DArray },
        { "sampler1DArrayShadow", TypeKind::sampler1DArrayShadow },
        { "sampler2DArrayShadow", TypeKind::sampler2DArrayShadow },
        { "sampler2DRect", TypeKind::sampler2DRect },
        { "sampler2DRectShadow", TypeKind::sampler2DRectShadow },
        { "samplerBuffer", TypeKind::samplerBuffer },
        { "sampler2DMS", TypeKind::sampler2DMS },
        { "sampler2DMSArray", TypeKind::sampler2DMSArray },
        { "isampler1D", TypeKind::isampler1D },
        { "isampler2D", TypeKind::isampler2D },
        { "isampler3D", TypeKind::isampler3D },
        { "isamplerCube", TypeKind::isamplerCube },
        { "isamplerCubeArray", TypeKind::isamplerCubeArray },
        { "isampler1DArray", TypeKind::isampler1DArray },
        { "isampler2DArray", TypeKind::isampler2DArray },
        { "isampler2DRect", TypeKind::isampler2DRect },
        { "isamplerBuffer", TypeKind::isamplerBuffer },
        { "isampler2DMS", TypeKind::isampler2DMS },
        { "isampler2DMSArray", TypeKind::isampler2DMSArray },
        { "usampler1D", TypeKind::usampler1D },
        { "usampler2D", TypeKind::usampler2D },
        { "usampler3D", TypeKind::usampler3D },
        { "usamplerCube", TypeKind::usamplerCube },
        { "usamplerCubeArray", TypeKind::usamplerCubeArray },
        { "usampler1DArray", TypeKind::usampler1DArray },
        { "usampler2DArray", TypeKind::usampler2DArray },
        { "usampler2DRect", TypeKind::usampler2DRect },
        { "usamplerBuffer", TypeKind::usamplerBuffer },
        { "usampler2DMS", TypeKind::usampler2DMS },
        { "usampler2DMSArray", TypeKind::usampler2DMSArray },
        { "image1D", TypeKind::image1D },
        { "image2D", TypeKind::image2D },
        { "image3D", TypeKind::image3D },
        { "imageCube", TypeKind::imageCube },
        { "imageCubeArray", TypeKind::imageCubeArray },
        { "image1DArray", TypeKind::image1DArray },
        { "image2DArray", TypeKind::image2DArray },
        { "image2DRect", TypeKind::image2DRect },
        { "imageBuffer", TypeKind::imageBuffer },
        { "image2DMS", TypeKind::image2DMS },
        { "image2DMSArray", TypeKind::image2DMSArray },
        { "iimage1D", TypeKind::iimage1D },
        { "iimage2D", TypeKind::iimage2D },
        { "iimage3D", TypeKind::iimage3D },
        { "iimageCube", TypeKind::iimageCube },
        { "iimageCubeArray", TypeKind::iimageCubeArray },
        { "iimage1DArray", TypeKind::iimage1DArray },
        { "iimage2DArray", TypeKind::iimage2DArray },
        { "iimage2DRect", TypeKind::iimage2DRect },
        { "iimageBuffer", TypeKind::iimageBuffer },
        { "iimage2DMS", TypeKind::iimage2DMS },
        { "iimage2DMSArray", TypeKind::iimage2DMSArray },
        { "uimage1D", TypeKind::uimage1D },
        { "uimage2D", TypeKind::uimage2D },
        { "uimage3D", TypeKind::uimage3D },
        { "uimageCube", TypeKind::uimageCube },
        { "uimageCubeArray", TypeKind::uimageCubeArray },
        { "uimage1DArray", TypeKind::uimage1DArray },
        { "uimage2DArray", TypeKind::uimage2DArray },
        { "uimage2DRect", TypeKind::uimage2DRect },
        { "uimageBuffer", TypeKind::uimageBuffer },
        { "uimage2DMS", TypeKind::uimage2DMS },
        { "uimage2DMSArray", TypeKind::uimage2DMSArray },
        { "atomic_uint", TypeKind::atomicUint },
        { "texture1D", TypeKind::texture1D },
        { "texture2D", TypeKind::texture2D },
        { "texture3D", TypeKind::texture3D },
        { "textureCube", TypeKind::textureCube },
        { "textureCubeArray", TypeKind::textureCubeArray },
        { "texture1DArray", TypeKind::texture1DArray },
        { "texture2DArray", TypeKind::texture2DArray },
        { "texture2DRect", TypeKind::texture2DRect },
        { "textureBuffer", TypeKind::textureBuffer },
        { "texture2DMS", TypeKind::texture2DMS },
        { "texture2DMSArray", TypeKind::texture2DMSArray },
        { "itexture1D", TypeKind::itexture1D },
        { "itexture2D", TypeKind::itexture2D },
        { "itexture3D", TypeKind::itexture3D },
        { "itextureCube", TypeKind::itextureCube },
        { "itextureCubeArray", TypeKind::itextureCubeArray },
        { "itexture1DArray", TypeKind::itexture1DArray },
        { "itexture2DArray", TypeKind::itexture2DArray },
        { "itexture2DRect", TypeKind::itexture2DRect },
        { "itextureBuffer", TypeKind::itextureBuffer },
        { "itexture2DMS", TypeKind::itexture2DMS },
        { "itexture2DMSArray", TypeKind::itexture2DMSArray },
        { "utexture1D", TypeKind::utexture1D },
        { "utexture2D", TypeKind::utexture2D },
        { "utexture3D", TypeKind::utexture3D },
        { "utextureCube", TypeKind::utextureCube },
        { "utextureCubeArray", TypeKind::utextureCubeArray },
        { "utexture1DArray", TypeKind::utexture1DArray },
        { "utexture2DArray", TypeKind::utexture2DArray },
        { "utexture2DRect", TypeKind::utexture2DRect },
        { "utextureBuffer", TypeKind::utextureBuffer },
        { "utexture2DMS", TypeKind::utexture2DMS },
        { "utexture2DMSArray", TypeKind::utexture2DMSArray },
        { "sampler", TypeKind::samplerType },
        { "samplerShadow", TypeKind::samplerShadow },
        { "subpassInput", TypeKind::subpassInput },
        { "isubpassInput", TypeKind::subpassInput },
        { "usubpassInput", TypeKind::subpassInput },
        { "subpassInputMS", TypeKind::subpassInputMS },
        { "isubpassInputMS", TypeKind::subpassInputMS },
        { "usubpassInputMS", TypeKind::subpassInputMS }
    };

    return names;
}

/** GLSL spelling of a built-in type, for diagnostics. */
inline std::string glslTypeName (TypeKind kind)
{
    for (const auto& [name, value] : glslTypeNames())
    {
        if (value == kind && name.rfind ("isubpass", 0) != 0 && name.rfind ("usubpass", 0) != 0)
            return name;
    }

    return "type";
}

//==============================================================================
/** Scalar component type of a scalar, vector or float matrix, or voidType for anything else. */
inline TypeKind scalarKindOf (TypeKind kind)
{
    switch (kind)
    {
        case TypeKind::floatType:
        case TypeKind::vec2:
        case TypeKind::vec3:
        case TypeKind::vec4:
        case TypeKind::mat2:
        case TypeKind::mat3:
        case TypeKind::mat4:
        case TypeKind::mat2x2:
        case TypeKind::mat2x3:
        case TypeKind::mat2x4:
        case TypeKind::mat3x2:
        case TypeKind::mat3x3:
        case TypeKind::mat3x4:
        case TypeKind::mat4x2:
        case TypeKind::mat4x3:
        case TypeKind::mat4x4:
            return TypeKind::floatType;

        case TypeKind::intType:
        case TypeKind::ivec2:
        case TypeKind::ivec3:
        case TypeKind::ivec4:
            return TypeKind::intType;

        case TypeKind::uintType:
        case TypeKind::uvec2:
        case TypeKind::uvec3:
        case TypeKind::uvec4:
            return TypeKind::uintType;

        case TypeKind::boolType:
        case TypeKind::bvec2:
        case TypeKind::bvec3:
        case TypeKind::bvec4:
            return TypeKind::boolType;

        default:
            return TypeKind::voidType;
    }
}

/** Returns 1 for scalars, 2 to 4 for vectors and 0 for anything else. */
inline int componentCount (TypeKind kind)
{
    switch (kind)
    {
        case TypeKind::floatType:
        case TypeKind::intType:
        case TypeKind::uintType:
        case TypeKind::boolType:
            return 1;

        case TypeKind::vec2:
        case TypeKind::ivec2:
        case TypeKind::uvec2:
        case TypeKind::bvec2:
            return 2;

        case TypeKind::vec3:
        case TypeKind::ivec3:
        case TypeKind::uvec3:
        case TypeKind::bvec3:
            return 3;

        case TypeKind::vec4:
        case TypeKind::ivec4:
        case TypeKind::uvec4:
        case TypeKind::bvec4:
            return 4;

        default:
            return 0;
    }
}

/** Scalar (count 1) or vector type with the given component type, or voidType. */
inline TypeKind vectorKind (TypeKind scalar, int count)
{
    static constexpr TypeKind floats[] = { TypeKind::floatType, TypeKind::vec2, TypeKind::vec3, TypeKind::vec4 };
    static constexpr TypeKind ints[] = { TypeKind::intType, TypeKind::ivec2, TypeKind::ivec3, TypeKind::ivec4 };
    static constexpr TypeKind uints[] = { TypeKind::uintType, TypeKind::uvec2, TypeKind::uvec3, TypeKind::uvec4 };
    static constexpr TypeKind bools[] = { TypeKind::boolType, TypeKind::bvec2, TypeKind::bvec3, TypeKind::bvec4 };

    if (count < 1 || count > 4)
        return TypeKind::voidType;

    switch (scalar)
    {
        case TypeKind::floatType:
            return floats[count - 1];
        case TypeKind::intType:
            return ints[count - 1];
        case TypeKind::uintType:
            return uints[count - 1];
        case TypeKind::boolType:
            return bools[count - 1];
        default:
            return TypeKind::voidType;
    }
}

/** Returns { columns, rows } of a float matrix (GLSL matCxR), or { 0, 0 }. */
inline std::pair<int, int> matrixShape (TypeKind kind)
{
    switch (kind)
    {
        case TypeKind::mat2:
        case TypeKind::mat2x2:
            return { 2, 2 };
        case TypeKind::mat2x3:
            return { 2, 3 };
        case TypeKind::mat2x4:
            return { 2, 4 };
        case TypeKind::mat3x2:
            return { 3, 2 };
        case TypeKind::mat3:
        case TypeKind::mat3x3:
            return { 3, 3 };
        case TypeKind::mat3x4:
            return { 3, 4 };
        case TypeKind::mat4x2:
            return { 4, 2 };
        case TypeKind::mat4x3:
            return { 4, 3 };
        case TypeKind::mat4:
        case TypeKind::mat4x4:
            return { 4, 4 };
        default:
            return { 0, 0 };
    }
}

inline bool isMatrixType (TypeKind kind)
{
    return matrixShape (kind).first > 0;
}

inline TypeKind matrixKind (int columns, int rows)
{
    static constexpr TypeKind kinds[3][3] = {
        { TypeKind::mat2x2, TypeKind::mat2x3, TypeKind::mat2x4 },
        { TypeKind::mat3x2, TypeKind::mat3x3, TypeKind::mat3x4 },
        { TypeKind::mat4x2, TypeKind::mat4x3, TypeKind::mat4x4 }
    };

    if (columns < 2 || columns > 4 || rows < 2 || rows > 4)
        return TypeKind::voidType;

    return kinds[columns - 2][rows - 2];
}

inline bool isDoubleType (TypeKind kind)
{
    switch (kind)
    {
        case TypeKind::doubleType:
        case TypeKind::dvec2:
        case TypeKind::dvec3:
        case TypeKind::dvec4:
        case TypeKind::dmat2:
        case TypeKind::dmat3:
        case TypeKind::dmat4:
        case TypeKind::dmat2x2:
        case TypeKind::dmat2x3:
        case TypeKind::dmat2x4:
        case TypeKind::dmat3x2:
        case TypeKind::dmat3x3:
        case TypeKind::dmat3x4:
        case TypeKind::dmat4x2:
        case TypeKind::dmat4x3:
        case TypeKind::dmat4x4:
            return true;
        default:
            return false;
    }
}

//==============================================================================
/** Shape of a texture-like type: combined sampler, separate texture or storage image. */
struct TextureShape
{
    enum class Dim
    {
        none,
        d1,
        d2,
        d3,
        cube,
        rect,
        buffer
    };

    Dim dim = Dim::none;
    bool arrayed = false;
    bool multisampled = false;
    bool shadow = false;
    TypeKind sampledScalar = TypeKind::voidType; // float, int or uint component type of texel values
};

inline TextureShape textureShape (TypeKind kind)
{
    using D = TextureShape::Dim;
    const auto f = TypeKind::floatType;
    const auto i = TypeKind::intType;
    const auto u = TypeKind::uintType;

    const auto shape = [] (D d, bool arrayed, bool ms, bool shadow, TypeKind scalar)
    {
        TextureShape s;
        s.dim = d;
        s.arrayed = arrayed;
        s.multisampled = ms;
        s.shadow = shadow;
        s.sampledScalar = scalar;
        return s;
    };

    switch (kind)
    {
        case TypeKind::sampler1D:
        case TypeKind::texture1D:
        case TypeKind::image1D:
            return shape (D::d1, false, false, false, f);
        case TypeKind::sampler2D:
        case TypeKind::texture2D:
        case TypeKind::image2D:
            return shape (D::d2, false, false, false, f);
        case TypeKind::sampler3D:
        case TypeKind::texture3D:
        case TypeKind::image3D:
            return shape (D::d3, false, false, false, f);
        case TypeKind::samplerCube:
        case TypeKind::textureCube:
        case TypeKind::imageCube:
            return shape (D::cube, false, false, false, f);
        case TypeKind::samplerCubeArray:
        case TypeKind::textureCubeArray:
        case TypeKind::imageCubeArray:
            return shape (D::cube, true, false, false, f);
        case TypeKind::sampler1DArray:
        case TypeKind::texture1DArray:
        case TypeKind::image1DArray:
            return shape (D::d1, true, false, false, f);
        case TypeKind::sampler2DArray:
        case TypeKind::texture2DArray:
        case TypeKind::image2DArray:
            return shape (D::d2, true, false, false, f);
        case TypeKind::sampler2DRect:
        case TypeKind::texture2DRect:
        case TypeKind::image2DRect:
            return shape (D::rect, false, false, false, f);
        case TypeKind::samplerBuffer:
        case TypeKind::textureBuffer:
        case TypeKind::imageBuffer:
            return shape (D::buffer, false, false, false, f);
        case TypeKind::sampler2DMS:
        case TypeKind::texture2DMS:
        case TypeKind::image2DMS:
            return shape (D::d2, false, true, false, f);
        case TypeKind::sampler2DMSArray:
        case TypeKind::texture2DMSArray:
        case TypeKind::image2DMSArray:
            return shape (D::d2, true, true, false, f);

        case TypeKind::sampler1DShadow:
            return shape (D::d1, false, false, true, f);
        case TypeKind::sampler2DShadow:
            return shape (D::d2, false, false, true, f);
        case TypeKind::samplerCubeShadow:
            return shape (D::cube, false, false, true, f);
        case TypeKind::samplerCubeArrayShadow:
            return shape (D::cube, true, false, true, f);
        case TypeKind::sampler1DArrayShadow:
            return shape (D::d1, true, false, true, f);
        case TypeKind::sampler2DArrayShadow:
            return shape (D::d2, true, false, true, f);
        case TypeKind::sampler2DRectShadow:
            return shape (D::rect, false, false, true, f);

        case TypeKind::isampler1D:
        case TypeKind::itexture1D:
        case TypeKind::iimage1D:
            return shape (D::d1, false, false, false, i);
        case TypeKind::isampler2D:
        case TypeKind::itexture2D:
        case TypeKind::iimage2D:
            return shape (D::d2, false, false, false, i);
        case TypeKind::isampler3D:
        case TypeKind::itexture3D:
        case TypeKind::iimage3D:
            return shape (D::d3, false, false, false, i);
        case TypeKind::isamplerCube:
        case TypeKind::itextureCube:
        case TypeKind::iimageCube:
            return shape (D::cube, false, false, false, i);
        case TypeKind::isamplerCubeArray:
        case TypeKind::itextureCubeArray:
        case TypeKind::iimageCubeArray:
            return shape (D::cube, true, false, false, i);
        case TypeKind::isampler1DArray:
        case TypeKind::itexture1DArray:
        case TypeKind::iimage1DArray:
            return shape (D::d1, true, false, false, i);
        case TypeKind::isampler2DArray:
        case TypeKind::itexture2DArray:
        case TypeKind::iimage2DArray:
            return shape (D::d2, true, false, false, i);
        case TypeKind::isampler2DRect:
        case TypeKind::itexture2DRect:
        case TypeKind::iimage2DRect:
            return shape (D::rect, false, false, false, i);
        case TypeKind::isamplerBuffer:
        case TypeKind::itextureBuffer:
        case TypeKind::iimageBuffer:
            return shape (D::buffer, false, false, false, i);
        case TypeKind::isampler2DMS:
        case TypeKind::itexture2DMS:
        case TypeKind::iimage2DMS:
            return shape (D::d2, false, true, false, i);
        case TypeKind::isampler2DMSArray:
        case TypeKind::itexture2DMSArray:
        case TypeKind::iimage2DMSArray:
            return shape (D::d2, true, true, false, i);

        case TypeKind::usampler1D:
        case TypeKind::utexture1D:
        case TypeKind::uimage1D:
            return shape (D::d1, false, false, false, u);
        case TypeKind::usampler2D:
        case TypeKind::utexture2D:
        case TypeKind::uimage2D:
            return shape (D::d2, false, false, false, u);
        case TypeKind::usampler3D:
        case TypeKind::utexture3D:
        case TypeKind::uimage3D:
            return shape (D::d3, false, false, false, u);
        case TypeKind::usamplerCube:
        case TypeKind::utextureCube:
        case TypeKind::uimageCube:
            return shape (D::cube, false, false, false, u);
        case TypeKind::usamplerCubeArray:
        case TypeKind::utextureCubeArray:
        case TypeKind::uimageCubeArray:
            return shape (D::cube, true, false, false, u);
        case TypeKind::usampler1DArray:
        case TypeKind::utexture1DArray:
        case TypeKind::uimage1DArray:
            return shape (D::d1, true, false, false, u);
        case TypeKind::usampler2DArray:
        case TypeKind::utexture2DArray:
        case TypeKind::uimage2DArray:
            return shape (D::d2, true, false, false, u);
        case TypeKind::usampler2DRect:
        case TypeKind::utexture2DRect:
        case TypeKind::uimage2DRect:
            return shape (D::rect, false, false, false, u);
        case TypeKind::usamplerBuffer:
        case TypeKind::utextureBuffer:
        case TypeKind::uimageBuffer:
            return shape (D::buffer, false, false, false, u);
        case TypeKind::usampler2DMS:
        case TypeKind::utexture2DMS:
        case TypeKind::uimage2DMS:
            return shape (D::d2, false, true, false, u);
        case TypeKind::usampler2DMSArray:
        case TypeKind::utexture2DMSArray:
        case TypeKind::uimage2DMSArray:
            return shape (D::d2, true, true, false, u);

        default:
            return {};
    }
}

/** Combined image samplers: sampler2D, isampler3D, sampler2DShadow, ... */
inline bool isSamplerType (TypeKind kind)
{
    return kind >= TypeKind::sampler1D && kind <= TypeKind::usampler2DMSArray;
}

inline bool isImageType (TypeKind kind)
{
    return kind >= TypeKind::image1D && kind <= TypeKind::uimage2DMSArray;
}

/** Vulkan GLSL separate textures: texture2D, itexture2D, ... */
inline bool isSeparateTextureType (TypeKind kind)
{
    return (kind >= TypeKind::texture1D && kind <= TypeKind::utexture2DMSArray);
}

/** Vulkan GLSL separate samplers: sampler and samplerShadow. */
inline bool isSeparateSamplerType (TypeKind kind)
{
    return kind == TypeKind::samplerType || kind == TypeKind::samplerShadow;
}

/** Types that can only live in handle (resource) variables. */
inline bool isOpaqueType (TypeKind kind)
{
    return isSamplerType (kind) || isImageType (kind) || isSeparateTextureType (kind) || isSeparateSamplerType (kind)
        || kind == TypeKind::atomicUint || kind == TypeKind::subpassInput || kind == TypeKind::subpassInputMS;
}

/** Maps a combined sampler type to the equivalent separate texture type (sampler2DShadow maps to texture2D). */
inline TypeKind separateTextureKind (TypeKind combined)
{
    static const std::pair<TypeKind, TypeKind> table[] = {
        { TypeKind::sampler1D, TypeKind::texture1D },
        { TypeKind::sampler2D, TypeKind::texture2D },
        { TypeKind::sampler3D, TypeKind::texture3D },
        { TypeKind::samplerCube, TypeKind::textureCube },
        { TypeKind::samplerCubeArray, TypeKind::textureCubeArray },
        { TypeKind::sampler1DArray, TypeKind::texture1DArray },
        { TypeKind::sampler2DArray, TypeKind::texture2DArray },
        { TypeKind::sampler2DRect, TypeKind::texture2DRect },
        { TypeKind::samplerBuffer, TypeKind::textureBuffer },
        { TypeKind::sampler2DMS, TypeKind::texture2DMS },
        { TypeKind::sampler2DMSArray, TypeKind::texture2DMSArray },
        { TypeKind::sampler1DShadow, TypeKind::texture1D },
        { TypeKind::sampler2DShadow, TypeKind::texture2D },
        { TypeKind::samplerCubeShadow, TypeKind::textureCube },
        { TypeKind::samplerCubeArrayShadow, TypeKind::textureCubeArray },
        { TypeKind::sampler1DArrayShadow, TypeKind::texture1DArray },
        { TypeKind::sampler2DArrayShadow, TypeKind::texture2DArray },
        { TypeKind::sampler2DRectShadow, TypeKind::texture2DRect },
        { TypeKind::isampler1D, TypeKind::itexture1D },
        { TypeKind::isampler2D, TypeKind::itexture2D },
        { TypeKind::isampler3D, TypeKind::itexture3D },
        { TypeKind::isamplerCube, TypeKind::itextureCube },
        { TypeKind::isamplerCubeArray, TypeKind::itextureCubeArray },
        { TypeKind::isampler1DArray, TypeKind::itexture1DArray },
        { TypeKind::isampler2DArray, TypeKind::itexture2DArray },
        { TypeKind::isampler2DRect, TypeKind::itexture2DRect },
        { TypeKind::isamplerBuffer, TypeKind::itextureBuffer },
        { TypeKind::isampler2DMS, TypeKind::itexture2DMS },
        { TypeKind::isampler2DMSArray, TypeKind::itexture2DMSArray },
        { TypeKind::usampler1D, TypeKind::utexture1D },
        { TypeKind::usampler2D, TypeKind::utexture2D },
        { TypeKind::usampler3D, TypeKind::utexture3D },
        { TypeKind::usamplerCube, TypeKind::utextureCube },
        { TypeKind::usamplerCubeArray, TypeKind::utextureCubeArray },
        { TypeKind::usampler1DArray, TypeKind::utexture1DArray },
        { TypeKind::usampler2DArray, TypeKind::utexture2DArray },
        { TypeKind::usampler2DRect, TypeKind::utexture2DRect },
        { TypeKind::usamplerBuffer, TypeKind::utextureBuffer },
        { TypeKind::usampler2DMS, TypeKind::utexture2DMS },
        { TypeKind::usampler2DMSArray, TypeKind::utexture2DMSArray }
    };

    for (const auto& [from, to] : table)
    {
        if (from == combined)
            return to;
    }

    return combined;
}

//==============================================================================
inline bool sameType (const TypeSpecifier& a, const TypeSpecifier& b);

inline bool sameArraySize (const ArraySpecifier& a, const ArraySpecifier& b)
{
    if (a.isUnsized != b.isUnsized || (a.sizeExpr == nullptr) != (b.sizeExpr == nullptr))
        return false;

    if (a.sizeExpr == nullptr)
        return true;

    if (a.sizeExpr->is<ExprIntConst>() && b.sizeExpr->is<ExprIntConst>())
        return a.sizeExpr->as<ExprIntConst>().value == b.sizeExpr->as<ExprIntConst>().value;

    if (a.sizeExpr->is<ExprUIntConst>() && b.sizeExpr->is<ExprUIntConst>())
        return a.sizeExpr->as<ExprUIntConst>().value == b.sizeExpr->as<ExprUIntConst>().value;

    if (a.sizeExpr->is<ExprVariable>() && b.sizeExpr->is<ExprVariable>())
        return a.sizeExpr->as<ExprVariable>().name == b.sizeExpr->as<ExprVariable>().name;

    return false;
}

inline bool sameType (const TypeSpecifier& a, const TypeSpecifier& b)
{
    if (a.kind != b.kind || a.arraySpecifiers.size() != b.arraySpecifiers.size())
        return false;

    if (a.kind == TypeKind::namedStruct && a.structName != b.structName)
        return false;

    for (size_t i = 0; i < a.arraySpecifiers.size(); ++i)
    {
        if (! sameArraySize (a.arraySpecifiers[i], b.arraySpecifiers[i]))
            return false;
    }

    return true;
}

//==============================================================================
inline const char* wgslScalarName (TypeKind scalar)
{
    switch (scalar)
    {
        case TypeKind::floatType:
            return "f32";
        case TypeKind::intType:
            return "i32";
        case TypeKind::uintType:
            return "u32";
        case TypeKind::boolType:
            return "bool";
        default:
            return nullptr;
    }
}

/** WGSL name of a sampled texture type (combined sampler or separate texture), or empty if WGSL has none. */
inline std::string wgslTextureTypeName (TypeKind kind, bool depth)
{
    using D = TextureShape::Dim;
    const auto shape = textureShape (kind);
    depth = depth || shape.shadow;

    if (depth)
    {
        if (shape.multisampled)
            return shape.arrayed ? "" : "texture_depth_multisampled_2d";

        switch (shape.dim)
        {
            case D::d2:
                return shape.arrayed ? "texture_depth_2d_array" : "texture_depth_2d";
            case D::cube:
                return shape.arrayed ? "texture_depth_cube_array" : "texture_depth_cube";
            default:
                return "";
        }
    }

    const std::string scalar = std::string ("<") + wgslScalarName (shape.sampledScalar) + ">";

    if (shape.multisampled)
        return shape.arrayed ? "" : "texture_multisampled_2d" + scalar;

    switch (shape.dim)
    {
        case D::d1:
            return shape.arrayed ? "" : "texture_1d" + scalar;
        case D::d2:
            return (shape.arrayed ? "texture_2d_array" : "texture_2d") + scalar;
        case D::d3:
            return shape.arrayed ? "" : "texture_3d" + scalar;
        case D::cube:
            return (shape.arrayed ? "texture_cube_array" : "texture_cube") + scalar;
        default:
            return "";
    }
}

/** WGSL name of a non-aggregate GLSL type, or empty if WGSL has no equivalent. Storage images are named elsewhere. */
inline std::string wgslTypeName (TypeKind kind)
{
    if (const auto* scalar = wgslScalarName (kind))
        return scalar;

    if (const auto count = componentCount (kind); count > 1)
        return "vec" + std::to_string (count) + "<" + wgslScalarName (scalarKindOf (kind)) + ">";

    if (const auto [columns, rows] = matrixShape (kind); columns > 0)
        return "mat" + std::to_string (columns) + "x" + std::to_string (rows) + "<f32>";

    if (kind == TypeKind::atomicI32)
        return "atomic<i32>";

    if (kind == TypeKind::atomicU32)
        return "atomic<u32>";

    if (kind == TypeKind::samplerType)
        return "sampler";

    if (kind == TypeKind::samplerShadow)
        return "sampler_comparison";

    if (isSamplerType (kind) || isSeparateTextureType (kind))
        return wgslTextureTypeName (kind, false);

    return "";
}

/** Formats a diagnostic the way every stage of the transpiler reports it. */
inline std::string formatDiagnostic (const SourceLocation& loc, const std::string& message)
{
    return std::to_string (loc.line) + ":" + std::to_string (loc.column) + ": " + message;
}

/** Error raised by the lowering stages, carrying a "line:column: message" diagnostic. */
class LoweringError : public std::runtime_error
{
public:
    LoweringError (const SourceLocation& loc, const std::string& message)
        : std::runtime_error (formatDiagnostic (loc, message))
    {
    }
};

} // namespace wgsl
} // namespace yup
