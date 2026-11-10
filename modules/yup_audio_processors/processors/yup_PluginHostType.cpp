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

namespace
{

struct PluginHostMatcher
{
    const char* token;
    PluginHostType::Host host;
    const char* displayName;
};

// Order matters: more specific tokens must come before generic ones.
constexpr PluginHostMatcher pluginHostMatchers[] = {
    { "Bitwig", PluginHostType::Host::bitwigStudio, "Bitwig Studio" },
    { "Ableton", PluginHostType::Host::abletonLive, "Ableton Live" },
    { "Live ", PluginHostType::Host::abletonLive, "Ableton Live" },
    { "Audition", PluginHostType::Host::adobeAudition, "Adobe Audition" },
    { "GarageBand", PluginHostType::Host::appleGarageBand, "GarageBand" },
    { "MainStage", PluginHostType::Host::appleMainStage, "MainStage" },
    { "Logic", PluginHostType::Host::appleLogic, "Logic Pro" },
    { "auvaltool", PluginHostType::Host::auVal, "auval" },
    { "auval", PluginHostType::Host::auVal, "auval" },
    { "Ardour", PluginHostType::Host::ardour, "Ardour" },
    { "Mixbus", PluginHostType::Host::ardour, "Ardour" },
    { "Pro Tools", PluginHostType::Host::avidProTools, "Pro Tools" },
    { "ProTools", PluginHostType::Host::avidProTools, "Pro Tools" },
    { "Cakewalk", PluginHostType::Host::cakewalk, "Cakewalk" },
    { "SONAR", PluginHostType::Host::cakewalk, "Cakewalk" },
    { "Carla", PluginHostType::Host::carla, "Carla" },
    { "Digital Performer", PluginHostType::Host::digitalPerformer, "Digital Performer" },
    { "FL Studio", PluginHostType::Host::flStudio, "FL Studio" },
    { "OsxFL", PluginHostType::Host::flStudio, "FL Studio" },
    { "FL64", PluginHostType::Host::flStudio, "FL Studio" },
    { "pluginval", PluginHostType::Host::pluginval, "pluginval" },
    { "AudioPluginHost", PluginHostType::Host::juceAudioPluginHost, "JUCE AudioPluginHost" },
    { "Studio One", PluginHostType::Host::presonusStudioOne, "Studio One" },
    { "Studio Pro", PluginHostType::Host::presonusStudioOne, "Studio One" },
    { "REAPER", PluginHostType::Host::reaper, "REAPER" },
    { "Reason", PluginHostType::Host::reason, "Reason" },
    { "Renoise", PluginHostType::Host::renoise, "Renoise" },
    { "Redux", PluginHostType::Host::renoise, "Renoise" },
    { "Nuendo", PluginHostType::Host::steinbergNuendo, "Nuendo" },
    { "Cubase", PluginHostType::Host::steinbergCubase, "Cubase" },
    { "WaveLab", PluginHostType::Host::steinbergWavelab, "WaveLab" },
    { "Waveform", PluginHostType::Host::tracktionWaveform, "Waveform" },
    { "Tracktion", PluginHostType::Host::tracktionWaveform, "Waveform" },
    { "Vienna Ensemble Pro", PluginHostType::Host::viennaEnsemblePro, "Vienna Ensemble Pro" },
    { "YUP Plugin Host", PluginHostType::Host::yupPluginHost, "YUP Plugin Host" },
};

std::optional<PluginHostType::Host> matchPluginHost (const String& text)
{
    if (text.isEmpty())
        return std::nullopt;

    for (const auto& matcher : pluginHostMatchers)
    {
        if (text.containsIgnoreCase (matcher.token))
            return matcher.host;
    }

    return std::nullopt;
}

} // namespace

//==============================================================================

PluginHostType PluginHostType::detect (StringRef hostName, StringRef executablePath)
{
    if (const auto host = matchPluginHost (String (hostName).trim()))
        return PluginHostType (*host);

    const auto executable = File::createFileWithoutCheckingPath (executablePath);
    const auto executableName = executable.getFileName();
    if (executableName.equalsIgnoreCase ("Live"))
        return PluginHostType (Host::abletonLive);

    if (const auto host = matchPluginHost (executableName))
        return PluginHostType (*host);

    if (const auto host = matchPluginHost (executable.getParentDirectory().getFileName()))
        return PluginHostType (*host);

    for (auto bundle = executable.getParentDirectory(); bundle.getFullPathName().isNotEmpty() && bundle != bundle.getParentDirectory(); bundle = bundle.getParentDirectory())
    {
        if (! bundle.getFileExtension().equalsIgnoreCase (".app"))
            continue;

        if (const auto host = matchPluginHost (bundle.getFileNameWithoutExtension()))
            return PluginHostType (*host);

        break;
    }

    return {};
}

PluginHostType PluginHostType::detectFromCurrentProcess (StringRef hostName)
{
    return detect (hostName, File::getSpecialLocation (File::hostApplicationPath).getFullPathName());
}

StringRef PluginHostType::getHostName() const noexcept
{
    for (const auto& matcher : pluginHostMatchers)
    {
        if (matcher.host == host)
            return matcher.displayName;
    }

    return "Unknown";
}

} // namespace yup
