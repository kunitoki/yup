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

#include <gtest/gtest.h>

#include <yup_core/yup_core.h>

#include <map>
#include <memory>

using namespace yup;

// ==============================================================================
// XmlElement Tests
// ==============================================================================

class XmlElementTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup common test data
        simpleXml = "<root><child attr='value'>text</child></root>";
        complexXml = R"(<document version="1.0" encoding="UTF-8">
            <header>
                <title>Test Document</title>
                <author>Test Author</author>
            </header>
            <body>
                <paragraph id="1">First paragraph</paragraph>
                <paragraph id="2">Second paragraph</paragraph>
                <list>
                    <item>Item 1</item>
                    <item>Item 2</item>
                </list>
            </body>
        </document>)";
    }

    String simpleXml;
    String complexXml;
};

// Test float formatting (existing test)
TEST_F (XmlElementTest, FloatFormatting)
{
    auto element = std::make_unique<XmlElement> ("test");
    Identifier number ("number");

    std::map<double, String> tests;
    tests[1] = "1.0";
    tests[1.1] = "1.1";
    tests[1.01] = "1.01";
    tests[0.76378] = "0.76378";
    tests[-10] = "-10.0";
    tests[10.01] = "10.01";
    tests[0.0123] = "0.0123";
    tests[-3.7e-27] = "-3.7e-27";
    tests[1e+40] = "1.0e40";
    tests[-12345678901234567.0] = "-1.234567890123457e16";
    tests[192000] = "192000.0";
    tests[1234567] = "1.234567e6";
    tests[0.00006] = "0.00006";
    tests[0.000006] = "6.0e-6";

    for (auto& test : tests)
    {
        element->setAttribute (number, test.first);
        EXPECT_EQ (element->getStringAttribute (number), test.second);
    }
}

// Test all constructor variants
TEST_F (XmlElementTest, Constructors)
{
    // Test String constructor
    XmlElement element1 ("testElement");
    EXPECT_EQ (element1.getTagName(), "testElement");

    // Test char* constructor
    XmlElement element2 ("testElement2");
    EXPECT_EQ (element2.getTagName(), "testElement2");

    // Test Identifier constructor
    Identifier id ("testElement3");
    XmlElement element3 (id);
    EXPECT_EQ (element3.getTagName(), "testElement3");

    // Test StringRef constructor
    StringRef ref ("testElement4");
    XmlElement element4 (ref);
    EXPECT_EQ (element4.getTagName(), "testElement4");

    // Test copy constructor
    element1.setAttribute ("attr", "value");
    XmlElement element5 (element1);
    EXPECT_EQ (element5.getTagName(), "testElement");
    EXPECT_EQ (element5.getStringAttribute ("attr"), "value");

    // Test move constructor
    XmlElement element6 (std::move (element1));
    EXPECT_EQ (element6.getTagName(), "testElement");
    EXPECT_EQ (element6.getStringAttribute ("attr"), "value");

    // Test copy assignment
    XmlElement element7 ("temp");
    element7 = element5;
    EXPECT_EQ (element7.getTagName(), "testElement");
    EXPECT_EQ (element7.getStringAttribute ("attr"), "value");

    // Test move assignment
    XmlElement element8 ("temp");
    element8 = std::move (element5);
    EXPECT_EQ (element8.getTagName(), "testElement");
    EXPECT_EQ (element8.getStringAttribute ("attr"), "value");
}

// Test tag name operations
TEST_F (XmlElementTest, TagNameOperations)
{
    XmlElement element ("ns:tagName");

    // Test basic tag name
    EXPECT_EQ (element.getTagName(), "ns:tagName");

    // Test hasTagName
    EXPECT_TRUE (element.hasTagName ("ns:tagName"));
    EXPECT_FALSE (element.hasTagName ("otherTag"));

    // Test namespace operations
    EXPECT_EQ (element.getNamespace(), "ns");
    EXPECT_EQ (element.getTagNameWithoutNamespace(), "tagName");

    // Test hasTagNameIgnoringNamespace
    EXPECT_TRUE (element.hasTagNameIgnoringNamespace ("tagName"));
    EXPECT_TRUE (element.hasTagNameIgnoringNamespace ("ns:tagName"));
    EXPECT_FALSE (element.hasTagNameIgnoringNamespace ("otherTag"));

    // Test setTagName
    element.setTagName ("newTag");
    EXPECT_EQ (element.getTagName(), "newTag");

    // Test empty namespace
    XmlElement element2 ("simpleTag");
    EXPECT_EQ (element2.getNamespace(), "");
    EXPECT_EQ (element2.getTagNameWithoutNamespace(), "simpleTag");
}

