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

class LayoutFontsExample : public yup::Component
{
public:
    LayoutFontsExample (const yup::Font& font)
        : Component ("LayoutFontsExample")
        , font (font)
    {
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        const int numTexts = yup::numElementsInArray (text);
        for (int i = 0; i < numTexts; ++i)
        {
            const auto labelBounds = bounds.removeFromTop (getHeight() / (float) (numTexts + 1)).reduced (10.0f, 5.0f);
            const float fontSize = labelBounds.getHeight() * 0.4f;

            text[i].prepare (font, fontSize, labelBounds);

            bounds.removeFromTop ((getHeight() / (float) numTexts) / (float) numTexts);
        }
    }

    void paint (yup::Graphics& g) override
    {
        const int numTexts = yup::numElementsInArray (text);
        for (int i = 0; i < numTexts; ++i)
        {
            auto labelBounds = text[i].bounds;

            g.setFillColor (0xffffffff);
            g.setFeather (10.0f);
            g.fillFittedText (text[i].styledText, labelBounds);

            g.setFeather (0.0f);
            g.fillFittedText (text[i].styledText, labelBounds);

            /*
            g.setStrokeColor (yup::Colors::green);
            g.strokeLine (labelBounds.getX(), labelBounds.getTop(), labelBounds.getRight(), labelBounds.getTop());

            g.setStrokeColor (yup::Colors::magenta);
            g.strokeLine (labelBounds.getX(), labelBounds.getCenter().getY(), labelBounds.getRight(), labelBounds.getCenter().getY());

            g.setStrokeColor (yup::Colors::blue);
            g.strokeLine (labelBounds.getX(), labelBounds.getBottom(), labelBounds.getRight(), labelBounds.getBottom());
            */

            auto offset = text[i].styledText.getOffset (labelBounds);

            g.setStrokeColor (yup::Colors::magenta);
            const auto& lines = text[i].styledText.getOrderedLines();
            for (const auto& line : lines)
            {
                for (auto [glyph, _] : line)
                {
                    for (auto xPos : glyph->xpos)
                    {
                        g.strokeLine (
                            labelBounds.getX() + offset.getX() + xPos,
                            labelBounds.getTop() + offset.getY(),
                            labelBounds.getX() + offset.getX() + xPos,
                            labelBounds.getTop() + offset.getY() + text[i].styledText.getComputedTextBounds().getHeight());

                        //accum += advances; // / g.getContextScale();
                    }
                }
            }
        }
    }

private:
    yup::Font font;

    struct TextBox
    {
        yup::String text;
        yup::StyledText::HorizontalAlign hAlign;
        yup::StyledText::VerticalAlign vAlign;
        yup::StyledText::TextOverflow overflow;
        yup::StyledText::TextWrap wrap;
        yup::StyledText styledText;
        yup::Rectangle<float> bounds;

        void prepare (const yup::Font& font, float fontSize, const yup::Rectangle<float>& newBounds)
        {
            bounds = newBounds;

            auto modifier = styledText.startUpdate();
            modifier.setMaxSize (newBounds.getSize());
            modifier.setHorizontalAlign (hAlign);
            modifier.setVerticalAlign (vAlign);
            modifier.setParagraphSpacing (0.0f);
            modifier.setOverflow (overflow);
            modifier.setWrap (wrap);

            modifier.clear();
            modifier.appendText (text, nullptr, font.getFont(), fontSize);
        }
    };

    TextBox text[9] = {
        { "Left Top", yup::StyledText::left, yup::StyledText::top, yup::StyledText::ellipsis, yup::StyledText::noWrap },
        { "Center Top", yup::StyledText::center, yup::StyledText::top, yup::StyledText::ellipsis, yup::StyledText::noWrap },
        { "Right Top", yup::StyledText::right, yup::StyledText::top, yup::StyledText::ellipsis, yup::StyledText::noWrap },
        { "Left Middle", yup::StyledText::left, yup::StyledText::middle, yup::StyledText::ellipsis, yup::StyledText::noWrap },
        { "Center Middle", yup::StyledText::center, yup::StyledText::middle, yup::StyledText::ellipsis, yup::StyledText::noWrap },
        { "Right Middle", yup::StyledText::right, yup::StyledText::middle, yup::StyledText::ellipsis, yup::StyledText::noWrap },
        { "Left Bottom", yup::StyledText::left, yup::StyledText::bottom, yup::StyledText::ellipsis, yup::StyledText::noWrap },
        { "Center Bottom", yup::StyledText::center, yup::StyledText::bottom, yup::StyledText::ellipsis, yup::StyledText::noWrap },
        { "Right Bottom", yup::StyledText::right, yup::StyledText::bottom, yup::StyledText::ellipsis, yup::StyledText::noWrap },
    };
};
