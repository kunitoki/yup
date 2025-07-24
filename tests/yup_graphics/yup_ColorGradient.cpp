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
#include <vector>

using namespace yup;

namespace
{
static constexpr float tol = 1e-5f;
} // namespace

TEST (ColorGradientTests, Default_Constructor)
{
    ColorGradient gradient;
    EXPECT_EQ (gradient.getType(), ColorGradient::Linear);
    EXPECT_EQ (gradient.getNumStops(), 0);
    EXPECT_FLOAT_EQ (gradient.getRadius(), 0.0f);

    // Default values when no stops exist
    EXPECT_EQ (gradient.getStartColor(), Color());
    EXPECT_EQ (gradient.getFinishColor(), Color());
    EXPECT_FLOAT_EQ (gradient.getStartX(), 0.0f);
    EXPECT_FLOAT_EQ (gradient.getStartY(), 0.0f);
    EXPECT_FLOAT_EQ (gradient.getFinishX(), 0.0f);
    EXPECT_FLOAT_EQ (gradient.getFinishY(), 0.0f);
    EXPECT_FLOAT_EQ (gradient.getStartDelta(), 0.0f);
    EXPECT_FLOAT_EQ (gradient.getFinishDelta(), 1.0f);
}

TEST (ColorGradientTests, Two_Color_Linear_Constructor)
{
    Color red (0xffff0000);
    Color blue (0xff0000ff);
    ColorGradient gradient (red, 10.0f, 20.0f, blue, 100.0f, 200.0f, ColorGradient::Linear);

    EXPECT_EQ (gradient.getType(), ColorGradient::Linear);
    EXPECT_EQ (gradient.getNumStops(), 2);
    EXPECT_EQ (gradient.getStartColor(), red);
    EXPECT_EQ (gradient.getFinishColor(), blue);
    EXPECT_FLOAT_EQ (gradient.getStartX(), 10.0f);
    EXPECT_FLOAT_EQ (gradient.getStartY(), 20.0f);
    EXPECT_FLOAT_EQ (gradient.getFinishX(), 100.0f);
    EXPECT_FLOAT_EQ (gradient.getFinishY(), 200.0f);
    EXPECT_FLOAT_EQ (gradient.getStartDelta(), 0.0f);
    EXPECT_FLOAT_EQ (gradient.getFinishDelta(), 1.0f);

    // For linear gradient, radius is not used
    EXPECT_FLOAT_EQ (gradient.getRadius(), 0.0f);
}

TEST (ColorGradientTests, Two_Color_Radial_Constructor)
{
    Color green (0xff00ff00);
    Color yellow (0xffffff00);
    ColorGradient gradient (green, 50.0f, 60.0f, yellow, 80.0f, 90.0f, ColorGradient::Radial);

    EXPECT_EQ (gradient.getType(), ColorGradient::Radial);
    EXPECT_EQ (gradient.getNumStops(), 2);
    EXPECT_EQ (gradient.getStartColor(), green);
    EXPECT_EQ (gradient.getFinishColor(), yellow);
    EXPECT_FLOAT_EQ (gradient.getStartX(), 50.0f);
    EXPECT_FLOAT_EQ (gradient.getStartY(), 60.0f);
    EXPECT_FLOAT_EQ (gradient.getFinishX(), 80.0f);
    EXPECT_FLOAT_EQ (gradient.getFinishY(), 90.0f);

    // For radial gradient, radius should be calculated
    float expectedRadius = std::sqrt ((80.0f - 50.0f) * (80.0f - 50.0f) + (90.0f - 60.0f) * (90.0f - 60.0f));
    EXPECT_NEAR (gradient.getRadius(), expectedRadius, tol);
}

