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

#include <yup_gui/yup_gui.h>

//==============================================================================

class PathsExample : public yup::Component
{
public:
    PathsExample()
        : Component ("PathsExample")
    {
    }

    void paint (yup::Graphics& g) override
    {
        // Draw title
        /*
        g.setColour (yup::Color (50, 50, 80));
        g.setFont (yup::Font (24.0f, yup::Font::bold));
        g.drawText ("YUP Path Class - Complete Feature Demonstration",
                   yup::Rectangle<float> (20, 10, getWidth() - 40, 40),
                   yup::Justification::centred);
        */

        auto bounds = getLocalBounds().to<float>().reduced (20, 60);
        auto sectionHeight = bounds.getHeight() / 4.0f;
        auto sectionWidth = bounds.getWidth() / 3.0f;

        // Section 1: Basic Path Operations
        drawBasicPathOperations (g, yup::Rectangle<float> (bounds.getX(), bounds.getY(),
                                                          sectionWidth, sectionHeight));

        // Section 2: Basic Shapes
        drawBasicShapes (g, yup::Rectangle<float> (bounds.getX() + sectionWidth, bounds.getY(),
                                                  sectionWidth, sectionHeight));

        // Section 3: Complex Shapes (Polygons, Stars, Bubbles)
        drawComplexShapes (g, yup::Rectangle<float> (bounds.getX() + sectionWidth * 2, bounds.getY(),
                                                     sectionWidth, sectionHeight));

        // Section 4: Arcs and Curves
        drawArcsAndCurves (g, yup::Rectangle<float> (bounds.getX(), bounds.getY() + sectionHeight,
                                                     sectionWidth, sectionHeight));

        // Section 5: Path Transformations
        drawPathTransformations (g, yup::Rectangle<float> (bounds.getX() + sectionWidth, bounds.getY() + sectionHeight,
                                                          sectionWidth, sectionHeight));

        // Section 6: Advanced Features
        drawAdvancedFeatures (g, yup::Rectangle<float> (bounds.getX() + sectionWidth * 2, bounds.getY() + sectionHeight,
                                                       sectionWidth, sectionHeight));

        // Section 7: Path Utilities
        drawPathUtilities (g, yup::Rectangle<float> (bounds.getX(), bounds.getY() + sectionHeight * 2,
                                                    sectionWidth, sectionHeight));

        // Section 8: SVG Path Data
        drawSVGPathData (g, yup::Rectangle<float> (bounds.getX() + sectionWidth, bounds.getY() + sectionHeight * 2,
                                                  sectionWidth, sectionHeight));

        // Section 9: Creative Examples
        drawCreativeExamples (g, yup::Rectangle<float> (bounds.getX() + sectionWidth * 2, bounds.getY() + sectionHeight * 2,
                                                       sectionWidth, sectionHeight));

        // Section 10: Interactive Features Demo
        drawInteractiveDemo (g, yup::Rectangle<float> (bounds.getX(), bounds.getY() + sectionHeight * 3,
                                                      bounds.getWidth(), sectionHeight));
    }

private:
    void drawSectionTitle (yup::Graphics& g, const yup::String& title, yup::Rectangle<float> area)
    {
        yup::StyledText text;

        {
            auto modifier = text.startUpdate();
            modifier.setMaxSize (area.getSize());
            modifier.setHorizontalAlign (yup::StyledText::center);
            modifier.appendText (title, yup::ApplicationTheme::getGlobalTheme()->getDefaultFont());
        }

        g.setFillColor (yup::Colors::white);
        g.fillFittedText(text, area.removeFromTop (20));
    }

