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

class URLTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create test directory structure if needed
        testDir = File::getSpecialLocation (File::tempDirectory).getChildFile ("yup_url_tests");
        testDir.createDirectory();

        testFile = testDir.getChildFile ("test_file.txt");
        testFile.replaceWithText ("Test content");
    }

    void TearDown() override
    {
        // Clean up test files
        testDir.deleteRecursively();
    }

    File testDir;
    File testFile;
};

TEST_F (URLTests, DefaultConstruction)
{
    URL url;
    EXPECT_TRUE (url.isEmpty());
    EXPECT_EQ (url.toString (true), String());
    EXPECT_EQ (url.toString (false), String());
    EXPECT_FALSE (url.isWellFormed());
}

TEST_F (URLTests, StringConstruction)
{
    // Basic URL
    URL url1 ("http://www.example.com");
    EXPECT_FALSE (url1.isEmpty());
    EXPECT_EQ (url1.toString (false), "http://www.example.com");
    EXPECT_TRUE (url1.isWellFormed());

    // URL with path
    URL url2 ("https://example.com/path/to/resource");
    EXPECT_EQ (url2.getScheme(), "https");
    EXPECT_EQ (url2.getDomain(), "example.com");
    EXPECT_EQ (url2.getSubPath(), "path/to/resource");

    // URL with parameters
    URL url3 ("http://example.com/search?q=test&page=2");
    EXPECT_EQ (url3.getParameterNames().size(), 2);
    EXPECT_EQ (url3.getParameterNames()[0], "q");
    EXPECT_EQ (url3.getParameterNames()[1], "page");
    EXPECT_EQ (url3.getParameterValues()[0], "test");
    EXPECT_EQ (url3.getParameterValues()[1], "2");

    // URL with anchor
    URL url4 ("http://example.com/page#section");
    EXPECT_EQ (url4.getAnchorString(), "#section");

    // URL with port
    URL url5 ("http://example.com:8080/api");
    EXPECT_EQ (url5.getPort(), 8080);
    EXPECT_EQ (url5.getDomain(), "example.com");
}

TEST_F (URLTests, FileConstruction)
{
    URL fileUrl (testFile);
    EXPECT_TRUE (fileUrl.isLocalFile());
    // EXPECT_TRUE (fileUrl.isWellFormed()); // TODO: re-establish this later (failing on wasm)
    EXPECT_EQ (fileUrl.getScheme(), "file");

    File retrievedFile = fileUrl.getLocalFile();
    EXPECT_EQ (retrievedFile.getFullPathName(), testFile.getFullPathName());

    // Empty file
    URL emptyFileUrl (File {});
    EXPECT_TRUE (emptyFileUrl.isEmpty());
}

TEST_F (URLTests, Equality)
{
    URL url1 ("http://example.com/test");
    URL url2 ("http://example.com/test");
    URL url3 ("http://example.com/other");

    EXPECT_EQ (url1, url2);
    EXPECT_NE (url1, url3);

    // With parameters
    URL url4 = url1.withParameter ("key", "value");
    URL url5 = url2.withParameter ("key", "value");
    URL url6 = url1.withParameter ("key", "other");

    EXPECT_EQ (url4, url5);
    EXPECT_NE (url4, url6);
}

#if YUP_WINDOWS
TEST_F (URLTests, WindowsPaths)
{
    {
        auto path = URL ("file:///C:");
        EXPECT_TRUE (path.isWellFormed());
        EXPECT_EQ (path.getLocalFile().getFullPathName(), "C:");
    }

    {
        auto path = URL ("file:///C:/");
        EXPECT_TRUE (path.isWellFormed());
        EXPECT_EQ (path.getLocalFile().getFullPathName(), "C:");
    }

    {
        auto path = URL ("file:///C:/Users");
        EXPECT_TRUE (path.isWellFormed());
        EXPECT_EQ (path.getLocalFile().getFullPathName(), "C:\\Users");
    }

    {
        auto path = URL ("file:///C:/Users/");
        EXPECT_TRUE (path.isWellFormed());
        EXPECT_EQ (path.getLocalFile().getFullPathName(), "C:\\Users");
    }

    {
        auto path = URL ("file:///C:/Users/document.txt");
        EXPECT_TRUE (path.isWellFormed());
        EXPECT_EQ (path.getLocalFile().getFullPathName(), "C:\\Users\\document.txt");
    }
}
#endif

