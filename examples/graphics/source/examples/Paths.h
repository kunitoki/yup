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

#pragma once

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
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();

        // Draw title
        /*
        g.setColour (yup::Color (50, 50, 80));
        g.setFont (yup::Font (24.0f, yup::Font::bold));
        g.drawText ("YUP Path Class - Complete Feature Demonstration",
                   yup::Rectangle<float> (20, 10, getWidth() - 40, 40),
                   yup::Justification::centred);
        */

        auto bounds = getLocalBounds().to<float>().reduced (10, 20);
        auto sectionHeight = bounds.getHeight() / 6.0f; // 6 rows instead of 4
        auto sectionWidth = bounds.getWidth() / 2.0f;   // 2 columns instead of 3

        // Row 1: Basic Operations and Basic Shapes
        drawBasicPathOperations (g, yup::Rectangle<float> (bounds.getX(), bounds.getY(), sectionWidth, sectionHeight));
        drawBasicShapes (g, yup::Rectangle<float> (bounds.getX() + sectionWidth, bounds.getY(), sectionWidth, sectionHeight));

        // Row 2: Complex Shapes and Arcs & Curves
        drawComplexShapes (g, yup::Rectangle<float> (bounds.getX(), bounds.getY() + sectionHeight, sectionWidth, sectionHeight));
        drawArcsAndCurves (g, yup::Rectangle<float> (bounds.getX() + sectionWidth, bounds.getY() + sectionHeight, sectionWidth, sectionHeight));

        // Row 3: Transformations and Advanced Features
        drawPathTransformations (g, yup::Rectangle<float> (bounds.getX(), bounds.getY() + sectionHeight * 2, sectionWidth, sectionHeight));
        drawAdvancedFeatures (g, yup::Rectangle<float> (bounds.getX() + sectionWidth, bounds.getY() + sectionHeight * 2, sectionWidth, sectionHeight));

        // Row 4: Path Utilities and SVG Path Data
        drawPathUtilities (g, yup::Rectangle<float> (bounds.getX(), bounds.getY() + sectionHeight * 3, sectionWidth, sectionHeight));
        drawSVGPathData (g, yup::Rectangle<float> (bounds.getX() + sectionWidth, bounds.getY() + sectionHeight * 3, sectionWidth, sectionHeight));

        // Row 5: Creative Examples (full width)
        drawCreativeExamples (g, yup::Rectangle<float> (bounds.getX(), bounds.getY() + sectionHeight * 4, bounds.getWidth(), sectionHeight));

        // Row 6: Interactive Demo (full width)
        drawInteractiveDemo (g, yup::Rectangle<float> (bounds.getX(), bounds.getY() + sectionHeight * 5, bounds.getWidth(), sectionHeight));
    }

