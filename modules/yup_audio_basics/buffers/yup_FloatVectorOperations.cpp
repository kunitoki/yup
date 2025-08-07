/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

namespace FloatVectorHelpers
{
#define YUP_INCREMENT_SRC_DEST     \
    dest += (16 / sizeof (*dest)); \
    src += (16 / sizeof (*dest));
#define YUP_INCREMENT_SRC1_SRC2_DEST \
    dest += (16 / sizeof (*dest));   \
    src1 += (16 / sizeof (*dest));   \
    src2 += (16 / sizeof (*dest));
#define YUP_INCREMENT_DEST dest += (16 / sizeof (*dest));

#if YUP_USE_SSE_INTRINSICS
static bool isAligned (const void* p) noexcept
{
    return (((pointer_sized_int) p) & 15) == 0;
}

struct BasicOps32
{
    using Type = float;
    using ParallelType = __m128;
    using IntegerType = __m128;

    enum
    {
        numParallel = 4
    };

    // Integer and parallel types are the same for SSE. On neon they have different types
    static forcedinline IntegerType toint (ParallelType v) noexcept { return v; }

    static forcedinline ParallelType toflt (IntegerType v) noexcept { return v; }

    static forcedinline ParallelType load1 (Type v) noexcept { return _mm_load1_ps (&v); }

    static forcedinline ParallelType loadA (const Type* v) noexcept { return _mm_load_ps (v); }

    static forcedinline ParallelType loadU (const Type* v) noexcept { return _mm_loadu_ps (v); }

    static forcedinline void storeA (Type* dest, ParallelType a) noexcept { _mm_store_ps (dest, a); }

    static forcedinline void storeU (Type* dest, ParallelType a) noexcept { _mm_storeu_ps (dest, a); }

    static forcedinline ParallelType add (ParallelType a, ParallelType b) noexcept { return _mm_add_ps (a, b); }

    static forcedinline ParallelType sub (ParallelType a, ParallelType b) noexcept { return _mm_sub_ps (a, b); }

    static forcedinline ParallelType mul (ParallelType a, ParallelType b) noexcept { return _mm_mul_ps (a, b); }

    static forcedinline ParallelType max (ParallelType a, ParallelType b) noexcept { return _mm_max_ps (a, b); }

    static forcedinline ParallelType min (ParallelType a, ParallelType b) noexcept { return _mm_min_ps (a, b); }

    static forcedinline ParallelType bit_and (ParallelType a, ParallelType b) noexcept { return _mm_and_ps (a, b); }

    static forcedinline ParallelType bit_not (ParallelType a, ParallelType b) noexcept { return _mm_andnot_ps (a, b); }

    static forcedinline ParallelType bit_or (ParallelType a, ParallelType b) noexcept { return _mm_or_ps (a, b); }

    static forcedinline ParallelType bit_xor (ParallelType a, ParallelType b) noexcept { return _mm_xor_ps (a, b); }

    static forcedinline Type max (ParallelType a) noexcept
    {
        Type v[numParallel];
        storeU (v, a);
        return jmax (v[0], v[1], v[2], v[3]);
    }

    static forcedinline Type min (ParallelType a) noexcept
    {
        Type v[numParallel];
        storeU (v, a);
        return jmin (v[0], v[1], v[2], v[3]);
    }
};

struct BasicOps64
{
    using Type = double;
    using ParallelType = __m128d;
    using IntegerType = __m128d;

    enum
    {
        numParallel = 2
    };

    // Integer and parallel types are the same for SSE. On neon they have different types
    static forcedinline IntegerType toint (ParallelType v) noexcept { return v; }

    static forcedinline ParallelType toflt (IntegerType v) noexcept { return v; }

    static forcedinline ParallelType load1 (Type v) noexcept { return _mm_load1_pd (&v); }

    static forcedinline ParallelType loadA (const Type* v) noexcept { return _mm_load_pd (v); }

    static forcedinline ParallelType loadU (const Type* v) noexcept { return _mm_loadu_pd (v); }

    static forcedinline void storeA (Type* dest, ParallelType a) noexcept { _mm_store_pd (dest, a); }

    static forcedinline void storeU (Type* dest, ParallelType a) noexcept { _mm_storeu_pd (dest, a); }

    static forcedinline ParallelType add (ParallelType a, ParallelType b) noexcept { return _mm_add_pd (a, b); }

    static forcedinline ParallelType sub (ParallelType a, ParallelType b) noexcept { return _mm_sub_pd (a, b); }

    static forcedinline ParallelType mul (ParallelType a, ParallelType b) noexcept { return _mm_mul_pd (a, b); }

    static forcedinline ParallelType max (ParallelType a, ParallelType b) noexcept { return _mm_max_pd (a, b); }

    static forcedinline ParallelType min (ParallelType a, ParallelType b) noexcept { return _mm_min_pd (a, b); }

    static forcedinline ParallelType bit_and (ParallelType a, ParallelType b) noexcept { return _mm_and_pd (a, b); }

    static forcedinline ParallelType bit_not (ParallelType a, ParallelType b) noexcept { return _mm_andnot_pd (a, b); }

    static forcedinline ParallelType bit_or (ParallelType a, ParallelType b) noexcept { return _mm_or_pd (a, b); }

    static forcedinline ParallelType bit_xor (ParallelType a, ParallelType b) noexcept { return _mm_xor_pd (a, b); }

    static forcedinline Type max (ParallelType a) noexcept
    {
        Type v[numParallel];
        storeU (v, a);
        return jmax (v[0], v[1]);
    }

