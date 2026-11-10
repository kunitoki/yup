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

// Build-time helper executable that generates manifest.ttl and plugin.ttl for
// an LV2 bundle. It instantiates the plugin processor to query bus layout and
// parameter metadata, then writes both TTL files.
//
// Port index order must stay in sync with NonAudioPort enum in
// yup_audio_plugin_client_LV2.cpp.

#include <yup_audio_plugin_client/yup_audio_plugin_client.h>

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

//==============================================================================

extern "C" yup::AudioProcessor* YUP_AUDIO_PLUGIN_CREATE_FUNCTION();

//==============================================================================

struct Options
{
    std::string uri;
    std::string name;
    std::string vendor;
    std::string version;
    std::string description;
    std::string outputDir;
    std::string binaryName;
    std::string ttlName;
    bool isSynth = false;
};

//==============================================================================

static std::string escapeString (const std::string& s)
{
    std::string r;
    r.reserve (s.size());
    for (char c : s)
    {
        if (c == '"')
            r += "\\\"";
        else if (c == '\\')
            r += "\\\\";
        else
            r += c;
    }
    return r;
}

static std::string fmtFloat (float v)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision (6) << v;
    std::string s = ss.str();
    if (s.find ('.') != std::string::npos)
    {
        s.erase (s.find_last_not_of ('0') + 1);
        if (s.back() == '.')
            s += '0';
    }
    return s;
}

static const char* lv2UnitForParameter (const yup::AudioParameter& parameter)
{
    using Unit = yup::AudioParameter::ParameterUnit;

    switch (parameter.getUnit())
    {
        case Unit::Percent:
            return "units:pc";
        case Unit::Decibels:
            return "units:db";
        case Unit::Hertz:
            return "units:hz";
        case Unit::Milliseconds:
            return "units:ms";
        case Unit::Seconds:
            return "units:s";
        case Unit::Degrees:
            return "units:degree";
        case Unit::Cents:
            return "units:cent";
        case Unit::Semitones:
            return "units:semitone12TET";
        case Unit::Octaves:
            return "units:oct";
        case Unit::BPM:
            return "units:bpm";
        case Unit::Beats:
            return "units:beat";
        case Unit::MIDINoteNumber:
            return "units:midiNote";
        default:
            return nullptr;
    }
}

static bool isToggleParameter (const yup::AudioParameter& parameter)
{
    return parameter.isStepped()
        && parameter.getNumSteps() == 1
        && parameter.getMinimumValue() == 0.0f
        && parameter.getMaximumValue() == 1.0f;
}

static bool isIntegerParameter (const yup::AudioParameter& parameter)
{
    if (! parameter.isStepped() || parameter.getNumSteps() <= 0)
        return false;

    const auto stepSize = (parameter.getMaximumValue() - parameter.getMinimumValue()) / static_cast<float> (parameter.getNumSteps());
    return std::abs (stepSize - std::round (stepSize)) < 1.0e-6f
        && std::abs (parameter.getMinimumValue() - std::round (parameter.getMinimumValue())) < 1.0e-6f;
}

static void writeParameterMetadata (std::ostream& f, const yup::AudioParameter& parameter)
{
    constexpr int maxScalePoints = 128;

    const bool isToggle = isToggleParameter (parameter);
    const bool isInteger = ! isToggle && isIntegerParameter (parameter);

    f << "    rdfs:range " << (isToggle ? "atom:Bool" : (isInteger ? "atom:Int" : "atom:Float")) << " ;\n";

    if (isToggle)
        f << "    lv2:portProperty lv2:toggled ;\n";
    else if (parameter.isEnum())
        f << "    lv2:portProperty lv2:integer, lv2:enumeration ;\n";
    else if (isInteger)
        f << "    lv2:portProperty lv2:integer ;\n";

    if (! parameter.isAutomatable())
        f << "    lv2:portProperty pprops:notAutomatic ;\n";

    if (const auto* unit = lv2UnitForParameter (parameter))
    {
        f << "    units:unit " << unit << " ;\n";
    }
    else if (parameter.getUnit() == yup::AudioParameter::ParameterUnit::Custom && parameter.getUnitName().isNotEmpty())
    {
        const std::string unitName = escapeString (parameter.getUnitName().toStdString());
        f << "    units:unit [ a units:Unit ; rdfs:label \"" << unitName << "\" ; units:symbol \"" << unitName << "\" ; units:render \"%f " << unitName << "\" ] ;\n";
    }

    if (parameter.isEnum() && parameter.getNumSteps() < maxScalePoints)
    {
        const int numSteps = parameter.getNumSteps();
        const float minVal = parameter.getMinimumValue();
        const float stepSize = (parameter.getMaximumValue() - minVal) / static_cast<float> (numSteps);

        f << "    lv2:scalePoint ";
        for (int step = 0; step <= numSteps; ++step)
        {
            const float value = minVal + stepSize * static_cast<float> (step);
            f << "[ rdfs:label \"" << escapeString (parameter.convertToString (value).toStdString()) << "\" ; rdf:value " << fmtFloat (value) << " ]"
              << (step < numSteps ? " , " : " ;\n");
        }
    }
}

