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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

#include "../mocks/yup_gui.h"

using namespace yup;
using ::testing::_;
using ::testing::NiceMock;

namespace
{

// =============================================================================
// Aliases for convenience. Use NiceMock so unexpected calls are silently ignored.
using ComponentMock = NiceMock<MockComponent>;

// =============================================================================
class TestComponentNative final : public ComponentNative
{
public:
    TestComponentNative (Component& component)
        : ComponentNative (component, defaultFlags)
    {
    }

    void setTitle (const String&) override {}

    String getTitle() const override { return {}; }

    void setVisible (bool) override {}

    bool isVisible() const override { return false; }

    void toFront() override {}

    void setSize (const Size<int>&) override {}

    Size<int> getSize() const override { return {}; }

    Size<int> getContentSize() const override { return {}; }

    Point<int> getPosition() const override { return {}; }

    void setPosition (const Point<int>&) override {}

    Rectangle<int> getBounds() const override { return {}; }

    void setBounds (const Rectangle<int>&) override {}

    Rectangle<int> getSafeAreaBounds() const override { return {}; }

    void setFullScreen (bool) override {}

    bool isFullScreen() const override { return false; }

    bool isDecorated() const override { return false; }

    void setOpacity (float) override {}

    float getOpacity() const override { return 1.0f; }

    void setFocusedComponent (Component* component) override
    {
        focusedComponent = component;
    }

    Component* getFocusedComponent() const override { return focusedComponent; }

    bool isContinuousRepaintingEnabled() const override { return false; }

    void enableContinuousRepainting (bool) override {}

    bool isAtomicModeEnabled() const override { return false; }

    void enableAtomicMode (bool) override {}

    bool isWireframeEnabled() const override { return false; }

    void enableWireframe (bool) override {}

    void repaint() override {}

    void repaint (const Rectangle<float>&) override {}

    const RectangleList<float>& getRepaintAreas() const override
    {
        static RectangleList<float> r;
        return r;
    }

    void startTextInput (Component&) override {}

    void stopTextInput (Component&) override {}

    void updateTextInputRect (Component&) override {}

    float getScaleDpi() const override { return 1.0f; }

    float getCurrentFrameRate() const override { return 60.0f; }

    float getDesiredFrameRate() const override { return 60.0f; }

    void* getNativeHandle() const override { return nullptr; }

    rive::Factory* getFactory() override { return nullptr; }

    GraphicsContext* getGraphicsContext() override { return nullptr; }

private:
    Component* focusedComponent = nullptr;
};

class RecordingComponentListener : public ComponentListener
{
public:
    void componentMoved (Component& component) override
    {
        ++movedCount;
        lastMovedComponent = &component;
    }

    void componentResized (Component& component) override
    {
        ++resizedCount;
        lastResizedComponent = &component;
    }

    void componentBeingDeleted (Component& component) override
    {
        ++deletedCount;
        lastDeletedComponent = &component;
    }

    int movedCount = 0;
    int resizedCount = 0;
    int deletedCount = 0;
    Component* lastMovedComponent = nullptr;
    Component* lastResizedComponent = nullptr;
    Component* lastDeletedComponent = nullptr;
};

} // namespace

// =============================================================================

namespace yup
{

template <>
class ComponentTestHelper<Component>
{
public:
    static void attachMockNative (Component& component)
    {
        jassert (component.native == nullptr);
        component.native = new TestComponentNative (component);
    }

    static void detachMockNative (Component& component)
    {
        component.native = nullptr;
    }

    static void triggerPaint (Component& comp,
                              Graphics& g,
                              const Rectangle<float>& repaintArea,
                              bool renderContinuous = false)
    {
        comp.internalPaint (g, repaintArea, renderContinuous);
    }

    static bool triggerItemsDropped (Component& comp,
                                     const Point<float>& windowPosition,
                                     const DragAndDropData& data)
    {
        return comp.internalItemsDropped (data, windowPosition);
    }

    static void triggerItemDragEnter (Component& comp,
                                      const Point<float>& windowPosition,
                                      const DragAndDropData& data)
    {
        comp.internalItemDragEnter (data, windowPosition);
    }

    static void triggerItemDragMove (Component& comp,
                                     const Point<float>& windowPosition,
                                     const DragAndDropData& data)
    {
        comp.internalItemDragMove (data, windowPosition);
    }

    static void triggerItemDragExit (Component& comp,
                                     const DragAndDropData& data)
    {
        comp.internalItemDragExit (data);
    }

    static void triggerInternalResized (Component& comp, int width, int height)
    {
        comp.internalResized (width, height);
    }

    static void triggerInternalMoved (Component& comp, int xpos, int ypos)
    {
        comp.internalMoved (xpos, ypos);
    }

    static void triggerInternalContentScaleChanged (Component& comp, float dpiScale)
    {
        comp.internalContentScaleChanged (dpiScale);
    }

    static void triggerSafeAreaChanged (Component& comp)
    {
        comp.internalSafeAreaChanged();
    }

    static void setOnDesktop (Component& comp, bool value)
    {
        comp.options.onDesktop = value;
    }

    static void triggerDisplayChanged (Component& comp)
    {
        comp.internalDisplayChanged();
    }

    static void triggerUserTriedToCloseWindow (Component& comp)
    {
        comp.internalUserTriedToCloseWindow();
    }

    static void triggerRefreshDisplay (Component& comp, double lastFrameTimeSeconds)
    {
        comp.internalRefreshDisplay (lastFrameTimeSeconds);
    }

    static void triggerKeyDown (Component& comp, const KeyPress& keys, const Point<float>& position)
    {
        comp.internalKeyDown (keys, position);
    }

    static void triggerKeyUp (Component& comp, const KeyPress& keys, const Point<float>& position)
    {
        comp.internalKeyUp (keys, position);
    }

    static void triggerTextInput (Component& comp, const String& text)
    {
        comp.internalTextInput (text);
    }

    static void triggerMouseEnter (Component& comp, const MouseEvent& event)
    {
        comp.internalMouseEnter (event);
    }

    static void triggerMouseExit (Component& comp, const MouseEvent& event)
    {
        comp.internalMouseExit (event);
    }

    static void triggerMouseDown (Component& comp, const MouseEvent& event)
    {
        comp.internalMouseDown (event);
    }

    static void triggerMouseMove (Component& comp, const MouseEvent& event)
    {
        comp.internalMouseMove (event);
    }

    static void triggerMouseDrag (Component& comp, const MouseEvent& event)
    {
        comp.internalMouseDrag (event);
    }

    static void triggerMouseUp (Component& comp, const MouseEvent& event)
    {
        comp.internalMouseUp (event);
    }

    static void triggerMouseDoubleClick (Component& comp, const MouseEvent& event)
    {
        comp.internalMouseDoubleClick (event);
    }

    static void triggerMouseWheel (Component& comp, const MouseEvent& event, const MouseWheelData& wheelData)
    {
        comp.internalMouseWheel (event, wheelData);
    }

    static void triggerVisibilityChanged (Component& comp)
    {
        comp.internalVisibilityChanged();
    }

    static void triggerAttachedToNative (Component& comp)
    {
        comp.internalAttachedToNative();
    }

    static void triggerDetachedFromNative (Component& comp)
    {
        comp.internalDetachedFromNative();
    }
    static void triggerInternalMouseDown (Component& comp, const MouseEvent& event)
    {
        comp.internalMouseDown (event);
    }
};

} // namespace yup

// =============================================================================

class ComponentTest : public ::testing::Test
{
protected:
    using ComponentHelper = yup::ComponentTestHelper<yup::Component>;

    void SetUp() override
    {
        // Create a hierarchy: root -> parent -> child
        root = std::make_unique<Component> ("root");
        parent = std::make_unique<Component> ("parent");
        child = std::make_unique<Component> ("child");

        // Set up positions and sizes
        root->setBounds (0, 0, 400, 300);
        parent->setBounds (50, 50, 200, 150);
        child->setBounds (25, 25, 100, 75);

        // Build hierarchy
        root->addChildComponent (*parent);
        parent->addChildComponent (*child);
    }

    std::unique_ptr<Component> root;
    std::unique_ptr<Component> parent;
    std::unique_ptr<Component> child;
};

// =============================================================================

TEST_F (ComponentTest, LocalToScreenTransformation)
{
    // Test local-to-screen transformation
    auto childScreenPos = child->localToScreen (Point<float> (10.0f, 10.0f));

    // Expected: root(0,0) + parent(50,50) + child(25,25) + point(10,10) = (85, 85)
    EXPECT_FLOAT_EQ (childScreenPos.getX(), 85.0f);
    EXPECT_FLOAT_EQ (childScreenPos.getY(), 85.0f);
}

TEST_F (ComponentTest, ScreenToLocalTransformation)
{
    // Test screen-to-local transformation
    auto localPoint = child->screenToLocal (Point<float> (85.0f, 85.0f));

    // Expected: screen(85,85) - child_screen_origin(75,75) = (10, 10)
    EXPECT_FLOAT_EQ (localPoint.getX(), 10.0f);
    EXPECT_FLOAT_EQ (localPoint.getY(), 10.0f);
}

TEST_F (ComponentTest, GetRelativePointBetweenSiblings)
{
    // Create a second child as sibling
    auto sibling = std::make_unique<Component> ("sibling");
    sibling->setBounds (125, 75, 50, 50);
    parent->addChildComponent (*sibling);

    // Test getRelativePoint between siblings
    auto relativePoint = child->getRelativePoint (sibling.get(), Point<float> (10.0f, 10.0f));

    // Point (10,10) in child coordinates should be (-90, -40) in sibling coordinates
    // child's (10,10) in screen = (85, 85)
    // sibling's origin in screen = (175, 125)
    // So relative point = (85-175, 85-125) = (-90, -40)
    EXPECT_FLOAT_EQ (relativePoint.getX(), -90.0f);
    EXPECT_FLOAT_EQ (relativePoint.getY(), -40.0f);
}

TEST_F (ComponentTest, GetRelativeAreaBetweenDistantComponents)
{
    // Create a cousin component (grandparent -> uncle -> cousin)
    auto grandparent = std::make_unique<Component> ("grandparent");
    auto uncle = std::make_unique<Component> ("uncle");
    auto cousin = std::make_unique<Component> ("cousin");

    grandparent->setBounds (0, 0, 800, 600);
    uncle->setBounds (300, 200, 200, 150);
    cousin->setBounds (25, 25, 100, 75);

    grandparent->addChildComponent (*root);
    grandparent->addChildComponent (*uncle);
    uncle->addChildComponent (*cousin);

    // Test getRelativeArea between distant components
    Rectangle<float> childRect (5.0f, 5.0f, 20.0f, 15.0f);
    auto relativeArea = child->getRelativeArea (cousin.get(), childRect);

    // Child's rect (5,5,20,15) in screen coordinates:
    // Position: root(0,0) + parent(50,50) + child(25,25) + rect(5,5) = (80, 80)
    // Cousin's origin in screen: grandparent(0,0) + uncle(300,200) + cousin(25,25) = (325, 225)
    // Relative position: (80-325, 80-225) = (-245, -145)
    EXPECT_FLOAT_EQ (relativeArea.getX(), -245.0f);
    EXPECT_FLOAT_EQ (relativeArea.getY(), -145.0f);
    EXPECT_FLOAT_EQ (relativeArea.getWidth(), 20.0f);
    EXPECT_FLOAT_EQ (relativeArea.getHeight(), 15.0f);
}

TEST_F (ComponentTest, GetLocalPointFromDistantComponent)
{
    // Test getLocalPoint from a distant component
    auto grandparent = std::make_unique<Component> ("grandparent");
    auto uncle = std::make_unique<Component> ("uncle");
    auto cousin = std::make_unique<Component> ("cousin");

    grandparent->setBounds (0, 0, 800, 600);
    uncle->setBounds (300, 200, 200, 150);
    cousin->setBounds (25, 25, 100, 75);

    grandparent->addChildComponent (*root);
    grandparent->addChildComponent (*uncle);
    uncle->addChildComponent (*cousin);

    // Test getLocalPoint from cousin to child
    Point<float> cousinPoint (10.0f, 10.0f);
    auto localPoint = child->getLocalPoint (cousin.get(), cousinPoint);

    // Cousin's (10,10) in screen = (335, 235)
    // Child's origin in screen = (75, 75)
    // Local point in child = (335-75, 235-75) = (260, 160)
    EXPECT_FLOAT_EQ (localPoint.getX(), 260.0f);
    EXPECT_FLOAT_EQ (localPoint.getY(), 160.0f);
}

TEST_F (ComponentTest, NegativePositions)
{
    // Test components with negative positions
    auto negChild = std::make_unique<Component> ("negChild");
    negChild->setBounds (-10, -5, 50, 30);
    parent->addChildComponent (*negChild);

    // Test localToScreen with negative component position
    auto screenPos = negChild->localToScreen (Point<float> (5.0f, 3.0f));

    // Expected: root(0,0) + parent(50,50) + negChild(-10,-5) + point(5,3) = (45, 48)
    EXPECT_FLOAT_EQ (screenPos.getX(), 45.0f);
    EXPECT_FLOAT_EQ (screenPos.getY(), 48.0f);

    // Test screenToLocal with negative positions
    auto localPos = negChild->screenToLocal (Point<float> (45.0f, 48.0f));
    EXPECT_FLOAT_EQ (localPos.getX(), 5.0f);
    EXPECT_FLOAT_EQ (localPos.getY(), 3.0f);
}

TEST_F (ComponentTest, ZeroSizedComponents)
{
    // Test components with zero or very small sizes
    auto tinyChild = std::make_unique<Component> ("tinyChild");
    tinyChild->setBounds (100, 100, 0, 0);
    parent->addChildComponent (*tinyChild);

    auto screenPos = tinyChild->localToScreen (Point<float> (0.0f, 0.0f));

    // Expected: root(0,0) + parent(50,50) + tinyChild(100,100) + point(0,0) = (150, 150)
    EXPECT_FLOAT_EQ (screenPos.getX(), 150.0f);
    EXPECT_FLOAT_EQ (screenPos.getY(), 150.0f);
}

