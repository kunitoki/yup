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

/** Animated curl-noise bands by Abhishek Shivakumar (@godofecht).

    Renders a precompiled fragment shader using the shared fullscreen triangle.
    The uniform buffer supplies elapsed time and the render target dimensions.
*/
class CurlBandsDemo : public yup::Component
{
public:
    CurlBandsDemo()
        : yup::Component ("CurlBandsDemo")
    {
        statusLabel.setText ("Curl Bands by Abhishek Shivakumar (@godofecht)", yup::dontSendNotification);
        addAndMakeVisible (statusLabel);
    }

    void resized() override
    {
        statusLabel.setBounds (getLocalBounds().removeFromBottom (30.0f).reduced (8.0f, 0.0f));
    }

    void refreshDisplay (double seconds) override
    {
        if (isShowing())
        {
            elapsedTime += seconds;
            repaint();
        }
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (yup::ThemePalette::Role::background));
        g.fillAll();

        auto bounds = getLocalBounds();
        bounds.removeFromBottom (30.0f);
        const int width = static_cast<int> (bounds.getWidth());
        const int height = static_cast<int> (bounds.getHeight());
        if (width < 2 || height < 2 || ! ensurePipeline (g.getGraphicsContext()))
            return;

        if (target == nullptr || target->getWidth() != width || target->getHeight() != height)
            target = yup::GpuTarget::create (device, width, height);

        if (target == nullptr)
            return;

        const Params params { static_cast<float> (elapsedTime), static_cast<float> (width), static_cast<float> (height), 0.0f };
        auto frame = yup::GpuFrame::begin (device);
        if (! frame.isValid())
            return;

        auto pass = target->beginRenderPass (frame);
        if (! pass.isValid())
            return;

        pass.setPipeline (pipeline);
        pass.setUniformBuffer (0, 0, &params, sizeof (params));
        if (! pass.draw (3) || ! pass.finish() || ! frame.submit())
            return;

        g.drawTexture (target->asTexture(), bounds);
    }

private:
    struct alignas (16) Params
    {
        float time;
        float width;
        float height;
        float padding;
    };

    bool ensurePipeline (yup::GraphicsContext& context)
    {
        if (pipeline != nullptr)
            return true;

        if (compileAttempted)
            return false;

        if (! context.isGpuAvailable())
        {
            statusLabel.setText ("Curl Bands requires GPU rendering. Shader by Abhishek Shivakumar (@godofecht).", yup::dontSendNotification);
            return false;
        }

        compileAttempted = true;
        device = context.getGpuDevice();
        yup::GpuPipelineOptions options;
        options.colorTargets.emplace_back().blendEnabled = false;
        auto result = compilePipelineFromBundle (device, "curl_bands", options);
        if (result.failed())
        {
            statusLabel.setText ("Curl Bands: " + result.getErrorMessage(), yup::dontSendNotification);
            yup::Logger::outputDebugString ("CurlBandsDemo: " + result.getErrorMessage());
            return false;
        }

        pipeline = result.getValue();
        return true;
    }

    yup::Label statusLabel;
    yup::GpuDevice::Ptr device;
    yup::GpuPipeline::Ptr pipeline;
    yup::GpuTarget::Ptr target;
    double elapsedTime = 0.0;
    bool compileAttempted = false;
};
