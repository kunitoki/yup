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

#include <array>
#include <cmath>
#include <optional>

//==============================================================================
/**
    A component effect that turns a live panel into liquid glass.

    A lens magnifies the panel under the pointer and rings of waves spread from
    where the pointer moved; shaders/ripple.frag samples the panel through the
    warp and lights its slopes. displayToContent() repeats the same warp on the
    CPU, so a click lands on the widget the visitor sees under the pointer.
*/
class RippleEffect : public yup::ComponentEffect
{
public:
    //==============================================================================
    struct Ring
    {
        yup::Point<float> center; // in uv of the panel
        float radius = 0.0f;      // how far the wave front travelled, as a fraction of the height
        float amplitude = 0.0f;
    };

    struct State
    {
        yup::Point<float> lensCenter; // in uv of the panel
        float lensStrength = 0.0f;
        std::array<Ring, 8> rings {};
        int numRings = 0;
    };

    static constexpr float lensRadius = 0.2f;

    //==============================================================================
    /** Sets the lens and the rings drawn from the next frame on. */
    void setState (const State& newState)
    {
        const yup::SpinLock::ScopedLockType sl (stateLock);
        state = newState;
    }

    //==============================================================================
    void apply (yup::Graphics& g, yup::GpuTexture::Ptr input, yup::Rectangle<float> bounds) override
    {
        if (input == nullptr)
            return;

        auto& context = g.getGraphicsContext();
        if (! ensurePipeline (context))
        {
            g.drawTexture (input, bounds);
            return;
        }

        if (target == nullptr || target->getWidth() != input->getWidth() || target->getHeight() != input->getHeight())
            target = yup::GpuTarget::create (device, input->getWidth(), input->getHeight());

        if (target == nullptr)
        {
            g.drawTexture (input, bounds);
            return;
        }

        Params params;
        params.frame[0] = static_cast<float> (input->getWidth());
        params.frame[1] = static_cast<float> (input->getHeight());

        {
            const yup::SpinLock::ScopedLockType sl (stateLock);

            params.lens[0] = state.lensCenter.getX();
            params.lens[1] = state.lensCenter.getY();
            params.lens[2] = lensRadius;
            params.lens[3] = state.lensStrength;
            params.frame[3] = static_cast<float> (state.numRings);

            for (int i = 0; i < state.numRings; ++i)
            {
                const auto& ring = state.rings[static_cast<std::size_t> (i)];
                params.rings[i][0] = ring.center.getX();
                params.rings[i][1] = ring.center.getY();
                params.rings[i][2] = ring.radius;
                params.rings[i][3] = ring.amplitude;
            }
        }

        auto frame = yup::GpuFrame::begin (device);
        if (! frame.isValid())
            return;

        {
            auto pass = target->beginRenderPass (frame, { true, yup::Colors::transparentBlack });
            pass.setPipeline (pipeline);
            pass.setTexture (0, 0, input);
            pass.setUniformBuffer (0, 2, &params, sizeof (params));
            pass.draw (3);
            pass.finish();
        }

        frame.submit();
        g.drawTexture (target->asTexture(), bounds);
    }

    //==============================================================================
    std::optional<yup::Point<float>> displayToContent (yup::Point<float> displayPoint, yup::Rectangle<float> bounds) const override
    {
        if (bounds.isEmpty())
            return std::nullopt;

        const yup::Point<float> size { bounds.getWidth(), bounds.getHeight() };
        const auto uv = (displayPoint - bounds.getTopLeft()) / size;

        return bounds.getTopLeft() + (uv + sampleOffset (uv, bounds.getWidth() / bounds.getHeight())) * size;
    }

    std::optional<yup::Point<float>> contentToDisplay (yup::Point<float> contentPoint, yup::Rectangle<float> bounds) const override
    {
        // The warp is small and smooth, so a few fixed point steps invert it
        auto displayPoint = contentPoint;

        for (int iteration = 0; iteration < 8; ++iteration)
        {
            const auto sampled = displayToContent (displayPoint, bounds);
            if (! sampled)
                return std::nullopt;

            displayPoint = contentPoint - (*sampled - displayPoint);
        }

        return displayPoint;
    }

private:
    //==============================================================================
    struct alignas (16) Params
    {
        float frame[4] = {};
        float lens[4] = {};
        float rings[8][4] = {};
    };

    static constexpr float ringWavenumber = 70.0f;
    static constexpr float ringWidth = 0.05f;

    /** Mirrors glassHeight() in shaders/ripple.frag: where the panel is sampled from, relative to uv. */
    yup::Point<float> sampleOffset (yup::Point<float> uv, float aspect) const
    {
        const yup::SpinLock::ScopedLockType sl (stateLock);

        const auto fromLens = (uv - state.lensCenter) * yup::Point<float> (aspect, 1.0f);
        const auto bump = state.lensStrength * std::exp (-fromLens.dotProduct (fromLens) / (lensRadius * lensRadius));
        auto offset = (uv - state.lensCenter) * -bump;

        for (int i = 0; i < state.numRings; ++i)
        {
            const auto& ring = state.rings[static_cast<std::size_t> (i)];
            const auto fromRing = (uv - ring.center) * yup::Point<float> (aspect, 1.0f);
            const auto across = fromRing.distanceTo ({}) - ring.radius;
            const auto wave = ring.amplitude * std::sin (across * ringWavenumber) * std::exp (-across * across / (ringWidth * ringWidth));

            offset = offset + (fromRing + yup::Point<float> (1.0e-5f, 1.0e-5f)).normalized() / yup::Point<float> (aspect, 1.0f) * wave;
        }

        return offset;
    }

    bool ensurePipeline (yup::GraphicsContext& context)
    {
        if (pipeline != nullptr)
            return true;

        if (compileAttempted || ! context.isGpuAvailable())
            return false;

        compileAttempted = true;
        device = context.getGpuDevice();
        if (device == nullptr)
            return false;

        auto result = compileEmbeddedPipeline (device, website::RippleShader_data, website::RippleShader_size, {});
        if (result.failed())
        {
            yup::Logger::outputDebugString ("RippleEffect: " + result.getErrorMessage());
            return false;
        }

        pipeline = result.getValue();
        return true;
    }

    //==============================================================================
    mutable yup::SpinLock stateLock;
    State state;

    yup::GpuDevice::Ptr device;
    yup::GpuPipeline::Ptr pipeline;
    yup::GpuTarget::Ptr target;
    bool compileAttempted = false;
};
