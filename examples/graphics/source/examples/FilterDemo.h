/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#include <yup_dsp/yup_dsp.h>

#include <memory>
#include <random>
#include <chrono>

//==============================================================================

class WhiteNoiseGenerator
{
public:
    WhiteNoiseGenerator()
        : distribution (-1.0f, 1.0f)
    {
        randomEngine.seed (static_cast<uint32_t> (std::chrono::steady_clock::now().time_since_epoch().count()));
    }

    float getNextSample()
    {
        return distribution (randomEngine) * amplitude.getNextValue();
    }

    void setAmplitude (float newAmplitude)
    {
        amplitude.setTargetValue (newAmplitude);
    }

    void setSampleRate (double sampleRate)
    {
        amplitude.reset (sampleRate, 0.02);
    }

private:
    std::mt19937 randomEngine;
    std::uniform_real_distribution<float> distribution;
    yup::SmoothedValue<float> amplitude { 0.1f };
};

//==============================================================================

class PhaseResponseDisplay : public yup::Component
{
public:
    void updateResponse (const std::vector<yup::Point<double>>& data)
    {
        phaseData = data;
        repaint();
    }

private:
    void paint (yup::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        // Background
        g.setFillColor (yup::Color (0xFF1E1E1E));
        g.fillRect (bounds);

        // Reserve space for labels
        auto titleBounds = bounds.removeFromTop (20);
        auto bottomLabelSpace = bounds.removeFromBottom (20);

        // Grid
        g.setStrokeColor (yup::Color (0xFF333333));
        g.setStrokeWidth (1.0f);

        // Frequency grid lines (logarithmic)
        for (double freq : { 20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 })
        {
            float x = frequencyToX (freq, bounds);
            g.strokeLine ({ x, bounds.getY() }, { x, bounds.getBottom() });
        }

        // Phase grid lines
        for (double phase : { -180.0, -135.0, -90.0, -45.0, 0.0, 45.0, 90.0, 135.0, 180.0 })
        {
            float y = phaseToY (phase, bounds);
            g.strokeLine ({ bounds.getX(), y }, { bounds.getRight(), y });
        }

        // Zero line
        g.setStrokeColor (yup::Color (0xFF666666));
        g.setStrokeWidth (2.0f);
        float y0 = phaseToY (0.0, bounds);
        g.strokeLine ({ bounds.getX(), y0 }, { bounds.getRight(), y0 });

        // Plot phase response
        if (! phaseData.empty())
        {
            yup::Path path;
            bool firstPoint = true;

            g.setStrokeColor (yup::Color (0xFF00FF88));
            g.setStrokeWidth (2.0f);

            for (const auto& point : phaseData)
            {
                float x = frequencyToX (point.getX(), bounds);
                float y = phaseToY (point.getY(), bounds);

                if (firstPoint)
                {
                    path.startNewSubPath (x, y);
                    firstPoint = false;
                }
                else
                {
                    path.lineTo (x, y);
                }
            }

            g.strokePath (path);
        }

        // Labels
        g.setFillColor (yup::Colors::white);
        auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f);

        // Title
        g.fillFittedText ("Phase Response", font, titleBounds, yup::Justification::center);

        // Frequency labels
        for (double freq : { 100.0, 1000.0, 10000.0 })
        {
            float x = frequencyToX (freq, bounds);
            yup::String label;
            if (freq >= 1000.0)
                label = yup::String (freq / 1000.0, 0) + "k";
            else
                label = yup::String (freq, 0);

            g.fillFittedText (label, font.withHeight (10.0f), { x - 20, bottomLabelSpace.getY(), 40, 15 }, yup::Justification::center);
        }

        // Phase labels
        for (double phase : { -180.0, -90.0, 0.0, 90.0, 180.0 })
        {
            float y = phaseToY (phase, bounds);
            yup::String label = yup::String (phase, 0) + "°";
            g.fillFittedText (label, font.withHeight (10.0f), { bounds.getX() + 5, y - 8, 60, 16 }, yup::Justification::left);
        }
    }

    float frequencyToX (double freq, yup::Rectangle<float> bounds) const
    {
        double logFreq = std::log10 (yup::jlimit (20.0, 20000.0, freq));
        double logMin = std::log10 (20.0);
        double logMax = std::log10 (20000.0);
        return bounds.getX() + (logFreq - logMin) / (logMax - logMin) * bounds.getWidth();
    }

    float phaseToY (double phase, yup::Rectangle<float> bounds) const
    {
        return bounds.getBottom() - (phase + 180.0) / 360.0 * bounds.getHeight();
    }

    std::vector<yup::Point<double>> phaseData;
};

//==============================================================================

class GroupDelayDisplay : public yup::Component
{
public:
    void updateResponse (const std::vector<yup::Point<double>>& data)
    {
        groupDelayData = data;
        repaint();
    }

private:
    void paint (yup::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        // Background
        g.setFillColor (yup::Color (0xFF1E1E1E));
        g.fillRect (bounds);

        // Reserve space for labels
        auto titleBounds = bounds.removeFromTop (20);
        auto bottomLabelSpace = bounds.removeFromBottom (20);

        // Grid
        g.setStrokeColor (yup::Color (0xFF333333));
        g.setStrokeWidth (1.0f);

        // Frequency grid lines
        for (double freq : { 20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 })
        {
            float x = frequencyToX (freq, bounds);
            g.strokeLine ({ x, bounds.getY() }, { x, bounds.getBottom() });
        }

        // Group delay grid lines (in samples at 44.1kHz)
        for (double delay : { 0.0, 1.0, 2.0, 5.0, 10.0, 20.0, 50.0 })
        {
            float y = delayToY (delay, bounds);
            g.strokeLine ({ bounds.getX(), y }, { bounds.getRight(), y });
        }

        // Plot group delay
        if (! groupDelayData.empty())
        {
            yup::Path path;
            bool firstPoint = true;

            g.setStrokeColor (yup::Color (0xFFFF8800));
            g.setStrokeWidth (2.0f);

            for (const auto& point : yup::Span (groupDelayData.data() + 1, groupDelayData.size() - 1))
            {
                float x = frequencyToX (point.getX(), bounds);
                float y = delayToY (point.getY(), bounds);

                if (firstPoint)
                {
                    path.startNewSubPath (x, y);
                    firstPoint = false;
                }
                else
                {
                    path.lineTo (x, y);
                }
            }

            g.strokePath (path);
        }

        // Labels
        g.setFillColor (yup::Colors::white);
        auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f);

