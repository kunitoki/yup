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

class Artboard;
class ArtboardFile;
class ArtboardViewModel;

//==============================================================================
/** Represents a live instance of a Rive ViewModel schema.

    Instances hold the actual property values (numbers, strings, colors, ...)
    that drive the data bindings of a Rive artboard. They are created from an
    ArtboardFile through ArtboardFile::createArtboardViewModelInstance() and are
    then bound to an Artboard via Artboard::bindViewModelInstance().

    Instances are refcounted and can be copied and stored freely. The handle
    keeps its owning ArtboardFile (and the underlying Rive file) alive for as
    long as it is referenced, so property accessors never dangle.

    Property values can be read and written by property name or through dotted
    paths into nested viewmodels and lists, e.g. "score", "player.name" or
    "items.2.quantity". A path may also terminate on a list index ("items.2"),
    which designates the item's viewmodel instance rather than a property value;
    such paths resolve through hasProperty() and getNestedInstance(), but not
    through the value accessors. Property names are the ones authored in the
    Rive editor. Value writes are applied to the artboard data bindings on the
    next Artboard::advanceAndApply().

    A path segment made only of decimal digits is always read as a list index,
    so a property literally named "2" cannot be addressed through a path.

    Instances are not internally synchronized. They are safe to use from the
    message thread and from Component::refreshDisplay (which YUP serialises
    against the message thread), but not concurrently from both.

    @see ArtboardFile::createArtboardViewModelInstance, Artboard::bindViewModelInstance
*/
class YUP_API ArtboardViewModelInstance : public ReferenceCountedObject
{
public:
    //==============================================================================
    using Ptr = ReferenceCountedObjectPtr<ArtboardViewModelInstance>;

    /** Callback invoked when a property value of this instance changes.

        The callback fires synchronously when a value is written through this
        handle or by the artboard (output bindings applied during
        Artboard::advanceAndApply()).

        @param instance The instance whose property changed.
        @param name     The dotted property path that changed.
        @param value    The new value (mapped like ArtboardViewModelInstance::getProperty).
    */
    using PropertyChangedCallback = std::function<void (ArtboardViewModelInstance& instance, const String& name, const var& value)>;

    //==============================================================================
    /** Destructor. */
    ~ArtboardViewModelInstance() override;

    //==============================================================================
    /** Returns the ArtboardFile this instance belongs to, or null if it has been released. */
    ArtboardFile* getArtboardFile() const noexcept;

    /** Returns the name of the ViewModel schema this instance instantiates. */
    String getName() const;

    /** Returns the authored instance name this instance was cloned from, if any. */
    String getInstanceName() const;

    //==============================================================================
    /** Returns true if a property (or nested dotted path) is defined by this instance.

        Index-terminated paths ("items.2") are accepted and report whether the
        list item exists.
    */
    bool hasProperty (StringRef nameOrPath) const;

    //==============================================================================
    /** Reads a property value by name or dotted path.

        Mapping: boolean → var(bool), number → var(double), string → var(String),
        color → var(int64 ARGB), enum → var(String) of the selected option.
        Container values (viewmodels, lists), triggers and index-terminated paths
        return an empty var, as do unknown paths: the two cases are not
        distinguishable through this accessor, use hasProperty() to tell them apart.

        @param nameOrPath The property name or dotted path, e.g. "player.health".
    */
    var getProperty (StringRef nameOrPath) const;

    /** Writes a property value by name or dotted path.

        Accepts the same var representations as getProperty, plus integers for
        number properties and for enum properties either the option name or its
        index. Triggers cannot be written here, use trigger().

        @param nameOrPath The property name or dotted path.
        @param value      The value to write.
        @return True if the path existed and the value was applied.
    */
    bool setProperty (StringRef nameOrPath, const var& value);

    /** Fires a trigger property by name or dotted path.

        @param nameOrPath The name of the trigger property.
        @return True if the path resolved to a trigger and it was fired.
    */
    bool trigger (StringRef nameOrPath);

    //==============================================================================
    /** Reads a boolean property, or nullopt if the path is unknown or not a boolean.

        Every typed getter below collapses "unknown path" and "wrong type" into
        nullopt; use hasProperty() to distinguish them.
    */
    std::optional<bool> getBoolProperty (StringRef nameOrPath) const;

    /** Reads a number property as double, or nullopt if the path is unknown or not a number. */
    std::optional<double> getNumberProperty (StringRef nameOrPath) const;

    /** Reads a string property, or nullopt if the path is unknown or not a string. */
    std::optional<String> getStringProperty (StringRef nameOrPath) const;

    /** Reads a color property, or nullopt if the path is unknown or not a color. */
    std::optional<Color> getColorProperty (StringRef nameOrPath) const;

    /** Reads the selected option of an enum property, or nullopt if the path is unknown. */
    std::optional<String> getEnumProperty (StringRef nameOrPath) const;

