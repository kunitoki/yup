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

#include "RetraceMCPServer.h"
#include "RetraceAudioAnalysis.h"
#include "RetraceAudioCapture.h"
#include "RetraceAudioRender.h"
#include "../RetraceProcessor.h"
#include "../dsp/SynthEffectsParameters.h"
#include "../dsp/SynthTypes.h"
#include "../ui/SynthPresetLibrary.h"

#include <yup_gui/yup_gui.h>
#include <yup_audio_formats/yup_audio_formats.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <vector>

namespace
{
constexpr yup::int64 maxInlineAudioBytes = 4 * 1024 * 1024;
constexpr double maxLiveSeconds = 60.0;

//==============================================================================
struct Registry
{
    yup::CriticalSection lock;
    std::vector<RetraceMCPServer*> servers;
};

Registry& getRegistry()
{
    static Registry registry;
    return registry;
}

//==============================================================================
yup::var makeObject()
{
    return yup::var (std::make_unique<yup::DynamicObject>());
}

void set (yup::var& object, const yup::Identifier& name, const yup::var& value)
{
    if (auto* dynamicObject = object.getDynamicObject())
        dynamicObject->setProperty (name, value);
}

bool isSet (const yup::var& arguments, const yup::Identifier& name)
{
    const auto* object = arguments.getDynamicObject();
    return object != nullptr && object->hasProperty (name) && ! object->getProperty (name).isVoid();
}

yup::MCPToolDefinition makeTool (const yup::String& name, const yup::String& description, const yup::String& properties, const yup::String& required = {})
{
    auto schema = yup::JSON::parse ("{\"type\":\"object\",\"properties\":{" + properties + "}"
                                    + (required.isNotEmpty() ? ",\"required\":[" + required + "]" : yup::String())
                                    + "}");
    jassert (schema.isObject()); // A tool schema in this file is not valid JSON

    return { name, description, schema };
}

const yup::String noteProperties = R"(
    "notes": { "type": "array", "description": "The notes to play.", "items": { "type": "object", "properties": {
        "note": { "type": "integer", "minimum": 0, "maximum": 127, "description": "MIDI note number, 60 is middle C." },
        "velocity": { "type": "number", "minimum": 0, "maximum": 1, "default": 0.8 },
        "start": { "type": "number", "minimum": 0, "description": "Seconds from the start." },
        "duration": { "type": "number", "minimum": 0, "default": 1, "description": "Seconds the key is held." },
        "channel": { "type": "integer", "minimum": 1, "maximum": 16, "default": 1 } }, "required": ["note"] } },
    "automation": { "type": "array", "description": "Parameter moves while the notes play.", "items": { "type": "object", "properties": {
        "id": { "type": "string", "description": "The parameter ID." },
        "time": { "type": "number", "minimum": 0, "description": "Seconds from the start." },
        "value": { "type": ["number", "string"], "description": "A plain value, a normalized one when normalized is true, or a choice name." },
        "normalized": { "type": "boolean", "default": false } }, "required": ["id", "time", "value"] } },
    "tail": { "type": "number", "minimum": 0, "default": 2, "description": "Seconds kept after the last note ends." },
    "length": { "type": "number", "minimum": 0, "description": "Total seconds, overriding the tail." })";

const yup::String audioOutputProperties = R"(
    "path": { "type": "string", "description": "An absolute path to write the WAV file to." },
    "inline": { "type": "boolean", "description": "Returns the WAV as audio content. Defaults to true without a path, for files up to 4 MB." },
    "bitsPerSample": { "type": "integer", "enum": [16, 24, 32], "default": 24 })";

//==============================================================================
yup::String getSection (const yup::AudioParameter& parameter)
{
    if (const auto path = parameter.getModulePath(); path.isNotEmpty())
        return path;

    return parameter.getID().upToFirstOccurrenceOf (".", false, false);
}

yup::var describeParameter (const yup::AudioParameter& parameter, bool isRouteDestination)
{
    auto object = makeObject();
    set (object, "id", parameter.getID());
    set (object, "name", parameter.getName());
    set (object, "section", getSection (parameter));
    set (object, "min", parameter.getMinimumValue());
    set (object, "max", parameter.getMaximumValue());
    set (object, "default", parameter.getDefaultValue());
    set (object, "value", parameter.getValue());
    set (object, "normalized", parameter.getNormalizedValue());
    set (object, "text", parameter.toString());

    if (const auto unit = parameter.getUnitName(); unit.isNotEmpty())
        set (object, "unit", unit);

    if (const auto steps = parameter.getNumSteps(); steps > 0)
    {
        set (object, "steps", steps);

        if (parameter.isEnum() && steps <= 128)
        {
            yup::Array<yup::var> choices;

            for (int step = 0; step <= steps; ++step)
                choices.add (parameter.convertToString (parameter.convertToDenormalizedValue (static_cast<float> (step) / static_cast<float> (steps))));

            set (object, "choices", choices);
        }
    }

    set (object, "automatable", parameter.isAutomatable());
    set (object, "modulatable", isRouteDestination);
    return object;
}

yup::var describeValue (const yup::AudioParameter& parameter)
{
    auto object = makeObject();
    set (object, "value", parameter.getValue());
    set (object, "normalized", parameter.getNormalizedValue());
    set (object, "text", parameter.toString());
    return object;
}

/** Reads a plain value, a normalized one or a choice name for a parameter, or NaN. */
float readValue (const yup::AudioParameter& parameter, const yup::var& value, bool normalized)
{
    if (value.isString())
        return parameter.convertFromString (value.toString());

    if (! (value.isDouble() || value.isInt() || value.isInt64() || value.isBool()))
        return std::numeric_limits<float>::quiet_NaN();

    const auto number = static_cast<float> (static_cast<double> (value));
    return normalized ? parameter.convertToDenormalizedValue (yup::jlimit (0.0f, 1.0f, number)) : number;
}

yup::var describeInfo (const SynthPatch::PresetInfo& info)
{
    auto object = makeObject();
    set (object, "name", info.name);
    set (object, "category", info.category);
    set (object, "author", info.author);
    set (object, "bank", info.bank);
    set (object, "tags", info.tags);
    set (object, "description", info.description);
    set (object, "notes", info.notes);
    set (object, "id", info.id);

    yup::Array<yup::var> macros;

    for (int macro = 0; macro < SynthExample::macroCount; ++macro)
        macros.add (info.getMacroName (macro));

    set (object, "macroNames", macros);
    return object;
}

