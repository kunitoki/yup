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

#include "PrismEditor.h"
#include "ui/SynthTheme.h"
#include "ui/SynthThemes.h"

PrismEditor::PrismEditor (PrismProcessor& processorToEdit)
    : processor (processorToEdit)
{
    SynthThemes::applyTheme (SynthThemes::loadSelectedTheme());
    buildInterface();

    setSize (getPreferredSize().to<float>());
}

PrismEditor::~PrismEditor()
{
    stopTimer();
}

void PrismEditor::resized()
{
    if (interface == nullptr)
        return;

    const auto design = getPreferredSize().to<float>();
    const auto scale = yup::jmin (getWidth() / design.getWidth(), getHeight() / design.getHeight());

    interface->setBounds ({ (getWidth() - design.getWidth() * scale) * 0.5f,
                            (getHeight() - design.getHeight() * scale) * 0.5f,
                            design.getWidth(),
                            design.getHeight() });
    interface->setTransform (yup::AffineTransform::scaling (scale));
}

void PrismEditor::paint (yup::Graphics& g)
{
    g.setFillColor (SynthTheme::frameColor());
    g.fillAll();
}

void PrismEditor::buildInterface()
{
    interface = std::make_unique<SynthInterface> (processor);
    interface->onThemeChosen = [this] (int index)
    {
        pendingTheme = index;
        startTimerHz (30);
    };
    interface->onPostEffectChanged = [this] { setComponentEffect (SynthInterface::createPostEffect()); };
    setComponentEffect (SynthInterface::createPostEffect());

    addAndMakeVisible (*interface);
    resized();
}

void PrismEditor::timerCallback()
{
    stopTimer();

    if (pendingTheme < 0)
        return;

    SynthThemes::saveSelectedTheme (pendingTheme);
    SynthThemes::applyTheme (pendingTheme);
    pendingTheme = -1;

    interface.reset();
    buildInterface();
}
