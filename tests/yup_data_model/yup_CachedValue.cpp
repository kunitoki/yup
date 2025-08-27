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

#include <yup_data_model/yup_data_model.h>

#include <atomic>
#include <thread>

using namespace yup;

namespace
{
constexpr int kDefaultIntValue = 42;
constexpr double kDefaultDoubleValue = 3.14159;
const Identifier kTestPropertyName = "testProperty";
const Identifier kAnotherPropertyName = "anotherProperty";

// Custom test types for VariantConverter testing
struct Point
{
    int x = 0;
    int y = 0;

    Point() = default;

    Point (int x_, int y_)
        : x (x_)
        , y (y_)
    {
    }

    bool operator== (const Point& other) const
    {
        return x == other.x && y == other.y;
    }

    bool operator!= (const Point& other) const
    {
        return ! (*this == other);
    }
};

struct Color
{
    uint8 r = 0, g = 0, b = 0, a = 255;

    Color() = default;

    Color (uint8 red, uint8 green, uint8 blue, uint8 alpha = 255)
        : r (red)
        , g (green)
        , b (blue)
        , a (alpha)
    {
    }

    bool operator== (const Color& other) const
    {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    bool operator!= (const Color& other) const
    {
        return ! (*this == other);
    }
};

// Test type that throws on invalid conversion
struct StrictPoint
{
    int x = 0;
    int y = 0;

    StrictPoint() = default;

    StrictPoint (int x_, int y_)
        : x (x_)
        , y (y_)
    {
    }

    bool operator== (const StrictPoint& other) const
    {
        return x == other.x && y == other.y;
    }

    bool operator!= (const StrictPoint& other) const
    {
        return ! (*this == other);
    }
};
} // namespace

// Custom VariantConverter specializations for testing
namespace yup
{
template <>
struct VariantConverter<Point>
{
    static Point fromVar (const var& v)
    {
        if (auto* obj = v.getDynamicObject())
            return Point (static_cast<int> (obj->getProperty ("x", 0)), static_cast<int> (obj->getProperty ("y", 0)));

        return Point {};
    }

    static var toVar (const Point& p)
    {
        auto obj = std::make_unique<DynamicObject>();
        obj->setProperty ("x", p.x);
        obj->setProperty ("y", p.y);
        return obj.release();
    }
};

template <>
struct VariantConverter<Color>
{
    static Color fromVar (const var& v)
    {
        if (v.isString())
        {
            // Parse hex color string like "#RGBA" or "#RGB"
            String str = v.toString();
            if (str.startsWith ("#") && (str.length() == 7 || str.length() == 9))
            {
                String hex = str.substring (1);
                uint32 value = static_cast<uint32> (hex.getHexValue32());

                if (str.length() == 7) // #RRGGBB
                {
                    return Color (static_cast<uint8> ((value >> 16) & 0xFF),
                                  static_cast<uint8> ((value >> 8) & 0xFF),
                                  static_cast<uint8> (value & 0xFF),
                                  255);
                }
                else // #RRGGBBAA
                {
                    return Color (static_cast<uint8> ((value >> 24) & 0xFF),
                                  static_cast<uint8> ((value >> 16) & 0xFF),
                                  static_cast<uint8> ((value >> 8) & 0xFF),
                                  static_cast<uint8> (value & 0xFF));
                }
            }
        }
        else if (auto* obj = v.getDynamicObject())
        {
            return Color (static_cast<uint8> (static_cast<int> (obj->getProperty ("r", 0))),
                          static_cast<uint8> (static_cast<int> (obj->getProperty ("g", 0))),
                          static_cast<uint8> (static_cast<int> (obj->getProperty ("b", 0))),
                          static_cast<uint8> (static_cast<int> (obj->getProperty ("a", 255))));
        }
        return Color {};
    }

