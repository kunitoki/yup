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

#include <yup_graphics/yup_graphics.h>

#include <cmath>

using namespace yup;

TEST (GraphicsTests, Default_Constructor)
{
    auto context = GraphicsContext::createContext (GraphicsContext::Headless, {});
    auto renderer = context->makeRenderer (200, 200);

    Graphics g (*context, *renderer);
    EXPECT_FLOAT_EQ (g.getContextScale(), 1.0f);
    EXPECT_EQ (g.getStrokeJoin(), StrokeJoin::Miter);
    EXPECT_EQ (g.getStrokeCap(), StrokeCap::Square);
    EXPECT_EQ (g.getFillColor(), Color (0xff000000));
    EXPECT_EQ (g.getStrokeColor(), Color (0xff000000));
    EXPECT_FLOAT_EQ (g.getStrokeWidth(), 1.0f);
    EXPECT_FLOAT_EQ (g.getFeather(), 0.0f);
    EXPECT_TRUE (g.getDrawingArea().isEmpty());
    EXPECT_TRUE (g.getTransform().isIdentity());
    EXPECT_EQ (g.getBlendMode(), BlendMode::SrcOver);
    EXPECT_FLOAT_EQ (g.getOpacity(), 1.0f);
}
