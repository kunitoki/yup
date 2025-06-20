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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

//==============================================================================
/**
    Used by the YUP_DECLARE_SINGLETON macros to manage a static pointer
    to a singleton instance.

    You generally won't use this directly, but see the macros YUP_DECLARE_SINGLETON,
    YUP_DECLARE_SINGLETON_SINGLETHREADED, YUP_DECLARE_SINGLETON_SINGLETHREADED_MINIMAL,
    and YUP_IMPLEMENT_SINGLETON for how it is intended to be used.

    @tags{Core}
*/
template <typename Type, typename MutexType, bool onlyCreateOncePerRun>
struct SingletonHolder : private MutexType // (inherited so we can use the empty-base-class optimisation)
{
    SingletonHolder() = default;

    ~SingletonHolder()
    {
        /* The static singleton holder is being deleted before the object that it holds
           has been deleted. This could mean that you've forgotten to call clearSingletonInstance()
           in the class's destructor, or have failed to delete it before your app shuts down.
           If you're having trouble cleaning up your singletons, perhaps consider using the
           SharedResourcePointer class instead.
        */
        jassert (instance.load() == nullptr);
    }

    /** Returns the current instance, or creates a new instance if there isn't one. */
    Type* get()
    {
        if (auto* ptr = instance.load())
            return ptr;

        typename MutexType::ScopedLockType sl (*this);

        if (auto* ptr = instance.load())
            return ptr;

        auto once = onlyCreateOncePerRun; // (local copy avoids VS compiler warning about this being constant)

        if (once)
        {
            static bool createdOnceAlready = false;

            if (createdOnceAlready)
            {
                // This means that the doNotRecreateAfterDeletion flag was set
                // and you tried to create the singleton more than once.
                jassertfalse;
                return nullptr;
            }

            createdOnceAlready = true;
        }

        static bool alreadyInside = false;

        if (alreadyInside)
        {
            // This means that your object's constructor has done something which has
            // ended up causing a recursive loop of singleton creation.
            jassertfalse;
            return nullptr;
        }

        const ScopedValueSetter<bool> scope (alreadyInside, true);
        return getWithoutChecking();
    }

    /** Returns the current instance, or creates a new instance if there isn't one, but doesn't do
        any locking, or checking for recursion or error conditions.
    */
    Type* getWithoutChecking()
    {
        if (auto* p = instance.load())
            return p;

        auto* newObject = new Type(); // (create into a local so that instance is still null during construction)
        instance.store (newObject);
        return newObject;
    }

    /** Deletes and resets the current instance, if there is one. */
    void deleteInstance()
    {
        typename MutexType::ScopedLockType sl (*this);
        delete instance.exchange (nullptr);
    }

    /** Called by the class's destructor to clear the pointer if it is currently set to the given object. */
    void clear (Type* expectedObject) noexcept
    {
        instance.compare_exchange_strong (expectedObject, nullptr);
    }

    // This must be atomic, otherwise a late call to get() may attempt to read instance while it is
    // being modified by the very first call to get().
    std::atomic<Type*> instance { nullptr };
};

#ifndef DOXYGEN
#define YUP_PRIVATE_DECLARE_SINGLETON(Classname, mutex, doNotRecreate, inlineToken, getter)                   \
    static inlineToken yup::SingletonHolder<Classname, mutex, doNotRecreate> singletonHolder;                 \
    friend yup::SingletonHolder<Classname, mutex, doNotRecreate>;                                             \
    static Classname* YUP_CALLTYPE getInstance() { return singletonHolder.getter(); }                         \
    static Classname* YUP_CALLTYPE getInstanceWithoutCreating() noexcept { return singletonHolder.instance; } \
    static void YUP_CALLTYPE deleteInstance() noexcept { singletonHolder.deleteInstance(); }                  \
    void clearSingletonInstance() noexcept { singletonHolder.clear (this); }
#endif

//==============================================================================
/**
    Macro to generate the appropriate methods and boilerplate for a singleton class.

    To use this, add the line YUP_DECLARE_SINGLETON (MyClass, doNotRecreateAfterDeletion)
    to the class's definition.

    Then put a macro YUP_IMPLEMENT_SINGLETON (MyClass) along with the class's
    implementation code.

    It's also a very good idea to also add the call clearSingletonInstance() in your class's
    destructor, in case it is deleted by other means than deleteInstance()

    Clients can then call the static method MyClass::getInstance() to get a pointer
    to the singleton, or MyClass::getInstanceWithoutCreating() which will return nullptr if
    no instance currently exists.

    e.g. @code

        struct MySingleton
        {
            MySingleton() {}

            ~MySingleton()
            {
                // this ensures that no dangling pointers are left when the
                // singleton is deleted.
                clearSingletonInstance();
            }

            YUP_DECLARE_SINGLETON (MySingleton, false)
        };

        // ..and this goes in a suitable .cpp file:
        YUP_IMPLEMENT_SINGLETON (MySingleton)


        // example of usage:
        auto* m = MySingleton::getInstance(); // creates the singleton if there isn't already one.

        ...

        MySingleton::deleteInstance(); // safely deletes the singleton (if it's been created).

    @endcode

    If doNotRecreateAfterDeletion = true, it won't allow the object to be created more
    than once during the process's lifetime - i.e. after you've created and deleted the
    object, getInstance() will refuse to create another one. This can be useful to stop
    objects being accidentally re-created during your app's shutdown code.

    If you know that your object will only be created and deleted by a single thread, you
    can use the slightly more efficient YUP_DECLARE_SINGLETON_SINGLETHREADED macro instead
    of this one.

    @see YUP_IMPLEMENT_SINGLETON, YUP_DECLARE_SINGLETON_SINGLETHREADED
*/
#define YUP_DECLARE_SINGLETON(Classname, doNotRecreateAfterDeletion) \
    YUP_PRIVATE_DECLARE_SINGLETON (Classname, yup::CriticalSection, doNotRecreateAfterDeletion, , get)