/** Overwrites the fields of a description that the arguments hold. */
SynthPatch::PresetInfo updateInfo (SynthPatch::PresetInfo info, const yup::var& arguments)
{
    const std::pair<const char*, yup::String*> fields[] { { "name", &info.name }, { "category", &info.category }, { "author", &info.author }, { "bank", &info.bank },
                                                          { "tags", &info.tags }, { "description", &info.description }, { "notes", &info.notes } };

    for (const auto& [name, field] : fields)
        if (isSet (arguments, name))
            *field = arguments[name].toString();

    if (const auto* macros = arguments["macroNames"].getArray())
        for (int macro = 0; macro < yup::jmin (SynthExample::macroCount, macros->size()); ++macro)
            info.macroNames[static_cast<std::size_t> (macro)] = macros->getReference (macro).toString();

    return info;
}

yup::var describeEntry (const PresetEntry& entry)
{
    auto object = makeObject();
    set (object, "key", entry.key);
    set (object, "name", entry.name);
    set (object, "category", entry.category);
    set (object, "author", entry.author);
    set (object, "bank", entry.bank);
    set (object, "tags", entry.tags.joinIntoString (", "));
    set (object, "location", entry.location);

    if (entry.isFactory())
        set (object, "factoryIndex", entry.factoryIndex);
    else
        set (object, "file", entry.file.getFullPathName());

    return object;
}

//==============================================================================
/** Reads a patch given as a JSON object, JSON text or preset XML text. */
yup::DataTree parsePatch (const yup::var& patch, const yup::Identifier& defaultType)
{
    auto json = patch;

    if (patch.isString())
    {
        const auto text = patch.toString().trim();

        if (text.startsWithChar ('<'))
        {
            const auto xml = yup::parseXML (text);
            return xml != nullptr ? yup::DataTree::fromXml (*xml) : yup::DataTree();
        }

        json = yup::JSON::parse (text);
    }

    if (auto* object = json.getDynamicObject(); object != nullptr && ! object->hasProperty ("type"))
    {
        json = json.clone();
        set (json, "type", defaultType.toString());
    }

    return yup::DataTree::fromJson (json);
}

/** Lays the properties and children of a partial patch over a whole one; a child replaces
    the child of the same type. */
yup::DataTree mergePatch (yup::DataTree base, const yup::DataTree& changes)
{
    {
        auto transaction = base.beginTransaction();

        for (int index = 0; index < changes.getNumProperties(); ++index)
        {
            const auto name = changes.getPropertyName (index);
            transaction.setProperty (name, changes.getProperty (name));
        }

        for (const auto& child : changes)
        {
            if (const auto existing = base.getChildWithName (child.getType()); existing.isValid())
                transaction.removeChild (existing);

            transaction.addChild (child.clone());
        }
    }

    return base;
}

//==============================================================================
yup::MemoryBlock encodeWav (const yup::AudioBuffer<float>& audio, double sampleRate, int bitsPerSample)
{
    yup::MemoryBlock block;
    auto stream = std::make_unique<yup::MemoryOutputStream> (block, false);
    yup::WaveAudioFormat format;
    auto writer = format.createWriterFor (stream.get(), sampleRate, audio.getNumChannels(), bitsPerSample, {}, 0);

    if (writer == nullptr)
        return {};

    stream.release(); // The writer owns it now
    const auto written = writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
    writer.reset(); // Finishes the header and the block

    return written ? block : yup::MemoryBlock();
}

yup::File createOutputFile (const yup::String& prefix, const yup::String& extension)
{
    const auto name = prefix + "-" + yup::Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S") + extension;

    return yup::File::getSpecialLocation (yup::File::tempDirectory)
        .getChildFile ("Retrace MCP")
        .getChildFile (name)
        .getNonexistentSibling();
}

yup::Result writeOutputFile (const yup::File& file, const yup::MemoryBlock& data)
{
    if (! file.getParentDirectory().createDirectory() || ! file.replaceWithData (data.getData(), data.getSize()))
        return yup::Result::fail ("Cannot write " + file.getFullPathName());

    return yup::Result::ok();
}

/** Encodes audio and returns it as the arguments ask: a file, inline audio content or
    both, with its analysis. */
yup::MCPToolResult makeAudioResult (const yup::AudioBuffer<float>& audio, double sampleRate, const yup::var& arguments, yup::var details, const yup::String& prefix)
{
    const auto requestedBits = static_cast<int> (arguments.getProperty ("bitsPerSample", 24));
    const auto bits = requestedBits == 16 || requestedBits == 32 ? requestedBits : 24;
    const auto wav = encodeWav (audio, sampleRate, bits);

    if (wav.isEmpty())
        return yup::MCPToolResult::fromError ("Cannot encode the audio as WAV");

    auto path = arguments["path"].toString();
    const auto fitsInline = static_cast<yup::int64> (wav.getSize()) <= maxInlineAudioBytes;
    const auto wantsInline = isSet (arguments, "inline") ? static_cast<bool> (arguments["inline"]) : path.isEmpty();
    const auto returnsInline = wantsInline && fitsInline;

    if (wantsInline && ! fitsInline)
        set (details, "note", "The audio is larger than 4 MB, so it was written to a file instead of returned inline");

    if (path.isEmpty() && ! returnsInline)
        path = createOutputFile (prefix, ".wav").getFullPathName();

    if (path.isNotEmpty())
    {
        if (! yup::File::isAbsolutePath (path))
            return yup::MCPToolResult::fromError ("The path must be absolute: " + path);

        if (const auto result = writeOutputFile (yup::File (path), wav); result.failed())
            return yup::MCPToolResult::fromError (result.getErrorMessage());

        set (details, "path", path);
    }

    set (details, "bytes", static_cast<yup::int64> (wav.getSize()));
    set (details, "analysis", RetraceAudioAnalysis::analyse (audio, sampleRate));

    auto result = yup::MCPToolResult::fromJson (details);

    if (returnsInline)
        result.content.push_back (yup::MCPContent::makeAudio (wav, "audio/wav"));

    return result;
}

