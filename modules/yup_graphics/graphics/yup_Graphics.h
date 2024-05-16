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

namespace yup
{

//==============================================================================
class GraphicsContext;

//==============================================================================
/** A graphical interface for drawing operations within a defined rendering context.

    This class provides a high-level drawing interface to perform various graphical
    operations such as drawing shapes, setting colors, and managing the state of a
    drawing context. It utilizes an underlying GraphicsContext and Renderer for
    actual drawing operations.
*/
class JUCE_API Graphics
{
public:

    //==============================================================================
    /** Represents a saved state of the Graphics object.

        This class allows Graphics states to be saved and restored, which is useful for complex rendering tasks where
        temporary changes to the state are made and later reverted.
    */
    class JUCE_API SavedState
    {
    public:
        /** Constructs a SavedState linked to a specific Graphics object.

            @param g Reference to the Graphics object whose state is to be saved.
        */
        SavedState (Graphics& g);

        /** Move constructor and assignment operator. */
        SavedState (SavedState&&);
        SavedState& operator= (SavedState&&);

        /** Deleted copy constructor/assignment operators to avoid unintentional copying. */
        SavedState (const SavedState&) = delete;
        SavedState& operator= (const SavedState&) = delete;

        /** Destroys the SavedState, potentially restoring the Graphics state. */
        ~SavedState();

    private:
        Graphics* g = nullptr;
    };

    //==============================================================================
    /** Constructs a Graphics object with a specific GraphicsContext and Renderer.

        @param context Reference to the GraphicsContext that defines the drawing surface and capabilities.
        @param renderer Reference to the Renderer that executes the drawing commands.
    */
    Graphics (GraphicsContext& context, rive::Renderer& renderer) noexcept;

    //==============================================================================
    /** Saves the current state of the Graphics object.

        @return A SavedState object that can be used to restore the state later.
    */
    [[nodiscard]] SavedState saveState();

    //==============================================================================
    /** Sets the current drawing color.

        @param color The new color to use for subsequent drawing operations.
    */
    void setColor (Color color);

    /** Retrieves the current drawing color.

        @return The current color.
    */
    Color getColor() const;

    /** Sets the current color gradient.

        @param gradient The new color gradient to use for subsequent drawing operations.
    */
    void setColorGradient (ColorGradient gradient);

    /** Retrieves the current color gradient.

        @return The current color gradient.
    */
    ColorGradient getColorGradient() const;

    /** Sets the opacity for subsequent drawing operations.

        @param opacity The new opacity level (0-255).
    */
    void setOpacity (uint8 opacity);

    /** Retrieves the current opacity setting.

        @return The current opacity level.
    */
    uint8 getOpacity() const;

    /** Sets the stroke join style for drawing lines and paths.

        @param join The join style to use.
    */
    void setStrokeJoin (StrokeJoin join);

    /** Retrieves the current stroke join style.

        @return The stroke join style.
    */
    StrokeJoin getStrokeJoin() const;

    /** Sets the stroke cap style for drawing lines and paths.

        @param cap The cap style to use.
    */
    void setStrokeCap (StrokeCap cap);

    /** Retrieves the current stroke cap style.

        @return The stroke cap style.
    */
    StrokeCap getStrokeCap() const;

    /** Defines the area within which drawing operations are clipped.

        @param r The rectangle that defines the clipping region.
    */
    void setDrawingArea (const Rectangle<float>& r);

    /** Retrieves the current drawing area, within which all drawing operations are clipped.

        @return The current clipping rectangle.
    */
    Rectangle<float> getDrawingArea() const;

    /** Defines the affine transformation to use when drawing.

        @param transform The affine transformation used when drawing.
    */
    void setTransform (const AffineTransform& transform);

    /** Retrieves the current affine transformation.

        @return The current affine transformation.
    */
    AffineTransform getTransform() const;