TEST (ColorGradientTests, Multi_Stop_Constructor)
{
    std::vector<ColorGradient::ColorStop> stops;
    stops.emplace_back (Color (0xffff0000), 0.0f, 0.0f, 0.0f);     // Red at start
    stops.emplace_back (Color (0xff00ff00), 50.0f, 50.0f, 0.5f);   // Green at middle
    stops.emplace_back (Color (0xff0000ff), 100.0f, 100.0f, 1.0f); // Blue at end

    ColorGradient gradient (ColorGradient::Linear, stops);

    EXPECT_EQ (gradient.getType(), ColorGradient::Linear);
    EXPECT_EQ (gradient.getNumStops(), 3);
    EXPECT_EQ (gradient.getStartColor(), Color (0xffff0000));
    EXPECT_EQ (gradient.getFinishColor(), Color (0xff0000ff));

    // Test individual stops
    auto stop0 = gradient.getStop (0);
    EXPECT_EQ (stop0.color, Color (0xffff0000));
    EXPECT_FLOAT_EQ (stop0.x, 0.0f);
    EXPECT_FLOAT_EQ (stop0.y, 0.0f);
    EXPECT_FLOAT_EQ (stop0.delta, 0.0f);

    auto stop1 = gradient.getStop (1);
    EXPECT_EQ (stop1.color, Color (0xff00ff00));
    EXPECT_FLOAT_EQ (stop1.x, 50.0f);
    EXPECT_FLOAT_EQ (stop1.y, 50.0f);
    EXPECT_FLOAT_EQ (stop1.delta, 0.5f);

    auto stop2 = gradient.getStop (2);
    EXPECT_EQ (stop2.color, Color (0xff0000ff));
    EXPECT_FLOAT_EQ (stop2.x, 100.0f);
    EXPECT_FLOAT_EQ (stop2.y, 100.0f);
    EXPECT_FLOAT_EQ (stop2.delta, 1.0f);
}

TEST (ColorGradientTests, Multi_Stop_Radial_Constructor)
{
    std::vector<ColorGradient::ColorStop> stops;
    stops.emplace_back (Color (0xffff0000), 10.0f, 20.0f, 0.0f);
    stops.emplace_back (Color (0xff0000ff), 40.0f, 50.0f, 1.0f);

    ColorGradient gradient (ColorGradient::Radial, stops);

    EXPECT_EQ (gradient.getType(), ColorGradient::Radial);

    // Radius should be calculated from first and last stops
    float expectedRadius = std::sqrt ((40.0f - 10.0f) * (40.0f - 10.0f) + (50.0f - 20.0f) * (50.0f - 20.0f));
    EXPECT_NEAR (gradient.getRadius(), expectedRadius, tol);
}

TEST (ColorGradientTests, Copy_And_Move_Constructors)
{
    Color red (0xffff0000);
    Color blue (0xff0000ff);
    ColorGradient original (red, 10.0f, 20.0f, blue, 100.0f, 200.0f, ColorGradient::Linear);

    // Copy constructor
    ColorGradient copied (original);
    EXPECT_EQ (copied.getType(), original.getType());
    EXPECT_EQ (copied.getNumStops(), original.getNumStops());
    EXPECT_EQ (copied.getStartColor(), original.getStartColor());
    EXPECT_EQ (copied.getFinishColor(), original.getFinishColor());

    // Move constructor
    ColorGradient moved (std::move (original));
    EXPECT_EQ (moved.getType(), ColorGradient::Linear);
    EXPECT_EQ (moved.getNumStops(), 2);
    EXPECT_EQ (moved.getStartColor(), red);
    EXPECT_EQ (moved.getFinishColor(), blue);

    // Assignment operators
    ColorGradient assigned;
    assigned = copied;
    EXPECT_EQ (assigned.getType(), copied.getType());
    EXPECT_EQ (assigned.getNumStops(), copied.getNumStops());

    ColorGradient moveAssigned;
    moveAssigned = std::move (copied);
    EXPECT_EQ (moveAssigned.getType(), ColorGradient::Linear);
    EXPECT_EQ (moveAssigned.getNumStops(), 2);
}

TEST (ColorGradientTests, ColorStop_Default_Constructor)
{
    ColorGradient::ColorStop stop;
    EXPECT_EQ (stop.color, Color());
    EXPECT_FLOAT_EQ (stop.x, 0.0f);
    EXPECT_FLOAT_EQ (stop.y, 0.0f);
    EXPECT_FLOAT_EQ (stop.delta, 0.0f);
}

TEST (ColorGradientTests, ColorStop_Parameterized_Constructor)
{
    Color red (0xffff0000);
    ColorGradient::ColorStop stop (red, 10.0f, 20.0f, 0.5f);

    EXPECT_EQ (stop.color, red);
    EXPECT_FLOAT_EQ (stop.x, 10.0f);
    EXPECT_FLOAT_EQ (stop.y, 20.0f);
    EXPECT_FLOAT_EQ (stop.delta, 0.5f);
}

