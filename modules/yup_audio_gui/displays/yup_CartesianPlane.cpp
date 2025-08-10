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

CartesianPlane::CartesianPlane()
{
    setOpaque (true);

    auto defaultFont = ApplicationTheme::getGlobalTheme()->getDefaultFont();
    titleFont = defaultFont.withHeight (14.0f);
}

//==============================================================================
// Axis configuration

void CartesianPlane::setXRange (double minX, double maxX)
{
    if (minX >= maxX)
        return;

    this->xMin = minX;
    this->xMax = maxX;
    repaint();
}

void CartesianPlane::setYRange (double minY, double maxY)
{
    if (minY >= maxY)
        return;

    this->yMin = minY;
    this->yMax = maxY;
    repaint();
}

void CartesianPlane::setXScaleType (AxisScaleType scaleType)
{
    if (scaleType == AxisScaleType::logarithmic && xMin <= 0.0)
        return; // Cannot use log scale with non-positive values

    xScaleType = scaleType;
    repaint();
}

void CartesianPlane::setYScaleType (AxisScaleType scaleType)
{
    if (scaleType == AxisScaleType::logarithmic && yMin <= 0.0)
        return; // Cannot use log scale with non-positive values

    yScaleType = scaleType;
    repaint();
}

//==============================================================================
// Margins configuration

void CartesianPlane::setMargins (int top, int left, int bottom, int right)
{
    marginTop = jmax (0, top);
    marginLeft = jmax (0, left);
    marginBottom = jmax (0, bottom);
    marginRight = jmax (0, right);
    repaint();
}

//==============================================================================
// Title configuration

void CartesianPlane::setTitle (const String& title)
{
    titleText = title;
    repaint();
}

void CartesianPlane::setTitleFont (const Font& font)
{
    titleFont = font;
    repaint();
}

void CartesianPlane::setTitleColor (const Color& color)
{
    titleColor = color;
    repaint();
}

void CartesianPlane::setTitleJustification (Justification justification)
{
    titleJustification = justification;
    repaint();
}

//==============================================================================
// Background and colors

void CartesianPlane::setBackgroundColor (const Color& color)
{
    backgroundColor = color;
    repaint();
}

//==============================================================================
// Grid lines

void CartesianPlane::clearVerticalGridLines()
{
    verticalGridLines.clear();
    repaint();
}

void CartesianPlane::addVerticalGridLine (double value, const Color& color, float strokeWidth, bool emphasize)
{
    verticalGridLines.emplace_back (value, color, strokeWidth, emphasize);
    repaint();
}

void CartesianPlane::setVerticalGridLines (const std::vector<double>& values, const Color& color, float strokeWidth)
{
    verticalGridLines.clear();
    for (auto value : values)
        verticalGridLines.emplace_back (value, color, strokeWidth, false);
    repaint();
}

void CartesianPlane::clearHorizontalGridLines()
{
    horizontalGridLines.clear();
    repaint();
}

void CartesianPlane::addHorizontalGridLine (double value, const Color& color, float strokeWidth, bool emphasize)
{
    horizontalGridLines.emplace_back (value, color, strokeWidth, emphasize);
    repaint();
}

void CartesianPlane::setHorizontalGridLines (const std::vector<double>& values, const Color& color, float strokeWidth)
{
    horizontalGridLines.clear();
    for (auto value : values)
        horizontalGridLines.emplace_back (value, color, strokeWidth, false);
    repaint();
}

//==============================================================================
// Axis labels

void CartesianPlane::clearXAxisLabels()
{
    xAxisLabels.clear();
    repaint();
}

void CartesianPlane::addXAxisLabel (double value, const String& text, const Color& color, float fontSize)
{
    xAxisLabels.emplace_back (value, text, color, fontSize);
    repaint();
}

