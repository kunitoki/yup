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
            auto label = labels.add (std::make_unique<yup::Label>());
            label->setFont (font);
            addAndMakeVisible (label);

            auto slider = sliders.add (std::make_unique<yup::Slider>("FontSize", font));
            slider->setDefaultValue (16.0f);
            slider->setRange ({ 4.0f, 32.0f });
            slider->setValue (16.0f);
            slider->onValueChanged = [this] (float value)
            {
                updateLabel (0);

                fontSize = value;

                resized();
                repaint (textBounds);
            };
            addAndMakeVisible (slider);
        }

        for (int index = 0; index < font.getNumAxis(); ++index)
        {
            auto axisInfo = font.getAxisDescription (index);
            if (! axisInfo)
                continue;

            auto label = labels.add (std::make_unique<yup::Label>());
            label->setFont (font);
            addAndMakeVisible (label);

            auto slider = sliders.add (std::make_unique<yup::Slider>(axisInfo->tagName, font));
            slider->setDefaultValue (axisInfo->defaultValue);
            slider->setRange ({ axisInfo->minimumValue, axisInfo->maximumValue });
            slider->setValue (axisInfo->defaultValue);
            slider->onValueChanged = [this, index] (float value)
            {
                updateLabel (index + 1);

                this->font = this->font.withAxisValue (index, value);

                resized();
                repaint (textBounds);
            };
            addAndMakeVisible (slider);

            updateLabel (index);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        textBounds = bounds.removeFromBottom (proportionOfHeight (0.5f)).reduced (10);

        styledText.setMaxSize (textBounds.getSize());
        styledText.setHorizontalAlign (yup::StyledText::right);
        styledText.setVerticalAlign (yup::StyledText::middle);
        styledText.setWrap (yup::StyledText::wrap);
        styledText.clear();
        styledText.appendText (text, nullptr, font.getFont(), fontSize);
        styledText.update();

        bounds = bounds.reduced (10);

        int slidersPerRow = 5;
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
        g.setStrokeColor (0xffff0000);
        g.strokeFittedText (styledText, textBounds);

        g.setFillColor (0xffffffff);
        g.fillFittedText (styledText, textBounds);
    }

private:
    void updateLabel (int index)
    {
        labels[index]->setText (sliders[index]->getComponentID() + ": " + yup::String (sliders[index]->getValue(), 2));
    }

    yup::Font font;

    yup::String text;
    yup::StyledText styledText;
    yup::Rectangle<float> textBounds;
    float fontSize = 16.0f;

    yup::OwnedArray<yup::Slider> sliders;
    yup::OwnedArray<yup::Label> labels;
};
