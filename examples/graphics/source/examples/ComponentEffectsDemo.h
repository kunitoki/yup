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

#include <yup_shading/yup_shading.h>

//==============================================================================

/**
    Demonstrates ComponentEffect, snapshotToImage, and setCachedToTexture.

    A SlowBackground component renders an expensive radial pattern that changes
    once per second, cached via setCachedToTexture. An AnimatedPattern child
    spins on top every frame.

    Six shader effects selectable via ComboBox:
      - Blur        (separable Gaussian, 2-pass)
      - Pixelate    (block downsample)
      - Edge Detect (Sobel operator)
      - Wave        (sinusoidal displacement)
      - Sharpen     (unsharp mask kernel)
      - CRT Scan    (scanlines + vignette)

    Controls:
    - Effect combo: selects the active shader effect
    - Parameter slider: adjusts the selected effect's main parameter
    - Cache toggle: enables setCachedToTexture on the background
    - Snapshot button: captures a CPU-side Image
    - +Effects toggle: includes the active effect in the snapshot
    - Paint time label: shows last frame paint duration (subtree total)
*/
class ComponentEffectsDemo : public yup::Component
    , private yup::ComponentListener
{
public:
    //==============================================================================
    ComponentEffectsDemo()
        : yup::Component ("ComponentEffectsDemo")
    {
        effectCombo = std::make_unique<yup::ComboBox> ("effectCombo");
        effectCombo->addItem ("Blur", 1);
        effectCombo->addItem ("Pixelate", 2);
        effectCombo->addItem ("Edge Detect", 3);
        effectCombo->addItem ("Wave", 4);
        effectCombo->addItem ("Sharpen", 5);
        effectCombo->addItem ("CRT Scan", 6);
        effectCombo->setSelectedId (1, yup::dontSendNotification);
        effectCombo->onSelectedItemChanged = [this]
        {
            selectEffect();
        };
        addAndMakeVisible (effectCombo.get());

        paramSlider = std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal);
        paramSlider->setRange (0.0, 64.0);
        paramSlider->setValue (8.0);
        paramSlider->onValueChanged = [this] (double v)
        {
            effectParam = (float) v;
            if (shaderEffect != nullptr)
                shaderEffect->setEffectParameter (effectParam);
            background->repaint();
        };
        addAndMakeVisible (paramSlider.get());

        cacheToggle = std::make_unique<yup::ToggleButton> ("cacheToggle");
        cacheToggle->setButtonText ("Cache Bg");
        cacheToggle->onClick = [this]
        {
            background->setCachedToTexture (cacheToggle->getToggleState());
            updateStatusLabel();
        };
        addAndMakeVisible (cacheToggle.get());

        snapshotButton = std::make_unique<yup::TextButton> ("Take Snapshot");
        snapshotButton->onClick = [this]
        {
            takeSnapshot();
        };
        addAndMakeVisible (snapshotButton.get());

        snapshotEffectsToggle = std::make_unique<yup::ToggleButton> ("snapshotEffectsToggle");
        snapshotEffectsToggle->setButtonText ("+Effects");
        snapshotEffectsToggle->setToggleState (true, yup::dontSendNotification);
        addAndMakeVisible (snapshotEffectsToggle.get());

        statusLabel = std::make_unique<yup::Label> ("status");
        statusLabel->setText ("Effect: Blur, cache off", yup::dontSendNotification);
        addAndMakeVisible (statusLabel.get());

        paintTimeLabel = std::make_unique<yup::Label> ("paintTime");
        paintTimeLabel->setText ("Paint: -- us", yup::dontSendNotification);
        addAndMakeVisible (paintTimeLabel.get());

        background = std::make_unique<SlowBackground> ("Background");
        background->addComponentListener (this);
        addAndMakeVisible (background.get());

        spinner = std::make_unique<AnimatedPattern> ("Spinner");
        background->addAndMakeVisible (spinner.get());

        snapshotPreview = std::make_unique<SnapshotPreview> ("SnapshotPreview");
        addAndMakeVisible (snapshotPreview.get());

        selectEffect();
    }

    ~ComponentEffectsDemo() override
    {
        if (background != nullptr)
            background->removeComponentListener (this);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId)
                            .value_or (yup::Colors::dimgray));
        g.fillAll();

        if (capturedContext == nullptr)
            capturedContext = &g.getGraphicsContext();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().to<float>().reduced (10.0f);

        auto toolRow = bounds.removeFromTop (32.0f);
        effectCombo->setBounds (toolRow.removeFromLeft (130.0f).reduced (0.0f, 2.0f));
        toolRow.removeFromLeft (6.0f);
        paramSlider->setBounds (toolRow.removeFromLeft (180.0f).reduced (0.0f, 2.0f));
        toolRow.removeFromLeft (10.0f);
        cacheToggle->setBounds (toolRow.removeFromLeft (80.0f).reduced (0.0f, 2.0f));
        toolRow.removeFromLeft (8.0f);
        snapshotButton->setBounds (toolRow.removeFromLeft (120.0f).reduced (0.0f, 2.0f));
        toolRow.removeFromLeft (4.0f);
        snapshotEffectsToggle->setBounds (toolRow.removeFromLeft (60.0f).reduced (0.0f, 2.0f));

        auto statusRow = bounds.removeFromTop (24.0f);
        statusLabel->setBounds (statusRow.removeFromLeft (380.0f));
        paintTimeLabel->setBounds (statusRow);

        bounds.removeFromTop (6.0f);
        auto animBounds = bounds.removeFromLeft (bounds.getWidth() * 0.67f);
        background->setBounds (animBounds.reduced (4.0f));

        bounds.removeFromLeft (8.0f);
        snapshotPreview->setBounds (bounds);

        spinner->setBounds (background->getLocalBounds().to<float>().reduced (20.0f));
    }

    void componentPaintCompleted (yup::Component& component,
                                  const yup::ComponentPaintMetrics& metrics) override
    {
        if (&component != background.get())
            return;

        // The effect runs after this callback, so its timing is one frame behind.
        // Comparing the two numbers against the observed frame rate localises a
        // slow frame: if both stay small, the time is going to the GPU.
        const auto paintMicros = ticksToMicroseconds (metrics.totalTicks);
        const auto effectMicros = shaderEffect != nullptr ? ticksToMicroseconds (shaderEffect->getLastApplyTicks()) : 0;

        yup::MessageManager::callAsync ([this, paintMicros, effectMicros]
        {
            paintTimeLabel->setText ("Paint: " + yup::String (paintMicros)
                                         + " us, effect: " + yup::String (effectMicros) + " us",
                                     yup::dontSendNotification);
        });

        if (! compileErrorReported && shaderEffect != nullptr && shaderEffect->getCompileError().isNotEmpty())
        {
            compileErrorReported = true;

            yup::MessageManager::callAsync ([this, error = shaderEffect->getCompileError()]
            {
                statusLabel->setText ("Shader compile FAILED: " + error, yup::dontSendNotification);
            });
        }
    }

