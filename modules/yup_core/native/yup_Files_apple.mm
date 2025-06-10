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

namespace yup
{

/*
    Note that a lot of methods that you'd expect to find in this file actually
    live in yup_posix_SharedCode.h!
*/

//==============================================================================
bool File::copyInternal(const File& dest) const
{
    YUP_AUTORELEASEPOOL
    {
        NSFileManager* fm = [NSFileManager defaultManager];

        return [fm fileExistsAtPath:yupStringToNS(fullPath)] && [fm copyItemAtPath:yupStringToNS(fullPath)
                                                                            toPath:yupStringToNS(dest.getFullPathName())
                                                                             error:nil];
    }
}

void File::findFileSystemRoots(Array<File>& destArray)
{
    destArray.add(File("/"));
}

//==============================================================================
namespace MacFileHelpers
{
static bool isFileOnDriveType(const File& f, const char* const* types)
{
    struct statfs buf;

    if (yup_doStatFS(f, buf))
    {
        const String type(buf.f_fstypename);

        while (*types != nullptr)
            if (type.equalsIgnoreCase(*types++))
                return true;
    }

    return false;
}

static bool isHiddenFile(const String& path)
{
#if YUP_MAC
    YUP_AUTORELEASEPOOL
    {
        NSNumber* hidden = nil;
        NSError* err = nil;

        return [createNSURLFromFile(path) getResourceValue:&hidden forKey:NSURLIsHiddenKey error:&err] && [hidden boolValue];
    }
#else
    return File(path).getFileName().startsWithChar('.');
#endif
}

#if YUP_IOS
static String getIOSSystemLocation(NSSearchPathDirectory type)
{
    return nsStringToYup([NSSearchPathForDirectoriesInDomains(type, NSUserDomainMask, YES)
        objectAtIndex:0]);
}
#else
static bool launchExecutable(const String& pathAndArguments, const Array<char*>* environment = nullptr)
{
    auto cpid = fork();

    if (cpid == 0)
    {
        const char* const argv[4] = {"/bin/sh", "-c", pathAndArguments.toUTF8(), nullptr};

        // Child process
        if (execve(argv[0], (char**)argv, environment ? environment->getRawDataPointer() : environ) < 0)
            exit(0);
    }
    else
    {
        if (cpid < 0)
            return false;
    }

    return true;
}
#endif

bool openDocument(const String& fileName, const String& parameters, const Array<char*>* environment = nullptr)
{
    YUP_AUTORELEASEPOOL
    {
        NSString* fileNameAsNS(yupStringToNS(fileName));
        NSURL* filenameAsURL = File::createFileWithoutCheckingPath(fileName).exists() ? [NSURL fileURLWithPath:fileNameAsNS]
                                                                                      : [NSURL URLWithString:fileNameAsNS];

#if YUP_IOS
        ignoreUnused(parameters);

        if (@available(iOS 10.0, *))
        {
            [[UIApplication sharedApplication] openURL:filenameAsURL
                                               options:@{}
                                     completionHandler:nil];

            return true;
        }

        YUP_BEGIN_IGNORE_DEPRECATION_WARNINGS
        return [[UIApplication sharedApplication] openURL:filenameAsURL];
        YUP_END_IGNORE_DEPRECATION_WARNINGS

#else
        auto workspace = [NSWorkspace sharedWorkspace];
        bool isUrl = filenameAsURL && ![[filenameAsURL scheme] hasPrefix:@"file"];

        if (!isUrl)
        {
            const File file(fileName);

            if (file.isBundle() && file.getFileExtension().equalsIgnoreCase(".app"))
            {
                StringArray params;
                params.addTokens(parameters, true);

                NSMutableArray* paramArray = [[NSMutableArray new] autorelease];
                for (int i = 0; i < params.size(); ++i)
                    [paramArray addObject:yupStringToNS(params[i])];

                NSMutableDictionary* envDict = [[NSMutableDictionary new] autorelease];
                if (environment)
                {
                    for (int i = 0; i < environment->size(); ++i)
                    {
                        if (environment->getUnchecked(i) == nullptr)
                            continue;

                        auto keyValue = String(const_cast<const char*>(environment->getUnchecked(i)));

                        [envDict setObject:yupStringToNS(keyValue.fromFirstOccurrenceOf("=", false, false))
                                    forKey:yupStringToNS(keyValue.upToFirstOccurrenceOf("=", false, false))];
                    }
                }

#if (defined MAC_OS_X_VERSION_10_15) && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_15
                if (@available(macOS 10.15, *))
                {
                    auto config = [NSWorkspaceOpenConfiguration configuration];
                    [config setCreatesNewApplicationInstance:YES];
                    config.arguments = paramArray;

                    if (environment)
                        config.environment = envDict;

                    [workspace openApplicationAtURL:filenameAsURL
                                      configuration:config
                                  completionHandler:nil];

                    return true;
                }
#endif

                YUP_BEGIN_IGNORE_DEPRECATION_WARNINGS

                NSMutableDictionary* dict = [[NSMutableDictionary new] autorelease];

                if (params.size())
                {
                    [dict setObject:paramArray
                             forKey:nsStringLiteral("NSWorkspaceLaunchConfigurationArguments")];
                }

                if (environment)
                {
                    [dict setObject:envDict
                             forKey:nsStringLiteral("NSWorkspaceLaunchConfigurationEnvironment")];
                }

                return [workspace launchApplicationAtURL:filenameAsURL
                                                 options:NSWorkspaceLaunchDefault | NSWorkspaceLaunchNewInstance
                                           configuration:dict
                                                   error:nil];

                YUP_END_IGNORE_DEPRECATION_WARNINGS
            }
        }

        auto fileManager = [NSFileManager defaultManager];

        if (isUrl || File(fileName).isDirectory() || ![fileManager isExecutableFileAtPath:fileNameAsNS])
        {
            // NB: the length check here is because of strange failures involving long filenames,
            // probably due to filesystem name length limitations..
            YUP_BEGIN_IGNORE_DEPRECATION_WARNINGS
            return (fileName.length() < 1024 && [workspace openFile:fileNameAsNS]) || [workspace openURL:filenameAsURL];
            YUP_END_IGNORE_DEPRECATION_WARNINGS
        }

        if (File(fileName).exists())
            return MacFileHelpers::launchExecutable("\"" + fileName + "\" " + parameters, environment);

        return false;
#endif
    }
}
} // namespace MacFileHelpers

bool File::isOnCDRomDrive() const
{
    static const char* const cdTypes[] = {"cd9660", "cdfs", "cddafs", "udf", nullptr};

    return MacFileHelpers::isFileOnDriveType(*this, cdTypes);
}

bool File::isOnHardDisk() const
{
    static const char* const nonHDTypes[] = {"nfs", "smbfs", "ramfs", nullptr};

    return !(isOnCDRomDrive() || MacFileHelpers::isFileOnDriveType(*this, nonHDTypes));
}

bool File::isOnRemovableDrive() const
{
#if YUP_IOS
    return false; // xxx is this possible?
#else
    YUP_AUTORELEASEPOOL
    {
        BOOL removable = false;

        [[NSWorkspace sharedWorkspace]
            getFileSystemInfoForPath:yupStringToNS(getFullPathName())
                         isRemovable:&removable
                          isWritable:nil
                       isUnmountable:nil
                         description:nil
                                type:nil];

        return removable;
    }
#endif
}

bool File::isHidden() const
{
    return MacFileHelpers::isHiddenFile(getFullPathName());
}

//==============================================================================
const char* const* yup_argv = nullptr;
int yup_argc = 0;

File File::getSpecialLocation(const SpecialLocationType type)
{
    YUP_AUTORELEASEPOOL
    {
        String resultPath;

        switch (type)
        {
            case userHomeDirectory:
                resultPath = nsStringToYup(NSHomeDirectory());
                break;

#if YUP_IOS
            case userDocumentsDirectory:
                resultPath = MacFileHelpers::getIOSSystemLocation(NSDocumentDirectory);
                break;
            case userDesktopDirectory:
                resultPath = MacFileHelpers::getIOSSystemLocation(NSDesktopDirectory);
                break;

            case tempDirectory:
            {
                File tmp(MacFileHelpers::getIOSSystemLocation(NSCachesDirectory));
                tmp = tmp.getChildFile(yup_getExecutableFile().getFileNameWithoutExtension());
                tmp.createDirectory();
                return tmp.getFullPathName();
            }

#else
            case userDocumentsDirectory:
                resultPath = "~/Documents";
                break;
            case userDesktopDirectory:
                resultPath = "~/Desktop";
                break;

            case tempDirectory:
            {
                File tmp("~/Library/Caches/" + yup_getExecutableFile().getFileNameWithoutExtension());
                tmp.createDirectory();
                return File(tmp.getFullPathName());
            }
#endif
            case userMusicDirectory:
                resultPath = "~/Music";
                break;
            case userMoviesDirectory:
                resultPath = "~/Movies";
                break;
            case userPicturesDirectory:
                resultPath = "~/Pictures";
                break;
            case userApplicationDataDirectory:
                resultPath = "~/Library";
                break;
            case commonApplicationDataDirectory:
                resultPath = "/Library";
                break;
            case commonDocumentsDirectory:
                resultPath = "/Users/Shared";
                break;
            case globalApplicationsDirectory:
                resultPath = "/Applications";
                break;

            case invokedExecutableFile:
                if (yup_argv != nullptr && yup_argc > 0)
                    return File::getCurrentWorkingDirectory().getChildFile(String(CharPointer_UTF8(yup_argv[0])));
                // deliberate fall-through...
                YUP_FALLTHROUGH

            case currentExecutableFile:
                return yup_getExecutableFile();

            case currentApplicationFile:
            {
                const File exe(yup_getExecutableFile());
                const File parent(exe.getParentDirectory());

#if YUP_IOS
                return parent;
#else
                return parent.getFullPathName().endsWithIgnoreCase("Contents/MacOS")
                           ? parent.getParentDirectory().getParentDirectory()
                           : exe;
#endif
            }

            case hostApplicationPath:
            {
                unsigned int size = 8192;
                HeapBlock<char> buffer;
                buffer.calloc(size + 8);

                _NSGetExecutablePath(buffer.get(), &size);
                return File(String::fromUTF8(buffer, (int)size));
            }

            default:
                jassertfalse; // unknown type?
                break;
        }

        if (resultPath.isNotEmpty())
            return File(resultPath.convertToPrecomposedUnicode());
    }

    return {};
}

//==============================================================================
String File::getVersion() const
{
    YUP_AUTORELEASEPOOL
    {
        if (NSBundle* bundle = [NSBundle bundleWithPath:yupStringToNS(getFullPathName())])
            if (NSDictionary* info = [bundle infoDictionary])
                if (NSString* name = [info valueForKey:nsStringLiteral("CFBundleShortVersionString")])
                    return nsStringToYup(name);
    }

    return {};
}

//==============================================================================
static NSString* getFileLink(const String& path)
{
    return [[NSFileManager defaultManager] destinationOfSymbolicLinkAtPath:yupStringToNS(path) error:nil];
}

bool File::isSymbolicLink() const
{
    return getFileLink(fullPath) != nil;
}

String File::getNativeLinkedTarget() const
{
    if (NSString* dest = getFileLink(fullPath))
        return nsStringToYup(dest);

    return {};
}

//==============================================================================
bool File::moveToTrash() const
{
    if (!exists())
        return true;

    YUP_AUTORELEASEPOOL
    {
        if (@available(macOS 10.8, iOS 11.0, *))
        {
            NSError* error = nil;
            return [[NSFileManager defaultManager] trashItemAtURL:createNSURLFromFile(*this)
                                                 resultingItemURL:nil
                                                            error:&error];
        }

#if YUP_IOS
        return deleteFile();
#else
        [[NSWorkspace sharedWorkspace] recycleURLs:[NSArray arrayWithObject:createNSURLFromFile(*this)]
                                 completionHandler:nil];

        // recycleURLs is async, so we need to block until it has finished. We can't use a
        // run-loop here because it'd dispatch unexpected messages, so have to do this very
        // nasty bodge. But this is only needed for support of pre-10.8 versions.
        for (int retries = 100; --retries >= 0;)
        {
            if (!exists())
                return true;

            Thread::sleep(5);
        }

        return false;
#endif
    }
}

//==============================================================================
class DirectoryIterator::NativeIterator::Pimpl
{
   public:
    Pimpl(const File& directory, const String& wildcard)
        : parentDir(File::addTrailingSeparator(directory.getFullPathName())),
          wildCard(wildcard)
    {
        YUP_AUTORELEASEPOOL
        {
            enumerator = [[[NSFileManager defaultManager] enumeratorAtPath:yupStringToNS(directory.getFullPathName())] retain];
        }
    }

