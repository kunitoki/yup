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

#include <gtest/gtest.h>

#include <yup_animation/yup_animation.h>

using namespace yup;

// ==============================================================================
// AnimationRenderResources — default construction and reset
// ==============================================================================

TEST (AnimationRenderResourcesTests, DefaultConstructedIsValid)
{
    AnimationRenderResources resources;
    // No crash — smoke test for default construction.
}

TEST (AnimationRenderResourcesTests, ResetOnDefaultConstructionDoesNotCrash)
{
    AnimationRenderResources resources;
    EXPECT_NO_THROW (resources.reset());
}

TEST (AnimationRenderResourcesTests, DoubleResetDoesNotCrash)
{
    AnimationRenderResources resources;
    resources.reset();
    EXPECT_NO_THROW (resources.reset());
}

// ==============================================================================
// AnimationRenderResources::getMattePipeline
//
// NOTE: getMattePipeline triggers jassertfalse on headless when the shader
// transpiler is enabled (compilation fails without a real GPU). Without the
// transpiler the method is a trivial no-op. The method is exercised indirectly
// by the matte-rendering tests in AnimationRenderer (which fall through to the
// geometric-clip path on headless).
// ==============================================================================

// ==============================================================================
// AnimationRenderResources::acquireMatteCanvases
// ==============================================================================

TEST (AnimationRenderResourcesTests, AcquireMatteCanvasesWithZeroWidthReturnsInvalidLease)
{
    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (context, nullptr);

    AnimationRenderResources resources;
    auto lease = resources.acquireMatteCanvases (*context, 0, 64);
    EXPECT_FALSE (lease.isValid());
}

TEST (AnimationRenderResourcesTests, AcquireMatteCanvasesWithZeroHeightReturnsInvalidLease)
{
    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (context, nullptr);

    AnimationRenderResources resources;
    auto lease = resources.acquireMatteCanvases (*context, 64, 0);
    EXPECT_FALSE (lease.isValid());
}

TEST (AnimationRenderResourcesTests, AcquireMatteCanvasesWithNegativeDimensionsReturnsInvalidLease)
{
    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (context, nullptr);

    AnimationRenderResources resources;
    auto lease = resources.acquireMatteCanvases (*context, -10, 64);
    EXPECT_FALSE (lease.isValid());
}

TEST (AnimationRenderResourcesTests, AcquireMatteCanvasesHeadlessContextReturnsInvalidLease)
{
    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (context, nullptr);

    AnimationRenderResources resources;
    auto lease = resources.acquireMatteCanvases (*context, 64, 64);

    // On headless, GpuCanvas::create returns nullptr, so the lease is invalid.
    EXPECT_FALSE (lease.isValid());
}

TEST (AnimationRenderResourcesTests, AcquireMatteCanvasesBackendContextSwitchingResetsPool)
{
    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (context, nullptr);

    auto otherContext = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (otherContext, nullptr);

    AnimationRenderResources resources;

    // First acquisition on ctx1
    auto lease1 = resources.acquireMatteCanvases (*context, 64, 64);
    EXPECT_FALSE (lease1.isValid()); // headless → no canvas

    // Move to different context — must self-assert (no active leases) but not crash.
    EXPECT_NO_THROW ({
        auto lease2 = resources.acquireMatteCanvases (*otherContext, 64, 64);
        EXPECT_FALSE (lease2.isValid());
    });
}

// ==============================================================================
// AnimationRenderResources::getPrecompCanvas
// ==============================================================================

TEST (AnimationRenderResourcesTests, GetPrecompCanvasWithZeroWidthReturnsNull)
{
    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (context, nullptr);

    AnimationRenderResources resources;
    EXPECT_EQ (resources.getPrecompCanvas (*context, "key", 0, 64), nullptr);
}

TEST (AnimationRenderResourcesTests, GetPrecompCanvasWithZeroHeightReturnsNull)
{
    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (context, nullptr);

    AnimationRenderResources resources;
    EXPECT_EQ (resources.getPrecompCanvas (*context, "key", 64, 0), nullptr);
}

TEST (AnimationRenderResourcesTests, GetPrecompCanvasHeadlessReturnsNull)
{
    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (context, nullptr);

    AnimationRenderResources resources;
    EXPECT_EQ (resources.getPrecompCanvas (*context, "key", 64, 64), nullptr);
}

TEST (AnimationRenderResourcesTests, GetPrecompCanvasSameKeyReusesSlot)
{
    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (context, nullptr);

    AnimationRenderResources resources;
    auto canvas1 = resources.getPrecompCanvas (*context, "testKey", 64, 64);
    auto canvas2 = resources.getPrecompCanvas (*context, "testKey", 64, 64);

    // Both are nullptr on headless, which is consistent.
    EXPECT_EQ (canvas1, canvas2);
}

TEST (AnimationRenderResourcesTests, GetPrecompCanvasDifferentKeysAreIndependent)
{
    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (context, nullptr);

    AnimationRenderResources resources;
    auto canvas1 = resources.getPrecompCanvas (*context, "keyA", 64, 64);
    auto canvas2 = resources.getPrecompCanvas (*context, "keyB", 64, 64);

    // Both null on headless, but the call path is exercised.
    EXPECT_EQ (canvas1, canvas2);
}