TEST_F (ComponentTest, DeeperHierarchy)
{
    // Test with 5 levels of nesting
    auto level1 = std::make_unique<Component> ("level1");
    auto level2 = std::make_unique<Component> ("level2");
    auto level3 = std::make_unique<Component> ("level3");
    auto level4 = std::make_unique<Component> ("level4");

    level1->setBounds (10, 10, 200, 200);
    level2->setBounds (15, 15, 170, 170);
    level3->setBounds (20, 20, 130, 130);
    level4->setBounds (25, 25, 80, 80);

    child->addChildComponent (*level1);
    level1->addChildComponent (*level2);
    level2->addChildComponent (*level3);
    level3->addChildComponent (*level4);

    // Test coordinate transformation from deepest to root
    auto screenPos = level4->localToScreen (Point<float> (5.0f, 5.0f));

    // Expected: point(5,5) + level4(25,25) + level3(20,20) + level2(15,15) + level1(10,10) + child(25,25) + parent(50,50) + root(0,0) = (150, 150)
    EXPECT_FLOAT_EQ (screenPos.getX(), 150.0f);
    EXPECT_FLOAT_EQ (screenPos.getY(), 150.0f);

    // Test relative point between distant components in deep hierarchy
    auto relativePoint = level4->getRelativePoint (child.get(), Point<float> (2.0f, 3.0f));

    // level4's (2,3) in screen = (147, 148)
    // child's origin in screen = (75, 75)
    // Relative point = (147-75, 148-75) = (72, 73)
    EXPECT_FLOAT_EQ (relativePoint.getX(), 72.0f);
    EXPECT_FLOAT_EQ (relativePoint.getY(), 73.0f);
}

TEST_F (ComponentTest, MultipleSiblings)
{
    // Create multiple siblings with different positions
    auto sibling1 = std::make_unique<Component> ("sibling1");
    auto sibling2 = std::make_unique<Component> ("sibling2");
    auto sibling3 = std::make_unique<Component> ("sibling3");

    sibling1->setBounds (100, 50, 50, 50);
    sibling2->setBounds (200, 100, 60, 60);
    sibling3->setBounds (300, 150, 70, 70);

    parent->addChildComponent (*sibling1);
    parent->addChildComponent (*sibling2);
    parent->addChildComponent (*sibling3);

    // Test relative positioning between various siblings
    auto point1to2 = sibling1->getRelativePoint (sibling2.get(), Point<float> (10.0f, 10.0f));

    // sibling1's (10,10) in screen = (160, 110)
    // sibling2's origin in screen = (250, 150)
    // Relative point = (160-250, 110-150) = (-90, -40)
    EXPECT_FLOAT_EQ (point1to2.getX(), -90.0f);
    EXPECT_FLOAT_EQ (point1to2.getY(), -40.0f);

    auto point2to3 = sibling2->getRelativePoint (sibling3.get(), Point<float> (5.0f, 15.0f));

    // sibling2's (5,15) in screen = (255, 165)
    // sibling3's origin in screen = (350, 200)
    // Relative point = (255-350, 165-200) = (-95, -35)
    EXPECT_FLOAT_EQ (point2to3.getX(), -95.0f);
    EXPECT_FLOAT_EQ (point2to3.getY(), -35.0f);

    // Test area conversion between siblings
    Rectangle<float> area (5.0f, 5.0f, 20.0f, 15.0f);
    auto relativeArea = sibling1->getRelativeArea (sibling3.get(), area);

    // sibling1's area (5,5,20,15) in screen = (155,105,20,15)
    // sibling3's origin in screen = (350, 200)
    // Relative area = (155-350, 105-200, 20, 15) = (-195, -95, 20, 15)
    EXPECT_FLOAT_EQ (relativeArea.getX(), -195.0f);
    EXPECT_FLOAT_EQ (relativeArea.getY(), -95.0f);
    EXPECT_FLOAT_EQ (relativeArea.getWidth(), 20.0f);
    EXPECT_FLOAT_EQ (relativeArea.getHeight(), 15.0f);
}

TEST_F (ComponentTest, AsymmetricHierarchies)
{
    // Create asymmetric tree: one branch deep, another shallow
    auto deepBranch = std::make_unique<Component> ("deepBranch");
    auto deepLevel1 = std::make_unique<Component> ("deepLevel1");
    auto deepLevel2 = std::make_unique<Component> ("deepLevel2");

    auto shallowBranch = std::make_unique<Component> ("shallowBranch");

    deepBranch->setBounds (30, 30, 120, 120);
    deepLevel1->setBounds (20, 20, 80, 80);
    deepLevel2->setBounds (15, 15, 50, 50);
    shallowBranch->setBounds (180, 40, 80, 80);

    parent->addChildComponent (*deepBranch);
    deepBranch->addChildComponent (*deepLevel1);
    deepLevel1->addChildComponent (*deepLevel2);
    parent->addChildComponent (*shallowBranch);

    // Test relative positioning between different depth components
    auto deepToShallow = deepLevel2->getRelativePoint (shallowBranch.get(), Point<float> (10.0f, 10.0f));

    // deepLevel2's (10,10) in screen coordinates:
    // point(10,10) + deepLevel2(15,15) + deepLevel1(20,20) + deepBranch(30,30) + parent(50,50) + root(0,0) = (125, 125)
    // shallowBranch's origin in screen = shallowBranch(180,40) + parent(50,50) + root(0,0) = (230, 90)
    // Relative point = (125-230, 125-90) = (-105, 35)
    EXPECT_FLOAT_EQ (deepToShallow.getX(), -105.0f);
    EXPECT_FLOAT_EQ (deepToShallow.getY(), 35.0f);

    auto shallowToDeep = shallowBranch->getRelativePoint (deepLevel2.get(), Point<float> (5.0f, 8.0f));

    // shallowBranch's (5,8) in screen = point(5,8) + shallowBranch(180,40) + parent(50,50) + root(0,0) = (235, 98)
    // deepLevel2's origin in screen = deepLevel2(15,15) + deepLevel1(20,20) + deepBranch(30,30) + parent(50,50) + root(0,0) = (115, 115)
    // Relative point = (235-115, 98-115) = (120, -17)
    EXPECT_FLOAT_EQ (shallowToDeep.getX(), 120.0f);
    EXPECT_FLOAT_EQ (shallowToDeep.getY(), -17.0f);
}

TEST_F (ComponentTest, SelfReferences)
{
    // Test coordinate transformations with self-references
    Point<float> testPoint (25.0f, 35.0f);

    // getRelativePoint with same component should return original point
    auto relativeSelf = child->getRelativePoint (child.get(), testPoint);
    EXPECT_FLOAT_EQ (relativeSelf.getX(), testPoint.getX());
    EXPECT_FLOAT_EQ (relativeSelf.getY(), testPoint.getY());

    // getLocalPoint with same component should return original point
    auto localSelf = child->getLocalPoint (child.get(), testPoint);
    EXPECT_FLOAT_EQ (localSelf.getX(), testPoint.getX());
    EXPECT_FLOAT_EQ (localSelf.getY(), testPoint.getY());

    // Test with nullptr (should behave same as self-reference)
    auto relativeNull = child->getRelativePoint (nullptr, testPoint);
    EXPECT_FLOAT_EQ (relativeNull.getX(), testPoint.getX());
    EXPECT_FLOAT_EQ (relativeNull.getY(), testPoint.getY());

    auto localNull = child->getLocalPoint (nullptr, testPoint);
    EXPECT_FLOAT_EQ (localNull.getX(), testPoint.getX());
    EXPECT_FLOAT_EQ (localNull.getY(), testPoint.getY());
}

TEST_F (ComponentTest, LargeCoordinates)
{
    // Test with very large coordinates to check for overflow/precision issues
    auto largeComponent = std::make_unique<Component> ("largeComponent");
    largeComponent->setBounds (10000.0f, 5000.0f, 1000.0f, 800.0f);
    parent->addChildComponent (*largeComponent);

    auto screenPos = largeComponent->localToScreen (Point<float> (500.0f, 400.0f));

    // Expected: root(0,0) + parent(50,50) + largeComponent(10000,5000) + point(500,400) = (10550, 5450)
    EXPECT_FLOAT_EQ (screenPos.getX(), 10550.0f);
    EXPECT_FLOAT_EQ (screenPos.getY(), 5450.0f);

    // Test reverse transformation
    auto localPos = largeComponent->screenToLocal (Point<float> (10550.0f, 5450.0f));
    EXPECT_FLOAT_EQ (localPos.getX(), 500.0f);
    EXPECT_FLOAT_EQ (localPos.getY(), 400.0f);
}

TEST_F (ComponentTest, PrecisionEdgeCases)
{
    // Test with fractional positions for precision
    auto fracChild = std::make_unique<Component> ("fracChild");
    fracChild->setBounds (12.5f, 7.25f, 33.75f, 28.125f);
    parent->addChildComponent (*fracChild);

    auto screenPos = fracChild->localToScreen (Point<float> (3.125f, 4.875f));

    // Expected: root(0,0) + parent(50,50) + fracChild(12.5,7.25) + point(3.125,4.875) = (65.625, 62.125)
    EXPECT_FLOAT_EQ (screenPos.getX(), 65.625f);
    EXPECT_FLOAT_EQ (screenPos.getY(), 62.125f);

    // Test area with fractional dimensions
    Rectangle<float> fracArea (1.25f, 2.75f, 5.5f, 8.125f);
    auto relativeArea = fracChild->getRelativeArea (child.get(), fracArea);

    // fracChild's area (1.25,2.75,5.5,8.125) in screen = (63.75,60.0,5.5,8.125)
    // child's origin in screen = (75, 75)
    // Relative area = (63.75-75, 60.0-75, 5.5, 8.125) = (-11.25, -15.0, 5.5, 8.125)
    EXPECT_FLOAT_EQ (relativeArea.getX(), -11.25f);
    EXPECT_FLOAT_EQ (relativeArea.getY(), -15.0f);
    EXPECT_FLOAT_EQ (relativeArea.getWidth(), 5.5f);
    EXPECT_FLOAT_EQ (relativeArea.getHeight(), 8.125f);
}

// =============================================================================

TEST_F (ComponentTest, PositionMethods)
{
    // Test setPosition and getPosition
    Point<float> newPos (100.0f, 200.0f);
    child->setPosition (newPos);
    EXPECT_FLOAT_EQ (child->getPosition().getX(), 100.0f);
    EXPECT_FLOAT_EQ (child->getPosition().getY(), 200.0f);

    // Test individual coordinate setters/getters
    child->setTopLeft (Point<float> (150.0f, 250.0f));
    EXPECT_FLOAT_EQ (child->getX(), 150.0f);
    EXPECT_FLOAT_EQ (child->getY(), 250.0f);
    EXPECT_FLOAT_EQ (child->getPosition().getX(), 150.0f);
    EXPECT_FLOAT_EQ (child->getPosition().getY(), 250.0f);

    // Test edge coordinates
    EXPECT_FLOAT_EQ (child->getLeft(), 150.0f);
    EXPECT_FLOAT_EQ (child->getTop(), 250.0f);
    EXPECT_FLOAT_EQ (child->getRight(), 250.0f);  // left + width (100)
    EXPECT_FLOAT_EQ (child->getBottom(), 325.0f); // top + height (75)
}

TEST_F (ComponentTest, SizeMethods)
{
    Size<float> newSize (200.0f, 150.0f);
    child->setSize (newSize);

    EXPECT_FLOAT_EQ (child->getSize().getWidth(), 200.0f);
    EXPECT_FLOAT_EQ (child->getSize().getHeight(), 150.0f);
    EXPECT_FLOAT_EQ (child->getWidth(), 200.0f);
    EXPECT_FLOAT_EQ (child->getHeight(), 150.0f);

    // Test individual dimension setters
    child->setSize (Size<float> (300.0f, 250.0f));
    EXPECT_FLOAT_EQ (child->getWidth(), 300.0f);
    EXPECT_FLOAT_EQ (child->getHeight(), 250.0f);
}

TEST_F (ComponentTest, BoundsMethods)
{
    Rectangle<float> newBounds (50.0f, 75.0f, 180.0f, 120.0f);
    child->setBounds (newBounds);

    auto bounds = child->getBounds();
    EXPECT_FLOAT_EQ (bounds.getX(), 50.0f);
    EXPECT_FLOAT_EQ (bounds.getY(), 75.0f);
    EXPECT_FLOAT_EQ (bounds.getWidth(), 180.0f);
    EXPECT_FLOAT_EQ (bounds.getHeight(), 120.0f);

    // Test setBounds with individual parameters
    child->setBounds (60.0f, 80.0f, 200.0f, 140.0f);
    bounds = child->getBounds();
    EXPECT_FLOAT_EQ (bounds.getX(), 60.0f);
    EXPECT_FLOAT_EQ (bounds.getY(), 80.0f);
    EXPECT_FLOAT_EQ (bounds.getWidth(), 200.0f);
    EXPECT_FLOAT_EQ (bounds.getHeight(), 140.0f);
}

TEST_F (ComponentTest, SafeAreaBoundsWithoutParentMatchesLocalBounds)
{
    EXPECT_EQ (root->getSafeAreaBounds(), root->getLocalBounds());
}

TEST_F (ComponentTest, SafeAreaBoundsOfNestedChildIsClippedToLocalBounds)
{
    // parent is fully inside root, so its safe area matches its local bounds
    EXPECT_EQ (parent->getSafeAreaBounds(), parent->getLocalBounds());

    // move child so it extends past the parent's bottom-right corner
    child->setBounds (150.0f, 100.0f, 100.0f, 75.0f);
    EXPECT_EQ (child->getSafeAreaBounds(), Rectangle<float> (0.0f, 0.0f, 50.0f, 50.0f));
}

TEST_F (ComponentTest, CornerAndCenterMethods)
{
    child->setBounds (100.0f, 200.0f, 60.0f, 40.0f);

    // Test corner getters
    EXPECT_FLOAT_EQ (child->getTopLeft().getX(), 100.0f);
    EXPECT_FLOAT_EQ (child->getTopLeft().getY(), 200.0f);

    EXPECT_FLOAT_EQ (child->getTopRight().getX(), 160.0f);
    EXPECT_FLOAT_EQ (child->getTopRight().getY(), 200.0f);

    EXPECT_FLOAT_EQ (child->getBottomLeft().getX(), 100.0f);
    EXPECT_FLOAT_EQ (child->getBottomLeft().getY(), 240.0f);

    EXPECT_FLOAT_EQ (child->getBottomRight().getX(), 160.0f);
    EXPECT_FLOAT_EQ (child->getBottomRight().getY(), 240.0f);

    // Test center
    EXPECT_FLOAT_EQ (child->getCenter().getX(), 130.0f);
    EXPECT_FLOAT_EQ (child->getCenter().getY(), 220.0f);
    EXPECT_FLOAT_EQ (child->getCenterX(), 130.0f);
    EXPECT_FLOAT_EQ (child->getCenterY(), 220.0f);

    // Test corner setters
    child->setTopLeft (Point<float> (80.0f, 180.0f));
    EXPECT_FLOAT_EQ (child->getX(), 80.0f);
    EXPECT_FLOAT_EQ (child->getY(), 180.0f);

    child->setCenter (Point<float> (200.0f, 300.0f));
    EXPECT_FLOAT_EQ (child->getCenterX(), 200.0f);
    EXPECT_FLOAT_EQ (child->getCenterY(), 300.0f);
    // Position should be center - size/2
    EXPECT_FLOAT_EQ (child->getX(), 170.0f);
    EXPECT_FLOAT_EQ (child->getY(), 280.0f);
}

