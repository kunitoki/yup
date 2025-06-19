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

// For iOS 13.0 compatibility, we use string-based UTI identifiers
static NSArray<NSString*>* createDocumentTypes(const String& filters)
{
    if (filters.isEmpty())
        return @[ @"public.item" ];

    NSMutableArray<NSString*>* types = [NSMutableArray array];
    StringArray extensions = StringArray::fromTokens(filters, ";,", String());

    for (const auto& ext : extensions)
    {
        String extension = ext.trim();
        if (extension.startsWith("*."))
            extension = extension.substring(2);
        else if (extension.startsWith("*"))
            extension = extension.substring(1);
        else if (extension.startsWith("."))
            extension = extension.substring(1);

        if (extension.isNotEmpty())
        {
            NSString* nsExt = [NSString stringWithUTF8String:extension.toUTF8()];

            // Map common extensions to UTI identifiers
            NSString* uti = nil;
            if ([nsExt isEqualToString:@"txt"])
                uti = @"public.plain-text";
            else if ([nsExt isEqualToString:@"pdf"])
                uti = @"com.adobe.pdf";
            else if ([nsExt isEqualToString:@"png"])
                uti = @"public.png";
            else if ([nsExt isEqualToString:@"jpg"] || [nsExt isEqualToString:@"jpeg"])
                uti = @"public.jpeg";
            else if ([nsExt isEqualToString:@"mp3"])
                uti = @"public.mp3";
            else if ([nsExt isEqualToString:@"mp4"])
                uti = @"public.mpeg-4";
            else if ([nsExt isEqualToString:@"zip"])
                uti = @"public.zip-archive";
            else if ([nsExt isEqualToString:@"json"])
                uti = @"public.json";
            else if ([nsExt isEqualToString:@"xml"])
                uti = @"public.xml";
            else
            {
                // For unknown extensions, try to create a UTI
                uti = [NSString stringWithFormat:@"public.%@", nsExt];
            }

            if (uti != nil)
                [types addObject:uti];
        }
    }

    if (types.count == 0)
        [types addObject:@"public.item"];

    return [NSArray arrayWithArray:types];
}

// iOS 14.0+ version using UTType
static NSArray* createUTTypes(const String& filters) API_AVAILABLE(ios(14.0))
{
    if (@available(iOS 14.0, *))
    {
        if (filters.isEmpty())
            return @[ UTTypeItem ];

        NSMutableArray* types = [NSMutableArray array];
        StringArray extensions = StringArray::fromTokens(filters, ";,", String());

        for (const auto& ext : extensions)
        {
            String extension = ext.trim();
            if (extension.startsWith("*."))
                extension = extension.substring(2);
            else if (extension.startsWith("*"))
                extension = extension.substring(1);
            else if (extension.startsWith("."))
                extension = extension.substring(1);

            if (extension.isNotEmpty())
            {
                NSString* nsExt = [NSString stringWithUTF8String:extension.toUTF8()];
                UTType* type = [UTType typeWithFilenameExtension:nsExt];

                if (type != nil)
                    [types addObject:type];
            }
        }

        if (types.count == 0)
            [types addObject:UTTypeItem];

        return [NSArray arrayWithArray:types];
    }
    return nil;
}

} // namespace yup

@interface YUPFileChooserDelegate : NSObject <UIDocumentPickerDelegate>
@property (nonatomic) yup::FileChooser* fileChooser;
@property (nonatomic) yup::Array<yup::File>* results;
@property (nonatomic) bool completed;
@end

@implementation YUPFileChooserDelegate

- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls
{
    if (self.results != nullptr)
    {
        for (NSURL* url in urls)
        {
            NSString* path = [url path];
            if (path != nil)
                self.results->add(yup::File(yup::String::fromUTF8([path UTF8String])));
        }
    }

    self.completed = true;
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)controller
{
    self.completed = true;
}

@end

