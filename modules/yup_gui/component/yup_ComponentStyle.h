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
class ApplicationTheme;

//==============================================================================
/**
    The ComponentStyle class is the base class for all components styles.
 */
class JUCE_API ComponentStyle : public ReferenceCountedObject
{
public:
    //==============================================================================
    /** A pointer to a ComponentStyle object. */
    using Ptr = ReferenceCountedObjectPtr<ComponentStyle>;

    /** A pointer to a const ComponentStyle object. */
    using ConstPtr = ReferenceCountedObjectPtr<const ComponentStyle>;

    //==============================================================================
    /** Destructor. */
    virtual ~ComponentStyle() = default;

    //==============================================================================
    /** Paints the component with the specified style.

        This is the main method that derived classes must implement to define
        how a component should be painted with this style.

        @param g The graphics context to use for drawing.
        @param theme The application theme to use when painting.
        @param component The component to be painted.
    */
    virtual void paint (Graphics& g, const ApplicationTheme& theme, const Component& component) = 0;

    //==============================================================================
    /** Creates a style for a specific component type with a custom paint callback.

        This factory method creates a ComponentStyle that will call the provided
        paint callback when the component needs to be painted. The callback will
        receive a reference to the component cast to the specified ComponentType.

        @param paintCallback A function or lambda that will be called to paint the component.

        @return A new ComponentStyle object that uses the provided paint callback.

        @code
        // Example usage:
        auto buttonStyle = ComponentStyle::createStyle<Button>(
            [](Graphics& g, const ApplicationTheme& theme, const Button& button) {
                // Custom painting code for buttons
            });
        @endcode
    */
    template <class ComponentType, class F>
    static ComponentStyle::Ptr createStyle (F&& paintCallback)
    {
        class ComponentCachedStyle final : public ComponentStyle
        {
        public:
            using PaintCallback = std::function<void (Graphics&, const ApplicationTheme&, const ComponentType&)>;

            ComponentCachedStyle (PaintCallback paintCallback)
                : paintCallback (std::move (paintCallback))
            {
            }

            void paint (Graphics& g, const ApplicationTheme& theme, const Component& component) override
            {
                paintCallback (g, theme, static_cast<const ComponentType&> (component));
            }

        private:
            PaintCallback paintCallback;
        };

        return ComponentStyle::Ptr { new ComponentCachedStyle (std::move (paintCallback)) };
    }

protected:
    ComponentStyle() = default;
};

} // namespace yup
