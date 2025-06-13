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

namespace yup
{

void MACAddress::findAllAddresses (Array<MACAddress>& result)
{
    result.clearQuick();
}

bool YUP_CALLTYPE Process::openEmailWithAttachments (const String& /* targetEmailAddress */,
                                                     const String& /* emailSubject */,
                                                     const String& /* bodyText */,
                                                     const StringArray& /* filesToAttach */)
{
    jassertfalse; // xxx todo
    return false;
}

//==============================================================================
#if YUP_EMSCRIPTEN && ! YUP_USE_CURL
class WebInputStream::Pimpl
{
public:
    Pimpl (WebInputStream& pimplOwner, const URL& urlToCopy, bool addParametersToBody)
        : owner (pimplOwner)
        , url (urlToCopy)
        , addParametersToRequestBody (addParametersToBody)
        , hasBodyDataToSend (addParametersToRequestBody || url.hasBodyDataToSend())
        , httpRequestCmd (hasBodyDataToSend ? "POST" : "GET")
    {
    }

    ~Pimpl()
    {
        cancel();
    }

    //==============================================================================
    // WebInputStream methods
    void withExtraHeaders (const String& extraHeaders)
    {
        if (! headers.endsWithChar ('\n') && headers.isNotEmpty())
            headers << "\r\n";

        headers << extraHeaders;

        if (! headers.endsWithChar ('\n') && headers.isNotEmpty())
            headers << "\r\n";
    }

    void withCustomRequestCommand (const String& customRequestCommand) { httpRequestCmd = customRequestCommand; }

    void withConnectionTimeout (int timeoutInMs) { timeOutMs = timeoutInMs; }

    void withNumRedirectsToFollow (int maxRedirectsToFollow) { numRedirectsToFollow = maxRedirectsToFollow; }

    int getStatusCode() const { return statusCode; }

    StringPairArray getRequestHeaders() const { return WebInputStream::parseHttpHeaders (headers); }

    StringPairArray getResponseHeaders() const
    {
        return responseHeaders;
    }

    bool connect (WebInputStream::Listener* listener)
    {
        if (hasBeenCancelled)
            return false;

        address = url.toString (! addParametersToRequestBody);

        statusCode = 0;
        finished = false;
        position = 0;
        totalBytesRead = 0;

        // Prepare the fetch attributes
        emscripten_fetch_attr_init (&fetchAttr);

        std::strncpy (fetchAttr.requestMethod, httpRequestCmd.toRawUTF8(), sizeof (fetchAttr.requestMethod) - 1);
        fetchAttr.requestMethod[sizeof (fetchAttr.requestMethod) - 1] = '\0';

        // Handle headers
        headerData = parseHeaders (headers);
        fetchAttr.requestHeaders = headerData.data();

        // Handle POST data
        if (hasBodyDataToSend)
        {
            WebInputStream::createHeadersAndPostData (url, headers, postData, addParametersToRequestBody);
            fetchAttr.requestData = (const char*) postData.getData();
            fetchAttr.requestDataSize = postData.getSize();
        }

        fetchAttr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
        fetchAttr.onsuccess = &onFetchSuccess;
        fetchAttr.onprogress = &onFetchProgress;
        fetchAttr.onerror = &onFetchError;
        fetchAttr.userData = this;

        // Start the fetch request
        fetchHandle = emscripten_fetch (&fetchAttr, address.toRawUTF8());

        return true;
    }

    void cancel()
    {
        hasBeenCancelled = true;
        finished = true;

        if (fetchHandle)
        {
            emscripten_fetch_close (fetchHandle);
            fetchHandle = nullptr;
        }
    }

    //==============================================================================
    bool isError() const { return fetchHandle == nullptr || statusCode == 0; }

    bool isExhausted() { return finished && position >= contentLength; }

    int64 getPosition() { return position; }

    int64 getTotalLength() { return contentLength; }

    int read (void* buffer, int bytesToRead)
    {
        if (isError() || totalBytesRead >= contentLength)
            return 0;

        int bytesAvailable = (int) (dataBuffer.size() - position);
        int bytesToCopy = jmin (bytesToRead, bytesAvailable);

        if (bytesToCopy > 0)
        {
            std::memcpy (buffer, dataBuffer.data() + position, bytesToCopy);
            position += bytesToCopy;
            totalBytesRead += bytesToCopy;
            return bytesToCopy;
        }

        // No data available yet, return 0
        return 0;
    }

