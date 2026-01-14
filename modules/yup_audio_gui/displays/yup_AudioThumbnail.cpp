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
constexpr int kCacheMagic = 0x59555054; // "YUPT"
constexpr int kCacheVersion = 1;
} // namespace

//==============================================================================
struct AudioThumbnail::PeakJob final : public ThreadPoolJob
{
    PeakJob (AudioThumbnail& ownerToUse,
             int jobIdToUse,
             int samplesPerPeakToUse,
             bool shouldUseDiskCache)
        : ThreadPoolJob ("AudioThumbnail PeakJob")
        , owner (ownerToUse)
        , jobId (jobIdToUse)
        , samplesPerPeak (samplesPerPeakToUse)
        , shouldCacheToDisk (shouldUseDiskCache)
    {
    }

    JobStatus runJob() override
    {
        if (shouldExit())
            return jobHasFinished;

        auto profile = owner.buildPeakProfile (samplesPerPeak, this);

        if (profile == nullptr || shouldExit())
        {
            owner.setProgressValue (0.0);
            owner.setProgressVisible (false);
            return jobHasFinished;
        }

        if (shouldCacheToDisk)
            owner.saveProfileToCache (*profile);

        owner.applyPeakProfile (std::move (profile), jobId);
        return jobHasFinished;
    }

    AudioThumbnail& owner;
    int jobId = 0;
    int samplesPerPeak = 0;
    bool shouldCacheToDisk = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PeakJob)
};

//==============================================================================
AudioThumbnail::AudioThumbnail()
{
}

AudioThumbnail::~AudioThumbnail()
{
    ++jobCounter;
    activeJobId.store (jobCounter.load());
    masterReference.clear();
}

void AudioThumbnail::addListener (Listener* listener)
{
    listeners.add (listener);
}

void AudioThumbnail::removeListener (Listener* listener)
{
    listeners.remove (listener);
}

//==============================================================================
void AudioThumbnail::setAudioBuffer (const AudioBuffer<float>* newBuffer, double newSampleRate)
{
    audioBuffer = newBuffer;
    sampleRate = newSampleRate;
    audioFile = File {};
    usingAudioFile = false;
    ownedFormatManager.reset();
    audioFormatManager = nullptr;
    totalSamples = audioBuffer != nullptr ? audioBuffer->getNumSamples() : 0;
    numChannels = audioBuffer != nullptr ? audioBuffer->getNumChannels() : 0;

    {
        const ScopedLock lock (peakLock);
        peakCache.clear();
        activeProfile.reset();
    }

    pendingSamplesPerPeak.store (0);
    activeJobId.store (++jobCounter);

    if (audioBuffer == nullptr || totalSamples <= 0)
    {
        setProgressValue (0.0);
        setProgressVisible (false);
        notifyThumbnailChanged();
        return;
    }

    setProgressValue (0.0);
    setProgressVisible (false);
    notifyThumbnailChanged();
}

void AudioThumbnail::setAudioFile (const File& file, AudioFormatManager* managerToUse)
{
    audioBuffer = nullptr;
    audioFile = file;
    usingAudioFile = true;
    ownedFormatManager.reset();
    audioFormatManager = managerToUse;
    sampleRate = 0.0;
    totalSamples = 0;
    numChannels = 0;

    if (audioFormatManager == nullptr)
    {
        ownedFormatManager = std::make_unique<AudioFormatManager>();
        ownedFormatManager->registerDefaultFormats (AudioFormatType::all);
        audioFormatManager = ownedFormatManager.get();
    }

    if (audioFile.existsAsFile() && audioFormatManager != nullptr)
    {
        if (auto reader = audioFormatManager->createReaderFor (audioFile))
        {
            totalSamples = static_cast<int> (reader->lengthInSamples);
            numChannels = static_cast<int> (reader->numChannels);
            sampleRate = reader->sampleRate;
        }
    }

    if (cacheKey.isEmpty() && audioFile.existsAsFile())
        cacheKey = audioFile.getFullPathName();

    {
        const ScopedLock lock (peakLock);
        peakCache.clear();
        activeProfile.reset();
    }

    pendingSamplesPerPeak.store (0);
    activeJobId.store (++jobCounter);

    if (totalSamples <= 0 || numChannels <= 0)
    {
        setProgressValue (0.0);
        setProgressVisible (false);
        notifyThumbnailChanged();
        return;
    }

    setProgressValue (0.0);
    setProgressVisible (false);
    notifyThumbnailChanged();
}