    static var toVar (const Color& c)
    {
        return String::formatted ("#%02X%02X%02X%02X", c.r, c.g, c.b, c.a);
    }
};

template <>
struct VariantConverter<StrictPoint>
{
    static StrictPoint fromVar (const var& v)
    {
        if (auto* obj = v.getDynamicObject())
        {
            if (obj->hasProperty ("x") && obj->hasProperty ("y"))
                return StrictPoint (static_cast<int> (obj->getProperty ("x")), static_cast<int> (obj->getProperty ("y")));
        }

        // Throw exception for invalid data to trigger catch block in CachedValue
        throw std::runtime_error ("Invalid data for StrictPoint conversion");
    }

    static var toVar (const StrictPoint& p)
    {
        auto obj = std::make_unique<DynamicObject>();
        obj->setProperty ("x", p.x);
        obj->setProperty ("y", p.y);
        return obj.release();
    }
};
} // namespace yup

class CachedValueTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        undoManager = UndoManager::Ptr (new UndoManager);
        dataTree = DataTree ("Test");

        // Set up initial property values
        {
            auto transaction = dataTree.beginTransaction (undoManager);
            transaction.setProperty (kTestPropertyName, 123);
            transaction.setProperty (kAnotherPropertyName, "hello");
            transaction.commit();
        }
    }

    void TearDown() override
    {
        dataTree = DataTree();
        undoManager.reset();
    }

    UndoManager::Ptr undoManager;
    DataTree dataTree;
};

TEST_F (CachedValueTests, DefaultConstructorCreatesUnboundValue)
{
    CachedValue<int> cachedValue;

    EXPECT_FALSE (cachedValue.isBound());
    EXPECT_EQ (0, cachedValue.get()); // Default constructed T{}
}

TEST_F (CachedValueTests, ConstructorWithTreeAndPropertyBindsCorrectly)
{
    CachedValue<int> cachedValue (dataTree, kTestPropertyName);

    EXPECT_TRUE (cachedValue.isBound());
    EXPECT_FALSE (cachedValue.isUsingDefault());
    EXPECT_EQ (123, cachedValue.get());
    EXPECT_EQ (kTestPropertyName, cachedValue.getPropertyName());
}

TEST_F (CachedValueTests, ConstructorWithDefaultValueSetsDefault)
{
    CachedValue<int> cachedValue (dataTree, "nonExistentProperty", kDefaultIntValue);

    EXPECT_TRUE (cachedValue.isBound());
    EXPECT_TRUE (cachedValue.isUsingDefault());
    EXPECT_EQ (kDefaultIntValue, cachedValue.get());
    EXPECT_EQ (kDefaultIntValue, cachedValue.getDefault());
}

TEST_F (CachedValueTests, SetDefaultChangesDefaultValue)
{
    CachedValue<int> cachedValue (dataTree, "nonExistentProperty");
    EXPECT_EQ (0, cachedValue.get()); // Default T{}

    cachedValue.setDefault (kDefaultIntValue);
    EXPECT_EQ (kDefaultIntValue, cachedValue.get());
    EXPECT_TRUE (cachedValue.isUsingDefault());
}

TEST_F (CachedValueTests, ImplicitConversionWorks)
{
    CachedValue<int> cachedValue (dataTree, kTestPropertyName);
    int value = cachedValue; // Implicit conversion
    EXPECT_EQ (123, value);
}

TEST_F (CachedValueTests, BindMethodUpdatesBinding)
{
    CachedValue<String> cachedValue;
    EXPECT_FALSE (cachedValue.isBound());

    cachedValue.bind (dataTree, kAnotherPropertyName);
    EXPECT_TRUE (cachedValue.isBound());
    EXPECT_EQ ("hello", cachedValue.get());

    cachedValue.bind (dataTree, kAnotherPropertyName, "default");
    EXPECT_EQ ("default", cachedValue.getDefault());
    EXPECT_EQ ("hello", cachedValue.get()); // Still gets actual property value
}

TEST_F (CachedValueTests, UnbindRemovesBinding)
{
    CachedValue<int> cachedValue (dataTree, kTestPropertyName);
    EXPECT_TRUE (cachedValue.isBound());

    cachedValue.unbind();
    EXPECT_FALSE (cachedValue.isBound());
    EXPECT_EQ (0, cachedValue.get()); // Returns default T{}
}