    static forcedinline Type min (ParallelType a) noexcept
    {
        Type v[numParallel];
        storeU (v, a);
        return jmin (v[0], v[1]);
    }
};

#define YUP_BEGIN_VEC_OP                                             \
    using Mode = FloatVectorHelpers::ModeType<sizeof (*dest)>::Mode; \
    {                                                                \
        const auto numLongOps = num / Mode::numParallel;

#define YUP_FINISH_VEC_OP(normalOp)                 \
    num &= (Mode::numParallel - 1);                 \
    if (num == 0)                                   \
        return;                                     \
    }                                               \
    for (auto i = (decltype (num)) 0; i < num; ++i) \
        normalOp;

#define YUP_PERFORM_VEC_OP_DEST(normalOp, vecOp, locals, setupOp)                                                                                                                                                                \
    YUP_BEGIN_VEC_OP                                                                                                                                                                                                             \
    setupOp if (FloatVectorHelpers::isAligned (dest)) YUP_VEC_LOOP (vecOp, dummy, Mode::loadA, Mode::storeA, locals, YUP_INCREMENT_DEST) else YUP_VEC_LOOP (vecOp, dummy, Mode::loadU, Mode::storeU, locals, YUP_INCREMENT_DEST) \
        YUP_FINISH_VEC_OP (normalOp)

#define YUP_PERFORM_VEC_OP_SRC_DEST(normalOp, vecOp, locals, increment, setupOp)            \
    YUP_BEGIN_VEC_OP                                                                        \
    setupOp if (FloatVectorHelpers::isAligned (dest))                                       \
    {                                                                                       \
        if (FloatVectorHelpers::isAligned (src))                                            \
            YUP_VEC_LOOP (vecOp, Mode::loadA, Mode::loadA, Mode::storeA, locals, increment) \
        else                                                                                \
            YUP_VEC_LOOP (vecOp, Mode::loadU, Mode::loadA, Mode::storeA, locals, increment) \
    }                                                                                       \
    else                                                                                    \
    {                                                                                       \
        if (FloatVectorHelpers::isAligned (src))                                            \
            YUP_VEC_LOOP (vecOp, Mode::loadA, Mode::loadU, Mode::storeU, locals, increment) \
        else                                                                                \
            YUP_VEC_LOOP (vecOp, Mode::loadU, Mode::loadU, Mode::storeU, locals, increment) \
    }                                                                                       \
    YUP_FINISH_VEC_OP (normalOp)

#define YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST(normalOp, vecOp, locals, increment, setupOp)                      \
    YUP_BEGIN_VEC_OP                                                                                        \
    setupOp if (FloatVectorHelpers::isAligned (dest))                                                       \
    {                                                                                                       \
        if (FloatVectorHelpers::isAligned (src1))                                                           \
        {                                                                                                   \
            if (FloatVectorHelpers::isAligned (src2))                                                       \
                YUP_VEC_LOOP_TWO_SOURCES (vecOp, Mode::loadA, Mode::loadA, Mode::storeA, locals, increment) \
            else                                                                                            \
                YUP_VEC_LOOP_TWO_SOURCES (vecOp, Mode::loadA, Mode::loadU, Mode::storeA, locals, increment) \
        }                                                                                                   \
        else                                                                                                \
        {                                                                                                   \
            if (FloatVectorHelpers::isAligned (src2))                                                       \
                YUP_VEC_LOOP_TWO_SOURCES (vecOp, Mode::loadU, Mode::loadA, Mode::storeA, locals, increment) \
            else                                                                                            \
                YUP_VEC_LOOP_TWO_SOURCES (vecOp, Mode::loadU, Mode::loadU, Mode::storeA, locals, increment) \
        }                                                                                                   \
    }                                                                                                       \
    else                                                                                                    \
    {                                                                                                       \
        if (FloatVectorHelpers::isAligned (src1))                                                           \
        {                                                                                                   \
            if (FloatVectorHelpers::isAligned (src2))                                                       \
                YUP_VEC_LOOP_TWO_SOURCES (vecOp, Mode::loadA, Mode::loadA, Mode::storeU, locals, increment) \
            else                                                                                            \
                YUP_VEC_LOOP_TWO_SOURCES (vecOp, Mode::loadA, Mode::loadU, Mode::storeU, locals, increment) \
        }                                                                                                   \
        else                                                                                                \
        {                                                                                                   \
            if (FloatVectorHelpers::isAligned (src2))                                                       \
                YUP_VEC_LOOP_TWO_SOURCES (vecOp, Mode::loadU, Mode::loadA, Mode::storeU, locals, increment) \
            else                                                                                            \
                YUP_VEC_LOOP_TWO_SOURCES (vecOp, Mode::loadU, Mode::loadU, Mode::storeU, locals, increment) \
        }                                                                                                   \
    }                                                                                                       \
    YUP_FINISH_VEC_OP (normalOp)

#define YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST_DEST(normalOp, vecOp, locals, increment, setupOp)                                             \
    YUP_BEGIN_VEC_OP                                                                                                                    \
    setupOp if (FloatVectorHelpers::isAligned (dest))                                                                                   \
    {                                                                                                                                   \
        if (FloatVectorHelpers::isAligned (src1))                                                                                       \
        {                                                                                                                               \
            if (FloatVectorHelpers::isAligned (src2))                                                                                   \
                YUP_VEC_LOOP_TWO_SOURCES_WITH_DEST_LOAD (vecOp, Mode::loadA, Mode::loadA, Mode::loadA, Mode::storeA, locals, increment) \
            else                                                                                                                        \
                YUP_VEC_LOOP_TWO_SOURCES_WITH_DEST_LOAD (vecOp, Mode::loadA, Mode::loadU, Mode::loadA, Mode::storeA, locals, increment) \
        }                                                                                                                               \
        else                                                                                                                            \
        {                                                                                                                               \
            if (FloatVectorHelpers::isAligned (src2))                                                                                   \
                YUP_VEC_LOOP_TWO_SOURCES_WITH_DEST_LOAD (vecOp, Mode::loadU, Mode::loadA, Mode::loadA, Mode::storeA, locals, increment) \
            else                                                                                                                        \
                YUP_VEC_LOOP_TWO_SOURCES_WITH_DEST_LOAD (vecOp, Mode::loadU, Mode::loadU, Mode::loadA, Mode::storeA, locals, increment) \
        }                                                                                                                               \
    }                                                                                                                                   \
    else                                                                                                                                \
    {                                                                                                                                   \
        if (FloatVectorHelpers::isAligned (src1))                                                                                       \
        {                                                                                                                               \
            if (FloatVectorHelpers::isAligned (src2))                                                                                   \
                YUP_VEC_LOOP_TWO_SOURCES_WITH_DEST_LOAD (vecOp, Mode::loadA, Mode::loadA, Mode::loadU, Mode::storeU, locals, increment) \
            else                                                                                                                        \
                YUP_VEC_LOOP_TWO_SOURCES_WITH_DEST_LOAD (vecOp, Mode::loadA, Mode::loadU, Mode::loadU, Mode::storeU, locals, increment) \
        }                                                                                                                               \
        else                                                                                                                            \
        {                                                                                                                               \
            if (FloatVectorHelpers::isAligned (src2))                                                                                   \
                YUP_VEC_LOOP_TWO_SOURCES_WITH_DEST_LOAD (vecOp, Mode::loadU, Mode::loadA, Mode::loadU, Mode::storeU, locals, increment) \
            else                                                                                                                        \
                YUP_VEC_LOOP_TWO_SOURCES_WITH_DEST_LOAD (vecOp, Mode::loadU, Mode::loadU, Mode::loadU, Mode::storeU, locals, increment) \
        }                                                                                                                               \
    }                                                                                                                                   \
    YUP_FINISH_VEC_OP (normalOp)

//==============================================================================
#elif YUP_USE_ARM_NEON

struct BasicOps32
{
    using Type = float;
    using ParallelType = float32x4_t;
    using IntegerType = uint32x4_t;

    union signMaskUnion
    {
        ParallelType f;
        IntegerType i;
    };

    enum
    {
        numParallel = 4
    };

    static forcedinline IntegerType toint (ParallelType v) noexcept
    {
        signMaskUnion u;
        u.f = v;
        return u.i;
    }

    static forcedinline ParallelType toflt (IntegerType v) noexcept
    {
        signMaskUnion u;
        u.i = v;
        return u.f;
    }

    static forcedinline ParallelType load1 (Type v) noexcept { return vld1q_dup_f32 (&v); }

    static forcedinline ParallelType loadA (const Type* v) noexcept { return vld1q_f32 (v); }

    static forcedinline ParallelType loadU (const Type* v) noexcept { return vld1q_f32 (v); }

    static forcedinline void storeA (Type* dest, ParallelType a) noexcept { vst1q_f32 (dest, a); }

    static forcedinline void storeU (Type* dest, ParallelType a) noexcept { vst1q_f32 (dest, a); }

    static forcedinline ParallelType add (ParallelType a, ParallelType b) noexcept { return vaddq_f32 (a, b); }

    static forcedinline ParallelType sub (ParallelType a, ParallelType b) noexcept { return vsubq_f32 (a, b); }

    static forcedinline ParallelType mul (ParallelType a, ParallelType b) noexcept { return vmulq_f32 (a, b); }

    static forcedinline ParallelType max (ParallelType a, ParallelType b) noexcept { return vmaxq_f32 (a, b); }

    static forcedinline ParallelType min (ParallelType a, ParallelType b) noexcept { return vminq_f32 (a, b); }

    static forcedinline ParallelType bit_and (ParallelType a, ParallelType b) noexcept { return toflt (vandq_u32 (toint (a), toint (b))); }

    static forcedinline ParallelType bit_not (ParallelType a, ParallelType b) noexcept { return toflt (vbicq_u32 (toint (a), toint (b))); }

    static forcedinline ParallelType bit_or (ParallelType a, ParallelType b) noexcept { return toflt (vorrq_u32 (toint (a), toint (b))); }

    static forcedinline ParallelType bit_xor (ParallelType a, ParallelType b) noexcept { return toflt (veorq_u32 (toint (a), toint (b))); }

    static forcedinline Type max (ParallelType a) noexcept
    {
        Type v[numParallel];
        storeU (v, a);
        return jmax (v[0], v[1], v[2], v[3]);
    }

    static forcedinline Type min (ParallelType a) noexcept
    {
        Type v[numParallel];
        storeU (v, a);
        return jmin (v[0], v[1], v[2], v[3]);
    }
};

struct BasicOps64
{
    using Type = double;
    using ParallelType = double;
    using IntegerType = uint64;

    union signMaskUnion
    {
        ParallelType f;
        IntegerType i;
    };

    enum
    {
        numParallel = 1
    };

    static forcedinline IntegerType toint (ParallelType v) noexcept
    {
        signMaskUnion u;
        u.f = v;
        return u.i;
    }

