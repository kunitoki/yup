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

#include <tuple>
#include <cmath>
#include <utility>
#include <initializer_list>

//==============================================================================

/**
    GPU fluid simulation ported from the WebGL Fluid Simulation by Pavel
    Dobryakov (https://github.com/PavelDoGreat/WebGL-Fluid-Simulation, MIT).

    Faithful fragment-pass port of the original architecture: every pass of the
    incompressible Navier-Stokes solver (splat, vorticity confinement,
    divergence, pressure projection, gradient subtraction, semi-Lagrangian
    advection) plus a simplified bloom and the shading composite run as
    fullscreen-triangle GpuPipeline fragment shaders over ping-pong offscreen
    surfaces (GpuTarget - the render-pass surface a GpuCanvas is built on), and
    the final composite is presented straight from the GPU with
    Graphics::drawTexture(). There is no CPU readback or upload per frame.

    YUP's offscreen targets are 8-bit (rgba8unorm), while the original stores
    its simulation fields in half-float targets (the solver needs signed values
    with magnitudes well beyond [0, 1]). To keep numeric fidelity the sim
    fields are packed into the 8-bit channels as fixed point:

    - velocity: 2 x 16-bit fixed (RG bytes for vx, BA bytes for vy), range
      [-1000, 1000] texels/s - matching the vorticity confinement clamp and
      giving ~0.03 texels/s resolution, finer than float16 for small values.
    - divergence / curl / pressure: 24-bit fixed across RGB, range
      [-2048, 2048], ~0.00024 resolution.

    Encoded fields are sampled at exact texel centers (with a linear sampler a
    center sample is exact), and velocity sampling in advection decodes each of
    the four bilinear corner texels first - the equivalent of the original's
    MANUAL_FILTERING fallback. Dye and bloom buffers are plain 8-bit color.

    Bloom is simplified relative to the original (no half-float mip pyramid,
    which only accumulates 8-bit quantization): the dye is prefiltered with the
    original's threshold / soft-knee curve into a small surface (~1/8 dye size),
    blurred a couple of passes there, and the composite samples it with hardware
    linear filtering (the smooth upscale hides the 8-bit steps). An ordered
    (Bayer) dither in the final composite removes the remaining banding on slow
    fades.

    Features mirrored from the original:
    - Mouse splats that add both velocity (SPLAT_FORCE scaled) and dye
    - HSV color cycling (colorful mode) with burst splats (Space or button)
    - Vorticity confinement, Jacobi pressure iterations, PRESSURE damping
    - Semi-Lagrangian advection with dissipation
    - Bloom (threshold / soft-knee prefilter, small-surface blur, gamma add),
      fake shading from dye luminance gradients
    - Pause with P, burst splats with Space

    @see GpuPipeline, GpuRenderPass, GpuFrame, GpuTarget, GpuTexture
*/
class FluidSimulationDemo : public yup::Component
{
public:
    //==============================================================================
    FluidSimulationDemo()
        : yup::Component ("FluidSimulationDemo")
    {
        setWantsKeyboardFocus (true);

        statusLabel = std::make_unique<yup::Label> ("statusLabel");
        statusLabel->setText ("Initializing GPU fluid simulation...", yup::dontSendNotification);
        addAndMakeVisible (statusLabel.get());

        auto addToggle = [this] (yup::StringRef name, bool initialState)
        {
            auto button = std::make_unique<yup::ToggleButton> (name);
            button->setButtonText (name);
            button->setToggleState (initialState, yup::dontSendNotification);
            button->setWantsKeyboardFocus (false); // keep keyboard shortcuts on the demo
            addAndMakeVisible (button.get());
            return button;
        };

        pausedButton = addToggle ("Paused", false);
        pausedButton->onClick = [this] { config.paused = pausedButton->getToggleState(); };

        shadingButton = addToggle ("Shading", true);
        shadingButton->onClick = [this] { config.shading = shadingButton->getToggleState(); };

        bloomButton = addToggle ("Bloom", true);
        bloomButton->onClick = [this] { config.bloom = bloomButton->getToggleState(); };

        colorfulButton = addToggle ("Colorful", true);
        colorfulButton->onClick = [this] { config.colorful = colorfulButton->getToggleState(); };

        randomSplatsButton = std::make_unique<yup::TextButton> ("Random Splats");
        randomSplatsButton->setWantsKeyboardFocus (false);
        randomSplatsButton->onClick = [this] { queueRandomSplats (12); };
        addAndMakeVisible (randomSplatsButton.get());

        auto addSlider = [this] (double min, double max, double value, auto&& onChange)
        {
            auto slider = std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal);
            slider->setRange (min, max);
            slider->setValue (value);
            slider->setWantsKeyboardFocus (false);
            slider->onValueChanged = onChange;
            addAndMakeVisible (slider.get());
            return slider;
        };