// Test all attribute operations
TEST_F (XmlElementTest, AttributeOperations)
{
    XmlElement element ("test");

    // Test attribute count
    EXPECT_EQ (element.getNumAttributes(), 0);

    // Test string attributes
    element.setAttribute ("stringAttr", "testValue");
    EXPECT_EQ (element.getNumAttributes(), 1);
    EXPECT_TRUE (element.hasAttribute ("stringAttr"));
    EXPECT_EQ (element.getStringAttribute ("stringAttr"), "testValue");
    EXPECT_EQ (element.getStringAttribute ("nonExistent", "default"), "default");

    // Test integer attributes
    element.setAttribute ("intAttr", 42);
    EXPECT_EQ (element.getIntAttribute ("intAttr"), 42);
    EXPECT_EQ (element.getIntAttribute ("nonExistent", 99), 99);

    // Test float attributes
    element.setAttribute ("floatAttr", 3.14);
    EXPECT_FLOAT_EQ (element.getFloatAttribute ("floatAttr"), 3.14f);
    EXPECT_FLOAT_EQ (element.getFloatAttribute ("nonExistent", 1.5f), 1.5f);

    // Test double attributes
    element.setAttribute ("doubleAttr", 3.14159);
    EXPECT_DOUBLE_EQ (element.getDoubleAttribute ("doubleAttr"), 3.14159);
    EXPECT_DOUBLE_EQ (element.getDoubleAttribute ("nonExistent", 2.71), 2.71);

    // Test boolean attributes
    element.setAttribute ("boolAttr1", "true");
    element.setAttribute ("boolAttr2", "1");
    element.setAttribute ("boolAttr3", "y");
    element.setAttribute ("boolAttr4", "T");
    element.setAttribute ("boolAttr5", "Y");
    element.setAttribute ("boolAttr6", "false");
    element.setAttribute ("boolAttr7", "0");

    EXPECT_TRUE (element.getBoolAttribute ("boolAttr1"));
    EXPECT_TRUE (element.getBoolAttribute ("boolAttr2"));
    EXPECT_TRUE (element.getBoolAttribute ("boolAttr3"));
    EXPECT_TRUE (element.getBoolAttribute ("boolAttr4"));
    EXPECT_TRUE (element.getBoolAttribute ("boolAttr5"));
    EXPECT_FALSE (element.getBoolAttribute ("boolAttr6"));
    EXPECT_FALSE (element.getBoolAttribute ("boolAttr7"));
    EXPECT_TRUE (element.getBoolAttribute ("nonExistent", true));

    // Test compareAttribute
    EXPECT_TRUE (element.compareAttribute ("stringAttr", "testValue"));
    EXPECT_FALSE (element.compareAttribute ("stringAttr", "otherValue"));
    EXPECT_TRUE (element.compareAttribute ("stringAttr", "TESTVALUE", true));   // ignore case
    EXPECT_FALSE (element.compareAttribute ("stringAttr", "TESTVALUE", false)); // case sensitive

    // Test attribute by index
    EXPECT_GT (element.getNumAttributes(), 0);
    EXPECT_FALSE (element.getAttributeName (0).isEmpty());
    EXPECT_FALSE (element.getAttributeValue (0).isEmpty());

    // Test out of bounds
    EXPECT_TRUE (element.getAttributeName (999).isEmpty());
    EXPECT_TRUE (element.getAttributeValue (999).isEmpty());

    // Test remove attribute
    element.removeAttribute ("stringAttr");
    EXPECT_FALSE (element.hasAttribute ("stringAttr"));

    // Test remove all attributes
    element.removeAllAttributes();
    EXPECT_EQ (element.getNumAttributes(), 0);
}

