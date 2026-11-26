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

#include <yup_audio_gui/yup_audio_gui.h>

#include <gtest/gtest.h>

using namespace yup;

namespace yup
{
extern std::unique_ptr<yup::GraphicsContext> yup_constructHeadlessGraphicsContext (yup::GpuDevice::Options, yup::GpuDevice::Ptr);
} // namespace yup

class CartesianPlaneTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        plane = std::make_unique<CartesianPlane>();
        plane->setBounds (0.0f, 0.0f, 800.0f, 600.0f);
    }

    std::unique_ptr<CartesianPlane> plane;
};

//==============================================================================
// Construction Tests
//==============================================================================

TEST_F (CartesianPlaneTests, DefaultConstructor)
{
    auto range = plane->getXRange();
    EXPECT_DOUBLE_EQ (0.0, range.getStart());
    EXPECT_DOUBLE_EQ (1.0, range.getEnd());

    range = plane->getYRange();
    EXPECT_DOUBLE_EQ (0.0, range.getStart());
    EXPECT_DOUBLE_EQ (1.0, range.getEnd());
}

TEST_F (CartesianPlaneTests, DefaultScaleTypeIsLinear)
{
    EXPECT_EQ (CartesianPlane::AxisScaleType::linear, plane->getXScaleType());
    EXPECT_EQ (CartesianPlane::AxisScaleType::linear, plane->getYScaleType());
}

//==============================================================================
// Axis Range Tests
//==============================================================================

TEST_F (CartesianPlaneTests, SetXRangeUpdatesRange)
{
    plane->setXRange (0.0, 100.0);

    auto range = plane->getXRange();
    EXPECT_DOUBLE_EQ (0.0, range.getStart());
    EXPECT_DOUBLE_EQ (100.0, range.getEnd());
}

TEST_F (CartesianPlaneTests, SetYRangeUpdatesRange)
{
    plane->setYRange (-50.0, 50.0);

    auto range = plane->getYRange();
    EXPECT_DOUBLE_EQ (-50.0, range.getStart());
    EXPECT_DOUBLE_EQ (50.0, range.getEnd());
}

TEST_F (CartesianPlaneTests, SetXRangeWithNegativeValues)
{
    plane->setXRange (-100.0, -10.0);

    auto range = plane->getXRange();
    EXPECT_DOUBLE_EQ (-100.0, range.getStart());
    EXPECT_DOUBLE_EQ (-10.0, range.getEnd());
}

TEST_F (CartesianPlaneTests, SetYRangeWithLargeValues)
{
    plane->setYRange (0.0, 1000000.0);

    auto range = plane->getYRange();
    EXPECT_DOUBLE_EQ (0.0, range.getStart());
    EXPECT_DOUBLE_EQ (1000000.0, range.getEnd());
}

TEST_F (CartesianPlaneTests, SetXRangeWithEqualValues)
{
    plane->setXRange (50.0, 50.0);

    // Equal values are rejected, range stays at default
    auto range = plane->getXRange();
    EXPECT_DOUBLE_EQ (0.0, range.getStart());
    EXPECT_DOUBLE_EQ (1.0, range.getEnd());
}

TEST_F (CartesianPlaneTests, SetYRangeWithInvertedValues)
{
    plane->setYRange (100.0, 0.0);

    // Inverted values are rejected, range stays at default
    auto range = plane->getYRange();
    EXPECT_DOUBLE_EQ (0.0, range.getStart());
    EXPECT_DOUBLE_EQ (1.0, range.getEnd());
}

//==============================================================================
// Axis Scale Type Tests
//==============================================================================

TEST_F (CartesianPlaneTests, SetXScaleTypeToLogarithmic)
{
    // Logarithmic scale requires positive range
    plane->setXRange (1.0, 1000.0);
    plane->setXScaleType (CartesianPlane::AxisScaleType::logarithmic);
    EXPECT_EQ (CartesianPlane::AxisScaleType::logarithmic, plane->getXScaleType());
}

TEST_F (CartesianPlaneTests, SetYScaleTypeToLogarithmic)
{
    // Logarithmic scale requires positive range
    plane->setYRange (1.0, 1000.0);
    plane->setYScaleType (CartesianPlane::AxisScaleType::logarithmic);
    EXPECT_EQ (CartesianPlane::AxisScaleType::logarithmic, plane->getYScaleType());
}

TEST_F (CartesianPlaneTests, ToggleXScaleType)
{
    // Logarithmic scale requires positive range
    plane->setXRange (1.0, 1000.0);
    plane->setXScaleType (CartesianPlane::AxisScaleType::logarithmic);
    EXPECT_EQ (CartesianPlane::AxisScaleType::logarithmic, plane->getXScaleType());

    plane->setXScaleType (CartesianPlane::AxisScaleType::linear);
    EXPECT_EQ (CartesianPlane::AxisScaleType::linear, plane->getXScaleType());
}

TEST_F (CartesianPlaneTests, ToggleYScaleType)
{
    // Logarithmic scale requires positive range
    plane->setYRange (1.0, 1000.0);
    plane->setYScaleType (CartesianPlane::AxisScaleType::logarithmic);
    EXPECT_EQ (CartesianPlane::AxisScaleType::logarithmic, plane->getYScaleType());

    plane->setYScaleType (CartesianPlane::AxisScaleType::linear);
    EXPECT_EQ (CartesianPlane::AxisScaleType::linear, plane->getYScaleType());
}

//==============================================================================
// Margins Tests
//==============================================================================

TEST_F (CartesianPlaneTests, SetMarginsUpdatesMargins)
{
    plane->setMargins (40, 80, 30, 20);

    auto margins = plane->getMargins();
    EXPECT_EQ (80, margins.getLeft());
    EXPECT_EQ (40, margins.getTop());
}

