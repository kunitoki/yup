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

namespace
{

//==============================================================================

Array<String> splitPath (String path)
{
    Array<String> segments;

    for (;;)
    {
        const int dot = path.indexOfChar ('.');
        if (dot < 0)
        {
            segments.add (path);
            return segments;
        }

        segments.add (path.substring (0, dot));
        path = path.substring (dot + 1);
    }
}

bool isIndexSegment (const String& segment)
{
    if (segment.isEmpty())
        return false;

    for (auto character : segment)
        if (character < '0' || character > '9')
            return false;

    return true;
}

/** What a dotted path resolved to.

    A path either terminates on a property value ("player.health") or, when its
    last segment is a list index ("items.2"), on the item's viewmodel instance.
    At most one of the two members is ever set.
*/
struct ResolvedPath
{
    rive::ViewModelInstanceValue* value = nullptr;
    rive::ViewModelInstance* instance = nullptr;
};

ResolvedPath resolvePath (rive::ViewModelInstance* root, const Array<String>& segments)
{
    if (root == nullptr || segments.isEmpty())
        return {};

    auto* current = root;
    rive::ViewModelInstanceValue* value = nullptr;

    for (int i = 0; i < segments.size(); ++i)
    {
        const auto& segment = segments.getReference (i);
        const bool isLast = (i == segments.size() - 1);

        if (isIndexSegment (segment))
        {
            // An index only addresses something when it follows a list property.
            // rive::Core::as<> only asserts in debug builds, so check first.
            if (value == nullptr || ! value->is<rive::ViewModelInstanceList>())
                return {};

            auto item = value->as<rive::ViewModelInstanceList>()->item (static_cast<uint32> (segment.getIntValue()));
            if (item == nullptr)
                return {};

            current = item->viewModelInstance().get();
            value = nullptr;

            if (current == nullptr)
                return {};

            if (isLast)
                return { nullptr, current };

            continue;
        }

        auto* propertyValue = current->propertyValue (segment.toStdString());
        if (propertyValue == nullptr)
            return {};

        if (isLast)
            return { propertyValue, nullptr };

        if (propertyValue->is<rive::ViewModelInstanceViewModel>())
        {
            auto nested = propertyValue->as<rive::ViewModelInstanceViewModel>()->referenceViewModelInstance();
            if (nested == nullptr)
                return {};

            current = nested.get();
            value = nullptr;
        }
        else
        {
            value = propertyValue;
        }
    }

    return {};
}

ResolvedPath resolve (const rive::rcp<rive::ViewModelInstance>& root, StringRef nameOrPath)
{
    return resolvePath (root.get(), splitPath (String (nameOrPath)));
}

template <class T>
T* asValue (rive::ViewModelInstanceValue* value)
{
    return value != nullptr && value->is<T>() ? value->as<T>() : nullptr;
}

template <class T>
T* resolveAs (const rive::rcp<rive::ViewModelInstance>& root, StringRef nameOrPath)
{
    return asValue<T> (resolve (root, nameOrPath).value);
}

rive::ViewModelPropertyEnum* enumPropertyOf (rive::ViewModelInstanceValue* value)
{
    if (value == nullptr)
        return nullptr;

    auto* property = value->viewModelProperty();

    return property != nullptr && property->is<rive::ViewModelPropertyEnum>()
             ? property->as<rive::ViewModelPropertyEnum>()
             : nullptr;
}

std::optional<String> enumValueNameOf (rive::ViewModelInstanceEnum* enumValue)
{
    auto* enumProperty = enumPropertyOf (enumValue);
    if (enumProperty == nullptr || enumProperty->dataEnum() == nullptr)
        return std::nullopt;

    return String (enumProperty->value (enumValue->propertyValue()));
}

var valueToVar (rive::ViewModelInstanceValue* value)
{
    if (value == nullptr)
        return {};

    if (auto* boolean = asValue<rive::ViewModelInstanceBoolean> (value))
        return var (boolean->propertyValue());

    if (auto* number = asValue<rive::ViewModelInstanceNumber> (value))
        return var (static_cast<double> (number->propertyValue()));

    if (auto* string = asValue<rive::ViewModelInstanceString> (value))
        return var (String (string->propertyValue()));

    if (auto* color = asValue<rive::ViewModelInstanceColor> (value))
        return var (static_cast<int64> (static_cast<uint32> (color->propertyValue())));

    if (auto* enumValue = asValue<rive::ViewModelInstanceEnum> (value))
    {
        if (const auto name = enumValueNameOf (enumValue))
            return var (*name);

        return {};
    }

    return {};
}

int enumIndexForName (rive::ViewModelPropertyEnum* enumProperty, const String& name)
{
    if (enumProperty == nullptr || enumProperty->dataEnum() == nullptr)
        return -1;

    const std::string key = name.toStdString();

    int index = enumProperty->valueIndex (key);
    if (index != -1)
        return index;

    int fallback = 0;
    for (auto* enumValue : enumProperty->dataEnum()->values())
    {
        const auto& value = enumValue->value();
        const auto& candidate = value.empty() ? enumValue->key() : value;
        if (candidate == key)
            return fallback;

        ++fallback;
    }

    return -1;
}

} // namespace

