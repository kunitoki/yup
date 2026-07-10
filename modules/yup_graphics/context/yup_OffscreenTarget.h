/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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
/** Opaque platform-specific GPU resources for a fixed-size offscreen render.

    Created by GraphicsContext::createOffscreenTarget(); passed to
    beginOffscreen/endOffscreen and readOffscreenPixels.

    Each instance may own a dedicated RenderContext when nesting is detected,
    allowing recursive offscreen rendering (TransparencyLayer inside GpuCanvas,
    nested precomps) without re-entering beginFrame on a shared context.
*/
class YUP_API OffscreenTarget
{
public:
    virtual ~OffscreenTarget() = default;

    /** Returns the width of this offscreen target in pixels. */
    virtual int getWidth() const noexcept = 0;

    /** Returns the height of this offscreen target in pixels. */
    virtual int getHeight() const noexcept = 0;

    /** Returns the underlying Rive render target. */
    virtual rive::gpu::RenderTarget* getRenderTarget() noexcept = 0;

    /** Returns the Rive render context used by this offscreen target. */
    virtual rive::gpu::RenderContext* getRenderContext() noexcept { return nullptr; }

    /** Returns the underlying Rive render canvas, if this target is backed by one. */
    virtual rive::rcp<rive::gpu::RenderCanvas> getRenderCanvas() noexcept { return nullptr; }

    /** Returns the rendered result as a sampled Rive GPU texture suitable for use in drawImage.
        Must be called after endOffscreen(). Returns nullptr on failure. */
    virtual rive::rcp<rive::gpu::Texture> adoptAsTexture() = 0;

    /** Creates (on first call) and returns the Y-flipped companion texture. Must only be
        called by commit() — not by asTexture() — so that the mirror exists before the
        endOffscreen flush runs blitMirrorIfRegistered. No-op on non-GL backends. */
    virtual rive::rcp<rive::gpu::Texture> getOrCreateSampledTexture() { return nullptr; }

    /** Returns the Y-flipped companion texture if it was already created by a prior call to
        getOrCreateSampledTexture(), or nullptr if it was never created. Used by asTexture()
        so that GPU render-pass canvases (which never call commit()) never get a stale mirror
        attached to their GpuTexture. */
    virtual rive::rcp<rive::gpu::Texture> getSampledTexture() const { return nullptr; }
};

} // namespace yup
