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

class SliderDemo : public yup::Component
{
public:
    SliderDemo()
        : Component ("SliderDemo")
    {
        setupSliders();
        setupLabels();
    }

private:
    void setupSliders()
    {
        // Horizontal Linear Slider
        horizontalSlider = std::make_unique<yup::Slider>(yup::Slider::LinearHorizontal);
        horizontalSlider->setRange(0.0, 100.0);
        horizontalSlider->setValue(50.0);
        horizontalSlider->onValueChanged = [this](double value) {
            horizontalLabel->setText("Horizontal: " + yup::String(value, 1), yup::dontSendNotification);
        };
        addAndMakeVisible(horizontalSlider.get());

        // Vertical Linear Slider
        verticalSlider = std::make_unique<yup::Slider>(yup::Slider::LinearVertical);
        verticalSlider->setRange(0.0, 100.0);
        verticalSlider->setValue(30.0);
        verticalSlider->onValueChanged = [this](double value) {
            verticalLabel->setText("Vertical: " + yup::String(value, 1), yup::dontSendNotification);
        };
        addAndMakeVisible(verticalSlider.get());

        // Rotary Slider (Horizontal Drag)
        rotarySlider = std::make_unique<yup::Slider>(yup::Slider::RotaryHorizontalDrag);
        rotarySlider->setRange(0.0, 100.0);
        rotarySlider->setValue(70.0);
        rotarySlider->onValueChanged = [this](double value) {
            rotaryLabel->setText("Rotary: " + yup::String(value, 1), yup::dontSendNotification);
        };
        addAndMakeVisible(rotarySlider.get());

        // Bar Horizontal Slider
        barHorizontalSlider = std::make_unique<yup::Slider>(yup::Slider::LinearBarHorizontal);
        barHorizontalSlider->setRange(0.0, 100.0);
        barHorizontalSlider->setValue(75.0);
        barHorizontalSlider->onValueChanged = [this](double value) {
            barHorizontalLabel->setText("Bar H: " + yup::String(value, 0) + "%", yup::dontSendNotification);
        };
        addAndMakeVisible(barHorizontalSlider.get());

        // Bar Vertical Slider
        barVerticalSlider = std::make_unique<yup::Slider>(yup::Slider::LinearBarVertical);
        barVerticalSlider->setRange(0.0, 10.0);
        barVerticalSlider->setValue(6.0);
        barVerticalSlider->onValueChanged = [this](double value) {
            barVerticalLabel->setText("Bar V: " + yup::String(value, 1), yup::dontSendNotification);
        };
        addAndMakeVisible(barVerticalSlider.get());

        // Two Value Horizontal Slider
        twoValueSlider = std::make_unique<yup::Slider>(yup::Slider::TwoValueHorizontal);
        twoValueSlider->setRange(0.0, 100.0);
        twoValueSlider->setMinValue(25.0);
        twoValueSlider->setMaxValue(75.0);
        twoValueSlider->onValueChanged = [this](double) {
            twoValueLabel->setText("Range: " + yup::String(twoValueSlider->getMinValue(), 0) +
                                  "-" + yup::String(twoValueSlider->getMaxValue(), 0), yup::dontSendNotification);
        };
        addAndMakeVisible(twoValueSlider.get());
    }

