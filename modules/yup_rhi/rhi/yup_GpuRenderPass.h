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
class GpuSampler;
class GpuBuffer;
class GpuCanvas;
class GpuFrame;

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
    void setPipeline (GpuPipeline::Ptr pipeline);

    /** Binds a texture to the given (group, binding) slot.

        The texture may come from GpuCanvas::asTexture() or Image::getGpuTexture().
        If the same slot is set more than once the later call wins.

        @param group    The uniform group index (0..N).
        @param binding  The uniform binding index (0..N).
        @param texture  The GpuTexture to bind.
    */
    void setTexture (int group, int binding, GpuTexture::Ptr texture);

    /** Binds a sampler to the given (group, binding) slot.

        Sampler slots the caller never sets keep the pipeline's default linear /
        clamp-to-edge sampler, so this only has to be called for slots that need
        different filtering, wrapping, LOD clamping or anisotropy. If the same
        slot is set more than once the later call wins.

        @param group    The uniform group index (0..N).
        @param binding  The uniform binding index (0..N).
        @param sampler  The GpuSampler to bind.
    */
    void setSampler (int group, int binding, GpuSampler::Ptr sampler);

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
    /** Adds a colour attachment beyond the one the pass was begun on (MRT).

        Attachment 0 is always the surface beginRenderPass() was called on, so
        @p index must be 1..3. The pipeline's GpuPipelineOptions::colorTargetCount
        and per-target formats must match the attachments bound here, or the draw
        is rejected by the backend.

        @param index    Attachment index, 1..3.
        @param texture  The texture to render into. Must be a render target.
        @param options  Load / store behaviour and clear color for this attachment.
        @param view     Which mip level and layer of @p texture to render into.
    */
    void setColorAttachment (int index,
                             GpuTexture::Ptr texture,
                             const GpuRenderOptions& options = {},
                             const GpuTextureViewDesc& view = {});

    /** Sets the depth/stencil attachment used by every draw in this pass.

        Required whenever the bound pipeline declares
        GpuPipelineOptions::depthStencil.enabled, and the texture's format must
        match GpuDepthStencilState::format.

        The attachment is cleared once, before the first draw of the pass; later
        draws load it, so depth testing works across draws.

        @param texture  A render-target texture with a depth/stencil format.
        @param options  Depth and stencil load / store behaviour and clear values.
        @param view     Which mip level and layer of @p texture to render into.
    */
    void setDepthStencilAttachment (GpuTexture::Ptr texture,
                                    const GpuDepthStencilOptions& options = {},
                                    const GpuTextureViewDesc& view = {});

    /** Sets the MSAA resolve destination for a colour attachment.

        Only meaningful when the attachment itself was created with a sample count
        above 1. The resolve target must be single-sampled and of the same format
        and size.

        @param index    Attachment index, 0..3.
        @param texture  The single-sampled texture to resolve into.
        @param view     Which mip level and layer of @p texture to resolve into.
    */
    void setResolveTarget (int index, GpuTexture::Ptr texture, const GpuTextureViewDesc& view = {});

    //==============================================================================
    /** Restricts rendering to a sub-rectangle of the attachments.

        Defaults to the full extent of the surface the pass was begun on. The
        setting is sticky and applies to every subsequent draw.

        @param x         Left edge in pixels.
        @param y         Top edge in pixels.
        @param width     Width in pixels.
        @param height    Height in pixels.
        @param minDepth  Near depth range value.
        @param maxDepth  Far depth range value.
    */
    void setViewport (float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);

    /** Discards fragments outside the given rectangle.

        Disabled by default. The setting is sticky and applies to every subsequent
        draw.

        @param x       Left edge in pixels.
        @param y       Top edge in pixels.
        @param width   Width in pixels.
        @param height  Height in pixels.
    */
    void setScissorRect (int x, int y, int width, int height);

    /** Sets the reference value compared against by the stencil test.

        The setting is sticky and applies to every subsequent draw.

        @param reference  The stencil reference value.
    */
    void setStencilReference (uint32_t reference);

    /** Sets the constant color used by the blendColor / oneMinusBlendColor blend factors.

        The setting is sticky and applies to every subsequent draw.

        @param color  The constant blend color.
    */
    void setBlendColor (GpuColor color);

    //==============================================================================
    /** Encodes a non-indexed draw of @c vertexCount vertices.

        For fullscreen passes that generate vertices from the vertex index, pass
        vertexCount = 3 with no vertex buffers bound.

        The attachments are cleared (if their load op says so) before the first
        draw only; subsequent draws in the same pass load the existing contents,
        so several draws can accumulate into one surface and share one depth buffer.

        @param vertexCount    Number of vertices to draw. The vertex buffers must hold at least this many vertices.
        @param instanceCount  Number of instances to draw. Per-instance vertex buffers must hold at least this many elements.
        @param firstVertex    Index of the first vertex to draw.
        @param firstInstance  Index of the first instance to draw.

        @returns true on success; false if invalid.
    */
    bool draw (uint32_t vertexCount,
               uint32_t instanceCount = 1,
               uint32_t firstVertex = 0,
               uint32_t firstInstance = 0);

    /** Encodes an indexed draw of @c indexCount indices.

        Binds the vertex buffers and index buffer set via setVertexBuffer() /
        setIndexBuffer().

        @param indexCount     Number of indices to draw. The index buffer must hold at least this many indices.
        @param instanceCount  Number of instances to draw.
        @param firstIndex     Index of the first element to read from the index buffer.
        @param baseVertex     Value added to every index before indexing the vertex buffers.
        @param firstInstance  Index of the first instance to draw.

        @returns true on success; false if invalid or no index buffer is bound.
    */
    bool drawIndexed (uint32_t indexCount,
                      uint32_t instanceCount = 1,
                      uint32_t firstIndex = 0,
                      int32_t baseVertex = 0,
                      uint32_t firstInstance = 0);

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

    static constexpr size_t ImplSizeBytes = 1024;
    TypeErasedObject<ImplSizeBytes> impl;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuRenderPass)
};

} // namespace yup
