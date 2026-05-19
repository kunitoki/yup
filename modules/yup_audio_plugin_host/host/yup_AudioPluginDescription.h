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

/**
    Stable metadata describing a discovered audio plugin.

    Instances are produced by AudioPluginScanner and consumed by
    AudioPluginFormat::loadPlugin(). Fields are format-independent.
*/
struct AudioPluginDescription
{
    /** The plugin format. */
    AudioPluginFormatType formatType = AudioPluginFormatType::unknown;

    /** Human-readable name (e.g. "Surge XT"). */
    String name;

    /** Plugin vendor / manufacturer name. */
    String vendor;

    /** Version string as reported by the plugin. */
    String version;

    /** Plugin category string (e.g. "Instrument", "Fx"). */
    String category;

    /**
        Format-specific unique identifier.
        - VST3: base64-encoded 16-byte FUID
        - CLAP: clap_plugin_descriptor::id string
        - AUv2: "type/subt/mfgr" four-char-code triplet
    */
    String identifier;

    /** Absolute path to the plugin file or bundle on disk. */
    String fileOrBundlePath;

    /** True when the plugin is a synthesiser / instrument. */
    bool isInstrument = false;

    /** True when the plugin is an audio effect. */
    bool isEffect = false;

    /** Reported input channel count (sum across main audio input buses). */
    int numInputChannels = 0;

    /** Reported output channel count (sum across main audio output buses). */
    int numOutputChannels = 0;

    /** Reported MIDI input port count. */
    int numMidiInputPorts = 0;

    /** Reported MIDI output port count. */
    int numMidiOutputPorts = 0;

    bool operator== (const AudioPluginDescription& other) const noexcept
    {
        return formatType == other.formatType
            && name == other.name
            && vendor == other.vendor
            && version == other.version
            && category == other.category
            && identifier == other.identifier
            && fileOrBundlePath == other.fileOrBundlePath
            && isInstrument == other.isInstrument
            && isEffect == other.isEffect
            && numInputChannels == other.numInputChannels
            && numOutputChannels == other.numOutputChannels
            && numMidiInputPorts == other.numMidiInputPorts
            && numMidiOutputPorts == other.numMidiOutputPorts;
    }

    bool operator!= (const AudioPluginDescription& other) const noexcept
    {
        return ! (*this == other);
    }
};

} // namespace yup
