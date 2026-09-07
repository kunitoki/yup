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
AudioViewComponent::AudioViewComponent (std::shared_ptr<AudioPeakProfileCache> cacheToUse)
{
    ownedThumbnail = std::make_unique<AudioThumbnail> (cacheToUse);
    attachThumbnail (*ownedThumbnail);
}

AudioViewComponent::AudioViewComponent (AudioThumbnail& thumbnailToUse)
{
    attachThumbnail (thumbnailToUse);
}

AudioViewComponent::~AudioViewComponent()
{
    if (thumbnail != nullptr)
        thumbnail->removeListener (this);
}

void AudioViewComponent::setSource (const AudioBuffer<float>* newBuffer, double newSampleRate)
{
    thumbnail->setSource (newBuffer, newSampleRate);
}

void AudioViewComponent::setSource (std::unique_ptr<AudioFormatReader> reader, double newSampleRate)
{
    thumbnail->setSource (std::move (reader), newSampleRate);
}

void AudioViewComponent::clear()
{
    thumbnail->clear();
    viewRangeSamples = {};
    zoomFactor = 1.0;
    updateScrollBar();
    repaint();
}

void AudioViewComponent::setZoomFactor (double newZoomFactor)
{
    const int totalSamples = getTotalSamples();
    if (totalSamples <= 0)
    {
        zoomFactor = 1.0;
        viewRangeSamples = {};
        updateScrollBar();
        repaint();
        return;
    }

    // Limit maximum zoom to prevent viewing less than 100 samples (or total if smaller)
    const double maxZoom = jmin (static_cast<double> (totalSamples), static_cast<double> (totalSamples) / 100.0);
    zoomFactor = jlimit (1.0, maxZoom, newZoomFactor);

    const double viewLength = jmax (100.0, static_cast<double> (totalSamples) / zoomFactor);
    auto newRange = Range<double>::withStartAndLength (viewRangeSamples.getStart(), viewLength);

    setViewRangeSamplesInternal (newRange, true);
}

void AudioViewComponent::setViewRangeSamples (Range<double> newRange)
{
    setViewRangeSamplesInternal (newRange, true);
}

void AudioViewComponent::scrollToSample (double newStartSample)
{
    setViewRangeSamplesInternal (Range<double>::withStartAndLength (newStartSample,
                                                                    viewRangeSamples.getLength()),
                                 true);
}

void AudioViewComponent::setLabelWidth (int newLabelWidth)
{
    labelWidth = jmax (0, newLabelWidth);
    resized();
    repaint();
}

void AudioViewComponent::setChannelLabelsVisible (bool shouldShow) noexcept
{
    if (showChannelLabels == shouldShow)
        return;

    showChannelLabels = shouldShow;
    resized();
    repaint();
}

void AudioViewComponent::setSelectable (bool shouldBeSelectable) noexcept
{
    if (selectable == shouldBeSelectable)
        return;

    selectable = shouldBeSelectable;
    setWantsKeyboardFocus (selectable);

    if (! selectable)
        leaveKeyboardFocus();
}

int AudioViewComponent::getTotalSamples() const noexcept
{
    return thumbnail->getTotalSamples();
}

int AudioViewComponent::getNumChannels() const noexcept
{
    return thumbnail->getNumChannels();
}

double AudioViewComponent::getSampleRate() const noexcept
{
    return thumbnail->getSampleRate();
}

double AudioViewComponent::timeToSample (double seconds) const noexcept
{
    const double sampleRate = getSampleRate();
    return sampleRate > 0.0 ? seconds * sampleRate : 0.0;
}

double AudioViewComponent::sampleToTime (double sample) const noexcept
{
    const double sampleRate = getSampleRate();
    return sampleRate > 0.0 ? sample / sampleRate : 0.0;
}

float AudioViewComponent::sampleToX (double sample, const Rectangle<float>& waveformBounds) const noexcept
{
    if (waveformBounds.getWidth() <= 0.0f)
        return waveformBounds.getX();

    const auto range = viewRangeSamples.isEmpty()
                         ? Range<double>::withStartAndLength (0.0, 1.0)
                         : viewRangeSamples;
    const double clamped = jlimit (range.getStart(), range.getEnd(), sample);
    const double proportion = (clamped - range.getStart()) / range.getLength();
    return waveformBounds.getX() + static_cast<float> (proportion * waveformBounds.getWidth());
}

double AudioViewComponent::xToSample (float x, const Rectangle<float>& waveformBounds) const noexcept
{
    if (waveformBounds.getWidth() <= 0.0f)
        return viewRangeSamples.getStart();

    const auto range = viewRangeSamples.isEmpty()
                         ? Range<double>::withStartAndLength (0.0, 1.0)
                         : viewRangeSamples;
    const float clampedX = jlimit (waveformBounds.getX(), waveformBounds.getRight(), x);
    const double proportion = (clampedX - waveformBounds.getX()) / waveformBounds.getWidth();
    return range.getStart() + proportion * range.getLength();
}

