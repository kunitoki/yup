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
/** An image used by a Material, with the sampler state to read it.

    The CPU image is the source of truth. The GPU texture is created the first time a device
    asks for it, with a full mip chain generated on the CPU, and created again when another
    device asks.

    A texture can also live only on the GPU, for example to show a Component rendered with
    Component::renderToTexture() on a mesh: create it from a GpuTexture and replace it with
    setGpuTexture() whenever the content changes.

    Color textures (base color and emissive) hold sRGB values: mark them with @a isSrgb so the
    renderer decodes them to linear before lighting. Data textures (normals, metallic-roughness,
    occlusion) are linear.

    @see Material, GltfModel
*/
class YUP_API Texture : public ReferenceCountedObject
{
public:
    //==============================================================================
    using Ptr = ReferenceCountedObjectPtr<Texture>;

    //==============================================================================
    /** Creates a texture.

        @param sourceImage  The pixels. Any pixel format is accepted, it is converted to RGBA on upload.
        @param sampler      How the texture is filtered and wrapped.
        @param isSrgb       True if the pixels are sRGB encoded colors.
    */
    Texture (Image sourceImage, const GpuSamplerDesc& sampler = {}, bool isSrgb = false);

    /** Creates a texture whose pixels live on the GPU only.

        @param texture  The GPU texture, or nullptr until setGpuTexture() provides one.
        @param sampler  How the texture is filtered and wrapped.
        @param isSrgb   True if the pixels are sRGB encoded colors.
    */
    Texture (GpuTexture::Ptr texture, const GpuSamplerDesc& sampler = {}, bool isSrgb = false);

    /** Destructor. */
    ~Texture() override;

    //==============================================================================
    /** Returns the CPU image, which is invalid for a texture created from a GpuTexture. */
    const Image& getImage() const noexcept { return image; }

    /** Returns the sampler state. */
    const GpuSamplerDesc& getSamplerDesc() const noexcept { return samplerDesc; }

    /** Returns true if the pixels are sRGB encoded colors. */
    bool isSrgb() const noexcept { return srgb; }

    /** Returns true if the texture was created from a GpuTexture instead of an Image. */
    bool isGpuOnly() const noexcept { return gpuOnly; }

    /** Replaces the pixels of a texture created from a GpuTexture.

        The texture must belong to the device that renders it.

        @param newTexture The new GPU texture.
    */
    void setGpuTexture (GpuTexture::Ptr newTexture);

    //==============================================================================
    /** Returns the GPU texture for a device, uploading it with its mip chain if needed.

        A texture created from a GpuTexture returns that texture as is.

        @param device The device that renders the texture.

        @return The texture, or nullptr if the image is invalid or the upload failed.
    */
    GpuTexture::Ptr getGpuTexture (const GpuDevice::Ptr& device);

    /** Returns the GPU sampler for a device, creating it if needed.

        Anisotropic filtering is turned off when the device doesn't support it, or when the
        sampler doesn't filter linearly for minification, magnification and mipmaps.

        @param device The device that renders the texture.
    */
    GpuSampler::Ptr getGpuSampler (const GpuDevice::Ptr& device);

    //==============================================================================
    /** Returns the RGBA8 pixels of every mip level, from the full size image down to 1x1.

        Each level halves the previous one with a box filter, rounding odd sizes down.

        @param image The source image, in any pixel format.
    */
    static std::vector<std::vector<uint8>> createMipChain (const Image& image);

private:
    void useDevice (const GpuDevice::Ptr& device);

    Image image;
    GpuSamplerDesc samplerDesc;
    bool srgb = false;
    bool gpuOnly = false;

    GpuDevice::Ptr cachedDevice;
    GpuTexture::Ptr gpuTexture;
    GpuSampler::Ptr gpuSampler;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Texture)
};

} // namespace yup
