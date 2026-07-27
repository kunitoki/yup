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
/**
    Stores multi-resolution peak data for efficient waveform rendering at any zoom level.

    This class computes and stores peak profiles (min/max values) from audio sources at
    multiple resolution levels. The base level stores high-resolution peaks, and additional
    aggregated levels provide progressively coarser representations for efficient rendering
    when zoomed out.

    The resolution is adaptive based on file size:
    - Files < 10M samples: 1 sample per peak (full resolution)
    - Files 10M-100M samples: 256 samples per peak
    - Files 100M-1B samples: 512 samples per peak
    - Files > 1B samples: 1024 samples per peak

    Aggregation levels (16x, 256x, 4096x) are pre-computed for optimal zoomed-out performance.

    @see AudioPeakProfileCache, AudioThumbnail
*/
class YUP_API AudioPeakProfile
{
public:
    //==============================================================================
    /** Stores min/max peak values for a single channel. */
    struct ChannelPeaks
    {
        std::vector<float> minValues;
        std::vector<float> maxValues;
    };

    //==============================================================================
    /** Creates an empty, invalid profile. */
    AudioPeakProfile();

    /** Destructor. */
    ~AudioPeakProfile();

    //==============================================================================
    /** Builds a peak profile from an audio buffer.

        @param buffer               The audio buffer to analyze
        @param baseResolution       Number of samples per base-level peak
        @param aggregationFactors   Aggregation multipliers (e.g., [16, 256, 4096])
        @param progressCallback     Optional callback for progress updates (returns false to cancel)
        @returns                    Result indicating success or failure
    */
    Result buildFromBuffer (const AudioBuffer<float>& buffer,
                            int baseResolution,
                            const std::vector<int>& aggregationFactors,
                            std::function<bool (double)> progressCallback = nullptr);

    /** Builds a peak profile from an audio format reader.

        This method is more efficient for large files as it doesn't require loading
        the entire audio into memory.

        @param reader               The audio format reader
        @param baseResolution       Number of samples per base-level peak
        @param aggregationFactors   Aggregation multipliers (e.g., [16, 256, 4096])
        @param progressCallback     Optional callback for progress updates (returns false to cancel)
        @returns                    Result indicating success or failure
    */
    Result buildFromReader (AudioFormatReader& reader,
                            int baseResolution,
                            const std::vector<int>& aggregationFactors,
                            std::function<bool (double)> progressCallback = nullptr);

    //==============================================================================
    /** Returns the total number of samples represented by this profile. */
    int getNumSamples() const noexcept { return numSamples; }

    /** Returns the number of audio channels in this profile. */
    int getNumChannels() const noexcept { return numChannels; }

    /** Returns the base resolution (samples per peak) for level 0. */
    int getBaseResolution() const noexcept { return baseResolution; }

    /** Returns true if this profile has been successfully built and contains valid data. */
    bool isValid() const noexcept { return numSamples > 0 && numChannels > 0 && ! levels.empty(); }

    //==============================================================================
    /** Returns the peak data for a specific channel and aggregation level.

        @param channel           The channel index (0-based)
        @param aggregationLevel  The aggregation level (0 = base, 1+ = aggregated)
        @returns                 Reference to the ChannelPeaks for the specified channel/level
    */
    const ChannelPeaks& getChannelPeaks (int channel, int aggregationLevel = 0) const;

    /** Returns the total number of aggregation levels (including base level 0). */
    int getNumAggregationLevels() const noexcept { return static_cast<int> (levels.size()); }

    /** Returns the aggregation factor for a specific level.

        @param level  The aggregation level (0 = base with factor 1)
        @returns      The aggregation factor (e.g., 1, 16, 256, 4096)
    */
    int getAggregationFactor (int level) const;

    /** Calculates the peak index range for a given sample range at a specific aggregation level.

        @param sampleRange       The range of samples
        @param aggregationLevel  The aggregation level
        @returns                 The corresponding range of peak indices
    */
    Range<int> getPeakRangeForSamples (Range<int> sampleRange, int aggregationLevel) const;

    //==============================================================================
    /** Saves the profile to a disk file for caching.

        @param file  The file to save to
        @returns     Result indicating success or failure
    */
    Result saveToFile (const File& file) const;

    /** Loads a profile from a disk cache file.

        @param file  The file to load from
        @returns     Result indicating success or failure
    */
    Result loadFromFile (const File& file);

    /** Serializes the profile to a memory block.

        @returns  The serialized data
    */
    MemoryBlock serialize() const;

    /** Deserializes a profile from a memory block.

        @param data  The serialized data
        @returns     Result indicating success or failure
    */
    Result deserialize (const MemoryBlock& data);

    //==============================================================================
    /** Calculates the optimal base resolution based on file size.

        This provides adaptive resolution to balance memory usage and quality.

        @param numSamples  The total number of samples in the audio
        @returns           The recommended samples per peak
    */
    static int calculateOptimalBaseResolution (int64 numSamples);

    /** Returns the default aggregation factors used for multi-level caching.

        @returns  Vector of aggregation factors [16, 256, 4096]
    */
    static std::vector<int> getDefaultAggregationFactors();

private:
    //==============================================================================
    struct Level
    {
        int aggregationFactor = 1;
        std::vector<ChannelPeaks> channelPeaks;
    };

    void computeAggregatedLevel (int sourceLevelIndex, int aggregationFactor);
    void clear();

    int numSamples = 0;
    int numChannels = 0;
    int baseResolution = 1;
    std::vector<Level> levels; // levels[0] = base, levels[1+] = aggregated

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPeakProfile)
};

} // namespace yup