        // Title
        g.fillFittedText ("Group Delay", font, titleBounds, yup::Justification::center);

        // Frequency labels
        for (double freq : { 100.0, 1000.0, 10000.0 })
        {
            float x = frequencyToX (freq, bounds);
            yup::String label;
            if (freq >= 1000.0)
                label = yup::String (freq / 1000.0, 0) + "k";
            else
                label = yup::String (freq, 0);

            g.fillFittedText (label, font.withHeight (10.0f), { x - 20, bottomLabelSpace.getY(), 40, 15 }, yup::Justification::center);
        }

        // Delay labels
        for (double delay : { 0.0, 5.0, 10.0, 50.0 })
        {
            float y = delayToY (delay, bounds);
            yup::String label = yup::String (delay, 0) + "s";
            g.fillFittedText (label, font.withHeight (10.0f), { bounds.getX() + 5, y - 8, 60, 16 }, yup::Justification::left);
        }
    }

    float frequencyToX (double freq, yup::Rectangle<float> bounds) const
    {
        double logFreq = std::log10 (yup::jlimit (20.0, 20000.0, freq));
        double logMin = std::log10 (20.0);
        double logMax = std::log10 (20000.0);
        return bounds.getX() + (logFreq - logMin) / (logMax - logMin) * bounds.getWidth();
    }

    float delayToY (double delay, yup::Rectangle<float> bounds) const
    {
        double maxDelay = 50.0; // Max delay in samples
        return bounds.getBottom() - yup::jlimit (0.0, 1.0, delay / maxDelay) * bounds.getHeight();
    }

    std::vector<yup::Point<double>> groupDelayData;
};

//==============================================================================

class StepResponseDisplay : public yup::Component
{
public:
    void updateResponse (const std::vector<yup::Point<double>>& data)
    {
        stepData = data;
        repaint();
    }

private:
    void paint (yup::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        // Background
        g.setFillColor (yup::Color (0xFF1E1E1E));
        g.fillRect (bounds);

        // Reserve space for labels
        auto titleBounds = bounds.removeFromTop (20);
        auto bottomLabelSpace = bounds.removeFromBottom (20);

        // Grid
        g.setStrokeColor (yup::Color (0xFF333333));
        g.setStrokeWidth (1.0f);

        // Time grid lines
        for (int i = 0; i <= 10; ++i)
        {
            float x = bounds.getX() + i * bounds.getWidth() / 10.0f;
            g.strokeLine ({ x, bounds.getY() }, { x, bounds.getBottom() });
        }

        // Amplitude grid lines
        for (double amp : { -1.0, -0.5, 0.0, 0.5, 1.0 })
        {
            float y = amplitudeToY (amp, bounds);
            g.strokeLine ({ bounds.getX(), y }, { bounds.getRight(), y });
        }

        // Zero line
        g.setStrokeColor (yup::Color (0xFF666666));
        g.setStrokeWidth (2.0f);
        float y0 = amplitudeToY (0.0, bounds);
        g.strokeLine ({ bounds.getX(), y0 }, { bounds.getRight(), y0 });

        // Step reference
        g.setStrokeColor (yup::Color (0xFF444444));
        g.setStrokeWidth (1.0f);
        float y1 = amplitudeToY (1.0, bounds);
        g.strokeLine ({ bounds.getX(), y1 }, { bounds.getRight(), y1 });

        // Plot step response
        if (! stepData.empty())
        {
            yup::Path path;
            bool firstPoint = true;

            g.setStrokeColor (yup::Color (0xFF8888FF));
            g.setStrokeWidth (2.0f);

            for (const auto& point : stepData)
            {
                float x = timeToX (point.getX(), bounds);
                float y = amplitudeToY (point.getY(), bounds);

                if (firstPoint)
                {
                    path.startNewSubPath (x, y);
                    firstPoint = false;
                }
                else
                {
                    path.lineTo (x, y);
                }
            }

            g.strokePath (path);
        }

        // Labels
        g.setFillColor (yup::Colors::white);
        auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f);

        // Title
        g.fillFittedText ("Step Response", font, titleBounds, yup::Justification::center);

        // Time labels
        for (int i = 0; i <= 5; ++i)
        {
            float x = bounds.getX() + i * bounds.getWidth() / 5.0f;
            yup::String label = yup::String (i * 20.0f, 0) + "s"; // 20 samples per division
            g.fillFittedText (label, font.withHeight (10.0f), { x - 20, bottomLabelSpace.getY(), 40, 15 }, yup::Justification::center);
        }

        // Amplitude labels
        for (double amp : { -1.0, -0.5, 0.0, 0.5, 1.0 })
        {
            float y = amplitudeToY (amp, bounds);
            yup::String label = yup::String (amp, 1);
            g.fillFittedText (label, font.withHeight (10.0f), { bounds.getX() + 5, y - 8, 40, 16 }, yup::Justification::left);
        }
    }

    float timeToX (double time, yup::Rectangle<float> bounds) const
    {
        double maxTime = 100.0; // 100 samples max
        return bounds.getX() + yup::jlimit (0.0, 1.0, time / maxTime) * bounds.getWidth();
    }

    float amplitudeToY (double amplitude, yup::Rectangle<float> bounds) const
    {
        return bounds.getBottom() - yup::jlimit (0.0, 1.0, (amplitude + 1.0) / 2.0) * bounds.getHeight();
    }

    std::vector<yup::Point<double>> stepData;
};

//==============================================================================

