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

InPlaceTextEditor::InPlaceTextEditor() = default;

InPlaceTextEditor::~InPlaceTextEditor()
{
    close();
}

//==============================================================================

void InPlaceTextEditor::open (Component& anchorToUse, const String& text, Component* parent)
{
    close();

    if (parent == nullptr)
        parent = anchorToUse.getPopupParentComponent();

    if (editor == nullptr)
    {
        editor = std::make_unique<TextEditor>();
        editor->addStyleClass (Identifier ("edit"));
        editor->setExcludedFromParentLayout (true);
        editor->onReturnKey = [this] { commit(); };
        editor->onEscapeKey = [this] { close(); };
        editor->onFocusLost = [this] { commit(); };
    }

    anchor = &anchorToUse;
    openedText = text;

    // The anchor and what is between it and the parent of the editor: what moves the anchor in that parent
    for (auto* component = &anchorToUse; component != nullptr; component = component->getParentComponent())
    {
        component->addComponentListener (this);
        watched.emplace_back (component);

        if (component == parent)
            break;
    }

    parent->addMouseListener (this, true);
    Desktop::getInstance()->addGlobalMouseListener (this);

    editor->setText (text, dontSendNotification);
    parent->addAndMakeVisible (*editor);
    updateBounds();

    editor->selectAll();
    editor->takeKeyboardFocus();
}

void InPlaceTextEditor::commit()
{
    if (! isOpen())
        return;

    const auto text = editor->getText();
    const auto isEdited = text != openedText;

    close();

    // Held, as the callback may open the editor again for something else
    if (const auto callback = onCommit; isEdited && callback != nullptr)
        callback (text);
}

void InPlaceTextEditor::close()
{
    if (! isOpen())
        return;

    // Closed from here on: the focus the editor loses below commits nothing
    anchor = nullptr;

    for (const auto& component : std::exchange (watched, {}))
        if (component != nullptr)
            component->removeComponentListener (this);

    if (auto* desktop = Desktop::getInstanceWithoutCreating())
        desktop->removeGlobalMouseListener (this);

    editor->leaveKeyboardFocus();

    if (auto* parent = editor->getParentComponent())
    {
        parent->removeMouseListener (this);
        parent->removeChildComponent (editor.get());
    }
}

//==============================================================================

void InPlaceTextEditor::updateBounds()
{
    if (auto* parent = editor->getParentComponent(); parent != nullptr && anchor != nullptr)
        editor->setBounds (parent->getLocalArea (anchor.get(), anchor->getLocalBounds()));
}

void InPlaceTextEditor::componentMoved (Component&)
{
    updateBounds();
}

void InPlaceTextEditor::componentResized (Component&)
{
    updateBounds();
}

void InPlaceTextEditor::componentBeingDeleted (Component&)
{
    close();
}

void InPlaceTextEditor::mouseDown (const MouseEvent& event)
{
    if (! isOpen())
        return;

    // A press told by a component names it. One told by the desktop is anywhere on the screen
    const auto* source = event.getSourceComponent();
    const auto isOnEditor = source != nullptr
                              ? source == editor.get()
                              : editor->getScreenBounds().contains (event.getScreenPosition());

    if (! isOnEditor)
        commit();
}

} // namespace yup