TEST_F (CachedValueTests, RefreshUpdatesCache)
{
    CachedValue<int> cachedValue (dataTree, kTestPropertyName);
    EXPECT_EQ (123, cachedValue.get());

    // Change property directly
    {
        auto transaction = dataTree.beginTransaction();
        transaction.setProperty (kTestPropertyName, 456);
        transaction.commit();
    }

    // CachedValue should automatically update via listener
    EXPECT_EQ (456, cachedValue.get());

    // Manual refresh should also work
    cachedValue.refresh();
    EXPECT_EQ (456, cachedValue.get());
}

TEST_F (CachedValueTests, CacheUpdatesOnPropertyChange)
{
    CachedValue<int> cachedValue (dataTree, kTestPropertyName);
    EXPECT_EQ (123, cachedValue.get());
    EXPECT_FALSE (cachedValue.isUsingDefault());

    // Change the property value
    {
        auto transaction = dataTree.beginTransaction();
        transaction.setProperty (kTestPropertyName, 456);
        transaction.commit();
    }

    // Cache should automatically update
    EXPECT_EQ (456, cachedValue.get());
    EXPECT_FALSE (cachedValue.isUsingDefault());

    // Change again
    {
        auto transaction = dataTree.beginTransaction();
        transaction.setProperty (kTestPropertyName, 789);
        transaction.commit();
    }

    EXPECT_EQ (789, cachedValue.get());
    EXPECT_FALSE (cachedValue.isUsingDefault());
}

TEST_F (CachedValueTests, PropertyDeletionUsesDefault)
{
    CachedValue<int> cachedValue (dataTree, kTestPropertyName, kDefaultIntValue);
    EXPECT_EQ (123, cachedValue.get()); // Property exists
    EXPECT_FALSE (cachedValue.isUsingDefault());

    // Remove the property
    {
        auto transaction = dataTree.beginTransaction();
        transaction.removeProperty (kTestPropertyName);
        transaction.commit();
    }

    // Should now use default
    EXPECT_EQ (kDefaultIntValue, cachedValue.get());
    EXPECT_TRUE (cachedValue.isUsingDefault());
}

TEST_F (CachedValueTests, PropertyChangeFromDifferentPropertyDoesNotAffectCache)
{
    CachedValue<int> cachedValue (dataTree, kTestPropertyName);
    EXPECT_EQ (123, cachedValue.get());

    // Change a different property
    {
        auto transaction = dataTree.beginTransaction();
        transaction.setProperty (kAnotherPropertyName, "changed");
        transaction.commit();
    }

    EXPECT_EQ (123, cachedValue.get()); // Should remain unchanged
}

TEST_F (CachedValueTests, TreeRedirectionUpdatesBinding)
{
    CachedValue<int> cachedValue (dataTree, kTestPropertyName);
    EXPECT_EQ (123, cachedValue.get());

    // Create new tree with different value
    DataTree newTree ("xyz");
    {
        auto transaction = newTree.beginTransaction (undoManager);
        transaction.setProperty (kTestPropertyName, 888);
        transaction.commit();
    }

    // Redirect the tree (this would happen through DataTree internal mechanisms)
    // For testing, we'll simulate by rebinding
    cachedValue.bind (newTree, kTestPropertyName);

    EXPECT_EQ (888, cachedValue.get());
}

TEST (CachedValueTypeTests, WorksWithDifferentTypes)
{
    auto undoManager = UndoManager::Ptr (new UndoManager);
    DataTree tree ("xyz");

    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("stringProp", "test string");
        transaction.setProperty ("doubleProp", 2.5);
        transaction.setProperty ("boolProp", true);
        transaction.commit();
    }

    CachedValue<String> stringValue (tree, "stringProp");
    CachedValue<double> doubleValue (tree, "doubleProp");
    CachedValue<bool> boolValue (tree, "boolProp");

    EXPECT_EQ ("test string", stringValue.get());
    EXPECT_DOUBLE_EQ (2.5, doubleValue.get());
    EXPECT_TRUE (boolValue.get());
}

