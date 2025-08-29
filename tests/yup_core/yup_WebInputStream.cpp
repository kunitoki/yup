/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#include <yup_core/yup_core.h>

using namespace yup;

#if YUP_MAC

namespace
{

class SimpleHttpServer : public Thread
{
public:
    SimpleHttpServer()
        : Thread ("HttpTestServer")
    {
        serverSocket = std::make_unique<StreamingSocket>();
    }

    ~SimpleHttpServer() override
    {
        stop();
    }

    bool start (int port = 9876)
    {
        if (! serverSocket->createListener (port, "127.0.0.1"))
            return false;

        serverPort = serverSocket->getPort();
        startThread();
        return true;
    }

    void stop()
    {
        signalThreadShouldExit();

        if (serverSocket != nullptr)
            serverSocket->close();

        stopThread (1000);
    }

    int getPort() const { return serverPort; }

    String getBaseUrl() const
    {
        return "http://127.0.0.1:" + String (serverPort);
    }

    void run() override
    {
        while (! threadShouldExit())
        {
            std::unique_ptr<StreamingSocket> clientSocket (serverSocket->waitForNextConnection());

            if (clientSocket != nullptr && ! threadShouldExit())
                handleRequest (clientSocket.get());
        }
    }

private:
    void handleRequest (StreamingSocket* socket)
    {
        auto connectionStatus = socket->waitUntilReady (true, 1000);
        if (connectionStatus == -1)
            return;

        auto readHttpPayload = [] (StreamingSocket& connection) -> MemoryBlock
        {
            MemoryBlock payload;
            uint8 data[1024] = { 0 };

            while (true)
            {
                auto numBytesRead = connection.read (data, numElementsInArray (data), false);
                if (numBytesRead <= 0)
                    break;

                payload.append (data, static_cast<size_t> (numBytesRead));
            }

            return payload;
        };

        String request = readHttpPayload (*socket).toString();
        String response;

        if (request.startsWith ("GET / "))
        {
            response = createHttpResponse (200, "text/html", "<!DOCTYPE html><html><head><title>Test Page</title></head>"
                                                             "<body><h1>Hello World</h1><p>This is a test page.</p></body></html>");
        }
        else if (request.startsWith ("GET /api/test "))
        {
            response = createHttpResponse (200, "application/json", "{\"message\":\"Hello from API\",\"status\":\"success\"}");
        }
        else if (request.startsWith ("POST /api/echo "))
        {
            auto bodyStart = request.indexOf ("\r\n\r\n");
            String body = bodyStart >= 0 ? request.substring (bodyStart + 4) : "{}";

            response = createHttpResponse (200, "application/json", "{\"echo\":" + body.quoted() + ",\"method\":\"POST\"}");
        }
        else if (request.startsWith ("GET /headers "))
        {
            String customHeaders = "X-Test-Header: TestValue\r\n"
                                   "X-Custom: CustomValue\r\n";
            response = createHttpResponse (200, "text/plain", "Headers test", customHeaders);
        }
        else if (request.startsWith ("GET /large "))
        {
            String largeContent;
            for (int i = 0; i < 1000; ++i)
                largeContent += "This is line " + String (i) + " of the large response.\n";

            response = createHttpResponse (200, "text/plain", largeContent);
        }
        else if (request.startsWith ("GET /slow "))
        {
            Thread::sleep (100);
            response = createHttpResponse (200, "text/plain", "This response was delayed");
        }
        else
        {
            response = createHttpResponse (404, "text/plain", "Not Found");
        }

        socket->write (response.toRawUTF8(), (int) response.getNumBytesAsUTF8());
    }

    String createHttpResponse (int statusCode, const String& contentType, const String& content, const String& extraHeaders = {})
    {
        String statusText = (statusCode == 200) ? "OK" : (statusCode == 404) ? "Not Found"
                                                                             : "Error";

        String response = "HTTP/1.1 " + String (statusCode) + " " + statusText + "\r\n";
        response += "Content-Type: " + contentType + "\r\n";
        response += "Content-Length: " + String (content.getNumBytesAsUTF8()) + "\r\n";
        response += "Connection: close\r\n";

        if (extraHeaders.isNotEmpty())
            response += extraHeaders;

        response += "\r\n";
        response += content;

        return response;
    }