// Test child element operations
TEST_F (XmlElementTest, ChildElementOperations)
{
    XmlElement parent ("parent");

    // Test empty parent
    EXPECT_EQ (parent.getNumChildElements(), 0);
    EXPECT_EQ (parent.getFirstChildElement(), nullptr);
    EXPECT_EQ (parent.getChildElement (0), nullptr);
    EXPECT_EQ (parent.getChildByName ("child"), nullptr);
    EXPECT_EQ (parent.getChildByAttribute ("attr", "value"), nullptr);

    // Test addChildElement
    auto child1 = new XmlElement ("child1");
    child1->setAttribute ("id", "1");
    parent.addChildElement (child1);

    EXPECT_EQ (parent.getNumChildElements(), 1);
    EXPECT_EQ (parent.getFirstChildElement(), child1);
    EXPECT_EQ (parent.getChildElement (0), child1);
    EXPECT_EQ (parent.getChildByName ("child1"), child1);
    EXPECT_EQ (parent.getChildByAttribute ("id", "1"), child1);

    // Test prependChildElement
    auto child2 = new XmlElement ("child2");
    child2->setAttribute ("id", "2");
    parent.prependChildElement (child2);

    EXPECT_EQ (parent.getNumChildElements(), 2);
    EXPECT_EQ (parent.getFirstChildElement(), child2); // prepended, so first
    EXPECT_EQ (parent.getChildElement (0), child2);
    EXPECT_EQ (parent.getChildElement (1), child1);

    // Test insertChildElement
    auto child3 = new XmlElement ("child3");
    child3->setAttribute ("id", "3");
    parent.insertChildElement (child3, 1);

    EXPECT_EQ (parent.getNumChildElements(), 3);
    EXPECT_EQ (parent.getChildElement (0), child2);
    EXPECT_EQ (parent.getChildElement (1), child3);
    EXPECT_EQ (parent.getChildElement (2), child1);

    // Test createNewChildElement
    auto child4 = parent.createNewChildElement ("child4");
    EXPECT_EQ (parent.getNumChildElements(), 4);
    EXPECT_EQ (child4->getTagName(), "child4");
    EXPECT_TRUE (parent.containsChildElement (child4));

    // Test replaceChildElement
    auto replacement = new XmlElement ("replacement");
    EXPECT_TRUE (parent.replaceChildElement (child3, replacement));
    EXPECT_EQ (parent.getNumChildElements(), 4);
    EXPECT_EQ (parent.getChildElement (1), replacement);
    EXPECT_FALSE (parent.containsChildElement (child3));

    // Test removeChildElement
    parent.removeChildElement (child1, false); // don't delete
    EXPECT_EQ (parent.getNumChildElements(), 3);
    EXPECT_FALSE (parent.containsChildElement (child1));
    delete child1; // clean up

    // Test deleteAllChildElementsWithTagName
    parent.addChildElement (new XmlElement ("child2")); // add another child2
    EXPECT_EQ (parent.getNumChildElements(), 4);
    parent.deleteAllChildElementsWithTagName ("child2");
    EXPECT_EQ (parent.getNumChildElements(), 2);

    // Test getNextElement and getNextElementWithTagName
    auto firstChild = parent.getFirstChildElement();
    EXPECT_NE (firstChild, nullptr);
    auto nextChild = firstChild->getNextElement();
    EXPECT_NE (nextChild, nullptr);

    // Test deleteAllChildElements
    parent.deleteAllChildElements();
    EXPECT_EQ (parent.getNumChildElements(), 0);
}

// Test findParentElementOf
TEST_F (XmlElementTest, FindParentElement)
{
    XmlElement root ("root");
    auto child1 = new XmlElement ("child1");
    auto grandchild = new XmlElement ("grandchild");

    child1->addChildElement (grandchild);
    root.addChildElement (child1);

    EXPECT_EQ (root.findParentElementOf (child1), &root);
    EXPECT_EQ (root.findParentElementOf (grandchild), child1);
    EXPECT_EQ (root.findParentElementOf (&root), nullptr); // can't be parent of itself

    XmlElement separate ("separate");
    EXPECT_EQ (root.findParentElementOf (&separate), nullptr);
}

