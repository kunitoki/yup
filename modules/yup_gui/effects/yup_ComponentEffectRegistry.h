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
    A process-wide registry of ComponentEffect factories, keyed by name.

    Stylesheets select effects by name with an `effect:` declaration, so an application
    registers the effects it wants to style before attaching its sheets:

    @code
    ComponentEffectRegistry::getInstance().registerEffect ("neonGlow", []
    {
        return ComponentEffect::Ptr (new NeonGlowEffect());
    });
    @endcode

    The factory runs once per styled component, so effects keeping per-component state
    never share it.

    @see ComponentEffect, Component::setComponentEffect, StyleContext

    @tags{UI}
*/
class YUP_API ComponentEffectRegistry
{
public:
    //==============================================================================
    /** Creates a new effect instance. */
    using Factory = std::function<ComponentEffect::Ptr()>;

    //==============================================================================
    /** Returns the process-wide registry instance. */
    static ComponentEffectRegistry& getInstance();

    //==============================================================================
    /** Registers an effect factory under a name, replacing any previous one.

        @param name    The name `effect:` declarations select it with.
        @param factory The function creating a new effect instance.
    */
    void registerEffect (const Identifier& name, Factory factory);

    /** Removes an effect by name. */
    void unregisterEffect (const Identifier& name);

    /** Returns true when an effect with the given name is registered. */
    bool isRegistered (const Identifier& name) const;

    /** Creates a new instance of a registered effect, or nullptr when the name is unknown. */
    ComponentEffect::Ptr createEffect (const Identifier& name) const;

private:
    //==============================================================================
    ComponentEffectRegistry() = default;

    std::unordered_map<Identifier, Factory> factories;
    mutable CriticalSection lock;

    YUP_DECLARE_NON_COPYABLE (ComponentEffectRegistry)
};

} // namespace yup
