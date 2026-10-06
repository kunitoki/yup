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
/** Services the builtin lowering needs from the statement lowering that drives it. */
class WgslBuiltinHost
{
public:
    virtual ~WgslBuiltinHost() = default;

    /** Evaluates e once into a temporary declared in pre, unless it is already safe to repeat. */
    virtual Expr spill (Expr e, std::vector<Statement>& pre) = 0;

    /** Adds a helper function to the output, once per name. */
    virtual void requirePolyfill (const std::string& name, const std::string& code) = 0;

    virtual ShaderStage getStage() const = 0;

    /** A fresh identifier for a temporary. */
    virtual std::string allocateName (const std::string& base) = 0;

    /** True for separate textures that WGSL declares as depth textures. */
    virtual bool isDepthTexture (const Expr& texture) const = 0;
};

//==============================================================================
/**
    Rewrites calls to GLSL builtin functions and GLSL constructors into WGSL.

    Arguments must already be lowered and typed. Combined image samplers arrive as
    sampler2D(texture, sampler) pairs.
*/
class WgslBuiltins
{
public:
    /** Returns the WGSL replacement of a GLSL builtin call, throwing LoweringError when WGSL has no equivalent. */
    static Expr lowerCall (Expr call, std::vector<Statement>& pre, WgslBuiltinHost& host);

    /** Returns the WGSL replacement of a GLSL type constructor, which is often more permissive than WGSL's. */
    static Expr lowerConstructor (Expr constructor, std::vector<Statement>& pre, WgslBuiltinHost& host);
};

} // namespace wgsl
} // namespace yup
