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

#if YUP_MAC

/** The source an AppKit drag session reports its operations to.

    Stateless, and shared: it only implements the one method AppKit requires, and the session retains
    it for its duration. The operation the destination settles on arrives in the optional "ended"
    callback, which is not wired up yet. */
@interface YUPDraggingSource : NSObject <NSDraggingSource>
@end

@implementation YUPDraggingSource

- (NSDragOperation) draggingSession: (NSDraggingSession*) __unused session
    sourceOperationMaskForDraggingContext: (NSDraggingContext) __unused context
{
    // Offer all three so the destination can choose for itself.
    return NSDragOperationCopy | NSDragOperationMove | NSDragOperationLink;
}

@end

namespace yup
{

//==============================================================================
namespace
{

//==============================================================================

NSString* toNSString (const String& text)
{
    return [NSString stringWithUTF8String: text.toRawUTF8()];
}

/** The drag image: a badge carrying the item count.

    AppKit would normally snapshot the view the drag started from, but our components are drawn by
    our own renderer rather than by that view, so a snapshot would show the whole window. */
NSImage* makeDragImage (int itemCount, CGFloat size)
{
    return [NSImage imageWithSize: NSMakeSize (size, size)
                          flipped: NO
                   drawingHandler: ^BOOL (NSRect dstRect)
    {
        [[NSColor colorWithCalibratedRed: 0.25f
                                   green: 0.43f
                                    blue: 0.62f
                                   alpha: 0.85f] setFill];

        [[NSBezierPath bezierPathWithOvalInRect: dstRect] fill];

        NSMutableParagraphStyle* style = [[NSMutableParagraphStyle alloc] init];
        [style setAlignment: NSTextAlignmentCenter];

        NSDictionary* attributes = @{ NSFontAttributeName : [NSFont systemFontOfSize: 20.0f],
                                      NSForegroundColorAttributeName : [NSColor whiteColor],
                                      NSParagraphStyleAttributeName : style };

        [[NSString stringWithFormat: @"%d", itemCount] drawInRect: dstRect withAttributes: attributes];

        return YES;
    }];
}

/** Adds one dragging item per payload entry.

    An NSPasteboardItem carries a single value per type, so several files have to become several
    items - which is also what makes the destination see them as distinct files. */
void addDragItem (NSMutableArray<NSDraggingItem*>* items, NSPasteboardItem* pasteboardItem, NSView* view, NSPoint origin, CGFloat size)
{
    auto* dragItem = [[NSDraggingItem alloc] initWithPasteboardWriter: pasteboardItem];

    [dragItem setDraggingFrame: NSMakeRect (origin.x - size * 0.5, origin.y - size * 0.5, size, size)
                      contents: makeDragImage (1, size)];

    [items addObject: dragItem];
}

} // namespace

//==============================================================================

std::optional<DragAndDropAction> performNativeDrag (Component& sourceComponent, const DragAndDropData& data)
{
    auto* native = sourceComponent.getNativeComponent();

    if (native == nullptr)
        return std::nullopt;

    NSWindow* window = (__bridge NSWindow*) native->getNativeHandle();

    if (window == nil)
        return std::nullopt;

    NSView* view = [window contentView];

    if (view == nil)
        return std::nullopt;

    // AppKit starts a session from the event that began the drag, and by the time the gesture has left
    // our windows that event is no longer being dispatched. currentEvent is what the application is
    // processing, which during a mouse drag is the drag event itself. With nothing to start from, the
    // drag stays in app.
    NSEvent* event = [NSApp currentEvent];

    if (event == nil)
        return std::nullopt;

    constexpr CGFloat imageSize = 52.0f;

    // mouseLocation is in screen coordinates; the dragging frame is in the view's.
    const auto windowPoint = [window convertPointFromScreen: [NSEvent mouseLocation]];
    const auto origin = [view convertPoint: windowPoint fromView: nil];

    NSMutableArray<NSDraggingItem*>* items = [NSMutableArray array];

    for (const auto& file : data.getFiles())
    {
        // What goes on the pasteboard is a reference to the file, not its contents: whoever accepts
        // the drop does the copying.
        NSURL* url = [NSURL fileURLWithPath: toNSString (file.getFullPathName())];

        auto* pasteboardItem = [[NSPasteboardItem alloc] init];
        [pasteboardItem setString: [url absoluteString] forType: NSPasteboardTypeFileURL];

        addDragItem (items, pasteboardItem, view, origin, imageSize);
    }

    const auto text = data.hasText() ? data.getText()
                                     : data.hasUris() ? data.getUris().joinIntoString ("\n")
                                                      : String();

    // An image payload is already PNG encoded by the payload itself, so it goes on as it is. It also
    // makes the better drag image: exactly what is being dragged, rather than a badge. It shares one
    // pasteboard item with the text, so a destination that only wants text still sees the name.
    const auto png = data.getMimeData (DragAndDropData::mimeTypePng);

    NSData* pngData = png.getSize() > 0 ? [NSData dataWithBytes: png.getData() length: (NSUInteger) png.getSize()] : nil;
    NSImage* pngImage = pngData != nil ? [[NSImage alloc] initWithData: pngData] : nil;

    if (text.isNotEmpty() || pngData != nil)
    {
        auto* pasteboardItem = [[NSPasteboardItem alloc] init];

        if (pngData != nil)
        {
            [pasteboardItem setData: pngData forType: NSPasteboardTypePNG];

            NSData* tiffData = [pngImage TIFFRepresentation];

            if (tiffData != nil)
                [pasteboardItem setData: tiffData forType: NSPasteboardTypeTIFF];
        }

        if (text.isNotEmpty())
            [pasteboardItem setString: toNSString (text) forType: NSPasteboardTypeString];

        auto* dragItem = [[NSDraggingItem alloc] initWithPasteboardWriter: pasteboardItem];

        if (pngImage != nil)
        {
            // Sized to the image instead of a fixed frame, so the drag keeps its aspect ratio.
            const auto payloadSize = [pngImage size];

            [dragItem setDraggingFrame: NSMakeRect (origin.x - payloadSize.width * 0.5,
                                                    origin.y - payloadSize.height * 0.5,
                                                    payloadSize.width, payloadSize.height)
                              contents: pngImage];
        }
        else
        {
            [dragItem setDraggingFrame: NSMakeRect (origin.x - imageSize * 0.5, origin.y - imageSize * 0.5, imageSize, imageSize)
                              contents: makeDragImage (1, imageSize)];
        }

        // The payload leads the file items: it is the most descriptive thing being dragged.
        [items insertObject: dragItem atIndex: 0];
    }

    // Nothing AppKit can carry, so the drag stays in app, where the ghost still shows it.
    if ([items count] == 0)
        return std::nullopt;

    static YUPDraggingSource* dragSource = [[YUPDraggingSource alloc] init];

    [view beginDraggingSessionWithItems: items event: event source: dragSource];

    // AppKit runs the session inside its own event handling, so as far as the in app session is
    // concerned the gesture is finished: the manager drops it without telling the source the drag
    // ended. Reporting which operation the destination chose needs the session's "ended" callback,
    // and is the next step for this platform.
    return DragAndDropAction::none;
}

} // namespace yup

#endif // YUP_MAC
