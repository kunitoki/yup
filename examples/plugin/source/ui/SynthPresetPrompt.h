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

#include <yup_gui/yup_gui.h>

#include <functional>
#include <memory>

//==============================================================================
/** A modal overlay inside a component asking for a name or a confirmation.

    It dims the component it covers and shows a tile with a title, a name field or a
    message, an error line, and Cancel and confirm buttons. Return confirms and escape
    cancels, so escape never reaches the components behind it. The prompt deletes itself
    when it closes.

    @see SynthPresetBrowser
*/
class SynthPresetPrompt
    : public yup::Component
    , private yup::ComponentListener
{
public:
    /** Asks for a name. onConfirm returns an error to show, keeping the prompt open, or an
        empty string to close it.
    */
    static void askForName (yup::Component& host, const yup::String& title, const yup::String& initialName, std::function<yup::String (const yup::String&)> onConfirm);

    /** Asks to confirm an action named by confirmLabel; onConfirm runs when confirmed. */
    static void askToConfirm (yup::Component& host, const yup::String& title, const yup::String& message, const yup::String& confirmLabel, std::function<void()> onConfirm);

    ~SynthPresetPrompt() override;

    void resized() override;
    void paint (yup::Graphics& g) override;
    bool keyDown (const yup::KeyPress& key, const yup::Point<float>& position) override;

private:
    class NameField;

    SynthPresetPrompt (const yup::String& title, const yup::String& message, const yup::String& confirmLabel, bool asksForName);
    void show (yup::Component& hostToCover, const yup::String& initialName);
    void componentResized (yup::Component& component) override;
    void componentBeingDeleted (yup::Component& component) override;
    void confirm();
    void close();

    yup::WeakReference<yup::Component> host;
    yup::String title;
    yup::String message;
    yup::String error;
    std::unique_ptr<NameField> field;
    yup::TextButton cancelButton;
    yup::TextButton confirmButton;
    yup::Rectangle<float> tile;
    yup::Rectangle<float> fieldArea;
    std::function<yup::String (const yup::String&)> onConfirmName;
    std::function<void()> onConfirmAction;
    yup::Font captionFont;
    yup::Font valueFont;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthPresetPrompt)
};