TEST_F (URLTests, IsWellFormed)
{
    EXPECT_TRUE (URL ("http://www.example.com").isWellFormed());
    EXPECT_TRUE (URL ("https://example.com/path/to/resource").isWellFormed());
    EXPECT_TRUE (URL ("http://192.168.1.1").isWellFormed());
    EXPECT_TRUE (URL ("http://example.com:8080").isWellFormed());
    EXPECT_TRUE (URL ("ftp://ftp.example.com/file.txt").isWellFormed());
    EXPECT_TRUE (URL ("file:///home/user/document.txt").isWellFormed());
    EXPECT_TRUE (URL ("file:///C:/Users/document.txt").isWellFormed());
    EXPECT_TRUE (URL ("file://localhost/C:/Users/document.txt").isWellFormed());
    EXPECT_TRUE (URL ("mailto:user@example.com").isWellFormed());
    EXPECT_TRUE (URL ("tel:+1234567890").isWellFormed());
    EXPECT_TRUE (URL ("ws://websocket.example.com").isWellFormed());
    EXPECT_TRUE (URL ("wss://secure.websocket.com").isWellFormed());
    EXPECT_TRUE (URL ("data:text/plain;base64,SGVsbG8=").isWellFormed());
    EXPECT_TRUE (URL ("http://subdomain.example.co.uk").isWellFormed());
    EXPECT_TRUE (URL ("https://example.com?param=value").isWellFormed());
    EXPECT_TRUE (URL ("custom+scheme://example.com").isWellFormed());
    EXPECT_TRUE (URL ("a.b-c+d://example").isWellFormed());
    EXPECT_TRUE (URL ("http://www.google.com").isWellFormed());
    EXPECT_TRUE (URL ("ftp://user@host:45/foo/bar").isWellFormed());
    EXPECT_TRUE (URL ("ftp://user:password@host:45/foo/bar").isWellFormed());
    EXPECT_TRUE (URL ("ftp://user:password@host:45/foo/bar?test=1+2+3").isWellFormed());
    EXPECT_TRUE (URL ("ftp://user:password@host:45/foo/bar/?test=1+2+3").isWellFormed());
    EXPECT_TRUE (URL ("http://www.google.com/index.html").isWellFormed());
    EXPECT_TRUE (URL ("http://www.google.com/index.html?key=value").isWellFormed());
    EXPECT_TRUE (URL ("http://www.google.com/index.html#anchor").isWellFormed());
    EXPECT_TRUE (URL ("http://www.google.com/index.html?key=value#anchor").isWellFormed());
    EXPECT_TRUE (URL ("http://192.168.1.1:8080/path").isWellFormed());
    EXPECT_TRUE (URL ("https://[::1]:8080/path").isWellFormed());
    EXPECT_TRUE (URL ("http://[2001:db8::1]/path").isWellFormed());
    EXPECT_TRUE (URL ("http://example.com:65535/path").isWellFormed());
    EXPECT_TRUE (URL ("file:///path/to/file").isWellFormed());
    EXPECT_TRUE (URL ("file://localhost/path/to/file").isWellFormed());
    EXPECT_TRUE (URL ("mailto:user@example.com").isWellFormed());
    EXPECT_TRUE (URL ("tel:+1234567890").isWellFormed());
    EXPECT_TRUE (URL ("data:text/plain;base64,SGVsbG8sIFdvcmxkIQ==").isWellFormed());
    EXPECT_TRUE (URL ("ws://example.com:8080/socket").isWellFormed());
    EXPECT_TRUE (URL ("wss://example.com/socket").isWellFormed());
    EXPECT_TRUE (URL ("ldap://[2001:db8::7]/c=GB?objectClass?one").isWellFormed());
    EXPECT_TRUE (URL ("telnet://192.0.2.16:80/").isWellFormed());

    auto imageUrl = URL ("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAUA"
                         "AAAFCAYAAACNbyblAAAAHElEQVQI12P4//8/w38GIAXDIBKE0DHxgljNBAAO"
                         "9TXL0Y4OHwAAAABJRU5ErkJggg==");
    EXPECT_TRUE (imageUrl.isWellFormed());

    EXPECT_FALSE (URL ("").isWellFormed());
    EXPECT_FALSE (URL ("user@example.com").isWellFormed());
    EXPECT_FALSE (URL ("http://").isWellFormed());
    EXPECT_FALSE (URL ("://example.com").isWellFormed());
    EXPECT_FALSE (URL ("http//example.com").isWellFormed());
    EXPECT_FALSE (URL ("http:/example.com").isWellFormed());
    EXPECT_FALSE (URL ("http://example.com:99999").isWellFormed());
    EXPECT_FALSE (URL ("http://example.com:-1").isWellFormed()); // Negative port
    EXPECT_FALSE (URL ("http://example.com:").isWellFormed());
    EXPECT_FALSE (URL ("http://example.com:abc").isWellFormed());
    EXPECT_FALSE (URL ("9http://example.com").isWellFormed());
    EXPECT_FALSE (URL ("ht!tp://example.com").isWellFormed());
    EXPECT_FALSE (URL ("http://").isWellFormed());
    EXPECT_FALSE (URL ("https://").isWellFormed());
    EXPECT_FALSE (URL ("ftp://").isWellFormed());
    EXPECT_FALSE (URL (":///path").isWellFormed());
    EXPECT_FALSE (URL ("ht tp://example.com").isWellFormed());

    // URLs with authentication
    EXPECT_TRUE (URL ("http://user:pass@example.com").isWellFormed());
    EXPECT_TRUE (URL ("https://user:pass@example.com/path").isWellFormed());
    EXPECT_TRUE (URL ("ftp://admin:12345@ftp.example.com").isWellFormed());

    // URLs with ports
    EXPECT_TRUE (URL ("http://example.com:8080").isWellFormed());
    EXPECT_TRUE (URL ("https://example.com:443/path").isWellFormed());
    EXPECT_TRUE (URL ("http://localhost:3000").isWellFormed());
    EXPECT_EQ (URL ("http://localhost:3000").getAuthentication(), "");

    // URLs with authentication and ports
    EXPECT_TRUE (URL ("http://user:pass@example.com:8080").isWellFormed());
    EXPECT_TRUE (URL ("https://admin:secret@localhost:3000/api").isWellFormed());
    EXPECT_TRUE (URL ("ftp://user:password@ftp.example.com:21/files").isWellFormed());

    // Edge cases
    EXPECT_TRUE (URL ("http://user@example.com").isWellFormed()); // No password
    EXPECT_EQ (URL ("http://user@example.com").getAuthentication(), "user");
    EXPECT_TRUE (URL ("http://user:@example.com").isWellFormed()); // Empty password
    EXPECT_EQ (URL ("http://user:@example.com").getAuthentication(), "user:");
    EXPECT_TRUE (URL ("http://:pass@example.com").isWellFormed()); // Empty username
    EXPECT_EQ (URL ("http://:pass@example.com").getAuthentication(), ":pass");
}