void AudioThumbnail::clear()
{
    audioBuffer = nullptr;
    sampleRate = 0.0;
    audioFile = File {};
    usingAudioFile = false;
    totalSamples = 0;
    numChannels = 0;
    ownedFormatManager.reset();
    audioFormatManager = nullptr;

    {
        const ScopedLock lock (peakLock);
        peakCache.clear();
        activeProfile.reset();
    }

    pendingSamplesPerPeak.store (0);
    activeJobId.store (++jobCounter);
    setProgressValue (0.0);
    setProgressVisible (false);
    notifyThumbnailChanged();
}

void AudioThumbnail::setMaxPeakCount (int newMaxPeakCount)
{
    maxPeakCount = jmax (1, newMaxPeakCount);
    notifyThumbnailChanged();
}

void AudioThumbnail::setMinimumSamplesPerPeak (int newMinimumSamplesPerPeak)
{
    minimumSamplesPerPeak = jmax (1, newMinimumSamplesPerPeak);
    notifyThumbnailChanged();
}

//==============================================================================
void AudioThumbnail::setBackgroundCalculationEnabled (bool shouldCalculateInBackground) noexcept
{
    useBackgroundCalculation = shouldCalculateInBackground;
    notifyThumbnailChanged();
}

void AudioThumbnail::setThreadPool (ThreadPool* newThreadPool) noexcept
{
    threadPool = newThreadPool;
    notifyThumbnailChanged();
}

void AudioThumbnail::setDiskCacheEnabled (bool shouldUseDiskCache) noexcept
{
    useDiskCache = shouldUseDiskCache;
    notifyThumbnailChanged();
}

void AudioThumbnail::setCacheDirectory (const File& newDirectory)
{
    cacheDirectory = newDirectory;
}

void AudioThumbnail::setCacheKey (const String& newKey)
{
    cacheKey = newKey;
}

//==============================================================================
int AudioThumbnail::getTotalSamples() const noexcept
{
    return totalSamples;
}

int AudioThumbnail::getNumChannels() const noexcept
{
    return numChannels;
}

double AudioThumbnail::timeToSample (double seconds) const noexcept
{
    if (sampleRate <= 0.0)
        return 0.0;

    return seconds * sampleRate;
}

double AudioThumbnail::sampleToTime (double sample) const noexcept
{
    if (sampleRate <= 0.0)
        return 0.0;

    return sample / sampleRate;
}

