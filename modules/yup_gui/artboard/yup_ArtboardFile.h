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
*/

namespace yup
{

class Artboard;

//==============================================================================
/** Represents a Rive file.

    This class is used to load Rive binary files (aka .riv files). It also
    provides access to the ViewModel schemas stored in the file and creates
    ViewModel instances that can be bound to Artboard components.

    Files are always held through a shared pointer, handed out by load(). Every
    handle created from a file (ArtboardViewModel, ArtboardViewModelInstance)
    keeps it alive, so a file outlives everything that reads from it.

    Instances are not internally synchronized, and inherit the threading
    contract described on Artboard.
*/
class YUP_API ArtboardFile : public std::enable_shared_from_this<ArtboardFile>
{
public:
    //==============================================================================
    /** A shared pointer type for ArtboardFile instances. */
    using Ptr = std::shared_ptr<ArtboardFile>;

    //==============================================================================
    /** A structure containing information about an asset within the Rive file.

        Passed to an AssetLoadCallback so the host can supply the bytes of assets
        that the .riv file references without embedding.
    */
    struct AssetInfo
    {
        /** Default constructor. */
        AssetInfo() = default;

        /** The asset's name as authored in the Rive editor. */
        String uniqueName;

        /** The asset's authored name decorated with its Rive asset id and its
            extension, e.g. "logo-1234.png".

            This is a bare file name, not a path: it is the conventional name to
            look for in a directory of out-of-band assets, and nothing on disk is
            guaranteed to match it. Resolve it against your own asset directory
            with File::getChildFile(). */
        String uniqueFilename;

        /** The asset's file extension without the leading dot, e.g. "png". */
        String extension;
    };

    //==============================================================================
    /** A callback type for loading assets within the Rive file.

        Invoked once per asset referenced by the file while it is importing, or twice
        for a scripted file loaded with a GPU render context, which is imported again
        through a recording session with that session as the factory. To
        provide an asset, decode the bytes with the given factory, hand the result
        to the asset and return true. Return false to leave the asset unresolved,
        in which case Rive falls back to the in-band bytes when the file carries
        them. Returning false is not an import failure: a file whose assets all go
        unresolved still loads.

        @param info        Describes the asset being loaded.
        @param inBandBytes The bytes embedded in the .riv file, empty when the
                           asset is referenced out of band.
        @param factory     The factory to decode the asset with.
        @return True if the callback handled the asset.
    */
    using AssetLoadCallback = std::function<bool (const AssetInfo& info, Span<const uint8> inBandBytes, rive::Factory& factory)>;

    /** Loads a Rive file from a file.

        @param file The file to load.
        @param factory The factory to use to create the Rive file.

        @return The result of loading the Rive file.
    */
    static ResultValue<ArtboardFile::Ptr> load (const File& file, rive::Factory& factory);

    /** Loads a Rive file from a file, resolving its assets through a callback.

        @param file The file to load.
        @param factory The factory to use to create the Rive file.
        @param assetCallback The callback invoked once per asset referenced by the
                             file. Pass an empty callback to use the default in-band
                             asset loading.

        @return The result of loading the Rive file.
    */
    static ResultValue<ArtboardFile::Ptr> load (const File& file, rive::Factory& factory, const AssetLoadCallback& assetCallback);

    /** Loads a Rive file from an input stream.

        @param is The input stream to load the Rive file from.
        @param factory The factory to use to create the Rive file.

        @return The result of loading the Rive file.
    */
    static ResultValue<ArtboardFile::Ptr> load (InputStream& is, rive::Factory& factory);

    /** Loads a Rive file from an input stream, resolving its assets through a callback.

        @param is The input stream to load the Rive file from.
        @param factory The factory to use to create the Rive file.
        @param assetCallback The callback invoked once per asset referenced by the
                             file. Pass an empty callback to use the default in-band
                             asset loading.

        @return The result of loading the Rive file.
    */
    static ResultValue<ArtboardFile::Ptr> load (InputStream& is, rive::Factory& factory, const AssetLoadCallback& assetCallback);

    //==============================================================================
    /** Returns the number of ViewModel schemas stored in the Rive file. */
    int getNumViewModels() const noexcept;

    /** Returns the names of all ViewModel schemas stored in the Rive file. */
    StringArray getViewModelNames() const;

