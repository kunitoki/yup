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
    Runs plugin scans requested by an AudioPluginScanner in a separate process.

    Opening a plugin can crash or hang, so an AudioPluginScanner configured with
    AudioPluginScanner::setOutOfProcessScanning() launches a worker executable for
    each scanning thread and asks it to open files with AudioPluginFormat::scanFileFully().
    A crash or hang only takes down the worker, and the file is reported as failed.

    The worker executable is typically the host application itself, relaunched with
    a special command line. Create the worker at startup, before anything else:

    @code
    void initialise (const String& commandLine) override
    {
        auto worker = std::make_unique<AudioPluginScannerWorker>();
        worker->addFormat (std::make_unique<VST3Format>());
        worker->addFormat (std::make_unique<CLAPFormat>());

        if (worker->initialiseFromCommandLine (commandLine))
        {
            scannerWorker = std::move (worker); // keep it alive, don't open any window
            return;
        }

        // ... normal application startup
    }
    @endcode

    Scans run on the message thread of the worker process, which must be running a
    dispatch loop. The worker terminates itself as soon as the scanner disconnects.
*/
class AudioPluginScannerWorker : private ChildProcessWorker
{
public:
    //==============================================================================

    /** The command line identifier used by default by both the scanner and the worker. */
    static constexpr const char* defaultCommandLineUniqueID = "yupPluginScanner";

    //==============================================================================

    AudioPluginScannerWorker();
    ~AudioPluginScannerWorker() override;

    //==============================================================================

    /** Registers a format backend able to open plugins, replacing one of the same type. */
    void addFormat (std::unique_ptr<AudioPluginFormat> format);

    /** Sets the host context used when opening plugins during scans. */
    void setHostContext (const AudioPluginHostContext& newContext);

    //==============================================================================

    /**
        Connects to the scanner when @p commandLine was produced by an AudioPluginScanner.

        Returns false when the process was not launched as a scanner worker, in which
        case the application should carry on with its normal startup.

        @param commandLine          The command line the application was started with.
        @param commandLineUniqueID  Must match AudioPluginScanner::OutOfProcessOptions::commandLineUniqueID.
    */
    bool initialiseFromCommandLine (const String& commandLine,
                                    const String& commandLineUniqueID = defaultCommandLineUniqueID);

private:
    void handleMessageFromCoordinator (const MemoryBlock& message) override;
    void handleConnectionLost() override;

    void scanOnMessageThread (const MemoryBlock& request);

    std::vector<std::unique_ptr<AudioPluginFormat>> formats;
    AudioPluginHostContext context;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginScannerWorker)
};

} // namespace yup
