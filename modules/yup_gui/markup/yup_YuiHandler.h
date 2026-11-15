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
/** A parsed `.yui` handler: optional argument names, then one call or a block of calls.

    @code
    onClick: action ("save");
    onValueChanged: (v) => action ("gain", v * 2);
    onPicked: (index, name) => { action ("pick", name); root.onChanged (index); }
    @endcode
*/
struct YuiHandler
{
    /** One call, such as `action ("save", 1)` or `root.onChanged (index)`, with its parsed argument expressions. */
    struct Call
    {
        String target;
        String name;
        std::vector<Expression> arguments;
    };

    /** The names given to the arguments of the signal, by position. */
    StringArray argumentNames;

    /** The calls, run in order. */
    std::vector<Call> calls;

    /** Parses the text of a handler, the message of a failure carrying no position. */
    static ResultValue<YuiHandler> parse (const String& text);
};

//==============================================================================
/** The named arguments of a handler, which its expressions read by their bare names. */
class YuiHandlerArguments final : public BindingProvider
{
public:
    YuiHandlerArguments (const StringArray& names, const std::vector<ComponentSignal::Argument>& arguments);

    /** Sets the values of the arguments the signal was emitted with. */
    void setValues (const Array<var>& newValues) { values = newValues; }

    std::vector<Property> getProperties() const override { return properties; }
    var getValue (const Identifier& name) const override;
    void setValue (const Identifier&, const var&) override {}

private:
    std::vector<Property> properties;
    Array<var> values;
};

//==============================================================================
/** Runs a handler with the new value each time a property of a component changes. */
class YuiPropertyWatcher final : private ComponentListener
{
public:
    YuiPropertyWatcher (Component& component, const Identifier& property, ComponentSignal::Handler handler);
    ~YuiPropertyWatcher() override;

private:
    void componentPropertyChanged (Component& changed, const Identifier& changedProperty) override;

    WeakReference<Component> component;
    Identifier property;
    ComponentSignal::Handler handler;
};

} // namespace yup
