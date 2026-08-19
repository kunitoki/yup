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
struct CancelToken::State
{
    std::atomic<bool> cancelled { false };

    CriticalSection callbacksLock;
    std::vector<std::pair<uint64, std::function<void()>>> callbacks;
    uint64 nextCallbackId = 0;

    WaitableEvent cancelEvent { true };

    void requestCancellation() noexcept
    {
        const bool wasAlreadyCancelled = cancelled.exchange (true);
        cancelEvent.signal();

        if (! wasAlreadyCancelled)
        {
            std::vector<std::function<void()>> toInvoke;

            {
                const ScopedLock sl (callbacksLock);

                toInvoke.reserve (callbacks.size());

                for (auto& callback : callbacks)
                    toInvoke.push_back (std::move (callback.second));

                callbacks.clear();
            }

            for (auto& callback : toInvoke)
            {
                try
                {
                    callback();
                }
                catch (...)
                {
                    // Your callbacks must not throw exceptions!
                    jassertfalse;
                }
            }
        }
    }
};

//==============================================================================
CancelToken::~CancelToken() = default;

CancelToken& CancelToken::operator= (const CancelToken&) = default;

CancelToken& CancelToken::operator= (CancelToken&&) noexcept = default;

//==============================================================================
CancelToken::Registration::~Registration()
{
    unregister();
}

CancelToken::Registration& CancelToken::Registration::operator= (Registration&& other) noexcept
{
    if (this != &other)
    {
        unregister();

        state = std::move (other.state);
        callbackId = std::exchange (other.callbackId, 0);
    }

    return *this;
}

CancelToken::Registration::Registration (Registration&& other) noexcept
    : state (std::move (other.state))
    , callbackId (std::exchange (other.callbackId, 0))
{
}

CancelToken::Registration::Registration (std::shared_ptr<State> stateToHold, uint64 callbackIdToHold) noexcept
    : state (std::move (stateToHold))
    , callbackId (callbackIdToHold)
{
}

void CancelToken::Registration::unregister() noexcept
{
    if (auto s = state)
    {
        const ScopedLock sl (s->callbacksLock);

        for (auto it = s->callbacks.begin(); it != s->callbacks.end(); ++it)
        {
            if (it->first == callbackId)
            {
                s->callbacks.erase (it);
                break;
            }
        }
    }

    state.reset();
    callbackId = 0;
}

bool CancelToken::Registration::isValid() const noexcept
{
    return state != nullptr;
}

//==============================================================================
CancelToken::CancelToken (std::shared_ptr<State> stateToHold) noexcept
    : state (std::move (stateToHold))
{
}

bool CancelToken::wasCancelled() const noexcept
{
    auto s = state;
    return s != nullptr && s->cancelled.load();
}

bool CancelToken::isCancellable() const noexcept
{
    return state != nullptr;
}

CancelToken CancelToken::none()
{
    return CancelToken (nullptr);
}

bool CancelToken::waitForCancellation (int timeOutMilliseconds) const
{
    auto s = state;
    return s != nullptr && s->cancelEvent.wait (timeOutMilliseconds);
}

CancelToken::Registration CancelToken::registerCallback (std::function<void()> callback)
{
    auto s = state;

    if (s == nullptr)
        return Registration {};

    {
        const ScopedLock sl (s->callbacksLock);

        if (! s->cancelled.load())
        {
            const auto id = s->nextCallbackId++;
            s->callbacks.emplace_back (id, std::move (callback));

            return Registration (std::move (s), id);
        }
    }

    try
    {
        callback();
    }
    catch (...)
    {
        // Your callbacks must not throw exceptions!
        jassertfalse;
    }

    return Registration {};
}

bool CancelToken::operator== (const CancelToken& other) const noexcept
{
    return state == other.state;
}

bool CancelToken::operator!= (const CancelToken& other) const noexcept
{
    return state != other.state;
}

} // namespace yup