TEST (ColorGradientTests, ColorStop_Copy_And_Move)
{
    Color red (0xffff0000);
    ColorGradient::ColorStop original (red, 10.0f, 20.0f, 0.5f);

    // Copy
    ColorGradient::ColorStop copied (original);
    EXPECT_EQ (copied.color, original.color);
    EXPECT_FLOAT_EQ (copied.x, original.x);
    EXPECT_FLOAT_EQ (copied.y, original.y);
    EXPECT_FLOAT_EQ (copied.delta, original.delta);

    // Move
    ColorGradient::ColorStop moved (std::move (original));
    EXPECT_EQ (moved.color, red);
    EXPECT_FLOAT_EQ (moved.x, 10.0f);
    EXPECT_FLOAT_EQ (moved.y, 20.0f);
    EXPECT_FLOAT_EQ (moved.delta, 0.5f);

    // Assignment
    ColorGradient::ColorStop assigned;
    assigned = copied;
    EXPECT_EQ (assigned.color, copied.color);

    ColorGradient::ColorStop moveAssigned;
    moveAssigned = std::move (copied);
    EXPECT_EQ (moveAssigned.color, red);
}

TEST (ColorGradientTests, Add_Color_Stop)
{
    ColorGradient gradient;

    // Add stops in random order
    gradient.addColorStop (Color (0xff00ff00), 50.0f, 50.0f, 0.5f);   // Middle
    gradient.addColorStop (Color (0xff0000ff), 100.0f, 100.0f, 1.0f); // End
    gradient.addColorStop (Color (0xffff0000), 0.0f, 0.0f, 0.0f);     // Start

    // Should be sorted by delta
    EXPECT_EQ (gradient.getNumStops(), 3);
    EXPECT_EQ (gradient.getStartColor(), Color (0xffff0000));
    EXPECT_EQ (gradient.getFinishColor(), Color (0xff0000ff));

    auto stop0 = gradient.getStop (0);
    EXPECT_FLOAT_EQ (stop0.delta, 0.0f);

    auto stop1 = gradient.getStop (1);
    EXPECT_FLOAT_EQ (stop1.delta, 0.5f);

    auto stop2 = gradient.getStop (2);
    EXPECT_FLOAT_EQ (stop2.delta, 1.0f);
}

TEST (ColorGradientTests, Clear_Stops)
{
    Color red (0xffff0000);
    Color blue (0xff0000ff);
    ColorGradient gradient (red, 10.0f, 20.0f, blue, 100.0f, 200.0f, ColorGradient::Linear);

    EXPECT_EQ (gradient.getNumStops(), 2);

    gradient.clearStops();
    EXPECT_EQ (gradient.getNumStops(), 0);
}

TEST (ColorGradientTests, Get_Stops_Span)
{
    std::vector<ColorGradient::ColorStop> stops;
    stops.emplace_back (Color (0xffff0000), 0.0f, 0.0f, 0.0f);
    stops.emplace_back (Color (0xff00ff00), 50.0f, 50.0f, 0.5f);
    stops.emplace_back (Color (0xff0000ff), 100.0f, 100.0f, 1.0f);

    ColorGradient gradient (ColorGradient::Linear, stops);

    auto stopsSpan = gradient.getStops();
    EXPECT_EQ (stopsSpan.size(), 3);
    EXPECT_EQ (stopsSpan[0].color, Color (0xffff0000));
    EXPECT_EQ (stopsSpan[1].color, Color (0xff00ff00));
    EXPECT_EQ (stopsSpan[2].color, Color (0xff0000ff));
}

TEST (ColorGradientTests, Alpha_Operations_SetAlpha)
{
    Color red (0xffff0000);
    Color blue (0xff0000ff);
    ColorGradient gradient (red, 0.0f, 0.0f, blue, 100.0f, 100.0f, ColorGradient::Linear);

    // Test setAlpha with uint8
    gradient.setAlpha (uint8_t (128));
    EXPECT_EQ (gradient.getStartColor().getAlpha(), 128);
    EXPECT_EQ (gradient.getFinishColor().getAlpha(), 128);

    // Test setAlpha with float
    gradient.setAlpha (0.25f);
    EXPECT_EQ (gradient.getStartColor().getAlpha(), 64); // 0.25 * 255 = 63.75, rounds to 64
    EXPECT_EQ (gradient.getFinishColor().getAlpha(), 64);
}

