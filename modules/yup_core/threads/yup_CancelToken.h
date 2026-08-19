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

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace yup
{

//==============================================================================
/**
    A copyable, thread-safe *observer* token used to observe the cancellation
    of a long-running operation.

    CancelToken itself is observer-only: it can never request cancellation.
    Cancellation is requested through a CancelTokenSource, which owns the
    cancellation state and cancels it either explicitly via
    CancelTokenSource::cancel() or automatically when the source is destroyed.
    Tokens obtained from a source via CancelTokenSource::getToken() observe
    that cancellation.

    All copies of a CancelToken share the same underlying cancellation state,
    so a cancellation requested by the source is immediately visible to every
    copy. Tokens can be handed to worker threads or asynchronous sub-operations
    freely; no token copy can ever trigger cancellation itself.

    The token can be observed in three complementary ways:

      - Non-blocking polling: wasCancelled() is a lock-free atomic read that
        can be checked from any thread, including real-time threads.

      - Blocking wait: waitForCancellation() suspends the caller until the
        token is cancelled, or a timeout expires.

      - Callbacks: registerCallback() attaches a callback that is invoked
        exactly once, on the thread that cancels the token, in registration
        order. A callback registered after the token was already cancelled is
        invoked synchronously by registerCallback().

    Tokens that can never be cancelled can be created with the default
    constructor or with none(); calling waitForCancellation() on such a token
    returns false immediately and registered callbacks are never invoked.

    This class is header-light and wasCancelled() performs no allocation, so it
    is safe to use on hot paths.

    @see CancelTokenSource

    @tags{Core}
*/
class YUP_API CancelToken
{
private:
    //==============================================================================
    struct State;

public:
    //==============================================================================
    /** A handle to a registered cancellation callback.

        Returned by CancelToken::registerCallback(). The callback remains
        registered while this object is alive; destroying it (or calling
        unregister()) removes the callback so it will not be invoked by any
        future cancellation.

        This class is move-only: copies would make unregistration ambiguous, so
        they are disabled.

        @see CancelToken::registerCallback
    */
    class YUP_API Registration
    {
    public:
        //==============================================================================
        /** Creates an empty registration that is not attached to any token. */
        Registration() = default;

        /** Destructor.

            If this registration is still attached to a token, the callback is
            unregistered before the handle is destroyed.

            @see unregister
        */
        ~Registration();

        /** Moves another registration's state into this one, releasing any
            callback this object currently owns.
        */
        Registration& operator= (Registration&& other) noexcept;

        /** Moves another registration's state into this one.

            After the move, the source registration becomes invalid.
        */
        Registration (Registration&& other) noexcept;

        //==============================================================================
        /** Removes the associated callback from the token.

            After this method returns, the callback will not be invoked by any
            future cancellation. Note that if a cancellation was already in
            progress and had captured this callback before unregister() ran,
            that in-flight invocation may still complete.

            Calling unregister() on an already-released registration is a
            harmless no-op.

            @see isValid
        */
        void unregister() noexcept;

        /** Returns true if this registration is still attached to a token,
            i.e. unregister() has not yet been called (or the state moved away).

            @see unregister
        */
        bool isValid() const noexcept;

    private:
        friend class CancelToken;

        Registration (std::shared_ptr<State> stateToHold, uint64 callbackIdToHold) noexcept;

        std::shared_ptr<State> state;
        uint64 callbackId = 0;

        YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Registration)
    };

    //==============================================================================
    /** Creates a token that can never be cancelled.

        This is equivalent to none(). A cancellable token can only be obtained
        from a CancelTokenSource via CancelTokenSource::getToken().

        @see none, CancelTokenSource::getToken
    */
    CancelToken() = default;

    /** Destructor. */
    ~CancelToken();

    /** Copies the token.

        All copies share the same cancellation state, so a cancellation
        requested by the token's CancelTokenSource is observed by all the
        others.

        @see wasCancelled
    */
    CancelToken (const CancelToken&) = default;

    /** Moves the token.

        After the move, the source token becomes non-cancellable (equivalent to
        none()) while the destination keeps the shared cancellation state.
    */
    CancelToken (CancelToken&&) noexcept = default;

    /** Replaces this token's state with a copy of another token's state. */
    CancelToken& operator= (const CancelToken&);

    /** Replaces this token's state with another token's state, leaving the
        source token non-cancellable.
    */
    CancelToken& operator= (CancelToken&&) noexcept;

    //==============================================================================
    /** Returns a token that can never be cancelled.

        This is equivalent to the default constructor. Calling
        waitForCancellation() on such a token returns false immediately,
        wasCancelled() always returns false and isCancellable() returns false.
        All such tokens compare equal to each other.

        @see isCancellable, CancelTokenSource
    */
    static CancelToken none();

    //==============================================================================
    /** Returns true if the token has been cancelled.

        This happens when the token's CancelTokenSource has been cancelled or
        destroyed. It is a lock-free atomic read, safe to call from any thread,
        including real-time threads.

        @see CancelTokenSource::cancel
    */
    bool wasCancelled() const noexcept;

    /** Returns true if this token is linked to a cancellation state that can
        still be cancelled, i.e. it was obtained from a live CancelTokenSource.

        This is false for tokens created with the default constructor or with
        none(), and for tokens that were moved-from.

        @see CancelTokenSource::getToken, none
    */
    bool isCancellable() const noexcept;

    //==============================================================================
    /** Blocks the calling thread until the token is cancelled.

        This uses a manual-reset waitable event, so if the token has already
        been cancelled the method returns immediately, and any number of
        threads can wait concurrently - all of them are woken up when the
        token's CancelTokenSource is cancelled.

        @param timeOutMilliseconds  the maximum time to wait, in milliseconds.
                                    A negative value waits forever.

        @returns    true if the token was cancelled; false if the timeout
                    expired first, or if this token can never be cancelled
                    (default-constructed / none() / moved-from).

        @see wasCancelled, CancelTokenSource
    */
    bool waitForCancellation (int timeOutMilliseconds = -1) const;

    //==============================================================================
    /** Registers a callback to be invoked when the token is cancelled.

        The callback will be invoked exactly once, on the thread that cancels
        the token (i.e. the thread calling CancelTokenSource::cancel() or
        destroying the source), and in the order the callbacks were registered.

        If the token has already been cancelled, the callback is invoked
        synchronously by this method, before it returns, and the returned
        registration is invalid.

        If the token can never be cancelled (default-constructed / none()),
        the callback is never invoked and the returned registration is invalid.

        The callback must not be empty, must not block, and must not throw
        exceptions - a throwing callback is caught and asserted, and does not
        prevent the remaining callbacks from running.

        @param callback    the function to invoke on cancellation

        @returns    a move-only Registration handle. Keep it alive for as long
                    as the callback should remain registered; destroying it
                    (or calling Registration::unregister()) removes the
                    callback so a future cancellation will not invoke it. Note
                    that if a cancellation is already in progress and has
                    captured this callback, it may still run once after
                    unregister() returns.

        @see Registration, CancelTokenSource
    */
    [[nodiscard]] Registration registerCallback (std::function<void()> callback);

    //==============================================================================
    /** Returns true if both tokens share the same underlying cancellation state. */
    bool operator== (const CancelToken& other) const noexcept;

    /** Returns true if the tokens do not share the same cancellation state. */
    bool operator!= (const CancelToken& other) const noexcept;

private:
    friend class CancelTokenSource;

    explicit CancelToken (std::shared_ptr<State> stateToHold) noexcept;

    std::shared_ptr<State> state;

    YUP_LEAK_DETECTOR (CancelToken)
};

} // namespace yup
