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

ComponentPropertyAttachment::ComponentPropertyAttachment (Component& componentToBind,
                                                          const Identifier& propertyName,
                                                          DataTree tree,
                                                          const Identifier& dataTreeProperty,
                                                          UndoManager::Ptr undoManagerToUse)
    : component (&componentToBind)
    , propertyName (propertyName)
    , dataTree (tree)
    , dataTreeProperty (dataTreeProperty)
    , undoManager (std::move (undoManagerToUse))
{
    cachedValue.bind (dataTree, dataTreeProperty);

    componentToBind.addComponentListener (this);
    dataTree.addListener (this);

    if (dataTree.hasProperty (dataTreeProperty))
        pushModelToComponent();
    else
        pushComponentToModel (nullptr);
}

ComponentPropertyAttachment::~ComponentPropertyAttachment()
{
    if (auto* boundComponent = component.get())
        boundComponent->removeComponentListener (this);

    dataTree.removeListener (this);
}

void ComponentPropertyAttachment::componentPropertyChanged (Component& changedComponent, const Identifier& name)
{
    if (ignoreCallbacks || &changedComponent != component.get() || name != propertyName)
        return;

    pushComponentToModel (undoManager);
}

void ComponentPropertyAttachment::propertyChanged (DataTree&, const Identifier& property)
{
    if (ignoreCallbacks || property != dataTreeProperty)
        return;

    pushModelToComponent();
}

void ComponentPropertyAttachment::pushModelToComponent()
{
    auto* boundComponent = component.get();

    if (boundComponent == nullptr)
        return;

    const var modelValue = cachedValue.get();

    if (boundComponent->getProperty (propertyName) == modelValue)
        return;

    const ScopedValueSetter<bool> guard (ignoreCallbacks, true);
    boundComponent->setProperty (propertyName, modelValue);
}

void ComponentPropertyAttachment::pushComponentToModel (UndoManager::Ptr undoManagerToUse)
{
    auto* boundComponent = component.get();

    if (boundComponent == nullptr)
        return;

    const var componentValue = boundComponent->getProperty (propertyName);

    if (cachedValue.get() == componentValue)
        return;

    const ScopedValueSetter<bool> guard (ignoreCallbacks, true);

    auto transaction = dataTree.beginTransaction (undoManagerToUse.get());
    transaction.setProperty (dataTreeProperty, componentValue);
    transaction.commit();
}

} // namespace yup
