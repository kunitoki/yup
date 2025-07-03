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

//==============================================================================

class PythonDemo : public yup::Component
{
public:
    PythonDemo()
        : Component ("PythonDemoDemo")
    {
        setOpaque (false);

        runPython.setButtonText("Run Python!");
        runPython.onClick = [this]
        {
            engine.runScript("print ('Hello, World!')");
        };
        addAndMakeVisible (runPython);
    }

    void resized() override
    {
        constexpr int margin = 5;
        constexpr int buttonWidth = 100;
        constexpr int buttonHeight = 30;

        auto bounds = getLocalBounds().reduced (margin);

        auto buttons1 = bounds.removeFromTop (buttonHeight);
        runPython.setBounds (buttons1.removeFromLeft (buttonWidth));
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();
    }

private:
    yup::TextButton runPython;
    yup::ScriptEngine engine;
};