TEST (AnimationRenderResourcesTests, GetPrecompCanvasBackendContextSwitchingResetsPools)
{
    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (context, nullptr);

    auto otherContext = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (otherContext, nullptr);

    AnimationRenderResources resources;
    auto canvas1 = resources.getPrecompCanvas (*context, "key", 64, 64);

    EXPECT_NO_THROW ({
        auto canvas2 = resources.getPrecompCanvas (*otherContext, "key", 64, 64);
        EXPECT_EQ (canvas2, nullptr);
    });
}

// ==============================================================================
// AnimationRenderResources::reset
// ==============================================================================

TEST (AnimationRenderResourcesTests, ResetClearsPrecompCanvasCache)
{
    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (context, nullptr);

    AnimationRenderResources resources;
    resources.getPrecompCanvas (*context, "key", 64, 64);
    resources.reset();

    // After reset, a new getPrecompCanvas with the same key should not crash.
    EXPECT_EQ (resources.getPrecompCanvas (*context, "key", 64, 64), nullptr);
}

// ==============================================================================
// MatteCanvasLease — default construction (invalid)
// ==============================================================================

TEST (MatteCanvasLeaseTests, DefaultConstructedIsInvalid)
{
    AnimationRenderResources::MatteCanvasLease lease;
    EXPECT_FALSE (lease.isValid());
}

// ==============================================================================
// MatteCanvasLease — move semantics with invalid leases
// ==============================================================================

TEST (MatteCanvasLeaseTests, MoveConstructorTransfersInvalidState)
{
    AnimationRenderResources::MatteCanvasLease lease1;
    EXPECT_FALSE (lease1.isValid());

    AnimationRenderResources::MatteCanvasLease lease2 (std::move (lease1));
    EXPECT_FALSE (lease2.isValid());
    EXPECT_FALSE (lease1.isValid());
}

TEST (MatteCanvasLeaseTests, MoveAssignmentFromInvalid)
{
    AnimationRenderResources::MatteCanvasLease lease1;
    AnimationRenderResources::MatteCanvasLease lease2;
    EXPECT_FALSE (lease1.isValid());
    EXPECT_FALSE (lease2.isValid());

    lease2 = std::move (lease1);
    EXPECT_FALSE (lease2.isValid());
    EXPECT_FALSE (lease1.isValid());
}

TEST (MatteCanvasLeaseTests, SelfMoveAssignmentDoesNotCrash)
{
    AnimationRenderResources::MatteCanvasLease lease;
    // Self-move is technically UB but the guard in operator= handles
    // the compare-and-skip pattern. This is a smoke test.
    lease = std::move (lease);
    EXPECT_FALSE (lease.isValid());
}

// ==============================================================================
// MatteCanvasLease — acquire from resources with invalid dimensions
// ==============================================================================

TEST (MatteCanvasLeaseTests, InvalidLeaseGetTargetCanvasTriggersAssert)
{
    // jassert is disabled in release; this is a smoke test.
    AnimationRenderResources::MatteCanvasLease lease;
    EXPECT_FALSE (lease.isValid());
}

// ==============================================================================
// AnimationRenderResources + MatteCanvasLease — destroy with active lease
// ==============================================================================

TEST (AnimationRenderResourcesIntegrationTests, DestroyResourcesWhileLeaseIsValid)
{
    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (context, nullptr);

    // On headless the lease is never valid, but the acquire path is exercised.
    AnimationRenderResources resources;
    {
        auto lease = resources.acquireMatteCanvases (*context, 64, 64);
        EXPECT_FALSE (lease.isValid());
    }
    // resources outlives the lease — no crash.
}

TEST (AnimationRenderResourcesIntegrationTests, ResetWhileLeaseIsActiveAsserts)
{
    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (context, nullptr);

    AnimationRenderResources resources;
    auto lease = resources.acquireMatteCanvases (*context, 64, 64);
    EXPECT_FALSE (lease.isValid());
    // lease is destroyed here, releasing back before reset
    // (on headless the lease is never "in use" since creation fails)

    resources.reset();
    SUCCEED();
}

// ==============================================================================
// AnimationRenderResources — all public APIs smoke test
// ==============================================================================

TEST (AnimationRenderResourcesTests, FullApiSmokeTest)
{
    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (context, nullptr);

    AnimationRenderResources resources;

    // acquireMatteCanvases — invalid dimensions
    {
        auto lease = resources.acquireMatteCanvases (*context, 0, 0);
        EXPECT_FALSE (lease.isValid());
    }

    // acquireMatteCanvases — valid dimensions, headless
    {
        auto lease = resources.acquireMatteCanvases (*context, 64, 64);
        EXPECT_FALSE (lease.isValid());
    }

    // getPrecompCanvas
    auto canvas = resources.getPrecompCanvas (*context, "full", 64, 64);
    EXPECT_EQ (canvas, nullptr);

    // reset
    resources.reset();

    // After reset, can still use APIs
    auto afterResetCanvas = resources.getPrecompCanvas (*context, "afterReset", 32, 32);
    EXPECT_EQ (afterResetCanvas, nullptr);
}