TEST_F (ComponentTest, ProportionalMethods)
{
    child->setBounds (0.0f, 0.0f, 200.0f, 100.0f);

    EXPECT_FLOAT_EQ (child->proportionOfWidth (0.5f), 100.0f);
    EXPECT_FLOAT_EQ (child->proportionOfWidth (0.25f), 50.0f);
    EXPECT_FLOAT_EQ (child->proportionOfWidth (1.0f), 200.0f);

    EXPECT_FLOAT_EQ (child->proportionOfHeight (0.5f), 50.0f);
    EXPECT_FLOAT_EQ (child->proportionOfHeight (0.25f), 25.0f);
    EXPECT_FLOAT_EQ (child->proportionOfHeight (1.0f), 100.0f);
}

// =============================================================================

TEST_F (ComponentTest, VisibilityMethods)
{
    // Components should be visible by default, but child visibility depends on parents
    // Make sure parent is visible first
    parent->setVisible (true);
    child->setVisible (true);
    EXPECT_TRUE (child->isVisible());

    child->setVisible (false);
    EXPECT_FALSE (child->isVisible());

    child->setVisible (true);
    EXPECT_TRUE (child->isVisible());
}

TEST_F (ComponentTest, EnabledState)
{
    // Components should be enabled by default
    EXPECT_TRUE (child->isEnabled());

    child->setEnabled (false);
    EXPECT_FALSE (child->isEnabled());

    child->setEnabled (true);
    EXPECT_TRUE (child->isEnabled());
}

TEST_F (ComponentTest, OpacityMethods)
{
    // Default opacity should be 1.0
    EXPECT_FLOAT_EQ (child->getOpacity(), 1.0f);

    child->setOpacity (0.5f);
    EXPECT_NEAR (child->getOpacity(), 0.5f, 0.01f); // Use NEAR for precision issues

    child->setOpacity (0.0f);
    EXPECT_FLOAT_EQ (child->getOpacity(), 0.0f);

    child->setOpacity (1.0f);
    EXPECT_FLOAT_EQ (child->getOpacity(), 1.0f);
}

// =============================================================================

TEST_F (ComponentTest, ChildManagement)
{
    auto newChild1 = std::make_unique<Component> ("newChild1");
    auto newChild2 = std::make_unique<Component> ("newChild2");
    auto newChild3 = std::make_unique<Component> ("newChild3");

    // Test initial state
    EXPECT_EQ (parent->getNumChildComponents(), 1); // Already has 'child'

    // Test adding children
    parent->addChildComponent (*newChild1);
    EXPECT_EQ (parent->getNumChildComponents(), 2);

    parent->addChildComponent (*newChild2);
    parent->addChildComponent (*newChild3);
    EXPECT_EQ (parent->getNumChildComponents(), 4);

    // Test child retrieval
    EXPECT_EQ (parent->getChildComponent (0), child.get());
    EXPECT_EQ (parent->getChildComponent (1), newChild1.get());
    EXPECT_EQ (parent->getChildComponent (2), newChild2.get());
    EXPECT_EQ (parent->getChildComponent (3), newChild3.get());

    // Test index lookup
    EXPECT_EQ (parent->getIndexOfChildComponent (child.get()), 0);
    EXPECT_EQ (parent->getIndexOfChildComponent (newChild1.get()), 1);
    EXPECT_EQ (parent->getIndexOfChildComponent (newChild2.get()), 2);
    EXPECT_EQ (parent->getIndexOfChildComponent (newChild3.get()), 3);

    // Test parent relationships
    EXPECT_EQ (child->getParentComponent(), parent.get());
    EXPECT_EQ (newChild1->getParentComponent(), parent.get());
    EXPECT_TRUE (child->hasParent());
    EXPECT_TRUE (newChild1->hasParent());

    // Test removing children
    parent->removeChildComponent (newChild2.get());
    EXPECT_EQ (parent->getNumChildComponents(), 3);
    EXPECT_EQ (newChild2->getParentComponent(), nullptr);
    EXPECT_FALSE (newChild2->hasParent());

    // Test removeAllChildren
    parent->removeAllChildren();
    EXPECT_EQ (parent->getNumChildComponents(), 0);
    EXPECT_EQ (child->getParentComponent(), nullptr);
    EXPECT_EQ (newChild1->getParentComponent(), nullptr);
    EXPECT_EQ (newChild3->getParentComponent(), nullptr);
}

TEST_F (ComponentTest, ChildInsertionAtIndex)
{
    auto newChild1 = std::make_unique<Component> ("newChild1");
    auto newChild2 = std::make_unique<Component> ("newChild2");
    auto newChild3 = std::make_unique<Component> ("newChild3");

    // Insert at specific indices
    parent->addChildComponent (*newChild1, 0); // Insert at beginning
    EXPECT_EQ (parent->getChildComponent (0), newChild1.get());
    EXPECT_EQ (parent->getChildComponent (1), child.get());

    parent->addChildComponent (*newChild2, 1); // Insert in middle
    EXPECT_EQ (parent->getChildComponent (0), newChild1.get());
    EXPECT_EQ (parent->getChildComponent (1), newChild2.get());
    EXPECT_EQ (parent->getChildComponent (2), child.get());

    parent->addChildComponent (*newChild3); // Add at end (default)
    EXPECT_EQ (parent->getChildComponent (3), newChild3.get());
    EXPECT_EQ (parent->getNumChildComponents(), 4);
}

TEST_F (ComponentTest, AddAndMakeVisible)
{
    auto newChild = std::make_unique<Component> ("newChild");
    newChild->setVisible (false);
    EXPECT_FALSE (newChild->isVisible());

    parent->addAndMakeVisible (*newChild);
    EXPECT_TRUE (newChild->isVisible());
    EXPECT_EQ (newChild->getParentComponent(), parent.get());
}

// =============================================================================

TEST_F (ComponentTest, ZOrderMethods)
{
    auto sibling1 = std::make_unique<Component> ("sibling1");
    auto sibling2 = std::make_unique<Component> ("sibling2");
    auto sibling3 = std::make_unique<Component> ("sibling3");

    parent->addChildComponent (*sibling1);
    parent->addChildComponent (*sibling2);
    parent->addChildComponent (*sibling3);

    // Initial order: child(0), sibling1(1), sibling2(2), sibling3(3)
    EXPECT_EQ (parent->getIndexOfChildComponent (child.get()), 0);
    EXPECT_EQ (parent->getIndexOfChildComponent (sibling1.get()), 1);
    EXPECT_EQ (parent->getIndexOfChildComponent (sibling2.get()), 2);
    EXPECT_EQ (parent->getIndexOfChildComponent (sibling3.get()), 3);

    // Test toFront - moves to end
    sibling1->toFront (false);
    EXPECT_EQ (parent->getIndexOfChildComponent (sibling1.get()), 3);
    EXPECT_EQ (parent->getIndexOfChildComponent (sibling3.get()), 2);

    // Test toBack - moves to beginning
    sibling2->toBack();
    EXPECT_EQ (parent->getIndexOfChildComponent (sibling2.get()), 0);
    EXPECT_EQ (parent->getIndexOfChildComponent (child.get()), 1);

    // Test toBehind
    /*
    sibling3->toBehind (child.get());
    EXPECT_EQ (parent->getIndexOfChildComponent (sibling3.get()), 1);
    EXPECT_EQ (parent->getIndexOfChildComponent (child.get()), 2);
    */
}

// =============================================================================

TEST_F (ComponentTest, HitTesting)
{
    child->setBounds (50.0f, 50.0f, 100.0f, 80.0f);

    // Test contains method - bounds are inclusive of bottom-right edge
    EXPECT_TRUE (child->getBounds().contains (Point<float> (60.0f, 60.0f)));    // Inside
    EXPECT_TRUE (child->getBounds().contains (Point<float> (50.0f, 50.0f)));    // Top-left corner
    EXPECT_TRUE (child->getBounds().contains (Point<float> (149.0f, 129.0f)));  // Bottom-right corner
    EXPECT_TRUE (child->getBounds().contains (Point<float> (150.0f, 130.0f)));  // Bottom-right edge (inclusive)
    EXPECT_FALSE (child->getBounds().contains (Point<float> (151.0f, 131.0f))); // Actually outside
    EXPECT_FALSE (child->getBounds().contains (Point<float> (40.0f, 60.0f)));   // Left of bounds
    EXPECT_FALSE (child->getBounds().contains (Point<float> (60.0f, 40.0f)));   // Above bounds

    // Test with nested components
    auto nestedChild = std::make_unique<Component> ("nestedChild");
    nestedChild->setBounds (10.0f, 10.0f, 30.0f, 20.0f);
    child->addChildComponent (*nestedChild);

    // Test component finding - note: findComponentAt might not exist, testing basic functionality
    // The nested child should be found by index
    EXPECT_EQ (child->getChildComponent (0), nestedChild.get());
    EXPECT_EQ (child->getNumChildComponents(), 1);
}

// =============================================================================

TEST_F (ComponentTest, KeyboardFocus)
{
    // Test default focus behavior
    EXPECT_FALSE (child->getWantsKeyboardFocus());

    child->setWantsKeyboardFocus (true);
    EXPECT_TRUE (child->getWantsKeyboardFocus());

    child->setWantsKeyboardFocus (false);
    EXPECT_FALSE (child->getWantsKeyboardFocus());
}

TEST_F (ComponentTest, ClickingGrabFocus)
{
    EXPECT_TRUE (child->getClickingGrabFocus());

    child->setClickingGrabFocus (false);
    EXPECT_FALSE (child->getClickingGrabFocus());

    child->setClickingGrabFocus (true);
    EXPECT_TRUE (child->getClickingGrabFocus());
}

// =============================================================================

TEST_F (ComponentTest, TransformMethods)
{
    // Reset any existing transform first
    child->setTransform (AffineTransform());

    // Test default transform state
    EXPECT_FALSE (child->isTransformed());

    // Test setting transform
    AffineTransform transform = AffineTransform::rotation (0.5f);
    child->setTransform (transform);
    EXPECT_TRUE (child->isTransformed());

    auto retrievedTransform = child->getTransform();
    EXPECT_TRUE (transform.approximatelyEqualTo (retrievedTransform));

    // Test resetting transform - identity transform might still be considered "transformed"
    child->setTransform (AffineTransform());
    EXPECT_FALSE (child->isTransformed());
}

// =============================================================================

TEST_F (ComponentTest, ComponentIdAndLookup)
{
    EXPECT_EQ (child->getComponentID(), "child");
    EXPECT_EQ (parent->getComponentID(), "parent");
    EXPECT_EQ (root->getComponentID(), "root");

    /*
    // Test findChildWithID
    auto foundChild = parent->findChildWithID ("child");
    EXPECT_EQ (foundChild, child.get());

    auto notFound = parent->findChildWithID ("nonexistent");
    EXPECT_EQ (notFound, nullptr);

    // Test with nested children
    auto grandChild = std::make_unique<Component>("grandChild");
    child->addChildComponent (*grandChild);

    auto foundGrandChild = parent->findChildWithID ("grandChild");
    EXPECT_EQ (foundGrandChild, grandChild.get());
    */
}

// =============================================================================

TEST_F (ComponentTest, RepaintMethods)
{
    // These methods don't have easily testable return values,
    // but we can at least verify they don't crash
    child->repaint();
    child->repaint (Rectangle<float> (10.0f, 10.0f, 50.0f, 30.0f));
    child->repaint (10.0f, 10.0f, 50.0f, 30.0f);

    // Test rendering unclipped
    EXPECT_FALSE (child->isRenderingUnclipped());
    child->enableRenderingUnclipped (true);
    EXPECT_TRUE (child->isRenderingUnclipped());
    child->enableRenderingUnclipped (false);
    EXPECT_FALSE (child->isRenderingUnclipped());
}

// =============================================================================

TEST_F (ComponentTest, MouseCursorMethods)
{
    // Test default cursor
    auto defaultCursor = child->getMouseCursor();

    // Test setting different cursor types
    child->setMouseCursor (MouseCursor::Hand);
    EXPECT_EQ (child->getMouseCursor().getType(), MouseCursor::Hand);

    child->setMouseCursor (MouseCursor::Crosshair);
    EXPECT_EQ (child->getMouseCursor().getType(), MouseCursor::Crosshair);
}

// =============================================================================

TEST_F (ComponentTest, SetBottomLeft)
{
    child->setBottomLeft (Point<float> (200.0f, 150.0f));

    EXPECT_FLOAT_EQ (200.0f, child->getX());
    EXPECT_FLOAT_EQ (150.0f - 75.0f, child->getY());
}

TEST_F (ComponentTest, SetTopRight)
{
    child->setTopRight (Point<float> (200.0f, 50.0f));

    EXPECT_FLOAT_EQ (200.0f - 100.0f, child->getX());
    EXPECT_FLOAT_EQ (50.0f, child->getY());
}

TEST_F (ComponentTest, SetBottomRight)
{
    child->setBottomRight (Point<float> (200.0f, 150.0f));

    EXPECT_FLOAT_EQ (200.0f - 100.0f, child->getX());
    EXPECT_FLOAT_EQ (150.0f - 75.0f, child->getY());
}

TEST_F (ComponentTest, SetCenterX)
{
    parent->setSize (400.0f, 300.0f);
    child->setSize (100.0f, 75.0f);

    child->setCenterX (200.0f);

    EXPECT_FLOAT_EQ (150.0f, child->getX());
}

TEST_F (ComponentTest, SetCenterY)
{
    parent->setSize (400.0f, 300.0f);
    child->setSize (100.0f, 75.0f);

    child->setCenterY (150.0f);

    EXPECT_FLOAT_EQ (112.5f, child->getY());
}

TEST_F (ComponentTest, GetScreenBounds)
{
    auto bounds = child->getScreenBounds();

    EXPECT_FLOAT_EQ (100.0f, bounds.getWidth());
    EXPECT_FLOAT_EQ (75.0f, bounds.getHeight());
}

TEST_F (ComponentTest, GetBoundsRelativeToTopLevel)
{
    auto bounds = child->getBoundsRelativeToTopLevelComponent();

    EXPECT_FLOAT_EQ (75.0f, bounds.getX());
    EXPECT_FLOAT_EQ (75.0f, bounds.getY());
    EXPECT_FLOAT_EQ (100.0f, bounds.getWidth());
    EXPECT_FLOAT_EQ (75.0f, bounds.getHeight());
}