class PolesZerosDisplay : public yup::Component
{
public:
    void updatePolesZeros (const std::vector<std::complex<double>>& poles,
                           const std::vector<std::complex<double>>& zeros)
    {
        this->poles = poles;
        this->zeros = zeros;
        repaint();
    }

private:
    void paint (yup::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        // Background
        g.setFillColor (yup::Color (0xFF1E1E1E));
        g.fillRect (bounds);

        // Reserve space for labels
        auto titleBounds = bounds.removeFromTop (20);
        bounds.removeFromBottom (10); // Just a small margin at bottom

        // Unit circle
        auto center = bounds.getCenter();
        float radius = std::min (bounds.getWidth(), bounds.getHeight()) * 0.4f;

        g.setStrokeColor (yup::Color (0xFF666666));
        g.setStrokeWidth (2.0f);
        g.strokeEllipse (center.getX() - radius, center.getY() - radius, radius * 2, radius * 2);

        // Grid lines
        g.setStrokeColor (yup::Color (0xFF333333));
        g.setStrokeWidth (1.0f);

        // Real axis
        g.strokeLine ({ bounds.getX(), center.getY() }, { bounds.getRight(), center.getY() });
        // Imaginary axis
        g.strokeLine ({ center.getX(), bounds.getY() }, { center.getX(), bounds.getBottom() });

        // Concentric circles at 0.5, 0.8 radii
        for (float r : { 0.5f, 0.8f })
        {
            float circleRadius = radius * r;
            g.strokeEllipse (center.getX() - circleRadius, center.getY() - circleRadius, circleRadius * 2, circleRadius * 2);
        }

        // Plot zeros (circles)
        g.setFillColor (yup::Color (0xFF00FF88));
        g.setStrokeColor (yup::Color (0xFF00AA55));
        g.setStrokeWidth (2.0f);

        for (const auto& zero : zeros)
        {
            float x = center.getX() + static_cast<float> (zero.real()) * radius;
            float y = center.getY() - static_cast<float> (zero.imag()) * radius;

            g.strokeEllipse (x - 4, y - 4, 8, 8);
        }

        // Plot poles (crosses)
        g.setStrokeColor (yup::Color (0xFFFF4444));
        g.setStrokeWidth (3.0f);

        for (const auto& pole : poles)
        {
            float x = center.getX() + static_cast<float> (pole.real()) * radius;
            float y = center.getY() - static_cast<float> (pole.imag()) * radius;

            g.strokeLine ({ x - 5, y - 5 }, { x + 5, y + 5 });
            g.strokeLine ({ x - 5, y + 5 }, { x + 5, y - 5 });
        }

        // Labels
        g.setFillColor (yup::Colors::white);
        auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f);

        // Title
        g.fillFittedText ("Poles & Zeros", font, titleBounds, yup::Justification::center);

        // Axis labels
        g.fillFittedText ("Real", font.withHeight (10.0f), { bounds.getRight() - 40, center.getY() - 8, 35, 16 }, yup::Justification::right);
        g.fillFittedText ("Imag", font.withHeight (10.0f), { center.getX() - 20, bounds.getY() + 5, 40, 16 }, yup::Justification::center);

        // Legend
        auto legendY = bounds.getY();
        g.setStrokeColor (yup::Color (0xFF00FF88));
        g.setStrokeWidth (2.0f);
        g.strokeEllipse (bounds.getX() + 10, legendY, 10, 10);
        g.fillFittedText ("Zeros", font.withHeight (10.0f), { bounds.getX() + 25, legendY, 40, 10 }, yup::Justification::centerLeft);

        g.setStrokeColor (yup::Color (0xFFFF4444));
        g.setStrokeWidth (3.0f);
        legendY += 16;
        g.strokeLine ({ bounds.getX() + 11, legendY + 1 }, { bounds.getX() + 19, legendY + 9 });
        g.strokeLine ({ bounds.getX() + 11, legendY + 9 }, { bounds.getX() + 19, legendY + 1 });
        g.fillFittedText ("Poles", font.withHeight (10.0f), { bounds.getX() + 25, legendY, 40, 10 }, yup::Justification::centerLeft);
    }

    std::vector<std::complex<double>> poles;
    std::vector<std::complex<double>> zeros;
};

//==============================================================================

class FrequencyResponsePlot : public yup::Component
{
public:
    FrequencyResponsePlot()
        : Component ("FrequencyResponsePlot")
        , sampleRate (44100.0)
        , minFreq (20.0)
        , maxFreq (20000.0)
        , minDb (-60.0)
        , maxDb (20.0)
    {
        updateResponseData();
    }

    void setSampleRate (double newSampleRate)
    {
        sampleRate = newSampleRate;
        maxFreq = sampleRate * 0.45; // Nyquist - some margin
        updateResponseData();
    }

    void setFilter (std::shared_ptr<yup::FilterBase<float, double>> newFilter)
    {
        filter = newFilter;
        updateResponseData();
    }

    const std::vector<yup::Point<float>>& getPhaseData() const { return phaseData; }

    const std::vector<yup::Point<float>>& getGroupDelayData() const { return groupDelayData; }

    const std::vector<yup::Point<float>>& getStepResponseData() const { return stepResponseData; }

    void updateResponseData()
    {
        if (! filter)
        {
            repaint();
            return;
        }

        responseData.clear();
        phaseData.clear();
        groupDelayData.clear();
        stepResponseData.clear();

        const int numPoints = 512;

        // Calculate frequency response
        for (int i = 0; i < numPoints; ++i)
        {
            // Logarithmic frequency sweep
            const double ratio = static_cast<double> (i) / (numPoints - 1);
            const double freq = minFreq * std::pow (maxFreq / minFreq, ratio);

            // Get complex response
            auto response = filter->getComplexResponse (freq);

            // Calculate magnitude in dB
            double magnitude = std::abs (response);
            double magnitudeDb = 20.0 * std::log10 (yup::jmax (magnitude, 1e-12));

            // Calculate phase in degrees
            double phaseRad = std::arg (response);
            double phaseDeg = phaseRad * 180.0 / yup::MathConstants<double>::pi;

            // Calculate group delay (numerical derivative of phase)
            double groupDelay = 0.0;
            if (i > 0 && i < numPoints - 1)
            {
                const double deltaFreq = freq * 0.01; // Small frequency step
                auto responseLow = filter->getComplexResponse (freq - deltaFreq);
                auto responseHigh = filter->getComplexResponse (freq + deltaFreq);

                double phaseLow = std::arg (responseLow);
                double phaseHigh = std::arg (responseHigh);

                // Unwrap phase difference
                double phaseDiff = phaseHigh - phaseLow;
                while (phaseDiff > yup::MathConstants<double>::pi)
                    phaseDiff -= 2.0 * yup::MathConstants<double>::pi;
                while (phaseDiff < -yup::MathConstants<double>::pi)
                    phaseDiff += 2.0 * yup::MathConstants<double>::pi;

                groupDelay = -phaseDiff / (2.0 * deltaFreq * 2.0 * yup::MathConstants<double>::pi) * sampleRate;
            }

            responseData.push_back ({ static_cast<float> (freq), static_cast<float> (magnitudeDb) });
            phaseData.push_back ({ static_cast<float> (freq), static_cast<float> (phaseDeg) });
            groupDelayData.push_back ({ static_cast<float> (freq), static_cast<float> (groupDelay) });
        }

        // Calculate step response
        calculateStepResponse();

        repaint();
    }

