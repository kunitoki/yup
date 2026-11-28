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

namespace yup
{

//==============================================================================
/**
    Edits a text in place, in a single line TextEditor shown over a component.

    The editor is opened over an anchor, the component showing what is edited: a label, a
    knob. It is added to the interface the anchor is in rather than to a window of its own,
    covers the anchor and follows it when it moves or is resized. It opens with its text
    selected and takes the keyboard focus.

    Return, a press anywhere else and the loss of the keyboard focus commit the edit, Escape
    cancels it. Both close the editor, as does the anchor going away. One editor shows at a
    time: opening it again moves it.

    @code
    InPlaceTextEditor editor;
    editor.onCommit = [this] (const String& text) { slider.setValue (text.getDoubleValue()); };
    editor.open (label, String (slider.getValue()));
    @endcode

    The TextEditor is styled as any other, and carries the style class `edit` for a style
    sheet to tell it apart.

    @see TextEditor, Component::getPopupParentComponent

    @tags{UI}
*/
class YUP_API InPlaceTextEditor final
    : private ComponentListener
    , private MouseListener
{
public:
    //==============================================================================
    /** Creates an editor that is not open. */
    InPlaceTextEditor();

    /** Destructor, closing the editor without committing. */
    ~InPlaceTextEditor() override;

    //==============================================================================
    /** Opens the editor over a component, closing it first without committing if it was open.

        @param anchor  The component the editor covers and follows.
        @param text    The text the editor opens with, all selected.
        @param parent  The component the editor is added to, which is the anchor or holds it.
                       When null it is the one the popups of the anchor go to
                       (see Component::getPopupParentComponent).
    */
    void open (Component& anchor, const String& text, Component* parent = nullptr);

    /** Closes the editor and reports its text with onCommit, when it is not the text it was opened with. */
    void commit();

    /** Closes the editor, dropping what was typed. */
    void close();

    /** Returns true while the editor is open. */
    bool isOpen() const noexcept { return anchor != nullptr; }

    //==============================================================================
    /** Returns the component the editor is open over, or nullptr when it is closed. */
    Component* getAnchor() const noexcept { return anchor.get(); }

    /** Returns the TextEditor, or nullptr when the editor was never opened. It is kept once made. */
    TextEditor* getTextEditor() const noexcept { return editor.get(); }

    //==============================================================================
    /** Called with the text when an edit is committed with another text than the one it opened with.

        The editor is closed by then, so the callback can open it again.
    */
    std::function<void (const String& text)> onCommit;

private:
    //==============================================================================
    void updateBounds();

    void componentMoved (Component& component) override;
    void componentResized (Component& component) override;
    void componentBeingDeleted (Component& component) override;
    void mouseDown (const MouseEvent& event) override;

    std::unique_ptr<TextEditor> editor;
    WeakReference<Component> anchor;
    std::vector<WeakReference<Component>> watched;
    String openedText;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InPlaceTextEditor)
};

} // namespace yup