    void drawBasicPathOperations (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Basic Path Operations", area);
        area = area.reduced (10); // .withTrimmedTop (25);

        yup::Path path;
        float x = area.getX() + 20;
        float y = area.getY() + 30;

        // MoveTo and LineTo demo
        path.moveTo (x, y);
        path.lineTo (x + 60, y);
        path.lineTo (x + 60, y + 40);
        path.lineTo (x, y + 40);
        path.close();

        g.setFillColor (yup::Color (100, 150, 255));
        g.fillPath (path);
        g.setStrokeColor (yup::Color (50, 100, 200));
        g.setStrokeWidth (2.0f);
        g.strokePath (path);

        // QuadTo demo
        yup::Path quadPath;
        x += 80;
        quadPath.moveTo (x, y + 40);
        quadPath.quadTo (x + 30, y, x + 60, y + 40);

        g.setStrokeColor (yup::Color (255, 150, 100));
        g.setStrokeWidth (3.0f);
        g.strokePath (quadPath);

        // CubicTo demo
        yup::Path cubicPath;
        x += 80;
        cubicPath.moveTo (x, y + 40);
        cubicPath.cubicTo (x + 60, y + 40, x + 10, y, x + 50, y);

        g.setStrokeColor (yup::Color (150, 255, 150));
        g.setStrokeWidth (3.0f);
        g.strokePath (cubicPath);

        // Draw labels
        //g.setColour (yup::Color (80, 80, 80));
        //g.setFont (yup::Font (10.0f));
        //g.drawText ("Rectangle", yup::Rectangle<float> (area.getX(), y + 50, 80, 15), yup::Justification::centred);
        //g.drawText ("Quadratic", yup::Rectangle<float> (area.getX() + 80, y + 50, 80, 15), yup::Justification::centred);
        //g.drawText ("Cubic Curve", yup::Rectangle<float> (area.getX() + 160, y + 50, 80, 15), yup::Justification::centred);
    }

    void drawBasicShapes (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Basic Shapes", area);
        area = area.reduced (10); // .withTrimmedTop (25);

        float x = area.getX() + 10;
        float y = area.getY() + 10;
        float spacing = 80;

        // Rectangle
        yup::Path rectPath;
        rectPath.addRectangle (x, y, 60, 40);
        g.setFillColor (yup::Color (255, 200, 200));
        g.fillPath (rectPath);
        g.setStrokeColor (yup::Color (200, 100, 100));
        g.setStrokeWidth (1.0f);
        g.strokePath (rectPath);

        // Rounded Rectangle
        yup::Path roundedRectPath;
        roundedRectPath.addRoundedRectangle (x, y + 60, 60, 40, 10);
        g.setFillColor (yup::Color (200, 255, 200));
        g.fillPath (roundedRectPath);
        g.setStrokeColor (yup::Color (100, 200, 100));
        g.strokePath (roundedRectPath);

        // Ellipse
        yup::Path ellipsePath;
        ellipsePath.addEllipse (x + spacing, y, 60, 40);
        g.setFillColor (yup::Color (200, 200, 255));
        g.fillPath (ellipsePath);
        g.setStrokeColor (yup::Color (100, 100, 200));
        g.strokePath (ellipsePath);

        // Centered Ellipse
        yup::Path centeredEllipsePath;
        centeredEllipsePath.addCenteredEllipse (yup::Point<float> (x + spacing + 30, y + 80), 30, 20);
        g.setFillColor (yup::Color (255, 255, 200));
        g.fillPath (centeredEllipsePath);
        g.setStrokeColor (yup::Color (200, 200, 100));
        g.strokePath (centeredEllipsePath);

        // Labels
        /*
        g.setColour (yup::Color (80, 80, 80));
        g.setFont (yup::Font (9.0f));
        g.drawText ("Rectangle", yup::Rectangle<float> (x - 10, y + 45, 80, 12), yup::Justification::centred);
        g.drawText ("Rounded", yup::Rectangle<float> (x - 10, y + 105, 80, 12), yup::Justification::centred);
        g.drawText ("Ellipse", yup::Rectangle<float> (x + spacing - 10, y + 45, 80, 12), yup::Justification::centred);
        g.drawText ("Centered", yup::Rectangle<float> (x + spacing - 10, y + 105, 80, 12), yup::Justification::centred);
        */
    }

