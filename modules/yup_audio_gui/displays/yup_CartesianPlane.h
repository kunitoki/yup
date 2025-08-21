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

namespace yup
{

//==============================================================================
/** A flexible Cartesian coordinate plotting component

    This component provides a configurable 2D plotting area with:
    - Customizable X and Y axis ranges and scaling (linear/logarithmic)
    - Configurable margins for labels and title
    - Multiple signal plotting with custom colors and stroke widths
    - Customizable grid lines and labels
    - Legend support
    - Title with configurable font, size, and position
*/
class YUP_API CartesianPlane : public Component
{
public:
    //==============================================================================

    /** Configuration for axis scaling behavior. */
    enum class AxisScaleType
    {
        linear,
        logarithmic
    };

    //==============================================================================
    /** A signal data container for plotting on CartesianPlane. */
    struct YUP_API PlotSignal
    {
        String name;
        std::vector<Point<double>> data;
        Color color { Colors::white };
        float strokeWidth { 2.0f };
        bool visible { true };

        PlotSignal() = default;

        PlotSignal (const String& signalName, const Color& signalColor = Colors::white, float width = 2.0f)
            : name (signalName)
            , color (signalColor)
            , strokeWidth (width)
        {
        }
    };

    //==============================================================================
    /** Grid line configuration. */
    struct YUP_API GridLine
    {
        double value;
        Color color { Color (0xFF333333) };
        float strokeWidth { 1.0f };
        bool emphasize { false };

        GridLine() = default;

        GridLine (double val, const Color& col = Color (0xFF333333), float width = 1.0f, bool emp = false)
            : value (val)
            , color (col)
            , strokeWidth (width)
            , emphasize (emp)
        {
        }
    };

    //==============================================================================
    /** Axis label configuration. */
    struct YUP_API AxisLabel
    {
        double value;
        String text;
        Color color { Colors::white };
        float fontSize { 10.0f };

        AxisLabel() = default;

        AxisLabel (double val, const String& labelText, const Color& col = Colors::white, float size = 10.0f)
            : value (val)
            , text (labelText)
            , color (col)
            , fontSize (size)
        {
        }
    };

    //==============================================================================

    CartesianPlane();
    ~CartesianPlane() override = default;

    //==============================================================================
    // Axis configuration

    /** Set the range for the X axis */
    void setXRange (double minX, double maxX);

    /** Set the range for the Y axis */
    void setYRange (double minY, double maxY);

    /** Get the current X axis range */
    Range<double> getXRange() const { return { xMin, xMax }; }

    /** Get the current Y axis range */
    Range<double> getYRange() const { return { yMin, yMax }; }

    /** Set the scaling type for X axis */
    void setXScaleType (AxisScaleType scaleType);

    /** Set the scaling type for Y axis */
    void setYScaleType (AxisScaleType scaleType);

    /** Get the X axis scale type */
    AxisScaleType getXScaleType() const { return xScaleType; }

    /** Get the Y axis scale type */
    AxisScaleType getYScaleType() const { return yScaleType; }

    //==============================================================================
    // Margins configuration

    /** Set margins around the plot area */
    void setMargins (int top, int left, int bottom, int right);

    /** Get current margins */
    Rectangle<int> getMargins() const { return { marginLeft, marginTop, marginRight - marginLeft, marginBottom - marginTop }; }

    //==============================================================================
    // Title configuration

    /** Set the plot title */
    void setTitle (const String& title);

    /** Get the current title */
    const String& getTitle() const { return titleText; }

    /** Set title font and size */
    void setTitleFont (const Font& font);

    /** Get title font */
    const Font& getTitleFont() const { return titleFont; }

    /** Set title color */
    void setTitleColor (const Color& color);

    /** Get title color */
    const Color& getTitleColor() const { return titleColor; }

    /** Set title justification */
    void setTitleJustification (Justification justification);

    /** Get title justification */
    Justification getTitleJustification() const { return titleJustification; }

    //==============================================================================
    // Background and colors

    /** Set background color */
    void setBackgroundColor (const Color& color);

    /** Get background color */
    const Color& getBackgroundColor() const { return backgroundColor; }

    //==============================================================================
    // Grid lines

    /** Clear all vertical grid lines */
    void clearVerticalGridLines();

    /** Add a vertical grid line */
    void addVerticalGridLine (double value, const Color& color = Color (0xFF333333), float strokeWidth = 1.0f, bool emphasize = false);

    /** Set vertical grid lines from a list of values */
    void setVerticalGridLines (const std::vector<double>& values, const Color& color = Color (0xFF333333), float strokeWidth = 1.0f);

    /** Clear all horizontal grid lines */
    void clearHorizontalGridLines();

    /** Add a horizontal grid line */
    void addHorizontalGridLine (double value, const Color& color = Color (0xFF333333), float strokeWidth = 1.0f, bool emphasize = false);

