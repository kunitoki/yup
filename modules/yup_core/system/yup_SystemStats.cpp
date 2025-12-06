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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

String SystemStats::getYUPVersion()
{
    // Some basic tests, to keep an eye on things and make sure these types work ok
    // on all platforms. Let me know if any of these assertions fail on your system!
    static_assert (sizeof (pointer_sized_int) == sizeof (void*), "Basic sanity test failed: please report!");
    static_assert (sizeof (int8) == 1, "Basic sanity test failed: please report!");
    static_assert (sizeof (uint8) == 1, "Basic sanity test failed: please report!");
    static_assert (sizeof (int16) == 2, "Basic sanity test failed: please report!");
    static_assert (sizeof (uint16) == 2, "Basic sanity test failed: please report!");
    static_assert (sizeof (int32) == 4, "Basic sanity test failed: please report!");
    static_assert (sizeof (uint32) == 4, "Basic sanity test failed: please report!");
    static_assert (sizeof (int64) == 8, "Basic sanity test failed: please report!");
    static_assert (sizeof (uint64) == 8, "Basic sanity test failed: please report!");

    return "YUP v" YUP_STRINGIFY (YUP_MAJOR_VERSION) "." YUP_STRINGIFY (YUP_MINOR_VERSION) "." YUP_STRINGIFY (YUP_BUILDNUMBER);
}

#if YUP_ANDROID && ! defined(YUP_DISABLE_VERSION_PRINTING)
#define YUP_DISABLE_VERSION_PRINTING 1
#endif

#if YUP_DEBUG && ! YUP_DISABLE_VERSION_PRINTING
struct YupVersionPrinter
{
    YupVersionPrinter()
    {
        YUP_DBG (SystemStats::getYUPVersion());
    }
};

static YupVersionPrinter yupVersionPrinter;
#endif

String getLegacyUniqueDeviceID();

StringArray SystemStats::getMachineIdentifiers (MachineIdFlags flags)
{
    auto macAddressProvider = [] (StringArray& arr)
    {
        for (const auto& mac : MACAddress::getAllAddresses())
            arr.add (mac.toString());
    };

    auto fileSystemProvider = [] (StringArray& arr)
    {
#if YUP_WINDOWS
        File f (File::getSpecialLocation (File::windowsSystemDirectory));
#else
        File f ("~");
#endif
        if (auto num = f.getFileIdentifier())
            arr.add (String::toHexString ((int64) num));
    };

    auto legacyIdProvider = [] ([[maybe_unused]] StringArray& arr)
    {
#if YUP_WINDOWS
        arr.add (getLegacyUniqueDeviceID());
#endif
    };

    auto uniqueIdProvider = [] (StringArray& arr)
    {
        arr.add (getUniqueDeviceID());
    };

    struct Provider
    {
        MachineIdFlags flag;
        void (*func) (StringArray&);
    };

    static const Provider providers[] = {
        { MachineIdFlags::macAddresses, macAddressProvider },
        { MachineIdFlags::fileSystemId, fileSystemProvider },
        { MachineIdFlags::legacyUniqueId, legacyIdProvider },
        { MachineIdFlags::uniqueId, uniqueIdProvider }
    };

    StringArray ids;

    for (const auto& provider : providers)
    {
        if (hasBitValueSet (flags, provider.flag))
            provider.func (ids);
    }

    return ids;
}

//==============================================================================
struct CPUInformation
{
    CPUInformation() noexcept { initialise(); }

    void initialise() noexcept;

    int numLogicalCPUs = 0, numPhysicalCPUs = 0;

    bool hasMMX = false, hasSSE = false, hasSSE2 = false, hasSSE3 = false,
         has3DNow = false, hasFMA3 = false, hasFMA4 = false, hasSSSE3 = false,
         hasSSE41 = false, hasSSE42 = false, hasAVX = false, hasAVX2 = false,
         hasAVX512F = false, hasAVX512BW = false, hasAVX512CD = false,
         hasAVX512DQ = false, hasAVX512ER = false, hasAVX512IFMA = false,
         hasAVX512PF = false, hasAVX512VBMI = false, hasAVX512VL = false,
         hasAVX512VPOPCNTDQ = false,
         hasNeon = false;
};

