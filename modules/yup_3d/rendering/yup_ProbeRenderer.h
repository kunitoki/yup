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

#pragma once

namespace yup
{

//==============================================================================
/** Render passes that build and filter radiance cubes, for the environment and the probes.

    Every radiance texture it reads or writes has the format getRadianceFormat(): rgba16float, or
    rgba8unorm holding RGBM on devices that cannot render half floats. The passes record into the
    caller's frame and run on both tiers; none of them uses compute.

    - equirectToCube() renders a latitude-longitude texture into mip 0 of a cube.
    - downsampleCube() fills the other mips of that cube, keeping its energy.
    - prefilterToCube() and prefilterToAtlas() filter that cube for GGX reflections, one roughness
      per mip, into another cube or into an octahedral tile of an atlas.
    - projectIrradiance() and renderVisibility() bake one probe of an irradiance volume: the first
      two bands of the spherical harmonics of its radiance cube, and the visibility moments of its
      distance cube.

    @code
    ProbeRenderer probes (device);
    auto source = probes.createCube (128);
    auto filtered = probes.createCube (128);

    auto frame = GpuFrame::begin (device);
    probes.equirectToCube (frame, equirect, source);
    probes.downsampleCube (frame, source);
    probes.prefilterToCube (frame, source, filtered);
    frame.submit();
    @endcode

    No call keeps a render target once it returns, so no texture is held alive by the toolkit.

    @internal
*/
class YUP_API ProbeRenderer
{
public:
    //==============================================================================
    /** Creates the toolkit for a device. The passes are compiled on their first use. */
    explicit ProbeRenderer (GpuDevice::Ptr device);

    /** Destructor. */
    ~ProbeRenderer();

    //==============================================================================
    /** Returns the device the passes run on. */
    const GpuDevice::Ptr& getDevice() const noexcept { return device; }

    /** Format of every radiance texture: rgba16float, or rgba8unorm holding RGBM below the float floor. */
    GpuTextureFormat getRadianceFormat() const noexcept;

    /** Creates a render-target cube in getRadianceFormat().

        @param faceSize  The width and height of each face at mip 0, a power of two.
        @param numMips   The number of mips, at most the full chain, or 0 for the full chain.

        @returns The cube, or nullptr without a device, for a face size that is not a power of two,
                 for more mips than the full chain, or if it could not be created.
    */
    GpuTexture::Ptr createCube (int faceSize, int numMips = 0) const;

    //==============================================================================
    /** Renders an equirect texture (with mips) into mip 0 of every face of @p cube.

        The equirect maps directions as EnvironmentMap::getTextureCoordinates() does. Each cube
        texel reads the equirect mip whose rows match its angular size, so a large equirect does
        not alias into a small cube.

        An RGBM equirect (rgba8unorm) is read at mip 0 only, its texels decoded and then filtered
        bilinearly here: filtering encoded texels would mix their multipliers. Its other mips are
        ignored, so keep its width at most about 4 times the cube's face size, as EnvironmentMap
        does, or the cube aliases. Every other RGBM read of these passes decodes texel by texel too.

        @param frame     The frame to record into.
        @param equirect  A 2D texture in getRadianceFormat(), with any number of mips.
        @param cube      A render-target cube in getRadianceFormat(), its face size a power of two.

        @returns False on a null or wrong-shaped texture, recording nothing, or if the pass could
                 not be compiled or recorded.
    */
    bool equirectToCube (GpuFrame& frame, const GpuTexture::Ptr& equirect, const GpuTexture::Ptr& cube);

    /** Fills mips 1..N-1 of every face by a 2x2 solid-angle-weighted downsample of the mip above.

        @param frame  The frame to record into, after mip 0 was written.
        @param cube   A render-target cube in getRadianceFormat(), its face size a power of two.

        @returns False on a null or wrong-shaped texture, recording nothing, or if the pass could
                 not be compiled or recorded.
    */
    bool downsampleCube (GpuFrame& frame, const GpuTexture::Ptr& cube);

    /** GGX-prefilters @p source (with mips) into every mip of @p destination, mip m filtered for
        the perceptual roughness m / (destinationMips - 1). Mip 0 is a copy: the source mip of its
        size, or the one closest to it.

        @param frame        The frame to record into.
        @param source       A cube in getRadianceFormat() with its full mip chain filled, as
                            downsampleCube() leaves it, its face size a power of two.
        @param destination  Another render-target cube in getRadianceFormat(), its face size any
                            power of two.

        @returns False on a null or wrong-shaped texture or when both are the same texture,
                 recording nothing, or if the pass could not be compiled or recorded.
    */
    bool prefilterToCube (GpuFrame& frame, const GpuTexture::Ptr& source, const GpuTexture::Ptr& destination);

