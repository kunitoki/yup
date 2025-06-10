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

//==============================================================================
#include "generated/yup_YupHTTPStream_bytecode.h"
#define javaYupHttpStream yup::javaYupHTTPStreamBytecode

//==============================================================================
#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    METHOD (constructor, "<init>", "()V")                                     \
    METHOD (toString, "toString", "()Ljava/lang/String;")

DECLARE_JNI_CLASS (StringBuffer, "java/lang/StringBuffer")
#undef JNI_CLASS_MEMBERS

//==============================================================================
#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK)                                                                                                      \
    STATICMETHOD (createHTTPStream, "createHTTPStream", "(Ljava/lang/String;Z[BLjava/lang/String;I[ILjava/lang/StringBuffer;ILjava/lang/String;)Lorg/kunitoki/yup/YupHTTPStream;") \
    METHOD (connect, "connect", "()Z")                                                                                                                                             \
    METHOD (release, "release", "()V")                                                                                                                                             \
    METHOD (read, "read", "([BI)I")                                                                                                                                                \
    METHOD (getPosition, "getPosition", "()J")                                                                                                                                     \
    METHOD (getTotalLength, "getTotalLength", "()J")                                                                                                                               \
    METHOD (isExhausted, "isExhausted", "()Z")                                                                                                                                     \
    METHOD (setPosition, "setPosition", "(J)Z")

DECLARE_JNI_CLASS_WITH_BYTECODE (HTTPStream, "org/kunitoki/yup/YupHTTPStream", 16, javaYupHttpStream)
#undef JNI_CLASS_MEMBERS

//==============================================================================
#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    METHOD (acquire, "acquire", "()V")                                        \
    METHOD (release, "release", "()V")

DECLARE_JNI_CLASS (AndroidMulticastLock, "android/net/wifi/WifiManager$MulticastLock")
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    METHOD (createMulticastLock, "createMulticastLock", "(Ljava/lang/String;)Landroid/net/wifi/WifiManager$MulticastLock;")

DECLARE_JNI_CLASS (AndroidWifiManager, "android/net/wifi/WifiManager")
#undef JNI_CLASS_MEMBERS

static jobject getMulticastLock()
{
    static GlobalRef multicastLock = [&]
    {
        auto* env = getEnv();

        LocalRef<jobject> wifiManager (env->CallObjectMethod (getAppContext().get(),
                                                              AndroidContext.getSystemService,
                                                              javaString ("wifi").get()));

        if (wifiManager == nullptr)
            return GlobalRef {};

        return GlobalRef (LocalRef<jobject> (env->CallObjectMethod (wifiManager.get(),
                                                                    AndroidWifiManager.createMulticastLock,
                                                                    javaString ("YUP_MulticastLock").get())));
    }();

    return multicastLock.get();
}

YUP_API void YUP_CALLTYPE acquireMulticastLock();

YUP_API void YUP_CALLTYPE acquireMulticastLock()
{
    auto multicastLock = getMulticastLock();

    if (multicastLock != nullptr)
        getEnv()->CallVoidMethod (multicastLock, AndroidMulticastLock.acquire);
}

YUP_API void YUP_CALLTYPE releaseMulticastLock();

YUP_API void YUP_CALLTYPE releaseMulticastLock()
{
    auto multicastLock = getMulticastLock();

    if (multicastLock != nullptr)
        getEnv()->CallVoidMethod (multicastLock, AndroidMulticastLock.release);
}

//==============================================================================
void MACAddress::findAllAddresses (Array<MACAddress>& /*result*/)
{
    // TODO
}

YUP_API bool YUP_CALLTYPE Process::openEmailWithAttachments (const String& /*targetEmailAddress*/,
                                                             const String& /*emailSubject*/,
                                                             const String& /*bodyText*/,
                                                             const StringArray& /*filesToAttach*/)
{
    // TODO
    return false;
}

//==============================================================================
bool URL::isLocalFile() const
{
    if (getScheme() == "file")
        return true;

    if (getScheme() == "content")
    {
        auto file = AndroidContentUriResolver::getLocalFileFromContentUri (*this);
        return (file != File());
    }

    return false;
}

File URL::getLocalFile() const
{
    if (getScheme() == "content")
    {
        auto path = AndroidContentUriResolver::getLocalFileFromContentUri (*this);

        // This URL does not refer to a local file
        // Call URL::isLocalFile to first check if the URL
        // refers to a local file.
        jassert (path != File());

        return path;
    }

    return fileFromFileSchemeURL (*this);
}

String URL::getFileName() const
{
    if (getScheme() == "content")
        return AndroidContentUriResolver::getFileNameFromContentUri (*this);

    return toString (false).fromLastOccurrenceOf ("/", false, true);
}

//==============================================================================
class WebInputStream::Pimpl
{
public:
    enum
    {
        contentStreamCacheSize = 1024
    };