    /** Set horizontal grid lines from a list of values */
    void setHorizontalGridLines (const std::vector<double>& values, const Color& color = Color (0xFF333333), float strokeWidth = 1.0f);

    //==============================================================================
    // Axis labels

    /** Clear all X axis labels */
    void clearXAxisLabels();

    /** Add an X axis label */
    void addXAxisLabel (double value, const String& text, const Color& color = Colors::white, float fontSize = 10.0f);

    /** Set X axis labels from values with automatic text formatting */
    void setXAxisLabels (const std::vector<double>& values, const Color& color = Colors::white, float fontSize = 10.0f);

    /** Clear all Y axis labels */
    void clearYAxisLabels();

    /** Add a Y axis label */
    void addYAxisLabel (double value, const String& text, const Color& color = Colors::white, float fontSize = 10.0f);

    /** Set Y axis labels from values with automatic text formatting */
    void setYAxisLabels (const std::vector<double>& values, const Color& color = Colors::white, float fontSize = 10.0f);

    //==============================================================================
    // Signals

    /** Clear all signals */
    void clearSignals();

    /** Add a signal to plot */
    int addSignal (const String& name, const Color& color = Colors::white, float strokeWidth = 2.0f);

    /** Update signal data */
    void updateSignalData (int signalIndex, const std::vector<Point<double>>& data);

    /** Set signal visibility */
    void setSignalVisible (int signalIndex, bool visible);

    /** Set signal color */
    void setSignalColor (int signalIndex, const Color& color);

    /** Set signal stroke width */
    void setSignalStrokeWidth (int signalIndex, float strokeWidth);

    /** Get number of signals */
    int getNumSignals() const { return static_cast<int> (signals.size()); }

    /** Get signal by index */
    const PlotSignal* getSignal (int index) const;

    //==============================================================================
    // Legend

    /** Enable or disable legend */
    void setLegendVisible (bool visible);

    /** Check if legend is visible */
    bool isLegendVisible() const { return showLegend; }

    /** Set legend position (as a fraction of the plot area) */
    void setLegendPosition (Point<float> position);

    /** Get legend position */
    Point<float> getLegendPosition() const { return legendPosition; }

    /** Set legend background color */
    void setLegendBackgroundColor (const Color& color);

    /** Get legend background color */
    const Color& getLegendBackgroundColor() const { return legendBackgroundColor; }

    //==============================================================================
    // Coordinate transformations

    /** Convert X value to screen coordinate */
    float valueToX (double value) const;

    /** Convert Y value to screen coordinate */
    float valueToY (double value) const;

    /** Convert screen X coordinate to value */
    double xToValue (float x) const;

    /** Convert screen Y coordinate to value */
    double yToValue (float y) const;

    /** Get the plotting bounds (excludes margins) */
    Rectangle<float> getPlotBounds() const;

    //==============================================================================
    // Component overrides

    void paint (Graphics& g) override;

private:
    //==============================================================================

    void drawBackground (Graphics& g);
    void drawGrid (Graphics& g, const Rectangle<float>& bounds);
    void drawSignals (Graphics& g, const Rectangle<float>& bounds);
    void drawAxisLabels (Graphics& g, const Rectangle<float>& bounds);
    void drawTitle (Graphics& g);
    void drawLegend (Graphics& g, const Rectangle<float>& bounds);

    String formatAxisValue (double value, AxisScaleType scaleType) const;
    int determineAxisPrecision (const std::vector<double>& values, AxisScaleType scaleType) const;
    String formatAxisValueWithPrecision (double value, AxisScaleType scaleType, int precision) const;
    std::optional<Point<float>> findBoundsIntersection (const Point<float>& p1, const Point<float>& p2, const Rectangle<float>& bounds) const;

    //==============================================================================

    // Axis configuration
    double xMin { 0.0 }, xMax { 1.0 };
    double yMin { 0.0 }, yMax { 1.0 };
    AxisScaleType xScaleType { AxisScaleType::linear };
    AxisScaleType yScaleType { AxisScaleType::linear };

    // Margins
    int marginTop { 30 }, marginLeft { 60 }, marginBottom { 25 }, marginRight { 20 };

    // Title
    String titleText;
    Font titleFont;
    Color titleColor { Colors::white };
    Justification titleJustification { Justification::center };

    // Colors
    Color backgroundColor { Color (0xFF1E1E1E) };

    // Grid lines
    std::vector<GridLine> verticalGridLines;
    std::vector<GridLine> horizontalGridLines;

    // Axis labels
    std::vector<AxisLabel> xAxisLabels;
    std::vector<AxisLabel> yAxisLabels;

    // Signals
    std::vector<PlotSignal> signals;

    // Legend
    bool showLegend { true };
    Point<float> legendPosition { 0.8f, 0.1f };
    Color legendBackgroundColor { Color (0x80000000) };

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CartesianPlane)
};

} // namespace yup