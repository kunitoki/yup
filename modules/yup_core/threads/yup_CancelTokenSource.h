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

namespace yup
{

//==============================================================================
/**
    A move-only RAII owner of a CancelToken that requests cancellation of the
    token when it is destroyed.

    This is the cancelling counterpart to the observer-only CancelToken: create a
    source where an operation is started, hand observer copies obtained from
    getToken() to the worker threads or sub-operations, and when the source is
    destroyed (e.g. the owning view, session or processor goes away) the token
    is cancelled automatically - waking any waitForCancellation() waiters and
    invoking any registered callbacks.

    Cancellation can also be requested explicitly at any time with cancel().

    The source is move-only: copies are disabled because two owners would make
    the destruction semantics ambiguous. Moving transfers ownership: the
    moved-from source becomes inert and does not cancel on destruction. Tokens
    obtained from getToken() are plain observer CancelTokens and can never trigger
    cancellation themselves.

    @see CancelToken

    @tags{Core}
*/
class YUP_API CancelTokenSource
{
public:
    //==============================================================================
    /** Creates a source that owns a fresh, cancellable token.

        The token can be observed (and copied) through getToken(); when this
        source is destroyed it will request cancellation of that token.

        @see getToken
    */
    CancelTokenSource();

    /** Destructor.

        Requests cancellation of the owned token - waking up any threads
        blocked in CancelToken::waitForCancellation() and invoking any
        registered callbacks on the calling thread - unless this source has
        been moved-from, in which case it does nothing.

        @see cancel
    */
    ~CancelTokenSource();

    /** Moves ownership of the token from another source into this one.

        After the move the other source becomes inert: it no longer owns a
        cancellable token and its destructor will not request cancellation.

        @see getToken
    */
    CancelTokenSource (CancelTokenSource&&) noexcept;

    /** Moves ownership from another source, first cancelling whatever token
        this source currently owns.

        @see cancel
    */
    CancelTokenSource& operator= (CancelTokenSource&&) noexcept;

    //==============================================================================
    /** Requests cancellation of the owned token.

        This marks the token as cancelled and wakes up any threads blocked in
        CancelToken::waitForCancellation(), invoking any registered callbacks
        on the calling thread. It is idempotent and safe to call from any
        thread; the destructor also calls it, so explicit cancellation is
        optional.

        @see wasCancelled, getToken
    */
    void cancel() noexcept;

    /** Returns a copy of the owned token that can be observed and copied
        freely.

        The returned token shares the cancellation state with this source, so
        it is cancelled when this source is cancelled or destroyed. Copies of
        the returned token never trigger cancellation themselves.

        @see cancel, wasCancelled
    */
    CancelToken getToken() const;

    /** Returns true if the owned token has been cancelled. */
    bool wasCancelled() const noexcept;

    /** Returns true if this source still owns a cancellable token (i.e. it
        has not been moved-from).

        @see getToken
    */
    bool isCancellable() const noexcept;

private:
    std::shared_ptr<CancelToken::State> state;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CancelTokenSource)
};

} // namespace yup
