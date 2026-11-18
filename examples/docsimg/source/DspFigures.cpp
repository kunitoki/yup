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

#include <yup_dsp/yup_dsp.h>

#include <cmath>

namespace
{

//==============================================================================

struct Axis
{
    float getValue (float proportion) const
    {
        return logarithmic ? from * std::pow (to / from, proportion) : from + (to - from) * proportion;
    }

    float getProportion (float value) const
    {
        return logarithmic ? std::log (value / from) / std::log (to / from) : (value - from) / (to - from);
    }

    float from = 0.0f;
    float to = 1.0f;
    const char* fromLabel = "";
    const char* toLabel = "";
    bool logarithmic = false;
};

struct Trace
{
    const char* name;
    std::function<float (float)> function;
};

/** Plots functions of the horizontal axis, named below when there are several of them.

    @returns The area the axes span.
*/
yup::Rectangle<float> plot (yup::Graphics& g, yup::Rectangle<float> area, const Axis& x, const Axis& y, const std::vector<Trace>& traces)
{
    static constexpr yup::Color colors[] = { yup::Color (0xff3b9cff), yup::Colors::tomato, yup::Colors::mediumseagreen, yup::Colors::orange, yup::Colors::mediumpurple };

    const auto small = font (11.0f);

    if (traces.size() > 1)
    {
        auto legend = area.removeFromBottom (20.0f);
        const auto width = legend.getWidth() / static_cast<float> (traces.size());

        for (std::size_t index = 0; index < traces.size(); ++index)
        {
            g.setFillColor (colors[index % std::size (colors)]);
            g.fillFittedText (traces[index].name, font (12.0f), legend.removeFromLeft (width), yup::Justification::center);
        }
    }

    const auto hasLabels = [] (const Axis& axis)
    {
        return *axis.fromLabel != 0 || *axis.toLabel != 0;
    };

    g.setFillColor (role (Role::textMuted));

    if (hasLabels (y))
    {
        const auto labels = area.removeFromLeft (46.0f).withTrimmedRight (6.0f).withTrimmedBottom (hasLabels (x) ? 16.0f : 0.0f);
        g.fillFittedText (y.toLabel, small, labels, yup::Justification::topRight);
        g.fillFittedText (y.fromLabel, small, labels, yup::Justification::bottomRight);
    }

    if (hasLabels (x))
    {
        const auto labels = area.removeFromBottom (16.0f);
        g.fillFittedText (x.fromLabel, small, labels, yup::Justification::centerLeft);
        g.fillFittedText (x.toLabel, small, labels, yup::Justification::centerRight);
    }

    g.setFillColor (role (Role::surface));
    g.fillRoundedRect (area, 6.0f);

    g.setStrokeColor (role (Role::outline).withAlpha (0.6f));
    g.setStrokeWidth (1.0f);

    for (int line = 1; line < 4; ++line)
    {
        const auto height = area.getY() + area.getHeight() * static_cast<float> (line) / 4.0f;
        g.strokeLine (area.getX(), height, area.getRight(), height);
    }

    const auto saved = g.saveState();
    g.setClipPath (area);
    g.setStrokeWidth (2.0f);

    for (std::size_t index = 0; index < traces.size(); ++index)
    {
        yup::Path path;

        for (int step = 0; step <= 240; ++step)
        {
            const auto proportion = static_cast<float> (step) / 240.0f;
            const auto level = yup::jlimit (-0.5f, 1.5f, y.getProportion (traces[index].function (x.getValue (proportion))));
            const yup::Point<float> point (area.getX() + area.getWidth() * proportion, area.getBottom() - area.getHeight() * level);

            if (step == 0)
                path.moveTo (point);
            else
                path.lineTo (point);
        }

        g.setStrokeColor (colors[index % std::size (colors)]);
        g.strokePath (path);
    }

    return area;
}

/** The magnitude response of a filter in decibels, as a function of the frequency. */
template <class Filter>
std::function<float (float)> response (std::shared_ptr<Filter> filter)
{
    return [filter] (float frequency)
    {
        return static_cast<float> (yup::Decibels::gainToDecibels (filter->getMagnitudeResponse (frequency)));
    };
}

} // namespace

//==============================================================================

