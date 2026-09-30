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
/** Hands out identifiers that collide neither with user code nor with each other. */
class WgslNameAllocator
{
public:
    /** Marks a name as taken. */
    void reserve (const std::string& name) { used.insert (name); }

    bool isTaken (const std::string& name) const { return used.count (name) > 0; }

    /** Returns base itself if free, otherwise base_1, base_2, ... and reserves the result. */
    std::string allocate (const std::string& base)
    {
        auto candidate = base;

        for (int suffix = 1; used.count (candidate) > 0; ++suffix)
            candidate = base + "_" + std::to_string (suffix);

        used.insert (candidate);
        return candidate;
    }

private:
    std::unordered_set<std::string> used;
};

//==============================================================================
/** State shared by the lowering passes of one translation unit. */
struct WgslLoweringContext
{
    ShaderStage stage = ShaderStage::vertex;
    WgslNameAllocator names;

    /** Companion sampler variable of each global combined image sampler. */
    std::map<std::string, std::string> samplerCompanions;

    /** Separate textures sampled through a shadow sampler constructor, which WGSL declares as depth textures. */
    std::set<std::string> depthTextures;

    std::string innerFunction;
    std::vector<std::string> warnings;
    std::vector<std::string> polyfills;
    std::set<std::string> polyfillNames;
};

//==============================================================================
/**
    Rewrites function bodies into statements and expressions WGSL can express.

    Runs after WgslTypeLegalizer, relying on the types it recorded on every expression.
*/
class WgslStatementLowering
{
public:
    /** Lowers every function of the program in place, throwing LoweringError on unsupported constructs. */
    static void lower (TranslationUnit& ast, WgslLoweringContext& context);
};

} // namespace wgsl
} // namespace yup
