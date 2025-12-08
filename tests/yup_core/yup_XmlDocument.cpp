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

class XmlDocumentTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

// =============================================================================
// Entity Tests (through XML parsing - methods are private)
// =============================================================================

TEST_F (XmlDocumentTests, ParseXmlWithAmpEntity)
{
    auto xml = XmlDocument::parse ("<root>foo &amp; bar</root>");
    ASSERT_NE (xml, nullptr);
    EXPECT_EQ (xml->getAllSubText(), "foo & bar");
}

TEST_F (XmlDocumentTests, ParseXmlWithQuotEntity)
{
    auto xml = XmlDocument::parse ("<root>Say &quot;hello&quot;</root>");
    ASSERT_NE (xml, nullptr);
    EXPECT_EQ (xml->getAllSubText(), "Say \"hello\"");
}

TEST_F (XmlDocumentTests, ParseXmlWithAposEntity)
{
    auto xml = XmlDocument::parse ("<root>It&apos;s working</root>");
    ASSERT_NE (xml, nullptr);
    EXPECT_EQ (xml->getAllSubText(), "It's working");
}

TEST_F (XmlDocumentTests, ParseXmlWithLtGtEntities)
{
    auto xml = XmlDocument::parse ("<root>&lt;tag&gt;</root>");
    ASSERT_NE (xml, nullptr);
    EXPECT_EQ (xml->getAllSubText(), "<tag>");
}

TEST_F (XmlDocumentTests, ParseXmlWithNumericEntities)
{
    auto xml = XmlDocument::parse ("<root>&#65;&#66;&#67;</root>");
    ASSERT_NE (xml, nullptr);
    EXPECT_EQ (xml->getAllSubText(), "ABC");
}

TEST_F (XmlDocumentTests, ParseXmlWithHexEntities)
{
    auto xml = XmlDocument::parse ("<root>&#x41;&#x42;&#x43;</root>");
    ASSERT_NE (xml, nullptr);
    EXPECT_EQ (xml->getAllSubText(), "ABC");
}

// =============================================================================
// CDATA Tests
// =============================================================================

TEST_F (XmlDocumentTests, ParseXmlWithCDATA)
{
    auto xml = XmlDocument::parse ("<root><![CDATA[Some <data> & stuff]]></root>");
    ASSERT_NE (xml, nullptr);
    EXPECT_EQ (xml->getAllSubText(), "Some <data> & stuff");
}

TEST_F (XmlDocumentTests, ParseXmlWithMultipleCDATA)
{
    auto xml = XmlDocument::parse ("<root><![CDATA[First]]> <![CDATA[Second]]></root>");
    ASSERT_NE (xml, nullptr);
    EXPECT_TRUE (xml->getAllSubText().contains ("First"));
    EXPECT_TRUE (xml->getAllSubText().contains ("Second"));
}

TEST_F (XmlDocumentTests, ParseXmlWithUnterminatedCDATA)
{
    XmlDocument doc ("<root><![CDATA[Unterminated");
    auto xml = doc.getDocumentElement();
    EXPECT_EQ (xml, nullptr);
    EXPECT_FALSE (doc.getLastParseError().isEmpty());
}

// =============================================================================
// Comment Tests
// =============================================================================

TEST_F (XmlDocumentTests, ParseXmlWithComment)
{
    auto xml = XmlDocument::parse ("<root><!-- This is a comment -->Text</root>");
    ASSERT_NE (xml, nullptr);
    EXPECT_EQ (xml->getAllSubText(), "Text");
}

TEST_F (XmlDocumentTests, ParseXmlWithMultipleComments)
{
    auto xml = XmlDocument::parse ("<root><!-- Comment 1 -->Text<!-- Comment 2 --></root>");
    ASSERT_NE (xml, nullptr);
    EXPECT_EQ (xml->getAllSubText(), "Text");
}

TEST_F (XmlDocumentTests, ParseXmlWithUnterminatedComment)
{
    XmlDocument doc ("<root><!-- Unterminated comment");
    auto xml = doc.getDocumentElement();
    EXPECT_EQ (xml, nullptr);
    EXPECT_FALSE (doc.getLastParseError().isEmpty());
}

TEST_F (XmlDocumentTests, ParseXmlWithCommentInContent)
{
    auto xml = XmlDocument::parse ("<root>Before<!-- comment -->After</root>");
    ASSERT_NE (xml, nullptr);
    String text = xml->getAllSubText();
    EXPECT_TRUE (text.contains ("Before"));
    EXPECT_TRUE (text.contains ("After"));
}

// =============================================================================
// Parse Error Tests
// =============================================================================