    Pimpl (WebInputStream&, const URL& urlToCopy, bool addParametersToBody)
        : url (urlToCopy)
        , isContentURL (urlToCopy.getScheme() == "content")
        , addParametersToRequestBody (addParametersToBody)
        , hasBodyDataToSend (addParametersToRequestBody || url.hasBodyDataToSend())
        , httpRequest (hasBodyDataToSend ? "POST" : "GET")
    {
    }

    ~Pimpl()
    {
        cancel();
    }

    void cancel()
    {
        if (isContentURL)
        {
            stream.callVoidMethod (AndroidInputStream.close);
            return;
        }

        const ScopedLock lock (createStreamLock);

        if (stream != nullptr)
        {
            stream.callVoidMethod (HTTPStream.release);
            stream.clear();
        }

        hasBeenCancelled = true;
    }

    bool connect (WebInputStream::Listener* /*listener*/)
    {
        auto* env = getEnv();

        if (isContentURL)
        {
            GlobalRef urlRef { urlToUri (url) };
            auto inputStream = AndroidStreamHelpers::createStream (urlRef, AndroidStreamHelpers::StreamKind::input);

            if (inputStream != nullptr)
            {
                stream = GlobalRef (inputStream);
                statusCode = 200;

                return true;
            }
        }
        else
        {
            String address = url.toString (! addParametersToRequestBody);

            if (! address.contains ("://"))
                address = "http://" + address;

            MemoryBlock postData;

            if (hasBodyDataToSend)
                WebInputStream::createHeadersAndPostData (url,
                                                          headers,
                                                          postData,
                                                          addParametersToRequestBody);

            jbyteArray postDataArray = nullptr;

            if (! postData.isEmpty())
            {
                postDataArray = env->NewByteArray (static_cast<jsize> (postData.getSize()));
                env->SetByteArrayRegion (postDataArray, 0, static_cast<jsize> (postData.getSize()), (const jbyte*) postData.getData());
            }

            LocalRef<jobject> responseHeaderBuffer (env->NewObject (StringBuffer, StringBuffer.constructor));

            // Annoyingly, the android HTTP functions will choke on this call if you try to do it on the message
            // thread. You'll need to move your networking code to a background thread to keep it happy..
            jassert (Thread::getCurrentThread() != nullptr);

            jintArray statusCodeArray = env->NewIntArray (1);
            jassert (statusCodeArray != nullptr);

            {
                const ScopedLock lock (createStreamLock);

                if (! hasBeenCancelled)
                    stream = GlobalRef (LocalRef<jobject> (env->CallStaticObjectMethod (HTTPStream,
                                                                                        HTTPStream.createHTTPStream,
                                                                                        javaString (address).get(),
                                                                                        (jboolean) addParametersToRequestBody,
                                                                                        postDataArray,
                                                                                        javaString (headers).get(),
                                                                                        (jint) timeOutMs,
                                                                                        statusCodeArray,
                                                                                        responseHeaderBuffer.get(),
                                                                                        (jint) numRedirectsToFollow,
                                                                                        javaString (httpRequest).get())));
            }

            if (stream != nullptr && ! stream.callBooleanMethod (HTTPStream.connect))
                stream.clear();

            jint* const statusCodeElements = env->GetIntArrayElements (statusCodeArray, nullptr);
            statusCode = statusCodeElements[0];
            env->ReleaseIntArrayElements (statusCodeArray, statusCodeElements, 0);
            env->DeleteLocalRef (statusCodeArray);

            if (postDataArray != nullptr)
                env->DeleteLocalRef (postDataArray);

            if (stream != nullptr)
            {
                StringArray headerLines;

                {
                    LocalRef<jstring> headersString ((jstring) env->CallObjectMethod (responseHeaderBuffer.get(),
                                                                                      StringBuffer.toString));
                    headerLines.addLines (yupString (env, headersString));
                }

                for (int i = 0; i < headerLines.size(); ++i)
                {
                    const String& header = headerLines[i];
                    const String key (header.upToFirstOccurrenceOf (": ", false, false));
                    const String value (header.fromFirstOccurrenceOf (": ", false, false));
                    const String previousValue (responseHeaders[key]);

                    responseHeaders.set (key, previousValue.isEmpty() ? value : (previousValue + "," + value));
                }

                return true;
            }
        }

        return false;
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

    void withCustomRequestCommand (const String& customRequestCommand) { httpRequest = customRequestCommand; }

    void withConnectionTimeout (int timeoutInMs) { timeOutMs = timeoutInMs; }

    void withNumRedirectsToFollow (int maxRedirectsToFollow) { numRedirectsToFollow = maxRedirectsToFollow; }

    StringPairArray getRequestHeaders() const { return WebInputStream::parseHttpHeaders (headers); }

    StringPairArray getResponseHeaders() const { return responseHeaders; }

    int getStatusCode() const { return statusCode; }

    //==============================================================================
    bool isError() const { return stream == nullptr; }

    bool isExhausted() { return (isContentURL ? eofStreamReached : stream != nullptr && stream.callBooleanMethod (HTTPStream.isExhausted)); }

    int64 getTotalLength() { return (isContentURL ? -1 : (stream != nullptr ? stream.callLongMethod (HTTPStream.getTotalLength) : 0)); }

    int64 getPosition() { return (isContentURL ? readPosition : (stream != nullptr ? stream.callLongMethod (HTTPStream.getPosition) : 0)); }

    //==============================================================================
    bool setPosition (int64 wantedPos)
    {
        if (isContentURL)
        {
            if (wantedPos < readPosition)
                return false;

            auto bytesToSkip = wantedPos - readPosition;

            if (bytesToSkip == 0)
                return true;

            HeapBlock<char> buffer (bytesToSkip);

            return (read (buffer.getData(), (int) bytesToSkip) > 0);
        }

        return stream != nullptr && stream.callBooleanMethod (HTTPStream.setPosition, (jlong) wantedPos);
    }

    int read (void* buffer, int bytesToRead)
    {
        jassert (buffer != nullptr && bytesToRead >= 0);

        const ScopedLock lock (createStreamLock);

        if (stream == nullptr)
            return 0;

        JNIEnv* env = getEnv();

        jbyteArray javaArray = env->NewByteArray (bytesToRead);

        auto numBytes = (isContentURL ? stream.callIntMethod (AndroidInputStream.read, javaArray, 0, (jint) bytesToRead)
                                      : stream.callIntMethod (HTTPStream.read, javaArray, (jint) bytesToRead));

        if (numBytes > 0)
            env->GetByteArrayRegion (javaArray, 0, numBytes, static_cast<jbyte*> (buffer));

        env->DeleteLocalRef (javaArray);

        readPosition += jmax (0, numBytes);

        if (numBytes == -1)
            eofStreamReached = true;

        return numBytes;
    }

    //==============================================================================
    int statusCode = 0;

private:
    const URL url;
    const bool isContentURL, addParametersToRequestBody, hasBodyDataToSend;
    bool eofStreamReached = false;
    int numRedirectsToFollow = 5, timeOutMs = 0;
    String httpRequest, headers;
    StringPairArray responseHeaders;
    CriticalSection createStreamLock;
    bool hasBeenCancelled = false;
    int readPosition = 0;

    GlobalRef stream;
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl)
};