//==============================================================================
/** Reads the notes and automation of play_notes and render_audio. */
yup::ResultValue<RetraceAudioRender::Request> parseRequest (const yup::var& arguments, const RetraceProcessor& processor)
{
    RetraceAudioRender::Request request;

    if (const auto* notes = arguments["notes"].getArray())
    {
        for (const auto& note : *notes)
        {
            if (! note.isObject())
                return yup::makeResultValueFail ("Every note must be an object");

            RetraceAudioRender::Note parsed;
            parsed.note = static_cast<int> (note.getProperty ("note", 60));
            parsed.velocity = static_cast<float> (static_cast<double> (note.getProperty ("velocity", 0.8)));
            parsed.start = static_cast<double> (note.getProperty ("start", 0.0));
            parsed.duration = static_cast<double> (note.getProperty ("duration", 1.0));
            parsed.channel = static_cast<int> (note.getProperty ("channel", 1));
            request.notes.push_back (parsed);
        }
    }

    if (const auto* automation = arguments["automation"].getArray())
    {
        for (const auto& point : *automation)
        {
            const auto id = point.getProperty ("id", {}).toString();
            const auto parameter = processor.getParameterByID (id);

            if (parameter == nullptr)
                return yup::makeResultValueFail ("No parameter " + id);

            const auto value = readValue (*parameter, point.getProperty ("value", {}), static_cast<bool> (point.getProperty ("normalized", false)));

            if (! std::isfinite (value))
                return yup::makeResultValueFail ("Invalid value for " + id);

            request.automation.push_back ({ parameter->getIndexInContainer(),
                                            static_cast<double> (point.getProperty ("time", 0.0)),
                                            parameter->convertToNormalizedValue (value) });
        }
    }

    request.length = static_cast<double> (arguments.getProperty ("length", 0.0));
    request.tail = static_cast<double> (arguments.getProperty ("tail", 2.0));
    return yup::makeResultValueOk (std::move (request));
}

//==============================================================================
yup::String describePatchFormat()
{
    yup::String text;
    text << "A Retrace patch is one DataTree, exchanged as JSON {\"type\", \"properties\", \"children\"} "
            "(or as the XML of preset files). Read it with get_patch, edit it and apply it with set_patch.\n\n"
            "Root: type \"RetraceState\". Properties:\n"
            "- one per parameter, keyed by its ID, holding its plain value (see list_parameters). A missing one loads the "
            "parameter's default when the patch replaces the current one.\n"
            "- version: keep it as get_patch returns it. preset: the factory preset index, or -1.\n"
            "- name, bank, author, tags (comma separated), category, description, notes, presetId, macro1Name to macro"
         << SynthExample::macroCount << "Name.\n\n"
                                        "Children (a missing one loads its default):\n"
                                        "- Partials1 to Partials"
         << SynthExample::oscillatorCount << ": the drawn harmonics of an oscillator. custom: true plays the drawn series "
                                             "instead of the waveform. cosines and sines: "
         << SynthExample::editableHarmonics << " space separated coefficients, for harmonics 1 upwards, each within -16 and 16.\n"
                                               "- Curves: children Envelope and LFO (property index, 0 to "
         << SynthExample::envelopeCount - 1 << ") and Shaper. points: \"time level curve\" triples separated by \";\", "
                                               "time in seconds (0 to 100, ascending), at least two points; sustain: a point index or -1; "
                                               "loopStart and loopEnd: point indexes or -1. Envelope 0 drives the amplitude and ends at level 0.\n"
                                               "- Routes: children Route {slot 0 to "
         << SynthExample::modulationSlots - 1 << ", source, destination}. destination is the ID of a parameter listed as "
                                                 "modulatable; the depth is the parameter route.<slot + 1>.depth. Sources: "
         << getSynthModulationSourceNames().joinIntoString (", ") << ".\n"
                                                                     "- EffectChain: children Effect {name} in processing order. Effects: "
         << getSynthEffectNames().joinIntoString (", ") << ".\n"
                                                           "- Tuning: name, preset, scale (Scala .scl text), keyMap (.kbm text), tuning (AnaMark .tun text), "
                                                           "bendDown and bendUp (0 to 48), bendMode (0 bends in semitones, 1 in scale steps).\n\n"
                                                           "set_patch with mode \"merge\" lays the given properties over the current patch and replaces "
                                                           "only the children it holds, so {\"properties\": {\"filter.1.cutoff\": 800}} is a whole patch edit.";
    return text;
}
} // namespace

//==============================================================================
RetraceMCPServer::RetraceMCPServer (RetraceProcessor& processorToControl)
    : processor (processorToControl)
    , server ([]
    {
        yup::MCPServer::Options options;
        options.serverName = "Retrace";
        options.serverVersion = YupPlugin_Version;
        options.instructions = "Controls one instance of the Retrace synthesizer. "
                               "Start with get_synth_info and list_parameters. "
                               "To design a sound, read get_patch and describe_patch_format, then use set_parameters for "
                               "single controls or set_patch for whole patches, and listen with render_audio, which plays notes "
                               "offline and returns a WAV and an analysis. play_notes plays through the host in real time and can "
                               "capture the result. save_preset stores the patch in the user's library and screenshot shows the editor.";
        return options;
    }())
{
    registerInfoTools();
    registerPatchTools();
    registerPresetTools();
    registerPlayTools();
    registerAudioTools();
    registerEditorTools();
    registerResources();

    auto& registry = getRegistry();
    const yup::CriticalSection::ScopedLockType lock (registry.lock);
    registry.servers.push_back (this);
}

RetraceMCPServer::~RetraceMCPServer()
{
    {
        auto& registry = getRegistry();
        const yup::CriticalSection::ScopedLockType lock (registry.lock);
        registry.servers.erase (std::remove (registry.servers.begin(), registry.servers.end(), this), registry.servers.end());
    }

    setEnabled (false);
}

yup::Result RetraceMCPServer::setEnabled (bool shouldBeEnabled)
{
    if (shouldBeEnabled == isEnabled())
        return yup::Result::ok();

    if (! shouldBeEnabled)
    {
        // Abandons the hops still queued, so the server threads stop waiting for them.
        session->store (0);
        server.stop();
        transport = nullptr;
        return yup::Result::ok();
    }

    yup::MCPHttpServerTransport::Options options;
    options.port = basePort;
    options.numPortsToTry = numPortsToTry;

    auto newTransport = std::make_unique<yup::MCPHttpServerTransport> (options);
    auto* started = newTransport.get();

    session->store (++lastSession);

    if (auto result = server.start (std::move (newTransport)); result.failed())
    {
        session->store (0);
        return result;
    }

    transport = started;
    return yup::Result::ok();
}

yup::String RetraceMCPServer::getUrl() const
{
    return transport != nullptr ? transport->getUrl() : yup::String();
}

void RetraceMCPServer::setEnabledEverywhere (bool shouldBeEnabled)
{
    std::vector<RetraceMCPServer*> servers;

    {
        auto& registry = getRegistry();
        const yup::CriticalSection::ScopedLockType lock (registry.lock);
        servers = registry.servers;
    }

    for (auto* server : servers)
        server->setEnabled (shouldBeEnabled);
}

