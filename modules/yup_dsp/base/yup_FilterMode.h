/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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
    Filter mode flag types for type-safe filter mode specification.

    Used with yup::FlagSet to create composite filter modes while maintaining
    type safety and allowing filters to specify exactly which modes they support.
*/
namespace FilterModeFlags
{
struct lowpass;     /**< Low-pass filter */
struct highpass;    /**< High-pass filter */
struct bandpassCsg; /**< Band-pass filter (constant skirt gain, peak gain = Q) */
struct bandpassCpg; /**< Band-pass filter (constant peak gain = 0dB) */
struct bandstop;    /**< Band-stop (notch) filter */
struct peak;        /**< Peaking filter */
struct lowshelf;    /**< Low-shelf filter */
struct highshelf;   /**< High-shelf filter */
struct allpass;     /**< All-pass filter */
} // namespace FilterModeFlags

/**
    Type-safe filter mode using FlagSet.

    Allows creation of composite modes like `bandpass = bandpassCsg | bandpassCpg`
    while maintaining type safety and enabling compile-time capability checking.
*/
using FilterModeType = FlagSet<uint32_t,
                               FilterModeFlags::lowpass,
                               FilterModeFlags::highpass,
                               FilterModeFlags::bandpassCsg,
                               FilterModeFlags::bandpassCpg,
                               FilterModeFlags::bandstop,
                               FilterModeFlags::peak,
                               FilterModeFlags::lowshelf,
                               FilterModeFlags::highshelf,
                               FilterModeFlags::allpass>;

//==============================================================================
/** Pre-defined filter modes for convenience */
namespace FilterMode
{
static inline constexpr auto lowpass = FilterModeType::declareValue<FilterModeFlags::lowpass>();
static inline constexpr auto highpass = FilterModeType::declareValue<FilterModeFlags::highpass>();
static inline constexpr auto bandpassCsg = FilterModeType::declareValue<FilterModeFlags::bandpassCsg>();
static inline constexpr auto bandpassCpg = FilterModeType::declareValue<FilterModeFlags::bandpassCpg>();
static inline constexpr auto bandstop = FilterModeType::declareValue<FilterModeFlags::bandstop>();
static inline constexpr auto peak = FilterModeType::declareValue<FilterModeFlags::peak>();
static inline constexpr auto lowshelf = FilterModeType::declareValue<FilterModeFlags::lowshelf>();
static inline constexpr auto highshelf = FilterModeType::declareValue<FilterModeFlags::highshelf>();
static inline constexpr auto allpass = FilterModeType::declareValue<FilterModeFlags::allpass>();

/** Composite modes */
static inline constexpr auto bandpass = bandpassCsg | bandpassCpg; /**< Any band-pass filter variant */
} // namespace FilterMode

//==============================================================================
/**
    Resolves a composite filter mode to the best supported variant for a specific filter.

    @param requestedMode    The mode requested (could be composite like 'bandpass')
    @param supportedModes   The modes actually supported by the filter
    @returns               The resolved specific mode, or empty FilterMode if none supported
*/
constexpr FilterModeType resolveFilterMode (FilterModeType requestedMode, FilterModeType supportedModes) noexcept
{
    // If the exact mode is supported, use it
    if (supportedModes.test (requestedMode))
        return requestedMode;

    // Handle composite mode resolution
    if (requestedMode.test (FilterMode::bandpass))
    {
        // Priority order: CSG first, then CPG
        if (supportedModes.test (FilterMode::bandpassCsg))
            return FilterMode::bandpassCsg;

        else if (supportedModes.test (FilterMode::bandpassCpg))
            return FilterMode::bandpassCpg;
    }

    // Could add more composite mode logic here in the future
    // e.g., if we had FilterMode::shelf = lowshelf | highshelf

    // No supported variant found
    return FilterMode::lowpass; // Empty/null mode
}

} // namespace yup
