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

class ProbeRenderer;

//==============================================================================
/** The light surrounding a scene, for reflections and soft diffuse light.

    The radiance function is sampled once on the CPU into a latitude-longitude image, four
    samples per texel, and projected into nine spherical harmonics coefficients for the
    diffuse light. On the GPU, buildCube() turns the image into a cube whose faces are a quarter
    of its width, then GGX-prefilters it into a cube with one mip per roughness, from mirror-like
    at mip 0 to fully rough at the last one. The SceneRenderer drawing the environment builds and
    keeps its cube; the environment itself never changes after construction, so any number of
    renderers, on any threads, can share it.

    @code
    // A blue sky over a dark ground
    auto sky = [] (const yup::Vector3<float>& direction)
    {
        return direction.getY() > 0.0f ? yup::Vector3<float> (0.4f, 0.6f, 1.0f) : yup::Vector3<float> (0.05f, 0.05f, 0.05f);
    };

    scene->setEnvironment (new yup::EnvironmentMap (sky));
    @endcode

    @see Scene::setEnvironment
*/
class YUP_API EnvironmentMap : public ReferenceCountedObject
{
public:
    //==============================================================================
    using Ptr = ReferenceCountedObjectPtr<EnvironmentMap>;

    /** Returns the linear RGB radiance seen along a unit direction. */
    using RadianceFunction = std::function<Vector3<float> (const Vector3<float>& direction)>;

    static constexpr int defaultWidth = 512;   ///< The width of the sampled latitude-longitude image; cube faces are a quarter of it.
    static constexpr int defaultNumLevels = 7; ///< The number of roughness levels (cube mips).

    //==============================================================================
    /** Samples an environment.

        @param radiance   The radiance function, called four times for every texel of the image.
        @param width      The width of the latitude-longitude image, a power of two of at least 8.
                          Its height is half, and the cube faces are a quarter of it.
        @param numLevels  The number of roughness levels, each a cube mip half the size of the
                          previous one, limited by the face size.
    */
    explicit EnvironmentMap (const RadianceFunction& radiance, int width = defaultWidth, int numLevels = defaultNumLevels);

    /** Destructor. */
    ~EnvironmentMap() override;

    //==============================================================================
    /** Returns the number of roughness levels, the mips of the GPU cube. */
    int getNumLevels() const noexcept { return numLevels; }

    /** Returns the width and height of the GPU cube's faces at mip 0, in texels. */
    int getFaceSize() const noexcept { return width / 4; }

    /** Returns the perceptual roughness a level is filtered for, from 0 to 1. */
    static float getRoughnessForLevel (int level, int numLevels) noexcept;

    //==============================================================================
    /** Returns the nine irradiance coefficients of the diffuse light, computed on the CPU, in
        the order 1, y, z, x, xy, yz, 3z^2 - 1, xz, x^2 - y^2. Evaluate them with
        SphericalHarmonics::evaluateIrradiance().
    */
    const std::array<Vector3<float>, 9>& getIrradianceCoefficients() const noexcept { return irradiance; }

    //==============================================================================
    /** Returns the texture coordinates of a unit direction: u around the vertical axis, starting
        towards +Z, and v from the zenith (0) to the nadir (1).
    */
    static Point<float> getTextureCoordinates (const Vector3<float>& direction) noexcept;

    /** Returns the unit direction at some texture coordinates, the inverse of getTextureCoordinates(). */
    static Vector3<float> getDirection (Point<float> textureCoordinates) noexcept;

    //==============================================================================
    /** A cube buildCube() submitted, and the outcome of its frame. */
    struct CubeBuild
    {
        GpuTexture::Ptr cube;  ///< The prefiltered cube, or nullptr if nothing could be recorded.
        GpuFrameStatus status; ///< The frame building it: poll it before trusting the cube, it is undefined if the GPU failed.

        /** The rgba16float latitude-longitude image with its mips (about 1.3 MB at the default
            width), or nullptr on the RGBM path. Keep it as long as the cube: it owns the pipeline
            of its GpuTexture::generateMips() pass, which the frame does not retain, so dropping it
            while the build is pending would destroy a pipeline the GPU still uses on Vulkan and
            Direct3D 12. The frame retains every other texture of the build.
        */
        GpuTexture::Ptr image;
    };

    /** Builds the GGX-prefiltered cube with the passes of @p probes, on its device.

        The cube has getNumLevels() mips, mip m filtered for getRoughnessForLevel (m, getNumLevels()),
        in ProbeRenderer::getRadianceFormat(): rgba16float, or rgba8unorm holding RGBM below the
        float floor. The build begins a GpuFrame of its own, records every pass into it and submits
        it without waiting for the GPU, so call it outside an open frame and keep @p probes alive
        until the status completes. Every call builds a new cube: the caller keeps it.

        @returns The cube and its frame's status. No cube and an invalid status if the frame could
                 not begin; no cube with a valid status if the textures could not be created or
                 the passes could not be recorded, which a retry would not change.
    */
    CubeBuild buildCube (ProbeRenderer& probes) const;

private:
    int width = defaultWidth;
    int numLevels = defaultNumLevels;
    std::vector<uint16> pixels; // The latitude-longitude image as half floats, kept to build again
    std::array<Vector3<float>, 9> irradiance;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvironmentMap)
};

} // namespace yup