    static forcedinline ParallelType toflt (IntegerType v) noexcept
    {
        signMaskUnion u;
        u.i = v;
        return u.f;
    }

    static forcedinline ParallelType load1 (Type v) noexcept { return v; }

    static forcedinline ParallelType loadA (const Type* v) noexcept { return *v; }

    static forcedinline ParallelType loadU (const Type* v) noexcept { return *v; }

    static forcedinline void storeA (Type* dest, ParallelType a) noexcept { *dest = a; }

    static forcedinline void storeU (Type* dest, ParallelType a) noexcept { *dest = a; }

    static forcedinline ParallelType add (ParallelType a, ParallelType b) noexcept { return a + b; }

    static forcedinline ParallelType sub (ParallelType a, ParallelType b) noexcept { return a - b; }

    static forcedinline ParallelType mul (ParallelType a, ParallelType b) noexcept { return a * b; }

    static forcedinline ParallelType max (ParallelType a, ParallelType b) noexcept { return jmax (a, b); }

    static forcedinline ParallelType min (ParallelType a, ParallelType b) noexcept { return jmin (a, b); }

    static forcedinline ParallelType bit_and (ParallelType a, ParallelType b) noexcept { return toflt (toint (a) & toint (b)); }

    static forcedinline ParallelType bit_not (ParallelType a, ParallelType b) noexcept { return toflt ((~toint (a)) & toint (b)); }

    static forcedinline ParallelType bit_or (ParallelType a, ParallelType b) noexcept { return toflt (toint (a) | toint (b)); }

    static forcedinline ParallelType bit_xor (ParallelType a, ParallelType b) noexcept { return toflt (toint (a) ^ toint (b)); }

    static forcedinline Type max (ParallelType a) noexcept { return a; }

    static forcedinline Type min (ParallelType a) noexcept { return a; }
};

#define YUP_BEGIN_VEC_OP                                             \
    using Mode = FloatVectorHelpers::ModeType<sizeof (*dest)>::Mode; \
    if (Mode::numParallel > 1)                                       \
    {                                                                \
        const auto numLongOps = num / Mode::numParallel;

#define YUP_FINISH_VEC_OP(normalOp)                 \
    num &= (Mode::numParallel - 1);                 \
    if (num == 0)                                   \
        return;                                     \
    }                                               \
    for (auto i = (decltype (num)) 0; i < num; ++i) \
        normalOp;

#define YUP_PERFORM_VEC_OP_DEST(normalOp, vecOp, locals, setupOp)                          \
    YUP_BEGIN_VEC_OP                                                                       \
    setupOp                                                                                \
        YUP_VEC_LOOP (vecOp, dummy, Mode::loadU, Mode::storeU, locals, YUP_INCREMENT_DEST) \
            YUP_FINISH_VEC_OP (normalOp)

#define YUP_PERFORM_VEC_OP_SRC_DEST(normalOp, vecOp, locals, increment, setupOp)        \
    YUP_BEGIN_VEC_OP                                                                    \
    setupOp                                                                             \
        YUP_VEC_LOOP (vecOp, Mode::loadU, Mode::loadU, Mode::storeU, locals, increment) \
            YUP_FINISH_VEC_OP (normalOp)

#define YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST(normalOp, vecOp, locals, increment, setupOp)              \
    YUP_BEGIN_VEC_OP                                                                                \
    setupOp                                                                                         \
        YUP_VEC_LOOP_TWO_SOURCES (vecOp, Mode::loadU, Mode::loadU, Mode::storeU, locals, increment) \
            YUP_FINISH_VEC_OP (normalOp)

#define YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST_DEST(normalOp, vecOp, locals, increment, setupOp)                                     \
    YUP_BEGIN_VEC_OP                                                                                                            \
    setupOp                                                                                                                     \
        YUP_VEC_LOOP_TWO_SOURCES_WITH_DEST_LOAD (vecOp, Mode::loadU, Mode::loadU, Mode::loadU, Mode::storeU, locals, increment) \
            YUP_FINISH_VEC_OP (normalOp)

//==============================================================================
#else
#define YUP_PERFORM_VEC_OP_DEST(normalOp, vecOp, locals, setupOp) \
    for (auto i = (decltype (num)) 0; i < num; ++i)               \
        normalOp;

#define YUP_PERFORM_VEC_OP_SRC_DEST(normalOp, vecOp, locals, increment, setupOp) \
    for (auto i = (decltype (num)) 0; i < num; ++i)                              \
        normalOp;

#define YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST(normalOp, vecOp, locals, increment, setupOp) \
    for (auto i = (decltype (num)) 0; i < num; ++i)                                    \
        normalOp;

#define YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST_DEST(normalOp, vecOp, locals, increment, setupOp) \
    for (auto i = (decltype (num)) 0; i < num; ++i)                                         \
        normalOp;

#endif

//==============================================================================
#define YUP_VEC_LOOP(vecOp, srcLoad, dstLoad, dstStore, locals, increment) \
    for (auto i = (decltype (numLongOps)) 0; i < numLongOps; ++i)          \
    {                                                                      \
        locals (srcLoad, dstLoad);                                         \
        dstStore (dest, vecOp);                                            \
        increment;                                                         \
    }

#define YUP_VEC_LOOP_TWO_SOURCES(vecOp, src1Load, src2Load, dstStore, locals, increment) \
    for (auto i = (decltype (numLongOps)) 0; i < numLongOps; ++i)                        \
    {                                                                                    \
        locals (src1Load, src2Load);                                                     \
        dstStore (dest, vecOp);                                                          \
        increment;                                                                       \
    }

#define YUP_VEC_LOOP_TWO_SOURCES_WITH_DEST_LOAD(vecOp, src1Load, src2Load, dstLoad, dstStore, locals, increment) \
    for (auto i = (decltype (numLongOps)) 0; i < numLongOps; ++i)                                                \
    {                                                                                                            \
        locals (src1Load, src2Load, dstLoad);                                                                    \
        dstStore (dest, vecOp);                                                                                  \
        increment;                                                                                               \
    }

#define YUP_LOAD_NONE(srcLoad, dstLoad)
#define YUP_LOAD_DEST(srcLoad, dstLoad) const Mode::ParallelType d = dstLoad (dest);
#define YUP_LOAD_SRC(srcLoad, dstLoad) const Mode::ParallelType s = srcLoad (src);
#define YUP_LOAD_SRC1_SRC2(src1Load, src2Load) const Mode::ParallelType s1 = src1Load (src1), s2 = src2Load (src2);
#define YUP_LOAD_SRC1_SRC2_DEST(src1Load, src2Load, dstLoad) const Mode::ParallelType d = dstLoad (dest), s1 = src1Load (src1), s2 = src2Load (src2);
#define YUP_LOAD_SRC_DEST(srcLoad, dstLoad) const Mode::ParallelType d = dstLoad (dest), s = srcLoad (src);

union signMask32
{
    float f;
    uint32 i;
};

union signMask64
{
    double d;
    uint64 i;
};

#if YUP_USE_SSE_INTRINSICS || YUP_USE_ARM_NEON
template <int typeSize>
struct ModeType
{
    using Mode = BasicOps32;
};

template <>
struct ModeType<8>
{
    using Mode = BasicOps64;
};

template <typename Mode>
struct MinMax
{
    using Type = typename Mode::Type;
    using ParallelType = typename Mode::ParallelType;

    template <typename Size>
    static Type findMinOrMax (const Type* src, Size num, const bool isMinimum) noexcept
    {
        auto numLongOps = num / Mode::numParallel;

        if (numLongOps > 1)
        {
            ParallelType val;

#if ! YUP_USE_ARM_NEON
            if (isAligned (src))
            {
                val = Mode::loadA (src);

                if (isMinimum)
                {
                    while (--numLongOps > 0)
                    {
                        src += Mode::numParallel;
                        val = Mode::min (val, Mode::loadA (src));
                    }
                }
                else
                {
                    while (--numLongOps > 0)
                    {
                        src += Mode::numParallel;
                        val = Mode::max (val, Mode::loadA (src));
                    }
                }
            }
            else
#endif
            {
                val = Mode::loadU (src);

                if (isMinimum)
                {
                    while (--numLongOps > 0)
                    {
                        src += Mode::numParallel;
                        val = Mode::min (val, Mode::loadU (src));
                    }
                }
                else
                {
                    while (--numLongOps > 0)
                    {
                        src += Mode::numParallel;
                        val = Mode::max (val, Mode::loadU (src));
                    }
                }
            }

            Type result = isMinimum ? Mode::min (val)
                                    : Mode::max (val);

            num &= (Mode::numParallel - 1);
            src += Mode::numParallel;

            for (auto i = (decltype (num)) 0; i < num; ++i)
                result = isMinimum ? jmin (result, src[i])
                                   : jmax (result, src[i]);

            return result;
        }

        if (num <= 0)
            return 0;

        return isMinimum ? *std::min_element (src, src + num)
                         : *std::max_element (src, src + num);
    }

