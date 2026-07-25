/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#include <yup_audio_gui/yup_audio_gui.h>

#include <gtest/gtest.h>

using namespace yup;

namespace yup
{
extern std::unique_ptr<yup::GraphicsContext> yup_constructHeadlessGraphicsContext (yup::GpuDevice::Options, yup::GpuDevice::Ptr);
} // namespace yup

TEST (ThemeVersion1Tests, CreateReturnsNonNullTheme)
{
    auto theme = createThemeVersion1();
    EXPECT_NE (nullptr, theme.get());
}

TEST (ThemeVersion1Tests, SliderColorsAreRegistered)
{
    auto theme = createThemeVersion1();
    ASSERT_NE (nullptr, theme.get());

    Slider slider (Slider::LinearHorizontal);
    EXPECT_TRUE (theme->findColor (slider, Slider::Style::backgroundColorId).has_value());
    EXPECT_TRUE (theme->findColor (slider, Slider::Style::trackColorId).has_value());
    EXPECT_TRUE (theme->findColor (slider, Slider::Style::thumbColorId).has_value());
    EXPECT_TRUE (theme->findColor (slider, Slider::Style::textColorId).has_value());
}

TEST (ThemeVersion1Tests, LabelColorsAreRegistered)
{
    auto theme = createThemeVersion1();
    ASSERT_NE (nullptr, theme.get());

    Label label;
    EXPECT_TRUE (theme->findColor (label, Label::Style::textFillColorId).has_value());
    EXPECT_TRUE (theme->findColor (label, Label::Style::backgroundColorId).has_value());
}

TEST (ThemeVersion1Tests, ProgressBarColorsAreRegistered)
{
    auto theme = createThemeVersion1();
    ASSERT_NE (nullptr, theme.get());

    ProgressBar progressBar;
    EXPECT_TRUE (theme->findColor (progressBar, ProgressBar::Style::backgroundColorId).has_value());
    EXPECT_TRUE (theme->findColor (progressBar, ProgressBar::Style::foregroundColorId).has_value());
}

TEST (ThemeVersion1Tests, ScrollBarColorsAreRegistered)
{
    auto theme = createThemeVersion1();
    ASSERT_NE (nullptr, theme.get());

    ScrollBar scrollBar (ScrollBar::Orientation::vertical);
    EXPECT_TRUE (theme->findColor (scrollBar, ScrollBar::Style::trackColorId).has_value());
    EXPECT_TRUE (theme->findColor (scrollBar, ScrollBar::Style::thumbColorId).has_value());
}

TEST (ThemeVersion1Tests, PaintsCoreComponents)
{
    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 600);
    Graphics g (*context, *renderer);

    Slider slider (Slider::LinearHorizontal);
    slider.setRange (0.0, 1.0);
    slider.setValue (0.25);
    slider.setBounds (20.0f, 20.0f, 240.0f, 40.0f);
    slider.paint (g);

    TextButton textButton;
    textButton.setBounds (20.0f, 80.0f, 140.0f, 40.0f);
    textButton.paint (g);

    ToggleButton toggleButton;
    toggleButton.setToggleState (true, NotificationType::dontSendNotification);
    toggleButton.setBounds (20.0f, 140.0f, 160.0f, 40.0f);
    toggleButton.paint (g);

    SwitchButton switchButton;
    switchButton.setToggleState (true, NotificationType::dontSendNotification);
    switchButton.setBounds (20.0f, 200.0f, 120.0f, 40.0f);
    switchButton.paint (g);

    TextEditor textEditor;
    textEditor.setText ("Theme", NotificationType::dontSendNotification);
    textEditor.setBounds (20.0f, 260.0f, 240.0f, 40.0f);
    textEditor.paint (g);

    ComboBox comboBox;
    comboBox.addItem ("Item 1", 1);
    comboBox.setSelectedId (1, NotificationType::dontSendNotification);
    comboBox.setBounds (20.0f, 320.0f, 200.0f, 40.0f);
    comboBox.paint (g);

    Label label;
    label.setText ("Label", NotificationType::dontSendNotification);
    label.setBounds (20.0f, 380.0f, 200.0f, 30.0f);
    label.paint (g);

    ScrollBar scrollBar (ScrollBar::Orientation::horizontal);
    scrollBar.setRangeLimits (0.0, 100.0);
    scrollBar.setCurrentRange (20.0, 40.0);
    scrollBar.setBounds (20.0f, 430.0f, 240.0f, 18.0f);
    scrollBar.paint (g);

    ProgressBar progressBar;
    progressBar.setProgress (0.5, NotificationType::dontSendNotification);
    progressBar.setBounds (20.0f, 470.0f, 240.0f, 20.0f);
    progressBar.paint (g);

    KMeterState meterState (48000.0, 2);
    KMeterComponent meter (meterState, 0);
    meter.setBounds (300.0f, 20.0f, 60.0f, 240.0f);
    meter.paint (g);

    EXPECT_TRUE (true);
}
