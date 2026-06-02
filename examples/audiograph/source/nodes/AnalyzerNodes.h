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

#include <algorithm>
#include <atomic>
#include <cmath>
#include <vector>

#include "NodeViewHelpers.h"

//==============================================================================
namespace AnalyzerNodeHelpers
{

inline void pushMonoSamples (const yup::AudioBuffer<float>& audioBuffer,
                             yup::SpectrumAnalyzerState& analyzerState,
                             std::atomic<float>& peakLevel) noexcept
{
    const auto numChannels = audioBuffer.getNumChannels();
    const auto numSamples = audioBuffer.getNumSamples();

    if (numChannels <= 0 || numSamples <= 0)
    {
        peakLevel.store (0.0f, std::memory_order_relaxed);
        return;
    }

    float blockPeak = 0.0f;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float monoSample = 0.0f;

        for (int channel = 0; channel < numChannels; ++channel)
            monoSample += audioBuffer.getSample (channel, sample);

        monoSample /= static_cast<float> (numChannels);
        blockPeak = yup::jmax (blockPeak, std::abs (monoSample));
        analyzerState.pushSample (monoSample);
    }

    peakLevel.store (blockPeak, std::memory_order_relaxed);
}

inline yup::String formatPeakLevel (float peakLevel)
{
    if (peakLevel <= 0.000001f)
        return "-inf dB";

    return yup::String (20.0f * std::log10 (peakLevel), 1) + " dB";
}

inline yup::DataTree createStatelessNodeState (const yup::Identifier& type)
{
    yup::DataTree state (type);
    auto transaction = state.beginTransaction();
    transaction.setProperty ("version", 1);
    return state;
}

inline yup::Result loadStatelessNodeState (const yup::DataTree& state, const yup::Identifier& expectedType)
{
    if (! state.isValid() || state.getType() != expectedType)
        return yup::Result::fail ("Invalid node state");

    if (static_cast<int> (state.getProperty ("version", 0)) != 1)
        return yup::Result::fail ("Unsupported node state version");

    return yup::Result::ok();
}

inline yup::Rectangle<float> getAnalyzerBounds (const yup::Component& component, int preferredWidth)
{
    const auto scale = component.getLocalBounds().getWidth() / static_cast<float> (preferredWidth);
    auto body = component.getLocalBounds().to<float>().reduced (14.0f * scale, 0.0f);

    return { body.getX() + 8.0f * scale,
             45.0f * scale,
             body.getWidth() - 16.0f * scale,
             72.0f * scale };
}

inline int getDisplayPointCount (float displayWidth,
                                 int sourcePointCount,
                                 int minimumPointCount,
                                 int maximumPointCount,
                                 float pixelsPerPoint = 1.0f) noexcept
{
    if (displayWidth <= 1.0f || sourcePointCount <= 0)
        return 0;

    const auto maxPointCount = yup::jmin (maximumPointCount, sourcePointCount);
    const auto minPointCount = yup::jmin (minimumPointCount, maxPointCount);
    const auto widthPointCount = static_cast<int> (std::ceil (displayWidth / yup::jmax (1.0f, pixelsPerPoint)));

    return yup::jlimit (minPointCount, maxPointCount, widthPointCount);
}

inline void drawAnalyzerBackground (yup::Graphics& g, yup::Rectangle<float> bounds, yup::Color accentColor)
{
    g.setFillColor (yup::Color (0xff0f1318));
    g.fillRect (bounds);

    g.setStrokeWidth (1.0f);
    g.setStrokeColor (accentColor.withAlpha (0.13f));

    const auto horizontalLines = bounds.getHeight() >= 36.0f ? 3 : 1;
    const auto verticalLines = bounds.getWidth() >= 90.0f ? 5 : (bounds.getWidth() >= 45.0f ? 2 : 0);

    for (int i = 1; i <= horizontalLines; ++i)
    {
        const auto y = bounds.getY() + bounds.getHeight() * static_cast<float> (i) / static_cast<float> (horizontalLines + 1);
        g.strokeLine (bounds.getX(), y, bounds.getRight(), y);
    }

    for (int i = 1; i <= verticalLines; ++i)
    {
        const auto x = bounds.getX() + bounds.getWidth() * static_cast<float> (i) / static_cast<float> (verticalLines + 1);
        g.strokeLine (x, bounds.getY(), x, bounds.getBottom());
    }
}

} // namespace AnalyzerNodeHelpers

