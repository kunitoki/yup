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

static int findEndOfScheme (const String& url)
{
    int i = 0;

    while (CharacterFunctions::isLetterOrDigit (url[i]) || url[i] == '+' || url[i] == '-' || url[i] == '.')
        ++i;

    return url.substring (i).startsWith (":") ? i + 1 : 0;
}

static int findStartOfNetLocation (const String& url)
{
    int start = findEndOfScheme (url);

    while (url[start] == '/')
        ++start;

    return start;
}

static int findStartOfPath (const String& url)
{
    return url.indexOfChar (findStartOfNetLocation (url), '/') + 1;
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
    auto startOfPath = findStartOfPath (url);
    auto lastSlash = url.lastIndexOfChar ('/');

    if (lastSlash > startOfPath && lastSlash == url.length() - 1)
        return removeLastPathSection (url.dropLastCharacters (1));

    if (lastSlash < 0)
        return url;

    return url.substring (0, std::max (startOfPath, lastSlash));
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
    auto schemeEnd = URLHelpers::findEndOfScheme (url);
    if (schemeEnd <= 1) // No scheme or empty scheme
        return false;

    auto scheme = url.substring (0, schemeEnd - 1).toLowerCase();

    // Common valid schemes
    static const StringArray validSchemes = { "http", "https", "ftp", "ftps", "file", "mailto", "tel", "data", "ws", "wss" };

    bool hasValidScheme = false;
    for (const auto& validScheme : validSchemes)
    {
        if (scheme == validScheme)
        {
            hasValidScheme = true;
            break;
        }
    }

    // Also accept any scheme that contains only valid characters
    if (! hasValidScheme)
    {
        for (int i = 0; i < scheme.length(); ++i)
        {
            auto ch = scheme[i];
            if (i == 0)
            {
                // First character must be a letter
                if (! CharacterFunctions::isLetter (ch))
                    return false;
            }
            else
            {
                // Subsequent characters can be letters, digits, +, -, or .
                if (! (CharacterFunctions::isLetterOrDigit (ch) || ch == '+' || ch == '-' || ch == '.'))
                    return false;
            }
        }
    }

    // Special schemes that don't require "//" or domain
    const StringArray specialSchemes = { "mailto", "tel", "data" };
    bool isSpecialScheme = false;
    for (const auto& special : specialSchemes)
    {
        if (scheme == special)
        {
            isSpecialScheme = true;
            break;
        }
    }

    if (! isSpecialScheme)
    {
        // Check for "://" after scheme
        if (! url.substring (schemeEnd - 1).startsWith ("://"))
            return false;

        // Check for valid domain/host
        auto domainStart = URLHelpers::findStartOfNetLocation (url);

        // Check for authentication info (user:pass@)
        auto atPos = url.indexOfChar (domainStart, '@');
        auto firstSlash = url.indexOfChar (domainStart, '/');
        if (atPos > 0 && (firstSlash < 0 || atPos < firstSlash))
        {
            // Validate authentication part
            auto authPart = url.substring (domainStart, atPos);
            if (authPart.isEmpty())
                return false;
            domainStart = atPos + 1;
        }

        // Find the end of domain (either path start, port start, or end of string)
        auto pathStart = url.indexOfChar (domainStart, '/');

        String domain;
        String portStr;

        // Check if we have an IPv6 address
        if (url[domainStart] == '[')
        {
            // IPv6 address - find the closing bracket
            auto closeBracket = url.indexOfChar (domainStart, ']');
            if (closeBracket < 0)
                return false; // No closing bracket

            domain = url.substring (domainStart, closeBracket + 1);

            // Check for port after IPv6 address
            if (closeBracket + 1 < url.length() && url[closeBracket + 1] == ':')
            {
                auto portEnd = pathStart > 0 ? pathStart : url.length();
                portStr = url.substring (closeBracket + 2, portEnd);
            }
        }
        else
        {
            // For regular domain or IPv4, look for port colon after the domain start
            // (to avoid confusion with colons in authentication)
            auto portStart = url.indexOfChar (domainStart, ':');

            // Make sure the colon is before any path and is not part of IPv6
            if (portStart > domainStart && (pathStart < 0 || portStart < pathStart))
            {
                // We have a port
                domain = url.substring (domainStart, portStart);
                auto portEnd = pathStart > 0 ? pathStart : url.length();
                portStr = url.substring (portStart + 1, portEnd);

                // If we found a colon but no port number, it's invalid
                if (portStr.isEmpty())
                    return false;
            }
            else if (pathStart > 0)
            {
                // No port, but we have a path
                domain = url.substring (domainStart, pathStart);
            }
            else
            {
                // No port, no path
                domain = url.substring (domainStart);
            }
        }

        // Validate port if present
        if (! portStr.isEmpty())
        {
            // Validate port number
            for (int i = 0; i < portStr.length(); ++i)
            {
                if (! CharacterFunctions::isDigit (portStr[i]))
                    return false;
            }

            auto port = portStr.getIntValue();
            if (port < 0 || port > 65535)
                return false;
        }

        // File URLs can have empty domain (for local files)
        if (scheme == "file" && domain.isEmpty())
        {
            // File URLs are allowed to have empty domain for local files
            // e.g., file:///path/to/file or file://localhost/path
        }
        else if (domain.isEmpty())
        {
            return false; // Non-file URLs must have a domain
        }
        else
        {
            // Validate domain characters
            // Domain can be IP address or hostname

            // Check if it's an IPv4 address
            bool isIPv4 = true;
            int dotCount = 0;
            StringArray octets = StringArray::fromTokens (domain, ".", "");

            if (octets.size() == 4)
            {
                for (const auto& octet : octets)
                {
                    if (octet.isEmpty() || octet.length() > 3)
                    {
                        isIPv4 = false;
                        break;
                    }

                    for (int i = 0; i < octet.length(); ++i)
                    {
                        if (! CharacterFunctions::isDigit (octet[i]))
                        {
                            isIPv4 = false;
                            break;
                        }
                    }

                    if (isIPv4)
                    {
                        auto value = octet.getIntValue();
                        if (value < 0 || value > 255)
                        {
                            isIPv4 = false;
                            break;
                        }
                    }
                }
            }
            else
            {
                isIPv4 = false;
            }

            // Check if it's an IPv6 address
            bool isIPv6 = false;
            if (domain.startsWith ("[") && domain.endsWith ("]"))
            {
                // Extract the address between brackets
                auto ipv6Addr = domain.substring (1, domain.length() - 1);

                // Empty brackets are invalid
                if (ipv6Addr.isEmpty())
                    return false;

                // Basic IPv6 validation
                // Check for valid characters (hex digits, colons, dots for IPv4-mapped, % for zone)
                bool hasDoubleColon = ipv6Addr.contains ("::");
                int colonCount = 0;

                for (int i = 0; i < ipv6Addr.length(); ++i)
                {
                    auto ch = ipv6Addr[i];

                    if (ch == ':')
                        colonCount++;

                    else if (ch == '.')
                        continue;

                    else if (ch == '%')
                        break;

                    else if (CharacterFunctions::getHexDigitValue (ch) < 0)
                        return false;
                }

                // At minimum, an IPv6 address needs some colons
                if (colonCount == 0)
                    return false;

                isIPv6 = true;
            }

            if (! isIPv4 && ! isIPv6)
            {
                // Validate as hostname
                // Check for valid hostname characters
                bool lastWasDot = false;
                bool lastWasHyphen = false;

                for (int i = 0; i < domain.length(); ++i)
                {
                    auto ch = domain[i];

                    if (ch == '.')
                    {
                        if (i == 0 || i == domain.length() - 1 || lastWasDot || lastWasHyphen)
                            return false;
                        lastWasDot = true;
                        lastWasHyphen = false;
                    }
                    else if (ch == '-')
                    {
                        if (i == 0 || i == domain.length() - 1 || lastWasDot)
                            return false;
                        lastWasHyphen = true;
                        lastWasDot = false;
                    }
                    else if (CharacterFunctions::isLetterOrDigit (ch))
                    {
                        lastWasDot = false;
                        lastWasHyphen = false;
                    }
                    else
                    {
                        // Invalid character in hostname
                        return false;
                    }
                }
            }
        }
    }

    // Basic path validation - check for invalid characters
    auto pathStart = URLHelpers::findStartOfPath (url);
    if (pathStart > 0 && pathStart < url.length())
    {
        auto path = url.substring (pathStart);

        // Remove query string and anchor for path validation
        auto queryPos = path.indexOfChar ('?');
        if (queryPos >= 0)
            path = path.substring (0, queryPos);

        auto anchorPos = path.indexOfChar ('#');
        if (anchorPos >= 0)
            path = path.substring (0, anchorPos);

        // Path should not contain certain invalid characters
        // Note: We're being lenient here as paths can contain many special characters
        for (int i = 0; i < path.length(); ++i)
        {
            auto ch = path[i];
            // Disallow control characters
            if (ch < 32 || ch == 127)
                return false;
        }
    }

    return true;
}

