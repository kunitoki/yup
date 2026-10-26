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

#pragma once

#include <yup_audio_gui/yup_audio_gui.h>

#include "RetraceProcessor.h"
#include "ui/SynthInterface.h"

#include <memory>

//==============================================================================
/** The RETRACE editor: hosts the interface and rebuilds it when the theme changes.

    The interface is laid out at the preferred size and scaled to fit the editor, keeping
    its aspect ratio, so resizing zooms the whole instrument. The post effect sits on the
    editor rather than on the interface, so it renders at the size shown.

    The interface wears the theme of the processor it edits, so editors of different
    instances can wear different themes. A new theme, picked in the interface or loaded
    with a session, builds a new interface; the editor's timer notices it, which also
    keeps the interface from being replaced from inside its own settings menu.

    When the editor first shows, the post effect plays the tube powering on.

    @see SynthInterface, RetraceProcessor
*/
class RetraceEditor
    : public yup::AudioProcessorEditor
    , private yup::Timer
{
public:
    explicit RetraceEditor (RetraceProcessor& processorToEdit);
    ~RetraceEditor() override;

    //==============================================================================
    bool isResizable() const override { return true; }
    yup::Size<int> getPreferredSize() const override { return { 1280, 900 }; }
    bool shouldPreserveAspectRatio() const override { return true; }

    //==============================================================================
    void resized() override;
    void paint (yup::Graphics& g) override;

private:
    void timerCallback() override;
    int getTheme() const;
    void buildInterface();
    void updatePostEffect();
    void advanceOpening();

    static constexpr double openingSeconds = 0.42;

    RetraceProcessor& processor;
    std::unique_ptr<SynthInterface> interface;
    int builtTheme = -1;
    double openingStart = 0.0;
    float opening = 0.0f;
};
