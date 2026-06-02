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
/** A base class for buttons.

    To create a custom button, inherit from this class and implement the paintButton() method.

    The button will automatically track mouse events to determine when it is being hovered over
    or clicked, and will call the onClick callback when it is clicked.

    @see Component, DrawableButton
*/
class YUP_API Button : public Component
{
public:
    //==============================================================================
    /** Creates a button with the given component ID. */
    Button (StringRef componentID);

    //==============================================================================
    /** Returns true if the button is currently being hovered over. */
    bool isButtonOver() const { return isButtonCurrentlyOver; }

    /** Returns true if the button is currently being clicked. */
    bool isButtonDown() const { return isButtonCurrentlyDown; }

    //==============================================================================
    /** Paints the button.
    
        This method must be implemented by subclasses to define how the button should be drawn.
        The button's current state (hovered, clicked) can be determined using the isButtonOver() and
        isButtonDown() methods.

        @param g The graphics context to use for drawing.
    */
    virtual void paintButton (Graphics& g) = 0;

    //==============================================================================
    /** A callback that is called when the button is clicked. */
    std::function<void()> onClick;

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void mouseEnter (const MouseEvent& event) override;
    /** @internal */
    void mouseExit (const MouseEvent& event) override;
    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void mouseUp (const MouseEvent& event) override;

private:
    bool isButtonCurrentlyOver = false;
    bool isButtonCurrentlyDown = false;
};

} // namespace yup