    //==============================================================================
    /** Draws a line between two points with a specified thickness.

        @param x1 The x-coordinate of the first point.
        @param y1 The y-coordinate of the first point.
        @param x2 The x-coordinate of the second point.
        @param y2 The y-coordinate of the second point.
        @param thickness The thickness of the line.
    */
    void drawLine (float x1, float y1, float x2, float y2, float thickness);

    /** Draws a line between two points with a specified thickness.

        @param p1 The first point.
        @param p2 The second point.
        @param thickness The thickness of the line.
    */
    void drawLine (const Point<float>& p1, const Point<float>& p2, float thickness);

    //==============================================================================
    /** Fills the entire drawing area with the current color or gradient.
    */
    void fillAll();

    //==============================================================================
    /** Fills a rectangle with the current color or gradient.

        @param x The x-coordinate of the rectangle's top-left corner.
        @param y The y-coordinate of the rectangle's top-left corner.
        @param width The width of the rectangle.
        @param height The height of the rectangle.
    */
    void fillRect (float x, float y, float width, float height);

    /** Fills a rectangle with the current color or gradient.

        @param r The rectangle to fill.
    */
    void fillRect (const Rectangle<float>& r);

    /** Draws a rectangle with a specified thickness.

        @param x The x-coordinate of the rectangle's top-left corner.
        @param y The y-coordinate of the rectangle's top-left corner.
        @param width The width of the rectangle.
        @param height The height of the rectangle.
        @param thickness The thickness of the line used to draw the rectangle.
    */
    void drawRect (float x, float y, float width, float height, float thickness);

    /** Draws a rectangle with a specified thickness.

        @param r The rectangle to draw.
        @param thickness The thickness of the line used to draw the rectangle.
    */
    void drawRect (const Rectangle<float>& r, float thickness);

    //==============================================================================
    /** Fills a rounded rectangle with specific corner radii.

        @param x The x-coordinate of the rounded rectangle's top-left corner.
        @param y The y-coordinate of the rounded rectangle's top-left corner.
        @param width The width of the rounded rectangle.
        @param height The height of the rounded rectangle.
        @param radiusTopLeft The radius of the top-left corner.
        @param radiusTopRight The radius of the top-right corner.
        @param radiusBottomLeft The radius of the bottom-left corner.
        @param radiusBottomRight The radius of the bottom-right corner.
    */
    void fillRoundedRect (float x, float y, float width, float height, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight);

    /** Fills a rounded rectangle with specific corner radii.

        @param r The rectangle to fill.
        @param radiusTopLeft The radius of the top-left corner.
        @param radiusTopRight The radius of the top-right corner.
        @param radiusBottomLeft The radius of the bottom-left corner.
        @param radiusBottomRight The radius of the bottom-right corner.
    */
    void fillRoundedRect (const Rectangle<float>& r, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight);

    /** Fills a rounded rectangle with a uniform corner radius.

        @param x The x-coordinate of the rounded rectangle's top-left corner.
        @param y The y-coordinate of the rounded rectangle's top-left corner.
        @param width The width of the rounded rectangle.
        @param height The height of the rounded rectangle.
        @param radius The radius of all corners.
    */
    void fillRoundedRect (float x, float y, float width, float height, float radius);

    /** Fills a rounded rectangle with a uniform corner radius.

        @param r The rectangle to fill.
        @param radius The radius of all corners.
    */
    void fillRoundedRect (const Rectangle<float>& r, float radius);

    /** Draws a rounded rectangle with specific corner radii and a specified thickness.

        @param x The x-coordinate of the rounded rectangle's top-left corner.
        @param y The y-coordinate of the rounded rectangle's top-left corner.
        @param width The width of the rounded rectangle.
        @param height The height of the rounded rectangle.
        @param radiusTopLeft The radius of the top-left corner.
        @param radiusTopRight The radius of the top-right corner.
        @param radiusBottomLeft The radius of the bottom-left corner.
        @param radiusBottomRight The radius of the bottom-right corner.
        @param thickness The thickness of the line used to draw the rounded rectangle.
    */
    void drawRoundedRect (float x, float y, float width, float height, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight, float thickness);