    void calculateStepResponse()
    {
        if (! filter)
            return;

        stepResponseData.clear();

        // Reset filter state
        filter->reset();

        const int stepLength = 100; // 100 samples

        for (int i = 0; i < stepLength; ++i)
        {
            // Apply unit step input
            float input = (i == 0) ? 1.0f : 0.0f;
            float output = filter->processSample (input);

            stepResponseData.push_back ({ static_cast<float> (i), static_cast<float> (output) });
        }

        // Reset filter again for normal operation
        filter->reset();
    }

    void paint (yup::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        // Background
        g.setFillColor (yup::Color (0xff1a1a1a));
        g.fillAll();

        // Reserve space for labels
        auto titleBounds = bounds.removeFromTop (20);
        auto bottomLabelSpace = bounds.removeFromBottom (20);

        // Grid
        drawGrid (g, bounds);

        // Plot frequency response
        if (! responseData.empty())
        {
            drawMagnitudeResponse (g, bounds);
        }

        // Labels and title
        drawLabels (g, bounds, titleBounds, bottomLabelSpace);
    }

private:
    void drawGrid (yup::Graphics& g, yup::Rectangle<float> bounds)
    {
        g.setStrokeColor (yup::Color (0xff333333));
        g.setStrokeWidth (1.0f);

        // Vertical frequency lines (decades)
        for (double freq = 100.0; freq <= maxFreq; freq *= 10.0)
        {
            float x = frequencyToX (freq, bounds);
            g.strokeLine ({ x, bounds.getY() }, { x, bounds.getBottom() });
        }

        // Horizontal dB lines
        for (double db = -60.0; db <= 20.0; db += 20.0)
        {
            float y = dbToY (db, bounds);
            g.strokeLine ({ bounds.getX(), y }, { bounds.getRight(), y });
        }

        // 0 dB line
        g.setStrokeColor (yup::Color (0xff666666));
        g.setStrokeWidth (2.0f);
        float y0db = dbToY (0.0, bounds);
        g.strokeLine ({ bounds.getX(), y0db }, { bounds.getRight(), y0db });
    }

    void drawMagnitudeResponse (yup::Graphics& g, yup::Rectangle<float> bounds)
    {
        if (responseData.size() < 2)
            return;

        yup::Path path;
        bool firstPoint = true;

        for (const auto& point : responseData)
        {
            float x = frequencyToX (point.getX(), bounds);
            float y = dbToY (point.getY(), bounds);

            if (firstPoint)
            {
                path.moveTo (x, y);
                firstPoint = false;
            }
            else
            {
                path.lineTo (x, y);
            }
        }

        // Draw the response curve
        g.setStrokeColor (yup::Color (0xff4fc3f7));
        g.setStrokeWidth (3.0f);
        g.strokePath (path);

        // Add glow effect
        g.setStrokeColor (yup::Color (0xff4fc3f7).withAlpha (0.3f));
        g.setStrokeWidth (6.0f);
        g.strokePath (path);
    }

    void drawLabels (yup::Graphics& g, yup::Rectangle<float> bounds, yup::Rectangle<float> titleBounds, yup::Rectangle<float> bottomLabelSpace)
    {
        g.setFillColor (yup::Colors::white);
        auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f);

        // Title
        g.fillFittedText ("Filter Frequency Response", font, titleBounds, yup::Justification::center);

        // Frequency labels
        for (double freq = 100.0; freq <= maxFreq; freq *= 10.0)
        {
            float x = frequencyToX (freq, bounds);
            yup::String label;

            if (freq >= 1000.0)
                label = yup::String (freq / 1000.0, 0) + "k";
            else
                label = yup::String (freq, 0);

            g.fillFittedText (label, font.withHeight (10.0f), { x - 20, bottomLabelSpace.getY(), 40, 15 }, yup::Justification::center);
        }

        // dB labels
        for (double db = -60.0; db <= 20.0; db += 20.0)
        {
            float y = dbToY (db, bounds);
            yup::String label = yup::String (db, 0) + " dB";
            g.fillFittedText (label, font.withHeight (10.0f), { bounds.getX() + 5, y - 8, 60, 16 }, yup::Justification::left);
        }
    }

    float frequencyToX (double freq, yup::Rectangle<float> bounds) const
    {
        double ratio = std::log (freq / minFreq) / std::log (maxFreq / minFreq);
        return static_cast<float> (bounds.getX() + ratio * bounds.getWidth());
    }

    float dbToY (double db, yup::Rectangle<float> bounds) const
    {
        double ratio = (db - minDb) / (maxDb - minDb);
        return static_cast<float> (bounds.getBottom() - ratio * bounds.getHeight());
    }

    std::shared_ptr<yup::FilterBase<float, double>> filter;
    std::vector<yup::Point<float>> responseData;
    std::vector<yup::Point<float>> phaseData;
    std::vector<yup::Point<float>> groupDelayData;
    std::vector<yup::Point<float>> stepResponseData;

    double sampleRate;
    double minFreq, maxFreq;
    double minDb, maxDb;
};

//==============================================================================

class FilterOscilloscope : public yup::Component
{
public:
    void setRenderData (const std::vector<float>& data, int newReadPos)
    {
        renderData = data;
    }

    void paint (yup::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        g.setFillColor (yup::Color (0xff101010));
        g.fillAll();

        if (renderData.empty())
            return;

        yup::Path path;
        float xStep = static_cast<float> (bounds.getWidth()) / renderData.size();
        float centerY = bounds.getHeight() * 0.5f;

        path.moveTo (0, centerY + renderData[0] * centerY);
        for (size_t i = 1; i < renderData.size(); ++i)
            path.lineTo (i * xStep, centerY + renderData[i] * centerY);

        g.setStrokeColor (yup::Color (0xff4fc3f7));
        g.setStrokeWidth (2.0f);
        g.strokePath (path);
    }

private:
    std::vector<float> renderData;
};

//==============================================================================