void CartesianPlane::setXAxisLabels (const std::vector<double>& values, const Color& color, float fontSize)
{
    xAxisLabels.clear();
    for (auto value : values)
    {
        String text = formatAxisValue (value, xScaleType);
        xAxisLabels.emplace_back (value, text, color, fontSize);
    }
    repaint();
}

void CartesianPlane::clearYAxisLabels()
{
    yAxisLabels.clear();
    repaint();
}

void CartesianPlane::addYAxisLabel (double value, const String& text, const Color& color, float fontSize)
{
    yAxisLabels.emplace_back (value, text, color, fontSize);
    repaint();
}

void CartesianPlane::setYAxisLabels (const std::vector<double>& values, const Color& color, float fontSize)
{
    yAxisLabels.clear();
    for (auto value : values)
    {
        String text = formatAxisValue (value, yScaleType);
        yAxisLabels.emplace_back (value, text, color, fontSize);
    }
    repaint();
}

//==============================================================================
// Signals

void CartesianPlane::clearSignals()
{
    signals.clear();
    repaint();
}

int CartesianPlane::addSignal (const String& name, const Color& color, float strokeWidth)
{
    signals.emplace_back (name, color, strokeWidth);
    repaint();
    return static_cast<int> (signals.size() - 1);
}

void CartesianPlane::updateSignalData (int signalIndex, const std::vector<Point<double>>& data)
{
    if (isPositiveAndBelow (signalIndex, signals.size()))
    {
        signals[static_cast<size_t> (signalIndex)].data = data;
        repaint();
    }
}

void CartesianPlane::setSignalVisible (int signalIndex, bool visible)
{
    if (isPositiveAndBelow (signalIndex, signals.size()))
    {
        signals[static_cast<size_t> (signalIndex)].visible = visible;
        repaint();
    }
}

void CartesianPlane::setSignalColor (int signalIndex, const Color& color)
{
    if (isPositiveAndBelow (signalIndex, signals.size()))
    {
        signals[static_cast<size_t> (signalIndex)].color = color;
        repaint();
    }
}

void CartesianPlane::setSignalStrokeWidth (int signalIndex, float strokeWidth)
{
    if (isPositiveAndBelow (signalIndex, signals.size()))
    {
        signals[static_cast<size_t> (signalIndex)].strokeWidth = strokeWidth;
        repaint();
    }
}

const CartesianPlane::PlotSignal* CartesianPlane::getSignal (int index) const
{
    if (isPositiveAndBelow (index, signals.size()))
        return &signals[static_cast<size_t> (index)];
    return nullptr;
}

//==============================================================================
// Legend

void CartesianPlane::setLegendVisible (bool visible)
{
    showLegend = visible;
    repaint();
}

void CartesianPlane::setLegendPosition (Point<float> position)
{
    legendPosition = position;
    repaint();
}

void CartesianPlane::setLegendBackgroundColor (const Color& color)
{
    legendBackgroundColor = color;
    repaint();
}

//==============================================================================
// Coordinate transformations

float CartesianPlane::valueToX (double value) const
{
    auto bounds = getPlotBounds();

    double normalised = 0.0;
    if (xScaleType == AxisScaleType::logarithmic)
    {
        if (value <= 0.0 || xMin <= 0.0 || xMax <= 0.0)
            return bounds.getX();

        double logValue = std::log10 (value);
        double logMin = std::log10 (xMin);
        double logMax = std::log10 (xMax);
        normalised = (logValue - logMin) / (logMax - logMin);
    }
    else
    {
        normalised = (value - xMin) / (xMax - xMin);
    }

    return bounds.getX() + static_cast<float> (normalised * bounds.getWidth());
}

float CartesianPlane::valueToY (double value) const
{
    auto bounds = getPlotBounds();

    double normalised = 0.0;
    if (yScaleType == AxisScaleType::logarithmic)
    {
        if (value <= 0.0 || yMin <= 0.0 || yMax <= 0.0)
            return bounds.getBottom();

        double logValue = std::log10 (value);
        double logMin = std::log10 (yMin);
        double logMax = std::log10 (yMax);
        normalised = (logValue - logMin) / (logMax - logMin);
    }
    else
    {
        normalised = (value - yMin) / (yMax - yMin);
    }

    return bounds.getBottom() - static_cast<float> (normalised * bounds.getHeight());
}