namespace yup
{

void FileChooser::showPlatformDialog(int flags, Component* previewComponent)
{
    YUP_AUTORELEASEPOOL
    {
        const bool isSave = (flags & saveMode) != 0;
        const bool canChooseFiles = (flags & canSelectFiles) != 0;
        const bool canChooseDirectories = (flags & canSelectDirectories) != 0;
        const bool allowsMultiple = (flags & canSelectMultipleItems) != 0;

        UIViewController* rootViewController = nil;

        // Find the root view controller
        UIWindow* keyWindow = nil;
        if (@available(iOS 13.0, *))
        {
            for (UIWindowScene* windowScene in [UIApplication sharedApplication].connectedScenes)
            {
                if (windowScene.activationState != UISceneActivationStateForegroundActive)
                    continue;

                for (UIWindow* window in windowScene.windows)
                {
                    if (window.isKeyWindow)
                    {
                        keyWindow = window;
                        break;
                    }
                }

                if (keyWindow != nil)
                    break;
            }
        }

        if (keyWindow == nil)
        {
            YUP_BEGIN_IGNORE_DEPRECATION_WARNINGS
            keyWindow = [UIApplication sharedApplication].keyWindow;
            YUP_END_IGNORE_DEPRECATION_WARNINGS
        }

        rootViewController = keyWindow.rootViewController;
        if (rootViewController == nil)
            return;

        UIDocumentPickerViewController* documentPicker = nil;

        if (isSave)
        {
            // For save operations on iOS 14.0+
            if (@available(iOS 14.0, *))
            {
                NSURL* tempURL = nil;

                if (startingFile.exists() && startingFile.existsAsFile())
                {
                    tempURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:startingFile.getFullPathName().toUTF8()]];
                }
                else
                {
                    // Create a temporary file for export
                    NSString* tempDir = NSTemporaryDirectory();
                    NSString* fileName = startingFile.getFileName().isEmpty() ? @"untitled" : [NSString stringWithUTF8String:startingFile.getFileName().toUTF8()];
                    NSString* tempPath = [tempDir stringByAppendingPathComponent:fileName];
                    tempURL = [NSURL fileURLWithPath:tempPath];

                    // Create empty file
                    [@"" writeToURL:tempURL atomically:YES encoding:NSUTF8StringEncoding error:nil];
                }

                if (tempURL != nil)
                    documentPicker = [[UIDocumentPickerViewController alloc] initForExportingURLs:@[ tempURL ]];
            }
            else
            {
                // For iOS 13.0, save mode is not directly supported with document picker
                // We'll fall back to the open mode
                NSArray<NSString*>* documentTypes = createDocumentTypes(filters);

                YUP_BEGIN_IGNORE_DEPRECATION_WARNINGS
                documentPicker = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:documentTypes inMode:UIDocumentPickerModeOpen];
                YUP_END_IGNORE_DEPRECATION_WARNINGS
            }
        }
        else
        {
            if (@available(iOS 14.0, *))
            {
                // iOS 14.0+ using UTType
                NSArray* allowedTypes;

                if (canChooseDirectories && !canChooseFiles)
                    allowedTypes = @[ UTTypeFolder ];
                else
                    allowedTypes = createUTTypes(filters);

                documentPicker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:allowedTypes];
                documentPicker.allowsMultipleSelection = allowsMultiple;
            }
            else
            {
                // iOS 13.0 using string-based document types
                NSArray<NSString*>* documentTypes;

                if (canChooseDirectories && !canChooseFiles)
                    documentTypes = @[ @"public.folder" ];
                else
                    documentTypes = createDocumentTypes(filters);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
                documentPicker = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:documentTypes inMode:UIDocumentPickerModeOpen];
#pragma clang diagnostic pop

                if (@available(iOS 11.0, *))
                    documentPicker.allowsMultipleSelection = allowsMultiple;
            }
        }

        if (documentPicker == nil)
            return;

        YUPFileChooserDelegate* delegate = [[YUPFileChooserDelegate alloc] init];
        delegate.fileChooser = this;
        delegate.results = &results;
        delegate.completed = false;

        documentPicker.delegate = delegate;
        documentPicker.modalPresentationStyle = UIModalPresentationPageSheet;

        [rootViewController presentViewController:documentPicker animated:YES completion:nil];

        // Wait for completion - this is a simplified approach
        // In a real application, you might want to use a more sophisticated callback mechanism
        NSRunLoop* runLoop = [NSRunLoop currentRunLoop];
        while (!delegate.completed && [runLoop runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.1]])
        {
            // Keep the run loop alive while waiting
        }
    }
}

} // namespace yup