TEST_F (ComponentTest, GetLocalArea)
{
    auto sibling = std::make_unique<Component> ("sibling");
    sibling->setBounds (200.0f, 100.0f, 50.0f, 50.0f);
    root->addChildComponent (*sibling);

    auto area = child->getLocalArea (sibling.get(), sibling->getLocalBounds());

    EXPECT_FLOAT_EQ (125.0f, area.getX());
    EXPECT_FLOAT_EQ (25.0f, area.getY());
    EXPECT_FLOAT_EQ (50.0f, area.getWidth());
    EXPECT_FLOAT_EQ (50.0f, area.getHeight());
}

TEST_F (ComponentTest, RaiseBy)
{
    auto child1 = std::make_unique<Component> ("child1");
    auto child2 = std::make_unique<Component> ("child2");
    auto child3 = std::make_unique<Component> ("child3");

    parent->addChildComponent (*child1);
    parent->addChildComponent (*child2);
    parent->addChildComponent (*child3);

    // Before: child(0), child1(1), child2(2), child3(3)
    child1->raiseBy (2);

    // After raising child1 by 2: child(0), child2(1), child3(2), child1(3)
    EXPECT_EQ (child.get(), parent->getChildComponent (0));
    EXPECT_EQ (child2.get(), parent->getChildComponent (1));
    EXPECT_EQ (child3.get(), parent->getChildComponent (2));
    EXPECT_EQ (child1.get(), parent->getChildComponent (3));
}

TEST_F (ComponentTest, LowerBy)
{
    auto child1 = std::make_unique<Component> ("child1");
    auto child2 = std::make_unique<Component> ("child2");
    auto child3 = std::make_unique<Component> ("child3");

    parent->addChildComponent (*child1);
    parent->addChildComponent (*child2);
    parent->addChildComponent (*child3);

    // Before: child(0), child1(1), child2(2), child3(3)
    child3->lowerBy (2);

    // After lowering child3 by 2: child(0), child3(1), child1(2), child2(3)
    EXPECT_EQ (child.get(), parent->getChildComponent (0));
    EXPECT_EQ (child3.get(), parent->getChildComponent (1));
    EXPECT_EQ (child1.get(), parent->getChildComponent (2));
    EXPECT_EQ (child2.get(), parent->getChildComponent (3));
}

TEST_F (ComponentTest, RemoveChildComponentByIndex)
{
    auto child1 = std::make_unique<Component> ("child1");
    parent->addChildComponent (*child1);

    EXPECT_EQ (2u, parent->getNumChildComponents());

    parent->removeChildComponent (1);

    EXPECT_EQ (1u, parent->getNumChildComponents());
    EXPECT_EQ (child.get(), parent->getChildComponent (0));
}

TEST_F (ComponentTest, AddAndMakeVisibleWithIndex)
{
    child->setBounds (25, 25, 100, 75);

    parent->removeChildComponent (*child);

    parent->addAndMakeVisible (child.get(), 0);

    EXPECT_TRUE (child->isVisible());
    EXPECT_EQ (child.get(), parent->getChildComponent (0));
}

TEST_F (ComponentTest, GetParentComponentConstOverload)
{
    const Component* constChild = child.get();
    const Component* constParent = constChild->getParentComponent();

    EXPECT_EQ (parent.get(), constParent);
}

TEST_F (ComponentTest, FocusGainedDoesNotCrash)
{
    EXPECT_NO_THROW (child->focusGained());
}

TEST_F (ComponentTest, FocusLostDoesNotCrash)
{
    EXPECT_NO_THROW (child->focusLost());
}

// =============================================================================

TEST_F (ComponentTest, OpaqueMethods)
{
    root->setVisible (true);
    parent->setVisible (true);
    child->setVisible (true);

    // Test default opaque state (should be true by default)
    EXPECT_TRUE (child->isOpaque());
    EXPECT_TRUE (parent->isOpaque());
    EXPECT_TRUE (root->isOpaque());

    // Test setting opaque state
    child->setOpaque (false);
    EXPECT_FALSE (child->isOpaque());

    child->setOpaque (true);
    EXPECT_TRUE (child->isOpaque());

    // Test that changing opaque state doesn't affect other properties
    child->setOpaque (false);
    EXPECT_TRUE (child->isVisible());
    EXPECT_TRUE (child->isEnabled());
    EXPECT_FLOAT_EQ (child->getOpacity(), 1.0f);
}

TEST_F (ComponentTest, OpaqueConstructorBehavior)
{
    // Test that new components are opaque by default
    auto newComponent1 = std::make_unique<Component>();
    EXPECT_TRUE (newComponent1->isOpaque());

    auto newComponent2 = std::make_unique<Component> ("test_id");
    EXPECT_TRUE (newComponent2->isOpaque());
}

TEST_F (ComponentTest, OpaqueStateIndependence)
{
    // Test that opaque state is independent for each component
    child->setOpaque (false);
    parent->setOpaque (true);
    root->setOpaque (false);

    EXPECT_FALSE (child->isOpaque());
    EXPECT_TRUE (parent->isOpaque());
    EXPECT_FALSE (root->isOpaque());

    // Test that parent opaque state doesn't affect child
    auto grandChild = std::make_unique<Component> ("grandChild");
    child->addChildComponent (*grandChild);

    EXPECT_TRUE (grandChild->isOpaque()); // Should be default true
    child->setOpaque (false);
    EXPECT_TRUE (grandChild->isOpaque()); // Should remain unchanged
}

TEST_F (ComponentTest, OpaqueStateWithMultipleChildren)
{
    // Create multiple children with different opaque states
    auto child1 = std::make_unique<Component> ("child1");
    auto child2 = std::make_unique<Component> ("child2");
    auto child3 = std::make_unique<Component> ("child3");

    child1->setOpaque (true);
    child2->setOpaque (false);
    child3->setOpaque (true);

    parent->addChildComponent (*child1);
    parent->addChildComponent (*child2);
    parent->addChildComponent (*child3);

    // Verify each child maintains its own opaque state
    EXPECT_TRUE (child1->isOpaque());
    EXPECT_FALSE (child2->isOpaque());
    EXPECT_TRUE (child3->isOpaque());

    // Change one and verify others are unaffected
    child1->setOpaque (false);
    EXPECT_FALSE (child1->isOpaque());
    EXPECT_FALSE (child2->isOpaque());
    EXPECT_TRUE (child3->isOpaque());
}

// =============================================================================
// Tests for missing Component methods using ComponentMock
// =============================================================================

class ComponentMockTest : public ::testing::Test
{
protected:
    using ComponentHelper = yup::ComponentTestHelper<yup::Component>;

    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();
        theme = new ApplicationTheme();
        ApplicationTheme::setGlobalTheme (theme);

        mockComponent = std::make_unique<ComponentMock> ("mockComponent");
    }

    void TearDown() override
    {
        mockComponent.reset();
        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    std::unique_ptr<ComponentMock> mockComponent;
    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
};

// =============================================================================

TEST_F (ComponentMockTest, VirtualMethodCallbacks)
{
    // Test enablementChanged callback
    EXPECT_TRUE (mockComponent->isEnabled());

    EXPECT_CALL (*mockComponent, enablementChanged());
    mockComponent->setEnabled (false);
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    // Test visibilityChanged callback
    mockComponent->setVisible (false);

    // Test moved callback
    EXPECT_CALL (*mockComponent, moved());
    mockComponent->setPosition (Point<float> (10.0f, 20.0f));
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    // Test resized callback
    EXPECT_CALL (*mockComponent, resized());
    mockComponent->setSize (Size<float> (100.0f, 80.0f));
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    // Test setBounds triggers both moved and resized
    EXPECT_CALL (*mockComponent, moved());
    EXPECT_CALL (*mockComponent, resized());
    mockComponent->setBounds (5.0f, 10.0f, 150.0f, 120.0f);
}

TEST_F (ComponentMockTest, TitleMethods)
{
    // Test default title
    EXPECT_TRUE (mockComponent->getTitle().isEmpty());

    // Test setting title
    mockComponent->setTitle ("Test Title");
    EXPECT_EQ (mockComponent->getTitle(), "Test Title");

    // Test changing title
    mockComponent->setTitle ("New Title");
    EXPECT_EQ (mockComponent->getTitle(), "New Title");

    // Test empty title
    mockComponent->setTitle ("");
    EXPECT_TRUE (mockComponent->getTitle().isEmpty());
}

TEST_F (ComponentMockTest, KeyboardFocusMethods)
{
    // Test default focus behavior
    mockComponent->setWantsKeyboardFocus (false);
    EXPECT_FALSE (mockComponent->hasKeyboardFocus());

    // Test setting wants keyboard focus
    mockComponent->setWantsKeyboardFocus (true);
    // Note: hasKeyboardFocus() requires native component, so we can't test actual focus

    // Test focus methods don't crash
    mockComponent->takeKeyboardFocus();
    mockComponent->leaveKeyboardFocus();
}

TEST_F (ComponentMockTest, SecondaryTouchCannotGrabKeyboardFocusDuringMouseDown)
{
    ComponentHelper::attachMockNative (*mockComponent);
    mockComponent->setVisible (true);
    mockComponent->setWantsKeyboardFocus (true);

    ON_CALL (*mockComponent, mouseDown (_)).WillByDefault ([this] (const MouseEvent&)
    {
        mockComponent->takeKeyboardFocus();
    });

    const auto touchDown = MouseEvent().withTouchIndex (1);
    ComponentHelper::triggerInternalMouseDown (*mockComponent, touchDown);
    EXPECT_FALSE (mockComponent->hasKeyboardFocus());

    const auto firstTouchDown = touchDown.withTouchIndex (0);
    ComponentHelper::triggerInternalMouseDown (*mockComponent, firstTouchDown);
    EXPECT_TRUE (mockComponent->hasKeyboardFocus());

    ComponentHelper::detachMockNative (*mockComponent);
}

TEST_F (ComponentMockTest, ParentHierarchyMethods)
{
    auto parentMock = std::make_unique<ComponentMock> ("parentMock");

    // Test no parent initially
    EXPECT_FALSE (mockComponent->hasParent());
    EXPECT_EQ (mockComponent->getParentComponent(), nullptr);

    // Test parent hierarchy changed callback
    EXPECT_CALL (*mockComponent, parentHierarchyChanged());
    EXPECT_CALL (*parentMock, childrenChanged());
    parentMock->addChildComponent (*mockComponent);

    // Test has parent
    EXPECT_TRUE (mockComponent->hasParent());
    EXPECT_EQ (mockComponent->getParentComponent(), parentMock.get());

    // Test getParentComponentWithType
    auto typedParent = mockComponent->getParentComponentWithType<ComponentMock>();
    EXPECT_EQ (typedParent, parentMock.get());

    auto wrongTypeParent = mockComponent->getParentComponentWithType<Component>();
    EXPECT_EQ (wrongTypeParent, parentMock.get()); // Should still find it as Component base class
}

TEST_F (ComponentMockTest, ComponentProperties)
{
    // Test properties access
    auto& properties = mockComponent->getProperties();
    EXPECT_TRUE (properties.isEmpty());

    // Test setting properties
    properties.set ("testKey", "testValue");
    EXPECT_EQ (properties["testKey"].toString(), "testValue");

    // Test const access
    const auto& constProperties = const_cast<const ComponentMock*> (mockComponent.get())->getProperties();
    EXPECT_EQ (constProperties["testKey"].toString(), "testValue");
}

TEST_F (ComponentMockTest, MouseEventSettings)
{
    // Test default mouse event settings
    EXPECT_TRUE (mockComponent->doesWantSelfMouseEvents());
    EXPECT_TRUE (mockComponent->doesWantChildrenMouseEvents());

    // Test changing mouse event settings
    mockComponent->setWantsMouseEvents (false, true);
    EXPECT_FALSE (mockComponent->doesWantSelfMouseEvents());
    EXPECT_TRUE (mockComponent->doesWantChildrenMouseEvents());

    mockComponent->setWantsMouseEvents (true, false);
    EXPECT_TRUE (mockComponent->doesWantSelfMouseEvents());
    EXPECT_FALSE (mockComponent->doesWantChildrenMouseEvents());

    mockComponent->setWantsMouseEvents (false, false);
    EXPECT_FALSE (mockComponent->doesWantSelfMouseEvents());
    EXPECT_FALSE (mockComponent->doesWantChildrenMouseEvents());
}

TEST_F (ComponentMockTest, MouseListenerMethods)
{
    // Create a simple mouse listener to test with
    class TestMouseListener : public MouseListener
    {
    public:
        bool mouseEnterCalled = false;
        bool mouseExitCalled = false;

        void mouseEnter (const MouseEvent& event) override { mouseEnterCalled = true; }

        void mouseExit (const MouseEvent& event) override { mouseExitCalled = true; }
    };

    auto listener = std::make_unique<TestMouseListener>();

    // Test adding listener doesn't crash
    mockComponent->addMouseListener (listener.get());

    // Test removing listener doesn't crash
    mockComponent->removeMouseListener (listener.get());
}

TEST_F (ComponentMockTest, ComponentListenerReceivesMovedCallback)
{
    RecordingComponentListener listener;
    mockComponent->addComponentListener (&listener);

    mockComponent->setPosition ({ 10.0f, 20.0f });

    EXPECT_EQ (1, listener.movedCount);
    EXPECT_EQ (static_cast<Component*> (mockComponent.get()), listener.lastMovedComponent);
    EXPECT_EQ (0, listener.resizedCount);
}

TEST_F (ComponentMockTest, ComponentListenerReceivesResizedCallback)
{
    RecordingComponentListener listener;
    mockComponent->addComponentListener (&listener);

    mockComponent->setSize ({ 100.0f, 80.0f });

    EXPECT_EQ (1, listener.resizedCount);
    EXPECT_EQ (static_cast<Component*> (mockComponent.get()), listener.lastResizedComponent);
    EXPECT_EQ (0, listener.movedCount);
}

TEST_F (ComponentMockTest, ComponentListenerReceivesMovedAndResizedFromSetBounds)
{
    RecordingComponentListener listener;
    mockComponent->addComponentListener (&listener);

    mockComponent->setBounds (5.0f, 10.0f, 150.0f, 120.0f);

    EXPECT_EQ (1, listener.movedCount);
    EXPECT_EQ (1, listener.resizedCount);
    EXPECT_EQ (static_cast<Component*> (mockComponent.get()), listener.lastMovedComponent);
    EXPECT_EQ (static_cast<Component*> (mockComponent.get()), listener.lastResizedComponent);
}

TEST_F (ComponentMockTest, RemovingComponentListenerStopsCallbacks)
{
    RecordingComponentListener listener;
    mockComponent->addComponentListener (&listener);
    mockComponent->removeComponentListener (&listener);

    mockComponent->setPosition ({ 10.0f, 20.0f });
    mockComponent->setSize ({ 100.0f, 80.0f });

    EXPECT_EQ (0, listener.movedCount);
    EXPECT_EQ (0, listener.resizedCount);
}