TEST (CachedValueAtomicTests, WorksWithAtomicInt)
{
    auto undoManager = UndoManager::Ptr (new UndoManager);
    DataTree tree ("atomicTest");

    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("atomicIntProp", 42);
        transaction.commit();
    }

    AtomicCachedValue<int> atomicValue (tree, "atomicIntProp");

    EXPECT_TRUE (atomicValue.isBound());
    EXPECT_FALSE (atomicValue.isUsingDefault());
    EXPECT_EQ (42, atomicValue.get());
}

TEST (CachedValueAtomicTests, AtomicWithDefault)
{
    auto undoManager = UndoManager::Ptr (new UndoManager);
    DataTree tree ("atomicTest");

    AtomicCachedValue<int> atomicValue (tree, "nonExistentProp", 999);

    EXPECT_TRUE (atomicValue.isBound());
    EXPECT_TRUE (atomicValue.isUsingDefault());
    EXPECT_EQ (999, atomicValue.get());
    EXPECT_EQ (999, atomicValue.getDefault());
}

TEST (CachedValueAtomicTests, AtomicUpdatesOnPropertyChange)
{
    auto undoManager = UndoManager::Ptr (new UndoManager);
    DataTree tree ("atomicTest");

    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("atomicIntProp", 100);
        transaction.commit();
    }

    AtomicCachedValue<int> atomicValue (tree, "atomicIntProp");
    EXPECT_EQ (100, atomicValue.get());
    EXPECT_FALSE (atomicValue.isUsingDefault());

    // Change the property value
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("atomicIntProp", 200);
        transaction.commit();
    }

    // Atomic cache should automatically update
    EXPECT_EQ (200, atomicValue.get());
    EXPECT_FALSE (atomicValue.isUsingDefault());
}

TEST (CachedValueAtomicTests, AtomicSetDefault)
{
    auto undoManager = UndoManager::Ptr (new UndoManager);
    DataTree tree ("atomicTest");

    AtomicCachedValue<int> atomicValue (tree, "nonExistentProp");
    EXPECT_EQ (0, atomicValue.get()); // Default T{}

    atomicValue.setDefault (777);
    EXPECT_EQ (777, atomicValue.get());
    EXPECT_TRUE (atomicValue.isUsingDefault());
}

TEST (CachedValueAtomicTests, AtomicWithBool)
{
    auto undoManager = UndoManager::Ptr (new UndoManager);
    DataTree tree ("atomicTest");

    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("atomicBoolProp", true);
        transaction.commit();
    }

    AtomicCachedValue<bool> atomicBool (tree, "atomicBoolProp");

    EXPECT_TRUE (atomicBool.get());
    EXPECT_FALSE (atomicBool.isUsingDefault());

    // Change to false
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("atomicBoolProp", false);
        transaction.commit();
    }

    EXPECT_FALSE (atomicBool.get());
}

TEST (CachedValueAtomicTests, AtomicThreadSafeAccess)
{
    auto undoManager = UndoManager::Ptr (new UndoManager);
    DataTree tree ("atomicTest");

    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("atomicIntProp", 0);
        transaction.commit();
    }

    AtomicCachedValue<int> atomicValue (tree, "atomicIntProp");
    std::atomic<bool> stopFlag { false };
    std::atomic<int> readCount { 0 };

    // Reader thread - performs atomic reads
    std::thread readerThread ([&]
    {
        while (! stopFlag.load())
        {
            int value = atomicValue.get(); // Atomic read
            (void) value;
            readCount.fetch_add (1);
            std::this_thread::yield();
        }
    });

    // Writer thread - modifies the DataTree property
    std::thread writerThread ([&]
    {
        for (int i = 1; i <= 10 && ! stopFlag.load(); ++i)
        {
            auto transaction = tree.beginTransaction (undoManager);
            transaction.setProperty ("atomicIntProp", i * 10);
            transaction.commit();
            std::this_thread::sleep_for (std::chrono::microseconds (100));
        }
    });

    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    stopFlag.store (true);

    readerThread.join();
    writerThread.join();

    EXPECT_GT (readCount.load(), 0);
    EXPECT_EQ (100, atomicValue.get()); // Should be the final value
}

