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

namespace juce
{

//==============================================================================
/**

    @tags{Core}
*/
class ThreadValueInitialiser
{
public:
    /** Create a ThreadValueInitialiser from a callable. */
    template <typename F>
    ThreadValueInitialiser (F&& initialisationFunction) noexcept
        : func (std::forward<F> (initialisationFunction))
    {
    }

    /** Returns if the value was already initialised. */
    forcedinline bool isInitialised() const noexcept
    {
        return state == State::initialised;
    }

    /** Initialises the value if needed, will run at most once. */
    void ensureInitialised()
    {
        if (isInitialised())
            return;

        auto expected = State::uninitialised;

        if (state.compare_exchange_strong (expected, State::initialising))
        {
            func();

            state = State::initialised;
            return;
        }

        while (! isInitialised())
        {
            int yieldCount = 100;
            while (! isInitialised() && --yieldCount >= 0)
                std::this_thread::yield();

            int sleepZeroCount = 10;
            while (! isInitialised() && --sleepZeroCount >= 0)
                std::this_thread::sleep_for (std::chrono::microseconds (0));

            int sleepOneCount = 1;
            while (! isInitialised() && --sleepOneCount >= 0)
                std::this_thread::sleep_for (std::chrono::microseconds (1));
        }
    }

private:
    //==============================================================================
    enum class State
    {
        uninitialised,
        initialising,
        initialised
    };

    std::atomic<State> state { State::uninitialised };
    std::function<void()> func = nullptr;

    JUCE_DECLARE_NON_COPYABLE (ThreadValueInitialiser)
};

} // namespace juce