// Test text operations
TEST_F (XmlElementTest, TextOperations)
{
    // Test regular element text operations
    XmlElement element ("test");
    element.addTextElement ("Hello ");
    element.addTextElement ("World");

    EXPECT_EQ (element.getAllSubText(), "Hello World");
    EXPECT_EQ (element.getChildElementAllSubText ("nonExistent", "default"), "default");

    // Test text element
    auto textElement = XmlElement::createTextElement ("Test Text");
    EXPECT_TRUE (textElement->isTextElement());
    EXPECT_EQ (textElement->getText(), "Test Text");

    textElement->setText ("Modified Text");
    EXPECT_EQ (textElement->getText(), "Modified Text");

    delete textElement;

    // Test deleteAllTextElements
    element.deleteAllTextElements();
    EXPECT_EQ (element.getAllSubText(), "");

    // Test complex text structure
    XmlElement complex ("complex");
    complex.addTextElement ("Start ");
    auto child = new XmlElement ("child");
    child->addTextElement ("Middle");
    complex.addChildElement (child);
    complex.addTextElement (" End");

    EXPECT_EQ (complex.getAllSubText(), "Start Middle End");

    // Test getChildElementAllSubText
    auto namedChild = new XmlElement ("named");
    namedChild->addTextElement ("Named Content");
    complex.addChildElement (namedChild);

    EXPECT_EQ (complex.getChildElementAllSubText ("named", "default"), "Named Content");
    EXPECT_EQ (complex.getChildElementAllSubText ("nonExistent", "default"), "default");
}

// Test isEquivalentTo
TEST_F (XmlElementTest, IsEquivalentTo)
{
    XmlElement element1 ("test");
    element1.setAttribute ("attr1", "value1");
    element1.setAttribute ("attr2", "value2");

    XmlElement element2 ("test");
    element2.setAttribute ("attr1", "value1");
    element2.setAttribute ("attr2", "value2");

    // Test equivalent elements
    EXPECT_TRUE (element1.isEquivalentTo (&element2, false));
    EXPECT_TRUE (element1.isEquivalentTo (&element2, true));

    // Test different tag names
    XmlElement element3 ("different");
    element3.setAttribute ("attr1", "value1");
    element3.setAttribute ("attr2", "value2");
    EXPECT_FALSE (element1.isEquivalentTo (&element3, false));

    // Test different attribute values
    XmlElement element4 ("test");
    element4.setAttribute ("attr1", "value1");
    element4.setAttribute ("attr2", "differentValue");
    EXPECT_FALSE (element1.isEquivalentTo (&element4, false));

    // Test different number of attributes
    XmlElement element5 ("test");
    element5.setAttribute ("attr1", "value1");
    EXPECT_FALSE (element1.isEquivalentTo (&element5, false));

    // Test different attribute order (should matter when ignoreOrderOfAttributes is false)
    XmlElement element6 ("test");
    element6.setAttribute ("attr2", "value2");
    element6.setAttribute ("attr1", "value1");
    EXPECT_FALSE (element1.isEquivalentTo (&element6, false)); // order matters
    EXPECT_TRUE (element1.isEquivalentTo (&element6, true));   // order ignored

    // Test with children
    element1.addChildElement (new XmlElement ("child1"));
    element2.addChildElement (new XmlElement ("child1"));
    EXPECT_TRUE (element1.isEquivalentTo (&element2, false));

    element2.addChildElement (new XmlElement ("child2"));
    EXPECT_FALSE (element1.isEquivalentTo (&element2, false));

    // Test null comparison
    EXPECT_FALSE (element1.isEquivalentTo (nullptr, false));

    // Test self comparison
    EXPECT_TRUE (element1.isEquivalentTo (&element1, false));
}

