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

namespace yup
{

//==============================================================================
/**
    An interface for components that need to accept text input.

    Components that need to accept text input (such as TextEditor) should inherit
    from this class and implement the required methods. This enables features like
    on-screen keyboards on mobile devices and IME (Input Method Editor) support.

    The text input system is automatically managed based on focus changes. When a
    component that implements TextInputTarget gains focus, text input will be started.
    When it loses focus, text input will be stopped (unless the newly focused component
    also implements TextInputTarget).

    Example usage:
    @code
    class MyTextComponent : public Component,
                           public TextInputTarget
    {
    public:
        MyTextComponent()
        {
            setWantsKeyboardFocus (true);
        }

        void focusGained() override
        {
            Component::focusGained();
            requestTextInput();
        }

        void focusLost() override
        {
            Component::focusLost();
            relinquishTextInput();
        }

        Rectangle<float> getTextInputRect() const override
        {
            return getLocalBounds();
        }
    };
    @endcode

    @see Component, TextEditor
*/
class TextInputTarget
{
public:
    /** Destructor. */
    virtual ~TextInputTarget() = default;

    //==============================================================================
    /**
        Called to get the screen rectangle where text input is being edited.

        This rectangle is used to position on-screen keyboards and IME windows
        to avoid obscuring the text being edited.

        @return The rectangle in screen coordinates where text is being edited
    */
    virtual Rectangle<float> getTextInputRect() const = 0;

    //==============================================================================
    /**
        Requests that the system starts accepting text input for this target.

        Call this method when your component wants to receive text input events,
        typically in focusGained(). The system will show on-screen keyboards on
        mobile devices and enable IME where appropriate.
    */
    void requestTextInput();

    /**
        Relinquishes text input, telling the system this target no longer needs text input.

        Call this method when your component no longer needs text input,
        typically in focusLost(). This will hide on-screen keyboards and
        disable IME.
    */
    void relinquishTextInput();

    /**
        Updates the active text input rectangle for this target.

        Call this after the caret or edited text area moves while text input is active.
    */
    void updateTextInputRect();

    /**
        Returns true if this target currently has active text input.

        @return True if text input is currently active for this target
    */
    bool isTextInputActive() const noexcept;

private:
    bool textInputActive = false;
};

} // namespace yup
