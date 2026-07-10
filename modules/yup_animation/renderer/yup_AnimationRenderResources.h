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

namespace yup
{

//==============================================================================
/** Persistent GPU resources reused across animation render calls.

    Holds the compiled fullscreen matte-composite pipeline used by
    AnimationRenderer to blend track mattes on the GPU. Callers that render the
    same animation every frame (e.g. AnimationPlayer) should own an
    AnimationRenderResources and pass it to the render call so the matte shader
    is compiled once instead of per frame.

    An AnimationRenderResources holds GPU handles tied to a specific
    GraphicsContext and MUST be destroyed before that GraphicsContext. Never
    store one with static lifetime: its GPU handles would then be released at
    process exit, after the GPU context is already gone.

    @see AnimationRenderer, AnimationPlayer
*/
class YUP_API AnimationRenderResources
{
public:
    //==============================================================================
    /** Creates an empty resource set. The matte pipeline is compiled lazily on
        first use against the GraphicsContext of the render call. */
    AnimationRenderResources() = default;

    //==============================================================================
    /** Returns the matte-composite pipeline, compiling it against @p context on
        first use. Returns nullptr if the context has no GPU or compilation fails
        (callers then fall back to the geometric-clip matte path). */
    GpuPipeline::Ptr getMattePipeline (GraphicsContext& context);

    /** Releases all cached GPU resources. Safe to call while the owning
        GraphicsContext is still alive. */
    void reset();

private:
    GpuPipeline::Ptr mattePipeline_;
    bool mattePipelineCompiled_ = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimationRenderResources)
};

} // namespace yup