// Test XML formatting and output
TEST_F (XmlElementTest, FormattingAndOutput)
{
    XmlElement element ("root");
    element.setAttribute ("version", "1.0");

    auto child = new XmlElement ("child");
    child->setAttribute ("id", "1");
    child->addTextElement ("Hello World");
    element.addChildElement (child);

    // Test default formatting
    String xml = element.toString();
    EXPECT_TRUE (xml.contains ("<?xml"));
    EXPECT_TRUE (xml.contains ("<root"));
    EXPECT_TRUE (xml.contains ("version=\"1.0\""));
    EXPECT_TRUE (xml.contains ("<child"));
    EXPECT_TRUE (xml.contains ("Hello World"));

    // Test custom formatting
    XmlElement::TextFormat format;
    format.addDefaultHeader = false;
    format.newLineChars = nullptr; // single line

    String compactXml = element.toString (format);
    EXPECT_FALSE (compactXml.contains ("<?xml"));
    EXPECT_FALSE (compactXml.contains ("\n"));

    // Test singleLine format
    String singleLineXml = element.toString (XmlElement::TextFormat().singleLine());
    EXPECT_FALSE (singleLineXml.contains ("\n"));

    // Test withoutHeader format
    String noHeaderXml = element.toString (XmlElement::TextFormat().withoutHeader());
    EXPECT_FALSE (noHeaderXml.contains ("<?xml"));

    // Test writeTo file
    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test.xml");
    EXPECT_TRUE (element.writeTo (tempFile));
    EXPECT_TRUE (tempFile.exists());

    String fileContents = tempFile.loadFileAsString();
    EXPECT_TRUE (fileContents.contains ("<root"));

    tempFile.deleteFile();
}

// Test XML validation
TEST_F (XmlElementTest, XmlValidation)
{
    // Test valid XML names
    EXPECT_TRUE (XmlElement::isValidXmlName ("validName"));
    EXPECT_TRUE (XmlElement::isValidXmlName ("valid_name"));
    EXPECT_TRUE (XmlElement::isValidXmlName ("valid-name"));
    EXPECT_TRUE (XmlElement::isValidXmlName ("valid.name"));
    EXPECT_TRUE (XmlElement::isValidXmlName ("valid123"));
    EXPECT_TRUE (XmlElement::isValidXmlName ("_validName"));
    EXPECT_TRUE (XmlElement::isValidXmlName ("ns:validName"));

    // Test invalid XML names
    EXPECT_FALSE (XmlElement::isValidXmlName (""));
    EXPECT_FALSE (XmlElement::isValidXmlName ("123invalid"));
    EXPECT_FALSE (XmlElement::isValidXmlName ("invalid name"));
}

// Test iterators
TEST_F (XmlElementTest, Iterators)
{
    XmlElement parent ("parent");
    parent.addChildElement (new XmlElement ("child1"));
    parent.addChildElement (new XmlElement ("child2"));
    parent.addChildElement (new XmlElement ("child1")); // duplicate tag name
    parent.addChildElement (new XmlElement ("child3"));

    // Test general child iterator
    int count = 0;
    for (auto* child : parent.getChildIterator())
    {
        EXPECT_NE (child, nullptr);
        count++;
    }
    EXPECT_EQ (count, 4);

    // Test filtered iterator
    int child1Count = 0;
    for (auto* child : parent.getChildWithTagNameIterator ("child1"))
    {
        EXPECT_EQ (child->getTagName(), "child1");
        child1Count++;
    }
    EXPECT_EQ (child1Count, 2);

    // Test non-existent tag
    int nonExistentCount = 0;
    for (auto* child : parent.getChildWithTagNameIterator ("nonExistent"))
    {
        nonExistentCount++;
    }
    EXPECT_EQ (nonExistentCount, 0);
}

// Test sorting children
TEST_F (XmlElementTest, SortChildren)
{
    XmlElement parent ("parent");

    auto child1 = new XmlElement ("child");
    child1->setAttribute ("order", "3");
    parent.addChildElement (child1);

    auto child2 = new XmlElement ("child");
    child2->setAttribute ("order", "1");
    parent.addChildElement (child2);

    auto child3 = new XmlElement ("child");
    child3->setAttribute ("order", "2");
    parent.addChildElement (child3);

    // Create a simple comparator
    struct OrderComparator
    {
        static int compareElements (const XmlElement* first, const XmlElement* second)
        {
            int firstOrder = first->getIntAttribute ("order");
            int secondOrder = second->getIntAttribute ("order");
            return firstOrder - secondOrder;
        }
    };

    // Sort children
    OrderComparator comparator;
    parent.sortChildElements (comparator);

    // Check order
    EXPECT_EQ (parent.getChildElement (0)->getIntAttribute ("order"), 1);
    EXPECT_EQ (parent.getChildElement (1)->getIntAttribute ("order"), 2);
    EXPECT_EQ (parent.getChildElement (2)->getIntAttribute ("order"), 3);
}

