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

namespace
{

struct ButtonIds
{
    static inline const Identifier value { "value" };
    static inline const Identifier selection { "selection" };
    static inline const Identifier offValue { "offValue" };
};

} // namespace

//==============================================================================

const Identifier Button::textPropertyId ("text");
const Identifier Button::checkedPropertyId ("checked");
const Identifier Button::radioGroupPropertyId ("radioGroup");

//==============================================================================

Button::Button (StringRef componentID)
    : Component (componentID)
{
    setMouseCursor (MouseCursor::Hand);
    setWantsKeyboardFocus (true);
}

const ComponentTypeInfo& Button::getClassTypeInfo()
{
    static const auto info = ComponentTypeBuilder<Button> (Identifier ("Button"))
                                 .property (radioGroupPropertyId, &Button::getRadioGroupId, &Button::setRadioGroupId, 0, "The group of buttons of the same parent of which a single one is checked at a time, 0 for none.")
                                 .property (ButtonIds::value, &Button::getValue, &Button::setValue, var(), "What the button stands for among the buttons sharing a selection. With a value, a button with a checked state is checked while its selection equals it, and a click selects it instead of toggling the button.")
                                 .property (ButtonIds::selection, &Button::getSelection, &Button::setSelection, var(), "The value selected among the buttons sharing it, which a click on a button with a value changes: link it with <=> to the value they share.")
                                 .property (ButtonIds::offValue, &Button::getOffValue, &Button::setOffValue, var(), "What a click selects when the button is checked, to leave the selection empty. Not set, the checked button stays checked.")
                                 .signal ("onClick", &Button::onClick, "Emitted when the button is clicked")
                                 .build();

    return info;
}

//==============================================================================

void Button::setRadioGroupId (int newGroupId)
{
    if (radioGroupId == newGroupId)
        return;

    radioGroupId = newGroupId;

    auto bailOutChecker = BailOutChecker (this);

    notifyPropertyChanged (radioGroupPropertyId);
    if (bailOutChecker.shouldBailOut())
        return;

    if (static_cast<bool> (getProperty (checkedPropertyId)))
        turnOffOtherButtonsInGroup();
}

void Button::turnOffOtherButtonsInGroup()
{
    auto* parent = getParentComponent();

    if (radioGroupId == 0 || parent == nullptr)
        return;

    // Turning a button off runs its handlers, which may change the children of the parent
    std::vector<WeakReference<Component>> others;

    for (int index = 0; index < parent->getNumChildComponents(); ++index)
    {
        auto* other = dynamic_cast<Button*> (parent->getChildComponent (index));

        if (other != nullptr && other != this && other->radioGroupId == radioGroupId)
            others.emplace_back (other);
    }

    for (const auto& other : others)
        if (other != nullptr && static_cast<bool> (other->getProperty (checkedPropertyId)))
            other->setProperty (checkedPropertyId, false);
}

//==============================================================================

void Button::setValue (const var& newValue)
{
    setSelectionState (value, ButtonIds::value, newValue);
}

void Button::setSelection (const var& newSelection)
{
    setSelectionState (selection, ButtonIds::selection, newSelection);
}

void Button::setOffValue (const var& newOffValue)
{
    setSelectionState (offValue, ButtonIds::offValue, newOffValue);
}

void Button::setSelectionState (var& state, const Identifier& name, const var& newValue)
{
    if (state.equalsWithSameType (newValue))
        return;

    state = newValue;

    const BailOutChecker bailOutChecker (this);

    notifyPropertyChanged (name);

    // A button with a value is on while the selection is that value
    if (! bailOutChecker.shouldBailOut() && ! value.isVoid() && getTypeInfo().findProperty (checkedPropertyId) != nullptr)
        setProperty (checkedPropertyId, selection == value);
}

bool Button::selectValueOnClick()
{
    if (value.isVoid())
        return false;

    // The button that is on stays on, unless it has something else to select
    if (! static_cast<bool> (getProperty (checkedPropertyId)))
        setSelection (value);
    else if (! offValue.isVoid())
        setSelection (offValue);
    else
        Button::clicked();

    return true;
}

//==============================================================================

void Button::paint (Graphics& g)
{
    paintButton (g);
}

//==============================================================================

void Button::mouseEnter (const MouseEvent& event)
{
    isButtonCurrentlyOver = true;

    repaint();
}

void Button::mouseExit (const MouseEvent& event)
{
    isButtonCurrentlyOver = false;

    repaint();
}

void Button::mouseDown (const MouseEvent& event)
{
    isButtonCurrentlyDown = true;

    if (getWantsKeyboardFocus())
        takeKeyboardFocus();

    repaint();
}

void Button::mouseUp (const MouseEvent& event)
{
    // A disabled button is given the release of a press it was not given
    const bool wasDown = std::exchange (isButtonCurrentlyDown, false);

    repaint();

    if (wasDown && isEnabled() && getLocalBounds().contains (event.getPosition()))
        clicked();
}

void Button::clicked()
{
    if (onClick)
        onClick();
}

} // namespace yup