Rectangle<float> AudioViewComponent::getWaveformBounds() const
{
    auto bounds = getLocalBounds().reduced (8);
    if (horizontalScrollBar != nullptr && horizontalScrollBar->isVisible())
        bounds.removeFromBottom (static_cast<int> (horizontalScrollBar->getScrollBarWidth()));
    if (progressBar != nullptr && progressBar->isVisible())
        bounds.removeFromBottom (progressBarHeight);
    if (showChannelLabels && labelWidth > 0)
        bounds.removeFromLeft (labelWidth);
    return bounds;
}

void AudioViewComponent::paint (Graphics& g)
{
    paintBackground (g, getLocalBounds());

    auto bounds = getLocalBounds().reduced (8);
    if (horizontalScrollBar != nullptr && horizontalScrollBar->isVisible())
        bounds.removeFromBottom (static_cast<int> (horizontalScrollBar->getScrollBarWidth()));
    if (progressBar != nullptr && progressBar->isVisible())
        bounds.removeFromBottom (progressBarHeight);

    const bool shouldShowLabels = showChannelLabels && labelWidth > 0;
    auto labelArea = shouldShowLabels ? bounds.removeFromLeft (labelWidth) : Rectangle<float>();
    auto waveformArea = bounds;

    const int numChannels = thumbnail->getNumChannels();
    const int totalSamples = thumbnail->getTotalSamples();

    if (totalSamples <= 0 || numChannels <= 0)
    {
        paintPlaceholder (g, waveformArea);
        return;
    }

    auto profile = thumbnail->getPeakProfile();
    if (profile == nullptr || ! profile->isValid())
    {
        paintPlaceholder (g, waveformArea);
        return;
    }

    const float laneHeight = waveformArea.getHeight() / static_cast<float> (numChannels);
    auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f);

    // Calculate visible sample range
    const Range<double> visibleSamples = viewRangeSamples.isEmpty()
                                           ? Range<double> (0.0, static_cast<double> (totalSamples))
                                           : viewRangeSamples;

    // Paint each channel
    for (int channel = 0; channel < numChannels; ++channel)
    {
        Rectangle<float> lane (waveformArea.getX(),
                               waveformArea.getY() + laneHeight * channel,
                               waveformArea.getWidth(),
                               laneHeight);

        // Draw lane background
        g.setFillColor (Color (0xFF181818));
        g.fillRect (lane);

        g.setStrokeColor (Color (0xFF2A2A2A));
        g.setStrokeWidth (1.0f);
        g.strokeRect (lane);

        // Draw channel label
        if (shouldShowLabels)
        {
            auto labelBounds = labelArea.withY (lane.getY()).withHeight (lane.getHeight());
            g.setFillColor (Colors::white);
            g.fillFittedText (getChannelLabel (channel),
                              font,
                              labelBounds,
                              Justification::center);
        }

        // Paint waveform using new simplified API
        thumbnail->paintChannel (g, lane, channel, visibleSamples, waveformArea.getWidth());
    }

    paintOverlay (g, waveformArea);
}

void AudioViewComponent::resized()
{
    updateScrollBar();
    updateLayout();
    repaint();
}

void AudioViewComponent::mouseDown (const MouseEvent& event)
{
    if (selectable)
    {
        if (getWantsKeyboardFocus())
            takeKeyboardFocus();
    }

    Component::mouseDown (event);
}

void AudioViewComponent::mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData)
{
    if (! selectable)
    {
        Component::mouseWheel (event, wheelData);
        return;
    }

    if (getTotalSamples() <= 0)
        return;

    const float deltaX = wheelData.getDeltaX();
    const float deltaY = wheelData.getDeltaY();

    if (deltaY != 0.0f)
    {
        const float magnitude = deltaY >= 0.0f ? deltaY : -deltaY;
        const double zoomStep = 1.0 + static_cast<double> (magnitude) * 0.25;
        const double zoomMultiplier = deltaY > 0.0f ? zoomStep : 1.0 / zoomStep;
        const auto waveformBounds = getWaveformBounds();
        const double anchorSample = xToSample (event.getPosition().getX(), waveformBounds);
        zoomAroundSample (zoomMultiplier, anchorSample);
    }

    if (deltaX != 0.0f)
    {
        const double scrollAmount = viewRangeSamples.getLength() * static_cast<double> (deltaX) * 0.15;
        scrollBySamples (scrollAmount);
    }
}