// Test edge cases and error conditions
TEST_F (XmlElementTest, EdgeCases)
{
    // Test empty element
    XmlElement empty ("empty");
    EXPECT_EQ (empty.getNumAttributes(), 0);
    EXPECT_EQ (empty.getNumChildElements(), 0);
    EXPECT_EQ (empty.getAllSubText(), "");

    // Test element with only whitespace text
    XmlElement whitespace ("whitespace");
    whitespace.addTextElement ("   \n\t  ");
    EXPECT_EQ (whitespace.getAllSubText().trim(), "");

    // Test deeply nested structure
    XmlElement deep ("level1");
    auto current = &deep;
    for (int i = 2; i <= 10; ++i)
    {
        auto child = new XmlElement ("level" + String (i));
        current->addChildElement (child);
        current = child;
    }

    EXPECT_EQ (deep.getNumChildElements(), 1);
    EXPECT_NE (deep.findParentElementOf (current), nullptr);

    // Test null pointer safety
    XmlElement safe ("safe");
    safe.addChildElement (nullptr);          // should not crash
    safe.removeChildElement (nullptr, true); // should not crash
    safe.insertChildElement (nullptr, 0);    // should not crash
    safe.prependChildElement (nullptr);      // should not crash
    EXPECT_FALSE (safe.replaceChildElement (nullptr, nullptr));
}

// ==============================================================================
// XmlDocument Tests
// ==============================================================================

class XmlDocumentTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        validXml = R"(<?xml version="1.0" encoding="UTF-8"?>
        <root>
            <child id="1">First child</child>
            <child id="2">Second child</child>
        </root>)";

        invalidXml = R"(<root>
            <child id="1">Unclosed child
            <child id="2">Second child</child>
        </root>)";

        xmlWithDTD = R"(<?xml version="1.0"?>
        <!DOCTYPE root [
            <!ELEMENT root (child*)>
            <!ELEMENT child (#PCDATA)>
        ]>
        <root>
            <child>Content</child>
        </root>)";

        emptyXml = "";

        // Create temporary files
        tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test.xml");
        tempFile.replaceWithText (validXml);

        invalidFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("invalid.xml");
        invalidFile.replaceWithText (invalidXml);
    }

    void TearDown() override
    {
        tempFile.deleteFile();
        invalidFile.deleteFile();
    }

    String validXml;
    String invalidXml;
    String xmlWithDTD;
    String emptyXml;
    File tempFile;
    File invalidFile;
};

// Test XmlDocument constructors
TEST_F (XmlDocumentTest, Constructors)
{
    // Test string constructor
    XmlDocument doc1 (validXml);
    EXPECT_TRUE (doc1.getLastParseError().isEmpty());

    auto element1 = doc1.getDocumentElement();
    EXPECT_NE (element1, nullptr);
    EXPECT_EQ (element1->getTagName(), "root");

    // Test file constructor
    XmlDocument doc2 (tempFile);
    EXPECT_TRUE (doc2.getLastParseError().isEmpty());

    auto element2 = doc2.getDocumentElement();
    EXPECT_NE (element2, nullptr);
    EXPECT_EQ (element2->getTagName(), "root");

    // Test non-existent file
    File nonExistent = File::getSpecialLocation (File::tempDirectory).getChildFile ("nonexistent.xml");
    XmlDocument doc3 (nonExistent);
    auto element3 = doc3.getDocumentElement();
    EXPECT_EQ (element3, nullptr);
    EXPECT_FALSE (doc3.getLastParseError().isEmpty());
}

