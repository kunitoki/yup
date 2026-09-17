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

#include "../component/yup_ComponentNative.h"

namespace yup
{
namespace detail
{

class FramePacer
{
public:
    enum class EffectiveMode
    {
        off,
        software,
        presentationDriven
    };

    struct Configuration
    {
        ComponentNative::FramePacingMode requestedMode = ComponentNative::FramePacingMode::automatic;
        std::optional<uint32_t> maximumFramesInFlight;
        double targetFrameRate = 60.0;
        bool vsyncEnabled = false;
    };

    struct WaitPlan
    {
        EffectiveMode effectiveMode = EffectiveMode::off;
        std::optional<double> softwareWakeTimeSeconds;
        bool shouldWaitForFrameLatency = false;
        double targetIntervalSeconds = 1.0 / 60.0;
        double leadTimeSeconds = 0.0;
    };

    struct RefreshDelta
    {
        double seconds = 0.0;
        bool usedPresentationTiming = false;
    };

    struct Diagnostics
    {
        EffectiveMode effectiveMode = EffectiveMode::off;
        double measuredCpuRenderDurationSeconds = 0.0;
        double measuredSubmitDurationSeconds = 0.0;
        double measuredPresentBlockDurationSeconds = 0.0;
        double schedulerLatenessSeconds = 0.0;
        uint64_t missedDeadlines = 0;
        bool usingPresentationTiming = false;
    };

    FramePacer() noexcept = default;

    FramePacer (Configuration newConfiguration,
                GraphicsContext::FrameTimingCapabilities newCapabilities) noexcept
    {
        configure (newConfiguration, newCapabilities);
    }

    void configure (Configuration newConfiguration,
                    GraphicsContext::FrameTimingCapabilities newCapabilities) noexcept
    {
        configuration = newConfiguration;
        capabilities = newCapabilities;
        diagnostics.effectiveMode = resolveEffectiveMode();
    }

    void reset (double nowSeconds) noexcept
    {
        lastRefreshTimeSeconds = nowSeconds;
        nextFrameDeadlineSeconds = nowSeconds + getTargetIntervalSeconds();
        lastPresentationTimeSeconds.reset();
        lastPresentedDeltaSeconds.reset();
        lastPresentationCount = 0;
        diagnostics.schedulerLatenessSeconds = 0.0;
        diagnostics.usingPresentationTiming = false;
    }

    WaitPlan planWait (double nowSeconds) noexcept
    {
        auto effectiveMode = resolveEffectiveMode();
        diagnostics.effectiveMode = effectiveMode;

        if (! nextFrameDeadlineSeconds.has_value())
            nextFrameDeadlineSeconds = nowSeconds + getTargetIntervalSeconds();

        WaitPlan plan;
        plan.effectiveMode = effectiveMode;
        plan.targetIntervalSeconds = getTargetIntervalSeconds();
        plan.leadTimeSeconds = leadTimeSeconds;

        if (effectiveMode == EffectiveMode::presentationDriven && capabilities.hasFrameLatencyWait)
            plan.shouldWaitForFrameLatency = true;

        if (effectiveMode == EffectiveMode::software)
        {
            const double wakeTimeSeconds = *nextFrameDeadlineSeconds - leadTimeSeconds;
            plan.softwareWakeTimeSeconds = wakeTimeSeconds;
            diagnostics.schedulerLatenessSeconds = jmax (0.0, nowSeconds - wakeTimeSeconds);
        }

        return plan;
    }

    RefreshDelta makeRefreshDelta (double nowSeconds,
                                   const GraphicsContext::FrameTimingInfo& timingInfo) noexcept
    {
        absorbPresentationTiming (timingInfo);

        RefreshDelta result;

        if (lastPresentedDeltaSeconds.has_value())
        {
            result.seconds = *lastPresentedDeltaSeconds;
            result.usedPresentationTiming = true;
            diagnostics.usingPresentationTiming = true;
        }
        else
        {
            const double previousTimeSeconds = lastRefreshTimeSeconds.value_or (nowSeconds);
            result.seconds = clampDelta (nowSeconds - previousTimeSeconds);
            diagnostics.usingPresentationTiming = false;
        }

        lastRefreshTimeSeconds = nowSeconds;
        return result;
    }

    void recordFrame (double frameCompletedAtSeconds,
                      double cpuRenderDurationSeconds,
                      double submitDurationSeconds,
                      double presentBlockDurationSeconds) noexcept
    {
        diagnostics.measuredCpuRenderDurationSeconds = jmax (0.0, cpuRenderDurationSeconds);
        diagnostics.measuredSubmitDurationSeconds = jmax (0.0, submitDurationSeconds);
        diagnostics.measuredPresentBlockDurationSeconds = jmax (0.0, presentBlockDurationSeconds);

        const double measuredLeadTimeSeconds = diagnostics.measuredCpuRenderDurationSeconds
                                             + diagnostics.measuredSubmitDurationSeconds
                                             + 0.001;

        updateLeadTime (measuredLeadTimeSeconds);
        advanceDeadline (frameCompletedAtSeconds);
    }

