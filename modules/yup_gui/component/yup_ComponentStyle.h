/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

class Component;

//==============================================================================
/**
    The ComponentStyle class is the base class for all components styles.
 */
class JUCE_API ComponentStyle : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<ComponentStyle>;
    using ConstPtr = ReferenceCountedObjectPtr<const ComponentStyle>;

    virtual ~ComponentStyle() = default;

    virtual void invalidate() = 0;

    virtual void updateCache (const Component& component) = 0;

    virtual void paint (Graphics& g, const Component& component) = 0;

protected:
    ComponentStyle() = default;
};

//==============================================================================

template <class ComponentType>
class JUCE_API ComponentCachedStyle : public ComponentStyle
{
public:
    using PaintCallback = std::function<void (Graphics&, const ComponentType&)>;
    using UpdateCacheCallback = std::function<void (const ComponentType&)>;

    ComponentCachedStyle (PaintCallback paintCallback, UpdateCacheCallback updateCacheCallback = {})
        : paintCallback (std::move (paintCallback))
        , updateCacheCallback (std::move (updateCacheCallback))
    {
    }

    void invalidate() override final
    {
        cacheValid = false;
    }

    void updateCache (const Component& component) override final
    {
        if (cacheValid || updateCacheCallback == nullptr)
            return;

        updateCacheCallback (static_cast<const ComponentType&> (component));
        cacheValid = true;
    }

    void paint (Graphics& g, const Component& component) override final
    {
        updateCache (component);

        paintCallback (g, static_cast<const ComponentType&> (component));
    }

private:
    PaintCallback paintCallback;
    UpdateCacheCallback updateCacheCallback;
    bool cacheValid = false;
};

} // namespace yup