//==============================================================================
bool RetraceMCPServer::runOnMessageThread (std::function<void()> work, int timeoutMs)
{
    if (yup::MessageManager::existsAndIsCurrentThread())
    {
        work();
        return true;
    }

    const auto expected = session->load();

    if (expected == 0)
        return false;

    auto done = std::make_shared<yup::WaitableEvent>();

    const auto posted = yup::MessageManager::callAsync ([currentSession = session, expected, done, work = std::move (work)]
    {
        if (currentSession->load() == expected)
            work();

        done->signal();
    });

    if (! posted)
        return false;

    const auto deadline = yup::Time::getMillisecondCounter() + static_cast<yup::uint32> (timeoutMs);

    while (! done->wait (50.0))
    {
        if (session->load() != expected || yup::Time::getMillisecondCounter() > deadline)
            return false;
    }

    return true;
}

template <typename T>
std::optional<T> RetraceMCPServer::callOnMessageThread (std::function<T()> work)
{
    // Shared with the hop, which may run after a timeout gave up on it.
    auto result = std::make_shared<std::optional<T>>();

    if (! runOnMessageThread ([result, work = std::move (work)] { *result = work(); }))
        return std::nullopt;

    return std::move (*result);
}

yup::MCPToolResult RetraceMCPServer::onMessageThread (std::function<yup::MCPToolResult()> work)
{
    if (auto result = callOnMessageThread<yup::MCPToolResult> (std::move (work)))
        return std::move (*result);

    return yup::MCPToolResult::fromError ("The synth did not answer in time: its user interface may be busy, or the server is stopping");
}

//==============================================================================
void RetraceMCPServer::registerInfoTools()
{
    server.registerTool (makeTool ("get_synth_info", "Returns the state of this Retrace instance: version, host, audio settings, current preset, voices and whether the editor is open.", {}),
                         [this] (const yup::var&)
    {
        return onMessageThread ([this]
        {
            auto info = makeObject();
            set (info, "product", "Retrace");
            set (info, "description", "A four operator spectral FM synthesizer with drawn partials, " + yup::String (SynthExample::envelopeCount) + " envelopes, " + yup::String (SynthExample::lfoCount) + " LFOs, " + yup::String (SynthExample::modulationSlots) + " modulation routes and a master effects chain.");
            set (info, "version", YupPlugin_Version);
            set (info, "wrapper", yup::String (yup::AudioProcessor::getWrapperTypeName (yup::AudioProcessor::getWrapperType())));
            set (info, "url", getUrl());
            set (info, "sampleRate", processor.getSampleRate());
            set (info, "latencySamples", processor.getLatencySamples());
            set (info, "oversampling", processor.getOversamplingFactor());
            set (info, "voices", processor.getNumVoices());
            set (info, "activeVoices", processor.getNumActiveVoices());
            set (info, "cpuPercent", std::round (processor.getLoadPercentage() * 10.0) / 10.0);
            set (info, "numParameters", static_cast<int> (processor.getParameters().size()));
            set (info, "factoryPresetIndex", processor.getCurrentPreset());
            set (info, "presetKey", processor.getCurrentPresetKey());
            set (info, "preset", describeInfo (processor.getPresetInfo()));
            set (info, "theme", processor.getThemeName());
            set (info, "editorOpen", processor.getEditorComponent() != nullptr);
            set (info, "capturing", processor.getAudioCapture().isCapturing());
            return yup::MCPToolResult::fromJson (info);
        });
    });

    server.registerTool (makeTool ("list_parameters", "Lists the parameters with their ID, section, range, default, current value, unit, choices and whether modulation routes can reach them.",
                                   R"("filter": { "type": "string", "description": "Only parameters whose ID, name or section contains this text, ignoring case." })"),
                         [this] (const yup::var& arguments)
    {
        const auto filter = arguments["filter"].toString();

        return onMessageThread ([this, filter]
        {
            const auto parameters = processor.getParameters();
            yup::Array<yup::var> list;

            for (const auto& parameter : parameters)
            {
                if (filter.isNotEmpty()
                    && ! parameter->getID().containsIgnoreCase (filter)
                    && ! parameter->getName().containsIgnoreCase (filter)
                    && ! getSection (*parameter).containsIgnoreCase (filter))
                    continue;

                list.add (describeParameter (*parameter, SynthPatch::findRouteDestination (parameters, parameter->getID()) >= 0));
            }

            auto result = makeObject();
            set (result, "count", list.size());
            set (result, "parameters", list);
            return yup::MCPToolResult::fromJson (result);
        });
    });

    server.registerTool (makeTool ("get_parameters", "Returns the plain value, normalized value and display text of parameters.",
                                   R"("ids": { "type": "array", "items": { "type": "string" }, "description": "The parameter IDs, or all parameters when missing." })"),
                         [this] (const yup::var& arguments)
    {
        const auto ids = arguments["ids"];

        return onMessageThread ([this, ids]
        {
            auto values = makeObject();
            yup::Array<yup::var> unknown;

            if (const auto* list = ids.getArray())
            {
                for (const auto& id : *list)
                {
                    if (const auto parameter = processor.getParameterByID (id.toString()))
                        set (values, parameter->getID(), describeValue (*parameter));
                    else
                        unknown.add (id);
                }
            }
            else
            {
                for (const auto& parameter : processor.getParameters())
                    set (values, parameter->getID(), describeValue (*parameter));
            }

            auto result = makeObject();
            set (result, "values", values);

            if (! unknown.isEmpty())
                set (result, "unknown", unknown);

            return yup::MCPToolResult::fromJson (result);
        });
    });

    server.registerTool (makeTool ("set_parameters", "Sets parameters live, as moving their controls would: the host records them as automation gestures. Values are plain (in the parameter's range) unless normalized is true; choices also take their names.",
                                   R"("values": { "type": "object", "additionalProperties": { "type": ["number", "string", "boolean"] }, "description": "Parameter ID to value." },
                                      "normalized": { "type": "boolean", "default": false, "description": "Values are 0 to 1 across each parameter's range." })",
                                   R"("values")"),
                         [this] (const yup::var& arguments)
    {
        const auto values = arguments["values"];
        const auto normalized = static_cast<bool> (arguments.getProperty ("normalized", false));

        if (values.getDynamicObject() == nullptr)
            return yup::MCPToolResult::fromError ("values must be an object of parameter IDs to values");

        return onMessageThread ([this, values, normalized]
        {
            auto applied = makeObject();
            yup::Array<yup::var> rejected;

            for (const auto& property : values.getDynamicObject()->getProperties())
            {
                const auto id = property.name.toString();
                const auto parameter = processor.getParameterByID (id);

                if (parameter == nullptr || parameter->isReadOnly())
                {
                    rejected.add (id + (parameter == nullptr ? ": no such parameter" : ": read only"));
                    continue;
                }

                const auto value = readValue (*parameter, property.value, normalized);

                if (! std::isfinite (value))
                {
                    rejected.add (id + ": invalid value");
                    continue;
                }

                parameter->beginChangeGesture();
                parameter->setValueNotifyingHost (value);
                parameter->endChangeGesture();
                set (applied, id, describeValue (*parameter));
            }

            auto result = makeObject();
            set (result, "applied", applied);

            if (! rejected.isEmpty())
                set (result, "rejected", rejected);

            auto toolResult = yup::MCPToolResult::fromJson (result);
            toolResult.isError = applied.getDynamicObject()->getProperties().isEmpty() && ! rejected.isEmpty();
            return toolResult;
        });
    });
}

