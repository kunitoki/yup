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

//==============================================================================

class ExpressionChartDemo
    : public yup::Component
    , public yup::Timer
{
public:
    ExpressionChartDemo()
        : Component ("ExpressionChartDemo")
        , data (64, 0.0f)
    {
        yup::addGraphicsExpressionTypes (context);

        values = context.addArray<float> ("values");
        row = context.addInput<int> ("row");
        bounds = context.addInput<yup::Rectangle<float>> ("bounds");
        time = context.addInput<float> ("time");

        // Compiled once, evaluated for every row on every repaint
        bars = createDrawing (R"(
            let w = bounds.width / len (values);
            let h = values[row] * bounds.height;
            rect = rect (bounds.x + row * w + 1, bounds.bottom - h, w - 2, h);
            cornerRadius = min (3, w / 4);
            fill = hsl (0.6 - values[row] * 0.6, 0.8, 0.55).withAlpha (0.85);
        )");

        curve = createDrawing (R"(
            let w = bounds.width / len (values);
            let v = 0.5 + 0.4 * sin (row * 0.2 + time * 2);
            vertex = point (bounds.x + (row + 0.5) * w, bounds.bottom - v * bounds.height);
            stroke = rgb (255, 255, 255);
            strokeWidth = 2;
        )");

        markers = createDrawing (R"(
            let w = bounds.width / len (values);
            let v = 0.5 + 0.4 * sin (row * 0.2 + time * 2);
            visible = row % 4 == 0;
            path = path().addStar (point (bounds.x + (row + 0.5) * w, bounds.bottom - v * bounds.height), 5, 3, 7, time);
            fill = rgb (255, 210, 80);
        )");

        for (auto* drawing : { &bars, &curve, &markers })
        {
            auto& state = *states.emplace_back (std::make_unique<yup::ExpressionState> (drawing->getProgram()));
            state.bindArray (values, data.data(), (int) data.size());
            state.bindRowIndex (row);
        }

        startTimerHz (60);
    }

    ~ExpressionChartDemo() override
    {
        stopTimer();
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (yup::ThemePalette::Role::background));
        g.fillAll();

        const auto area = getLocalBounds().reduced (20.0f);

        for (auto& state : states)
        {
            state->set (bounds, area);
            state->set (time, phase);
        }

        bars.draw (g, *states[0], (int) data.size(), nullptr, &caches[0]);
        curve.draw (g, *states[1], (int) data.size(), nullptr, &caches[1]);
        markers.draw (g, *states[2], (int) data.size(), nullptr, &caches[2]);
    }

    void timerCallback() override
    {
        phase += 1.0f / 60.0f;

        for (size_t i = 0; i < data.size(); ++i)
            data[i] = 0.5f + 0.45f * std::sin ((float) i * 0.3f + phase) * std::cos ((float) i * 0.07f - phase * 0.5f);

        repaint();
    }

private:
    yup::ExpressionDrawing createDrawing (yup::StringRef text)
    {
        auto drawing = yup::ExpressionDrawing::create (text, context);
        jassert (drawing.wasOk());

        if (drawing.failed())
        {
            yup::Logger::outputDebugString (drawing.getErrorMessage());
            return {};
        }

        return drawing.getValue();
    }

    yup::ExpressionContext context;
    yup::ExpressionArray<float> values;
    yup::ExpressionInput<int> row;
    yup::ExpressionInput<yup::Rectangle<float>> bounds;
    yup::ExpressionInput<float> time;

    yup::ExpressionDrawing bars, curve, markers;
    std::vector<std::unique_ptr<yup::ExpressionState>> states;
    std::array<yup::ExpressionDrawing::Cache, 3> caches;

    std::vector<float> data;
    float phase = 0.0f;
};
