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

    Holds the compiled fullscreen matte-composite pipeline, reusable
    target/source/result canvas triples for track mattes, and reusable precomp
    canvases. Callers that render the same animation every frame (e.g.
    AnimationPlayer) should own an AnimationRenderResources and pass it to the
    render call so shader compilation and canvas texture allocation occur once.

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
    /** Owns one target/source/result canvas triple while a matte is rendered.

        The lease returns its canvas triple to the resource pool on destruction.
        It is move-only so recursive mattes reserve distinct triples while
        sequential frames reuse the same GPU allocations.
    */
    class YUP_API MatteCanvasLease
    {
    public:
        MatteCanvasLease() = default;
        MatteCanvasLease (MatteCanvasLease&& other) noexcept;
        MatteCanvasLease& operator= (MatteCanvasLease&& other) noexcept;
        MatteCanvasLease (const MatteCanvasLease&) = delete;
        MatteCanvasLease& operator= (const MatteCanvasLease&) = delete;
        ~MatteCanvasLease();

        /** Returns true when all three canvases were acquired successfully. */
        bool isValid() const noexcept;

        /** Returns the canvas used to render the matted layer. */
        GpuCanvas& getTargetCanvas() const noexcept;

        /** Returns the canvas used to render the matte source. */
        GpuCanvas& getSourceCanvas() const noexcept;

        /** Returns the canvas used to render the composited result. */
        GpuCanvas& getResultCanvas() const noexcept;

    private:
        friend class AnimationRenderResources;

        MatteCanvasLease (AnimationRenderResources& owner, size_t slotIndex) noexcept;
        void release() noexcept;

        AnimationRenderResources* owner_ = nullptr;
        size_t slotIndex_ = 0;
    };

    //==============================================================================
    /** Returns the matte-composite pipeline, compiling it against @p context on
        first use. Returns nullptr if the context has no GPU or compilation fails
        (callers then fall back to the geometric-clip matte path). */
    GpuPipeline::Ptr getMattePipeline (GraphicsContext& context);

    /** Acquires three persistent canvases for a GPU matte at the specified size.

        The returned lease remains valid until destruction. If a nested matte is
        active, a separate canvas triple is allocated. Resources are tied to the
        supplied GraphicsContext; using another context resets the pool.
    */
    MatteCanvasLease acquireMatteCanvases (GraphicsContext& context, int width, int height);

    /** Returns a persistent canvas for rendering a precomposition.

        @p key identifies the precomposition asset. A canvas is recreated only
        when the asset's required pixel size changes. Resources are tied to the
        supplied GraphicsContext; using another context resets the pool.
    */
    GpuCanvas::Ptr getPrecompCanvas (GraphicsContext& context, const String& key, int width, int height);

    /** Releases all cached GPU resources. Safe to call while the owning
        GraphicsContext is still alive. */
    void reset();

private:
    struct MatteCanvasSlot
    {
        GpuCanvas::Ptr targetCanvas;
        GpuCanvas::Ptr sourceCanvas;
        GpuCanvas::Ptr resultCanvas;
        int width = 0;
        int height = 0;
        bool inUse = false;
    };

    struct PrecompCanvasSlot
    {
        String key;
        GpuCanvas::Ptr canvas;
        int width = 0;
        int height = 0;
    };

    void releaseMatteCanvasSlot (size_t slotIndex) noexcept;

    GpuPipeline::Ptr mattePipeline_;
    bool mattePipelineCompiled_ = false;
    GraphicsContext* matteCanvasContext_ = nullptr;
    std::vector<MatteCanvasSlot> matteCanvasPool_;
    std::vector<PrecompCanvasSlot> precompCanvasPool_;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimationRenderResources)
};

} // namespace yup
