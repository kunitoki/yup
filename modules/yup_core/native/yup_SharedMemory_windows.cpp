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

namespace
{
String getWindowsSharedMemoryName (const String& name)
{
    return "Local\\yup_shm_" + name;
}
} // namespace

Result SharedMemory::create (const String& newName, size_t numBytes)
{
    close();

    if (newName.isEmpty() || numBytes == 0)
        return Result::fail ("Invalid shared memory name or size");

    const auto size64 = static_cast<uint64> (numBytes);
    auto mapping = CreateFileMappingW (INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, static_cast<DWORD> (size64 >> 32), static_cast<DWORD> (size64 & 0xffffffff), getWindowsSharedMemoryName (newName).toWideCharPointer());

    if (mapping == nullptr)
        return Result::fail ("Cannot create shared memory " + newName);

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle (mapping);
        return Result::fail ("Shared memory " + newName + " already exists");
    }

    auto* mapped = MapViewOfFile (mapping, FILE_MAP_ALL_ACCESS, 0, 0, numBytes);
    if (mapped == nullptr)
    {
        CloseHandle (mapping);
        return Result::fail ("Cannot map shared memory " + newName);
    }

    data = mapped;
    size = numBytes;
    name = newName;
    owner = true;
    handle = reinterpret_cast<intptr_t> (mapping);
    return Result::ok();
}

Result SharedMemory::open (const String& newName, size_t numBytes)
{
    close();

    if (newName.isEmpty() || numBytes == 0)
        return Result::fail ("Invalid shared memory name or size");

    auto mapping = OpenFileMappingW (FILE_MAP_ALL_ACCESS, FALSE, getWindowsSharedMemoryName (newName).toWideCharPointer());
    if (mapping == nullptr)
        return Result::fail ("Cannot open shared memory " + newName);

    auto* mapped = MapViewOfFile (mapping, FILE_MAP_ALL_ACCESS, 0, 0, numBytes);
    if (mapped == nullptr)
    {
        CloseHandle (mapping);
        return Result::fail ("Cannot map shared memory " + newName);
    }

    data = mapped;
    size = numBytes;
    name = newName;
    owner = false;
    handle = reinterpret_cast<intptr_t> (mapping);
    return Result::ok();
}

void SharedMemory::close()
{
    if (data == nullptr)
        return;

    UnmapViewOfFile (data);
    CloseHandle (reinterpret_cast<HANDLE> (handle));

    data = nullptr;
    size = 0;
    name = {};
    owner = false;
    handle = 0;
}

} // namespace yup