class FilterDemo
    : public yup::Component
    , public yup::AudioIODeviceCallback
{
public:
    FilterDemo()
        : Component ("FilterDemo")
    {
        // Initialize audio device
        deviceManager.initialiseWithDefaultDevices (0, 2);

        // Create UI components
        setupUI();

        // Initialize filters
        initializeFilters();

        // Set default parameters
        setDefaultParameters();
    }

    ~FilterDemo() override
    {
        deviceManager.removeAudioCallback (this);
        deviceManager.closeAudioDevice();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Title area
        auto titleBounds = bounds.removeFromTop (40);
        titleLabel->setBounds (titleBounds);

        // Control panel area (left side)
        auto controlPanelWidth = proportionOfWidth (0.25f);
        auto controlPanel = bounds.removeFromLeft (controlPanelWidth);
        layoutControlPanel (controlPanel);

        // Analysis displays area (right side)
        auto analysisArea = bounds;

        // Create a 3x2 grid for the analysis displays
        int margin = 5;
        int displayWidth = (analysisArea.getWidth() - 3 * margin) / 2;
        int displayHeight = (analysisArea.getHeight() - 4 * margin) / 3;

        // Top row: Frequency Response and Phase Response
        frequencyResponsePlot.setBounds (analysisArea.getX() + margin,
                                         analysisArea.getY() + margin,
                                         displayWidth,
                                         displayHeight);

        phaseResponseDisplay.setBounds (analysisArea.getX() + displayWidth + 2 * margin,
                                        analysisArea.getY() + margin,
                                        displayWidth,
                                        displayHeight);

        // Middle row: Group Delay and Step Response
        groupDelayDisplay.setBounds (analysisArea.getX() + margin,
                                     analysisArea.getY() + displayHeight + 2 * margin,
                                     displayWidth,
                                     displayHeight);

        stepResponseDisplay.setBounds (analysisArea.getX() + displayWidth + 2 * margin,
                                       analysisArea.getY() + displayHeight + 2 * margin,
                                       displayWidth,
                                       displayHeight);

        // Bottom row: Poles/Zeros and Oscilloscope
        polesZerosDisplay.setBounds (analysisArea.getX() + margin,
                                     analysisArea.getY() + 2 * displayHeight + 3 * margin,
                                     displayWidth,
                                     displayHeight);

        oscilloscope.setBounds (analysisArea.getX() + displayWidth + 2 * margin,
                                analysisArea.getY() + 2 * displayHeight + 3 * margin,
                                displayWidth,
                                displayHeight);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();
    }

    void refreshDisplay (double lastFrameTimeSeconds) override
    {
        // Update oscilloscope
        {
            const yup::CriticalSection::ScopedLockType sl (renderMutex);
            oscilloscope.setRenderData (renderData, readPos);
        }

        if (oscilloscope.isVisible())
            oscilloscope.repaint();
    }

    void visibilityChanged() override
    {
        if (! isVisible())
            deviceManager.removeAudioCallback (this);
        else
            deviceManager.addAudioCallback (this);
    }

    // AudioIODeviceCallback methods
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const yup::AudioIODeviceCallbackContext& context) override
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Check if any parameters are changing and update filter coefficients if needed
            if (smoothedFrequency.isSmoothing() || smoothedQ.isSmoothing() ||
                smoothedGain.isSmoothing() || smoothedOrder.isSmoothing())
            {
                updateAudioFilterParametersSmooth();
            }

            // Generate white noise
            float noiseSample = noiseGenerator.getNextSample();

            // Apply current audio filter
            float filteredSample = noiseSample;
            if (currentAudioFilter)
                filteredSample = currentAudioFilter->processSample (noiseSample);

            // Apply output gain
            filteredSample *= outputGain.getNextValue();

            // Output to all channels
            for (int channel = 0; channel < numOutputChannels; ++channel)
                outputChannelData[channel][sample] = filteredSample;

            // Store for oscilloscope
            auto pos = readPos.fetch_add (1);
            inputData[pos % inputData.size()] = filteredSample;
            readPos = readPos % inputData.size();
        }

        // Update render data for oscilloscope
        const yup::CriticalSection::ScopedLockType sl (renderMutex);
        std::swap (inputData, renderData);
    }

    void audioDeviceAboutToStart (yup::AudioIODevice* device) override
    {
        double sampleRate = device->getCurrentSampleRate();

        // Setup noise generator
        noiseGenerator.setSampleRate (sampleRate);
        outputGain.reset (sampleRate, 0.02);

        // Initialize smoothed parameter values
        smoothedFrequency.reset (sampleRate, 0.05); // 50ms smoothing time
        smoothedQ.reset (sampleRate, 0.05);
        smoothedGain.reset (sampleRate, 0.05);
        smoothedOrder.reset (sampleRate, 0.1); // Slower for order changes

        // Set initial values
        smoothedFrequency.setCurrentAndTargetValue (static_cast<float> (frequencySlider->getValue()));
        smoothedQ.setCurrentAndTargetValue (static_cast<float> (qSlider->getValue()));
        smoothedGain.setCurrentAndTargetValue (static_cast<float> (gainSlider->getValue()));
        smoothedOrder.setCurrentAndTargetValue (static_cast<float> (orderSlider->getValue()));

        // Prepare all audio filters
        for (auto& filter : allAudioFilters)
        {
            if (filter)
                filter->prepare (sampleRate, device->getCurrentBufferSizeSamples());
        }

        // Prepare all UI filters
        for (auto& filter : allUIFilters)
        {
            if (filter)
                filter->prepare (sampleRate, device->getCurrentBufferSizeSamples());
        }

        // Initialize audio buffers
        inputData.resize (device->getCurrentBufferSizeSamples());
        renderData.resize (inputData.size());
        readPos = 0;

        // Store sample rate for parameter updates
        currentSampleRate = sampleRate;

        // Setup frequency response plot
        frequencyResponsePlot.setSampleRate (sampleRate);

        // Update current audio filter based on stored settings
        updateCurrentAudioFilter();
    }

    void audioDeviceStopped() override
    {
    }

