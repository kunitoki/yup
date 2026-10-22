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

#include "../dsp/SynthSpectrumDerivation.h"
#include "SynthWaveformShader.h"

#include <memory>
#include <optional>
#include <vector>

//==============================================================================
/** The waveform of one oscillator, either drawn or edited a partial at a time.

    In drawing mode the component shows one period of the reconstructed series, and a
    drag draws that period freehand: the stroke is analyzed back into partials, phase
    included, which become the oscillator's custom series. In editing mode it shows the
    magnitude of each harmonic as a bar that can be dragged.

    Dragging writes straight into the partials, but the generation counter the audio
    thread watches is only bumped by commitPendingEdits(), once per user interface
    frame. Without that, one drag would queue an inverse FFT per mouse event on every
    sounding voice.

    @see SynthPartials, SynthOscillatorPanel
*/
class WaveformEditor : public yup::Component
{
public:
    WaveformEditor (const SynthOscillatorParameters& parametersToShow,
                    SynthPartials& partialsToEdit,
                    const SynthOscillatorResources& sharedResources,
                    std::shared_ptr<SynthWaveformShader> sharedShader);

    /** Switches between drawing the waveform and editing its partials. */
    void setEditingPartials (bool shouldEdit);

    bool isEditingPartials() const noexcept { return editingPartials; }

    /** Rereads the series from the parameters and partials, after anything changed them. */
    void refresh();

    /** Publishes a pending drag to the audio thread, coalescing a frame's worth of edits. */
    void commitPendingEdits();

    /** Drops the edited partials and returns to the selected waveform preset. */
    void revertToPreset();

    //==============================================================================
    void paint (yup::Graphics& g) override;
    void refreshDisplay (double lastFrameTimeSeconds) override;
    void mouseDown (const yup::MouseEvent& event) override;
    void mouseDrag (const yup::MouseEvent& event) override;
    void mouseUp (const yup::MouseEvent& event) override;

private:
    //==============================================================================
    void reconstruct (const yup::FourierSeries<double>& series) noexcept;
    yup::GpuTexture::Ptr renderLandscape (yup::Graphics& g);
    static float measurePeak (const std::vector<float>& samples) noexcept;

    void paintDrawnCycle (yup::Graphics& g, yup::Rectangle<float> bounds);
    void paintWaveform (yup::Graphics& g, yup::Rectangle<float> bounds);
    void paintPartials (yup::Graphics& g, yup::Rectangle<float> bounds);

    void beginCustomEdit();
    void publishEdit();
    void seedDrawnCycle();
    void applyDraw (const yup::MouseEvent& event);
    void applyEdit (const yup::MouseEvent& event);

    //==============================================================================
    static constexpr int displayResolution = 256;
    static constexpr float contentInset = 6.0f;
    static constexpr float cornerRadius = 4.0f;
    static constexpr float barGap = 1.0f;

    static_assert (displayResolution == SynthWaveformShader::sampleCount);

    const SynthOscillatorParameters& parameters;
    SynthPartials& partials;
    const SynthOscillatorResources& resources;
    std::shared_ptr<SynthWaveformShader> shader;
    yup::GpuTarget::Ptr landscapeTarget;
    float animationTime = 0.0f;

    SynthSpectrumDerivation preview;
    yup::FourierSeries<double> displaySeries;
    std::vector<float> displaySamples;
    yup::Path path;

    std::vector<float> drawnCycle;
    yup::Path drawnPath;
    yup::FourierSeries<double> drawnSeries;
    std::optional<int> lastDrawnIndex;

    bool editingPartials = false;
    bool pendingEdit = false;
};