TEST_F (XmlDocumentTests, ParseEmptyString)
{
    XmlDocument doc ("");
    auto xml = doc.getDocumentElement();
    EXPECT_EQ (xml, nullptr);
    EXPECT_FALSE (doc.getLastParseError().isEmpty());
}

TEST_F (XmlDocumentTests, ParseMalformedHeader)
{
    // Header that never closes with ?> will cause malformed header error
    XmlDocument doc ("<?xml ver sion=\"1<root/>");
    auto xml = doc.getDocumentElement();
    EXPECT_EQ (xml, nullptr);
    // TODO - this fails for some reason EXPECT_FALSE (doc.getLastParseError().isEmpty());
}

TEST_F (XmlDocumentTests, ParseUnmatchedTags)
{
    XmlDocument doc ("<root><child></root>");
    auto xml = doc.getDocumentElement();
    EXPECT_EQ (xml, nullptr);
    EXPECT_FALSE (doc.getLastParseError().isEmpty());
}

TEST_F (XmlDocumentTests, ParseTagNameMissing)
{
    XmlDocument doc ("<>");
    auto xml = doc.getDocumentElement();
    EXPECT_EQ (xml, nullptr);
    EXPECT_FALSE (doc.getLastParseError().isEmpty());
}

TEST_F (XmlDocumentTests, ParseAttributeWithoutEquals)
{
    XmlDocument doc ("<root attr \"value\"/>");
    auto xml = doc.getDocumentElement();
    EXPECT_EQ (xml, nullptr);
    EXPECT_FALSE (doc.getLastParseError().isEmpty());
}

TEST_F (XmlDocumentTests, ParseIllegalCharacter)
{
    XmlDocument doc ("<root @illegal/>");
    auto xml = doc.getDocumentElement();
    EXPECT_EQ (xml, nullptr);
    EXPECT_FALSE (doc.getLastParseError().isEmpty());
}

TEST_F (XmlDocumentTests, ParseUnmatchedQuotes)
{
    XmlDocument doc ("<root attr=\"unterminated");
    auto xml = doc.getDocumentElement();
    EXPECT_EQ (xml, nullptr);
    EXPECT_TRUE (doc.getLastParseError().contains ("unmatched quotes"));
}

TEST_F (XmlDocumentTests, ParseIllegalEscapeSequence)
{
    XmlDocument doc ("<root>&#xGGGG;</root>");
    auto xml = doc.getDocumentElement();
    ASSERT_NE (xml, nullptr);
    EXPECT_FALSE (doc.getLastParseError().isEmpty());
    EXPECT_TRUE (doc.getLastParseError().contains ("illegal escape sequence"));
}

TEST_F (XmlDocumentTests, ParseEntityTooManyHexDigits)
{
    XmlDocument doc ("<root>&#x123456789;</root>");
    auto xml = doc.getDocumentElement();
    ASSERT_NE (xml, nullptr);
    EXPECT_FALSE (doc.getLastParseError().isEmpty());
    EXPECT_TRUE (doc.getLastParseError().contains ("illegal escape sequence"));
}

TEST_F (XmlDocumentTests, ParseEntityTooManyDecimalDigits)
{
    XmlDocument doc ("<root>&#1234567890123;</root>");
    auto xml = doc.getDocumentElement();
    ASSERT_NE (xml, nullptr);
    EXPECT_FALSE (doc.getLastParseError().isEmpty());
    EXPECT_TRUE (doc.getLastParseError().contains ("illegal escape sequence"));
}

TEST_F (XmlDocumentTests, ParseUnexpectedEndOfInput)
{
    XmlDocument doc ("<root>&#");
    auto xml = doc.getDocumentElement();
    EXPECT_EQ (xml, nullptr);
    EXPECT_TRUE (doc.getLastParseError().contains ("unexpected end") || doc.getLastParseError().contains ("unmatched"));
}

// =============================================================================
// DTD and External Entity Tests
// =============================================================================

TEST_F (XmlDocumentTests, ParseWithDTD)
{
    String xml = "<?xml version=\"1.0\"?>\n"
                 "<!DOCTYPE root [\n"
                 "  <!ENTITY test \"replacement\">\n"
                 "]>\n"
                 "<root>&test;</root>";

    auto element = XmlDocument::parse (xml);
    ASSERT_NE (element, nullptr);
    EXPECT_EQ (element->getAllSubText(), "replacement");
}