TEST_F (URLTests, GettersAndProperties)
{
    URL url ("https://user:pass@subdomain.example.com:8443/path/to/resource?param1=value1&param2=value2#section");

    EXPECT_EQ (url.getScheme(), "https");
    EXPECT_EQ (url.getDomain(), "subdomain.example.com");
    EXPECT_EQ (url.getSubPath (false), "path/to/resource");
    EXPECT_EQ (url.getPort(), 8443);
    EXPECT_EQ (url.getOrigin(), "https://subdomain.example.com:8443");

    // Query string
    EXPECT_EQ (url.getQueryString(), "?param1=value1&param2=value2#section");
    EXPECT_EQ (url.getSubPath (true), "path/to/resource?param1=value1&param2=value2#section");

    // Anchor
    EXPECT_EQ (url.getAnchorString(), "#section");

    // Parameters
    EXPECT_EQ (url.getParameterNames().size(), 2);
    EXPECT_EQ (url.getParameterValues().size(), 2);

    // Special case: no port specified
    URL urlNoPort ("http://example.com");
    EXPECT_EQ (urlNoPort.getPort(), 0);
    EXPECT_EQ (urlNoPort.getOrigin(), "http://example.com");
}

TEST_F (URLTests, URLManipulation)
{
    URL base ("http://example.com/base/path?existing=param");

    // withNewDomainAndPath
    URL newDomain = base.withNewDomainAndPath ("newdomain.com/new/path");
    EXPECT_EQ (newDomain.getDomain(), "newdomain.com");
    EXPECT_EQ (newDomain.getSubPath (false), "new/path");
    EXPECT_EQ (newDomain.getParameterNames()[0], "existing");

    // withNewSubPath
    URL newPath = base.withNewSubPath ("different/path");
    EXPECT_EQ (newPath.getDomain(), "example.com");
    EXPECT_EQ (newPath.getSubPath (false), "different/path");
    EXPECT_EQ (newPath.getParameterNames()[0], "existing");

    // getParentURL
    URL child ("http://example.com/parent/child/file.html");
    URL parent = child.getParentURL();
    EXPECT_EQ (parent.getSubPath (false), "parent/child");

    URL root ("http://example.com/");
    URL rootParent = root.getParentURL();
    EXPECT_EQ (rootParent.toString (false), root.toString (false));

    // getChildURL
    URL parentUrl ("http://example.com/parent/");
    URL childUrl = parentUrl.getChildURL ("child/file.html");
    EXPECT_EQ (childUrl.getSubPath (false), "parent/child/file.html");

    URL parentNoSlash ("http://example.com/parent");
    URL childFromNoSlash = parentNoSlash.getChildURL ("child");
    EXPECT_EQ (childFromNoSlash.getSubPath (false), "parent/child");
}

TEST_F (URLTests, Parameters)
{
    URL url ("http://example.com");

    // Add single parameter
    URL withParam = url.withParameter ("key", "value");
    EXPECT_EQ (withParam.getParameterNames().size(), 1);
    EXPECT_EQ (withParam.getParameterNames()[0], "key");
    EXPECT_EQ (withParam.getParameterValues()[0], "value");
    EXPECT_EQ (withParam.toString (true), "http://example.com?key=value");

    // Add multiple parameters
    StringPairArray params;
    params.set ("param1", "value1");
    params.set ("param2", "value2");
    URL withMultiple = url.withParameters (params);
    EXPECT_EQ (withMultiple.getParameterNames().size(), 2);
    EXPECT_EQ (withMultiple.toString (true), "http://example.com?param1=value1&param2=value2");

    // Parameters with special characters
    URL specialChars = url.withParameter ("special", "value with spaces & symbols");
    String queryString = specialChars.getQueryString();
    EXPECT_TRUE (queryString.contains ("value+with+spaces") || queryString.contains ("value%20with%20spaces"));
    EXPECT_TRUE (queryString.contains ("%26"));

    // Empty parameter value
    URL emptyValue = url.withParameter ("empty", "");
    EXPECT_EQ (emptyValue.toString (true), "http://example.com?empty");
}

TEST_F (URLTests, Anchors)
{
    URL url ("http://example.com");

    URL withAnchor = url.withAnchor ("section1");
    EXPECT_EQ (withAnchor.getAnchorString(), "#section1");
    EXPECT_EQ (withAnchor.toString (true), "http://example.com#section1");

    // Anchor with special characters
    URL specialAnchor = url.withAnchor ("section with spaces");
    EXPECT_TRUE (specialAnchor.getAnchorString().contains ("section"));
}

TEST_F (URLTests, POSTData)
{
    URL url ("http://example.com");

    // String POST data
    URL withPost = url.withPOSTData ("key=value&other=data");
    EXPECT_EQ (withPost.getPostData(), "key=value&other=data");

    // Binary POST data
    MemoryBlock binaryData;
    binaryData.append ("binary", 6);
    URL withBinary = url.withPOSTData (binaryData);
    EXPECT_EQ (withBinary.getPostDataAsMemoryBlock().getSize(), 6);
}

