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

namespace yup
{

//==============================================================================
/** A button component that maintains a toggled state.

    The ToggleButton is a Button that can be switched on and off. It maintains
    its toggled state and provides visual feedback about whether it's currently
    toggled or not.

    @see Button, TextButton
*/
class YUP_API ToggleButton : public Button
{
public:
    //==============================================================================
    /** Creates a new ToggleButton.

        @param componentID    The component identifier for this button
    */
    ToggleButton (StringRef componentID = {});

    //==============================================================================
    /** Returns true if the button is currently toggled on.

        @returns true if the button is in its toggled state
    */
    bool getToggleState() const noexcept { return toggleState; }

    /** Sets the button's toggle state.

        @param shouldBeToggled    Whether the button should be toggled on
        @param notification      Whether to send a notification about the change
    */
    void setToggleState (bool shouldBeToggled, NotificationType notification = sendNotification);

    //==============================================================================
    /** Returns the button's current text.

        @returns The text displayed on the button
    */
    String getButtonText() const noexcept { return buttonText; }

    /** Sets the text to display on the button.

        @param newText    The new text to display
    */
    void setButtonText (String newText);

    //==============================================================================
    /** Called when the toggle state changes.
        Override this to perform custom actions when the button is toggled.
    */
    virtual void toggleStateChanged() {}

    //==============================================================================
    struct Colors
    {
        static const Identifier backgroundColorId;
        static const Identifier backgroundToggledColorId;
        static const Identifier textColorId;
        static const Identifier textToggledColorId;
        static const Identifier borderColorId;
        static const Identifier borderToggledColorId;
    };

    //==============================================================================
    /** @internal */
    void paintButton (Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void mouseUp (const MouseEvent& event) override;
    /** @internal */
    void focusGained() override;
    /** @internal */
    void focusLost() override;

private:
    String buttonText;
    StyledText styledText;
    bool toggleState = false;
    bool hasFocus = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToggleButton)
};

} // namespace yup