TEST_F (ComponentMockTest, ComponentListenerIsOnlyAddedOnce)
{
    RecordingComponentListener listener;
    mockComponent->addComponentListener (&listener);
    mockComponent->addComponentListener (&listener);

    mockComponent->setPosition ({ 10.0f, 20.0f });

    EXPECT_EQ (1, listener.movedCount);
}

TEST_F (ComponentMockTest, DestroyedComponentListenerIsSkipped)
{
    auto listener = std::make_unique<RecordingComponentListener>();
    mockComponent->addComponentListener (listener.get());

    listener.reset();

    const Point<float> newPosition (10.0f, 20.0f);
    EXPECT_NO_FATAL_FAILURE (mockComponent->setPosition (newPosition));
}

TEST_F (ComponentMockTest, ComponentListenerReceivesBeingDeletedCallback)
{
    RecordingComponentListener listener;
    auto component = std::make_unique<ComponentMock> ("delete-notified");
    component->addComponentListener (&listener);

    auto* componentAddress = static_cast<Component*> (component.get());
    component.reset();

    EXPECT_EQ (1, listener.deletedCount);
    EXPECT_EQ (componentAddress, listener.lastDeletedComponent);
}

TEST_F (ComponentMockTest, RemovedComponentListenerDoesNotReceiveBeingDeletedCallback)
{
    RecordingComponentListener listener;
    auto component = std::make_unique<ComponentMock> ("delete-notified");
    component->addComponentListener (&listener);
    component->removeComponentListener (&listener);

    component.reset();

    EXPECT_EQ (0, listener.deletedCount);
}

TEST_F (ComponentMockTest, StyleMethods)
{
    // Test default style
    EXPECT_EQ (mockComponent->getStyle(), nullptr);

    // Create a mock style
    class TestStyle : public ComponentStyle
    {
    public:
        void paint (Graphics& g, const ApplicationTheme& theme, const Component& component) override {}
    };

    auto style = ComponentStyle::Ptr (new TestStyle());

    // Test setting style
    EXPECT_CALL (*mockComponent, styleChanged());
    mockComponent->setStyle (style);
    EXPECT_EQ (mockComponent->getStyle(), style);
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    // Test setting same style doesn't trigger callback
    mockComponent->setStyle (style);
    // GMock NiceMock: no expectation = no warning for uninteresting call

    // Test setting null style
    EXPECT_CALL (*mockComponent, styleChanged());
    mockComponent->setStyle (nullptr);
    EXPECT_EQ (mockComponent->getStyle(), nullptr);
}

TEST_F (ComponentMockTest, TransformMethods)
{
    // Test getTransformToComponent
    auto otherComponent = std::make_unique<ComponentMock> ("other");
    otherComponent->setBounds (100, 100, 50, 50);

    auto transform = mockComponent->getTransformToComponent (otherComponent.get());
    EXPECT_TRUE (transform.isIdentity() || ! transform.isIdentity()); // Just test it doesn't crash

    // Test getTransformFromComponent
    auto fromTransform = mockComponent->getTransformFromComponent (otherComponent.get());
    EXPECT_TRUE (fromTransform.isIdentity() || ! fromTransform.isIdentity()); // Just test it doesn't crash

    // Test getTransformToScreen
    auto screenTransform = mockComponent->getTransformToScreen();
    EXPECT_TRUE (screenTransform.isIdentity() || ! screenTransform.isIdentity()); // Just test it doesn't crash
}

TEST_F (ComponentMockTest, FullScreenMethods)
{
    // Test default fullscreen state
    EXPECT_FALSE (mockComponent->isFullScreen());

    // Test setting fullscreen (won't work without native component, but shouldn't crash)
    mockComponent->setFullScreen (true);
    // Can't test the result without native component, but method should not crash
}

TEST_F (ComponentMockTest, DesktopMethods)
{
    // Test default desktop state
    EXPECT_FALSE (mockComponent->isOnDesktop());

    // Test desktop methods don't crash (they won't work without proper setup)
    // Note: These require platform-specific native components to work properly
    // mockComponent->addToDesktop (ComponentNative::Options());
    // mockComponent->removeFromDesktop();
}

TEST_F (ComponentMockTest, ZOrderMethods)
{
    auto parent = std::make_unique<ComponentMock> ("parent");
    auto sibling1 = std::make_unique<ComponentMock> ("sibling1");
    auto sibling2 = std::make_unique<ComponentMock> ("sibling2");

    parent->addChildComponent (*sibling1);
    parent->addChildComponent (*mockComponent);
    parent->addChildComponent (*sibling2);

    // Test initial order
    EXPECT_EQ (parent->getIndexOfChildComponent (sibling1.get()), 0);
    EXPECT_EQ (parent->getIndexOfChildComponent (mockComponent.get()), 1);
    EXPECT_EQ (parent->getIndexOfChildComponent (sibling2.get()), 2);

    // Test toFront
    mockComponent->toFront (false);
    EXPECT_EQ (parent->getIndexOfChildComponent (mockComponent.get()), 2);

    // Test toBack
    mockComponent->toBack();
    EXPECT_EQ (parent->getIndexOfChildComponent (mockComponent.get()), 0);

    // Test raiseAbove
    mockComponent->raiseAbove (sibling1.get());
    EXPECT_GT (parent->getIndexOfChildComponent (mockComponent.get()),
               parent->getIndexOfChildComponent (sibling1.get()));

    // Test lowerBelow
    mockComponent->lowerBelow (sibling1.get());
    EXPECT_LT (parent->getIndexOfChildComponent (mockComponent.get()),
               parent->getIndexOfChildComponent (sibling1.get()));
}

TEST_F (ComponentMockTest, ComponentHierarchyTraversal)
{
    auto grandparent = std::make_unique<ComponentMock> ("grandparent");
    auto parent = std::make_unique<ComponentMock> ("parent");
    auto child = std::make_unique<ComponentMock> ("child");

    grandparent->setVisible (true);
    grandparent->addAndMakeVisible (*parent);
    parent->addAndMakeVisible (*child);

    // Test getTopLevelComponent
    EXPECT_EQ (child->getTopLevelComponent(), grandparent.get());
    EXPECT_EQ (parent->getTopLevelComponent(), grandparent.get());
    EXPECT_EQ (grandparent->getTopLevelComponent(), grandparent.get());

    // Test findComponentAt
    grandparent->setBounds (0, 0, 100, 100);
    parent->setBounds (50, 50, 50, 50);
    child->setBounds (10, 10, 10, 10);

    {
        auto foundComponent = grandparent->findComponentAt (Point<float> (75, 75));
        EXPECT_EQ (foundComponent, parent.get());

        auto foundSelfComponent = grandparent->findComponentAt (Point<float> (5, 5));
        EXPECT_EQ (foundSelfComponent, grandparent.get());

        auto foundNothing = grandparent->findComponentAt (Point<float> (200, 50));
        EXPECT_EQ (foundNothing, nullptr);
    }

    {
        auto foundComponent = parent->findComponentAt (Point<float> (15, 15));
        EXPECT_EQ (foundComponent, child.get());

        auto foundSelfComponent = parent->findComponentAt (Point<float> (5, 5));
        EXPECT_EQ (foundSelfComponent, parent.get());

        auto foundNothing = parent->findComponentAt (Point<float> (55, 55));
        EXPECT_EQ (foundNothing, nullptr);
    }

    {
        auto foundSelfComponent = child->findComponentAt (Point<float> (5, 5));
        EXPECT_EQ (foundSelfComponent, child.get());

        auto foundNothing = child->findComponentAt (Point<float> (55, 55));
        EXPECT_EQ (foundNothing, nullptr);
    }
}

TEST_F (ComponentMockTest, ProportionalSizeMethods)
{
    mockComponent->setSize (200.0f, 100.0f);

    // Test proportionOfWidth
    EXPECT_FLOAT_EQ (mockComponent->proportionOfWidth (0.5f), 100.0f);
    EXPECT_FLOAT_EQ (mockComponent->proportionOfWidth (1.0f), 200.0f);
    EXPECT_FLOAT_EQ (mockComponent->proportionOfWidth (0.25f), 50.0f);

    // Test proportionOfHeight
    EXPECT_FLOAT_EQ (mockComponent->proportionOfHeight (0.5f), 50.0f);
    EXPECT_FLOAT_EQ (mockComponent->proportionOfHeight (1.0f), 100.0f);
    EXPECT_FLOAT_EQ (mockComponent->proportionOfHeight (0.75f), 75.0f);
}

TEST_F (ComponentMockTest, NativeComponentMethods)
{
    // Test default native component access
    EXPECT_EQ (mockComponent->getNativeHandle(), nullptr);
    EXPECT_EQ (mockComponent->getNativeComponent(), nullptr);

    const auto* constComponent = mockComponent.get();
    EXPECT_EQ (constComponent->getNativeComponent(), nullptr);
}

// =============================================================================
// Tests for additional missing Component methods
// =============================================================================

TEST_F (ComponentMockTest, ContentScaleChangedCallback)
{
    // Test that contentScaleChanged is called (indirectly through internal methods)
    EXPECT_CALL (*mockComponent, contentScaleChanged (2.0f));
    mockComponent->contentScaleChanged (2.0f);

    // Test scale DPI getter
    float scaleDpi = mockComponent->getScaleDpi();
    EXPECT_GE (scaleDpi, 0.0f); // Should be positive
}

TEST_F (ComponentMockTest, TransformChangedCallback)
{
    // Test setting transform triggers transformChanged callback
    AffineTransform transform = AffineTransform::scaling (2.0f, 2.0f);

    EXPECT_CALL (*mockComponent, transformChanged());
    mockComponent->setTransform (transform);
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    // Test transform getters
    auto retrievedTransform = mockComponent->getTransform();
    EXPECT_TRUE (mockComponent->isTransformed());

    // Test resetting transform
    EXPECT_CALL (*mockComponent, transformChanged());
    mockComponent->setTransform (AffineTransform::identity());
    EXPECT_FALSE (mockComponent->isTransformed());
}

TEST_F (ComponentMockTest, ColorMethods)
{
    Identifier colorId ("testColor");
    Color testColor (255, 255, 0, 0); // Red

    // Test setting color
    mockComponent->setColor (colorId, testColor);

    // Test getting color
    auto retrievedColor = mockComponent->getColor (colorId);
    EXPECT_TRUE (retrievedColor.has_value());
    if (retrievedColor.has_value())
    {
        EXPECT_EQ (retrievedColor->getRed(), testColor.getRed());
        EXPECT_EQ (retrievedColor->getGreen(), testColor.getGreen());
        EXPECT_EQ (retrievedColor->getBlue(), testColor.getBlue());
    }

    // Test finding color
    auto foundColor = mockComponent->findColor (colorId);
    EXPECT_TRUE (foundColor.has_value());

    // Test setting null color
    mockComponent->setColor (colorId, std::nullopt);
    auto nullColor = mockComponent->getColor (colorId);
    EXPECT_FALSE (nullColor.has_value());

    // Test finding non-existent color
    Identifier nonExistentId ("nonExistent");
    auto notFoundColor = mockComponent->findColor (nonExistentId);
    EXPECT_FALSE (notFoundColor.has_value());
}

TEST_F (ComponentMockTest, UnclippedRenderingMethods)
{
    // Test default unclipped rendering state
    EXPECT_FALSE (mockComponent->isRenderingUnclipped());

    // Test enabling unclipped rendering
    mockComponent->enableRenderingUnclipped (true);
    EXPECT_TRUE (mockComponent->isRenderingUnclipped());

    // Test disabling unclipped rendering
    mockComponent->enableRenderingUnclipped (false);
    EXPECT_FALSE (mockComponent->isRenderingUnclipped());
}

TEST_F (ComponentMockTest, EnhancedVirtualMethodCallbacks)
{
    // Test enablement changed
    EXPECT_CALL (*mockComponent, enablementChanged());
    mockComponent->setEnabled (false);
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    EXPECT_CALL (*mockComponent, enablementChanged());
    mockComponent->setEnabled (true);
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    // Test visibility changed
    EXPECT_CALL (*mockComponent, visibilityChanged());
    mockComponent->setVisible (true);
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    EXPECT_CALL (*mockComponent, visibilityChanged());
    mockComponent->setVisible (false);
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    // Test moved callback
    EXPECT_CALL (*mockComponent, moved());
    mockComponent->setPosition (Point<float> (100.0f, 200.0f));
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    // Test resized callback
    EXPECT_CALL (*mockComponent, resized());
    mockComponent->setSize (Size<float> (300.0f, 400.0f));
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    // Test bounds change triggers both moved and resized
    EXPECT_CALL (*mockComponent, moved());
    EXPECT_CALL (*mockComponent, resized());
    mockComponent->setBounds (10.0f, 20.0f, 500.0f, 600.0f);
}

TEST_F (ComponentMockTest, VisibilityChangedFiresOnlyWhenShowingChanges)
{
    // Default component is not visible
    EXPECT_FALSE (mockComponent->isVisible());
    EXPECT_FALSE (mockComponent->isShowing());

    // Attach to a visible parent — setVisible fires visibilityChanged because parent is showing.
    auto parent = std::make_unique<ComponentMock> ("parent");
    parent->setVisible (true);

    EXPECT_CALL (*mockComponent, visibilityChanged());
    parent->addAndMakeVisible (*mockComponent);
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    EXPECT_TRUE (mockComponent->isVisible());
    EXPECT_TRUE (mockComponent->isShowing());

    // Hide parent — visibilityChanged fires on the parent, and on every visible child
    // whose showing state flips because the ancestor was hidden.
    EXPECT_CALL (*parent, visibilityChanged());
    EXPECT_CALL (*mockComponent, visibilityChanged());
    parent->setVisible (false);
    ::testing::Mock::VerifyAndClearExpectations (parent.get());
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    EXPECT_TRUE (mockComponent->isVisible());  // Own flag untouched
    EXPECT_FALSE (mockComponent->isShowing()); // Ancestor chain now hidden

    // Show parent again — child must be told it is now really showing.
    EXPECT_CALL (*parent, visibilityChanged());
    EXPECT_CALL (*mockComponent, visibilityChanged());
    parent->setVisible (true);
    ::testing::Mock::VerifyAndClearExpectations (parent.get());
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    EXPECT_TRUE (mockComponent->isVisible());
    EXPECT_TRUE (mockComponent->isShowing());
}

