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

void skipHandlerTrivia (const char* text, int& offset) noexcept
{
    for (;;)
    {
        const auto c = text[offset];

        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        {
            ++offset;
        }
        else if (c == '/' && text[offset + 1] == '/')
        {
            while (text[offset] != 0 && text[offset] != '\n')
                ++offset;
        }
        else if (c == '/' && text[offset + 1] == '*')
        {
            const auto* end = std::strstr (text + offset + 2, "*/");
            if (end == nullptr)
                return;

            offset = static_cast<int> (end - text) + 2;
        }
        else
        {
            return;
        }
    }
}

std::optional<YuiHandler::Call> extractHandlerCall (const Expression& expr, char firstChar)
{
    if (expr.getKind() == Expression::Kind::call && firstChar != '#')
    {
        YuiHandler::Call call;
        call.name = expr.getName();
        call.arguments.reserve (static_cast<std::size_t> (expr.getNumInputs()));

        for (int i = 0; i < expr.getNumInputs(); ++i)
            call.arguments.push_back (expr.getInput (i));

        return call;
    }

    if (expr.getKind() == Expression::Kind::method && expr.getInput (0).getKind() == Expression::Kind::symbol)
    {
        YuiHandler::Call call;
        call.target = expr.getInput (0).getName();
        call.name = expr.getName();
        call.arguments.reserve (static_cast<std::size_t> (expr.getNumInputs() - 1));

        for (int i = 1; i < expr.getNumInputs(); ++i)
            call.arguments.push_back (expr.getInput (i));

        return call;
    }

    return std::nullopt;
}

} // namespace

//==============================================================================

ResultValue<YuiHandler> YuiHandler::parse (const String& text)
{
    static const String callExpectedError ("expected a call, like action (\"name\") or root.signal()");

    YuiHandler handler;
    auto body = text.trim();

    if (body.startsWithChar ('('))
    {
        const auto close = body.indexOfChar (')');
        const auto rest = close >= 0 ? body.substring (close + 1).trimStart() : String();

        if (rest.startsWith ("=>"))
        {
            for (const auto& name : StringArray::fromTokens (body.substring (1, close), ",", {}))
            {
                const auto trimmed = name.trim();
                if (trimmed.isEmpty())
                    continue;

                if (handler.argumentNames.contains (trimmed))
                    return ResultValue<YuiHandler>::fail ("the argument '" + trimmed + "' is named twice");

                handler.argumentNames.add (trimmed);
            }

            body = rest.substring (2).trim();
        }
    }

    if (body.startsWithChar ('{'))
    {
        if (! body.endsWithChar ('}'))
            return ResultValue<YuiHandler>::fail (callExpectedError);

        const auto block = body.substring (1, body.length() - 1);
        const auto* utf8 = block.toRawUTF8();
        int offset = 0;

        for (;;)
        {
            skipHandlerTrivia (utf8, offset);

            while (utf8[offset] == ';')
            {
                ++offset;
                skipHandlerTrivia (utf8, offset);
            }

            if (utf8[offset] == 0)
                break;

            const auto firstChar = utf8[offset];
            const auto parsed = Expression::parseExpression (StringRef (CharPointer_UTF8 (utf8)), offset);

            if (parsed.failed() || (utf8[offset] != ';' && utf8[offset] != 0))
                return ResultValue<YuiHandler>::fail (callExpectedError);

            auto call = extractHandlerCall (parsed.getReference(), firstChar);
            if (! call.has_value())
                return ResultValue<YuiHandler>::fail (callExpectedError);

            handler.calls.push_back (std::move (*call));
        }

        return ResultValue<YuiHandler>::ok (std::move (handler));
    }

    if (body.isEmpty())
        return ResultValue<YuiHandler>::fail (callExpectedError);

    const auto* utf8 = body.toRawUTF8();
    int offset = 0;
    const auto parsed = Expression::parseExpression (StringRef (CharPointer_UTF8 (utf8)), offset);

    if (parsed.failed() || utf8[offset] != 0)
        return ResultValue<YuiHandler>::fail (callExpectedError);

    auto call = extractHandlerCall (parsed.getReference(), utf8[0]);
    if (! call.has_value())
        return ResultValue<YuiHandler>::fail (callExpectedError);

    handler.calls.push_back (std::move (*call));
    return ResultValue<YuiHandler>::ok (std::move (handler));
}

//==============================================================================

YuiHandlerArguments::YuiHandlerArguments (const StringArray& names, const std::vector<ComponentSignal::Argument>& arguments)
{
    std::vector<ComponentProperty> properties;
    properties.reserve (static_cast<size_t> (names.size()));

    for (int i = 0; i < names.size(); ++i)
    {
        const auto& argument = arguments[static_cast<size_t> (i)];

        properties.push_back ({
            .name = Identifier (names[i]),
            .type = argument.type,
            .elementType = argument.elementType,
            .getter = [i] (const Component& component)
            {
                const auto& currentValues = static_cast<const YuiHandlerArguments&> (component).values;
                return isPositiveAndBelow (i, currentValues.size()) ? currentValues.getReference (i) : var();
            },
            .isStyleable = false,
            .isInherited = false
        });
    }

    info = std::make_unique<ComponentTypeInfo> (Identifier ("Arguments"), nullptr, std::move (properties));
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
