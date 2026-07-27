/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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
    A thread-safe cached value for a single DataTree property using atomic storage.

    AtomicCachedValue provides thread-safe read/write access to a DataTree property
    while automatically updating when the property changes. Uses std::atomic<T> for
    the cached value to ensure thread-safe access.

    Features:
    - Thread-safe atomic reads and writes
    - Automatic invalidation when DataTree property changes
    - Support for default values when property doesn't exist
    - Same API as CachedValue but with atomic guarantees

    @tparam T The type of value to cache atomically (must be atomic-compatible)
*/
template <typename T>
class YUP_API AtomicCachedValue : private DataTree::Listener
{
public:
    //==============================================================================
    /** Creates an unbound AtomicCachedValue. */
    AtomicCachedValue() = default;

    /** Creates an AtomicCachedValue bound to a specific DataTree property. */
    AtomicCachedValue (DataTree tree, const Identifier& propertyName)
        : dataTree (tree)
        , propertyName (propertyName)
    {
        setupBinding();
        refresh();
    }

    /** Creates an AtomicCachedValue bound to a specific DataTree property with a default value. */
    AtomicCachedValue (DataTree tree, const Identifier& propertyName, const T& defaultValue)
        : dataTree (tree)
        , propertyName (propertyName)
        , defaultValue (defaultValue)
        , hasDefaultValue (true)
    {
        setupBinding();
        refresh();
    }

    /** Destructor. */
    ~AtomicCachedValue()
    {
        cleanupBinding();
    }

    //==============================================================================
    /** Binds this AtomicCachedValue to a DataTree property. */
    void bind (DataTree tree, const Identifier& propertyName)
    {
        cleanupBinding();

        dataTree = tree;
        this->propertyName = propertyName;

        setupBinding();
        refresh();
    }

    /** Binds this AtomicCachedValue to a DataTree property with a default value. */
    void bind (DataTree tree, const Identifier& propertyName, const T& defaultValue)
    {
        cleanupBinding();

        dataTree = tree;
        this->propertyName = propertyName;
        this->defaultValue = defaultValue;
        hasDefaultValue = true;

        setupBinding();
        refresh();
    }

    /** Unbinds this AtomicCachedValue from its DataTree. */
    void unbind()
    {
        cleanupBinding();

        dataTree = DataTree();
        propertyName = Identifier();
        hasDefaultValue = false;
        usingDefault = false;
        cachedValue.store (T {});
        defaultValue = T {};
    }

    /** Returns true if this AtomicCachedValue is bound to a DataTree property. */
    bool isBound() const noexcept
    {
        return dataTree.isValid() && propertyName.isValid();
    }

    //==============================================================================
    /** Returns the current cached value atomically. */
    T get() const noexcept
    {
        return cachedValue.load();
    }

    /** Implicit conversion to the cached type for easy access. */
    operator T() const noexcept
    {
        return get();
    }

    /** Sets the property value in the DataTree using VariantConverter. */
    void set (const T& newValue)
    {
        if (! isBound())
            return;

        try
        {
            var varValue = VariantConverter<T>::toVar (newValue);

            auto transaction = dataTree.beginTransaction();
            transaction.setProperty (propertyName, varValue);
        }
        catch (...)
        {
            // If conversion fails, silently ignore
        }
    }

    /** Sets the default value to be used when the property doesn't exist. */
    void setDefault (const T& defaultValue)
    {
        this->defaultValue = defaultValue;
        hasDefaultValue = true;

        // Refresh to update cache and usingDefault flag
        refresh();
    }

    /** Returns the current default value. */
    T getDefault() const noexcept
    {
        return defaultValue;
    }

    /** Returns true if the cached value is using the default (property doesn't exist). */
    bool isUsingDefault() const noexcept
    {
        return usingDefault;
    }

    /** Forces a refresh of the cached value from the DataTree. */
    void refresh()
    {
        if (isBound())
            refreshCacheFromDataTree();
        else
        {
            // Handle unbound case
            usingDefault = hasDefaultValue;
            cachedValue.store (hasDefaultValue ? defaultValue : T {});
        }
    }

    //==============================================================================
    /** Returns the DataTree this AtomicCachedValue is bound to. */
    DataTree getDataTree() const noexcept { return dataTree; }

    /** Returns the property name this AtomicCachedValue monitors. */
    Identifier getPropertyName() const noexcept { return propertyName; }

private:
    //==============================================================================
    // DataTree::Listener implementation
    void propertyChanged (DataTree& tree, const Identifier& property) override
    {
        if (property == propertyName)
            refreshCacheFromDataTree();
    }

    void treeRedirected (DataTree& tree) override
    {
        cleanupBinding();
        dataTree = tree;
        setupBinding();
        refresh();
    }

    //==============================================================================
    void refreshCacheFromDataTree()
    {
        if (! isBound())
        {
            cachedValue.store (hasDefaultValue ? defaultValue : T {});
            usingDefault = hasDefaultValue;
            return;
        }

        if (! dataTree.hasProperty (propertyName))
        {
            cachedValue.store (hasDefaultValue ? defaultValue : T {});
            usingDefault = hasDefaultValue;
            return;
        }

        try
        {
            var propertyValue = dataTree.getProperty (propertyName);
            T newValue = VariantConverter<T>::fromVar (propertyValue);
            cachedValue.store (newValue);
            usingDefault = false;
        }
        catch (...)
        {
            cachedValue.store (hasDefaultValue ? defaultValue : T {});
            usingDefault = hasDefaultValue;
        }
    }

    void setupBinding()
    {
        if (isBound())
            dataTree.addListener (this);
    }

    void cleanupBinding()
    {
        if (isBound())
            dataTree.removeListener (this);
    }

    //==============================================================================
    std::atomic<T> cachedValue {};
    DataTree dataTree;
    Identifier propertyName;
    T defaultValue {};
    bool hasDefaultValue = false;
    bool usingDefault = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AtomicCachedValue)
};

} // namespace yup
