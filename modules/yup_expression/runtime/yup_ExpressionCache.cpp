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

//==============================================================================
ExpressionCache::ExpressionCache (const ExpressionContext& contextToUse)
    : context (contextToUse)
{
}

ExpressionCache::~ExpressionCache() = default;

//==============================================================================
ResultValue<CompiledExpression> ExpressionCache::compile (StringRef text)
{
    const ScopedLock sl (lock);

    const String key (text);

    if (const auto it = programs.find (key); it != programs.end())
        return it->second;

    return programs.emplace (key, CompiledExpression::compile (text, context)).first->second;
}

//==============================================================================
int ExpressionCache::size() const
{
    const ScopedLock sl (lock);

    return (int) programs.size();
}

int ExpressionCache::removeUnused()
{
    const ScopedLock sl (lock);

    const auto numPrograms = programs.size();

    std::erase_if (programs, [] (const auto& entry)
    {
        return entry.second.failed() || entry.second.getReference().program.use_count() == 1;
    });

    return (int) (numPrograms - programs.size());
}

void ExpressionCache::clear()
{
    const ScopedLock sl (lock);

    programs.clear();
}

} // namespace yup
