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
    A K-Meter component that displays audio levels according to Bob Katz's K-System.

    The KMeterComponent provides visual metering with the following features:
    - Vertical meter display (horizontal support in future phases)
    - Tri-color zones: green (safe), amber (caution 0-4dB), red (approaching clip >4dB)
    - Dual indicators: average level (filled bar) + peak level (thin line)
    - Optional peak hold marker with auto-release
    - K-20, K-14, and K-12 scale support
    - Theme-based styling with customizable colors

    The component reads levels from a KMeterState object and automatically updates
    at a configurable refresh rate (default: 30 Hz). All level reads are thread-safe
    via atomic variables in KMeterState.

    The meter asks for a repaint only when a level moved to another pixel of its scale, or at
    any change of a level when a style of its own paints it. It follows the state at every
    frame only while it shows and the levels move: once they have kept still for a second it
    looks at them ten times a second, which lets its window idle, and follows them again as
    soon as one moves.

    The levels it reads are its properties `peakDb`, `averageDb`, `peakHoldDb` and `clipping`,
    and the scale of its state gives `rangeMinDb` and `rangeMaxDb`. It tells each of them when
    it reads another value of it, so that what reads one, as a binding of a `.yui` document,
    follows it (see Component::notifyPropertyChanged).

    Example usage:
    @code
    KMeterState meterState (48000.0, 2);
    KMeterComponent leftMeter (meterState, 0);   // Channel 0
    leftMeter.setShowPeakHold (true);
    leftMeter.setBounds (0, 0, 40, 300);
    @endcode

    The state can be given after construction with setState(), which is how a meter created
    by a `.yui` document as `KMeter` gets it (see YuiDocument::setInitializer). Without a
    state the meter shows silence.

    @see KMeterState, KMeterScale, Component

    @tags{GUI, Metering}
*/
class KMeterComponent : public Component
{
public:
    //==============================================================================
    /** Style identifiers for theme customization. */
    struct Style
    {
        /** Background color of the meter. */
        static const Identifier backgroundColorId;

        /** Green zone color (safe operating range, below 0dB). */
        static const Identifier greenZoneColorId;

        /** Amber zone color (caution zone, 0dB to +4dB). */
        static const Identifier amberZoneColorId;

        /** Red zone color (approaching clip, above +4dB). */
        static const Identifier redZoneColorId;

        /** Average level indicator color (filled bar). */
        static const Identifier averageLevelColorId;

        /** Peak level indicator color (thin line, normal). */
        static const Identifier peakLevelColorId;

        /** Peak level indicator color when clipping. */
        static const Identifier peakLevelClipColorId;

        /** Peak hold marker color. */
        static const Identifier peakHoldColorId;
    };

    //==============================================================================
    /** Scale mapping modes for rendering. */
    enum class ScaleMapping
    {
        linear,   /**< Linear dB mapping for the full range. */
        segmented /**< K-System segmented mapping. */
    };

    //==============================================================================
    /** A stretch of the scale of the meter, along which the levels are placed linearly in dB.

        @see getScaleSegments
    */
    struct ScaleSegment
    {
        float topDb = 0.0f;    /**< The level at its top, in dB. */
        float bottomDb = 0.0f; /**< The level at its bottom, in dB. */
        float topY = 0.0f;     /**< Where its top is in the meter. */
        float bottomY = 0.0f;  /**< Where its bottom is in the meter. */
    };

    /** A mark of the scale of the meter, with the label naming its level when it has one.

        @see getScaleTicks
    */
    struct ScaleTick
    {
        int db = 0;                   /**< The level it marks, in dB. */
        float y = 0.0f;               /**< Where it is in the meter. */
        String label;                 /**< The text naming its level, empty for a tick without a label. */
        Rectangle<float> labelBounds; /**< Where the label goes in the meter, empty for a tick without a label. */
    };

    //==============================================================================
    /** Creates a KMeterComponent with no state, to be given one with setState().

        @param componentID  An optional identifier for this component
    */
    explicit KMeterComponent (StringRef componentID = {});

