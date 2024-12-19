/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

#if JUCE_ENABLE_PROFILING

PERFETTO_TRACK_EVENT_STATIC_STORAGE();

namespace juce
{

//==============================================================================

template <std::size_t sizeResult, std::size_t sizeTest>
constexpr bool stringsEqual (const std::array<char, sizeResult>& result, const char (&test)[sizeTest])
{
    static_assert (sizeTest > 1);
    static_assert (sizeResult + 1 >= sizeTest);

    return std::string_view (test, sizeTest) == std::string_view (result.data(), sizeTest);
}

// clang-format off
static_assert (stringsEqual (Profiler::compileTimePrettierFunction ([] { return "int main"; }), "main"));
static_assert (stringsEqual (Profiler::compileTimePrettierFunction ([] { return "void AudioProcessor::processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &)::(anonymous class)::operator()()::(anonymous class)::operator()(uint32_t) const"; }), "AudioProcessor::processBlock"));
static_assert (stringsEqual (Profiler::compileTimePrettierFunction ([] { return "void __cdecl AudioProcessor::processBlock::<lambda_1>::operator"; }), "AudioProcessor::processBlock"));
static_assert (stringsEqual (Profiler::compileTimePrettierFunction ([] { return "void __fastcall AudioProcessor::processBlock::<lambda_1>::operator"; }), "AudioProcessor::processBlock"));
// clang-format on

//==============================================================================

JUCE_IMPLEMENT_SINGLETON (Profiler)

//==============================================================================

Profiler::Profiler()
{
    perfetto::TracingInitArgs args;
    args.backends |= perfetto::kInProcessBackend;

    perfetto::Tracing::Initialize (args);

    perfetto::TrackEvent::Register();
    perfetto::ConsoleInterceptor::Register();
}

Profiler::~Profiler()
{
    clearSingletonInstance();
}

//==============================================================================

void Profiler::startTracing()
{
    startTracing (1024 * 128);
}

void Profiler::startTracing (uint32 sizeInKilobytes)
{
    perfetto::TraceConfig traceConfig;
    traceConfig.add_buffers()->set_size_kb (sizeInKilobytes);

    auto dataSourceConfig = traceConfig.add_data_sources()->mutable_config();
    dataSourceConfig->set_name ("track_event");

    String fileName;
    fileName
        << "yup-profile"
#if JUCE_DEBUG
        << "-DEBUG-"
#else
        << "-RELEASE-"
#endif
        << Time::getCurrentTime().formatted ("%Y-%m-%d_%H%M%S")
        << ".pftrace";

    const auto destination = File::getSpecialLocation (File::userHomeDirectory) // TODO - make it configurable
                                 .getChildFile (fileName);

    if (destination.existsAsFile())
        destination.deleteFile();

    fileDescriptor = open (destination.getFullPathName().toRawUTF8(), O_RDWR | O_CREAT | O_TRUNC, 0600);

    session = perfetto::Tracing::NewTrace();
    session->Setup (traceConfig, fileDescriptor);
    session->StartBlocking();
}

//==============================================================================

void Profiler::stopTracing()
{
    if (session == nullptr)
        return;

    perfetto::TrackEvent::Flush();

    session->StopBlocking();

    close (fileDescriptor);

    Profiler::deleteInstance();
}

} // namespace juce

#endif
