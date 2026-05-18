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

namespace yup
{

//==============================================================================
/** Helper type returned by yup::makeResultValueOk(), implicitly convertible to a successful ResultValue<T>.

    @see makeResultValueOk, ResultValue
    @tags{Core}
*/
template <class T>
struct YUP_API OkValue
{
    template <class U>
        requires std::constructible_from<T, U>
    explicit constexpr OkValue (U&& v) noexcept (std::is_nothrow_constructible_v<T, U>)
        : value (std::forward<U> (v))
    {
    }

    T value;
};

/** Helper type returned by yup::makeResultValueFail(), implicitly convertible to a failed ResultValue<T>.

    @see makeResultValueFail, ResultValue
    @tags{Core}
*/
struct YUP_API FailValue
{
    explicit FailValue (StringRef errorMessage) noexcept
        : message (errorMessage.isEmpty() ? StringRef ("Unknown Error") : errorMessage)
    {
    }

    String message;
};

//==============================================================================
/** Creates an OkValue that implicitly converts to a successful ResultValue<T>, allowing
    return-site type deduction without naming ResultValue<T> explicitly.

    @code
    ResultValue<int> myOperation()
    {
        return yup::makeResultValueOk (1337); // T is deduced from the argument
    }
    @endcode

    @see ResultValue, makeResultValueFail
    @tags{Core}
*/
template <class T>
[[nodiscard]] constexpr auto makeResultValueOk (T&& value) noexcept (std::is_nothrow_constructible_v<std::decay_t<T>, T>)
    -> OkValue<std::decay_t<T>>
{
    return OkValue<std::decay_t<T>> (std::forward<T> (value));
}

/** Creates a FailValue that implicitly converts to a failed ResultValue<T>, allowing
    return-site failure without naming ResultValue<T> explicitly.

    @code
    ResultValue<int> myOperation()
    {
        return yup::makeResultValueFail ("something went wrong"); // Works for any ResultValue<T>
    }
    @endcode

    @see ResultValue, makeResultValueOk
    @tags{Core}
*/
[[nodiscard]] inline FailValue makeResultValueFail (StringRef errorMessage) noexcept
{
    return FailValue (errorMessage);
}

//==============================================================================
/**
    Represents the 'success' or 'failure' of an operation that returns a value, and holds an associated
    error message to describe the error when there's a failure.

    Prefer the free functions yup::makeResultValueOk (..) and yup::makeResultValueFail() over the static methods to avoid
    having to repeat the template type at the call site:

    @code
    ResultValue<int> myOperation()
    {
        if (doSomeKindOfFoobar())
            return yup::makeResultValueOk (1337);
        else
            return yup::makeResultValueFail ("foobar didn't work!");
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
class YUP_API ResultValue
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
        return ResultValue (errorMessage.isEmpty() ? StringRef ("Unknown Error") : errorMessage, ErrorTag {});
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
    T getValue() const&
        requires std::copy_constructible<T>
    {
        jassert (valueOrErrorMessage.index() == 1); // Trying to access the value of the result, when the result is holding an error instead!

        return std::get<1> (valueOrErrorMessage);
    }

    /** Returns a moved from value that was set when this result was created. */
    T getValue() &&
        requires std::move_constructible<T>
    {
        jassert (valueOrErrorMessage.index() == 1); // Trying to access the value of the result, when the result is holding an error instead!

        return std::get<1> (std::move (valueOrErrorMessage));
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

    /** Constructs a successful result from an OkValue, enabling type-deduced return syntax. */
    template <class U>
        requires std::constructible_from<T, U>
    ResultValue (OkValue<U>&& okVal) noexcept (std::is_nothrow_constructible_v<T, U>)
        : valueOrErrorMessage (std::in_place_index<1>, std::move (okVal.value))
    {
    }

    /** Constructs a failed result from a FailValue, enabling type-deduced return syntax. */
    ResultValue (FailValue&& failVal) noexcept
        : valueOrErrorMessage (std::in_place_index<2>, std::move (failVal.message))
    {
    }

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

} // namespace yup