//==============================================================================

struct ArtboardViewModelInstance::Impl
{
    class DispatchGuard
    {
    public:
        explicit DispatchGuard (ArtboardViewModelInstance& ownerToUse)
            : owner (ownerToUse)
        {
            owner.enterObserverDispatch();
        }

        ~DispatchGuard()
        {
            owner.leaveObserverDispatch();
        }

    private:
        ArtboardViewModelInstance& owner;
    };

    class Observer : public rive::ViewModelInstanceValueDelegate
    {
    public:
        Observer (ArtboardViewModelInstance& ownerToUse,
                  const rive::rcp<rive::ViewModelInstanceValue>& valueToWatch,
                  String pathToUse)
            : owner (ownerToUse)
            , value (valueToWatch)
            , path (std::move (pathToUse))
        {
            value->addDelegate (this);
        }

        ~Observer()
        {
            removeFromValue();
        }

        void valueChanged() override
        {
            {
                DispatchGuard guard (owner);

                if (active)
                    owner.notifyValueChanged (value.get(), path);
            }

            // The callback may have rebuilt the observer tree, retiring observers
            // that could not be freed while the dispatch was unwinding. Only purge
            // while this observer is still live, otherwise it would free itself.
            if (active)
                owner.purgeRetiredObservers();
        }

        void detach()
        {
            active = false;
            removeFromValue();
        }

    private:
        void removeFromValue()
        {
            if (registered && value != nullptr)
            {
                value->removeDelegate (this);
                registered = false;
            }
        }

        ArtboardViewModelInstance& owner;
        rive::rcp<rive::ViewModelInstanceValue> value;
        String path;
        bool active = true;
        bool registered = true;
    };

    rive::rcp<rive::ViewModelInstance> instance;
    PropertyChangedCallback propertyChangedCallback;
    std::vector<std::unique_ptr<Observer>> observers;
    std::vector<std::unique_ptr<Observer>> retiredObservers;

    // Viewmodel instance graphs may be cyclic (a nested viewmodel referencing back
    // into an ancestor), so remember what is already observed to bound the walk and
    // to keep incremental attachments from observing the same value twice.
    std::unordered_set<rive::ViewModelInstance*> observedInstances;
};

//==============================================================================

ArtboardViewModelInstance::ArtboardViewModelInstance (const std::shared_ptr<ArtboardFile>& fileToUse, rive::ViewModelInstance* riveInstance)
    : file (fileToUse)
    , impl (std::make_unique<Impl>())
{
    impl->instance = rive::rcp<rive::ViewModelInstance> (riveInstance);
}

ArtboardViewModelInstance::~ArtboardViewModelInstance()
{
    detachObservers();
}

//==============================================================================

ArtboardFile* ArtboardViewModelInstance::getArtboardFile() const noexcept
{
    return file.get();
}

String ArtboardViewModelInstance::getName() const
{
    if (impl->instance != nullptr)
        if (auto* viewModel = impl->instance->viewModel())
            return String (viewModel->name());

    return {};
}

String ArtboardViewModelInstance::getInstanceName() const
{
    if (impl->instance != nullptr)
        return String (impl->instance->name());

    return {};
}

//==============================================================================

bool ArtboardViewModelInstance::hasProperty (StringRef nameOrPath) const
{
    const auto resolved = resolve (impl->instance, nameOrPath);

    return resolved.value != nullptr || resolved.instance != nullptr;
}

//==============================================================================

var ArtboardViewModelInstance::getProperty (StringRef nameOrPath) const
{
    return valueToVar (resolve (impl->instance, nameOrPath).value);
}

std::optional<bool> ArtboardViewModelInstance::getBoolProperty (StringRef nameOrPath) const
{
    const auto value = getProperty (nameOrPath);
    return value.isBool() ? std::optional<bool> (static_cast<bool> (value)) : std::nullopt;
}

