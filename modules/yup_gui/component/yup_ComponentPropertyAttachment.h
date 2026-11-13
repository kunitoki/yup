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
/** Two-way binds a reflected component property to a DataTree property.

    State-backed properties such as a Slider's @c value or a TextEditor's @c text reflect the
    widget's own model. This attachment keeps one such property in sync with a single DataTree
    property so the widget mirrors the document model and writes edits back to it.

    The model side is held by a CachedValue, which caches the DataTree property for fast reads and
    refreshes automatically when the document changes. On a document change the attachment pushes
    the new value into the component through Component::setProperty(); when the component reports a
    change through Component::notifyPropertyChanged() (Slider drag, text edit, ...), the attachment
    writes it back through a DataTree transaction. A re-entrancy guard and value comparison stop the
    two sides from looping.

    Pass an UndoManager to make the write-backs undoable, so widget edits join the same undo history
    as the rest of the document; leave it null for direct, non-undoable writes.

    On construction the model wins when the DataTree already holds the property; otherwise the
    property is seeded from the component's current value. A weak reference to the component lets the
    component and the DataTree outlive the attachment in either order. Create, use and destroy
    attachments on the message thread.

    @see Component::setProperty, Component::notifyPropertyChanged, CachedValue, DataTree

    @tags{GUI}
*/
class YUP_API ComponentPropertyAttachment : private ComponentListener
                                          , private DataTree::Listener
{
public:
    //==============================================================================
    /** Creates an attachment binding a component property to a DataTree property.

        @param component        The component whose property is bound.
        @param propertyName      The reflected property name on the component (for example "value").
        @param tree              The DataTree holding the model property.
        @param dataTreeProperty  The property identifier within @p tree.
        @param undoManager       Optional UndoManager; when supplied, component edits written back to
                                 the model become undoable. Pass nullptr for direct writes.
    */
    ComponentPropertyAttachment (Component& component,
                                 const Identifier& propertyName,
                                 DataTree tree,
                                 const Identifier& dataTreeProperty,
                                 UndoManager::Ptr undoManager = nullptr);

    /** Destructor. Detaches from the component and the DataTree. */
    ~ComponentPropertyAttachment() override;

    //==============================================================================
    /** Returns the bound component, or nullptr once it has been deleted. */
    Component* getComponent() const noexcept { return component.get(); }

    /** Returns the bound component property name. */
    const Identifier& getPropertyName() const noexcept { return propertyName; }

    /** Returns the DataTree holding the model property. */
    DataTree getDataTree() const noexcept { return dataTree; }

    /** Returns the model property identifier within the DataTree. */
    const Identifier& getDataTreeProperty() const noexcept { return dataTreeProperty; }

    /** Returns the UndoManager write-backs are recorded in, or nullptr. */
    UndoManager::Ptr getUndoManager() const noexcept { return undoManager; }

    /** Returns true while the component is still alive and bound. */
    bool isAttached() const noexcept { return component.get() != nullptr; }

private:
    //==============================================================================
    void componentPropertyChanged (Component& component, const Identifier& name) override;
    void propertyChanged (DataTree& tree, const Identifier& property) override;

    void pushModelToComponent();
    void pushComponentToModel (UndoManager::Ptr undoManagerToUse);

    //==============================================================================
    WeakReference<Component> component;
    Identifier propertyName;
    DataTree dataTree;
    Identifier dataTreeProperty;
    UndoManager::Ptr undoManager;
    CachedValue<var> cachedValue;
    bool ignoreCallbacks = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComponentPropertyAttachment)
};

} // namespace yup