    void drawComplexShapes (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Complex Shapes", area);
        area = area.reduced (10); // .withTrimmedTop (25);

        float x = area.getX() + 40;
        float y = area.getY() + 40;

        // Pentagon
        yup::Path pentagonPath;
        pentagonPath.addPolygon (yup::Point<float> (x, y), 5, 25, -yup::MathConstants<float>::halfPi);
        g.setFillColor (yup::Color (255, 180, 120));
        g.fillPath (pentagonPath);
        g.setStrokeColor (yup::Color (200, 120, 60));
        g.setStrokeWidth (1.5f);
        g.strokePath (pentagonPath);

        // Hexagon
        yup::Path hexagonPath;
        hexagonPath.addPolygon (yup::Point<float> (x + 80, y), 6, 25, 0);
        g.setFillColor (yup::Color (180, 255, 180));
        g.fillPath (hexagonPath);
        g.setStrokeColor (yup::Color (120, 200, 120));
        g.strokePath (hexagonPath);

        // Star
        yup::Path starPath;
        starPath.addStar (yup::Point<float> (x + 160, y), 5, 15, 25, -yup::MathConstants<float>::halfPi);
        g.setFillColor (yup::Color (255, 255, 120));
        g.fillPath (starPath);
        g.setStrokeColor (yup::Color (200, 200, 60));
        g.strokePath (starPath);

        // Speech Bubble
        yup::Path bubblePath;
        yup::Rectangle<float> bodyArea (x - 20, y + 70, 80, 40);
        yup::Rectangle<float> maxArea = bodyArea.enlarged(20);
        yup::Point<float> tipPosition (x + 70, y + 120);
        bubblePath.addBubble (bodyArea, maxArea, tipPosition, 8, 12);
        g.setFillColor (yup::Color (220, 240, 255));
        g.fillPath (bubblePath);
        g.setStrokeColor (yup::Color (100, 150, 200));
        g.strokePath (bubblePath);

        // Triangle (3-sided polygon)
        yup::Path trianglePath;
        trianglePath.addPolygon (yup::Point<float> (x + 120, y + 85), 3, 20, -yup::MathConstants<float>::halfPi);
        g.setFillColor (yup::Color (255, 200, 255));
        g.fillPath (trianglePath);
        g.setStrokeColor (yup::Color (200, 100, 200));
        g.strokePath (trianglePath);

        // Labels
        /*
        g.setColour (yup::Color (80, 80, 80));
        g.setFont (yup::Font (8.0f));
        g.drawText ("Pentagon", yup::Rectangle<float> (x - 25, y + 30, 50, 12), yup::Justification::centred);
        g.drawText ("Hexagon", yup::Rectangle<float> (x + 55, y + 30, 50, 12), yup::Justification::centred);
        g.drawText ("Star", yup::Rectangle<float> (x + 135, y + 30, 50, 12), yup::Justification::centred);
        g.drawText ("Bubble", yup::Rectangle<float> (x - 25, y + 120, 50, 12), yup::Justification::centred);
        g.drawText ("Triangle", yup::Rectangle<float> (x + 95, y + 110, 50, 12), yup::Justification::centred);
        */
    }