// Test parsing methods
TEST_F (XmlDocumentTest, ParsingMethods)
{
    // Test getDocumentElement
    XmlDocument doc (validXml);
    auto element = doc.getDocumentElement();
    EXPECT_NE (element, nullptr);
    EXPECT_EQ (element->getTagName(), "root");
    EXPECT_EQ (element->getNumChildElements(), 2);

    // Test getDocumentElement with onlyReadOuterDocumentElement
    auto outerElement = doc.getDocumentElement (true);
    EXPECT_NE (outerElement, nullptr);
    EXPECT_EQ (outerElement->getTagName(), "root");

    // Test getDocumentElementIfTagMatches
    auto matchedElement = doc.getDocumentElementIfTagMatches ("root");
    EXPECT_NE (matchedElement, nullptr);
    EXPECT_EQ (matchedElement->getTagName(), "root");

    auto nonMatchedElement = doc.getDocumentElementIfTagMatches ("nonexistent");
    EXPECT_EQ (nonMatchedElement, nullptr);

    // Test static parse methods
    auto parsedFromString = XmlDocument::parse (validXml);
    EXPECT_NE (parsedFromString, nullptr);
    EXPECT_EQ (parsedFromString->getTagName(), "root");

    auto parsedFromFile = XmlDocument::parse (tempFile);
    EXPECT_NE (parsedFromFile, nullptr);
    EXPECT_EQ (parsedFromFile->getTagName(), "root");
}

// Test global parse functions
TEST_F (XmlDocumentTest, GlobalParseFunctions)
{
    // Test parseXML
    auto element1 = parseXML (validXml);
    EXPECT_NE (element1, nullptr);
    EXPECT_EQ (element1->getTagName(), "root");

    auto element2 = parseXML (tempFile);
    EXPECT_NE (element2, nullptr);
    EXPECT_EQ (element2->getTagName(), "root");

    // Test parseXMLIfTagMatches
    auto matched1 = parseXMLIfTagMatches (validXml, "root");
    EXPECT_NE (matched1, nullptr);
    EXPECT_EQ (matched1->getTagName(), "root");

    auto matched2 = parseXMLIfTagMatches (tempFile, "root");
    EXPECT_NE (matched2, nullptr);
    EXPECT_EQ (matched2->getTagName(), "root");

    auto nonMatched1 = parseXMLIfTagMatches (validXml, "nonexistent");
    EXPECT_EQ (nonMatched1, nullptr);

    auto nonMatched2 = parseXMLIfTagMatches (tempFile, "nonexistent");
    EXPECT_EQ (nonMatched2, nullptr);
}

// Test error handling
TEST_F (XmlDocumentTest, ErrorHandling)
{
    // Test invalid XML
    XmlDocument invalidDoc (invalidXml);
    auto element = invalidDoc.getDocumentElement();
    EXPECT_EQ (element, nullptr);
    EXPECT_FALSE (invalidDoc.getLastParseError().isEmpty());

    // Test empty XML
    XmlDocument emptyDoc (emptyXml);
    auto emptyElement = emptyDoc.getDocumentElement();
    EXPECT_EQ (emptyElement, nullptr);
    EXPECT_FALSE (emptyDoc.getLastParseError().isEmpty());

    // Test invalid file
    XmlDocument invalidFileDoc (invalidFile);
    auto invalidElement = invalidFileDoc.getDocumentElement();
    EXPECT_EQ (invalidElement, nullptr);
    EXPECT_FALSE (invalidFileDoc.getLastParseError().isEmpty());

    // Test error persistence
    String firstError = invalidDoc.getLastParseError();
    invalidDoc.getDocumentElement(); // call again
    EXPECT_EQ (invalidDoc.getLastParseError(), firstError);
}

// Test document options
TEST_F (XmlDocumentTest, DocumentOptions)
{
    XmlDocument doc (validXml);

    // Test setEmptyTextElementsIgnored
    doc.setEmptyTextElementsIgnored (true);
    auto element1 = doc.getDocumentElement();
    EXPECT_NE (element1, nullptr);

    doc.setEmptyTextElementsIgnored (false);
    auto element2 = doc.getDocumentElement();
    EXPECT_NE (element2, nullptr);

    // Note: The actual behavior difference would be more apparent with XML containing empty text elements

    // Test setInputSource (basic test - actual functionality depends on InputSource implementation)
    doc.setInputSource (nullptr); // should not crash
}

