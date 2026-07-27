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
AudioPeakProfile::AudioPeakProfile()
{
}

AudioPeakProfile::~AudioPeakProfile()
{
}

//==============================================================================
Result AudioPeakProfile::buildFromBuffer (const AudioBuffer<float>& buffer,
                                          int baseRes,
                                          const std::vector<int>& aggregationFactors,
                                          std::function<bool (double)> progressCallback)
{
    clear();

    if (buffer.getNumSamples() == 0 || buffer.getNumChannels() == 0)
        return Result::fail ("Buffer is empty");

    if (baseRes < 1)
        return Result::fail ("Base resolution must be >= 1");

    numSamples = buffer.getNumSamples();
    numChannels = buffer.getNumChannels();
    baseResolution = baseRes;

    // Calculate number of peaks needed
    const int numPeaks = (numSamples + baseResolution - 1) / baseResolution;

    // Build base level (level 0)
    Level baseLevel;
    baseLevel.aggregationFactor = 1;
    baseLevel.channelPeaks.resize (numChannels);

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto& peaks = baseLevel.channelPeaks[channel];
        peaks.minValues.resize (numPeaks);
        peaks.maxValues.resize (numPeaks);

        const float* channelData = buffer.getReadPointer (channel);

        for (int peakIndex = 0; peakIndex < numPeaks; ++peakIndex)
        {
            const int startSample = peakIndex * baseResolution;
            const int endSample = jmin (numSamples, startSample + baseResolution);

            if (startSample >= endSample)
                continue;

            float minValue = channelData[startSample];
            float maxValue = channelData[startSample];

            for (int sample = startSample + 1; sample < endSample; ++sample)
            {
                const float value = channelData[sample];
                minValue = jmin (minValue, value);
                maxValue = jmax (maxValue, value);
            }

            peaks.minValues[peakIndex] = minValue;
            peaks.maxValues[peakIndex] = maxValue;
        }

        // Report progress for base level computation
        if (progressCallback)
        {
            const double progress = (channel + 1.0) / (numChannels * (1.0 + aggregationFactors.size()));
            if (! progressCallback (progress))
                return Result::fail ("Cancelled by user");
        }
    }

    levels.push_back (std::move (baseLevel));

    // Build aggregated levels (always aggregate from base level)
    for (size_t i = 0; i < aggregationFactors.size(); ++i)
    {
        computeAggregatedLevel (0, aggregationFactors[i]);

        if (progressCallback)
        {
            const double progress = (numChannels + (i + 1.0) * numChannels) / (numChannels * (1.0 + aggregationFactors.size()));
            if (! progressCallback (progress))
                return Result::fail ("Cancelled by user");
        }
    }

    return Result::ok();
}

