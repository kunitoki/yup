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
/** The textures, storage buffers and shared samplers a renderer needs from frame to frame.

    Transient textures are pooled by descriptor: a texture acquired in one frame is handed back by
    the same acquire() in the next one, and released after a frame that did not acquire it.
    History textures are ping-pong pairs kept across frames, for effects that read last frame's
    output such as temporal anti-aliasing, and released after a frame that did not use them.

    Descriptors match on width, height, format, type, depth or layer count, mip levels, sample count,
    render target and storage flags; labels are ignored.

    Storage buffers rotate over a ring of three per slot, advanced by every beginFrame(), so the
    CPU writes a different buffer from the ones the previous frames bound. The whole ring of a slot
    is released after a frame that did not acquire it.

    The samplers every pass binds in set 0 are created once per pool, so every pass of a renderer
    shares the same sampler objects.

    @code
    resources.beginFrame (width, height);
    auto sceneColor = resources.acquire (GpuTextureDesc (width, height, GpuTextureFormat::rgba16float, true));
    auto history = resources.getHistory ("taa", GpuTextureDesc (width, height, GpuTextureFormat::rgba16float, true));
    // ... render into sceneColor, resolve into history.write, reading history.read when history.valid
    resources.endFrame();
    @endcode

    @internal
*/
class YUP_API FrameResources
{
public:
    //==============================================================================
    /** Creates the pool for a device. */
    explicit FrameResources (GpuDevice::Ptr device);

    //==============================================================================
    /** Starts a frame; a size change releases every texture and history. */
    void beginFrame (int widthPx, int heightPx);

    /** A transient texture for this frame. The same descriptor gets the same texture back next frame;
        two acquires of one descriptor within a frame get two textures.

        @returns The texture, or nullptr when the device cannot create it.
    */
    GpuTexture::Ptr acquire (const GpuTextureDesc& desc);

    //==============================================================================
    /** A ping-pong pair kept across frames: read() is last frame's write(). */
    struct History
    {
        GpuTexture::Ptr read;
        GpuTexture::Ptr write;
        bool valid = false; ///< False on the first frame after creation, a resize or invalidateHistory().
    };

    /** Returns the history pair called @p name, creating it on first use.

        Calls within one frame return the same pair. A different descriptor reallocates the pair,
        which is then invalid.
    */
    History getHistory (StringRef name, const GpuTextureDesc& desc);

    /** Returns the history pair called @p name as its owner's getHistory() left it, for a reader
        of a history another pass owns: it never creates, reallocates or uses the pair, so the
        owner alone decides its size and lifetime. Both textures are nullptr and the history is
        invalid when there is no such pair.
    */
    History findHistory (StringRef name) const;

    /** Marks every history invalid, so the next getHistory() reports it as such. The textures are kept.

        A history already fetched this frame stays invalid in the next frame too, since this frame's
        write was computed from the stale contents.
    */
    void invalidateHistory() noexcept;

    //==============================================================================
    /** The storage buffers a frame can acquire. */
    enum class StorageSlot
    {
        lights,           ///< The packed lights.
        clusterLights,    ///< The light lists of the clusters.
        materials,        ///< The material table.
        reflectionProbes, ///< The reflection probes of the full tier.
        irradianceVolumes ///< The irradiance volumes of the full tier.
    };

    /** A storage buffer of at least @p byteSize for this frame. Contents are undefined until written.

        Each slot rotates over a ring of three buffers, one step per beginFrame() call: the ring
        counts calls, not frames the GPU finished. Write each slot once per frame with
        GpuDevice::updateBuffer(), before the first pass binding it is recorded: the contents are
        then visible to the whole frame when it executes, and a frame retains the buffers it binds.

        Acquiring a slot again within one frame returns the same buffer, unless @p byteSize no
        longer fits: a buffer too small is replaced by a new one of the next power of two size,
        including by a larger acquire later in the same frame.

        @returns The buffer, or nullptr when the device cannot create it.
    */
    GpuBuffer::Ptr acquireStorageBuffer (StorageSlot slot, size_t byteSize);

    //==============================================================================
    /** The samplers shared by the passes, bound in set 0. */
    struct Samplers
    {
        GpuSampler::Ptr linearClamp;   ///< Set 0 binding 1: trilinear filtering, clamped to the edge.
        GpuSampler::Ptr pointClamp;    ///< Set 0 binding 3: nearest filtering, clamped to the edge.
        GpuSampler::Ptr shadowCompare; ///< Set 0 binding 4: linear filtering, clamped to the edge, comparing with lessEqual, or nullptr when the device cannot create it.
    };

    /** Returns the shared samplers, created by the first call.

        Failures are remembered instead of retried every frame. Only linearClamp and pointClamp
        decide the result: a missing shadowCompare leaves it nullptr in the samplers, and the
        passes reading shadow maps check it themselves.

        @returns The samplers, or nullptr when the device cannot create linearClamp or pointClamp.
    */
    const Samplers* getSamplers();

    /** @internal Test hook, not for production use: the shadowCompare sampler fails to create, as
        if the device could not create it. Call it before the first getSamplers().
    */
    void failShadowCompareSamplerForTesting() noexcept { failShadowCompareSampler = true; }

    //==============================================================================
    /** Releases transients and storage slots not acquired and histories not used this frame, and swaps the used histories. */
    void endFrame();

    //==============================================================================
    /** Returns the width passed to the last beginFrame(). */
    int getWidth() const noexcept;

    /** Returns the height passed to the last beginFrame(). */
    int getHeight() const noexcept;

private:
    struct Transient
    {
        GpuTextureDesc desc;
        GpuTexture::Ptr texture;
        bool acquired = false;
    };

    struct HistoryPair
    {
        String name;
        GpuTextureDesc desc;
        GpuTexture::Ptr textures[2];
        int writeIndex = 0;
        bool valid = false;
        bool used = false;
        bool invalidatedAfterUse = false;
    };

    // One buffer per frame the device keeps in flight (GpuDevice::framesInFlight)
    static constexpr size_t storageRingSize = 3;
    static constexpr size_t numStorageSlots = 5;

    GpuDevice::Ptr device;
    std::vector<Transient> transients;
    std::vector<HistoryPair> histories;
    std::array<std::array<GpuBuffer::Ptr, storageRingSize>, numStorageSlots> storageBuffers;
    std::array<bool, numStorageSlots> storageAcquired {};
    Samplers samplers;
    std::optional<bool> samplersCreated;
    bool failShadowCompareSampler = false;
    uint64 frameCounter = 0;
    int width = 0;
    int height = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrameResources)
};

} // namespace yup
