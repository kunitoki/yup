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

PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category ("rendering"),
    perfetto::Category ("network")
);

namespace juce
{

//==============================================================================

class JUCE_API Profiler
{
public:
    void startTracing();
    void startTracing (uint32 sizeInKilobytes);
    void startTracing (StringRef traceName);
    void startTracing (uint32 sizeInKilobytes, StringRef traceName);

    void stopTracing();

    JUCE_DECLARE_SINGLETON (Profiler, false)

protected:
    Profiler();

    std::unique_ptr<perfetto::TracingSession> session;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Profiler);
};

} // namespace juce

#define YUP_PROFILE_START(...) ::juce::Profiler::getInstance()->startTracing (__VA_ARGS__)
#define YUP_PROFILE_STOP(...) ::juce::Profiler::getInstance()->stopTracing()
#define YUP_PROFILE_EVENT(...) TRACE_EVENT (__VA_ARGS__)
#else
#define YUP_PROFILE_START(...)
#define YUP_PROFILE_STOP(...)
#define YUP_PROFILE_EVENT(...)
#endif
