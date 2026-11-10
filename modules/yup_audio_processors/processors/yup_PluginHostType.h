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
    Identifies the application that is hosting a plugin binary.

    Plugin wrappers detect the host when they are loaded, from the name reported
    by the plugin API (VST3 IHostApplication, CLAP host name, AU host bundle) with a
    fallback to the host executable name. Use it only to work around known host
    quirks; prefer capability queries when a format offers them.

    @code
        if (AudioProcessor::getHostType().isBitwigStudio())
            ...
    @endcode

    @see AudioProcessor::getHostType
*/
class YUP_API PluginHostType
{
public:
    //==============================================================================
    /** Known host application families. */
    enum class Host
    {
        unknown,
        abletonLive,
        adobeAudition,
        appleGarageBand,
        appleLogic,
        appleMainStage,
        ardour,
        auVal,
        avidProTools,
        bitwigStudio,
        cakewalk,
        carla,
        digitalPerformer,
        flStudio,
        juceAudioPluginHost,
        pluginval,
        presonusStudioOne,
        reaper,
        reason,
        renoise,
        steinbergCubase,
        steinbergNuendo,
        steinbergWavelab,
        tracktionWaveform,
        viennaEnsemblePro,
        yupPluginHost
    };

    //==============================================================================
    /** Creates an unknown host type. */
    PluginHostType() = default;

    /** Creates a host type for a specific host. */
    explicit PluginHostType (Host host) noexcept
        : host (host)
    {
    }

    /** Detects the host from a host-reported name and the host executable path.

        Either string may be empty. The host name is checked first, then the executable.
    */
    static PluginHostType detect (StringRef hostName, StringRef executablePath);

    /** Detects the host from a host-reported name, falling back to the current executable. */
    static PluginHostType detectFromCurrentProcess (StringRef hostName = {});

    //==============================================================================
    /** Returns the detected host family. */
    Host getHost() const noexcept { return host; }

    /** Returns a human-readable name for the detected host family. */
    StringRef getHostName() const noexcept;

    //==============================================================================
    bool isUnknown() const noexcept { return host == Host::unknown; }
    bool isAbletonLive() const noexcept { return host == Host::abletonLive; }
    bool isAppleLogic() const noexcept { return host == Host::appleLogic; }
    bool isAppleGarageBand() const noexcept { return host == Host::appleGarageBand; }
    bool isArdour() const noexcept { return host == Host::ardour; }
    bool isBitwigStudio() const noexcept { return host == Host::bitwigStudio; }
    bool isCubaseOrNuendo() const noexcept { return host == Host::steinbergCubase || host == Host::steinbergNuendo; }
    bool isFLStudio() const noexcept { return host == Host::flStudio; }
    bool isProTools() const noexcept { return host == Host::avidProTools; }
    bool isReaper() const noexcept { return host == Host::reaper; }
    bool isRenoise() const noexcept { return host == Host::renoise; }
    bool isStudioOne() const noexcept { return host == Host::presonusStudioOne; }
    bool isWavelab() const noexcept { return host == Host::steinbergWavelab; }

    /** Returns true for hosts that only validate or inspect plugins (auval, pluginval...). */
    bool isValidator() const noexcept { return host == Host::auVal || host == Host::pluginval; }

    //==============================================================================
    bool operator== (const PluginHostType& other) const noexcept = default;

private:
    Host host = Host::unknown;
};

} // namespace yup
