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

class Image;
class GpuCanvas;
class GpuDevice;
class GpuTarget;
class GpuRenderPass;
class Graphics;

//==============================================================================
/** An opaque GPU texture representing rendered GPU content.

    A Texture is the currency that connects GpuCanvas output to Image/Graphics
    drawing. Obtain one via GpuCanvas::asTexture() or Image::fromTexture().

    Texture is reference-counted; keep it alive as long as you need to draw from it.
    The underlying GPU resource lives for as long as at least one Texture::Ptr exists.

    @see GpuCanvas, Image::fromTexture, Graphics::drawTexture
*/
class YUP_API GpuTexture : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<GpuTexture>;

    /** Destructor. Releases the GPU resources with the rendering context current. */
    ~GpuTexture() override;

    //==============================================================================
    /** Returns the width of the texture in pixels. */
    int getWidth() const noexcept;

    /** Returns the height of the texture in pixels. */
    int getHeight() const noexcept;

    /** Returns true if this texture holds valid GPU resources. */
    bool isValid() const noexcept;

    /** Returns true if this texture was produced by a render pass (GpuCanvas). */
    bool isRenderTarget() const noexcept;

private:
    //==============================================================================
    friend class Image;
    friend class GpuCanvas;
    friend class GpuTarget;
    friend class GpuRenderPass;
    friend class Graphics;

    GpuTexture() = default;

    /** Creates a GpuTexture from a raw GPU texture of known dimensions. The device,
        when given, is used to release the GPU resources with its rendering context
        current on destruction. */
    static GpuTexture::Ptr fromGpuTexture (ReferenceCountedObjectPtr<GpuDevice> device, rive::rcp<rive::gpu::Texture> texture, int width, int height);

    /** Creates a GpuTexture from a render canvas of known dimensions. The device,
        when given, is used to release the GPU resources with its rendering context
        current on destruction. */
    static GpuTexture::Ptr fromRenderCanvas (ReferenceCountedObjectPtr<GpuDevice> device, rive::rcp<rive::gpu::RenderCanvas> canvas, int width, int height);

    /** Returns the raw GPU texture, extracting it from the render canvas if needed. */
    rive::rcp<rive::gpu::Texture> getOrAdoptGpuTexture() const;

    /** Returns the render canvas, or nullptr if this texture is not canvas-backed. */
    rive::rcp<rive::gpu::RenderCanvas> getInternalRenderCanvas() const;

    /** Returns the RenderImage from the render canvas, or nullptr. */
    rive::RenderImage* getRenderImage() const;

    //==============================================================================
    int width = 0;
    int height = 0;
    bool renderTarget = false;

    ReferenceCountedObjectPtr<GpuDevice> device;
    mutable rive::rcp<rive::gpu::Texture> gpuTexture;
    rive::rcp<rive::gpu::RenderCanvas> renderCanvas;
    rive::rcp<rive::gpu::Texture> sampledTexture; // GL Y-flipped mirror; null on non-GL backends

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuTexture)
};

} // namespace yup
