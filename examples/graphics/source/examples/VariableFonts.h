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

class VariableFontsExample : public yup::Component
{
public:
    VariableFontsExample (const yup::Font& font)
        : Component ("VariableFontsExample")
        , font (font)
    {
        text =
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed non aliquam risus, eu molestie sem. "
            "Proin fringilla porttitor metus at pharetra. Nunc quis lacus gravida justo pellentesque dignissim a "
            "convallis justo. Morbi suscipit ultricies risus commodo accumsan. Suspendisse maximus lacinia odio, "
            "et sodales massa gravida sed. Aliquam quis purus pellentesque, vestibulum tortor ut, tincidunt libero. "
            "Nulla et tincidunt lectus. Sed molestie, eros id ultrices tempor, justo lectus lobortis eros, in pretium "
            "est nisl in risus. Cras aliquam, est eget luctus hendrerit, ante ligula bibendum lorem, a posuere eros "
            "lectus nec sem. Sed posuere eu tellus sed scelerisque. Fusce non sem in quam commodo finibus. Pellentesque "
            "sed elit nec purus condimentum ullamcorper eget at elit. Suspendisse accumsan nisi quis odio venenatis "
            "tincidunt. Maecenas facilisis libero sed vehicula accumsan. Quisque sed justo nisl.";

        {
            addControl ("FontSize", 0, 16.0f, 4.0f, 32.0f, fontSize);
            addControl ("Stroke", 1, 0.0f, 0.0f, 8.0f, strokeWidth);
            addControl ("Feather", 2, 0.0f, 0.0f, 10.0f, feather);
        }

        const int offsetIndex = sliders.size();
        for (int index = 0; index < font.getNumAxis(); ++index)
        {
            auto axisInfo = font.getAxisDescription (index);
            if (! axisInfo)
                continue;

            auto label = labels.add (std::make_unique<yup::Label>());
            label->setFont (font);
            addAndMakeVisible (label);

            auto slider = sliders.add (std::make_unique<yup::Slider> (axisInfo->tagName, font));
            slider->setDefaultValue (axisInfo->defaultValue);
            slider->setRange ({ axisInfo->minimumValue, axisInfo->maximumValue });
            slider->setValue (axisInfo->defaultValue);
            slider->onValueChanged = [this, index, offsetIndex] (float value)
            {
                updateLabel (index + offsetIndex);

                this->font = this->font.withAxisValue (index, value);

                resized();
                repaint (textBounds);
            };
            addAndMakeVisible (slider);

            updateLabel (index + offsetIndex);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        textBounds = bounds.removeFromBottom (proportionOfHeight (0.5f)).reduced (10);

        styledText.setMaxSize (textBounds.getSize());
        styledText.setHorizontalAlign (yup::StyledText::left);
        styledText.setVerticalAlign (yup::StyledText::middle);
        styledText.setOverflow (yup::StyledText::fit);
        styledText.setWrap (yup::StyledText::wrap);
        styledText.clear();
        styledText.appendText (text, nullptr, font.getFont(), fontSize);
        styledText.update();

        bounds = bounds.reduced (10);

        int slidersPerRow = 6;
        int sliderWidth = bounds.getWidth() / slidersPerRow;
        int labelHeight = 16;
        int sliderIndex = 0;

        while (sliderIndex < sliders.size())
        {
            auto sliderBounds = bounds.removeFromTop (sliderWidth + labelHeight);

            int index = 0;
            for (; index < slidersPerRow; ++index)
            {
                if (sliderIndex + index >= sliders.size())
                    break;

                auto sb = sliderBounds.removeFromLeft (bounds.proportionOfWidth (1.0f / slidersPerRow));

                sliders.getUnchecked (sliderIndex + index)->setBounds (sb.removeFromTop (sliderWidth));

                labels.getUnchecked (sliderIndex + index)->setBounds (sb);
            }

            sliderIndex += index;
        }
    }

    void paint (yup::Graphics& g) override
    {
        if (feather > 0.0f)
        {
            g.setFillColor (0xffffffff);
            g.setFeather (feather);
            g.fillFittedText (styledText, textBounds);
        }

        if (strokeWidth > 0.0f)
        {
            g.setStrokeColor (0xff000000);
            g.setStrokeWidth (strokeWidth);
            g.setStrokeCap (yup::StrokeCap::Round);
            g.setStrokeJoin (yup::StrokeJoin::Round);
            g.strokeFittedText (styledText, textBounds);
        }

        g.setFillColor (0xffffffff);
        g.setFeather (0.0f);
        g.fillFittedText (styledText, textBounds);
    }

private:
    void updateLabel (int index)
    {
        labels[index]->setText (sliders[index]->getComponentID() + ": " + yup::String (sliders[index]->getValue(), 2));
    }

    void addControl (yup::StringRef name, int index, float defaultValue, float minValue, float maxValue, float& valueToSet)
    {
        auto label = labels.add (std::make_unique<yup::Label>());
        label->setFont (font);
        addAndMakeVisible (label);

        auto slider = sliders.add (std::make_unique<yup::Slider> (name, font));
        slider->setDefaultValue (defaultValue);
        slider->setRange ({ minValue, maxValue });
        slider->setValue (defaultValue);
        slider->onValueChanged = [this, index, &valueToSet] (float value)
        {
            updateLabel (index);

            valueToSet = value;

            resized();
            repaint (textBounds);
        };
        addAndMakeVisible (slider);

        updateLabel (index);
    }

    yup::Font font;

    yup::String text;
    yup::StyledText styledText;
    yup::Rectangle<float> textBounds;
    float fontSize = 16.0f;
    float strokeWidth = 0.0f;
    float feather = 0.0f;

    yup::OwnedArray<yup::TextButton> buttons;
    yup::OwnedArray<yup::Slider> sliders;
    yup::OwnedArray<yup::Label> labels;
};
