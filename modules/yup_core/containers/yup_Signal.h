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
/** Keeps a Signal callback connected for as long as it lives.

    Returned by Signal::connect(). Destroying or reassigning the connection disconnects the
    callback; a default constructed connection is not connected. It may outlive its signal,
    disconnecting then does nothing.

    @see Signal
*/
class YUP_API SignalConnection
{
public:
    //==============================================================================
    /** The part of a signal a connection talks to, implemented by Signal. */
    class Owner
    {
    public:
        virtual ~Owner() = default;
        virtual void disconnect (uint64 id) noexcept = 0;
        virtual bool isConnected (uint64 id) const noexcept = 0;
    };

    //==============================================================================
    /** Creates a connection that is not connected. */
    SignalConnection() = default;

    /** Creates a connection to a callback of a signal, used by Signal::connect(). */
    SignalConnection (std::weak_ptr<Owner> signalOwner, uint64 callbackId) noexcept;

    /** Disconnects the callback. */
    ~SignalConnection();

    SignalConnection (SignalConnection&& other) noexcept;
    SignalConnection& operator= (SignalConnection&& other) noexcept;

    //==============================================================================
    /** Disconnects the callback, it won't be called again, even by an emit in progress. */
    void disconnect() noexcept;

    /** Returns true while the callback is connected to a living signal. */
    bool isConnected() const noexcept;

private:
    std::weak_ptr<Owner> owner;
    uint64 id = 0;

    YUP_DECLARE_NON_COPYABLE (SignalConnection)
};

//==============================================================================
/** Delivers the callbacks of Signal::emitAsync().

    yup_core has no message loop, so asynchronous emission goes through a dispatcher installed
    by whoever owns one. The yup_events MessageManager installs a dispatcher posting to the
    message thread while it exists; without a dispatcher Signal::emitAsync() delivers nothing.

    @see Signal::emitAsync
*/
class YUP_API SignalDispatcher
{
public:
    //==============================================================================
    /** Queues a function to be called later, returning false if it couldn't be queued. */
    using Function = std::function<bool (std::function<void()>)>;

    //==============================================================================
    /** Installs the dispatcher used by Signal::emitAsync(), nullptr removes it.

        @return The previously installed dispatcher, so it can be restored.
    */
    static Function setAsyncDispatcher (Function newDispatcher);

    /** Queues a function on the installed dispatcher.

        @return False if no dispatcher is installed or it couldn't queue the function.
    */
    static bool dispatchAsync (std::function<void()> function);
};

//==============================================================================
/** A list of callbacks called in connection order.

    @code
    yup::Signal<const yup::String&> nameChanged;
    auto connection = nameChanged.connect ([] (const yup::String& name) { DBG (name); });

    nameChanged.emit (yup::String ("now"));       // calls every callback before returning
    nameChanged.emitAsync (yup::String ("later")); // calls them from the message thread
    @endcode

    emit() forwards its arguments: every callback but the last one gets them as lvalues, the last
    one gets them as they were passed, so an rvalue is moved only into the last callback.
    Arguments taken by value must be copyable, every callback gets its own value.

    Callbacks may connect or disconnect callbacks, or destroy the signal, while it emits:
    callbacks connected during an emit are called from the next one, callbacks disconnected
    during an emit are not called anymore.

    Connecting, disconnecting and emit() are meant for the message thread only. emitAsync() can be
    called from any thread while the signal is alive.

    @tparam Args  The callback parameter types. Rvalue references are not allowed, take the
                  value instead. Types taken by value must be copy constructible.

    @see SignalConnection, SignalDispatcher
*/
template <class... Args>
class Signal
{
    static_assert ((! std::is_rvalue_reference_v<Args> && ...), "Signal arguments can't be rvalue references, take them by value");
    static_assert (((std::is_reference_v<Args> || std::is_copy_constructible_v<Args>) && ...), "Signal arguments taken by value must be copyable, every callback gets its own copy");

public:
    //==============================================================================
    using Callback = std::function<void (Args...)>;

    //==============================================================================
    /** Creates a signal without callbacks. */
    Signal() = default;