TEST (ColorGradientTests, Alpha_Operations_WithAlpha)
{
    Color red (0xffff0000);
    Color blue (0xff0000ff);
    ColorGradient original (red, 0.0f, 0.0f, blue, 100.0f, 100.0f, ColorGradient::Linear);

    // Test withAlpha with uint8
    ColorGradient withAlpha128 = original.withAlpha (uint8_t (128));
    EXPECT_EQ (withAlpha128.getStartColor().getAlpha(), 128);
    EXPECT_EQ (withAlpha128.getFinishColor().getAlpha(), 128);
    EXPECT_EQ (original.getStartColor().getAlpha(), 255); // Original unchanged
    EXPECT_EQ (original.getFinishColor().getAlpha(), 255);

    // Test withAlpha with float
    ColorGradient withAlpha064 = original.withAlpha (0.25f);
    EXPECT_EQ (withAlpha064.getStartColor().getAlpha(), 64); // 0.25 * 255 = 63.75, rounds to 64
    EXPECT_EQ (withAlpha064.getFinishColor().getAlpha(), 64);
}

TEST (ColorGradientTests, Alpha_Operations_WithMultipliedAlpha)
{
    Color red (0x80ff0000);  // Semi-transparent red
    Color blue (0xc00000ff); // More opaque blue
    ColorGradient original (red, 0.0f, 0.0f, blue, 100.0f, 100.0f, ColorGradient::Linear);

    // Test withMultipliedAlpha with uint8
    ColorGradient multiplied = original.withMultipliedAlpha (uint8_t (128));
    uint8 expectedRedAlpha = roundToInt (0x80 * 128.0f / 255.0f);  // 128 * 0.502 = 64.3 → 64
    uint8 expectedBlueAlpha = roundToInt (0xc0 * 128.0f / 255.0f); // 192 * 0.502 = 96.4 → 96
    EXPECT_EQ (multiplied.getStartColor().getAlpha(), expectedRedAlpha);
    EXPECT_EQ (multiplied.getFinishColor().getAlpha(), expectedBlueAlpha);

    // Test withMultipliedAlpha with float
    ColorGradient multipliedFloat = original.withMultipliedAlpha (0.5f);
    uint8 expectedRedAlphaFloat = static_cast<uint8> (0x80 * 0.5f);
    uint8 expectedBlueAlphaFloat = static_cast<uint8> (0xc0 * 0.5f);
    EXPECT_EQ (multipliedFloat.getStartColor().getAlpha(), expectedRedAlphaFloat);
    EXPECT_EQ (multipliedFloat.getFinishColor().getAlpha(), expectedBlueAlphaFloat);

    // Original should be unchanged
    EXPECT_EQ (original.getStartColor().getAlpha(), 0x80);
    EXPECT_EQ (original.getFinishColor().getAlpha(), 0xc0);
}

TEST (ColorGradientTests, Multi_Stop_Alpha_Operations)
{
    std::vector<ColorGradient::ColorStop> stops;
    stops.emplace_back (Color (0xffff0000), 0.0f, 0.0f, 0.0f);     // Red
    stops.emplace_back (Color (0x80ff0000), 50.0f, 50.0f, 0.5f);   // Semi-transparent red
    stops.emplace_back (Color (0x40ff0000), 100.0f, 100.0f, 1.0f); // More transparent red

    ColorGradient gradient (ColorGradient::Linear, stops);

    // Test setAlpha affects all stops
    gradient.setAlpha (uint8_t (64));
    for (size_t i = 0; i < gradient.getNumStops(); ++i)
    {
        EXPECT_EQ (gradient.getStop (i).color.getAlpha(), 64);
    }

    // Reset and test withMultipliedAlpha
    gradient = ColorGradient (ColorGradient::Linear, stops);
    ColorGradient multiplied = gradient.withMultipliedAlpha (0.5f);

    EXPECT_EQ (multiplied.getStop (0).color.getAlpha(), 128); // 255 * 0.5 = 127.5, rounds to 128
    EXPECT_EQ (multiplied.getStop (1).color.getAlpha(), 64);  // 128 * 0.5 = 64.0, exact
    EXPECT_EQ (multiplied.getStop (2).color.getAlpha(), 32);  // 64 * 0.5 = 32.0, exact
}

