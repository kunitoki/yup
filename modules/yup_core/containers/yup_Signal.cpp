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

namespace
{
std::mutex& getSignalDispatcherLock()
{
    static std::mutex lock;
    return lock;
}

SignalDispatcher::Function& getSignalDispatcher()
{
    static SignalDispatcher::Function dispatcher;
    return dispatcher;
}
} // namespace

SignalDispatcher::Function SignalDispatcher::setAsyncDispatcher (Function newDispatcher)
{
    const std::scoped_lock lock (getSignalDispatcherLock());
    return std::exchange (getSignalDispatcher(), std::move (newDispatcher));
}

bool SignalDispatcher::dispatchAsync (std::function<void()> function)
{
    Function dispatcher;

    {
        const std::scoped_lock lock (getSignalDispatcherLock());
        dispatcher = getSignalDispatcher();
    }

    return dispatcher != nullptr && dispatcher (std::move (function));
}

//==============================================================================

SignalConnection::SignalConnection (std::weak_ptr<Owner> signalOwner, uint64 callbackId) noexcept
    : owner (std::move (signalOwner))
    , id (callbackId)
{
}

SignalConnection::~SignalConnection()
{
    disconnect();
}

SignalConnection::SignalConnection (SignalConnection&& other) noexcept
    : owner (std::move (other.owner))
    , id (std::exchange (other.id, 0))
{
}

SignalConnection& SignalConnection::operator= (SignalConnection&& other) noexcept
{
    if (this != &other)
    {
        disconnect();
        owner = std::move (other.owner);
        id = std::exchange (other.id, 0);
    }

    return *this;
}

void SignalConnection::disconnect() noexcept
{
    if (auto signal = owner.lock())
        signal->disconnect (id);

    owner.reset();
    id = 0;
}

bool SignalConnection::isConnected() const noexcept
{
    const auto signal = owner.lock();
    return signal != nullptr && signal->isConnected (id);
}

} // namespace yup
