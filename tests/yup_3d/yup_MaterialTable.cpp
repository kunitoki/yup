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

#include <yup_3d/yup_3d.h>

#include <gtest/gtest.h>

#include <vector>

using namespace yup;

class MaterialTableTests : public ::testing::Test
{
};

TEST_F (MaterialTableTests, AssignsStableIdsWithDefaultAtZero)
{
    Material a, b;
    a.ior = 1.33f;

    MaterialTable table;
    table.clear();

    EXPECT_EQ (table.getId (a), 1);
    EXPECT_EQ (table.getId (b), 2);
    EXPECT_EQ (table.getId (a), 1);

    ASSERT_EQ (table.getEntries().size(), 3u);
    EXPECT_FLOAT_EQ (table.getEntries()[0].extended[0], 1.5f);
    EXPECT_FLOAT_EQ (table.getEntries()[1].extended[0], 1.33f);
}

TEST_F (MaterialTableTests, NewTableHoldsTheDefaultMaterial)
{
    const MaterialTable table;
    const auto expected = GpuSceneData::packMaterial (Material());

    ASSERT_EQ (table.getEntries().size(), 1u);

    for (int i = 0; i < 4; ++i)
    {
        EXPECT_FLOAT_EQ (table.getEntries()[0].specular[i], expected.specular[i]);
        EXPECT_FLOAT_EQ (table.getEntries()[0].extended[i], expected.extended[i]);
    }
}

TEST_F (MaterialTableTests, EntriesArePackedByGpuSceneData)
{
    Material material;
    material.specularFactor = 0.25f;
    material.specularColorFactor = { 0.5f, 0.6f, 0.7f };
    material.emissiveStrength = 4.0f;

    MaterialTable table;
    table.clear();

    const auto id = table.getId (material);
    ASSERT_EQ (id, 1);

    const auto expected = GpuSceneData::packMaterial (material);
    const auto& entry = table.getEntries()[static_cast<size_t> (id)];

    for (int i = 0; i < 4; ++i)
    {
        EXPECT_FLOAT_EQ (entry.specular[i], expected.specular[i]);
        EXPECT_FLOAT_EQ (entry.extended[i], expected.extended[i]);
    }
}

TEST_F (MaterialTableTests, ClearStartsTheNextFrameOver)
{
    Material a, b;

    MaterialTable table;
    table.clear();
    EXPECT_EQ (table.getId (a), 1);

    table.clear();
    EXPECT_EQ (table.getEntries().size(), 1u);
    EXPECT_EQ (table.getId (b), 1);
    EXPECT_EQ (table.getId (a), 2);
}

TEST_F (MaterialTableTests, MaterialTableOverflowAliasesDefault)
{
    std::vector<Material> materials (MaterialTable::maxMaterials + 10);

    MaterialTable table;
    table.clear();

    int last = 0;
    for (auto& material : materials)
        last = table.getId (material);

    EXPECT_EQ (last, 0);
    EXPECT_EQ (static_cast<int> (table.getEntries().size()), MaterialTable::maxMaterials);

    // Materials numbered before the overflow keep their ids
    EXPECT_EQ (table.getId (materials.front()), 1);
    EXPECT_EQ (table.getId (materials[MaterialTable::maxMaterials - 2]), MaterialTable::maxMaterials - 1);
    EXPECT_EQ (table.getId (materials[MaterialTable::maxMaterials - 1]), 0);
}
