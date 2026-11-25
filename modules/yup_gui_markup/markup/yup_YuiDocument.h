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
    UndoManager set, each successful load is an undoable transaction, except the load of the
    document that is loaded already, which only reads again what it reads from other files.

    The other files the interface refers to, such as the source of a shader or a model, are read
    through the document with @ref readAsset, so that they come from where its text does: a folder,
    embedded resources or the unsaved texts of an editor (see YuiLoader).

    @code
    YuiDocument document;
    document.setModel ("params", paramsProvider);
    document.setAction ("reset", [this] (const Array<var>&) { reset(); });

    if (document.loadFromString (text).wasOk())
        addAndMakeVisible (*document.getRoot());

    if (auto* cutoff = document.findById<Slider> ("cutoff"))
        cutoff->setSkewFactor (0.5);
    @endcode

    @see YuiBuilder, YuiParser, StyleSheet, ComponentDocument
*/
class YUP_API YuiDocument : public ComponentDocument
{
public:
    //==============================================================================
    /** Creates an empty document. */
    YuiDocument();

    /** Destroys the document and every component it created. */
    ~YuiDocument() override;

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
    Result load (const DataTree& document, const DataTreeSourceMap& sourceMap = {});

    /** Destroys the interface. Actions and models are kept. */
    void clear();

    /** Sets the base directory used to resolve relative asset paths in components and drawings. */
    void setBaseDirectory (const File& directory) { builder.setBaseDirectory (directory); }

    /** Returns the base directory used to resolve relative asset paths in components and drawings. */
    const File& getBaseDirectory() const noexcept override { return builder.getBaseDirectory(); }

    /** Records each successful load as a transaction of an undo manager, or stops when null.

        Undoing a load loads the previous document again, in place. Loading the document that
        is loaded already, the same elements written at the same places, records nothing and
        keeps what can be redone: it is how the files the document reads are read again. The
        undo manager must outlive the document or be removed first; the transactions of a
        destroyed document do nothing.
    */
    void setUndoManager (UndoManager* undoManagerToUse) noexcept { undoManager = undoManagerToUse; }

    //==============================================================================
    /** Returns the bytes of a file the document refers to, or nothing when it can't be read. */
    using AssetReader = std::function<std::optional<MemoryBlock> (const String& path)>;

    /** Sets the function reading the files the document refers to, or removes it when null.

        These are the files read with @ref readAsset, such as the source of a shader or a model. A
        reader serves them from somewhere else than a folder: a YuiLoader sets its own on the document
        it fills, to read embedded resources and the unsaved texts of an editor, and removes it when
        it is destroyed.

        @param newReader  Receives the path given to @ref readAsset, and returns nothing for a path
                          it doesn't have, which is then read from its file.
    */
    void setAssetReader (AssetReader newReader) { assetReader = std::move (newReader); }

    /** Returns the bytes of a file the document refers to, or nothing when it can't be read.

        The asset reader is asked first, when there is one (see @ref setAssetReader). With no reader,
        and for a path the reader returns nothing for, the file is read: an absolute path as it is, a
        relative one from the base directory (see @ref setBaseDirectory), or from the working directory
        when the base directory doesn't have the file or isn't set, which is how Image and the drawings
        find their files.

        @param path  The path of the file, as written in the document. An empty path gives nothing.

        @see readAssetFor, getAssetFile
    */
    std::optional<MemoryBlock> readAsset (const String& path) const override;

    /** Returns the file on disk an asset path stands for, without reading it.

        This is for what has to follow the references a file makes to the ones next to it, such as a
        `.gltf` naming its buffers. The path is resolved as @ref readAsset does when it reads a file.

        @param path  The path of the file, as written in the document.

        @returns The file, which may not exist, or a default File for an empty path and when assets
                 aren't the files of a folder: a relative path with an asset reader set and no base
                 directory, as in a document loaded from embedded resources.
    */
    File getAssetFile (const String& path) const override;

    /** Tells the listeners that a file the document refers to changed (see Listener::documentAssetChanged).

        This call loads nothing: what read the file reads it again when told. A YuiLoader calls
        it for the files read through it that changed, each time it reloads and while it watches
        its files, where it then loads the document again so that its diagnostics are current.
        Call it after changing what an asset reader of your own returns for a path.

        @param path  The path of the file, as it was given to @ref readAsset.
    */
    void notifyAssetChanged (const String& path);

    //==============================================================================
    /** Returns the parsed document of the interface loaded last. */
    const DataTree& getTree() const noexcept { return tree; }

    /** Returns the root component, or nullptr if nothing is loaded. */
    Component* getRoot() const noexcept override { return builder.getRoot(); }

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

    //==============================================================================
    /** Registers the function run by `action ("name", ...)` handlers (see YuiBuilder::setAction). */
    void setAction (const String& name, YuiBuilder::Action action);

