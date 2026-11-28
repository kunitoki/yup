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
    /** Sets the value the button stands for among the buttons sharing a selection.

        A button with a `checked` state that has a value is on while its selection equals that
        value (see setSelection), and a click selects the value instead of toggling the button.
        Buttons whose selections are linked to one value are then exclusive, whatever their
        parents, and none needs a state of its own to bind:

        @code
        ToggleButton { text: "LFO";  value: 1; selection <=> mod.source; }
        ToggleButton { text: "STEP"; value: 2; selection <=> mod.source; }
        @endcode

        A click on the button that is on leaves it on, or selects its off value when it has one
        (see setOffValue).

        @param newValue  The value, or a void var for a button its clicks toggle.
    */
    void setValue (const var& newValue);

    /** Returns the value the button stands for, void when it has none. */
    const var& getValue() const noexcept { return value; }

    /** Sets the selection a button with a value is compared with: the button is on while the
        two are equal.

        It is what a click on the button changes, so it is the property to link to the value
        the buttons share.

        @param newSelection  The selected value.
    */
    void setSelection (const var& newSelection);

    /** Returns the selection, void when none was set. */
    const var& getSelection() const noexcept { return selection; }

    /** Sets what a click on a button with a value selects when the button is on, so that a
        click can leave the selection empty. Without it the button that is on stays on.

        @param newOffValue  The value selected instead, or a void var for none.
    */
    void setOffValue (const var& newOffValue);

    /** Returns what a click on the button selects when it is on, void for nothing. */
    const var& getOffValue() const noexcept { return offValue; }

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

    /** Selects what a click on a button with a value selects: its value when it is off, its off
        value when it is on and has one. A button with a `checked` state calls it when clicked.

        @returns False for a button with no value, which the click then toggles.
    */
    bool selectValueOnClick();

    static const Identifier textPropertyId;
    static const Identifier checkedPropertyId;
    static const Identifier radioGroupPropertyId;

private:
    void setSelectionState (var& state, const Identifier& name, const var& newValue);

    var value;
    var selection;
    var offValue;
    int radioGroupId = 0;
    bool isButtonCurrentlyOver = false;
    bool isButtonCurrentlyDown = false;
};

} // namespace yup
