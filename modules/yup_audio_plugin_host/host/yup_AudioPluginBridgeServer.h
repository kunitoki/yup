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


#pragma once

namespace yup
{

//==============================================================================
/**
    Runs plugins on behalf of RemoteAudioPluginInstance objects in a separate process.

    A plugin running in a bridge server process can crash or hang without taking the
    host down with it. Each RemoteAudioPluginInstance launches its own server process,
    which loads a single plugin and processes its audio through shared memory.

    The server executable is typically the host application itself, relaunched with a
    special command line. Create the server at startup, before anything else:

    @code
    void initialise (const String& commandLine) override
    {
        auto server = std::make_unique<AudioPluginBridgeServer>();
        server->addFormat (std::make_unique<VST3Format>());
        server->addFormat (std::make_unique<CLAPFormat>());

        if (server->initialiseFromCommandLine (commandLine))
        {
            bridgeServer = std::move (server); // keep it alive, don't open any window
            return;
        }

        // ... normal application startup
    }
    @endcode

    The plugin is loaded and controlled on the message thread of the server process, which
    must be running a dispatch loop, and processes audio on a dedicated real-time thread.
    Plugin editors open in a floating window owned by the server process. The server
    terminates itself as soon as the host disconnects.

    @see RemoteAudioPluginInstance
*/
class AudioPluginBridgeServer : private ChildProcessWorker
{
public:
    //==============================================================================

    /** The command line identifier used by default by both the host and the server. */
    static constexpr const char* defaultCommandLineUniqueID = "yupPluginBridge";

    //==============================================================================

    AudioPluginBridgeServer();
    ~AudioPluginBridgeServer() override;

    //==============================================================================

    /** Registers a format backend able to load plugins, replacing one of the same type. */
    void addFormat (std::unique_ptr<AudioPluginFormat> format);

    //==============================================================================

    /**
        Connects to the host when @p commandLine was produced by a RemoteAudioPluginInstance.

        Returns false when the process was not launched as a bridge server, in which case
        the application should carry on with its normal startup.

        @param commandLine          The command line the application was started with.
        @param commandLineUniqueID  Must match RemoteAudioPluginInstance::Options::commandLineUniqueID.
    */
    bool initialiseFromCommandLine (const String& commandLine,
                                    const String& commandLineUniqueID = defaultCommandLineUniqueID);

private:
    class Session;

    void handleMessageFromCoordinator (const MemoryBlock& message) override;
    void handleConnectionLost() override;

    void handleRequest (const MemoryBlock& message);
    void sendToHost (const MemoryBlock& message);

    std::vector<std::unique_ptr<AudioPluginFormat>> formats;
    std::unique_ptr<Session> session;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginBridgeServer)
};

} // namespace yup
