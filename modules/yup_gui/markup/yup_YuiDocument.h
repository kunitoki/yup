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

    Loading again updates the interface in place: the loaded tree is patched with YuiTreeDiff
    and rebuilt, so the components whose elements are still there are kept, with their
    pointers and their state (see YuiBuilder::build). When the text has an error, its
    stylesheet is invalid or its components can't be created, the interface already loaded
    is kept unchanged, so that a live editor keeps showing the last good version. Values
    which can't be applied don't make loading fail: they are reported by @ref getDiagnostics,
    with the line and column they were written at, and so are the style problems met later,
    such as a `:hover` rule with an invalid value (see Listener::documentDiagnosed). With an
    UndoManager set, each successful load is an undoable transaction.

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
    /** Creates a DataTreeSchema describing the `.yui` document grammar and all registered component types. */
    static DataTreeSchema::Ptr createSchema();

    //==============================================================================
    /** Parses `.yui` text and loads the interface it describes.

        @param text           The `.yui` text.
        @param resolveImport  Returns the text of each imported path, or null when the text
                              imports nothing.

        @returns A failure when the text, its stylesheet or its components are invalid.
    */
    Result loadFromString (const String& text, YuiParser::ImportResolver resolveImport = nullptr);

    /** Loads the interface of an already parsed document.

        @param document   A `yui` document, as returned by YuiParser.
        @param sourceMap  Where the elements of the document were written, as returned by
                          YuiParser, which gives the diagnostics their line and column.

        @returns A failure when the stylesheet or the components are invalid.
    */
    Result load (const DataTree& document, const YuiSourceMap& sourceMap = {});

    /** Destroys the interface. Actions and models are kept. */
    void clear();

    /** Records each successful load as a transaction of an undo manager, or stops when null.

        Undoing a load loads the previous document again, in place. The undo manager must
        outlive the document or be removed first; the transactions of a destroyed document do
        nothing.
    */
    void setUndoManager (UndoManager* undoManagerToUse) noexcept { undoManager = undoManagerToUse; }

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
    /** Returns the problems met by the last load and by the styling or bindings of its interface since.

        A failed parse leaves its error as the only entry. Each problem is listed once, with
        the line and column it was written at when known.
    */
    const std::vector<YuiDiagnostic>& getDiagnostics() const noexcept { return diagnostics; }

    /** Returns the builder creating the components, to inspect its bindings. */
    YuiBuilder& getBuilder() noexcept { return builder; }

    //==============================================================================
    /** Receives the loads of a document. */
    class YUP_API Listener
    {
    public:
        virtual ~Listener() = default;

        /** Called after each load, successful or not, including the loads of undo and redo.

            A failed load keeps the interface loaded before, so the root may be unchanged, or
            null when nothing was loaded yet.
        */
        virtual void documentLoaded (YuiDocument& document, const Result& result) = 0;

        /** Called when a problem is met after a load, and added to the diagnostics.

            This happens when a rule starts matching later, such as a `:hover` rule with an
            invalid value. The problems met while loading are reported by documentLoaded.
        */
        virtual void documentDiagnosed (YuiDocument& document, const YuiDiagnostic& diagnostic) {}
    };

    /** Adds a listener, which must be removed before it is destroyed. */
    void addListener (Listener* listener) { listeners.add (listener); }

    /** Removes a listener. */
    void removeListener (Listener* listener) { listeners.remove (listener); }

private:
    //==============================================================================
    class LoadAction;

    Result loadTree (const DataTree& document, const std::shared_ptr<const YuiSourceMap>& map);
    Result buildTree (const DataTree& document, const std::shared_ptr<const YuiSourceMap>& map);
    void handleLiveTreeMutation();
    void attachStyleContext (Component& root);
    Result fail (const String& message);
    void resetState();
    Result notifyLoaded (Result result);
    void report (const std::shared_ptr<const YuiSourceMap>& map, const YuiDiagnostic& diagnostic);

    YuiBuilder builder;
    DataTree tree;
    std::shared_ptr<const YuiSourceMap> sourceMap;
    StyleSheet::Ptr styleSheet;
    std::vector<YuiDiagnostic> diagnostics;
    UndoManager* undoManager = nullptr;
    bool isLoading = false;
    bool lastLoadSucceeded = false;
    ListenerList<Listener> listeners;

    YUP_DECLARE_WEAK_REFERENCEABLE (YuiDocument)
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YuiDocument)
};

} // namespace yup
