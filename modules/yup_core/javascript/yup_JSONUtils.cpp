/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

var JSONUtils::makeObject (const std::map<Identifier, var>& source)
{
    auto result = std::make_unique<DynamicObject>();

    for (const auto& [name, value] : source)
        result->setProperty (name, value);

    return var (result.release());
}

var JSONUtils::makeObjectWithKeyFirst (const std::map<Identifier, var>& source,
                                       Identifier key)
{
    auto result = std::make_unique<DynamicObject>();

    if (const auto iter = source.find (key); iter != source.end())
        result->setProperty (key, iter->second);

    for (const auto& [name, value] : source)
        if (name != key)
            result->setProperty (name, value);

    return var (result.release());
}

std::optional<var> JSONUtils::setPointer (const var& v,
                                          String pointer,
                                          const var& newValue)
{
    if (pointer.isEmpty())
        return newValue;

    if (! pointer.startsWith ("/"))
    {
        // This is not a well-formed JSON pointer
        jassertfalse;
        return {};
    }

    const auto findResult = pointer.indexOfChar (1, '/');
    const auto pos = findResult < 0 ? pointer.length() : findResult;
    const String head (pointer.begin() + 1, pointer.begin() + pos);
    const String tail (pointer.begin() + pos, pointer.end());

    const auto unescaped = head.replace ("~1", "/").replace ("~0", "~");

    if (auto* object = v.getDynamicObject())
    {
        if (const auto newProperty = setPointer (object->getProperty (unescaped), tail, newValue))
        {
            auto cloned = object->clone();
            cloned->setProperty (unescaped, *newProperty);
            return var (cloned.release());
        }
    }
    else if (auto* array = v.getArray())
    {
        const auto index = [&]() -> size_t
        {
            if (unescaped == "-")
                return (size_t) array->size();

            if (unescaped == "0")
                return 0;

            if (! unescaped.startsWith ("0"))
                return (size_t) unescaped.getLargeIntValue();

            return std::numeric_limits<size_t>::max();
        }();

        if (const auto newIndex = setPointer ((*array)[(int) index], tail, newValue))
        {
            auto copied = *array;

            if ((int) index == copied.size())
                copied.add ({});

            if (isPositiveAndBelow (index, copied.size()))
            {
                copied.getReference ((int) index) = *newIndex;
                return var (copied);
            }
        }
    }

    return {};
}

bool JSONUtils::deepEqual (const var& a, const var& b)
{
    const auto compareObjects = [] (const DynamicObject& x, const DynamicObject& y)
    {
        if (x.getProperties().size() != y.getProperties().size())
            return false;

        for (const auto& [key, value] : x.getProperties())
        {
            if (! y.hasProperty (key))
                return false;

            if (! deepEqual (value, y.getProperty (key)))
                return false;
        }

        return true;
    };

    if (auto* i = a.getDynamicObject())
        if (auto* j = b.getDynamicObject())
            return compareObjects (*i, *j);

    if (auto* i = a.getArray())
        if (auto* j = b.getArray())
            return std::equal (i->begin(), i->end(), j->begin(), j->end(), [] (const var& x, const var& y)
            {
                return deepEqual (x, y);
            });

    return a == b;
}

} // namespace yup
