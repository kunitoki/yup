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

#pragma once

namespace yup
{

//==============================================================================
/**
    View component that renders an AudioThumbnail with zooming, scrolling, and progress display.
*/
class YUP_API AudioViewComponent : public Component
    , public Timer
    , private AudioThumbnail::Listener
{
public:
    //==============================================================================
    /** Creates a view with an owned AudioThumbnail. */
    AudioViewComponent();

    /** Creates a view for an externally owned AudioThumbnail. */
    explicit AudioViewComponent (AudioThumbnail& thumbnailToUse);

    /** Destructor. */
    ~AudioViewComponent() override;

    //==============================================================================
    /** Returns the thumbnail used for rendering. */
    AudioThumbnail& getThumbnail() noexcept;

    /** Returns the thumbnail used for rendering. */
    const AudioThumbnail& getThumbnail() const noexcept;

    //==============================================================================
    /** Assigns the buffer to render and refreshes the peak cache. */
    void setAudioBuffer (const AudioBuffer<float>* newBuffer, double newSampleRate = 0.0);

    /** Assigns an audio file to render and refreshes the peak cache. */
    void setAudioFile (const File& file, AudioFormatManager* managerToUse = nullptr);

    /** Clears the waveform display and cache. */
    void clear();

    /** Returns the currently assigned audio buffer. */
    const AudioBuffer<float>* getAudioBuffer() const noexcept;

    /** Returns the currently assigned audio file (may be empty). */
    const File& getAudioFile() const noexcept;

    /** Returns true if the thumbnail is using an audio file source. */
    bool isUsingAudioFile() const noexcept;

    //==============================================================================
    /** Sets the zoom factor.

        A zoom factor of 1.0 fits the whole buffer in view. Higher values zoom in.
    */
    void setZoomFactor (double newZoomFactor);

    /** Returns the current zoom factor. */
    double getZoomFactor() const noexcept { return zoomFactor; }

    /** Sets the visible sample range. */
    void setViewRangeSamples (Range<double> newRange);

    /** Returns the currently visible sample range. */
    Range<double> getViewRangeSamples() const noexcept { return viewRangeSamples; }

    /** Scrolls to a new start sample using the current view length. */
    void scrollToSample (double newStartSample);

    //==============================================================================
    /** Sets the channel label width in pixels. */
    void setLabelWidth (int newLabelWidth);

    /** Returns the channel label width in pixels. */
    int getLabelWidth() const noexcept { return labelWidth; }

    /** Shows or hides the channel labels. */
    void setChannelLabelsVisible (bool shouldShow) noexcept;

    /** Returns true if the channel labels are visible. */
    bool isChannelLabelsVisible() const noexcept { return showChannelLabels; }

    /** Enables mouse/keyboard interaction for zooming and scrolling. */
    void setSelectable (bool shouldBeSelectable) noexcept;

    /** Returns true if mouse/keyboard interaction is enabled. */
    bool isSelectable() const noexcept { return selectable; }

    //==============================================================================
    /** Returns the horizontal scrollbar used for scrolling. */
    ScrollBar* getHorizontalScrollBar() const noexcept { return horizontalScrollBar.get(); }

    /** Returns the progress bar used during loading/analysis. */
    ProgressBar* getProgressBar() const noexcept { return progressBar.get(); }

    //==============================================================================
    /** Returns the total sample count of the assigned source. */
    int getTotalSamples() const noexcept;

    /** Returns the total channel count of the assigned source. */
    int getNumChannels() const noexcept;

    /** Returns the sample rate associated with the buffer. */
    double getSampleRate() const noexcept;

    /** Converts a time in seconds to a sample position. */
    double timeToSample (double seconds) const noexcept;

    /** Converts a sample position to time in seconds. */
    double sampleToTime (double sample) const noexcept;

    /** Converts a sample position to an X coordinate within the waveform bounds. */
    float sampleToX (double sample, const Rectangle<float>& waveformBounds) const noexcept;

    /** Converts an X coordinate within the waveform bounds to a sample position. */
    double xToSample (float x, const Rectangle<float>& waveformBounds) const noexcept;

    /** Returns the bounds of the waveform area (excluding labels and scrollbar). */
    Rectangle<float> getWaveformBounds() const;

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void timerCallback() override;
    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) override;
    /** @internal */
    void keyDown (const KeyPress& keys, const Point<float>& position) override;

protected:
    //==============================================================================
    /** Paints the component background. */
    virtual void paintBackground (Graphics& g, const Rectangle<float>& bounds);

    /** Paints the placeholder when no waveform data is available. */
    virtual void paintPlaceholder (Graphics& g, const Rectangle<float>& bounds);

    /** Paints overlays (playheads, markers) over the waveform. */
    virtual void paintOverlay (Graphics& g, const Rectangle<float>& waveformBounds);

    /** Returns the channel label to show for a given channel index. */
    virtual String getChannelLabel (int channelIndex) const;

private:
    void attachThumbnail (AudioThumbnail& thumbnailToUse);
    void updateLayout();
    void updateScrollBar();
    void handleScrollBarMoved();
    void setViewRangeSamplesInternal (Range<double> range, bool notifyScrollBar);
    void rebuildPeakProfileIfNeeded();
    void schedulePeakProfileUpdate (int samplesPerPeak);
    void scrollBySamples (double deltaSamples);
    void zoomAroundSample (double zoomMultiplier, double anchorSample);
    void ensureViewRangeIsValid();
    void updateProgressBar (double progress, bool isVisible);

    void thumbnailChanged (AudioThumbnail& thumbnail) override;
    void thumbnailProgressChanged (AudioThumbnail& thumbnail, double progress, bool isVisible) override;

    AudioThumbnail* thumbnail = nullptr;
    std::unique_ptr<AudioThumbnail> ownedThumbnail;
    std::unique_ptr<ScrollBar> horizontalScrollBar;
    std::unique_ptr<ProgressBar> progressBar;
    bool ignoreScrollBarCallback = false;
    double zoomFactor = 1.0;
    Range<double> viewRangeSamples;
    int labelWidth = 48;
    bool showChannelLabels = true;
    bool selectable = false;
    int lastWaveformWidth = 0;
    bool pendingRebuild = false;
    Time lastResizeTime;
    int pendingSamplesPerPeak = 0;
    int lastRequestedSamplesPerPeak = 0;

    static constexpr int progressBarHeight = 6;
    static constexpr int rebuildDebounceMs = 120;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioViewComponent)
};

} // namespace yup
