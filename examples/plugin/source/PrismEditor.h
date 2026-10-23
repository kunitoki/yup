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

#include "PrismProcessor.h"
#include "ui/SynthInterface.h"

#include <memory>

//==============================================================================
/** The PRISM editor: hosts the interface and rebuilds it when the theme changes.

    The interface reads its colors from the theme while it is built, so switching the
    palette builds a new one. The switch is deferred to the editor's timer, because it is
    requested from inside the interface being replaced.

    @see SynthInterface, PrismProcessor
*/
class PrismEditor
    : public yup::AudioProcessorEditor
    , private yup::Timer
{
public:
    explicit PrismEditor (PrismProcessor& processorToEdit);
    ~PrismEditor() override;

    //==============================================================================
    bool isResizable() const override { return true; }
    yup::Size<int> getPreferredSize() const override { return { 1280, 900 }; }

    //==============================================================================
    void resized() override;

private:
    void timerCallback() override;
    void buildInterface();

    PrismProcessor& processor;
    std::unique_ptr<SynthInterface> interface;
    int pendingTheme = -1;
};
