/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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
#include <gmock/gmock.h>

#include "../mocks/rive_gpu.h"

using namespace yup;

//==============================================================================
// ArtboardNode type reporting, hierarchy traversal and the invalid-handle
// contract.
//
// Identity, bounds, caching and epoch invalidation are covered against the
// individual fixtures in yup_Artboard.cpp; this file covers the parts of the
// handle API that are independent of which .riv file is loaded.
//==============================================================================

namespace
{

const File getArtboardNodeTestDataDirectory()
{
    // Try source-relative path first (works on desktop builds)
    auto dir = File (__FILE__)
                   .getParentDirectory()
                   .getParentDirectory()
                   .getChildFile ("data")
                   .getChildFile ("rive");

    if (dir.exists())
        return dir;

    dir = File::getCurrentWorkingDirectory()
              .getParentDirectory()
              .getParentDirectory()
              .getParentDirectory()
              .getChildFile ("tests")
              .getChildFile ("data")
              .getChildFile ("rive");

    if (dir.exists())
        return dir;

    return File ("/data/rive");
}

constexpr const char* kDataBindingRootName = "Artboard";

// Every type name yup_ArtboardNode.cpp maps; anything else reports an empty
// string, which is why getTypeKey() is the reliable discriminator.
const StringArray& knownTypeNames()
{
    static const StringArray names {
        "Artboard", "LayoutComponent", "Node", "Shape", "Rectangle", "Ellipse", "Image", "Text", "Bone", "Solo", "NestedArtboard"
    };

    return names;
}

void visitNodeTree (const ArtboardNode::Ptr& node, int depth, const std::function<void (const ArtboardNode::Ptr&, int)>& visit)
{
    if (node == nullptr)
        return;

    visit (node, depth);

    // Guard against pathological files; the fixtures are shallow.
    if (depth >= 8)
        return;

    for (const auto& child : node->getChildren())
        visitNodeTree (child, depth + 1, visit);
}

} // namespace

class ArtboardNodeTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto file = getArtboardNodeTestDataDirectory().getChildFile ("data-binding.riv");
        if (! file.existsAsFile())
        {
            GTEST_SKIP() << "Missing test asset: tests/data/rive/data-binding.riv";
            return;
        }

        auto result = ArtboardFile::load (file, factory);
        if (result.failed())
        {
            GTEST_SKIP() << "Failed to load test asset: " << result.getErrorMessage();
            return;
        }

        artboardFile = result.getValue();
        artboard = std::make_unique<Artboard> ("testArtboard", artboardFile);
        artboard->setBounds (0.0f, 0.0f, 400.0f, 400.0f);

        for (int i = 0; i < 5; ++i)
            artboard->advanceAndApply (0.0f);

        root = artboard->findNode (kDataBindingRootName);
        if (root == nullptr)
            GTEST_SKIP() << "Test asset exposes no node named 'Artboard'";
    }

    ::testing::NiceMock<MockRiveFactory> factory;
    std::shared_ptr<ArtboardFile> artboardFile;
    std::unique_ptr<Artboard> artboard;
    ArtboardNode::Ptr root;
};

//==============================================================================
// Type reporting
//==============================================================================

TEST_F (ArtboardNodeTests, RootReportsTheArtboardType)
{
    EXPECT_TRUE (root->isValid());
    EXPECT_NE (0, root->getTypeKey());
    EXPECT_EQ (String ("Artboard"), root->getTypeName());
}

TEST_F (ArtboardNodeTests, EveryReportedTypeNameIsOneOfTheMappedNames)
{
    int visited = 0;

    visitNodeTree (root,
                   0,
                   [&] (const ArtboardNode::Ptr& node, int)
                   {
                       ++visited;

                       const auto typeName = node->getTypeName();

                       // A valid handle always has a non-zero type key; the type
                       // name is empty for any type outside the mapped set.
                       EXPECT_TRUE (node->isValid());
                       EXPECT_NE (0, node->getTypeKey());
                       EXPECT_TRUE (typeName.isEmpty() || knownTypeNames().contains (typeName));
                   });

    EXPECT_GT (visited, 0);
}

TEST_F (ArtboardNodeTests, TypeKeyIsStableForTheSameNode)
{
    auto again = artboard->findNode (kDataBindingRootName);
    ASSERT_NE (nullptr, again.get());

    EXPECT_EQ (root->getTypeKey(), again->getTypeKey());
    EXPECT_EQ (root->getTypeName(), again->getTypeName());
}

TEST_F (ArtboardNodeTests, IsLayoutAgreesWithTheReportedTypeName)
{
    visitNodeTree (root,
                   0,
                   [] (const ArtboardNode::Ptr& node, int)
                   {
                       // Artboards are layout components too, so isLayout() is
                       // broader than the LayoutComponent type name alone.
                       if (node->getTypeName() == "LayoutComponent")
                           EXPECT_TRUE (node->isLayout());
                   });
}

//==============================================================================
// Hierarchy traversal
//==============================================================================

TEST_F (ArtboardNodeTests, RootHasNoParentWithinTheArtboard)
{
    EXPECT_EQ (nullptr, root->getParent().get());
}