TEST_F (CartesianPlaneTests, SetMarginsWithZeroValues)
{
    plane->setMargins (0, 0, 0, 0);

    auto margins = plane->getMargins();
    EXPECT_EQ (0, margins.getLeft());
    EXPECT_EQ (0, margins.getTop());
}

TEST_F (CartesianPlaneTests, SetMarginsWithLargeValues)
{
    plane->setMargins (200, 200, 200, 200);

    // Should not crash
    EXPECT_TRUE (true);
}

//==============================================================================
// Title Tests
//==============================================================================

TEST_F (CartesianPlaneTests, DefaultTitleIsEmpty)
{
    EXPECT_TRUE (plane->getTitle().isEmpty());
}

TEST_F (CartesianPlaneTests, SetTitleUpdatesTitle)
{
    plane->setTitle ("Frequency Response");
    EXPECT_EQ ("Frequency Response", plane->getTitle());
}

TEST_F (CartesianPlaneTests, SetEmptyTitle)
{
    plane->setTitle ("Test");
    plane->setTitle ("");
    EXPECT_TRUE (plane->getTitle().isEmpty());
}

TEST_F (CartesianPlaneTests, TheTitleIsTheOneOfTheComponent)
{
    // Taken as the component it is, the plane is given and tells the title it draws
    Component& component = *plane;

    component.setTitle ("Response");
    EXPECT_EQ (plane->getTitle(), "Response");
    EXPECT_EQ (component.getTitle(), "Response");

    plane->setTitle ("Phase");
    EXPECT_EQ (component.getTitle(), "Phase");
    EXPECT_EQ (component.getProperty (Identifier ("title")).toString(), "Phase");
}

TEST_F (CartesianPlaneTests, SetTitleColor)
{
    Color testColor (0xFF00FF00);
    plane->setTitleColor (testColor);
    EXPECT_EQ (testColor, plane->getTitleColor());
}

TEST_F (CartesianPlaneTests, SetTitleJustification)
{
    plane->setTitleJustification (Justification::left);
    EXPECT_EQ (Justification::left, plane->getTitleJustification());

    plane->setTitleJustification (Justification::right);
    EXPECT_EQ (Justification::right, plane->getTitleJustification());
}

//==============================================================================
// Background Color Tests
//==============================================================================

TEST_F (CartesianPlaneTests, SetBackgroundColor)
{
    Color testColor (0xFF123456);
    plane->setBackgroundColor (testColor);
    EXPECT_EQ (testColor, plane->getBackgroundColor());
}

//==============================================================================
// Vertical Grid Lines Tests
//==============================================================================

TEST_F (CartesianPlaneTests, AddVerticalGridLine)
{
    plane->addVerticalGridLine (0.5);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, AddMultipleVerticalGridLines)
{
    plane->addVerticalGridLine (0.25);
    plane->addVerticalGridLine (0.5);
    plane->addVerticalGridLine (0.75);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, SetVerticalGridLinesFromVector)
{
    std::vector<double> gridValues { 0.2, 0.4, 0.6, 0.8 };
    plane->setVerticalGridLines (gridValues);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, ClearVerticalGridLines)
{
    plane->addVerticalGridLine (0.5);
    plane->clearVerticalGridLines();

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, AddVerticalGridLineWithCustomColor)
{
    Color gridColor (0xFFFF0000);
    plane->addVerticalGridLine (0.5, gridColor, 2.0f, true);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, SetVerticalGridLinesWithEmptyVector)
{
    std::vector<double> emptyGrid;
    plane->setVerticalGridLines (emptyGrid);

    // Should not crash
    EXPECT_TRUE (true);
}

//==============================================================================
// Horizontal Grid Lines Tests
//==============================================================================

TEST_F (CartesianPlaneTests, AddHorizontalGridLine)
{
    plane->addHorizontalGridLine (0.5);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, AddMultipleHorizontalGridLines)
{
    plane->addHorizontalGridLine (0.25);
    plane->addHorizontalGridLine (0.5);
    plane->addHorizontalGridLine (0.75);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, SetHorizontalGridLinesFromVector)
{
    std::vector<double> gridValues { 0.2, 0.4, 0.6, 0.8 };
    plane->setHorizontalGridLines (gridValues);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, ClearHorizontalGridLines)
{
    plane->addHorizontalGridLine (0.5);
    plane->clearHorizontalGridLines();

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, AddHorizontalGridLineWithCustomColor)
{
    Color gridColor (0xFF00FF00);
    plane->addHorizontalGridLine (0.5, gridColor, 2.0f, true);

    // Should not crash
    EXPECT_TRUE (true);
}

//==============================================================================
// X Axis Labels Tests
//==============================================================================

TEST_F (CartesianPlaneTests, AddXAxisLabel)
{
    plane->addXAxisLabel (0.5, "0.5");

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, AddMultipleXAxisLabels)
{
    plane->addXAxisLabel (0.0, "0");
    plane->addXAxisLabel (0.5, "0.5");
    plane->addXAxisLabel (1.0, "1");

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, SetXAxisLabelsFromVector)
{
    std::vector<double> labelValues { 0.0, 0.25, 0.5, 0.75, 1.0 };
    plane->setXAxisLabels (labelValues);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, ClearXAxisLabels)
{
    plane->addXAxisLabel (0.5, "0.5");
    plane->clearXAxisLabels();

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, AddXAxisLabelWithCustomColor)
{
    Color labelColor (0xFFFFFFFF);
    plane->addXAxisLabel (0.5, "0.5", labelColor, 12.0f);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, SetXAxisLabelsWithEmptyVector)
{
    std::vector<double> emptyLabels;
    plane->setXAxisLabels (emptyLabels);

    // Should not crash
    EXPECT_TRUE (true);
}