static const CPUInformation& getCPUInformation() noexcept
{
    static CPUInformation info;
    return info;
}

int SystemStats::getNumCpus() noexcept { return getCPUInformation().numLogicalCPUs; }

int SystemStats::getNumPhysicalCpus() noexcept { return getCPUInformation().numPhysicalCPUs; }

bool SystemStats::hasMMX() noexcept { return getCPUInformation().hasMMX; }

bool SystemStats::has3DNow() noexcept { return getCPUInformation().has3DNow; }

bool SystemStats::hasFMA3() noexcept { return getCPUInformation().hasFMA3; }

bool SystemStats::hasFMA4() noexcept { return getCPUInformation().hasFMA4; }

bool SystemStats::hasSSE() noexcept { return getCPUInformation().hasSSE; }

bool SystemStats::hasSSE2() noexcept { return getCPUInformation().hasSSE2; }

bool SystemStats::hasSSE3() noexcept { return getCPUInformation().hasSSE3; }

bool SystemStats::hasSSSE3() noexcept { return getCPUInformation().hasSSSE3; }

bool SystemStats::hasSSE41() noexcept { return getCPUInformation().hasSSE41; }

bool SystemStats::hasSSE42() noexcept { return getCPUInformation().hasSSE42; }

bool SystemStats::hasAVX() noexcept { return getCPUInformation().hasAVX; }

bool SystemStats::hasAVX2() noexcept { return getCPUInformation().hasAVX2; }

bool SystemStats::hasAVX512F() noexcept { return getCPUInformation().hasAVX512F; }

bool SystemStats::hasAVX512BW() noexcept { return getCPUInformation().hasAVX512BW; }

bool SystemStats::hasAVX512CD() noexcept { return getCPUInformation().hasAVX512CD; }

bool SystemStats::hasAVX512DQ() noexcept { return getCPUInformation().hasAVX512DQ; }

bool SystemStats::hasAVX512ER() noexcept { return getCPUInformation().hasAVX512ER; }

bool SystemStats::hasAVX512IFMA() noexcept { return getCPUInformation().hasAVX512IFMA; }

bool SystemStats::hasAVX512PF() noexcept { return getCPUInformation().hasAVX512PF; }

bool SystemStats::hasAVX512VBMI() noexcept { return getCPUInformation().hasAVX512VBMI; }

bool SystemStats::hasAVX512VL() noexcept { return getCPUInformation().hasAVX512VL; }

bool SystemStats::hasAVX512VPOPCNTDQ() noexcept { return getCPUInformation().hasAVX512VPOPCNTDQ; }

bool SystemStats::hasNeon() noexcept { return getCPUInformation().hasNeon; }

//==============================================================================
extern uint64_t yup_compilationUniqueId;

uint64 SystemStats::getCompileUniqueId()
{
    return yup_compilationUniqueId;
}

//==============================================================================
#if YUP_ANDROID && __ANDROID_API__ < 33
struct BacktraceState
{
    BacktraceState (void** current, void** end)
        : current (current)
        , end (end)
    {
    }

    void** current;
    void** end;
};

static _Unwind_Reason_Code unwindCallback (struct _Unwind_Context* context, void* arg)
{
    BacktraceState* state = static_cast<BacktraceState*> (arg);

    if (uintptr_t pc = _Unwind_GetIP (context))
    {
        if (state->current == state->end)
            return _URC_END_OF_STACK;

        *state->current++ = reinterpret_cast<void*> (pc);
    }

    return _URC_NO_REASON;
}
#endif