    void drawArcsAndCurves (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Arcs & Curves", area);
        area = area.reduced (10); // .withTrimmedTop (25);

        float x = area.getX() + 30;
        float y = area.getY() + 30;

        // Simple Arc
        yup::Path arcPath;
        arcPath.addArc (x, y, 60, 60, 0, yup::MathConstants<float>::halfPi, true);
        g.setStrokeColor (yup::Color (255, 150, 150));
        g.setStrokeWidth (3.0f);
        g.strokePath (arcPath);

        // Centered Arc with rotation
        yup::Path centeredArcPath;
        centeredArcPath.addCenteredArc (yup::Point<float> (x + 100, y + 30), 25, 15,
                                        yup::MathConstants<float>::pi / 4, 0, yup::MathConstants<float>::pi, true);
        g.setStrokeColor (yup::Color (150, 255, 150));
        g.setStrokeWidth (3.0f);
        g.strokePath (centeredArcPath);

        // Complete circle using arc
        yup::Path circlePath;
        circlePath.addArc (x + 160, y, 50, 50, 0, yup::MathConstants<float>::twoPi, true);
        g.setFillColor (yup::Color (150, 150, 255));
        g.fillPath (circlePath);

        // Complex curve combination
        yup::Path complexPath;
        complexPath.moveTo (x, y + 80);
        complexPath.quadTo (x + 50, y + 60, x + 100, y + 80);
        complexPath.cubicTo (x + 150, y + 80, x + 180, y + 120, x + 200, y + 100);
        g.setStrokeColor (yup::Color (255, 200, 100));
        g.setStrokeWidth (2.0f);
        g.strokePath (complexPath);

        // Labels
        /*
        g.setColour (yup::Color (80, 80, 80));
        g.setFont (yup::Font (9.0f));
        g.drawText ("Arc", yup::Rectangle<float> (x - 10, y + 70, 60, 12), yup::Justification::centred);
        g.drawText ("Rotated Arc", yup::Rectangle<float> (x + 70, y + 70, 60, 12), yup::Justification::centred);
        g.drawText ("Circle", yup::Rectangle<float> (x + 140, y + 60, 60, 12), yup::Justification::centred);
        g.drawText ("Complex Curves", yup::Rectangle<float> (x + 50, y + 110, 100, 12), yup::Justification::centred);
        */
    }

    void drawPathTransformations (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Transformations", area);
        area = area.reduced (10); // .withTrimmedTop (25);

        float x = area.getX() + 20;
        float y = area.getY() + 20;

        // Original shape
        yup::Path originalPath;
        originalPath.addStar (yup::Point<float> (x + 30, y + 30), 5, 15, 25, 0);
        g.setFillColor (yup::Color (200, 200, 200));
        g.fillPath (originalPath);
        g.setStrokeColor (yup::Color (100, 100, 100));
        g.setStrokeWidth (1.0f);
        g.strokePath (originalPath);

        // Scaled version
        yup::Path scaledPath = originalPath.transformed (yup::AffineTransform::scaling (0.7f, 1.3f));
        scaledPath.transform (yup::AffineTransform::translation (80, 0));
        g.setFillColor (yup::Color (255, 200, 200));
        g.fillPath (scaledPath);
        g.setStrokeColor (yup::Color (200, 100, 100));
        g.strokePath (scaledPath);

        // Rotated version
        yup::Path rotatedPath = originalPath.transformed (
            yup::AffineTransform::rotation (yup::MathConstants<float>::pi / 4, x + 30, y + 30));
        rotatedPath.transform (yup::AffineTransform::translation (160, 0));
        g.setFillColor (yup::Color (200, 255, 200));
        g.fillPath (rotatedPath);
        g.setStrokeColor (yup::Color (100, 200, 100));
        g.strokePath (rotatedPath);

        // ScaleToFit demo
        yup::Path scaleToFitPath;
        scaleToFitPath.addPolygon (yup::Point<float> (0, 0), 6, 20, 0);
        scaleToFitPath.scaleToFit (x, y + 80, 180, 30, true);
        g.setFillColor (yup::Color (200, 200, 255));
        g.fillPath (scaleToFitPath);
        g.setStrokeColor (yup::Color (100, 100, 200));
        g.strokePath (scaleToFitPath);

        // Labels
        /*
        g.setColour (yup::Color (80, 80, 80));
        g.setFont (yup::Font (8.0f));
        g.drawText ("Original", yup::Rectangle<float> (x - 5, y + 60, 60, 12), yup::Justification::centred);
        g.drawText ("Scaled", yup::Rectangle<float> (x + 55, y + 60, 60, 12), yup::Justification::centred);
        g.drawText ("Rotated", yup::Rectangle<float> (x + 135, y + 60, 60, 12), yup::Justification::centred);
        g.drawText ("Scale to Fit", yup::Rectangle<float> (x + 40, y + 115, 100, 12), yup::Justification::centred);
        */
    }