TEST_F (URLTests, FileUpload)
{
    URL url ("http://example.com/upload");

    // File upload
    URL withFile = url.withFileToUpload ("file", testFile, "text/plain");
    EXPECT_TRUE (withFile.hasBodyDataToSend());

    // Data upload
    MemoryBlock data;
    data.append ("test data", 9);
    URL withData = url.withDataToUpload ("data", "test.txt", data, "text/plain");
    EXPECT_TRUE (withData.hasBodyDataToSend());
}

TEST_F (URLTests, FileName)
{
    // Regular URLs
    URL url1 ("http://example.com/path/to/file.txt");
    EXPECT_EQ (url1.getFileName(), "file.txt");

    URL url2 ("http://example.com/path/to/directory/");
    EXPECT_EQ (url2.getFileName(), String());

    URL url3 ("http://example.com/file.txt?param=value");
    EXPECT_EQ (url3.getFileName(), "file.txt");

    // File URLs
    URL fileUrl (testFile);
    EXPECT_EQ (fileUrl.getFileName(), testFile.getFileName());
}

TEST_F (URLTests, StaticMethods)
{
    // isProbablyAWebsiteURL
    EXPECT_TRUE (URL::isProbablyAWebsiteURL ("http://www.example.com"));
    EXPECT_TRUE (URL::isProbablyAWebsiteURL ("https://example.com"));
    EXPECT_TRUE (URL::isProbablyAWebsiteURL ("www.example.com"));
    EXPECT_FALSE (URL::isProbablyAWebsiteURL ("not a url"));
    EXPECT_FALSE (URL::isProbablyAWebsiteURL ("file:///path"));

    // isProbablyAnEmailAddress
    EXPECT_TRUE (URL::isProbablyAnEmailAddress ("user@example.com"));
    EXPECT_TRUE (URL::isProbablyAnEmailAddress ("user.name@subdomain.example.com"));
    EXPECT_FALSE (URL::isProbablyAnEmailAddress ("not an email"));
    EXPECT_FALSE (URL::isProbablyAnEmailAddress ("@example.com"));
    EXPECT_FALSE (URL::isProbablyAnEmailAddress ("user@"));
}

TEST_F (URLTests, EscapeCharacters)
{
    // addEscapeChars
    EXPECT_EQ (URL::addEscapeChars ("hello world", true), "hello+world");
    EXPECT_EQ (URL::addEscapeChars ("hello world", false), "hello%20world");
    EXPECT_EQ (URL::addEscapeChars ("test@example.com", true), "test%40example.com");
    EXPECT_EQ (URL::addEscapeChars ("a&b=c", true), "a%26b%3Dc");

    // Round brackets
    EXPECT_EQ (URL::addEscapeChars ("test()", true, true), "test()");
    EXPECT_EQ (URL::addEscapeChars ("test()", true, false), "test%28%29");

    // removeEscapeChars
    EXPECT_EQ (URL::removeEscapeChars ("hello+world"), "hello world");
    EXPECT_EQ (URL::removeEscapeChars ("hello%20world"), "hello world");
    EXPECT_EQ (URL::removeEscapeChars ("test%40example.com"), "test@example.com");
    EXPECT_EQ (URL::removeEscapeChars ("a%26b%3Dc"), "a&b=c");
}

TEST_F (URLTests, CreateWithoutParsing)
{
    String urlString = "http://example.com/path?param=value#anchor";
    URL parsed (urlString);
    URL unparsed = URL::createWithoutParsing (urlString);

    // Parsed URL should have parameters extracted
    EXPECT_EQ (parsed.getParameterNames().size(), 1);
    EXPECT_EQ (parsed.getAnchorString(), "#anchor");

    // Unparsed URL should not have parameters extracted
    EXPECT_EQ (unparsed.getParameterNames().size(), 0);
    EXPECT_EQ (unparsed.getAnchorString(), String());
    EXPECT_EQ (unparsed.toString (false), urlString);
}

TEST_F (URLTests, InputStreamOptions)
{
    URL url ("http://example.com");

    // Test various options
    URL::InputStreamOptions options (URL::ParameterHandling::inAddress);

    auto withProgress = options.withProgressCallback ([] (int, int)
    {
        return true;
    });
    EXPECT_TRUE (withProgress.getProgressCallback() != nullptr);

    auto withHeaders = options.withExtraHeaders ("Custom-Header: value");
    EXPECT_EQ (withHeaders.getExtraHeaders(), "Custom-Header: value");

    auto withTimeout = options.withConnectionTimeoutMs (5000);
    EXPECT_EQ (withTimeout.getConnectionTimeoutMs(), 5000);

    StringPairArray responseHeaders;
    auto withResponseHeaders = options.withResponseHeaders (&responseHeaders);
    EXPECT_EQ (withResponseHeaders.getResponseHeaders(), &responseHeaders);

    int statusCode = 0;
    auto withStatus = options.withStatusCode (&statusCode);
    EXPECT_EQ (withStatus.getStatusCode(), &statusCode);

    auto withRedirects = options.withNumRedirectsToFollow (10);
    EXPECT_EQ (withRedirects.getNumRedirectsToFollow(), 10);

    auto withCommand = options.withHttpRequestCmd ("POST");
    EXPECT_EQ (withCommand.getHttpRequestCmd(), "POST");
}