    /** Returns a handle to the ViewModel schema at the given index, or null if out of range.

        @param index The index of the ViewModel schema.
    */
    ArtboardViewModel::Ptr getArtboardViewModelAt (int index);

    /** Returns a handle to the ViewModel schema with the given name, or null if unknown.

        @param name The name of the ViewModel schema.
    */
    ArtboardViewModel::Ptr getArtboardViewModel (StringRef name);

    /** Creates a new instance of the ViewModel schema with the given name.

        The instance is populated with the schema's default property values.
        Bind the returned instance to an Artboard through
        Artboard::bindViewModelInstance() to drive its data bindings.

        @param viewModelName The name of the ViewModel schema to instantiate.
        @return The new instance handle, or null if the schema is unknown.
    */
    ArtboardViewModelInstance::Ptr createArtboardViewModelInstance (StringRef viewModelName);

    /** Creates an instance of the ViewModel schema by cloning a pre-authored instance.

        .riv files may ship with authored instances (see ArtboardViewModel::getInstanceNames()).
        The first authored instance is the file's "default" instance.

        @param viewModelName The name of the ViewModel schema to instantiate.
        @param instanceName  The name of the authored instance to clone.
        @return The new instance handle, or null if the schema or instance is unknown.
    */
    ArtboardViewModelInstance::Ptr createArtboardViewModelInstance (StringRef viewModelName, StringRef instanceName);

    //==============================================================================
    /** Returns the names of the file's global ViewModel schemas, in file order.

        A global ViewModel is not tied to one artboard: every Artboard showing this
        file binds the same shared instance of it, see getGlobalViewModelInstance().
    */
    StringArray getGlobalViewModelNames() const;

    /** Returns the file's shared instance of a global ViewModel.

        The instance is created from the schema's authored default the first time it
        is asked for, and every Artboard showing this file binds it, so writing a value
        through it updates all of them. Each call returns a new handle onto the same
        instance. Artboard::setGlobalViewModelInstance() replaces it for one artboard.

        @param name The name of a global ViewModel schema.
        @return The shared instance, or null if the name is not a global ViewModel of this file.
    */
    ArtboardViewModelInstance::Ptr getGlobalViewModelInstance (StringRef name);

    //==============================================================================
    /** Returns the underlying Rive file. */
    const rive::File* getRiveFile() const;

    /** Returns the underlying Rive file. */
    rive::File* getRiveFile();

    /** @internal Returns the factory the file was imported through.

        This is the factory passed to load(), except for scripted files rendering through a
        GPU render context, which import through a recording session instead.
    */
    rive::Factory* getFactory() const noexcept;

    /** @internal Returns the GPU render context the file draws with, or null when it was
        loaded with a factory that is not one.
    */
    rive::gpu::RenderContext* getRenderContext() const noexcept;

    /** Destructor. */
    ~ArtboardFile();

private:
    friend class Artboard;
    friend class ArtboardViewModelInstance;

    // Counts the observer callbacks currently unwinding across every instance
    // created from this file, so a retired observer is never freed while it is
    // still on the stack. A plain int matches the rest of the class: instances
    // and their observer vectors are unsynchronized, so the counter has nothing
    // to synchronize with.
    void enterObserverDispatch() noexcept { ++observerDispatchDepth; }

    void leaveObserverDispatch() noexcept { --observerDispatchDepth; }

    bool isObserverDispatchInProgress() const noexcept { return observerDispatchDepth != 0; }

    rive::cmd::DeferredSession* getDeferredSession() const noexcept { return deferredSession.get(); }
    rive::cmd::DeferredReplayer& getDeferredReplayer() noexcept;
    void bindDeferredRecordingThread();

    ArtboardFile() = default;
    ArtboardFile (rive::rcp<rive::File> rivFile,
                  rive::Factory& factory,
                  rive::gpu::RenderContext* renderContext,
                  std::unique_ptr<rive::cmd::DeferredSession> deferredSession);

    std::unique_ptr<rive::cmd::DeferredReplayer> deferredReplayer;
    std::unique_ptr<rive::cmd::DeferredSession> deferredSession;
    rive::rcp<rive::File> rivFile;
    rive::Factory* factory = nullptr;
    rive::gpu::RenderContext* renderContext = nullptr;
    HashMap<String, rive::rcp<rive::ViewModelInstance>> globalViewModelInstances;
    int observerDispatchDepth = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArtboardFile)
};

} // namespace yup