//==============================================================================
// Y Axis Labels Tests
//==============================================================================

TEST_F (CartesianPlaneTests, AddYAxisLabel)
{
    plane->addYAxisLabel (0.5, "0.5");

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, AddMultipleYAxisLabels)
{
    plane->addYAxisLabel (0.0, "0");
    plane->addYAxisLabel (0.5, "0.5");
    plane->addYAxisLabel (1.0, "1");

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, SetYAxisLabelsFromVector)
{
    std::vector<double> labelValues { 0.0, 0.25, 0.5, 0.75, 1.0 };
    plane->setYAxisLabels (labelValues);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, ClearYAxisLabels)
{
    plane->addYAxisLabel (0.5, "0.5");
    plane->clearYAxisLabels();

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, AddYAxisLabelWithCustomColor)
{
    Color labelColor (0xFFFFFFFF);
    plane->addYAxisLabel (0.5, "0.5", labelColor, 12.0f);

    // Should not crash
    EXPECT_TRUE (true);
}

//==============================================================================
// Signal Tests
//==============================================================================

TEST_F (CartesianPlaneTests, AddSignalReturnsIndex)
{
    int index = plane->addSignal ("Signal 1");
    EXPECT_EQ (0, index);
}

TEST_F (CartesianPlaneTests, AddMultipleSignals)
{
    int index1 = plane->addSignal ("Signal 1");
    int index2 = plane->addSignal ("Signal 2");

    EXPECT_EQ (0, index1);
    EXPECT_EQ (1, index2);
}

TEST_F (CartesianPlaneTests, GetNumSignals)
{
    plane->addSignal ("Signal 1");
    plane->addSignal ("Signal 2");

    EXPECT_EQ (2, plane->getNumSignals());
}

TEST_F (CartesianPlaneTests, GetSignalByIndex)
{
    plane->addSignal ("Test Signal", Colors::red, 3.0f);

    auto* signal = plane->getSignal (0);
    ASSERT_NE (nullptr, signal);
    EXPECT_EQ ("Test Signal", signal->name);
    EXPECT_EQ (Colors::red, signal->color);
    EXPECT_FLOAT_EQ (3.0f, signal->strokeWidth);
    EXPECT_TRUE (signal->visible);
}

TEST_F (CartesianPlaneTests, GetSignalWithInvalidIndex)
{
    auto* signal = plane->getSignal (10);
    EXPECT_EQ (nullptr, signal);
}

TEST_F (CartesianPlaneTests, UpdateSignalData)
{
    int index = plane->addSignal ("Signal 1");

    std::vector<Point<double>> data;
    data.push_back ({ 0.0, 0.0 });
    data.push_back ({ 0.5, 0.5 });
    data.push_back ({ 1.0, 1.0 });

    plane->updateSignalData (index, data);

    auto* signal = plane->getSignal (index);
    ASSERT_NE (nullptr, signal);
    EXPECT_EQ (3, static_cast<int> (signal->data.size()));
}

TEST_F (CartesianPlaneTests, UpdateSignalDataWithInvalidIndex)
{
    std::vector<Point<double>> data;
    data.push_back ({ 0.0, 0.0 });

    // Should not crash
    plane->updateSignalData (10, data);
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, SetSignalVisible)
{
    int index = plane->addSignal ("Signal 1");

    plane->setSignalVisible (index, false);

    auto* signal = plane->getSignal (index);
    ASSERT_NE (nullptr, signal);
    EXPECT_FALSE (signal->visible);
}

TEST_F (CartesianPlaneTests, SetSignalColor)
{
    int index = plane->addSignal ("Signal 1");

    Color newColor (0xFF00FF00);
    plane->setSignalColor (index, newColor);

    auto* signal = plane->getSignal (index);
    ASSERT_NE (nullptr, signal);
    EXPECT_EQ (newColor, signal->color);
}

TEST_F (CartesianPlaneTests, SetSignalStrokeWidth)
{
    int index = plane->addSignal ("Signal 1");

    plane->setSignalStrokeWidth (index, 5.0f);

    auto* signal = plane->getSignal (index);
    ASSERT_NE (nullptr, signal);
    EXPECT_FLOAT_EQ (5.0f, signal->strokeWidth);
}

TEST_F (CartesianPlaneTests, ClearSignals)
{
    plane->addSignal ("Signal 1");
    plane->addSignal ("Signal 2");

    plane->clearSignals();

    EXPECT_EQ (0, plane->getNumSignals());
}

TEST_F (CartesianPlaneTests, UpdateSignalDataWithEmptyVector)
{
    int index = plane->addSignal ("Signal 1");

    std::vector<Point<double>> emptyData;
    plane->updateSignalData (index, emptyData);

    auto* signal = plane->getSignal (index);
    ASSERT_NE (nullptr, signal);
    EXPECT_EQ (0, static_cast<int> (signal->data.size()));
}

TEST_F (CartesianPlaneTests, UpdateSignalDataWithSinglePoint)
{
    int index = plane->addSignal ("Signal 1");

    std::vector<Point<double>> data;
    data.push_back ({ 0.5, 0.5 });

    plane->updateSignalData (index, data);

    auto* signal = plane->getSignal (index);
    ASSERT_NE (nullptr, signal);
    EXPECT_EQ (1, static_cast<int> (signal->data.size()));
}

