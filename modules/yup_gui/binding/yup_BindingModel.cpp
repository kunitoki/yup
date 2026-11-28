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

BindingModel::BindingModel (BindingProvider& providerToUse)
    : provider (&providerToUse)
    , info (std::make_unique<ComponentTypeInfo> (Identifier ("Model"), nullptr, makeProperties (providerToUse.getProperties())))
{
    provider->addListener (this);
}

BindingModel::~BindingModel()
{
    stopTrackingGestures();

    if (provider != nullptr)
        provider->removeListener (this);
}

void BindingModel::refreshProperties()
{
    if (provider == nullptr)
        return;

    auto exposed = provider->getProperties();

    for (const auto& found : foundByName)
        if (std::none_of (exposed.begin(), exposed.end(), [&found] (const auto& listed) { return listed.name == found.name; }))
            exposed.push_back (found);

    const auto& reflected = info->getOwnProperties();

    // What reads the model holds on to its type info, which is only made again for other properties or other types
    const auto isUpToDate = std::equal (exposed.begin(), exposed.end(), reflected.begin(), reflected.end(), [] (const BindingProvider::Property& property, const ComponentProperty& known)
    {
        return property.name == known.name && property.type == known.type && property.elementType == known.elementType;
    });

    if (! isUpToDate)
        info = std::make_unique<ComponentTypeInfo> (Identifier ("Model"), nullptr, makeProperties (exposed));
}

bool BindingModel::ensureProperty (const Identifier& name)
{
    if (info->findProperty (name) != nullptr)
        return true;

    if (provider == nullptr)
        return false;

    auto found = provider->findProperty (name);
    if (! found.has_value())
        return false;

    foundByName.push_back (std::move (*found));
    refreshProperties();

    return info->findProperty (name) != nullptr;
}

void BindingModel::trackGestures (Component& component, const Identifier& property)
{
    if (! isTracking (component))
        component.addComponentListener (this);

    tracked.push_back ({ &component, property });
}

void BindingModel::stopTrackingGestures()
{
    while (! tracked.empty())
        stopTracking (*tracked.back().component);
}

std::vector<ComponentProperty> BindingModel::makeProperties (const std::vector<BindingProvider::Property>& exposed)
{
    std::vector<ComponentProperty> properties;

    for (const auto& property : exposed)
    {
        properties.push_back ({
            .name = property.name,
            .type = property.type,
            .elementType = property.elementType,
            .getter = [name = property.name] (const Component& component)
            {
                const auto* model = static_cast<const BindingModel&> (component).provider;
                return model != nullptr ? model->getValue (name) : var();
            },
            .setter = [name = property.name] (Component& component, const var& value)
            {
                static_cast<BindingModel&> (component).write (name, value);
            },
            .isStyleable = false,
            .isInherited = false
        });
    }

    return properties;
}

bool BindingModel::isTracking (const Component& component) const
{
    return std::any_of (tracked.begin(), tracked.end(), [&] (const auto& entry)
    {
        return entry.component == &component;
    });
}

void BindingModel::write (const Identifier& name, const var& value)
{
    if (provider == nullptr || provider->getValue (name) == value)
        return;

    const auto isInGesture = std::any_of (tracked.begin(), tracked.end(), [&] (const auto& entry)
    {
        return entry.property == name && entry.isInGesture;
    });

    const auto nameCopy = name;

    if (! isInGesture)
        provider->beginGesture (nameCopy);

    provider->setValue (nameCopy, value);

    if (! isInGesture)
        provider->endGesture (nameCopy);
}

void BindingModel::stopTracking (Component& component)
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

void BindingModel::bindingValueChanged (BindingProvider&, const Identifier& name)
{
    if (const auto* existing = getTypeInfo().findProperty (name);
        existing == nullptr || existing->type == ComponentProperty::Type::any)
    {
        refreshProperties();
    }

    notifyPropertyChanged (name);
}

void BindingModel::bindingProviderBeingDeleted (BindingProvider& deletedProvider)
{
    if (provider != &deletedProvider)
        return;

    for (auto& entry : tracked)
        entry.isInGesture = false;

    provider = nullptr;
}

void BindingModel::componentPropertyChanged (Component& component, const Identifier& property)
{
    static const Identifier pressedProperty ("pressed");

    if (property != pressedProperty || provider == nullptr)
        return;

    const auto isPressed = static_cast<bool> (component.getProperty (pressedProperty));

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

void BindingModel::componentBeingDeleted (Component& component)
{
    stopTracking (component);
}

} // namespace yup