std::unique_ptr<URL::DownloadTask> URL::downloadToFile (const File& targetLocation, const DownloadTaskOptions& options)
{
    return URL::DownloadTask::createFallbackDownloader (*this, targetLocation, options);
}

//==============================================================================
#if __ANDROID_API__ < 24 // Android support for getifadds was added in Android 7.0 (API 24) so the posix implementation does not apply

static IPAddress makeAddress (const sockaddr_in* addr_in)
{
    if (addr_in->sin_addr.s_addr == INADDR_NONE)
        return {};

    return IPAddress (ntohl (addr_in->sin_addr.s_addr));
}

struct InterfaceInfo
{
    IPAddress interfaceAddress, broadcastAddress;
};

static Array<InterfaceInfo> findIPAddresses (int dummySocket)
{
    ifconf cfg;
    HeapBlock<char> buffer;
    int bufferSize = 1024;

    do
    {
        bufferSize *= 2;
        buffer.calloc (bufferSize);

        cfg.ifc_len = bufferSize;
        cfg.ifc_buf = buffer;

        if (ioctl (dummySocket, SIOCGIFCONF, &cfg) < 0 && errno != EINVAL)
            return {};

    } while (bufferSize < cfg.ifc_len + 2 * (int) (IFNAMSIZ + sizeof (struct sockaddr_in6)));

    Array<InterfaceInfo> result;

    for (size_t i = 0; i < (size_t) cfg.ifc_len / (size_t) sizeof (struct ifreq); ++i)
    {
        auto& item = cfg.ifc_req[i];

        if (item.ifr_addr.sa_family == AF_INET)
        {
            InterfaceInfo info;
            info.interfaceAddress = makeAddress (reinterpret_cast<const sockaddr_in*> (&item.ifr_addr));

            if (! info.interfaceAddress.isNull())
            {
                if (ioctl (dummySocket, SIOCGIFBRDADDR, &item) == 0)
                    info.broadcastAddress = makeAddress (reinterpret_cast<const sockaddr_in*> (&item.ifr_broadaddr));

                result.add (info);
            }
        }
        else if (item.ifr_addr.sa_family == AF_INET6)
        {
            // TODO: IPv6
        }
    }

    return result;
}

static Array<InterfaceInfo> findIPAddresses()
{
    auto dummySocket = socket (AF_INET, SOCK_DGRAM, 0); // a dummy socket to execute the IO control

    if (dummySocket < 0)
        return {};

    auto result = findIPAddresses (dummySocket);
    ::close (dummySocket);
    return result;
}

void IPAddress::findAllAddresses (Array<IPAddress>& result, bool /*includeIPv6*/)
{
    for (auto& a : findIPAddresses())
        result.add (a.interfaceAddress);
}

IPAddress IPAddress::getInterfaceBroadcastAddress (const IPAddress& address)
{
    for (auto& a : findIPAddresses())
        if (a.interfaceAddress == address)
            return a.broadcastAddress;

    return {};
}

#endif

} // namespace yup