    void setupLabels()
    {
        // Title
        titleLabel = std::make_unique<yup::Label>("title");
        titleLabel->setText("YUP Slider Demo", yup::dontSendNotification);
        addAndMakeVisible(titleLabel.get());

        // Value labels
        horizontalLabel = std::make_unique<yup::Label>("value1");
        horizontalLabel->setText("Horizontal: 50.0", yup::dontSendNotification);
        addAndMakeVisible(horizontalLabel.get());

        verticalLabel = std::make_unique<yup::Label>("value2");
        verticalLabel->setText("Vertical: 30.0", yup::dontSendNotification);
        addAndMakeVisible(verticalLabel.get());

        rotaryLabel = std::make_unique<yup::Label>("value3");
        rotaryLabel->setText("Rotary: 70.0", yup::dontSendNotification);
        addAndMakeVisible(rotaryLabel.get());

        barHorizontalLabel = std::make_unique<yup::Label>("value4");
        barHorizontalLabel->setText("Bar H: 75%", yup::dontSendNotification);
        addAndMakeVisible(barHorizontalLabel.get());

        barVerticalLabel = std::make_unique<yup::Label>("value5");
        barVerticalLabel->setText("Bar V: 6.0", yup::dontSendNotification);
        addAndMakeVisible(barVerticalLabel.get());

        twoValueLabel = std::make_unique<yup::Label>("value6");
        twoValueLabel->setText("Range: 25-75", yup::dontSendNotification);
        addAndMakeVisible(twoValueLabel.get());
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto margin = 20;
        auto sliderHeight = 60;
        auto labelHeight = 25;
        auto spacing = 10;

        int y = margin;

        // Title
        titleLabel->setBounds(yup::Rectangle<float>(
            static_cast<float>(margin),
            static_cast<float>(y),
            static_cast<float>(bounds.getWidth() - 2 * margin),
            30.0f));
        y += 40;

        // Layout in a 2x3 grid
        auto sliderWidth = (bounds.getWidth() - 3 * margin) / 2;
        auto columnHeight = sliderHeight + labelHeight + spacing;

        // Left column
        horizontalSlider->setBounds(yup::Rectangle<float>(
            static_cast<float>(margin),
            static_cast<float>(y),
            static_cast<float>(sliderWidth),
            static_cast<float>(sliderHeight)));
        horizontalLabel->setBounds(yup::Rectangle<float>(
            static_cast<float>(margin),
            static_cast<float>(y + sliderHeight + 5),
            static_cast<float>(sliderWidth),
            static_cast<float>(labelHeight)));

        barHorizontalSlider->setBounds(yup::Rectangle<float>(
            static_cast<float>(margin),
            static_cast<float>(y + columnHeight),
            static_cast<float>(sliderWidth),
            static_cast<float>(sliderHeight)));
        barHorizontalLabel->setBounds(yup::Rectangle<float>(
            static_cast<float>(margin),
            static_cast<float>(y + columnHeight + sliderHeight + 5),
            static_cast<float>(sliderWidth),
            static_cast<float>(labelHeight)));

        twoValueSlider->setBounds(yup::Rectangle<float>(
            static_cast<float>(margin),
            static_cast<float>(y + 2 * columnHeight),
            static_cast<float>(sliderWidth),
            static_cast<float>(sliderHeight)));
        twoValueLabel->setBounds(yup::Rectangle<float>(
            static_cast<float>(margin),
            static_cast<float>(y + 2 * columnHeight + sliderHeight + 5),
            static_cast<float>(sliderWidth),
            static_cast<float>(labelHeight)));

        // Right column
        auto rightX = margin + sliderWidth + margin;

        verticalSlider->setBounds(yup::Rectangle<float>(
            static_cast<float>(rightX),
            static_cast<float>(y),
            80.0f,
            static_cast<float>(columnHeight)));
        verticalLabel->setBounds(yup::Rectangle<float>(
            static_cast<float>(rightX + 90),
            static_cast<float>(y),
            static_cast<float>(sliderWidth - 90),
            static_cast<float>(labelHeight)));

        rotarySlider->setBounds(yup::Rectangle<float>(
            static_cast<float>(rightX),
            static_cast<float>(y + columnHeight),
            80.0f,
            80.0f));
        rotaryLabel->setBounds(yup::Rectangle<float>(
            static_cast<float>(rightX + 90),
            static_cast<float>(y + columnHeight),
            static_cast<float>(sliderWidth - 90),
            static_cast<float>(labelHeight)));

        barVerticalSlider->setBounds(yup::Rectangle<float>(
            static_cast<float>(rightX),
            static_cast<float>(y + 2 * columnHeight),
            60.0f,
            static_cast<float>(columnHeight)));
        barVerticalLabel->setBounds(yup::Rectangle<float>(
            static_cast<float>(rightX + 70),
            static_cast<float>(y + 2 * columnHeight),
            static_cast<float>(sliderWidth - 70),
            static_cast<float>(labelHeight)));
    }

    void paint(yup::Graphics& g) override
    {
        g.setFillColor(yup::Colors::dimgray);
        g.fillAll();

        // Draw section dividers
        g.setStrokeColor(yup::Colors::gray.withAlpha(0.3f));
        g.setStrokeWidth(1.0f);

        auto bounds = getLocalBounds();
        auto margin = 20;

        // Horizontal line under title
        g.strokeLine(static_cast<float>(margin), 70.0f, static_cast<float>(bounds.getWidth() - margin), 70.0f);

        // Vertical line separating columns
        auto centerX = bounds.getWidth() / 2;
        g.strokeLine(static_cast<float>(centerX), 80.0f, static_cast<float>(centerX), static_cast<float>(bounds.getHeight() - margin));
    }

private:
    // Title
    std::unique_ptr<yup::Label> titleLabel;

    // Sliders
    std::unique_ptr<yup::Slider> horizontalSlider;
    std::unique_ptr<yup::Slider> verticalSlider;
    std::unique_ptr<yup::Slider> rotarySlider;
    std::unique_ptr<yup::Slider> barHorizontalSlider;
    std::unique_ptr<yup::Slider> barVerticalSlider;
    std::unique_ptr<yup::Slider> twoValueSlider;

    // Labels
    std::unique_ptr<yup::Label> horizontalLabel;
    std::unique_ptr<yup::Label> verticalLabel;
    std::unique_ptr<yup::Label> rotaryLabel;
    std::unique_ptr<yup::Label> barHorizontalLabel;
    std::unique_ptr<yup::Label> barVerticalLabel;
    std::unique_ptr<yup::Label> twoValueLabel;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliderDemo)
};