void AudioThumbnail::paintChannel (Graphics& g,
                                   const Rectangle<float>& lane,
                                   int channelIndex,
                                   const std::vector<float>& minValues,
                                   const std::vector<float>& maxValues,
                                   int startIndex,
                                   int endIndex,
                                   float startX,
                                   float stepX)
{
    ignoreUnused (startX, stepX);

    const float centerY = lane.getCenterY();
    const float amplitude = lane.getHeight() * 0.45f;

    const int numPeaks = jmin (endIndex, static_cast<int> (minValues.size())) - startIndex;
    if (numPeaks <= 0)
        return;

    const int pixelColumns = jmax (1, static_cast<int> (std::ceil (lane.getWidth())));
    const float peaksPerPixel = static_cast<float> (numPeaks) / static_cast<float> (pixelColumns);

    const auto baseColor = getChannelColor (channelIndex);
    g.setStrokeColor (baseColor.withAlpha (0.9f));
    g.setStrokeWidth (1.0f);

    const float minLineHeight = 1.0f;

    for (int pixel = 0; pixel < pixelColumns; ++pixel)
    {
        float minValue = 1.0f;
        float maxValue = -1.0f;

        if (peaksPerPixel >= 1.0f)
        {
            int peakStart = startIndex + static_cast<int> (std::floor (pixel * peaksPerPixel));
            int peakEnd = startIndex + static_cast<int> (std::floor ((pixel + 1) * peaksPerPixel));

            peakStart = jlimit (startIndex, endIndex - 1, peakStart);
            peakEnd = jlimit (peakStart + 1, endIndex, peakEnd);

            for (int peakIndex = peakStart; peakIndex < peakEnd; ++peakIndex)
            {
                minValue = jmin (minValue, minValues[static_cast<size_t> (peakIndex)]);
                maxValue = jmax (maxValue, maxValues[static_cast<size_t> (peakIndex)]);
            }
        }
        else
        {
            const int peakIndex = startIndex
                                + jlimit (0, numPeaks - 1, static_cast<int> (pixel * peaksPerPixel));
            minValue = minValues[static_cast<size_t> (peakIndex)];
            maxValue = maxValues[static_cast<size_t> (peakIndex)];
        }

        float top = centerY - maxValue * amplitude;
        float bottom = centerY - minValue * amplitude;

        top = jlimit (lane.getY(), lane.getBottom(), top);
        bottom = jlimit (lane.getY(), lane.getBottom(), bottom);

        if (bottom < top)
            std::swap (top, bottom);

        const float height = jmax (minLineHeight, bottom - top);
        const float x = lane.getX() + static_cast<float> (pixel) + 0.5f;
        g.strokeLine ({ x, top }, { x, top + height });
    }

    g.setStrokeColor (Color (0xFF3A3A3A));
    g.setStrokeWidth (1.0f);
    g.strokeLine ({ lane.getX(), centerY }, { lane.getRight(), centerY });
}

Color AudioThumbnail::getChannelColor (int channelIndex) const
{
    static const Color colors[] = {
        Color (0xFF5BC0EB),
        Color (0xFFFDE74C),
        Color (0xFF9BC53D),
        Color (0xFFE55934),
        Color (0xFFFA7921),
        Color (0xFF9D4EDD)
    };

    const int colorIndex = channelIndex % static_cast<int> (sizeof (colors) / sizeof (colors[0]));
    return colors[colorIndex];
}

std::shared_ptr<AudioThumbnail::PeakProfile> AudioThumbnail::getActiveProfile() const
{
    const ScopedLock lock (peakLock);
    return activeProfile;
}

void AudioThumbnail::requestProfile (int samplesPerPeak)
{
    if (getTotalSamples() <= 0 || getNumChannels() <= 0)
        return;

    if (samplesPerPeak <= 0)
        return;

    if (pendingSamplesPerPeak.load() == samplesPerPeak)
        return;

    {
        const ScopedLock lock (peakLock);
        if (activeProfile != nullptr && activeProfile->samplesPerPeak == samplesPerPeak)
            return;

        auto cached = findCachedProfile (samplesPerPeak);
        if (cached != nullptr)
        {
            activeJobId.store (++jobCounter);
            pendingSamplesPerPeak.store (0);
            activeProfile = cached;
            setProgressValue (1.0);
            setProgressVisible (false);
            notifyThumbnailChanged();
            return;
        }
    }

    rebuildPeakProfile (samplesPerPeak);
}

void AudioThumbnail::rebuildPeakProfile (int samplesPerPeak)
{
    if (getTotalSamples() <= 0 || getNumChannels() <= 0)
        return;

    pendingSamplesPerPeak.store (samplesPerPeak);

    const int jobId = ++jobCounter;
    activeJobId.store (jobId);

    setProgressVisible (true);
    setProgressValue (0.0);

    if (useDiskCache)
    {
        PeakProfile cachedProfile;
        if (loadProfileFromCache (samplesPerPeak, cachedProfile))
        {
            auto profile = std::make_shared<PeakProfile> (std::move (cachedProfile));
            applyPeakProfile (profile, jobId);
            return;
        }
    }

    if (useBackgroundCalculation && threadPool != nullptr)
    {
        threadPool->addJob (new PeakJob (*this, jobId, samplesPerPeak, useDiskCache), true);
        return;
    }

    auto profile = buildPeakProfile (samplesPerPeak);
    if (profile != nullptr && useDiskCache)
        saveProfileToCache (*profile);

    if (profile == nullptr)
    {
        setProgressValue (0.0);
        setProgressVisible (false);
    }

    applyPeakProfile (std::move (profile), jobId);
}

