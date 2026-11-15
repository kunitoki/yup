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

ComponentTypeRegistry& ComponentTypeRegistry::getInstance()
{
    static ComponentTypeRegistry instance;
    return instance;
}

void ComponentTypeRegistry::registerType (const ComponentTypeInfo& typeInfo)
{
    const ScopedLock sl (lock);
    types.insert_or_assign (typeInfo.getTypeName(), &typeInfo);
}

const ComponentTypeInfo* ComponentTypeRegistry::findType (const Identifier& typeName) const
{
    const ScopedLock sl (lock);

    if (auto it = types.find (typeName); it != types.end())
        return it->second;

    return nullptr;
}

} // namespace yup