//==============================================================================
void RetraceMCPServer::registerPatchTools()
{
    server.registerTool (makeTool ("get_patch", "Returns the whole current patch, the preset itself: every parameter, the drawn partials, curves, modulation routes, effects order, tuning and description. See describe_patch_format.",
                                   R"("format": { "type": "string", "enum": ["json", "xml"], "default": "json", "description": "xml is the preset file format." })"),
                         [this] (const yup::var& arguments)
    {
        const auto asXml = arguments["format"].toString() == "xml";

        return onMessageThread ([this, asXml]
        {
            const auto patch = processor.createPatch();

            if (! asXml)
                return yup::MCPToolResult::fromJson (patch.createJson());

            const auto xml = patch.createXml();
            return xml != nullptr ? yup::MCPToolResult::fromText (xml->toString()) : yup::MCPToolResult::fromError ("Cannot write the patch as XML");
        });
    });

    server.registerTool (makeTool ("set_patch", "Applies a patch, as loading a preset does. replace loads a whole patch, like get_patch returns; merge lays a partial one over the current patch. Clears the editor's undo history.",
                                   R"("patch": { "type": ["object", "string"], "description": "The patch as a JSON object, JSON text or preset XML text." },
                                      "mode": { "type": "string", "enum": ["replace", "merge"], "default": "merge" })",
                                   R"("patch")"),
                         [this] (const yup::var& arguments)
    {
        const auto patch = arguments["patch"];
        const auto merge = arguments["mode"].toString() != "replace";

        return onMessageThread ([this, patch, merge]
        {
            auto current = processor.createPatch();
            const auto changes = parsePatch (patch, current.getType());

            if (! changes.isValid())
                return yup::MCPToolResult::fromError ("The patch is not valid JSON or XML");

            if (const auto result = processor.applyPatch (merge ? mergePatch (current, changes) : changes); result.failed())
                return yup::MCPToolResult::fromError (result.getErrorMessage());

            auto result = makeObject();
            set (result, "applied", true);
            set (result, "preset", describeInfo (processor.getPresetInfo()));
            return yup::MCPToolResult::fromJson (result);
        });
    });

    server.registerTool (makeTool ("describe_patch_format", "Explains the patch format of get_patch and set_patch: the children holding partials, curves, routes, effects order and tuning.", {}),
                         [] (const yup::var&)
    {
        return yup::MCPToolResult::fromText (describePatchFormat());
    });

    server.registerTool (makeTool ("randomize_patch", "Replaces the patch with a random one (full), or moves the current sound a little keeping its structure (slight).",
                                   R"("amount": { "type": "string", "enum": ["full", "slight"], "default": "full" })"),
                         [this] (const yup::var& arguments)
    {
        const auto slight = arguments["amount"].toString() == "slight";

        return onMessageThread ([this, slight]
        {
            if (slight)
                processor.randomizeSlightly();
            else
                processor.randomize();

            return yup::MCPToolResult::fromJson (processor.createPatch().createJson());
        });
    });
}

