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

static NSArray<UTType*>* createUTTypes (const String& filters)
{
    if (filters.isEmpty())
        return @[UTTypeItem];

    NSMutableArray<UTType*>* types = [NSMutableArray array];
    StringArray extensions = StringArray::fromTokens (filters, ";,", String());

    for (const auto& ext : extensions)
    {
        String extension = ext.trim();
        if (extension.startsWith ("*."))
            extension = extension.substring (2);
        else if (extension.startsWith ("*"))
            extension = extension.substring (1);
        else if (extension.startsWith ("."))
            extension = extension.substring (1);

        if (extension.isNotEmpty())
        {
            NSString* nsExt = [NSString stringWithUTF8String: extension.toUTF8()];
            UTType* type = [UTType typeWithFilenameExtension: nsExt];

            if (type != nil)
                [types addObject: type];
        }
    }

    if (types.count == 0)
        [types addObject: UTTypeItem];

    return [NSArray arrayWithArray: types];
}

} // namespace yup

@interface YUPFileChooserDelegate : NSObject <UIDocumentPickerDelegate>
@property (nonatomic) yup::FileChooser* fileChooser;
@property (nonatomic) yup::Array<yup::File>* results;
@property (nonatomic) bool completed;
@end

@implementation YUPFileChooserDelegate

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls
{
    if (self.results != nullptr)
    {
        for (NSURL* url in urls)
        {
            NSString* path = [url path];
            if (path != nil)
            {
                self.results->add (yup::File (yup::String::fromUTF8 ([path UTF8String])));
            }
        }
    }

    self.completed = true;
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller
{
    self.completed = true;
}

@end

namespace yup
{

void FileChooser::showPlatformDialog (int flags, Component* previewComponent)
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
                if (windowScene.activationState == UISceneActivationStateForegroundActive)
                {
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
        }
        else
        {
            keyWindow = [UIApplication sharedApplication].keyWindow;
        }

        rootViewController = keyWindow.rootViewController;

        if (rootViewController == nil)
            return;

        UIDocumentPickerViewController* documentPicker = nil;

        if (isSave)
        {
            // For save operations, we'll use the export mode
            NSURL* tempURL = nil;

            if (startingFile.exists() && startingFile.existsAsFile())
            {
                tempURL = [NSURL fileURLWithPath: [NSString stringWithUTF8String: startingFile.getFullPathName().toUTF8()]];
            }
            else
            {
                // Create a temporary file for export
                NSString* tempDir = NSTemporaryDirectory();
                NSString* fileName = startingFile.getFileName().isEmpty() ? @"untitled" : [NSString stringWithUTF8String: startingFile.getFileName().toUTF8()];
                NSString* tempPath = [tempDir stringByAppendingPathComponent: fileName];
                tempURL = [NSURL fileURLWithPath: tempPath];

                // Create empty file
                [@"" writeToURL: tempURL atomically: YES encoding: NSUTF8StringEncoding error: nil];
            }

            if (tempURL != nil)
            {
                documentPicker = [[UIDocumentPickerViewController alloc] initForExportingURLs: @[tempURL]];
            }
        }
        else
        {
            NSArray<UTType*>* allowedTypes;

            if (canChooseDirectories && !canChooseFiles)
            {
                allowedTypes = @[UTTypeFolder];
            }
            else
            {
                allowedTypes = createUTTypes (filters);
            }

            documentPicker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes: allowedTypes];
            documentPicker.allowsMultipleSelection = allowsMultiple;
        }

        if (documentPicker == nil)
            return;

        YUPFileChooserDelegate* delegate = [[YUPFileChooserDelegate alloc] init];
        delegate.fileChooser = this;
        delegate.results = &results;
        delegate.completed = false;

        documentPicker.delegate = delegate;
        documentPicker.modalPresentationStyle = UIModalPresentationPageSheet;

        [rootViewController presentViewController: documentPicker animated: YES completion: nil];

        // Wait for completion - this is a simplified approach
        // In a real application, you might want to use a more sophisticated callback mechanism
        NSRunLoop* runLoop = [NSRunLoop currentRunLoop];
        while (!delegate.completed && [runLoop runMode: NSDefaultRunLoopMode beforeDate: [NSDate dateWithTimeIntervalSinceNow: 0.1]])
        {
            // Keep the run loop alive while waiting
        }
    }
}

} // namespace yup
