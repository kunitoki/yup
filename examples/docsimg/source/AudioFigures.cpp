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

#include "Figures.h"

#include <yup_audio_gui/yup_audio_gui.h>

namespace
{

//==============================================================================

/** Samples taken every millisecond, interpolated as a function of the time in milliseconds. */
std::function<float (float)> sampled (std::vector<float> samples)
{
    return [samples = std::move (samples)] (float milliseconds)
    {
        const auto position = yup::jlimit (0.0f, static_cast<float> (samples.size() - 2), milliseconds);
        const auto index = static_cast<std::size_t> (position);
        return yup::jmap (position - static_cast<float> (index), samples[index], samples[index + 1]);
    };
}

/** Components of a fixed width side by side, centered, each with its name below. */
struct Strip : yup::Component
{
    explicit Strip (float widthOfEach)
        : width (widthOfEach)
    {
        setOpaque (false);
    }

    template <class ComponentType>
    ComponentType& add (const char* name, std::unique_ptr<ComponentType> component)
    {
        auto& result = *component;
        addAndMakeVisible (result);

        names.push_back (name);
        components.push_back (std::move (component));
        return result;
    }

    yup::Rectangle<float> getCell (std::size_t index) const
    {
        const auto cellWidth = width + 48.0f;
        const auto left = (getWidth() - cellWidth * static_cast<float> (components.size())) / 2.0f;
        return { left + cellWidth * static_cast<float> (index), 0.0f, cellWidth, getHeight() };
    }

    void resized() override
    {
        for (std::size_t index = 0; index < components.size(); ++index)
            components[index]->setBounds (getCell (index).withTrimmedBottom (20.0f).withSizeKeepingCenter (width, getHeight() - 20.0f));
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (role (Role::textMuted));

        for (std::size_t index = 0; index < components.size(); ++index)
            g.fillFittedText (names[index], font (12.0f), getCell (index).removeFromBottom (18.0f), yup::Justification::center);
    }

    std::vector<const char*> names;
    std::vector<std::unique_ptr<yup::Component>> components;
    float width;
};

/** Two meters of each scale mapping, measuring a stereo signal a little louder on the left. */
struct Meters : Strip
{
    Meters()
        : Strip (28.0f)
    {
        std::vector<float> left (512), right (512);
        const float* channels[] = { left.data(), right.data() };

        for (int block = 0; block < 94; ++block)
        {
            for (int i = 0; i < 512; ++i)
            {
                const auto phase = static_cast<float> (block * 512 + i) * yup::MathConstants<float>::twoPi * 220.0f / 48000.0f;
                left[static_cast<std::size_t> (i)] = 0.2f * std::sin (phase);
                right[static_cast<std::size_t> (i)] = 0.1f * std::sin (phase * 1.5f);
            }

            state.pushSamples (channels, 2, 512);
            state.processPendingAudio();
        }

        for (const auto mapping : { yup::KMeterComponent::ScaleMapping::linear, yup::KMeterComponent::ScaleMapping::segmented })
        {
            for (int channel = 0; channel < 2; ++channel)
            {
                const auto linear = mapping == yup::KMeterComponent::ScaleMapping::linear;
                auto& meter = add (linear ? (channel == 0 ? "linear L" : "linear R") : (channel == 0 ? "segmented L" : "segmented R"),
                                   std::make_unique<yup::KMeterComponent> (state, channel));
                meter.setScaleMapping (mapping);
                meter.setShowPeakHold (true);
                meter.refreshDisplay (1.0);
            }
        }
    }

    yup::KMeterState state { 48000.0, 2 };
};

/** The mod wheel and the pitch wheel, both moved away from their default. */
struct Wheels : Strip
{
    Wheels()
        : Strip (40.0f)
    {
        add ("mod wheel", std::make_unique<yup::ModWheelComponent> (keyboardState)).setValue (0.65, yup::dontSendNotification);
        add ("pitch wheel", std::make_unique<yup::PitchWheelComponent> (keyboardState)).setValue (0.4, yup::dontSendNotification);
    }

    yup::MidiKeyboardState keyboardState;
};

} // namespace

//==============================================================================