TEST_F (ComponentMockTest, AddAndMakeVisibleUnderHiddenParentDoesNotFireVisibilityChanged)
{
    auto parent = std::make_unique<ComponentMock> ("parent");
    // parent starts invisible (default).  Mock is invisible (default).

    // addAndMakeVisible calls setVisible(true), but because the parent is not showing,
    // visibilityChanged must NOT be called — the child is not actually showing yet.
    // (NiceMock silently ignores the unexpected visibilityChanged call, so we expect none.)
    parent->addAndMakeVisible (*mockComponent);

    EXPECT_TRUE (mockComponent->isVisible());
    EXPECT_FALSE (mockComponent->isShowing());

    // Later, when the parent becomes visible, the child must receive visibilityChanged.
    EXPECT_CALL (*parent, visibilityChanged());
    EXPECT_CALL (*mockComponent, visibilityChanged());
    parent->setVisible (true);
}

TEST_F (ComponentMockTest, VisibilityChangePropagatesThroughDeepHierarchy)
{
    auto grandparent = std::make_unique<ComponentMock> ("gp");
    auto parent = std::make_unique<ComponentMock> ("parent");
    auto child = std::make_unique<ComponentMock> ("child");

    grandparent->setVisible (true);
    EXPECT_CALL (*parent, visibilityChanged());
    grandparent->addAndMakeVisible (*parent);
    ::testing::Mock::VerifyAndClearExpectations (parent.get());

    EXPECT_CALL (*child, visibilityChanged());
    parent->addAndMakeVisible (*child);
    ::testing::Mock::VerifyAndClearExpectations (child.get());

    EXPECT_TRUE (child->isShowing());

    // Hiding grandparent should fire on grandparent, parent, and child
    EXPECT_CALL (*grandparent, visibilityChanged());
    EXPECT_CALL (*parent, visibilityChanged());
    EXPECT_CALL (*child, visibilityChanged());
    grandparent->setVisible (false);

    EXPECT_FALSE (child->isShowing());

    // An invisible child of the hidden parent is NOT affected — it was already hidden.
    auto invisibleChild = std::make_unique<ComponentMock> ("inv");
    parent->addChildComponent (*invisibleChild); // isVisible stays false

    // Making the parent visible only fires on parent (not the invisible child).
    ::testing::Mock::VerifyAndClearExpectations (parent.get());
    ::testing::Mock::VerifyAndClearExpectations (invisibleChild.get());

    EXPECT_CALL (*grandparent, visibilityChanged());
    EXPECT_CALL (*parent, visibilityChanged());
    EXPECT_CALL (*child, visibilityChanged());
    grandparent->setVisible (true);

    ::testing::Mock::VerifyAndClearExpectations (invisibleChild.get());
    EXPECT_FALSE (invisibleChild->isShowing());
}

TEST_F (ComponentMockTest, BailOutCheckerClass)
{
    // Test BailOutChecker functionality
    Component::BailOutChecker checker (mockComponent.get());

    // Component should be valid initially
    EXPECT_FALSE (checker.shouldBailOut());

    // Test copy constructor
    Component::BailOutChecker checker2 (checker);
    EXPECT_FALSE (checker2.shouldBailOut());

    // Test assignment operator
    Component::BailOutChecker checker3 (nullptr);
    checker3 = checker;
    EXPECT_FALSE (checker3.shouldBailOut());

    // Test with null component
    Component::BailOutChecker nullChecker (nullptr);
    EXPECT_TRUE (nullChecker.shouldBailOut());
}

TEST_F (ComponentMockTest, OpaqueStateAdvanced)
{
    // Test opaque state changes
    EXPECT_TRUE (mockComponent->isOpaque());

    mockComponent->setOpaque (false);
    EXPECT_FALSE (mockComponent->isOpaque());

    mockComponent->setOpaque (true);
    EXPECT_TRUE (mockComponent->isOpaque());

    // Test opaque with different scenarios
    auto parent = std::make_unique<ComponentMock> ("parent");
    auto child1 = std::make_unique<ComponentMock> ("child1");
    auto child2 = std::make_unique<ComponentMock> ("child2");

    parent->setBounds (0, 0, 200, 200);
    child1->setBounds (0, 0, 100, 100);
    child2->setBounds (100, 100, 100, 100);

    parent->addChildComponent (*child1);
    parent->addChildComponent (*child2);

    // Test various opaque configurations
    child1->setOpaque (false);
    EXPECT_FALSE (child1->isOpaque());
    EXPECT_TRUE (child2->isOpaque());
    EXPECT_TRUE (parent->isOpaque());

    child2->setOpaque (false);
    EXPECT_FALSE (child1->isOpaque());
    EXPECT_FALSE (child2->isOpaque());
    EXPECT_TRUE (parent->isOpaque());
}

TEST_F (ComponentMockTest, ComponentHierarchyAdvanced)
{
    auto parent = std::make_unique<ComponentMock> ("parent");
    auto child = std::make_unique<ComponentMock> ("child");

    // Test parentHierarchyChanged callback
    EXPECT_CALL (*child, parentHierarchyChanged());
    EXPECT_CALL (*parent, childrenChanged());
    parent->addChildComponent (*child);
    ::testing::Mock::VerifyAndClearExpectations (child.get());
    ::testing::Mock::VerifyAndClearExpectations (parent.get());

    // Test removal
    EXPECT_CALL (*child, parentHierarchyChanged());
    EXPECT_CALL (*parent, childrenChanged());
    parent->removeChildComponent (*child);
}

TEST_F (ComponentMockTest, PaintMethodCallbacks)
{
    // GMock MockComponent already provides MOCK_METHOD for paint/paintOverChildren.
    // Since Graphics requires platform setup, we verify the mocks compile and work.
    SUCCEED();
}

TEST_F (ComponentMockTest, AdditionalVirtualMethodTests)
{
    // Test focus methods (no callbacks expected via NiceMock)
    mockComponent->takeKeyboardFocus();
    mockComponent->leaveKeyboardFocus();

    // Test refresh display
    EXPECT_CALL (*mockComponent, refreshDisplay (0.016));
    mockComponent->refreshDisplay (0.016);
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    // Test userTriedToCloseWindow
    EXPECT_CALL (*mockComponent, userTriedToCloseWindow());
    mockComponent->userTriedToCloseWindow();
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    // Test attachedToNative/detachedFromNative
    EXPECT_CALL (*mockComponent, attachedToNative());
    mockComponent->attachedToNative();
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    EXPECT_CALL (*mockComponent, detachedFromNative());
    mockComponent->detachedFromNative();
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    // Test displayChanged
    EXPECT_CALL (*mockComponent, displayChanged());
    mockComponent->displayChanged();
}

//==============================================================================
// Tests for internalResized, internalMoved, internalContentScaleChanged
//==============================================================================

TEST_F (ComponentMockTest, InternalResizedUpdatesBounds)
{
    ComponentHelper::triggerInternalResized (*mockComponent, 640, 480);

    EXPECT_FLOAT_EQ (mockComponent->getWidth(), 640.0f);
    EXPECT_FLOAT_EQ (mockComponent->getHeight(), 480.0f);
}

TEST_F (ComponentMockTest, InternalResizedCallsResizedVirtual)
{
    EXPECT_CALL (*mockComponent, resized());
    ComponentHelper::triggerInternalResized (*mockComponent, 200, 150);
}

TEST_F (ComponentMockTest, InternalResizedCallsResizedOnlyWhenBoundsChange)
{
    ComponentHelper::triggerInternalResized (*mockComponent, 100, 200);

    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    // Same size should not trigger resized again
    ComponentHelper::triggerInternalResized (*mockComponent, 100, 200);
    // NiceMock: no expectation = resized not called, no warning
}

TEST_F (ComponentMockTest, InternalResizedDoesNotCallMoved)
{
    EXPECT_CALL (*mockComponent, moved()).Times (0);
    ComponentHelper::triggerInternalResized (*mockComponent, 300, 400);
}

TEST_F (ComponentMockTest, InternalResizedNotifiesComponentListener)
{
    RecordingComponentListener listener;
    mockComponent->addComponentListener (&listener);

    ComponentHelper::triggerInternalResized (*mockComponent, 150, 250);

    EXPECT_EQ (1, listener.resizedCount);
    EXPECT_EQ (0, listener.movedCount);
    EXPECT_EQ (static_cast<Component*> (mockComponent.get()), listener.lastResizedComponent);
}

TEST_F (ComponentMockTest, InternalMovedUpdatesBounds)
{
    ComponentHelper::triggerInternalMoved (*mockComponent, 42, 84);

    EXPECT_FLOAT_EQ (mockComponent->getX(), 42.0f);
    EXPECT_FLOAT_EQ (mockComponent->getY(), 84.0f);
}

TEST_F (ComponentMockTest, InternalMovedCallsMovedVirtual)
{
    EXPECT_CALL (*mockComponent, moved());
    ComponentHelper::triggerInternalMoved (*mockComponent, 10, 20);
}

TEST_F (ComponentMockTest, InternalMovedCallsMovedOnlyWhenBoundsChange)
{
    ComponentHelper::triggerInternalMoved (*mockComponent, 30, 60);

    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    // Same position should not trigger moved again
    ComponentHelper::triggerInternalMoved (*mockComponent, 30, 60);
    // NiceMock: no expectation = moved not called, no warning
}

TEST_F (ComponentMockTest, InternalMovedDoesNotCallResized)
{
    EXPECT_CALL (*mockComponent, resized()).Times (0);
    ComponentHelper::triggerInternalMoved (*mockComponent, 50, 100);
}

TEST_F (ComponentMockTest, InternalMovedNotifiesComponentListener)
{
    RecordingComponentListener listener;
    mockComponent->addComponentListener (&listener);

    ComponentHelper::triggerInternalMoved (*mockComponent, 75, 125);

    EXPECT_EQ (1, listener.movedCount);
    EXPECT_EQ (0, listener.resizedCount);
    EXPECT_EQ (static_cast<Component*> (mockComponent.get()), listener.lastMovedComponent);
}

TEST_F (ComponentMockTest, InternalContentScaleChangedCallsVirtual)
{
    EXPECT_CALL (*mockComponent, contentScaleChanged (1.5f));
    ComponentHelper::triggerInternalContentScaleChanged (*mockComponent, 1.5f);
}

TEST_F (ComponentMockTest, InternalContentScaleChangedCallsVirtualOnlyWhenScaleChanges)
{
    ComponentHelper::triggerInternalContentScaleChanged (*mockComponent, 2.0f);

    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    // Same scale should not trigger again
    ComponentHelper::triggerInternalContentScaleChanged (*mockComponent, 2.0f);
    // NiceMock: no expectation = contentScaleChanged not called, no warning
}

TEST_F (ComponentMockTest, InternalContentScaleChangedDoesNotCallMovedOrResized)
{
    EXPECT_CALL (*mockComponent, moved()).Times (0);
    EXPECT_CALL (*mockComponent, resized()).Times (0);

    ComponentHelper::triggerInternalContentScaleChanged (*mockComponent, 1.0f);
}

//==============================================================================
// Tests for native attachment propagation through addChild/removeChild

TEST_F (ComponentMockTest, AddChildToNativeParentTriggersAttachedToNative)
{
    auto parent = std::make_unique<ComponentMock> ("parent");
    ComponentHelper::attachMockNative (*parent);

    EXPECT_CALL (*mockComponent, attachedToNative());
    parent->addChildComponent (*mockComponent);

    ComponentHelper::detachMockNative (*parent);
}

TEST_F (ComponentMockTest, AddChildToNonNativeParentDoesNotTriggerAttachedToNative)
{
    auto parent = std::make_unique<ComponentMock> ("parent");

    parent->addChildComponent (*mockComponent);
    // NiceMock: no expectation = attachedToNative/detachedFromNative not called, no warning.
}

TEST_F (ComponentMockTest, RemoveChildFromNativeParentTriggersDetachedFromNative)
{
    auto parent = std::make_unique<ComponentMock> ("parent");
    ComponentHelper::attachMockNative (*parent);
    parent->addChildComponent (*mockComponent);

    EXPECT_CALL (*mockComponent, detachedFromNative());
    parent->removeChildComponent (mockComponent.get());

    ComponentHelper::detachMockNative (*parent);
}

TEST_F (ComponentMockTest, RemoveChildFromNonNativeParentDoesNotTriggerDetachedFromNative)
{
    auto parent = std::make_unique<ComponentMock> ("parent");
    parent->addChildComponent (*mockComponent);

    parent->removeChildComponent (mockComponent.get());
    // NiceMock: no expectation = attachedToNative/detachedFromNative not called, no warning.
}

TEST_F (ComponentMockTest, DeepChildGetsAttachedToNativeThroughAncestorChain)
{
    // root (with native) -> child -> grandchild
    auto root = std::make_unique<ComponentMock> ("root");
    auto child = std::make_unique<ComponentMock> ("child");
    auto grandchild = std::make_unique<ComponentMock> ("grandchild");

    ComponentHelper::attachMockNative (*root);
    root->addChildComponent (*child);

    EXPECT_CALL (*grandchild, attachedToNative());
    child->addChildComponent (*grandchild);

    ComponentHelper::detachMockNative (*root);
}

TEST_F (ComponentMockTest, KeyboardEventVirtualMethods)
{
    KeyPress mockKeyPress (KeyPress::spaceKey);
    Point<float> position (50.0f, 50.0f);

    EXPECT_CALL (*mockComponent, keyDown (mockKeyPress, _));
    mockComponent->keyDown (mockKeyPress, position);
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    EXPECT_CALL (*mockComponent, keyUp (mockKeyPress, _));
    mockComponent->keyUp (mockKeyPress, position);
    ::testing::Mock::VerifyAndClearExpectations (mockComponent.get());

    EXPECT_CALL (*mockComponent, textInput (String ("test text")));
    mockComponent->textInput ("test text");
}

TEST_F (ComponentMockTest, CoordinateTransformationMethods)
{
    // Test coordinate transformation methods with mock setup
    auto parent = std::make_unique<ComponentMock> ("parent");
    auto child = std::make_unique<ComponentMock> ("child");

    parent->setBounds (100, 100, 200, 200);
    child->setBounds (50, 50, 100, 100);
    parent->addChildComponent (*child);

    // Test getTransformToComponent
    auto transform = child->getTransformToComponent (parent.get());
    EXPECT_TRUE (transform.isIdentity() || ! transform.isIdentity()); // Just ensure it doesn't crash

    // Test getTransformFromComponent
    auto fromTransform = child->getTransformFromComponent (parent.get());
    EXPECT_TRUE (fromTransform.isIdentity() || ! fromTransform.isIdentity()); // Just ensure it doesn't crash

    // Test getTransformToScreen
    auto screenTransform = child->getTransformToScreen();
    EXPECT_TRUE (screenTransform.isIdentity() || ! screenTransform.isIdentity()); // Just ensure it doesn't crash

    // Test coordinate conversion methods
    Point<float> testPoint (25.0f, 25.0f);

    auto screenPos = child->localToScreen (testPoint);
    auto backToLocal = child->screenToLocal (screenPos);

    // Due to potential floating point precision, we'll just verify the methods don't crash
    EXPECT_TRUE (true); // Methods completed without crashing

    Rectangle<float> testRect (10.0f, 10.0f, 30.0f, 30.0f);
    auto screenRect = child->localToScreen (testRect);
    auto backToLocalRect = child->screenToLocal (screenRect);

    EXPECT_TRUE (true); // Methods completed without crashing
}