    void drawAdvancedFeatures (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Advanced Features", area);
        area = area.reduced (10); // .withTrimmedTop (25);

        float x = area.getX() + 20;
        float y = area.getY() + 20;

        // Stroke polygon demo
        yup::Path originalCurve;
        originalCurve.moveTo (x, y + 40);
        originalCurve.quadTo (x + 40, y, x + 80, y + 40);

        yup::Path strokePolygon = originalCurve.createStrokePolygon (8.0f);
        g.setFillColor (yup::Color (255, 220, 180));
        g.fillPath (strokePolygon);
        g.setStrokeColor (yup::Color (200, 150, 100));
        g.setStrokeWidth (1.0f);
        g.strokePath (originalCurve);

        // Rounded corners demo
        yup::Path sharpPath;
        sharpPath.addRectangle (x + 100, y, 60, 60);
        yup::Path roundedPath = sharpPath.withRoundedCorners (10.0f);

        g.setFillColor (yup::Color (200, 255, 220));
        g.fillPath (roundedPath);
        g.setStrokeColor (yup::Color (100, 200, 120));
        g.strokePath (sharpPath);

        // Point along path demo
        yup::Path curvePath;
        curvePath.moveTo (x, y + 80);
        curvePath.cubicTo (x + 60, y + 80, x + 120, y + 120, x + 180, y + 100);

        g.setStrokeColor (yup::Color (100, 150, 255));
        g.setStrokeWidth (2.0f);
        g.strokePath (curvePath);

        // Draw points along the path
        for (float t = 0.0f; t <= 1.0f; t += 0.2f)
        {
            yup::Point<float> point = curvePath.getPointAlongPath (t);
            yup::Path pointPath;
            pointPath.addCenteredEllipse (point, 4, 4);
            g.setFillColor (yup::Color (255, 100, 100));
            g.fillPath (pointPath);
        }

        // Labels
        /*
        g.setColour (yup::Color (80, 80, 80));
        g.setFont (yup::Font (8.0f));
        g.drawText ("Stroke Polygon", yup::Rectangle<float> (x - 10, y + 60, 100, 12), yup::Justification::centred);
        g.drawText ("Rounded Corners", yup::Rectangle<float> (x + 90, y + 70, 80, 12), yup::Justification::centred);
        g.drawText ("Points Along Path", yup::Rectangle<float> (x + 40, y + 130, 100, 12), yup::Justification::centred);
        */
    }

    void drawPathUtilities (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Path Utilities", area);
        area = area.reduced (10); // .withTrimmedTop (25);

        float x = area.getX() + 20;
        float y = area.getY() + 20;

        // AppendPath demo
        yup::Path path1;
        path1.addEllipse (x, y, 40, 40);

        yup::Path path2;
        path2.addRectangle (x + 20, y + 20, 40, 40);

        path1.appendPath (path2);
        g.setFillColor (yup::Color (255, 200, 255));
        g.fillPath (path1);
        g.setStrokeColor (yup::Color (200, 100, 200));
        g.setStrokeWidth (1.0f);
        g.strokePath (path1);

        // Bounds demonstration
        yup::Path boundsPath;
        boundsPath.addStar (yup::Point<float> (x + 120, y + 30), 5, 15, 25, 0);
        yup::Rectangle<float> bounds = boundsPath.getBounds();

        g.setFillColor (yup::Color (180, 255, 180));
        g.fillPath (boundsPath);
        g.setStrokeColor (yup::Color (255, 100, 100));
        g.setStrokeWidth (1.0f);
        g.strokeRect (bounds);

        // Size and clear demo
        yup::Path infoPath;
        infoPath.addPolygon (yup::Point<float> (x + 60, y + 80), 8, 20, 0);

        g.setFillColor (yup::Color (200, 220, 255));
        g.fillPath (infoPath);

        // Display path info
        /*
        g.setColour (yup::Color (80, 80, 80));
        g.setFont (yup::Font (8.0f));
        yup::String sizeText = "Size: " + yup::String (infoPath.size());
        g.drawText (sizeText, yup::Rectangle<float> (x + 20, y + 110, 100, 12), yup::Justification::left);

        // Labels
        g.drawText ("Append Paths", yup::Rectangle<float> (x - 10, y + 70, 80, 12), yup::Justification::centred);
        g.drawText ("Bounds", yup::Rectangle<float> (x + 90, y + 70, 80, 12), yup::Justification::centred);
        g.drawText ("Path Info", yup::Rectangle<float> (x + 20, y + 130, 80, 12), yup::Justification::centred);
        */
    }

