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

struct FallbackDownloadTask final : public URL::DownloadTask
    , public Thread
{
    FallbackDownloadTask (std::unique_ptr<FileOutputStream> outputStreamToUse,
                          size_t bufferSizeToUse,
                          std::unique_ptr<WebInputStream> streamToUse,
                          URL::DownloadTask::Listener* listenerToUse)
        : Thread ("DownloadTask thread")
        , fileStream (std::move (outputStreamToUse))
        , stream (std::move (streamToUse))
        , bufferSize (bufferSizeToUse)
        , buffer (bufferSize)
        , listener (listenerToUse)
    {
        jassert (fileStream != nullptr);
        jassert (stream != nullptr);

        targetLocation = fileStream->getFile();
        contentLength = stream->getTotalLength();
        httpCode = stream->getStatusCode();

        startThread();
    }

    ~FallbackDownloadTask() override
    {
        signalThreadShouldExit();
        stream->cancel();
        waitForThreadToExit (-1);
    }

    //==============================================================================
    void run() override
    {
        while (! (stream->isExhausted() || stream->isError() || threadShouldExit()))
        {
            if (listener != nullptr)
                listener->progress (this, downloaded, contentLength);

            auto max = (int) jmin ((int64) bufferSize, contentLength < 0 ? std::numeric_limits<int64>::max() : static_cast<int64> (contentLength - downloaded));

            auto actual = stream->read (buffer.get(), max);

            if (actual < 0 || threadShouldExit() || stream->isError())
                break;

            if (! fileStream->write (buffer.get(), static_cast<size_t> (actual)))
            {
                error = true;
                break;
            }

            downloaded += actual;

            if (downloaded == contentLength)
                break;
        }

        fileStream.reset();

        if (threadShouldExit() || stream->isError())
            error = true;

        if (contentLength > 0 && downloaded < contentLength)
            error = true;

        finished = true;

        if (listener != nullptr && ! threadShouldExit())
            listener->finished (this, ! error);
    }

    //==============================================================================
    std::unique_ptr<FileOutputStream> fileStream;
    const std::unique_ptr<WebInputStream> stream;
    const size_t bufferSize;
    HeapBlock<char> buffer;
    URL::DownloadTask::Listener* const listener;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FallbackDownloadTask)
};

void URL::DownloadTaskListener::progress (DownloadTask*, int64, int64) {}

//==============================================================================
std::unique_ptr<URL::DownloadTask> URL::DownloadTask::createFallbackDownloader (const URL& urlToUse,
                                                                                const File& targetFileToUse,
                                                                                const DownloadTaskOptions& options)
{
    const size_t bufferSize = 0x8000;
    targetFileToUse.deleteFile();

    if (auto outputStream = targetFileToUse.createOutputStream (bufferSize))
    {
        auto stream = std::make_unique<WebInputStream> (urlToUse, options.usePost);
        stream->withExtraHeaders (options.extraHeaders);

        if (stream->connect (nullptr))
            return std::make_unique<FallbackDownloadTask> (std::move (outputStream),
                                                           bufferSize,
                                                           std::move (stream),
                                                           options.listener);
    }

    return nullptr;
}

URL::DownloadTask::DownloadTask() {}

URL::DownloadTask::~DownloadTask() {}

//==============================================================================
URL::URL() {}

URL::URL (const String& u)
    : url (u)
{
    init();
}

URL::URL (File localFile)
{
    if (localFile == File())
        return;

#if YUP_WINDOWS
    bool isUncPath = localFile.getFullPathName().startsWith ("\\\\");
#endif

    while (! localFile.isRoot())
    {
        url = "/" + addEscapeChars (localFile.getFileName(), false) + url;
        localFile = localFile.getParentDirectory();
    }

    url = addEscapeChars (localFile.getFileName(), false) + url;

#if YUP_WINDOWS
    if (isUncPath)
    {
        url = url.fromFirstOccurrenceOf ("/", false, false);
    }
    else
#endif
    {
        if (! url.startsWithChar (L'/'))
            url = "/" + url;
    }

    url = "file://" + url;

    jassert (isWellFormed());
}

URL::URL (const URL& other)
    : url (other.url)
    , postData (other.postData)
    , parameterNames (other.parameterNames)
    , parameterValues (other.parameterValues)
    , anchor (other.anchor)
    , filesToUpload (other.filesToUpload)
{
}

URL::URL (URL&& other)
    : url (std::move (other.url))
    , postData (std::move (other.postData))
    , parameterNames (std::move (other.parameterNames))
    , parameterValues (std::move (other.parameterValues))
    , anchor (std::move (other.anchor))
    , filesToUpload (std::move (other.filesToUpload))
{
}

