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

//==============================================================================

void Profiler::startTracing()
{
    startTracing (1024 * 128, "YupApp");
}

void Profiler::startTracing (uint32 sizeInKilobytes)
{
    startTracing (sizeInKilobytes, "YupApp");
}

void Profiler::startTracing (StringRef traceName)
{
    startTracing (1024 * 128, traceName);
}

void Profiler::startTracing (uint32 sizeInKilobytes, StringRef traceName)
{
    perfetto::TraceConfig traceConfig;
    traceConfig.add_buffers()->set_size_kb (sizeInKilobytes);
    auto dataSourceConfig = traceConfig.add_data_sources()->mutable_config();
    dataSourceConfig->set_name (String (traceName).toStdString());

    session = perfetto::Tracing::NewTrace();
    session->Setup (traceConfig);
    session->StartBlocking();
}

//==============================================================================

void Profiler::stopTracing()
{
    if (session == nullptr)
        return;

    perfetto::TrackEvent::Flush();

    session->StopBlocking();
    std::vector<char> traceData (session->ReadTraceBlocking());

    const auto destination = File::getSpecialLocation (File::userHomeDirectory)
        .getChildFile ("example.pftrace"); // TODO - make it configurable

    if (auto output = destination.createOutputStream(); output != nullptr && output->openedOk())
        output->write (traceData.data(), traceData.size());

    String message;
    message
        << "Trace written in " << destination.getFullPathName() << ". "
        << "To read this trace in text form, `./tools/traceconv text " << destination.getFullPathName() << "`";

    PERFETTO_LOG ("%s", message.toRawUTF8());

    Profiler::deleteInstance();
}

} // namespace juce

#endif