    /** Creates a KMeterComponent.

        @param state     Reference to the KMeterState to read levels from
        @param channel   Channel index to display (-1 for combined/max across all channels)
        @param componentID  An optional identifier for this component
    */
    explicit KMeterComponent (KMeterState& state, int channel = -1, StringRef componentID = {});

    /** Destructor. */
    ~KMeterComponent() override;

    //==============================================================================
    /** Changes the state the levels are read from.

        Call it on the message thread. The state must outlive the meter, or be removed
        first by setting a null one.

        @param newState  the state to read, or nullptr for none
    */
    void setState (KMeterState* newState);

    /** Returns the state the levels are read from, or nullptr if the meter has none. */
    KMeterState* getState() const noexcept { return meterState; }

    //==============================================================================
    /** Returns the static type info describing the KMeterComponent type, `KMeter` in `.yui`. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

    //==============================================================================
    /** Sets the channel of the state this meter is displaying.

        @param channel  channel index, or -1 for the loudest of all the channels
    */
    void setChannel (int channel);

    /** Returns the channel index this meter is displaying.

        @returns  channel index, or -1 for combined mode
    */
    int getChannel() const noexcept { return channelIndex; }

    /** Sets whether to show the peak hold marker.

        @param shouldShow  true to show peak hold marker
    */
    void setShowPeakHold (bool shouldShow);

    /** Returns whether the peak hold marker is shown.

        @returns  true if peak hold marker is visible
    */
    bool getShowPeakHold() const noexcept { return showPeakHold; }

    /** Sets whether to show the moving peak indicator.

        @param shouldShow  true to show peak indicator
    */
    void setShowPeak (bool shouldShow);

    /** Returns whether the moving peak indicator is shown.

        @returns  true if peak indicator is visible
    */
    bool getShowPeak() const noexcept { return showPeak; }

    /** Sets the scale mapping mode for rendering.

        @param mapping  mapping mode (linear or segmented)
    */
    void setScaleMapping (ScaleMapping mapping);

    /** Returns the scale mapping mode. */
    ScaleMapping getScaleMapping() const noexcept { return scaleMapping; }

    /** Sets how the OVER counter is computed (passes through to the meter state, ignored while the meter has none).

        @param mode  counter mode (contiguous or total)
    */
    void setOverCounterMode (KMeterState::OverCounterMode mode);

    /** Returns the current OVER counter mode, the contiguous one while the meter has no state. */
    KMeterState::OverCounterMode getOverCounterMode() const noexcept
    {
        return meterState != nullptr ? meterState->getOverCounterMode() : KMeterState::OverCounterMode::contiguous;
    }

    /** Sets the refresh rate for meter updates.

        Higher rates provide smoother animation but use more CPU.

        @param hz  refresh rate in Hz (default: 30, range: 10-60)
    */
    void setRefreshRate (int hz);

    /** Returns the current refresh rate.

        @returns  refresh rate in Hz
    */
    int getRefreshRate() const noexcept { return refreshRate; }

    //==============================================================================
    /** Returns the peak level shown, in dB. */
    float getPeakLevel() const noexcept { return levels.peakDb; }

    /** Returns the average level shown, in dB. */
    float getAverageLevel() const noexcept { return levels.averageDb; }

    /** Returns the peak level held, in dB. */
    float getPeakHoldLevel() const noexcept { return levels.peakHoldDb; }

    /** Returns whether the signal shown is clipping. */
    bool isClipping() const noexcept { return levels.clipping; }

    //==============================================================================
    /** Returns the K-System scale of the state shown, the one of K-20 while the meter has no state. */
    KMeterState::Scale getScale() const noexcept
    {
        return meterState != nullptr ? meterState->getScale() : KMeterState::Scale::k20;
    }

    /** Returns the segments of the scale, from the top of the meter to its bottom.

        The K-System mapping has one for each of its bars, which cover fewer dB toward the top
        of the scale, and the linear mapping has a single one. They are laid out again when the
        size of the meter, its mapping or the scale of its state changes. A meter with no area
        has none.

        @see getPositionForLevel
    */
    Span<const ScaleSegment> getScaleSegments() const;

