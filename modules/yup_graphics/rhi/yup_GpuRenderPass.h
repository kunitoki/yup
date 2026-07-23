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
    /** Default constructor. */
    constexpr GpuRenderOptions() = default;

    /** Constructs a GpuRenderOptions with the given clear flag and clear color. */
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
    /** Sets the compiled pipeline used by subsequent draws.
    
        @param pipeline  The GpuPipeline to use.
    */
    void setPipeline (GpuPipeline& pipeline);

    /** Binds a texture to the given (group, binding) slot.

        The texture may come from GpuCanvas::asTexture() or Image::getGpuTexture().
        If the same slot is set more than once the later call wins.

        @param group    The uniform group index (0..N).
        @param binding  The uniform binding index (0..N).
        @param texture  The GpuTexture to bind.
    */
    void setTexture (int group, int binding, GpuTexture::Ptr texture);

    /** Uploads raw uniform data to the given (group, binding) slot.

        The data is copied immediately; the caller need not keep it alive.
        If the same slot is set more than once the later call wins.

        @param group     The uniform group index (0..N).
        @param binding   The uniform binding index (0..N).
        @param data      Pointer to the source data to upload (must be non-null).
        @param byteSize  Number of bytes to upload (must be greater than zero).
    */
    void setUniformBuffer (int group, int binding, const void* data, size_t byteSize);

    /** Binds a vertex buffer to the given slot for custom geometry rendering.
    
        The buffer must hold at least as many vertices as will be drawn.

        @param slot    The vertex buffer slot to bind to (0..N).
        @param buffer  The GpuBuffer holding the vertex data.
    */
    void setVertexBuffer (int slot, GpuBuffer::Ptr buffer);

    /** Binds an index buffer for indexed geometry rendering (used by drawIndexed()).
    
        The buffer must hold at least as many indices as will be drawn.

        @param format  The index format (16-bit or 32-bit).
        @param buffer  The GpuBuffer holding the index data.
    */
    void setIndexBuffer (GpuIndexFormat format, GpuBuffer::Ptr buffer);

    //==============================================================================
    /** Encodes a non-indexed draw of @c vertexCount vertices.

        For fullscreen passes that generate vertices from the vertex index, pass
        vertexCount = 3 with no vertex buffers bound.

        @param vertexCount  Number of vertices to draw. The vertex buffers must hold at least this many vertices.

        @returns true on success; false if invalid.
    */
    bool draw (uint32_t vertexCount);

    /** Encodes an indexed draw of @c indexCount indices.

        Binds the vertex buffers and index buffer set via setVertexBuffer() /
        setIndexBuffer().

        @param indexCount  Number of indices to draw. The index buffer must hold at least this many indices.

        @returns true on success; false if invalid or no index buffer is bound.
    */
    bool drawIndexed (uint32_t indexCount);

    //==============================================================================
    /** Encodes all recorded draws and closes the render pass.

        Idempotent: a second call is a no-op and returns false.

        @returns true on success; false if invalid or already finished.
    */
    bool finish();

private:
    friend class GpuCanvas;
    friend class GpuTarget;

    GpuRenderPass() = default;

    struct Impl;
    Impl* getImpl() noexcept;
    const Impl* getImpl() const noexcept;

    static constexpr size_t ImplSizeBytes = 256;
    TypeErasedObject<ImplSizeBytes> impl;

    YUP_DECLARE_NON_COPYABLE (GpuRenderPass)
};

} // namespace yup