std::optional<double> ArtboardViewModelInstance::getNumberProperty (StringRef nameOrPath) const
{
    const auto value = getProperty (nameOrPath);
    if (value.isDouble())
        return static_cast<double> (value);

    if (value.isInt())
        return static_cast<double> (static_cast<int> (value));

    if (value.isInt64())
        return static_cast<double> (static_cast<int64> (value));

    return std::nullopt;
}

std::optional<String> ArtboardViewModelInstance::getStringProperty (StringRef nameOrPath) const
{
    const auto value = getProperty (nameOrPath);
    return value.isString() ? std::optional<String> (value.toString()) : std::nullopt;
}

std::optional<Color> ArtboardViewModelInstance::getColorProperty (StringRef nameOrPath) const
{
    if (auto* color = resolveAs<rive::ViewModelInstanceColor> (impl->instance, nameOrPath))
        return Color (static_cast<uint32> (color->propertyValue()));

    return std::nullopt;
}

std::optional<String> ArtboardViewModelInstance::getEnumProperty (StringRef nameOrPath) const
{
    auto* enumValue = resolveAs<rive::ViewModelInstanceEnum> (impl->instance, nameOrPath);
    if (enumValue == nullptr)
        return std::nullopt;

    return enumValueNameOf (enumValue);
}

//==============================================================================

bool ArtboardViewModelInstance::setProperty (StringRef nameOrPath, const var& value)
{
    auto* resolved = resolve (impl->instance, nameOrPath).value;
    if (resolved == nullptr)
        return false;

    if (auto* boolean = asValue<rive::ViewModelInstanceBoolean> (resolved))
    {
        if (! value.isBool())
            return false;

        boolean->propertyValue (static_cast<bool> (value));
        return true;
    }

    if (auto* number = asValue<rive::ViewModelInstanceNumber> (resolved))
    {
        if (! (value.isDouble() || value.isInt() || value.isInt64()))
            return false;

        number->propertyValue (static_cast<float> (static_cast<double> (value)));
        return true;
    }

    if (auto* string = asValue<rive::ViewModelInstanceString> (resolved))
    {
        if (! value.isString())
            return false;

        string->propertyValue (value.toString().toStdString());
        return true;
    }

    if (auto* color = asValue<rive::ViewModelInstanceColor> (resolved))
    {
        if (! (value.isInt() || value.isInt64()))
            return false;

        const auto argb = value.isInt64() ? static_cast<uint32> (static_cast<int64> (value))
                                          : static_cast<uint32> (static_cast<int> (value));

        color->propertyValue (static_cast<int> (argb));
        return true;
    }

    if (auto* enumValue = asValue<rive::ViewModelInstanceEnum> (resolved))
    {
        int index = -1;
        if (value.isString())
            index = enumIndexForName (enumPropertyOf (resolved), value.toString());
        else if (value.isInt())
            index = static_cast<int> (value);
        else if (value.isInt64())
            index = static_cast<int> (static_cast<int64> (value));

        if (index < 0)
            return false;

        return enumValue->value (static_cast<uint32> (index));
    }

    return false;
}

bool ArtboardViewModelInstance::trigger (StringRef nameOrPath)
{
    auto* resolved = resolveAs<rive::ViewModelInstanceTrigger> (impl->instance, nameOrPath);
    if (resolved == nullptr)
        return false;

    resolved->trigger();
    return true;
}

//==============================================================================

bool ArtboardViewModelInstance::setBoolProperty (StringRef nameOrPath, bool value)
{
    return setProperty (nameOrPath, var (value));
}

bool ArtboardViewModelInstance::setNumberProperty (StringRef nameOrPath, double value)
{
    return setProperty (nameOrPath, var (value));
}

bool ArtboardViewModelInstance::setStringProperty (StringRef nameOrPath, StringRef value)
{
    return setProperty (nameOrPath, var (String (value)));
}

bool ArtboardViewModelInstance::setColorProperty (StringRef nameOrPath, const Color& value)
{
    return setProperty (nameOrPath, var (static_cast<int64> (value.getARGB())));
}

bool ArtboardViewModelInstance::setEnumProperty (StringRef nameOrPath, StringRef valueName)
{
    return setProperty (nameOrPath, var (String (valueName)));
}

//==============================================================================

ArtboardViewModelInstance::Ptr ArtboardViewModelInstance::getNestedInstance (StringRef nameOrPath) const
{
    const auto resolved = resolve (impl->instance, nameOrPath);

    if (resolved.instance != nullptr)
        return createFromRive (file, resolved.instance);

    if (auto* nestedValue = asValue<rive::ViewModelInstanceViewModel> (resolved.value))
        if (auto nested = nestedValue->referenceViewModelInstance())
            return createFromRive (file, nested.get());

    return nullptr;
}

