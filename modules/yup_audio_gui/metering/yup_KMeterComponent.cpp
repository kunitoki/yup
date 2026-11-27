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

namespace
{

// How the K-System divides a scale into bars, from its top: bars of half a dB down to topBarsDb, then of 1 dB and
// twice as tall down to greenBarsDb, then one of 6 dB down to kMeterLinearAreaDb when that is lower, then bars of
// 10 dB, the last of which is lowestBarUnits tall
struct KMeterBarLimits
{
    float topBarsDb;
    float greenBarsDb;
    float lowestBarUnits;
};

constexpr KMeterBarLimits kMeterBarLimits[] = {
    { 18.0f, -24.0f, 4.0f }, // K-20
    { 12.0f, -30.0f, 4.0f }, // K-14
    { 10.0f, -30.0f, 5.0f }  // K-12
};

constexpr float kMeterLinearAreaDb = -30.0f;

// Levels that kept still this long are looked at by a timer, which leaves the window of the meter to idle
constexpr double kMeterRestSeconds = 1.0;
constexpr int kMeterRestPollMilliseconds = 100;

struct KMeterComponentIds
{
    static inline const Identifier peakDb { "peakDb" };
    static inline const Identifier averageDb { "averageDb" };
    static inline const Identifier peakHoldDb { "peakHoldDb" };
    static inline const Identifier clipping { "clipping" };
    static inline const Identifier rangeMinDb { "rangeMinDb" };
    static inline const Identifier rangeMaxDb { "rangeMaxDb" };
};

float findLevelPosition (const std::vector<KMeterComponent::ScaleSegment>& segments, float db, float emptyPosition)
{
    if (segments.empty())
        return emptyPosition;

    const float clampedDb = jlimit (segments.back().bottomDb, segments.front().topDb, db);

    for (const auto& segment : segments)
    {
        if (clampedDb <= segment.topDb && clampedDb >= segment.bottomDb)
        {
            const float range = segment.topDb - segment.bottomDb;
            const float normalized = range > 0.0f ? (clampedDb - segment.bottomDb) / range : 0.0f;
            return segment.bottomY - (segment.bottomY - segment.topY) * normalized;
        }
    }

    return segments.back().bottomY;
}

} // namespace

//==============================================================================
const Identifier KMeterComponent::Style::backgroundColorId { "kMeterBackground" };
const Identifier KMeterComponent::Style::greenZoneColorId { "kMeterGreenZone" };
const Identifier KMeterComponent::Style::amberZoneColorId { "kMeterAmberZone" };
const Identifier KMeterComponent::Style::redZoneColorId { "kMeterRedZone" };
const Identifier KMeterComponent::Style::averageLevelColorId { "kMeterAverageLevel" };
const Identifier KMeterComponent::Style::peakLevelColorId { "kMeterPeakLevel" };
const Identifier KMeterComponent::Style::peakLevelClipColorId { "kMeterPeakLevelClip" };
const Identifier KMeterComponent::Style::peakHoldColorId { "kMeterPeakHold" };

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
                                 .readOnly (KMeterComponentIds::peakDb, [] (const M& m) { return m.getPeakLevel(); }, -100.0, "The peak level shown, in dB.")
                                 .readOnly (KMeterComponentIds::averageDb, [] (const M& m) { return m.getAverageLevel(); }, -100.0, "The average level shown, in dB.")
                                 .readOnly (KMeterComponentIds::peakHoldDb, [] (const M& m) { return m.getPeakHoldLevel(); }, -100.0, "The peak level held, in dB.")
                                 .state (KMeterComponentIds::clipping, [] (const M& m) { return m.isClipping(); }, false, "Whether the signal is clipping.")
                                 .readOnly (KMeterComponentIds::rangeMinDb, [] (const M& m) { return KMeterState::rangeMinForScale (m.getScale()); }, {}, "The lowest level of the scale of the meter, in dB.")
                                 .readOnly (KMeterComponentIds::rangeMaxDb, [] (const M& m) { return KMeterState::rangeMaxForScale (m.getScale()); }, {}, "The highest level of the scale of the meter, in dB.")
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
    followLevels();
    notifyLevelChanges();
}

