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

    This class is used to load Rive binary files (aka .riv files).
*/
class YUP_API ArtboardFile
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
    /** Returns the underlying Rive file. */
    const rive::File* getRiveFile() const;

    /** Returns the underlying Rive file. */
    rive::File* getRiveFile();

private:
    ArtboardFile() = default;
    ArtboardFile (std::unique_ptr<rive::File> rivFile);

    std::unique_ptr<rive::File> rivFile;
};

} // namespace yup
