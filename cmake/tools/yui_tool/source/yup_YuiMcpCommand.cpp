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

#include "yup_YuiAgentTools.h"
#include "yup_YuiCommands.h"
#include "yup_YuiToolSupport.h"

#include <yup_ai/yup_ai.h>

#include <chrono>
#include <condition_variable>
#include <deque>
#include <future>
#include <iostream>
#include <mutex>

using namespace yup;

namespace
{

//==============================================================================
/** Runs on the thread of the application the calls a server receives on the thread of its transport.

    The tools build components, which belong to the thread that set the application up.
*/
class CallQueue
{
public:
    /** Waits for the thread of the application to run a call, and returns what it gave. */
    MCPToolResult run (std::function<MCPToolResult()> call)
    {
        std::packaged_task<MCPToolResult()> task (std::move (call));
        auto result = task.get_future();

        {
            const std::lock_guard lock (mutex);
            pending.push_back (std::move (task));
        }

        available.notify_one();
        return result.get();
    }

    /** Runs the calls waiting, after waiting for one for the time given at most. */
    void runPending (std::chrono::milliseconds timeToWait)
    {
        std::unique_lock lock (mutex);
        available.wait_for (lock, timeToWait, [this]
        {
            return ! pending.empty();
        });

        while (! pending.empty())
        {
            auto task = std::move (pending.front());
            pending.pop_front();

            lock.unlock();
            task();
            lock.lock();
        }
    }

private:
    std::mutex mutex;
    std::condition_variable available;
    std::deque<std::packaged_task<MCPToolResult()>> pending;
};

MCPToolResult toToolResult (const YuiAgentResult& answer)
{
    auto result = answer.isError ? MCPToolResult::fromError (answer.text) : MCPToolResult::fromText (answer.text);

    if (! answer.image.isEmpty())
        result.content.push_back (MCPContent::makeImage (answer.image, "image/png"));

    return result;
}

} // namespace

//==============================================================================

int runYuiMcpCommand (int argc, char** argv)
{
    if (argc > 2)
    {
        std::cerr << "invalid argument: " << argv[2] << "\n";
        return 2;
    }

    YuiAgentHost host;
    host.baseFolder = File::getCurrentWorkingDirectory();
    host.savesImages = true;

    // The device is asked for by the first call that draws, and kept for the next ones
    host.snapshot = [context = std::shared_ptr<GraphicsContext>()] (Component& component, double scale) mutable
    {
        if (context == nullptr)
            context = createYuiToolGraphicsContext();

        return context != nullptr ? component.snapshotToImage (*context, true, static_cast<float> (scale)) : Image();
    };

    MCPServer::Options options;
    options.serverName = "yup_yui_tool";
    options.instructions = getYuiAgentInstructions();

    MCPServer server (options);
    CallQueue calls;

    const auto tools = getYuiAgentTools (false);

    for (const auto& tool : *tools.getArray())
    {
        MCPToolDefinition definition;
        definition.name = tool["name"].toString();
        definition.description = tool["description"].toString();
        definition.inputSchema = tool["inputSchema"];

        server.registerTool (definition, [&calls, &host, name = definition.name] (const var& arguments)
        {
            return calls.run ([&]
            {
                return toToolResult (callYuiAgentTool (name, arguments, host));
            });
        });
    }

    auto transport = std::make_unique<MCPStdioServerTransport>();
    auto* stdio = transport.get();

    if (const auto started = server.start (std::move (transport)); started.failed())
    {
        std::cerr << started.getErrorMessage() << "\n";
        return 2;
    }

    // The client leaves by closing the input
    while (! stdio->waitForInputToEnd (0))
        calls.runPending (std::chrono::milliseconds (50));

    server.stop();
    return 0;
}