//==============================================================================
class OscilloscopeProcessor final : public yup::AudioProcessor
{
public:
    OscilloscopeProcessor()
        : AudioProcessor ("Oscilloscope",
                          yup::AudioBusLayout ({ yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Input, 2) },
                                               { yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
        , analyzerState (512)
    {
        analyzerState.setOverlapFactor (0.0f);
    }

    void prepareToPlay (float, int) override
    {
        analyzerState.reset();
        peakLevel.store (0.0f, std::memory_order_relaxed);
    }

    void releaseResources() override {}

    void processBlock (yup::AudioProcessContext<float>& context) override
    {
        AnalyzerNodeHelpers::pushMonoSamples (context.audio, analyzerState, peakLevel);
    }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    yup::String getPresetName (int) const override { return {}; }

    void setPresetName (int, yup::StringRef) override {}

    bool supportsDataTreeState() const noexcept override { return true; }

    yup::Result loadStateFromDataTree (const yup::DataTree& state) override
    {
        return AnalyzerNodeHelpers::loadStatelessNodeState (state, stateType);
    }

    yup::Result saveStateIntoDataTree (yup::DataTree& state) override
    {
        state = AnalyzerNodeHelpers::createStatelessNodeState (stateType);
        return yup::Result::ok();
    }

    bool hasEditor() const override { return false; }

    yup::SpectrumAnalyzerState& getAnalyzerState() noexcept { return analyzerState; }

    float getPeakLevel() const noexcept { return peakLevel.load (std::memory_order_relaxed); }

private:
    static constexpr const char* stateType = "OscilloscopeState";

    yup::SpectrumAnalyzerState analyzerState;
    std::atomic<float> peakLevel { 0.0f };
};

//==============================================================================
class OscilloscopeDisplayComponent final
    : public yup::Component
    , public yup::Timer
{
public:
    OscilloscopeDisplayComponent (OscilloscopeProcessor& processorIn, yup::Color accentColorIn)
        : processor (processorIn)
        , accentColor (accentColorIn)
        , scopeData (processorIn.getAnalyzerState().getFftSize(), 0.0f)
    {
        setOpaque (false);
        startTimerHz (30);
    }

    ~OscilloscopeDisplayComponent() override
    {
        stopTimer();
    }

    void paint (yup::Graphics& g) override
    {
        const auto bounds = getLocalBounds().to<float>();

        AnalyzerNodeHelpers::drawAnalyzerBackground (g, bounds, accentColor);

        const auto centerY = bounds.getCenterY();
        g.setStrokeColor (accentColor.withAlpha (0.28f));
        g.strokeLine (bounds.getX(), centerY, bounds.getRight(), centerY);

        if (scopeData.size() < 2)
            return;

        const auto displayPointCount = AnalyzerNodeHelpers::getDisplayPointCount (bounds.getWidth(),
                                                                                  static_cast<int> (scopeData.size()),
                                                                                  8,
                                                                                  static_cast<int> (scopeData.size()));

        if (displayPointCount < 2)
            return;

        if (displayPointCount >= static_cast<int> (scopeData.size()))
        {
            yup::Path waveform;
            const auto maxIndex = static_cast<float> (scopeData.size() - 1);

            for (size_t i = 0; i < scopeData.size(); ++i)
            {
                const auto sample = yup::jlimit (-1.0f, 1.0f, scopeData[i]);
                const auto x = bounds.getX() + bounds.getWidth() * static_cast<float> (i) / maxIndex;
                const auto y = centerY - sample * bounds.getHeight() * 0.46f;

                if (i == 0)
                    waveform.startNewSubPath (x, y);
                else
                    waveform.lineTo (x, y);
            }

            g.setStrokeJoin (yup::StrokeJoin::Round);
            g.setStrokeColor (accentColor.withAlpha (0.95f));
            g.setStrokeWidth (1.5f);
            g.strokePath (waveform);
            return;
        }

        yup::Path envelope;
        const auto sourcePointCount = static_cast<int> (scopeData.size());
        const auto maxDisplayIndex = static_cast<float> (displayPointCount - 1);

        for (int point = 0; point < displayPointCount; ++point)
        {
            const auto startIndex = point * sourcePointCount / displayPointCount;
            const auto endIndex = yup::jmax (startIndex + 1, (point + 1) * sourcePointCount / displayPointCount);
            float minimumSample = 1.0f;
            float maximumSample = -1.0f;

            for (int sourceIndex = startIndex; sourceIndex < endIndex; ++sourceIndex)
            {
                const auto sample = yup::jlimit (-1.0f, 1.0f, scopeData[static_cast<size_t> (sourceIndex)]);
                minimumSample = yup::jmin (minimumSample, sample);
                maximumSample = yup::jmax (maximumSample, sample);
            }

            const auto x = bounds.getX() + bounds.getWidth() * static_cast<float> (point) / maxDisplayIndex;
            const auto minimumY = centerY - maximumSample * bounds.getHeight() * 0.46f;
            const auto maximumY = centerY - minimumSample * bounds.getHeight() * 0.46f;

            envelope.startNewSubPath (x, minimumY);
            envelope.lineTo (x, maximumY);
        }

        g.setStrokeJoin (yup::StrokeJoin::Round);
        g.setStrokeColor (accentColor.withAlpha (0.95f));
        g.setStrokeWidth (1.5f);
        g.strokePath (envelope);
    }

private:
    void timerCallback() override
    {
        auto& analyzerState = processor.getAnalyzerState();
        bool hasNewData = false;

        for (int i = 0; i < 4 && analyzerState.isFFTDataReady(); ++i)
            hasNewData = analyzerState.getFFTData (scopeData.data()) || hasNewData;

        if (hasNewData)
            repaint();
    }

    OscilloscopeProcessor& processor;
    yup::Color accentColor;
    std::vector<float> scopeData;
};

//==============================================================================
class OscilloscopeNodeView final : public yup::AudioGraphNodeView
{
public:
    OscilloscopeNodeView (yup::AudioGraphNodeID nodeID, OscilloscopeProcessor& processorIn)
        : AudioGraphNodeView (nodeID)
        , processor (processorIn)
        , display (processorIn, getNodeColor())
    {
        setColor (Style::parameterBackgroundColorId, yup::Color (0x00000000));
        setColor (Style::parameterValueBackgroundColorId, yup::Color (0x00000000));
        addAndMakeVisible (display);
    }

    yup::String getNodeTitle() const override { return "SCOPE"; }

    int getNumInputPorts() const override { return 1; }

    int getNumOutputPorts() const override { return 1; }

    int getPreferredWidth() const override { return 260; }

    yup::Color getNodeColor() const override { return yup::Color (0xff22c55e); }

    yup::String getNodeSubtitle() const override { return AnalyzerNodeHelpers::formatPeakLevel (processor.getPeakLevel()); }

    PortInfo getInputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    PortInfo getOutputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    int getNumParameterRows() const override { return 3; }

    ParameterInfo getParameterInfo (int) const override
    {
        return { {}, {}, getNodeColor(), -1.0f, PortKind::parameter };
    }

    void resized() override
    {
        display.setBounds (AnalyzerNodeHelpers::getAnalyzerBounds (*this, getPreferredWidth()));
    }

private:
    OscilloscopeProcessor& processor;
    OscilloscopeDisplayComponent display;
};

//==============================================================================
class SpectrumAnalyzerProcessor final : public yup::AudioProcessor
{
public:
    SpectrumAnalyzerProcessor()
        : AudioProcessor ("Spectrum Analyzer",
                          yup::AudioBusLayout ({ yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Input, 2) },
                                               { yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
        , analyzerState (1024)
    {
        analyzerState.setOverlapFactor (0.5f);
    }

    void prepareToPlay (float, int) override
    {
        analyzerState.reset();
        peakLevel.store (0.0f, std::memory_order_relaxed);
    }

    void releaseResources() override {}

    void processBlock (yup::AudioProcessContext<float>& context) override
    {
        AnalyzerNodeHelpers::pushMonoSamples (context.audio, analyzerState, peakLevel);
    }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    yup::String getPresetName (int) const override { return {}; }

    void setPresetName (int, yup::StringRef) override {}

    bool supportsDataTreeState() const noexcept override { return true; }

    yup::Result loadStateFromDataTree (const yup::DataTree& state) override
    {
        return AnalyzerNodeHelpers::loadStatelessNodeState (state, stateType);
    }

    yup::Result saveStateIntoDataTree (yup::DataTree& state) override
    {
        state = AnalyzerNodeHelpers::createStatelessNodeState (stateType);
        return yup::Result::ok();
    }

    bool hasEditor() const override { return false; }

    yup::SpectrumAnalyzerState& getAnalyzerState() noexcept { return analyzerState; }

    float getPeakLevel() const noexcept { return peakLevel.load (std::memory_order_relaxed); }

private:
    static constexpr const char* stateType = "SpectrumAnalyzerState";

    yup::SpectrumAnalyzerState analyzerState;
    std::atomic<float> peakLevel { 0.0f };
};

//==============================================================================
class SpectrumAnalyzerDisplayComponent final
    : public yup::Component
    , public yup::Timer
{
public:
    SpectrumAnalyzerDisplayComponent (SpectrumAnalyzerProcessor& processorIn, yup::Color accentColorIn)
        : processor (processorIn)
        , accentColor (accentColorIn)
        , fftSize (processorIn.getAnalyzerState().getFftSize())
        , fftProcessor (fftSize)
        , fftInput (static_cast<size_t> (fftSize), 0.0f)
        , fftOutput (static_cast<size_t> (fftSize * 2), 0.0f)
        , displayLevels (maxDisplayPoints, 0.0f)
        , binRanges (maxDisplayPoints)
    {
        generateWindow();
        setOpaque (false);
        startTimerHz (30);
    }

    ~SpectrumAnalyzerDisplayComponent() override
    {
        stopTimer();
    }

    void paint (yup::Graphics& g) override
    {
        const auto bounds = getLocalBounds().to<float>();

        AnalyzerNodeHelpers::drawAnalyzerBackground (g, bounds, accentColor);

        const auto displayPointCount = getTargetDisplayPointCount();
        if (displayPointCount < 2)
            return;

        const auto baseline = bounds.getBottom();
        const auto maxDisplayIndex = static_cast<float> (displayPointCount - 1);
        yup::Path fillPath;
        yup::Path linePath;

        fillPath.startNewSubPath (bounds.getX(), baseline);

        for (int i = 0; i < displayPointCount; ++i)
        {
            const auto level = yup::jlimit (0.0f, 1.0f, displayLevels[static_cast<size_t> (i)]);
            const auto x = bounds.getX() + bounds.getWidth() * static_cast<float> (i) / maxDisplayIndex;
            const auto y = baseline - bounds.getHeight() * level;

            fillPath.lineTo (x, y);

            if (i == 0)
                linePath.startNewSubPath (x, y);
            else
                linePath.lineTo (x, y);
        }

        fillPath.lineTo (bounds.getRight(), baseline);
        fillPath.closeSubPath();

        g.setFillColor (accentColor.withAlpha (0.24f));
        g.fillPath (fillPath);

        g.setStrokeJoin (yup::StrokeJoin::Round);
        g.setStrokeColor (accentColor.withAlpha (0.95f));
        g.setStrokeWidth (1.5f);
        g.strokePath (linePath);
    }

private:
    void timerCallback() override
    {
        auto& analyzerState = processor.getAnalyzerState();
        const auto displayPointCount = getTargetDisplayPointCount();
        bool hasNewData = false;

        for (int i = 0; i < 4 && displayPointCount >= 2 && analyzerState.isFFTDataReady(); ++i)
        {
            if (analyzerState.getFFTData (fftInput.data()))
            {
                processFftFrame (displayPointCount);
                hasNewData = true;
            }
        }

        if (hasNewData)
            repaint();
    }

    void generateWindow()
    {
        window.resize (static_cast<size_t> (fftSize), 0.0f);

        for (int i = 0; i < fftSize; ++i)
        {
            const auto phase = yup::MathConstants<float>::twoPi * static_cast<float> (i) / static_cast<float> (fftSize - 1);
            window[static_cast<size_t> (i)] = 0.5f - 0.5f * std::cos (phase);
        }
    }

    int getTargetDisplayPointCount() const noexcept
    {
        return AnalyzerNodeHelpers::getDisplayPointCount (getLocalBounds().getWidth(),
                                                          maxDisplayPoints,
                                                          minDisplayPoints,
                                                          maxDisplayPoints,
                                                          2.0f);
    }

    void updateBinRanges (int displayPointCount)
    {
        if (displayPointCount == cachedDisplayPointCount)
            return;

        const auto maxBin = fftSize / 2;

        for (int displayBin = 0; displayBin < displayPointCount; ++displayBin)
        {
            const auto startNormalised = static_cast<float> (displayBin) / static_cast<float> (displayPointCount);
            const auto endNormalised = static_cast<float> (displayBin + 1) / static_cast<float> (displayPointCount);
            const auto startBin = yup::jlimit (1, maxBin, static_cast<int> (std::pow (startNormalised, 1.8f) * static_cast<float> (maxBin - 1)) + 1);
            const auto endBin = yup::jlimit (startBin, maxBin, static_cast<int> (std::pow (endNormalised, 1.8f) * static_cast<float> (maxBin - 1)) + 1);

            binRanges[static_cast<size_t> (displayBin)] = { startBin, endBin };
        }

        cachedDisplayPointCount = displayPointCount;
    }

    void processFftFrame (int displayPointCount)
    {
        updateBinRanges (displayPointCount);

        for (int i = 0; i < fftSize; ++i)
            fftInput[static_cast<size_t> (i)] *= window[static_cast<size_t> (i)];

        fftProcessor.performRealFFTForward (fftInput.data(), fftOutput.data());

        for (int displayBin = 0; displayBin < displayPointCount; ++displayBin)
        {
            const auto binRange = binRanges[static_cast<size_t> (displayBin)];
            float magnitude = 0.0f;

            for (int bin = binRange.startBin; bin <= binRange.endBin; ++bin)
            {
                const auto real = fftOutput[static_cast<size_t> (bin * 2)];
                const auto imag = fftOutput[static_cast<size_t> (bin * 2 + 1)];
                magnitude = yup::jmax (magnitude, std::sqrt (real * real + imag * imag));
            }

            const auto normalizedMagnitude = yup::jmax (0.000001f, magnitude * 2.0f / static_cast<float> (fftSize));
            const auto decibels = 20.0f * std::log10 (normalizedMagnitude);
            const auto targetLevel = yup::jlimit (0.0f, 1.0f, (decibels + 72.0f) / 72.0f);
            auto& displayLevel = displayLevels[static_cast<size_t> (displayBin)];

            displayLevel = targetLevel > displayLevel
                             ? targetLevel
                             : displayLevel * 0.78f + targetLevel * 0.22f;
        }
    }

    struct BinRange
    {
        int startBin = 1;
        int endBin = 1;
    };

    static constexpr int minDisplayPoints = 8;
    static constexpr int maxDisplayPoints = 160;

    SpectrumAnalyzerProcessor& processor;
    yup::Color accentColor;
    int fftSize = 0;
    yup::FFTProcessor fftProcessor;
    std::vector<float> fftInput;
    std::vector<float> fftOutput;
    std::vector<float> window;
    std::vector<float> displayLevels;
    std::vector<BinRange> binRanges;
    int cachedDisplayPointCount = 0;
};

//==============================================================================
class SpectrumAnalyzerNodeView final : public yup::AudioGraphNodeView
{
public:
    SpectrumAnalyzerNodeView (yup::AudioGraphNodeID nodeID, SpectrumAnalyzerProcessor& processorIn)
        : AudioGraphNodeView (nodeID)
        , processor (processorIn)
        , display (processorIn, getNodeColor())
    {
        setColor (Style::parameterBackgroundColorId, yup::Color (0x00000000));
        setColor (Style::parameterValueBackgroundColorId, yup::Color (0x00000000));
        addAndMakeVisible (display);
    }

    yup::String getNodeTitle() const override { return "SPECTRUM"; }

    int getNumInputPorts() const override { return 1; }

    int getNumOutputPorts() const override { return 1; }

    int getPreferredWidth() const override { return 260; }

    yup::Color getNodeColor() const override { return yup::Color (0xfff59e0b); }

    yup::String getNodeSubtitle() const override { return AnalyzerNodeHelpers::formatPeakLevel (processor.getPeakLevel()); }

    PortInfo getInputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    PortInfo getOutputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    int getNumParameterRows() const override { return 3; }

    ParameterInfo getParameterInfo (int) const override
    {
        return { {}, {}, getNodeColor(), -1.0f, PortKind::parameter };
    }

    void resized() override
    {
        display.setBounds (AnalyzerNodeHelpers::getAnalyzerBounds (*this, getPreferredWidth()));
    }

private:
    SpectrumAnalyzerProcessor& processor;
    SpectrumAnalyzerDisplayComponent display;
};