Result AudioPeakProfile::buildFromReader (AudioFormatReader& reader,
                                          int baseRes,
                                          const std::vector<int>& aggregationFactors,
                                          std::function<bool (double)> progressCallback)
{
    clear();

    if (reader.lengthInSamples == 0 || reader.numChannels == 0)
        return Result::fail ("Reader has no audio data");

    if (baseRes < 1)
        return Result::fail ("Base resolution must be >= 1");

    numSamples = static_cast<int> (reader.lengthInSamples);
    numChannels = static_cast<int> (reader.numChannels);
    baseResolution = baseRes;

    // Calculate number of peaks needed
    const int numPeaks = (numSamples + baseResolution - 1) / baseResolution;

    // Build base level using reader's readMaxLevels for efficiency
    Level baseLevel;
    baseLevel.aggregationFactor = 1;
    baseLevel.channelPeaks.resize (numChannels);

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto& peaks = baseLevel.channelPeaks[channel];
        peaks.minValues.resize (numPeaks);
        peaks.maxValues.resize (numPeaks);
    }

    // Read in chunks to avoid excessive memory usage
    const int chunkSize = jmin (8192, baseResolution);
    AudioBuffer<float> tempBuffer (numChannels, chunkSize);

    for (int peakIndex = 0; peakIndex < numPeaks; ++peakIndex)
    {
        const int64 startSample = static_cast<int64> (peakIndex) * baseResolution;
        const int64 endSample = jmin (static_cast<int64> (numSamples), startSample + baseResolution);
        const int samplesInPeak = static_cast<int> (endSample - startSample);

        // Read samples for this peak
        int samplesRead = 0;
        while (samplesRead < samplesInPeak)
        {
            const int samplesToRead = jmin (chunkSize, samplesInPeak - samplesRead);
            const int64 readPosition = startSample + samplesRead;

            if (! reader.read (&tempBuffer, 0, samplesToRead, readPosition, true, true))
                return Result::fail ("Failed to read from audio file");

            // Update min/max for each channel
            for (int channel = 0; channel < numChannels; ++channel)
            {
                const float* data = tempBuffer.getReadPointer (channel);
                float& minValue = baseLevel.channelPeaks[channel].minValues[peakIndex];
                float& maxValue = baseLevel.channelPeaks[channel].maxValues[peakIndex];

                if (samplesRead == 0 && samplesToRead > 0)
                {
                    minValue = data[0];
                    maxValue = data[0];
                }

                for (int i = (samplesRead == 0 ? 1 : 0); i < samplesToRead; ++i)
                {
                    const float value = data[i];
                    minValue = jmin (minValue, value);
                    maxValue = jmax (maxValue, value);
                }
            }

            samplesRead += samplesToRead;
        }

        // Report progress periodically
        if (progressCallback && (peakIndex % 1000 == 0 || peakIndex == numPeaks - 1))
        {
            const double baseProgress = (peakIndex + 1.0) / numPeaks;
            const double progress = baseProgress / (1.0 + aggregationFactors.size());
            if (! progressCallback (progress))
                return Result::fail ("Cancelled by user");
        }
    }

    levels.push_back (std::move (baseLevel));

    // Build aggregated levels (always aggregate from base level)
    for (size_t i = 0; i < aggregationFactors.size(); ++i)
    {
        computeAggregatedLevel (0, aggregationFactors[i]);

        if (progressCallback)
        {
            const double progress = (1.0 + i + 1.0) / (1.0 + aggregationFactors.size());
            if (! progressCallback (progress))
                return Result::fail ("Cancelled by user");
        }
    }

    return Result::ok();
}

//==============================================================================
const AudioPeakProfile::ChannelPeaks& AudioPeakProfile::getChannelPeaks (int channel, int aggregationLevel) const
{
    jassert (isValid());
    jassert (channel >= 0 && channel < numChannels);
    jassert (aggregationLevel >= 0 && aggregationLevel < static_cast<int> (levels.size()));

    aggregationLevel = jlimit (0, static_cast<int> (levels.size()) - 1, aggregationLevel);
    channel = jlimit (0, numChannels - 1, channel);

    return levels[aggregationLevel].channelPeaks[channel];
}

int AudioPeakProfile::getAggregationFactor (int level) const
{
    jassert (level >= 0 && level < static_cast<int> (levels.size()));

    if (level < 0 || level >= static_cast<int> (levels.size()))
        return 1;

    return levels[level].aggregationFactor;
}

Range<int> AudioPeakProfile::getPeakRangeForSamples (Range<int> sampleRange, int aggregationLevel) const
{
    jassert (isValid());
    jassert (aggregationLevel >= 0 && aggregationLevel < static_cast<int> (levels.size()));

    if (! isValid() || aggregationLevel < 0 || aggregationLevel >= static_cast<int> (levels.size()))
        return Range<int>();

    const int effectiveResolution = baseResolution * levels[aggregationLevel].aggregationFactor;
    const int startPeak = sampleRange.getStart() / effectiveResolution;
    const int endPeak = (sampleRange.getEnd() + effectiveResolution - 1) / effectiveResolution;

    return Range<int> (startPeak, endPeak);
}

