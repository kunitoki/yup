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
class GpuProgram;
class Graphics;

//==============================================================================
/** An opaque GPU texture representing rendered GPU content.

    A Texture is the currency that connects GpuCanvas output to Image/Graphics
    drawing. Obtain one via GpuCanvas::asTexture() or Image::fromTexture().

    Texture is reference-counted; keep it alive as long as you need to draw from it.
    The underlying GPU resource lives for as long as at least one Texture::Ptr exists.

    @see GpuCanvas, Image::fromTexture, Graphics::drawTexture
*/
class YUP_API Texture : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<Texture>;

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
    friend class GpuProgram;
    friend class Graphics;

    Texture() = default;

    /** Creates a Texture from a raw GPU texture of known dimensions. */
    static Texture::Ptr fromGpuTexture (rive::rcp<rive::gpu::Texture> texture, int width, int height);

    /** Creates a Texture from a render canvas of known dimensions. */
    static Texture::Ptr fromRenderCanvas (rive::rcp<rive::gpu::RenderCanvas> canvas, int width, int height);

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

    mutable rive::rcp<rive::gpu::Texture> gpuTexture;
    rive::rcp<rive::gpu::RenderCanvas> renderCanvas;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Texture)
};

} // namespace yup