std::shared_ptr<AudioThumbnail::PeakProfile> AudioThumbnail::buildPeakProfile (int samplesPerPeak, ThreadPoolJob* jobToCheck)
{
    if (! usingAudioFile && audioBuffer == nullptr)
        return nullptr;

    if (usingAudioFile && ! audioFile.existsAsFile())
        return nullptr;

    const int numSamples = getTotalSamples();
    const int numChannels = getNumChannels();
    if (numSamples <= 0 || numChannels <= 0 || samplesPerPeak <= 0)
        return nullptr;

    const int numPeaks = (numSamples + samplesPerPeak - 1) / samplesPerPeak;
    auto profile = std::make_shared<PeakProfile>();
    profile->samplesPerPeak = samplesPerPeak;
    profile->numSamples = numSamples;
    profile->numChannels = numChannels;
    profile->channelPeaks.resize (static_cast<size_t> (numChannels));

    const int progressStride = jmax (1, numPeaks / 100);
    const double totalSteps = static_cast<double> (numPeaks) * numChannels;
    double progressStep = 0.0;

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto& peaks = profile->channelPeaks[static_cast<size_t> (channel)];
        peaks.minValues.assign (static_cast<size_t> (numPeaks), 0.0f);
        peaks.maxValues.assign (static_cast<size_t> (numPeaks), 0.0f);
    }

    if (usingAudioFile)
    {
        if (audioFormatManager == nullptr)
            return nullptr;

        auto reader = audioFormatManager->createReaderFor (audioFile);
        if (reader == nullptr)
            return nullptr;

        std::vector<Range<float>> ranges (static_cast<size_t> (numChannels));

        for (int peakIndex = 0; peakIndex < numPeaks; ++peakIndex)
        {
            if (jobToCheck != nullptr && jobToCheck->shouldExit())
                return nullptr;

            const int64 startSample = static_cast<int64> (peakIndex) * samplesPerPeak;
            const int64 samplesToRead = jmin (static_cast<int64> (samplesPerPeak),
                                              static_cast<int64> (numSamples) - startSample);

            reader->readMaxLevels (startSample,
                                   samplesToRead,
                                   ranges.data(),
                                   numChannels);

            for (int channel = 0; channel < numChannels; ++channel)
            {
                const auto& range = ranges[static_cast<size_t> (channel)];
                auto& peaks = profile->channelPeaks[static_cast<size_t> (channel)];
                peaks.minValues[static_cast<size_t> (peakIndex)] = range.getStart();
                peaks.maxValues[static_cast<size_t> (peakIndex)] = range.getEnd();
            }

            progressStep += numChannels;
            if ((peakIndex % progressStride) == 0)
                setProgressValue (progressStep / totalSteps);
        }

        return profile;
    }

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto& peaks = profile->channelPeaks[static_cast<size_t> (channel)];
        const float* samples = audioBuffer->getReadPointer (channel);

        for (int peakIndex = 0; peakIndex < numPeaks; ++peakIndex)
        {
            if (jobToCheck != nullptr && jobToCheck->shouldExit())
                return nullptr;

            const int startSample = peakIndex * samplesPerPeak;
            const int endSample = jmin (numSamples, startSample + samplesPerPeak);

            float minValue = 1.0f;
            float maxValue = -1.0f;

            for (int sample = startSample; sample < endSample; ++sample)
            {
                const float value = samples[sample];
                minValue = jmin (minValue, value);
                maxValue = jmax (maxValue, value);
            }

            peaks.minValues[static_cast<size_t> (peakIndex)] = minValue;
            peaks.maxValues[static_cast<size_t> (peakIndex)] = maxValue;

            progressStep += 1.0;
            if ((peakIndex % progressStride) == 0)
                setProgressValue (progressStep / totalSteps);
        }
    }

    return profile;
}