    /** Returns the ticks of the scale, from the top of the meter to its bottom.

        There is one every 5 dB, or every 10 dB on a meter less than 200 high. The ones at a
        multiple of 10 dB have a label, when all of it fits the meter and it is off the labels
        kept above it and off the lowest one.
    */
    Span<const ScaleTick> getScaleTicks() const;

    /** Returns where a level is along the meter, in its own coordinates.

        @param db  the level in dB, brought inside the range of the scale

        @returns  the vertical position of the level, the bottom of the meter while it has no area
    */
    float getPositionForLevel (float db) const;

    //==============================================================================
    /** Sets the average integration time in seconds (passes through to the meter state, ignored while the meter has none).

        @param seconds  integration time in seconds
    */
    void setAverageIntegrationTime (double seconds);

    /** Sets the average fall time in seconds (passes through to the meter state, ignored while the meter has none).

        @param seconds  fall time in seconds
    */
    void setAverageFallTime (double seconds);

    /** Sets the peak hold time in seconds (passes through to the meter state, ignored while the meter has none).

        @param seconds  hold time in seconds (-1.0 = infinite)
    */
    void setPeakHoldTime (double seconds);

    //==============================================================================
    /** Callback invoked when levels change significantly.

        Assign a lambda or function to be notified of level updates:
        @code
        meter.onLevelChanged = [](float peakDb, float avgDb)
        {
            YUP_DBG ("Peak: " << peakDb << " dB, Average: " << avgDb << " dB");
        };
        @endcode
    */
    std::function<void (float peakDb, float avgDb)> onLevelChanged;

    /** Callback invoked when clipping state changes.

        @code
        meter.onClippingChanged = [](bool isClipping)
        {
            if (isClipping)
                YUP_DBG ("CLIPPING DETECTED!");
        };
        @endcode
    */
    std::function<void (bool isClipping)> onClippingChanged;

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;

    /** @internal */
    void visibilityChanged() override;

    /** @internal */
    void parentHierarchyChanged() override;

    /** @internal */
    void refreshDisplay (double lastFrameTimeSeconds);

private:
    //==============================================================================
    // What the meter shows of its state
    struct Levels
    {
        float peakDb = -100.0f;
        float averageDb = -100.0f;
        float peakHoldDb = -100.0f;
        bool clipping = false;
        KMeterState::Scale scale = KMeterState::Scale::k20;

        bool operator== (const Levels&) const = default;
    };

    // The scale as it is laid out, with what it was laid out for
    struct ScaleLayout
    {
        KMeterState::Scale scale = KMeterState::Scale::k20;
        ScaleMapping mapping = ScaleMapping::segmented;
        Rectangle<float> bounds;
        std::vector<ScaleSegment> segments;
        std::vector<ScaleTick> ticks;
    };

    bool updateLevels();
    bool showsDifferently (const Levels& previousLevels) const;
    void followLevels();
    void pollLevels();
    void showingChanged();
    void checkForCallbacks();
    void notifyLevelChanges();
    const ScaleLayout& getScaleLayout() const;

    //==============================================================================
    KMeterState* meterState = nullptr;
    int channelIndex = -1;

    // Display state, read from KMeterState when the meter updates
    Levels levels;
    mutable ScaleLayout scaleLayout;

    // Configuration
    bool showPeakHold = true;
    bool showPeak = true;
    ScaleMapping scaleMapping = ScaleMapping::segmented;
    int refreshRate = 30;
    double timeSinceLastUpdate = 0.0;
    double timeAtRest = 0.0;

    // For callbacks
    float lastCallbackPeak = -100.0f;
    float lastCallbackAverage = -100.0f;
    bool lastCallbackClipping = false;
    Levels lastNotifiedLevels;

    // The levels are followed at every frame while they move, and looked at by the timer once they keep still
    FrameAttachment frameAttachment { *this, bindFront (&KMeterComponent::refreshDisplay, this), false };
    TimedCallback restPoll { bindFront (&KMeterComponent::pollLevels, this) };

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KMeterComponent)
};

} // namespace yup
