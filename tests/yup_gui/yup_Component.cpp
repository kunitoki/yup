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

using namespace yup;

// =============================================================================

class ComponentTest : public ::testing::Test
{
protected:
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

/*
TEST_F (ComponentTest, KeyboardFocus)
{
    // Test default focus behavior
    EXPECT_FALSE (child->getWantsKeyboardFocus());

    child->setWantsKeyboardFocus (true);
    EXPECT_TRUE (child->getWantsKeyboardFocus());

    child->setWantsKeyboardFocus (false);
    EXPECT_FALSE (child->getWantsKeyboardFocus());
}
*/

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
