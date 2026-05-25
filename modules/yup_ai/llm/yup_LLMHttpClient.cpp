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
namespace
{
bool shouldRetryAiStatus (int statusCode)
{
    return statusCode == 0 || statusCode == 408 || statusCode == 429 || statusCode >= 500;
}
} // namespace

struct LLMHttpClient::Pimpl
{
    explicit Pimpl (LLMHttpClient& ownerToUse)
        : owner (ownerToUse)
    {
    }

    LLMResponse complete (const Request& request)
    {
        const auto body = owner.buildPayload (request);
        const auto endpoint = owner.getEndpointUrl();
        const auto headers = owner.buildHeaders();

        for (int attempt = 0; attempt <= owner.options.maxRetries; ++attempt)
        {
            int statusCode = 0;
            auto url = URL (endpoint).withPOSTData (body);
            auto options = URL::InputStreamOptions (URL::ParameterHandling::inPostData)
                               .withExtraHeaders (headers)
                               .withConnectionTimeoutMs (owner.options.timeoutMs)
                               .withStatusCode (&statusCode)
                               .withHttpRequestCmd ("POST");

            auto stream = url.createInputStream (options);
            const auto responseBody = stream != nullptr ? stream->readEntireStreamAsString() : String();

            if (stream != nullptr && statusCode >= 200 && statusCode < 300)
                return owner.parseResponse (JSON::parse (responseBody));

            // Build a meaningful error from the body before deciding whether to retry.
            LLMResponse errorResponse;
            if (responseBody.isNotEmpty())
            {
                auto parsed = JSON::parse (responseBody);
                if (! parsed.isVoid())
                    errorResponse = owner.parseResponse (parsed);
            }

            if (errorResponse.failed())
            {
                if (! shouldRetryAiStatus (statusCode) || attempt == owner.options.maxRetries)
                    return errorResponse;
            }
            else
            {
                const auto msg = statusCode > 0
                                   ? "AI HTTP request failed with status " + String (statusCode)
                                   : "AI HTTP request failed";

                if (! shouldRetryAiStatus (statusCode) || attempt == owner.options.maxRetries)
                    return LLMResponse::fromError (msg);
            }
        }

        return LLMResponse::fromError ("AI HTTP request failed after retries");
    }

    bool completeStreaming (const Request& request, LLMHttpClient::ChunkCallback onChunk)
    {
        if (! onChunk)
            return false;

        const auto body = owner.buildStreamingPayload (request);
        const auto endpoint = owner.getStreamingEndpointUrl();
        const auto headers = owner.buildHeaders();

        for (int attempt = 0; attempt <= owner.options.maxRetries; ++attempt)
        {
            int statusCode = 0;
            auto url = URL (endpoint).withPOSTData (body);
            auto options = URL::InputStreamOptions (URL::ParameterHandling::inPostData)
                               .withExtraHeaders (headers)
                               .withConnectionTimeoutMs (owner.options.timeoutMs)
                               .withStatusCode (&statusCode)
                               .withHttpRequestCmd ("POST");

            auto stream = url.createInputStream (options);

            if (stream != nullptr && statusCode >= 200 && statusCode < 300)
            {
                LLMResponse accumulatedResponse;

                while (! stream->isExhausted())
                {
                    auto line = stream->readNextLine().trim();

                    if (! line.startsWith ("data:"))
                        continue;

                    auto payload = line.substring (5).trim();
                    if (payload == "[DONE]")
                        return true;

                    auto parsed = JSON::parse (payload);
                    auto chunk = owner.parseChunk (parsed);

                    accumulatedResponse.appendStreamChunk (chunk);
                    onChunk (accumulatedResponse);

                    if (chunk.failed())
                        return false;
                }

                return true;
            }

            if (! shouldRetryAiStatus (statusCode) || attempt == owner.options.maxRetries)
                break;
        }

        return false;
    }

    LLMHttpClient& owner;
};

//==============================================================================
LLMHttpClient::LLMHttpClient (Options options)
    : LLMClient (std::move (options))
    , pimpl (std::make_unique<Pimpl> (*this))
{
}

LLMHttpClient::~LLMHttpClient() = default;

LLMResponse LLMHttpClient::complete (const Request& request)
{
    return pimpl->complete (request);
}

bool LLMHttpClient::completeStreaming (const Request& request, ChunkCallback onChunk)
{
    return pimpl->completeStreaming (request, std::move (onChunk));
}

//==============================================================================
String LLMHttpClient::makeProviderUrl (const String& baseUrl, const String& path)
{
    return baseUrl.endsWithChar ('/') ? baseUrl.dropLastCharacters (1) + path
                                      : baseUrl + path;
}

String LLMHttpClient::getStreamingEndpointUrl() const
{
    return getEndpointUrl();
}

String LLMHttpClient::buildStreamingPayload (const Request& request) const
{
    return buildPayload (request);
}

LLMResponse LLMHttpClient::parseChunk (const var& /*json*/) const
{
    return LLMResponse {};
}

} // namespace yup
