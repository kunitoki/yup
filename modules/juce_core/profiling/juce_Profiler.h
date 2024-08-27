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

PERFETTO_DEFINE_CATEGORIES (
    perfetto::Category ("rendering"),
    perfetto::Category ("network"));

namespace juce
{

//==============================================================================

/** A singleton class that handles performance tracing using Perfetto.

    The `Profiler` class allows you to start and stop performance tracing, with options for custom buffer sizes.
    It is implemented as a singleton and cannot be copied or moved. This class leverages Perfetto for tracing sessions.

    @tags{Profiling}
*/
class JUCE_API Profiler
{
public:
    ~Profiler();

    /** Starts a tracing session with the default buffer size.

        This method starts the tracing process using a default buffer size. The tracing session is managed internally
        and will continue until `stopTracing()` is called.
    */
    void startTracing();

    /** Starts a tracing session with a custom buffer size.

        This method allows you to specify the buffer size used for tracing. The buffer size is defined in kilobytes.

        @param sizeInKilobytes The size of the tracing buffer in kilobytes.
    */
    void startTracing (uint32 sizeInKilobytes);

    /** Stops the current tracing session.

        This method stops the tracing process and finalizes the trace data.
        Once tracing is stopped, the data can be retrieved and analyzed.
    */
    void stopTracing();

    /** A constexpr function that prettifies a function name at compile time.

        This template function accepts a function name and formats it in a  more readable manner at compile time.

        @tparam F A function name type.

        @param funcName The function name to prettify.

        @returns The prettified function name.
    */
    template <typename F>
    static constexpr auto compileTimePrettierFunction (F funcName);

    /** Singleton declaration for the Profiler class. */
    JUCE_DECLARE_SINGLETON (Profiler, false)

protected:
    Profiler();

    std::unique_ptr<perfetto::TracingSession> session;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Profiler);
};

template <typename F>
constexpr auto Profiler::compileTimePrettierFunction (F func)
{
    if (! isConstantEvaluated())
        jassertfalse;

    constexpr auto source = func();
    constexpr auto sourceSize = std::string_view (source).size();
    std::array<char, sourceSize + 1> result {};

    for (std::size_t i = 0; i < sourceSize; ++i)
    {
        // wait until after the return type (first space in the string)
        if (source[i] != ' ')
            continue;

        ++i; // skip the space

        // MSVC has an additional identifier after the return type: __cdecl, __fastcall, ...
        if (source[i] == '_')
        {
            while (source[i] != ' ')
                ++i;

            ++i;
        }

        std::size_t j = 0;

        // build result, stop when we hit the arguments
        // clang and gcc use (, MSVC uses <
        while ((source[i] != '(' && source[i] != '<') && i < sourceSize && j < sourceSize)
            result[j++] = source[i++];

        // really ugly clean up after msvc, remove the extra :: before <lambda_1>
        if (source[i] == '<')
            result[j - 2] = '\0';
        else
            result[j] = '\0';

        return result;
    }

    return result;
}
} // namespace juce

// clang-format off
/**
   Starts profiling/tracing with the given arguments.

   This macro is used to start a profiling session using the JUCE Profiler.
   It passes the provided arguments to the `startTracing` function of the `Profiler` singleton.

   @param ... Arguments to be passed to `juce::Profiler::startTracing`.
*/
#define YUP_PROFILE_START(...) ::juce::Profiler::getInstance()->startTracing (__VA_ARGS__)

/** Stops the profiling/tracing session.

    This macro stops the current profiling session using the JUCE Profiler.
    It calls the `stopTracing` function of the `Profiler` singleton.
*/
#define YUP_PROFILE_STOP(...) ::juce::Profiler::getInstance()->stopTracing()

/** Records a profiling trace event.

    This macro is used to trace events for profiling purposes.
    If tracing is enabled, it generates a trace event with the specified category and optional additional arguments.
    When tracing is disabled, it will do nothing.

    @param category The category for the trace event.
    @param ... Optional additional arguments for the trace event.
*/
#if ! YUP_PROFILE_DISABLE_TRACE
#define YUP_PROFILE_TRACE(category, ...)                                                                                                                        \
    constexpr auto JUCE_JOIN_MACRO (juce_pfn_, __LINE__) = ::juce::Profiler::compileTimePrettierFunction ([] { return PERFETTO_DEBUG_FUNCTION_IDENTIFIER(); }); \
    TRACE_EVENT (category, ::perfetto::StaticString (JUCE_JOIN_MACRO (juce_pfn_, __LINE__).data()), ##__VA_ARGS__)
#else
#define YUP_PROFILE_TRACE(category, ...)
#endif
// clang-format on

#else
#define YUP_PROFILE_START(...)
#define YUP_PROFILE_STOP(...)
#define YUP_PROFILE_TRACE(category, ...)

#endif
