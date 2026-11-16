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

ComponentEffectRegistry::ComponentEffectRegistry() = default;

ComponentEffectRegistry& ComponentEffectRegistry::getInstance()
{
    static ComponentEffectRegistry instance;
    return instance;
}

void ComponentEffectRegistry::registerEffect (const Identifier& name, Factory factory)
{
    const ScopedLock sl (lock);
    factories.insert_or_assign (name, std::move (factory));
}

void ComponentEffectRegistry::unregisterEffect (const Identifier& name)
{
    const ScopedLock sl (lock);
    factories.erase (name);
}

bool ComponentEffectRegistry::isRegistered (const Identifier& name) const
{
    const ScopedLock sl (lock);
    return factories.find (name) != factories.end();
}

ComponentEffect::Ptr ComponentEffectRegistry::createEffect (const Identifier& name) const
{
    Factory factory;

    {
        const ScopedLock sl (lock);

        if (const auto it = factories.find (name); it != factories.end())
            factory = it->second;
    }

    return factory != nullptr ? factory() : nullptr;
}

} // namespace yup
