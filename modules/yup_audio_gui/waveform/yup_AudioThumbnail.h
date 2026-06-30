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

    This class renders audio waveforms using pre-computed peak profiles from AudioPeakProfile.
    Peak computation happens once at load time with adaptive resolution, and multi-level
    aggregation provides efficient rendering at all zoom levels.

    Features:
    - Automatic peak profile generation with adaptive resolution
    - Multi-level aggregation (16x, 256x, 4096x) for optimal performance
    - Rectangle rendering when zoomed in (single peak spans multiple pixels)
    - Line rendering when zoomed out (multiple peaks per pixel)
    - Optional background computation via ThreadPool
    - Optional disk cache persistence

    @see AudioPeakProfile, AudioPeakProfileCache, AudioViewComponent
*/
class YUP_API AudioThumbnail : public AudioPeakProfileCache::Listener
{
public:
    //==============================================================================
    /** Receives notifications about thumbnail changes and progress. */
    class Listener
    {
    public:
        virtual ~Listener() = default;

        /** Called when the thumbnail data has changed and should be repainted. */
        virtual void thumbnailChanged (AudioThumbnail& thumbnail) { ignoreUnused (thumbnail); }

        /** Called periodically during peak profile generation.

            @param thumbnail    The thumbnail that changed
            @param progress     Progress value from 0.0 to 1.0
            @param isVisible    True if progress should be displayed
        */
        virtual void thumbnailProgressChanged (AudioThumbnail& thumbnail, double progress, bool isVisible)
        {
            ignoreUnused (thumbnail, progress, isVisible);
        }
    };

    //==============================================================================
    /** Creates an empty AudioThumbnail with an optional shared cache.

        @param cacheToUse  Optional cache to use (creates default if nullptr)
    */
    explicit AudioThumbnail (std::shared_ptr<AudioPeakProfileCache> cacheToUse = nullptr);

    /** Destructor. */
    ~AudioThumbnail();

    //==============================================================================
    /** Registers a listener for thumbnail changes. */
    void addListener (Listener* listener);

    /** Unregisters a listener for thumbnail changes. */
    void removeListener (Listener* listener);

    //==============================================================================
    /** Sets an audio source from a buffer pointer.

        The buffer must remain valid for the lifetime of the thumbnail or until
        another source is assigned.

        @param buffer      Pointer to the audio buffer (not owned)
        @param sampleRate  The sample rate of the audio
    */
    void setSource (const AudioBuffer<float>* buffer, double sampleRate = 0.0);

    /** Sets an audio source by copying a buffer.

        The buffer is copied and owned internally.

        @param buffer      The audio buffer to copy
        @param sampleRate  The sample rate of the audio
    */
    void setSource (const AudioBuffer<float>& buffer, double sampleRate = 0.0);

    /** Sets an audio source by moving a buffer.

        Takes ownership of the buffer via move semantics.

        @param buffer      The audio buffer to move
        @param sampleRate  The sample rate of the audio
    */
    void setSource (AudioBuffer<float>&& buffer, double sampleRate = 0.0);

    /** Sets an audio source from an AudioFormatReader.

        Takes ownership of the reader.

        @param reader      The audio format reader (ownership transferred)
        @param sampleRate  Optional override for sample rate (uses reader's if 0)
    */
    void setSource (std::unique_ptr<AudioFormatReader> reader, double sampleRate = 0.0);

    /** Clears the waveform display and cache. */
    void clear();

    //==============================================================================
    /** Returns the total sample count of the assigned source. */
    int getTotalSamples() const noexcept { return totalSamples; }

    /** Returns the total channel count of the assigned source. */
    int getNumChannels() const noexcept { return numChannels; }

    /** Returns the sample rate associated with the audio source. */
    double getSampleRate() const noexcept { return sampleRate; }

    /** Returns the active peak profile, or nullptr if not yet available. */
    std::shared_ptr<AudioPeakProfile> getPeakProfile() const;

    /** Returns the progress value of the current profile build (0.0 to 1.0). */
    double getProgress() const noexcept { return progressValue.load(); }

    /** Returns true if progress is currently visible. */
    bool isProgressVisible() const noexcept { return progressVisible.load(); }

    /** Clamps a view range to the available sample range. */
    Range<double> getClampedViewRange (Range<double> range) const;

    //==============================================================================
    /** Paints a single channel waveform within the specified lane.

        This method automatically selects the optimal aggregation level based on zoom,
        and uses rectangle rendering when zoomed in or line rendering when zoomed out.

        @param g             The graphics context
        @param lane          The rectangle area to paint within
        @param channelIndex  The audio channel index to paint
        @param sampleRange   The range of samples to display
        @param pixelWidth    The width in pixels for rendering
    */
    virtual void paintChannel (Graphics& g,
                               const Rectangle<float>& lane,
                               int channelIndex,
                               Range<double> sampleRange,
                               float pixelWidth);

    /** Returns the waveform color for the given channel index.

        Override this to customize channel colors.

        @param channelIndex  The channel index (0-based)
        @returns             The color to use for rendering this channel
    */
    virtual Color getChannelColor (int channelIndex) const;

private:
    //==============================================================================
    // AudioPeakProfileCache::Listener implementation
    void profileReady (const String& cacheKey, std::shared_ptr<AudioPeakProfile> profile) override;
    void profileProgress (const String& cacheKey, double progress) override;

    // Helper methods
    void requestPeakProfile();
    String getCurrentCacheKey() const;
    const AudioBuffer<float>* getActiveBuffer() const;
    void setProgressVisible (bool shouldShow);
    void setProgressValue (double newProgress);
    void notifyThumbnailChanged();
    void notifyThumbnailProgress();

    // Audio source storage
    const AudioBuffer<float>* audioBufferPtr = nullptr;
    std::unique_ptr<AudioBuffer<float>> ownedAudioBuffer;
    std::unique_ptr<AudioFormatReader> ownedReader;

    // Metadata
    double sampleRate = 0.0;
    int totalSamples = 0;
    int numChannels = 0;

    // Peak profile management
    String currentCacheKey;
    std::shared_ptr<AudioPeakProfile> currentProfile;
    std::shared_ptr<AudioPeakProfileCache> cache;

    // Progress tracking
    std::atomic<double> progressValue { 0.0 };
    std::atomic<bool> progressVisible { false };

    // Listeners
    ListenerList<Listener, Array<Listener*, CriticalSection>> listeners;

    WeakReference<AudioThumbnail>::Master masterReference;
    friend class WeakReference<AudioThumbnail>;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioThumbnail)
};

} // namespace yup
