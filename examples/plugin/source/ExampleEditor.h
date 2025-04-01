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

#include <yup_gui/yup_gui.h>

#include "ExamplePlugin.h"

//==============================================================================

class ExampleEditor : public yup::AudioProcessorEditor
{
public:
    ExampleEditor (ExamplePlugin& processor);

    // yup::AudioProcessorEditor
    bool isResizable() const override;
    bool shouldPreserveAspectRatio() const override;
    yup::Size<int> getPreferredSize() const override;

    // yup::Component
    void paint (yup::Graphics& g) override;
    void resized() override;

private:
    ExamplePlugin& audioProcessor;
    std::unique_ptr<yup::Slider> x;
};
