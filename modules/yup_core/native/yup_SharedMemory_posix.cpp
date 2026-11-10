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

Result SharedMemory::create (const String& newName, size_t numBytes)
{
    close();

    if (newName.isEmpty() || numBytes == 0)
        return Result::fail ("Invalid shared memory name or size");

    const auto posixName = "/" + newName;
    const int fd = shm_open (posixName.toRawUTF8(), O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd < 0)
        return Result::fail ("Cannot create shared memory " + newName + ": " + String (strerror (errno)));

    if (ftruncate (fd, static_cast<off_t> (numBytes)) != 0)
    {
        ::close (fd);
        shm_unlink (posixName.toRawUTF8());
        return Result::fail ("Cannot size shared memory " + newName);
    }

    auto* mapped = mmap (nullptr, numBytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ::close (fd);

    if (mapped == MAP_FAILED)
    {
        shm_unlink (posixName.toRawUTF8());
        return Result::fail ("Cannot map shared memory " + newName);
    }

    data = mapped;
    size = numBytes;
    name = newName;
    owner = true;
    return Result::ok();
}

Result SharedMemory::open (const String& newName, size_t numBytes)
{
    close();

    if (newName.isEmpty() || numBytes == 0)
        return Result::fail ("Invalid shared memory name or size");

    const auto posixName = "/" + newName;
    const int fd = shm_open (posixName.toRawUTF8(), O_RDWR, 0600);
    if (fd < 0)
        return Result::fail ("Cannot open shared memory " + newName + ": " + String (strerror (errno)));

    struct stat info;
    if (fstat (fd, &info) != 0 || static_cast<size_t> (info.st_size) < numBytes)
    {
        ::close (fd);
        return Result::fail ("Shared memory " + newName + " is smaller than requested");
    }

    auto* mapped = mmap (nullptr, numBytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ::close (fd);

    if (mapped == MAP_FAILED)
        return Result::fail ("Cannot map shared memory " + newName);

    data = mapped;
    size = numBytes;
    name = newName;
    owner = false;
    return Result::ok();
}

void SharedMemory::close()
{
    if (data == nullptr)
        return;

    munmap (data, size);

    if (owner)
        shm_unlink (("/" + name).toRawUTF8());

    data = nullptr;
    size = 0;
    name = {};
    owner = false;
}

} // namespace yup