String SystemStats::getStackBacktrace()
{
    String result;

#if (YUP_WASM && ! YUP_EMSCRIPTEN)
    jassertfalse; // sorry, not implemented yet!

#elif YUP_EMSCRIPTEN
    std::string temporaryStack;
    temporaryStack.resize (10 * EM_ASM_INT_V ({ return (lengthBytesUTF8 || Module.lengthBytesUTF8) (stackTrace()); }));
    EM_ASM_ARGS ({ (stringToUTF8 || Module.stringToUTF8) (stackTrace(), $0, $1); }, temporaryStack.data(), temporaryStack.size());
    result << temporaryStack.c_str();

#elif YUP_WINDOWS
    HANDLE process = GetCurrentProcess();
    SymInitialize (process, nullptr, TRUE);

    void* stack[128];
    int frames = (int) CaptureStackBackTrace (0, numElementsInArray (stack), stack, nullptr);

    HeapBlock<SYMBOL_INFO> symbol;
    symbol.calloc (sizeof (SYMBOL_INFO) + 256, 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof (SYMBOL_INFO);

    for (int i = 0; i < frames; ++i)
    {
        DWORD64 displacement = 0;

        if (SymFromAddr (process, (DWORD64) stack[i], &displacement, symbol))
        {
            result << i << ": ";

            IMAGEHLP_MODULE64 moduleInfo;
            zerostruct (moduleInfo);
            moduleInfo.SizeOfStruct = sizeof (moduleInfo);

            if (::SymGetModuleInfo64 (process, symbol->ModBase, &moduleInfo))
                result << moduleInfo.ModuleName << ": ";

            result << symbol->Name << " + 0x" << String::toHexString ((int64) displacement) << newLine;
        }
    }

#else
    void* stack[128];

#if YUP_ANDROID && __ANDROID_API__ < 33
    BacktraceState state (stack, stack + numElementsInArray (stack));
    _Unwind_Backtrace (unwindCallback, &state);

    auto frames = static_cast<size_t> (state.current - &stack[0]);
    char** frameStrings = nullptr;
#else
    auto frames = backtrace (stack, numElementsInArray (stack));
    char** frameStrings = backtrace_symbols (stack, frames);
#endif

    for (auto i = (decltype (frames)) 0; i < frames; ++i)
    {
        Dl_info info;
        if (dladdr (stack[i], &info))
        {
            int status = 0;
            std::unique_ptr<char, decltype (::free)*> demangled (abi::__cxa_demangle (info.dli_sname, nullptr, 0, &status), ::free);

            if (status == 0)
            {
                result
                    << yup::String (i).paddedRight (' ', 3)
                    << " " << yup::File (yup::String (info.dli_fname)).getFileName().paddedRight (' ', 35)
                    << " 0x" << yup::String::toHexString ((size_t) stack[i]).paddedLeft ('0', sizeof (void*) * 2)
                    << " " << demangled.get()
                    << " + " << ((char*) stack[i] - (char*) info.dli_saddr) << newLine;
                continue;
            }
        }

        if (frameStrings != nullptr)
            result << frameStrings[i] << newLine;
    }
#endif

    return result;
}

//==============================================================================
static SystemStats::CrashHandlerFunction globalCrashHandler = nullptr;

#if YUP_WINDOWS
static LONG WINAPI handleCrash (LPEXCEPTION_POINTERS ep)
{
    globalCrashHandler (ep);
    return EXCEPTION_EXECUTE_HANDLER;
}
#else
static void handleCrash (int signum)
{
    globalCrashHandler ((void*) (pointer_sized_int) signum);

#if ! YUP_WASM
    ::kill (getpid(), SIGKILL);
#endif
}

int yup_siginterrupt (int sig, int flag);
#endif

void SystemStats::setApplicationCrashHandler (CrashHandlerFunction handler)
{
    jassert (handler != nullptr); // This must be a valid function.
    globalCrashHandler = handler;

#if YUP_WINDOWS
    SetUnhandledExceptionFilter (handleCrash);

#elif YUP_WASM
    // TODO

#else
    const int signals[] = { SIGFPE, SIGILL, SIGSEGV, SIGBUS, SIGABRT, SIGSYS };

    for (int i = 0; i < numElementsInArray (signals); ++i)
    {
        ::signal (signals[i], handleCrash);
        yup_siginterrupt (signals[i], 1);
    }

#endif
}

bool SystemStats::isRunningInAppExtensionSandbox() noexcept
{
#if YUP_MAC || YUP_IOS
    static bool isRunningInAppSandbox = [&]
    {
        File bundle = File::getSpecialLocation (File::invokedExecutableFile).getParentDirectory();

#if YUP_MAC
        bundle = bundle.getParentDirectory().getParentDirectory();
#endif

        if (bundle.isDirectory())
            return bundle.getFileExtension() == ".appex";

        return false;
    }();

    return isRunningInAppSandbox;
#else
    return false;
#endif
}

} // namespace yup