void AudioViewComponent::keyDown (const KeyPress& keys, const Point<float>&)
{
    if (! selectable)
        return;

    if (getTotalSamples() <= 0)
        return;

    const int key = keys.getKey();

    if (key == KeyPress::leftKey || key == KeyPress::rightKey)
    {
        const double direction = key == KeyPress::leftKey ? -1.0 : 1.0;
        const double scrollAmount = viewRangeSamples.getLength() * 0.1 * direction;
        scrollBySamples (scrollAmount);
        return;
    }

    if (key == KeyPress::upKey || key == KeyPress::downKey)
    {
        const double direction = key == KeyPress::upKey ? 1.0 : -1.0;
        const double zoomMultiplier = direction > 0.0 ? 1.15 : (1.0 / 1.15);
        const double anchorSample = viewRangeSamples.getStart()
                                  + viewRangeSamples.getLength() * 0.5;
        zoomAroundSample (zoomMultiplier, anchorSample);
    }
}

void AudioViewComponent::paintBackground (Graphics& g, const Rectangle<float>&)
{
    g.setFillColor (Color (0xFF101010));
    g.fillAll();
}

void AudioViewComponent::paintPlaceholder (Graphics& g, const Rectangle<float>& bounds)
{
    const bool hasSource = (thumbnail->getTotalSamples() > 0 && thumbnail->getNumChannels() > 0);
    const auto placeholderText = hasSource
                                   ? String ("Analyzing waveform...")
                                   : String ("Load an audio file to view its waveform.");

    g.setFillColor (Colors::lightgray);
    auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (14.0f);
    g.fillFittedText (placeholderText, font, bounds, Justification::center);
}

void AudioViewComponent::paintOverlay (Graphics&, const Rectangle<float>&)
{
}

String AudioViewComponent::getChannelLabel (int channelIndex) const
{
    return "Ch " + String (channelIndex + 1);
}

void AudioViewComponent::attachThumbnail (AudioThumbnail& thumbnailToUse)
{
    thumbnail = &thumbnailToUse;
    thumbnail->addListener (this);

    horizontalScrollBar = std::make_unique<ScrollBar> (ScrollBar::Orientation::horizontal);
    horizontalScrollBar->setVisibilityMode (ScrollBar::VisibilityMode::autoHide);
    horizontalScrollBar->onScrollPositionChanged = [this] (double)
    {
        handleScrollBarMoved();
    };
    addAndMakeVisible (*horizontalScrollBar);

    progressBar = std::make_unique<ProgressBar> ("AudioThumbnailProgress");
    progressBar->setVisible (false);
    addAndMakeVisible (*progressBar);

    updateProgressBar (thumbnail->getProgress(), thumbnail->isProgressVisible());
    ensureViewRangeIsValid();
    updateScrollBar();
}

void AudioViewComponent::updateLayout()
{
    auto bounds = getLocalBounds().reduced (8);
    if (horizontalScrollBar != nullptr && horizontalScrollBar->isVisible())
    {
        auto height = static_cast<int> (horizontalScrollBar->getScrollBarWidth());
        horizontalScrollBar->setBounds (bounds.removeFromBottom (height));
    }
    else if (horizontalScrollBar != nullptr)
    {
        horizontalScrollBar->setBounds (bounds.removeFromBottom (0));
    }

    if (progressBar != nullptr && progressBar->isVisible())
        progressBar->setBounds (bounds.removeFromBottom (progressBarHeight));
    else if (progressBar != nullptr)
        progressBar->setBounds (bounds.removeFromBottom (0));
}

void AudioViewComponent::updateScrollBar()
{
    if (horizontalScrollBar == nullptr)
        return;

    const double totalSamples = static_cast<double> (getTotalSamples());
    const double viewStart = viewRangeSamples.isEmpty() ? 0.0 : viewRangeSamples.getStart();
    const double viewEnd = viewRangeSamples.isEmpty()
                             ? jmax (1.0, totalSamples)
                             : viewRangeSamples.getEnd();

    ignoreScrollBarCallback = true;
    horizontalScrollBar->setRangeLimits (0.0, jmax (1.0, totalSamples));
    horizontalScrollBar->setCurrentRange (viewStart, viewEnd);
    ignoreScrollBarCallback = false;
}

void AudioViewComponent::handleScrollBarMoved()
{
    if (ignoreScrollBarCallback || horizontalScrollBar == nullptr)
        return;

    const double newStart = horizontalScrollBar->getCurrentRangeStart();
    setViewRangeSamplesInternal (Range<double>::withStartAndLength (newStart,
                                                                    viewRangeSamples.getLength()),
                                 false);
}