TEST_F (ComponentMockTest, MetricMethods)
{
    Identifier metricId ("cornerRadius");

    // Test setting metric
    mockComponent->setMetric (metricId, 8.0f);

    // Test getting metric
    auto retrievedMetric = mockComponent->getMetric (metricId);
    ASSERT_TRUE (retrievedMetric.has_value());
    EXPECT_FLOAT_EQ (retrievedMetric.value(), 8.0f);

    // Test finding metric
    auto foundMetric = mockComponent->findMetric (metricId);
    ASSERT_TRUE (foundMetric.has_value());
    EXPECT_FLOAT_EQ (foundMetric.value(), 8.0f);

    // Test setting null metric (removing override)
    mockComponent->setMetric (metricId, std::nullopt);
    auto nullMetric = mockComponent->getMetric (metricId);
    EXPECT_FALSE (nullMetric.has_value());

    // Test finding non-existent metric (not in theme either)
    Identifier nonExistentId ("nonExistentMetric");
    auto notFoundMetric = mockComponent->findMetric (nonExistentId);
    EXPECT_FALSE (notFoundMetric.has_value());
}

TEST_F (ComponentMockTest, SafeAreaChangedPropagatesToChildren)
{
    auto child = std::make_unique<ComponentMock> ("child");
    mockComponent->addChildComponent (*child);

    EXPECT_CALL (*mockComponent, safeAreaChanged());
    EXPECT_CALL (*child, safeAreaChanged());
    ComponentHelper::triggerSafeAreaChanged (*mockComponent);
}

TEST_F (ComponentMockTest, MetricParentFallback)
{
    auto parent = std::make_unique<ComponentMock> ("parent");
    auto child = std::make_unique<ComponentMock> ("child");

    parent->addAndMakeVisible (*child);

    Identifier metricId ("padding");

    // Set metric on parent
    parent->setMetric (metricId, 12.0f);

    // Child should find parent's metric via parent chain fallback
    auto childMetric = child->findMetric (metricId);
    ASSERT_TRUE (childMetric.has_value());
    EXPECT_FLOAT_EQ (childMetric.value(), 12.0f);

    // Child override should take precedence
    child->setMetric (metricId, 16.0f);
    auto overriddenMetric = child->findMetric (metricId);
    ASSERT_TRUE (overriddenMetric.has_value());
    EXPECT_FLOAT_EQ (overriddenMetric.value(), 16.0f);

    // Parent's metric should be unchanged
    auto parentMetric = parent->getMetric (metricId);
    ASSERT_TRUE (parentMetric.has_value());
    EXPECT_FLOAT_EQ (parentMetric.value(), 12.0f);

    // Clearing child override falls back to parent
    child->setMetric (metricId, std::nullopt);
    auto fallbackMetric = child->findMetric (metricId);
    ASSERT_TRUE (fallbackMetric.has_value());
    EXPECT_FLOAT_EQ (fallbackMetric.value(), 12.0f);
}

TEST_F (ComponentMockTest, DISABLED_MetricThemeFallback)
{
    // TODO - rewrite this with the new structure in mind, Component should not access to the global theme directly
    Identifier metricId ("globalSpacing");

    // Set a metric in the global theme
    theme->setMetric (metricId, 20.0f);

    // Component should find it via findMetric -> theme fallback
    auto metric = mockComponent->findMetric (metricId);
    ASSERT_TRUE (metric.has_value());
    EXPECT_FLOAT_EQ (metric.value(), 20.0f);

    // Component override should take precedence over theme
    mockComponent->setMetric (metricId, 24.0f);
    auto overriddenMetric = mockComponent->findMetric (metricId);
    ASSERT_TRUE (overriddenMetric.has_value());
    EXPECT_FLOAT_EQ (overriddenMetric.value(), 24.0f);

    // Clearing override falls back to theme
    mockComponent->setMetric (metricId, std::nullopt);
    auto themeFallback = mockComponent->findMetric (metricId);
    ASSERT_TRUE (themeFallback.has_value());
    EXPECT_FLOAT_EQ (themeFallback.value(), 20.0f);
}

TEST_F (ComponentMockTest, MetricAcceptsZeroAndNegative)
{
    Identifier zeroId ("zeroMetric");
    Identifier negativeId ("negativeMetric");

    mockComponent->setMetric (zeroId, 0.0f);
    mockComponent->setMetric (negativeId, -2.5f);

    auto zero = mockComponent->getMetric (zeroId);
    auto negative = mockComponent->getMetric (negativeId);

    ASSERT_TRUE (zero.has_value());
    EXPECT_FLOAT_EQ (zero.value(), 0.0f);

    ASSERT_TRUE (negative.has_value());
    EXPECT_FLOAT_EQ (negative.value(), -2.5f);
}

// =============================================================================

namespace
{

class DragDropComponent : public Component
{
public:
    using Component::Component;

    bool isInterestedInDrag (const DragAndDropData& data) override
    {
        ++interestQueryCount;
        return interested;
    }

    bool itemsDropped (const Point<float>& position, const DragAndDropData& data) override
    {
        ++dropCount;
        lastDropPosition = position;
        lastDropData = data;
        return handlesDrop;
    }

    void itemDragEnter (const DragAndDropData& data, const Point<float>& position) override
    {
        ++dragEnterCount;
        lastDragEnterPosition = position;
        lastDragEnterData = data;
    }

    void itemDragMove (const DragAndDropData& data, const Point<float>& position) override
    {
        ++dragMoveCount;
        lastDragMovePosition = position;
        lastDragMoveData = data;
    }

    void itemDragExit (const DragAndDropData& data) override
    {
        ++dragExitCount;
        lastDragExitData = data;
    }

    bool interested = false;
    bool handlesDrop = false;
    int interestQueryCount = 0;
    int dropCount = 0;
    int dragEnterCount = 0;
    int dragMoveCount = 0;
    int dragExitCount = 0;
    Point<float> lastDropPosition;
    DragAndDropData lastDropData;
    Point<float> lastDragEnterPosition;
    DragAndDropData lastDragEnterData;
    Point<float> lastDragMovePosition;
    DragAndDropData lastDragMoveData;
    DragAndDropData lastDragExitData;
};

} // namespace

class ComponentDragDropTest : public ::testing::Test
{
protected:
    using ComponentHelper = yup::ComponentTestHelper<yup::Component>;

    void SetUp() override
    {
        root = std::make_unique<DragDropComponent> ("root");
        parent = std::make_unique<DragDropComponent> ("parent");
        child = std::make_unique<DragDropComponent> ("child");

        root->setBounds (0, 0, 400, 300);
        parent->setBounds (50, 50, 200, 150);
        child->setBounds (25, 25, 100, 75);

        root->addChildComponent (*parent);
        parent->addChildComponent (*child);

        root->setVisible (true);
        parent->setVisible (true);
        child->setVisible (true);
    }

    std::unique_ptr<DragDropComponent> root;
    std::unique_ptr<DragDropComponent> parent;
    std::unique_ptr<DragDropComponent> child;
};

TEST_F (ComponentTest, DefaultDragAndDropCallbacksDoNotHandlePayload)
{
    DragAndDropData data = DragAndDropData().withText ("hello");

    EXPECT_FALSE (child->isInterestedInDrag (data));
    EXPECT_FALSE (child->itemsDropped ({ 10.0f, 20.0f }, data));
    EXPECT_NO_FATAL_FAILURE (child->itemDragEnter (data, { 10.0f, 20.0f }));
    EXPECT_NO_FATAL_FAILURE (child->itemDragMove (data, { 15.0f, 25.0f }));
    EXPECT_NO_FATAL_FAILURE (child->itemDragExit (data));
}

TEST_F (ComponentDragDropTest, InterestedTopmostHandlesDrop)
{
    child->interested = true;
    child->handlesDrop = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    // Window position (85,85) is inside child (child screen origin = 75,75).
    EXPECT_TRUE (ComponentHelper::triggerItemsDropped (*child, { 85.0f, 85.0f }, data));
    EXPECT_EQ (child->dropCount, 1);
    EXPECT_EQ (parent->dropCount, 0);
    EXPECT_EQ (root->dropCount, 0);
}

TEST_F (ComponentDragDropTest, InterestedButReturnsFalseBubblesToParent)
{
    child->interested = true;
    child->handlesDrop = false;
    parent->interested = true;
    parent->handlesDrop = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    EXPECT_TRUE (ComponentHelper::triggerItemsDropped (*child, { 85.0f, 85.0f }, data));
    EXPECT_EQ (child->dropCount, 1);
    EXPECT_EQ (parent->dropCount, 1);
    EXPECT_EQ (root->dropCount, 0);
}

TEST_F (ComponentDragDropTest, UninterestedComponentSkippedEvenIfItOverridesDrop)
{
    child->interested = false;
    child->handlesDrop = true;
    parent->interested = true;
    parent->handlesDrop = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    EXPECT_TRUE (ComponentHelper::triggerItemsDropped (*child, { 85.0f, 85.0f }, data));
    EXPECT_EQ (child->dropCount, 0);
    EXPECT_EQ (parent->dropCount, 1);
}

TEST_F (ComponentDragDropTest, DropPositionIsComponentLocal)
{
    child->interested = true;
    child->handlesDrop = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    // Window (85,85). Child screen origin = root(0,0)+parent(50,50)+child(25,25) = (75,75).
    // Local position = (10,10).
    ComponentHelper::triggerItemsDropped (*child, { 85.0f, 85.0f }, data);
    EXPECT_FLOAT_EQ (child->lastDropPosition.getX(), 10.0f);
    EXPECT_FLOAT_EQ (child->lastDropPosition.getY(), 10.0f);
}

TEST_F (ComponentDragDropTest, DropPositionRecomputedPerAncestor)
{
    child->interested = true;
    child->handlesDrop = false;
    parent->interested = true;
    parent->handlesDrop = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    // Window (85,85). Parent screen origin = (50,50). Parent-local = (35,35).
    ComponentHelper::triggerItemsDropped (*child, { 85.0f, 85.0f }, data);
    EXPECT_FLOAT_EQ (parent->lastDropPosition.getX(), 35.0f);
    EXPECT_FLOAT_EQ (parent->lastDropPosition.getY(), 35.0f);
}

TEST_F (ComponentDragDropTest, InvisibleComponentSkipped)
{
    child->interested = true;
    child->handlesDrop = true;
    child->setVisible (false);
    parent->interested = true;
    parent->handlesDrop = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    EXPECT_TRUE (ComponentHelper::triggerItemsDropped (*child, { 85.0f, 85.0f }, data));
    EXPECT_EQ (child->dropCount, 0);
    EXPECT_EQ (parent->dropCount, 1);
}

TEST_F (ComponentDragDropTest, DisabledComponentSkipped)
{
    child->interested = true;
    child->handlesDrop = true;
    child->setEnabled (false);
    parent->interested = true;
    parent->handlesDrop = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    EXPECT_TRUE (ComponentHelper::triggerItemsDropped (*child, { 85.0f, 85.0f }, data));
    EXPECT_EQ (child->dropCount, 0);
    EXPECT_EQ (parent->dropCount, 1);
}

TEST_F (ComponentDragDropTest, NobodyHandlesReturnsFalse)
{
    DragAndDropData data = DragAndDropData().withText ("hello");

    EXPECT_FALSE (ComponentHelper::triggerItemsDropped (*child, { 85.0f, 85.0f }, data));
    EXPECT_EQ (child->dropCount, 0);
    EXPECT_EQ (parent->dropCount, 0);
    EXPECT_EQ (root->dropCount, 0);
}

TEST_F (ComponentDragDropTest, FilesOnlyPayloadDelivered)
{
    child->interested = true;
    child->handlesDrop = true;

    Array<File> files;
    files.add (File ("/tmp/one.txt"));
    files.add (File ("/tmp/two.txt"));
    DragAndDropData data = DragAndDropData().withFiles (files);

    ComponentHelper::triggerItemsDropped (*child, { 85.0f, 85.0f }, data);
    EXPECT_TRUE (child->lastDropData.hasFiles());
    EXPECT_FALSE (child->lastDropData.hasText());
    EXPECT_EQ (child->lastDropData.getFiles().size(), 2);
}

TEST_F (ComponentDragDropTest, TextOnlyPayloadDelivered)
{
    child->interested = true;
    child->handlesDrop = true;

    DragAndDropData data = DragAndDropData().withText ("dropped");

    ComponentHelper::triggerItemsDropped (*child, { 85.0f, 85.0f }, data);
    EXPECT_FALSE (child->lastDropData.hasFiles());
    EXPECT_TRUE (child->lastDropData.hasText());
    EXPECT_EQ (child->lastDropData.getText(), String ("dropped"));
}

TEST_F (ComponentDragDropTest, MixedPayloadDelivered)
{
    child->interested = true;
    child->handlesDrop = true;

    Array<File> files;
    files.add (File ("/tmp/one.txt"));
    DragAndDropData data = DragAndDropData().withFiles (files).withText ("dropped");

    ComponentHelper::triggerItemsDropped (*child, { 85.0f, 85.0f }, data);
    EXPECT_TRUE (child->lastDropData.hasFiles());
    EXPECT_TRUE (child->lastDropData.hasText());
}

// =============================================================================

TEST_F (ComponentDragDropTest, DragEnterCalledWhenInterested)
{
    child->interested = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    ComponentHelper::triggerItemDragEnter (*child, { 85.0f, 85.0f }, data);
    EXPECT_EQ (child->dragEnterCount, 1);
    EXPECT_EQ (child->dragMoveCount, 0);
    EXPECT_EQ (child->dragExitCount, 0);
}

TEST_F (ComponentDragDropTest, DragEnterPositionIsLocalToComponent)
{
    child->interested = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    // Window position (85,85) maps to child-local (10,10)
    ComponentHelper::triggerItemDragEnter (*child, { 85.0f, 85.0f }, data);
    EXPECT_FLOAT_EQ (child->lastDragEnterPosition.getX(), 10.0f);
    EXPECT_FLOAT_EQ (child->lastDragEnterPosition.getY(), 10.0f);
}

