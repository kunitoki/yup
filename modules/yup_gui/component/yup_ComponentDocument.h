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
    What a tree of components was built from, as its components see it.

    A component that reads a file, or finds something wrong with one of its values, asks the
    document that built it, found with @ref findFor. The file then comes from where the
    document does: a folder, embedded resources or the unsaved text of an editor. The problem
    is listed with the ones of the document, at the line its value was written at.

    @code
    // In a component: read through the document that built it, or from the file when none did
    if (const auto bytes = ComponentDocument::readAssetFor (this, "images/logo.png"))
        image = Image::loadFromData (bytes->asBytes());

    // Tell what is wrong with a value, where the document lists its problems
    if (auto* document = ComponentDocument::findFor (this))
        document->reportDiagnostic ({ .message = "The source can't be read", .property = "source" }, this);
    @endcode

    Code building components from a description of its own implements the interface, and calls
    @ref attachToRoot each time it has another root and @ref detachFromRoot before it goes. The
    root is what knows its document: there is no list of the documents there are.

    @see Component, DataTreeDiagnostic, DataTreeSourceMap

    @tags{UI}
*/
class YUP_API ComponentDocument
{
public:
    //==============================================================================
    /** Destructor. */
    virtual ~ComponentDocument();

    //==============================================================================
    /** Returns the root of the components the document built, or nullptr when it built none. */
    virtual Component* getRoot() const = 0;

    /** Returns the folder the relative paths of the document are resolved from, or a default File. */
    virtual const File& getBaseDirectory() const = 0;

    /** Returns the bytes of a file the document refers to, or nothing when it can't be read.

        @param path  The path of the file, as written in the document.

        @see readAssetFor, getAssetFile
    */
    virtual std::optional<MemoryBlock> readAsset (const String& path) const = 0;

    /** Returns the file on disk an asset path stands for, without reading it.

        This is for what has to follow the references a file makes to the ones next to it.

        @param path  The path of the file, as written in the document.

        @returns The file, which may not exist, or a default File when the assets of the document
                 are not the files of a folder.
    */
    virtual File getAssetFile (const String& path) const = 0;

    /** Returns where the elements of the document were written, or nullptr when it is not known.

        Each load gives the document another map. What tells one load from the next is
        @ref getLoadGeneration.
    */
    virtual std::shared_ptr<const DataTreeSourceMap> getSourceMap() const = 0;

    /** Returns the function finding the components and the models an expression names, as the
        values of the document find them.
    */
    virtual DrawingScope::ComponentResolver getComponentResolver() const = 0;

    /** Adds a problem to the ones the document lists.

        @param diagnostic  The problem. Its position is found from its element and its property.
        @param subject     The component the problem is about, or nullptr. A diagnostic with no
                           element is given the one the component was built from.
    */
    virtual void reportDiagnostic (DataTreeDiagnostic diagnostic, const Component* subject = nullptr) = 0;

    /** Returns a number that changes each time the document forgets the problems it listed.

        A load forgets them, and so does building the interface again for a change of what it was
        loaded from: what the components told with @ref reportDiagnostic is listed no more. A
        component with something wrong keeps the number it told it under, and compares it when it
        next can, at each of its frames for one, to tell again what is still wrong.

        The number is 0 until the document first loads. A load that fails, which keeps the
        interface there was, leaves it as it is.
    */
    uint32 getLoadGeneration() const noexcept { return loadGeneration; }

    //==============================================================================
    /** Receives what happens to a document, for the components that were built from it. */
    class YUP_API Listener
    {
    public:
        /** Destructor. A listener following a document is removed from it. */
        virtual ~Listener();

        /** Called after each load of the document, successful or not, and each time its interface is
            built again without a load: after a change of its tree or of the range of a repeat.
        */
        virtual void documentLoaded (ComponentDocument& document) {}