void URL::init()
{
    auto i = url.indexOfChar ('#');

    if (i >= 0)
    {
        anchor = removeEscapeChars (url.substring (i + 1));
        url = url.upToFirstOccurrenceOf ("#", false, false);
    }

    i = url.indexOfChar ('?');

    if (i >= 0)
    {
        do
        {
            auto nextAmp = url.indexOfChar (i + 1, '&');
            auto equalsPos = url.indexOfChar (i + 1, '=');

            if (nextAmp < 0)
            {
                addParameter (removeEscapeChars (equalsPos < 0 ? url.substring (i + 1) : url.substring (i + 1, equalsPos)),
                              equalsPos < 0 ? String() : removeEscapeChars (url.substring (equalsPos + 1)));
            }
            else if (equalsPos < 0 || equalsPos > nextAmp)
            {
                addParameter (removeEscapeChars (url.substring (i + 1, nextAmp)), String());
            }
            else
            {
                addParameter (removeEscapeChars (url.substring (i + 1, equalsPos)),
                              removeEscapeChars (url.substring (equalsPos + 1, nextAmp)));
            }

            i = nextAmp;
        } while (i >= 0);

        url = url.upToFirstOccurrenceOf ("?", false, false);
    }
}

URL::URL (const String& u, int)
    : url (u)
{
}

URL URL::createWithoutParsing (const String& u)
{
    return URL (u, 0);
}

bool URL::operator== (const URL& other) const
{
    return url == other.url
        && postData == other.postData
        && parameterNames == other.parameterNames
        && parameterValues == other.parameterValues
        && filesToUpload == other.filesToUpload;
}

bool URL::operator!= (const URL& other) const
{
    return ! operator== (other);
}

