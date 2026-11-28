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
    Shows a component drawn through a ComponentEffect, a stylesheet `filter:` or a cached texture,
    with and without a paint margin.

    The subject, in the middle of the stage, paints inside its bounds and around them: a caption
    above, a count below and a dot off each corner. What it paints around its bounds shows only
    as far as its paint margin reaches, and it has to show the same whether the subject is painted
    directly, through an effect or from a cached texture. The guides mark its bounds (the solid
    line) and its paint bounds (the dashed one).

    The panel chooses:
    - the effect: none, a built-in one, a CSS filter chain, or one of the shaders of this demo
    - what a stylesheet adds to the subject: `shadow:`, `glow:` and `filter:`
    - the paint margin, still or moving
    - how the subject is rendered: cached to a texture, opaque, rotated, half a pixel off
    - a snapshot, which holds the bounds only

    The numbers at the bottom are what the effect was given: its bounds, the size of its texture
    and how many times that size changed.
*/
class ComponentEffectsDemo : public yup::Component
    , private yup::ComponentListener
{
public:
    //==============================================================================
    ComponentEffectsDemo()
        : yup::Component ("ComponentEffectsDemo")
    {
        addAndMakeVisible (stage);
        stage.subject.addComponentListener (this);

        for (const auto& effect : effects)
            effectCombo.addItem (effect.name, effect.id);

        effectCombo.setSelectedId (none, yup::dontSendNotification);
        effectCombo.onSelectedItemChanged = [this]
        {
            selectEffect();
        };
        addAndMakeVisible (effectCombo);

        parameterSlider.onValueChanged = [this] (double value)
        {
            setEffectParameter (static_cast<float> (value));
        };
        addAndMakeVisible (parameterSlider);
        addAndMakeVisible (parameterLabel);

        chainEditor.setMultiLine (false);
        chainEditor.setText ("hue-rotate(90deg) drop-shadow(0 6 8 #000000a0)", yup::dontSendNotification);
        chainEditor.onTextChange = [this]
        {
            if (getEffectInfo().id == filterChain)
                installEffect();
        };
        addAndMakeVisible (chainEditor);

        const auto restyle = [this]
        {
            applyStyle();
        };

        addToggle (shadowToggle, "shadow", false, restyle);
        addToggle (glowToggle, "glow", false, restyle);
        addToggle (filterToggle, "filter", false, restyle);

        marginSlider.setRange (0.0, 40.0);
        marginSlider.setValue (24.0, yup::dontSendNotification);
        marginSlider.onValueChanged = [this] (double value)
        {
            setSubjectMargin (static_cast<float> (value));
        };
        addAndMakeVisible (marginSlider);
        addAndMakeVisible (marginLabel);

        addToggle (movingMarginToggle, "Moving", false, [this]
        {
            if (! movingMarginToggle.getToggleState())
                setSubjectMargin (static_cast<float> (marginSlider.getValue()));
        });

        addToggle (cacheToggle, "Cache", false, [this]
        {
            stage.subject.setCachedToTexture (cacheToggle.getToggleState());
        });

        addToggle (opaqueToggle, "Opaque", false, [this]
        {
            stage.subject.setOpaque (opaqueToggle.getToggleState());
            stage.subject.repaint();
        });

        addToggle (guidesToggle, "Guides", true, [this]
        {
            stage.showGuides = guidesToggle.getToggleState();
            stage.repaint();
        });

        addToggle (animatedToggle, "Animated", true, [this]
        {
            stage.subject.setAnimated (animatedToggle.getToggleState());
        });

        addToggle (rotatedToggle, "Rotated", false, [this]
        {
            stage.rotated = rotatedToggle.getToggleState();
            stage.layoutSubject();
        });

        addToggle (halfPixelToggle, "Half px", false, [this]
        {
            stage.halfPixel = halfPixelToggle.getToggleState();
            stage.layoutSubject();
        });

        snapshotButton.setButtonText ("Take snapshot");
        snapshotButton.onClick = [this]
        {
            takeSnapshot();
        };
        addAndMakeVisible (snapshotButton);

        addToggle (snapshotEffectsToggle, "With effects", true, {});
        addAndMakeVisible (snapshotPreview);

        setSubjectMargin (static_cast<float> (marginSlider.getValue()));
        selectEffect();
    }

    ~ComponentEffectsDemo() override
    {
        stage.subject.removeComponentListener (this);
    }

    //==============================================================================
    void paint (yup::Graphics& g) override
    {
        const auto theme = yup::ApplicationTheme::getGlobalTheme();
        const auto& palette = theme->getPalette();

        g.setFillColor (palette.getColor (yup::ThemePalette::Role::background));
        g.fillAll();

        if (capturedContext == nullptr)
            capturedContext = &g.getGraphicsContext();

        g.setFillColor (palette.getColor (yup::ThemePalette::Role::surface));
        g.fillRoundedRect (panelArea, 8.0f);

        g.setFillColor (palette.getColor (yup::ThemePalette::Role::textMuted));

        for (size_t index = 0; index < sectionAreas.size(); ++index)
            g.fillFittedText (sectionTitles[index], theme->getDefaultFont().withHeight (11.0f), sectionAreas[index], yup::Justification::centerLeft);

        const auto font = theme->getDefaultFont().withHeight (12.0f);
        auto lines = measuredArea;

        g.setFillColor (palette.getColor (yup::ThemePalette::Role::text));

        for (const auto& line : measured)
            g.fillFittedText (line, font, lines.removeFromTop (measuredLineHeight), yup::Justification::centerLeft);

        g.setFillColor (palette.getColor (yup::ThemePalette::Role::accent));
        g.fillFittedText (status, font, statusArea, yup::Justification::centerLeft);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10.0f);

        panelArea = area.removeFromRight (yup::jmin (300.0f, area.getWidth() * 0.45f));
        area.removeFromRight (10.0f);
        stage.setBounds (area);

        auto column = panelArea.reduced (10.0f);

        const auto title = [&] (size_t section)
        {
            column.removeFromTop (4.0f);
            sectionAreas[section] = column.removeFromTop (18.0f);
        };

        const auto row = [&]
        {
            auto result = column.removeFromTop (24.0f);
            column.removeFromTop (4.0f);
            return result;
        };

        const auto withValue = [] (yup::Rectangle<float> bounds, yup::Component& control, yup::Component& value)
        {
            value.setBounds (bounds.removeFromRight (48.0f));
            bounds.removeFromRight (4.0f);
            control.setBounds (bounds);
        };

        const auto thirds = [] (yup::Rectangle<float> bounds, yup::Component& first, yup::Component& second, yup::Component& third)
        {
            const auto width = (bounds.getWidth() - 8.0f) / 3.0f;

            first.setBounds (bounds.removeFromLeft (width));
            bounds.removeFromLeft (4.0f);
            second.setBounds (bounds.removeFromLeft (width));
            bounds.removeFromLeft (4.0f);
            third.setBounds (bounds);
        };

        title (0);
        effectCombo.setBounds (row());
        withValue (row(), parameterSlider, parameterLabel);
        chainEditor.setBounds (row());

        title (1);
        thirds (row(), shadowToggle, glowToggle, filterToggle);

        title (2);
        withValue (row(), marginSlider, marginLabel);
        movingMarginToggle.setBounds (row().removeFromLeft (96.0f));

        title (3);
        thirds (row(), cacheToggle, opaqueToggle, guidesToggle);
        thirds (row(), animatedToggle, rotatedToggle, halfPixelToggle);

        // The numbers keep their place at the bottom, and the snapshot takes what is left
        statusArea = column.removeFromBottom (18.0f);
        measuredArea = column.removeFromBottom (measuredLineHeight * 6.0f);
        sectionAreas[5] = column.removeFromBottom (18.0f);

        title (4);

        auto snapshotRow = row();
        snapshotButton.setBounds (snapshotRow.removeFromLeft (snapshotRow.getWidth() * 0.5f - 2.0f));
        snapshotRow.removeFromLeft (4.0f);
        snapshotEffectsToggle.setBounds (snapshotRow);

        column.removeFromBottom (4.0f);
        snapshotPreview.setBounds (column);
    }

    void refreshDisplay (double elapsedSeconds)
    {
        time += elapsedSeconds;

        // A margin that moves: its canvas grows to the largest it gets and stays, as the numbers tell
        if (movingMarginToggle.getToggleState())
            applySubjectMargin (static_cast<float> (marginSlider.getValue() * (0.5 + 0.5 * std::sin (time * 2.0))));

        if (shaderEffect != nullptr && status.isEmpty() && shaderEffect->getCompileError().isNotEmpty())
            setStatus ("Shader compile failed: " + shaderEffect->getCompileError());

        measureCountdown -= elapsedSeconds;

        if (measureCountdown <= 0.0)
        {
            measureCountdown = 0.2;
            updateMeasured();
        }
    }

    void componentPaintCompleted (yup::Component& component, const yup::ComponentPaintMetrics& metrics) override
    {
        if (&component == &stage.subject)
            subjectPaintTicks = metrics.totalTicks;
    }