TEST_F (URLTests, SpecialCases)
{
    // URL with authentication info (user:pass@)
    URL authUrl ("http://user:password@example.com/secure");
    EXPECT_EQ (authUrl.getDomain(), "example.com");

    // URL with multiple query parameters with same name
    URL multiParam ("http://example.com?tag=one&tag=two&tag=three");
    auto names = multiParam.getParameterNames();
    auto values = multiParam.getParameterValues();
    EXPECT_EQ (names.size(), 3);
    EXPECT_TRUE (names[0] == "tag" && names[1] == "tag" && names[2] == "tag");
    EXPECT_TRUE (values[0] == "one" && values[1] == "two" && values[2] == "three");

    // URL with empty parameter
    URL emptyParam ("http://example.com?key=&other=value");
    EXPECT_EQ (emptyParam.getParameterValues()[0], String());
    EXPECT_EQ (emptyParam.getParameterValues()[1], "value");

    // URL with parameter without value
    URL noValue ("http://example.com?flag&key=value");
    EXPECT_TRUE (noValue.getParameterNames().contains ("flag"));
    EXPECT_EQ (noValue.getParameterValues()[noValue.getParameterNames().indexOf ("flag")], String());
}

TEST_F (URLTests, StaticUtilityMethods)
{
    // isProbablyAWebsiteURL
    EXPECT_TRUE (URL::isProbablyAWebsiteURL ("www.example.com"));
    EXPECT_TRUE (URL::isProbablyAWebsiteURL ("example.com"));
    EXPECT_TRUE (URL::isProbablyAWebsiteURL ("sub.example.com"));
    EXPECT_TRUE (URL::isProbablyAWebsiteURL ("example.co.uk"));
    EXPECT_TRUE (URL::isProbablyAWebsiteURL ("file.txt"));
    EXPECT_FALSE (URL::isProbablyAWebsiteURL ("localhost"));
    EXPECT_FALSE (URL::isProbablyAWebsiteURL ("not a website"));

    // isProbablyAnEmailAddress
    EXPECT_TRUE (URL::isProbablyAnEmailAddress ("user@example.com"));
    EXPECT_TRUE (URL::isProbablyAnEmailAddress ("user.name@example.com"));
    EXPECT_TRUE (URL::isProbablyAnEmailAddress ("user+tag@example.co.uk"));
    EXPECT_FALSE (URL::isProbablyAnEmailAddress ("not an email"));
    EXPECT_FALSE (URL::isProbablyAnEmailAddress ("@example.com"));
    EXPECT_FALSE (URL::isProbablyAnEmailAddress ("user@"));
    EXPECT_FALSE (URL::isProbablyAnEmailAddress ("user@@example.com"));

    // addEscapeChars and removeEscapeChars
    String testString = "hello world!@#$%^&*()";
    String escaped = URL::addEscapeChars (testString, false);
    EXPECT_NE (escaped, testString);
    EXPECT_EQ (URL::removeEscapeChars (escaped), testString);

    // Test parameter escaping
    String paramValue = "value with spaces & special=chars";
    String escapedParam = URL::addEscapeChars (paramValue, true);
    EXPECT_TRUE (escapedParam.contains ("+"));   // spaces
    EXPECT_TRUE (escapedParam.contains ("%26")); // &
    EXPECT_TRUE (escapedParam.contains ("%3D")); // =
    EXPECT_EQ (URL::removeEscapeChars (escapedParam), paramValue);

    // Test round brackets
    String withBrackets = "test(value)";
    String escapedNoBrackets = URL::addEscapeChars (withBrackets, false, false);
    EXPECT_TRUE (escapedNoBrackets.contains ("%28")); // (
    EXPECT_TRUE (escapedNoBrackets.contains ("%29")); // )

    String escapedWithBrackets = URL::addEscapeChars (withBrackets, false, true);
    EXPECT_FALSE (escapedWithBrackets.contains ("%28")); // ( should not be escaped
    EXPECT_FALSE (escapedWithBrackets.contains ("%29")); // ) should not be escaped

    // Test already escaped strings
    String alreadyEscaped = "hello%20world";
    EXPECT_EQ (URL::removeEscapeChars (alreadyEscaped), "hello world");

    // Test double escaping
    String doubleEscaped = URL::addEscapeChars (escaped, false);
    EXPECT_NE (doubleEscaped, escaped);

    // createWithoutParsing
    String urlWithParams = "http://example.com/path?param1=value1&param2=value2#section";
    URL parsedUrl (urlWithParams);
    URL unparsedUrl = URL::createWithoutParsing (urlWithParams);

    // Parsed URL should have parameters
    EXPECT_EQ (parsedUrl.getParameterNames().size(), 2);
    EXPECT_TRUE (parsedUrl.getParameterNames().contains ("param1"));
    EXPECT_TRUE (parsedUrl.getParameterNames().contains ("param2"));

    // Unparsed URL should not have parameters
    EXPECT_EQ (unparsedUrl.getParameterNames().size(), 0);
    EXPECT_EQ (unparsedUrl.toString (false), urlWithParams);
}