void KMeterComponent::setChannel (int channel)
{
    if (channelIndex == channel)
        return;

    channelIndex = channel;

    updateLevels();
    repaint();
    notifyLevelChanges();
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
Span<const KMeterComponent::ScaleSegment> KMeterComponent::getScaleSegments() const
{
    return getScaleLayout().segments;
}

Span<const KMeterComponent::ScaleTick> KMeterComponent::getScaleTicks() const
{
    return getScaleLayout().ticks;
}

float KMeterComponent::getPositionForLevel (float db) const
{
    return findLevelPosition (getScaleLayout().segments, db, getLocalBounds().getBottom());
}

const KMeterComponent::ScaleLayout& KMeterComponent::getScaleLayout() const
{
    const auto scale = getScale();
    const auto bounds = getLocalBounds();

    // The layout a meter starts with is the one of a meter with no area, which has no segments and no ticks
    if (scaleLayout.scale == scale && scaleLayout.mapping == scaleMapping && scaleLayout.bounds == bounds)
        return scaleLayout;

    auto& segments = scaleLayout.segments;
    auto& ticks = scaleLayout.ticks;

    scaleLayout.scale = scale;
    scaleLayout.mapping = scaleMapping;
    scaleLayout.bounds = bounds;
    segments.clear();
    ticks.clear();

    if (bounds.getWidth() <= 0.0f || bounds.getHeight() <= 0.0f)
        return scaleLayout;

    const float rangeMin = KMeterState::rangeMinForScale (scale);
    const float rangeMax = KMeterState::rangeMaxForScale (scale);

    if (scaleMapping == ScaleMapping::segmented)
    {
        const auto& limits = kMeterBarLimits[jlimit (0, numElementsInArray (kMeterBarLimits) - 1, static_cast<int> (scale))];

        // The level a bar ends at and how tall it is, in units the bars share the height of the meter by
        const auto barBelow = [&] (float topDb) -> std::pair<float, float>
        {
            if (topDb > limits.topBarsDb)
                return { jmax (rangeMin, topDb - 0.5f), 1.0f };

            if (topDb > limits.greenBarsDb)
                return { jmax (rangeMin, topDb - 1.0f), 2.0f };

            if (topDb > kMeterLinearAreaDb)
                return { jmax (rangeMin, topDb - 6.0f), 3.0f };

            const float bottomDb = jmax (rangeMin, topDb - 10.0f);
            return { bottomDb, bottomDb <= rangeMin ? limits.lowestBarUnits : 3.0f };
        };

        float totalUnits = 0.0f;

        for (float topDb = rangeMax; topDb > rangeMin;)
        {
            const auto [bottomDb, units] = barBelow (topDb);
            totalUnits += units;
            topDb = bottomDb;
        }

        const float unitHeight = totalUnits > 0.0f ? bounds.getHeight() / totalUnits : bounds.getHeight();
        float topY = bounds.getY();

        for (float topDb = rangeMax; topDb > rangeMin;)
        {
            const auto [bottomDb, units] = barBelow (topDb);

            // The last bar takes what is left, whatever the others added up to
            const float bottomY = topY + (bottomDb > rangeMin ? units * unitHeight : bounds.getBottom() - topY);

            segments.push_back ({ topDb, bottomDb, topY, bottomY });
            topDb = bottomDb;
            topY = bottomY;
        }
    }
    else
    {
        segments.push_back ({ rangeMax, rangeMin, bounds.getY(), bounds.getBottom() });
    }

    // Fewer ticks on a small meter
    const int tickInterval = bounds.getHeight() < 200.0f ? 10 : 5;
    const int startDb = static_cast<int> (std::ceil (rangeMin / tickInterval)) * tickInterval;
    const int endDb = static_cast<int> (std::floor (rangeMax / tickInterval)) * tickInterval;
    int lowestLabel = -1;

    for (int db = endDb; db >= startDb; db -= tickInterval)
    {
        const float y = findLevelPosition (segments, static_cast<float> (db), bounds.getBottom());

        if (y < bounds.getY() || y > bounds.getBottom())
            continue;

        ScaleTick tick { db, y };

        // A label every 10 dB, the reference of 0 dB among them, when all of it is inside the meter
        const Rectangle<float> labelBounds (bounds.getX() + 5.0f, y - 6.0f, 22.0f, 12.0f);

        if (db % 10 == 0 && labelBounds.getY() >= bounds.getY() && labelBounds.getBottom() <= bounds.getBottom())
        {
            tick.labelBounds = labelBounds;
            lowestLabel = static_cast<int> (ticks.size());
        }

        ticks.push_back (std::move (tick));
    }

    // The lowest label is always kept, the others only when they are off the ones kept above them and off it
    float previousLabelBottom = bounds.getY();

    for (int index = 0; index <= lowestLabel; ++index)
    {
        auto& tick = ticks[static_cast<size_t> (index)];

        if (tick.labelBounds.isEmpty())
            continue;

        if (tick.labelBounds.getY() < previousLabelBottom
            || (index != lowestLabel && tick.labelBounds.getBottom() > ticks[static_cast<size_t> (lowestLabel)].labelBounds.getY()))
        {
            tick.labelBounds = {};
            continue;
        }

        tick.label = (tick.db > 0 ? "+" : "") + String (tick.db);
        previousLabelBottom = tick.labelBounds.getBottom();
    }

    return scaleLayout;
}

//==============================================================================
void KMeterComponent::paint (Graphics& g)
{
    // A meter that is painted shows, whatever it was told: it follows its levels if it does neither that nor rest
    if (meterState != nullptr && ! frameAttachment.isActive() && ! restPoll.isTimerRunning())
        followLevels();

    if (auto style = ApplicationTheme::findComponentStyle (*this))
    {
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
    }
}

void KMeterComponent::visibilityChanged()
{
    showingChanged();
}

void KMeterComponent::parentHierarchyChanged()
{
    showingChanged();
}

void KMeterComponent::refreshDisplay (double lastFrameTimeSeconds)
{
    if (meterState == nullptr)
        return;

    timeSinceLastUpdate += lastFrameTimeSeconds;
    timeAtRest += lastFrameTimeSeconds;

    // Update at configured refresh rate
    const double updateInterval = 1.0 / refreshRate;
    if (timeSinceLastUpdate >= updateInterval)
    {
        // What is over the interval counts toward the next update, so the rate holds whatever the frames last
        timeSinceLastUpdate = std::fmod (timeSinceLastUpdate, updateInterval);

        if (updateLevels())
        {
            timeAtRest = 0.0;
        }
        else if (frameAttachment.isActive() && timeAtRest >= kMeterRestSeconds)
        {
            // The audio thread cannot ask for frames again, so the timer looks at the levels meanwhile
            frameAttachment.setActive (false);
            restPoll.startTimer (kMeterRestPollMilliseconds);
        }

        checkForCallbacks();
    }
}

//==============================================================================
bool KMeterComponent::updateLevels()
{
    constexpr float silenceDb = -100.0f;

    // Read levels from KMeterState (thread-safe via atomics), silence without one
    const Levels newLevels {
        meterState != nullptr ? meterState->getPeakLevel (channelIndex) : silenceDb,
        meterState != nullptr ? meterState->getAverageLevel (channelIndex) : silenceDb,
        meterState != nullptr ? meterState->getPeakHoldLevel (channelIndex) : silenceDb,
        meterState != nullptr && meterState->isClipping(),
        getScale()
    };

    if (newLevels == levels)
        return false;

    if (showsDifferently (std::exchange (levels, newLevels)))
        repaint();

    return true;
}

bool KMeterComponent::showsDifferently (const Levels& previousLevels) const
{
    // A style of its own puts the levels where it likes, another scale moves them all, and clipping colors the peak
    if (getStyle() != nullptr || levels.scale != previousLevels.scale || levels.clipping != previousLevels.clipping)
        return true;

    const float rangeMin = KMeterState::rangeMinForScale (levels.scale);
    const float pixelsPerUnit = jmax (1.0f, getScaleDpi());

    // The theme puts a level where the meter lays it out, so it shows when it is at another pixel, or came onto
    // or went off the scale. What is on screen is at the pixel of the levels read last, so two reads are enough
    const auto hasMoved = [&] (float previousDb, float newDb)
    {
        return (previousDb > rangeMin) != (newDb > rangeMin)
            || roundToInt (getPositionForLevel (previousDb) * pixelsPerUnit) != roundToInt (getPositionForLevel (newDb) * pixelsPerUnit);
    };

    return hasMoved (previousLevels.averageDb, levels.averageDb)
        || (showPeakHold && hasMoved (previousLevels.peakHoldDb, levels.peakHoldDb))
        || (showPeak && hasMoved (previousLevels.peakDb, levels.peakDb));
}

void KMeterComponent::followLevels()
{
    // Frames are for a meter that has levels to show and shows
    restPoll.stopTimer();
    timeAtRest = 0.0;
    frameAttachment.setActive (meterState != nullptr && isShowing());
}

void KMeterComponent::pollLevels()
{
    // Levels that moved are followed again, and so are the ones of a meter that left its window, which gets no
    // frames until it is in one again
    if (updateLevels() || getNativeComponent() == nullptr)
        followLevels();

    checkForCallbacks();
}

void KMeterComponent::showingChanged()
{
    // A meter that shows again shows the levels of now, not the ones it was hidden with
    if (isShowing())
        updateLevels();

    followLevels();
}

void KMeterComponent::checkForCallbacks()
{
    const float peakDb = levels.peakDb;
    const float avgDb = levels.averageDb;
    const bool clipping = levels.clipping;

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

    notifyLevelChanges();
}

void KMeterComponent::notifyLevelChanges()
{
    // What follows the levels, as a binding reading one or a rule of a stylesheet selecting the clipping, is told
    // when they change
    if (lastNotifiedLevels == levels)
        return;

    const auto told = std::exchange (lastNotifiedLevels, levels);

    const std::pair<const Identifier*, bool> properties[] = {
        { &KMeterComponentIds::peakDb, levels.peakDb != told.peakDb },
        { &KMeterComponentIds::averageDb, levels.averageDb != told.averageDb },
        { &KMeterComponentIds::peakHoldDb, levels.peakHoldDb != told.peakHoldDb },
        { &KMeterComponentIds::clipping, levels.clipping != told.clipping },
        { &KMeterComponentIds::rangeMinDb, levels.scale != told.scale },
        { &KMeterComponentIds::rangeMaxDb, levels.scale != told.scale }
    };

    const BailOutChecker bailOutChecker (this);

    for (const auto& [property, hasChanged] : properties)
    {
        if (hasChanged)
            notifyPropertyChanged (*property);

        if (bailOutChecker.shouldBailOut())
            return;
    }
}

} // namespace yup