TEST_F (CartesianPlaneTests, UpdateSignalDataWithManyPoints)
{
    int index = plane->addSignal ("Signal 1");

    std::vector<Point<double>> data;
    for (int i = 0; i < 1000; ++i)
        data.push_back ({ static_cast<double> (i) / 1000.0, static_cast<double> (i) / 1000.0 });

    plane->updateSignalData (index, data);

    auto* signal = plane->getSignal (index);
    ASSERT_NE (nullptr, signal);
    EXPECT_EQ (1000, static_cast<int> (signal->data.size()));
}

//==============================================================================
// Legend Tests
//==============================================================================

TEST_F (CartesianPlaneTests, LegendVisibleByDefault)
{
    EXPECT_TRUE (plane->isLegendVisible());
}

TEST_F (CartesianPlaneTests, SetLegendVisibleHidesLegend)
{
    plane->setLegendVisible (false);
    EXPECT_FALSE (plane->isLegendVisible());
}

TEST_F (CartesianPlaneTests, ToggleLegendVisibility)
{
    plane->setLegendVisible (false);
    EXPECT_FALSE (plane->isLegendVisible());

    plane->setLegendVisible (true);
    EXPECT_TRUE (plane->isLegendVisible());
}

TEST_F (CartesianPlaneTests, SetLegendPosition)
{
    Point<float> newPosition (0.5f, 0.5f);
    plane->setLegendPosition (newPosition);

    auto position = plane->getLegendPosition();
    EXPECT_FLOAT_EQ (0.5f, position.getX());
    EXPECT_FLOAT_EQ (0.5f, position.getY());
}

TEST_F (CartesianPlaneTests, SetLegendBackgroundColor)
{
    Color newColor (0x80FF0000);
    plane->setLegendBackgroundColor (newColor);

    EXPECT_EQ (newColor, plane->getLegendBackgroundColor());
}

//==============================================================================
// Coordinate Transformation Tests
//==============================================================================

TEST_F (CartesianPlaneTests, ValueToXConversion)
{
    plane->setXRange (0.0, 100.0);

    float x = plane->valueToX (50.0);

    // Should be somewhere in the middle of the plot area
    EXPECT_GT (x, 0.0f);
    EXPECT_LT (x, 800.0f);
}

TEST_F (CartesianPlaneTests, ValueToYConversion)
{
    plane->setYRange (0.0, 100.0);

    float y = plane->valueToY (50.0);

    // Should be somewhere in the middle of the plot area
    EXPECT_GT (y, 0.0f);
    EXPECT_LT (y, 600.0f);
}

TEST_F (CartesianPlaneTests, XToValueConversion)
{
    plane->setXRange (0.0, 100.0);

    double value = plane->xToValue (400.0f);

    // Should be approximately in the middle of the range
    EXPECT_GT (value, 0.0);
    EXPECT_LT (value, 100.0);
}

TEST_F (CartesianPlaneTests, YToValueConversion)
{
    plane->setYRange (0.0, 100.0);

    double value = plane->yToValue (300.0f);

    // Should be within the range
    EXPECT_GT (value, 0.0);
    EXPECT_LT (value, 100.0);
}

TEST_F (CartesianPlaneTests, GetPlotBoundsReturnsValidBounds)
{
    auto bounds = plane->getPlotBounds();

    EXPECT_GT (bounds.getWidth(), 0.0f);
    EXPECT_GT (bounds.getHeight(), 0.0f);
}

TEST_F (CartesianPlaneTests, CoordinateTransformationRoundTrip)
{
    plane->setXRange (0.0, 100.0);

    double originalValue = 50.0;
    float x = plane->valueToX (originalValue);
    double convertedValue = plane->xToValue (x);

    EXPECT_NEAR (originalValue, convertedValue, 1.0);
}

TEST_F (CartesianPlaneTests, ValuesArePlacedInProportionOnALinearScale)
{
    plane->setXRange (-10.0, 30.0);
    plane->setYRange (2.0, 6.0);

    const auto plot = plane->getPlotBounds();

    // The ends of the ranges are the edges of the plot
    EXPECT_FLOAT_EQ (plane->valueToX (-10.0), plot.getX());
    EXPECT_FLOAT_EQ (plane->valueToX (30.0), plot.getRight());
    EXPECT_FLOAT_EQ (plane->valueToY (2.0), plot.getBottom());
    EXPECT_FLOAT_EQ (plane->valueToY (6.0), plot.getY());

    // A quarter of the way along the ranges is a quarter of the way across the plot, y going up
    EXPECT_FLOAT_EQ (plane->valueToX (0.0), plot.getX() + 0.25f * plot.getWidth());
    EXPECT_FLOAT_EQ (plane->valueToY (3.0), plot.getBottom() - 0.25f * plot.getHeight());

    // Values out of the ranges are as far out of the plot
    EXPECT_FLOAT_EQ (plane->valueToX (-20.0), plot.getX() - 0.25f * plot.getWidth());
    EXPECT_FLOAT_EQ (plane->valueToY (7.0), plot.getY() - 0.25f * plot.getHeight());

    EXPECT_NEAR (plane->xToValue (plane->valueToX (12.5)), 12.5, 1.0e-3);
    EXPECT_NEAR (plane->yToValue (plane->valueToY (4.5)), 4.5, 1.0e-3);
}

