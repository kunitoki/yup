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

DependencyObserver::~DependencyObserver()
{
    clear();
}

void DependencyObserver::observe (Component& component)
{
    if (++observed[&component] == 1)
        component.addComponentListener (this);
}

void DependencyObserver::unobserve (Component& component)
{
    const auto it = observed.find (&component);

    if (it == observed.end() || --it->second > 0)
        return;

    observed.erase (it);
    component.removeComponentListener (this);
}

void DependencyObserver::observe (const std::vector<DrawingScope::Dependency>& dependencies)
{
    for (const auto& dependency : dependencies)
        if (dependency.component != nullptr)
            observe (*dependency.component);
}

void DependencyObserver::unobserve (const std::vector<DrawingScope::Dependency>& dependencies)
{
    for (const auto& dependency : dependencies)
        if (dependency.component != nullptr)
            unobserve (*dependency.component);
}

void DependencyObserver::clear()
{
    auto current = std::move (observed);
    observed.clear();

    for (const auto& [component, count] : current)
        component->removeComponentListener (this);
}

void DependencyObserver::componentPropertyChanged (Component& component, const Identifier& property)
{
    if (onChanged != nullptr)
        onChanged (component, property);
}

void DependencyObserver::componentBeingDeleted (Component& component)
{
    observed.erase (&component);

    if (onDeleted != nullptr)
        onDeleted (component);
}

} // namespace yup
