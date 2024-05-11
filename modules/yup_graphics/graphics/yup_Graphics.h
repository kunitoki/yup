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
class JUCE_API Graphics
{
public:

    //==============================================================================
    class JUCE_API SavedState
    {
    public:
        SavedState (Graphics& g);

        SavedState (SavedState&&);
        SavedState& operator= (SavedState&&);

        SavedState (const SavedState&) = delete;
        SavedState& operator= (const SavedState&) = delete;

        ~SavedState();

    private:
        Graphics* g = nullptr;
    };

    //==============================================================================
    Graphics (GraphicsContext& context, rive::Renderer& renderer) noexcept;

    //==============================================================================
    SavedState saveState();

    //==============================================================================
    void setColor (Color color);
    Color getColor() const;

    void setColorGradient (ColorGradient gradient);
    ColorGradient getColorGradient() const;

    void setOpacity (uint8 opacity);
    uint8 getOpacity() const;

    void setStrokeJoin (StrokeJoin join);
    StrokeJoin getStrokeJoin() const;

    void setStrokeCap (StrokeCap cap);
    StrokeCap getStrokeCap() const;

    void setDrawingArea (const Rectangle<float>& r);
    Rectangle<float> getDrawingArea() const;

    //==============================================================================
    void drawLine (float x1, float y1, float x2, float y2, float thickness);
    void drawLine (const Point<float>& p1, const Point<float>& p2, float thickness);

    //==============================================================================
    void fillRect (float x, float y, float width, float height);
    void fillRect (const Rectangle<float>& r);

    void drawRect (float x, float y, float width, float height, float thickness);
    void drawRect (const Rectangle<float>& r, float thickness);

    //==============================================================================
    void fillRoundedRect (float x, float y, float width, float height, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight);
    void fillRoundedRect (const Rectangle<float>& r, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight);
    void fillRoundedRect (float x, float y, float width, float height, float radius);
    void fillRoundedRect (const Rectangle<float>& r, float radius);

    void drawRoundedRect (float x, float y, float width, float height, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight, float thickness);
    void drawRoundedRect (const Rectangle<float>& r, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight, float thickness);
    void drawRoundedRect (float x, float y, float width, float height, float radius, float thickness);
    void drawRoundedRect (const Rectangle<float>& r, float radius, float thickness);

    //==============================================================================
    void drawPath (const Path& path, float thickness);
    void fillPath (const Path& path);

    //==============================================================================
    rive::Factory* getFactory();
    rive::Renderer* getRenderer();

private:
    struct RenderOptions
    {
        RenderOptions() noexcept = default;

        bool isColor() const noexcept
        {
            return isCurrentBrushColor;
        }

        Color getColor() const noexcept
        {
            return color;
        }

        bool isColorGradient() const noexcept
        {
            return ! isCurrentBrushColor;
        }

        ColorGradient getColorGradient() const noexcept
        {
            return gradient;
        }

        float translateX (float x) const noexcept
        {
            return x + drawingArea.getX();
        }

        float translateY (float y) const noexcept
        {
            return y + drawingArea.getY();
        }

        StrokeJoin join = StrokeJoin::Miter;
        StrokeCap cap = StrokeCap::Square;
        Color color = 0xff000000;
        ColorGradient gradient;
        Rectangle<float> drawingArea;
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