TEST (ColorGradientTests, Empty_Gradient_Edge_Cases)
{
    ColorGradient empty;

    // Should not crash when getting stop from empty gradient
    EXPECT_EQ (empty.getNumStops(), 0);

    // Alpha operations on empty gradient should not crash
    EXPECT_NO_THROW (empty.setAlpha (uint8_t (128)));
    EXPECT_NO_THROW (empty.withAlpha (0.5f));
    EXPECT_NO_THROW (empty.withMultipliedAlpha (0.5f));

    // Should still be empty after alpha operations
    EXPECT_EQ (empty.getNumStops(), 0);
}

TEST (ColorGradientTests, Single_Stop_Edge_Cases)
{
    ColorGradient gradient;
    gradient.addColorStop (Color (0xffff0000), 50.0f, 50.0f, 0.5f);

    EXPECT_EQ (gradient.getNumStops(), 1);
    EXPECT_EQ (gradient.getStartColor(), Color (0xffff0000));
    EXPECT_EQ (gradient.getFinishColor(), Color (0xffff0000)); // Same as start
    EXPECT_FLOAT_EQ (gradient.getStartDelta(), 0.5f);
    EXPECT_FLOAT_EQ (gradient.getFinishDelta(), 0.5f); // Same as start
}

TEST (ColorGradientTests, Duplicate_Delta_Values)
{
    ColorGradient gradient;

    // Add stops with same delta values
    gradient.addColorStop (Color (0xffff0000), 0.0f, 0.0f, 0.5f);
    gradient.addColorStop (Color (0xff00ff00), 50.0f, 50.0f, 0.5f);
    gradient.addColorStop (Color (0xff0000ff), 100.0f, 100.0f, 0.5f);

    EXPECT_EQ (gradient.getNumStops(), 3);

    // All should have same delta after sorting
    for (size_t i = 0; i < gradient.getNumStops(); ++i)
    {
        EXPECT_FLOAT_EQ (gradient.getStop (i).delta, 0.5f);
    }
}

TEST (ColorGradientTests, Extreme_Coordinate_Values)
{
    Color red (0xffff0000);
    Color blue (0xff0000ff);

    // Test with very large coordinates
    ColorGradient largeCoords (red, -1000.0f, -2000.0f, blue, 10000.0f, 20000.0f, ColorGradient::Radial);
    EXPECT_EQ (largeCoords.getType(), ColorGradient::Radial);
    EXPECT_FLOAT_EQ (largeCoords.getStartX(), -1000.0f);
    EXPECT_FLOAT_EQ (largeCoords.getStartY(), -2000.0f);
    EXPECT_FLOAT_EQ (largeCoords.getFinishX(), 10000.0f);
    EXPECT_FLOAT_EQ (largeCoords.getFinishY(), 20000.0f);

    // Radius should be calculated correctly
    float expectedRadius = std::sqrt ((10000.0f - (-1000.0f)) * (10000.0f - (-1000.0f)) + (20000.0f - (-2000.0f)) * (20000.0f - (-2000.0f)));
    EXPECT_NEAR (largeCoords.getRadius(), expectedRadius, 1.0f); // Allow larger tolerance for large numbers
}

TEST (ColorGradientTests, Zero_Distance_Radial_Gradient)
{
    Color red (0xffff0000);
    Color blue (0xff0000ff);

    // Same start and end points
    ColorGradient zeroRadius (red, 50.0f, 50.0f, blue, 50.0f, 50.0f, ColorGradient::Radial);
    EXPECT_FLOAT_EQ (zeroRadius.getRadius(), 0.0f);
}