namespace URLHelpers
{

//==============================================================================
static String getMangledParameters (const URL& url)
{
    jassert (url.getParameterNames().size() == url.getParameterValues().size());
    String p;

    for (int i = 0; i < url.getParameterNames().size(); ++i)
    {
        if (i > 0)
            p << '&';

        auto val = url.getParameterValues()[i];

        p << URL::addEscapeChars (url.getParameterNames()[i], true);

        if (val.isNotEmpty())
            p << '=' << URL::addEscapeChars (val, true);
    }

    return p;
}

static Range<int> findSchemeRange (const String& url)
{
    int i = 0;
    const int length = url.length();

    // First character must be a letter
    if (i >= length || ! CharacterFunctions::isLetter (url[i]))
        return {};

    // Find end of scheme
    while (i < length)
    {
        auto ch = url[i];
        if (ch == ':')
            return Range<int> (0, i);

        if (! CharacterFunctions::isLetterOrDigit (ch) && ch != '+' && ch != '-' && ch != '.')
            break;

        ++i;
    }

    return {};
}

static Range<int> findAuthorityRange (const String& url)
{
    auto schemeRange = findSchemeRange (url);
    if (schemeRange.isEmpty())
        return {};

    int start = schemeRange.getEnd();
    const int length = url.length();

    // Check for "://" after scheme
    if (start + 3 > length || url.substring (start, start + 3) != "://")
        return {};

    start += 3; // Skip "://"

    // Find end of authority (next '/', '?', '#' or end of string)
    int end = start;
    while (end < length)
    {
        auto ch = url[end];
        if (ch == '/' || ch == '?' || ch == '#')
            break;
        ++end;
    }

    return end > start ? Range<int> (start, end) : Range<int>();
}

static Range<int> findPathRange (const String& url)
{
    auto schemeRange = findSchemeRange (url);
    if (schemeRange.isEmpty())
        return {};

    auto scheme = url.substring (schemeRange.getStart(), schemeRange.getEnd()).toLowerCase();

    // Special handling for file URLs with three slashes (file:///)
    if (scheme == "file" && url.substring (schemeRange.getEnd()).startsWith (":///"))
    {
        int start = schemeRange.getEnd() + 3; // Position after ":///"
        int end = start;
        const int length = url.length();

        while (end < length && url[end] != '?' && url[end] != '#')
            ++end;

        return Range<int> (start, end);
    }

    auto authorityRange = findAuthorityRange (url);
    int start = authorityRange.isEmpty() ? schemeRange.getEnd() + 3 : authorityRange.getEnd();

    if (start >= url.length() || url[start] != '/')
        return {};

    int end = start;
    const int length = url.length();

    while (end < length && url[end] != '?' && url[end] != '#')
        ++end;

    return Range<int> (start, end);
}

static void concatenatePaths (String& path, const String& suffix)
{
    if (! path.endsWithChar ('/'))
        path << '/';

    if (suffix.startsWithChar ('/'))
        path += suffix.substring (1);
    else
        path += suffix;
}

static String removeLastPathSection (const String& url)
{
    auto pathRange = findPathRange (url);
    if (pathRange.isEmpty())
        return url;

    auto path = url.substring (pathRange.getStart(), pathRange.getEnd());

    // Special case: if path is just "/" (root path), return unchanged
    if (path == "/")
        return url;

    auto lastSlash = url.lastIndexOfChar ('/');
    auto pathStart = pathRange.getStart();

    if (lastSlash > pathStart && lastSlash == url.length() - 1)
        return removeLastPathSection (url.dropLastCharacters (1));

    if (lastSlash < 0)
        return url;

    return url.substring (0, jmax (pathStart, lastSlash));
}

static Range<int> findHostRange (const String& url)
{
    auto authorityRange = findAuthorityRange (url);
    if (authorityRange.isEmpty())
        return {};

    int start = authorityRange.getStart();
    int end = authorityRange.getEnd();

    // Skip user info if present (everything before @)
    auto atPos = url.indexOfChar (start, '@');
    if (atPos >= 0 && atPos < end)
        start = atPos + 1;

    // Handle IPv6 addresses [::1]
    if (start < end && url[start] == '[')
    {
        auto closeBracket = url.indexOfChar (start, ']');
        if (closeBracket >= 0 && closeBracket < end)
            return Range<int> (start, closeBracket + 1);
    }

    // Handle regular host - find port separator (last colon in range)
    int colonPos = -1;
    for (int i = end - 1; i >= start; --i)
    {
        if (url[i] == ':')
        {
            colonPos = i;
            break;
        }
    }

    if (colonPos >= 0)
        end = colonPos;

    return Range<int> (start, end);
}

static Range<int> findPortRange (const String& url)
{
    auto authorityRange = findAuthorityRange (url);
    if (authorityRange.isEmpty())
        return {};

    int start = authorityRange.getStart();
    int end = authorityRange.getEnd();

    // Skip user info if present
    auto atPos = url.indexOfChar (start, '@');
    if (atPos >= 0 && atPos < end)
        start = atPos + 1;

    // Handle IPv6 addresses [::1]:port
    if (start < end && url[start] == '[')
    {
        auto closeBracket = url.indexOfChar (start, ']');
        if (closeBracket >= 0 && closeBracket + 1 < end && url[closeBracket + 1] == ':')
            return Range<int> (closeBracket + 2, end);

        return {};
    }

    // Handle regular host:port - find last colon in range
    int colonPos = -1;
    for (int i = end - 1; i >= start; --i)
    {
        if (url[i] == ':')
        {
            colonPos = i;
            break;
        }
    }

    if (colonPos >= 0 && colonPos + 1 < end)
        return Range<int> (colonPos + 1, end);

    return {};
}

static bool isValidIPv6Content (const String& ipv6Addr)
{
    if (ipv6Addr.isEmpty())
        return false;

    int colonCount = 0;
    int doubleColonCount = 0;
    bool hasDoubleColon = false;

    // Check for invalid characters and count colons
    for (int i = 0; i < ipv6Addr.length(); ++i)
    {
        auto ch = ipv6Addr[i];

        if (ch == ':')
        {
            colonCount++;
            // Check for double colon
            if (i + 1 < ipv6Addr.length() && ipv6Addr[i + 1] == ':')
            {
                if (hasDoubleColon)
                    return false; // Only one double colon allowed
                hasDoubleColon = true;
                doubleColonCount++;
                i++;          // Skip next colon
                colonCount++; // Count the second colon too
            }
        }
        else if (ch == '.')
        {
            // IPv4 embedded - basic check for IPv4 at end
            continue;
        }
        else if (ch == '%')
        {
            // Zone identifier - everything after this is zone
            break;
        }
        else if (CharacterFunctions::getHexDigitValue (ch) < 0)
        {
            return false; // Invalid hex character
        }
    }

    // Basic IPv6 structure validation
    if (colonCount < 2) // IPv6 needs at least 2 colons (unless it's just ::)
        return false;

    // Simple validation: if no double colon, should have 7 colons (8 groups)
    // If double colon present, can have fewer
    if (! hasDoubleColon && colonCount != 7)
        return false;

    return true;
}
} // namespace URLHelpers

void URL::addParameter (const String& name, const String& value)
{
    parameterNames.add (name);
    parameterValues.add (value);
}

String URL::toString (bool includeGetParameters) const
{
    if (includeGetParameters)
        return url + getQueryString();

    return url;
}

bool URL::isEmpty() const noexcept
{
    return url.isEmpty();
}