//==============================================================================
/**
    The same as YUP_DECLARE_SINGLETON, but does not require a matching
    YUP_IMPLEMENT_SINGLETON definition.
*/
#define YUP_DECLARE_SINGLETON_INLINE(Classname, doNotRecreateAfterDeletion) \
    YUP_PRIVATE_DECLARE_SINGLETON (Classname, yup::CriticalSection, doNotRecreateAfterDeletion, inline, get)

//==============================================================================
/** This is a counterpart to the YUP_DECLARE_SINGLETON macros.

    After adding the YUP_DECLARE_SINGLETON to the class definition, this macro has
    to be used in the cpp file.
*/
#define YUP_IMPLEMENT_SINGLETON(Classname) \
    decltype (Classname::singletonHolder) Classname::singletonHolder;

//==============================================================================
/**
    Macro to declare member variables and methods for a singleton class.

    This is exactly the same as YUP_DECLARE_SINGLETON, but doesn't use a critical
    section to make access to it thread-safe. If you know that your object will
    only ever be created or deleted by a single thread, then this is a
    more efficient version to use.

    If doNotRecreateAfterDeletion = true, it won't allow the object to be created more
    than once during the process's lifetime - i.e. after you've created and deleted the
    object, getInstance() will refuse to create another one. This can be useful to stop
    objects being accidentally re-created during your app's shutdown code.

    See the documentation for YUP_DECLARE_SINGLETON for more information about
    how to use it. Just like YUP_DECLARE_SINGLETON you need to also have a
    corresponding YUP_IMPLEMENT_SINGLETON statement somewhere in your code.

    @see YUP_IMPLEMENT_SINGLETON, YUP_DECLARE_SINGLETON, YUP_DECLARE_SINGLETON_SINGLETHREADED_MINIMAL
*/
#define YUP_DECLARE_SINGLETON_SINGLETHREADED(Classname, doNotRecreateAfterDeletion) \
    YUP_PRIVATE_DECLARE_SINGLETON (Classname, yup::DummyCriticalSection, doNotRecreateAfterDeletion, , get)

/**
    The same as YUP_DECLARE_SINGLETON_SINGLETHREADED, but does not require a matching
    YUP_IMPLEMENT_SINGLETON definition.
*/
#define YUP_DECLARE_SINGLETON_SINGLETHREADED_INLINE(Classname, doNotRecreateAfterDeletion) \
    YUP_PRIVATE_DECLARE_SINGLETON (Classname, yup::DummyCriticalSection, doNotRecreateAfterDeletion, inline, get)

//==============================================================================
/**
    Macro to declare member variables and methods for a singleton class.

    This is like YUP_DECLARE_SINGLETON_SINGLETHREADED, but doesn't do any checking
    for recursion or repeated instantiation. It's intended for use as a lightweight
    version of a singleton, where you're using it in very straightforward
    circumstances and don't need the extra checking.

    See the documentation for YUP_DECLARE_SINGLETON for more information about
    how to use it. Just like YUP_DECLARE_SINGLETON you need to also have a
    corresponding YUP_IMPLEMENT_SINGLETON statement somewhere in your code.

    @see YUP_IMPLEMENT_SINGLETON, YUP_DECLARE_SINGLETON
*/
#define YUP_DECLARE_SINGLETON_SINGLETHREADED_MINIMAL(Classname) \
    YUP_PRIVATE_DECLARE_SINGLETON (Classname, yup::DummyCriticalSection, false, , getWithoutChecking)

/**
    The same as YUP_DECLARE_SINGLETON_SINGLETHREADED_MINIMAL, but does not require a matching
    YUP_IMPLEMENT_SINGLETON definition.
*/
#define YUP_DECLARE_SINGLETON_SINGLETHREADED_MINIMAL_INLINE(Classname) \
    YUP_PRIVATE_DECLARE_SINGLETON (Classname, yup::DummyCriticalSection, false, inline, getWithoutChecking)

} // namespace yup
