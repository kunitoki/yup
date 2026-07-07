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

class GraphicsContext;

//==============================================================================
/** RAII scope for a single GPU frame.

    GpuFrame wraps the ore begin/submit/wait frame lifecycle. Begin a frame with
    GpuFrame::begin(), encode one or more render passes into it (via
    GpuCanvas::beginRenderPass()), then submit() the recorded work. The frame
    owns the transient GPU resources (uniform buffers, texture views, samplers)
    created while encoding its passes, keeping them alive until submission
    completes or the frame is destroyed.

    The type is move-only stack RAII: the destructor submits the frame if it has
    not already been submitted.

    @code
        auto frame = GpuFrame::begin (ctx);
        auto pass  = canvas->beginRenderPass (frame, { true, bg });
        pass.setPipeline (*pipeline);
        pass.draw (3);
        pass.finish();
        frame.submit();
    @endcode

    Requires the GraphicsContext to have been created with
    Options::enableOreContext = true.

    @see GpuCanvas, GpuRenderPass, GpuPipeline
*/
class YUP_API GpuFrame
{
public:
    //==============================================================================
    /** Begins a GPU frame on the given context.

        Returns an invalid frame (isValid() == false) if the context has no ore
        GPU context (enableOreContext = false or ore unavailable on this backend).
    */
    static GpuFrame begin (GraphicsContext& ctx);

    //==============================================================================
    /** Move constructor. */
    GpuFrame (GpuFrame&&) noexcept;

    /** Move assignment operator. */
    GpuFrame& operator= (GpuFrame&&) noexcept;

    /** Destructor. Submits the frame if not already submitted. */
    ~GpuFrame();

    //==============================================================================
    /** Returns true if this frame holds a valid ore GPU context. */
    bool isValid() const noexcept;

    /** Submits all render passes recorded since begin().

        Idempotent: a second call is a no-op and returns false. Does not block
        the CPU - call waitForGPU() afterwards if you need results immediately.

        @return true on success; false if invalid or already submitted.
    */
    bool submit();

    /** Blocks the calling thread until all submitted GPU work has completed.

        Also releases the transient resources held for this frame.
    */
    void waitForGPU();

private:
    friend class GpuCanvas;
    friend class GpuRenderPass;

    GpuFrame() = default;

    struct Impl;
    std::unique_ptr<Impl> impl;

    Impl* getImpl() noexcept { return impl.get(); }

    YUP_DECLARE_NON_COPYABLE (GpuFrame)
};

} // namespace yup