//==============================================================================
Result AudioPeakProfile::saveToFile (const File& file) const
{
    if (! isValid())
        return Result::fail ("Cannot save invalid profile");

    try
    {
        FileOutputStream stream (file);

        if (! stream.openedOk())
            return Result::fail ("Failed to open file for writing");

        // Write header
        stream.write ("YUPPEAKS", 8);
        stream.writeInt (1); // Version
        stream.writeInt (0); // Reserved

        // Write metadata
        stream.writeInt (numSamples);
        stream.writeInt (numChannels);
        stream.writeInt (baseResolution);
        stream.writeInt (static_cast<int> (levels.size()));

        // Write each level
        for (const auto& level : levels)
        {
            stream.writeInt (level.aggregationFactor);

            for (const auto& channelPeaks : level.channelPeaks)
            {
                const int numPeaks = static_cast<int> (channelPeaks.minValues.size());
                stream.writeInt (numPeaks);

                stream.write (channelPeaks.minValues.data(), numPeaks * sizeof (float));
                stream.write (channelPeaks.maxValues.data(), numPeaks * sizeof (float));
            }
        }

        return Result::ok();
    }
    catch (...)
    {
        return Result::fail ("Exception while saving profile");
    }
}

Result AudioPeakProfile::loadFromFile (const File& file)
{
    clear();

    if (! file.existsAsFile())
        return Result::fail ("File does not exist");

    try
    {
        FileInputStream stream (file);

        if (! stream.openedOk())
            return Result::fail ("Failed to open file for reading");

        // Read and verify header
        char magic[9] = { 0 };
        stream.read (magic, 8);

        if (std::string (magic) != "YUPPEAKS")
            return Result::fail ("Invalid file format");

        const int version = stream.readInt();
        if (version != 1)
            return Result::fail ("Unsupported version");

        stream.readInt(); // Reserved

        // Read metadata
        numSamples = stream.readInt();
        numChannels = stream.readInt();
        baseResolution = stream.readInt();
        const int numLevels = stream.readInt();

        if (numSamples <= 0 || numChannels <= 0 || baseResolution <= 0 || numLevels <= 0)
            return Result::fail ("Invalid metadata");

        // Read each level
        levels.reserve (numLevels);

        for (int levelIndex = 0; levelIndex < numLevels; ++levelIndex)
        {
            Level level;
            level.aggregationFactor = stream.readInt();
            level.channelPeaks.resize (numChannels);

            for (int channel = 0; channel < numChannels; ++channel)
            {
                const int numPeaks = stream.readInt();

                if (numPeaks <= 0)
                    return Result::fail ("Invalid peak count");

                auto& peaks = level.channelPeaks[channel];
                peaks.minValues.resize (numPeaks);
                peaks.maxValues.resize (numPeaks);

                stream.read (peaks.minValues.data(), numPeaks * sizeof (float));
                stream.read (peaks.maxValues.data(), numPeaks * sizeof (float));
            }

            levels.push_back (std::move (level));
        }

        return Result::ok();
    }
    catch (...)
    {
        clear();
        return Result::fail ("Exception while loading profile");
    }
}

MemoryBlock AudioPeakProfile::serialize() const
{
    MemoryBlock block;

    if (! isValid())
        return block;

    MemoryOutputStream stream (block, false);

    // Write header
    stream.write ("YUPPEAKS", 8);
    stream.writeInt (1); // Version
    stream.writeInt (0); // Reserved

    // Write metadata
    stream.writeInt (numSamples);
    stream.writeInt (numChannels);
    stream.writeInt (baseResolution);
    stream.writeInt (static_cast<int> (levels.size()));

    // Write each level
    for (const auto& level : levels)
    {
        stream.writeInt (level.aggregationFactor);

        for (const auto& channelPeaks : level.channelPeaks)
        {
            const int numPeaks = static_cast<int> (channelPeaks.minValues.size());
            stream.writeInt (numPeaks);

            stream.write (channelPeaks.minValues.data(), numPeaks * sizeof (float));
            stream.write (channelPeaks.maxValues.data(), numPeaks * sizeof (float));
        }
    }

    return block;
}

