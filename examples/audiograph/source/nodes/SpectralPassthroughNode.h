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

#include "NodeViewHelpers.h"

//==============================================================================
class SpectralPassthroughProcessor final : public yup::SpectralBridge
{
public:
    SpectralPassthroughProcessor()
        : SpectralBridge ("Spectral Passthrough",
                          yup::AudioBusLayout ({ yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Input, 2) },
                                               { yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    {
        fftSizeIndex = NodeViewHelpers::createParameter ("fftSizeIndex", "FFT Size", 0.0f, 6.0f, 3.0f, 1.0f);
        overlapFactorIndex = NodeViewHelpers::createParameter ("overlapFactorIndex", "Overlap", 0.0f, 2.0f, 1.0f, 1.0f);

        addParameter (fftSizeIndex);
        addParameter (overlapFactorIndex);

        applySettings();
    }

    float getFFTSizeIndex() const noexcept { return fftSizeIndex->getValue(); }

    float getOverlapFactorIndex() const noexcept { return overlapFactorIndex->getValue(); }

    void setFFTSizeIndex (float v) noexcept
    {
        fftSizeIndex->setValue (v);
        applySettings();
    }

    void setOverlapFactorIndex (float v) noexcept
    {
        overlapFactorIndex->setValue (v);
        applySettings();
    }

    int getCurrentFFTSize() const noexcept { return getFFTSize(); }

private:
    static constexpr const char* stateType = "SpectralPassthroughState";
    static constexpr int kFftSizeOptions[7] = { 128, 256, 512, 1024, 2048, 4096, 8192 };
    static constexpr int kOverlapOptions[3] = { 2, 4, 8 };

    void applySettings()
    {
        const int fftSizeIdx = yup::jlimit (0, 6, yup::roundToInt (getFFTSizeIndex()));
        const int overlapIdx = yup::jlimit (0, 2, yup::roundToInt (getOverlapFactorIndex()));

        setFFTSize (kFftSizeOptions[fftSizeIdx]);
        setOverlapFactor (kOverlapOptions[overlapIdx]);
    }

    yup::AudioParameter::Ptr fftSizeIndex;
    yup::AudioParameter::Ptr overlapFactorIndex;
};

//==============================================================================
class SpectralPassthroughNodeView final : public yup::AudioGraphNodeView
{
public:
    SpectralPassthroughNodeView (yup::AudioGraphNodeID nodeID, SpectralPassthroughProcessor& processorIn)
        : AudioGraphNodeView (nodeID)
        , processor (processorIn)
        , fftSizeSlider (yup::Slider::LinearBarHorizontal)
        , overlapSlider (yup::Slider::LinearBarHorizontal)
    {
        NodeViewHelpers::configureParameterSlider (fftSizeSlider, getPortKindColor (PortKind::parameter));
        fftSizeSlider.setRange (0.0, 6.0, 1.0);
        fftSizeSlider.setValue (processor.getFFTSizeIndex(), yup::dontSendNotification);
        fftSizeSlider.onValueChanged = [this] (double value)
        {
            processor.setFFTSizeIndex (static_cast<float> (value));
            repaint();
        };
        addAndMakeVisible (fftSizeSlider);

        NodeViewHelpers::configureParameterSlider (overlapSlider, getPortKindColor (PortKind::parameter));
        overlapSlider.setRange (0.0, 2.0, 1.0);
        overlapSlider.setValue (processor.getOverlapFactorIndex(), yup::dontSendNotification);
        overlapSlider.onValueChanged = [this] (double value)
        {
            processor.setOverlapFactorIndex (static_cast<float> (value));
            repaint();
        };
        addAndMakeVisible (overlapSlider);
    }

    yup::String getNodeTitle() const override { return "SPECTRAL"; }

    int getNumInputPorts() const override { return 1; }

    int getNumOutputPorts() const override { return 1; }

    int getPreferredWidth() const override { return 240; }

    yup::Color getNodeColor() const override { return yup::Color (0xff8b5cf6); }

    yup::String getNodeSubtitle() const override
    {
        return yup::String (processor.getCurrentFFTSize()) + " / " + yup::String (processor.getFFTSize() / processor.getOverlapFactor()) + " hop";
    }

    PortInfo getInputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    PortInfo getOutputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    int getNumParameterRows() const override { return 2; }

    ParameterInfo getParameterInfo (int row) const override
    {
        if (row == 0)
            return { "FFT", yup::String (processor.getCurrentFFTSize()), getPortKindColor (PortKind::parameter), -1.0f, PortKind::parameter };

        return { "Overlap", yup::String (1.0f / static_cast<float> (processor.getOverlapFactor()), 2), getPortKindColor (PortKind::parameter), -1.0f, PortKind::parameter };
    }

    void resized() override
    {
        fftSizeSlider.setBounds (NodeViewHelpers::getInlineSliderBounds (*this, getPreferredWidth(), 0));
        overlapSlider.setBounds (NodeViewHelpers::getInlineSliderBounds (*this, getPreferredWidth(), 1));
    }

private:
    SpectralPassthroughProcessor& processor;
    yup::Slider fftSizeSlider;
    yup::Slider overlapSlider;
};