    bool setPosition (int64 wantedPos)
    {
        if (isError())
            return false;

        if (wantedPos >= 0 && wantedPos <= (int64) dataBuffer.size())
        {
            position = wantedPos;
            return true;
        }

        return false;
    }

    //==============================================================================
    int statusCode = 0;

private:
    WebInputStream& owner;
    URL url;
    StringArray headerLines;
    String address, headers;
    MemoryBlock postData;
    int64 contentLength = -1, position = 0, totalBytesRead = 0;
    bool finished = false;
    const bool addParametersToRequestBody, hasBodyDataToSend;
    int timeOutMs = 0;
    int numRedirectsToFollow = 5;
    String httpRequestCmd;
    bool hasBeenCancelled = false;

    emscripten_fetch_attr_t fetchAttr;
    emscripten_fetch_t* fetchHandle = nullptr;
    std::vector<char> dataBuffer;
    StringPairArray responseHeaders;
    std::vector<const char*> headerData; // to keep header strings alive

    // Callback for successful fetch
    static void onFetchSuccess (emscripten_fetch_t* fetch)
    {
        Pimpl* self = static_cast<Pimpl*> (fetch->userData);

        self->statusCode = fetch->status;
        self->contentLength = fetch->numBytes;
        self->dataBuffer.assign (fetch->data, fetch->data + fetch->numBytes);
        self->finished = true;

        // Parse response headers
        if (auto headersSize = emscripten_fetch_get_response_headers_length (fetch); headersSize > 0)
        {
            std::vector<char> headersBuffer (headersSize + 1);
            emscripten_fetch_get_response_headers (fetch, headersBuffer.data(), headersBuffer.size());
            headersBuffer.back() = '\0';

            self->parseResponseHeaders (headersBuffer.data());
        }

        // Close the fetch
        emscripten_fetch_close (fetch);
        self->fetchHandle = nullptr;
    }

    // Callback for fetch error
    static void onFetchError (emscripten_fetch_t* fetch)
    {
        Pimpl* self = static_cast<Pimpl*> (fetch->userData);

        self->statusCode = fetch->status;
        self->finished = true;

        // Close the fetch
        emscripten_fetch_close (fetch);
        self->fetchHandle = nullptr;
    }

    // Callback for fetch progress (optional)
    static void onFetchProgress (emscripten_fetch_t* fetch)
    {
        // We can implement progress updates here if needed
        // For now, we can leave this empty or implement listener callbacks
    }

    // Helper to parse headers into the format expected by emscripten_fetch
    std::vector<const char*> parseHeaders (const String& headersString)
    {
        StringArray headerLines = StringArray::fromLines (headersString.trim());

        std::vector<const char*> headerArray;
        for (const auto& line : headerLines)
        {
            int colonPos = line.indexOfChar (':');
            if (colonPos > 0)
            {
                String key = line.substring (0, colonPos).trim();
                String value = line.substring (colonPos + 1).trim();

                // Store the key and value strings
                headerKeyValues.emplace_back (key.toRawUTF8());
                headerKeyValues.emplace_back (value.toRawUTF8());

                // Store pointers to the strings
                headerArray.push_back (headerKeyValues[headerKeyValues.size() - 2].c_str());
                headerArray.push_back (headerKeyValues.back().c_str());
            }
        }

        headerArray.push_back (nullptr);
        return headerArray;
    }

    std::vector<std::string> headerKeyValues; // To keep header strings alive

    // Helper to parse response headers
    void parseResponseHeaders (const char* headers)
    {
        if (headers == nullptr)
            return;

        String headersString (headers);
        StringArray lines;
        lines.addLines (headersString);

        for (const auto& line : lines)
        {
            int colonPos = line.indexOfChar (':');
            if (colonPos > 0)
            {
                String key = line.substring (0, colonPos).trim();
                String value = line.substring (colonPos + 1).trim();

                auto previousValue = responseHeaders[key];
                responseHeaders.set (key, previousValue.isEmpty() ? value : (previousValue + "," + value));
            }
        }
    }

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl)
};

std::unique_ptr<URL::DownloadTask> URL::downloadToFile (const File& targetLocation, const DownloadTaskOptions& options)
{
    return URL::DownloadTask::createFallbackDownloader (*this, targetLocation, options);
}
#endif

} // namespace yup