void addAudioFigures (Figures& f)
{
    using namespace yup;

    //==============================================================================
    // docs/audio/utilities.md

    f.paint ("audio/utilities_smoothed-value", { 560.0f, 210.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        const auto ramp = [] (auto smoothed)
        {
            smoothed.reset (1000.0, 0.04);
            smoothed.setCurrentAndTargetValue (0.1f);

            std::vector<float> samples;
            for (int millisecond = 0; millisecond <= 120; ++millisecond)
            {
                if (millisecond == 10)
                    smoothed.setTargetValue (1.0f);
                else if (millisecond == 65)
                    smoothed.setTargetValue (0.1f);

                samples.push_back (smoothed.getNextValue());
            }

            return sampled (std::move (samples));
        };

        plot (g, area, { 0.0f, 120.0f, "0 ms", "120 ms" }, { 0.0f, 1.1f, "0", "1.1" },
              { { "Linear", ramp (SmoothedValue<float, ValueSmoothingTypes::Linear>()) },
                { "Multiplicative", ramp (SmoothedValue<float, ValueSmoothingTypes::Multiplicative>()) } });
    });

    f.paint ("audio/utilities_adsr", { 560.0f, 210.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        ADSR envelope;
        envelope.setSampleRate (1000.0);
        envelope.setParameters ({ 0.1f, 0.2f, 0.6f, 0.4f });

        std::vector<float> samples;
        for (int millisecond = 0; millisecond <= 1400; ++millisecond)
        {
            if (millisecond == 50)
                envelope.noteOn();
            else if (millisecond == 800)
                envelope.noteOff();

            samples.push_back (envelope.getNextSample());
        }

        const Axis time { 0.0f, 1400.0f, "0 s", "1.4 s" };
        const auto plotArea = plot (g, area, time, { 0.0f, 1.1f, "0", "1.1" }, { { "ADSR", sampled (std::move (samples)) } });

        g.setFillColor (role (Role::textMuted));
        const std::pair<const char*, float> phases[] = { { "attack", 100.0f }, { "decay", 250.0f }, { "sustain", 575.0f }, { "release", 1000.0f } };

        for (const auto& [name, millisecond] : phases)
        {
            const auto x = plotArea.getX() + plotArea.getWidth() * time.getProportion (millisecond);
            g.fillFittedText (name, font (11.0f), { x - 40.0f, plotArea.getBottom(), 80.0f, 16.0f }, Justification::center);
        }
    });

    //==============================================================================
    // docs/audio/audio-gui.md

    f.component ("audio/audio-gui_meters", { 480.0f, 320.0f }, []
    {
        return std::make_unique<Meters>();
    });

    f.component ("audio/audio-gui_cartesian-plane", { 560.0f, 260.0f }, []
    {
        auto plane = std::make_unique<CartesianPlane>();
        plane->setBackgroundColor (role (Role::surface));
        plane->setXRange (20.0, 20000.0);
        plane->setXScaleType (CartesianPlane::AxisScaleType::logarithmic);
        plane->setYRange (-48.0, 12.0);
        plane->setVerticalGridLines ({ 20.0, 100.0, 1000.0, 10000.0, 20000.0 }, role (Role::outline));
        plane->setHorizontalGridLines ({ -48.0, -24.0, -12.0, 0.0, 12.0 }, role (Role::outline));
        plane->addHorizontalGridLine (0.0, role (Role::textMuted), 2.0f, true);
        plane->setXAxisLabels ({ 100.0, 1000.0, 10000.0 }, role (Role::textMuted));
        plane->setYAxisLabels ({ -48.0, -24.0, -12.0, 0.0, 12.0 }, role (Role::textMuted));
        plane->setLegendBackgroundColor (role (Role::surfaceRaised).withAlpha (0.9f));
        plane->setLegendTextColor (role (Role::text));
        plane->setLegendPosition ({ 0.3f, 0.6f });
        plane->setLegendVisible (true);

        for (const auto mode : { FilterMode::lowpass, FilterMode::highpass })
        {
            ButterworthFilter<float> filter (mode, 2, 2000.0f);
            filter.setParameters (mode, 2, 2000.0f, 0.0f, 48000.0);

            std::vector<Point<double>> response;
            for (int step = 0; step <= 200; ++step)
            {
                const auto frequency = 20.0 * std::pow (1000.0, step / 200.0);
                response.push_back ({ frequency, 2.0 * Decibels::gainToDecibels (filter.getMagnitudeResponse (static_cast<float> (frequency))) });
            }

            const auto lowpass = mode == FilterMode::lowpass;
            plane->updateSignalData (plane->addSignal (lowpass ? "Low" : "High", lowpass ? Color (0xff3b9cff) : Colors::tomato, 2.0f), response);
        }

        return plane;
    });

    f.component ("audio/audio-gui_wheels", { 280.0f, 200.0f }, []
    {
        return std::make_unique<Wheels>();
    });
}