TEST_F (CachedValueTests, SetMethodUpdatesDataTree)
{
    CachedValue<int> cachedValue (dataTree, kTestPropertyName);
    EXPECT_EQ (123, cachedValue.get());

    // Use set method to update value
    cachedValue.set (456);

    // Verify the DataTree was updated
    EXPECT_EQ (var (456), dataTree.getProperty (kTestPropertyName));
    EXPECT_EQ (456, cachedValue.get());
    EXPECT_FALSE (cachedValue.isUsingDefault());
}

TEST_F (CachedValueTests, SetMethodWithStringType)
{
    CachedValue<String> cachedValue (dataTree, kAnotherPropertyName);
    EXPECT_EQ ("hello", cachedValue.get());

    // Use set method to update string value
    cachedValue.set ("world");

    // Verify the DataTree was updated
    EXPECT_EQ (var ("world"), dataTree.getProperty (kAnotherPropertyName));
    EXPECT_EQ ("world", cachedValue.get());
}

TEST_F (CachedValueTests, SetMethodOnUnboundCachedValueDoesNothing)
{
    CachedValue<int> cachedValue;
    EXPECT_FALSE (cachedValue.isBound());

    // Set should do nothing when unbound
    cachedValue.set (999);
    EXPECT_EQ (0, cachedValue.get()); // Still default T{}
}

TEST (CachedValueAtomicTests, AtomicSetMethodUpdatesDataTree)
{
    auto undoManager = UndoManager::Ptr (new UndoManager);
    DataTree tree ("atomicSetTest");

    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("atomicProp", 111);
        transaction.commit();
    }

    AtomicCachedValue<int> atomicValue (tree, "atomicProp");
    EXPECT_EQ (111, atomicValue.get());

    // Use set method to update value
    atomicValue.set (222);

    // Verify the DataTree was updated
    EXPECT_EQ (var (222), tree.getProperty ("atomicProp"));
    EXPECT_EQ (222, atomicValue.get());
    EXPECT_FALSE (atomicValue.isUsingDefault());
}

TEST (CachedValueAtomicTests, AtomicSetMethodOnUnboundDoesNothing)
{
    AtomicCachedValue<int> atomicValue;
    EXPECT_FALSE (atomicValue.isBound());

    // Set should do nothing when unbound
    atomicValue.set (999);
    EXPECT_EQ (0, atomicValue.get()); // Still default T{}
}

//==============================================================================
// VariantConverter Tests

TEST (CachedValueVariantConverterTests, PointTypeWithCustomConverter)
{
    auto undoManager = UndoManager::Ptr (new UndoManager);
    DataTree tree ("pointTest");

    // Create CachedValue with Point type
    CachedValue<Point> pointValue (tree, "pointProp", Point (10, 20));

    // Initially should use default since property doesn't exist
    EXPECT_TRUE (pointValue.isUsingDefault());
    EXPECT_EQ (Point (10, 20), pointValue.get());

    // Set a new point value using the set method
    Point newPoint (100, 200);
    pointValue.set (newPoint);

    // Verify the DataTree was updated and cached value reflects the change
    EXPECT_FALSE (pointValue.isUsingDefault());
    EXPECT_EQ (newPoint, pointValue.get());

    // Verify the underlying var structure (should be DynamicObject with x,y properties)
    var storedValue = tree.getProperty ("pointProp");
    EXPECT_TRUE (storedValue.getDynamicObject() != nullptr);

    if (auto* obj = storedValue.getDynamicObject())
    {
        EXPECT_EQ (var (100), obj->getProperty ("x"));
        EXPECT_EQ (var (200), obj->getProperty ("y"));
    }
}