    std::unique_ptr<StreamingSocket> serverSocket;
    int serverPort = 0;
    std::atomic_bool isReady = false;
};
} // namespace

class WebInputStreamTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        server = std::make_unique<SimpleHttpServer>();
        ASSERT_TRUE (server->start()) << "Failed to start test HTTP server";

        while (! server->isThreadRunning())
            Thread::sleep (10);
    }

    void TearDown() override
    {
        server.reset();
    }

    int defaultTimeoutMs() const
    {
        return yup_isRunningUnderDebugger() ? -1 : 5000;
    }

    std::unique_ptr<SimpleHttpServer> server;
};

TEST_F (WebInputStreamTests, CanReadHtmlContent)
{
    URL url (server->getBaseUrl());
    WebInputStream stream (url, false);
    stream.withConnectionTimeout (defaultTimeoutMs());
    EXPECT_TRUE (stream.isError());

    auto content = stream.readEntireStreamAsString();
    ASSERT_FALSE (stream.isError());
    EXPECT_EQ (200, stream.getStatusCode());
    EXPECT_TRUE (content.containsIgnoreCase ("<!DOCTYPE"));
    EXPECT_TRUE (content.containsIgnoreCase ("Hello World"));
    EXPECT_TRUE (content.containsIgnoreCase ("test page"));
}

TEST_F (WebInputStreamTests, ResponseHeadersArePresent)
{
    URL url (server->getBaseUrl());
    WebInputStream stream (url, false);
    stream.withConnectionTimeout (defaultTimeoutMs());

    auto headers = stream.getResponseHeaders();
    ASSERT_FALSE (stream.isError());
    EXPECT_GT (headers.size(), 0);

    bool hasContentType = false;
    bool hasContentLength = false;

    for (const auto& headerName : headers.getAllKeys())
    {
        if (headerName.equalsIgnoreCase ("content-type"))
        {
            hasContentType = true;
            EXPECT_TRUE (headers.getValue (headerName, "").containsIgnoreCase ("text/html"));
        }

        if (headerName.equalsIgnoreCase ("content-length"))
            hasContentLength = true;
    }

    EXPECT_TRUE (hasContentType);
    EXPECT_TRUE (hasContentLength);
}

TEST_F (WebInputStreamTests, CustomHeadersInResponse)
{
    URL url (server->getBaseUrl() + "/headers");
    WebInputStream stream (url, false);
    stream.withConnectionTimeout (defaultTimeoutMs());

    auto headers = stream.getResponseHeaders();
    ASSERT_FALSE (stream.isError());

    bool hasTestHeader = false;
    bool hasCustomHeader = false;

    for (const auto& headerName : headers.getAllKeys())
    {
        if (headerName.equalsIgnoreCase ("X-Test-Header"))
        {
            hasTestHeader = true;
            EXPECT_EQ ("TestValue", headers.getValue (headerName, ""));
        }

        if (headerName.equalsIgnoreCase ("X-Custom"))
        {
            hasCustomHeader = true;
            EXPECT_EQ ("CustomValue", headers.getValue (headerName, ""));
        }
    }

    EXPECT_TRUE (hasTestHeader);
    EXPECT_TRUE (hasCustomHeader);
}

TEST_F (WebInputStreamTests, JsonApiEndpoint)
{
    URL url (server->getBaseUrl() + "/api/test");
    WebInputStream stream (url, false);
    stream.withConnectionTimeout (defaultTimeoutMs());

    String jsonResponse = stream.readEntireStreamAsString();
    ASSERT_FALSE (stream.isError());
    EXPECT_EQ (200, stream.getStatusCode());
    EXPECT_TRUE (jsonResponse.contains ("\"message\""));
    EXPECT_TRUE (jsonResponse.contains ("Hello from API"));
    EXPECT_TRUE (jsonResponse.contains ("\"status\":\"success\""));
}

TEST_F (WebInputStreamTests, DISABLED_PostRequestWithData)
{
    URL url (server->getBaseUrl() + "/api/echo");
    url = url.withPOSTData ("{\"test\":\"Hello POST\"}");

    WebInputStream stream (url, false);
    stream.withConnectionTimeout (defaultTimeoutMs());
    stream.withExtraHeaders ("Content-Type: application/json\r\n");

    String response = stream.readEntireStreamAsString();
    ASSERT_FALSE (stream.isError());
    EXPECT_EQ (200, stream.getStatusCode());
    EXPECT_TRUE (response.contains ("\"echo\""));
    EXPECT_TRUE (response.contains ("Hello POST"));
    EXPECT_TRUE (response.contains ("\"method\":\"POST\""));
}

