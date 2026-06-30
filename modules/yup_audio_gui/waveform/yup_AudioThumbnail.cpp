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
AudioThumbnail::AudioThumbnail (std::shared_ptr<AudioPeakProfileCache> cacheToUse)
    : cache (cacheToUse ? cacheToUse : std::make_shared<AudioPeakProfileCache>())
{
    cache->addListener (this);
}

AudioThumbnail::~AudioThumbnail()
{
    cache->removeListener (this);
    cache->cancelPendingRequests (currentCacheKey);
    masterReference.clear();
}

//==============================================================================
void AudioThumbnail::addListener (Listener* listener)
{
    listeners.add (listener);
}

void AudioThumbnail::removeListener (Listener* listener)
{
    listeners.remove (listener);
}

//==============================================================================
void AudioThumbnail::setSource (const AudioBuffer<float>* buffer, double newSampleRate)
{
    audioBufferPtr = buffer;
    ownedAudioBuffer.reset();
    ownedReader.reset();
    sampleRate = newSampleRate;
    totalSamples = buffer ? buffer->getNumSamples() : 0;
    numChannels = buffer ? buffer->getNumChannels() : 0;

    if (buffer && totalSamples > 0)
    {
        currentCacheKey = AudioPeakProfileCache::generateCacheKey (*buffer, sampleRate);
        requestPeakProfile();
    }
    else
    {
        currentProfile.reset();
        currentCacheKey.clear();
    }

    notifyThumbnailChanged();
}

void AudioThumbnail::setSource (const AudioBuffer<float>& buffer, double newSampleRate)
{
    audioBufferPtr = nullptr;
    ownedAudioBuffer = std::make_unique<AudioBuffer<float>> (buffer);
    ownedReader.reset();
    sampleRate = newSampleRate;
    totalSamples = ownedAudioBuffer->getNumSamples();
    numChannels = ownedAudioBuffer->getNumChannels();

    if (totalSamples > 0)
    {
        currentCacheKey = AudioPeakProfileCache::generateCacheKey (*ownedAudioBuffer, sampleRate);
        requestPeakProfile();
    }
    else
    {
        currentProfile.reset();
        currentCacheKey.clear();
    }

    notifyThumbnailChanged();
}

void AudioThumbnail::setSource (AudioBuffer<float>&& buffer, double newSampleRate)
{
    audioBufferPtr = nullptr;
    ownedAudioBuffer = std::make_unique<AudioBuffer<float>> (std::move (buffer));
    ownedReader.reset();
    sampleRate = newSampleRate;
    totalSamples = ownedAudioBuffer->getNumSamples();
    numChannels = ownedAudioBuffer->getNumChannels();

    if (totalSamples > 0)
    {
        currentCacheKey = AudioPeakProfileCache::generateCacheKey (*ownedAudioBuffer, sampleRate);
        requestPeakProfile();
    }
    else
    {
        currentProfile.reset();
        currentCacheKey.clear();
    }

    notifyThumbnailChanged();
}

void AudioThumbnail::setSource (std::unique_ptr<AudioFormatReader> reader, double newSampleRate)
{
    if (reader == nullptr)
    {
        clear();
        return;
    }

    audioBufferPtr = nullptr;
    ownedAudioBuffer.reset();
    ownedReader = std::move (reader);

    sampleRate = newSampleRate > 0.0 ? newSampleRate : ownedReader->sampleRate;
    totalSamples = static_cast<int> (ownedReader->lengthInSamples);
    numChannels = static_cast<int> (ownedReader->numChannels);

    // Generate cache key based on reader properties
    // For readers, we use a hash of length and channel count as identifier
    int64 hash = ownedReader->lengthInSamples;
    hash = hash * 31 + ownedReader->numChannels;
    hash = hash * 31 + static_cast<int64> (ownedReader->sampleRate);

    currentCacheKey = String::toHexString (hash);
    requestPeakProfile();
    notifyThumbnailChanged();
}

