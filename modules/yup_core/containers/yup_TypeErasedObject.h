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
/**
    A move-only, type-erased container for storing small objects.

    The `TypeErasedObject` template struct stores an object of a specified type in a type-erased manner, provided that the size
    of the object is less than or equal to the specified `NumBytes`. This struct ensures that objects are move-only, and it uses type erasure
    to store them in a generic way while still allowing retrieval of the original type at a later point.
 
    @tparam NumBytes The maximum number of bytes available for storing the payload object.

    @tags{Core}
*/
template <std::size_t NumBytes>
struct TypeErasedObject
{
    /** Default constructor that creates an empty `TypeErasedObject`. */
    TypeErasedObject() = default;

    /**
	    Constructs the payload by moving the provided value into the internal buffer.
	 
	    This constructor accepts a value of type `T`, moves it into the internal buffer, and sets up a deleter
	    to destroy the object when the payload is destructed.
	 
	    @tparam T The type of the object being stored. The size of `T` must be less than or equal to `NumBytes`.
	 
	    @param value The object to store in the payload.
	*/
    template <class T>
    explicit TypeErasedObject (T&& value)
        requires (sizeof (T) > 0 && sizeof (T) <= NumBytes)
        : type (typeid (T))
    {
        constructAt (reinterpret_cast<T*> (&objectBuffer[0]), std::forward<T> (value));

        deleterCallback = +[] (void* buffer)
        {
            destroyAt (std::launder (reinterpret_cast<T*> (buffer)));
        };
    }

    /** Destroys the payload and calls the stored deleter to clean up the contained object. */
    ~TypeErasedObject()
    {
        if (deleterCallback != nullptr)
            deleterCallback (static_cast<void*> (&objectBuffer[0]));
    }

    /**
	    Move constructor that transfers ownership of the payload from another instance.
	 
	    Moves the contents of the payload from `other` into this instance, ensuring that the
	    other instance is left in a valid but empty state.
	 
	    @param other The payload to move from.
	*/
    TypeErasedObject (TypeErasedObject&& other) noexcept
        : deleterCallback (std::exchange (other.deleterCallback, nullptr))
        , type (std::exchange (other.type, typeid (void)))
    {
        std::memcpy (objectBuffer, other.objectBuffer, jmin (sizeof (objectBuffer), sizeof (other.objectBuffer)));
    }

    /**
	    Move assignment operator that transfers ownership of the payload from another instance.
	 
	    Moves the contents of the payload from `other` into this instance, properly destroying the
	    current payload object if one exists, and leaving `other` in a valid but empty state.
	 
	    @param other The payload to move from.
	 
	    @return A reference to this `TypeErasedObject` after the move.
	*/
    TypeErasedObject& operator= (TypeErasedObject&& other)
    {
        if (auto deleter = std::exchange (deleterCallback, nullptr))
            deleter (reinterpret_cast<void*> (&objectBuffer[0]));

        deleterCallback = std::exchange (other.deleterCallback, nullptr);
        type = std::exchange (other.type, typeid (void));
        std::memcpy (objectBuffer, other.objectBuffer, jmin (sizeof (objectBuffer), sizeof (other.objectBuffer)));

        return *this;
    }

    /** Deleted copy constructor to ensure the payload is move-only. */
    TypeErasedObject (const TypeErasedObject&) = delete;

    /** Deleted copy assignment operator to ensure the payload is move-only. */
    TypeErasedObject& operator= (const TypeErasedObject&) = delete;

    /**
	    Retrieves a pointer to the stored payload object of type `T` (const version).
	 
	    Returns a pointer to the stored object if the stored type matches `T`; otherwise, returns `nullptr`.
	 
	    @tparam T The expected type of the stored object.
	 
	    @return A pointer to the stored object of type `T`, or `nullptr` if the types don't match.
	*/
    template <class T>
    const T* getPayload() const noexcept
        requires (sizeof (T) > 0 && sizeof (T) <= NumBytes)
    {
        if (deleterCallback != nullptr && typeid (T) == type)
            return std::launder (reinterpret_cast<const T*> (&objectBuffer[0]));

        return nullptr;
    }

    /**
	    Retrieves a pointer to the stored payload object of type `T` (non-const version).
	 
	    Returns a pointer to the stored object if the stored type matches `T`; otherwise, returns `nullptr`.
	 
	    @tparam T The expected type of the stored object.
	 
	    @return A pointer to the stored object of type `T`, or `nullptr` if the types don't match.
	*/
    template <class T>
    T* getPayload() noexcept
        requires (sizeof (T) > 0 && sizeof (T) <= NumBytes)
    {
        if (deleterCallback != nullptr && typeid (T) == type)
            return std::launder (reinterpret_cast<T*> (&objectBuffer[0]));

        return nullptr;
    }

private:
    alignas (alignof (std::max_align_t)) uint8 objectBuffer[NumBytes] = {};
    void (*deleterCallback) (void*) = nullptr;
    std::type_index type = typeid (void);
};

} // namespace yup