        /** Called when a file the document refers to changed, for what read it to read it again.

            @param path  The path of the file, as it was given to ComponentDocument::readAsset.
        */
        virtual void documentAssetChanged (ComponentDocument& document, const String& path) {}

        /** Adds the listener to a document, after removing it from the one it followed before.

            A listener that follows is removed by itself when it is destroyed, and may outlive the
            document: neither has to know which of the two goes first. What reads a file through
            the document of a component follows what ComponentDocument::findFor() returns before
            reading, to read the file again when it changes.

            @param document  The document to follow, or nullptr to follow none.

            @returns True when this is another document than the one followed so far.
        */
        bool follow (ComponentDocument* document);

    private:
        WeakReference<ComponentDocument> followed;
    };

    /** Adds a listener, which must be removed before it is destroyed. One that follows the
        document is added and removed by itself (see Listener::follow).
    */
    void addListener (Listener* listener) { documentListeners.add (listener); }

    /** Removes a listener. */
    void removeListener (Listener* listener) { documentListeners.remove (listener); }

    //==============================================================================
    /** Returns the document that built a component, or the closest ancestor of it, or nullptr.

        The search walks up the parents, so a child added in code to a component built from a
        document finds that document.

        @param component  The component to look up, which can be null.
    */
    static ComponentDocument* findFor (const Component* component);

    /** Returns the bytes of a file a component refers to, read through the document of the component.

        This is for the code that only has a component, such as an effect, a drawing or a component
        loading a model. A component of no document reads the file: an absolute path as it is, a
        relative one from the working directory.

        @param component  The component the path was written for, which can be null.
        @param path       The path of the file. An empty path gives nothing.
    */
    static std::optional<MemoryBlock> readAssetFor (const Component* component, const String& path);

    /** Returns the file on disk a path stands for, where @ref readAssetFor reads it.

        This is for what has to follow the references a file makes to the ones next to it, such as
        an SVG naming its images.

        @param component  The component the path was written for, which can be null.
        @param path       The path of the file.

        @returns The file, which may not exist, or a default File when the assets of the document
                 of the component are not the files of a folder.
    */
    static File getAssetFileFor (const Component* component, const String& path);

protected:
    //==============================================================================
    ComponentDocument() = default;

    /** Makes the document the one @ref findFor returns for its root and the components inside it.

        Call it each time @ref getRoot gives another component, as soon as it does: the components
        made under the root look for their document while they are given their values. Not from
        the constructor of this class, where the root can't be asked for. A document that has no
        root anymore is found by no component.

        A component is the root of one document: the last one attached to it.
    */
    void attachToRoot();

    /** Stops @ref findFor from returning the document.

        Call it first thing in a destructor: the components destroyed with the document may look
        for it, and would ask for the root of a document that is going.
    */
    void detachFromRoot();

    /** Tells the listeners that the document was loaded. */
    void notifyDocumentLoaded();

    /** Gives @ref getLoadGeneration another number.

        Call it each time the problems listed so far are forgotten, once the interface is the one
        of another load or was built again, and before the listeners are told.
    */
    void advanceLoadGeneration() noexcept { ++loadGeneration; }

    /** Tells the listeners that a file the document refers to changed. */
    void notifyDocumentAssetChanged (const String& path);

    /** Returns the file a path stands for: an absolute path as it is, a relative one from the
        base directory when that has the file, else from the working directory.
    */
    static File resolveAssetFile (const String& path, const File& baseDirectory);

    /** Returns the bytes of a file, or nothing when it can't be read. */
    static std::optional<MemoryBlock> readAssetFile (const File& file);

private:
    //==============================================================================
    ListenerList<Listener> documentListeners;
    WeakReference<Component> attachedRoot;
    uint32 loadGeneration = 0;

    YUP_DECLARE_WEAK_REFERENCEABLE (ComponentDocument)
    YUP_DECLARE_NON_COPYABLE (ComponentDocument)
};

} // namespace yup