void AudioThumbnail::applyPeakProfile (std::shared_ptr<PeakProfile> profile, int jobId)
{
    WeakReference<AudioThumbnail> weakThis (this);

    MessageManager::callAsync ([weakThis, profile = std::move (profile), jobId]()
    {
        auto* thumbnail = weakThis.get();
        if (thumbnail == nullptr)
            return;

        if (profile == nullptr || thumbnail->activeJobId.load() != jobId)
        {
            thumbnail->pendingSamplesPerPeak.store (0);
            return;
        }

        {
            const ScopedLock lock (thumbnail->peakLock);
            thumbnail->peakCache[profile->samplesPerPeak] = profile;
            thumbnail->activeProfile = profile;
        }

        thumbnail->pendingSamplesPerPeak.store (0);
        thumbnail->setProgressValue (1.0);
        thumbnail->setProgressVisible (false);
        thumbnail->notifyThumbnailChanged();
    });
}

std::shared_ptr<AudioThumbnail::PeakProfile> AudioThumbnail::findCachedProfile (int samplesPerPeak) const
{
    auto iterator = peakCache.find (samplesPerPeak);
    if (iterator == peakCache.end())
        return nullptr;

    return iterator->second;
}

int AudioThumbnail::getSamplesPerPeakForView (double viewLengthSamples, float waveformWidth) const
{
    const int totalSamples = getTotalSamples();
    if (totalSamples <= 0)
        return 0;

    const double viewLength = viewLengthSamples > 0.0
                                ? viewLengthSamples
                                : static_cast<double> (totalSamples);

    const double samplesPerPixel = viewLength / jmax (1.0f, waveformWidth);
    int samplesPerPeak = jmax (minimumSamplesPerPeak, static_cast<int> (samplesPerPixel));

    if (maxPeakCount > 0)
    {
        const int minSamplesForMaxPeaks = jmax (1, totalSamples / maxPeakCount);
        samplesPerPeak = jmax (samplesPerPeak, minSamplesForMaxPeaks);
    }

    return samplesPerPeak;
}

Range<double> AudioThumbnail::getClampedViewRange (Range<double> range) const
{
    const double totalSamples = static_cast<double> (getTotalSamples());
    if (totalSamples <= 0.0)
        return {};

    const double clampedLength = jlimit (1.0, totalSamples, range.getLength());
    const double maxStart = jmax (0.0, totalSamples - clampedLength);
    const double start = jlimit (0.0, maxStart, range.getStart());
    return Range<double>::withStartAndLength (start, clampedLength);
}

//==============================================================================
void AudioThumbnail::setProgressVisible (bool shouldShow)
{
    progressVisible.store (shouldShow);
    notifyThumbnailProgress();
}

void AudioThumbnail::setProgressValue (double newProgress)
{
    progressValue.store (newProgress);
    notifyThumbnailProgress();
}

void AudioThumbnail::notifyThumbnailChanged()
{
    if (MessageManager::getInstance()->isThisTheMessageThread())
    {
        listeners.call ([this] (Listener& listener)
        {
            listener.thumbnailChanged (*this);
        });
        return;
    }

    WeakReference<AudioThumbnail> weakThis (this);
    MessageManager::callAsync ([weakThis]()
    {
        auto* thumbnail = weakThis.get();
        if (thumbnail == nullptr)
            return;

        thumbnail->listeners.call ([thumbnail] (Listener& listener)
        {
            listener.thumbnailChanged (*thumbnail);
        });
    });
}

