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
    //==============================================================================

    using Ptr = ReferenceCountedObjectPtr<ComponentStyle>;
    using ConstPtr = ReferenceCountedObjectPtr<const ComponentStyle>;

    //==============================================================================

    virtual ~ComponentStyle() = default;

    //==============================================================================

    virtual void paint (Graphics& g, const Component& component) = 0;

    //==============================================================================

    template <class ComponentType, class F>
    static ComponentStyle::Ptr createStyle (F&& paintCallback)
    {
        class ComponentCachedStyle : public ComponentStyle
        {
        public:
            using PaintCallback = std::function<void (Graphics&, const ComponentType&)>;

            ComponentCachedStyle (PaintCallback paintCallback)
                : paintCallback (std::move (paintCallback))
            {
            }

            void paint (Graphics& g, const Component& component) override final
            {
                paintCallback (g, static_cast<const ComponentType&> (component));
            }

        private:
            PaintCallback paintCallback;
        };

        ComponentStyle::Ptr result (new ComponentCachedStyle (std::move (paintCallback)));
        return result;
    }

protected:
    ComponentStyle() = default;
};

} // namespace yup