private:
    //==============================================================================
    /** Fullscreen-triangle vertex shader, shared by all effects. */
    static constexpr char kVertSource[] = R"glsl(#version 450
void main() {
    float x = float((gl_VertexIndex & 1u) << 2u) - 1.0;
    float y = float((gl_VertexIndex & 2u) << 1u) - 1.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
}
)glsl";

    /** Uniform block shared by all effects (layout matches std140). */
    struct EffectParams
    {
        float p0, p1, p2, p3, p4, p5, p6, p7;
    };

    //==============================================================================
    /** Base for the demo's shader effects.

        Owns the pipeline and compiles it at most once. Compiling GLSL runs the
        shader transpiler - and, on WebGPU, the GLSL→WGSL lowering on top of that -
        which costs tens of milliseconds, so a failed compile is remembered
        instead of retried: retrying every frame would silently cap the frame rate
        rather than just falling back to an unfiltered draw.

        Also records the CPU time spent inside apply(), so the demo can show
        whether a slow frame is spent on the CPU or waiting on the GPU.
    */
    class ShaderEffect : public yup::ComponentEffect
    {
    public:
        void setEffectParameter (float v) { param = v; }

        /** CPU ticks spent inside the most recent apply() call. */
        yup::int64 getLastApplyTicks() const noexcept { return applyTicks; }

        /** The error from the single pipeline compile attempt, empty while it succeeded. */
        const yup::String& getCompileError() const noexcept { return compileError; }

        void apply (yup::Graphics& g, yup::GpuTexture::Ptr input, yup::Rectangle<float> bounds) final
        {
            if (input == nullptr || ! input->isValid())
                return;

            const auto startTicks = yup::Time::getHighResolutionTicks();

            if (ensurePipeline (g.getGraphicsContext()))
                applyEffect (g, input, bounds);
            else
                g.drawTexture (input, bounds);

            applyTicks = yup::Time::getHighResolutionTicks() - startTicks;
        }

    protected:
        /** Returns this effect's fragment shader source. */
        virtual const char* getFragmentSource() const = 0;

        /** Renders the effect. Only called once the pipeline compiled successfully. */
        virtual void applyEffect (yup::Graphics& g, yup::GpuTexture::Ptr input, yup::Rectangle<float> bounds) = 0;

        float param = 8.0f;
        yup::GpuPipeline::Ptr pipeline;

    private:
        bool ensurePipeline (yup::GraphicsContext& ctx)
        {
            if (pipeline != nullptr)
                return true;

            if (compileAttempted)
                return false;

            compileAttempted = true;

            auto result = yup::GpuPipeline::compileFromGlsl (ctx.getGpuDevice(),
                                                             kVertSource,
                                                             yup::String::fromUTF8 (getFragmentSource()),
                                                             {});
            if (result.wasOk())
            {
                pipeline = result.getValue();
                return true;
            }

            compileError = result.getErrorMessage();
            yup::Logger::outputDebugString ("ComponentEffectsDemo: shader compile failed: " + compileError);
            return false;
        }

        bool compileAttempted = false;
        yup::String compileError;
        yup::int64 applyTicks = 0;
    };

    //==============================================================================
    /** Two-pass separable Gaussian blur. Parameter = sigma (0..64). */
    class BlurEffect : public ShaderEffect
    {
    protected:
        const char* getFragmentSource() const override { return kBlurFrag; }

        void applyEffect (yup::Graphics& g, yup::GpuTexture::Ptr input, yup::Rectangle<float> bounds) override
        {
            auto& ctx = g.getGraphicsContext();
            const int w = input->getWidth(), h = input->getHeight();
            const float blurR = std::ceil (param * 3.0f);

            if (! ensureTargets (ctx, w, h))
            {
                g.drawTexture (input, bounds);
                return;
            }

            auto frame = yup::GpuFrame::begin (ctx.getGpuDevice());
            auto pass = [&] (yup::GpuTarget& t, const yup::GpuTexture::Ptr& in, float dx, float dy)
            {
                EffectParams p { param, blurR, (float) w, (float) h, dx, dy, 0, 0 };
                auto rp = t.beginRenderPass (frame, { true, yup::Colors::transparentBlack });
                rp.setPipeline (pipeline);
                rp.setTexture (0, 0, in);
                rp.setUniformBuffer (0, 2, &p, sizeof (p));
                rp.draw (3);
                rp.finish();
                return t.asTexture();
            };
            auto out = pass (*targetA, input, 1.0f, 0.0f);
            out = pass (*targetB, out, 0.0f, 1.0f);
            frame.submit();
            if (out)
                g.drawTexture (out, bounds);
        }

    private:
        yup::GpuTarget::Ptr targetA, targetB;

        bool ensureTargets (yup::GraphicsContext& ctx, int w, int h)
        {
            if (! targetA || targetA->getWidth() != w || targetA->getHeight() != h)
                targetA = yup::GpuTarget::create (ctx.getGpuDevice(), w, h);
            if (! targetB || targetB->getWidth() != w || targetB->getHeight() != h)
                targetB = yup::GpuTarget::create (ctx.getGpuDevice(), w, h);
            return targetA != nullptr && targetB != nullptr;
        }

        static constexpr char kBlurFrag[] = R"glsl(#version 450
layout(set=0,binding=0) uniform texture2D u_tex;
layout(set=0,binding=1) uniform sampler u_samp;
layout(set=0,binding=2) uniform Params { float s,r,rx,ry,dx,dy,pad0,pad1; } p;
layout(location=0) out vec4 fragColor;
void main() {
    vec2 uv = gl_FragCoord.xy / vec2(p.rx, p.ry);
    if (p.s <= 0.0001) { fragColor = texture(sampler2D(u_tex,u_samp), uv); return; }
    int   r = int(clamp(p.r, 1.0, 128.0));
    vec2  step = vec2(p.dx, p.dy) / vec2(p.rx, p.ry);
    float inv2s2 = 0.5 / (p.s * p.s);
    vec4  sum = texture(sampler2D(u_tex,u_samp), uv);
    float wsum = 1.0;
    for (int i = 1; i <= r; ++i) {
        float w = exp(-float(i*i) * inv2s2);
        vec2  off = step * float(i);
        sum += texture(sampler2D(u_tex,u_samp), uv + off) * w;
        sum += texture(sampler2D(u_tex,u_samp), uv - off) * w;
        wsum += 2.0 * w;
    }
    fragColor = sum / wsum;
}
)glsl";
    };

    //==============================================================================
    /** Single-pass fullscreen effect base. Shared by Pixelate, Edge, Wave, Sharpen, CRT. */
    class SinglePassEffect : public ShaderEffect
    {
    protected:
        /** Fills the uniform block for this effect from the input dimensions. */
        virtual EffectParams getEffectParams (const yup::GpuTexture& input) const = 0;

        void applyEffect (yup::Graphics& g, yup::GpuTexture::Ptr input, yup::Rectangle<float> bounds) override
        {
            auto& ctx = g.getGraphicsContext();
            if (! ensureTarget (ctx, input->getWidth(), input->getHeight()))
            {
                g.drawTexture (input, bounds);
                return;
            }

            const auto p = getEffectParams (*input);

            auto frame = yup::GpuFrame::begin (ctx.getGpuDevice());
            {
                auto rp = target->beginRenderPass (frame, { true, yup::Colors::transparentBlack });
                rp.setPipeline (pipeline);
                rp.setTexture (0, 0, input);
                rp.setUniformBuffer (0, 2, &p, sizeof (p));
                rp.draw (3);
                rp.finish();
            }
            frame.submit();
            g.drawTexture (target->asTexture(), bounds);
        }

    private:
        yup::GpuTarget::Ptr target;

        bool ensureTarget (yup::GraphicsContext& ctx, int w, int h)
        {
            if (! target || target->getWidth() != w || target->getHeight() != h)
                target = yup::GpuTarget::create (ctx.getGpuDevice(), w, h);
            return target != nullptr;
        }
    };

    //==============================================================================
    /** Pixelate: downsamples into blocks. Parameter = block size (1..64). */
    class PixelateEffect : public SinglePassEffect
    {
    protected:
        const char* getFragmentSource() const override { return kPixelateFrag; }

        EffectParams getEffectParams (const yup::GpuTexture& input) const override
        {
            return { param, (float) input.getWidth(), (float) input.getHeight(), 0, 0, 0, 0, 0 };
        }

    private:
        static constexpr char kPixelateFrag[] = R"glsl(#version 450
layout(set=0,binding=0) uniform texture2D u_tex;
layout(set=0,binding=1) uniform sampler u_samp;
layout(set=0,binding=2) uniform Params { float bs,resX,resY,pad0,pad1,pad2,pad3,pad4; } p;
layout(location=0) out vec4 fragColor;
void main() {
    vec2 uv = gl_FragCoord.xy / vec2(p.resX, p.resY);
    float bs = max(1.0, p.bs);
    vec2 block = floor(uv * vec2(p.resX, p.resY) / bs) * bs;
    vec2 sampleUV = (block + 0.5 * bs) / vec2(p.resX, p.resY);
    fragColor = texture(sampler2D(u_tex, u_samp), sampleUV);
}
)glsl";
    };

    //==============================================================================
    /** Sobel edge detection. Parameter = threshold (0..64). */
    class EdgeEffect : public SinglePassEffect
    {
    protected:
        const char* getFragmentSource() const override { return kEdgeFrag; }

        EffectParams getEffectParams (const yup::GpuTexture& input) const override
        {
            return { param * 0.05f, (float) input.getWidth(), (float) input.getHeight(), 0, 0, 0, 0, 0 };
        }

    private:
        static constexpr char kEdgeFrag[] = R"glsl(#version 450
layout(set=0,binding=0) uniform texture2D u_tex;
layout(set=0,binding=1) uniform sampler u_samp;
layout(set=0,binding=2) uniform Params { float thr,resX,resY,pad0,pad1,pad2,pad3,pad4; } p;
layout(location=0) out vec4 fragColor;
void main() {
    vec2 uv = gl_FragCoord.xy / vec2(p.resX, p.resY);
    vec2 t = 1.0 / vec2(p.resX, p.resY);
    vec4 tl = texture(sampler2D(u_tex,u_samp), uv + vec2(-1,-1)*t);
    vec4 top = texture(sampler2D(u_tex,u_samp), uv + vec2(0,-1)*t);
    vec4 tr = texture(sampler2D(u_tex,u_samp), uv + vec2(1,-1)*t);
    vec4 lf = texture(sampler2D(u_tex,u_samp), uv + vec2(-1,0)*t);
    vec4 rt = texture(sampler2D(u_tex,u_samp), uv + vec2(1,0)*t);
    vec4 bl = texture(sampler2D(u_tex,u_samp), uv + vec2(-1,1)*t);
    vec4 bm = texture(sampler2D(u_tex,u_samp), uv + vec2(0,1)*t);
    vec4 br = texture(sampler2D(u_tex,u_samp), uv + vec2(1,1)*t);
    vec3 h = -tl.rgb - 2.0*top.rgb - tr.rgb + bl.rgb + 2.0*bm.rgb + br.rgb;
    vec3 v = -tl.rgb - 2.0*lf.rgb + tr.rgb - bl.rgb + 2.0*rt.rgb + br.rgb;
    float edge = length(h) + length(v) > p.thr ? 1.0 : 0.0;
    fragColor = vec4(vec3(edge), 1.0);
}
)glsl";
    };

    //==============================================================================
    /** Sinusoidal wave displacement. Parameter = amplitude (0..64). */
    class WaveEffect : public SinglePassEffect
    {
    public:
        WaveEffect() { param = 12.0f; }

    protected:
        const char* getFragmentSource() const override { return kWaveFrag; }

        EffectParams getEffectParams (const yup::GpuTexture& input) const override
        {
            return { param, 12.0f, (float) yup::Time::getMillisecondCounterHiRes() * 0.002f, (float) input.getWidth(), (float) input.getHeight(), 0, 0, 0 };
        }

    private:
        static constexpr char kWaveFrag[] = R"glsl(#version 450
layout(set=0,binding=0) uniform texture2D u_tex;
layout(set=0,binding=1) uniform sampler u_samp;
layout(set=0,binding=2) uniform Params { float amp,freq,time,resX,resY,pad0,pad1,pad2; } p;
layout(location=0) out vec4 fragColor;
void main() {
    vec2 uv = gl_FragCoord.xy / vec2(p.resX, p.resY);
    float aspect = p.resX / p.resY;
    vec2 center = uv - 0.5;
    float dist = length(center * vec2(aspect, 1.0));
    float offset = sin(dist * p.freq - p.time) * p.amp * 0.003;
    vec2 sampleUV = uv + normalize(center + 0.001) * offset;
    fragColor = texture(sampler2D(u_tex, u_samp), sampleUV);
}
)glsl";
    };

    //==============================================================================
    /** Unsharp-mask sharpen. Parameter = strength (0..64). */
    class SharpenEffect : public SinglePassEffect
    {
    public:
        SharpenEffect() { param = 4.0f; }

    protected:
        const char* getFragmentSource() const override { return kSharpenFrag; }

        EffectParams getEffectParams (const yup::GpuTexture& input) const override
        {
            return { param * 0.1f, (float) input.getWidth(), (float) input.getHeight(), 0, 0, 0, 0, 0 };
        }

    private:
        static constexpr char kSharpenFrag[] = R"glsl(#version 450
layout(set=0,binding=0) uniform texture2D u_tex;
layout(set=0,binding=1) uniform sampler u_samp;
layout(set=0,binding=2) uniform Params { float str,resX,resY,pad0,pad1,pad2,pad3,pad4; } p;
layout(location=0) out vec4 fragColor;
void main() {
    vec2 uv = gl_FragCoord.xy / vec2(p.resX, p.resY);
    vec2 t = 1.0 / vec2(p.resX, p.resY);
    vec4 c  = texture(sampler2D(u_tex,u_samp), uv);
    vec4 bl = c - 0.25 * (
        texture(sampler2D(u_tex,u_samp), uv + vec2(-1,-1)*t) +
        texture(sampler2D(u_tex,u_samp), uv + vec2( 0,-1)*t) +
        texture(sampler2D(u_tex,u_samp), uv + vec2( 1,-1)*t) +
        texture(sampler2D(u_tex,u_samp), uv + vec2(-1, 0)*t) +
        texture(sampler2D(u_tex,u_samp), uv + vec2( 1, 0)*t) +
        texture(sampler2D(u_tex,u_samp), uv + vec2(-1, 1)*t) +
        texture(sampler2D(u_tex,u_samp), uv + vec2( 0, 1)*t) +
        texture(sampler2D(u_tex,u_samp), uv + vec2( 1, 1)*t)) * 0.125;
    fragColor = mix(c, c + bl * p.str, 0.8);
}
)glsl";
    };

    //==============================================================================
    /** CRT scanline + vignette. Parameter = intensity (0..64). */
    class CRTScanEffect : public SinglePassEffect
    {
    public:
        CRTScanEffect() { param = 12.0f; }

    protected:
        const char* getFragmentSource() const override { return kCRTFrag; }

        EffectParams getEffectParams (const yup::GpuTexture& input) const override
        {
            return { param * 0.02f, (float) input.getWidth(), (float) input.getHeight(), 0, 0, 0, 0, 0 };
        }

    private:
        static constexpr char kCRTFrag[] = R"glsl(#version 450
layout(set=0,binding=0) uniform texture2D u_tex;
layout(set=0,binding=1) uniform sampler u_samp;
layout(set=0,binding=2) uniform Params { float intensity,resX,resY,pad0,pad1,pad2,pad3,pad4; } p;
layout(location=0) out vec4 fragColor;
void main() {
    vec2 uv = gl_FragCoord.xy / vec2(p.resX, p.resY);
    vec4 col = texture(sampler2D(u_tex, u_samp), uv);
    // Scanlines
    float scanline = sin(uv.y * p.resY * 1.2) * 0.5 + 0.5;
    col.rgb *= 1.0 - (1.0 - scanline) * p.intensity * 0.6;
    // Vignette
    vec2 v = uv - 0.5;
    col.rgb *= 1.0 - dot(v, v) * p.intensity * 0.8;
    // Slight green tint
    col.rgb *= vec3(0.95, 1.05, 0.9);
    fragColor = col;
}
)glsl";
    };

    //==============================================================================
    /** Background that repaints once per second. Heavy rendering benefits from caching. */
    class SlowBackground : public yup::Component
    {
    public:
        SlowBackground (yup::StringRef id)
            : yup::Component (id)
        {
        }

        void paint (yup::Graphics& g) override
        {
            auto bounds = getLocalBounds();
            const auto cx = bounds.getCenterX();
            const auto cy = bounds.getCenterY();
            const auto maxR = std::min (cx, cy);

            g.setFillColor (yup::Color (0xff0d0d1a));
            g.fillAll();

            for (int i = 0; i < 30; ++i)
            {
                const float t = static_cast<float> (i) / 30.0f;
                const float r = maxR * (0.15f + t * 0.85f);
                const float hue = std::fmod (slowPhase * 0.1f + t * 0.4f, 1.0f);
                const float alpha = 0.1f + 0.15f * (1.0f - t);
                yup::ColorGradient grad { yup::Color::fromHSV (hue, 0.6f, 0.3f, alpha), cx, cy, yup::Color::fromHSV (hue, 0.8f, 0.5f, alpha * 1.5f), cx + r, cy, yup::ColorGradient::Radial };
                g.setFillColorGradient (grad);
                g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
            }

            g.setStrokeColor (yup::Color (0x10ffffff));
            g.setStrokeWidth (1.0f);
            const float step = 40.0f;
            for (float x = 0; x < bounds.getWidth(); x += step)
                g.strokeLine (x, 0.0f, x, bounds.getHeight());
            for (float y = 0; y < bounds.getHeight(); y += step)
                g.strokeLine (0.0f, y, bounds.getWidth(), y);
        }

        void refreshDisplay (double /*last*/) override
        {
            const double now = yup::Time::getMillisecondCounterHiRes() * 0.001;
            if (now - lastUpdateTime >= 1.0)
            {
                lastUpdateTime = now;
                slowPhase += 0.3f;
                repaint();
            }
        }

    private:
        float slowPhase = 0.0f;
        double lastUpdateTime = 0.0;
    };

    //==============================================================================
    class AnimatedPattern : public yup::Component
    {
    public:
        AnimatedPattern (yup::StringRef id)
            : yup::Component (id)
        {
            phaseOffset = (float) std::fmod (yup::Time::getMillisecondCounterHiRes() * 0.001, 1.0);
        }

        void paint (yup::Graphics& g) override
        {
            auto bounds = getLocalBounds();
            const auto cx = bounds.getCenterX();
            const auto cy = bounds.getCenterY();
            const auto maxR = std::min (cx, cy) * 0.85f;

            for (int i = 5; i >= 0; --i)
            {
                const float t = phaseOffset + (float) i * 0.15f;
                const float r = maxR * (0.4f + 0.5f * (0.5f + 0.5f * std::sin (t * 6.28f)));
                const float hue = std::fmod (t * 0.3f + (float) i * 0.15f, 1.0f);
                g.setFillColor (yup::Color::fromHSV (hue, 0.7f, 0.9f, 0.6f + 0.4f * (float) i / 5.0f));
                g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
            }

            g.setStrokeColor (yup::Color (0xccffffff));
            g.setStrokeWidth (2.0f);
            for (int i = 0; i < 8; ++i)
            {
                const float a = (phaseOffset * 3.14f * 2.0f) + (float) i * 3.14f / 4.0f;
                g.strokeLine (cx + std::cos (a) * maxR * 0.9f, cy + std::sin (a) * maxR * 0.9f, cx - std::cos (a) * maxR * 0.3f, cy - std::sin (a) * maxR * 0.3f);
            }

            g.setFillColor (yup::Colors::white);
            g.fillEllipse (cx - 10.0f, cy - 10.0f, 20.0f, 20.0f);
        }

        void refreshDisplay (double /*last*/) override
        {
            phaseOffset = (float) std::fmod (yup::Time::getMillisecondCounterHiRes() * 0.001 * 0.5, 1.0);
            repaint();
        }

    private:
        float phaseOffset = 0.0f;
    };

    //==============================================================================
    class SnapshotPreview : public yup::Component
    {
    public:
        SnapshotPreview (yup::StringRef id)
            : yup::Component (id)
        {
        }

        void setImage (yup::Image img)
        {
            snapshotImage = std::move (img);
            repaint();
        }

        void paint (yup::Graphics& g) override
        {
            auto bounds = getLocalBounds();
            g.setFillColor (yup::Color (0xff2a2a3e));
            g.fillRoundedRect (bounds.to<float>(), 6.0f);

            if (snapshotImage.isValid() && snapshotImage.getWidth() > 0)
            {
                const auto iw = (float) snapshotImage.getWidth();
                const auto ih = (float) snapshotImage.getHeight();
                const auto s = std::min (bounds.getWidth() / iw, bounds.getHeight() / ih) * 0.9f;
                g.drawImage (snapshotImage, yup::Rectangle<float> (bounds.getCenterX() - iw * s * 0.5f, bounds.getCenterY() - ih * s * 0.5f, iw * s, ih * s));
            }
            else
            {
                g.setFillColor (yup::Color (0x88ffffff));
                auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (16.0f);
                g.fillFittedText ("Snapshot will\nappear here", font, bounds.to<float>().reduced (10.0f), yup::Justification::center);
            }
        }

    private:
        yup::Image snapshotImage;
    };

    //==============================================================================
    void selectEffect()
    {
        background->setComponentEffect (nullptr);
        activeEffect = nullptr;
        shaderEffect = nullptr;
        compileErrorReported = false;

        struct Choice
        {
            ShaderEffect* effect;
            float defaultParam;
        };

        const auto choice = [id = effectCombo->getSelectedId()]() -> Choice
        {
            switch (id)
            {
                case 1:
                    return { new BlurEffect(), 8.0f };
                case 2:
                    return { new PixelateEffect(), 8.0f };
                case 3:
                    return { new EdgeEffect(), 16.0f };
                case 4:
                    return { new WaveEffect(), 16.0f };
                case 5:
                    return { new SharpenEffect(), 8.0f };
                case 6:
                    return { new CRTScanEffect(), 12.0f };
                default:
                    return { nullptr, 8.0f };
            }
        }();

        if (choice.effect != nullptr)
        {
            shaderEffect = choice.effect;
            activeEffect = yup::ComponentEffect::Ptr (choice.effect);

            effectParam = choice.defaultParam;
            shaderEffect->setEffectParameter (effectParam);
            paramSlider->setValue ((double) effectParam, yup::dontSendNotification);
            background->setComponentEffect (activeEffect);
        }

        updateStatusLabel();
    }

    void takeSnapshot()
    {
        if (capturedContext == nullptr)
            return;

        const bool includeEffects = snapshotEffectsToggle->getToggleState();
        auto snapshot = background->snapshotToImage (*capturedContext, includeEffects);

        if (snapshot.isValid())
        {
            snapshotPreview->setImage (std::move (snapshot));
            const auto effectName = effectCombo->getText();
            yup::String msg = "Snapshot taken";
            if (includeEffects && activeEffect != nullptr)
                msg << " (" << effectName << ")";
            statusLabel->setText (msg, yup::dontSendNotification);
        }
        else
        {
            statusLabel->setText ("Snapshot failed.", yup::dontSendNotification);
        }
    }

    void updateStatusLabel()
    {
        yup::String text;
        text << effectCombo->getText() << ", cache " << (cacheToggle->getToggleState() ? "ON" : "off");
        statusLabel->setText (text, yup::dontSendNotification);
    }

    static int ticksToMicroseconds (yup::int64 ticks)
    {
        return static_cast<int> (ticks * 1000000.0 / yup::Time::getHighResolutionTicksPerSecond());
    }

    //==============================================================================
    yup::GraphicsContext* capturedContext = nullptr;

    std::unique_ptr<yup::ComboBox> effectCombo;
    std::unique_ptr<yup::Slider> paramSlider;
    std::unique_ptr<yup::ToggleButton> cacheToggle;
    std::unique_ptr<yup::TextButton> snapshotButton;
    std::unique_ptr<yup::ToggleButton> snapshotEffectsToggle;
    std::unique_ptr<yup::Label> statusLabel;
    std::unique_ptr<yup::Label> paintTimeLabel;

    std::unique_ptr<SlowBackground> background;
    std::unique_ptr<AnimatedPattern> spinner;
    std::unique_ptr<SnapshotPreview> snapshotPreview;

    yup::ComponentEffect::Ptr activeEffect;
    ShaderEffect* shaderEffect = nullptr; // Same object as activeEffect, kept for its stats.
    bool compileErrorReported = false;
    float effectParam = 8.0f;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComponentEffectsDemo)
};