void AudioThumbnail::clear()
{
    cache->cancelPendingRequests (currentCacheKey);

    audioBufferPtr = nullptr;
    ownedAudioBuffer.reset();
    ownedReader.reset();
    currentProfile.reset();
    currentCacheKey.clear();

    sampleRate = 0.0;
    totalSamples = 0;
    numChannels = 0;

    setProgressValue (0.0);
    setProgressVisible (false);
    notifyThumbnailChanged();
}

//==============================================================================
std::shared_ptr<AudioPeakProfile> AudioThumbnail::getPeakProfile() const
{
    return currentProfile;
}

Range<double> AudioThumbnail::getClampedViewRange (Range<double> range) const
{
    return range.getIntersectionWith (Range<double> (0.0, static_cast<double> (totalSamples)));
}

//==============================================================================
void AudioThumbnail::paintChannel (Graphics& g,
                                   const Rectangle<float>& lane,
                                   int channelIndex,
                                   Range<double> sampleRange,
                                   float pixelWidth)
{
    if (! currentProfile || ! currentProfile->isValid())
        return;

    if (channelIndex < 0 || channelIndex >= numChannels)
        return;

    // Calculate how many pixels we'll actually draw
    const int pixelColumns = jmin (static_cast<int> (pixelWidth), static_cast<int> (lane.getWidth()));
    const int pixelRows = jmin (static_cast<int> (pixelWidth), static_cast<int> (lane.getHeight()));
    if (pixelColumns <= 0 || pixelRows <= 0)
        return;

    // Calculate zoom level and select best aggregation level
    const int numSamplesInRange = static_cast<int> (sampleRange.getLength());
    const float samplesPerPixel = static_cast<float> (numSamplesInRange) / static_cast<float> (pixelColumns);
    const int baseResolution = currentProfile->getBaseResolution();

    int bestLevel = 0;
    for (int i = 0; i < currentProfile->getNumAggregationLevels(); ++i)
    {
        const int factor = currentProfile->getAggregationFactor (i);
        const float samplesPerPeakAtLevel = static_cast<float> (baseResolution * factor);
        if (samplesPerPixel >= samplesPerPeakAtLevel * 2.0f)
            bestLevel = i;
        else
            break;
    }

    // Get peaks for selected level
    const auto& peaks = currentProfile->getChannelPeaks (channelIndex, bestLevel);
    const int peakBaseResolution = currentProfile->getBaseResolution() * currentProfile->getAggregationFactor (bestLevel);

    // Calculate peak range
    const int startPeak = static_cast<int> (sampleRange.getStart()) / peakBaseResolution;
    const int endPeak = jmin (static_cast<int> (peaks.minValues.size()),
                              (static_cast<int> (sampleRange.getEnd()) + peakBaseResolution - 1) / peakBaseResolution);

    if (endPeak <= startPeak)
        return;

    const int numPeaks = endPeak - startPeak;
    const float peaksPerPixelColumn = static_cast<float> (numPeaks) / static_cast<float> (pixelColumns);

    const float centerY = lane.getCenterY();
    const float amplitude = lane.getHeight() * 0.45f;
    const float minLineHeight = 1.0f;

    // Set fill color for drawing
    g.setFillColor (getChannelColor (channelIndex).withAlpha (0.9f));

    // Draw one vertical bar per pixel column
    const float x0 = lane.getX();
    for (int pixel = 0; pixel < pixelColumns; ++pixel)
    {
        const int peakStart = startPeak + static_cast<int> (pixel * peaksPerPixelColumn);
        int peakEnd = startPeak + static_cast<int> ((pixel + 1) * peaksPerPixelColumn);

        // Ensure we always render at least one peak per pixel (fixes spiky appearance when zoomed in)
        if (peakEnd <= peakStart)
            peakEnd = peakStart + 1;

        // Aggregate all peaks for this pixel column
        float minValue = 0.0f;
        float maxValue = 0.0f;

        for (int p = peakStart; p < peakEnd && p < endPeak; ++p)
        {
            const size_t idx = static_cast<size_t> (p);
            minValue = jmin (minValue, peaks.minValues[idx]);
            maxValue = jmax (maxValue, peaks.maxValues[idx]);
        }

        // Convert to screen coordinates
        float top = centerY - maxValue * amplitude;
        float bottom = centerY - minValue * amplitude;

        // Clamp to lane bounds
        top = jlimit (lane.getY(), lane.getBottom(), top);
        bottom = jlimit (lane.getY(), lane.getBottom(), bottom);

        // Ensure correct order
        if (bottom < top)
            std::swap (top, bottom);

        // Ensure minimum line height
        const float height = jmax (minLineHeight, bottom - top);

        // Use integer coordinates to avoid antialiasing gaps
        const int x = static_cast<int> (x0) + pixel;
        const int y = static_cast<int> (top);
        const int h = static_cast<int> (height) + 1;

        // Draw filled rectangle (1 pixel wide vertical bar)
        g.fillRect (Rectangle<float> (static_cast<float> (x), static_cast<float> (y), 1.0f, static_cast<float> (h)));
    }
}