    /** Same, into an octahedral tile of an atlas whose mips are aligned to the tile.

        Mip m of the tile is @p tileAtMip0 with every coordinate shifted right by m (ProbeAtlas
        alignment) and is filtered for the roughness m / (numMips - 1). Each mip maps the octahedron
        with a half-texel inset of its own, so the border texels lie on its edges and a bilinear
        lookup at a whole mip never leaves the tile. A lod between two mips reads each of them at
        its own uv and mixes the two: read the tile with octahedralTileTaps() in
        shaders/lib/octahedral.glsl, never with one trilinear lookup, which would read the
        neighboring tiles. The other tiles of the atlas are kept.

        @param frame       The frame to record into.
        @param source      A cube in getRadianceFormat() with its full mip chain filled, its face
                           size a power of two.
        @param atlas       A render-target 2D texture in getRadianceFormat(), with at least
                           @p numMips mips.
        @param tileAtMip0  A square tile inside the atlas, its position and size multiples of
                           2^(numMips - 1), at least 2 texels wide at the last mip.
        @param numMips     The number of mips to write, at least 1.

        @returns False on a null or wrong-shaped texture or a misplaced tile, recording nothing,
                 or if the pass could not be compiled or recorded.
    */
    bool prefilterToAtlas (GpuFrame& frame, const GpuTexture::Ptr& source, const GpuTexture::Ptr& atlas,
                           Rectangle<int> tileAtMip0, int numMips);

    //==============================================================================
    /** Projects the radiance of @p cube on the first two bands of the real spherical harmonics,
        convolved into irradiance as SphericalHarmonics::toIrradiance() does, into four texels of
        @p atlas: coefficient c, in the order 1, y, z, x, at (texel.x, texel.y + c). Each one sums
        the 384 texels of the cube mip whose faces are 8 texels wide (or mip 0 when smaller),
        weighted by their solid angle.

        Below the float floor (RGBM radiance) coefficient 0 is stored in RGBM and the others as
        their ratios to it per channel, SphericalHarmonics::encodeRatio(), since they may be negative.

        @param frame  The frame to record into.
        @param cube   A cube in getRadianceFormat() with its full mip chain filled, as downsampleCube() leaves it.
        @param atlas  A render-target 2D texture in getRadianceFormat().
        @param texel  The texel of coefficient 0; the column of four must lie inside @p atlas.

        @returns False on a null or wrong-shaped texture or a texel outside the atlas, recording
                 nothing, or if the pass could not be compiled or recorded.
    */
    bool projectIrradiance (GpuFrame& frame, const GpuTexture::Ptr& cube, const GpuTexture::Ptr& atlas, Point<int> texel);

    /** The format of the distance cubes and visibility atlases of the irradiance volumes. */
    static constexpr GpuTextureFormat visibilityFormat = GpuTextureFormat::rg16float;

    /** True when the device renders visibilityFormat and half float radiance: below the float floor
        the irradiance volumes have no visibility. */
    bool canRenderVisibility() const noexcept;

    /** Writes the visibility moments of an irradiance volume probe into a bordered octahedral tile
        of @p atlas, (tileSize + 2) texels wide with @p origin its top-left border texel (see
        shaders/lib/volumes.glsl): per texel direction, the mean distance and the mean squared
        distance in x and y, read from @p distanceCube under a cos^50 lobe with 64 samples, times
        @p distanceScale. The other texels of the atlas are kept.

        @param frame          The frame to record into.
        @param distanceCube   A cube in visibilityFormat holding the distance from the probe in x.
        @param atlas          A render-target 2D texture in visibilityFormat.
        @param origin         The top-left border texel of the tile.
        @param tileSize       The tile size without its border, at least 2.
        @param distanceScale  What the distances are multiplied by: 1 over the cell diagonal.

        @returns False when visibility cannot render (canRenderVisibility()), on a null or
                 wrong-shaped texture or a tile outside the atlas, recording nothing, or if the
                 pass could not be compiled or recorded.
    */
    bool renderVisibility (GpuFrame& frame, const GpuTexture::Ptr& distanceCube, const GpuTexture::Ptr& atlas,
                           Point<int> origin, int tileSize, float distanceScale);

    //==============================================================================
    /** @internal For tests only: use RGBM in rgba8unorm even when half floats are renderable. */
    void setForceRgbmForTesting (bool shouldForceRgbm);

private:
    /** A pass of the irradiance volumes, compiled on its first use and kept, apart from prepare(). */
    struct VolumePass
    {
        FullscreenPass pass;
        std::optional<Result> prepared;
    };

    bool prepare();
    bool usesRgbm() const noexcept;
    bool isRadianceCube (const GpuTexture::Ptr& texture) const noexcept;
    FullscreenPass* prepareVolumePass (VolumePass& volumePass, const uint8* bundle, size_t bundleSize, GpuTextureFormat format);

    GpuDevice::Ptr device;
    bool forceRgbm = false;
    std::optional<bool> prepared;

    FullscreenPass equirectPass;
    FullscreenPass downsamplePass;
    FullscreenPass prefilterPass;
    VolumePass irradiancePass;      // rgba16float
    VolumePass irradianceRgbmPass;  // rgba8unorm, below the float floor
    VolumePass visibilityPass;

    GpuSampler::Ptr equirectSampler;
    GpuSampler::Ptr cubeSampler;
    GpuSampler::Ptr distanceSampler;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProbeRenderer)
};

} // namespace yup