//==============================================================================

int ArtboardViewModelInstance::getListSize (StringRef nameOrPath) const
{
    auto* list = resolveAs<rive::ViewModelInstanceList> (impl->instance, nameOrPath);
    if (list == nullptr)
        return -1;

    return static_cast<int> (list->listItems().size());
}

ArtboardViewModelInstance::Ptr ArtboardViewModelInstance::getListItem (StringRef nameOrPath, int index) const
{
    if (index < 0)
        return nullptr;

    auto* list = resolveAs<rive::ViewModelInstanceList> (impl->instance, nameOrPath);
    if (list == nullptr || static_cast<size_t> (index) >= list->listItems().size())
        return nullptr;

    auto item = list->item (static_cast<uint32> (index));
    if (item == nullptr)
        return nullptr;

    auto itemInstance = item->viewModelInstance();
    if (itemInstance == nullptr)
        return nullptr;

    return createFromRive (file, itemInstance.get());
}

bool ArtboardViewModelInstance::addListItem (StringRef nameOrPath, StringRef elementViewModelName)
{
    auto* list = resolveAs<rive::ViewModelInstanceList> (impl->instance, nameOrPath);
    if (list == nullptr)
        return false;

    return addListItemAt (nameOrPath, static_cast<int> (list->listItems().size()), elementViewModelName);
}

bool ArtboardViewModelInstance::addListItemAt (StringRef nameOrPath, int index, StringRef elementViewModelName)
{
    if (file == nullptr)
        return false;

    auto* list = resolveAs<rive::ViewModelInstanceList> (impl->instance, nameOrPath);
    if (list == nullptr)
        return false;

    auto* rivFile = file->getRiveFile();
    if (rivFile == nullptr)
        return false;

    auto itemInstance = rivFile->createViewModelInstance (String (elementViewModelName).toStdString());
    if (itemInstance == nullptr)
        return false;

    auto* rawItem = rivFile->viewModelInstanceListItem (itemInstance, nullptr);
    if (rawItem == nullptr)
        return false;

    const auto size = static_cast<int> (list->listItems().size());
    const auto clampedIndex = jlimit (0, size, index);

    if (! list->addItemAt (rive::rcp<rive::ViewModelInstanceListItem> (rawItem), clampedIndex))
        return false;

    // Appending leaves the indices of the existing items (and therefore the paths
    // their observers were built with) untouched, so only the new subtree needs
    // observing. Any other insertion shifts them and forces a full rebuild.
    if (clampedIndex == size)
        attachObserversToTree (rawItem->viewModelInstance().get(),
                               String (nameOrPath) + "." + String (clampedIndex));
    else
        syncObservers();

    return true;
}

bool ArtboardViewModelInstance::removeListItem (StringRef nameOrPath, int index)
{
    if (index < 0)
        return false;

    auto* list = resolveAs<rive::ViewModelInstanceList> (impl->instance, nameOrPath);
    if (list == nullptr || static_cast<size_t> (index) >= list->listItems().size())
        return false;

    list->removeItem (index);
    syncObservers();
    return true;
}

bool ArtboardViewModelInstance::swapListItems (StringRef nameOrPath, int indexA, int indexB)
{
    if (indexA < 0 || indexB < 0)
        return false;

    auto* list = resolveAs<rive::ViewModelInstanceList> (impl->instance, nameOrPath);
    if (list == nullptr)
        return false;

    const auto size = list->listItems().size();
    if (static_cast<size_t> (indexA) >= size || static_cast<size_t> (indexB) >= size)
        return false;

    list->swap (static_cast<uint32> (indexA), static_cast<uint32> (indexB));
    syncObservers();
    return true;
}

void ArtboardViewModelInstance::clearListItems (StringRef nameOrPath)
{
    auto* list = resolveAs<rive::ViewModelInstanceList> (impl->instance, nameOrPath);
    if (list == nullptr)
        return;

    list->removeAllItems();
    syncObservers();
}

//==============================================================================

void ArtboardViewModelInstance::setPropertyChangedCallback (PropertyChangedCallback callback)
{
    impl->propertyChangedCallback = std::move (callback);

    // (Re)attach the value observers to cover the new callback (or drop them when clearing).
    syncObservers();
}

//==============================================================================