    ~Pimpl()
    {
        [enumerator release];
    }

    bool next(String& filenameFound,
              bool* const isDir,
              bool* const isHidden,
              int64* const fileSize,
              Time* const modTime,
              Time* const creationTime,
              bool* const isReadOnly)
    {
        YUP_AUTORELEASEPOOL
        {
            const char* wildcardUTF8 = nullptr;

            for (;;)
            {
                if (enumerator == nil)
                    return false;

                NSString* file = [enumerator nextObject];

                if (file == nil)
                    return false;

                [enumerator skipDescendents];
                filenameFound = nsStringToYup(file).convertToPrecomposedUnicode();

                if (wildcardUTF8 == nullptr)
                    wildcardUTF8 = wildCard.toUTF8();

                if (fnmatch(wildcardUTF8, filenameFound.toUTF8(), FNM_CASEFOLD) != 0)
                    continue;

                auto fullPath = parentDir + filenameFound;
                updateStatInfoForFile(fullPath, isDir, fileSize, modTime, creationTime, isReadOnly);

                if (isHidden != nullptr)
                    *isHidden = MacFileHelpers::isHiddenFile(fullPath);

                return true;
            }
        }
    }

   private:
    String parentDir, wildCard;
    NSDirectoryEnumerator* enumerator = nil;

