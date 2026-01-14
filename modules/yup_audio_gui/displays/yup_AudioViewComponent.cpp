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
AudioViewComponent::AudioViewComponent()
{
    ownedThumbnail = std::make_unique<AudioThumbnail>();
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

AudioThumbnail& AudioViewComponent::getThumbnail() noexcept
{
    return *thumbnail;
}

const AudioThumbnail& AudioViewComponent::getThumbnail() const noexcept
{
    return *thumbnail;
}

void AudioViewComponent::setAudioBuffer (const AudioBuffer<float>* newBuffer, double newSampleRate)
{
    thumbnail->setAudioBuffer (newBuffer, newSampleRate);
}

void AudioViewComponent::setAudioFile (const File& file, AudioFormatManager* managerToUse)
{
    thumbnail->setAudioFile (file, managerToUse);
}

void AudioViewComponent::clear()
{
    thumbnail->clear();
}

const AudioBuffer<float>* AudioViewComponent::getAudioBuffer() const noexcept
{
    return thumbnail->getAudioBuffer();
}

const File& AudioViewComponent::getAudioFile() const noexcept
{
    return thumbnail->getAudioFile();
}

bool AudioViewComponent::isUsingAudioFile() const noexcept
{
    return thumbnail->isUsingAudioFile();
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

    const double maxZoom = static_cast<double> (totalSamples);
    zoomFactor = jlimit (1.0, maxZoom, newZoomFactor);

    const double viewLength = jmax (1.0, static_cast<double> (totalSamples) / zoomFactor);
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
    return thumbnail->timeToSample (seconds);
}

double AudioViewComponent::sampleToTime (double sample) const noexcept
{
    return thumbnail->sampleToTime (sample);
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

    if (thumbnail->getTotalSamples() <= 0 || thumbnail->getNumChannels() <= 0)
    {
        paintPlaceholder (g, waveformArea);
        return;
    }

    auto profile = thumbnail->getActiveProfile();
    if (profile == nullptr || profile->channelPeaks.empty())
    {
        paintPlaceholder (g, waveformArea);
        return;
    }

    const int numChannels = profile->numChannels;
    if (numChannels <= 0)
    {
        paintPlaceholder (g, waveformArea);
        return;
    }

    const float laneHeight = waveformArea.getHeight() / static_cast<float> (numChannels);
    auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f);

    const int numPeaks = static_cast<int> (profile->channelPeaks[0].minValues.size());
    if (numPeaks <= 0)
    {
        paintPlaceholder (g, waveformArea);
        return;
    }

    const double viewStart = viewRangeSamples.getStart();
    const double viewEnd = viewRangeSamples.getEnd();
    const int samplesPerPeak = profile->samplesPerPeak;
    const int startIndex = jlimit (0, numPeaks - 1, static_cast<int> (viewStart / samplesPerPeak));
    const int endIndex = jlimit (startIndex + 1,
                                 numPeaks,
                                 static_cast<int> (std::ceil (viewEnd / samplesPerPeak)));

    const int numVisiblePeaks = jmax (1, endIndex - startIndex);
    const float stepX = waveformArea.getWidth() / static_cast<float> (numVisiblePeaks);
    const float startX = waveformArea.getX();

    for (int channel = 0; channel < numChannels; ++channel)
    {
        Rectangle<float> lane (waveformArea.getX(),
                               waveformArea.getY() + laneHeight * channel,
                               waveformArea.getWidth(),
                               laneHeight);

        g.setFillColor (Color (0xFF181818));
        g.fillRect (lane);

        g.setStrokeColor (Color (0xFF2A2A2A));
        g.setStrokeWidth (1.0f);
        g.strokeRect (lane);

        if (shouldShowLabels)
        {
            auto labelBounds = labelArea.withY (lane.getY()).withHeight (lane.getHeight());
            g.setFillColor (Colors::white);
            g.fillFittedText (getChannelLabel (channel),
                              font,
                              labelBounds,
                              Justification::center);
        }

        const auto& peaks = profile->channelPeaks[static_cast<size_t> (channel)];
        thumbnail->paintChannel (g,
                                 lane,
                                 channel,
                                 peaks.minValues,
                                 peaks.maxValues,
                                 startIndex,
                                 endIndex,
                                 startX,
                                 stepX);
    }

    paintOverlay (g, waveformArea);
}

