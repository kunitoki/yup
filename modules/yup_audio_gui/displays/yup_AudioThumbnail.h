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
    Draws a multi-channel waveform thumbnail with zooming and scrolling support.

    The thumbnail caches peak profiles to avoid recomputing waveform data on every repaint,
    with optional background generation via a ThreadPool and optional persistence on disk.

    The provided audio buffer must remain valid for the lifetime of the thumbnail or until
    a new buffer is assigned.

    Use AudioViewComponent to render the thumbnail with zooming, scrolling, and overlays.
*/
class YUP_API AudioThumbnail
{
public:
    //==============================================================================
    class Listener
    {
    public:
        virtual ~Listener() = default;

        virtual void thumbnailChanged (AudioThumbnail& thumbnail) { ignoreUnused (thumbnail); }

        virtual void thumbnailProgressChanged (AudioThumbnail& thumbnail, double progress, bool isVisible)
        {
            ignoreUnused (thumbnail, progress, isVisible);
        }
    };

    struct ChannelPeaks
    {
        std::vector<float> minValues;
        std::vector<float> maxValues;
    };

    struct PeakProfile
    {
        int samplesPerPeak = 0;
        int numSamples = 0;
        int numChannels = 0;
        std::vector<ChannelPeaks> channelPeaks;
    };

    //==============================================================================
    /** Creates an empty AudioThumbnail. */
    AudioThumbnail();

    /** Destructor. */
    ~AudioThumbnail();

    //==============================================================================
    /** Registers a listener for thumbnail changes. */
    void addListener (Listener* listener);

    /** Unregisters a listener for thumbnail changes. */
    void removeListener (Listener* listener);

    //==============================================================================
    /** Assigns the buffer to render and refreshes the peak cache.

        The buffer must remain valid for the lifetime of the thumbnail (or until another
        buffer is assigned).
    */
    void setAudioBuffer (const AudioBuffer<float>* newBuffer, double newSampleRate = 0.0);

    /** Assigns an audio file to render and refreshes the peak cache.

        The optional AudioFormatManager can be provided to share format registrations.
        When omitted, a private manager is created with default formats.
    */
    void setAudioFile (const File& file, AudioFormatManager* managerToUse = nullptr);

    /** Returns the currently assigned audio buffer. */
    const AudioBuffer<float>* getAudioBuffer() const noexcept { return audioBuffer; }

    /** Returns the currently assigned audio file (may be empty). */
    const File& getAudioFile() const noexcept { return audioFile; }

    /** Returns true if the thumbnail is using an audio file source. */
    bool isUsingAudioFile() const noexcept { return usingAudioFile; }

    /** Clears the waveform display and cache. */
    void clear();

    //==============================================================================
    /** Sets the maximum number of cached peaks to generate per profile. */
    void setMaxPeakCount (int newMaxPeakCount);

    /** Returns the maximum number of cached peaks per profile. */
    int getMaxPeakCount() const noexcept { return maxPeakCount; }

    /** Sets the minimum number of samples per peak. */
    void setMinimumSamplesPerPeak (int newMinimumSamplesPerPeak);

    /** Returns the minimum number of samples per peak. */
    int getMinimumSamplesPerPeak() const noexcept { return minimumSamplesPerPeak; }

    //==============================================================================
    /** Enables or disables background peak calculation. */
    void setBackgroundCalculationEnabled (bool shouldCalculateInBackground) noexcept;

    /** Returns true if background calculation is enabled. */
    bool isBackgroundCalculationEnabled() const noexcept { return useBackgroundCalculation; }

    /** Assigns a thread pool to use for background generation. */
    void setThreadPool (ThreadPool* newThreadPool) noexcept;

    //==============================================================================
    /** Enables or disables persistent disk caching of peak profiles. */
    void setDiskCacheEnabled (bool shouldUseDiskCache) noexcept;

    /** Returns true if disk caching is enabled. */
    bool isDiskCacheEnabled() const noexcept { return useDiskCache; }

