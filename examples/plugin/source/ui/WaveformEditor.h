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
#include "WaveformTools.h"

#include <memory>
#include <optional>
#include <vector>

//==============================================================================
/** The waveform of one oscillator, either drawn or edited a partial at a time.

    In drawing mode the component shows one period of the reconstructed series, and a
    drag draws into that period with the chosen tool: the stroke is analyzed back into
    partials, phase included, which become the oscillator's custom series. A drag with
    shift held rotates the period instead. In editing mode it shows each harmonic as a
    bar whose magnitude, or phase, can be dragged. The bars show a window of the
    editable harmonics: the wheel scrolls it, the wheel with command or control held
    or a pinch zooms it around the pointer, and the strip below the bars jumps to
    wherever it is pressed.

    Every stroke and operation is one step of the undo history the editor sits in.

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

    /** Rereads the series from the parameters and partials, after anything changed them.

        The values read are kept, so edits made on the display until the next refresh keep
        showing the same modulated spectrum.

        @param reader  Where the parameter values come from: the parameters themselves, or a
                       table with the modulation of the most recent note applied, so the
                       drawn wave follows a modulated spectrum.
    */
    void refresh (SynthParameterReader reader = {});

    /** Publishes a pending drag to the audio thread, coalescing a frame's worth of edits. */
    void commitPendingEdits();

    /** Drops the edited partials and returns to the selected waveform preset.

        @param joinsLastStep True to undo it together with the step recorded just before,
                             such as picking the preset that it reverts to
    */
    void revertToPreset (bool joinsLastStep = false);

    /** Lets the landscape move while something plays; it holds still while drawing. */
    void setAnimating (bool shouldAnimate) noexcept { animating = shouldAnimate; }

    //==============================================================================
    /** Chooses what a stroke draws in drawing mode. */
    void setDrawTool (WaveformDrawTool tool) noexcept { drawTool = tool; }

    /** Chooses what a drag changes in editing mode. */
    void setPartialsTool (PartialsTool tool);

    /** Sets the cells per period the span tools snap to, 0 for none. */
    void setGridDivisions (int divisions);

    int getGridDivisions() const noexcept { return gridDivisions; }

    /** Changes every partial at once, as one step of the undo history. */
    void applyOperation (PartialsOperation operation);

    //==============================================================================
    void paint (yup::Graphics& g) override;
    void refreshDisplay (double lastFrameTimeSeconds) override;
    void resized() override;
    void mouseDown (const yup::MouseEvent& event) override;
    void mouseDrag (const yup::MouseEvent& event) override;
    void mouseUp (const yup::MouseEvent& event) override;
    void mouseWheel (const yup::MouseEvent& event, const yup::MouseWheelData& wheelData) override;
    void gesturePinch (const yup::GestureEvent& event) override;

private:
    //==============================================================================
    void rebuild();
    void reconstruct (const yup::FourierSeries<double>& series) noexcept;
    yup::GpuTexture::Ptr renderLandscape (yup::Graphics& g);
    static float measurePeak (const std::vector<float>& samples) noexcept;

    void paintDrawnCycle (yup::Graphics& g, yup::Rectangle<float> bounds);
    void paintGrid (yup::Graphics& g, yup::Rectangle<float> bounds);
    void paintWaveform (yup::Graphics& g, yup::Rectangle<float> bounds);
    void paintPartials (yup::Graphics& g, yup::Rectangle<float> bounds);
    void paintPartialsOverview (yup::Graphics& g, yup::Rectangle<float> bounds);

    void beginCustomEdit();
    void publishEdit();
    void seedDrawnCycle();
    void applyDraw (const yup::MouseEvent& event);
    void applyRotation (const yup::MouseEvent& event);
    void applyEdit (const yup::MouseEvent& event);
    void recordEdit (const SynthPartials::Snapshot& before, bool joinsLastStep = false);

    void strokeGlowing (yup::Graphics& g, const yup::Path& pathToStroke);
    bool isDrawing() const noexcept;
    int cycleIndexAt (float x) const noexcept;
    float cycleValueAt (float y) const noexcept;

    yup::Rectangle<float> getPartialsArea() const noexcept;
    int getVisiblePartialCount() const noexcept;
    int getFirstVisiblePartial() const noexcept;
    void zoomPartials (float factor, float x);
    void centerPartialsAt (float x);
    void clampPartialsView() noexcept;

    //==============================================================================
    static constexpr int displayResolution = 256;
    static constexpr float contentInset = 6.0f;
    static constexpr float cornerRadius = 4.0f;
    static constexpr float barGap = 1.0f;
    static constexpr float overviewHeight = 3.0f;
    static constexpr float overviewGap = 4.0f;
    static constexpr int minimumVisiblePartials = 16;
    static constexpr int defaultVisiblePartials = 64;
    static constexpr double animationInterval = 1.0 / 30.0;

    static_assert (displayResolution == SynthWaveformShader::sampleCount);

    // A drawn cycle is analyzed back into partials, and its points carry no more than this.
    static_assert (SynthExample::displayHarmonics < displayResolution / 2);

    const SynthOscillatorParameters& parameters;
    SynthPartials& partials;
    const SynthOscillatorResources& resources;
    std::shared_ptr<SynthWaveformShader> shader;

    yup::GpuTexture::Ptr getLandscape (yup::Graphics& g);

    yup::GpuTarget::Ptr landscapeTarget;
    yup::GpuTexture::Ptr cachedLandscape;
    float animationTime = 0.0f;
    double sinceLastFrame = 0.0;
    bool landscapeDirty = true;
    bool animating = false;

    SynthSpectrumDerivation preview;
    SynthOscillatorValues shownValues;
    yup::FourierSeries<double> displaySeries;
    std::vector<float> displaySamples;
    yup::Path path;

    std::vector<float> drawnCycle;
    yup::Path drawnPath;
    yup::FourierSeries<double> drawnSeries;
    std::optional<int> lastDrawnIndex;

    WaveformDrawTool drawTool = WaveformDrawTool::brush;
    PartialsTool partialsTool = PartialsTool::magnitude;
    int gridDivisions = 0;
    std::vector<float> strokeBase;
    int pressIndex = 0;
    float pressValue = 0.0f;
    yup::int64 strokeSeed = 0;
    yup::Random random;

    std::optional<float> rotationStartX;
    yup::FourierSeries<double> rotationBase;
    yup::FourierSeries<double> rotated;

    SynthPartials::Snapshot gestureStart;

    // The window of partials shown, kept fractional so slow scrolls and pinches add up.
    float partialsViewStart = 0.0f;
    float partialsViewCount = static_cast<float> (defaultVisiblePartials);
    bool draggingOverview = false;

    bool editingPartials = false;
    bool pendingEdit = false;
};