    EffectiveMode getEffectiveMode() const noexcept
    {
        return diagnostics.effectiveMode;
    }

    std::optional<uint32_t> getMaximumFramesInFlight() const noexcept
    {
        return configuration.maximumFramesInFlight;
    }

    const Diagnostics& getDiagnostics() const noexcept
    {
        return diagnostics;
    }

private:
    static constexpr double minimumFrameRate = 1.0;
    static constexpr double maximumFrameRate = 1000.0;

    double getTargetIntervalSeconds() const noexcept
    {
        return 1.0 / jlimit (minimumFrameRate, maximumFrameRate, configuration.targetFrameRate);
    }

    EffectiveMode resolveEffectiveMode() const noexcept
    {
        const auto requestedMode = configuration.requestedMode;

        if (requestedMode == ComponentNative::FramePacingMode::off)
            return EffectiveMode::off;

        const bool hasPresentationDrivenSupport = capabilities.hasPresentationTiming
                                               || capabilities.hasFrameLatencyWait
                                               || capabilities.hasMaximumFramesInFlight;

        if (requestedMode == ComponentNative::FramePacingMode::presentationDriven)
            return hasPresentationDrivenSupport ? EffectiveMode::presentationDriven
                                                : (configuration.vsyncEnabled ? EffectiveMode::off
                                                                              : EffectiveMode::software);

        if (requestedMode == ComponentNative::FramePacingMode::software)
            return configuration.vsyncEnabled ? EffectiveMode::off : EffectiveMode::software;

        if (capabilities.hasPresentationTiming)
            return EffectiveMode::presentationDriven;

        return configuration.vsyncEnabled ? EffectiveMode::off : EffectiveMode::software;
    }

    double clampDelta (double deltaSeconds) const noexcept
    {
        const double maximumDeltaSeconds = jmax (0.25, getTargetIntervalSeconds() * 4.0);
        return jlimit (0.0, maximumDeltaSeconds, deltaSeconds);
    }

    void absorbPresentationTiming (const GraphicsContext::FrameTimingInfo& timingInfo) noexcept
    {
        if (! timingInfo.hasPresentationTimestamp || timingInfo.isDisjoint || timingInfo.presentationCount == 0)
            return;

        if (timingInfo.presentationCount == lastPresentationCount)
            return;

        double deltaSeconds = timingInfo.presentationIntervalSeconds;

        if (deltaSeconds <= 0.0 && lastPresentationTimeSeconds.has_value())
            deltaSeconds = timingInfo.presentedAtSeconds - *lastPresentationTimeSeconds;

        if (deltaSeconds <= 0.0 && lastRefreshTimeSeconds.has_value())
            deltaSeconds = timingInfo.presentedAtSeconds - *lastRefreshTimeSeconds;

        if (deltaSeconds > 0.0)
            lastPresentedDeltaSeconds = clampDelta (deltaSeconds);
        else
            lastPresentedDeltaSeconds.reset();

        lastPresentationTimeSeconds = timingInfo.presentedAtSeconds;
        lastPresentationCount = timingInfo.presentationCount;
    }

    void updateLeadTime (double measuredLeadTimeSeconds) noexcept
    {
        const double maximumLeadTimeSeconds = getTargetIntervalSeconds() * 0.75;
        const double clampedLeadTimeSeconds = jlimit (0.0, maximumLeadTimeSeconds, measuredLeadTimeSeconds);
        const double smoothing = clampedLeadTimeSeconds > leadTimeSeconds ? 0.35 : 0.1;

        leadTimeSeconds += (clampedLeadTimeSeconds - leadTimeSeconds) * smoothing;
        leadTimeSeconds = jlimit (0.0, maximumLeadTimeSeconds, leadTimeSeconds);
    }

    void advanceDeadline (double frameCompletedAtSeconds) noexcept
    {
        const double targetIntervalSeconds = getTargetIntervalSeconds();

        if (! nextFrameDeadlineSeconds.has_value())
        {
            nextFrameDeadlineSeconds = frameCompletedAtSeconds + targetIntervalSeconds;
            return;
        }

        *nextFrameDeadlineSeconds += targetIntervalSeconds;

        if (*nextFrameDeadlineSeconds <= frameCompletedAtSeconds)
        {
            const auto missed = static_cast<uint64_t> ((frameCompletedAtSeconds - *nextFrameDeadlineSeconds) / targetIntervalSeconds) + 1;
            *nextFrameDeadlineSeconds += static_cast<double> (missed) * targetIntervalSeconds;
            diagnostics.missedDeadlines += missed;
        }
    }

    Configuration configuration;
    GraphicsContext::FrameTimingCapabilities capabilities;
    Diagnostics diagnostics;
    std::optional<double> nextFrameDeadlineSeconds;
    std::optional<double> lastRefreshTimeSeconds;
    std::optional<double> lastPresentationTimeSeconds;
    std::optional<double> lastPresentedDeltaSeconds;
    double leadTimeSeconds = 0.0;
    uint64_t lastPresentationCount = 0;
};

} // namespace detail
} // namespace yup