    template <typename Size>
    static Range<Type> findMinAndMax (const Type* src, Size num) noexcept
    {
        auto numLongOps = num / Mode::numParallel;

        if (numLongOps > 1)
        {
            ParallelType mn, mx;

#if ! YUP_USE_ARM_NEON
            if (isAligned (src))
            {
                mn = Mode::loadA (src);
                mx = mn;

                while (--numLongOps > 0)
                {
                    src += Mode::numParallel;
                    const ParallelType v = Mode::loadA (src);
                    mn = Mode::min (mn, v);
                    mx = Mode::max (mx, v);
                }
            }
            else
#endif
            {
                mn = Mode::loadU (src);
                mx = mn;

                while (--numLongOps > 0)
                {
                    src += Mode::numParallel;
                    const ParallelType v = Mode::loadU (src);
                    mn = Mode::min (mn, v);
                    mx = Mode::max (mx, v);
                }
            }

            Range<Type> result (Mode::min (mn),
                                Mode::max (mx));

            num &= (Mode::numParallel - 1);
            src += Mode::numParallel;

            for (auto i = (decltype (num)) 0; i < num; ++i)
                result = result.getUnionWith (src[i]);

            return result;
        }

        return Range<Type>::findMinAndMax (src, num);
    }
};
#endif

//==============================================================================
namespace
{
template <typename Size>
void clear (float* dest, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vclr (dest, 1, (vDSP_Length) num);
#else
    zeromem (dest, (size_t) num * sizeof (float));
#endif
}

template <typename Size>
void clear (double* dest, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vclrD (dest, 1, (vDSP_Length) num);
#else
    zeromem (dest, (size_t) num * sizeof (double));
#endif
}

template <typename Size>
void fill (float* dest, float valueToFill, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vfill (&valueToFill, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_DEST (dest[i] = valueToFill,
                             val,
                             YUP_LOAD_NONE,
                             const Mode::ParallelType val = Mode::load1 (valueToFill);)
#endif
}

template <typename Size>
void fill (double* dest, double valueToFill, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vfillD (&valueToFill, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_DEST (dest[i] = valueToFill,
                             val,
                             YUP_LOAD_NONE,
                             const Mode::ParallelType val = Mode::load1 (valueToFill);)
#endif
}

template <typename Size>
void copyWithMultiply (float* dest, const float* src, float multiplier, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vsmul (src, 1, &multiplier, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] = src[i] * multiplier,
                                 Mode::mul (mult, s),
                                 YUP_LOAD_SRC,
                                 YUP_INCREMENT_SRC_DEST,
                                 const Mode::ParallelType mult = Mode::load1 (multiplier);)
#endif
}

template <typename Size>
void copyWithMultiply (double* dest, const double* src, double multiplier, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vsmulD (src, 1, &multiplier, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] = src[i] * multiplier,
                                 Mode::mul (mult, s),
                                 YUP_LOAD_SRC,
                                 YUP_INCREMENT_SRC_DEST,
                                 const Mode::ParallelType mult = Mode::load1 (multiplier);)
#endif
}

template <typename Size>
void add (float* dest, float amount, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vsadd (dest, 1, &amount, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_DEST (dest[i] += amount,
                             Mode::add (d, amountToAdd),
                             YUP_LOAD_DEST,
                             const Mode::ParallelType amountToAdd = Mode::load1 (amount);)
#endif
}

template <typename Size>
void add (double* dest, double amount, Size num) noexcept
{
    YUP_PERFORM_VEC_OP_DEST (dest[i] += amount,
                             Mode::add (d, amountToAdd),
                             YUP_LOAD_DEST,
                             const Mode::ParallelType amountToAdd = Mode::load1 (amount);)
}

template <typename Size>
void add (float* dest, const float* src, float amount, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vsadd (src, 1, &amount, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] = src[i] + amount,
                                 Mode::add (am, s),
                                 YUP_LOAD_SRC,
                                 YUP_INCREMENT_SRC_DEST,
                                 const Mode::ParallelType am = Mode::load1 (amount);)
#endif
}

template <typename Size>
void add (double* dest, const double* src, double amount, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vsaddD (src, 1, &amount, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] = src[i] + amount,
                                 Mode::add (am, s),
                                 YUP_LOAD_SRC,
                                 YUP_INCREMENT_SRC_DEST,
                                 const Mode::ParallelType am = Mode::load1 (amount);)
#endif
}

template <typename Size>
void add (float* dest, const float* src, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vadd (src, 1, dest, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] += src[i],
                                 Mode::add (d, s),
                                 YUP_LOAD_SRC_DEST,
                                 YUP_INCREMENT_SRC_DEST, )
#endif
}

template <typename Size>
void add (double* dest, const double* src, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vaddD (src, 1, dest, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] += src[i],
                                 Mode::add (d, s),
                                 YUP_LOAD_SRC_DEST,
                                 YUP_INCREMENT_SRC_DEST, )
#endif
}

template <typename Size>
void add (float* dest, const float* src1, const float* src2, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vadd (src1, 1, src2, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST (dest[i] = src1[i] + src2[i],
                                       Mode::add (s1, s2),
                                       YUP_LOAD_SRC1_SRC2,
                                       YUP_INCREMENT_SRC1_SRC2_DEST, )
#endif
}

template <typename Size>
void add (double* dest, const double* src1, const double* src2, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vaddD (src1, 1, src2, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST (dest[i] = src1[i] + src2[i],
                                       Mode::add (s1, s2),
                                       YUP_LOAD_SRC1_SRC2,
                                       YUP_INCREMENT_SRC1_SRC2_DEST, )
#endif
}

template <typename Size>
void subtract (float* dest, const float* src, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vsub (src, 1, dest, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] -= src[i],
                                 Mode::sub (d, s),
                                 YUP_LOAD_SRC_DEST,
                                 YUP_INCREMENT_SRC_DEST, )
#endif
}

template <typename Size>
void subtract (double* dest, const double* src, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vsubD (src, 1, dest, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] -= src[i],
                                 Mode::sub (d, s),
                                 YUP_LOAD_SRC_DEST,
                                 YUP_INCREMENT_SRC_DEST, )
#endif
}

template <typename Size>
void subtract (float* dest, const float* src1, const float* src2, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vsub (src2, 1, src1, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST (dest[i] = src1[i] - src2[i],
                                       Mode::sub (s1, s2),
                                       YUP_LOAD_SRC1_SRC2,
                                       YUP_INCREMENT_SRC1_SRC2_DEST, )
#endif
}

template <typename Size>
void subtract (double* dest, const double* src1, const double* src2, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vsubD (src2, 1, src1, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST (dest[i] = src1[i] - src2[i],
                                       Mode::sub (s1, s2),
                                       YUP_LOAD_SRC1_SRC2,
                                       YUP_INCREMENT_SRC1_SRC2_DEST, )
#endif
}

template <typename Size>
void addWithMultiply (float* dest, const float* src, float multiplier, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vsma (src, 1, &multiplier, dest, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] += src[i] * multiplier,
                                 Mode::add (d, Mode::mul (mult, s)),
                                 YUP_LOAD_SRC_DEST,
                                 YUP_INCREMENT_SRC_DEST,
                                 const Mode::ParallelType mult = Mode::load1 (multiplier);)
#endif
}

template <typename Size>
void addWithMultiply (double* dest, const double* src, double multiplier, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vsmaD (src, 1, &multiplier, dest, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] += src[i] * multiplier,
                                 Mode::add (d, Mode::mul (mult, s)),
                                 YUP_LOAD_SRC_DEST,
                                 YUP_INCREMENT_SRC_DEST,
                                 const Mode::ParallelType mult = Mode::load1 (multiplier);)
#endif
}

