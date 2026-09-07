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

#include <atomic>

#include "yup_ArtboardViewModel.h"
#include "yup_ArtboardViewModelInstance.h"

namespace yup
{

class Artboard;

//==============================================================================
/** Represents a Rive file.

    This class is used to load Rive binary files (aka .riv files). It also
    provides access to the ViewModel schemas stored in the file and creates
    ViewModel instances that can be bound to Artboard components.
*/
class YUP_API ArtboardFile : public std::enable_shared_from_this<ArtboardFile>
{
public:
    //==============================================================================

    struct AssetInfo
    {
        AssetInfo() = default;

        String uniqueName;
        File uniquePath;
        String extension;
    };

    //==============================================================================
    /** The result of loading a Rive file. */
    using LoadResult = ResultValue<std::shared_ptr<ArtboardFile>>;
    using AssetLoadCallback = std::function<bool (const AssetInfo&, Span<const uint8>, rive::Factory& factory)>;

    /** Loads a Rive file from a file.

        @param file The file to load.
        @param factory The factory to use to create the Rive file.

        @return The result of loading the Rive file.
    */
    static LoadResult load (const File& file, rive::Factory& factory);

    /** Loads a Rive file from a file.

        @param file The file to load.
        @param factory The factory to use to create the Rive file.

        @return The result of loading the Rive file.
    */
    static LoadResult load (const File& file, rive::Factory& factory, const AssetLoadCallback& assetCallback);

    /** Loads a Rive file from an input stream.

        @param is The input stream to load the Rive file from.
        @param factory The factory to use to create the Rive file.

        @return The result of loading the Rive file.
    */
    static LoadResult load (InputStream& is, rive::Factory& factory);

    /** Loads a Rive file from an input stream.

        @param is The input stream to load the Rive file from.
        @param factory The factory to use to create the Rive file.
        @param assetCallback The callback that will be invoked when loading  to use to create the Rive file.

        @return The result of loading the Rive file.
    */
    static LoadResult load (InputStream& is, rive::Factory& factory, const AssetLoadCallback& assetCallback);

    //==============================================================================
    /** Returns the number of ViewModel schemas stored in the Rive file. */
    int getNumViewModels() const noexcept;

    /** Returns the names of all ViewModel schemas stored in the Rive file. */
    StringArray getViewModelNames();

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
    /** Returns the underlying Rive file. */
    const rive::File* getRiveFile() const;

    /** Returns the underlying Rive file. */
    rive::File* getRiveFile();

private:
    friend class ArtboardViewModelInstance;

    void enterObserverDispatch() noexcept { ++observerDispatchDepth; }
    void leaveObserverDispatch() noexcept { --observerDispatchDepth; }
    bool isObserverDispatchInProgress() const noexcept { return observerDispatchDepth.load() != 0; }

    ArtboardFile() = default;
    ArtboardFile (rive::rcp<rive::File> rivFile);

    rive::rcp<rive::File> rivFile;
    std::atomic<int> observerDispatchDepth { 0 };
};

} // namespace yup
