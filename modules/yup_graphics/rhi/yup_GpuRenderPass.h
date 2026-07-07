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

class GpuPipeline;
class GpuTexture;
class GpuBuffer;
class GpuCanvas;
class GpuFrame;

//==============================================================================
/** Per-render-pass options controlling attachment load behaviour. */
struct GpuRenderOptions
{
    constexpr GpuRenderOptions() = default;

    constexpr GpuRenderOptions (bool clear, Color clearColor)
        : clear (clear)
        , clearColor (clearColor)
    {
    }

    /** Whether to clear the target before drawing (LoadOp::clear). When false
        the existing contents are loaded (LoadOp::load). */
    bool clear = true;

    /** Clear color used when @c clear is true. */
    Color clearColor = Colors::transparentBlack;
};

//==============================================================================
/** A transient render-pass encoder targeting a GpuCanvas.

    A GpuRenderPass records draw commands into a single ore render pass that
    outputs to a GpuCanvas's backing texture. Obtain one from
    GpuCanvas::beginRenderPass(); bind a pipeline and resources, issue draws,
    then finish() the pass. The type is move-only stack RAII: the destructor
    finishes the pass if it has not already been finished.

    Binding state (pipeline, textures, uniform buffers, vertex/index buffers)
    lives entirely on the render pass, so a single immutable GpuPipeline can be
    reused across many passes with different bindings.

    @code
        auto pass = canvas->beginRenderPass (frame, { true, bg });
        pass.setPipeline (*pipeline);
        pass.setUniformBuffer (0, 0, &u, sizeof u);
        pass.setVertexBuffer (0, vbo);
        pass.setIndexBuffer (GpuIndexFormat::uint16, ibo);
        pass.drawIndexed (indexCount);
        pass.finish();
    @endcode

    @see GpuCanvas, GpuFrame, GpuPipeline, GpuBuffer, GpuTexture
*/
class YUP_API GpuRenderPass
{
public:
    //==============================================================================
    /** Move constructor. */
    GpuRenderPass (GpuRenderPass&&) noexcept;

    /** Move assignment operator. */
    GpuRenderPass& operator= (GpuRenderPass&&) noexcept;

    /** Destructor. Finishes the pass if not already finished. */
    ~GpuRenderPass();

    //==============================================================================
    /** Returns true if this pass holds a valid encoding target. */
    bool isValid() const noexcept;

    //==============================================================================
    /** Sets the compiled pipeline used by subsequent draws. */
    void setPipeline (GpuPipeline& pipeline);

    /** Binds a texture to the given (group, binding) slot.

        The texture may come from GpuCanvas::asTexture() or Image::getGpuTexture().
        If the same slot is set more than once the later call wins.
    */
    void setTexture (int group, int binding, GpuTexture::Ptr texture);

    /** Uploads raw uniform data to the given (group, binding) slot.

        The data is copied immediately; the caller need not keep it alive.
        If the same slot is set more than once the later call wins.
    */
    void setUniformBuffer (int group, int binding, const void* data, size_t byteSize);

    /** Binds a vertex buffer to the given slot for custom geometry rendering. */
    void setVertexBuffer (int slot, GpuBuffer::Ptr buffer);

    /** Binds an index buffer for indexed geometry rendering (used by drawIndexed()). */
    void setIndexBuffer (GpuIndexFormat format, GpuBuffer::Ptr buffer);

    //==============================================================================
    /** Encodes a non-indexed draw of @c vertexCount vertices.

        For fullscreen passes that generate vertices from the vertex index, pass
        vertexCount = 3 with no vertex buffers bound.

        @return true on success; false if invalid.
    */
    bool draw (uint32_t vertexCount);

    /** Encodes an indexed draw of @c indexCount indices.

        Binds the vertex buffers and index buffer set via setVertexBuffer() /
        setIndexBuffer().

        @return true on success; false if invalid or no index buffer is bound.
    */
    bool drawIndexed (uint32_t indexCount);

    //==============================================================================
    /** Encodes all recorded draws and closes the ore render pass.

        Idempotent: a second call is a no-op and returns false.

        @return true on success; false if invalid or already finished.
    */
    bool finish();

private:
    friend class GpuCanvas;

    GpuRenderPass() = default;

    struct Impl;
    std::unique_ptr<Impl> impl;

    YUP_DECLARE_NON_COPYABLE (GpuRenderPass)
};

} // namespace yup
