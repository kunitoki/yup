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

#include "ExampleEditor.h"

//==============================================================================

ExampleEditor::ExampleEditor (ExamplePlugin& processor)
    : audioProcessor (processor)
{
    x = std::make_unique<yup::Slider> ("Slider", yup::Font());
    x->setValue (audioProcessor.getParameters()[0]->getValue());
    x->onValueChanged = [this] (float value)
    {
        audioProcessor.getParameters()[0]->setValueNotifyingHost (value);
    };
    addAndMakeVisible (*x);

    setSize (getPreferredSize().to<float>());

    startTimerHz (60);
}

bool ExampleEditor::isResizable() const
{
    return true;
}

bool ExampleEditor::shouldPreserveAspectRatio() const
{
    return false;
}

yup::Size<int> ExampleEditor::getPreferredSize() const
{
    return { 600, 400 };
}

void ExampleEditor::resized()
{
    x->setBounds (getLocalBounds().largestFittingSquare());
}

void ExampleEditor::paint (yup::Graphics& g)
{
    g.setFillColor (0xff404040);
    g.fillAll();
}

void ExampleEditor::timerCallback()
{
    x->setValue (audioProcessor.getParameters()[0]->getValue());
}
