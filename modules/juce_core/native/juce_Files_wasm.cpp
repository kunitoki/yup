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

namespace juce
{

enum
{
    U_ISOFS_SUPER_MAGIC = 0x9660, // linux/iso_fs.h
    U_MSDOS_SUPER_MAGIC = 0x4d44, // linux/msdos_fs.h
    U_NFS_SUPER_MAGIC = 0x6969,   // linux/nfs_fs.h
    U_SMB_SUPER_MAGIC = 0x517B    // linux/smb_fs.h
};

bool File::isOnCDRomDrive() const
{
    struct statfs buf;

    return statfs (getFullPathName().toUTF8(), &buf) == 0
        && buf.f_type == (short) U_ISOFS_SUPER_MAGIC;
}

bool File::isOnHardDisk() const
{
    struct statfs buf;

    if (statfs (getFullPathName().toUTF8(), &buf) == 0)
    {
        switch (buf.f_type)
        {
            case U_ISOFS_SUPER_MAGIC: // CD-ROM
            case U_MSDOS_SUPER_MAGIC: // Probably floppy (but could be mounted FAT filesystem)
            case U_NFS_SUPER_MAGIC:   // Network NFS
            case U_SMB_SUPER_MAGIC:   // Network Samba
                return false;

            default:
                break;
        }
    }

    // Assume so if this fails for some reason
    return true;
}

bool File::isOnRemovableDrive() const
{
    jassertfalse; // xxx not implemented for linux!
    return false;
}

String File::getVersion() const
{
    return {}; // xxx not yet implemented
}

bool File::isSymbolicLink() const
{
    return false; // xxx not yet implemented
}

String File::getNativeLinkedTarget() const
{
    return {}; // xxx not yet implemented
}

//==============================================================================

const char* const* juce_argv = nullptr;
int juce_argc = 0;

File File::getSpecialLocation (const SpecialLocationType type)
{
    /*
    switch (type)
    {
        case userHomeDirectory:
        {
            if (const char* homeDir = getenv ("HOME"))
                return File (CharPointer_UTF8 (homeDir));

            if (auto* pw = getpwuid (getuid()))
                return File (CharPointer_UTF8 (pw->pw_dir));

            return {};
        }

        case userDocumentsDirectory:          return resolveXDGFolder ("XDG_DOCUMENTS_DIR", "~/Documents");
        case userMusicDirectory:              return resolveXDGFolder ("XDG_MUSIC_DIR",     "~/Music");
        case userMoviesDirectory:             return resolveXDGFolder ("XDG_VIDEOS_DIR",    "~/Videos");
        case userPicturesDirectory:           return resolveXDGFolder ("XDG_PICTURES_DIR",  "~/Pictures");
        case userDesktopDirectory:            return resolveXDGFolder ("XDG_DESKTOP_DIR",   "~/Desktop");
        case userApplicationDataDirectory:    return resolveXDGFolder ("XDG_CONFIG_HOME",   "~/.config");
        case commonDocumentsDirectory:
        case commonApplicationDataDirectory:  return File ("/opt");
        case globalApplicationsDirectory:     return File ("/usr");

        case tempDirectory:
        {
            if (const char* tmpDir = getenv ("TMPDIR"))
                return File (CharPointer_UTF8 (tmpDir));

            return File ("/tmp");
        }

        case invokedExecutableFile:
            if (juce_argv != nullptr && juce_argc > 0)
                return File (CharPointer_UTF8 (juce_argv[0]));
            // deliberate fall-through...

        case currentExecutableFile:
        case currentApplicationFile:
           #if ! JUCE_STANDALONE_APPLICATION
            return juce_getExecutableFile();
           #endif
            // deliberate fall-through if this is not a shared-library

        case hostApplicationPath:
        {
            const File f ("/proc/self/exe");
            return f.isSymbolicLink() ? f.getLinkedTarget() : juce_getExecutableFile();
        }

        default:
            jassertfalse; // unknown type?
            break;
    }
    */

    return { "/" };
}

//==============================================================================
bool File::moveToTrash() const
{
    if (! exists())
        return true;

    /*
    File trashCan ("~/.Trash");

    if (! trashCan.isDirectory())
        trashCan = "~/.local/share/Trash/files";

    if (! trashCan.isDirectory())
        return false;

    return moveFileTo (trashCan.getNonexistentChildFile (getFileNameWithoutExtension(),
                                                         getFileExtension()));
    */

    return false;
}

//==============================================================================
void File::revealToUser() const
{
    if (isDirectory())
        startAsProcess();

    else if (getParentDirectory().exists())
        getParentDirectory().startAsProcess();
}

//==============================================================================
class DirectoryIterator::NativeIterator::Pimpl
{
};

DirectoryIterator::NativeIterator::NativeIterator (const File& directory, const String& wildCardStr)
{
    ignoreUnused (directory, wildCardStr);
}

DirectoryIterator::NativeIterator::~NativeIterator() {}

bool DirectoryIterator::NativeIterator::next (String& filenameFound,
                                              bool* isDir,
                                              bool* isHidden,
                                              int64* fileSize,
                                              Time* modTime,
                                              Time* creationTime,
                                              bool* isReadOnly)
{
    ignoreUnused (filenameFound, isDir, isHidden, fileSize, modTime, creationTime, isReadOnly);
    return false;
}

//==============================================================================
static bool isFileExecutable (const String& filename)
{
    juce_statStruct info;

    return juce_stat (filename, info)
        && S_ISREG (info.st_mode)
        && access (filename.toUTF8(), X_OK) == 0;
}

bool Process::openDocument (const String& fileName, const String& parameters)
{
    auto cmdString = fileName.replace (" ", "\\ ", false);
    cmdString << " " << parameters;

    MAIN_THREAD_EM_ASM ({
        var elem = window.document.createElement ('a');
        elem.href = UTF8ToString ($0);
        elem.target = "_blank";
        document.body.appendChild (elem);
        elem.click();
        document.body.removeChild (elem);
    }, cmdString.toRawUTF8());

    return true;
}

bool Process::openDocument (const String& fileName, const String& parameters, const StringPairArray&)
{
    return openDocument (fileName, parameters);
}

} // namespace juce