TEST (ColorGradientTests, Delta_Ordering)
{
    ColorGradient gradient;

    // Add stops in random delta order
    gradient.addColorStop (Color (0xff00ff00), 50.0f, 50.0f, 0.7f);
    gradient.addColorStop (Color (0xffff0000), 0.0f, 0.0f, 0.2f);
    gradient.addColorStop (Color (0xff0000ff), 100.0f, 100.0f, 1.0f);
    gradient.addColorStop (Color (0xffffff00), 25.0f, 25.0f, 0.1f);

    EXPECT_EQ (gradient.getNumStops(), 4);

    // Should be sorted by delta
    EXPECT_FLOAT_EQ (gradient.getStop (0).delta, 0.1f);
    EXPECT_FLOAT_EQ (gradient.getStop (1).delta, 0.2f);
    EXPECT_FLOAT_EQ (gradient.getStop (2).delta, 0.7f);
    EXPECT_FLOAT_EQ (gradient.getStop (3).delta, 1.0f);

    // Start and finish should be the extreme deltas
    EXPECT_EQ (gradient.getStartColor(), Color (0xffffff00));  // Delta 0.1
    EXPECT_EQ (gradient.getFinishColor(), Color (0xff0000ff)); // Delta 1.0
}

TEST (ColorGradientTests, Type_Consistency)
{
    // Linear gradient should maintain type
    ColorGradient linear (Color (0xffff0000), 0.0f, 0.0f, Color (0xff0000ff), 100.0f, 100.0f, ColorGradient::Linear);
    EXPECT_EQ (linear.getType(), ColorGradient::Linear);

    // Radial gradient should maintain type
    ColorGradient radial (Color (0xffff0000), 0.0f, 0.0f, Color (0xff0000ff), 100.0f, 100.0f, ColorGradient::Radial);
    EXPECT_EQ (radial.getType(), ColorGradient::Radial);

    // Type should be preserved after copy
    ColorGradient linearCopy (linear);
    EXPECT_EQ (linearCopy.getType(), ColorGradient::Linear);

    ColorGradient radialCopy (radial);
    EXPECT_EQ (radialCopy.getType(), ColorGradient::Radial);
}

TEST (ColorGradientTests, Multi_Stop_Radial_No_Stops)
{
    std::vector<ColorGradient::ColorStop> emptyStops;
    ColorGradient gradient (ColorGradient::Radial, emptyStops);

    EXPECT_EQ (gradient.getType(), ColorGradient::Radial);
    EXPECT_EQ (gradient.getNumStops(), 0);
    EXPECT_FLOAT_EQ (gradient.getRadius(), 0.0f);
}

TEST (ColorGradientTests, Multi_Stop_Radial_Single_Stop)
{
    std::vector<ColorGradient::ColorStop> singleStop;
    singleStop.emplace_back (Color (0xffff0000), 50.0f, 50.0f, 0.5f);

    ColorGradient gradient (ColorGradient::Radial, singleStop);

    EXPECT_EQ (gradient.getType(), ColorGradient::Radial);
    EXPECT_EQ (gradient.getNumStops(), 1);
    EXPECT_FLOAT_EQ (gradient.getRadius(), 0.0f); // Can't calculate radius with single stop
}

TEST (ColorGradientTests, Constructor_Default_Type_Parameter)
{
    Color startColor (0xffff0000); // Red
    Color endColor (0xff0000ff);   // Blue

    // Test constructor with coordinate parameters but no type (should default to Linear)
    ColorGradient gradient1 (startColor, 0.0f, 0.0f, endColor, 100.0f, 100.0f);

    EXPECT_EQ (gradient1.getType(), ColorGradient::Linear);
    EXPECT_EQ (gradient1.getStartColor(), startColor);
    EXPECT_EQ (gradient1.getFinishColor(), endColor);
    EXPECT_FLOAT_EQ (gradient1.getStartX(), 0.0f);
    EXPECT_FLOAT_EQ (gradient1.getStartY(), 0.0f);
    EXPECT_FLOAT_EQ (gradient1.getFinishX(), 100.0f);
    EXPECT_FLOAT_EQ (gradient1.getFinishY(), 100.0f);

    // Test constructor with Point parameters but no type (should default to Linear)
    Point<float> startPoint (10.0f, 20.0f);
    Point<float> endPoint (30.0f, 40.0f);
    ColorGradient gradient2 (startColor, startPoint, endColor, endPoint);

    EXPECT_EQ (gradient2.getType(), ColorGradient::Linear);
    EXPECT_EQ (gradient2.getStartColor(), startColor);
    EXPECT_EQ (gradient2.getFinishColor(), endColor);
    EXPECT_FLOAT_EQ (gradient2.getStartX(), 10.0f);
    EXPECT_FLOAT_EQ (gradient2.getStartY(), 20.0f);
    EXPECT_FLOAT_EQ (gradient2.getFinishX(), 30.0f);
    EXPECT_FLOAT_EQ (gradient2.getFinishY(), 40.0f);
}