double CartesianPlane::xToValue (float x) const
{
    auto bounds = getPlotBounds();
    double normalised = (x - bounds.getX()) / bounds.getWidth();

    if (xScaleType == AxisScaleType::logarithmic)
    {
        double logMin = std::log10 (xMin);
        double logMax = std::log10 (xMax);
        double logValue = logMin + normalised * (logMax - logMin);
        return std::pow (10.0, logValue);
    }
    else
    {
        return xMin + normalised * (xMax - xMin);
    }
}

double CartesianPlane::yToValue (float y) const
{
    auto bounds = getPlotBounds();
    double normalised = (bounds.getBottom() - y) / bounds.getHeight();

    if (yScaleType == AxisScaleType::logarithmic)
    {
        double logMin = std::log10 (yMin);
        double logMax = std::log10 (yMax);
        double logValue = logMin + normalised * (logMax - logMin);
        return std::pow (10.0, logValue);
    }
    else
    {
        return yMin + normalised * (yMax - yMin);
    }
}

Rectangle<float> CartesianPlane::getPlotBounds() const
{
    auto bounds = getLocalBounds();
    return Rectangle<float> (
        static_cast<float> (marginLeft),
        static_cast<float> (marginTop),
        static_cast<float> (bounds.getWidth() - marginLeft - marginRight),
        static_cast<float> (bounds.getHeight() - marginTop - marginBottom)
    );
}

//==============================================================================
// Component overrides

void CartesianPlane::paint (Graphics& g)
{
    drawBackground (g);

    auto plotBounds = getPlotBounds();

    drawGrid (g, plotBounds);
    drawSignals (g, plotBounds);
    drawAxisLabels (g, plotBounds);
    drawTitle (g);

    if (showLegend && !signals.empty())
        drawLegend (g, plotBounds);
}

//==============================================================================
// Private methods

void CartesianPlane::drawBackground (Graphics& g)
{
    g.setFillColor (backgroundColor);
    g.fillAll();
}

void CartesianPlane::drawGrid (Graphics& g, const Rectangle<float>& bounds)
{
    // Draw vertical grid lines
    for (const auto& gridLine : verticalGridLines)
    {
        float x = valueToX (gridLine.value);
        if (x >= bounds.getX() && x <= bounds.getRight())
        {
            g.setStrokeColor (gridLine.color);
            g.setStrokeWidth (gridLine.strokeWidth);
            if (gridLine.emphasize)
                g.setStrokeWidth (gridLine.strokeWidth * 2.0f);

            g.strokeLine ({ x, bounds.getY() }, { x, bounds.getBottom() });
        }
    }

    // Draw horizontal grid lines
    for (const auto& gridLine : horizontalGridLines)
    {
        float y = valueToY (gridLine.value);
        if (y >= bounds.getY() && y <= bounds.getBottom())
        {
            g.setStrokeColor (gridLine.color);
            g.setStrokeWidth (gridLine.strokeWidth);
            if (gridLine.emphasize)
                g.setStrokeWidth (gridLine.strokeWidth * 2.0f);

            g.strokeLine ({ bounds.getX(), y }, { bounds.getRight(), y });
        }
    }
}

