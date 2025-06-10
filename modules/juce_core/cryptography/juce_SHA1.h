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

//==============================================================================
/**
    SHA-1 UNSECURE hash generator. Do not use this class for cryptographic uses.

    Create one of these objects from a block of source data or a stream, and it
    calculates the SHA-1 hash of that data.

    You can retrieve the hash as a raw 32-byte block, or as a 64-digit hex string.
    @see MD5, SHA256
*/
class YUP_API SHA1
{
public:
    //==============================================================================
    /** Creates an empty SHA1 object.
        The default constructor just creates a hash filled with zeros. (This is not
        equal to the hash of an empty block of data).
    */
    SHA1() noexcept;

    /** Destructor. */
    ~SHA1() noexcept;

    /** Creates a copy of another SHA1. */
    SHA1 (const SHA1& other) noexcept;

    /** Copies another SHA1. */
    SHA1& operator= (const SHA1& other) noexcept;

    //==============================================================================
    /** Creates a hash from a block of raw data. */
    explicit SHA1 (const MemoryBlock& data);

    /** Creates a hash from a block of raw data. */
    SHA1 (const void* data, size_t numBytes);

    /** Creates a hash from the contents of a stream.

        This will read from the stream until the stream is exhausted, or until
        maxBytesToRead bytes have been read. If maxBytesToRead is negative, the entire
        stream will be read.
    */
    SHA1 (InputStream& input, int64 maxBytesToRead = -1);

    /** Reads a file and generates the hash of its contents.
        If the file can't be opened, the hash will be left uninitialised (i.e. full
        of zeros).
    */
    explicit SHA1 (const File& file);

    /** Creates a checksum from a UTF-8 buffer.
        E.g.
        @code SHA1 checksum (myString.toUTF8());
        @endcode
    */
    explicit SHA1 (CharPointer_UTF8 utf8Text) noexcept;

    //==============================================================================
    /** Returns the hash as a 32-byte block of data. */
    Span<const uint8> getRawData() const { return result; }

    /** Returns the checksum as a 64-digit hex string. */
    String toHexString() const;

    //==============================================================================
    bool operator== (const SHA1&) const noexcept;
    bool operator!= (const SHA1&) const noexcept;

private:
    //==============================================================================
    uint8 result[20];
    void process (const void*, size_t);

    YUP_LEAK_DETECTOR (SHA1)
};

} // namespace yup