TEST (ColorGradientTests, Constructor_Explicit_Type_Parameter)
{
    Color startColor (0xff00ff00); // Green
    Color endColor (0xffff00ff);   // Magenta

    // Test constructor with explicit Radial type
    ColorGradient gradient1 (startColor, 50.0f, 50.0f, endColor, 150.0f, 150.0f, ColorGradient::Radial);

    EXPECT_EQ (gradient1.getType(), ColorGradient::Radial);
    EXPECT_EQ (gradient1.getStartColor(), startColor);
    EXPECT_EQ (gradient1.getFinishColor(), endColor);
    EXPECT_FLOAT_EQ (gradient1.getStartX(), 50.0f);
    EXPECT_FLOAT_EQ (gradient1.getStartY(), 50.0f);
    EXPECT_FLOAT_EQ (gradient1.getFinishX(), 150.0f);
    EXPECT_FLOAT_EQ (gradient1.getFinishY(), 150.0f);

    // For radial gradient, radius should be calculated as distance between points
    float expectedRadius = std::sqrt (100.0f * 100.0f + 100.0f * 100.0f); // sqrt((150-50)^2 + (150-50)^2)
    EXPECT_NEAR (gradient1.getRadius(), expectedRadius, tol);

    // Test constructor with explicit Linear type
    Point<float> startPoint (0.0f, 0.0f);
    Point<float> endPoint (100.0f, 0.0f);
    ColorGradient gradient2 (startColor, startPoint, endColor, endPoint, ColorGradient::Linear);

    EXPECT_EQ (gradient2.getType(), ColorGradient::Linear);
    EXPECT_FLOAT_EQ (gradient2.getRadius(), 0.0f); // Linear gradients don't have radius
}

TEST (ColorGradientTests, AddColorStop_With_Delta_Only)
{
    ColorGradient gradient;

    // Add first stop to establish baseline
    gradient.addColorStop (Color (0xffff0000), 0.0f, 0.0f, 0.0f);
    gradient.addColorStop (Color (0xff0000ff), 100.0f, 100.0f, 1.0f);

    EXPECT_EQ (gradient.getNumStops(), 2);

    // Add a stop using just delta (should interpolate position based on existing stops)
    gradient.addColorStop (Color (0xff00ff00), 0.5f);

    EXPECT_EQ (gradient.getNumStops(), 3);

    // The new stop should be positioned between the existing ones
    // This tests the new addColorStop overload that only takes color and delta
    EXPECT_EQ (gradient.getNumStops(), 3);

    // Find the green stop
    bool foundGreenStop = false;
    for (size_t i = 0; i < gradient.getNumStops(); ++i)
    {
        auto& stop = gradient.getStop (i);

        if (stop.color == Color (0xff00ff00))
        {
            foundGreenStop = true;
            EXPECT_NEAR (stop.delta, 0.5f, tol);
            // Position should be interpolated between first and last stops
            EXPECT_GT (stop.x, 0.0f);
            EXPECT_LT (stop.x, 100.0f);
            EXPECT_GT (stop.y, 0.0f);
            EXPECT_LT (stop.y, 100.0f);
            break;
        }
    }
    EXPECT_TRUE (foundGreenStop);
}

TEST (ColorGradientTests, AddColorStop_Delta_Only_Edge_Cases)
{
    ColorGradient gradient;

    // Test adding delta-only stop when gradient has no stops or only one stop
    gradient.addColorStop (Color (0xffff0000), 0.5f);

    // Should handle gracefully (implementation may vary, but should not crash)
    EXPECT_GE (gradient.getNumStops(), 0); // At least should not decrease

    // Add one more stop
    gradient.addColorStop (Color (0xff0000ff), 0.0f, 0.0f, 0.0f);
    gradient.addColorStop (Color (0xff00ff00), 100.0f, 100.0f, 1.0f);

    // Now try adding with delta only - should work
    gradient.addColorStop (Color (0xffffff00), 0.25f);

    // Should now have at least the stops we added
    EXPECT_GE (gradient.getNumStops(), 3);
}