void AudioViewComponent::resized()
{
    updateScrollBar();
    updateLayout();

    const int waveformWidth = static_cast<int> (getWaveformBounds().getWidth());
    if (waveformWidth != lastWaveformWidth)
    {
        lastWaveformWidth = waveformWidth;
        rebuildPeakProfileIfNeeded();
    }
}

void AudioViewComponent::timerCallback()
{
    if (! pendingRebuild)
    {
        stopTimer();
        return;
    }

    const auto elapsedMs = (Time::getCurrentTime() - lastResizeTime).inMilliseconds();
    if (elapsedMs < rebuildDebounceMs)
        return;

    pendingRebuild = false;
    stopTimer();
    if (pendingSamplesPerPeak > 0)
    {
        thumbnail->requestProfile (pendingSamplesPerPeak);
        lastRequestedSamplesPerPeak = pendingSamplesPerPeak;
        pendingSamplesPerPeak = 0;
    }
}

void AudioViewComponent::mouseDown (const MouseEvent& event)
{
    if (selectable)
        takeKeyboardFocus();

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
    viewRangeSamples = thumbnail->getClampedViewRange (range);

    const double totalSamples = static_cast<double> (getTotalSamples());
    zoomFactor = (totalSamples > 0.0 && viewRangeSamples.getLength() > 0.0)
                   ? totalSamples / viewRangeSamples.getLength()
                   : 1.0;

    if (notifyScrollBar)
        updateScrollBar();

    rebuildPeakProfileIfNeeded();
    repaint();
}

void AudioViewComponent::rebuildPeakProfileIfNeeded()
{
    if (getTotalSamples() <= 0 || getNumChannels() <= 0)
        return;

    const auto waveformBounds = getWaveformBounds();
    if (waveformBounds.getWidth() <= 0.0f)
        return;

    const double viewLength = viewRangeSamples.isEmpty()
                                ? static_cast<double> (getTotalSamples())
                                : viewRangeSamples.getLength();
    const int samplesPerPeak = thumbnail->getSamplesPerPeakForView (viewLength, waveformBounds.getWidth());
    if (samplesPerPeak <= 0)
        return;

    auto profile = thumbnail->getActiveProfile();
    if (profile == nullptr)
    {
        thumbnail->requestProfile (samplesPerPeak);
        lastRequestedSamplesPerPeak = samplesPerPeak;
        return;
    }

    schedulePeakProfileUpdate (samplesPerPeak);
}

void AudioViewComponent::schedulePeakProfileUpdate (int samplesPerPeak)
{
    if (samplesPerPeak == lastRequestedSamplesPerPeak)
        return;

    pendingSamplesPerPeak = samplesPerPeak;
    pendingRebuild = true;
    lastResizeTime = Time::getCurrentTime();
    startTimer (rebuildDebounceMs);
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
    setViewRangeSamplesInternal (Range<double>::withStartAndLength (viewStart + deltaSamples, viewLength),
                                 true);
}

void AudioViewComponent::zoomAroundSample (double zoomMultiplier, double anchorSample)
{
    const double totalSamples = static_cast<double> (getTotalSamples());
    if (totalSamples <= 0.0)
        return;

    const double currentZoom = zoomFactor;
    const double newZoom = jlimit (1.0, totalSamples, currentZoom * zoomMultiplier);
    const double newViewLength = jmax (1.0, totalSamples / newZoom);

    const double oldViewLength = viewRangeSamples.isEmpty()
                                   ? totalSamples
                                   : viewRangeSamples.getLength();
    const double oldViewStart = viewRangeSamples.isEmpty() ? 0.0 : viewRangeSamples.getStart();
    const double clampedAnchor = jlimit (0.0, totalSamples, anchorSample);
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
        return;
    }

    const auto defaultRange = Range<double>::withStartAndLength (0.0, static_cast<double> (totalSamples));
    const auto clampedRange = viewRangeSamples.isEmpty()
                                ? defaultRange
                                : thumbnail->getClampedViewRange (viewRangeSamples);
    setViewRangeSamplesInternal (clampedRange, true);
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
    lastRequestedSamplesPerPeak = 0;
    pendingSamplesPerPeak = 0;
    ensureViewRangeIsValid();
    repaint();
}

void AudioViewComponent::thumbnailProgressChanged (AudioThumbnail&, double progress, bool isVisible)
{
    updateProgressBar (progress, isVisible);
}

} // namespace yup