String URL::getDomain() const
{
    return getDomainInternal (false);
}

String URL::getSubPath (bool includeGetParameters) const
{
    auto startOfPath = URLHelpers::findStartOfPath (url);
    auto subPath = startOfPath <= 0 ? String()
                                    : url.substring (startOfPath);

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
    return url.substring (0, URLHelpers::findEndOfScheme (url) - 1);
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

    auto path = removeEscapeChars (fileURL.getDomainInternal (true)).replace ("+", "%2B");

#if YUP_WINDOWS
    bool isUncPath = (! fileURL.url.startsWith ("file:///"));
#else
    path = File::getSeparatorString() + path;
#endif

    auto urlElements = StringArray::fromTokens (fileURL.getSubPath(), "/", "");

    for (auto urlElement : urlElements)
        path += File::getSeparatorString() + removeEscapeChars (urlElement.replace ("+", "%2B"));

#if YUP_WINDOWS
    if (isUncPath)
        path = "\\\\" + path;
#endif

    return path;
}

int URL::getPort() const
{
    auto start = URLHelpers::findStartOfNetLocation (url);

    // Check for authentication info (user:pass@)
    auto atPos = url.indexOfChar (start, '@');
    auto firstSlash = url.indexOfChar (start, '/');
    if (atPos > 0 && (firstSlash < 0 || atPos < firstSlash))
        start = atPos + 1;

    auto colonPos = -1;

    // Check if this is an IPv6 address
    if (start < url.length() && url[start] == '[')
    {
        // Find the closing bracket for IPv6
        auto closeBracket = url.indexOfChar (start, ']');
        if (closeBracket > start)
        {
            // Look for port after the closing bracket
            if (closeBracket + 1 < url.length() && url[closeBracket + 1] == ':')
                colonPos = closeBracket + 1;
        }
    }
    else
    {
        // Regular domain - find the colon after the domain
        colonPos = url.indexOfChar (start, ':');
    }

    // Make sure the colon is before any path
    if (colonPos > 0)
    {
        auto pathStart = url.indexOfChar (start, '/');
        if (pathStart > 0 && colonPos > pathStart)
            return 0; // Colon is in the path, not a port separator

        return url.substring (colonPos + 1).getIntValue();
    }

    return 0;
}