    void drawSVGPathData (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "SVG Path Data", area);
        area = area.reduced (10); // .withTrimmedTop (25);

        float x = area.getX() + 20;
        float y = area.getY() + 20;

        // Parse SVG path data examples
        yup::Path svgHeart;
        svgHeart.parsePathData ("M12,21.35l-1.45-1.32C5.4,15.36,2,12.28,2,8.5 C2,5.42,4.42,3,7.5,3c1.74,0,3.41,0.81,4.5,2.09C13.09,3.81,14.76,3,16.5,3 C19.58,3,22,5.42,22,8.5c0,3.78-3.4,6.86-8.55,11.54L12,21.35z");

        // Scale and position the heart
        yup::Rectangle<float> heartBounds = svgHeart.getBounds();
        float scale = 3.0f;
        svgHeart.transform (yup::AffineTransform::scaling (scale));
        svgHeart.transform (yup::AffineTransform::translation (x - heartBounds.getX() * scale, y - heartBounds.getY() * scale));

        g.setFillColor (yup::Color (255, 150, 150));
        g.fillPath (svgHeart);
        g.setStrokeColor (yup::Color (200, 100, 100));
        g.setStrokeWidth (1.0f);
        g.strokePath (svgHeart);

        // Simple SVG path
        yup::Path svgTriangle;
        svgTriangle.parsePathData ("M100,20 L180,160 L20,160 Z");
        svgTriangle.scaleToFit (x + 100, y, 80, 80, true);

        g.setFillColor (yup::Color (150, 255, 150));
        g.fillPath (svgTriangle);
        g.setStrokeColor (yup::Color (100, 200, 100));
        g.setStrokeWidth (1.0f);
        g.strokePath (svgTriangle);