void AudioViewComponent::setViewRangeSamplesInternal (Range<double> range, bool notifyScrollBar)
{
    const double totalSamples = static_cast<double> (getTotalSamples());
    if (totalSamples <= 0.0)
    {
        viewRangeSamples = {};
        zoomFactor = 1.0;
        if (notifyScrollBar)
            updateScrollBar();
        repaint();
        return;
    }

    // Clamp view range to valid sample bounds
    double clampedStart = jlimit (0.0, totalSamples - 1.0, range.getStart());
    double clampedLength = range.getLength();

    // Ensure the view doesn't extend past the end
    if (clampedStart + clampedLength > totalSamples)
        clampedLength = totalSamples - clampedStart;

    // Ensure minimum length (at least 10 samples or total samples if smaller)
    const double minViewLength = jmin (10.0, totalSamples);
    clampedLength = jmax (minViewLength, clampedLength);

    // Final check: if start + length would exceed total, adjust start
    if (clampedStart + clampedLength > totalSamples)
        clampedStart = jmax (0.0, totalSamples - clampedLength);

    viewRangeSamples = Range<double>::withStartAndLength (clampedStart, clampedLength);

    zoomFactor = (totalSamples > 0.0 && viewRangeSamples.getLength() > 0.0)
                   ? totalSamples / viewRangeSamples.getLength()
                   : 1.0;

    if (notifyScrollBar)
        updateScrollBar();

    repaint();
}

void AudioViewComponent::scrollBySamples (double deltaSamples)
{
    const int totalSamples = getTotalSamples();
    if (totalSamples <= 0)
        return;

    const double viewLength = viewRangeSamples.isEmpty()
                                ? static_cast<double> (totalSamples)
                                : viewRangeSamples.getLength();
    const double viewStart = viewRangeSamples.isEmpty() ? 0.0 : viewRangeSamples.getStart();
    const double newStart = viewStart + deltaSamples;

    // Prevent scrolling beyond boundaries
    if (deltaSamples < 0.0 && newStart <= 0.0)
    {
        // At the beginning, don't scroll left
        setViewRangeSamplesInternal (Range<double>::withStartAndLength (0.0, viewLength), true);
        return;
    }

    if (deltaSamples > 0.0 && newStart + viewLength >= static_cast<double> (totalSamples))
    {
        // At the end, don't scroll right
        setViewRangeSamplesInternal (Range<double>::withStartAndLength (
                                         static_cast<double> (totalSamples) - viewLength, viewLength),
                                     true);
        return;
    }

    setViewRangeSamplesInternal (Range<double>::withStartAndLength (newStart, viewLength), true);
}

void AudioViewComponent::zoomAroundSample (double zoomMultiplier, double anchorSample)
{
    const double totalSamples = static_cast<double> (getTotalSamples());
    if (totalSamples <= 0.0)
        return;

    const double currentZoom = zoomFactor;
    // Limit maximum zoom to prevent viewing less than 100 samples (or total if smaller)
    const double maxZoom = jmin (totalSamples, totalSamples / 100.0);
    const double newZoom = jlimit (1.0, maxZoom, currentZoom * zoomMultiplier);
    const double newViewLength = jmax (100.0, totalSamples / newZoom);

    const double oldViewLength = viewRangeSamples.isEmpty()
                                   ? totalSamples
                                   : viewRangeSamples.getLength();
    const double oldViewStart = viewRangeSamples.isEmpty() ? 0.0 : viewRangeSamples.getStart();
    const double clampedAnchor = jlimit (0.0, totalSamples - 1.0, anchorSample);
    const double anchorRatio = oldViewLength > 0.0
                                 ? jlimit (0.0, 1.0, (clampedAnchor - oldViewStart) / oldViewLength)
                                 : 0.5;
    const double newStart = clampedAnchor - newViewLength * anchorRatio;

    setViewRangeSamplesInternal (Range<double>::withStartAndLength (newStart, newViewLength), true);
}

void AudioViewComponent::ensureViewRangeIsValid()
{
    const int totalSamples = getTotalSamples();
    if (totalSamples <= 0)
    {
        viewRangeSamples = {};
        zoomFactor = 1.0;
        updateScrollBar();
        return;
    }

    const auto defaultRange = Range<double>::withStartAndLength (0.0, static_cast<double> (totalSamples));
    const auto rangeToUse = viewRangeSamples.isEmpty() ? defaultRange : viewRangeSamples;
    setViewRangeSamplesInternal (rangeToUse, true);
}

void AudioViewComponent::updateProgressBar (double progress, bool isVisible)
{
    if (progressBar == nullptr)
        return;

    if (progressBar->isVisible() != isVisible)
    {
        progressBar->setVisible (isVisible);
        updateLayout();
    }

    progressBar->setProgress (progress, sendNotificationSync);
}

void AudioViewComponent::thumbnailChanged (AudioThumbnail&)
{
    ensureViewRangeIsValid();
    repaint();
}

void AudioViewComponent::thumbnailProgressChanged (AudioThumbnail&, double progress, bool isVisible)
{
    updateProgressBar (progress, isVisible);
}

} // namespace yup