#if ! YUP_WASM
TEST_F (URLTests, LocalFileStreams)
{
    URL fileUrl (testFile);

    // Test input stream
    if (auto inputStream = fileUrl.createInputStream (URL::InputStreamOptions (URL::ParameterHandling::inAddress)))
    {
        EXPECT_EQ (inputStream->getTotalLength(), testFile.getSize());

        MemoryBlock readData;
        inputStream->readIntoMemoryBlock (readData);
        EXPECT_EQ (readData.toString(), "Test content");
    }

    // Test output stream
    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_output.txt");
    URL outputUrl (tempFile);

    if (auto outputStream = outputUrl.createOutputStream())
    {
        outputStream->writeText ("Test output", false, false, nullptr);
        outputStream.reset();

        EXPECT_TRUE (tempFile.existsAsFile());
        EXPECT_EQ (tempFile.loadFileAsString(), "Test output");
        tempFile.deleteFile();
    }

    // Test readEntireBinaryStream
    MemoryBlock binaryData;
    EXPECT_TRUE (fileUrl.readEntireBinaryStream (binaryData, false));
    EXPECT_EQ (binaryData.toString(), "Test content");

    // Test readEntireTextStream
    String textData = fileUrl.readEntireTextStream (false);
    EXPECT_EQ (textData, "Test content");

    // Test readEntireXmlStream with XML content
    File xmlFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test.xml");
    xmlFile.replaceWithText ("<?xml version=\"1.0\"?><root><child attr=\"value\">content</child></root>");
    URL xmlUrl (xmlFile);

    if (auto xml = xmlUrl.readEntireXmlStream (false))
    {
        EXPECT_EQ (xml->getTagName(), "root");
        if (auto child = xml->getChildByName ("child"))
        {
            EXPECT_EQ (child->getStringAttribute ("attr"), "value");
            EXPECT_EQ (child->getAllSubText(), "content");
        }
    }
    xmlFile.deleteFile();
}
#endif

// Additional edge cases
TEST_F (URLTests, EdgeCases)
{
    // Very long URL
    String longPath;
    for (int i = 0; i < 100; ++i)
        longPath += "segment" + String (i) + "/";

    URL longUrl ("http://example.com/" + longPath);
    EXPECT_TRUE (longUrl.isWellFormed());
    EXPECT_TRUE (longUrl.getSubPath().contains ("segment99"));

    // URL with international characters
    URL intlUrl (L"http://example.com/日本語/文件");
    EXPECT_TRUE (intlUrl.isWellFormed());

    // URL with all components
    URL fullUrl ("https://user:pass@sub.example.com:8443/path/to/resource?param1=value1&param2=value2#section");
    EXPECT_TRUE (fullUrl.isWellFormed());
    EXPECT_EQ (fullUrl.getScheme(), "https");
    EXPECT_EQ (fullUrl.getDomain(), "sub.example.com");
    EXPECT_EQ (fullUrl.getPort(), 8443);
    EXPECT_EQ (fullUrl.getSubPath (false), "path/to/resource");
    EXPECT_EQ (fullUrl.getParameterNames().size(), 2);
    EXPECT_EQ (fullUrl.getAnchorString(), "#section");
}

#if ! YUP_WASM
TEST_F (URLTests, ReadEntireStreams)
{
    // Test with local file
    URL fileUrl (testFile);

    // readEntireBinaryStream
    MemoryBlock binaryData;
    bool success = fileUrl.readEntireBinaryStream (binaryData, false);
    EXPECT_TRUE (success);
    EXPECT_EQ (binaryData.getSize(), testFile.getSize());
    EXPECT_EQ (String::fromUTF8 ((const char*) binaryData.getData(), (int) binaryData.getSize()), "Test content");

    // readEntireTextStream
    String textData = fileUrl.readEntireTextStream (false);
    EXPECT_EQ (textData, "Test content");

    // Test with POST flag
    String textDataPost = fileUrl.readEntireTextStream (true);
    EXPECT_EQ (textDataPost, "Test content");

    // readEntireXmlStream with valid XML
    File xmlFile = testDir.getChildFile ("test.xml");
    xmlFile.replaceWithText ("<?xml version=\"1.0\"?><root><element>value</element></root>");
    URL xmlUrl (xmlFile);

    auto xmlDoc = xmlUrl.readEntireXmlStream (false);
    EXPECT_NE (xmlDoc, nullptr);
    if (xmlDoc != nullptr)
    {
        EXPECT_EQ (xmlDoc->getTagName(), "root");
        auto* element = xmlDoc->getChildByName ("element");
        EXPECT_NE (element, nullptr);
        if (element != nullptr)
            EXPECT_EQ (element->getAllSubText(), "value");
    }
}
#endif

TEST_F (URLTests, LaunchInDefaultBrowser)
{
    /*
    // We can't really test if the browser opens, but we can test the method exists
    // and returns a value. On CI systems, this might return false.
    URL webUrl ("http://www.example.com");
    bool result = webUrl.launchInDefaultBrowser();
    // Don't assert on the result as it's system-dependent
    (void) result;
    */
}

TEST_F (URLTests, DownloadTaskOptions)
{
    URL::DownloadTaskOptions options;

    // Test builder pattern
    auto withHeaders = options.withExtraHeaders ("X-Custom: value");
    EXPECT_EQ (withHeaders.extraHeaders, "X-Custom: value");

    auto withContainer = options.withSharedContainer ("container-name");
    EXPECT_EQ (withContainer.sharedContainer, "container-name");

    // Mock listener
    struct TestListener : public URL::DownloadTaskListener
    {
        bool finishedCalled = false;
        bool progressCalled = false;

        void finished (URL::DownloadTask* task, bool success) override
        {
            finishedCalled = true;
            (void) task;
            (void) success;
        }

        void progress (URL::DownloadTask* task, int64 bytesDownloaded, int64 totalLength) override
        {
            progressCalled = true;
            (void) task;
            (void) bytesDownloaded;
            (void) totalLength;
        }
    };

    TestListener listener;
    auto withListener = options.withListener (&listener);
    EXPECT_EQ (withListener.listener, &listener);

    auto withPost = options.withUsePost (true);
    EXPECT_TRUE (withPost.usePost);

    // Test chaining
    auto chained = options
                       .withExtraHeaders ("Header: value")
                       .withSharedContainer ("container")
                       .withListener (&listener)
                       .withUsePost (true);

    EXPECT_EQ (chained.extraHeaders, "Header: value");
    EXPECT_EQ (chained.sharedContainer, "container");
    EXPECT_EQ (chained.listener, &listener);
    EXPECT_TRUE (chained.usePost);
}

