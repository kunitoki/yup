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

#include <functional>
#include <memory>

class PluginEditorWindow final : public yup::DocumentWindow
{
public:
    PluginEditorWindow (yup::StringRef windowTitle,
                        std::unique_ptr<yup::AudioProcessorEditor> editorToOwn,
                        std::function<void()> onCloseCallback)
        : yup::DocumentWindow (makeWindowOptions (editorToOwn.get()), yup::Color (0xff101417))
        , editor (std::move (editorToOwn))
        , onClose (std::move (onCloseCallback))
    {
        setTitle (windowTitle);
        addAndMakeVisible (*editor);
        editor->attachedToNative();
        takeKeyboardFocus();
    }

    void resized() override
    {
        editor->setBounds (getLocalBounds());
    }

    void userTriedToCloseWindow() override
    {
        if (onClose != nullptr)
        {
            yup::MessageManager::callAsync (onClose);
        }
    }

private:
    static yup::ComponentNative::Options makeWindowOptions (yup::AudioProcessorEditor* editor)
    {
        return yup::ComponentNative::Options()
            .withResizableWindow (editor != nullptr && editor->isResizable())
            .withRenderContinuous (editor != nullptr && editor->shouldRenderContinuous());
    }

    std::unique_ptr<yup::AudioProcessorEditor> editor;
    std::function<void()> onClose;
};