template <typename Size>
void addWithMultiply (float* dest, const float* src1, const float* src2, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vma ((float*) src1, 1, (float*) src2, 1, dest, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST_DEST (dest[i] += src1[i] * src2[i],
                                            Mode::add (d, Mode::mul (s1, s2)),
                                            YUP_LOAD_SRC1_SRC2_DEST,
                                            YUP_INCREMENT_SRC1_SRC2_DEST, )
#endif
}

template <typename Size>
void addWithMultiply (double* dest, const double* src1, const double* src2, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vmaD ((double*) src1, 1, (double*) src2, 1, dest, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST_DEST (dest[i] += src1[i] * src2[i],
                                            Mode::add (d, Mode::mul (s1, s2)),
                                            YUP_LOAD_SRC1_SRC2_DEST,
                                            YUP_INCREMENT_SRC1_SRC2_DEST, )
#endif
}

template <typename Size>
void subtractWithMultiply (float* dest, const float* src, float multiplier, Size num) noexcept
{
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] -= src[i] * multiplier,
                                 Mode::sub (d, Mode::mul (mult, s)),
                                 YUP_LOAD_SRC_DEST,
                                 YUP_INCREMENT_SRC_DEST,
                                 const Mode::ParallelType mult = Mode::load1 (multiplier);)
}

template <typename Size>
void subtractWithMultiply (double* dest, const double* src, double multiplier, Size num) noexcept
{
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] -= src[i] * multiplier,
                                 Mode::sub (d, Mode::mul (mult, s)),
                                 YUP_LOAD_SRC_DEST,
                                 YUP_INCREMENT_SRC_DEST,
                                 const Mode::ParallelType mult = Mode::load1 (multiplier);)
}

template <typename Size>
void subtractWithMultiply (float* dest, const float* src1, const float* src2, Size num) noexcept
{
    YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST_DEST (dest[i] -= src1[i] * src2[i],
                                            Mode::sub (d, Mode::mul (s1, s2)),
                                            YUP_LOAD_SRC1_SRC2_DEST,
                                            YUP_INCREMENT_SRC1_SRC2_DEST, )
}

template <typename Size>
void subtractWithMultiply (double* dest, const double* src1, const double* src2, Size num) noexcept
{
    YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST_DEST (dest[i] -= src1[i] * src2[i],
                                            Mode::sub (d, Mode::mul (s1, s2)),
                                            YUP_LOAD_SRC1_SRC2_DEST,
                                            YUP_INCREMENT_SRC1_SRC2_DEST, )
}

template <typename Size>
void multiply (float* dest, const float* src, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vmul (src, 1, dest, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] *= src[i],
                                 Mode::mul (d, s),
                                 YUP_LOAD_SRC_DEST,
                                 YUP_INCREMENT_SRC_DEST, )
#endif
}

template <typename Size>
void multiply (double* dest, const double* src, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vmulD (src, 1, dest, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] *= src[i],
                                 Mode::mul (d, s),
                                 YUP_LOAD_SRC_DEST,
                                 YUP_INCREMENT_SRC_DEST, )
#endif
}

template <typename Size>
void multiply (float* dest, const float* src1, const float* src2, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vmul (src1, 1, src2, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST (dest[i] = src1[i] * src2[i],
                                       Mode::mul (s1, s2),
                                       YUP_LOAD_SRC1_SRC2,
                                       YUP_INCREMENT_SRC1_SRC2_DEST, )
#endif
}

template <typename Size>
void multiply (double* dest, const double* src1, const double* src2, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vmulD (src1, 1, src2, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST (dest[i] = src1[i] * src2[i],
                                       Mode::mul (s1, s2),
                                       YUP_LOAD_SRC1_SRC2,
                                       YUP_INCREMENT_SRC1_SRC2_DEST, )
#endif
}

template <typename Size>
void multiply (float* dest, float multiplier, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vsmul (dest, 1, &multiplier, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_DEST (dest[i] *= multiplier,
                             Mode::mul (d, mult),
                             YUP_LOAD_DEST,
                             const Mode::ParallelType mult = Mode::load1 (multiplier);)
#endif
}

template <typename Size>
void multiply (double* dest, double multiplier, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vsmulD (dest, 1, &multiplier, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_DEST (dest[i] *= multiplier,
                             Mode::mul (d, mult),
                             YUP_LOAD_DEST,
                             const Mode::ParallelType mult = Mode::load1 (multiplier);)
#endif
}

template <typename Size>
void multiply (float* dest, const float* src, float multiplier, Size num) noexcept
{
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] = src[i] * multiplier,
                                 Mode::mul (mult, s),
                                 YUP_LOAD_SRC,
                                 YUP_INCREMENT_SRC_DEST,
                                 const Mode::ParallelType mult = Mode::load1 (multiplier);)
}

template <typename Size>
void multiply (double* dest, const double* src, double multiplier, Size num) noexcept
{
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] = src[i] * multiplier,
                                 Mode::mul (mult, s),
                                 YUP_LOAD_SRC,
                                 YUP_INCREMENT_SRC_DEST,
                                 const Mode::ParallelType mult = Mode::load1 (multiplier);)
}

template <typename Size>
void negate (float* dest, const float* src, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vneg ((float*) src, 1, dest, 1, (vDSP_Length) num);
#else
    copyWithMultiply (dest, src, -1.0f, num);
#endif
}

template <typename Size>
void negate (double* dest, const double* src, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vnegD ((double*) src, 1, dest, 1, (vDSP_Length) num);
#else
    copyWithMultiply (dest, src, -1.0f, num);
#endif
}

template <typename Size>
void abs (float* dest, const float* src, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vabs ((float*) src, 1, dest, 1, (vDSP_Length) num);
#else
    [[maybe_unused]] FloatVectorHelpers::signMask32 signMask;
    signMask.i = 0x7fffffffUL;
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] = std::abs (src[i]),
                                 Mode::bit_and (s, mask),
                                 YUP_LOAD_SRC,
                                 YUP_INCREMENT_SRC_DEST,
                                 const Mode::ParallelType mask = Mode::load1 (signMask.f);)
#endif
}

template <typename Size>
void abs (double* dest, const double* src, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vabsD ((double*) src, 1, dest, 1, (vDSP_Length) num);
#else
    [[maybe_unused]] FloatVectorHelpers::signMask64 signMask;
    signMask.i = 0x7fffffffffffffffULL;

    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] = std::abs (src[i]),
                                 Mode::bit_and (s, mask),
                                 YUP_LOAD_SRC,
                                 YUP_INCREMENT_SRC_DEST,
                                 const Mode::ParallelType mask = Mode::load1 (signMask.d);)
#endif
}

template <typename Size>
void min (float* dest, const float* src, float comp, Size num) noexcept
{
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] = jmin (src[i], comp),
                                 Mode::min (s, cmp),
                                 YUP_LOAD_SRC,
                                 YUP_INCREMENT_SRC_DEST,
                                 const Mode::ParallelType cmp = Mode::load1 (comp);)
}

template <typename Size>
void min (double* dest, const double* src, double comp, Size num) noexcept
{
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] = jmin (src[i], comp),
                                 Mode::min (s, cmp),
                                 YUP_LOAD_SRC,
                                 YUP_INCREMENT_SRC_DEST,
                                 const Mode::ParallelType cmp = Mode::load1 (comp);)
}

template <typename Size>
void min (float* dest, const float* src1, const float* src2, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vmin ((float*) src1, 1, (float*) src2, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST (dest[i] = jmin (src1[i], src2[i]),
                                       Mode::min (s1, s2),
                                       YUP_LOAD_SRC1_SRC2,
                                       YUP_INCREMENT_SRC1_SRC2_DEST, )
#endif
}

template <typename Size>
void min (double* dest, const double* src1, const double* src2, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vminD ((double*) src1, 1, (double*) src2, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST (dest[i] = jmin (src1[i], src2[i]),
                                       Mode::min (s1, s2),
                                       YUP_LOAD_SRC1_SRC2,
                                       YUP_INCREMENT_SRC1_SRC2_DEST, )
#endif
}

template <typename Size>
void max (float* dest, const float* src, float comp, Size num) noexcept
{
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] = jmax (src[i], comp),
                                 Mode::max (s, cmp),
                                 YUP_LOAD_SRC,
                                 YUP_INCREMENT_SRC_DEST,
                                 const Mode::ParallelType cmp = Mode::load1 (comp);)
}