//==============================================================================
void RetraceMCPServer::registerPresetTools()
{
    server.registerTool (makeTool ("list_presets", "Lists the factory and user presets the browser shows.",
                                   R"("query": { "type": "string", "description": "Only presets whose name, category, tags, author or bank contain this text, ignoring case." },
                                      "source": { "type": "string", "enum": ["all", "factory", "user"], "default": "all" },
                                      "limit": { "type": "integer", "minimum": 1, "default": 200 })"),
                         [this] (const yup::var& arguments)
    {
        const auto query = arguments["query"].toString();
        const auto source = arguments["source"].toString();
        const auto limit = yup::jmax (1, static_cast<int> (arguments.getProperty ("limit", 200)));

        return onMessageThread ([this, query, source, limit]
        {
            auto& library = processor.getPresetLibrary().getLibrary();
            yup::Array<yup::var> list;
            int matches = 0;

            for (const auto& entry : library.getEntries())
            {
                if ((source == "factory" && ! entry.isFactory()) || (source == "user" && entry.isFactory()))
                    continue;

                if (query.isNotEmpty()
                    && ! entry.name.containsIgnoreCase (query)
                    && ! entry.category.containsIgnoreCase (query)
                    && ! entry.tags.joinIntoString (" ").containsIgnoreCase (query)
                    && ! entry.author.containsIgnoreCase (query)
                    && ! entry.bank.containsIgnoreCase (query))
                    continue;

                if (++matches <= limit)
                    list.add (describeEntry (entry));
            }

            auto result = makeObject();
            set (result, "count", matches);
            set (result, "presets", list);
            set (result, "scanning", library.isScanning());
            return yup::MCPToolResult::fromJson (result);
        });
    });

    server.registerTool (makeTool ("load_preset", "Loads a preset listed by list_presets, by key, or a factory preset by index.",
                                   R"("key": { "type": "string" }, "factoryIndex": { "type": "integer", "minimum": 0 })"),
                         [this] (const yup::var& arguments)
    {
        const auto key = arguments["key"].toString();
        const auto factoryIndex = static_cast<int> (arguments.getProperty ("factoryIndex", -1));

        return onMessageThread ([this, key, factoryIndex]
        {
            PresetEntry factory;
            const PresetEntry* entry = nullptr;

            if (key.isNotEmpty())
            {
                entry = processor.getPresetLibrary().getLibrary().findEntry (key);
            }
            else if (factoryIndex >= 0)
            {
                factory.factoryIndex = factoryIndex;
                entry = &factory;
            }

            if (entry == nullptr)
                return yup::MCPToolResult::fromError ("No preset " + (key.isNotEmpty() ? key : yup::String (factoryIndex)));

            if (const auto result = processor.loadPresetEntry (*entry); result.failed())
                return yup::MCPToolResult::fromError (result.getErrorMessage());

            return yup::MCPToolResult::fromJson (describeInfo (processor.getPresetInfo()));
        });
    });

    server.registerTool (makeTool ("set_preset_info", "Changes the description stored with the current patch; fields left out keep their value.",
                                   R"("name": { "type": "string" }, "category": { "type": "string", "description": "The one main kind of sound, like Bass or Pad." },
                                      "author": { "type": "string" }, "bank": { "type": "string" }, "tags": { "type": "string", "description": "Comma separated." },
                                      "description": { "type": "string" }, "notes": { "type": "string" },
                                      "macroNames": { "type": "array", "items": { "type": "string" }, "maxItems": 8 })"),
                         [this] (const yup::var& arguments)
    {
        return onMessageThread ([this, arguments]
        {
            processor.setPresetInfo (updateInfo (processor.getPresetInfo(), arguments));
            return yup::MCPToolResult::fromJson (describeInfo (processor.getPresetInfo()));
        });
    });

    server.registerTool (makeTool ("save_preset", "Saves the current patch as a user preset file, in the user preset folder unless a path is given, and shows it in the browser. Description fields given are stored with it.",
                                   R"("name": { "type": "string", "description": "The preset and file name." },
                                      "path": { "type": "string", "description": "An absolute file path, overriding the user preset folder." },
                                      "category": { "type": "string" }, "author": { "type": "string" }, "bank": { "type": "string" },
                                      "tags": { "type": "string" }, "description": { "type": "string" }, "notes": { "type": "string" },
                                      "overwrite": { "type": "boolean", "default": false })",
                                   R"("name")"),
                         [this] (const yup::var& arguments)
    {
        return onMessageThread ([this, arguments]
        {
            auto& library = processor.getPresetLibrary();
            const auto extension = library.getLibrary().getFormat().extension;
            const auto name = yup::File::createLegalFileName (arguments["name"].toString().trim());

            if (name.isEmpty())
                return yup::MCPToolResult::fromError ("The preset needs a name");

            auto file = SynthPresetLibrary::getUserFolder().getChildFile (name + extension);

            if (const auto path = arguments["path"].toString(); path.isNotEmpty())
            {
                if (! yup::File::isAbsolutePath (path))
                    return yup::MCPToolResult::fromError ("The path must be absolute: " + path);

                file = yup::File (path).withFileExtension (extension);
            }

            if (file.existsAsFile() && ! static_cast<bool> (arguments.getProperty ("overwrite", false)))
                return yup::MCPToolResult::fromError ("A preset already exists at " + file.getFullPathName() + ", pass overwrite to replace it");

            processor.setPresetInfo (updateInfo (processor.getPresetInfo(), arguments));

            if (! file.getParentDirectory().createDirectory())
                return yup::MCPToolResult::fromError ("Cannot create " + file.getParentDirectory().getFullPathName());

            if (const auto result = processor.savePresetFile (file); result.failed())
                return yup::MCPToolResult::fromError (result.getErrorMessage());

            library.getLibrary().rescan();

            auto result = describeInfo (processor.getPresetInfo());
            set (result, "file", file.getFullPathName());
            return yup::MCPToolResult::fromJson (result);
        });
    });
}

//==============================================================================
void RetraceMCPServer::registerPlayTools()
{
    server.registerTool (makeTool ("note_on", "Presses a key, as the on-screen keyboard does. It sounds until note_off or all_notes_off. The host must be running audio.",
                                   R"("note": { "type": "integer", "minimum": 0, "maximum": 127, "description": "MIDI note number, 60 is middle C." },
                                      "velocity": { "type": "number", "minimum": 0, "maximum": 1, "default": 0.8 },
                                      "channel": { "type": "integer", "minimum": 1, "maximum": 16, "default": 1 })",
                                   R"("note")"),
                         [this] (const yup::var& arguments)
    {
        const auto note = yup::jlimit (0, 127, static_cast<int> (arguments.getProperty ("note", 60)));
        const auto channel = yup::jlimit (1, 16, static_cast<int> (arguments.getProperty ("channel", 1)));
        const auto velocity = yup::jlimit (0.0f, 1.0f, static_cast<float> (static_cast<double> (arguments.getProperty ("velocity", 0.8))));

        processor.getKeyboardState().noteOn (channel, note, velocity);
        return yup::MCPToolResult::fromText ("Note " + yup::String (note) + " on");
    });

    server.registerTool (makeTool ("note_off", "Releases a key pressed with note_on.",
                                   R"("note": { "type": "integer", "minimum": 0, "maximum": 127 },
                                      "channel": { "type": "integer", "minimum": 1, "maximum": 16, "default": 1 })",
                                   R"("note")"),
                         [this] (const yup::var& arguments)
    {
        const auto note = yup::jlimit (0, 127, static_cast<int> (arguments.getProperty ("note", 60)));
        const auto channel = yup::jlimit (1, 16, static_cast<int> (arguments.getProperty ("channel", 1)));

        processor.getKeyboardState().noteOff (channel, note, 0.0f);
        return yup::MCPToolResult::fromText ("Note " + yup::String (note) + " off");
    });

    server.registerTool (makeTool ("all_notes_off", "Releases every key and every sounding voice.", {}),
                         [this] (const yup::var&)
    {
        processor.getKeyboardState().allNotesOff (0);
        processor.requestAllNotesOff();
        return yup::MCPToolResult::fromText ("All notes off");
    });

    server.registerTool (makeTool ("send_midi", "Moves a performance control: a controller (the mod wheel is 1), the pitch wheel or channel aftertouch.",
                                   R"("type": { "type": "string", "enum": ["controller", "pitchBend", "aftertouch"] },
                                      "controller": { "type": "integer", "minimum": 0, "maximum": 127, "default": 1 },
                                      "value": { "type": "number", "description": "0 to 127, or -1 to 1 for the pitch wheel." },
                                      "channel": { "type": "integer", "minimum": 1, "maximum": 16, "default": 1 })",
                                   R"("type", "value")"),
                         [this] (const yup::var& arguments)
    {
        const auto type = arguments["type"].toString();
        const auto value = static_cast<double> (arguments.getProperty ("value", 0.0));
        const auto channel = yup::jlimit (1, 16, static_cast<int> (arguments.getProperty ("channel", 1)));

        yup::MidiMessage message;

        if (type == "controller")
            message = yup::MidiMessage::controllerEvent (channel, yup::jlimit (0, 127, static_cast<int> (arguments.getProperty ("controller", 1))), yup::jlimit (0, 127, yup::roundToInt (value)));
        else if (type == "pitchBend")
            message = yup::MidiMessage::pitchWheel (channel, yup::jlimit (0, 16383, yup::roundToInt ((yup::jlimit (-1.0, 1.0, value) + 1.0) * 8192.0)));
        else if (type == "aftertouch")
            message = yup::MidiMessage::channelPressureChange (channel, yup::jlimit (0, 127, yup::roundToInt (value)));
        else
            return yup::MCPToolResult::fromError ("Unknown type " + type);

        return onMessageThread ([this, message]
        {
            return processor.sendMidiFromEditor (message) ? yup::MCPToolResult::fromText ("Sent " + message.getDescription())
                                                          : yup::MCPToolResult::fromError ("The MIDI queue is full");
        });
    });

    server.registerTool (makeTool ("play_notes", "Plays notes and parameter moves through the host in real time, up to 60 seconds, and returns when done. With capture, records the output and returns it as WAV with an analysis. The host must be running audio; render_audio works without it.",
                                   noteProperties + R"(, "capture": { "type": "boolean", "default": false }, )" + audioOutputProperties,
                                   R"("notes")"),
                         [this] (const yup::var& arguments)
    {
        return playNotes (arguments);
    });
}

