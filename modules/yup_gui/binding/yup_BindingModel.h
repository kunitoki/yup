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
    Exposes a BindingProvider as a Component so expressions, styles, drawings and links reach it uniformly.

    A BindingModel mirrors the properties of its BindingProvider into a standalone ComponentTypeInfo
    (with no Component base properties), forwards value notifications as Component::notifyPropertyChanged,
    refreshes its property descriptors when new properties appear on dynamic providers such as
    DataTreeBindingProvider, and bridges widget `pressed` transitions to BindingProvider::beginGesture
    and BindingProvider::endGesture for linked controls.

    @code
    DataTreeBindingProvider provider (stateTree, &undoManager);
    BindingModel model (provider);

    // Return &model from a DrawingScope::ComponentResolver for "params",
    // so expressions like `params.cutoff` read and observe it like any component.
    @endcode

    @see BindingProvider, DataTreeBindingProvider, BindingGraph, DrawingScope

    @tags{UI}
*/
class YUP_API BindingModel final
    : public Component
    , private BindingProvider::Listener
    , private ComponentListener
{
public:
    //==============================================================================
    /** Creates a model component backed by @p providerToUse.

        The provider is not owned; if it is destroyed first, the model detaches automatically.
    */
    explicit BindingModel (BindingProvider& providerToUse);

    /** Destructor. */
    ~BindingModel() override;

    //==============================================================================
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return *info; }

    /** Returns the underlying provider, or nullptr if it was destroyed. */
    BindingProvider* getProvider() const noexcept { return provider; }

    /** Rebuilds the reflected property table from the provider's current properties. */
    void refreshProperties();

    /** Bridges `pressed` state changes on @p component to gesture begin/end calls for @p property. */
    void trackGestures (Component& component, const Identifier& property);

    /** Stops gesture tracking on all tracked components, ending any active gestures. */
    void stopTrackingGestures();

private:
    struct Tracked
    {
        Component* component = nullptr;
        Identifier property;
        bool isInGesture = false;
    };

    static std::vector<ComponentProperty> makeProperties (BindingProvider& provider);

    bool isTracking (const Component& component) const;
    void write (const Identifier& name, const var& value);
    void stopTracking (Component& component);

    void bindingValueChanged (BindingProvider&, const Identifier& name) override;
    void bindingProviderBeingDeleted (BindingProvider& deletedProvider) override;
    void componentPropertyChanged (Component& component, const Identifier& property) override;
    void componentBeingDeleted (Component& component) override;

    BindingProvider* provider = nullptr;
    std::unique_ptr<ComponentTypeInfo> info;
    std::vector<Tracked> tracked;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BindingModel)
};

} // namespace yup
