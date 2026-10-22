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

#include <yup_renderer/yup_renderer.h>

#include <gtest/gtest.h>

using namespace yup;

class ReflectionProbeNodeTests : public ::testing::Test
{
};

TEST_F (ReflectionProbeNodeTests, BakeIdsAreNeverZero)
{
    ReflectionProbeNode probe;
    EXPECT_GT (probe.getBakeId(), 0u);

    probe.requestBake();
    EXPECT_GT (probe.getBakeId(), 0u);
}

TEST_F (ReflectionProbeNodeTests, RequestBakeChangesTheIdAndIdsAreNeverShared)
{
    ReflectionProbeNode a;
    ReflectionProbeNode b;
    EXPECT_NE (a.getBakeId(), b.getBakeId());

    const auto before = a.getBakeId();
    a.requestBake();
    EXPECT_NE (a.getBakeId(), before);
    EXPECT_NE (a.getBakeId(), b.getBakeId());
}

TEST_F (ReflectionProbeNodeTests, ProbeRecreatedAtTheSameAddressGetsANewId)
{
    std::optional<ReflectionProbeNode> probe;
    probe.emplace();
    const auto first = probe->getBakeId();

    probe.reset();
    probe.emplace();
    EXPECT_NE (probe->getBakeId(), first);
}