bool URL::isWellFormed() const
{
    // Empty URL is not well-formed
    if (url.isEmpty())
        return false;

    // Check for valid scheme
    auto schemeRange = URLHelpers::findSchemeRange (url);
    if (schemeRange.isEmpty())
        return false;

    auto scheme = url.substring (schemeRange.getStart(), schemeRange.getEnd()).toLowerCase();

    // Special schemes that don't require "//" or domain
    bool isSpecialScheme = false;
    for (const auto& special : StringArray { "mailto", "tel", "data" })
    {
        if (scheme == special)
        {
            isSpecialScheme = true;
            break;
        }
    }

    if (! isSpecialScheme)
    {
        // Check for valid authority/host
        auto authorityRange = URLHelpers::findAuthorityRange (url);

        // File URLs can have empty authority (for local files like file:///path)
        if (scheme != "file" && authorityRange.isEmpty())
            return false;

        if (! authorityRange.isEmpty())
        {
            // Check for authentication info
            auto authString = url.substring (authorityRange.getStart(), authorityRange.getEnd());
            auto atPos = authString.indexOfChar ('@');
            if (atPos >= 0)
            {
                // Validate authentication part
                auto authPart = authString.substring (0, atPos);
                if (authPart.isEmpty())
                    return false;
            }

            // Extract and validate host
            auto hostRange = URLHelpers::findHostRange (url);
            if (! hostRange.isEmpty())
            {
                auto host = url.substring (hostRange.getStart(), hostRange.getEnd());

                // Validate IPv6 addresses
                if (host.startsWith ("["))
                {
                    if (! host.endsWith ("]"))
                        return false;

                    auto ipv6Content = host.substring (1, host.length() - 1);
                    if (ipv6Content.isEmpty())
                        return false;

                    if (! URLHelpers::isValidIPv6Content (ipv6Content))
                        return false;
                }
                else if (host.isEmpty())
                {
                    return false; // Empty host not allowed for non-file URLs
                }

                // Check for empty port (invalid syntax like "example.com:")
                auto colonPos = hostRange.getEnd();
                if (colonPos < authorityRange.getEnd() && url[colonPos] == ':')
                {
                    auto portRange = URLHelpers::findPortRange (url);
                    if (portRange.isEmpty())
                        return false; // Empty port after colon is invalid

                    auto portStr = url.substring (portRange.getStart(), portRange.getEnd());
                    if (portStr.isEmpty())
                        return false;

                    for (int i = 0; i < portStr.length(); ++i)
                    {
                        if (! CharacterFunctions::isDigit (portStr[i]))
                            return false;
                    }

                    auto port = portStr.getIntValue();
                    if (port < 0 || port > 65535)
                        return false;
                }
            }
        }
    }

    auto pathRange = URLHelpers::findPathRange (url);
    if (! pathRange.isEmpty())
    {
        auto path = url.substring (pathRange.getStart(), pathRange.getEnd());

        for (int i = 0; i < path.length(); ++i)
        {
            auto ch = path[i];
            if (ch < 32 || ch == 127)
                return false;
        }
    }

    return true;
}

String URL::getDomain() const
{
    auto hostRange = URLHelpers::findHostRange (url);
    if (hostRange.isEmpty())
        return {};

    return url.substring (hostRange.getStart(), hostRange.getEnd());
}

String URL::getSubPath (bool includeGetParameters) const
{
    auto pathRange = URLHelpers::findPathRange (url);
    auto subPath = pathRange.isEmpty() ? String()
                                       : url.substring (pathRange.getStart(), pathRange.getEnd());

    if (subPath.startsWithChar ('/'))
        subPath = subPath.substring (1);

    if (includeGetParameters)
        subPath += getQueryString();

    return subPath;
}

String URL::getQueryString (bool includeAnchor) const
{
    String result;

    if (parameterNames.size() > 0)
        result += "?" + URLHelpers::getMangledParameters (*this);

    if (includeAnchor && anchor.isNotEmpty())
        result += getAnchorString();

    return result;
}

String URL::getAnchorString() const
{
    if (anchor.isNotEmpty())
        return "#" + URL::addEscapeChars (anchor, true);

    return {};
}

String URL::getScheme() const
{
    auto schemeRange = URLHelpers::findSchemeRange (url);
    if (schemeRange.isEmpty())
        return {};

    return url.substring (schemeRange.getStart(), schemeRange.getEnd());
}

#if ! YUP_ANDROID
bool URL::isLocalFile() const
{
    return getScheme() == "file";
}

File URL::getLocalFile() const
{
    return fileFromFileSchemeURL (*this);
}

String URL::getFileName() const
{
    return toString (false).fromLastOccurrenceOf ("/", false, true);
}
#endif

URL::ParameterHandling URL::toHandling (bool usePostData)
{
    return usePostData ? ParameterHandling::inPostData : ParameterHandling::inAddress;
}