template <typename Size>
void max (double* dest, const double* src, double comp, Size num) noexcept
{
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] = jmax (src[i], comp),
                                 Mode::max (s, cmp),
                                 YUP_LOAD_SRC,
                                 YUP_INCREMENT_SRC_DEST,
                                 const Mode::ParallelType cmp = Mode::load1 (comp);)
}

template <typename Size>
void max (float* dest, const float* src1, const float* src2, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vmax ((float*) src1, 1, (float*) src2, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST (dest[i] = jmax (src1[i], src2[i]),
                                       Mode::max (s1, s2),
                                       YUP_LOAD_SRC1_SRC2,
                                       YUP_INCREMENT_SRC1_SRC2_DEST, )
#endif
}

template <typename Size>
void max (double* dest, const double* src1, const double* src2, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vmaxD ((double*) src1, 1, (double*) src2, 1, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC1_SRC2_DEST (dest[i] = jmax (src1[i], src2[i]),
                                       Mode::max (s1, s2),
                                       YUP_LOAD_SRC1_SRC2,
                                       YUP_INCREMENT_SRC1_SRC2_DEST, )
#endif
}

template <typename Size>
void clip (float* dest, const float* src, float low, float high, Size num) noexcept
{
    jassert (high >= low);

#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vclip ((float*) src, 1, &low, &high, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] = jmax (jmin (src[i], high), low),
                                 Mode::max (Mode::min (s, hi), lo),
                                 YUP_LOAD_SRC,
                                 YUP_INCREMENT_SRC_DEST,
                                 const Mode::ParallelType lo = Mode::load1 (low);
                                 const Mode::ParallelType hi = Mode::load1 (high);)
#endif
}

template <typename Size>
void clip (double* dest, const double* src, double low, double high, Size num) noexcept
{
    jassert (high >= low);

#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vclipD ((double*) src, 1, &low, &high, dest, 1, (vDSP_Length) num);
#else
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] = jmax (jmin (src[i], high), low),
                                 Mode::max (Mode::min (s, hi), lo),
                                 YUP_LOAD_SRC,
                                 YUP_INCREMENT_SRC_DEST,
                                 const Mode::ParallelType lo = Mode::load1 (low);
                                 const Mode::ParallelType hi = Mode::load1 (high);)
#endif
}

template <typename Size>
Range<float> findMinAndMax (const float* src, Size num) noexcept
{
#if YUP_USE_SSE_INTRINSICS || YUP_USE_ARM_NEON
    return FloatVectorHelpers::MinMax<FloatVectorHelpers::BasicOps32>::findMinAndMax (src, num);
#else
    return Range<float>::findMinAndMax (src, num);
#endif
}

template <typename Size>
Range<double> findMinAndMax (const double* src, Size num) noexcept
{
#if YUP_USE_SSE_INTRINSICS || YUP_USE_ARM_NEON
    return FloatVectorHelpers::MinMax<FloatVectorHelpers::BasicOps64>::findMinAndMax (src, num);
#else
    return Range<double>::findMinAndMax (src, num);
#endif
}

template <typename Size>
float findMinimum (const float* src, Size num) noexcept
{
#if YUP_USE_SSE_INTRINSICS || YUP_USE_ARM_NEON
    return FloatVectorHelpers::MinMax<FloatVectorHelpers::BasicOps32>::findMinOrMax (src, num, true);
#else
    return yup::findMinimum (src, num);
#endif
}

template <typename Size>
double findMinimum (const double* src, Size num) noexcept
{
#if YUP_USE_SSE_INTRINSICS || YUP_USE_ARM_NEON
    return FloatVectorHelpers::MinMax<FloatVectorHelpers::BasicOps64>::findMinOrMax (src, num, true);
#else
    return yup::findMinimum (src, num);
#endif
}

template <typename Size>
float findMaximum (const float* src, Size num) noexcept
{
#if YUP_USE_SSE_INTRINSICS || YUP_USE_ARM_NEON
    return FloatVectorHelpers::MinMax<FloatVectorHelpers::BasicOps32>::findMinOrMax (src, num, false);
#else
    return yup::findMaximum (src, num);
#endif
}

template <typename Size>
double findMaximum (const double* src, Size num) noexcept
{
#if YUP_USE_SSE_INTRINSICS || YUP_USE_ARM_NEON
    return FloatVectorHelpers::MinMax<FloatVectorHelpers::BasicOps64>::findMinOrMax (src, num, false);
#else
    return yup::findMaximum (src, num);
#endif
}

template <typename Size>
void convertFixedToFloat (float* dest, const int* src, float multiplier, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vflt32 (reinterpret_cast<const int*> (src), 1, dest, 1, (vDSP_Length) num);
    vDSP_vsmul (dest, 1, &multiplier, dest, 1, (vDSP_Length) num);
#elif YUP_USE_ARM_NEON
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] = (float) src[i] * multiplier,
                                 vmulq_n_f32 (vcvtq_f32_s32 (vld1q_s32 (src)), multiplier),
                                 YUP_LOAD_NONE,
                                 YUP_INCREMENT_SRC_DEST, )
#elif YUP_USE_SSE_INTRINSICS
    YUP_PERFORM_VEC_OP_SRC_DEST (dest[i] = (float) src[i] * multiplier,
                                 Mode::mul (mult, _mm_cvtepi32_ps (_mm_loadu_si128 (reinterpret_cast<const __m128i*> (src)))),
                                 YUP_LOAD_NONE,
                                 YUP_INCREMENT_SRC_DEST,
                                 const Mode::ParallelType mult = Mode::load1 (multiplier);)
#else
    for (Size i = 0; i < num; ++i)
        dest[i] = (float) src[i] * multiplier;
#endif
}

template <typename Size>
void convertFloatToFixed (int* dest, const float* src, float multiplier, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    constexpr Size kStackBufferSize = 256;
    float stackBuffer[kStackBufferSize];

    if (num <= kStackBufferSize)
    {
        vDSP_vsmul (src, 1, &multiplier, stackBuffer, 1, (vDSP_Length) num);
        vDSP_vfix32 (stackBuffer, 1, reinterpret_cast<int*> (dest), 1, (vDSP_Length) num);
    }
    else
    {
        for (Size i = 0; i < num; i += kStackBufferSize)
        {
            const Size currentChunk = jmin (kStackBufferSize, num - i);
            vDSP_vsmul (src + i, 1, &multiplier, stackBuffer, 1, (vDSP_Length) currentChunk);
            vDSP_vfix32 (stackBuffer, 1, reinterpret_cast<int*> (dest + i), 1, (vDSP_Length) currentChunk);
        }
    }
#elif YUP_USE_ARM_NEON
    const auto numLongs = num & ~3;

    if (numLongs != 0)
    {
        for (Size i = 0; i < numLongs; i += 4)
        {
            float32x4_t floatVec = vld1q_f32 (src + i);
            float32x4_t scaledVec = vmulq_n_f32 (floatVec, multiplier);
            int32x4_t intVec = vcvtq_s32_f32 (scaledVec);
            vst1q_s32 (dest + i, intVec);
        }
    }

    for (Size i = numLongs; i < num; ++i)
        dest[i] = (int) (src[i] * multiplier);
#elif YUP_USE_SSE_INTRINSICS
    const auto numLongs = num & ~3;
    const __m128 mult = _mm_set1_ps (multiplier);

    if (numLongs != 0)
    {
        for (Size i = 0; i < numLongs; i += 4)
        {
            __m128 floatVec = _mm_loadu_ps (src + i);
            __m128 scaledVec = _mm_mul_ps (floatVec, mult);
            __m128i intVec = _mm_cvtps_epi32 (scaledVec);
            _mm_storeu_si128 (reinterpret_cast<__m128i*> (dest + i), intVec);
        }
    }

    for (Size i = numLongs; i < num; ++i)
        dest[i] = (int) (src[i] * multiplier);
#else
    for (Size i = 0; i < num; ++i)
        dest[i] = (int) (src[i] * multiplier);
#endif
}