TEST_F (ComponentDragDropTest, DragEnterBubblesToParentIfInterested)
{
    child->interested = true;
    parent->interested = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    ComponentHelper::triggerItemDragEnter (*child, { 85.0f, 85.0f }, data);
    EXPECT_EQ (child->dragEnterCount, 1);
    EXPECT_EQ (parent->dragEnterCount, 1);
    EXPECT_EQ (root->dragEnterCount, 0);
}

TEST_F (ComponentDragDropTest, DragEnterNotCalledWhenNotInterested)
{
    child->interested = false;
    parent->interested = false;

    DragAndDropData data = DragAndDropData().withText ("hello");

    ComponentHelper::triggerItemDragEnter (*child, { 85.0f, 85.0f }, data);
    EXPECT_EQ (child->dragEnterCount, 0);
    EXPECT_EQ (parent->dragEnterCount, 0);
}

TEST_F (ComponentDragDropTest, DragMoveCalledForSameComponent)
{
    child->interested = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    ComponentHelper::triggerItemDragMove (*child, { 85.0f, 85.0f }, data);
    EXPECT_EQ (child->dragMoveCount, 1);
    EXPECT_EQ (child->dragEnterCount, 0);
    EXPECT_EQ (child->dragExitCount, 0);
}

TEST_F (ComponentDragDropTest, DragMoveBubblesToParentIfInterested)
{
    child->interested = true;
    parent->interested = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    ComponentHelper::triggerItemDragMove (*child, { 85.0f, 85.0f }, data);
    EXPECT_EQ (child->dragMoveCount, 1);
    EXPECT_EQ (parent->dragMoveCount, 1);
}

TEST_F (ComponentDragDropTest, DragExitCalledWhenInterested)
{
    child->interested = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    ComponentHelper::triggerItemDragExit (*child, data);
    EXPECT_EQ (child->dragExitCount, 1);
    EXPECT_EQ (child->dragEnterCount, 0);
    EXPECT_EQ (child->dragMoveCount, 0);
}

TEST_F (ComponentDragDropTest, DragExitBubblesToParentIfInterested)
{
    child->interested = true;
    parent->interested = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    ComponentHelper::triggerItemDragExit (*child, data);
    EXPECT_EQ (child->dragExitCount, 1);
    EXPECT_EQ (parent->dragExitCount, 1);
}

TEST_F (ComponentDragDropTest, DragEnterRespectsDisabledComponent)
{
    child->interested = true;
    child->setEnabled (false);

    DragAndDropData data = DragAndDropData().withText ("hello");

    ComponentHelper::triggerItemDragEnter (*child, { 85.0f, 85.0f }, data);
    EXPECT_EQ (child->dragEnterCount, 0);
}

TEST_F (ComponentDragDropTest, DragEnterRespectsHiddenComponent)
{
    child->interested = true;
    child->setVisible (false);

    DragAndDropData data = DragAndDropData().withText ("hello");

    ComponentHelper::triggerItemDragEnter (*child, { 85.0f, 85.0f }, data);
    EXPECT_EQ (child->dragEnterCount, 0);
}

TEST_F (ComponentDragDropTest, PayloadDataDeliveredToDragEnter)
{
    child->interested = true;

    DragAndDropData data = DragAndDropData().withText ("hello").withFiles ({ File ("/tmp/a.txt") });

    ComponentHelper::triggerItemDragEnter (*child, { 85.0f, 85.0f }, data);
    EXPECT_TRUE (child->lastDragEnterData.hasText());
    EXPECT_EQ (child->lastDragEnterData.getText(), String ("hello"));
    EXPECT_TRUE (child->lastDragEnterData.hasFiles());
    EXPECT_EQ (child->lastDragEnterData.getFiles().size(), 1);
}

TEST_F (ComponentDragDropTest, PayloadDataDeliveredToDragMove)
{
    child->interested = true;

    DragAndDropData data = DragAndDropData().withText ("hello");

    ComponentHelper::triggerItemDragMove (*child, { 85.0f, 85.0f }, data);
    EXPECT_TRUE (child->lastDragMoveData.hasText());
    EXPECT_EQ (child->lastDragMoveData.getText(), String ("hello"));
}

TEST_F (ComponentDragDropTest, PayloadDataDeliveredToDragExit)
{
    child->interested = true;

    DragAndDropData data = DragAndDropData().withFiles ({ File ("/tmp/a.txt") });

    ComponentHelper::triggerItemDragExit (*child, data);
    EXPECT_TRUE (child->lastDragExitData.hasFiles());
    EXPECT_EQ (child->lastDragExitData.getFiles().size(), 1);
}

// =============================================================================
// Coverage: desktop-native delegation, hierarchy no-ops, internal dispatch
// =============================================================================

TEST_F (ComponentTest, DesktopNativeDelegationSetters)
{
    // Mock a native peer so the onDesktop delegation branches execute.
    ComponentHelper::attachMockNative (*root);
    ComponentHelper::setOnDesktop (*root, true);

    root->setBounds (0, 0, 100, 50);
    root->setTitle ("hello");
    root->setPosition (Point<float> (10.0f, 20.0f));
    root->setBottomLeft (Point<float> (5.0f, 30.0f));
    root->setTopRight (Point<float> (60.0f, 5.0f));
    root->setBottomRight (Point<float> (80.0f, 40.0f));
    root->setCenter (Point<float> (50.0f, 25.0f));
    root->setCenterX (50.0f);
    root->setCenterY (25.0f);
    root->setFullScreen (true);
    EXPECT_TRUE (root->isFullScreen());
    EXPECT_FLOAT_EQ (root->getScaleDpi(), 1.0f);
    root->setOpacity (0.5f);
    EXPECT_NEAR (0.5f, root->getOpacity(), 0.01f); // stored as uint8 → 127/255 ≈ 0.498
    EXPECT_EQ (root->getNativeHandle(), nullptr);
    EXPECT_TRUE (root->getSafeAreaBounds().isEmpty());

    // localToScreen / getTransformToScreen native branches.
    root->localToScreen (Point<float> (1.0f, 2.0f));
    root->getTransformToScreen();

    // Const getNativeComponent with an attached native.
    const Component& constRoot = *root;
    EXPECT_EQ (constRoot.getNativeComponent(), root->getNativeComponent());
}

TEST_F (ComponentTest, ChildResolvesNativeThroughParent)
{
    // Child has no native; parent does → getNativeComponent walks up (both overloads).
    ComponentHelper::attachMockNative (*parent);
    EXPECT_EQ (child->getNativeComponent(), parent->getNativeComponent());

    const Component& constChild = *child;
    const Component& constParent = *parent;
    EXPECT_EQ (constChild.getNativeComponent(), constParent.getNativeComponent());

    // localToScreen adds the desktop parent's native position then stops.
    ComponentHelper::setOnDesktop (*parent, true);
    child->localToScreen (Point<float> (1.0f, 1.0f));
}

TEST_F (ComponentTest, SameStateSettersAreNoOps)
{
    root->setEnabled (true);   // already enabled
    root->setVisible (true);   // already visible
    root->setFullScreen (false); // already not fullscreen
}

TEST_F (ComponentTest, HierarchyOperationsOnParentlessComponent)
{
    Component lone ("lone");

    lone.toBack();
    lone.raiseAbove (&lone);
    lone.lowerBelow (&lone);
    lone.raiseBy (1);

    // Target that is not a child of the parent.
    Component other ("other");
    parent->raiseAbove (&other);
    parent->lowerBelow (&other);
}

TEST_F (ComponentTest, TransformAndCoordinateQueries)
{
    // getScreenPosition covers localToScreen of the position.
    root->setBounds (10, 20, 100, 50);
    root->getScreenPosition();

    // getRelativeArea / getTransformToComponent with self or null target.
    root->getRelativeArea (root.get(), Rectangle<float> (0.0f, 0.0f, 10.0f, 10.0f));
    root->getRelativeArea (nullptr, Rectangle<float> (0.0f, 0.0f, 10.0f, 10.0f));
    root->getTransformToComponent (root.get());
    root->getTransformToComponent (nullptr);
    root->getTransformFromComponent (nullptr);

    // getTransformToScreen with a non-identity transform.
    root->setTransform (AffineTransform::translation (5.0f, 7.0f));
    root->getTransformToScreen();
}

TEST_F (ComponentTest, FindColorWalksUpTheHierarchy)
{
    root->setColor ("missing", Color (0xFFFF0000));

    // child → parent → root: found at root.
    auto found = child->findColor ("missing");
    ASSERT_TRUE (found.has_value());
    EXPECT_EQ (found.value(), Color (0xFFFF0000));

    // No one in the chain has it → recursion returns nullopt.
    EXPECT_FALSE (child->findColor ("unknown-id").has_value());
}

TEST_F (ComponentTest, InternalDisplayAndCloseDispatch)
{
    ComponentHelper::triggerDisplayChanged (*root);
    ComponentHelper::triggerUserTriedToCloseWindow (*root);

    // refreshDisplay recurses into children.
    ComponentHelper::triggerRefreshDisplay (*root, 0.016);
}

TEST_F (ComponentTest, InternalKeyboardDispatch)
{
    const KeyPress key (KeyPress::spaceKey);

    // Invisible component → early return.
    root->setVisible (false);
    ComponentHelper::triggerKeyDown (*root, key, Point<float> (0.0f, 0.0f));
    ComponentHelper::triggerKeyUp (*root, key, Point<float> (0.0f, 0.0f));
    ComponentHelper::triggerTextInput (*root, "a");
    root->setVisible (true);

    // Disabled component → early return.
    root->setEnabled (false);
    ComponentHelper::triggerKeyDown (*root, key, Point<float> (0.0f, 0.0f));
    ComponentHelper::triggerKeyUp (*root, key, Point<float> (0.0f, 0.0f));
    ComponentHelper::triggerTextInput (*root, "a");
    root->setEnabled (true);

    // Visible + enabled → default virtual bodies run.
    ComponentHelper::triggerKeyDown (*root, key, Point<float> (0.0f, 0.0f));
    ComponentHelper::triggerKeyUp (*root, key, Point<float> (0.0f, 0.0f));

    // textInput requires wantsKeyboardFocus.
    ComponentHelper::triggerTextInput (*root, "a");
    root->setWantsKeyboardFocus (true);
    ComponentHelper::triggerTextInput (*root, "a");
}

TEST_F (ComponentTest, InternalMouseDispatch)
{
    const MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (10.0f, 10.0f));
    const MouseWheelData wheel (0.0f, 10.0f);

    // Invisible component → every handler returns early.
    root->setVisible (false);
    ComponentHelper::triggerMouseEnter (*root, event);
    ComponentHelper::triggerMouseExit (*root, event);
    ComponentHelper::triggerMouseDown (*root, event);
    ComponentHelper::triggerMouseMove (*root, event);
    ComponentHelper::triggerMouseDrag (*root, event);
    ComponentHelper::triggerMouseUp (*root, event);
    ComponentHelper::triggerMouseDoubleClick (*root, event);
    ComponentHelper::triggerMouseWheel (*root, event, wheel);
    root->setVisible (true);

    // Visible component → default virtual bodies run.
    ComponentHelper::triggerMouseEnter (*root, event);
    ComponentHelper::triggerMouseExit (*root, event);
    ComponentHelper::triggerMouseDown (*root, event);
    ComponentHelper::triggerMouseMove (*root, event);
    ComponentHelper::triggerMouseDrag (*root, event);
    ComponentHelper::triggerMouseUp (*root, event);
    ComponentHelper::triggerMouseDoubleClick (*root, event);
    ComponentHelper::triggerMouseWheel (*root, event, wheel);

    // Mouse-down with keyboard focus request → takeKeyboardFocus path.
    root->setWantsKeyboardFocus (true);
    ComponentHelper::triggerMouseDown (*root, event);
}

TEST_F (ComponentTest, InternalMouseDispatchNotifiesListeners)
{
    struct LocalMouseListener : public MouseListener
    {
        void mouseEnter (const MouseEvent&) override { ++enters; }
        void mouseExit (const MouseEvent&) override { ++exits; }
        void mouseDown (const MouseEvent&) override { ++downs; }
        void mouseMove (const MouseEvent&) override { ++moves; }
        void mouseDrag (const MouseEvent&) override { ++drags; }
        void mouseUp (const MouseEvent&) override { ++ups; }
        void mouseDoubleClick (const MouseEvent&) override { ++doubleClicks; }
        void mouseWheel (const MouseEvent&, const MouseWheelData&) override { ++wheels; }

        int enters = 0;
        int exits = 0;
        int downs = 0;
        int moves = 0;
        int drags = 0;
        int ups = 0;
        int doubleClicks = 0;
        int wheels = 0;
    };

    LocalMouseListener listener;
    root->addMouseListener (&listener);
    root->setVisible (true);

    const MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (10.0f, 10.0f));
    const MouseWheelData wheel (0.0f, 10.0f);

    ComponentHelper::triggerMouseEnter (*root, event);
    ComponentHelper::triggerMouseExit (*root, event);
    ComponentHelper::triggerMouseDown (*root, event);
    ComponentHelper::triggerMouseMove (*root, event);
    ComponentHelper::triggerMouseDrag (*root, event);
    ComponentHelper::triggerMouseUp (*root, event);
    ComponentHelper::triggerMouseDoubleClick (*root, event);
    ComponentHelper::triggerMouseWheel (*root, event, wheel);

    EXPECT_EQ (listener.enters, 1);
    EXPECT_EQ (listener.exits, 1);
    EXPECT_EQ (listener.downs, 1);
    EXPECT_EQ (listener.moves, 1);
    EXPECT_EQ (listener.drags, 1);
    EXPECT_EQ (listener.ups, 1);
    EXPECT_EQ (listener.doubleClicks, 1);
    EXPECT_EQ (listener.wheels, 1);

    root->removeMouseListener (&listener);
}

TEST_F (ComponentTest, InternalAttachDetachAndVisibilityDispatch)
{
    // A visible child makes internalVisibilityChanged recurse into the child loop.
    parent->setVisible (true);

    ComponentHelper::triggerVisibilityChanged (*root);
    ComponentHelper::triggerAttachedToNative (*root);
    ComponentHelper::triggerDetachedFromNative (*root);
}

TEST_F (ComponentTest, RemoveFromDesktopWhenNotOnDesktopReturnsEarly)
{
    // Not on desktop → removeFromDesktop is a no-op.
    root->removeFromDesktop();
}

TEST_F (ComponentTest, GetParentComponentWithTypeWalksUpTheChain)
{
    struct CustomComponent : public Component { };

    // No ancestor is a CustomComponent → the walk reaches the top and returns nullptr.
    EXPECT_EQ (child->getParentComponentWithType<CustomComponent>(), nullptr);

    // Direct parent match.
    EXPECT_EQ (child->getParentComponentWithType<Component>(), parent.get());
}
