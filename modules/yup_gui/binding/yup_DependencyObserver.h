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
    Reference-counted ComponentListener tracking property changes and deletions across a set of components.

    Used by BindingGraph, DrawingStyle and StyleContext so a component referenced by multiple
    dependencies registers a single listener and unregisters cleanly when the last reference
    is dropped or when the component is destroyed.

    @see DrawingScope::Dependency, BindingGraph, DrawingStyle, StyleContext

    @tags{UI}
*/
class YUP_API DependencyObserver final : private ComponentListener
{
public:
    //==============================================================================
    DependencyObserver() = default;
    ~DependencyObserver() override;

    //==============================================================================
    /** Increments the observation count for @p component, adding a listener on the first reference. */
    void observe (Component& component);

    /** Decrements the observation count for @p component, removing the listener on the last reference. */
    void unobserve (Component& component);

    /** Increments the observation count for every non-null component in @p dependencies. */
    void observe (const std::vector<DrawingScope::Dependency>& dependencies);

    /** Decrements the observation count for every non-null component in @p dependencies. */
    void unobserve (const std::vector<DrawingScope::Dependency>& dependencies);

    /** Removes all listeners and resets all observation counts. */
    void clear();

    /** Returns true when no components are currently observed. */
    bool isEmpty() const noexcept { return observed.empty(); }

    //==============================================================================
    /** Called when an observed component notifies a property change. */
    std::function<void (Component& component, const Identifier& property)> onChanged;

    /** Called after an observed component is removed on deletion. */
    std::function<void (Component& component)> onDeleted;

private:
    void componentPropertyChanged (Component& component, const Identifier& property) override;
    void componentBeingDeleted (Component& component) override;

    std::map<Component*, int> observed;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DependencyObserver)
};

} // namespace yup