TEST (CachedValueVariantConverterTests, ColorTypeWithStringConverter)
{
    auto undoManager = UndoManager::Ptr (new UndoManager);
    DataTree tree ("colorTest");

    // Set up initial color value directly in DataTree as hex string
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("colorProp", "#FF0080FF"); // Red=255, Green=0, Blue=128, Alpha=255
    }

    // Create CachedValue that should parse the hex string
    CachedValue<Color> colorValue (tree, "colorProp");

    EXPECT_FALSE (colorValue.isUsingDefault());
    Color expectedColor (255, 0, 128, 255);
    EXPECT_EQ (expectedColor, colorValue.get());

    // Set a new color using the set method
    Color blueColor (0, 0, 255, 128);
    colorValue.set (blueColor);

    // Verify the DataTree now contains the hex representation
    var storedValue = tree.getProperty ("colorProp");
    EXPECT_TRUE (storedValue.isString());
    EXPECT_EQ ("#0000FF80", storedValue.toString()); // Blue with alpha 128

    // Verify the cached value
    EXPECT_EQ (blueColor, colorValue.get());
}

TEST (CachedValueVariantConverterTests, ColorTypeWithDefaultValue)
{
    auto undoManager = UndoManager::Ptr (new UndoManager);
    DataTree tree ("colorDefaultTest");

    Color defaultColor (255, 255, 255, 255); // White
    CachedValue<Color> colorValue (tree, "nonExistentColor", defaultColor);

    // Should use default since property doesn't exist
    EXPECT_TRUE (colorValue.isUsingDefault());
    EXPECT_EQ (defaultColor, colorValue.get());

    // Set the default to a different color
    Color newDefault (128, 128, 128, 255); // Gray
    colorValue.setDefault (newDefault);
    EXPECT_EQ (newDefault, colorValue.get());
    EXPECT_TRUE (colorValue.isUsingDefault());

    // Now set an actual value
    Color greenColor (0, 255, 0, 255);
    colorValue.set (greenColor);
    EXPECT_FALSE (colorValue.isUsingDefault());
    EXPECT_EQ (greenColor, colorValue.get());
}

TEST (CachedValueVariantConverterTests, PointTypePropertyChangeUpdatesCache)
{
    auto undoManager = UndoManager::Ptr (new UndoManager);
    DataTree tree ("pointChangeTest");

    CachedValue<Point> pointValue (tree, "pointProp", Point (0, 0));

    // Set initial value
    Point initialPoint (50, 75);
    pointValue.set (initialPoint);
    EXPECT_EQ (initialPoint, pointValue.get());

    // Change the property directly through DataTree transaction
    {
        auto transaction = tree.beginTransaction (undoManager);
        auto obj = std::make_unique<DynamicObject>();
        obj->setProperty ("x", 300);
        obj->setProperty ("y", 400);
        transaction.setProperty ("pointProp", obj.release());
    }

    // CachedValue should automatically update via listener
    Point expectedPoint (300, 400);
    EXPECT_EQ (expectedPoint, pointValue.get());
    EXPECT_FALSE (pointValue.isUsingDefault());
}

TEST (CachedValueAtomicVariantConverterTests, AtomicPointType)
{
    auto undoManager = UndoManager::Ptr (new UndoManager);
    DataTree tree ("atomicPointTest");

    // Create AtomicCachedValue with Point type
    Point defaultPoint (5, 10);
    AtomicCachedValue<Point> atomicPoint (tree, "atomicPointProp", defaultPoint);

    // Initially should use default
    EXPECT_TRUE (atomicPoint.isUsingDefault());
    EXPECT_EQ (defaultPoint, atomicPoint.get());

    // Set a value atomically
    Point newPoint (123, 456);
    atomicPoint.set (newPoint);

    // Verify atomic read
    EXPECT_EQ (newPoint, atomicPoint.get());
    EXPECT_FALSE (atomicPoint.isUsingDefault());

    // Verify DataTree was updated correctly
    var storedValue = tree.getProperty ("atomicPointProp");
    if (auto* obj = storedValue.getDynamicObject())
    {
        EXPECT_EQ (var (123), obj->getProperty ("x"));
        EXPECT_EQ (var (456), obj->getProperty ("y"));
    }
}

