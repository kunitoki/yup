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
    A lightweight cached value for a single DataTree property.

    CachedValue provides fast read access to a DataTree property while
    automatically updating when the property changes. Designed to be as lightweight
    as possible, focusing solely on efficient property caching.

    Features:
    - Fast reads for maximum performance
    - Automatic invalidation when DataTree property changes
    - Support for default values when property doesn't exist
    - Minimal memory footprint

    @tparam T The type of value to cache (must be copy-constructible)
*/
template<typename T>
class YUP_API CachedValue : private DataTree::Listener
{
public:

    //==============================================================================
    /** Creates an unbound CachedValue. */
    CachedValue() = default;

    /** Creates a CachedValue bound to a specific DataTree property. */
    CachedValue (DataTree tree, const Identifier& propertyName)
        : dataTree (tree)
        , propertyName (propertyName)
    {
        setupBinding();
        refresh();
    }

    /** Creates a CachedValue bound to a specific DataTree property with a default value. */
    CachedValue (DataTree tree, const Identifier& propertyName, const T& defaultValue)
        : dataTree (tree)
        , propertyName (propertyName)
        , defaultValue (defaultValue)
        , hasDefaultValue (true)
    {
        setupBinding();
        refresh();
    }

    /** Destructor. */
    ~CachedValue()
    {
        cleanupBinding();
    }

    //==============================================================================
    /** Binds this CachedValue to a DataTree property. */
    void bind (DataTree tree, const Identifier& propertyName)
    {
        cleanupBinding();

        dataTree = tree;
        this->propertyName = propertyName;

        setupBinding();
        refresh();
    }

    /** Binds this CachedValue to a DataTree property with a default value. */
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

    /** Unbinds this CachedValue from its DataTree. */
    void unbind()
    {
        cleanupBinding();

        dataTree = DataTree();
        propertyName = Identifier();
        hasDefaultValue = false;
        usingDefault = false;
        cachedValue = T{};
        defaultValue = T{};
    }

    /** Returns true if this CachedValue is bound to a DataTree property. */
    bool isBound() const noexcept
    {
        return dataTree.isValid() && propertyName.isValid();
    }

    //==============================================================================
    /** Returns the current cached value. */
    T get() const noexcept
    {
        return cachedValue;
    }

    /** Implicit conversion to the cached type for easy access. */
    operator T() const noexcept
    {
        return cachedValue;
    }

    /** Sets the property value in the DataTree using VariantConverter. */
    void set (const T& newValue)
    {
        if (! isBound())
            return;

        try
        {
            var varValue = VariantConverter<T>::toVar (newValue);
            auto transaction = dataTree.beginTransaction ("CachedValue Set");
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
            cachedValue = hasDefaultValue ? defaultValue : T{};
        }
    }

    //==============================================================================
    /** Returns the DataTree this CachedValue is bound to. */
    DataTree getDataTree() const noexcept { return dataTree; }

    /** Returns the property name this CachedValue monitors. */
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
            usingDefault = hasDefaultValue;
            cachedValue = hasDefaultValue ? defaultValue : T{};
            return;
        }

        if (! dataTree.hasProperty (propertyName))
        {
            usingDefault = hasDefaultValue;
            cachedValue = hasDefaultValue ? defaultValue : T{};
            return;
        }

        try
        {
            var propertyValue = dataTree.getProperty (propertyName);
            cachedValue = VariantConverter<T>::fromVar (propertyValue);
            usingDefault = false;
        }
        catch (...)
        {
            usingDefault = hasDefaultValue;
            cachedValue = hasDefaultValue ? defaultValue : T{};
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
    T cachedValue {};
    DataTree dataTree;
    Identifier propertyName;
    T defaultValue {};
    bool hasDefaultValue = false;
    bool usingDefault = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CachedValue)
};

} // namespace yup
