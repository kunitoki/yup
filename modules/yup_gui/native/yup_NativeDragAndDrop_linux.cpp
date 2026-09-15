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

#if YUP_LINUX

#include <X11/Xlib.h>
#include <X11/Xatom.h>

namespace yup
{

//==============================================================================
namespace
{

/** How often the pointer is polled while the drag runs, and how long the destination is given to ask
    for the data once a drop has been offered. */
constexpr int positionIntervalMs = 15;
constexpr int finishTimeoutMs = 2000;

/** The display connection and source window one drag owns, closed however the drag exits.

    A connection of our own is not a choice, which is worth stating plainly: a drag has to see the
    replies the destination sends back to the source window, and SDL's own connection consumes the
    client messages it does not understand, so those replies would never reach us. Owning a small
    unmapped window on a private connection keeps the whole conversation to ourselves.
*/
class XdndConnection
{
public:
    XdndConnection()
        : display (XOpenDisplay (nullptr))
    {
        if (display != nullptr)
            sourceWindow = XCreateSimpleWindow (display, DefaultRootWindow (display), 0, 0, 1, 1, 0, 0, 0);

        if (sourceWindow != 0)
            XSelectInput (display, sourceWindow, StructureNotifyMask);
    }

    ~XdndConnection()
    {
        if (display == nullptr)
            return;

        if (sourceWindow != 0)
            XDestroyWindow (display, sourceWindow);

        XCloseDisplay (display);
    }

    bool isValid() const { return display != nullptr && sourceWindow != 0; }

    Display* getDisplay() const { return display; }

    Window getSourceWindow() const { return sourceWindow; }

private:
    Display* display = nullptr;
    Window sourceWindow = 0;
};

//==============================================================================

/** The X11 names an XDND conversation is made of.

    The protocol calls its messages by name, so each one is an atom, and the payload formats are named
    as MIME types - the same ones the receiving side of SDL hands us.

    Note that `None` is undefined in this translation unit, because the X11 macro collides with
    ordinary identifiers elsewhere in YUP. X's "no value" is therefore written as 0 throughout.
*/
struct XdndAtoms
{
    bool internAll (Display* display)
    {
        selection = XInternAtom (display, "XdndSelection", False);
        aware = XInternAtom (display, "XdndAware", False);
        enter = XInternAtom (display, "XdndEnter", False);
        position = XInternAtom (display, "XdndPosition", False);
        status = XInternAtom (display, "XdndStatus", False);
        leave = XInternAtom (display, "XdndLeave", False);
        drop = XInternAtom (display, "XdndDrop", False);
        finished = XInternAtom (display, "XdndFinished", False);
        typeList = XInternAtom (display, "XdndTypeList", False);
        actionCopy = XInternAtom (display, "XdndActionCopy", False);
        actionMove = XInternAtom (display, "XdndActionMove", False);
        actionLink = XInternAtom (display, "XdndActionLink", False);

        uriList = XInternAtom (display, "text/uri-list", False);
        text = XInternAtom (display, "text/plain", False);
        utf8 = XInternAtom (display, "text/plain;charset=utf-8", False);
        png = XInternAtom (display, "image/png", False);
        targets = XInternAtom (display, "TARGETS", False);

        return selection != 0 && aware != 0 && enter != 0 && position != 0 && status != 0
            && leave != 0 && drop != 0 && finished != 0;
    }

