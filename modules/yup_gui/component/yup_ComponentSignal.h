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

class Component;

//==============================================================================
/**
    Describes a signal (an event) a component type emits, such as a button's `onClick`.

    A signal knows how to connect a handler to a component of its type, which is how `.yui`
    handlers such as `onClick: action ("reset");` are attached. The handler receives the
    arguments the signal is emitted with, for example the new value of a Slider.

    Connecting a handler to a widget callback replaces the handler already assigned to it.

    @see ComponentTypeInfo, ComponentProperty

    @tags{UI}
*/
struct ComponentSignal
{
    //==============================================================================
    /** Receives the arguments a signal is emitted with. */
    using Handler = std::function<void (const Array<var>& arguments)>;

    /** One argument the signal is emitted with, which a `.yui` handler can name: `(v) => action ("set", v)`. */
    struct Argument
    {
        /** The name the argument is documented with. Handlers name the arguments by position. */
        Identifier name;

        /** The type of the value, which decides how expressions read it. */
        ComponentProperty::Type type = ComponentProperty::Type::any;

        /** The type of the elements, when the value is an array. */
        ComponentProperty::Type elementType = ComponentProperty::Type::any;
    };

    //==============================================================================
    /** The name of the signal, as used in `.yui`, for example "onClick". */
    Identifier name;

    /** Connects a handler to a component of the type declaring the signal. */
    std::function<void (Component&, Handler)> connect;

    /** A human readable description, surfaced by tooling. */
    String description;

    /** The arguments the signal is emitted with, in order. */
    std::vector<Argument> arguments;
};

} // namespace yup
