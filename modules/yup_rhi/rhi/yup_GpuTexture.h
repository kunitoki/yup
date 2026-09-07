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
/** An opaque GPU texture, either allocated directly or wrapping rendered content.

    A GpuTexture is the currency that connects GpuCanvas output to Image/Graphics
    drawing. Obtain one by allocating it with create(), or from an existing
    surface via GpuCanvas::asTexture() / GpuTarget::asTexture() /
    Image::getGpuTexture().

    Directly allocated textures cover the shapes ore supports - 2D, cube, 3D and
    2D-array - with mip levels, MSAA sample counts and the full format list, and
    can be filled either from CPU memory with upload() or by rendering into them
    through GpuTarget::createFromTexture().

    GpuTexture is reference-counted; keep it alive as long as you need to draw
    from it. The underlying GPU resource lives for as long as at least one
    GpuTexture::Ptr exists.

    @code
        yup::GpuTextureDesc desc;
        desc.width = desc.height = 128;
        desc.format = yup::GpuTextureFormat::rgba16float;
        desc.type = yup::GpuTextureType::cube;
        desc.depthOrArrayLayers = 6;
        desc.mipLevels = 5;
        desc.renderTarget = true;

        auto cube = yup::GpuTexture::create (device, desc);
    @endcode

    @see GpuTextureDesc, GpuTarget, GpuCanvas, Image::fromTexture, Graphics::drawTexture
*/
class YUP_API GpuTexture : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<GpuTexture>;

    //==============================================================================
    /** Allocates a new GPU texture.

        Storage is allocated for every mip level and layer the descriptor asks
        for, but nothing is written to it - fill the levels with upload(), or by
        rendering into them via GpuTarget::createFromTexture().

        @param device  A GpuDevice with a GPU context available.
        @param desc    The texture to allocate.

        @returns A GpuTexture, or nullptr if the descriptor is invalid or the
                 backend could not allocate it.

        @warning Requires device->isGpuAvailable(). Probe unusual formats with
                 GpuDevice::isFormatSupported() / isFormatRenderable() first.
    */
    static GpuTexture::Ptr create (ReferenceCountedObjectPtr<GpuDevice> device, const GpuTextureDesc& desc);

    /** Destructor. Releases the GPU resources with the rendering context current. */
    ~GpuTexture() override;

    //==============================================================================
    /** Uploads CPU pixel data into one mip level of one layer.

        Only supported for textures allocated by create(); textures that wrap
        rendered content return false.

        @param data  The source pixels and the destination region.

        @returns True if the upload was encoded, false if this texture cannot be
                 uploaded to or the descriptor is out of bounds.
    */
    bool upload (const GpuTextureDataDesc& data);

    //==============================================================================
    /** Returns the width of the texture's base mip level in pixels. */
    int getWidth() const noexcept;

    /** Returns the height of the texture's base mip level in pixels. */
    int getHeight() const noexcept;

    /** Returns the texel format of this texture. */
    GpuTextureFormat getFormat() const noexcept;

    /** Returns the storage shape of this texture. */
    GpuTextureType getType() const noexcept;

    /** Returns the number of allocated mip levels (at least 1). */
    uint32_t getMipLevels() const noexcept;

    /** Returns the slice count (3D) or layer count (array / cube) of this texture. */
    uint32_t getDepthOrArrayLayers() const noexcept;

    /** Returns the MSAA sample count of this texture (1 when not multisampled). */
    uint32_t getSampleCount() const noexcept;

    /** Returns true if this texture holds valid GPU resources. */
    bool isValid() const noexcept;

    /** Returns true if this texture can be used as a render pass attachment. */
    bool isRenderTarget() const noexcept;

private:
    //==============================================================================
    friend class Image;
    friend class GpuCanvas;
    friend class GpuTarget;
    friend class GpuRenderPass;
    friend class Graphics;

    GpuTexture() = default;

    static GpuTexture::Ptr fromGpuTexture (ReferenceCountedObjectPtr<GpuDevice> device, rive::rcp<rive::gpu::Texture> texture, int width, int height);
    static GpuTexture::Ptr fromRenderCanvas (ReferenceCountedObjectPtr<GpuDevice> device, rive::rcp<rive::gpu::RenderCanvas> canvas, int width, int height);

    rive::rcp<rive::gpu::Texture> getOrAdoptGpuTexture() const;
    rive::rcp<rive::gpu::RenderCanvas> getInternalRenderCanvas() const;
    rive::RenderImage* getRenderImage() const;
    rive::ore::Texture* getOreTexture() const noexcept { return oreTexture.get(); }

    rive::rcp<rive::ore::TextureView> getOrCreateAttachmentView (rive::ore::Context& oreCtx,
                                                                 const GpuTextureViewDesc& viewDesc) const;

    rive::rcp<rive::ore::TextureView> getOrCreateSamplingView (rive::ore::Context& oreCtx) const;

    GpuTextureViewDesc getWholeTextureViewDesc() const noexcept;

    rive::rcp<rive::ore::TextureView> getOrCreateView (rive::ore::Context& oreCtx,
                                                      bool forRenderTarget,
                                                      const GpuTextureViewDesc& viewDesc) const;

    struct CachedView
    {
        GpuTextureViewDesc desc;
        bool forRenderTarget;
        rive::rcp<rive::ore::TextureView> view;
    };

    //==============================================================================
    int width = 0;
    int height = 0;
    bool renderTarget = false;
    GpuTextureFormat format = GpuTextureFormat::rgba8unorm;
    GpuTextureType type = GpuTextureType::texture2D;
    uint32_t mipLevels = 1;
    uint32_t depthOrArrayLayers = 1;
    uint32_t sampleCount = 1;

    ReferenceCountedObjectPtr<GpuDevice> device;
    rive::rcp<rive::ore::Texture> oreTexture;
    mutable std::vector<CachedView> viewCache;
    mutable rive::rcp<rive::gpu::Texture> gpuTexture;
    rive::rcp<rive::gpu::RenderCanvas> renderCanvas;
    rive::rcp<rive::gpu::Texture> sampledTexture;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuTexture)
};

} // namespace yup
