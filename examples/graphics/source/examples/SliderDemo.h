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
        auto margin = 20.0f;
        auto sliderHeight = 60.0f;
        auto labelHeight = 25.0f;
        auto spacing = 10.0f;

        auto y = margin;

        // Title
        titleLabel->setBounds(margin, y, bounds.getWidth() - 2 * margin, 30.0f);
        y += 40.0f;

        // Layout in a 2x3 grid
        auto sliderWidth = (bounds.getWidth() - 3.0f * margin) / 2.0f;
        auto columnHeight = sliderHeight + labelHeight + spacing;

        // Left column
        horizontalSlider->setBounds(margin, y, sliderWidth, sliderHeight);
        horizontalLabel->setBounds(margin, y + sliderHeight + 5.0f, sliderWidth, labelHeight);

        barHorizontalSlider->setBounds(margin, y + columnHeight, sliderWidth, sliderHeight);
        barHorizontalLabel->setBounds(margin, y + columnHeight + sliderHeight + 5.0f, sliderWidth, labelHeight);

        twoValueSlider->setBounds(margin, y + 2.0f * columnHeight, sliderWidth, sliderHeight);
        twoValueLabel->setBounds(margin, y + 2.0f * columnHeight + sliderHeight + 5.0f, sliderWidth, labelHeight);

        // Right column
        auto rightX = margin + sliderWidth + margin;

        verticalSlider->setBounds(rightX, y, 80.0f, columnHeight);
        verticalLabel->setBounds(rightX + 90.0f, y, sliderWidth - 90.0f, labelHeight);

        rotarySlider->setBounds(rightX, y + columnHeight, 80.0f, 80.0f);
        rotaryLabel->setBounds(rightX + 90.0f, y + columnHeight, sliderWidth - 90.0f, labelHeight);

        barVerticalSlider->setBounds(rightX, y + 2.0f * columnHeight, 60.0f, columnHeight);
        barVerticalLabel->setBounds(rightX + 70.0f, y + 2.0f * columnHeight, sliderWidth - 70.0f, labelHeight);
    }

    void paint(yup::Graphics& g) override
    {
        g.setFillColor(yup::Color (0xff404040));
        g.fillAll();

        // Draw section dividers
        g.setStrokeColor(yup::Colors::gray.withAlpha(0.3f));
        g.setStrokeWidth(1.0f);

        auto bounds = getLocalBounds();
        auto margin = 20;

        // Horizontal line under title
        g.strokeLine(margin, 70.0f, bounds.getWidth() - margin, 70.0f);

        // Vertical line separating columns
        auto centerX = bounds.getWidth() / 2;
        g.strokeLine(centerX, 80.0f, centerX, bounds.getHeight() - margin);
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