TEST_F (CartesianPlaneTests, ValuesArePlacedByTheirLogarithmOnALogarithmicScale)
{
    plane->setXRange (10.0, 10000.0);
    plane->setYRange (1.0, 100.0);
    plane->setXScaleType (CartesianPlane::AxisScaleType::logarithmic);
    plane->setYScaleType (CartesianPlane::AxisScaleType::logarithmic);

    const auto plot = plane->getPlotBounds();

    // Every decade takes the same room
    EXPECT_NEAR (plane->valueToX (10.0), plot.getX(), 0.01f);
    EXPECT_NEAR (plane->valueToX (100.0), plot.getX() + plot.getWidth() / 3.0f, 0.01f);
    EXPECT_NEAR (plane->valueToX (1000.0), plot.getX() + plot.getWidth() * 2.0f / 3.0f, 0.01f);
    EXPECT_NEAR (plane->valueToX (10000.0), plot.getRight(), 0.01f);
    EXPECT_NEAR (plane->valueToY (1.0), plot.getBottom(), 0.01f);
    EXPECT_NEAR (plane->valueToY (10.0), plot.getCenterY(), 0.01f);
    EXPECT_NEAR (plane->valueToY (100.0), plot.getY(), 0.01f);

    // A value under the range is before the plot, and one with no logarithm at the start of its axis
    EXPECT_NEAR (plane->valueToX (1.0), plot.getX() - plot.getWidth() / 3.0f, 0.01f);
    EXPECT_FLOAT_EQ (plane->valueToX (0.0), plot.getX());
    EXPECT_FLOAT_EQ (plane->valueToX (-5.0), plot.getX());
    EXPECT_FLOAT_EQ (plane->valueToY (0.0), plot.getBottom());
    EXPECT_FLOAT_EQ (plane->valueToY (-5.0), plot.getBottom());

    EXPECT_NEAR (plane->xToValue (plane->valueToX (250.0)), 250.0, 0.1);
    EXPECT_NEAR (plane->yToValue (plane->valueToY (25.0)), 25.0, 0.01);
}

//==============================================================================
// Logarithmic Scale Tests
//==============================================================================

TEST_F (CartesianPlaneTests, LogarithmicScaleForXAxis)
{
    plane->setXScaleType (CartesianPlane::AxisScaleType::logarithmic);
    plane->setXRange (1.0, 1000.0);

    float x1 = plane->valueToX (10.0);
    float x2 = plane->valueToX (100.0);

    // With logarithmic scale, these should have meaningful spacing
    EXPECT_NE (x1, x2);
}

TEST_F (CartesianPlaneTests, LogarithmicScaleForYAxis)
{
    plane->setYScaleType (CartesianPlane::AxisScaleType::logarithmic);
    plane->setYRange (1.0, 1000.0);

    float y1 = plane->valueToY (10.0);
    float y2 = plane->valueToY (100.0);

    // With logarithmic scale, these should have meaningful spacing
    EXPECT_NE (y1, y2);
}

TEST_F (CartesianPlaneTests, ALogarithmicScaleAppliesWhileTheRangeIsAboveZero)
{
    const auto plot = plane->getPlotBounds();

    // Asked on a range starting at zero, the scales are logarithmic once their ranges allow it
    plane->setXScaleType (CartesianPlane::AxisScaleType::logarithmic);
    plane->setYScaleType (CartesianPlane::AxisScaleType::logarithmic);
    EXPECT_EQ (CartesianPlane::AxisScaleType::linear, plane->getXScaleType());
    EXPECT_EQ (CartesianPlane::AxisScaleType::linear, plane->getYScaleType());

    plane->setXRange (1.0, 100.0);
    plane->setYRange (1.0, 100.0);
    EXPECT_EQ (CartesianPlane::AxisScaleType::logarithmic, plane->getXScaleType());
    EXPECT_EQ (CartesianPlane::AxisScaleType::logarithmic, plane->getYScaleType());

    EXPECT_NEAR (plane->valueToX (10.0), plot.getCenterX(), 0.01f);
    EXPECT_NEAR (plane->valueToY (10.0), plot.getCenterY(), 0.01f);
    EXPECT_NEAR (plane->xToValue (plot.getCenterX()), 10.0, 0.01);
    EXPECT_NEAR (plane->yToValue (plot.getCenterY()), 10.0, 0.01);

    // Ranges moved back down to zero are told linearly again, rather than not at all
    plane->setXRange (0.0, 100.0);
    plane->setYRange (0.0, 100.0);
    EXPECT_EQ (CartesianPlane::AxisScaleType::linear, plane->getXScaleType());
    EXPECT_EQ (CartesianPlane::AxisScaleType::linear, plane->getYScaleType());

    EXPECT_NEAR (plane->valueToX (50.0), plot.getCenterX(), 0.01f);
    EXPECT_NEAR (plane->valueToY (50.0), plot.getCenterY(), 0.01f);
    EXPECT_NEAR (plane->xToValue (plot.getCenterX()), 50.0, 0.01);
    EXPECT_NEAR (plane->yToValue (plot.getCenterY()), 50.0, 0.01);
}

TEST_F (CartesianPlaneTests, APlaneNoLargerThanItsMarginsHasAnEmptyPlot)
{
    plane->setXRange (20.0, 20000.0);
    plane->setYRange (-60.0, 6.0);

    // The margins take 80 by 55: nothing is left for the plot, which is no less than empty
    for (const auto size : { Size<float> (80.0f, 55.0f), Size<float> (40.0f, 30.0f) })
    {
        plane->setBounds (0.0f, 0.0f, size.getWidth(), size.getHeight());

        const auto plot = plane->getPlotBounds();
        EXPECT_FLOAT_EQ (plot.getWidth(), 0.0f);
        EXPECT_FLOAT_EQ (plot.getHeight(), 0.0f);

        // And every position of it is the start of the ranges
        EXPECT_DOUBLE_EQ (plane->xToValue (plot.getX()), 20.0);
        EXPECT_DOUBLE_EQ (plane->yToValue (plot.getBottom()), -60.0);
        EXPECT_TRUE (std::isfinite (plane->valueToX (1000.0)));
        EXPECT_TRUE (std::isfinite (plane->valueToY (0.0)));
    }
}

