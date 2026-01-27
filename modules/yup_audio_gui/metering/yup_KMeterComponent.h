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

    Example usage:
    @code
    KMeterState meterState (48000.0, 2);
    KMeterComponent leftMeter (meterState, 0);   // Channel 0
    leftMeter.setShowPeakHold (true);
    leftMeter.setBounds (0, 0, 40, 300);
    @endcode

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
    /** Creates a KMeterComponent.

        @param state     Reference to the KMeterState to read levels from
        @param channel   Channel index to display (-1 for combined/max across all channels)
        @param componentID  An optional identifier for this component
    */
    explicit KMeterComponent (KMeterState& state, int channel = -1, StringRef componentID = {});

    /** Destructor. */
    ~KMeterComponent() override;

    //==============================================================================
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

    /** Sets how the OVER counter is computed (passes through to the meter state).

        @param mode  counter mode (contiguous or total)
    */
    void setOverCounterMode (KMeterState::OverCounterMode mode);

    /** Returns the current OVER counter mode. */
    KMeterState::OverCounterMode getOverCounterMode() const noexcept { return meterState.getOverCounterMode(); }

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
    /** Sets the average integration time in seconds (passes through to the meter state).

        @param seconds  integration time in seconds
    */
    void setAverageIntegrationTime (double seconds);

    /** Sets the average fall time in seconds (passes through to the meter state).

        @param seconds  fall time in seconds
    */
    void setAverageFallTime (double seconds);

    /** Sets the peak hold time in seconds (passes through to the meter state).

        @param seconds  hold time in seconds (-1.0 = infinite)
    */
    void setPeakHoldTime (double seconds);

    //==============================================================================
    /** Callback invoked when levels change significantly.

        Assign a lambda or function to be notified of level updates:
        @code
        meter.onLevelChanged = [](float peakDb, float avgDb)
        {
            DBG ("Peak: " << peakDb << " dB, Average: " << avgDb << " dB");
        };
        @endcode
    */
    std::function<void (float peakDb, float avgDb)> onLevelChanged;

    /** Callback invoked when clipping state changes.

        @code
        meter.onClippingChanged = [](bool isClipping)
        {
            if (isClipping)
                DBG ("CLIPPING DETECTED!");
        };
        @endcode
    */
    std::function<void (bool isClipping)> onClippingChanged;

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;

    /** @internal */
    void refreshDisplay (double lastFrameTimeSeconds) override;

private:
    //==============================================================================
    friend void paintKMeter (Graphics&, const ApplicationTheme&, const KMeterComponent&);

    void updateLevels();
    void checkForCallbacks();

    //==============================================================================
    KMeterState& meterState;
    int channelIndex = -1;

    // Display state (read from KMeterState via atomics)
    Atomic<float> currentPeakDb { -100.0f };
    Atomic<float> currentAverageDb { -100.0f };
    Atomic<float> currentPeakHoldDb { -100.0f };
    Atomic<bool> currentClipping { false };

    // Configuration
    bool showPeakHold = true;
    bool showPeak = true;
    ScaleMapping scaleMapping = ScaleMapping::segmented;
    int refreshRate = 30;
    double timeSinceLastUpdate = 0.0;

    // For callbacks
    float lastCallbackPeak = -100.0f;
    float lastCallbackAverage = -100.0f;
    bool lastCallbackClipping = false;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KMeterComponent)
};

} // namespace yup