    /** Disconnects every callback, an emit in progress won't call the remaining ones. */
    ~Signal()
    {
        for (const auto& slot : state->slots)
            slot->connected = false;

        state->slots.clear();
    }

    //==============================================================================
    /** Connects a callback.

        @param callback  Any callable invocable with Args, forwarded into the signal.

        @return The connection that keeps the callback connected while it lives.
    */
    template <class F>
        requires std::is_invocable_v<F&, Args...>
    [[nodiscard]] SignalConnection connect (F&& callback)
    {
        Callback function (std::forward<F> (callback));
        jassert (function != nullptr);

        const auto id = state->nextId++;
        state->slots.push_back (std::make_shared<Slot> (Slot { id, std::move (function), true }));
        return { state, id };
    }

    /** Calls every connected callback, in connection order, before returning.

        @param args  The arguments, forwarded as described in the class description.
    */
    template <class... Ts>
        requires (sizeof... (Ts) == sizeof... (Args) && (std::is_convertible_v<Ts&&, Args> && ...))
    void emit (Ts&&... args) const
    {
        emitTo (state, std::forward<Ts> (args)...);
    }

    /** Calls every callback connected at delivery time, from the dispatcher thread.

        The arguments are decayed and stored until delivery, then moved into the last callback.
        Nothing is called if the signal is destroyed before
        delivery. Non-const lvalue reference arguments are not allowed, the callbacks would modify
        a copy.

        @return False if no SignalDispatcher is installed or it couldn't queue the emission.

        @see SignalDispatcher
    */
    template <class... Ts>
        requires (sizeof... (Ts) == sizeof... (Args) && (std::is_convertible_v<Ts&&, Args> && ...))
    bool emitAsync (Ts&&... args) const
    {
        static_assert ((! isMutableReference<Args> && ...), "emitAsync can't deliver non-const lvalue references");

        auto values = std::make_shared<std::tuple<std::decay_t<Args>...>> (std::forward<Ts> (args)...);

        return SignalDispatcher::dispatchAsync ([weakState = std::weak_ptr<State> (state), values = std::move (values)]
        {
            if (auto lockedState = weakState.lock())
            {
                std::apply ([&] (auto&... stored)
                {
                    emitTo (lockedState, std::move (stored)...);
                },
                            *values);
            }
        });
    }

    /** Returns the number of connected callbacks. */
    int getNumConnections() const noexcept { return static_cast<int> (state->slots.size()); }

private:
    //==============================================================================
    struct Slot
    {
        uint64 id = 0;
        Callback callback;
        bool connected = true;
    };

    struct State final : SignalConnection::Owner
    {
        void disconnect (uint64 id) noexcept override
        {
            const auto it = std::find_if (slots.begin(), slots.end(), [id] (const auto& slot) { return slot->id == id; });
            if (it == slots.end())
                return;

            (*it)->connected = false;
            slots.erase (it);
        }

        bool isConnected (uint64 id) const noexcept override
        {
            return std::any_of (slots.begin(), slots.end(), [id] (const auto& slot) { return slot->id == id; });
        }

        std::vector<std::shared_ptr<Slot>> slots;
        uint64 nextId = 1;
    };

    template <class T>
    static constexpr bool isMutableReference = std::is_lvalue_reference_v<T> && ! std::is_const_v<std::remove_reference_t<T>>;

    template <class... Ts>
    static void emitTo (const std::shared_ptr<State>& state, Ts&&... args)
    {
        if (state->slots.empty())
            return;

        const auto keepAlive = state;
        const auto snapshot = state->slots;
        const auto last = snapshot.size() - 1;

        for (std::size_t i = 0; i < last; ++i)
        {
            if (snapshot[i]->connected)
                snapshot[i]->callback (args...);
        }

        if (snapshot[last]->connected)
            snapshot[last]->callback (std::forward<Ts> (args)...);
    }

    std::shared_ptr<State> state = std::make_shared<State>();

    YUP_DECLARE_NON_COPYABLE (Signal)
};

} // namespace yup