static const char* lv2UIClass()
{
#if YUP_MAC
    return "ui:CocoaUI";
#elif YUP_WINDOWS
    return "ui:WindowsUI";
#else
    return "ui:X11UI";
#endif
}

static std::string uiUri (const Options& opts)
{
    return opts.uri + ":ui";
}

//==============================================================================

static bool writeManifest (const Options& opts, bool hasEditor)
{
    const std::string path = opts.outputDir + "/manifest.ttl";
    std::ofstream f (path);
    if (! f.is_open())
    {
        std::cerr << "lv2_ttl_generator: cannot open " << path << "\n";
        return false;
    }

    f << "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n"
      << "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
      << "@prefix ui:   <http://lv2plug.in/ns/extensions/ui#> .\n"
      << "\n"
      << "<" << opts.uri << ">\n"
      << "    a lv2:Plugin ;\n"
      << "    lv2:binary <" << opts.binaryName << "> ;\n"
      << "    rdfs:seeAlso <" << opts.ttlName << "> .\n";

    if (hasEditor)
    {
        f << "\n"
          << "<" << uiUri (opts) << ">\n"
          << "    a " << lv2UIClass() << " ;\n"
          << "    ui:binary <" << opts.binaryName << "> ;\n"
          << "    rdfs:seeAlso <" << opts.ttlName << "> .\n";
    }

    return true;
}

//==============================================================================