File URL::fileFromFileSchemeURL (const URL& fileURL)
{
    if (! fileURL.isLocalFile())
    {
        jassertfalse;
        return {};
    }

    auto hostRange = URLHelpers::findHostRange (fileURL.url);
    auto path = hostRange.isEmpty() ? String()
                                    : removeEscapeChars (fileURL.url.substring (hostRange.getStart(), hostRange.getEnd())).replace ("+", "%2B");

    auto subPath = fileURL.getSubPath();
    if (subPath.startsWith ("/"))
        subPath = subPath.substring (1);

#if ! YUP_WINDOWS
    if (! path.isEmpty())
        path = File::getSeparatorString() + path;
#endif

    for (auto urlElement : StringArray::fromTokens (subPath, "/", ""))
        path += File::getSeparatorString() + removeEscapeChars (urlElement.replace ("+", "%2B"));

#if YUP_WINDOWS
    if (path.startsWith ("/"))
        path = path.substring (1);
#endif

    printf (">>>>> %s\n", path.toRawUTF8());
    return path;
}

int URL::getPort() const
{
    auto portRange = URLHelpers::findPortRange (url);
    if (portRange.isEmpty())
        return 0;

    auto portStr = url.substring (portRange.getStart(), portRange.getEnd());
    return portStr.getIntValue();
}

String URL::getOrigin() const
{
    const auto schemeAndDomain = getScheme() + "://" + getDomain();
    const auto port = getPort();

    if (port > 0)
        return schemeAndDomain + ":" + String { port };

    return schemeAndDomain;
}

URL URL::withNewDomainAndPath (const String& newDomainAndPath) const
{
    URL u (*this);

    auto scheme = getScheme();
    if (scheme.isEmpty())
        scheme = "http";

    u.url = scheme + "://" + newDomainAndPath;

    return u;
}

URL URL::withNewSubPath (const String& newPath) const
{
    URL u (*this);

    auto pathRange = URLHelpers::findPathRange (url);

    if (! pathRange.isEmpty())
        u.url = url.substring (0, pathRange.getStart());

    URLHelpers::concatenatePaths (u.url, newPath);
    return u;
}

URL URL::getParentURL() const
{
    URL u (*this);
    u.url = URLHelpers::removeLastPathSection (u.url);
    return u;
}

URL URL::getChildURL (const String& subPath) const
{
    URL u (*this);
    URLHelpers::concatenatePaths (u.url, subPath);
    return u;
}

bool URL::hasBodyDataToSend() const
{
    return filesToUpload.size() > 0 || ! postData.isEmpty();
}

void URL::createHeadersAndPostData (String& headers,
                                    MemoryBlock& postDataToWrite,
                                    bool addParametersToBody) const
{
    MemoryOutputStream data (postDataToWrite, false);

    if (filesToUpload.size() > 0)
    {
        // (this doesn't currently support mixing custom post-data with uploads..)
        jassert (postData.isEmpty());

        auto boundary = String::toHexString (Random::getSystemRandom().nextInt64());

        headers << "Content-Type: multipart/form-data; boundary=" << boundary << "\r\n";

        data << "--" << boundary;

        for (int i = 0; i < parameterNames.size(); ++i)
        {
            data << "\r\nContent-Disposition: form-data; name=\"" << parameterNames[i]
                 << "\"\r\n\r\n"
                 << parameterValues[i]
                 << "\r\n--" << boundary;
        }

        for (auto* f : filesToUpload)
        {
            data << "\r\nContent-Disposition: form-data; name=\"" << f->parameterName
                 << "\"; filename=\"" << f->filename << "\"\r\n";

            if (f->mimeType.isNotEmpty())
                data << "Content-Type: " << f->mimeType << "\r\n";

            data << "Content-Transfer-Encoding: binary\r\n\r\n";

            if (f->data != nullptr)
                data << *f->data;
            else
                data << f->file;

            data << "\r\n--" << boundary;
        }

        data << "--\r\n";
    }
    else
    {
        if (addParametersToBody)
            data << URLHelpers::getMangledParameters (*this);

        data << postData;

        // if the user-supplied headers didn't contain a content-type, add one now..
        if (! headers.containsIgnoreCase ("Content-Type"))
            headers << "Content-Type: application/x-www-form-urlencoded\r\n";

        headers << "Content-length: " << (int) data.getDataSize() << "\r\n";
    }
}

//==============================================================================
bool URL::isProbablyAWebsiteURL (const String& possibleURL)
{
    for (auto* protocol : { "http:", "https:", "ftp:" })
        if (possibleURL.startsWithIgnoreCase (protocol))
            return true;

    if (possibleURL.containsChar ('@') || possibleURL.containsChar (' '))
        return false;

    auto topLevelDomain = possibleURL.upToFirstOccurrenceOf ("/", false, false)
                              .fromLastOccurrenceOf (".", false, false);

    return topLevelDomain.isNotEmpty() && topLevelDomain.length() <= 3;
}