        vorticityLabel = std::make_unique<yup::Label> ("vorticityLabel");
        vorticityLabel->setText ("Vorticity", yup::dontSendNotification);
        addAndMakeVisible (vorticityLabel.get());

        vorticitySlider = addSlider (0.0, 50.0, 30.0, [this] (double v)
        {
            config.curl = (float) v;
        });

        radiusLabel = std::make_unique<yup::Label> ("radiusLabel");
        radiusLabel->setText ("Splat", yup::dontSendNotification);
        addAndMakeVisible (radiusLabel.get());

        radiusSlider = addSlider (0.01, 1.0, 0.25, [this] (double v)
        {
            config.splatRadius = (float) v;
        });
    }

    ~FluidSimulationDemo() override = default;

    //==============================================================================
    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::Color (0xff05060a));
        g.fillAll();

        if (capturedContext == nullptr)
        {
            capturedContext = &g.getGraphicsContext();
            initGpu();
        }

        if (compiling)
        {
            pumpCompilation();

            if (compiling)
            {
                drawCompileProgress (g);
                return;
            }
        }

        if (! gpuReady)
            return;

        auto area = getFluidArea();
        if (area.getWidth() < 40.0f || area.getHeight() < 40.0f)
            return;

        const int viewW = yup::roundToInt (area.getWidth());
        const int viewH = yup::roundToInt (area.getHeight());

        if (! ensureSimulationSize (viewW, viewH))
            return;

        const auto now = yup::Time::getHighResolutionTicks();
        const float deltaTime = (float) yup::Time::highResolutionTicksToSeconds (now - lastFrameStamp);
        lastFrameStamp = now;
        const float dt = yup::jmin (deltaTime, 1.0f / 60.0f);

        auto frame = yup::GpuFrame::begin (device);
        if (! frame.isValid())
            return;

        updateColors (dt);
        applyInputs (frame);

        if (! config.paused)
            step (frame, dt);

        renderFrame (frame);

        frame.submit();

        if (displayTexture != nullptr)
            g.drawTexture (displayTexture, area);

        ++frameCount;
        ++statusFrames;
        statusAccumulator += (double) dt;
        if (statusAccumulator >= 0.5)
        {
            statusAccumulator = 0.0;
            updateStatus();
        }
    }

    //==============================================================================
    void resized() override
    {
        auto bounds = getLocalBounds().to<float>().reduced (10.0f);

        const float stripHeight = 20.0f + 3.0f + 24.0f + 3.0f + 24.0f + 3.0f;
        auto controls = bounds.removeFromBottom (stripHeight);

        statusLabel->setBounds (controls.removeFromTop (20.0f));
        controls.removeFromTop (3.0f);

        auto togglesRow = controls.removeFromTop (24.0f);
        controls.removeFromTop (3.0f);

        auto slidersRow = controls.removeFromTop (24.0f);

        const float toggleWidth = togglesRow.getWidth() / 5.0f;
        pausedButton->setBounds (togglesRow.removeFromLeft (toggleWidth));
        shadingButton->setBounds (togglesRow.removeFromLeft (toggleWidth));
        bloomButton->setBounds (togglesRow.removeFromLeft (toggleWidth));
        colorfulButton->setBounds (togglesRow.removeFromLeft (toggleWidth));
        randomSplatsButton->setBounds (togglesRow);

        vorticityLabel->setBounds (slidersRow.removeFromLeft (72.0f));
        vorticitySlider->setBounds (slidersRow.removeFromLeft (slidersRow.getWidth() * 0.42f));
        radiusLabel->setBounds (slidersRow.removeFromLeft (52.0f));
        radiusSlider->setBounds (slidersRow);
    }

    //==============================================================================
    void refreshDisplay (double /*lastFrameTimeSeconds*/) override
    {
        if (gpuReady || compiling)
            repaint();
    }

    //==============================================================================
    void keyDown (const yup::KeyPress& keys, const yup::Point<float>&) override
    {
        const int key = keys.getKey();

        if (key == yup::KeyPress::textPKey || key == 112) // P or p
        {
            config.paused = ! config.paused;
            pausedButton->setToggleState (config.paused, yup::dontSendNotification);
        }
        else if (key == yup::KeyPress::spaceKey)
        {
            queueRandomSplats (12);
        }
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        auto area = getFluidArea();
        if (! area.contains (event.getPosition()))
            return;

        takeKeyboardFocus();
        pointerDown (event.getPosition(), area);
    }

    void mouseDrag (const yup::MouseEvent& event) override
    {
        auto area = getFluidArea();
        if (! pointer.down)
            return;

        pointerMove (event.getPosition(), area);
    }

    void mouseUp (const yup::MouseEvent&) override
    {
        pointer.down = false;
    }

    void visibilityChanged() override
    {
        if (isVisible() && getParentComponent() != nullptr)
            takeKeyboardFocus();
    }