yup::MCPToolResult RetraceMCPServer::playNotes (const yup::var& arguments)
{
    auto parsed = parseRequest (arguments, processor);

    if (parsed.failed())
        return yup::MCPToolResult::fromError (parsed.getErrorMessage());

    const auto& request = parsed.getReference();
    const auto length = RetraceAudioRender::getLength (request);

    if (length > maxLiveSeconds)
        return yup::MCPToolResult::fromError ("play_notes plays up to 60 seconds, use render_audio for longer");

    const auto capture = static_cast<bool> (arguments.getProperty ("capture", false));
    auto& audioCapture = processor.getAudioCapture();

    if (capture && ! audioCapture.start (processor.getSampleRate(), length + 0.25))
        return yup::MCPToolResult::fromError ("Cannot capture: another capture is running, or the host has not started audio");

    struct Event
    {
        double time = 0.0;
        int note = -1; // or the automation index
        bool isOn = false;
        int channel = 1;
        float value = 0.0f;
    };

    std::vector<Event> events;

    for (const auto& note : request.notes)
    {
        const auto number = yup::jlimit (0, 127, note.note);
        const auto channel = yup::jlimit (1, 16, note.channel);
        events.push_back ({ yup::jmax (0.0, note.start), number, true, channel, yup::jlimit (0.0f, 1.0f, note.velocity) });
        events.push_back ({ yup::jmax (0.0, note.start) + yup::jmax (0.001, note.duration), number, false, channel, 0.0f });
    }

    std::vector<std::pair<double, RetraceAudioRender::Automation>> moves;

    for (const auto& point : request.automation)
        moves.emplace_back (point.time, point);

    std::stable_sort (events.begin(), events.end(), [] (const auto& a, const auto& b) { return a.time != b.time ? a.time < b.time : (! a.isOn && b.isOn); });
    std::stable_sort (moves.begin(), moves.end(), [] (const auto& a, const auto& b) { return a.first < b.first; });

    const auto expected = session->load();
    const auto startMs = yup::Time::getMillisecondCounterHiRes();
    auto& keyboard = processor.getKeyboardState();
    auto nextEvent = events.begin();
    auto nextMove = moves.begin();
    auto stopped = false;

    while (! stopped)
    {
        const auto elapsed = (yup::Time::getMillisecondCounterHiRes() - startMs) / 1000.0;

        for (; nextEvent != events.end() && nextEvent->time <= elapsed; ++nextEvent)
        {
            if (nextEvent->isOn)
                keyboard.noteOn (nextEvent->channel, nextEvent->note, nextEvent->value);
            else
                keyboard.noteOff (nextEvent->channel, nextEvent->note, 0.0f);
        }

        for (; nextMove != moves.end() && nextMove->first <= elapsed; ++nextMove)
        {
            const auto move = nextMove->second;

            yup::MessageManager::callAsync ([currentSession = session, expected, move, this]
            {
                if (currentSession->load() != expected)
                    return;

                if (const auto parameter = processor.getParameters()[static_cast<std::size_t> (move.parameterIndex)])
                {
                    parameter->beginChangeGesture();
                    parameter->setValueNotifyingHost (parameter->convertToDenormalizedValue (move.normalizedValue));
                    parameter->endChangeGesture();
                }
            });
        }

        if (elapsed >= length)
            break;

        stopped = session->load() != expected;

        if (! stopped)
            yup::Thread::sleep (2);
    }

    // Releases what a stop interrupted.
    for (; nextEvent != events.end(); ++nextEvent)
        if (! nextEvent->isOn)
            keyboard.noteOff (nextEvent->channel, nextEvent->note, 0.0f);

    if (! capture)
    {
        auto result = makeObject();
        set (result, "played", static_cast<int> (request.notes.size()));
        set (result, "duration", length);
        set (result, "stopped", stopped);
        return yup::MCPToolResult::fromJson (result);
    }

    const auto audio = audioCapture.stop();

    if (stopped)
        return yup::MCPToolResult::fromError ("The server stopped while playing");

    auto details = makeObject();
    set (details, "played", static_cast<int> (request.notes.size()));
    return makeAudioResult (audio, audioCapture.getSampleRate(), arguments, details, "play");
}

//==============================================================================
void RetraceMCPServer::registerAudioTools()
{
    server.registerTool (makeTool ("start_capture", "Starts recording the synth's output as the host plays it, for up to maxSeconds. Stop it with stop_capture.",
                                   R"("maxSeconds": { "type": "number", "minimum": 0.1, "maximum": 600, "default": 30 })"),
                         [this] (const yup::var& arguments)
    {
        const auto maxSeconds = yup::jlimit (0.1, 600.0, static_cast<double> (arguments.getProperty ("maxSeconds", 30.0)));

        if (! processor.getAudioCapture().start (processor.getSampleRate(), maxSeconds))
            return yup::MCPToolResult::fromError ("Cannot capture: another capture is running, or the host has not started audio");

        return yup::MCPToolResult::fromText ("Capturing up to " + yup::String (maxSeconds) + " seconds");
    });

    server.registerTool (makeTool ("stop_capture", "Stops the capture started by start_capture and returns the recording as WAV with an analysis.",
                                   audioOutputProperties),
                         [this] (const yup::var& arguments)
    {
        auto& audioCapture = processor.getAudioCapture();

        if (! audioCapture.isCapturing())
            return yup::MCPToolResult::fromError ("No capture is running");

        const auto full = audioCapture.isFull();
        const auto audio = audioCapture.stop();

        auto details = makeObject();
        set (details, "truncated", full);
        return makeAudioResult (audio, audioCapture.getSampleRate(), arguments, details, "capture");
    });

    server.registerTool (makeTool ("render_audio", "Renders notes and parameter automation offline through a private copy of the synth, faster than real time and without the host, and returns a WAV with an analysis. Uses the current patch, or a patch merged over it.",
                                   noteProperties + R"(,
                                      "sampleRate": { "type": "number", "description": "Defaults to the host's, or 48000." },
                                      "patch": { "type": ["object", "string"], "description": "A patch merged over the current one for this render only, as set_patch takes." }, )"
                                       + audioOutputProperties,
                                   R"("notes")"),
                         [this] (const yup::var& arguments)
    {
        return renderAudio (arguments);
    });
}