bool URL::isProbablyAnEmailAddress (const String& possibleEmailAddress)
{
    auto atSign = possibleEmailAddress.indexOfChar ('@');

    return atSign > 0
        && atSign == possibleEmailAddress.lastIndexOfChar ('@')
        && possibleEmailAddress.lastIndexOfChar ('.') > (atSign + 1)
        && ! possibleEmailAddress.endsWithChar ('.')
        && ! possibleEmailAddress.containsChar (':');
}

//==============================================================================
#if YUP_IOS
URL::Bookmark::Bookmark (void* bookmarkToUse)
    : data (bookmarkToUse)
{
}

URL::Bookmark::~Bookmark()
{
    [(NSData*) data release];
}

void setURLBookmark (URL& u, void* bookmark)
{
    u.bookmark = new URL::Bookmark (bookmark);
}

void* getURLBookmark (URL& u)
{
    if (u.bookmark.get() == nullptr)
        return nullptr;

    return u.bookmark.get()->data;
}

template <typename Stream>
struct iOSFileStreamWrapperFlush
{
    static void flush (Stream*) {}
};

template <>
struct iOSFileStreamWrapperFlush<FileOutputStream>
{
    static void flush (OutputStream* o) { o->flush(); }
};

template <typename Stream>
class iOSFileStreamWrapper final : public Stream
{
public:
    iOSFileStreamWrapper (URL& urlToUse)
        : Stream (getLocalFileAccess (urlToUse))
        , url (urlToUse)
    {
    }

    ~iOSFileStreamWrapper()
    {
        iOSFileStreamWrapperFlush<Stream>::flush (this);

        if (NSData* bookmark = (NSData*) getURLBookmark (url))
        {
            BOOL isBookmarkStale = false;
            NSError* error = nil;

            auto nsURL = [NSURL URLByResolvingBookmarkData:bookmark
                                                   options:0
                                             relativeToURL:nil
                                       bookmarkDataIsStale:&isBookmarkStale
                                                     error:&error];

            if (error == nil)
            {
                if (isBookmarkStale)
                    updateStaleBookmark (nsURL, url);

                [nsURL stopAccessingSecurityScopedResource];
            }
            else
            {
                [[maybe_unused]] auto desc = [error localizedDescription];
                jassertfalse;
            }
        }
    }

private:
    URL url;
    bool securityAccessSucceeded = false;

    File getLocalFileAccess (URL& urlToUse)
    {
        if (NSData* bookmark = (NSData*) getURLBookmark (urlToUse))
        {
            BOOL isBookmarkStale = false;
            NSError* error = nil;

            auto nsURL = [NSURL URLByResolvingBookmarkData:bookmark
                                                   options:0
                                             relativeToURL:nil
                                       bookmarkDataIsStale:&isBookmarkStale
                                                     error:&error];

            if (error == nil)
            {
                securityAccessSucceeded = [nsURL startAccessingSecurityScopedResource];

                if (isBookmarkStale)
                    updateStaleBookmark (nsURL, urlToUse);

                return urlToUse.getLocalFile();
            }

            [[maybe_unused]] auto desc = [error localizedDescription];
            jassertfalse;
        }

        return urlToUse.getLocalFile();
    }

    void updateStaleBookmark (NSURL* nsURL, URL& yupUrl)
    {
        NSError* error = nil;

        NSData* bookmark = [nsURL bookmarkDataWithOptions:NSURLBookmarkCreationSuitableForBookmarkFile
                           includingResourceValuesForKeys:nil
                                            relativeToURL:nil
                                                    error:&error];

        if (error == nil)
            setURLBookmark (yupUrl, (void*) bookmark);
        else
            jassertfalse;
    }
};
#endif

//==============================================================================
template <typename Member, typename Item>
static URL::InputStreamOptions with (URL::InputStreamOptions options, Member&& member, Item&& item)
{
    options.*member = std::forward<Item> (item);
    return options;
}

URL::InputStreamOptions::InputStreamOptions (ParameterHandling handling)
    : parameterHandling (handling)
{
}

URL::InputStreamOptions URL::InputStreamOptions::withProgressCallback (std::function<bool (int, int)> cb) const
{
    return with (*this, &InputStreamOptions::progressCallback, std::move (cb));
}

URL::InputStreamOptions URL::InputStreamOptions::withExtraHeaders (const String& headers) const
{
    return with (*this, &InputStreamOptions::extraHeaders, headers);
}

