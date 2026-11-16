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
    An alpha-silhouette drop shadow post-processing effect for a Component subtree.

    Unlike stylesheet `shadow:` layers, which feather the component's bounding rectangle,
    DropShadowEffect blurs the rendered alpha silhouette of the entire subtree (including
    text, icons, and transparent children), draws the tinted blurred silhouette at an
    offset, and composites the sharp component subtree on top. Stylesheets apply alpha
    drop shadows through `filter: drop-shadow(...)` (see @ref FilterEffect).

    @see ComponentEffect, FilterEffect, BlurEffect, GlowEffect

    @tags{UI}
*/
class YUP_API DropShadowEffect : public ComponentEffect
{
public:
    //==============================================================================
    /** Creates a drop shadow effect.

        @param shadowColor  Color and opacity of the shadow.
        @param radiusPoints Blur standard deviation in logical points.
        @param offsetPoints Shadow offset in logical points.
    */
    explicit DropShadowEffect (Color shadowColor = Color (0x80000000),
                               float radiusPoints = 6.0f,
                               Point<float> offsetPoints = { 0.0f, 3.0f }) noexcept;

    /** Destructor. */
    ~DropShadowEffect() override;

    //==============================================================================
    /** Sets the shadow color. */
    void setColor (Color shadowColor) noexcept;

    /** Returns the shadow color. */
    Color getColor() const noexcept;

    /** Sets the blur radius in logical points. */
    void setRadius (float radiusPoints) noexcept;

    /** Returns the blur radius in logical points. */
    float getRadius() const noexcept;

    /** Sets the shadow offset in logical points. */
    void setOffset (Point<float> offsetPoints) noexcept;

    /** Returns the shadow offset in logical points. */
    Point<float> getOffset() const noexcept;

    //==============================================================================
    /** @internal */
    void apply (Graphics& g, GpuTexture::Ptr inputTexture, Rectangle<float> bounds) override;

private:
    std::atomic<uint32> colorArgb { 0x80000000u };
    std::atomic<float> radius { 6.0f };
    std::atomic<float> offsetX { 0.0f };
    std::atomic<float> offsetY { 3.0f };
    GpuDevice::Ptr pipelineDevice;
    GpuPipeline::Ptr pipeline;
    GpuTarget::Ptr targetA;
    GpuTarget::Ptr targetB;
    bool compileAttempted = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DropShadowEffect)
};

} // namespace yup
