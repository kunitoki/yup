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

//==============================================================================
/** Shares file descriptors registered by hosted plugins with the Linux event loop.

    Plugin modules often share one connection (and so one descriptor) across all their
    instances, while LinuxEventLoop keeps a single callback per descriptor. This keeps
    every subscriber alive and only unregisters a descriptor when its last subscriber
    leaves. Message thread only.
*/
class PluginFdDispatcher
{
public:
    using Callback = std::function<void (int)>;

    /** Subscribes to a descriptor, returning a token to pass to remove(). */
    static int add (int fd, Callback callback, short eventMask)
    {
        auto& entry = getEntries()[fd];
        const auto token = ++getNextToken();
        const auto previousMask = entry.eventMask;

        entry.eventMask = static_cast<short> (entry.eventMask | eventMask);
        entry.subscribers.emplace_back (token, std::move (callback));

        if (entry.subscribers.size() == 1 || entry.eventMask != previousMask)
        {
            if (entry.subscribers.size() > 1)
                LinuxEventLoop::unregisterFdCallback (fd);

            LinuxEventLoop::registerFdCallback (fd, [fd] (int readyFd) { dispatch (fd, readyFd); }, entry.eventMask);
        }

        return token;
    }

    /** Removes a subscription created by add(). */
    static void remove (int fd, int token)
    {
        auto& entries = getEntries();
        const auto it = entries.find (fd);
        if (it == entries.end())
            return;

        auto& subscribers = it->second.subscribers;
        subscribers.erase (std::remove_if (subscribers.begin(), subscribers.end(), [token] (const auto& s) { return s.first == token; }), subscribers.end());

        if (! subscribers.empty())
            return;

        entries.erase (it);
        LinuxEventLoop::unregisterFdCallback (fd);
    }

private:
    struct Entry
    {
        short eventMask = 0;
        std::vector<std::pair<int, Callback>> subscribers;
    };

    static std::map<int, Entry>& getEntries()
    {
        static std::map<int, Entry> entries;
        return entries;
    }

    static int& getNextToken()
    {
        static int nextToken = 0;
        return nextToken;
    }

    static void dispatch (int fd, int readyFd)
    {
        // Callbacks may add or remove subscriptions, so look each one up again before calling it
        std::vector<int> tokens;

        if (const auto it = getEntries().find (fd); it != getEntries().end())
            for (const auto& subscriber : it->second.subscribers)
                tokens.push_back (subscriber.first);

        for (const auto token : tokens)
        {
            const auto it = getEntries().find (fd);
            if (it == getEntries().end())
                return;

            for (const auto& [subscriberToken, callback] : it->second.subscribers)
            {
                if (subscriberToken != token)
                    continue;

                const auto callbackCopy = callback;
                callbackCopy (readyFd);
                break;
            }
        }
    }
};

} // namespace yup
