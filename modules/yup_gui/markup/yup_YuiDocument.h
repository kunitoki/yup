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
    A user interface loaded from `.yui` text.

    The document parses the text with its imports (see YuiParser), builds the stylesheet
    of its `style` and `palette` blocks (see StyleSheet), creates the components (see
    YuiBuilder) and attaches the stylesheet to the root, which is then ready to be added to
    a parent or a window.

    Loading again replaces the interface. When the text has an error, or its stylesheet is
    invalid, the interface already loaded is kept, so that a live editor keeps showing the
    last good version; when the components can't be created, nothing is left loaded. Values
    which can't be applied don't make loading fail: they are reported by @ref getDiagnostics.

    @code
    YuiDocument document;
    document.setModel ("params", paramsProvider);
    document.setAction ("reset", [this] (const Array<var>&) { reset(); });

    if (document.loadFromString (text).wasOk())
        addAndMakeVisible (*document.getRoot());

    if (auto* cutoff = document.findById<Slider> ("cutoff"))
        cutoff->setSkewFactor (0.5);
    @endcode

    @see YuiBuilder, YuiParser, StyleSheet
*/
class YUP_API YuiDocument
{
public:
    //==============================================================================
    /** Creates an empty document. */
    YuiDocument();

    /** Destroys the document and every component it created. */
    ~YuiDocument();

    //==============================================================================
    /** Parses `.yui` text and loads the interface it describes.

        @param text           The `.yui` text.
        @param resolveImport  Returns the text of each imported path, or null when the text
                              imports nothing.

        @returns A failure when the text, its stylesheet or its components are invalid.
    */
    Result loadFromString (const String& text, YuiParser::ImportResolver resolveImport = nullptr);

    /** Loads the interface of an already parsed document.

        @param document A `yui` document, as returned by YuiParser.

        @returns A failure when the stylesheet or the components are invalid.
    */
    Result load (const DataTree& document);

    /** Destroys the interface. Actions and models are kept. */
    void clear();

    //==============================================================================
    /** Returns the parsed document of the interface loaded last. */
    const DataTree& getTree() const noexcept { return tree; }

    /** Returns the root component, or nullptr if nothing is loaded. */
    Component* getRoot() const noexcept { return builder.getRoot(); }

    /** Returns the component with the given ID, or nullptr (see YuiBuilder::findById). */
    Component* findById (const String& id) const { return builder.findById (id); }

    /** Returns the component with the given ID and type, or nullptr. */
    template <class T>
    T* findById (const String& id) const
    {
        return builder.findById<T> (id);
    }

    /** Returns the type info of a type declared by the document or registered, or nullptr. */
    const ComponentTypeInfo* findTypeInfo (const Identifier& typeName) { return builder.findTypeInfo (typeName); }

    /** Returns the stylesheet attached to the root, or nullptr if nothing is loaded. */
    StyleSheet::Ptr getStyleSheet() const noexcept { return styleSheet; }

    //==============================================================================
    /** Registers the function run by `action ("name", ...)` handlers (see YuiBuilder::setAction). */
    void setAction (const String& name, YuiBuilder::Action action);

    /** Makes the values of a provider available as `name.property` (see YuiBuilder::setModel).

        Set models before loading: they are seen by the interfaces loaded afterwards.
    */
    void setModel (const String& name, BindingProvider& provider);

    /** Removes a model, dropping the bindings and links made to it. */
    void removeModel (const String& name);

    //==============================================================================
    /** Returns the problems met by the last load, the parse error first if any. */
    const StringArray& getDiagnostics() const noexcept { return diagnostics; }

    /** Returns the parse error of the last load, with its line, column and source, if any. */
    const std::optional<YuiParser::Diagnostic>& getParseError() const noexcept { return parseError; }

    /** Returns the builder creating the components, to inspect its bindings. */
    YuiBuilder& getBuilder() noexcept { return builder; }

private:
    //==============================================================================
    YuiBuilder builder;
    DataTree tree;
    StyleSheet::Ptr styleSheet;
    std::optional<YuiParser::Diagnostic> parseError;
    StringArray diagnostics;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YuiDocument)
};

} // namespace yup
