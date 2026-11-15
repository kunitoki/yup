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

namespace
{

// Splits on the separators outside strings and brackets, dropping empty pieces
StringArray splitOutsideNesting (const String& text, yup_wchar separator)
{
    StringArray pieces;
    auto position = text.getCharPointer();
    auto start = position;
    yup_wchar quote = 0;
    int depth = 0;

    for (;;)
    {
        const auto current = position;
        const auto c = position.getAndAdvance();

        if (c == 0 || (c == separator && depth == 0 && quote == 0))
        {
            const auto piece = String (start, current).trim();

            if (piece.isNotEmpty())
                pieces.add (piece);

            if (c == 0)
                break;

            start = position;
        }
        else if (quote != 0)
        {
            if (c == '\\' && ! position.isEmpty())
                ++position;
            else if (c == quote)
                quote = 0;
        }
        else if (c == '"' || c == '\'')
        {
            quote = c;
        }
        else if (c == '(' || c == '[')
        {
            ++depth;
        }
        else if ((c == ')' || c == ']') && depth > 0)
        {
            --depth;
        }
    }

    return pieces;
}

// Parses `callee (argument, ...)`
std::optional<YuiHandler::Call> parseCall (const String& text)
{
    const auto open = text.indexOfChar ('(');

    if (open <= 0 || ! text.endsWithChar (')'))
        return std::nullopt;

    YuiHandler::Call call;
    call.callee = text.substring (0, open).trim();

    if (! call.callee.containsOnly ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_."))
        return std::nullopt;

    call.arguments = splitOutsideNesting (text.substring (open + 1, text.length() - 1), ',');
    return call;
}

} // namespace

//==============================================================================

ResultValue<YuiHandler> YuiHandler::parse (const String& text)
{
    YuiHandler handler;
    auto body = text.trim();

    if (body.startsWithChar ('('))
    {
        const auto close = body.indexOfChar (')');
        const auto rest = body.substring (close + 1).trimStart();

        if (rest.startsWith ("=>"))
        {
            for (const auto& name : StringArray::fromTokens (body.substring (1, close), ",", {}))
            {
                const auto trimmed = name.trim();

                if (handler.argumentNames.contains (trimmed))
                    return ResultValue<YuiHandler>::fail ("the argument '" + trimmed + "' is named twice");

                handler.argumentNames.add (trimmed);
            }

            body = rest.substring (2).trim();
        }
    }

    const auto statements = body.startsWithChar ('{')
                              ? splitOutsideNesting (body.substring (1, body.length() - 1), ';')
                              : StringArray (body);

    for (const auto& statement : statements)
    {
        auto call = parseCall (statement);

        if (! call.has_value())
            return ResultValue<YuiHandler>::fail ("expected a call, like action (\"name\") or root.signal()");

        handler.calls.push_back (std::move (*call));
    }

    return ResultValue<YuiHandler>::ok (std::move (handler));
}

//==============================================================================

YuiHandlerArguments::YuiHandlerArguments (const StringArray& names, const std::vector<ComponentSignal::Argument>& arguments)
{
    for (int i = 0; i < names.size(); ++i)
    {
        const auto& argument = arguments[static_cast<size_t> (i)];
        properties.push_back ({ Identifier (names[i]), argument.type, argument.elementType });
    }
}

var YuiHandlerArguments::getValue (const Identifier& name) const
{
    const auto found = std::find_if (properties.begin(), properties.end(), [&] (const auto& property) { return property.name == name; });
    return found != properties.end() ? values[static_cast<int> (found - properties.begin())] : var();
}

//==============================================================================

YuiPropertyWatcher::YuiPropertyWatcher (Component& componentToWatch, const Identifier& propertyToWatch, ComponentSignal::Handler handlerToRun)
    : component (&componentToWatch)
    , property (propertyToWatch)
    , handler (std::move (handlerToRun))
{
    componentToWatch.addComponentListener (this);
}

YuiPropertyWatcher::~YuiPropertyWatcher()
{
    if (auto* watched = component.get())
        watched->removeComponentListener (this);
}

void YuiPropertyWatcher::componentPropertyChanged (Component& changed, const Identifier& changedProperty)
{
    if (changedProperty == property)
        handler ({ changed.getProperty (property) });
}

} // namespace yup