URL::InputStreamOptions URL::InputStreamOptions::withConnectionTimeoutMs (int timeout) const
{
    return with (*this, &InputStreamOptions::connectionTimeOutMs, timeout);
}

URL::InputStreamOptions URL::InputStreamOptions::withResponseHeaders (StringPairArray* headers) const
{
    return with (*this, &InputStreamOptions::responseHeaders, headers);
}

URL::InputStreamOptions URL::InputStreamOptions::withStatusCode (int* status) const
{
    return with (*this, &InputStreamOptions::statusCode, status);
}

URL::InputStreamOptions URL::InputStreamOptions::withNumRedirectsToFollow (int numRedirects) const
{
    return with (*this, &InputStreamOptions::numRedirectsToFollow, numRedirects);
}

URL::InputStreamOptions URL::InputStreamOptions::withHttpRequestCmd (const String& cmd) const
{
    return with (*this, &InputStreamOptions::httpRequestCmd, cmd);
}

//==============================================================================
std::unique_ptr<InputStream> URL::createInputStream (const InputStreamOptions& options) const
{
    if (isLocalFile())
    {
#if YUP_IOS
        // We may need to refresh the embedded bookmark.
        return std::make_unique<iOSFileStreamWrapper<FileInputStream>> (const_cast<URL&> (*this));
#else
        return getLocalFile().createInputStream();
#endif
    }

    auto webInputStream = [&]
    {
        const auto usePost = options.getParameterHandling() == ParameterHandling::inPostData;
        auto stream = std::make_unique<WebInputStream> (*this, usePost);

        auto extraHeaders = options.getExtraHeaders();

        if (extraHeaders.isNotEmpty())
            stream->withExtraHeaders (extraHeaders);

        auto timeout = options.getConnectionTimeoutMs();

        if (timeout != 0)
            stream->withConnectionTimeout (timeout);

        auto requestCmd = options.getHttpRequestCmd();

        if (requestCmd.isNotEmpty())
            stream->withCustomRequestCommand (requestCmd);

        stream->withNumRedirectsToFollow (options.getNumRedirectsToFollow());

        return stream;
    }();

    struct ProgressCallbackCaller final : public WebInputStream::Listener
    {
        ProgressCallbackCaller (std::function<bool (int, int)> progressCallbackToUse)
            : callback (std::move (progressCallbackToUse))
        {
        }

        bool postDataSendProgress (WebInputStream&, int bytesSent, int totalBytes) override
        {
            return callback (bytesSent, totalBytes);
        }

        std::function<bool (int, int)> callback;
    };

    auto callbackCaller = [&options]() -> std::unique_ptr<ProgressCallbackCaller>
    {
        if (auto progressCallback = options.getProgressCallback())
            return std::make_unique<ProgressCallbackCaller> (progressCallback);

        return {};
    }();

    auto success = webInputStream->connect (callbackCaller.get());

    if (auto* status = options.getStatusCode())
        *status = webInputStream->getStatusCode();

    if (auto* responseHeaders = options.getResponseHeaders())
        *responseHeaders = webInputStream->getResponseHeaders();

    if (! success || webInputStream->isError())
        return nullptr;

    // std::move() needed here for older compilers
    YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wredundant-move")
    return std::move (webInputStream);
    YUP_END_IGNORE_WARNINGS_GCC_LIKE
}

std::unique_ptr<OutputStream> URL::createOutputStream() const
{
#if YUP_ANDROID
    if (auto stream = AndroidDocument::fromDocument (*this).createOutputStream())
        return stream;
#endif

    if (isLocalFile())
    {
#if YUP_IOS
        // We may need to refresh the embedded bookmark.
        return std::make_unique<iOSFileStreamWrapper<FileOutputStream>> (const_cast<URL&> (*this));
#else
        return std::make_unique<FileOutputStream> (getLocalFile());
#endif
    }

    return nullptr;
}

//==============================================================================
bool URL::readEntireBinaryStream (MemoryBlock& destData, bool usePostCommand) const
{
    const std::unique_ptr<InputStream> in (isLocalFile() ? getLocalFile().createInputStream()
                                                         : createInputStream (InputStreamOptions (toHandling (usePostCommand))));

    if (in != nullptr)
    {
        in->readIntoMemoryBlock (destData);
        return true;
    }

    return false;
}

String URL::readEntireTextStream (bool usePostCommand) const
{
    const std::unique_ptr<InputStream> in (isLocalFile() ? getLocalFile().createInputStream()
                                                         : createInputStream (InputStreamOptions (toHandling (usePostCommand))));

    if (in != nullptr)
        return in->readEntireStreamAsString();

    return {};
}

std::unique_ptr<XmlElement> URL::readEntireXmlStream (bool usePostCommand) const
{
    return parseXML (readEntireTextStream (usePostCommand));
}

