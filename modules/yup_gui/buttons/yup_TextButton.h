/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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
/** A button that displays text.

    To create a custom text button, inherit from this class and implement the paintButton() method.

    The button will automatically track mouse events to determine when it is being hovered over
    or clicked, and will call the onClick callback when it is clicked.

    @see Button, Component
*/
class YUP_API TextButton : public Button
{
public:
    //==============================================================================
    /** Creates a text button with the given component ID. */
    TextButton (StringRef componentID = {});

    //==============================================================================
    /** Returns the text displayed on the button. */
    String getButtonText() const { return buttonText; }

    /** Sets the text displayed on the button
    
        @param newButtonText The new text to display on the button.
    */
    void setButtonText (StringRef newButtonText);

    //==============================================================================
    /** Color identifiers used by the text editor. */
    struct Style
    {
        static const Identifier backgroundColorId;
        static const Identifier backgroundPressedColorId;
        static const Identifier textColorId;
        static const Identifier textPressedColorId;
        static const Identifier outlineColorId;
        static const Identifier outlineFocusedColorId;
    };

    //==============================================================================
    /** Returns the bounds of the text within the button. */
    Rectangle<float> getTextBounds() const;

    //==============================================================================
    /** @internal */
    StyledText& getStyledText() const noexcept { return const_cast<StyledText&> (styledText); }

    /** @internal */
    void paintButton (Graphics& g) override;
    /** @internal */
    void resized() override;

private:
    void updateTextLayout();

    String buttonText;
    StyledText styledText;
};

} // namespace yup