static bool writePluginTTL (const Options& opts, yup::AudioProcessor& processor)
{
    const std::string path = opts.outputDir + "/" + opts.ttlName;
    std::ofstream f (path);
    if (! f.is_open())
    {
        std::cerr << "lv2_ttl_generator: cannot open " << path << "\n";
        return false;
    }

    const auto& layout = processor.getBusLayout();
    const int numInputs = layout.getNumAudioInputChannels();
    const int numOutputs = layout.getNumAudioOutputChannels();
    const auto& params = processor.getParameters();

    f << "@prefix atom:   <http://lv2plug.in/ns/ext/atom#> .\n"
      << "@prefix bufsz:  <http://lv2plug.in/ns/ext/buf-size#> .\n"
      << "@prefix doap:   <http://usefulinc.com/ns/doap#> .\n"
      << "@prefix foaf:   <http://xmlns.com/foaf/0.1/> .\n"
      << "@prefix lv2:    <http://lv2plug.in/ns/lv2core#> .\n"
      << "@prefix midi:   <http://lv2plug.in/ns/ext/midi#> .\n"
      << "@prefix opts:   <http://lv2plug.in/ns/ext/options#> .\n"
      << "@prefix patch:  <http://lv2plug.in/ns/ext/patch#> .\n"
      << "@prefix pprops: <http://lv2plug.in/ns/ext/port-props#> .\n"
      << "@prefix rdf:    <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
      << "@prefix rdfs:   <http://www.w3.org/2000/01/rdf-schema#> .\n"
      << "@prefix state:  <http://lv2plug.in/ns/ext/state#> .\n"
      << "@prefix time:   <http://lv2plug.in/ns/ext/time#> .\n"
      << "@prefix ui:     <http://lv2plug.in/ns/extensions/ui#> .\n"
      << "@prefix units:  <http://lv2plug.in/ns/extensions/units#> .\n"
      << "@prefix urid:   <http://lv2plug.in/ns/ext/urid#> .\n"
      << "\n";

    f << "<" << opts.uri << ">\n"
      << "    a lv2:Plugin";
    if (opts.isSynth)
        f << ", lv2:InstrumentPlugin";
    f << " ;\n";

    f << "    doap:name \"" << escapeString (opts.name) << "\" ;\n";
    if (! opts.description.empty())
        f << "    rdfs:comment \"" << escapeString (opts.description) << "\" ;\n";
    if (! opts.vendor.empty())
        f << "    doap:maintainer [ foaf:name \"" << escapeString (opts.vendor) << "\" ] ;\n";
    if (! opts.version.empty())
    {
        // LV2 carries the major version in the URI, so only the minor and micro parts are written
        const auto parts = yup::StringArray::fromTokens (yup::String (opts.version), ".", {});
        f << "    lv2:minorVersion " << parts[1].getIntValue() << " ;\n"
          << "    lv2:microVersion " << parts[2].getIntValue() << " ;\n";
    }
    f << "\n";

    f << "    lv2:requiredFeature urid:map, opts:options, bufsz:boundedBlockLength ;\n"
      << "    lv2:optionalFeature\n"
      << "        lv2:hardRTCapable,\n"
      << "        lv2:isLive ;\n"
      << "    opts:requiredOption bufsz:maxBlockLength ;\n"
      << "\n"
      << "    lv2:extensionData state:interface ;\n"
      << "\n";

    if (processor.hasEditor())
        f << "    ui:ui <" << uiUri (opts) << "> ;\n\n";

    std::vector<yup::AudioParameter::Ptr> writableParams;
    for (const auto& param : params)
        if (! param->isReadOnly())
            writableParams.push_back (param);

    if (! writableParams.empty())
    {
        f << "    patch:writable\n";
        for (std::size_t i = 0; i < writableParams.size(); ++i)
        {
            const int hostId = static_cast<int> (writableParams[i]->getHostParameterID());
            f << "        <" << opts.uri << ":param_" << hostId << ">";
            f << (i + 1 < writableParams.size() ? " ,\n" : " ;\n");
        }
        f << "\n";
    }

    if (! params.empty())
    {
        f << "    patch:readable\n";
        for (std::size_t i = 0; i < params.size(); ++i)
        {
            const int hostId = static_cast<int> (params[i]->getHostParameterID());
            f << "        <" << opts.uri << ":param_" << hostId << ">";
            f << (i + 1 < params.size() ? " ,\n" : " ;\n");
        }
        f << "\n";
    }

    // Port list - order matches NonAudioPort enum:
    //   audio in [0..numInputs), audio out [numInputs..+numOutputs),
    //   seqInput, seqOutput, latencyOutput, freeWheelingInput, enabledInput.
    int portIdx = 0;
    bool firstPort = true;

    auto beginPort = [&]() -> std::ostream&
    {
        if (firstPort)
        {
            f << "    lv2:port [\n";
            firstPort = false;
        }
        else
        {
            f << "    ] , [\n";
        }
        return f;
    };

    for (int ch = 0; ch < numInputs; ++ch, ++portIdx)
    {
        beginPort()
            << "        a lv2:AudioPort, lv2:InputPort ;\n"
            << "        lv2:index " << portIdx << " ;\n"
            << "        lv2:symbol \"audio_in_" << ch << "\" ;\n"
            << "        lv2:name \"Audio Input " << ch << "\" ;\n";
    }

    for (int ch = 0; ch < numOutputs; ++ch, ++portIdx)
    {
        beginPort()
            << "        a lv2:AudioPort, lv2:OutputPort ;\n"
            << "        lv2:index " << portIdx << " ;\n"
            << "        lv2:symbol \"audio_out_" << ch << "\" ;\n"
            << "        lv2:name \"Audio Output " << ch << "\" ;\n";
    }

    beginPort()
        << "        a atom:AtomPort, lv2:InputPort ;\n"
        << "        lv2:index " << portIdx++ << " ;\n"
        << "        lv2:symbol \"seq_in\" ;\n"
        << "        lv2:name \"Control Input\" ;\n"
        << "        atom:bufferType atom:Sequence ;\n"
        << "        atom:supports midi:MidiEvent, patch:Message, time:Position ;\n";

    beginPort()
        << "        a atom:AtomPort, lv2:OutputPort ;\n"
        << "        lv2:index " << portIdx++ << " ;\n"
        << "        lv2:symbol \"seq_out\" ;\n"
        << "        lv2:name \"Control Output\" ;\n"
        << "        atom:bufferType atom:Sequence ;\n"
        << "        atom:supports patch:Message ;\n";

    beginPort()
        << "        a lv2:ControlPort, lv2:OutputPort ;\n"
        << "        lv2:index " << portIdx++ << " ;\n"
        << "        lv2:symbol \"latency\" ;\n"
        << "        lv2:name \"Latency\" ;\n"
        << "        lv2:designation lv2:latency ;\n"
        << "        lv2:portProperty lv2:reportsLatency ;\n";

    beginPort()
        << "        a lv2:ControlPort, lv2:InputPort ;\n"
        << "        lv2:index " << portIdx++ << " ;\n"
        << "        lv2:symbol \"freewheel\" ;\n"
        << "        lv2:name \"Freewheel\" ;\n"
        << "        lv2:designation lv2:freeWheeling ;\n"
        << "        lv2:default 0.0 ;\n"
        << "        lv2:minimum 0.0 ;\n"
        << "        lv2:maximum 1.0 ;\n";

    beginPort()
        << "        a lv2:ControlPort, lv2:InputPort ;\n"
        << "        lv2:index " << portIdx++ << " ;\n"
        << "        lv2:symbol \"enabled\" ;\n"
        << "        lv2:name \"Enabled\" ;\n"
        << "        lv2:designation lv2:enabled ;\n"
        << "        lv2:default 1.0 ;\n"
        << "        lv2:minimum 0.0 ;\n"
        << "        lv2:maximum 1.0 ;\n";

    f << "    ] .\n\n";

    if (processor.hasEditor())
    {
        f << "<" << uiUri (opts) << ">\n"
          << "    a " << lv2UIClass() << " ;\n"
          << "    lv2:requiredFeature <http://lv2plug.in/ns/ext/instance-access>, ui:parent ;\n"
          << "    lv2:optionalFeature ui:resize, ui:idleInterface ;\n"
          << "    lv2:extensionData ui:idleInterface, ui:resize .\n\n";
    }

    for (const auto& param : params)
    {
        const int hostId = static_cast<int> (param->getHostParameterID());
        const std::string label = param->getName().toRawUTF8();
        const float defVal = param->getDefaultValue();
        const float minVal = param->getMinimumValue();
        const float maxVal = param->getMaximumValue();

        f << "<" << opts.uri << ":param_" << hostId << ">\n"
          << "    a lv2:Parameter ;\n"
          << "    rdfs:label \"" << escapeString (label) << "\" ;\n";

        writeParameterMetadata (f, *param);

        f << "    lv2:default " << fmtFloat (defVal) << " ;\n"
          << "    lv2:minimum " << fmtFloat (minVal) << " ;\n"
          << "    lv2:maximum " << fmtFloat (maxVal) << " .\n\n";
    }

    return true;
}