    /** Sets the function run by the handlers calling an action no function is registered for (see YuiBuilder::setFallbackAction). */
    void setFallbackAction (YuiBuilder::FallbackAction action);

    /** Makes the values of a provider available as `name.property` (see YuiBuilder::setModel).

        Set models before loading: they are seen by the interfaces loaded afterwards.
    */
    void setModel (const String& name, BindingProvider& provider);

    /** Removes a model, dropping the bindings and links made to it. */
    void removeModel (const String& name);

    /** Makes again, now, the components of the `repeat` blocks whose range changed (see YuiBuilder::applyPendingChanges).

        The message loop does it by itself, a tool or a test with none calls this.
    */
    void applyPendingChanges();

    //==============================================================================
    /** Returns the problems met by the last load and by the styling or bindings of its interface since.

        A failed parse leaves its error as the only entry. Each problem is listed once, with
        the line and column it was written at when known.
    */
    const std::vector<DataTreeDiagnostic>& getDiagnostics() const noexcept { return diagnostics; }

    /** Returns the builder creating the components, to inspect its bindings. */
    YuiBuilder& getBuilder() noexcept { return builder; }

    /** Returns the source map of the document loaded last, or nullptr.

        The map turns an element or a property of the tree into the line and column it was
        written at, which is how a development tool jumps from a component to its markup.
    */
    std::shared_ptr<const DataTreeSourceMap> getSourceMap() const noexcept override { return sourceMap; }

    /** Returns the resolver finding the components and the models of the document (see YuiBuilder::getComponentResolver). */
    DrawingScope::ComponentResolver getComponentResolver() const override { return builder.getComponentResolver(); }

    /** Adds a problem to the diagnostics, as the ones the bindings meet after a load are.

        This is how a component tells what is wrong with one of its values. Nothing is added while
        the document loads, nor after a load that failed.

        @param diagnostic  The problem, located with the source map of the document.
        @param subject     The component the problem is about, or nullptr. A diagnostic with no
                           element is given the one the component was built from.
    */
    void reportDiagnostic (DataTreeDiagnostic diagnostic, const Component* subject = nullptr) override;

    /** Returns the stylesheet built from the document, or nullptr when it declares no style.

        The sheet gives the rules styling the interface, which a development tool walks to show
        why a component looks the way it does.
    */
    StyleSheet::Ptr getStyleSheet() const noexcept { return styleSheet; }

    //==============================================================================
    /** Returns the document that built @p component, or the closest ancestor of it, or nullptr.

        Development tools pick a component on screen and need to know whether it came from markup
        before deciding where an edit should be written. The search walks up the parents, so a
        child added in code to a component built from a document reports that document.

        @param component The component to look up.
    */
    static YuiDocument* findDocumentFor (const Component* component);

    /** Returns every document that exists now, in the order they were created.

        Development tools gather the diagnostics of a whole application without being handed each
        of its documents, which is what this is for.
    */
    static std::vector<YuiDocument*> getAllDocuments();

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
        virtual void documentDiagnosed (YuiDocument& document, const DataTreeDiagnostic& diagnostic) {}

        /** Called when a file the document refers to changed, for what read it to read it again.

            Telling it does not load the document again (see YuiDocument::notifyAssetChanged): a
            YuiLoader watching the file does that next.

            @param path  The path of the file, as it was given to YuiDocument::readAsset.
        */
        virtual void documentAssetChanged (YuiDocument& document, const String& path) {}
    };

    /** Adds a listener, which must be removed before it is destroyed. */
    void addListener (Listener* listener) { listeners.add (listener); }

    /** Removes a listener. */
    void removeListener (Listener* listener) { listeners.remove (listener); }

    using ComponentDocument::addListener;
    using ComponentDocument::removeListener;

private:
    //==============================================================================
    class LoadAction;

    Result loadTree (const DataTree& document, const std::shared_ptr<const DataTreeSourceMap>& map);
    Result buildTree (const DataTree& document, const std::shared_ptr<const DataTreeSourceMap>& map);
    void handleLiveTreeMutation();
    void handleRangeChange();
    void attachStyleContext (Component& root);
    Result fail (const String& message);
    void resetState();
    Result notifyLoaded (Result result);
    void report (const std::shared_ptr<const DataTreeSourceMap>& map, const DataTreeDiagnostic& diagnostic);

    YuiBuilder builder;
    DataTree tree;
    std::shared_ptr<const DataTreeSourceMap> sourceMap;
    StyleSheet::Ptr styleSheet;
    std::vector<DataTreeDiagnostic> diagnostics;
    AssetReader assetReader;
    UndoManager* undoManager = nullptr;
    bool isLoading = false;
    bool lastLoadSucceeded = false;
    ListenerList<Listener> listeners;

    YUP_DECLARE_WEAK_REFERENCEABLE (YuiDocument)
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YuiDocument)
};

} // namespace yup