    Atom selection = 0, aware = 0, enter = 0, position = 0, status = 0, leave = 0, drop = 0, finished = 0;
    Atom typeList = 0, actionCopy = 0, actionMove = 0, actionLink = 0;
    Atom uriList = 0, text = 0, utf8 = 0, png = 0, targets = 0;
};

/** Maps one of the protocol's action atoms onto ours. */
DragAndDropAction toAction (Atom atom, const XdndAtoms& atoms)
{
    if (atom == atoms.actionCopy)
        return DragAndDropAction::copy;

    if (atom == atoms.actionMove)
        return DragAndDropAction::move;

    if (atom == atoms.actionLink)
        return DragAndDropAction::link;

    return DragAndDropAction::none;
}

//==============================================================================

/** Percent-encodes a path into a file URL, which is how XDND carries files.

    Encoding works on UTF-8 bytes rather than on characters, which is what the URI rules ask for and
    what keeps non-ASCII file names intact.
*/
String toFileUrl (const File& file)
{
    static constexpr const char* hexDigits = "0123456789ABCDEF";

    const auto path = file.getFullPathName().replaceCharacter ('\\', '/');
    const auto* utf8Bytes = path.toRawUTF8();

    String encoded;

    for (const auto* pointer = utf8Bytes; *pointer != 0; ++pointer)
    {
        const auto value = static_cast<unsigned int> (static_cast<unsigned char> (*pointer));

        // Everything a shell or a browser takes literally passes through; the rest, spaces included,
        // is escaped.
        const auto isLiteral = (value >= 'a' && value <= 'z')
                            || (value >= 'A' && value <= 'Z')
                            || (value >= '0' && value <= '9')
                            || *pointer == '/' || *pointer == '-' || *pointer == '_'
                            || *pointer == '.' || *pointer == '~';

        if (isLiteral)
        {
            encoded += *pointer;
        }
        else
        {
            const char escape[] = { '%', hexDigits[(value >> 4) & 0xf], hexDigits[value & 0xf], 0 };
            encoded += escape;
        }
    }

    return "file://" + encoded;
}

//==============================================================================

/** Finds the window under the pointer that is willing to accept a drop.

    XDND is driven by the source, so the source has to find its own destination: it walks the window
    tree down to whatever is under the pointer, then back up to the nearest ancestor that advertises
    the protocol.
*/
class DropDestination
{
public:
    DropDestination (Display* displayToUse, const XdndAtoms& atomsToUse)
        : display (displayToUse)
        , atoms (atomsToUse)
    {
    }

    /** Window 0 means the pointer is over nothing worth talking to. */
    Window find (int rootX, int rootY)
    {
        auto current = findDeepestWindow (rootX, rootY);

        while (current != 0)
        {
            if (const auto foundVersion = readAwareVersion (current))
            {
                version = foundVersion;
                return current;
            }

            Window rootReturn = 0, parentReturn = 0;
            Window* children = nullptr;
            unsigned int childCount = 0;

            if (XQueryTree (display, current, &rootReturn, &parentReturn, &children, &childCount) == 0)
                break;

            if (children != nullptr)
                XFree (children);

            current = parentReturn;
        }

        version = 0;
        return 0;
    }

    int getVersion() const { return version; }

private:
    Window findDeepestWindow (int rootX, int rootY)
    {
        auto current = DefaultRootWindow (display);

        while (true)
        {
            Window rootReturn = 0, parentReturn = 0, childReturn = 0;
            int rootXReturn = 0, rootYReturn = 0, windowX = 0, windowY = 0;
            unsigned int mask = 0;

            if (XQueryPointer (display, current, &rootReturn, &parentReturn,
                               &rootXReturn, &rootYReturn, &windowX, &windowY, &mask) == False)
                return 0;

            if (childReturn == 0)
                return current;

            current = childReturn;
        }
    }

    int readAwareVersion (Window candidate)
    {
        Atom actualType = 0;
        int actualFormat = 0;
        unsigned long count = 0, bytesAfter = 0;
        unsigned char* data = nullptr;

        // Only the version, which is the first item of the XdndAware property. Anything below 3
        // predates the parts of the protocol used here.
        if (XGetWindowProperty (display, candidate, atoms.aware, 0, 1, False, AnyPropertyType,
                                &actualType, &actualFormat, &count, &bytesAfter, &data) != Success)
            return 0;

        auto foundVersion = 0;

        if (actualType == XA_ATOM && actualFormat == 32 && count >= 1)
            foundVersion = static_cast<int> (reinterpret_cast<unsigned long*> (data)[0]);

        if (data != nullptr)
            XFree (data);

        return foundVersion >= 3 ? foundVersion : 0;
    }

