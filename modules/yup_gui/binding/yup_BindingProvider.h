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
    Exposes application state as named properties a UI can bind to.

    A provider is the model side of a data-driven UI: a set of typed values, such as the
    parameters of a synth or the settings of an application, that `.yui` reads and writes by
    name once it is registered as a model of the document, with its `setModel`:

    @code
    Slider { value <=> params.cutoff; }
    Label { text: format ("cutoff %.2f", params.cutoff); }
    @endcode

    Implementations report every change of a value with @ref notifyValueChanged, whoever made
    it, so bound components stay up to date. A continuous edit, such as dragging a knob, is
    wrapped in @ref beginGesture and @ref endGesture, which lets the provider group it into one
    undoable step or tell a host that a parameter is being automated. A BindingModel wraps any other
    change, from the keyboard, the mouse wheel or code, in a gesture of its own, and doesn't write
    a value equal to the current one.

    @see DataTreeBindingProvider, BindingModel, BindingGraph

    @tags{UI}
*/
class YUP_API BindingProvider
{
public:
    //==============================================================================
    /** One value a provider exposes. */
    struct Property
    {
        /** The name the value is bound with, as in `params.cutoff`. */
        Identifier name;

        /** The type of the value, which decides how expressions read it. */
        ComponentProperty::Type type = ComponentProperty::Type::any;

        /** The type of the elements, when the value is an array. */
        ComponentProperty::Type elementType = ComponentProperty::Type::any;
    };

    //==============================================================================
    /** Receives the changes of the values of a provider. */
    class YUP_API Listener
    {
    public:
        virtual ~Listener() = default;

        /** Called after a value of the provider changed. */
        virtual void bindingValueChanged (BindingProvider& provider, const Identifier& name) = 0;

        /** Called when the provider is about to be destroyed. */
        virtual void bindingProviderBeingDeleted (BindingProvider& provider)
        {
            ignoreUnused (provider);
        }
    };

    //==============================================================================
    virtual ~BindingProvider()
    {
        listeners.call ([this] (Listener& listener)
        {
            listener.bindingProviderBeingDeleted (*this);
        });

        masterReference.clear();
    }

    //==============================================================================
    /** Returns the values the provider exposes. */
    virtual std::vector<Property> getProperties() const = 0;

    /** Finds a single exposed property by name, or std::nullopt when the provider doesn't expose it.

        A provider may find here a property that getProperties() does not list, such as one it
        makes when it is first asked for: see BindingModel::ensureProperty().
    */
    virtual std::optional<Property> findProperty (const Identifier& name) const
    {
        for (const auto& property : getProperties())
            if (property.name == name)
                return property;

        return std::nullopt;
    }

    /** Returns the current value of a property, or void if the provider doesn't expose it. */
    virtual var getValue (const Identifier& name) const = 0;

    /** Changes the value of a property, notifying the listeners if it changed. */
    virtual void setValue (const Identifier& name, const var& value) = 0;

    /** Called when a continuous edit of a property starts, for example when a knob is pressed. */
    virtual void beginGesture (const Identifier& name) {}

    /** Called when a continuous edit of a property ends. */
    virtual void endGesture (const Identifier& name) {}

    /** Returns the text a value of a property is shown with, such as "440 Hz", or nothing when
        the provider has no text of its own for it, which is the default.

        It is the text a value opens with when it is edited as text, as `edit (params.cutoff)` does in `.yui`.

        @param name   The property.
        @param value  The value to show, which may not be the current one.
    */
    virtual std::optional<String> getTextForValue (const Identifier& name, const var& value) const
    {
        ignoreUnused (name, value);
        return std::nullopt;
    }

    /** Returns the value a text typed for a property stands for, or nothing when the provider
        does not parse the texts of the property itself, which is the default.

        @param name  The property.
        @param text  The text, as getTextForValue() returns it or as someone typed it.
    */
    virtual std::optional<var> getValueForText (const Identifier& name, const String& text) const
    {
        ignoreUnused (name, text);
        return std::nullopt;
    }

    //==============================================================================
    /** Adds a listener, which must be removed before it is destroyed. */
    void addListener (Listener* listener) { listeners.add (listener); }

    /** Removes a listener. */
    void removeListener (Listener* listener) { listeners.remove (listener); }

protected:
    //==============================================================================
    /** Tells the listeners that a value changed. Call it for every change, whoever made it. */
    void notifyValueChanged (const Identifier& name)
    {
        listeners.call ([&] (Listener& listener)
        {
            listener.bindingValueChanged (*this, name);
        });
    }

private:
    friend class WeakReference<BindingProvider>;

    ListenerList<Listener> listeners;
    WeakReference<BindingProvider>::Master masterReference;
};

} // namespace yup
