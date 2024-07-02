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

#include <gtest/gtest.h>

#include <juce_core/juce_core.h>

class PropertySetTests : public ::testing::Test
{
protected:
    juce::PropertySet propertySet;

    void SetUp() override
    {
        propertySet.setValue ("stringKey", "stringValue");
        propertySet.setValue ("intKey", 123);
        propertySet.setValue ("doubleKey", 45.67);
        propertySet.setValue ("boolKey", true);

        auto xml = std::make_unique<juce::XmlElement> ("root");
        xml->setAttribute ("attribute", "value");
        propertySet.setValue ("xmlKey", xml.get());
    }
};

TEST_F (PropertySetTests, GetValue)
{
    EXPECT_EQ (propertySet.getValue ("stringKey"), "stringValue");
    EXPECT_EQ (propertySet.getIntValue ("intKey"), 123);
    EXPECT_DOUBLE_EQ (propertySet.getDoubleValue ("doubleKey"), 45.67);
    EXPECT_TRUE (propertySet.getBoolValue ("boolKey"));
}

TEST_F (PropertySetTests, GetFallbackValue)
{
    juce::PropertySet fallbackPropertySet;
    fallbackPropertySet.setValue ("fallbackKey", "fallbackValue");
    propertySet.setFallbackPropertySet (&fallbackPropertySet);

    EXPECT_EQ (propertySet.getValue ("fallbackKey"), "fallbackValue");
    propertySet.setFallbackPropertySet (nullptr); // Clear fallback after test

    EXPECT_EQ (propertySet.getValue ("fallbackKey"), "");
}

TEST_F (PropertySetTests, GetXmlValue)
{
    auto xml = propertySet.getXmlValue ("xmlKey");
    ASSERT_TRUE (xml != nullptr);
    EXPECT_EQ (xml->getTagName(), "root");
    EXPECT_EQ (xml->getStringAttribute ("attribute"), "value");

    auto nonExistingXml = propertySet.getXmlValue ("xmlKey2");
    ASSERT_TRUE (nonExistingXml == nullptr);
}

TEST_F (PropertySetTests, NonExistingKey)
{
    EXPECT_EQ (propertySet.getValue ("nonExistingKey", "default"), "default");
    EXPECT_EQ (propertySet.getIntValue ("nonExistingKey", 0), 0);
}

TEST_F (PropertySetTests, RemoveAndClearValues)
{
    propertySet.removeValue ("stringKey");
    EXPECT_FALSE (propertySet.containsKey ("stringKey"));
    EXPECT_TRUE (propertySet.containsKey ("intKey"));
    EXPECT_TRUE (propertySet.containsKey ("doubleKey"));
    EXPECT_TRUE (propertySet.containsKey ("boolKey"));

    propertySet.clear();
    EXPECT_FALSE (propertySet.containsKey ("intKey"));
    EXPECT_FALSE (propertySet.containsKey ("doubleKey"));
    EXPECT_FALSE (propertySet.containsKey ("boolKey"));
}

TEST_F (PropertySetTests, CopyAndAssignment)
{
    juce::PropertySet anotherPropertySet (propertySet);
    EXPECT_EQ (anotherPropertySet.getValue ("stringKey"), "stringValue");

    juce::PropertySet yetAnotherPropertySet;
    yetAnotherPropertySet = propertySet;
    EXPECT_EQ (yetAnotherPropertySet.getValue ("stringKey"), "stringValue");
}

TEST_F (PropertySetTests, CreateAndRestoreXml)
{
    auto xml = propertySet.createXml ("Properties");
    ASSERT_TRUE (xml != nullptr);
    EXPECT_TRUE (xml->hasTagName ("Properties"));

    juce::PropertySet restoredSet;
    restoredSet.restoreFromXml (*xml);
    EXPECT_EQ (restoredSet.getValue ("stringKey"), "stringValue");
}