void CartesianPlane::drawSignals (Graphics& g, const Rectangle<float>& bounds)
{
    for (const auto& signal : signals)
    {
        if (!signal.visible || signal.data.empty())
            continue;

        g.setStrokeColor (signal.color);
        g.setStrokeWidth (signal.strokeWidth);
        
        Path path;
        bool firstPoint = true;
        Point<float> previousPoint;
        bool previousPointValid = false;

        for (const auto& point : signal.data)
        {
            float x = valueToX (point.getX());
            float y = valueToY (point.getY());
            
            Point<float> currentPoint (x, y);
            bool currentPointInBounds = bounds.contains (currentPoint);
            
            // Handle visibility and path continuity
            if (currentPointInBounds)
            {
                if (firstPoint)
                {
                    path.startNewSubPath (x, y);
                    firstPoint = false;
                }
                else if (previousPointValid && !bounds.contains (previousPoint))
                {
                    // Previous point was outside, current is inside - find intersection and start new subpath
                    auto intersection = findBoundsIntersection (previousPoint, currentPoint, bounds);
                    if (intersection.has_value())
                    {
                        path.startNewSubPath (intersection->getX(), intersection->getY());
                        path.lineTo (x, y);
                    }
                    else
                    {
                        path.startNewSubPath (x, y);
                    }
                }
                else
                {
                    path.lineTo (x, y);
                }
            }
            else if (previousPointValid && bounds.contains (previousPoint))
            {
                // Previous point was inside, current is outside - draw to intersection
                auto intersection = findBoundsIntersection (previousPoint, currentPoint, bounds);
                if (intersection.has_value())
                {
                    path.lineTo (intersection->getX(), intersection->getY());
                }
            }
            
            previousPoint = currentPoint;
            previousPointValid = true;
        }

        if (!firstPoint)
            g.strokePath (path);
    }
}

void CartesianPlane::drawAxisLabels (Graphics& g, const Rectangle<float>& bounds)
{
    // Draw X axis labels
    for (const auto& label : xAxisLabels)
    {
        float x = valueToX (label.value);
        if (x >= bounds.getX() && x <= bounds.getRight())
        {
            g.setFillColor (label.color);
            auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (label.fontSize);

            Rectangle<int> labelBounds (
                static_cast<int> (x - 30),
                static_cast<int> (bounds.getBottom() + 2),
                60,
                marginBottom - 2
            );

            g.fillFittedText (label.text, font, labelBounds, Justification::center);
        }
    }

    // Draw Y axis labels
    for (const auto& label : yAxisLabels)
    {
        float y = valueToY (label.value);
        if (y >= bounds.getY() && y <= bounds.getBottom())
        {
            g.setFillColor (label.color);
            auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (label.fontSize);

            Rectangle<int> labelBounds (
                2,
                static_cast<int> (y - 8),
                marginLeft - 4,
                16
            );

            g.fillFittedText (label.text, font, labelBounds, Justification::right);
        }
    }
}

void CartesianPlane::drawTitle (Graphics& g)
{
    if (titleText.isEmpty())
        return;

    g.setFillColor (titleColor);

    Rectangle<int> titleBounds (
        marginLeft,
        2,
        getWidth() - marginLeft - marginRight,
        marginTop - 4
    );

    g.fillFittedText (titleText, titleFont, titleBounds, titleJustification);
}

