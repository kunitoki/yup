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
/** Renders a Scene into an offscreen texture through the RHI.

    Shading follows the glTF metallic-roughness model (GGX distribution, Smith visibility,
    Schlick Fresnel) with KHR_lights_punctual lights, a flat ambient term, normal mapping,
    alpha masking and blending, and double-sided materials. The result is exposed, tone mapped
    with Reinhard and sRGB encoded into an rgba8unorm texture.

    Geometry edges are antialiased with MSAA, 4 samples by default, resolved into the returned
    texture. Highlights of very smooth surfaces are widened where normals change quickly across
    a pixel, to keep them from sparkling.

    The color and depth targets are kept between frames and created again when the size, the
    sample count or the device changes, so render() can be called every frame.

    @see SceneComponent, SceneDrawList
*/
class YUP_API SceneRenderer
{
public:
    //==============================================================================
    /** The maximum number of lights shading a frame. Extra lights are ignored. */
    static constexpr int maxLights = 8;

    //==============================================================================
    /** Creates a renderer. GPU objects are created on the first render. */
    SceneRenderer();

    /** Destructor. */
    ~SceneRenderer();

    //==============================================================================
    /** Renders a scene.

        @param device    The device to render with.
        @param scene     The scene to render.
        @param widthPx   The width of the result in pixels.
        @param heightPx  The height of the result in pixels.

        @return The rendered texture, valid until the next render, or nullptr on failure; see getLastError().
    */
    GpuTexture::Ptr render (const GpuDevice::Ptr& device, Scene& scene, int widthPx, int heightPx);

    /** Returns why the last render failed, or an empty string if it succeeded. */
    const String& getLastError() const noexcept { return lastError; }

    //==============================================================================
    /** Changes the number of MSAA samples per pixel.

        The count is rounded down to a power of two and limited to what the device supports,
        see GpuDevice::getMaximumSampleCount(). 1 disables multisampling.

        @param newSampleCount The requested sample count, 1 or above.
    */
    void setSampleCount (int newSampleCount) noexcept { sampleCount = jmax (1, newSampleCount); }

    /** Returns the requested number of MSAA samples per pixel. */
    int getSampleCount() const noexcept { return sampleCount; }

private:
    struct Impl;

    std::unique_ptr<Impl> impl;
    String lastError;
    int sampleCount = 4;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SceneRenderer)
};

} // namespace yup