template <typename Size>
void convertFixedToFloat (double* dest, const int* src, double multiplier, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    vDSP_vflt32D (reinterpret_cast<const int*> (src), 1, dest, 1, (vDSP_Length) num);
    vDSP_vsmulD (dest, 1, &multiplier, dest, 1, (vDSP_Length) num);
#elif YUP_USE_ARM_NEON
    const auto numLongs = num & ~1;

    if (numLongs != 0)
    {
        for (Size i = 0; i < numLongs; i += 2)
        {
            int32x2_t intVec = vld1_s32 (src + i);
            int val0 = vget_lane_s32 (intVec, 0);
            int val1 = vget_lane_s32 (intVec, 1);
            dest[i] = (double) val0 * multiplier;
            dest[i + 1] = (double) val1 * multiplier;
        }
    }

    for (Size i = numLongs; i < num; ++i)
        dest[i] = (double) src[i] * multiplier;
#elif YUP_USE_SSE_INTRINSICS
    const auto numLongs = num & ~1;

    if (numLongs != 0)
    {
        for (Size i = 0; i < numLongs; i += 2)
        {
            __m128i intVec = _mm_loadl_epi64 (reinterpret_cast<const __m128i*> (src + i));
            int val0 = _mm_extract_epi32 (intVec, 0);
            int val1 = _mm_extract_epi32 (intVec, 1);
            dest[i] = (double) val0 * multiplier;
            dest[i + 1] = (double) val1 * multiplier;
        }
    }

    for (Size i = numLongs; i < num; ++i)
        dest[i] = (double) src[i] * multiplier;
#else
    for (Size i = 0; i < num; ++i)
        dest[i] = (double) src[i] * multiplier;

#endif
}

template <typename Size>
void convertFloatToFixed (int* dest, const double* src, double multiplier, Size num) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    constexpr Size kStackBufferSize = 256;
    double stackBuffer[kStackBufferSize];

    if (num <= kStackBufferSize)
    {
        vDSP_vsmulD (src, 1, &multiplier, stackBuffer, 1, (vDSP_Length) num);
        vDSP_vfix32D (stackBuffer, 1, reinterpret_cast<int*> (dest), 1, (vDSP_Length) num);
    }
    else
    {
        for (Size i = 0; i < num; i += kStackBufferSize)
        {
            const Size currentChunk = jmin (kStackBufferSize, num - i);
            vDSP_vsmulD (src + i, 1, &multiplier, stackBuffer, 1, (vDSP_Length) currentChunk);
            vDSP_vfix32D (stackBuffer, 1, reinterpret_cast<int*> (dest + i), 1, (vDSP_Length) currentChunk);
        }
    }
#elif YUP_USE_ARM_NEON
    const auto numLongs = num & ~1;

    if (numLongs != 0)
    {
        for (Size i = 0; i < numLongs; i += 2)
        {
            float64x2_t doubleVec = vld1q_f64 (src + i);
            float64x2_t scaledVec = vmulq_n_f64 (doubleVec, multiplier);
            double d0 = vgetq_lane_f64 (scaledVec, 0);
            double d1 = vgetq_lane_f64 (scaledVec, 1);
            dest[i] = (int) d0;
            dest[i + 1] = (int) d1;
        }
    }

    for (Size i = numLongs; i < num; ++i)
        dest[i] = (int) (src[i] * multiplier);
#elif YUP_USE_SSE_INTRINSICS
    const auto numLongs = num & ~1;
    const __m128d mult = _mm_set1_pd (multiplier);

    if (numLongs != 0)
    {
        for (Size i = 0; i < numLongs; i += 2)
        {
            __m128d doubleVec = _mm_loadu_pd (src + i);
            __m128d scaledVec = _mm_mul_pd (doubleVec, mult);
            __m128i intVec = _mm_cvtpd_epi32 (scaledVec);
            _mm_storel_epi64 (reinterpret_cast<__m128i*> (dest + i), intVec);
        }
    }

    for (Size i = numLongs; i < num; ++i)
        dest[i] = (int) (src[i] * multiplier);
#else
    for (Size i = 0; i < num; ++i)
        dest[i] = (int) (src[i] * multiplier);
#endif
}

} // namespace
} // namespace FloatVectorHelpers

