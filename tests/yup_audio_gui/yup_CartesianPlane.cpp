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
    EXPECT_EQ (80, margins.getX());
    EXPECT_EQ (40, margins.getY());
}

TEST_F (CartesianPlaneTests, SetMarginsWithZeroValues)
{
    plane->setMargins (0, 0, 0, 0);

    auto margins = plane->getMargins();
    EXPECT_EQ (0, margins.getX());
    EXPECT_EQ (0, margins.getY());
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
