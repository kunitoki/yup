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
/** Exposes a BindingProvider as a component, so expressions and links reach it like any element. */
class YuiBuilder::Model final
    : public Component
    , private BindingProvider::Listener
    , private ComponentListener
{
public:
    explicit Model (BindingProvider& providerToUse)
        : provider (&providerToUse)
        , info (Identifier ("Model"), &Component::getClassTypeInfo(), makeProperties (providerToUse))
    {
        setTypeInfo (&info);
        provider->addListener (this);
    }

    ~Model() override
    {
        stopTrackingGestures();

        if (provider != nullptr)
            provider->removeListener (this);
    }

    BindingProvider* getProvider() const noexcept { return provider; }

    void refreshProperties()
    {
        if (provider == nullptr)
            return;

        info = ComponentTypeInfo (Identifier ("Model"), &Component::getClassTypeInfo(), makeProperties (*provider));
        setTypeInfo (&info);
    }

    // The provider is told about a gesture while the component linked to one of its values is pressed
    void trackGestures (Component& component, const Identifier& property)
    {
        if (! isTracking (component))
            component.addComponentListener (this);

        tracked.push_back ({ &component, property });
    }

    void stopTrackingGestures()
    {
        while (! tracked.empty())
            stopTracking (*tracked.back().component);
    }

private:
    struct Tracked
    {
        Component* component = nullptr;
        Identifier property;
        bool isInGesture = false;
    };

    static std::vector<ComponentProperty> makeProperties (BindingProvider& provider)
    {
        std::vector<ComponentProperty> properties;

        for (const auto& property : provider.getProperties())
        {
            properties.push_back ({
                .name = property.name,
                .type = property.type,
                .elementType = property.elementType,
                .getter = [name = property.name] (const Component& component)
                {
                    const auto* model = static_cast<const Model&> (component).provider;
                    return model != nullptr ? model->getValue (name) : var();
                },
                .setter = [name = property.name] (Component& component, const var& value)
                {
                    static_cast<Model&> (component).write (name, value);
                },
                .isStyleable = false,
                .isInherited = false });
        }

        return properties;
    }

    bool isTracking (const Component& component) const
    {
        return std::any_of (tracked.begin(), tracked.end(), [&] (const auto& entry)
        {
            return entry.component == &component;
        });
    }

    void write (const Identifier& name, const var& value)
    {
        if (provider == nullptr || provider->getValue (name) == value)
            return;

        const auto isInGesture = std::any_of (tracked.begin(), tracked.end(), [&] (const auto& entry)
        {
            return entry.property == name && entry.isInGesture;
        });

        if (! isInGesture)
            provider->beginGesture (name);

        provider->setValue (name, value);

        if (! isInGesture)
            provider->endGesture (name);
    }

    void stopTracking (Component& component)
    {
        for (auto& entry : tracked)
            if (entry.component == &component && entry.isInGesture && provider != nullptr)
                provider->endGesture (entry.property);

        tracked.erase (std::remove_if (tracked.begin(), tracked.end(), [&] (const auto& entry)
        {
            return entry.component == &component;
        }),
                       tracked.end());

        component.removeComponentListener (this);
    }

    void bindingValueChanged (BindingProvider&, const Identifier& name) override
    {
        notifyPropertyChanged (name);
    }

    void bindingProviderBeingDeleted (BindingProvider& deletedProvider) override
    {
        if (provider != &deletedProvider)
            return;

        for (auto& entry : tracked)
            entry.isInGesture = false;

        provider = nullptr;
    }

    void componentPropertyChanged (Component& component, const Identifier& property) override
    {
        static const Identifier pressedProperty ("pressed");

        if (property != pressedProperty || provider == nullptr)
            return;

        const auto isPressed = component.isPressed();

        for (auto& entry : tracked)
        {
            if (entry.component != &component || entry.isInGesture == isPressed)
                continue;

            entry.isInGesture = isPressed;

            if (isPressed)
                provider->beginGesture (entry.property);
            else
                provider->endGesture (entry.property);
        }
    }

    void componentBeingDeleted (Component& component) override
    {
        stopTracking (component);
    }

    BindingProvider* provider = nullptr;
    ComponentTypeInfo info;
    std::vector<Tracked> tracked;
};

} // namespace yup