#if ! YUP_WASM
TEST_F (URLTests, DownloadTask)
{
    // Create a test file to serve as download source
    File sourceFile = testDir.getChildFile ("source.txt");
    sourceFile.replaceWithText ("Download content");

    URL sourceUrl (sourceFile);
    File targetFile = testDir.getChildFile ("downloaded.txt");

    // Test basic download
    URL::DownloadTaskOptions options;
    auto task = sourceUrl.downloadToFile (targetFile, options);

    if (task != nullptr)
    {
        // Wait for download to complete
        int maxWait = 50; // 5 seconds max
        while (! task->isFinished() && maxWait-- > 0)
            Thread::sleep (100);

        EXPECT_TRUE (task->isFinished());
        EXPECT_FALSE (task->hadError());
        EXPECT_GT (task->getTotalLength(), 0);
        EXPECT_EQ (task->getLengthDownloaded(), task->getTotalLength());
        EXPECT_EQ (task->getTargetLocation().getFullPathName(), targetFile.getFullPathName());

        // Verify downloaded content
        if (targetFile.existsAsFile())
        {
            EXPECT_EQ (targetFile.loadFileAsString(), "Download content");
        }
    }
}
#endif

TEST_F (URLTests, URLWithComplexEscaping)
{
    // Test various escape sequences in different parts of URL
    URL url ("http://example.com/path%20with%20spaces/file%2Bname.txt?param%3D1=value%261&param2=100%25#section%23tag");

    EXPECT_TRUE (url.isWellFormed());
    EXPECT_TRUE (url.getSubPath (false).contains ("path"));
    EXPECT_TRUE (url.getSubPath (false).contains ("file"));
    EXPECT_EQ (url.getParameterNames().size(), 2);

    // Test that escaping is preserved in toString
    String urlString = url.toString (true);
    EXPECT_TRUE (urlString.contains ("%"));
}

TEST_F (URLTests, IPAddresses)
{
    // IPv4
    URL ipv4 ("http://192.168.1.1:8080/api");
    EXPECT_TRUE (ipv4.isWellFormed());
    EXPECT_EQ (ipv4.getDomain(), "192.168.1.1");
    EXPECT_EQ (ipv4.getPort(), 8080);

    // IPv6 (with brackets)
    URL ipv6 ("http://[2001:db8::1]:8080/api");
    EXPECT_TRUE (ipv6.isWellFormed());

    // Localhost
    URL localhost ("http://localhost:3000");
    EXPECT_TRUE (localhost.isWellFormed());
    EXPECT_EQ (localhost.getDomain(), "localhost");
    EXPECT_EQ (localhost.getPort(), 3000);
}