    Display* display;
    const XdndAtoms& atoms;
    int version = 0;
};

} // namespace

//==============================================================================

bool performNativeDrag (Component&,
                        const DragAndDropData& data,
                        std::function<void (std::optional<DragAndDropAction>)> onComplete)
{
    // Hand-rolled rather than delegated to SDL, which has no drag source: its drop events cover the
    // receiving side only, and the serial Wayland would need in order to start a drag is not exposed.
    XdndConnection connection;

    if (! connection.isValid())
        return false;

    auto* display = connection.getDisplay();
    const auto sourceWindow = connection.getSourceWindow();

    XdndAtoms atoms;

    if (! atoms.internAll (display))
        return false;

    // The formats the destination is offered, in the order it should prefer them.
    Array<Atom> offeredTypes;

    if (! data.getFiles().isEmpty())
        offeredTypes.add (atoms.uriList);

    if (data.hasText() || data.hasUris())
    {
        offeredTypes.add (atoms.utf8);
        offeredTypes.add (atoms.text);
    }

    const auto pngPayload = data.getMimeData (DragAndDropData::mimeTypePng);

    if (pngPayload.getSize() > 0)
        offeredTypes.add (atoms.png);

    if (offeredTypes.isEmpty())
        return false;

    XSetSelectionOwner (display, atoms.selection, sourceWindow, CurrentTime);

    if (XGetSelectionOwner (display, atoms.selection) != sourceWindow)
        return false;

    // More than three formats do not fit in the XdndEnter message, so the whole list goes into a
    // property the destination reads from the source window.
    const auto needsTypeListProperty = offeredTypes.size() > 3;

    if (needsTypeListProperty)
    {
        Array<unsigned long> typeAtoms;

        for (const auto type : offeredTypes)
            typeAtoms.add (type);

        XChangeProperty (display, sourceWindow, atoms.typeList, XA_ATOM, 32, PropModeReplace,
                         reinterpret_cast<const unsigned char*> (typeAtoms.getData()),
                         typeAtoms.size());
    }

    //==============================================================================
    const auto sendXdndMessage = [&] (Window target, Atom type, long a, long b, long c, long d)
    {
        XEvent message {};
        message.xclient.type = ClientMessage;
        message.xclient.display = display;
        message.xclient.window = target;
        message.xclient.message_type = type;
        message.xclient.format = 32;
        message.xclient.data.l[0] = static_cast<long> (sourceWindow);
        message.xclient.data.l[1] = a;
        message.xclient.data.l[2] = b;
        message.xclient.data.l[3] = c;
        message.xclient.data.l[4] = d;

        XSendEvent (display, target, False, 0, &message);
        XFlush (display);
    };

    const auto offerTargets = [&] (Window target)
    {
        sendXdndMessage (target, atoms.enter,
                         static_cast<long> ((5 << 24) | (needsTypeListProperty ? 1 : 0)),
                         needsTypeListProperty ? 0 : offeredTypes[0],
                         (! needsTypeListProperty && offeredTypes.size() > 1) ? offeredTypes[1] : 0,
                         (! needsTypeListProperty && offeredTypes.size() > 2) ? offeredTypes[2] : 0);
    };

    // Answers the destination's request for the payload, which is the only point at which the data
    // crosses over.
    const auto serveSelectionRequest = [&] (const XSelectionRequestEvent& request)
    {
        auto property = request.property;

        if (property == 0)
            property = request.target; // obsolete clients ask for the target as the property

        const char* transfer = nullptr;
        unsigned long transferLength = 0;
        auto transferFormat = 8;
        auto transferType = request.target;
        MemoryBlock converted;
        Array<unsigned long> typeAtoms;

        if (request.target == atoms.targets)
        {
            transferFormat = 32;
            transferType = XA_ATOM;

            typeAtoms.add (atoms.targets);

            for (const auto type : offeredTypes)
                typeAtoms.add (type);

            transfer = reinterpret_cast<const char*> (typeAtoms.getData());
            transferLength = static_cast<unsigned long> (typeAtoms.size());
        }
        else if (request.target == atoms.uriList && ! data.getFiles().isEmpty())
        {
            StringArray list;

            for (const auto& file : data.getFiles())
                list.add (toFileUrl (file));

            const auto urls = list.joinIntoString ("\r\n") + "\r\n";

            converted.append (urls.toRawUTF8(), urls.getNumBytesAsUTF8());

            transfer = static_cast<const char*> (converted.getData());
            transferLength = converted.getSize();
        }
        else if (request.target == atoms.utf8 || request.target == atoms.text)
        {
            const auto text = data.hasText() ? data.getText() : data.getUris().joinIntoString ("\n");

            converted.append (text.toRawUTF8(), text.getNumBytesAsUTF8());

            transfer = static_cast<const char*> (converted.getData());
            transferLength = converted.getSize();
        }
        else if (request.target == atoms.png && pngPayload.getSize() > 0)
        {
            transfer = static_cast<const char*> (pngPayload.getData());
            transferLength = pngPayload.getSize();
        }

        if (transfer != nullptr && transferLength > 0)
            XChangeProperty (display, request.requestor, property, transferType, transferFormat,
                             PropModeReplace, reinterpret_cast<const unsigned char*> (transfer),
                             static_cast<int> (transferLength));

        XEvent notify {};
        notify.xselection.type = SelectionNotify;
        notify.xselection.display = display;
        notify.xselection.requestor = request.requestor;
        notify.xselection.selection = request.selection;
        notify.xselection.target = request.target;
        notify.xselection.property = transfer != nullptr ? property : 0;
        notify.xselection.time = request.time;

        XSendEvent (display, request.requestor, False, 0, &notify);
        XFlush (display);
    };

    auto accepted = false;
    auto actionReported = false;
    auto performed = DragAndDropAction::copy;

    const auto pumpPendingEvents = [&]
    {
        while (XPending (display) > 0)
        {
            XEvent event {};

            if (XNextEvent (display, &event) != 0)
                break;

            if (event.type == SelectionRequest)
            {
                serveSelectionRequest (event.xselectionrequest);
            }
            else if (event.type == ClientMessage && event.xclient.message_type == atoms.status)
            {
                // The status carries both the verdict on this position and the action the
                // destination would take, which is all we need unless it changes its mind at the end.
                accepted = (event.xclient.data.l[1] & 1) != 0;

                if (accepted)
                    performed = toAction (static_cast<Atom> (event.xclient.data.l[4]), atoms);
            }
            else if (event.type == ClientMessage && event.xclient.message_type == atoms.finished)
            {
                if ((event.xclient.data.l[1] & 1) != 0)
                    performed = toAction (static_cast<Atom> (event.xclient.data.l[2]), atoms);

                actionReported = true;
            }
        }
    };

    //==============================================================================
    DropDestination destination (display, atoms);
    Window lastWindow = 0;
    auto lastOfferedPosition = -1;

    // The drag loop. SDL's events go unpumped while this runs, which costs nothing here: an exported
    // drag has no in-app session and no ghost, so our own windows have nothing to redraw.
    while (true)
    {
        Window rootReturn = 0, childReturn = 0;
        int rootX = 0, rootY = 0, windowX = 0, windowY = 0;
        unsigned int mask = 0;

        if (XQueryPointer (display, DefaultRootWindow (display), &rootReturn, &childReturn,
                           &rootX, &rootY, &windowX, &windowY, &mask) == False)
            break;

        if ((mask & Button1Mask) == 0)
            break;

        pumpPendingEvents();

        const auto target = destination.find (rootX, rootY);

        if (target != lastWindow)
        {
            if (lastWindow != 0)
                sendXdndMessage (lastWindow, atoms.leave, 0, 0, 0, 0);

            if (target != 0)
                offerTargets (target);

            lastWindow = target;
            lastOfferedPosition = -1;
        }

        if (target != 0)
        {
            const auto packedPosition = ((rootX & 0xffff) << 16) | (rootY & 0xffff);

            if (packedPosition != lastOfferedPosition)
            {
                // Shift asks for a move, matching how the in-app side reads the same gesture.
                const auto wantsMove = (mask & ShiftMask) != 0;

                sendXdndMessage (target, atoms.position, 0, packedPosition, CurrentTime,
                                 wantsMove ? atoms.actionMove : atoms.actionCopy);

                lastOfferedPosition = packedPosition;
            }
        }

        Thread::sleep (positionIntervalMs);
    }

    // The button came up: an accepted offer becomes a drop, which is what makes the destination ask
    // for the data. Anything else is a leave.
    if (lastWindow != 0)
    {
        pumpPendingEvents();

        if (accepted)
        {
            sendXdndMessage (lastWindow, atoms.drop, 0, CurrentTime, 0, 0);

            const auto startedWaitingAt = Time::getMillisecondCounter();

            while (! actionReported && Time::getMillisecondCounter() - startedWaitingAt < finishTimeoutMs)
            {
                pumpPendingEvents();
                Thread::sleep (positionIntervalMs);
            }
        }
        else
        {
            sendXdndMessage (lastWindow, atoms.leave, 0, 0, 0, 0);
        }
    }

    // This implementation blocks, so the outcome is already known by the time we get here: there is no
    // separate thread to hand it over from. The message thread is the right one to report on either
    // way, and it is where we already are.
    onComplete (accepted ? performed : DragAndDropAction::none);

    return true;
}

} // namespace yup

#endif // YUP_LINUX