    /** Sets the directory to use for cached peak profiles. */
    void setCacheDirectory (const File& newDirectory);

    /** Sets the cache key used to name cache files. */
    void setCacheKey (const String& newKey);

    //==============================================================================
    /** Returns the total sample count of the assigned source. */
    int getTotalSamples() const noexcept;

    /** Returns the total channel count of the assigned source. */
    int getNumChannels() const noexcept;

    /** Returns the sample rate associated with the buffer. */
    double getSampleRate() const noexcept { return sampleRate; }

    /** Converts a time in seconds to a sample position. */
    double timeToSample (double seconds) const noexcept;

    /** Converts a sample position to time in seconds. */
    double sampleToTime (double sample) const noexcept;

    /** Returns the active peak profile, or nullptr if not yet available. */
    std::shared_ptr<PeakProfile> getActiveProfile() const;

    /** Returns the progress value of the current profile build. */
    double getProgress() const noexcept { return progressValue.load(); }

    /** Returns true if progress is currently visible. */
    bool isProgressVisible() const noexcept { return progressVisible.load(); }

    /** Requests a peak profile for the given samples per peak. */
    void requestProfile (int samplesPerPeak);

    /** Returns the number of samples per peak for a given view length and width. */
    int getSamplesPerPeakForView (double viewLengthSamples, float waveformWidth) const;

    /** Clamps a view range to the available sample range. */
    Range<double> getClampedViewRange (Range<double> range) const;

    /** Paints a single channel lane using cached peaks. */
    virtual void paintChannel (Graphics& g,
                               const Rectangle<float>& lane,
                               int channelIndex,
                               const std::vector<float>& minValues,
                               const std::vector<float>& maxValues,
                               int startIndex,
                               int endIndex,
                               float startX,
                               float stepX);

    /** Returns the waveform color for the given channel index. */
    virtual Color getChannelColor (int channelIndex) const;

private:
    struct PeakJob;
    friend struct PeakJob;

    void rebuildPeakProfile (int samplesPerPeak);
    std::shared_ptr<PeakProfile> buildPeakProfile (int samplesPerPeak, ThreadPoolJob* jobToCheck = nullptr);
    void applyPeakProfile (std::shared_ptr<PeakProfile> profile, int jobId);
    std::shared_ptr<PeakProfile> findCachedProfile (int samplesPerPeak) const;
    void setProgressVisible (bool shouldShow);
    void setProgressValue (double newProgress);
    void notifyThumbnailChanged();
    void notifyThumbnailProgress();

    bool loadProfileFromCache (int samplesPerPeak, PeakProfile& profile) const;
    void saveProfileToCache (const PeakProfile& profile) const;
    File getCacheFileForProfile (int samplesPerPeak) const;

    const AudioBuffer<float>* audioBuffer = nullptr;
    double sampleRate = 0.0;
    File audioFile;
    bool usingAudioFile = false;
    int totalSamples = 0;
    int numChannels = 0;
    std::unique_ptr<AudioFormatManager> ownedFormatManager;
    AudioFormatManager* audioFormatManager = nullptr;
    int maxPeakCount = 120000;
    int minimumSamplesPerPeak = 1;

    std::shared_ptr<PeakProfile> activeProfile;
    std::map<int, std::shared_ptr<PeakProfile>> peakCache;

    mutable CriticalSection peakLock;

    ThreadPool* threadPool = nullptr;
    bool useBackgroundCalculation = false;
    bool useDiskCache = false;
    File cacheDirectory;
    String cacheKey;

    std::atomic<int> jobCounter { 0 };
    std::atomic<int> activeJobId { 0 };
    std::atomic<int> pendingSamplesPerPeak { 0 };
    std::atomic<double> progressValue { 0.0 };
    std::atomic<bool> progressVisible { false };
    ListenerList<Listener> listeners;
    WeakReference<AudioThumbnail>::Master masterReference;
    friend class WeakReference<AudioThumbnail>;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioThumbnail)
};

} // namespace yup