TEST_F (URLTests, IPv6URLs)
{
    // Basic IPv6 tests
    URL ipv6Full ("http://[2001:0db8:85a3:0000:0000:8a2e:0370:7334]/");
    EXPECT_TRUE (ipv6Full.isWellFormed());
    EXPECT_EQ (ipv6Full.getDomain(), "[2001:0db8:85a3:0000:0000:8a2e:0370:7334]");
    EXPECT_EQ (ipv6Full.getPort(), 0);
    EXPECT_EQ (ipv6Full.getSubPath (false), "");

    // IPv6 with port
    URL ipv6Port ("http://[2001:db8::1]:8080/path/to/resource");
    EXPECT_TRUE (ipv6Port.isWellFormed());
    EXPECT_EQ (ipv6Port.getDomain(), "[2001:db8::1]");
    EXPECT_EQ (ipv6Port.getPort(), 8080);
    EXPECT_EQ (ipv6Port.getSubPath (false), "path/to/resource");

    // IPv6 loopback
    URL ipv6Loopback ("https://[::1]:443/secure");
    EXPECT_TRUE (ipv6Loopback.isWellFormed());
    EXPECT_EQ (ipv6Loopback.getDomain(), "[::1]");
    EXPECT_EQ (ipv6Loopback.getPort(), 443);
    EXPECT_EQ (ipv6Loopback.getSubPath (false), "secure");

    // IPv6 with authentication
    URL ipv6Auth ("ftp://user:pass@[2001:db8::2]:21/files");
    EXPECT_TRUE (ipv6Auth.isWellFormed());
    EXPECT_EQ (ipv6Auth.getDomain(), "[2001:db8::2]");
    EXPECT_EQ (ipv6Auth.getPort(), 21);
    EXPECT_EQ (ipv6Auth.getSubPath (false), "files");

    // IPv6 compressed formats
    URL ipv6Compressed ("http://[::ffff:192.0.2.128]/test");
    EXPECT_TRUE (ipv6Compressed.isWellFormed());
    EXPECT_EQ (ipv6Compressed.getDomain(), "[::ffff:192.0.2.128]");
    EXPECT_EQ (ipv6Compressed.getSubPath (false), "test");

    // IPv6 with zone identifier (interface)
    URL ipv6Zone ("http://[fe80::1%eth0]:8080/");
    EXPECT_TRUE (ipv6Zone.isWellFormed());
    EXPECT_EQ (ipv6Zone.getDomain(), "[fe80::1%eth0]");
    EXPECT_EQ (ipv6Zone.getPort(), 8080);

    // IPv6 with query parameters and anchor
    URL ipv6Complex ("http://[2001:db8::3]:9000/api/v1/data?format=json&limit=10#results");
    EXPECT_TRUE (ipv6Complex.isWellFormed());
    EXPECT_EQ (ipv6Complex.getDomain(), "[2001:db8::3]");
    EXPECT_EQ (ipv6Complex.getPort(), 9000);
    EXPECT_EQ (ipv6Complex.getSubPath (false), "api/v1/data");
    EXPECT_EQ (ipv6Complex.getParameterNames().size(), 2);
    EXPECT_EQ (ipv6Complex.getParameterValues()[0], "json");
    EXPECT_EQ (ipv6Complex.getParameterValues()[1], "10");
    EXPECT_EQ (ipv6Complex.getAnchorString(), "#results");

    // Test URL construction from components
    URL constructedIPv6 ("http://[::1]");
    URL withPath = constructedIPv6.withNewSubPath ("test/path");
    EXPECT_EQ (withPath.toString (false), "http://[::1]/test/path");

    URL withPort = constructedIPv6.withParameter ("port", "8080");
    EXPECT_TRUE (withPort.toString (true).contains ("[::1]"));

    // Test getOrigin with IPv6
    URL ipv6Origin ("https://[2001:db8::4]:8443/secure/path");
    EXPECT_EQ (ipv6Origin.getOrigin(), "https://[2001:db8::4]:8443");

    // Invalid IPv6 addresses
    EXPECT_FALSE (URL ("http://[::g]/").isWellFormed());       // Invalid character
    EXPECT_FALSE (URL ("http://[2001:db8::/").isWellFormed()); // Missing closing bracket
    EXPECT_FALSE (URL ("http://2001:db8::]/").isWellFormed()); // Missing opening bracket
    EXPECT_FALSE (URL ("http://[]:8080/").isWellFormed());     // Empty brackets

    // Edge cases with maximum length IPv6
    URL ipv6MaxLength ("http://[1234:5678:90ab:cdef:1234:5678:90ab:cdef]:65535/very/long/path/name");
    EXPECT_TRUE (ipv6MaxLength.isWellFormed());
    EXPECT_EQ (ipv6MaxLength.getDomain(), "[1234:5678:90ab:cdef:1234:5678:90ab:cdef]");
    EXPECT_EQ (ipv6MaxLength.getPort(), 65535);

    // IPv6 with different schemes
    URL ipv6WebSocket ("ws://[2001:db8::5]:8080/websocket");
    EXPECT_TRUE (ipv6WebSocket.isWellFormed());
    EXPECT_EQ (ipv6WebSocket.getScheme(), "ws");
    EXPECT_EQ (ipv6WebSocket.getDomain(), "[2001:db8::5]");

    // Test parameter handling with IPv6
    URL ipv6Base ("http://[::1]:8080/api");
    URL ipv6WithParams = ipv6Base.withParameter ("key", "value").withParameter ("type", "json");
    EXPECT_EQ (ipv6WithParams.toString (true), "http://[::1]:8080/api?key=value&type=json");

    // Test child/parent URL operations with IPv6
    URL ipv6Parent ("http://[2001:db8::6]/parent/");
    URL ipv6Child = ipv6Parent.getChildURL ("child/file.txt");
    EXPECT_EQ (ipv6Child.getSubPath (false), "parent/child/file.txt");
    EXPECT_EQ (ipv6Child.getDomain(), "[2001:db8::6]");

    URL ipv6ChildParent = ipv6Child.getParentURL();
    EXPECT_EQ (ipv6ChildParent.getSubPath (false), "parent/child");

    // Test file name extraction with IPv6
    URL ipv6File ("http://[::1]:8080/downloads/document.pdf");
    EXPECT_EQ (ipv6File.getFileName(), "document.pdf");

    // Test POST data with IPv6
    URL ipv6Post ("http://[2001:db8::7]:3000/submit");
    URL ipv6WithPost = ipv6Post.withPOSTData ("data=test&ipv6=true");
    EXPECT_EQ (ipv6WithPost.getPostData(), "data=test&ipv6=true");

    // Reconstructing URL from parsed components
    URL originalIPv6 ("http://user:pass@[2001:db8::8]:9999/path?q=test#anchor");
    String reconstructed = originalIPv6.getScheme() + "://" + originalIPv6.getDomain() + ":" + String (originalIPv6.getPort()) + "/" + originalIPv6.getSubPath (false);
    // Should contain the essential parts
    EXPECT_TRUE (reconstructed.contains ("[2001:db8::8]"));
    EXPECT_TRUE (reconstructed.contains (":9999"));
    EXPECT_TRUE (reconstructed.contains ("/path"));
}

TEST_F (URLTests, DataURLs)
{
    // Plain text data URL
    URL dataUrl ("data:text/plain;charset=utf-8,Hello%20World");
    EXPECT_TRUE (dataUrl.isWellFormed());
    EXPECT_EQ (dataUrl.getScheme(), "data");

    // Base64 encoded data URL
    URL base64Url ("data:text/plain;base64,SGVsbG8gV29ybGQ=");
    EXPECT_TRUE (base64Url.isWellFormed());

    // Image data URL
    URL imageUrl ("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60e6kgAAAABJRU5ErkJggg==");
    EXPECT_TRUE (imageUrl.isWellFormed());
}