Color AudioThumbnail::getChannelColor (int channelIndex) const
{
    constexpr uint32 colors[] = {
        0xFF4A9EFF, // Blue
        0xFFFF6B6B, // Red
        0xFF4ECDC4, // Teal
        0xFFFFA07A, // Orange
        0xFF9B59B6, // Purple
        0xFF2ECC71, // Green
    };

    return Color (colors[std::abs (channelIndex) % 6]);
}

//==============================================================================
void AudioThumbnail::profileReady (const String& cacheKey, std::shared_ptr<AudioPeakProfile> profile)
{
    if (cacheKey == currentCacheKey)
    {
        currentProfile = profile;
        setProgressValue (1.0);
        setProgressVisible (false);
        notifyThumbnailChanged();
    }
}

void AudioThumbnail::profileProgress (const String& cacheKey, double progress)
{
    if (cacheKey == currentCacheKey)
    {
        setProgressValue (progress);
        notifyThumbnailProgress();
    }
}

//==============================================================================
void AudioThumbnail::requestPeakProfile()
{
    auto buildFunction = [this]() -> std::shared_ptr<AudioPeakProfile>
    {
        auto profile = std::make_shared<AudioPeakProfile>();

        // Determine optimal base resolution
        int baseRes = AudioPeakProfile::calculateOptimalBaseResolution (totalSamples);
        auto factors = AudioPeakProfile::getDefaultAggregationFactors();

        // Progress callback
        auto progressCallback = [this] (double progress) -> bool
        {
            setProgressValue (progress);
            setProgressVisible (true);
            notifyThumbnailProgress();
            return true; // Continue
        };

        Result result = Result::ok();

        if (ownedReader)
        {
            // Build from reader
            result = profile->buildFromReader (*ownedReader, baseRes, factors, progressCallback);
        }
        else
        {
            // Build from buffer
            const auto* buffer = getActiveBuffer();
            if (buffer == nullptr)
                return nullptr;

            result = profile->buildFromBuffer (*buffer, baseRes, factors, progressCallback);
        }

        if (result.wasOk() && profile->isValid())
            return profile;

        return nullptr;
    };

    setProgressVisible (true);
    setProgressValue (0.0);

    // Always request background calculation - the cache decides based on thread pool availability
    cache->requestProfile (currentCacheKey, buildFunction, true);
}

String AudioThumbnail::getCurrentCacheKey() const
{
    return currentCacheKey;
}

const AudioBuffer<float>* AudioThumbnail::getActiveBuffer() const
{
    return ownedAudioBuffer ? ownedAudioBuffer.get() : audioBufferPtr;
}

void AudioThumbnail::setProgressVisible (bool shouldShow)
{
    if (progressVisible.load() != shouldShow)
    {
        progressVisible.store (shouldShow);
        notifyThumbnailProgress();
    }
}

void AudioThumbnail::setProgressValue (double newProgress)
{
    progressValue.store (jlimit (0.0, 1.0, newProgress));
}

void AudioThumbnail::notifyThumbnailChanged()
{
    listeners.call ([this] (Listener& l)
    {
        l.thumbnailChanged (*this);
    });
}

void AudioThumbnail::notifyThumbnailProgress()
{
    const double progress = progressValue.load();
    const bool visible = progressVisible.load();
    listeners.call ([this, progress, visible] (Listener& l)
    {
        l.thumbnailProgressChanged (*this, progress, visible);
    });
}

} // namespace yup
