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
/**
    A bloom / glow post-processing effect for a Component subtree.

    Blurs the rendered component subtree and additively blends the tinted bloom with the
    sharp subtree content.

    The components the effect is set on show a change of its radius, intensity or color by
    themselves, with nothing to call on them: they are drawn through the effect again,
    without being painted again. Set them on the message thread, or with the message
    manager locked.

    @see ComponentEffect, FilterEffect, BlurEffect, DropShadowEffect

    @tags{UI}
*/
class YUP_API GlowEffect : public ComponentEffect
{
public:
    //==============================================================================
    /** Creates a glow effect.

        @param radiusPoints Blur standard deviation in logical points.
        @param glowIntensity Additive blend strength in [0, 1].
        @param tintColor    Color multiplied into the blurred bloom (white keeps source colors).
    */
    explicit GlowEffect (float radiusPoints = 6.0f,
                         float glowIntensity = 0.75f,
                         Color tintColor = Colors::white) noexcept;

    /** Destructor. */
    ~GlowEffect() override;

    //==============================================================================
    /** Sets the blur radius in logical points. */
    void setRadius (float radiusPoints) noexcept;

    /** Returns the blur radius in logical points. */
    float getRadius() const noexcept;

    /** Sets the additive bloom intensity in [0, 1]. */
    void setIntensity (float glowIntensity) noexcept;

    /** Returns the additive bloom intensity in [0, 1]. */
    float getIntensity() const noexcept;

    /** Sets the tint color multiplied into the bloom. */
    void setColor (Color tintColor) noexcept;

    /** Returns the tint color multiplied into the bloom. */
    Color getColor() const noexcept;

    //==============================================================================
    /** @internal */
    void apply (Graphics& g, GpuTexture::Ptr inputTexture, Rectangle<float> bounds) override;

    /** @internal */
    void releaseResources() override;

private:
    std::atomic<uint32> colorArgb { 0xffffffffu };
    std::atomic<float> radius { 6.0f };
    std::atomic<float> intensity { 0.75f };
    GpuDevice::Ptr pipelineDevice;
    GpuPipeline::Ptr pipeline;
    GpuTarget::Ptr targetA;
    GpuTarget::Ptr targetB;
    bool compileAttempted = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlowEffect)
};

} // namespace yup