    //==============================================================================
    /** Writes a boolean property.

        @return True if the path existed and the value was applied.
    */
    bool setBoolProperty (StringRef nameOrPath, bool value);

    /** Writes a number property.

        @return True if the path existed and the value was applied.
    */
    bool setNumberProperty (StringRef nameOrPath, double value);

    /** Writes a string property.

        @return True if the path existed and the value was applied.
    */
    bool setStringProperty (StringRef nameOrPath, StringRef value);

    /** Writes a color property.

        @return True if the path existed and the value was applied.
    */
    bool setColorProperty (StringRef nameOrPath, const Color& value);

    /** Writes an enum property, accepting the option key or display value.

        @return True if the path existed and the option was found.
    */
    bool setEnumProperty (StringRef nameOrPath, StringRef valueName);

    //==============================================================================
    /** Returns a handle to the nested viewmodel instance behind a viewModel-typed
        property, or behind an index-terminated list path ("items.2"), or null if
        the path is unknown or names something else.

        The returned handle shares the same ArtboardFile and observes the same
        underlying Rive instance as its parent.
    */
    ArtboardViewModelInstance::Ptr getNestedInstance (StringRef nameOrPath) const;

    //==============================================================================
    /** Returns the number of items of a list property, or -1 if the path is unknown
        or does not name a list. */
    int getListSize (StringRef nameOrPath) const;

    /** Returns a handle to the item at the given index of a list property, or null
        if the path does not name a list or the index is out of range. */
    ArtboardViewModelInstance::Ptr getListItem (StringRef nameOrPath, int index) const;

    /** Appends a new item to a list property.

        @param nameOrPath           The dotted path to the list property.
        @param elementViewModelName The name of the ViewModel schema the new item
                                    instantiates (must match the list's element type).
        @return True if the list was found and the item was appended.
    */
    bool addListItem (StringRef nameOrPath, StringRef elementViewModelName);

    /** Inserts a new item at the given index of a list property.

        @param nameOrPath           The dotted path to the list property.
        @param index                The insertion index (0-based, clamped to the list).
        @param elementViewModelName The name of the ViewModel schema the new item
                                    instantiates (must match the list's element type).
        @return True if the list was found and the item was inserted.
    */
    bool addListItemAt (StringRef nameOrPath, int index, StringRef elementViewModelName);

    /** Removes the item at the given index of a list property.

        @return True if the list and index were valid.
    */
    bool removeListItem (StringRef nameOrPath, int index);

    /** Swaps two items of a list property.

        @return True if the list and both indices were valid.
    */
    bool swapListItems (StringRef nameOrPath, int indexA, int indexB);

    /** Removes every item of a list property. */
    void clearListItems (StringRef nameOrPath);

    //==============================================================================
    /** Registers a callback invoked whenever any property of this instance (and its
        nested viewmodels and list items) changes.

        Structural list changes (addListItem, removeListItem, swapListItems,
        clearListItems) also notify, reporting the path of the list itself and an
        empty var as the value.

        Pass an empty callback to remove the listener. Callbacks are delivered
        synchronously on the thread that performed the write; when the write comes
        from the artboard's data bindings, that is the thread running
        Artboard::refreshDisplay, on the thread described above.

        Mutating this instance from within the callback (writing values or
        resizing lists) is supported, as is replacing or clearing the callback
        itself. The last reference to the instance must however not be released
        from inside the callback.

        @param callback The callback to invoke on property changes.
    */
    void setPropertyChangedCallback (PropertyChangedCallback callback);

    //==============================================================================
    /** @internal */
    static Ptr createFromFile (const std::shared_ptr<ArtboardFile>& file, StringRef viewModelName);

    /** @internal */
    static Ptr createFromFile (const std::shared_ptr<ArtboardFile>& file, StringRef viewModelName, StringRef instanceName);

    /** @internal */
    static Ptr createFromRive (const std::shared_ptr<ArtboardFile>& file, rive::ViewModelInstance* riveInstance);

    /** @internal */
    rive::ViewModelInstance* internalRiveInstance() const noexcept;

private:
    ArtboardViewModelInstance (const std::shared_ptr<ArtboardFile>& file, rive::ViewModelInstance* riveInstance);

    struct Impl;

    void enterObserverDispatch() noexcept;
    void leaveObserverDispatch() noexcept;
    bool isObserverDispatchInProgress() const noexcept;

    void notifyValueChanged (rive::ViewModelInstanceValue* riveValue, const String& name);
    void attachObserversToTree (rive::ViewModelInstance* instance, const String& prefix);
    void detachObservers();
    void syncObservers();
    void purgeRetiredObservers();

    std::shared_ptr<ArtboardFile> file;
    std::unique_ptr<Impl> impl;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArtboardViewModelInstance)
};

} // namespace yup