private:
    //==============================================================================
    // ---- Simulation configuration (defaults mirror the original) -------------

    struct Config
    {
        float simResolution = 128.0f;   // max side of the velocity/pressure grids
        float dyeResolution = 512.0f;   // max side of the dye grid
        float densityDissipation = 1.0f;
        float velocityDissipation = 0.2f;
        float pressure = 0.8f;
        int pressureIterations = 6;
        float curl = 30.0f;
        float splatRadius = 0.25f;
        float splatForce = 6000.0f;
        float bloomIntensity = 0.8f;
        float bloomThreshold = 0.6f;
        float bloomSoftKnee = 0.7f;
        bool shading = true;
        bool colorful = true;
        bool paused = false;
        bool bloom = true;
    };

    Config config;

    //==============================================================================
    // ---- Shaders ------------------------------------------------------------------
    // Every pass is a fluid_*.frag bundle precompiled from data/shaders, drawn with
    // the fullscreen triangle of fluid_fullscreen.vert.

    //==============================================================================
    // ---- Resource bookkeeping ----------------------------------------------------

    struct GridDims
    {
        int width = 0;
        int height = 0;

        bool operator== (const GridDims& other) const noexcept
        {
            return width == other.width && height == other.height;
        }

        bool operator!= (const GridDims& other) const noexcept
        {
            return ! operator== (other);
        }
    };

    struct CanvasPair
    {
        yup::GpuTarget::Ptr read;
        yup::GpuTarget::Ptr write;

        void swap() noexcept
        {
            std::swap (read, write);
        }
    };

    struct Pointer
    {
        bool down = false;
        bool moved = false;
        float texcoordX = 0.0f;
        float texcoordY = 0.0f;
        float prevTexcoordX = 0.0f;
        float prevTexcoordY = 0.0f;
        float deltaX = 0.0f;
        float deltaY = 0.0f;
        float colorR = 0.0f;
        float colorG = 0.0f;
        float colorB = 0.0f;
    };

    struct PendingSplat
    {
        float x, y, dx, dy;
        float r, g, b;
    };

    struct alignas (16) KernelParams
    {
        float data[16] = {};
    };

    struct TextureSlot
    {
        int binding;
        yup::GpuTexture::Ptr texture;
    };

    struct CompileJob
    {
        yup::String name;
        yup::GpuPipeline::Ptr* target = nullptr;
    };

    static constexpr int kCompilesPerFrame = 3;

    //==============================================================================
    // ---- GPU setup --------------------------------------------------------------

    void initGpu()
    {
        if (capturedContext == nullptr || device != nullptr)
            return;

        device = capturedContext->getGpuDevice();
        if (device == nullptr)
        {
            statusLabel->setText ("GPU device unavailable.", yup::dontSendNotification);
            return;
        }

        if (! capturedContext->isGpuAvailable())
        {
            statusLabel->setText ("GPU context unavailable.", yup::dontSendNotification);
            YUP_DBG ("FluidSimulationDemo: GPU context unavailable.");
            return;
        }

        // Queue all pipeline compiles; they run a few per frame in pumpCompilation().
        auto addJob = [this] (yup::StringRef name, yup::GpuPipeline::Ptr& target)
        {
            compileJobs.push_back ({ name, &target });
        };

        addJob ("fluid_clear", clearPipeline);
        addJob ("fluid_splat_velocity", splatVelocityPipeline);
        addJob ("fluid_splat_dye", splatDyePipeline);
        addJob ("fluid_curl", curlPipeline);
        addJob ("fluid_vorticity", vorticityPipeline);
        addJob ("fluid_pressure", pressurePipeline);
        addJob ("fluid_gradient_subtract", gradientSubtractPipeline);
        addJob ("fluid_advect_velocity", advectVelocityPipeline);
        addJob ("fluid_advect_dye", advectDyePipeline);
        addJob ("fluid_bloom_prefilter", bloomPrefilterPipeline);
        addJob ("fluid_blur4", blur4Pipeline);
        addJob ("fluid_display", displayPipeline);

        compileCursor = 0;
        compiling = true;
        statusLabel->setText ("Compiling shaders...", yup::dontSendNotification);
    }

    /** Compiles one queued job into its target pipeline member. */
    void compileJob (const CompileJob& job)
    {
        yup::GpuPipelineOptions options;
        options.topology = yup::GpuPrimitiveTopology::triangleList;
        options.cullMode = yup::GpuCullMode::none;
        options.colorTargets.emplace_back().blendEnabled = false; // passes overwrite every pixel

        auto result = compilePipelineFromBundle (device, job.name, options);

        if (result.failed())
        {
            YUP_DBG ("FluidSimulationDemo: pass '" << job.name << "' compile failed: " << result.getErrorMessage());
            lastCompileError = job.name + ": " + result.getErrorMessage();
            return;
        }

        *job.target = result.getValue();
    }

    void pumpCompilation()
    {
        if (! compiling)
            return;

        int remaining = kCompilesPerFrame;
        while (compiling && remaining-- > 0 && compileCursor < compileJobs.size())
        {
            compileJob (compileJobs[compileCursor]);
            ++compileCursor;
        }

        statusLabel->setText (yup::String::formatted ("Compiling shaders (%d/%d)...",
                                                      (int) compileCursor,
                                                      (int) compileJobs.size()),
                              yup::dontSendNotification);

        if (compileCursor >= compileJobs.size())
        {
            compiling = false;
            compileJobs.clear();
            compileCursor = 0;

            const bool coreOk = clearPipeline != nullptr
                && splatVelocityPipeline != nullptr
                && splatDyePipeline != nullptr
                && curlPipeline != nullptr
                && vorticityPipeline != nullptr
                && pressurePipeline != nullptr
                && gradientSubtractPipeline != nullptr
                && advectVelocityPipeline != nullptr
                && advectDyePipeline != nullptr
                && bloomPrefilterPipeline != nullptr
                && blur4Pipeline != nullptr
                && displayPipeline != nullptr;

            if (! coreOk)
            {
                statusLabel->setText ("Core fluid passes failed to compile: " + lastCompileError.substring (0, 80),
                                      yup::dontSendNotification);
                return;
            }

            bloomAvailable = bloomPrefilterPipeline != nullptr && blur4Pipeline != nullptr;

            gpuReady = true;
            statusLabel->setText ("GPU fluid simulation ready - drag to splat", yup::dontSendNotification);
        }
    }

    void drawCompileProgress (yup::Graphics& g)
    {
        auto area = getLocalBounds().to<float>().reduced (10.0f);
        const float barWidth = yup::jmin (560.0f, area.getWidth() * 0.7f);
        const float barHeight = 10.0f;

        auto bar = yup::Rectangle<float> ((area.getWidth() - barWidth) * 0.5f,
                                          (area.getHeight() - barHeight) * 0.5f,
                                          barWidth,
                                          barHeight);

        g.setFillColor (yup::Color (0xff1c1f26));
        g.fillRect (bar);

        const float fraction = compileJobs.empty() ? 1.0f
                                                   : (float) compileCursor / (float) compileJobs.size();
        auto fill = bar.withWidth (bar.getWidth() * fraction);
        g.setFillColor (yup::Color (0xff4f8cff));
        g.fillRect (fill);

        statusLabel->setText (yup::String::formatted ("Compiling shaders (%d/%d)...",
                                                      (int) compileCursor,
                                                      (int) compileJobs.size()),
                              yup::dontSendNotification);
    }

    //==============================================================================
    // ---- Grid & surface lifecycle --------------------------------------------------

    static GridDims makeGridDims (float resolution, float areaWidth, float areaHeight)
    {
        const float aspect = areaWidth / yup::jmax (areaHeight, 1.0f);
        const bool wide = aspect >= 1.0f;
        const float ratio = wide ? aspect : 1.0f / aspect;

        GridDims dims;
        if (wide)
        {
            dims.width = yup::roundToInt (resolution * ratio);
            dims.height = yup::roundToInt (resolution);
        }
        else
        {
            dims.width = yup::roundToInt (resolution);
            dims.height = yup::roundToInt (resolution * ratio);
        }

        const int maxSide = yup::roundToInt (resolution * 1.5f);
        const float scale = yup::jmin (1.0f, (float) maxSide / (float) yup::jmax (dims.width, dims.height, 1));
        dims.width = yup::jmax (8, yup::roundToInt ((float) dims.width * scale));
        dims.height = yup::jmax (8, yup::roundToInt ((float) dims.height * scale));
        return dims;
    }

    yup::GpuTarget::Ptr makeSurface (const GridDims& dims)
    {
        return yup::GpuTarget::create (device, dims.width, dims.height);
    }

    void clearSurface (yup::GpuFrame& frame, const yup::GpuTarget::Ptr& target, float r, float g, float b, float a)
    {
        if (target == nullptr || clearPipeline == nullptr)
            return;

        auto pass = target->beginRenderPass (frame, { false, yup::Colors::transparentBlack });
        if (! pass.isValid())
            return;

        KernelParams color {};
        color.data[0] = r;
        color.data[1] = g;
        color.data[2] = b;
        color.data[3] = a;

        pass.setPipeline (clearPipeline);
        pass.setUniformBuffer (0, 0, color.data, sizeof (color.data));
        pass.draw (3);
        pass.finish();
    }

    void allocateSimulation (const GridDims& simDims, const GridDims& dyeDims)
    {
        velocity.read = makeSurface (simDims);
        velocity.write = makeSurface (simDims);
        pressure.read = makeSurface (simDims);
        pressure.write = makeSurface (simDims);
        curl = makeSurface (simDims);

        dye.read = makeSurface (dyeDims);
        dye.write = makeSurface (dyeDims);
        displayCanvas = makeSurface (dyeDims);

        bloomDims.width = yup::jmax (8, dyeDims.width / 8);
        bloomDims.height = yup::jmax (8, dyeDims.height / 8);
        bloom = makeSurface (bloomDims);
        bloomTemp = makeSurface (bloomDims);

        const float velZero[4] = { 0.0f, 128.0f / 255.0f, 0.0f, 128.0f / 255.0f };
        const float scalarZero[4] = { 0.0f, 0.0f, 128.0f / 255.0f, 1.0f };
        const float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

        auto frame = yup::GpuFrame::begin (device);
        if (frame.isValid())
        {
            clearSurface (frame, velocity.read, velZero[0], velZero[1], velZero[2], velZero[3]);
            clearSurface (frame, velocity.write, velZero[0], velZero[1], velZero[2], velZero[3]);
            clearSurface (frame, pressure.read, scalarZero[0], scalarZero[1], scalarZero[2], scalarZero[3]);
            clearSurface (frame, pressure.write, scalarZero[0], scalarZero[1], scalarZero[2], scalarZero[3]);
            clearSurface (frame, curl, scalarZero[0], scalarZero[1], scalarZero[2], scalarZero[3]);
            clearSurface (frame, dye.read, black[0], black[1], black[2], black[3]);
            clearSurface (frame, dye.write, black[0], black[1], black[2], black[3]);
            clearSurface (frame, displayCanvas, black[0], black[1], black[2], black[3]);
            clearSurface (frame, bloom, black[0], black[1], black[2], black[3]);
            clearSurface (frame, bloomTemp, black[0], black[1], black[2], black[3]);

            frame.submit();
            frame.waitForGPU();
        }
    }

    bool ensureSimulationSize (int viewW, int viewH)
    {
        const GridDims simDims = makeGridDims (config.simResolution, (float) viewW, (float) viewH);
        const GridDims dyeDims = makeGridDims (config.dyeResolution, (float) viewW, (float) viewH);

        if (simDims == currentSimDims && dyeDims == currentDyeDims && ready)
            return true;

        currentSimDims = simDims;
        currentDyeDims = dyeDims;

        allocateSimulation (simDims, dyeDims);

        displayTexture = displayCanvas->asTexture();

        // Give the empty field something to show immediately.
        queueRandomSplats (12);
        ready = true;
        return true;
    }

    //==============================================================================
    // ---- Low level pass dispatch --------------------------------------------------

    bool runPass (yup::GpuFrame& frame,
                  const yup::GpuPipeline::Ptr& pipeline,
                  const yup::GpuTarget::Ptr& target,
                  const KernelParams& params,
                  std::initializer_list<TextureSlot> textures)
    {
        if (pipeline == nullptr || target == nullptr)
            return false;

        auto pass = target->beginRenderPass (frame, { false, yup::Colors::transparentBlack });
        if (! pass.isValid())
            return false;

        KernelParams uploaded = params;
        uploaded.data[15] = 1.0f;

        pass.setPipeline (pipeline);
        pass.setUniformBuffer (0, 0, uploaded.data, sizeof (uploaded.data));

        for (const auto& slot : textures)
        {
            if (slot.texture != nullptr)
                pass.setTexture (0, slot.binding, slot.texture);
        }

        pass.draw (3);
        pass.finish();
        return true;
    }

    //==============================================================================
    // ---- Simulation passes (mirror the original step()) --------------------------
    // Binding convention: UBO at (0,0); texture i at (0, 1 + 2*i) with its
    // auto-created sampler at (0, 2 + 2*i) - matches the shader declarations.

    static KernelParams gridParams (const GridDims& dims)
    {
        KernelParams params {};
        params.data[0] = (float) dims.width;
        params.data[1] = (float) dims.height;
        return params;
    }

    void splat (yup::GpuFrame& frame, float x, float y, float dx, float dy, float r, float g, float b)
    {
        const float aspect = (float) currentSimDims.width / (float) yup::jmax (currentSimDims.height, 1);
        float radius = config.splatRadius / 100.0f;
        if (aspect > 1.0f)
            radius *= aspect;

        // Velocity splat: inject the (dx, dy) impulse on the sim grid.
        {
            KernelParams params {};
            params.data[0] = (float) currentSimDims.width;
            params.data[1] = (float) currentSimDims.height;
            params.data[2] = aspect;
            params.data[3] = radius;
            params.data[4] = x;
            params.data[5] = y;
            params.data[6] = dx;
            params.data[7] = dy;

            runPass (frame, splatVelocityPipeline, velocity.write, params,
                     { { 1, velocity.read->asTexture() } });
            velocity.swap();
        }

        // Dye splat: inject the color on the dye grid.
        {
            KernelParams params {};
            params.data[0] = (float) currentDyeDims.width;
            params.data[1] = (float) currentDyeDims.height;
            params.data[2] = aspect;
            params.data[3] = radius;
            params.data[4] = x;
            params.data[5] = y;
            params.data[6] = r;
            params.data[7] = g;
            params.data[8] = b;

            runPass (frame, splatDyePipeline, dye.write, params,
                     { { 1, dye.read->asTexture() } });
            dye.swap();
        }
    }

    void step (yup::GpuFrame& frame, float dt)
    {
        // 1. Curl of the velocity field.
        runPass (frame, curlPipeline, curl,
                 gridParams (currentSimDims),
                 { { 1, velocity.read->asTexture() } });

        // 2. Vorticity confinement.
        {
            KernelParams params {};
            params.data[0] = (float) currentSimDims.width;
            params.data[1] = (float) currentSimDims.height;
            params.data[2] = config.curl;
            params.data[3] = dt;

            runPass (frame, vorticityPipeline, velocity.write, params,
                     { { 1, velocity.read->asTexture() }, { 3, curl->asTexture() } });
            velocity.swap();
        }

        // 3. Jacobi pressure projection iterations. The divergence is computed
        // inline from the (unchanged) velocity field, and the per-frame PRESSURE
        // damping is folded into the first iteration as an input scale.
        for (int i = 0; i < config.pressureIterations; ++i)
        {
            KernelParams params {};
            params.data[0] = (float) currentSimDims.width;
            params.data[1] = (float) currentSimDims.height;
            params.data[2] = (i == 0) ? config.pressure : 1.0f;

            runPass (frame, pressurePipeline, pressure.write, params,
                     { { 1, pressure.read->asTexture() }, { 3, velocity.read->asTexture() } });
            pressure.swap();
        }

        // 4. Gradient subtraction.
        runPass (frame, gradientSubtractPipeline, velocity.write,
                 gridParams (currentSimDims),
                 { { 1, pressure.read->asTexture() }, { 3, velocity.read->asTexture() } });
        velocity.swap();

        // 5. Advect velocity (self-advection) and dye.
        {
            KernelParams params {};
            params.data[0] = (float) currentSimDims.width;
            params.data[1] = (float) currentSimDims.height;
            params.data[2] = dt;
            params.data[3] = config.velocityDissipation;

            runPass (frame, advectVelocityPipeline, velocity.write, params,
                     { { 1, velocity.read->asTexture() } });
            velocity.swap();
        }
        {
            KernelParams params {};
            params.data[0] = (float) currentSimDims.width;
            params.data[1] = (float) currentSimDims.height;
            params.data[2] = (float) currentDyeDims.width;
            params.data[3] = (float) currentDyeDims.height;
            params.data[4] = dt;
            params.data[5] = config.densityDissipation;

            runPass (frame, advectDyePipeline, dye.write, params,
                     { { 1, dye.read->asTexture() }, { 3, velocity.read->asTexture() } });
            dye.swap();
        }
    }

    //==============================================================================
    // ---- Display composition ------------------------------------------------------

    static KernelParams blurParams (const GridDims& src)
    {
        KernelParams params {};
        params.data[0] = (float) src.width;
        params.data[1] = (float) src.height;
        return params;
    }

    void renderFrame (yup::GpuFrame& frame)
    {
        const bool doBloom = config.bloom && bloomAvailable;

        // Bloom: prefilter the dye into a small surface, smooth it with a couple
        // of 4-tap blurs, and let the composite upsample it with linear filtering.
        if (doBloom)
        {
            // Prefilter (original threshold / soft-knee curve).
            {
                KernelParams params {};
                const float knee = config.bloomThreshold * config.bloomSoftKnee + 0.0001f;
                params.data[0] = config.bloomThreshold;
                params.data[1] = config.bloomThreshold - knee;
                params.data[2] = knee * 2.0f;
                params.data[3] = 0.25f / knee;

                runPass (frame, bloomPrefilterPipeline, bloom, params,
                         { { 1, dye.read->asTexture() } });
            }

            // Two blur passes, ping-ponging between the small bloom surfaces.
            runPass (frame, blur4Pipeline, bloomTemp, blurParams (bloomDims),
                     { { 1, bloom->asTexture() } });
            runPass (frame, blur4Pipeline, bloom, blurParams (bloomDims),
                     { { 1, bloomTemp->asTexture() } });
        }

        // Final composite into the display surface.
        {
            KernelParams params {};
            params.data[0] = (float) currentDyeDims.width;
            params.data[1] = (float) currentDyeDims.height;
            params.data[2] = config.shading ? 1.0f : 0.0f;
            params.data[3] = doBloom ? 1.0f : 0.0f;
            params.data[4] = config.bloomIntensity;
            params.data[5] = 0.0f; // background color (black)
            params.data[6] = 0.0f;
            params.data[7] = 0.0f;

            runPass (frame, displayPipeline, displayCanvas, params,
                     { { 1, dye.read->asTexture() }, { 3, bloom->asTexture() } });
        }
    }

    //==============================================================================
    // ---- Input & colors -----------------------------------------------------------

    yup::Rectangle<float> getFluidArea() const
    {
        auto bounds = getLocalBounds().to<float>().reduced (10.0f);

        const float stripHeight = 20.0f + 3.0f + 24.0f + 3.0f + 24.0f + 3.0f;
        bounds.removeFromBottom (stripHeight);

        return bounds;
    }

    void pointerDown (yup::Point<float> pos, const yup::Rectangle<float>& area)
    {
        pointer.down = true;
        pointer.moved = false;
        pointer.texcoordX = (pos.getX() - area.getX()) / area.getWidth();
        pointer.texcoordY = pointerUvY (pos.getY() - area.getY(), area.getHeight());
        pointer.prevTexcoordX = pointer.texcoordX;
        pointer.prevTexcoordY = pointer.texcoordY;
        pointer.deltaX = 0.0f;
        pointer.deltaY = 0.0f;
        generateColor (pointer);
    }

    void pointerMove (yup::Point<float> pos, const yup::Rectangle<float>& area)
    {
        pointer.prevTexcoordX = pointer.texcoordX;
        pointer.prevTexcoordY = pointer.texcoordY;
        pointer.texcoordX = (pos.getX() - area.getX()) / area.getWidth();
        pointer.texcoordY = pointerUvY (pos.getY() - area.getY(), area.getHeight());

        const float aspect = area.getWidth() / yup::jmax (area.getHeight(), 1.0f);

        float dx = pointer.texcoordX - pointer.prevTexcoordX;
        float dy = pointer.texcoordY - pointer.prevTexcoordY;
        if (aspect < 1.0f) dx *= aspect;
        if (aspect > 1.0f) dy /= aspect;

        pointer.deltaX = dx;
        pointer.deltaY = dy;
        pointer.moved = std::abs (dx) > 0.0f || std::abs (dy) > 0.0f;
    }

    float pointerUvY (float y, float height) const
    {
        return 1.0f - y / yup::jmax (height, 1.0f);
    }

    void generateColor (Pointer& p)
    {
        auto [r, g, b] = hsvToRgb (rng.nextFloat(), 1.0f, 1.0f);
        p.colorR = r * 0.15f;
        p.colorG = g * 0.15f;
        p.colorB = b * 0.15f;
    }

    static std::tuple<float, float, float> hsvToRgb (float h, float s, float v)
    {
        const float i = std::floor (h * 6.0f);
        const float f = h * 6.0f - i;
        const float p = v * (1.0f - s);
        const float q = v * (1.0f - f * s);
        const float t = v * (1.0f - (1.0f - f) * s);

        switch ((int) i % 6)
        {
            case 0: return { v, t, p };
            case 1: return { q, v, p };
            case 2: return { p, v, t };
            case 3: return { p, q, v };
            case 4: return { t, p, v };
            default: return { v, p, q };
        }
    }

    void updateColors (float dt)
    {
        if (! config.colorful)
            return;

        colorUpdateTimer += dt * 10.0f; // COLOR_UPDATE_SPEED
        if (colorUpdateTimer >= 1.0f)
        {
            colorUpdateTimer = std::fmod (colorUpdateTimer, 1.0f);
            generateColor (pointer);
        }
    }

    void applyInputs (yup::GpuFrame& frame)
    {
        // Random burst splats queued by the button / space / resize.
        for (const auto& pending : pendingSplats)
            splat (frame, pending.x, pending.y, pending.dx, pending.dy, pending.r, pending.g, pending.b);
        pendingSplats.clear();

        // Pointer drag splats (splat only when the pointer actually moved).
        if (pointer.down && pointer.moved)
        {
            pointer.moved = false;
            splat (frame,
                   pointer.texcoordX,
                   pointer.texcoordY,
                   pointer.deltaX * config.splatForce,
                   pointer.deltaY * config.splatForce,
                   pointer.colorR,
                   pointer.colorG,
                   pointer.colorB);
        }
    }

    void queueRandomSplats (int amount)
    {
        if (! gpuReady || currentDyeDims.width == 0)
            return;

        for (int i = 0; i < amount; ++i)
        {
            auto [r, g, b] = hsvToRgb (rng.nextFloat(), 1.0f, 1.0f);

            PendingSplat splat;
            splat.x = rng.nextFloat();
            splat.y = rng.nextFloat();
            splat.dx = 1000.0f * (rng.nextFloat() - 0.5f);
            splat.dy = 1000.0f * (rng.nextFloat() - 0.5f);
            splat.r = r * 10.0f;
            splat.g = g * 10.0f;
            splat.b = b * 10.0f;
            pendingSplats.push_back (splat);
        }
    }

    //==============================================================================
    // ---- Misc helpers -------------------------------------------------------------

    void updateStatus()
    {
        const int fps = (int) ((double) statusFrames / yup::jmax (statusAccumulator, 0.001));
        statusFrames = 0;

        yup::String text;
        text << "GPU fluid | " << fps << " fps | " << currentSimDims.width << "x" << currentSimDims.height
             << " sim | " << currentDyeDims.width << "x" << currentDyeDims.height << " dye";
        statusLabel->setText (text, yup::dontSendNotification);
    }

    //==============================================================================
    yup::GraphicsContext* capturedContext = nullptr;
    yup::GpuDevice::Ptr device;

    // Pipelines (fullscreen-triangle fragment passes).
    yup::GpuPipeline::Ptr clearPipeline;
    yup::GpuPipeline::Ptr splatVelocityPipeline;
    yup::GpuPipeline::Ptr splatDyePipeline;
    yup::GpuPipeline::Ptr curlPipeline;
    yup::GpuPipeline::Ptr vorticityPipeline;
    yup::GpuPipeline::Ptr pressurePipeline;
    yup::GpuPipeline::Ptr gradientSubtractPipeline;
    yup::GpuPipeline::Ptr advectVelocityPipeline;
    yup::GpuPipeline::Ptr advectDyePipeline;
    yup::GpuPipeline::Ptr bloomPrefilterPipeline;
    yup::GpuPipeline::Ptr blur4Pipeline;
    yup::GpuPipeline::Ptr displayPipeline;

    yup::String lastCompileError;

    // Fields (rgba8 ping-pong surfaces; encoded fixed point as documented).
    CanvasPair velocity;
    CanvasPair dye;
    CanvasPair pressure;
    yup::GpuTarget::Ptr curl;

    // Bloom (small filtered surface, upsampled linearly in the composite).
    GridDims bloomDims;
    yup::GpuTarget::Ptr bloom;
    yup::GpuTarget::Ptr bloomTemp;

    // Display.
    GridDims currentSimDims;
    GridDims currentDyeDims;
    yup::GpuTarget::Ptr displayCanvas;
    yup::GpuTexture::Ptr displayTexture;

    // Compilation progress.
    std::vector<CompileJob> compileJobs;
    size_t compileCursor = 0;
    bool compiling = false;

    bool gpuReady = false;
    bool ready = false;
    bool bloomAvailable = false;

    // Timing / stats.
    yup::int64 lastFrameStamp = 0;
    int frameCount = 0;
    int statusFrames = 0;
    double statusAccumulator = 0.0;

    // Colors / input.
    float colorUpdateTimer = 0.0f;
    Pointer pointer;
    std::vector<PendingSplat> pendingSplats;
    yup::Random rng;

    // UI.
    std::unique_ptr<yup::Label> statusLabel;
    std::unique_ptr<yup::ToggleButton> pausedButton;
    std::unique_ptr<yup::ToggleButton> shadingButton;
    std::unique_ptr<yup::ToggleButton> bloomButton;
    std::unique_ptr<yup::ToggleButton> colorfulButton;
    std::unique_ptr<yup::TextButton> randomSplatsButton;
    std::unique_ptr<yup::Label> vorticityLabel;
    std::unique_ptr<yup::Slider> vorticitySlider;
    std::unique_ptr<yup::Label> radiusLabel;
    std::unique_ptr<yup::Slider> radiusSlider;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FluidSimulationDemo)
};