    YUP_DECLARE_NON_COPYABLE(Pimpl)
};

DirectoryIterator::NativeIterator::NativeIterator(const File& directory, const String& wildcard)
    : pimpl(new DirectoryIterator::NativeIterator::Pimpl(directory, wildcard))
{
}

DirectoryIterator::NativeIterator::~NativeIterator()
{
}

bool DirectoryIterator::NativeIterator::next(String& filenameFound,
                                             bool* const isDir,
                                             bool* const isHidden,
                                             int64* const fileSize,
                                             Time* const modTime,
                                             Time* const creationTime,
                                             bool* const isReadOnly)
{
    return pimpl->next(filenameFound, isDir, isHidden, fileSize, modTime, creationTime, isReadOnly);
}

//==============================================================================
bool YUP_CALLTYPE Process::openDocument(const String& fileName, const String& parameters)
{
    return MacFileHelpers::openDocument(fileName, parameters);
}

bool YUP_CALLTYPE Process::openDocument(const String& fileName, const String& parameters, const StringPairArray& environment)
{
    StringArray envValues;

    for (const auto& key : environment.getAllKeys())
        envValues.add(key + "=" + environment.getValue(key, {}));

    Array<char*> env;

    for (auto& value : envValues)
        if (value.isNotEmpty())
            env.add(const_cast<char*>(value.toRawUTF8()));

    env.add(nullptr);

    return MacFileHelpers::openDocument(fileName, parameters, &env);
}

void File::revealToUser() const
{
#if !YUP_IOS
    if (exists())
        [[NSWorkspace sharedWorkspace] selectFile:yupStringToNS(getFullPathName()) inFileViewerRootedAtPath:nsEmptyString()];
    else if (getParentDirectory().exists())
        getParentDirectory().revealToUser();
#endif
}

//==============================================================================
OSType File::getMacOSType() const
{
    YUP_AUTORELEASEPOOL
    {
        NSDictionary* fileDict = [[NSFileManager defaultManager] attributesOfItemAtPath:yupStringToNS(getFullPathName()) error:nil];
        return [fileDict fileHFSTypeCode];
    }
}

bool File::isBundle() const
{
#if YUP_IOS
    return false; // xxx can't find a sensible way to do this without trying to open the bundle..
#else
    YUP_AUTORELEASEPOOL
    {
        return [[NSWorkspace sharedWorkspace] isFilePackageAtPath:yupStringToNS(getFullPathName())];
    }
#endif
}

#if YUP_MAC
void File::addToDock() const
{
    // check that it's not already there...
    if (!yup_getOutputFromCommand("defaults read com.apple.dock persistent-apps").containsIgnoreCase(getFullPathName()))
    {
        yup_runSystemCommand("defaults write com.apple.dock persistent-apps -array-add \"<dict><key>tile-data</key><dict><key>file-data</key><dict><key>_CFURLString</key><string>" + getFullPathName() + "</string><key>_CFURLStringType</key><integer>0</integer></dict></dict></dict>\"");

        yup_runSystemCommand("osascript -e \"tell application \\\"Dock\\\" to quit\"");
    }
}
#endif

File File::getContainerForSecurityApplicationGroupIdentifier(const String& appGroup)
{
    if (@available(macOS 10.8, *))
        if (auto* url = [[NSFileManager defaultManager] containerURLForSecurityApplicationGroupIdentifier:yupStringToNS(appGroup)])
            return File(nsStringToYup([url path]));

    return File();
}

} // namespace yup
