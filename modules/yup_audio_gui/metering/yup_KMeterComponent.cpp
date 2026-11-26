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

const Identifier KMeterComponent::clippingPropertyId { "clipping" };

//==============================================================================
const ComponentTypeInfo& KMeterComponent::getClassTypeInfo()
{
    using M = KMeterComponent;

    static const auto info = ComponentTypeBuilder<M> (Identifier ("KMeter"))
                                 .color ("backgroundColor", Style::backgroundColorId)
                                 .color ("greenZoneColor", Style::greenZoneColorId)
                                 .color ("amberZoneColor", Style::amberZoneColorId)
                                 .color ("redZoneColor", Style::redZoneColorId)
                                 .color ("averageLevelColor", Style::averageLevelColorId)
                                 .color ("peakLevelColor", Style::peakLevelColorId)
                                 .color ("peakLevelClipColor", Style::peakLevelClipColorId)
                                 .color ("peakHoldColor", Style::peakHoldColorId)
                                 .property ("channel", &M::getChannel, &M::setChannel, -1, "The channel of the state shown, -1 for the loudest of all its channels.")
                                 .property ("showPeak", &M::getShowPeak, &M::setShowPeak, true, "Whether the moving peak indicator is shown.")
                                 .property ("showPeakHold", &M::getShowPeakHold, &M::setShowPeakHold, true, "Whether the peak hold marker is shown.")
                                 .enumeration<ScaleMapping> (
                                     "scaleMapping",
                                     { { "segmented", ScaleMapping::segmented },
                                       { "linear", ScaleMapping::linear } },
                                     &M::getScaleMapping,
                                     &M::setScaleMapping,
                                     "How the levels are placed along the meter: with the segments of the K-System, or linearly in dB.")
                                 .property ("refreshRate", &M::getRefreshRate, &M::setRefreshRate, 30, "The updates a second, from 10 to 60.")
                                 .readOnly ("peakDb", [] (const M& m) { return m.currentPeakDb.get(); }, -100.0, "The peak level shown, in dB.")
                                 .readOnly ("averageDb", [] (const M& m) { return m.currentAverageDb.get(); }, -100.0, "The average level shown, in dB.")
                                 .readOnly ("peakHoldDb", [] (const M& m) { return m.currentPeakHoldDb.get(); }, -100.0, "The peak level held, in dB.")
                                 .state (clippingPropertyId, [] (const M& m) { return m.currentClipping.get(); }, false, "Whether the signal is clipping.")
                                 .readOnly ("rangeMinDb", [] (const M& m) { return KMeterState::rangeMinForScale (m.meterState != nullptr ? m.meterState->getScale() : KMeterState::Scale::k20); }, {}, "The lowest level of the scale of the meter, in dB.")
                                 .readOnly ("rangeMaxDb", [] (const M& m) { return KMeterState::rangeMaxForScale (m.meterState != nullptr ? m.meterState->getScale() : KMeterState::Scale::k20); }, {}, "The highest level of the scale of the meter, in dB.")
                                 .signal ("onLevelChanged", &M::onLevelChanged, "Emitted with the peak and the average level, in dB, when either moved by half a dB.", { { Identifier ("peakDb"), ComponentProperty::Type::number }, { Identifier ("averageDb"), ComponentProperty::Type::number } })
                                 .signal ("onClippingChanged", &M::onClippingChanged, "Emitted when the signal starts or stops clipping.", { { Identifier ("clipping"), ComponentProperty::Type::boolean } })
                                 .build();

    return info;
}

//==============================================================================
KMeterComponent::KMeterComponent (StringRef componentID)
    : Component (componentID)
{
    setOpaque (true);
}

KMeterComponent::KMeterComponent (KMeterState& state, int channel, StringRef componentID)
    : KMeterComponent (componentID)
{
    channelIndex = channel;
    setState (&state);
}

KMeterComponent::~KMeterComponent()
{
}

//==============================================================================
void KMeterComponent::setState (KMeterState* newState)
{
    if (meterState == newState)
        return;

    meterState = newState;

    // The levels shown are the ones of the previous state
    updateLevels();
    repaint();
    notifyClippingChange();
}

void KMeterComponent::setChannel (int channel)
{
    if (channelIndex == channel)
        return;

    channelIndex = channel;

    updateLevels();
    repaint();
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
    if (meterState != nullptr)
        meterState->setOverCounterMode (mode);
}

void KMeterComponent::setRefreshRate (int hz)
{
    refreshRate = jlimit (10, 60, hz);
}

void KMeterComponent::setAverageIntegrationTime (double seconds)
{
    if (meterState != nullptr)
        meterState->setIntegrationTime (seconds);
}

void KMeterComponent::setAverageFallTime (double seconds)
{
    if (meterState != nullptr)
        meterState->setAverageFallTime (seconds);
}

void KMeterComponent::setPeakHoldTime (double seconds)
{
    if (meterState != nullptr)
        meterState->setPeakHoldTime (seconds);
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
    if (meterState == nullptr)
        return;

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
    constexpr float silenceDb = -100.0f;

    // Read levels from KMeterState (thread-safe via atomics), silence without one
    const float peakDb = meterState != nullptr ? meterState->getPeakLevel (channelIndex) : silenceDb;
    const float averageDb = meterState != nullptr ? meterState->getAverageLevel (channelIndex) : silenceDb;
    const float peakHoldDb = meterState != nullptr ? meterState->getPeakHoldLevel (channelIndex) : silenceDb;
    const bool clipping = meterState != nullptr && meterState->isClipping();

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
        const BailOutChecker bailOutChecker (this);

        lastCallbackClipping = clipping;
        onClippingChanged (clipping);

        if (bailOutChecker.shouldBailOut())
            return;
    }

    notifyClippingChange();
}

void KMeterComponent::notifyClippingChange()
{
    // What follows the state, as a rule of a stylesheet selecting it, is told when it changes
    const bool clipping = currentClipping.get();

    if (clipping != lastNotifiedClipping)
    {
        lastNotifiedClipping = clipping;
        notifyPropertyChanged (clippingPropertyId);
    }
}

} // namespace yup
