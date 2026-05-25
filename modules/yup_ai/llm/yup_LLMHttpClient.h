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

//==============================================================================
/** Abstract HTTP transport base for LLM provider clients.

    Provides the concrete HTTP POST + SSE streaming mechanics.  Subclasses
    supply the provider-specific pieces by overriding the pure virtual methods:

    - getEndpointUrl()       — full URL for non-streaming requests
    - buildHeaders()         — raw header string (key: value\\r\\n…)
    - buildPayload()         — JSON request body (non-streaming)
    - parseResponse()        — parse full JSON response → LLMResponse

    Three more methods have working defaults and may be overridden when the
    streaming request differs from the non-streaming one:

    - getStreamingEndpointUrl()  → getEndpointUrl()
    - buildStreamingPayload()    → buildPayload(request)
    - parseChunk()               → LLMResponse{} (empty / no-op chunk)

    Use LLMClientFactory::create() to obtain the correct concrete subclass
    for a given Provider enum value.

    @tags{AI}
*/
class YUP_API LLMHttpClient : public LLMClient
{
public:
    explicit LLMHttpClient (Options options);
    ~LLMHttpClient() override;

    LLMResponse complete (const Request& request) override;
    bool completeStreaming (const Request& request, ChunkCallback onChunk) override;

protected:
    //==============================================================================
    // Pure virtual — implement in every provider subclass.

    /** Returns the endpoint URL for non-streaming requests. */
    virtual String getEndpointUrl() const = 0;

    /** Returns the raw HTTP header string ("Key: Value\\r\\n" pairs). */
    virtual String buildHeaders() const = 0;

    /** Builds the JSON request body for a non-streaming request. */
    virtual String buildPayload (const Request& request) const = 0;

    /** Parses a complete JSON response into an LLMResponse. */
    virtual LLMResponse parseResponse (const var& json) const = 0;

    //==============================================================================
    // Virtual with sensible defaults — override when streaming differs.

    /** Returns the endpoint URL for streaming requests.
        Default: same as getEndpointUrl().
    */
    virtual String getStreamingEndpointUrl() const;

    /** Builds the JSON request body for a streaming request.
        Default: buildPayload(request) — override to add stream flags or pick a
        different endpoint body (e.g. OpenAI Chat adds "stream":true here).
    */
    virtual String buildStreamingPayload (const Request& request) const;

    /** Parses a single SSE data-line JSON object into a delta LLMResponse.
        Default: returns LLMResponse{} (empty chunk, safe no-op in accumulation).
    */
    virtual LLMResponse parseChunk (const var& json) const;

    //==============================================================================
    /** Normalises baseUrl + path, stripping a trailing slash from the base. */
    static String makeProviderUrl (const String& baseUrl, const String& path);

private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};

} // namespace yup