//==============================================================================
// Paint Tests
//==============================================================================

TEST_F (CartesianPlaneTests, PaintWithoutCrashing)
{
    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 600);
    Graphics g (*context, *renderer);

    plane->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, PaintWithSignals)
{
    int index = plane->addSignal ("Test Signal");
    std::vector<Point<double>> data;
    data.push_back ({ 0.0, 0.0 });
    data.push_back ({ 1.0, 1.0 });
    plane->updateSignalData (index, data);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 600);
    Graphics g (*context, *renderer);

    plane->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, PaintWithGridLines)
{
    plane->addVerticalGridLine (0.5);
    plane->addHorizontalGridLine (0.5);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 600);
    Graphics g (*context, *renderer);

    plane->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, PaintWithLabels)
{
    plane->addXAxisLabel (0.0, "0");
    plane->addXAxisLabel (1.0, "1");
    plane->addYAxisLabel (0.0, "0");
    plane->addYAxisLabel (1.0, "1");

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 600);
    Graphics g (*context, *renderer);

    plane->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, PaintWithTitle)
{
    plane->setTitle ("Test Plot");

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 600);
    Graphics g (*context, *renderer);

    plane->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, PaintWithLegend)
{
    plane->addSignal ("Signal 1", Colors::red);
    plane->addSignal ("Signal 2", Colors::blue);
    plane->setLegendVisible (true);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 600);
    Graphics g (*context, *renderer);

    plane->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, PaintWithZeroSize)
{
    plane->setBounds (0.0f, 0.0f, 0.0f, 0.0f);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (1, 1);
    Graphics g (*context, *renderer);

    plane->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, PaintsALegendThatDoesNotFitInThePlot)
{
    plane->addSignal ("Signal 1", Colors::red);
    plane->addSignal ("Signal 2", Colors::blue);
    plane->setLegendVisible (true);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (200, 100);
    Graphics g (*context, *renderer);

    // A plot narrower and lower than its legend, then one with no room at all
    for (const auto size : { Size<float> (150.0f, 75.0f), Size<float> (60.0f, 40.0f) })
    {
        plane->setBounds (0.0f, 0.0f, size.getWidth(), size.getHeight());
        plane->paint (g);
    }

    EXPECT_GE (plane->getPlotBounds().getWidth(), 0.0f);
    EXPECT_GE (plane->getPlotBounds().getHeight(), 0.0f);
}

TEST_F (CartesianPlaneTests, PaintsASignalDenserThanThePixelsOfThePlotAndPaintsItAgain)
{
    plane->setXRange (0.0, 1.0);
    plane->setYRange (-1.0, 1.0);
    plane->setXScaleType (CartesianPlane::AxisScaleType::logarithmic);

    // Many points for each pixel of the plot, some of them over and under it
    const auto makeData = [] (double level)
    {
        std::vector<Point<double>> data;

        for (int i = 0; i < 20000; ++i)
            data.push_back ({ i / 19999.0, level * std::sin (i * 0.37) });

        return data;
    };

    const int index = plane->addSignal ("Dense");
    plane->updateSignalData (index, makeData (1.5));

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 600);
    Graphics g (*context, *renderer);

    // The second paint finds the signal as the first one left it, and the ones after a change make it again
    plane->paint (g);
    plane->paint (g);

    plane->updateSignalData (index, makeData (0.5));
    plane->paint (g);

    plane->setXRange (0.001, 1.0);
    plane->paint (g);

    g.setTransform (AffineTransform::scaling (0.5f));
    plane->paint (g);

    plane->clearSignals();
    plane->paint (g);

    EXPECT_EQ (0, plane->getNumSignals());
}

//==============================================================================
// Edge Cases
//==============================================================================

TEST_F (CartesianPlaneTests, MultipleSignalsWithDifferentColors)
{
    plane->addSignal ("Red Signal", Colors::red, 1.0f);
    plane->addSignal ("Green Signal", Colors::green, 2.0f);
    plane->addSignal ("Blue Signal", Colors::blue, 3.0f);

    EXPECT_EQ (3, plane->getNumSignals());

    auto* signal1 = plane->getSignal (0);
    auto* signal2 = plane->getSignal (1);
    auto* signal3 = plane->getSignal (2);

    EXPECT_EQ (Colors::red, signal1->color);
    EXPECT_EQ (Colors::green, signal2->color);
    EXPECT_EQ (Colors::blue, signal3->color);
}

TEST_F (CartesianPlaneTests, ComplexGridConfiguration)
{
    plane->setVerticalGridLines ({ 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9 });
    plane->setHorizontalGridLines ({ 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9 });

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, SignalDataOutsideRange)
{
    plane->setXRange (0.0, 1.0);
    plane->setYRange (0.0, 1.0);

    int index = plane->addSignal ("Out of bounds");

    std::vector<Point<double>> data;
    data.push_back ({ -1.0, -1.0 });
    data.push_back ({ 2.0, 2.0 });

    plane->updateSignalData (index, data);

    // Should handle gracefully
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, NegativeAxisRanges)
{
    plane->setXRange (-100.0, -10.0);
    plane->setYRange (-50.0, 50.0);

    int index = plane->addSignal ("Negative Range");
    std::vector<Point<double>> data;
    data.push_back ({ -50.0, 0.0 });

    plane->updateSignalData (index, data);

    // Should handle gracefully
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, ZeroRangeHandling)
{
    plane->setXRange (5.0, 5.0);
    plane->setYRange (5.0, 5.0);

    // Should handle gracefully
    EXPECT_TRUE (true);
}

//==============================================================================
// Type Info Tests
//==============================================================================

