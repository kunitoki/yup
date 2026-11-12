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

#include "RetraceEditor.h"
#include "ui/SynthNeonEffect.h"
#include "ui/SynthTheme.h"

RetraceEditor::RetraceEditor (RetraceProcessor& processorToEdit)
    : processor (processorToEdit)
    , appliedOptions (processor.getViewOptions())
{
    buildInterface();
    updatePostEffect();

    setSize (getConstrainedSize (getInitialSize()).to<float>());

    // Fast while the tube powers on, then only polling for a new theme.
    startTimerHz (60);

    // Drains the processor's parameter-change queue to the interface's attachments on the message
    // thread, so controls refresh reactively instead of polling every parameter every frame.
    startParameterListening (processor);

    processor.setEditorComponent (this);

    if (auto* host = yup::StandalonePluginHost::getInstance())
        host->setToolbarVisible (false);
}

yup::Size<int> RetraceEditor::getInitialSize() const
{
    const auto size = processor.getViewOptions().editorSize;
    return size.isEmpty() ? getPreferredSize() : size;
}

RetraceEditor::~RetraceEditor()
{
    stopTimer();

    if (processor.getEditorComponent() == this)
        processor.setEditorComponent (nullptr);
}

void RetraceEditor::resized()
{
    if (interface == nullptr)
        return;

    if (const auto size = getSize().roundToInt(); ! size.isEmpty())
        processor.setEditorSize (size);

    const auto design = getPreferredSize().to<float>();
    const auto scale = yup::jmin (getWidth() / design.getWidth(), getHeight() / design.getHeight());

    interface->setBounds ({ (getWidth() - design.getWidth() * scale) * 0.5f,
                            (getHeight() - design.getHeight() * scale) * 0.5f,
                            design.getWidth(),
                            design.getHeight() });
    interface->setTransform (yup::AffineTransform::scaling (scale));
}

void RetraceEditor::paint (yup::Graphics& g)
{
    g.setFillColor (SynthTheme::frameColor (interface != nullptr ? static_cast<const yup::Component&> (*interface) : *this));
    g.fillAll();
}

void RetraceEditor::buildInterface()
{
    interface = std::make_unique<SynthInterface> (processor, *this, appliedOptions);
    interface->onViewOptionsChanged = [this] { syncViewOptions (false); };
    interface->onScalePercentRequested = [this] (int percent) { applyScale (percent); };
    interface->getScalePercent = [this] { return scalePercent(); };

    addAndMakeVisible (*interface);
    resized();
}

void RetraceEditor::applyScale (int percent)
{
    const auto target = (getPreferredSize().to<float>() * (static_cast<float> (percent) / 100.0f)).roundToInt();
    setSize (getConstrainedSize (target).to<float>());
}

int RetraceEditor::scalePercent() const
{
    const auto design = getPreferredSize();
    const auto current = getSize().roundToInt();

    if (design.getWidth() <= 0 || current.getWidth() <= 0)
        return 100;

    return yup::roundToInt (static_cast<float> (current.getWidth()) * 100.0f / static_cast<float> (design.getWidth()));
}

void RetraceEditor::syncViewOptions (bool canRebuild)
{
    const auto options = processor.getViewOptions();
    const auto themeChanged = options.getThemeIndex() != appliedOptions.getThemeIndex();

    if (themeChanged && ! canRebuild)
        return;

    const auto effectChanged = options.postEffect != appliedOptions.postEffect || options.neon != appliedOptions.neon;
    const auto interfaceChanged = options.frameRate != appliedOptions.frameRate || options.landscapes != appliedOptions.landscapes;
    appliedOptions = options;

    if (themeChanged)
    {
        interface.reset();
        buildInterface();
    }
    else if (interfaceChanged)
    {
        interface->setViewOptions (options);
    }

    if (effectChanged)
        updatePostEffect();
}

void RetraceEditor::updatePostEffect()
{
    auto effect = SynthInterface::createPostEffect (appliedOptions);

    if (auto* neon = dynamic_cast<SynthNeonEffect*> (effect.get()))
        neon->setOpening (opening);

    setComponentEffect (effect);
}

void RetraceEditor::advanceOpening()
{
    // Hosts may build an editor before showing it, so the tube powers on once it is on screen.
    if (getNativeComponent() == nullptr || ! isShowing())
        return;

    const auto now = yup::Time::getMillisecondCounterHiRes();

    if (openingStart <= 0.0)
        openingStart = now;

    opening = static_cast<float> (yup::jmin (1.0, (now - openingStart) / (openingSeconds * 1000.0)));

    if (auto* neon = dynamic_cast<SynthNeonEffect*> (getComponentEffect().get()))
        neon->setOpening (opening);

    repaint();

    if (opening >= 1.0f)
        startTimerHz (10);
}

void RetraceEditor::timerCallback()
{
    if (opening < 1.0f)
        advanceOpening();

    syncViewOptions (true);
}