private:
    //==============================================================================
    enum EffectId
    {
        none = 1,
        builtInBlur,
        builtInDropShadow,
        builtInGlow,
        builtInGrayscale,
        filterChain,
        shaderBlur,
        shaderPixelate,
        shaderEdge,
        shaderWave,
        shaderSharpen,
        shaderCrt
    };

    /** An effect of the list, and the range of the one number it takes. */
    struct EffectInfo
    {
        EffectId id;
        const char* name;
        bool hasParameter;
        float minimum, maximum, initial;
    };

    static constexpr EffectInfo effects[] = {
        { none, "None", false, 0.0f, 1.0f, 0.0f },
        { builtInBlur, "BlurEffect", true, 0.0f, 24.0f, 6.0f },
        { builtInDropShadow, "DropShadowEffect", true, 0.0f, 24.0f, 8.0f },
        { builtInGlow, "GlowEffect", true, 0.0f, 24.0f, 10.0f },
        { builtInGrayscale, "ColorMatrixEffect, grayscale", true, 0.0f, 1.0f, 1.0f },
        { filterChain, "FilterEffect, CSS chain", false, 0.0f, 1.0f, 0.0f },
        { shaderBlur, "Shader: blur", true, 0.0f, 32.0f, 6.0f },
        { shaderPixelate, "Shader: pixelate", true, 1.0f, 32.0f, 8.0f },
        { shaderEdge, "Shader: edge detect", true, 0.0f, 64.0f, 16.0f },
        { shaderWave, "Shader: wave", true, 0.0f, 64.0f, 24.0f },
        { shaderSharpen, "Shader: sharpen", true, 0.0f, 64.0f, 8.0f },
        { shaderCrt, "Shader: CRT scan", true, 0.0f, 64.0f, 12.0f },
    };

    static constexpr const char* sectionTitles[] = { "EFFECT", "STYLESHEET", "PAINT MARGIN", "RENDERING", "SNAPSHOT, THE BOUNDS ONLY", "WHAT THE EFFECT IS GIVEN" };
    static constexpr float measuredLineHeight = 16.0f;

    //==============================================================================
    /** Uniform block shared by all effects (layout matches std140). */
    struct EffectParams
    {
        float p0, p1, p2, p3, p4, p5, p6, p7;
    };

    //==============================================================================
    /** Base for the demo's shader effects.

        Owns the pipeline and compiles it at most once. A failed compile is
        remembered instead of retried: retrying every frame would silently cap the
        frame rate rather than just falling back to an unfiltered draw.

        Also records the CPU time spent inside apply(), so the demo can show
        whether a slow frame is spent on the CPU or waiting on the GPU.
    */
    class ShaderEffect : public yup::ComponentEffect
    {
    public:
        void setEffectParameter (float v)
        {
            param = v;
            invalidate();
        }

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

        /** The pipeline is the one of the device of the window the component leaves: the next apply() compiles it again. */
        void releaseResources() override
        {
            pipeline = nullptr;
            compileAttempted = false;
        }

    protected:
        /** Returns the name of this effect's shader bundle. */
        virtual const char* getShaderName() const = 0;

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

            auto result = compilePipelineFromBundle (ctx.getGpuDevice(), getShaderName());
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
    class ShaderBlurEffect : public ShaderEffect
    {
    public:
        void releaseResources() override
        {
            ShaderEffect::releaseResources();
            targetA = targetB = nullptr;
        }

    protected:
        const char* getShaderName() const override { return "effect_blur"; }

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
    };

    //==============================================================================
    /** Single-pass fullscreen effect base. Shared by Pixelate, Edge, Wave, Sharpen, CRT. */
    class SinglePassEffect : public ShaderEffect
    {
    public:
        void releaseResources() override
        {
            ShaderEffect::releaseResources();
            target = nullptr;
        }

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
        const char* getShaderName() const override { return "effect_pixelate"; }

        EffectParams getEffectParams (const yup::GpuTexture& input) const override
        {
            return { param, (float) input.getWidth(), (float) input.getHeight(), 0, 0, 0, 0, 0 };
        }
    };

    //==============================================================================
    /** Sobel edge detection. Parameter = threshold (0..64). */
    class EdgeEffect : public SinglePassEffect
    {
    protected:
        const char* getShaderName() const override { return "effect_edge"; }

        EffectParams getEffectParams (const yup::GpuTexture& input) const override
        {
            return { param * 0.05f, (float) input.getWidth(), (float) input.getHeight(), 0, 0, 0, 0, 0 };
        }
    };

    //==============================================================================
    /** Sinusoidal wave displacement. Parameter = amplitude (0..64).

        The effect also maps pointer positions the same way the shader samples the texture,
        so clicks reach the widget displayed under the pointer while it waves.
    */
    class WaveEffect : public SinglePassEffect
    {
    public:
        WaveEffect() { param = 12.0f; }

        std::optional<yup::Point<float>> displayToContent (yup::Point<float> displayPoint, yup::Rectangle<float> bounds) const override
        {
            if (bounds.isEmpty())
                return std::nullopt;

            const auto uv = (displayPoint - bounds.getTopLeft()) / yup::Point<float> (bounds.getWidth(), bounds.getHeight());
            return bounds.getTopLeft() + getSampleUV (uv, bounds.getWidth() / bounds.getHeight()) * yup::Point<float> (bounds.getWidth(), bounds.getHeight());
        }

        std::optional<yup::Point<float>> contentToDisplay (yup::Point<float> contentPoint, yup::Rectangle<float> bounds) const override
        {
            // The displacement is small and smooth, so a few fixed point steps invert it
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

    protected:
        const char* getShaderName() const override { return "effect_wave"; }

        EffectParams getEffectParams (const yup::GpuTexture& input) const override
        {
            // Published for the input mapping, which runs on the message thread
            publishedAmplitude = param;
            // The counter runs since boot: wrap the phase in double precision, a float of it
            // only has a resolution of hundreds of milliseconds after a few days of uptime
            publishedTime = static_cast<float> (std::fmod (yup::Time::getMillisecondCounterHiRes() * 0.002, yup::MathConstants<double>::twoPi));

            return { publishedAmplitude, frequency, publishedTime, (float) input.getWidth(), (float) input.getHeight(), 0, 0, 0 };
        }

    private:
        /** Mirrors the sample coordinate computed by effect_wave.frag. */
        yup::Point<float> getSampleUV (yup::Point<float> uv, float aspect) const
        {
            const auto center = uv - yup::Point<float> (0.5f, 0.5f);
            const auto dist = std::hypot (center.getX() * aspect, center.getY());
            const auto offset = std::sin (dist * frequency - publishedTime.load()) * publishedAmplitude.load() * 0.003f;

            return uv + (center + yup::Point<float> (0.001f, 0.001f)).normalized() * offset;
        }

        static constexpr float frequency = 12.0f;

        mutable std::atomic<float> publishedAmplitude { 0.0f };
        mutable std::atomic<float> publishedTime { 0.0f };
    };

    //==============================================================================
    /** Unsharp-mask sharpen. Parameter = strength (0..64). */
    class SharpenEffect : public SinglePassEffect
    {
    public:
        SharpenEffect() { param = 4.0f; }

    protected:
        const char* getShaderName() const override { return "effect_sharpen"; }

        EffectParams getEffectParams (const yup::GpuTexture& input) const override
        {
            return { param * 0.1f, (float) input.getWidth(), (float) input.getHeight(), 0, 0, 0, 0, 0 };
        }
    };

    //==============================================================================
    /** CRT scanline + vignette. Parameter = intensity (0..64). */
    class CRTScanEffect : public SinglePassEffect
    {
    public:
        CRTScanEffect() { param = 12.0f; }

    protected:
        const char* getShaderName() const override { return "effect_crt"; }

        EffectParams getEffectParams (const yup::GpuTexture& input) const override
        {
            return { param * 0.02f, (float) input.getWidth(), (float) input.getHeight(), 0, 0, 0, 0, 0 };
        }
    };

    //==============================================================================
    /** Wraps the effect of the subject, and keeps what it was given.

        It is written while painting and read between two frames, and the message thread is
        locked for both, so nothing here is atomic.
    */
    class ProbeEffect : public yup::ComponentEffect
    {
    public:
        explicit ProbeEffect (yup::ComponentEffect::Ptr effectToMeasure)
            : effect (std::move (effectToMeasure))
        {
        }

        void apply (yup::Graphics& g, yup::GpuTexture::Ptr input, yup::Rectangle<float> bounds) override
        {
            const auto startTicks = yup::Time::getHighResolutionTicks();

            if (input != nullptr)
            {
                const yup::Size<int> size (input->getWidth(), input->getHeight());

                if (size != textureSize)
                {
                    textureSize = size;
                    ++textureSizeChanges;
                }
            }

            appliedBounds = bounds;
            effect->apply (g, std::move (input), bounds);

            applyTicks = yup::Time::getHighResolutionTicks() - startTicks;
        }

        bool isActive() const override
        {
            return effect->isActive();
        }

        void releaseResources() override
        {
            effect->releaseResources();
        }

        std::optional<yup::Point<float>> displayToContent (yup::Point<float> displayPoint, yup::Rectangle<float> bounds) const override
        {
            return effect->displayToContent (displayPoint, bounds);
        }

        std::optional<yup::Point<float>> contentToDisplay (yup::Point<float> contentPoint, yup::Rectangle<float> bounds) const override
        {
            return effect->contentToDisplay (contentPoint, bounds);
        }

        yup::Rectangle<float> appliedBounds;
        yup::Size<int> textureSize;
        int textureSizeChanges = 0;
        yup::int64 applyTicks = 0;

    private:
        yup::ComponentEffect::Ptr effect;
    };

    //==============================================================================
    /** Spins in the subject, repainting on every frame. */
    class Spinner : public yup::Component
    {
    public:
        Spinner()
            : yup::Component ("spinner")
        {
            setOpaque (false);
        }

        void paint (yup::Graphics& g) override
        {
            const auto bounds = getLocalBounds();
            const auto cx = bounds.getCenterX();
            const auto cy = bounds.getCenterY();
            const auto maxR = yup::jmin (cx, cy) * 0.9f;

            for (int i = 4; i >= 0; --i)
            {
                const auto t = phase + static_cast<float> (i) * 0.15f;
                const auto r = maxR * (0.35f + 0.5f * (0.5f + 0.5f * std::sin (t * yup::MathConstants<float>::twoPi)));

                g.setFillColor (yup::Color::fromHSV (std::fmod (t * 0.3f + static_cast<float> (i) * 0.15f, 1.0f), 0.7f, 0.9f, 0.5f + 0.1f * static_cast<float> (i)));
                g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
            }

            g.setStrokeColor (yup::Color (0xccffffff));
            g.setStrokeWidth (2.0f);

            for (int i = 0; i < 8; ++i)
            {
                const auto angle = (phase + static_cast<float> (i) / 8.0f) * yup::MathConstants<float>::twoPi;
                g.strokeLine (cx + std::cos (angle) * maxR * 0.9f, cy + std::sin (angle) * maxR * 0.9f, cx - std::cos (angle) * maxR * 0.3f, cy - std::sin (angle) * maxR * 0.3f);
            }
        }

        void refreshDisplay (double)
        {
            if (! animated)
                return;

            phase = static_cast<float> (std::fmod (yup::Time::getMillisecondCounterHiRes() * 0.0005, 1.0));
            repaint();
        }

        bool animated = true;

    private:
        float phase = 0.0f;
        yup::FrameAttachment frameAttachment { *this, yup::bindFront (&Spinner::refreshDisplay, this) };
    };

    //==============================================================================
    /** What the effects, the filters and the cache are applied to.

        It paints a card in its bounds, changing once a second, and around them a caption, the
        number of times it was painted and a dot off each corner, as far as 22 units away. Its
        widgets take the pointer where they are displayed, whatever the effect does to them.
    */
    class Subject : public yup::Component
    {
    public:
        Subject()
            : yup::Component ("subject")
        {
            setOpaque (false);

            addAndMakeVisible (spinner);

            button.setButtonText ("Click me");
            button.onClick = [this]
            {
                button.setButtonText ("Clicked " + yup::String (++clicks) + "x");
            };
            addAndMakeVisible (button);

            slider.setRange (0.0, 1.0);
            slider.setValue (0.5, yup::dontSendNotification);
            addAndMakeVisible (slider);
        }

        void setAnimated (bool shouldBeAnimated)
        {
            animated = shouldBeAnimated;
            spinner.animated = shouldBeAnimated;
        }

        void paint (yup::Graphics& g) override
        {
            ++paintCount;

            const auto theme = yup::ApplicationTheme::getGlobalTheme();
            const auto& palette = theme->getPalette();
            const auto bounds = getLocalBounds();
            const auto cx = bounds.getCenterX();
            const auto cy = bounds.getCenterY();
            const auto maxR = yup::jmin (cx, cy) - 4.0f;

            // An opaque component paints all of its bounds, so it has no rounded corners to show through
            g.setFillColor (yup::Color (0xff12162a));

            if (isOpaque())
                g.fillAll();
            else
                g.fillRoundedRect (bounds, cornerRadius);

            for (int i = 0; i < 14; ++i)
            {
                const auto t = static_cast<float> (i) / 14.0f;
                const auto r = maxR * (0.2f + t * 0.8f);
                const auto hue = std::fmod (phase * 0.1f + t * 0.4f, 1.0f);
                const auto alpha = 0.12f + 0.18f * (1.0f - t);

                yup::ColorGradient gradient { yup::Color::fromHSV (hue, 0.6f, 0.35f, alpha), cx, cy, yup::Color::fromHSV (hue, 0.8f, 0.6f, alpha * 1.5f), cx + r, cy, yup::ColorGradient::Radial };
                g.setFillColorGradient (gradient);
                g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
            }

            g.setStrokeColor (palette.getColor (yup::ThemePalette::Role::outline));
            g.setStrokeWidth (1.0f);

            if (isOpaque())
                g.strokeRect (bounds.reduced (0.5f));
            else
                g.strokeRoundedRect (bounds.reduced (0.5f), cornerRadius);

            // Around the bounds: this shows as far as the paint margin reaches
            const auto font = theme->getDefaultFont().withHeight (12.0f);

            g.setFillColor (palette.getColor (yup::ThemePalette::Role::text));
            g.fillFittedText ("painted above the bounds", font, yup::Rectangle<float> (0.0f, -22.0f, bounds.getWidth(), 16.0f), yup::Justification::center);
            g.fillFittedText ("painted " + yup::String (paintCount) + " times", font, yup::Rectangle<float> (0.0f, bounds.getHeight() + 6.0f, bounds.getWidth(), 16.0f), yup::Justification::center);

            g.setFillColor (palette.getColor (yup::ThemePalette::Role::accent));

            for (const auto corner : { bounds.getTopLeft(), bounds.getTopRight(), bounds.getBottomLeft(), bounds.getBottomRight() })
            {
                const auto dot = corner + yup::Point<float> (corner.getX() > 0.0f ? 12.0f : -12.0f, corner.getY() > 0.0f ? 12.0f : -12.0f);
                g.fillEllipse (dot.getX() - 4.0f, dot.getY() - 4.0f, 8.0f, 8.0f);
            }
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced (14.0f);

            auto controls = area.removeFromBottom (28.0f);
            button.setBounds (controls.removeFromLeft (yup::jmin (110.0f, controls.getWidth() * 0.4f)));
            controls.removeFromLeft (10.0f);
            slider.setBounds (controls);

            area.removeFromBottom (8.0f);
            spinner.setBounds (area);
        }

        void refreshDisplay (double)
        {
            if (! animated)
                return;

            const auto now = yup::Time::getMillisecondCounterHiRes() * 0.001;

            if (now - lastChangeSeconds >= 1.0)
            {
                lastChangeSeconds = now;
                phase += 0.3f;
                repaint();
            }
        }

        static constexpr float cornerRadius = 14.0f;

        int paintCount = 0;

    private:
        Spinner spinner;
        yup::TextButton button { "button" };
        yup::Slider slider { yup::Slider::LinearHorizontal, "slider" };

        bool animated = true;
        float phase = 0.0f;
        double lastChangeSeconds = 0.0;
        int clicks = 0;
        yup::FrameAttachment frameAttachment { *this, yup::bindFront (&Subject::refreshDisplay, this) };
    };

    //==============================================================================
    /** Holds the subject over a checkerboard, so what is transparent shows, and draws the guides. */
    class Stage : public yup::Component
    {
    public:
        Stage()
            : yup::Component ("stage")
        {
            addAndMakeVisible (subject);
        }

        void layoutSubject()
        {
            const auto width = yup::jlimit (160.0f, 380.0f, getWidth() - 160.0f);
            const auto height = yup::jlimit (120.0f, 250.0f, getHeight() - 180.0f);

            // Half a unit off is half a pixel off on a display of scale 1: the texture of an effect
            // is then drawn between two pixels
            const auto offset = halfPixel ? 0.5f : 0.0f;

            subject.setBounds (std::round ((getWidth() - width) * 0.5f) + offset, std::round ((getHeight() - height) * 0.5f) + offset, width, height);
            subject.setTransform (rotated ? yup::AffineTransform::rotation (yup::degreesToRadians (8.0f), width * 0.5f, height * 0.5f)
                                          : yup::AffineTransform());
            repaint();
        }

        void paint (yup::Graphics& g) override
        {
            const auto& palette = yup::ApplicationTheme::getGlobalTheme()->getPalette();

            g.setFillColor (palette.getColor (yup::ThemePalette::Role::surface));
            g.fillAll();

            g.setFillColor (palette.getColor (yup::ThemePalette::Role::surfaceRaised));
            g.fillPath (checker);
        }

        void paintOverChildren (yup::Graphics& g) override
        {
            if (! showGuides)
                return;

            const auto toStage = subject.getCombinedTransform().translated (subject.getPosition());

            yup::Path bounds, paintBounds;
            bounds.addRectangle (subject.getLocalBounds());
            paintBounds.addRectangle (subject.getPaintBounds());

            const float dashes[] = { 4.0f, 4.0f };

            g.setStrokeColor (yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (yup::ThemePalette::Role::accent));
            g.setStrokeWidth (1.0f);
            g.strokePath (bounds.transformed (toStage));
            g.strokePath (paintBounds.transformed (toStage).createDashed ({ dashes, 2 }));
        }

        void resized() override
        {
            const auto cell = 16.0f;

            checker.clear();

            for (int row = 0; static_cast<float> (row) * cell < getHeight(); ++row)
                for (int column = row % 2; static_cast<float> (column) * cell < getWidth(); column += 2)
                    checker.addRectangle ({ static_cast<float> (column) * cell, static_cast<float> (row) * cell, cell, cell });

            layoutSubject();
        }

        Subject subject;
        bool showGuides = true;
        bool rotated = false;
        bool halfPixel = false;

    private:
        yup::Path checker;
    };

    //==============================================================================
    class SnapshotPreview : public yup::Component
    {
    public:
        SnapshotPreview()
            : yup::Component ("snapshotPreview")
        {
            setOpaque (false);
        }

        void setImage (yup::Image imageToShow)
        {
            image = std::move (imageToShow);
            repaint();
        }

        void paint (yup::Graphics& g) override
        {
            const auto bounds = getLocalBounds();
            const auto& palette = yup::ApplicationTheme::getGlobalTheme()->getPalette();

            g.setFillColor (palette.getColor (yup::ThemePalette::Role::background));
            g.fillRoundedRect (bounds, 6.0f);

            if (! image.isValid() || image.getWidth() <= 0 || bounds.isEmpty())
                return;

            const auto width = static_cast<float> (image.getWidth());
            const auto height = static_cast<float> (image.getHeight());
            const auto scale = yup::jmin (bounds.getWidth() / width, bounds.getHeight() / height) * 0.9f;

            g.drawImage (image, yup::Rectangle<float> (bounds.getCenterX() - width * scale * 0.5f, bounds.getCenterY() - height * scale * 0.5f, width * scale, height * scale));
        }

    private:
        yup::Image image;
    };

    //==============================================================================
    static yup::String number (float value)
    {
        return yup::String (value, 1);
    }

    static yup::String describe (const yup::EdgeSpacing<float>& edges)
    {
        return number (edges.getLeft()) + "  " + number (edges.getTop()) + "  " + number (edges.getRight()) + "  " + number (edges.getBottom());
    }

    static yup::String describe (const yup::Rectangle<float>& area)
    {
        return number (area.getX()) + ", " + number (area.getY()) + "   " + number (area.getWidth()) + " x " + number (area.getHeight());
    }

    static int ticksToMicroseconds (yup::int64 ticks)
    {
        return static_cast<int> (static_cast<double> (ticks) * 1000000.0 / static_cast<double> (yup::Time::getHighResolutionTicksPerSecond()));
    }

    void addToggle (yup::ToggleButton& toggle, const char* text, bool isOn, std::function<void()> onClick)
    {
        toggle.setButtonText (text);
        toggle.setToggleState (isOn, yup::dontSendNotification);
        toggle.onClick = std::move (onClick);
        addAndMakeVisible (toggle);
    }

    void setStatus (const yup::String& newStatus)
    {
        if (status == newStatus)
            return;

        status = newStatus;
        repaint (statusArea);
    }

    const EffectInfo& getEffectInfo() const
    {
        for (const auto& effect : effects)
            if (effect.id == effectCombo.getSelectedId())
                return effect;

        return effects[0];
    }

    //==============================================================================
    void selectEffect()
    {
        const auto& info = getEffectInfo();

        parameterSlider.setRange (info.minimum, info.maximum);
        parameterSlider.setValue (info.initial, yup::dontSendNotification);
        parameterSlider.setEnabled (info.hasParameter);
        parameterLabel.setText (info.hasParameter ? number (info.initial) : yup::String(), yup::dontSendNotification);
        chainEditor.setEnabled (info.id == filterChain);

        installEffect();
    }

    void installEffect()
    {
        setStatus ({});

        auto effect = createEffect (static_cast<float> (parameterSlider.getValue()));

        if (effect != nullptr)
        {
            probe = new ProbeEffect (std::move (effect));
            stage.subject.setComponentEffect (probe);
        }
        else
        {
            probe = nullptr;
            stage.subject.setComponentEffect (nullptr);
        }

        describeSpread();
    }

    void setEffectParameter (float value)
    {
        parameterLabel.setText (number (value), yup::dontSendNotification);

        if (setParameter == nullptr)
            return;

        setParameter (value);

        // The effect measured is set on no component, the probe is: the subject is drawn through it again, without
        // being painted again
        if (probe != nullptr)
            probe->invalidate();

        describeSpread();
    }

    /** Creates the chosen effect, with the function that gives it its number. */
    yup::ComponentEffect::Ptr createEffect (float value)
    {
        setParameter = nullptr;
        getSpread = nullptr;
        shaderEffect = nullptr;

        // A blur spreads three times its radius on every side, the reach of its kernel
        const auto blurSpread = [] (float radius, float below = 0.0f)
        {
            const auto reach = std::ceil (radius * 3.0f);
            return yup::EdgeSpacing<float> (reach, reach, reach, reach + below);
        };

        const auto useShader = [this, value] (ShaderEffect* effect) -> yup::ComponentEffect::Ptr
        {
            shaderEffect = effect;
            shaderEffect->setEffectParameter (value);

            setParameter = [effect = shaderEffect] (float newValue)
            {
                effect->setEffectParameter (newValue);
            };

            return shaderEffect;
        };

        switch (getEffectInfo().id)
        {
            case builtInBlur:
            {
                yup::ReferenceCountedObjectPtr<yup::BlurEffect> effect (new yup::BlurEffect (value));

                setParameter = [effect] (float radius)
                {
                    effect->setRadius (radius);
                };

                getSpread = [effect, blurSpread]
                {
                    return blurSpread (effect->getRadius());
                };

                return effect;
            }

            case builtInDropShadow:
            {
                yup::ReferenceCountedObjectPtr<yup::DropShadowEffect> effect (new yup::DropShadowEffect (yup::Color (0xb0000000), value, { 0.0f, 6.0f }));

                setParameter = [effect] (float radius)
                {
                    effect->setRadius (radius);
                };

                getSpread = [effect, blurSpread]
                {
                    return blurSpread (effect->getRadius(), 6.0f);
                };

                return effect;
            }

            case builtInGlow:
            {
                yup::ReferenceCountedObjectPtr<yup::GlowEffect> effect (new yup::GlowEffect (value, 0.9f));

                setParameter = [effect] (float radius)
                {
                    effect->setRadius (radius);
                };

                getSpread = [effect, blurSpread]
                {
                    return blurSpread (effect->getRadius());
                };

                return effect;
            }

            case builtInGrayscale:
            {
                yup::ReferenceCountedObjectPtr<yup::ColorMatrixEffect> effect (new yup::ColorMatrixEffect (yup::ColorMatrixEffect::grayscaleMatrix (value)));

                setParameter = [effect] (float amount)
                {
                    effect->setMatrix (yup::ColorMatrixEffect::grayscaleMatrix (amount));
                };

                return effect;
            }

            case filterChain:
            {
                auto filter = yup::FilterEffect::fromString (chainEditor.getText());

                if (filter.failed())
                {
                    setStatus (filter.getErrorMessage());
                    return nullptr;
                }

                getSpread = [effect = filter.getValue()]
                {
                    return effect->getRequiredMargin();
                };

                return filter.getValue();
            }

            case shaderBlur:
                return useShader (new ShaderBlurEffect());

            case shaderPixelate:
                return useShader (new PixelateEffect());

            case shaderEdge:
                return useShader (new EdgeEffect());

            case shaderWave:
                return useShader (new WaveEffect());

            case shaderSharpen:
                return useShader (new SharpenEffect());

            case shaderCrt:
                return useShader (new CRTScanEffect());

            case none:
                break;
        }

        return nullptr;
    }

    /** An effect set from C++ gets no paint margin on its own: tells the one it needs. */
    void describeSpread()
    {
        if (getSpread == nullptr)
            return;

        const auto spread = getSpread();

        setStatus (spread != yup::EdgeSpacing<float>() ? "Spreads " + describe (spread) + ": needs that paint margin"
                                                       : yup::String());
    }

    //==============================================================================
    void setSubjectMargin (float margin)
    {
        marginLabel.setText (number (margin), yup::dontSendNotification);
        applySubjectMargin (margin);
    }

    void applySubjectMargin (float margin)
    {
        stage.subject.setPaintMargin (margin);

        // A sheet widens the margin it finds to hold its shadows, so it is told about the new one
        if (stage.getStyleContext() != nullptr)
            applyStyle();
    }

    /** Gives the subject the `shadow:`, `glow:` and `filter:` chosen through a stylesheet. */
    void applyStyle()
    {
        // The sheet is written as the tree a `.yui` text is parsed into, so no parser is needed
        yup::DataTree rule ("rule", { { "selector", "#subject" } });

        const auto declare = [&rule] (const char* name, const yup::String& value)
        {
            rule.beginTransaction().addChild (yup::DataTree ("declaration", { { "name", name }, { "value", value } }));
        };

        if (shadowToggle.getToggleState())
            declare ("shadow", "0 10 20 #000000b0");

        if (glowToggle.getToggleState())
            declare ("glow", "16 2 palette.accent.withAlpha (0.6)");

        if (filterToggle.getToggleState())
            declare ("filter", "hue-rotate(120deg) saturate(130%)");

        if (rule.getNumChildren() == 0)
        {
            stage.setStyleSheet (nullptr);
            return;
        }

        declare ("cornerRadius", yup::String (Subject::cornerRadius));

        auto sheet = yup::StyleSheet::fromTree (yup::DataTree ("style", { rule }));

        if (sheet.failed())
        {
            setStatus (sheet.getErrorMessage());
            return;
        }

        stage.setStyleSheet (sheet.getReference());

        if (auto* context = stage.getStyleContext())
            context->restyleNow();
    }

    //==============================================================================
    void takeSnapshot()
    {
        if (capturedContext == nullptr)
            return;

        auto snapshot = stage.subject.snapshotToImage (*capturedContext, snapshotEffectsToggle.getToggleState());

        if (! snapshot.isValid())
        {
            setStatus ("The snapshot failed");
            return;
        }

        setStatus ("Snapshot of " + yup::String (snapshot.getWidth()) + " x " + yup::String (snapshot.getHeight()) + " px");
        snapshotPreview.setImage (std::move (snapshot));
    }

    void updateMeasured()
    {
        const auto& subject = stage.subject;

        yup::StringArray lines;
        lines.add ("paint margin    " + describe (subject.getPaintMargin()));
        lines.add ("paint bounds    " + describe (subject.getPaintBounds()));

        if (probe != nullptr)
        {
            lines.add ("effect bounds   " + describe (probe->appliedBounds));
            lines.add ("texture         " + yup::String (probe->textureSize.getWidth()) + " x " + yup::String (probe->textureSize.getHeight()) + " px, resized " + yup::String (probe->textureSizeChanges) + " times");
            lines.add ("effect          " + yup::String (ticksToMicroseconds (probe->applyTicks)) + " us");
        }
        else
        {
            lines.add ("effect bounds   no effect");
            lines.add ("texture         no effect");
            lines.add ("effect          no effect");
        }

        lines.add ("subject         painted " + yup::String (subject.paintCount) + " times, " + yup::String (ticksToMicroseconds (subjectPaintTicks)) + " us");

        if (lines == measured)
            return;

        measured = lines;
        repaint (measuredArea);
    }

    //==============================================================================
    Stage stage;

    yup::ComboBox effectCombo { "effectCombo" };
    yup::Slider parameterSlider { yup::Slider::LinearHorizontal, "parameterSlider" };
    yup::Label parameterLabel { "parameterLabel" };
    yup::TextEditor chainEditor { "chainEditor" };

    yup::ToggleButton shadowToggle { "shadowToggle" };
    yup::ToggleButton glowToggle { "glowToggle" };
    yup::ToggleButton filterToggle { "filterToggle" };

    yup::Slider marginSlider { yup::Slider::LinearHorizontal, "marginSlider" };
    yup::Label marginLabel { "marginLabel" };
    yup::ToggleButton movingMarginToggle { "movingMarginToggle" };

    yup::ToggleButton cacheToggle { "cacheToggle" };
    yup::ToggleButton opaqueToggle { "opaqueToggle" };
    yup::ToggleButton guidesToggle { "guidesToggle" };
    yup::ToggleButton animatedToggle { "animatedToggle" };
    yup::ToggleButton rotatedToggle { "rotatedToggle" };
    yup::ToggleButton halfPixelToggle { "halfPixelToggle" };

    yup::TextButton snapshotButton { "snapshotButton" };
    yup::ToggleButton snapshotEffectsToggle { "snapshotEffectsToggle" };
    SnapshotPreview snapshotPreview;

    yup::ReferenceCountedObjectPtr<ProbeEffect> probe;
    yup::ReferenceCountedObjectPtr<ShaderEffect> shaderEffect;
    std::function<void (float)> setParameter;
    std::function<yup::EdgeSpacing<float>()> getSpread;

    yup::GraphicsContext* capturedContext = nullptr;

    yup::Rectangle<float> panelArea, measuredArea, statusArea;
    std::array<yup::Rectangle<float>, 6> sectionAreas;
    yup::StringArray measured;
    yup::String status;

    // Written while the subject is painted, read between two frames
    yup::int64 subjectPaintTicks = 0;

    double time = 0.0;
    double measureCountdown = 0.0;
    yup::FrameAttachment frameAttachment { *this, yup::bindFront (&ComponentEffectsDemo::refreshDisplay, this) };

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComponentEffectsDemo)
};
