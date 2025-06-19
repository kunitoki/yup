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

static Array<String> parseFileExtensions(const String& pattern)
{
    Array<String> extensions;

    if (pattern.isEmpty())
        return extensions;

    StringArray tokens = StringArray::fromTokens(pattern, ";,", String());

    for (const auto& token : tokens)
    {
        String ext = token.trim();
        if (ext.startsWith("*."))
            ext = ext.substring(2);
        else if (ext.startsWith("*"))
            ext = ext.substring(1);

        if (ext.isNotEmpty())
            extensions.add(ext);
    }

    return extensions;
}

static NSArray* createAllowedFileTypes(const String& filters)
{
    auto extensions = parseFileExtensions(filters);

    if (extensions.isEmpty())
        return nil;

    NSMutableArray* types = [NSMutableArray array];

    for (const auto& ext : extensions)
    {
        NSString* nsExt = [NSString stringWithUTF8String:ext.toUTF8()];
        if (nsExt != nil)
            [types addObject:nsExt];
    }

    return types.count > 0 ? [NSArray arrayWithArray:types] : nil;
}

void FileChooser::showPlatformDialog(CompletionCallback callback, int flags)
{
    YUP_AUTORELEASEPOOL
    {
        const bool isSave = (flags & saveMode) != 0;
        const bool canChooseFiles = (flags & canSelectFiles) != 0;
        const bool canChooseDirectories = (flags & canSelectDirectories) != 0;
        const bool allowsMultipleSelection = (flags & canSelectMultipleItems) != 0;
        const bool warnAboutOverwrite = (flags & warnAboutOverwriting) != 0;

        if (isSave)
        {
            NSSavePanel* panel = [NSSavePanel savePanel];

            [panel setTitle:[NSString stringWithUTF8String:title.toUTF8()]];
            [panel setCanCreateDirectories:YES];
            [panel setShowsHiddenFiles:NO];

            if (warnAboutOverwrite)
                [panel setExtensionHidden:NO];

            NSArray* allowedTypes = createAllowedFileTypes(filters);
            if (allowedTypes != nil)
            {
                [panel setAllowedFileTypes:allowedTypes];
                [panel setAllowsOtherFileTypes:NO];
            }

            if (startingFile.exists())
            {
                if (startingFile.isDirectory())
                {
                    [panel setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:startingFile.getFullPathName().toUTF8()]]];
                }
                else
                {
                    [panel setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:startingFile.getParentDirectory().getFullPathName().toUTF8()]]];
                    [panel setNameFieldStringValue:[NSString stringWithUTF8String:startingFile.getFileName().toUTF8()]];
                }
            }

            NSModalResponse result = [panel runModal]; // - (void) beginWithCompletionHandler:(void (^)(NSModalResponse result)) handler;

            [panel beginWithCompletionHandler:^(NSModalResponse result)
            {
                Array<File> results;

                if (result == NSModalResponseOK)
                {
                    NSURL* url = [panel URL];
                    if (url != nil)
                    {
                        NSString* path = [url path];
                        if (path != nil)
                            results.add(File(String::fromUTF8([path UTF8String])));
                    }
                }

                MessageManager::callAsync([this, callback = std::move(callback), result, results]
                {
                    invokeCallback(std::move(callback), result == NSModalResponseOK, results);
                });
            }];
        }
        else
        {
            NSOpenPanel* panel = [NSOpenPanel openPanel];

            [panel setTitle:[NSString stringWithUTF8String:title.toUTF8()]];
            [panel setCanChooseFiles:canChooseFiles];
            [panel setCanChooseDirectories:canChooseDirectories];
            [panel setAllowsMultipleSelection:allowsMultipleSelection];
            [panel setShowsHiddenFiles:NO];
            [panel setTreatsFilePackagesAsDirectories:packageDirsAsFiles];

            NSArray* allowedTypes = createAllowedFileTypes(filters);
            if (allowedTypes != nil && canChooseFiles)
            {
                [panel setAllowedFileTypes:allowedTypes];
                [panel setAllowsOtherFileTypes:NO];
            }

            if (startingFile.exists())
                [panel setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:startingFile.getFullPathName().toUTF8()]]];


            [panel beginWithCompletionHandler:^(NSModalResponse result)
            {
                Array<File> results;

                if (result == NSModalResponseOK)
                {
                    NSArray* urls = [panel URLs];

                    for (NSURL* url in urls)
                    {
                        NSString* path = [url path];
                        if (path != nil)
                            results.add(File(String::fromUTF8([path UTF8String])));
                    }
                }

                MessageManager::callAsync([this, callback = std::move(callback), result, results]
                {
                    invokeCallback(std::move(callback), result == NSModalResponseOK, results);
                });
            }];
        }
    }
}

} // namespace yup