void CartesianPlane::drawLegend (Graphics& g, const Rectangle<float>& bounds)
{
    // Count visible signals
    int visibleSignalCount = 0;
    for (const auto& signal : signals)
    {
        if (signal.visible && !signal.name.isEmpty())
            visibleSignalCount++;
    }

    if (visibleSignalCount == 0)
        return;

    const int itemHeight = 16;
    const int itemSpacing = 2;
    const int padding = 8;
    const int legendWidth = 120;
    const int legendHeight = visibleSignalCount * (itemHeight + itemSpacing) - itemSpacing + 2 * padding;

    // Calculate legend position
    float legendX = bounds.getX() + legendPosition.getX() * bounds.getWidth() - legendWidth;
    float legendY = bounds.getY() + legendPosition.getY() * bounds.getHeight();

    // Keep legend within bounds
    legendX = jlimit (bounds.getX(), bounds.getRight() - legendWidth, legendX);
    legendY = jlimit (bounds.getY(), bounds.getBottom() - legendHeight, legendY);

    Rectangle<float> legendBounds (legendX, legendY, static_cast<float> (legendWidth), static_cast<float> (legendHeight));

    // Draw legend background
    g.setFillColor (legendBackgroundColor);
    g.fillRoundedRect (legendBounds, 4.0f);

    // Draw legend border
    g.setStrokeColor (Color (0x40FFFFFF));
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (legendBounds, 4.0f);

    // Draw legend items
    auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (11.0f);
    float itemY = legendY + padding;

    for (const auto& signal : signals)
    {
        if (!signal.visible || signal.name.isEmpty())
            continue;

        // Draw color indicator
        Rectangle<float> colorRect (legendX + padding, itemY + 2, 12, itemHeight - 4);
        g.setFillColor (signal.color);
        g.fillRect (colorRect);

        // Draw signal name
        Rectangle<int> textBounds (
            static_cast<int> (legendX + padding + 18),
            static_cast<int> (itemY),
            legendWidth - padding - 18 - padding,
            itemHeight
        );

        g.setFillColor (Colors::white);
        g.fillFittedText (signal.name, font, textBounds, Justification::centerLeft);

        itemY += itemHeight + itemSpacing;
    }
}

String CartesianPlane::formatAxisValue (double value, AxisScaleType scaleType) const
{
    if (scaleType == AxisScaleType::logarithmic)
    {
        if (value >= 1000.0)
            return String (value / 1000.0, (value >= 10000.0) ? 0 : 1) + "k";
        else
            return String (value, (value >= 100.0) ? 0 : 1);
    }
    else
    {
        if (std::abs (value) >= 1000.0)
            return String (value / 1000.0, 1) + "k";
        else if (std::abs (value) >= 1.0)
            return String (value, (std::abs (value) >= 10.0) ? 0 : 1);
        else
            return String (value, 3);
    }
}

std::optional<Point<float>> CartesianPlane::findBoundsIntersection (const Point<float>& p1, const Point<float>& p2, const Rectangle<float>& bounds) const
{
    // Find intersection of line segment p1-p2 with rectangle bounds
    
    float dx = p2.getX() - p1.getX();
    float dy = p2.getY() - p1.getY();
    
    if (std::abs (dx) < 1e-6f && std::abs (dy) < 1e-6f)
        return std::nullopt; // Points are the same
    
    float t_min = 0.0f;
    float t_max = 1.0f;
    
    // Check intersection with vertical bounds (left and right edges)
    if (std::abs (dx) > 1e-6f)
    {
        float t_left = (bounds.getX() - p1.getX()) / dx;
        float t_right = (bounds.getRight() - p1.getX()) / dx;
        
        float t_min_x = jmin (t_left, t_right);
        float t_max_x = jmax (t_left, t_right);
        
        t_min = jmax (t_min, t_min_x);
        t_max = jmin (t_max, t_max_x);
    }
    else
    {
        // Line is vertical
        if (p1.getX() < bounds.getX() || p1.getX() > bounds.getRight())
            return std::nullopt;
    }
    
    // Check intersection with horizontal bounds (top and bottom edges)
    if (std::abs (dy) > 1e-6f)
    {
        float t_top = (bounds.getY() - p1.getY()) / dy;
        float t_bottom = (bounds.getBottom() - p1.getY()) / dy;
        
        float t_min_y = jmin (t_top, t_bottom);
        float t_max_y = jmax (t_top, t_bottom);
        
        t_min = jmax (t_min, t_min_y);
        t_max = jmin (t_max, t_max_y);
    }
    else
    {
        // Line is horizontal
        if (p1.getY() < bounds.getY() || p1.getY() > bounds.getBottom())
            return std::nullopt;
    }
    
    if (t_min <= t_max && t_min >= 0.0f && t_min <= 1.0f)
        return Point<float> (p1.getX() + t_min * dx, p1.getY() + t_min * dy);
    
    return std::nullopt;
}

} // namespace yup