    /** Draws a rounded rectangle with specific corner radii and a specified thickness.

        @param r The rectangle to draw.
        @param radiusTopLeft The radius of the top-left corner.
        @param radiusTopRight The radius of the top-right corner.
        @param radiusBottomLeft The radius of the bottom-left corner.
        @param radiusBottomRight The radius of the bottom-right corner.
        @param thickness The thickness of the line used to draw the rounded rectangle.
    */
    void drawRoundedRect (const Rectangle<float>& r, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight, float thickness);

    /** Draws a rounded rectangle with a uniform corner radius and a specified thickness.

        @param x The x-coordinate of the rounded rectangle's top-left corner.
        @param y The y-coordinate of the rounded rectangle's top-left corner.
        @param width The width of the rounded rectangle.
        @param height The height of the rounded rectangle.
        @param radius The radius of all corners.
        @param thickness The thickness of the line used to draw the rounded rectangle.
    */
    void drawRoundedRect (float x, float y, float width, float height, float radius, float thickness);

    /** Draws a rounded rectangle with a uniform corner radius and a specified thickness.

        @param r The rectangle to draw.
        @param radius The radius of all corners.
        @param thickness The thickness of the line used to draw the rounded rectangle.
    */
    void drawRoundedRect (const Rectangle<float>& r, float radius, float thickness);

    //==============================================================================
    /** Draws a path with a specified thickness.

        @param path The path to draw.
        @param thickness The thickness of the line used to draw the path.
    */
    void drawPath (const Path& path, float thickness);

    /** Fills a path with the current color or gradient.

        @param path The path to fill.
    */
    void fillPath (const Path& path);

    //==============================================================================
    /** Draws an attributed text.
    */
    void drawFittedText (const StyledText& text, const Rectangle<float>& rect, rive::TextAlign align = rive::TextAlign::center);

    //==============================================================================
    /** Clips the drawing area to the specified rectangle.

        @param r The rectangle to clip to.
    */
    void clipPath (const Rectangle<float>& r);

    /** Clips the drawing area to the specified path.

        @param path The path to clip to.
    */
    void clipPath (const Path& path);

    //==============================================================================
    /** Retrieves the factory used for creating graphical objects.

        @return Pointer to a rive::Factory object.
    */
    rive::Factory* getFactory();

    /** Retrieves the renderer used for drawing operations.

        @return Pointer to a rive::Renderer object.
    */
    rive::Renderer* getRenderer();

private:
    struct RenderOptions
    {
        constexpr RenderOptions() noexcept = default;

        constexpr bool isColor() const noexcept
        {
            return isCurrentBrushColor;
        }

        constexpr Color getColor() const noexcept
        {
            return color.withAlpha (alpha);
        }

        constexpr bool isColorGradient() const noexcept
        {
            return ! isCurrentBrushColor;
        }

        constexpr ColorGradient getColorGradient() const noexcept
        {
            return gradient.withAlpha (alpha);
        }

        constexpr const Rectangle<float>& getDrawingArea() const noexcept
        {
            return drawingArea;
        }

        constexpr AffineTransform getTransform() const noexcept
        {
            return transform
                .translated (drawingArea.getX(), drawingArea.getY());
        }

        StrokeJoin join = StrokeJoin::Miter;
        StrokeCap cap = StrokeCap::Square;
        Color color = 0xff000000;
        ColorGradient gradient;
        Rectangle<float> drawingArea;
        AffineTransform transform;
        uint8 alpha = 255;
        bool isCurrentBrushColor = true;
    };

    RenderOptions& currentRenderOptions();
    const RenderOptions& currentRenderOptions() const;
    void restoreState();

    void renderDrawPath (rive::RawPath& rawPath, const RenderOptions& options, float thickness);
    void renderFillPath (rive::RawPath& rawPath, const RenderOptions& options);

    GraphicsContext& context;

    rive::Factory& factory;
    rive::Renderer& renderer;

    std::vector<RenderOptions> renderOptions;
};

} // namespace yup