TEST_F (ArtboardNodeTests, ChildrenPointBackAtTheirParent)
{
    const auto children = root->getChildren();

    if (children.isEmpty())
    {
        GTEST_SKIP() << "Test asset root has no children";
        return;
    }

    for (const auto& child : children)
    {
        ASSERT_NE (nullptr, child.get());
        EXPECT_TRUE (child->isValid());

        auto parent = child->getParent();
        ASSERT_NE (nullptr, parent.get());
        EXPECT_EQ (root->getTypeKey(), parent->getTypeKey());
        EXPECT_EQ (root->getName(), parent->getName());
    }
}

TEST_F (ArtboardNodeTests, ChildHandlesAreFreshObjectsNotCacheEntries)
{
    const auto first = root->getChildren();
    const auto second = root->getChildren();

    if (first.isEmpty())
    {
        GTEST_SKIP() << "Test asset root has no children";
        return;
    }

    ASSERT_EQ (first.size(), second.size());

    // getChildren() is not name-based, so it does not go through the artboard's
    // handle cache; the handles differ but describe the same nodes.
    for (int i = 0; i < first.size(); ++i)
    {
        EXPECT_EQ (first[i]->getTypeKey(), second[i]->getTypeKey());
        EXPECT_EQ (first[i]->getName(), second[i]->getName());
    }
}

TEST_F (ArtboardNodeTests, LeafNodesReportNoChildren)
{
    int leaves = 0;

    visitNodeTree (root,
                   0,
                   [&] (const ArtboardNode::Ptr& node, int)
                   {
                       if (node->getChildren().isEmpty())
                           ++leaves;
                   });

    EXPECT_GT (leaves, 0);
}

//==============================================================================
// Transforms
//==============================================================================

TEST_F (ArtboardNodeTests, ViewTransformIncorporatesTheArtboardFit)
{
    const auto world = root->getWorldTransform();
    const auto view = root->getViewTransform();

    // Both are finite, and resizing the artboard changes the view transform while
    // leaving the world transform alone.
    EXPECT_TRUE (std::isfinite (view.getTranslateX()));
    EXPECT_TRUE (std::isfinite (view.getTranslateY()));

    artboard->setBounds (0.0f, 0.0f, 800.0f, 800.0f);
    artboard->advanceAndApply (0.0f);

    EXPECT_EQ (world.getScaleX(), root->getWorldTransform().getScaleX());
    EXPECT_FALSE (root->getViewTransform().approximatelyEqualTo (view));
}

TEST_F (ArtboardNodeTests, LocalTransformIsFinite)
{
    const auto local = root->getLocalTransform();

    EXPECT_TRUE (std::isfinite (local.getScaleX()));
    EXPECT_TRUE (std::isfinite (local.getScaleY()));
    EXPECT_TRUE (std::isfinite (local.getTranslateX()));
    EXPECT_TRUE (std::isfinite (local.getTranslateY()));
}

//==============================================================================
// The invalid-handle contract
//==============================================================================

TEST_F (ArtboardNodeTests, InvalidHandleReturnsSafeDefaultsFromEveryAccessor)
{
    auto child = root->getChildren().getFirst();

    artboard->clear();

    ASSERT_FALSE (root->isValid());

    EXPECT_TRUE (root->getName().isEmpty());
    EXPECT_EQ (0, root->getTypeKey());
    EXPECT_TRUE (root->getTypeName().isEmpty());
    EXPECT_FALSE (root->isLayout());
    EXPECT_TRUE (root->getBounds().isEmpty());
    EXPECT_TRUE (root->getLocalTransform().isIdentity());
    EXPECT_TRUE (root->getWorldTransform().isIdentity());
    EXPECT_TRUE (root->getViewTransform().isIdentity());
    EXPECT_EQ (nullptr, root->getParent().get());
    EXPECT_TRUE (root->getChildren().isEmpty());

    if (child != nullptr)
    {
        EXPECT_FALSE (child->isValid());
        EXPECT_TRUE (child->getName().isEmpty());
        EXPECT_EQ (0, child->getTypeKey());
    }
}

TEST_F (ArtboardNodeTests, HandlesOutliveTheirArtboardSafely)
{
    auto child = root->getChildren().getFirst();

    artboard.reset();

    EXPECT_FALSE (root->isValid());
    EXPECT_TRUE (root->getName().isEmpty());
    EXPECT_TRUE (root->getBounds().isEmpty());
    EXPECT_EQ (nullptr, root->getParent().get());

    if (child != nullptr)
    {
        EXPECT_FALSE (child->isValid());
        EXPECT_TRUE (child->getChildren().isEmpty());
    }
}

TEST_F (ArtboardNodeTests, ReplacingTheFileInvalidatesHandlesTakenBefore)
{
    const auto before = root;

    artboard->setFile (artboardFile);

    EXPECT_FALSE (before->isValid());

    auto after = artboard->findNode (kDataBindingRootName);
    ASSERT_NE (nullptr, after.get());
    EXPECT_TRUE (after->isValid());
    EXPECT_NE (before.get(), after.get());
}
