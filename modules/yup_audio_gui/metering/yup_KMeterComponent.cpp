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

namespace yup
{

//==============================================================================
const Identifier KMeterComponent::Style::backgroundColorId { "backgroundColorId" };
const Identifier KMeterComponent::Style::greenZoneColorId { "greenZoneColorId" };
const Identifier KMeterComponent::Style::amberZoneColorId { "amberZoneColorId" };
const Identifier KMeterComponent::Style::redZoneColorId { "redZoneColorId" };
const Identifier KMeterComponent::Style::averageLevelColorId { "averageLevelColorId" };
const Identifier KMeterComponent::Style::peakLevelColorId { "peakLevelColorId" };
const Identifier KMeterComponent::Style::peakLevelClipColorId { "peakLevelClipColorId" };
const Identifier KMeterComponent::Style::peakHoldColorId { "peakHoldColorId" };

//==============================================================================
KMeterComponent::KMeterComponent (KMeterState& state, int channel, StringRef componentID)
    : Component (componentID)
    , meterState (state)
    , channelIndex (channel)
{
    setOpaque (true);
}

KMeterComponent::~KMeterComponent()
{
}

//==============================================================================
void KMeterComponent::setShowPeakHold (bool shouldShow)
{
    if (showPeakHold != shouldShow)
    {
        showPeakHold = shouldShow;
        repaint();
    }
}

void KMeterComponent::setShowPeak (bool shouldShow)
{
    if (showPeak != shouldShow)
    {
        showPeak = shouldShow;
        repaint();
    }
}

void KMeterComponent::setScaleMapping (ScaleMapping mapping)
{
    if (scaleMapping != mapping)
    {
        scaleMapping = mapping;
        repaint();
    }
}

void KMeterComponent::setOverCounterMode (KMeterState::OverCounterMode mode)
{
    meterState.setOverCounterMode (mode);
}

void KMeterComponent::setRefreshRate (int hz)
{
    refreshRate = jlimit (10, 60, hz);
}

void KMeterComponent::setAverageIntegrationTime (double seconds)
{
    meterState.setIntegrationTime (seconds);
}

void KMeterComponent::setAverageFallTime (double seconds)
{
    meterState.setAverageFallTime (seconds);
}

void KMeterComponent::setPeakHoldTime (double seconds)
{
    meterState.setPeakHoldTime (seconds);
}

//==============================================================================
void KMeterComponent::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
    {
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
    }
}

void KMeterComponent::refreshDisplay (double lastFrameTimeSeconds)
{
    Component::refreshDisplay (lastFrameTimeSeconds);

    timeSinceLastUpdate += lastFrameTimeSeconds;

    // Update at configured refresh rate
    const double updateInterval = 1.0 / refreshRate;
    if (timeSinceLastUpdate >= updateInterval)
    {
        timeSinceLastUpdate = 0.0;
        updateLevels();
        checkForCallbacks();
        repaint();
    }
}

//==============================================================================
void KMeterComponent::updateLevels()
{
    // Read levels from KMeterState (thread-safe via atomics)
    const float peakDb = meterState.getPeakLevel (channelIndex);
    const float averageDb = meterState.getAverageLevel (channelIndex);
    const float peakHoldDb = meterState.getPeakHoldLevel (channelIndex);
    const bool clipping = meterState.isClipping();

    // Store in atomics for paint thread (no additional smoothing needed)
    currentPeakDb.set (peakDb);
    currentAverageDb.set (averageDb);
    currentPeakHoldDb.set (peakHoldDb);
    currentClipping.set (clipping);
}

void KMeterComponent::checkForCallbacks()
{
    const float peakDb = currentPeakDb.get();
    const float avgDb = currentAverageDb.get();
    const bool clipping = currentClipping.get();

    // Level changed callback (throttled to avoid excessive calls)
    if (onLevelChanged)
    {
        constexpr float threshold = 0.5f; // 0.5 dB change triggers callback
        if (std::abs (peakDb - lastCallbackPeak) > threshold || std::abs (avgDb - lastCallbackAverage) > threshold)
        {
            lastCallbackPeak = peakDb;
            lastCallbackAverage = avgDb;
            onLevelChanged (peakDb, avgDb);
        }
    }

    // Clipping state changed callback
    if (onClippingChanged && clipping != lastCallbackClipping)
    {
        lastCallbackClipping = clipping;
        onClippingChanged (clipping);
    }
}

} // namespace yup
