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
    A two-pass separable Gaussian blur post-processing effect for a Component subtree.

    When instantiated in C++, the blur radius in logical points can be adjusted at any time
    with @ref setRadius. Stylesheets apply blurs through the CSS `filter: blur(...)` declaration
    (see @ref FilterEffect).

    Without a GPU context or when the radius is zero, the input texture is drawn unfiltered.

    @see ComponentEffect, FilterEffect, Component::setComponentEffect

    @tags{UI}
*/
class YUP_API BlurEffect : public ComponentEffect
{
public:
    //==============================================================================
    /** Creates a blur effect with the given radius in logical points.

        @param radiusPoints Standard deviation of the Gaussian kernel, in logical points.
    */
    explicit BlurEffect (float radiusPoints = 4.0f) noexcept;

    /** Destructor. */
    ~BlurEffect() override;

    //==============================================================================
    /** Sets the blur radius in logical points. */
    void setRadius (float radiusPoints) noexcept;

    /** Returns the blur radius in logical points. */
    float getRadius() const noexcept;

    //==============================================================================
    /** @internal */
    void apply (Graphics& g, GpuTexture::Ptr inputTexture, Rectangle<float> bounds) override;

private:
    std::atomic<float> radius { 4.0f };
    GpuDevice::Ptr pipelineDevice;
    GpuPipeline::Ptr pipeline;
    GpuTarget::Ptr targetA;
    GpuTarget::Ptr targetB;
    bool compileAttempted = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlurEffect)
};

} // namespace yup