Result AudioPeakProfile::deserialize (const MemoryBlock& data)
{
    clear();

    if (data.getSize() < 20)
        return Result::fail ("Data too small");

    try
    {
        MemoryInputStream stream (data, false);

        // Read and verify header
        char magic[9] = { 0 };
        stream.read (magic, 8);

        if (std::string (magic) != "YUPPEAKS")
            return Result::fail ("Invalid format");

        const int version = stream.readInt();
        if (version != 1)
            return Result::fail ("Unsupported version");

        stream.readInt(); // Reserved

        // Read metadata
        numSamples = stream.readInt();
        numChannels = stream.readInt();
        baseResolution = stream.readInt();
        const int numLevels = stream.readInt();

        if (numSamples <= 0 || numChannels <= 0 || baseResolution <= 0 || numLevels <= 0)
            return Result::fail ("Invalid metadata");

        // Read each level
        levels.reserve (numLevels);

        for (int levelIndex = 0; levelIndex < numLevels; ++levelIndex)
        {
            Level level;
            level.aggregationFactor = stream.readInt();
            level.channelPeaks.resize (numChannels);

            for (int channel = 0; channel < numChannels; ++channel)
            {
                const int numPeaks = stream.readInt();

                if (numPeaks <= 0)
                    return Result::fail ("Invalid peak count");

                auto& peaks = level.channelPeaks[channel];
                peaks.minValues.resize (numPeaks);
                peaks.maxValues.resize (numPeaks);

                stream.read (peaks.minValues.data(), numPeaks * sizeof (float));
                stream.read (peaks.maxValues.data(), numPeaks * sizeof (float));
            }

            levels.push_back (std::move (level));
        }

        return Result::ok();
    }
    catch (...)
    {
        clear();
        return Result::fail ("Exception while deserializing");
    }
}

//==============================================================================
int AudioPeakProfile::calculateOptimalBaseResolution (int64 numSamples)
{
    if (numSamples < 10'000'000)
        return 1; // < 10M samples: sample-level resolution

    if (numSamples < 100'000'000)
        return 256; // 10M-100M: 256 samples/peak

    if (numSamples < 1'000'000'000)
        return 512; // 100M-1B: 512 samples/peak

    return 1024; // > 1B samples: 1024 samples/peak
}

std::vector<int> AudioPeakProfile::getDefaultAggregationFactors()
{
    return { 16, 256, 4096 };
}

//==============================================================================
void AudioPeakProfile::computeAggregatedLevel (int sourceLevelIndex, int aggregationFactor)
{
    jassert (sourceLevelIndex >= 0 && sourceLevelIndex < static_cast<int> (levels.size()));
    jassert (aggregationFactor > 1);

    const auto& sourceLevel = levels[sourceLevelIndex];
    const int sourcePeakCount = static_cast<int> (sourceLevel.channelPeaks[0].minValues.size());
    const int aggregatedPeakCount = (sourcePeakCount + aggregationFactor - 1) / aggregationFactor;

    Level aggregatedLevel;
    aggregatedLevel.aggregationFactor = sourceLevel.aggregationFactor * aggregationFactor;
    aggregatedLevel.channelPeaks.resize (numChannels);

    for (int channel = 0; channel < numChannels; ++channel)
    {
        const auto& sourcePeaks = sourceLevel.channelPeaks[channel];
        auto& aggregatedPeaks = aggregatedLevel.channelPeaks[channel];

        aggregatedPeaks.minValues.resize (aggregatedPeakCount);
        aggregatedPeaks.maxValues.resize (aggregatedPeakCount);

        for (int aggIndex = 0; aggIndex < aggregatedPeakCount; ++aggIndex)
        {
            const int startPeak = aggIndex * aggregationFactor;
            const int endPeak = jmin (sourcePeakCount, startPeak + aggregationFactor);

            if (startPeak >= endPeak)
                continue;

            float minValue = sourcePeaks.minValues[startPeak];
            float maxValue = sourcePeaks.maxValues[startPeak];

            for (int peakIndex = startPeak + 1; peakIndex < endPeak; ++peakIndex)
            {
                minValue = jmin (minValue, sourcePeaks.minValues[peakIndex]);
                maxValue = jmax (maxValue, sourcePeaks.maxValues[peakIndex]);
            }

            aggregatedPeaks.minValues[aggIndex] = minValue;
            aggregatedPeaks.maxValues[aggIndex] = maxValue;
        }
    }

    levels.push_back (std::move (aggregatedLevel));
}

void AudioPeakProfile::clear()
{
    numSamples = 0;
    numChannels = 0;
    baseResolution = 1;
    levels.clear();
}

} // namespace yup
