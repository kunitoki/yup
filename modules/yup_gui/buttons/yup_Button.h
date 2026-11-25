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
    or clicked, and will call the onClick callback when it is clicked: once for each press that
    is released over the button while it is enabled.

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
    /** Makes the button one of a group of which a single one is on at a time.

        A button with a `checked` state that is turned on turns off the other buttons of the
        group in its parent, and a click on the one that is on leaves it on. The program can
        still turn it off, which leaves the group with none on. Buttons of different parents
        are in different groups, whatever their identifier. A button that is on when it joins
        a group turns off the others of it.

        @param newGroupId  The identifier of the group, 0 for no group.
    */
    void setRadioGroupId (int newGroupId);

    /** Returns the identifier of the radio group of the button, 0 when it is in none. */
    int getRadioGroupId() const noexcept { return radioGroupId; }

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
    /** Returns the static type info describing the Button base type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

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

protected:
    /** Called when the button is clicked: pressed, then released over it while it is enabled.

        It runs onClick. A button with a state of its own changes it here instead, when that
        change runs onClick itself, so that a click runs it once.
    */
    virtual void clicked();

    /** Turns off the other buttons of the radio group. A button with a `checked` state calls it when it is turned on. */
    void turnOffOtherButtonsInGroup();

    static const Identifier textPropertyId;
    static const Identifier checkedPropertyId;
    static const Identifier radioGroupPropertyId;

private:
    int radioGroupId = 0;
    bool isButtonCurrentlyOver = false;
    bool isButtonCurrentlyDown = false;
};

} // namespace yup