private:
    void setupUI()
    {
        // Title
        titleLabel = std::make_unique<yup::Label> ("Title");
        titleLabel->setText ("YUP DSP Filter Demo");
        titleLabel->setColor (yup::Label::Style::textFillColorId, yup::Colors::white);
        //titleLabel->setJustification (yup::Justification::center);
        addAndMakeVisible (*titleLabel);

        // Filter type selector
        filterTypeCombo = std::make_unique<yup::ComboBox> ("FilterType");
        filterTypeCombo->addItem ("RBJ", 1);
        filterTypeCombo->addItem ("State Variable", 2);
        filterTypeCombo->addItem ("First Order", 3);
        filterTypeCombo->setSelectedId (1);
        filterTypeCombo->onSelectedItemChanged = [this]
        {
            updateCurrentFilter();
        };
        addAndMakeVisible (*filterTypeCombo);

        // Response type selector
        responseTypeCombo = std::make_unique<yup::ComboBox> ("ResponseType");
        responseTypeCombo->addItem ("Lowpass", 1);
        responseTypeCombo->addItem ("Highpass", 2);
        responseTypeCombo->addItem ("Bandpass", 3);
        responseTypeCombo->addItem ("Bandstop", 4);
        responseTypeCombo->addItem ("Peak", 5);
        responseTypeCombo->addItem ("Low Shelf", 6);
        responseTypeCombo->addItem ("High Shelf", 7);
        responseTypeCombo->addItem ("Allpass", 8);
        responseTypeCombo->setSelectedId (1);
        responseTypeCombo->onSelectedItemChanged = [this]
        {
            updateCurrentFilter();
        };
        addAndMakeVisible (*responseTypeCombo);

        // Parameter controls with smoothed parameter updates
        frequencySlider = std::make_unique<yup::Slider> (yup::Slider::LinearBarHorizontal, "Frequency");
        frequencySlider->setRange ({ 20.0, 20000.0 });
        frequencySlider->setSkewFactorFromMidpoint (1000.0); // 1kHz at midpoint
        frequencySlider->setValue (1000.0);
        frequencySlider->onValueChanged = [this] (float value)
        {
            smoothedFrequency.setTargetValue (value);
            updateAnalysisDisplays();
        };
        addAndMakeVisible (*frequencySlider);

        qSlider = std::make_unique<yup::Slider> (yup::Slider::LinearBarHorizontal, "Q / Resonance");
        qSlider->setRange ({ 0.0, 1.0 });
        qSlider->setSkewFactorFromMidpoint (0.3); // More resolution at lower Q values
        qSlider->setValue (0.0);
        qSlider->onValueChanged = [this] (float value)
        {
            smoothedQ.setTargetValue (value);
            updateAnalysisDisplays();
        };
        addAndMakeVisible (*qSlider);

        gainSlider = std::make_unique<yup::Slider> (yup::Slider::LinearBarHorizontal, "Gain (dB)");
        gainSlider->setRange ({ -48.0, 20.0 });
        gainSlider->setSkewFactorFromMidpoint (0.0); // 0 dB at midpoint
        gainSlider->setValue (0.0);
        gainSlider->onValueChanged = [this] (float value)
        {
            smoothedGain.setTargetValue (value);
            updateAnalysisDisplays();
        };
        addAndMakeVisible (*gainSlider);

        orderSlider = std::make_unique<yup::Slider> (yup::Slider::LinearBarHorizontal, "Order");
        orderSlider->setRange ({ 1.0, 10.0 });
        orderSlider->setValue (2.0);
        orderSlider->onValueChanged = [this] (float value)
        {
            smoothedOrder.setTargetValue (value);
            updateAnalysisDisplays();
        };
        addAndMakeVisible (*orderSlider);

        // Noise gain control
        noiseGainSlider = std::make_unique<yup::Slider> (yup::Slider::LinearBarHorizontal, "Noise Level");
        noiseGainSlider->setRange ({ 0.0, 1.0 });
        noiseGainSlider->setValue (0.1);
        noiseGainSlider->onValueChanged = [this] (float value)
        {
            noiseGenerator.setAmplitude (value);
        };
        addAndMakeVisible (*noiseGainSlider);

        // Output gain control
        outputGainSlider = std::make_unique<yup::Slider> (yup::Slider::LinearBarHorizontal, "Output Level");
        outputGainSlider->setRange ({ 0.0, 1.0 });
        outputGainSlider->setValue (0.5);
        outputGainSlider->onValueChanged = [this] (float value)
        {
            outputGain.setTargetValue (value);
        };
        addAndMakeVisible (*outputGainSlider);

        // Frequency response plot
        addAndMakeVisible (frequencyResponsePlot);

        // Additional analysis displays
        addAndMakeVisible (phaseResponseDisplay);
        addAndMakeVisible (groupDelayDisplay);
        addAndMakeVisible (stepResponseDisplay);
        addAndMakeVisible (polesZerosDisplay);

        // Oscilloscope
        addAndMakeVisible (oscilloscope);

        // Labels for parameter controls
        auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (10.0f);

        for (const auto& labelText : { "Filter Type:", "Response Type:", "Frequency:", "Q/Resonance:", "Gain (dB):", "Order:", "Noise Level:", "Output Level:" })
        {
            auto label = parameterLabels.add (std::make_unique<yup::Label> (labelText));
            label->setText (labelText);
            label->setColor (yup::Label::Style::textFillColorId, yup::Colors::lightgray);
            label->setFont (font);
            addAndMakeVisible (*label);
        }
    }

    void layoutControlPanel (yup::Rectangle<float> bounds)
    {
        bounds = bounds.reduced (10);

        int rowHeight = 60;
        int labelHeight = 15;
        int spacing = 5;

        auto layouts = std::vector<std::pair<yup::Label*, yup::Component*>> {
            { parameterLabels[0], filterTypeCombo.get() },
            { parameterLabels[1], responseTypeCombo.get() },
            { parameterLabels[2], frequencySlider.get() },
            { parameterLabels[3], qSlider.get() },
            { parameterLabels[4], gainSlider.get() },
            { parameterLabels[5], orderSlider.get() },
            { parameterLabels[6], noiseGainSlider.get() },
            { parameterLabels[7], outputGainSlider.get() }
        };

        for (auto& [label, component] : layouts)
        {
            auto row = bounds.removeFromTop (rowHeight);
            auto labelBounds = row.removeFromTop (labelHeight);
            label->setBounds (labelBounds);
            component->setBounds (row.reduced (5));
            bounds.removeFromTop (spacing);
        }
    }

    void initializeFilters()
    {
        // Create instances of all filter types for audio thread
        audioRbj = std::make_shared<yup::RbjFilter<float>>();
        audioSvf = std::make_shared<yup::StateVariableFilter<float>>();
        audioFirstOrder = std::make_shared<yup::FirstOrderFilter<float>>();

        // Create instances of all filter types for UI thread
        uiRbj = std::make_shared<yup::RbjFilter<float>>();
        uiSvf = std::make_shared<yup::StateVariableFilter<float>>();
        uiFirstOrder = std::make_shared<yup::FirstOrderFilter<float>>();

        // Store in arrays for easy management
        allAudioFilters = {
            audioRbj, audioSvf, audioFirstOrder
        };

        allUIFilters = {
            uiRbj, uiSvf, uiFirstOrder
        };

        // Set default filters
        currentAudioFilter = audioRbj;
        currentUIFilter = uiRbj;

        // Set default filter type settings
        currentFilterTypeId = 1; // RBJ
        currentResponseTypeId = 1; // Lowpass
    }

    void setDefaultParameters()
    {
        noiseGenerator.setAmplitude (0.1f);
        outputGain.setCurrentAndTargetValue (0.5f);
        updateCurrentFilter();
    }

    void updateCurrentFilter()
    {
        // Store filter type settings for audio thread
        currentFilterTypeId = filterTypeCombo->getSelectedId();
        currentResponseTypeId = responseTypeCombo->getSelectedId();

        // Map combo box selection to UI filter instance
        switch (currentFilterTypeId)
        {
            case 1: currentUIFilter = uiRbj; break;
            case 2: currentUIFilter = uiSvf; break;
            case 3: currentUIFilter = uiFirstOrder; break;
            default: currentUIFilter = uiRbj; break;
        }

        // Synchronize smoothed values with current UI values when switching filters
        smoothedFrequency.setCurrentAndTargetValue (static_cast<float> (frequencySlider->getValue()));
        smoothedQ.setCurrentAndTargetValue (static_cast<float> (qSlider->getValue()));
        smoothedGain.setCurrentAndTargetValue (static_cast<float> (gainSlider->getValue()));
        smoothedOrder.setCurrentAndTargetValue (static_cast<float> (orderSlider->getValue()));

        // Update audio filter selection (thread-safe since we're just changing a pointer)
        updateCurrentAudioFilter();

        // Update UI filter with current parameters
        updateUIFilterParameters();

        // Update displays using UI filter
        frequencyResponsePlot.setFilter (currentUIFilter);
        frequencyResponsePlot.updateResponseData();
        updateAnalysisDisplays();
    }

    void updateAudioFilterParametersSmooth()
    {
        if (! currentAudioFilter)
            return;

        double freq = smoothedFrequency.getNextValue();
        double q = smoothedQ.getNextValue();
        double gain = smoothedGain.getNextValue();
        int order = static_cast<int> (smoothedOrder.getNextValue());

        // Update parameters based on filter type using smoothed values and stored filter type
        if (auto rf = std::dynamic_pointer_cast<yup::RbjFilter<float>> (currentAudioFilter))
        {
            rf->setParameters (getRbjMode (currentResponseTypeId), freq, 0.1f + q * 10.0f, gain, currentSampleRate);
        }
        else if (auto svf = std::dynamic_pointer_cast<yup::StateVariableFilter<float>> (currentAudioFilter))
        {
            svf->setParameters (getSvfMode (currentResponseTypeId), freq, 0.707 + q * (10.0f - 0.707), currentSampleRate);
        }
        else if (auto fof = std::dynamic_pointer_cast<yup::FirstOrderFilter<float>> (currentAudioFilter))
        {
            auto coeffs = getFirstOrderCoefficients (currentResponseTypeId, freq, gain, currentSampleRate);
            fof->setCoefficients (coeffs);
        }
    }

    void updateUIFilterParameters()
    {
        if (! currentUIFilter)
            return;

        double freq = frequencySlider->getValue();
        double q = qSlider->getValue();
        double gain = gainSlider->getValue();
        int order = static_cast<int> (orderSlider->getValue());

        // Update parameters based on filter type using direct UI values
        if (auto rf = std::dynamic_pointer_cast<yup::RbjFilter<float>> (currentUIFilter))
        {
            rf->setParameters (getRbjMode (currentResponseTypeId), freq, 0.1f + q * 10.0f, gain, currentSampleRate);
        }
        else if (auto svf = std::dynamic_pointer_cast<yup::StateVariableFilter<float>> (currentUIFilter))
        {
            svf->setParameters (getSvfMode (currentResponseTypeId), freq, 0.707 + q * (10.0f - 0.707), currentSampleRate);
        }
        else if (auto fof = std::dynamic_pointer_cast<yup::FirstOrderFilter<float>> (currentUIFilter))
        {
            auto coeffs = getFirstOrderCoefficients (currentResponseTypeId, freq, gain, currentSampleRate);
            fof->setCoefficients (coeffs);
        }
    }

    void updateCurrentAudioFilter()
    {
        // Map filter type to audio filter instance (using stored filter type, not UI)
        switch (currentFilterTypeId)
        {
            case 1: currentAudioFilter = audioRbj; break;
            case 2: currentAudioFilter = audioSvf; break;
            case 3: currentAudioFilter = audioFirstOrder; break;
            default: currentAudioFilter = audioRbj; break;
        }

        // Synchronize smoothed values with current UI values when switching filters
        smoothedFrequency.setCurrentAndTargetValue (static_cast<float> (frequencySlider->getValue()));
        smoothedQ.setCurrentAndTargetValue (static_cast<float> (qSlider->getValue()));
        smoothedGain.setCurrentAndTargetValue (static_cast<float> (gainSlider->getValue()));
        smoothedOrder.setCurrentAndTargetValue (static_cast<float> (orderSlider->getValue()));

        // Update audio filter with current smoothed parameters
        updateAudioFilterParametersSmooth();
    }

    void updateAnalysisDisplays()
    {
        if (! currentUIFilter)
            return;

        // Update UI filter parameters first
        updateUIFilterParameters();

        // Update frequency response plot
        frequencyResponsePlot.setFilter (currentUIFilter);
        frequencyResponsePlot.updateResponseData();

        // Update phase response
        auto phaseData = frequencyResponsePlot.getPhaseData();
        std::vector<yup::Point<double>> phaseDataDouble;
        for (const auto& point : phaseData)
            phaseDataDouble.push_back ({ static_cast<double> (point.getX()), static_cast<double> (point.getY()) });
        phaseResponseDisplay.updateResponse (phaseDataDouble);

        // Update group delay
        auto groupDelayData = frequencyResponsePlot.getGroupDelayData();
        std::vector<yup::Point<double>> groupDelayDataDouble;
        for (const auto& point : groupDelayData)
            groupDelayDataDouble.push_back ({ static_cast<double> (point.getX()), static_cast<double> (point.getY()) });
        groupDelayDisplay.updateResponse (groupDelayDataDouble);

        // Update step response
        auto stepData = frequencyResponsePlot.getStepResponseData();
        std::vector<yup::Point<double>> stepDataDouble;
        for (const auto& point : stepData)
            stepDataDouble.push_back ({ static_cast<double> (point.getX()), static_cast<double> (point.getY()) });
        stepResponseDisplay.updateResponse (stepDataDouble);

        // Update poles and zeros
        updatePolesZerosDisplay();
    }

    void updateDisplayParameters()
    {
        if (! currentUIFilter)
            return;

        // Update UI filter parameters and displays
        updateUIFilterParameters();
        frequencyResponsePlot.setFilter (currentUIFilter);
        frequencyResponsePlot.updateResponseData();
        updateAnalysisDisplays();
    }

    void updatePolesZerosDisplay()
    {
        poles.clear();
        zeros.clear();

        if (currentUIFilter != nullptr)
            currentUIFilter->getPolesZeros (poles, zeros);

        polesZerosDisplay.updatePolesZeros (poles, zeros);
    }

    yup::FilterMode getFilterType (int responseTypeId)
    {
        switch (responseTypeId)
        {
            case 1:
                return yup::FilterMode::lowpass;
            case 2:
                return yup::FilterMode::highpass;
            case 3:
                return yup::FilterMode::bandpass;
            case 4:
                return yup::FilterMode::bandstop;
            case 5:
                return yup::FilterMode::peak;
            case 6:
                return yup::FilterMode::lowshelf;
            case 7:
                return yup::FilterMode::highshelf;
            case 8:
                return yup::FilterMode::allpass;
            default:
                return yup::FilterMode::lowpass;
        }
    }

    yup::RbjFilter<float>::Mode getRbjMode (int responseTypeId)
    {
        switch (responseTypeId)
        {
            case 1:
                return yup::RbjFilter<float>::Mode::lowpass;
            case 2:
                return yup::RbjFilter<float>::Mode::highpass;
            case 3:
                return yup::RbjFilter<float>::Mode::bandpassCsg;
            case 4:
                return yup::RbjFilter<float>::Mode::notch;
            case 5:
                return yup::RbjFilter<float>::Mode::peaking;
            case 6:
                return yup::RbjFilter<float>::Mode::lowshelf;
            case 7:
                return yup::RbjFilter<float>::Mode::highshelf;
            case 8:
                return yup::RbjFilter<float>::Mode::allpass;
            default:
                return yup::RbjFilter<float>::Mode::lowpass;
        }
    }

    yup::StateVariableFilter<float>::Mode getSvfMode (int responseTypeId)
    {
        switch (responseTypeId)
        {
            case 1:
                return yup::StateVariableFilter<float>::Mode::lowpass;
            case 2:
                return yup::StateVariableFilter<float>::Mode::highpass;
            case 3:
                return yup::StateVariableFilter<float>::Mode::bandpass;
            case 4:
                return yup::StateVariableFilter<float>::Mode::notch;
            default:
                return yup::StateVariableFilter<float>::Mode::lowpass;
        }
    }

    yup::FirstOrderCoefficients<double> getFirstOrderCoefficients (int responseTypeId, double freq, double gain, double sampleRate)
    {
        switch (responseTypeId)
        {
            case 1:
                return yup::FilterDesigner<double>::designFirstOrderLowpass (freq, sampleRate);
            case 2:
                return yup::FilterDesigner<double>::designFirstOrderHighpass (freq, sampleRate);
            case 6:
                return yup::FilterDesigner<double>::designFirstOrderLowShelf (freq, gain, sampleRate);
            case 7:
                return yup::FilterDesigner<double>::designFirstOrderHighShelf (freq, gain, sampleRate);
            case 8:
                return yup::FilterDesigner<double>::designFirstOrderAllpass (freq, sampleRate);
            default:
                return yup::FilterDesigner<double>::designFirstOrderLowpass (freq, sampleRate);
        }
    }

    // Audio components
    yup::AudioDeviceManager deviceManager;
    WhiteNoiseGenerator noiseGenerator;
    yup::SmoothedValue<float> outputGain { 0.5f };

    // Smoothed parameter values for interpolation
    yup::SmoothedValue<float> smoothedFrequency { 1000.0f };
    yup::SmoothedValue<float> smoothedQ { 0.1f };
    yup::SmoothedValue<float> smoothedGain { 0.0f };
    yup::SmoothedValue<float> smoothedOrder { 2.0f };

    double currentSampleRate = 44100.0;
    std::atomic<bool> needsDisplayUpdate { false };
    int displayUpdateCounter = 0;

    std::vector<std::complex<double>> poles;
    std::vector<std::complex<double>> zeros;

    // Filter type settings (thread-safe storage)
    std::atomic<int> currentFilterTypeId { 1 };
    std::atomic<int> currentResponseTypeId { 1 };

    // Audio thread filter instances
    std::shared_ptr<yup::RbjFilter<float>> audioRbj;
    std::shared_ptr<yup::StateVariableFilter<float>> audioSvf;
    std::shared_ptr<yup::FirstOrderFilter<float>> audioFirstOrder;

    // UI thread filter instances
    std::shared_ptr<yup::RbjFilter<float>> uiRbj;
    std::shared_ptr<yup::StateVariableFilter<float>> uiSvf;
    std::shared_ptr<yup::FirstOrderFilter<float>> uiFirstOrder;

    std::vector<std::shared_ptr<yup::FilterBase<float>>> allAudioFilters;
    std::vector<std::shared_ptr<yup::FilterBase<float>>> allUIFilters;
    std::shared_ptr<yup::FilterBase<float>> currentAudioFilter;
    std::shared_ptr<yup::FilterBase<float>> currentUIFilter;

    // UI Components
    std::unique_ptr<yup::Label> titleLabel;
    std::unique_ptr<yup::ComboBox> filterTypeCombo;
    std::unique_ptr<yup::ComboBox> responseTypeCombo;
    std::unique_ptr<yup::Slider> frequencySlider;
    std::unique_ptr<yup::Slider> qSlider;
    std::unique_ptr<yup::Slider> gainSlider;
    std::unique_ptr<yup::Slider> orderSlider;
    std::unique_ptr<yup::Slider> noiseGainSlider;
    std::unique_ptr<yup::Slider> outputGainSlider;
    yup::OwnedArray<yup::Label> parameterLabels;

    // Visualization components
    FrequencyResponsePlot frequencyResponsePlot;
    PhaseResponseDisplay phaseResponseDisplay;
    GroupDelayDisplay groupDelayDisplay;
    StepResponseDisplay stepResponseDisplay;
    PolesZerosDisplay polesZerosDisplay;
    FilterOscilloscope oscilloscope;

    // Audio buffer management
    std::vector<float> inputData;
    std::vector<float> renderData;
    yup::CriticalSection renderMutex;
    std::atomic_int readPos { 0 };
};