//==============================================================================
URL URL::withParameter (const String& parameterName,
                        const String& parameterValue) const
{
    auto u = *this;
    u.addParameter (parameterName, parameterValue);
    return u;
}

URL URL::withParameters (const StringPairArray& parametersToAdd) const
{
    auto u = *this;

    for (int i = 0; i < parametersToAdd.size(); ++i)
        u.addParameter (parametersToAdd.getAllKeys()[i], parametersToAdd.getAllValues()[i]);

    return u;
}

URL URL::withAnchor (const String& anchorToAdd) const
{
    auto u = *this;

    u.anchor = anchorToAdd;
    return u;
}

URL URL::withPOSTData (const String& newPostData) const
{
    return withPOSTData (MemoryBlock (newPostData.toRawUTF8(), newPostData.getNumBytesAsUTF8()));
}

URL URL::withPOSTData (const MemoryBlock& newPostData) const
{
    auto u = *this;
    u.postData = newPostData;
    return u;
}

//==============================================================================
URL::Upload::Upload (const String& param, const String& name, const String& mime, const File& f, MemoryBlock* mb)
    : parameterName (param)
    , filename (name)
    , mimeType (mime)
    , file (f)
    , data (mb)
{
    jassert (mimeType.isNotEmpty()); // You need to supply a mime type!
}

URL URL::withUpload (Upload* const f) const
{
    auto u = *this;

    for (int i = u.filesToUpload.size(); --i >= 0;)
        if (u.filesToUpload.getObjectPointerUnchecked (i)->parameterName == f->parameterName)
            u.filesToUpload.remove (i);

    u.filesToUpload.add (f);
    return u;
}

URL URL::withFileToUpload (const String& parameterName, const File& fileToUpload, const String& mimeType) const
{
    return withUpload (new Upload (parameterName, fileToUpload.getFileName(), mimeType, fileToUpload, nullptr));
}

URL URL::withDataToUpload (const String& parameterName, const String& filename, const MemoryBlock& fileContentToUpload, const String& mimeType) const
{
    return withUpload (new Upload (parameterName, filename, mimeType, File(), new MemoryBlock (fileContentToUpload)));
}

//==============================================================================
String URL::removeEscapeChars (const String& s)
{
    auto result = s.replaceCharacter ('+', ' ');

    if (! result.containsChar ('%'))
        return result;

    // We need to operate on the string as raw UTF8 chars, and then recombine them into unicode
    // after all the replacements have been made, so that multi-byte chars are handled.
    Array<char> utf8 (result.toRawUTF8(), (int) result.getNumBytesAsUTF8());

    for (int i = 0; i < utf8.size(); ++i)
    {
        if (utf8.getUnchecked (i) == '%')
        {
            auto hexDigit1 = CharacterFunctions::getHexDigitValue ((yup_wchar) (uint8) utf8[i + 1]);
            auto hexDigit2 = CharacterFunctions::getHexDigitValue ((yup_wchar) (uint8) utf8[i + 2]);

            if (hexDigit1 >= 0 && hexDigit2 >= 0)
            {
                utf8.set (i, (char) ((hexDigit1 << 4) + hexDigit2));
                utf8.removeRange (i + 1, 2);
            }
        }
    }

    return String::fromUTF8 (utf8.getRawDataPointer(), utf8.size());
}

String URL::addEscapeChars (const String& s, bool isParameter, bool roundBracketsAreLegal)
{
    String legalChars (isParameter ? "_-.~" : ",$_-.*!'");

    if (roundBracketsAreLegal)
        legalChars += "()";

    Array<char> utf8 (s.toRawUTF8(), (int) s.getNumBytesAsUTF8());

    for (int i = 0; i < utf8.size(); ++i)
    {
        auto c = utf8.getUnchecked (i);

        if (! (CharacterFunctions::isLetterOrDigit (c)
               || legalChars.containsChar ((yup_wchar) c)))
        {
            // For form parameters, encode space as +
            if (isParameter && c == ' ')
            {
                utf8.set (i, '+');
            }
            else
            {
                utf8.set (i, '%');
                utf8.insert (++i, "0123456789ABCDEF"[((uint8) c) >> 4]);
                utf8.insert (++i, "0123456789ABCDEF"[c & 15]);
            }
        }
    }

    return String::fromUTF8 (utf8.getRawDataPointer(), utf8.size());
}

//==============================================================================
bool URL::launchInDefaultBrowser() const
{
    auto u = toString (true);

    if (isProbablyAnEmailAddress (u) && ! u.startsWith ("mailto:"))
        u = "mailto:" + u;

    return Process::openDocument (u, {});
}

} // namespace yup
