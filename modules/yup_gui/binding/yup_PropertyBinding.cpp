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

PropertyBinding::PropertyBinding (Component& targetToUse, const Identifier& propertyToUse, std::unique_ptr<PropertyExpression> expressionToUse)
    : target (&targetToUse)
    , property (propertyToUse)
    , expression (std::move (expressionToUse))
{
    jassert (expression != nullptr);

    dependencies = expression->getDependencies();
}

PropertyBinding::~PropertyBinding() = default;

//==============================================================================

bool PropertyBinding::dependsOn (const Component& component, const Identifier& name) const
{
    return std::any_of (dependencies.begin(), dependencies.end(), [&] (const auto& dependency)
    {
        return dependency.component == &component && dependency.property == name;
    });
}

bool PropertyBinding::dependsOn (const BindingProvider& provider, const Identifier& name) const
{
    return std::any_of (dependencies.begin(), dependencies.end(), [&] (const auto& dependency)
    {
        return dependency.provider == &provider && dependency.property == name;
    });
}

bool PropertyBinding::evaluate()
{
    auto* component = target.get();
    if (component == nullptr)
        return false;

    const auto value = expression->evaluate();

    if (component->getProperty (property) == value)
        return false;

    component->setProperty (property, value);
    return true;
}

} // namespace yup