String URL::getOrigin() const
{
    const auto schemeAndDomain = getScheme() + "://" + getDomain();
    const auto port = getPort();

    if (port > 0)
        return schemeAndDomain + ":" + String { port };

    return schemeAndDomain;
}

URL URL::withNewDomainAndPath (const String& newURL) const
{
    URL u (*this);
    u.url = newURL;
    return u;
}

URL URL::withNewSubPath (const String& newPath) const
{
    URL u (*this);

    auto startOfPath = URLHelpers::findStartOfPath (url);

    if (startOfPath > 0)
        u.url = url.substring (0, startOfPath);

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
        && ! possibleEmailAddress.endsWithChar ('.');
}

String URL::getDomainInternal (bool ignorePort) const
{
    auto start = URLHelpers::findStartOfNetLocation (url);

    // Check for authentication info (user:pass@)
    auto atPos = url.indexOfChar (start, '@');
    auto firstSlash = url.indexOfChar (start, '/');
    if (atPos > 0 && (firstSlash < 0 || atPos < firstSlash))
        start = atPos + 1;

    auto end1 = url.indexOfChar (start, '/');
    auto end2 = -1;

    if (! ignorePort)
    {
        // Check if this is an IPv6 address
        if (start < url.length() && url[start] == '[')
        {
            // Find the closing bracket for IPv6
            auto closeBracket = url.indexOfChar (start, ']');
            if (closeBracket > start)
            {
                // Look for port after the closing bracket
                if (closeBracket + 1 < url.length() && url[closeBracket + 1] == ':')
                    end2 = closeBracket + 1;
            }
        }
        else
        {
            // Regular domain - look for port
            end2 = url.indexOfChar (start, ':');
        }
    }

    auto end = (end1 < 0 && end2 < 0) ? std::numeric_limits<int>::max()
                                      : ((end1 < 0 || end2 < 0) ? jmax (end1, end2)
                                                                : jmin (end1, end2));
    return url.substring (start, end);
}

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
        u.addParameter (parametersToAdd.getAllKeys()[i],
                        parametersToAdd.getAllValues()[i]);

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
    String legalChars (isParameter ? "_-.~"
                                   : ",$_-.*!'");

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

    if (u.containsChar ('@') && ! u.containsChar (':'))
        u = "mailto:" + u;

    return Process::openDocument (u, {});
}

} // namespace yup