//==============================================================================

int main (int argc, char** argv)
{
    Options opts;

    for (int i = 1; i + 1 < argc; i += 2)
    {
        const std::string key (argv[i]);
        const std::string val (argv[i + 1]);

        if (key == "--uri")
            opts.uri = val;
        else if (key == "--name")
            opts.name = val;
        else if (key == "--vendor")
            opts.vendor = val;
        else if (key == "--version")
            opts.version = val;
        else if (key == "--description")
            opts.description = val;
        else if (key == "--output-dir")
            opts.outputDir = val;
        else if (key == "--binary-name")
            opts.binaryName = val;
        else if (key == "--ttl-name")
            opts.ttlName = val;
        else if (key == "--is-synth")
            opts.isSynth = (val == "1" || val == "ON" || val == "TRUE" || val == "true");
    }

    if (opts.uri.empty() || opts.outputDir.empty() || opts.binaryName.empty() || opts.ttlName.empty())
    {
        std::cerr << "Usage: lv2_ttl_generator\n"
                  << "    --uri <uri>            Plugin URI\n"
                  << "    --name <name>          Plugin display name\n"
                  << "    --vendor <vendor>      Vendor name\n"
                  << "    --version <version>    Plugin version\n"
                  << "    --description <desc>   Plugin description\n"
                  << "    --output-dir <dir>     Output directory for TTL files\n"
                  << "    --binary-name <file>   Binary filename inside bundle\n"
                  << "    --ttl-name <file>      Plugin TTL filename\n"
                  << "    [--is-synth 1|ON]      Mark plugin as an instrument\n";
        return 1;
    }

    std::unique_ptr<yup::AudioProcessor> processor (yup::createPluginProcessorForWrapper (yup::AudioPluginWrapperType::lv2, ::YUP_AUDIO_PLUGIN_CREATE_FUNCTION));
    if (processor == nullptr)
    {
        std::cerr << "lv2_ttl_generator: createPluginProcessor() returned nullptr\n";
        return 1;
    }

    if (! writeManifest (opts, processor->hasEditor()))
        return 1;

    if (! writePluginTTL (opts, *processor))
        return 1;

    std::cout << "lv2_ttl_generator: wrote "
              << opts.outputDir << "/manifest.ttl and "
              << opts.outputDir << "/" << opts.ttlName << "\n";
    return 0;
}