//==============================================================================
template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::clear (FloatType* dest,
                                                                          CountType numValues) noexcept
{
    FloatVectorHelpers::clear (dest, numValues);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::fill (FloatType* dest,
                                                                         FloatType valueToFill,
                                                                         CountType numValues) noexcept
{
    FloatVectorHelpers::fill (dest, valueToFill, numValues);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::copy (FloatType* dest,
                                                                         const FloatType* src,
                                                                         CountType numValues) noexcept
{
    memcpy (dest, src, (size_t) numValues * sizeof (FloatType));
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::copyWithMultiply (FloatType* dest,
                                                                                     const FloatType* src,
                                                                                     FloatType multiplier,
                                                                                     CountType numValues) noexcept
{
    FloatVectorHelpers::copyWithMultiply (dest, src, multiplier, numValues);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::add (FloatType* dest,
                                                                        FloatType amountToAdd,
                                                                        CountType numValues) noexcept
{
    FloatVectorHelpers::add (dest, amountToAdd, numValues);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::add (FloatType* dest,
                                                                        const FloatType* src,
                                                                        FloatType amount,
                                                                        CountType numValues) noexcept
{
    FloatVectorHelpers::add (dest, src, amount, numValues);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::add (FloatType* dest,
                                                                        const FloatType* src,
                                                                        CountType numValues) noexcept
{
    FloatVectorHelpers::add (dest, src, numValues);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::add (FloatType* dest,
                                                                        const FloatType* src1,
                                                                        const FloatType* src2,
                                                                        CountType num) noexcept
{
    FloatVectorHelpers::add (dest, src1, src2, num);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::subtract (FloatType* dest,
                                                                             const FloatType* src,
                                                                             CountType numValues) noexcept
{
    FloatVectorHelpers::subtract (dest, src, numValues);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::subtract (FloatType* dest,
                                                                             const FloatType* src1,
                                                                             const FloatType* src2,
                                                                             CountType num) noexcept
{
    FloatVectorHelpers::subtract (dest, src1, src2, num);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::addWithMultiply (FloatType* dest,
                                                                                    const FloatType* src,
                                                                                    FloatType multiplier,
                                                                                    CountType numValues) noexcept
{
    FloatVectorHelpers::addWithMultiply (dest, src, multiplier, numValues);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::addWithMultiply (FloatType* dest,
                                                                                    const FloatType* src1,
                                                                                    const FloatType* src2,
                                                                                    CountType num) noexcept
{
    FloatVectorHelpers::addWithMultiply (dest, src1, src2, num);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::subtractWithMultiply (FloatType* dest,
                                                                                         const FloatType* src,
                                                                                         FloatType multiplier,
                                                                                         CountType numValues) noexcept
{
    FloatVectorHelpers::subtractWithMultiply (dest, src, multiplier, numValues);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::subtractWithMultiply (FloatType* dest,
                                                                                         const FloatType* src1,
                                                                                         const FloatType* src2,
                                                                                         CountType num) noexcept
{
    FloatVectorHelpers::subtractWithMultiply (dest, src1, src2, num);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::multiply (FloatType* dest,
                                                                             const FloatType* src,
                                                                             CountType numValues) noexcept
{
    FloatVectorHelpers::multiply (dest, src, numValues);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::multiply (FloatType* dest,
                                                                             const FloatType* src1,
                                                                             const FloatType* src2,
                                                                             CountType numValues) noexcept
{
    FloatVectorHelpers::multiply (dest, src1, src2, numValues);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::multiply (FloatType* dest,
                                                                             FloatType multiplier,
                                                                             CountType numValues) noexcept
{
    FloatVectorHelpers::multiply (dest, multiplier, numValues);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::multiply (FloatType* dest,
                                                                             const FloatType* src,
                                                                             FloatType multiplier,
                                                                             CountType num) noexcept
{
    FloatVectorHelpers::multiply (dest, src, multiplier, num);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::negate (FloatType* dest,
                                                                           const FloatType* src,
                                                                           CountType numValues) noexcept
{
    FloatVectorHelpers::negate (dest, src, numValues);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::abs (FloatType* dest,
                                                                        const FloatType* src,
                                                                        CountType numValues) noexcept
{
    FloatVectorHelpers::abs (dest, src, numValues);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::min (FloatType* dest,
                                                                        const FloatType* src,
                                                                        FloatType comp,
                                                                        CountType num) noexcept
{
    FloatVectorHelpers::min (dest, src, comp, num);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::min (FloatType* dest,
                                                                        const FloatType* src1,
                                                                        const FloatType* src2,
                                                                        CountType num) noexcept
{
    FloatVectorHelpers::min (dest, src1, src2, num);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::max (FloatType* dest,
                                                                        const FloatType* src,
                                                                        FloatType comp,
                                                                        CountType num) noexcept
{
    FloatVectorHelpers::max (dest, src, comp, num);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::max (FloatType* dest,
                                                                        const FloatType* src1,
                                                                        const FloatType* src2,
                                                                        CountType num) noexcept
{
    FloatVectorHelpers::max (dest, src1, src2, num);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::clip (FloatType* dest,
                                                                         const FloatType* src,
                                                                         FloatType low,
                                                                         FloatType high,
                                                                         CountType num) noexcept
{
    FloatVectorHelpers::clip (dest, src, low, high, num);
}

template <typename FloatType, typename CountType>
Range<FloatType> YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::findMinAndMax (const FloatType* src,
                                                                                              CountType numValues) noexcept
{
    return FloatVectorHelpers::findMinAndMax (src, numValues);
}

template <typename FloatType, typename CountType>
FloatType YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::findMinimum (const FloatType* src,
                                                                                     CountType numValues) noexcept
{
    return FloatVectorHelpers::findMinimum (src, numValues);
}

template <typename FloatType, typename CountType>
FloatType YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::findMaximum (const FloatType* src,
                                                                                     CountType numValues) noexcept
{
    return FloatVectorHelpers::findMaximum (src, numValues);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::convertFixedToFloat (FloatType* dest,
                                                                                        const int* src,
                                                                                        FloatType multiplier,
                                                                                        CountType numValues) noexcept
{
    FloatVectorHelpers::convertFixedToFloat (dest, src, multiplier, numValues);
}

template <typename FloatType, typename CountType>
void YUP_CALLTYPE FloatVectorOperationsBase<FloatType, CountType>::convertFloatToFixed (int* dest,
                                                                                        const FloatType* src,
                                                                                        FloatType multiplier,
                                                                                        CountType numValues) noexcept
{
    FloatVectorHelpers::convertFloatToFixed (dest, src, multiplier, numValues);
}

//==============================================================================

template struct FloatVectorOperationsBase<float, int>;
template struct FloatVectorOperationsBase<float, size_t>;
template struct FloatVectorOperationsBase<double, int>;
template struct FloatVectorOperationsBase<double, size_t>;

//==============================================================================

intptr_t YUP_CALLTYPE FloatVectorOperations::getFpStatusRegister() noexcept
{
    intptr_t fpsr = 0;
#if YUP_INTEL && YUP_USE_SSE_INTRINSICS
    fpsr = static_cast<intptr_t> (_mm_getcsr());
#elif (YUP_64BIT && YUP_ARM) || YUP_USE_ARM_NEON
#if _MSC_VER
    // _control87 returns static values for x86 bits that don't exist on arm
    // to emulate x86 behaviour. We are only ever interested in de-normal bits
    // so mask out only those.
    fpsr = (intptr_t) (_control87 (0, 0) & _MCW_DN);
#else
#if YUP_64BIT
    asm volatile ("mrs %0, fpcr"
                  : "=r"(fpsr));
#elif YUP_USE_ARM_NEON
    asm volatile ("vmrs %0, fpscr"
                  : "=r"(fpsr));
#endif
#endif
#else
#if ! (defined(YUP_INTEL) || defined(YUP_ARM))
    jassertfalse; // No support for getting the floating point status register for your platform
#endif
#endif

    return fpsr;
}

void YUP_CALLTYPE FloatVectorOperations::setFpStatusRegister ([[maybe_unused]] intptr_t fpsr) noexcept
{
#if YUP_INTEL && YUP_USE_SSE_INTRINSICS
    // the volatile keyword here is needed to workaround a bug in AppleClang 13.0
    // which aggressively optimises away the variable otherwise
    volatile auto fpsr_w = static_cast<uint32_t> (fpsr);
    _mm_setcsr (fpsr_w);
#elif (YUP_64BIT && YUP_ARM) || YUP_USE_ARM_NEON
#if _MSC_VER
    _control87 ((unsigned int) fpsr, _MCW_DN);
#else
#if YUP_64BIT
    asm volatile ("msr fpcr, %0"
                  :
                  : "ri"(fpsr));
#elif YUP_USE_ARM_NEON
    asm volatile ("vmsr fpscr, %0"
                  :
                  : "ri"(fpsr));
#endif
#endif
#else
#if ! (defined(YUP_INTEL) || defined(YUP_ARM))
    jassertfalse; // No support for getting the floating point status register for your platform
#endif
#endif
}

void YUP_CALLTYPE FloatVectorOperations::enableFlushToZeroMode ([[maybe_unused]] bool shouldEnable) noexcept
{
#if YUP_USE_SSE_INTRINSICS || (YUP_USE_ARM_NEON || (YUP_64BIT && YUP_ARM))
#if YUP_USE_SSE_INTRINSICS
    intptr_t mask = _MM_FLUSH_ZERO_MASK;
#else /*YUP_USE_ARM_NEON*/
    intptr_t mask = (1 << 24 /* FZ */);
#endif
    setFpStatusRegister ((getFpStatusRegister() & (~mask)) | (shouldEnable ? mask : 0));
#else
#if ! (defined(YUP_INTEL) || defined(YUP_ARM))
    jassertfalse; // No support for flush to zero mode on your platform
#endif
#endif
}

void YUP_CALLTYPE FloatVectorOperations::disableDenormalisedNumberSupport ([[maybe_unused]] bool shouldDisable) noexcept
{
#if YUP_USE_SSE_INTRINSICS || (YUP_USE_ARM_NEON || (YUP_64BIT && YUP_ARM))
#if YUP_USE_SSE_INTRINSICS
    intptr_t mask = 0x8040;
#else /*YUP_USE_ARM_NEON*/
    intptr_t mask = (1 << 24 /* FZ */);
#endif

    setFpStatusRegister ((getFpStatusRegister() & (~mask)) | (shouldDisable ? mask : 0));
#else

#if ! (defined(YUP_INTEL) || defined(YUP_ARM))
    jassertfalse; // No support for disable denormals mode on your platform
#endif
#endif
}

bool YUP_CALLTYPE FloatVectorOperations::areDenormalsDisabled() noexcept
{
#if YUP_USE_SSE_INTRINSICS || (YUP_USE_ARM_NEON || (YUP_64BIT && YUP_ARM))
#if YUP_USE_SSE_INTRINSICS
    intptr_t mask = 0x8040;
#else /*YUP_USE_ARM_NEON*/
    intptr_t mask = (1 << 24 /* FZ */);
#endif

    return ((getFpStatusRegister() & mask) == mask);
#else
    return false;
#endif
}

//==============================================================================

ScopedNoDenormals::ScopedNoDenormals() noexcept
{
#if YUP_USE_SSE_INTRINSICS || (YUP_USE_ARM_NEON || (YUP_64BIT && YUP_ARM))
#if YUP_USE_SSE_INTRINSICS
    intptr_t mask = 0x8040;
#else /*YUP_USE_ARM_NEON*/
    intptr_t mask = (1 << 24 /* FZ */);
#endif

    fpsr = FloatVectorOperations::getFpStatusRegister();
    FloatVectorOperations::setFpStatusRegister (fpsr | mask);
#endif
}

ScopedNoDenormals::~ScopedNoDenormals() noexcept
{
#if YUP_USE_SSE_INTRINSICS || (YUP_USE_ARM_NEON || (YUP_64BIT && YUP_ARM))
    FloatVectorOperations::setFpStatusRegister (fpsr);
#endif
}

} // namespace yup
