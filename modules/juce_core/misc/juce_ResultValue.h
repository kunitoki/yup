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
    Represents the 'success' or 'failure' of an operation that returns a value, and holds an associated
    error message to describe the error when there's a failure.

    E.g.
    @code
    ResultValue<int> myOperation()
    {
        if (doSomeKindOfFoobar())
            return ResultValue<int>::ok (1337);
        else
            return ResultValue<int>::fail ("foobar didn't work!");
    }

    const ResultValue<int> result (myOperation());

    if (result.wasOk())
    {
        const int& resultInteger = result.getReference();

        ...it's all good, use the value...
    }
    else
    {
        warnUserAboutFailure ("The foobar operation failed! Error message was: "
                                + result.getErrorMessage());
    }
    @endcode

    @tags{Core}
*/
template <class T>
class JUCE_API ResultValue
{
public:
    //==============================================================================
    /** Creates and returns a 'successful' result value. */
    template <class U>
    static auto ok (U&& value) noexcept
        -> std::enable_if_t<std::is_constructible_v<T, U>, ResultValue>
    {
        return ResultValue (std::forward<U> (value));
    }

    /** Creates a 'failure' result.
        If you pass a blank error message in here, a default "Unknown Error" message will be used instead.
    */
    static ResultValue fail (StringRef errorMessage) noexcept
    {
        return ResultValue (errorMessage.isEmpty() ? "Unknown Error" : errorMessage, ErrorTag {});
    }

    //==============================================================================
    /** Returns true if this result indicates a success. */
    bool wasOk() const noexcept
    {
        return valueOrErrorMessage.index() == 1;
    }

    /** Returns true if this result indicates a failure.
        You can use getErrorMessage() to retrieve the error message associated
        with the failure.
    */
    bool failed() const noexcept
    {
        return valueOrErrorMessage.index() != 1;
    }

    /** Returns true if this result indicates a success.
        This is equivalent to calling wasOk().
    */
    explicit operator bool() const noexcept
    {
        return valueOrErrorMessage.index() == 1;
    }

    /** Returns true if this result indicates a failure.
        This is equivalent to calling failed().
    */
    bool operator!() const noexcept
    {
        return valueOrErrorMessage.index() != 1;
    }

    /** Returns a copy of the value that was set when this result was created. */
    T getValue() const
    {
        jassert (valueOrErrorMessage.index() == 1); // Trying to access the value of the result, when the result is holding an error instead!

        return std::get<1> (valueOrErrorMessage);
    }

    /** Returns the mutable reference that was set when this result was created. */
    T& getReference() noexcept
    {
        jassert (valueOrErrorMessage.index() == 1); // Trying to access the value of the result, when the result is holding an error instead!

        return std::get<1> (valueOrErrorMessage);
    }

    /** Returns the const reference that was set when this result was created. */
    const T& getReference() const noexcept
    {
        jassert (valueOrErrorMessage.index() == 1); // Trying to access the value of the result, when the result is holding an error instead!

        return std::get<1> (valueOrErrorMessage);
    }

    /** Returns the error message that was set when this result was created.
        For a successful result, this will be an empty string;
    */
    const String& getErrorMessage() const noexcept
    {
        jassert (valueOrErrorMessage.index() == 2); // Trying to access the error message of the result, when the result is holding a value instead!

        return std::get<2> (valueOrErrorMessage);
    }

    //==============================================================================
    ResultValue (const ResultValue&) = default;
    ResultValue& operator= (const ResultValue&) = default;
    ResultValue (ResultValue&&) noexcept = default;
    ResultValue& operator= (ResultValue&&) noexcept = default;

    bool operator== (const ResultValue& other) const noexcept
    {
        return valueOrErrorMessage == other.valueOrErrorMessage;
    }

    bool operator!= (const ResultValue& other) const noexcept
    {
        return valueOrErrorMessage != other.valueOrErrorMessage;
    }

private:
    std::variant<std::monostate, T, String> valueOrErrorMessage;

    struct ErrorTag
    {
    };

    // The default constructor is not for public use!
    // Instead, use ResultValue::ok() or ResultValue::fail()
    ResultValue() noexcept {}

    template <class U>
    explicit ResultValue (U&& value) noexcept
        : valueOrErrorMessage (std::in_place_index<1>, std::forward<U> (value))
    {
    }

    ResultValue (StringRef message, ErrorTag) noexcept
        : valueOrErrorMessage (std::in_place_index<2>, message)
    {
    }

    // These casts are private to prevent people trying to use the ResultValue object in numeric contexts
    operator int() const;
    operator void*() const;
};

} // namespace juce
