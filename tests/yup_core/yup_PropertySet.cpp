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

#include <yup_core/yup_core.h>

namespace
{

struct InvocablePropertySet : yup::PropertySet
{
    InvocablePropertySet (std::function<void()> callback)
        : callback (callback)
    {
    }

    void propertyChanged() override
    {
        callback();
    }

private:
    std::function<void()> callback;
};

} // namespace

class PropertySetTests : public ::testing::Test
{
protected:
    yup::PropertySet propertySet;
    yup::PropertySet propertySetInsensitive { true };

    void SetUp() override
    {
        propertySet.setValue ("stringKey", "stringValue");
        propertySet.setValue ("intKey", 123);
        propertySet.setValue ("doubleKey", 45.67);
        propertySet.setValue ("boolKey", true);

        propertySetInsensitive.setValue ("stringKey", "stringValue");
        propertySetInsensitive.setValue ("intKey", 123);
        propertySetInsensitive.setValue ("doubleKey", 45.67);
        propertySetInsensitive.setValue ("boolKey", true);

        auto xml = std::make_unique<yup::XmlElement> ("root");
        xml->setAttribute ("attribute", "value");
        propertySet.setValue ("xmlKey", xml.get());
    }
};

TEST_F (PropertySetTests, Empty)
{
    yup::PropertySet emptyPropertySet (false);
    EXPECT_EQ (emptyPropertySet.getAllProperties().size(), 0);

    yup::PropertySet emptyPropertySet2 (true);
    EXPECT_EQ (emptyPropertySet2.getAllProperties().size(), 0);
}

TEST_F (PropertySetTests, Copy)
{
    yup::PropertySet anotherPropertySet (propertySet);
    EXPECT_EQ (anotherPropertySet.getAllProperties(), propertySet.getAllProperties());

    yup::PropertySet anotherPropertySet2;
    anotherPropertySet2 = anotherPropertySet;
    EXPECT_EQ (anotherPropertySet2.getAllProperties(), anotherPropertySet.getAllProperties());
}

TEST_F (PropertySetTests, Move)
{
    auto allProperties = propertySet.getAllProperties();

    yup::PropertySet anotherPropertySet (std::move (propertySet));
    EXPECT_EQ (anotherPropertySet.getAllProperties(), allProperties);

    yup::PropertySet anotherPropertySet2;
    anotherPropertySet2 = std::move (anotherPropertySet);
    EXPECT_EQ (anotherPropertySet2.getAllProperties(), allProperties);
}

TEST_F (PropertySetTests, GetValue)
{
    EXPECT_EQ (propertySet.getValue ("stringKey"), "stringValue");
    EXPECT_EQ (propertySet.getIntValue ("intKey"), 123);
    EXPECT_DOUBLE_EQ (propertySet.getDoubleValue ("doubleKey"), 45.67);
    EXPECT_TRUE (propertySet.getBoolValue ("boolKey"));

    EXPECT_EQ (propertySetInsensitive.getValue ("stringkey"), "stringValue");
    EXPECT_EQ (propertySetInsensitive.getIntValue ("intkey"), 123);
    EXPECT_DOUBLE_EQ (propertySetInsensitive.getDoubleValue ("doublekey"), 45.67);
    EXPECT_TRUE (propertySetInsensitive.getBoolValue ("boolkey"));
}

TEST_F (PropertySetTests, GetFallbackValue)
{
    yup::PropertySet fallbackPropertySet;
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
    EXPECT_DOUBLE_EQ (propertySet.getDoubleValue ("nonExistingKey", 45.67), 45.67);
    EXPECT_TRUE (propertySet.getBoolValue ("nonExistingKey", true));
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

TEST_F (PropertySetTests, AddAllPropertiesFrom)
{
    yup::PropertySet anotherPropertySet;
    anotherPropertySet.setValue ("stringKey", "stringValue2");
    anotherPropertySet.setValue ("intKey", 456);
    anotherPropertySet.setValue ("double2Key", 45.67);
    anotherPropertySet.setValue ("bool2Key", true);
    anotherPropertySet.addAllPropertiesFrom (propertySet);

    EXPECT_EQ (anotherPropertySet.getValue ("stringKey"), "stringValue");
    EXPECT_EQ (anotherPropertySet.getIntValue ("intKey"), 123);
    EXPECT_DOUBLE_EQ (anotherPropertySet.getDoubleValue ("doubleKey"), 45.67);
    EXPECT_TRUE (anotherPropertySet.getBoolValue ("boolKey"));
    EXPECT_DOUBLE_EQ (anotherPropertySet.getDoubleValue ("double2Key"), 45.67);
    EXPECT_TRUE (anotherPropertySet.getBoolValue ("bool2Key"));
}

TEST_F (PropertySetTests, CreateAndRestoreXml)
{
    auto xml = propertySet.createXml ("Properties");
    ASSERT_TRUE (xml != nullptr);
    EXPECT_TRUE (xml->hasTagName ("Properties"));

    yup::PropertySet restoredSet;
    restoredSet.restoreFromXml (*xml);
    EXPECT_EQ (restoredSet.getValue ("stringKey"), "stringValue");
}

TEST_F (PropertySetTests, PropertyChanged)
{
    bool changed = false;

    InvocablePropertySet anotherPropertySet ([&changed]
    {
        changed = true;
    });

    EXPECT_FALSE (changed);
    anotherPropertySet.setValue ("abc", 1);
    EXPECT_TRUE (changed);
}