ArtboardViewModelInstance::Ptr ArtboardViewModelInstance::createFromFile (const std::shared_ptr<ArtboardFile>& fileToUse, StringRef viewModelName)
{
    if (fileToUse == nullptr)
        return nullptr;

    auto* rivFile = fileToUse->getRiveFile();
    if (rivFile == nullptr)
        return nullptr;

    auto instance = rivFile->createViewModelInstance (String (viewModelName).toStdString());
    if (instance == nullptr)
        return nullptr;

    // release() hands over the reference the rcp already holds, matching the
    // adopting rive::rcp the constructor builds.
    return Ptr (new ArtboardViewModelInstance (fileToUse, instance.release()));
}

ArtboardViewModelInstance::Ptr ArtboardViewModelInstance::createFromFile (const std::shared_ptr<ArtboardFile>& fileToUse, StringRef viewModelName, StringRef instanceName)
{
    if (fileToUse == nullptr)
        return nullptr;

    auto* rivFile = fileToUse->getRiveFile();
    if (rivFile == nullptr)
        return nullptr;

    auto instance = rivFile->createViewModelInstance (String (viewModelName).toStdString(),
                                                      String (instanceName).toStdString());
    if (instance == nullptr)
        return nullptr;

    return Ptr (new ArtboardViewModelInstance (fileToUse, instance.release()));
}

ArtboardViewModelInstance::Ptr ArtboardViewModelInstance::createFromRive (const std::shared_ptr<ArtboardFile>& fileToUse, rive::ViewModelInstance* riveInstance)
{
    if (fileToUse == nullptr || riveInstance == nullptr)
        return nullptr;

    // rive::rcp's pointer constructor adopts without reffing, so take the extra
    // reference here: this handle observes an instance it does not own.
    riveInstance->ref();

    return Ptr (new ArtboardViewModelInstance (fileToUse, riveInstance));
}

rive::ViewModelInstance* ArtboardViewModelInstance::internalRiveInstance() const noexcept
{
    return impl->instance.get();
}

//==============================================================================

void ArtboardViewModelInstance::notifyValueChanged (rive::ViewModelInstanceValue* riveValue, const String& name)
{
    if (! impl->propertyChangedCallback)
        return;

    // The callback is free to replace or clear itself, which would destroy the
    // closure mid-call, so run a copy that outlives the assignment.
    auto callback = impl->propertyChangedCallback;
    callback (*this, name, valueToVar (riveValue));
}

//==============================================================================

void ArtboardViewModelInstance::enterObserverDispatch() noexcept
{
    if (file != nullptr)
        file->enterObserverDispatch();
}

void ArtboardViewModelInstance::leaveObserverDispatch() noexcept
{
    if (file != nullptr)
        file->leaveObserverDispatch();
}

bool ArtboardViewModelInstance::isObserverDispatchInProgress() const noexcept
{
    return file != nullptr && file->isObserverDispatchInProgress();
}

//==============================================================================

void ArtboardViewModelInstance::attachObserversToTree (rive::ViewModelInstance* instanceToObserve, const String& prefix)
{
    if (instanceToObserve == nullptr
        || ! impl->propertyChangedCallback
        || ! impl->observedInstances.insert (instanceToObserve).second)
        return;

    for (auto& propertyValue : instanceToObserve->propertyValues())
    {
        if (propertyValue == nullptr)
            continue;

        auto* value = propertyValue.get();

        auto* property = value->viewModelProperty();
        if (property == nullptr)
            continue;

        const String path = prefix.isEmpty()
                              ? String (property->name())
                              : prefix + "." + String (property->name());

        impl->observers.push_back (std::make_unique<Impl::Observer> (*this, propertyValue, path));

        if (auto* nestedValue = asValue<rive::ViewModelInstanceViewModel> (value))
        {
            if (auto nested = nestedValue->referenceViewModelInstance())
                attachObserversToTree (nested.get(), path);
        }
        else if (auto* list = asValue<rive::ViewModelInstanceList> (value))
        {
            int index = 0;
            for (auto& item : list->listItems())
            {
                attachObserversToTree (item->viewModelInstance().get(), path + "." + String (index));
                ++index;
            }
        }
    }
}

void ArtboardViewModelInstance::detachObservers()
{
    for (auto& observer : impl->observers)
        if (observer != nullptr)
        {
            observer->detach();
            impl->retiredObservers.push_back (std::move (observer));
        }

    impl->observers.clear();
    impl->observedInstances.clear();

    purgeRetiredObservers();
}

void ArtboardViewModelInstance::purgeRetiredObservers()
{
    // A retired observer may be the one currently dispatching, so it can only be
    // freed once every dispatch on the owning file has unwound.
    if (isObserverDispatchInProgress())
        return;

    impl->retiredObservers.clear();
}

void ArtboardViewModelInstance::syncObservers()
{
    detachObservers();

    attachObserversToTree (impl->instance.get(), {});
}

} // namespace yup