yup::MCPToolResult RetraceMCPServer::renderAudio (const yup::var& arguments)
{
    auto parsed = parseRequest (arguments, processor);

    if (parsed.failed())
        return yup::MCPToolResult::fromError (parsed.getErrorMessage());

    auto request = parsed.getReference();
    const auto hostRate = processor.getSampleRate();
    request.sampleRate = static_cast<double> (arguments.getProperty ("sampleRate", hostRate > 0.0 ? hostRate : 48000.0));

    struct Prepared
    {
        std::shared_ptr<RetraceProcessor> renderer;
        yup::String error;
    };

    // The copy is built and destroyed on the message thread, as patches load there; it
    // renders here, so the user interface keeps running.
    const auto patch = arguments["patch"];
    auto prepared = callOnMessageThread<Prepared> ([this, patch]
    {
        auto current = processor.createPatch();
        auto state = current;

        if (! patch.isVoid())
        {
            const auto changes = parsePatch (patch, current.getType());

            if (! changes.isValid())
                return Prepared { nullptr, "The patch is not valid JSON or XML" };

            state = mergePatch (current, changes);
        }

        auto renderer = std::make_shared<RetraceProcessor> (RetraceProcessor::Role::renderer);

        if (const auto result = renderer->applyPatch (state); result.failed())
            return Prepared { nullptr, result.getErrorMessage() };

        return Prepared { renderer, {} };
    });

    if (! prepared.has_value())
        return yup::MCPToolResult::fromError ("The synth did not answer in time: its user interface may be busy, or the server is stopping");

    if (prepared->renderer == nullptr)
        return yup::MCPToolResult::fromError (prepared->error);

    const auto expected = session->load();
    const auto rendered = RetraceAudioRender::render (*prepared->renderer, request, [currentSession = session, expected]
    {
        return currentSession->load() != expected;
    });

    yup::MessageManager::callAsync ([renderer = std::move (prepared->renderer)] {});

    if (rendered.failed())
        return yup::MCPToolResult::fromError (rendered.getErrorMessage());

    auto details = makeObject();
    set (details, "notes", static_cast<int> (request.notes.size()));
    return makeAudioResult (rendered.getReference(), request.sampleRate, arguments, details, "render");
}

//==============================================================================
void RetraceMCPServer::registerEditorTools()
{
    server.registerTool (makeTool ("screenshot", "Takes a PNG screenshot of the open editor. Fails when no editor is open.",
                                   R"("includeEffects": { "type": "boolean", "default": true, "description": "Includes the CRT post effect." },
                                      "path": { "type": "string", "description": "An absolute path to also write the PNG to." })"),
                         [this] (const yup::var& arguments)
    {
        return onMessageThread ([this, arguments]
        {
            return takeScreenshot (arguments);
        });
    });
}

yup::MCPToolResult RetraceMCPServer::takeScreenshot (const yup::var& arguments)
{
    auto* editor = processor.getEditorComponent();

    if (editor == nullptr || ! editor->isShowing())
        return yup::MCPToolResult::fromError ("No editor is open: open the Retrace window first");

    auto* native = editor->getNativeComponent();
    auto* context = native != nullptr ? native->getGraphicsContext() : nullptr;

    if (context == nullptr)
        return yup::MCPToolResult::fromError ("The editor has no graphics context to draw with");

    const auto image = editor->snapshotToImage (*context, static_cast<bool> (arguments.getProperty ("includeEffects", true)));

    if (! image.isValid())
        return yup::MCPToolResult::fromError ("The editor could not be drawn, it may be minimized");

    yup::MemoryBlock png;

    {
        yup::PngImageFormatWriter writer (new yup::MemoryOutputStream (png, false), image.getPixelFormat());

        if (! writer.writeImage (image))
            return yup::MCPToolResult::fromError ("Cannot encode the screenshot as PNG");
    }

    auto details = makeObject();
    set (details, "width", image.getWidth());
    set (details, "height", image.getHeight());

    if (const auto path = arguments["path"].toString(); path.isNotEmpty())
    {
        if (! yup::File::isAbsolutePath (path))
            return yup::MCPToolResult::fromError ("The path must be absolute: " + path);

        if (const auto result = writeOutputFile (yup::File (path), png); result.failed())
            return yup::MCPToolResult::fromError (result.getErrorMessage());

        set (details, "path", path);
    }

    auto result = yup::MCPToolResult::fromJson (details);
    result.content.push_back (yup::MCPContent::makeImage (png, "image/png"));
    return result;
}

//==============================================================================
void RetraceMCPServer::registerResources()
{
    const auto readOnMessageThread = [this] (std::function<yup::String()> read)
    {
        return [this, read]
        {
            return callOnMessageThread<yup::String> (read).value_or (yup::String());
        };
    };

    server.registerResource ({ "retrace://patch/current", "Current patch", "The whole current patch as get_patch returns it.", "application/json" },
                             readOnMessageThread ([this] { return yup::JSON::toString (processor.createPatch().createJson()); }));

    server.registerResource ({ "retrace://parameters", "Parameters", "Every parameter as list_parameters returns it.", "application/json" },
                             readOnMessageThread ([this]
    {
        const auto parameters = processor.getParameters();
        yup::Array<yup::var> list;

        for (const auto& parameter : parameters)
            list.add (describeParameter (*parameter, SynthPatch::findRouteDestination (parameters, parameter->getID()) >= 0));

        return yup::JSON::toString (list);
    }));

    server.registerResource ({ "retrace://patch/format", "Patch format", "How patches are laid out, for writing them.", "text/plain" },
                             [] { return describePatchFormat(); });

    server.registerResource ({ "retrace://presets", "Presets", "Every factory and user preset.", "application/json" },
                             readOnMessageThread ([this]
    {
        yup::Array<yup::var> list;

        for (const auto& entry : processor.getPresetLibrary().getLibrary().getEntries())
            list.add (describeEntry (entry));

        return yup::JSON::toString (list);
    }));
}
