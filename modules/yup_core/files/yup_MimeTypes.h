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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once

namespace yup
{

/** A table for managing MIME types and their associated file extensions.

    This is used by the File class to determine the MIME type of a file based on its
    extension, and vice versa. Custom MIME types can be registered for specific file
    extensions, and the table can be queried to retrieve MIME types or file extensions.

*/
struct MimeTypeTable
{
    /** Registers a custom MIME type for a specific file extension.
    
        This allows the application to define its own MIME types for file extensions
        that may not be covered by the default table.

        @param mimeType The MIME type to register (e.g., "application/x-custom").
        @param fileExtension The file extension to associate with the MIME type (e.g., ".custom").
    */
    static void registerCustomMimeTypeForFileExtension (const String& mimeType, const String& fileExtension);

    /** Retrieves the MIME types associated with a given file extension.

        @param fileExtension The file extension to query (e.g., ".txt").

        @returns A StringArray containing all MIME types associated with the file extension.
    */
    static StringArray getMimeTypesForFileExtension (const String& fileExtension);

    /** Retrieves the file extensions associated with a given MIME type.

        @param mimeType The MIME type to query (e.g., "text/plain").

        @returns A StringArray containing all file extensions associated with the MIME type.
    */
    static StringArray getFileExtensionsForMimeType (const String& mimeType);
};

} // namespace yup