void addDspFigures (Figures& f)
{
    using namespace yup;

    const Axis frequency { 20.0f, 20000.0f, "20 Hz", "20 kHz", true };

    //==============================================================================
    // docs/dsp/filters.md

    f.paint ("dsp/filters_rbj-modes", { 560.0f, 228.0f }, [frequency] (Graphics& g, Rectangle<float> area)
    {
        const auto rbj = [] (FilterModeType mode, float gain)
        {
            auto filter = std::make_shared<RbjFilter<float>> (mode);
            filter->setParameters (mode, 1000.0f, 1.0f, gain, 48000.0);
            return response (filter);
        };

        const std::vector<Trace> traces {
            { "lowpass", rbj (FilterMode::lowpass, 0.0f) },
            { "highpass", rbj (FilterMode::highpass, 0.0f) },
            { "bandpassCpg", rbj (FilterMode::bandpassCpg, 0.0f) },
            { "peak, +6 dB", rbj (FilterMode::peak, 6.0f) },
            { "bandstop", rbj (FilterMode::bandstop, 0.0f) },
        };

        plot (g, area, frequency, { -36.0f, 12.0f, "-36 dB", "+12 dB" }, traces);
    });

    f.paint ("dsp/filters_butterworth-orders", { 560.0f, 210.0f }, [frequency] (Graphics& g, Rectangle<float> area)
    {
        const auto butterworth = [] (int order)
        {
            auto filter = std::make_shared<ButterworthFilter<float>> (FilterMode::lowpass, order, 1000.0f);
            filter->setParameters (FilterMode::lowpass, order, 1000.0f, 0.0f, 48000.0);
            return response (filter);
        };

        const std::vector<Trace> traces {
            { "order 2", butterworth (2) },
            { "order 4", butterworth (4) },
            { "order 8", butterworth (8) },
        };

        plot (g, area, frequency, { -96.0f, 12.0f, "-96 dB", "+12 dB" }, traces);
    });

    //==============================================================================
    // docs/dsp/filter-design.md

    f.paint ("dsp/filter-design_linkwitz-riley", { 560.0f, 210.0f }, [frequency] (Graphics& g, Rectangle<float> area)
    {
        using Sections = std::vector<BiquadCoefficients<double>>;

        const auto low = std::make_shared<Sections>();
        const auto high = std::make_shared<Sections>();
        FilterDesigner<double>::designLinkwitzRiley4 (*low, *high, 2000.0, 48000.0);

        const auto cascade = [] (const Sections& sections, float frequency)
        {
            Complex<double> result (1.0);
            for (const auto& section : sections)
                result *= section.getComplexResponse (frequency, 48000.0);

            return result;
        };

        const auto decibels = [] (Complex<double> response)
        {
            return static_cast<float> (Decibels::gainToDecibels (std::abs (response)));
        };

        const std::vector<Trace> traces {
            { "low", [=] (float frequency)
        {
            return decibels (cascade (*low, frequency));
        } },
            { "high", [=] (float frequency)
        {
            return decibels (cascade (*high, frequency));
        } },
            { "low + high", [=] (float frequency)
        {
            return decibels (cascade (*low, frequency) + cascade (*high, frequency));
        } },
        };

        plot (g, area, frequency, { -48.0f, 12.0f, "-48 dB", "+12 dB" }, traces);
    });

    //==============================================================================
    // docs/dsp/resampling.md

    f.paint ("dsp/resampling_sinc-table", { 480.0f, 200.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        using Table = SincTable<double, 256, 8>;

        const auto plain = std::make_shared<Table>();
        plain->configure (44100.0);

        const auto windowed = std::make_shared<Table>();
        windowed->configure (44100.0);
        windowed->applyKaiserWindow (9.0);

        const auto kernel = [] (std::shared_ptr<Table> table) -> std::function<float (float)>
        {
            return [table] (float time)
            {
                const auto position = jmin (std::abs (time), 7.99f);
                const auto tap = static_cast<int> (position);

                return static_cast<float> ((*table) (tap, static_cast<int> ((position - static_cast<float> (tap)) * 256.0f)));
            };
        };

        const std::vector<Trace> traces {
            { "configure", kernel (plain) },
            { "applyKaiserWindow (9)", kernel (windowed) },
        };

        const auto peak = static_cast<float> ((*plain) (0, 0));
        plot (g, area, { -8.0f, 8.0f, "tap -8", "+8" }, { -0.3f * peak, 1.1f * peak }, traces);
    });

    //==============================================================================
    // docs/dsp/envelopes.md

    f.paint ("dsp/envelopes_breakpoint-curve", { 480.0f, 200.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        BreakpointCurve<float> curve;
        curve.addPoint (0.0f, 0.0f);
        curve.addPoint (0.01f, 1.0f);
        curve.addPoint (0.3f, 0.6f, -0.5f);
        curve.addPoint (0.8f, 0.0f, -0.5f);
        curve.setSustainIndex (2);

        const Axis time { 0.0f, 0.8f, "0 s", "0.8 s" };
        const Axis level { -0.1f, 1.1f };

        const auto frame = plot (g, area, time, level, { { "", [&curve] (float seconds)
        {
            return curve.evaluate (seconds);
        } } });

        // The points of the curve, the sustain one larger
        g.setFillColor (Colors::tomato);

        for (int index = 0; index < curve.getNumPoints(); ++index)
        {
            const auto& point = curve.getPoint (index);
            const auto radius = index == 2 ? 5.0f : 3.0f;
            const auto x = frame.getX() + frame.getWidth() * time.getProportion (point.time);
            const auto y = frame.getBottom() - frame.getHeight() * level.getProportion (point.level);

            g.fillEllipse (x - radius, y - radius, radius * 2.0f, radius * 2.0f);
        }
    });

    f.paint ("dsp/envelopes_segment-curves", { 480.0f, 240.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        const std::pair<const char*, float> amounts[] = { { "c = -1", -1.0f }, { "c = -0.5", -0.5f }, { "c = 0", 0.0f }, { "c = 0.5", 0.5f }, { "c = 1", 1.0f } };
        std::vector<Trace> traces;

        for (const auto& [name, amount] : amounts)
        {
            BreakpointCurve<float> segment;
            segment.addPoint (0.0f, 0.0f);
            segment.addPoint (1.0f, 1.0f, amount);

            traces.push_back ({ name, [segment] (float time)
            {
                return segment.evaluate (time);
            } });
        }

        plot (g, area, {}, { -0.05f, 1.05f }, traces);
    });

    //==============================================================================
    // docs/dsp/oscillators.md

    f.paint ("dsp/oscillators_lfo-shapes", { 560.0f, 132.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        labelled (g, area, { "sine", "triangle", "sawtooth", "square" }, 4, [&g] (int index, Rectangle<float> cell)
        {
            plot (g, cell, { 0.0f, 2.0f }, { -1.3f, 1.3f }, { { "", [index] (float phase)
            {
                LFO<float> lfo;
                lfo.prepare (48000.0);
                lfo.setShape (static_cast<LFO<float>::Shape> (index));
                lfo.reset (phase - std::floor (phase));
                return lfo.getValue();
            } } });
        });
    });

    //==============================================================================
    // docs/dsp/dynamics.md

    f.paint ("dsp/dynamics_clippers", { 480.0f, 250.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        const auto blunter = [] (float input)
        {
            return BlunterClipper<float> (1.0f, 1.0f).processSample (input);
        };

        const auto soft = [] (float input)
        {
            return SoftClipper<float> (1.0f, 0.5f).processSample (input);
        };

        const auto clamp = [] (float input)
        {
            return jlimit (-1.0f, 1.0f, input);
        };

        const std::vector<Trace> traces {
            { "BlunterClipper", blunter },
            { "SoftClipper (1, 0.5)", soft },
            { "clamp (x, -1, 1)", clamp },
        };

        plot (g, area, { -2.0f, 2.0f, "input -2", "+2" }, { -1.25f, 1.25f, "-1.25", "+1.25" }, traces);
    });

    //==============================================================================
    // docs/dsp/math.md

    f.paint ("dsp/math_windows", { 560.0f, 210.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        const auto window = [] (WindowType type) -> std::function<float (float)>
        {
            return [type] (float position)
            {
                return WindowFunctions<float>::getValue (type, roundToInt (position * 240.0f), 241);
            };
        };

        const std::vector<Trace> traces {
            { "hann", window (WindowType::hann) },
            { "hamming", window (WindowType::hamming) },
            { "blackman", window (WindowType::blackman) },
            { "kaiser", window (WindowType::kaiser) },
            { "flattop", window (WindowType::flattop) },
        };

        plot (g, area, { 0.0f, 1.0f, "0", "N - 1" }, { -0.1f, 1.1f }, traces);
    });
}