void AudioThumbnail::notifyThumbnailProgress()
{
    if (MessageManager::getInstance()->isThisTheMessageThread())
    {
        const double progress = progressValue.load();
        const bool isVisible = progressVisible.load();
        listeners.call ([this, progress, isVisible] (Listener& listener)
        {
            listener.thumbnailProgressChanged (*this, progress, isVisible);
        });
        return;
    }

    WeakReference<AudioThumbnail> weakThis (this);
    MessageManager::callAsync ([weakThis]()
    {
        auto* thumbnail = weakThis.get();
        if (thumbnail == nullptr)
            return;

        const double progress = thumbnail->progressValue.load();
        const bool isVisible = thumbnail->progressVisible.load();
        thumbnail->listeners.call ([thumbnail, progress, isVisible] (Listener& listener)
        {
            listener.thumbnailProgressChanged (*thumbnail, progress, isVisible);
        });
    });
}

//==============================================================================
bool AudioThumbnail::loadProfileFromCache (int samplesPerPeak, PeakProfile& profile) const
{
    auto cacheFile = getCacheFileForProfile (samplesPerPeak);
    if (! cacheFile.existsAsFile())
        return false;

    FileInputStream input (cacheFile);
    if (! input.openedOk())
        return false;

    const int magic = input.readInt();
    const int version = input.readInt();
    if (magic != kCacheMagic || version != kCacheVersion)
        return false;

    const int numSamples = input.readInt();
    const int numChannels = input.readInt();
    const int cachedSamplesPerPeak = input.readInt();
    const int numPeaks = input.readInt();

    if (numSamples != getTotalSamples()
        || numChannels != getNumChannels()
        || cachedSamplesPerPeak != samplesPerPeak
        || numPeaks <= 0)
    {
        return false;
    }

    profile.samplesPerPeak = samplesPerPeak;
    profile.numSamples = numSamples;
    profile.numChannels = numChannels;
    profile.channelPeaks.resize (static_cast<size_t> (numChannels));

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto& peaks = profile.channelPeaks[static_cast<size_t> (channel)];
        peaks.minValues.resize (static_cast<size_t> (numPeaks));
        peaks.maxValues.resize (static_cast<size_t> (numPeaks));

        for (int i = 0; i < numPeaks; ++i)
            peaks.minValues[static_cast<size_t> (i)] = input.readFloat();

        for (int i = 0; i < numPeaks; ++i)
            peaks.maxValues[static_cast<size_t> (i)] = input.readFloat();
    }

    return true;
}

void AudioThumbnail::saveProfileToCache (const PeakProfile& profile) const
{
    if (! useDiskCache)
        return;

    auto cacheFile = getCacheFileForProfile (profile.samplesPerPeak);
    if (cacheFile == File())
        return;

    if (! cacheFile.getParentDirectory().exists())
        cacheFile.getParentDirectory().createDirectory();

    FileOutputStream output (cacheFile);
    if (! output.openedOk())
        return;

    output.writeInt (kCacheMagic);
    output.writeInt (kCacheVersion);
    output.writeInt (profile.numSamples);
    output.writeInt (profile.numChannels);
    output.writeInt (profile.samplesPerPeak);

    const int numPeaks = profile.channelPeaks.empty()
                           ? 0
                           : static_cast<int> (profile.channelPeaks[0].minValues.size());
    output.writeInt (numPeaks);

    for (int channel = 0; channel < profile.numChannels; ++channel)
    {
        const auto& peaks = profile.channelPeaks[static_cast<size_t> (channel)];
        for (int i = 0; i < numPeaks; ++i)
            output.writeFloat (peaks.minValues[static_cast<size_t> (i)]);

        for (int i = 0; i < numPeaks; ++i)
            output.writeFloat (peaks.maxValues[static_cast<size_t> (i)]);
    }
}

File AudioThumbnail::getCacheFileForProfile (int samplesPerPeak) const
{
    if (! useDiskCache || cacheKey.isEmpty() || cacheDirectory == File())
        return {};

    const auto safeKey = File::createLegalFileName (cacheKey);
    const auto fileName = safeKey + "_" + String (getTotalSamples()) + "_"
                        + String (getNumChannels()) + "_"
                        + String (samplesPerPeak) + ".yupthumb";
    return cacheDirectory.getChildFile (fileName);
}

} // namespace yup