// Test DTD handling
TEST_F (XmlDocumentTest, DTDHandling)
{
    XmlDocument doc (xmlWithDTD);
    auto element = doc.getDocumentElement();
    EXPECT_NE (element, nullptr);
    EXPECT_EQ (element->getTagName(), "root");
    EXPECT_TRUE (doc.getLastParseError().isEmpty());
}

// Test encoding handling
TEST_F (XmlDocumentTest, EncodingHandling)
{
    // Test UTF-8 encoding
    String utf8Xml = R"(<?xml version="1.0" encoding="UTF-8"?>
    <root>
        <text>Hello World</text>
    </root>)";

    XmlDocument doc (utf8Xml);
    auto element = doc.getDocumentElement();
    EXPECT_NE (element, nullptr);
    EXPECT_TRUE (doc.getLastParseError().isEmpty());

    // Test without encoding declaration
    String noEncodingXml = R"(<?xml version="1.0"?>
    <root>
        <text>Hello World</text>
    </root>)";

    XmlDocument doc2 (noEncodingXml);
    auto element2 = doc2.getDocumentElement();
    EXPECT_NE (element2, nullptr);
    EXPECT_TRUE (doc2.getLastParseError().isEmpty());
}

// Test complex XML structures
TEST_F (XmlDocumentTest, ComplexStructures)
{
    String complexXml = R"(<?xml version="1.0"?>
    <document>
        <metadata>
            <title>Test Document</title>
            <author>Test Author</author>
            <date>2024-01-01</date>
        </metadata>
        <content>
            <section id="1">
                <title>Introduction</title>
                <paragraph>This is the introduction.</paragraph>
                <paragraph>This is another paragraph.</paragraph>
            </section>
            <section id="2">
                <title>Body</title>
                <paragraph>This is the body content.</paragraph>
                <list>
                    <item>Item 1</item>
                    <item>Item 2</item>
                    <item>Item 3</item>
                </list>
            </section>
        </content>
    </document>)";

    XmlDocument doc (complexXml);
    auto root = doc.getDocumentElement();
    EXPECT_NE (root, nullptr);
    EXPECT_EQ (root->getTagName(), "document");

    auto metadata = root->getChildByName ("metadata");
    EXPECT_NE (metadata, nullptr);
    EXPECT_EQ (metadata->getChildByName ("title")->getAllSubText(), "Test Document");

    auto content = root->getChildByName ("content");
    EXPECT_NE (content, nullptr);
    EXPECT_EQ (content->getNumChildElements(), 2);

    auto section1 = content->getChildByAttribute ("id", "1");
    EXPECT_NE (section1, nullptr);
    EXPECT_EQ (section1->getChildByName ("title")->getAllSubText(), "Introduction");

    auto list = content->getChildByAttribute ("id", "2")->getChildByName ("list");
    EXPECT_NE (list, nullptr);
    EXPECT_EQ (list->getNumChildElements(), 3);
}

// Test performance and large documents
TEST_F (XmlDocumentTest, PerformanceTest)
{
    // Generate a large XML document
    MemoryOutputStream xmlStream;
    xmlStream << "<?xml version=\"1.0\"?>\n<root>\n";

    for (int i = 0; i < 1000; ++i)
    {
        xmlStream << "  <item id=\"" << i << "\">Item " << i << "</item>\n";
    }

    xmlStream << "</root>\n";

    String largeXml = xmlStream.toString();

    // Test parsing performance
    XmlDocument doc (largeXml);
    auto startTime = Time::getMillisecondCounter();
    auto element = doc.getDocumentElement();
    auto endTime = Time::getMillisecondCounter();

    EXPECT_NE (element, nullptr);
    EXPECT_EQ (element->getTagName(), "root");
    EXPECT_EQ (element->getNumChildElements(), 1000);

    // Performance should be reasonable (less than 1 second for 1000 elements)
    EXPECT_LT (endTime - startTime, 1000);
}