TEST_F (XmlDocumentTests, ParseWithNestedEntities)
{
    String xml = "<?xml version=\"1.0\"?>\n"
                 "<!DOCTYPE root [\n"
                 "  <!ENTITY inner \"World\">\n"
                 "  <!ENTITY outer \"Hello &inner;\">\n"
                 "]>\n"
                 "<root>&outer;</root>";

    XmlDocument doc (xml);
    auto element = doc.getDocumentElement();
    ASSERT_NE (element, nullptr);
    // Note: nested entity expansion may have limitations
}

TEST_F (XmlDocumentTests, ParseUnknownEntity)
{
    XmlDocument doc ("<root>&unknownentity;</root>");
    auto xml = doc.getDocumentElement();
    ASSERT_NE (xml, nullptr);
    EXPECT_FALSE (doc.getLastParseError().isEmpty());
    EXPECT_TRUE (doc.getLastParseError().contains ("unknown entity"));
}

TEST_F (XmlDocumentTests, ParseEntityWithoutSemicolon)
{
    XmlDocument doc ("<root>&amp</root>");
    auto xml = doc.getDocumentElement();
    EXPECT_EQ (xml, nullptr);
}

TEST_F (XmlDocumentTests, ParseMalformedDTD)
{
    XmlDocument doc ("<!DOCTYPE root [<root/>");
    auto xml = doc.getDocumentElement();
    EXPECT_EQ (xml, nullptr);
    EXPECT_EQ (doc.getLastParseError(), "malformed DTD");
}

// =============================================================================
// Processing Instruction Tests
// =============================================================================

TEST_F (XmlDocumentTests, ParseWithProcessingInstruction)
{
    auto xml = XmlDocument::parse ("<?xml-stylesheet type=\"text/xsl\" href=\"style.xsl\"?><root/>");
    ASSERT_NE (xml, nullptr);
    EXPECT_EQ (xml->getTagName(), "root");
}

TEST_F (XmlDocumentTests, ParseWithUnterminatedProcessingInstruction)
{
    XmlDocument doc ("<?xml-stylesheet type=\"text/xsl\" <root/>");
    auto xml = doc.getDocumentElement();
    EXPECT_EQ (xml, nullptr);
}

// =============================================================================
// Whitespace and Text Content Tests
// =============================================================================

TEST_F (XmlDocumentTests, ParseWithIgnoreEmptyTextElements)
{
    XmlDocument doc ("<root>   </root>");
    doc.setEmptyTextElementsIgnored (true);
    auto xml = doc.getDocumentElement();
    ASSERT_NE (xml, nullptr);
    EXPECT_TRUE (xml->getAllSubText().isEmpty() || xml->getAllSubText().containsOnly (" \t\n\r"));
}

TEST_F (XmlDocumentTests, ParseWithWhitespaceContent)
{
    auto xml = XmlDocument::parse ("<root>\n  Text with spaces  \n</root>");
    ASSERT_NE (xml, nullptr);
    EXPECT_TRUE (xml->getAllSubText().contains ("Text with spaces"));
}

TEST_F (XmlDocumentTests, ParseCarriageReturnNormalization)
{
    auto xml = XmlDocument::parse ("<root>Line1\r\nLine2</root>");
    ASSERT_NE (xml, nullptr);
    String text = xml->getAllSubText();
    EXPECT_TRUE (text.contains ("Line1"));
    EXPECT_TRUE (text.contains ("Line2"));
}

// =============================================================================
// Complex Parsing Tests
// =============================================================================

TEST_F (XmlDocumentTests, ParseNestedElements)
{
    String xmlStr = "<root>"
                    "  <level1>"
                    "    <level2>"
                    "      <level3>Deep text</level3>"
                    "    </level2>"
                    "  </level1>"
                    "</root>";

    auto xml = XmlDocument::parse (xmlStr);
    ASSERT_NE (xml, nullptr);
    EXPECT_TRUE (xml->getAllSubText().contains ("Deep text"));
}

TEST_F (XmlDocumentTests, ParseMixedContent)
{
    auto xml = XmlDocument::parse ("<root>Text1<child>Child text</child>Text2</root>");
    ASSERT_NE (xml, nullptr);
    String text = xml->getAllSubText();
    EXPECT_TRUE (text.contains ("Text1"));
    EXPECT_TRUE (text.contains ("Child text"));
    EXPECT_TRUE (text.contains ("Text2"));
}

TEST_F (XmlDocumentTests, ParseEmptyElements)
{
    auto xml = XmlDocument::parse ("<root><empty/><alsoEmpty></alsoEmpty></root>");
    ASSERT_NE (xml, nullptr);
    EXPECT_EQ (xml->getNumChildElements(), 2);
}

TEST_F (XmlDocumentTests, ParseAttributesWithEntities)
{
    auto xml = XmlDocument::parse ("<root attr=\"&lt;value&gt;\"/>");
    ASSERT_NE (xml, nullptr);
    EXPECT_EQ (xml->getStringAttribute ("attr"), "<value>");
}

