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

#include <gtest/gtest.h>

#include <yup_ai/yup_ai.h>

#include <sstream>

using namespace yup;

#if ! YUP_WASM

class MCPStdioServerTransportTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        MCPToolDefinition echo;
        echo.name = "echo";
        echo.description = "Echoes text.";

        server.registerTool (echo, [] (const var& arguments)
        {
            return arguments["value"];
        });
    }

    /** Serves the lines of a client until they end, and returns the answers, one per line. */
    std::vector<var> serve (const std::string& lines)
    {
        std::istringstream input (lines);
        std::ostringstream output;

        auto newTransport = std::make_unique<MCPStdioServerTransport> (input, output);
        auto* transport = newTransport.get();

        EXPECT_TRUE (server.start (std::move (newTransport)).wasOk());
        EXPECT_TRUE (transport->waitForInputToEnd (5000));
        EXPECT_FALSE (transport->isConnected());

        server.stop();

        StringArray answerLines;
        answerLines.addLines (String (output.str()));
        answerLines.removeEmptyStrings();

        std::vector<var> answers;

        for (const auto& line : answerLines)
            answers.push_back (JSON::parse (line));

        return answers;
    }

    MCPServer server;
};

TEST_F (MCPStdioServerTransportTests, AnswersEachRequestOnALineOfItsOwn)
{
    const auto answers = serve (R"({"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"echo","arguments":{"value":"over\nstdio"}}})"
                                "\n"
                                R"({"jsonrpc":"2.0","id":2,"method":"ping"})"
                                "\n");

    ASSERT_EQ (2u, answers.size());
    EXPECT_EQ (1, static_cast<int> (answers[0]["id"]));
    EXPECT_EQ ("over\nstdio", answers[0]["result"]["content"][0]["text"].toString());
    EXPECT_EQ (2, static_cast<int> (answers[1]["id"]));
}

TEST_F (MCPStdioServerTransportTests, AnswersTheLastLineWithoutANewline)
{
    const auto answers = serve (R"({"jsonrpc":"2.0","id":7,"method":"ping"})");

    ASSERT_EQ (1u, answers.size());
    EXPECT_EQ (7, static_cast<int> (answers[0]["id"]));
}

TEST_F (MCPStdioServerTransportTests, NotificationsAndBlankLinesGetNoAnswer)
{
    const auto answers = serve ("\n"
                                R"({"jsonrpc":"2.0","method":"notifications/initialized"})"
                                "\n   \n"
                                R"({"jsonrpc":"2.0","id":3,"method":"ping"})"
                                "\n");

    ASSERT_EQ (1u, answers.size());
    EXPECT_EQ (3, static_cast<int> (answers[0]["id"]));
}

TEST_F (MCPStdioServerTransportTests, ReportsWhatIsNotAMessageAndKeepsServing)
{
    const auto answers = serve ("{ not json\n"
                                R"([{"jsonrpc":"2.0","id":1,"method":"ping"}])"
                                "\n"
                                R"({"jsonrpc":"2.0","id":4,"method":"ping"})"
                                "\n");

    ASSERT_EQ (3u, answers.size());
    EXPECT_EQ (MCPErrorCodes::parseError, static_cast<int> (answers[0]["error"]["code"]));
    EXPECT_EQ (MCPErrorCodes::invalidRequest, static_cast<int> (answers[1]["error"]["code"]));
    EXPECT_EQ (4, static_cast<int> (answers[2]["id"]));
}

TEST_F (MCPStdioServerTransportTests, StartsOnlyOnce)
{
    std::istringstream input;
    std::ostringstream output;
    MCPStdioServerTransport transport (input, output);

    EXPECT_FALSE (transport.isConnected());
    EXPECT_TRUE (transport.start().wasOk());
    EXPECT_TRUE (transport.waitForInputToEnd (5000));
    EXPECT_TRUE (transport.start().failed());
    EXPECT_TRUE (transport.receiveMessage().failed());
}

#endif