private:
    void drawSectionTitle (yup::Graphics& g, const yup::String& title, yup::Rectangle<float> area)
    {
        yup::StyledText text;

        {
            auto modifier = text.startUpdate();
            modifier.setMaxSize (area.getSize());
            modifier.setHorizontalAlign (yup::StyledText::center);
            modifier.appendText (title, yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f));
        }

        g.setFillColor (yup::Colors::white);
        g.fillFittedText (text, area.removeFromTop (16));
    }

    void drawBasicPathOperations (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Basic Operations", area);
        area = area.reduced (5).withTrimmedTop (20);

        yup::Path path;
        float x = area.getX() + 10;
        float y = area.getY() + 10;

        // Smaller rectangle
        path.moveTo (x, y);
        path.lineTo (x + 40, y);
        path.lineTo (x + 40, y + 25);
        path.lineTo (x, y + 25);
        path.close();

        g.setFillColor (yup::Color (100, 150, 255));
        g.fillPath (path);
        g.setStrokeColor (yup::Color (50, 100, 200));
        g.setStrokeWidth (1.5f);
        g.strokePath (path);

        // QuadTo demo - smaller
        yup::Path quadPath;
        x += 50;
        quadPath.moveTo (x, y + 25);
        quadPath.quadTo (x + 20, y, x + 40, y + 25);

        g.setStrokeColor (yup::Color (255, 150, 100));
        g.setStrokeWidth (2.0f);
        g.strokePath (quadPath);

        // CubicTo demo - smaller
        yup::Path cubicPath;
        y += 35;
        x = area.getX() + 10;
        cubicPath.moveTo (x, y + 25);
        cubicPath.cubicTo (x + 40, y + 25, x + 5, y, x + 35, y);

        g.setStrokeColor (yup::Color (150, 255, 150));
        g.setStrokeWidth (2.0f);
        g.strokePath (cubicPath);
    }

    void drawBasicShapes (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Basic Shapes", area);
        area = area.reduced (5).withTrimmedTop (20);

        float x = area.getX() + 5;
        float y = area.getY() + 5;
        float spacing = 60;

        // Rectangle - smaller
        yup::Path rectPath;
        rectPath.addRectangle (x, y, 40, 25);
        g.setFillColor (yup::Color (255, 200, 200));
        g.fillPath (rectPath);
        g.setStrokeColor (yup::Color (200, 100, 100));
        g.setStrokeWidth (1.0f);
        g.strokePath (rectPath);

        // Rounded Rectangle
        yup::Path roundedRectPath;
        roundedRectPath.addRoundedRectangle (x + spacing, y, 40, 25, 8);
        g.setFillColor (yup::Color (200, 255, 200));
        g.fillPath (roundedRectPath);
        g.setStrokeColor (yup::Color (100, 200, 100));
        g.strokePath (roundedRectPath);

        // Ellipse
        yup::Path ellipsePath;
        ellipsePath.addEllipse (x, y + 35, 40, 25);
        g.setFillColor (yup::Color (200, 200, 255));
        g.fillPath (ellipsePath);
        g.setStrokeColor (yup::Color (100, 100, 200));
        g.strokePath (ellipsePath);

        // Centered Ellipse
        yup::Path centeredEllipsePath;
        centeredEllipsePath.addCenteredEllipse (yup::Point<float> (x + spacing + 20, y + 47), 20, 12);
        g.setFillColor (yup::Color (255, 255, 200));
        g.fillPath (centeredEllipsePath);
        g.setStrokeColor (yup::Color (200, 200, 100));
        g.strokePath (centeredEllipsePath);
    }

    void drawComplexShapes (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Complex Shapes", area);
        area = area.reduced (5).withTrimmedTop (20);

        float x = area.getX() + 30;
        float y = area.getY() + 25;

        // Pentagon - smaller
        yup::Path pentagonPath;
        pentagonPath.addPolygon (yup::Point<float> (x, y), 5, 18, -yup::MathConstants<float>::halfPi);
        g.setFillColor (yup::Color (255, 180, 120));
        g.fillPath (pentagonPath);
        g.setStrokeColor (yup::Color (200, 120, 60));
        g.setStrokeWidth (1.0f);
        g.strokePath (pentagonPath);

        // Star
        yup::Path starPath;
        starPath.addStar (yup::Point<float> (x + 60, y), 5, 10, 18, -yup::MathConstants<float>::halfPi);
        g.setFillColor (yup::Color (255, 255, 120));
        g.fillPath (starPath);
        g.setStrokeColor (yup::Color (200, 200, 60));
        g.strokePath (starPath);

        // Speech Bubble - smaller
        yup::Path bubblePath;
        yup::Rectangle<float> bodyArea (x - 15, y + 30, 50, 25);
        yup::Rectangle<float> maxArea = bodyArea.enlarged (10);
        yup::Point<float> tipPosition (x + 45, y + 65);
        bubblePath.addBubble (bodyArea, maxArea, tipPosition, 5, 8);
        g.setFillColor (yup::Color (220, 240, 255));
        g.fillPath (bubblePath);
        g.setStrokeColor (yup::Color (100, 150, 200));
        g.strokePath (bubblePath);

        // Triangle
        yup::Path trianglePath;
        trianglePath.addPolygon (yup::Point<float> (x + 75, y + 42), 3, 15, -yup::MathConstants<float>::halfPi);
        g.setFillColor (yup::Color (255, 200, 255));
        g.fillPath (trianglePath);
        g.setStrokeColor (yup::Color (200, 100, 200));
        g.strokePath (trianglePath);
    }

    void drawArcsAndCurves (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Arcs & Curves", area);
        area = area.reduced (5).withTrimmedTop (20);

        float x = area.getX() + 15;
        float y = area.getY() + 15;

        // Simple Arc - smaller
        yup::Path arcPath;
        arcPath.addArc (x, y, 40, 40, 0, yup::MathConstants<float>::halfPi, true);
        g.setStrokeColor (yup::Color (255, 150, 150));
        g.setStrokeWidth (2.0f);
        g.strokePath (arcPath);

        // Centered Arc with rotation
        yup::Path centeredArcPath;
        centeredArcPath.addCenteredArc (yup::Point<float> (x + 70, y + 20), 18, 12, yup::MathConstants<float>::pi / 4, 0, yup::MathConstants<float>::pi, true);
        g.setStrokeColor (yup::Color (150, 255, 150));
        g.setStrokeWidth (2.0f);
        g.strokePath (centeredArcPath);

        // Complete circle using arc
        yup::Path circlePath;
        circlePath.addArc (x + 100, y, 35, 35, 0, yup::MathConstants<float>::twoPi, true);
        g.setFillColor (yup::Color (150, 150, 255));
        g.fillPath (circlePath);

        // Complex curve combination
        yup::Path complexPath;
        complexPath.moveTo (x, y + 50);
        complexPath.quadTo (x + 35, y + 35, x + 70, y + 50);
        complexPath.cubicTo (x + 100, y + 50, x + 120, y + 75, x + 130, y + 60);
        g.setStrokeColor (yup::Color (255, 200, 100));
        g.setStrokeWidth (1.5f);
        g.strokePath (complexPath);
    }

    void drawPathTransformations (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Transformations", area);
        area = area.reduced (5).withTrimmedTop (20);

        float x = area.getX() + 15;
        float y = area.getY() + 15;

        // Original shape - smaller
        yup::Path originalPath;
        originalPath.addStar (yup::Point<float> (x + 20, y + 20), 5, 10, 18, 0);
        g.setFillColor (yup::Color (200, 200, 200));
        g.fillPath (originalPath);
        g.setStrokeColor (yup::Color (100, 100, 100));
        g.setStrokeWidth (1.0f);
        g.strokePath (originalPath);

        // Scaled version
        yup::Path scaledPath = originalPath.transformed (yup::AffineTransform::scaling (0.6f, 1.2f));
        scaledPath.transform (yup::AffineTransform::translation (50, 0));
        g.setFillColor (yup::Color (255, 200, 200));
        g.fillPath (scaledPath);
        g.setStrokeColor (yup::Color (200, 100, 100));
        g.strokePath (scaledPath);

        // Rotated version
        yup::Path rotatedPath = originalPath.transformed (
            yup::AffineTransform::rotation (yup::MathConstants<float>::pi / 4, x + 20, y + 20));
        rotatedPath.transform (yup::AffineTransform::translation (100, 0));
        g.setFillColor (yup::Color (200, 255, 200));
        g.fillPath (rotatedPath);
        g.setStrokeColor (yup::Color (100, 200, 100));
        g.strokePath (rotatedPath);

        // ScaleToFit demo - smaller
        yup::Path scaleToFitPath;
        scaleToFitPath.addPolygon (yup::Point<float> (0, 0), 6, 15, 0);
        scaleToFitPath.scaleToFit (x, y + 50, 120, 20, true);
        g.setFillColor (yup::Color (200, 200, 255));
        g.fillPath (scaleToFitPath);
        g.setStrokeColor (yup::Color (100, 100, 200));
        g.strokePath (scaleToFitPath);
    }

    void drawAdvancedFeatures (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Advanced Features", area);
        area = area.reduced (5).withTrimmedTop (20);

        float x = area.getX() + 10;
        float y = area.getY() + 10;

        // Stroke polygon demo - smaller
        yup::Path originalCurve;
        originalCurve.moveTo (x, y + 25);
        originalCurve.quadTo (x + 25, y, x + 50, y + 25);

        yup::Path strokePolygon = originalCurve.createStrokePolygon (5.0f);
        g.setFillColor (yup::Color (255, 220, 180));
        g.fillPath (strokePolygon);
        g.setStrokeColor (yup::Color (200, 150, 100));
        g.setStrokeWidth (1.0f);
        g.strokePath (originalCurve);

        // Rounded corners demo
        yup::Path sharpPath;
        sharpPath.addRectangle (x + 60, y, 40, 40);
        yup::Path roundedPath = sharpPath.withRoundedCorners (8.0f);

        g.setFillColor (yup::Color (200, 255, 220));
        g.fillPath (roundedPath);
        g.setStrokeColor (yup::Color (100, 200, 120));
        g.strokePath (sharpPath);

        // Point along path demo - smaller
        yup::Path curvePath;
        curvePath.moveTo (x, y + 50);
        curvePath.cubicTo (x + 40, y + 50, x + 80, y + 75, x + 120, y + 65);

        g.setStrokeColor (yup::Color (100, 150, 255));
        g.setStrokeWidth (1.5f);
        g.strokePath (curvePath);

        // Draw points along the path - smaller
        for (float t = 0.0f; t <= 1.0f; t += 0.25f)
        {
            yup::Point<float> point = curvePath.getPointAlongPath (t);
            yup::Path pointPath;
            pointPath.addCenteredEllipse (point, 3, 3);
            g.setFillColor (yup::Color (255, 100, 100));
            g.fillPath (pointPath);
        }
    }

    void drawPathUtilities (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Path Utilities", area);
        area = area.reduced (5).withTrimmedTop (20);

        float x = area.getX() + 10;
        float y = area.getY() + 10;

        // AppendPath demo - smaller
        yup::Path path1;
        path1.addEllipse (x, y, 30, 30);

        yup::Path path2;
        path2.addRectangle (x + 15, y + 15, 30, 30);

        path1.appendPath (path2);
        g.setFillColor (yup::Color (255, 200, 255));
        g.fillPath (path1);
        g.setStrokeColor (yup::Color (200, 100, 200));
        g.setStrokeWidth (1.0f);
        g.strokePath (path1);

        // Bounds demonstration
        yup::Path boundsPath;
        boundsPath.addStar (yup::Point<float> (x + 80, y + 20), 5, 10, 18, 0);
        yup::Rectangle<float> bounds = boundsPath.getBounds();

        g.setFillColor (yup::Color (180, 255, 180));
        g.fillPath (boundsPath);
        g.setStrokeColor (yup::Color (255, 100, 100));
        g.setStrokeWidth (1.0f);
        g.strokeRect (bounds);

        // Size and clear demo
        yup::Path infoPath;
        infoPath.addPolygon (yup::Point<float> (x + 40, y + 50), 6, 15, 0);

        g.setFillColor (yup::Color (200, 220, 255));
        g.fillPath (infoPath);
    }

    void drawSVGPathData (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "SVG Path Data", area);
        area = area.reduced (5).withTrimmedTop (20);

        float x = area.getX() + 10;
        float y = area.getY() + 10;

        // Parse SVG path data examples - smaller scale
        yup::Path svgHeart;
        svgHeart.fromString ("M12,21.35l-1.45-1.32C5.4,15.36,2,12.28,2,8.5 C2,5.42,4.42,3,7.5,3c1.74,0,3.41,0.81,4.5,2.09C13.09,3.81,14.76,3,16.5,3 C19.58,3,22,5.42,22,8.5c0,3.78-3.4,6.86-8.55,11.54L12,21.35z");

        // Scale and position the heart - smaller
        yup::Rectangle<float> heartBounds = svgHeart.getBounds();
        float scale = 1.8f;
        svgHeart.transform (yup::AffineTransform::scaling (scale));
        svgHeart.transform (yup::AffineTransform::translation (x - heartBounds.getX() * scale, y - heartBounds.getY() * scale));

        g.setFillColor (yup::Color (255, 150, 150));
        g.fillPath (svgHeart);
        g.setStrokeColor (yup::Color (200, 100, 100));
        g.setStrokeWidth (1.0f);
        g.strokePath (svgHeart);

        // Simple SVG path - smaller
        yup::Path svgTriangle;
        svgTriangle.fromString ("M100,20 L180,160 L20,160 Z");
        svgTriangle.scaleToFit (x + 60, y, 50, 50, true);

        g.setFillColor (yup::Color (150, 255, 150));
        g.fillPath (svgTriangle);
        g.setStrokeColor (yup::Color (100, 200, 100));
        g.setStrokeWidth (1.0f);
        g.strokePath (svgTriangle);
    }

    void drawCreativeExamples (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Creative Examples", area);
        area = area.reduced (5).withTrimmedTop (20);

        float x = area.getX() + 40;
        float y = area.getY() + 40;

        // Flower pattern using multiple shapes - smaller
        yup::Point<float> center (x, y);

        // Petals - smaller
        for (int i = 0; i < 6; ++i)
        {
            float angle = i * yup::MathConstants<float>::twoPi / 6.0f;
            yup::Path petal;
            petal.addCenteredEllipse (yup::Point<float> (0, 0), 8, 18);
            petal.transform (yup::AffineTransform::rotation (angle));
            petal.transform (yup::AffineTransform::translation (center.getX(), center.getY()));

            float hue = i / 6.0f;
            g.setFillColor (yup::Color::fromHSV (hue, 0.7f, 1.0f, 0.8f));
            g.fillPath (petal);
        }

        // Center
        yup::Path flowerCenter;
        flowerCenter.addCenteredEllipse (center, 8, 8);
        g.setFillColor (yup::Color (255, 255, 100));
        g.fillPath (flowerCenter);
        g.setStrokeColor (yup::Color (200, 200, 50));
        g.setStrokeWidth (1.0f);
        g.strokePath (flowerCenter);

        // Gear shape using polygons - smaller
        yup::Path gear;
        gear.addPolygon (yup::Point<float> (x + 120, y), 10, 25, 0);
        yup::Path innerGear;
        innerGear.addPolygon (yup::Point<float> (x + 120, y), 10, 18, yup::MathConstants<float>::pi / 10);

        g.setFillColor (yup::Color (180, 180, 180));
        g.fillPath (gear);
        g.setFillColor (yup::Color (220, 220, 220));
        g.fillPath (innerGear);
        g.setStrokeColor (yup::Color (100, 100, 100));
        g.strokePath (gear);

        // Center hole
        yup::Path centerHole;
        centerHole.addCenteredEllipse (yup::Point<float> (x + 120, y), 6, 6);
        g.setFillColor (yup::Color (245, 245, 250));
        g.fillPath (centerHole);
    }

    void drawInteractiveDemo (yup::Graphics& g, yup::Rectangle<float> area)
    {
        drawSectionTitle (g, "Interactive Demo", area);
        area = area.reduced (5).withTrimmedTop (20);

        // Create a complex path combining multiple features - mobile optimized
        yup::Path masterPath;

        float centerX = area.getCenterX();
        float centerY = area.getCenterY();

        // Base shape - rounded rectangle (smaller)
        masterPath.addRoundedRectangle (centerX - 100, centerY - 25, 200, 50, 12);

        // Add decorative elements (fewer)
        for (int i = 0; i < 3; ++i)
        {
            yup::Path star;
            float x = centerX - 60 + i * 60;
            star.addStar (yup::Point<float> (x, centerY - 40), 5, 5, 10, 0);
            masterPath.appendPath (star);
        }

        // Add speech bubble (smaller)
        yup::Path bubble;
        yup::Rectangle<float> bubbleBody (centerX + 110, centerY - 20, 60, 30);
        bubble.addBubble (bubbleBody, bubbleBody.enlarged (8), yup::Point<float> (centerX + 90, centerY), 6, 10);
        masterPath.appendPath (bubble);

        // Add connecting arc (smaller)
        yup::Path arc;
        arc.addCenteredArc (yup::Point<float> (centerX, centerY + 40), 150, 15, 0, -yup::MathConstants<float>::pi, 0, true);
        masterPath.appendPath (arc);

        // Render with gradient-like effect
        g.setFillColor (yup::Color (100, 150, 255).withAlpha (0.3f));
        g.fillPath (masterPath);
        g.setStrokeColor (yup::Color (50, 100, 200));
        g.setStrokeWidth (1.5f);
        g.strokePath (masterPath);
    }
};