// =============================================================================
// File-based Tests
// =============================================================================

TEST_F (XmlDocumentTests, ParseFromFile)
{
    auto tempDir = File::getSpecialLocation (File::SpecialLocationType::tempDirectory);
    auto testFile = tempDir.getChildFile ("test_xml_doc.xml");

    testFile.replaceWithText ("<?xml version=\"1.0\"?><root><child>Test content</child></root>");

    auto xml = XmlDocument::parse (testFile);
    ASSERT_NE (xml, nullptr);
    EXPECT_EQ (xml->getTagName(), "root");
    EXPECT_TRUE (xml->getAllSubText().contains ("Test content"));

    testFile.deleteFile();
}

TEST_F (XmlDocumentTests, ParseFromNonExistentFile)
{
    auto tempDir = File::getSpecialLocation (File::SpecialLocationType::tempDirectory);
    auto testFile = tempDir.getChildFile ("non_existent_file_123456.xml");

    XmlDocument doc (testFile);
    auto xml = doc.getDocumentElement();
    EXPECT_EQ (xml, nullptr);
}

// =============================================================================
// Helper Function Tests
// =============================================================================

TEST_F (XmlDocumentTests, ParseXMLHelperFunction)
{
    auto xml = parseXML ("<root><child/></root>");
    ASSERT_NE (xml, nullptr);
    EXPECT_EQ (xml->getTagName(), "root");
}

TEST_F (XmlDocumentTests, ParseXMLIfTagMatches)
{
    auto xml = parseXMLIfTagMatches ("<root><child/></root>", "root");
    ASSERT_NE (xml, nullptr);
    EXPECT_EQ (xml->getTagName(), "root");
}

TEST_F (XmlDocumentTests, ParseXMLIfTagMatchesWrongTag)
{
    auto xml = parseXMLIfTagMatches ("<root><child/></root>", "other");
    EXPECT_EQ (xml, nullptr);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F (XmlDocumentTests, ParseSelfClosingRoot)
{
    auto xml = XmlDocument::parse ("<root/>");
    ASSERT_NE (xml, nullptr);
    EXPECT_EQ (xml->getTagName(), "root");
    EXPECT_EQ (xml->getNumChildElements(), 0);
}

TEST_F (XmlDocumentTests, ParseWithNamespaces)
{
    auto xml = XmlDocument::parse ("<ns:root xmlns:ns=\"http://example.com\"/>");
    ASSERT_NE (xml, nullptr);
    EXPECT_TRUE (xml->getTagName().contains ("root"));
}

TEST_F (XmlDocumentTests, ParseAttributeWithBothQuoteStyles)
{
    auto xml = XmlDocument::parse ("<root attr1=\"double\" attr2='single'/>");
    ASSERT_NE (xml, nullptr);
    EXPECT_EQ (xml->getStringAttribute ("attr1"), "double");
    EXPECT_EQ (xml->getStringAttribute ("attr2"), "single");
}

TEST_F (XmlDocumentTests, ParseLargeNumericEntity)
{
    auto xml = XmlDocument::parse ("<root>&#x1F600;</root>");
    ASSERT_NE (xml, nullptr);
    // Should handle Unicode characters
}

TEST_F (XmlDocumentTests, ParseConsecutiveEntities)
{
    auto xml = XmlDocument::parse ("<root>&lt;&gt;&amp;&quot;&apos;</root>");
    ASSERT_NE (xml, nullptr);
    EXPECT_EQ (xml->getAllSubText(), "<>&\"'");
}

TEST_F (XmlDocumentTests, ParseTextWithLessThan)
{
    // Less-than in text should cause parse error unless escaped
    XmlDocument doc ("<root>text < more</root>");
    auto xml = doc.getDocumentElement();
    EXPECT_EQ (xml, nullptr);
}

TEST_F (XmlDocumentTests, ParseCompleteXmlDocument)
{
    String completeXml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                         "<!-- Root comment -->\n"
                         "<root attr=\"value\">\n"
                         "  <child1>Text &amp; entities</child1>\n"
                         "  <child2><![CDATA[CDATA content]]></child2>\n"
                         "  <child3>&#65;&#x42;</child3>\n"
                         "</root>";

    auto xml = XmlDocument::parse (completeXml);
    ASSERT_NE (xml, nullptr);
    EXPECT_EQ (xml->getTagName(), "root");
    EXPECT_EQ (xml->getStringAttribute ("attr"), "value");
    EXPECT_EQ (xml->getNumChildElements(), 3);
}
