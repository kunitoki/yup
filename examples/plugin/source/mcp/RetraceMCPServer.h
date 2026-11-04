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

#include <yup_ai/yup_ai.h>

#include <atomic>
#include <functional>
#include <memory>
#include <optional>

class RetraceProcessor;

//==============================================================================
/** Lets MCP clients, like coding assistants and chat apps, drive one Retrace instance.

    It serves the Model Context Protocol over Streamable HTTP on the loopback address, on
    the first free port from basePort, so every instance in a process or across hosts gets
    its own URL. The tools read and write every parameter and the whole patch, browse,
    load and save presets, play notes, capture or render audio with an analysis of it,
    and take screenshots of the editor.

    Tools run on the server's threads and hop to the message thread for anything that
    touches the patch or the editor, giving up after a timeout so a blocked message thread
    never hangs a client. Stopping the server abandons the hops still queued.

    Serving is off by default and follows the "MCP Server" setting of every instance.

    @see RetraceProcessor
*/
class RetraceMCPServer
{
public:
    /** The first port tried. */
    static constexpr int basePort = 7380;

    /** How many ports from basePort are tried. */
    static constexpr int numPortsToTry = 32;

    /** Creates a stopped server for a processor, which must outlive it. Message thread. */
    explicit RetraceMCPServer (RetraceProcessor& processor);

    /** Stops serving. Message thread. */
    ~RetraceMCPServer();

    /** Starts or stops serving. Message thread. */
    yup::Result setEnabled (bool shouldBeEnabled);

    /** Returns true while serving. */
    bool isEnabled() const noexcept { return session->load() != 0; }

    /** Returns the URL clients connect to, or an empty string when not serving. Message thread. */
    yup::String getUrl() const;

    /** Starts or stops the servers of every Retrace instance in this process. Message thread. */
    static void setEnabledEverywhere (bool shouldBeEnabled);

private:
    void registerInfoTools();
    void registerPatchTools();
    void registerPresetTools();
    void registerPlayTools();
    void registerAudioTools();
    void registerEditorTools();
    void registerResources();

    yup::MCPToolResult playNotes (const yup::var& arguments);
    yup::MCPToolResult renderAudio (const yup::var& arguments);
    yup::MCPToolResult takeScreenshot (const yup::var& arguments);

    bool runOnMessageThread (std::function<void()> work, int timeoutMs = 10000);

    template <typename T>
    std::optional<T> callOnMessageThread (std::function<T()> work);

    yup::MCPToolResult onMessageThread (std::function<yup::MCPToolResult()> work);

    RetraceProcessor& processor;
    yup::MCPServer server;
    yup::MCPHttpServerTransport* transport = nullptr;

    // The generation being served, 0 when stopped. Shared with the hops to the message
    // thread, which only run while the generation that queued them is still served.
    std::shared_ptr<std::atomic<int>> session = std::make_shared<std::atomic<int>> (0);
    int lastSession = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RetraceMCPServer)
};
