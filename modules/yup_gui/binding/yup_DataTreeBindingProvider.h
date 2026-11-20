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
    Exposes the properties of a DataTree to a UI.

    Each property of the tree becomes a bindable value with the same name, typed from the value
    it currently holds: numbers (integer or not) as `number`, booleans as `boolean`, strings as
    `string`, arrays as `array`, and anything else as `any`. Properties added to the tree later
    are exposed dynamically as well.

    Changes made through the UI are written in a transaction of the tree, recorded by the undo
    manager when one is given. Each gesture, such as a knob drag, runs inside a scoped undo
    transaction from @ref beginGesture to @ref endGesture, so undoing reverts the whole drag at
    once. Changes made to the tree by anything else are reported to the UI as well.

    @code
    DataTree params ("params");
    params.beginTransaction().setProperty ("cutoff", 0.5);

    DataTreeBindingProvider provider (params, &undoManager);
    document.setModel ("params", provider);
    @endcode

    @see BindingProvider, DataTree, UndoManager

    @tags{UI}
*/
class YUP_API DataTreeBindingProvider final
    : public BindingProvider
    , private DataTreeListener
{
public:
    //==============================================================================
    /** Creates a provider over a tree.

        @param tree          The tree whose properties are exposed.
        @param undoManager   The undo manager recording the changes made through the UI, or
                             nullptr. It must outlive the provider.
    */
    explicit DataTreeBindingProvider (DataTree tree, UndoManager* undoManager = nullptr);

    /** Destructor. */
    ~DataTreeBindingProvider() override;

    //==============================================================================
    /** Returns the tree the provider exposes. */
    const DataTree& getTree() const noexcept { return tree; }

    //==============================================================================
    /** @internal */
    std::vector<Property> getProperties() const override;
    /** @internal */
    std::optional<Property> findProperty (const Identifier& name) const override;
    /** @internal */
    var getValue (const Identifier& name) const override;
    /** @internal */
    void setValue (const Identifier& name, const var& value) override;
    /** @internal */
    void beginGesture (const Identifier& name) override;
    /** @internal */
    void endGesture (const Identifier& name) override;

private:
    //==============================================================================
    void propertyChanged (DataTree& changedTree, const Identifier& property) override;

    static ComponentProperty::Type inferType (const var& value);
    static Property makeProperty (const Identifier& name, const var& value);

    DataTree tree;
    UndoManager* undoManager = nullptr;
    std::optional<UndoManager::ScopedTransaction> activeGesture;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DataTreeBindingProvider)
};

} // namespace yup