TEST_F (CartesianPlaneTests, TheTypeInfoSetsTheRangesInAnyOrderAndTheScalesAfterThem)
{
    const auto& type = CartesianPlane::getClassTypeInfo();
    EXPECT_EQ (type.getTypeName().toString(), "CartesianPlane");
    EXPECT_EQ (&plane->getTypeInfo(), &type);

    const auto created = type.createInstance();
    auto* instance = dynamic_cast<CartesianPlane*> (created.get());
    ASSERT_NE (nullptr, instance);

    // The title is the one the plane draws
    instance->setProperty ("title", "Response");
    EXPECT_EQ (instance->getTitle(), "Response");
    EXPECT_EQ (instance->getProperty ("title").toString(), "Response");

    // An end written past the other one moves it out of its way, one written before it leaves it alone
    instance->setProperty ("xMin", 20.0);
    instance->setProperty ("xMax", 20000.0);
    EXPECT_DOUBLE_EQ (20.0, instance->getXRange().getStart());
    EXPECT_DOUBLE_EQ (20000.0, instance->getXRange().getEnd());

    instance->setProperty ("yMax", -6.0);
    instance->setProperty ("yMin", -60.0);
    EXPECT_DOUBLE_EQ (-60.0, instance->getYRange().getStart());
    EXPECT_DOUBLE_EQ (-6.0, instance->getYRange().getEnd());

    // A scale is logarithmic for a range above zero only
    instance->setProperty ("xScaleType", "logarithmic");
    instance->setProperty ("yScaleType", "logarithmic");
    EXPECT_EQ (CartesianPlane::AxisScaleType::logarithmic, instance->getXScaleType());
    EXPECT_EQ (CartesianPlane::AxisScaleType::linear, instance->getYScaleType());

    instance->setProperty ("legendVisible", false);
    EXPECT_FALSE (instance->isLegendVisible());
}

//==============================================================================
// GPU Path Tests (skipped when no GPU backend is available)
//==============================================================================

class CartesianPlaneGpuTests : public ::testing::Test
{
protected:
    static constexpr int width = 400;
    static constexpr int height = 300;

    static void SetUpTestSuite()
    {
        gpuContext = GraphicsContext::createContext (GpuPlatform::Metal, {});
        if (gpuContext == nullptr)
            return;

        if (GpuCanvas::create (*gpuContext, 64, 64) == nullptr)
            gpuContext.reset();
    }

    static void TearDownTestSuite()
    {
        gpuContext.reset();
    }

    void SetUp() override
    {
        if (gpuContext == nullptr)
            GTEST_SKIP() << "No Metal GPU context available";

        canvas = GpuCanvas::create (*gpuContext, width, height);
        if (canvas == nullptr)
            GTEST_SKIP() << "Unable to create paint GpuCanvas";

        // A plot taking the whole plane, with nothing on it but a red signal over black
        plane = std::make_unique<CartesianPlane>();
        plane->setBounds (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height));
        plane->setMargins (0, 0, 0, 0);
        plane->setLegendVisible (false);
        plane->setBackgroundColor (Colors::black);
        signalIndex = plane->addSignal ("Signal", Color (0xffff0000), 4.0f);
    }

    void TearDown() override
    {
        plane.reset();
        canvas.reset();
    }

    /** Paints the plane into the canvas, at a number of pixels a unit, and reads its pixels back. */
    bool paintPlane (float pixelsPerUnit = 1.0f)
    {
        plane->paint (canvas->beginDraw ({}, pixelsPerUnit));
        canvas->commit();

        pixels.assign (static_cast<size_t> (width) * static_cast<size_t> (height) * 4u, 0);
        return canvas->readPixels (pixels.data(), pixels.size());
    }

    /** Whether the signal covers a pixel of the canvas, as it was last painted. */
    bool isSignalAt (int x, int y) const
    {
        return pixels[(static_cast<size_t> (y) * static_cast<size_t> (width) + static_cast<size_t> (x)) * 4u] > 128;
    }

    /** A level line of two points, from 5% to 95% of the default range of x. */
    static std::vector<Point<double>> makeLine (double level)
    {
        return { { 0.05, level }, { 0.95, level } };
    }

    /** Many points for each pixel, going from a level to another and back, from 5% to 95% of the default range of x. */
    static std::vector<Point<double>> makeZigzag (double low, double high)
    {
        std::vector<Point<double>> data;

        for (int i = 0; i < 16000; ++i)
            data.push_back ({ 0.05 + 0.9 * i / 15999.0, i % 2 == 0 ? low : high });

        return data;
    }

    static std::unique_ptr<GraphicsContext> gpuContext;

    GpuCanvas::Ptr canvas;
    std::unique_ptr<CartesianPlane> plane;
    std::vector<uint8> pixels;
    int signalIndex = -1;
};

std::unique_ptr<GraphicsContext> CartesianPlaneGpuTests::gpuContext;

TEST_F (CartesianPlaneGpuTests, PaintsASparseSignalAndADenseOneWhereTheirValuesAre)
{
    // Two points: a line a quarter of the way up the plot
    plane->updateSignalData (signalIndex, makeLine (0.25));
    ASSERT_TRUE (paintPlane());

    for (const auto x : { 30, 200, 370 })
    {
        EXPECT_TRUE (isSignalAt (x, 225)) << "at x = " << x;
        EXPECT_FALSE (isSignalAt (x, 150)) << "at x = " << x;
        EXPECT_FALSE (isSignalAt (x, 75)) << "at x = " << x;
    }

    EXPECT_FALSE (isSignalAt (5, 225));
    EXPECT_FALSE (isSignalAt (395, 225));

    // 16000 points going between a quarter and three quarters of the way up: the band they cover
    plane->updateSignalData (signalIndex, makeZigzag (0.25, 0.75));
    ASSERT_TRUE (paintPlane());

    for (const auto x : { 30, 200, 370 })
    {
        for (const auto y : { 85, 150, 215 })
            EXPECT_TRUE (isSignalAt (x, y)) << "at " << x << ", " << y;

        EXPECT_FALSE (isSignalAt (x, 30)) << "at x = " << x;
        EXPECT_FALSE (isSignalAt (x, 270)) << "at x = " << x;
    }

    EXPECT_FALSE (isSignalAt (5, 150));
    EXPECT_FALSE (isSignalAt (395, 150));
}

