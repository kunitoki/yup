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

YuiDocument::YuiDocument() = default;

YuiDocument::~YuiDocument()
{
    clear();
}

//==============================================================================

Result YuiDocument::loadFromString (const String& text, YuiParser::ImportResolver resolveImport)
{
    const auto parsed = resolveImport != nullptr ? YuiParser::parse (text, resolveImport) : YuiParser::parse (text);

    if (! parsed.wasOk())
    {
        parseError = parsed.diagnostics.front();
        diagnostics = StringArray (parseError->toString());
        return Result::fail (parseError->toString());
    }

    return load (parsed.document);
}

Result YuiDocument::load (const DataTree& document)
{
    parseError.reset();
    diagnostics.clear();

    auto sheet = StyleSheet::fromTree (document);

    if (sheet.failed())
    {
        diagnostics.add (sheet.getErrorMessage());
        return Result::fail (sheet.getErrorMessage());
    }

    tree = document;
    styleSheet = nullptr;

    if (auto result = builder.build (document); result.failed())
    {
        diagnostics.add (result.getErrorMessage());
        diagnostics.addArray (builder.getDiagnostics());
        return result;
    }

    styleSheet = sheet.getReference();

    auto& root = *builder.getRoot();
    root.setStyleSheet (styleSheet);

    diagnostics.addArray (builder.getDiagnostics());

    if (auto* context = root.getStyleContext())
    {
        context->restyleNow();
        diagnostics.addArray (context->getDiagnostics());
    }

    return Result::ok();
}

void YuiDocument::clear()
{
    builder.clear();
    tree = {};
    styleSheet = nullptr;
    parseError.reset();
    diagnostics.clear();
}

//==============================================================================

void YuiDocument::setAction (const String& name, YuiBuilder::Action action)
{
    builder.setAction (name, std::move (action));
}

void YuiDocument::setModel (const String& name, BindingProvider& provider)
{
    builder.setModel (name, provider);
}

void YuiDocument::removeModel (const String& name)
{
    builder.removeModel (name);
}

} // namespace yup
