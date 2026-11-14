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
    name once it is registered with YuiBuilder::setModel or YuiDocument::setModel:

    @code
    Slider { value <=> params.cutoff; }
    Label { text: format ("cutoff %.2f", params.cutoff); }
    @endcode

    Implementations report every change of a value with @ref notifyValueChanged, whoever made
    it, so bound components stay up to date. A continuous edit, such as dragging a knob, is
    wrapped in @ref beginGesture and @ref endGesture, which lets the provider group it into one
    undoable step or tell a host that a parameter is being automated.

    @see DataTreeBindingProvider, YuiBuilder, BindingGraph

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
    };

    //==============================================================================
    /** Receives the changes of the values of a provider. */
    class YUP_API Listener
    {
    public:
        virtual ~Listener() = default;

        /** Called after a value of the provider changed. */
        virtual void bindingValueChanged (BindingProvider& provider, const Identifier& name) = 0;
    };

    //==============================================================================
    virtual ~BindingProvider() = default;

    //==============================================================================
    /** Returns the values the provider exposes. */
    virtual std::vector<Property> getProperties() const = 0;

    /** Returns the current value of a property, or void if the provider doesn't expose it. */
    virtual var getValue (const Identifier& name) const = 0;

    /** Changes the value of a property, notifying the listeners if it changed. */
    virtual void setValue (const Identifier& name, const var& value) = 0;

    /** Called when a continuous edit of a property starts, for example when a knob is pressed. */
    virtual void beginGesture (const Identifier& name) {}

    /** Called when a continuous edit of a property ends. */
    virtual void endGesture (const Identifier& name) {}

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
    ListenerList<Listener> listeners;
};

} // namespace yup