TEST_F (CartesianPlaneGpuTests, ADenseSignalIsOnlyPaintedWhereItIsInsideThePlot)
{
    // Level across the plot, but for its middle third which is far over it
    std::vector<Point<double>> data;

    for (int i = 0; i < 16000; ++i)
    {
        const auto position = i / 15999.0;
        data.push_back ({ 0.05 + 0.9 * position, position > 0.35 && position < 0.65 ? 5.0 : 0.5 });
    }

    plane->updateSignalData (signalIndex, data);
    ASSERT_TRUE (paintPlane());

    EXPECT_TRUE (isSignalAt (60, 150));
    EXPECT_TRUE (isSignalAt (340, 150));
    EXPECT_FALSE (isSignalAt (200, 150));
    EXPECT_FALSE (isSignalAt (200, 20));
}

TEST_F (CartesianPlaneGpuTests, ASignalIsPaintedAgainWhereItsNewDataIs)
{
    plane->updateSignalData (signalIndex, makeLine (0.25));
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (200, 225));

    plane->updateSignalData (signalIndex, makeLine (0.75));
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (200, 75));
    EXPECT_FALSE (isSignalAt (200, 225));

    // A signal added once the others were cleared is not the one that was painted in its place
    plane->clearSignals();
    const auto added = plane->addSignal ("Other", Color (0xffff0000), 4.0f);
    plane->updateSignalData (added, makeLine (0.5));
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (200, 150));
    EXPECT_FALSE (isSignalAt (200, 75));

    // What is kept of a signal is not painted while it is hidden
    plane->setSignalVisible (added, false);
    ASSERT_TRUE (paintPlane());
    EXPECT_FALSE (isSignalAt (200, 150));

    plane->setSignalVisible (added, true);
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (200, 150));
}

TEST_F (CartesianPlaneGpuTests, ASignalIsPaintedAgainWhereTheRangesOfItsAxesPlaceIt)
{
    plane->updateSignalData (signalIndex, makeLine (0.25));
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (300, 225));

    // Twice the range of y: half as high
    plane->setYRange (0.0, 2.0);
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (300, 262));
    EXPECT_FALSE (isSignalAt (300, 225));

    // Twice the range of x: half as long
    plane->setXRange (0.0, 2.0);
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (100, 262));
    EXPECT_FALSE (isSignalAt (300, 262));
}

TEST_F (CartesianPlaneGpuTests, ASignalIsPaintedAgainWhereTheScalesOfItsAxesPlaceIt)
{
    plane->setXRange (1.0, 100.0);
    plane->setYRange (1.0, 100.0);
    plane->updateSignalData (signalIndex, { { 2.0, 10.0 }, { 10.0, 10.0 } });
    ASSERT_TRUE (paintPlane());

    // Linear: from 4 to 36 pixels along x, 27 pixels up
    EXPECT_TRUE (isSignalAt (20, 272));
    EXPECT_FALSE (isSignalAt (20, 150));

    // 10 is half the way from 1 to 100 on a logarithmic scale
    plane->setYScaleType (CartesianPlane::AxisScaleType::logarithmic);
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (20, 150));
    EXPECT_FALSE (isSignalAt (20, 272));
    EXPECT_FALSE (isSignalAt (150, 150));

    // And 2 a little under a sixth of it: from 60 to 200 pixels
    plane->setXScaleType (CartesianPlane::AxisScaleType::logarithmic);
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (150, 150));
    EXPECT_FALSE (isSignalAt (20, 150));
}

TEST_F (CartesianPlaneGpuTests, ASignalIsPaintedAgainWhereThePlotIs)
{
    plane->updateSignalData (signalIndex, makeLine (0.25));
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (300, 225));

    // A margin at the top leaves the plot two thirds of the height
    plane->setMargins (100, 0, 0, 0);
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (300, 250));
    EXPECT_FALSE (isSignalAt (300, 225));

    // And a plane half as wide half of the width
    plane->setBounds (0.0f, 0.0f, 200.0f, static_cast<float> (height));
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (100, 250));
    EXPECT_FALSE (isSignalAt (300, 250));
}

TEST_F (CartesianPlaneGpuTests, ADenseSignalIsPaintedForThePixelsOfTheScaleItIsPaintedAt)
{
    // Half the canvas each way, which it fills at two pixels a unit
    plane->setBounds (0.0f, 0.0f, 200.0f, 150.0f);
    plane->updateSignalData (signalIndex, makeZigzag (0.25, 0.75));

    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (100, 75));
    EXPECT_FALSE (isSignalAt (100, 20));
    EXPECT_FALSE (isSignalAt (300, 150));

    ASSERT_TRUE (paintPlane (2.0f));
    EXPECT_TRUE (isSignalAt (200, 150));
    EXPECT_TRUE (isSignalAt (300, 100));
    EXPECT_TRUE (isSignalAt (300, 200));
    EXPECT_FALSE (isSignalAt (300, 40));
    EXPECT_FALSE (isSignalAt (300, 260));

    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (100, 75));
    EXPECT_FALSE (isSignalAt (300, 150));
}