        // Labels
        /*
        g.setColour (yup::Color (80, 80, 80));
        g.setFont (yup::Font (8.0f));
        g.drawText ("SVG Heart", yup::Rectangle<float> (x - 10, y + 90, 80, 12), yup::Justification::centred);
        g.drawText ("SVG Triangle", yup::Rectangle<float> (x + 90, y + 90, 80, 12), yup::Justification::centred);
        */
    }

    void drawCreativeExamples (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Creative Examples", area);
        area = area.reduced (10); // .withTrimmedTop (25);

        float x = area.getX() + 20;
        float y = area.getY() + 20;

        // Flower pattern using multiple shapes
        yup::Point<float> center (x + 60, y + 60);

        // Petals
        for (int i = 0; i < 8; ++i)
        {
            float angle = i * yup::MathConstants<float>::twoPi / 8.0f;
            yup::Path petal;
            petal.addCenteredEllipse (yup::Point<float> (0, 0), 15, 30);
            petal.transform (yup::AffineTransform::rotation (angle));
            petal.transform (yup::AffineTransform::translation (center.getX(), center.getY()));

            float hue = i / 8.0f;
            g.setFillColor (yup::Color::fromHSV (hue, 0.7f, 1.0f, 0.8f));
            g.fillPath (petal);
        }

        // Center
        yup::Path flowerCenter;
        flowerCenter.addCenteredEllipse (center, 12, 12);
        g.setFillColor (yup::Color (255, 255, 100));
        g.fillPath (flowerCenter);
        g.setStrokeColor (yup::Color (200, 200, 50));
        g.setStrokeWidth (1.0f);
        g.strokePath (flowerCenter);

        // Gear shape using polygons
        yup::Path gear;
        gear.addPolygon (yup::Point<float> (x + 180, y + 60), 12, 35, 0);
        yup::Path innerGear;
        innerGear.addPolygon (yup::Point<float> (x + 180, y + 60), 12, 25, yup::MathConstants<float>::pi / 12);

        g.setFillColor (yup::Color (180, 180, 180));
        g.fillPath (gear);
        g.setFillColor (yup::Color (220, 220, 220));
        g.fillPath (innerGear);
        g.setStrokeColor (yup::Color (100, 100, 100));
        g.strokePath (gear);

        // Center hole
        yup::Path centerHole;
        centerHole.addCenteredEllipse (yup::Point<float> (x + 180, y + 60), 10, 10);
        g.setFillColor (yup::Color (245, 245, 250));
        g.fillPath (centerHole);

        // Labels
        /*
        g.setColour (yup::Color (80, 80, 80));
        g.setFont (yup::Font (8.0f));
        g.drawText ("Flower", yup::Rectangle<float> (x + 20, y + 130, 80, 12), yup::Justification::centred);
        g.drawText ("Gear", yup::Rectangle<float> (x + 140, y + 130, 80, 12), yup::Justification::centred);
        */
    }

    void drawInteractiveDemo (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Interactive Path Builder Demo", area);
        area = area.reduced (10); // .withTrimmedTop (25);

        // Create a complex path combining multiple features
        yup::Path masterPath;

        float centerX = area.getCenterX();
        float centerY = area.getCenterY();

        // Base shape - rounded rectangle
        masterPath.addRoundedRectangle (centerX - 150, centerY - 40, 300, 80, 20);

        // Add decorative elements
        for (int i = 0; i < 5; ++i)
        {
            yup::Path star;
            float x = centerX - 120 + i * 60;
            star.addStar (yup::Point<float> (x, centerY - 60), 5, 8, 15, 0);
            masterPath.appendPath (star);
        }

        // Add speech bubble
        yup::Path bubble;
        yup::Rectangle<float> bubbleBody (centerX + 160, centerY - 30, 80, 40);
        bubble.addBubble (bubbleBody, bubbleBody.enlarged (10), yup::Point<float> (centerX + 140, centerY), 8, 15);
        masterPath.appendPath (bubble);

        // Add connecting arc
        yup::Path arc;
        arc.addCenteredArc (yup::Point<float> (centerX, centerY + 60), 200, 20, 0, -yup::MathConstants<float>::pi, 0, true);
        masterPath.appendPath (arc);

        // Render with gradient-like effect
        g.setFillColor (yup::Color (100, 150, 255).withAlpha (0.3f));
        g.fillPath (masterPath);
        g.setStrokeColor (yup::Color (50, 100, 200));
        g.setStrokeWidth (2.0f);
        g.strokePath (masterPath);

        // Add text
        /*
        g.setColour (yup::Color (255, 255, 255));
        g.setFont (yup::Font (14.0f, yup::Font::bold));
        g.drawText ("YUP Path System",
                   yup::Rectangle<float> (centerX - 150, centerY - 10, 300, 20),
                   yup::Justification::centred);

        // Info text in bubble
        g.setColour (yup::Color (50, 50, 50));
        g.setFont (yup::Font (10.0f));
        g.drawText ("Powerful &\nFlexible",
                   yup::Rectangle<float> (centerX + 165, centerY - 20, 70, 30),
                   yup::Justification::centred);
        */
    }
};