TEST_F (WebInputStreamTests, HandlesNotFoundUrl)
{
    URL url (server->getBaseUrl() + "/nonexistent");
    WebInputStream stream (url, false);
    stream.withConnectionTimeout (defaultTimeoutMs());

    String response = stream.readEntireStreamAsString();
    EXPECT_FALSE (stream.isError());
    EXPECT_EQ (404, stream.getStatusCode());
    EXPECT_TRUE (response.contains ("Not Found"));
}

TEST_F (WebInputStreamTests, HandlesInvalidUrl)
{
    URL url ("http://127.0.0.1:99999");
    WebInputStream stream (url, false);
    stream.withConnectionTimeout (1000);

    EXPECT_TRUE (stream.isError());
    stream.readEntireStreamAsString();
    EXPECT_TRUE (stream.isError());
}

TEST_F (WebInputStreamTests, CanGetContentLength)
{
    URL url (server->getBaseUrl());
    WebInputStream stream (url, false);
    stream.withConnectionTimeout (defaultTimeoutMs());

    auto contentLength = stream.getTotalLength();
    ASSERT_FALSE (stream.isError());
    EXPECT_GT (contentLength, 0);

    String content = stream.readEntireStreamAsString();
    EXPECT_EQ (contentLength, content.getNumBytesAsUTF8());
}

TEST_F (WebInputStreamTests, StreamPositionWorks)
{
    URL url (server->getBaseUrl());
    WebInputStream stream (url, false);
    stream.withConnectionTimeout (defaultTimeoutMs());

    EXPECT_EQ (0, stream.getPosition());

    char buffer[100];
    auto bytesRead = stream.read (buffer, sizeof (buffer));
    ASSERT_FALSE (stream.isError());
    EXPECT_GT (bytesRead, 0);
    EXPECT_EQ (bytesRead, stream.getPosition());
}

TEST_F (WebInputStreamTests, MultipleReadsWork)
{
    URL url (server->getBaseUrl());
    WebInputStream stream (url, false);
    stream.withConnectionTimeout (defaultTimeoutMs());

    char buffer1[50];
    char buffer2[50];

    auto bytesRead1 = stream.read (buffer1, sizeof (buffer1));
    ASSERT_FALSE (stream.isError());
    auto bytesRead2 = stream.read (buffer2, sizeof (buffer2));
    ASSERT_FALSE (stream.isError());

    EXPECT_GT (bytesRead1, 0);
    EXPECT_GE (bytesRead2, 0);

    EXPECT_EQ (bytesRead1 + bytesRead2, stream.getPosition());

    if (bytesRead1 > 0 && bytesRead2 > 0)
    {
        String content1 (buffer1, bytesRead1);
        String content2 (buffer2, bytesRead2);
        EXPECT_NE (content1, content2);
    }
    else
    {
        FAIL();
    }
}

TEST_F (WebInputStreamTests, LargeContentHandling)
{
    URL url (server->getBaseUrl() + "/large");
    WebInputStream stream (url, false);
    stream.withConnectionTimeout (defaultTimeoutMs());

    String content = stream.readEntireStreamAsString();
    ASSERT_FALSE (stream.isError());
    EXPECT_GT (content.length(), 10000);
    EXPECT_TRUE (content.contains ("This is line 0"));
    EXPECT_TRUE (content.contains ("This is line 999"));
}

TEST_F (WebInputStreamTests, DISABLED_SlowResponseHandling)
{
    URL url (server->getBaseUrl() + "/slow");

    auto startTime = Time::getMillisecondCounter();
    WebInputStream stream (url, false);
    stream.withConnectionTimeout (defaultTimeoutMs());

    String content = stream.readEntireStreamAsString();
    auto endTime = Time::getMillisecondCounter();

    ASSERT_FALSE (stream.isError());
    EXPECT_TRUE (content.contains ("delayed"));
    EXPECT_GE (endTime - startTime, 100); // Should take at least 100ms due to server delay
}

#endif // YUP_MAC