TEST (CachedValueAtomicVariantConverterTests, AtomicColorTypeThreadSafety)
{
    auto undoManager = UndoManager::Ptr (new UndoManager);
    DataTree tree ("atomicColorThreadTest");

    // Initialize with a color
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("atomicColorProp", "#FF000000"); // Red with no alpha
    }

    AtomicCachedValue<Color> atomicColor (tree, "atomicColorProp");
    EXPECT_EQ (Color (255, 0, 0, 0), atomicColor.get());

    std::atomic<bool> stopFlag { false };
    std::atomic<int> readCount { 0 };
    Color finalColor (0, 255, 255, 255); // Cyan

    // Reader thread - performs atomic reads
    std::thread readerThread ([&]
    {
        while (! stopFlag.load())
        {
            Color color = atomicColor.get(); // Atomic read
            (void) color;                    // Use the value to prevent optimization
            readCount.fetch_add (1);
            std::this_thread::yield();
        }
    });

    // Writer thread - modifies the color through set method
    std::thread writerThread ([&]
    {
        for (int i = 1; i <= 5 && ! stopFlag.load(); ++i)
        {
            Color stepColor (i * 50, 255 - i * 40, i * 30, 255);
            atomicColor.set (stepColor);
            std::this_thread::sleep_for (std::chrono::microseconds (200));
        }
        atomicColor.set (finalColor);
    });

    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    stopFlag.store (true);

    readerThread.join();
    writerThread.join();

    EXPECT_GT (readCount.load(), 0);
    EXPECT_EQ (finalColor, atomicColor.get());
}

TEST (CachedValueVariantConverterTests, ConversionFailureHandling)
{
    auto undoManager = UndoManager::Ptr (new UndoManager);
    DataTree tree ("conversionFailTest");

    // Set up invalid data that cannot be converted to Point
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("badPoint", "not a point object");
    }

    Point defaultPoint (999, 888);
    CachedValue<Point> pointValue (tree, "badPoint", defaultPoint);

    // The current VariantConverter for Point will convert any non-DynamicObject to Point(0,0)
    // This is expected behavior - the converter successfully converts the string to Point(0,0)
    EXPECT_FALSE (pointValue.isUsingDefault());
    EXPECT_EQ (Point (0, 0), pointValue.get());

    // Test with a property that doesn't exist - this should use default
    CachedValue<Point> pointValueNoProperty (tree, "nonExistentProperty", defaultPoint);
    EXPECT_TRUE (pointValueNoProperty.isUsingDefault());
    EXPECT_EQ (defaultPoint, pointValueNoProperty.get());
}

TEST (CachedValueVariantConverterTests, StrictConversionFailureHandling)
{
    auto undoManager = UndoManager::Ptr (new UndoManager);
    DataTree tree ("strictConversionFailTest");

    // Set up invalid data that will cause StrictPoint converter to throw
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("strictPoint", "not a point object");
    }

    StrictPoint defaultStrictPoint (999, 888);
    CachedValue<StrictPoint> strictPointValue (tree, "strictPoint", defaultStrictPoint);

    // Since StrictPoint converter throws on invalid data, should fall back to default
    EXPECT_TRUE (strictPointValue.isUsingDefault());
    EXPECT_EQ (defaultStrictPoint, strictPointValue.get());

    // Test with valid data - should work correctly
    {
        auto transaction = tree.beginTransaction (undoManager);
        auto obj = std::make_unique<DynamicObject>();
        obj->setProperty ("x", 100);
        obj->setProperty ("y", 200);
        transaction.setProperty ("strictPoint", obj.release());
    }

    // Should now parse successfully and not use default
    EXPECT_FALSE (strictPointValue.isUsingDefault());
    EXPECT_EQ (StrictPoint (100, 200), strictPointValue.get());
}
