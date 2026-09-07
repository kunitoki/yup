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
    if (segment.isEmpty() || segment.length() > 6)
        return false;

    for (auto character : segment)
        if (character < '0' || character > '9')
            return false;

    return true;
}

struct Resolved
{
    rive::ViewModelInstance* instance = nullptr;
    rive::ViewModelInstanceValue* value = nullptr;
};

Resolved resolvePath (rive::ViewModelInstance* root, const Array<String>& segments)
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
            auto* list = value != nullptr ? value->as<rive::ViewModelInstanceList>() : nullptr;
            if (list == nullptr)
                return {};

            auto item = list->item (static_cast<uint32> (segment.getIntValue()));
            if (item == nullptr)
                return {};

            current = item->viewModelInstance().get();
            value = nullptr;

            if (current == nullptr)
                return {};

            continue;
        }

        auto* propertyValue = current->propertyValue (segment.toStdString());
        if (propertyValue == nullptr)
            return {};

        if (isLast)
            return { current, propertyValue };

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

var valueToVar (rive::ViewModelInstanceValue* value)
{
    if (value == nullptr)
        return {};

    if (value->is<rive::ViewModelInstanceBoolean>())
        return var (value->as<rive::ViewModelInstanceBoolean>()->propertyValue());

    if (value->is<rive::ViewModelInstanceNumber>())
        return var (static_cast<double> (value->as<rive::ViewModelInstanceNumber>()->propertyValue()));

    if (value->is<rive::ViewModelInstanceString>())
        return var (String (value->as<rive::ViewModelInstanceString>()->propertyValue()));

    if (value->is<rive::ViewModelInstanceColor>())
        return var (static_cast<int64> (static_cast<uint32> (value->as<rive::ViewModelInstanceColor>()->propertyValue())));

    if (value->is<rive::ViewModelInstanceEnum>())
    {
        auto* enumValue = value->as<rive::ViewModelInstanceEnum>();
        auto* enumProperty = value->viewModelProperty() != nullptr
            ? value->viewModelProperty()->as<rive::ViewModelPropertyEnum>()
            : nullptr;

        if (enumProperty != nullptr && enumProperty->dataEnum() != nullptr)
            return var (String (enumProperty->value (enumValue->propertyValue())));

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
    class Observer;

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
            DispatchGuard guard (owner);

            if (active)
                owner.notifyValueChanged (value.get(), path);
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

    class Suppressor
    {
    public:
        explicit Suppressor (ArtboardViewModelInstance& ownerToUse)
            : owner (ownerToUse)
            , wasSuppressing (ownerToUse.impl->suppressNotifications)
        {
            ownerToUse.impl->suppressNotifications = true;
        }

        ~Suppressor()
        {
            owner.impl->suppressNotifications = wasSuppressing;
        }

    private:
        ArtboardViewModelInstance& owner;
        bool wasSuppressing;
    };

    rive::rcp<rive::ViewModelInstance> instance;
    PropertyChangedCallback propertyChangedCallback;
    std::vector<std::unique_ptr<Observer>> observers;
    std::vector<std::unique_ptr<Observer>> retiredObservers;
    bool suppressNotifications = false;
};

//==============================================================================

ArtboardViewModelInstance::ArtboardViewModelInstance (const std::shared_ptr<ArtboardFile>& fileToUse, void* riveInstance)
    : file (fileToUse)
    , impl (std::make_shared<Impl>())
{
    impl->instance = rive::rcp<rive::ViewModelInstance> (static_cast<rive::ViewModelInstance*> (riveInstance));
    syncObservers();
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
    if (impl != nullptr && impl->instance != nullptr)
        if (auto* viewModel = impl->instance->viewModel())
            return String (viewModel->name());

    return {};
}

String ArtboardViewModelInstance::getInstanceName() const
{
    if (impl != nullptr && impl->instance != nullptr)
        return String (impl->instance->name());

    return {};
}

//==============================================================================

bool ArtboardViewModelInstance::hasProperty (StringRef nameOrPath) const noexcept
{
    if (impl == nullptr || impl->instance == nullptr)
        return false;

    const auto segments = splitPath (String (nameOrPath));
    return resolvePath (impl->instance.get(), segments).value != nullptr;
}

//==============================================================================

var ArtboardViewModelInstance::getProperty (StringRef nameOrPath) const
{
    if (impl == nullptr || impl->instance == nullptr)
        return {};

    const auto segments = splitPath (String (nameOrPath));
    return valueToVar (resolvePath (impl->instance.get(), segments).value);
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
    if (impl == nullptr || impl->instance == nullptr)
        return std::nullopt;

    const auto segments = splitPath (String (nameOrPath));
    auto* resolved = resolvePath (impl->instance.get(), segments).value;
    if (resolved == nullptr || ! resolved->is<rive::ViewModelInstanceColor>())
        return std::nullopt;

    return Color (static_cast<uint32> (resolved->as<rive::ViewModelInstanceColor>()->propertyValue()));
}

std::optional<String> ArtboardViewModelInstance::getEnumProperty (StringRef nameOrPath) const
{
    if (impl == nullptr || impl->instance == nullptr)
        return std::nullopt;

    const auto segments = splitPath (String (nameOrPath));
    auto* resolved = resolvePath (impl->instance.get(), segments).value;
    if (resolved == nullptr || ! resolved->is<rive::ViewModelInstanceEnum>())
        return std::nullopt;

    auto* enumProperty = resolved->viewModelProperty() != nullptr
        ? resolved->viewModelProperty()->as<rive::ViewModelPropertyEnum>()
        : nullptr;

    if (enumProperty == nullptr || enumProperty->dataEnum() == nullptr)
        return std::nullopt;

    return String (enumProperty->value (resolved->as<rive::ViewModelInstanceEnum>()->propertyValue()));
}

//==============================================================================

bool ArtboardViewModelInstance::setProperty (StringRef nameOrPath, const var& value)
{
    if (impl == nullptr || impl->instance == nullptr)
        return false;

    const auto segments = splitPath (String (nameOrPath));
    auto* resolved = resolvePath (impl->instance.get(), segments).value;
    if (resolved == nullptr)
        return false;

    if (resolved->is<rive::ViewModelInstanceBoolean>())
    {
        if (! value.isBool())
            return false;

        resolved->as<rive::ViewModelInstanceBoolean>()->propertyValue (static_cast<bool> (value));
        return true;
    }

    if (resolved->is<rive::ViewModelInstanceNumber>())
    {
        if (! (value.isDouble() || value.isInt() || value.isInt64()))
            return false;

        resolved->as<rive::ViewModelInstanceNumber>()->propertyValue (static_cast<float> (static_cast<double> (value)));
        return true;
    }

    if (resolved->is<rive::ViewModelInstanceString>())
    {
        if (! value.isString())
            return false;

        resolved->as<rive::ViewModelInstanceString>()->propertyValue (value.toString().toStdString());
        return true;
    }

    if (resolved->is<rive::ViewModelInstanceColor>())
    {
        if (! (value.isInt() || value.isInt64()))
            return false;

        const auto argb = value.isInt64() ? static_cast<uint32> (static_cast<int64> (value))
                                          : static_cast<uint32> (static_cast<int> (value));

        resolved->as<rive::ViewModelInstanceColor>()->propertyValue (static_cast<int> (argb));
        return true;
    }

    if (resolved->is<rive::ViewModelInstanceEnum>())
    {
        auto* enumProperty = resolved->viewModelProperty() != nullptr
            ? resolved->viewModelProperty()->as<rive::ViewModelPropertyEnum>()
            : nullptr;

        int index = -1;
        if (value.isString())
            index = enumIndexForName (enumProperty, value.toString());
        else if (value.isInt())
            index = static_cast<int> (value);
        else if (value.isInt64())
            index = static_cast<int> (static_cast<int64> (value));

        if (index < 0)
            return false;

        return resolved->as<rive::ViewModelInstanceEnum>()->value (static_cast<uint32> (index));
    }

    return false;
}

bool ArtboardViewModelInstance::trigger (StringRef nameOrPath)
{
    if (impl == nullptr || impl->instance == nullptr)
        return false;

    const auto segments = splitPath (String (nameOrPath));
    auto* resolved = resolvePath (impl->instance.get(), segments).value;
    if (resolved == nullptr || ! resolved->is<rive::ViewModelInstanceTrigger>())
        return false;

    resolved->as<rive::ViewModelInstanceTrigger>()->trigger();
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
    if (impl == nullptr || impl->instance == nullptr)
        return false;

    const auto segments = splitPath (String (nameOrPath));
    auto* resolved = resolvePath (impl->instance.get(), segments).value;
    if (resolved == nullptr || ! resolved->is<rive::ViewModelInstanceColor>())
        return false;

    resolved->as<rive::ViewModelInstanceColor>()->propertyValue (static_cast<int> (value.getARGB()));
    return true;
}

bool ArtboardViewModelInstance::setEnumProperty (StringRef nameOrPath, StringRef valueName)
{
    if (impl == nullptr || impl->instance == nullptr)
        return false;

    const auto segments = splitPath (String (nameOrPath));
    auto* resolved = resolvePath (impl->instance.get(), segments).value;
    if (resolved == nullptr || ! resolved->is<rive::ViewModelInstanceEnum>())
        return false;

    auto* enumProperty = resolved->viewModelProperty() != nullptr
        ? resolved->viewModelProperty()->as<rive::ViewModelPropertyEnum>()
        : nullptr;

    const int index = enumIndexForName (enumProperty, String (valueName));
    if (index < 0)
        return false;

    return resolved->as<rive::ViewModelInstanceEnum>()->value (static_cast<uint32> (index));
}

//==============================================================================

ArtboardViewModelInstance::Ptr ArtboardViewModelInstance::getNestedInstance (StringRef nameOrPath) const
{
    if (impl == nullptr || impl->instance == nullptr)
        return nullptr;

    const auto segments = splitPath (String (nameOrPath));
    auto* resolved = resolvePath (impl->instance.get(), segments).value;
    if (resolved == nullptr || ! resolved->is<rive::ViewModelInstanceViewModel>())
        return nullptr;

    auto nested = resolved->as<rive::ViewModelInstanceViewModel>()->referenceViewModelInstance();
    if (nested == nullptr)
        return nullptr;

    return createFromRive (file, nested.get());
}

//==============================================================================

int ArtboardViewModelInstance::getListSize (StringRef nameOrPath) const
{
    if (impl == nullptr || impl->instance == nullptr)
        return -1;

    const auto segments = splitPath (String (nameOrPath));
    auto* resolved = resolvePath (impl->instance.get(), segments).value;
    if (resolved == nullptr || ! resolved->is<rive::ViewModelInstanceList>())
        return -1;

    return static_cast<int> (resolved->as<rive::ViewModelInstanceList>()->listItems().size());
}

ArtboardViewModelInstance::Ptr ArtboardViewModelInstance::getListItem (StringRef nameOrPath, int index) const
{
    if (impl == nullptr || impl->instance == nullptr || index < 0)
        return nullptr;

    const auto segments = splitPath (String (nameOrPath));
    auto* resolved = resolvePath (impl->instance.get(), segments).value;
    if (resolved == nullptr || ! resolved->is<rive::ViewModelInstanceList>())
        return nullptr;

    auto* list = resolved->as<rive::ViewModelInstanceList>();
    if (static_cast<size_t> (index) >= list->listItems().size())
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
    const int size = getListSize (nameOrPath);
    return size >= 0 && addListItemAt (nameOrPath, size, elementViewModelName);
}

bool ArtboardViewModelInstance::addListItemAt (StringRef nameOrPath, int index, StringRef elementViewModelName)
{
    if (impl == nullptr || impl->instance == nullptr || file == nullptr)
        return false;

    const auto segments = splitPath (String (nameOrPath));
    auto* resolved = resolvePath (impl->instance.get(), segments).value;
    if (resolved == nullptr || ! resolved->is<rive::ViewModelInstanceList>())
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

    auto* list = resolved->as<rive::ViewModelInstanceList>();

    const auto size = list->listItems().size();
    const auto clampedIndex = static_cast<size_t> (jlimit (0, static_cast<int> (size), index));

    Impl::Suppressor suppress (*this);
    const bool success = list->addItemAt (rive::rcp<rive::ViewModelInstanceListItem> (rawItem),
                                          static_cast<int> (clampedIndex));
    syncObservers();
    return success;
}

bool ArtboardViewModelInstance::removeListItem (StringRef nameOrPath, int index)
{
    if (impl == nullptr || impl->instance == nullptr || index < 0)
        return false;

    const auto segments = splitPath (String (nameOrPath));
    auto* resolved = resolvePath (impl->instance.get(), segments).value;
    if (resolved == nullptr || ! resolved->is<rive::ViewModelInstanceList>())
        return false;

    auto* list = resolved->as<rive::ViewModelInstanceList>();
    if (static_cast<size_t> (index) >= list->listItems().size())
        return false;

    Impl::Suppressor suppress (*this);
    list->removeItem (index);
    syncObservers();
    return true;
}

bool ArtboardViewModelInstance::swapListItems (StringRef nameOrPath, int indexA, int indexB)
{
    if (impl == nullptr || impl->instance == nullptr || indexA < 0 || indexB < 0)
        return false;

    const auto segments = splitPath (String (nameOrPath));
    auto* resolved = resolvePath (impl->instance.get(), segments).value;
    if (resolved == nullptr || ! resolved->is<rive::ViewModelInstanceList>())
        return false;

    auto* list = resolved->as<rive::ViewModelInstanceList>();
    const auto size = list->listItems().size();
    if (static_cast<size_t> (indexA) >= size || static_cast<size_t> (indexB) >= size)
        return false;

    Impl::Suppressor suppress (*this);
    list->swap (static_cast<uint32> (indexA), static_cast<uint32> (indexB));
    syncObservers();
    return true;
}

void ArtboardViewModelInstance::clearListItems (StringRef nameOrPath)
{
    if (impl == nullptr || impl->instance == nullptr)
        return;

    const auto segments = splitPath (String (nameOrPath));
    auto* resolved = resolvePath (impl->instance.get(), segments).value;
    if (resolved == nullptr || ! resolved->is<rive::ViewModelInstanceList>())
        return;

    Impl::Suppressor suppress (*this);
    resolved->as<rive::ViewModelInstanceList>()->removeAllItems();
    syncObservers();
}

//==============================================================================

void ArtboardViewModelInstance::setPropertyChangedCallback (PropertyChangedCallback callback)
{
    if (impl == nullptr)
        return;

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

ArtboardViewModelInstance::Ptr ArtboardViewModelInstance::createFromRive (const std::shared_ptr<ArtboardFile>& fileToUse, void* riveInstance)
{
    if (fileToUse == nullptr || riveInstance == nullptr)
        return nullptr;

    auto* instance = static_cast<rive::ViewModelInstance*> (riveInstance);
    instance->ref();

    return Ptr (new ArtboardViewModelInstance (fileToUse, instance));
}

void* ArtboardViewModelInstance::internalRiveInstance() const noexcept
{
    return impl != nullptr && impl->instance != nullptr ? impl->instance.get() : nullptr;
}

//==============================================================================

void ArtboardViewModelInstance::notifyValueChanged (void* riveValue, const String& name)
{
    if (impl == nullptr || impl->suppressNotifications || ! impl->propertyChangedCallback)
        return;

    impl->propertyChangedCallback (*this, name, valueToVar (static_cast<rive::ViewModelInstanceValue*> (riveValue)));
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

void ArtboardViewModelInstance::attachObserversToTree (void* instanceToObserve, const String& prefix)
{
    if (impl == nullptr)
        return;

    // Viewmodel instance graphs may be cyclic (a nested viewmodel referencing
    // back into an ancestor), so guard the recursion with a visited set.
    std::unordered_set<rive::ViewModelInstance*> visited;

    std::function<void (rive::ViewModelInstance*, const String&)> attach =
        [&] (rive::ViewModelInstance* observedInstance, const String& pathPrefix)
    {
        if (observedInstance == nullptr || ! visited.insert (observedInstance).second)
            return;

        for (auto& propertyValue : observedInstance->propertyValues())
        {
            if (propertyValue == nullptr)
                continue;

            auto* value = propertyValue.get();

            auto* property = value->viewModelProperty();
            if (property == nullptr)
                continue;

            const String path = pathPrefix.isEmpty()
                ? String (property->name())
                : pathPrefix + "." + String (property->name());

            impl->observers.push_back (std::make_unique<Impl::Observer> (*this, propertyValue, path));

            if (value->is<rive::ViewModelInstanceViewModel>())
            {
                auto nested = value->as<rive::ViewModelInstanceViewModel>()->referenceViewModelInstance();
                if (nested != nullptr)
                    attach (nested.get(), path);
            }
            else if (value->is<rive::ViewModelInstanceList>())
            {
                auto* list = value->as<rive::ViewModelInstanceList>();
                const auto items = list->listItems();

                int index = 0;
                for (auto& item : items)
                {
                    auto itemInstance = item->viewModelInstance();
                    if (itemInstance != nullptr)
                        attach (itemInstance.get(), path + "." + String (index));

                    ++index;
                }
            }
        }
    };

    attach (static_cast<rive::ViewModelInstance*> (instanceToObserve), prefix);
}

void ArtboardViewModelInstance::detachObservers()
{
    if (impl == nullptr)
        return;

    for (auto& observer : impl->observers)
        if (observer != nullptr)
            observer->detach();

    for (auto& observer : impl->observers)
        if (observer != nullptr)
            impl->retiredObservers.push_back (std::move (observer));

    impl->observers.clear();

    purgeRetiredObservers();
}

void ArtboardViewModelInstance::purgeRetiredObservers()
{
    if (impl == nullptr)
        return;

    if (isObserverDispatchInProgress())
        return;

    impl->retiredObservers.clear();
}

void ArtboardViewModelInstance::syncObservers()
{
    detachObservers();

    if (impl == nullptr || impl->instance == nullptr || ! impl->propertyChangedCallback)
        return;

    attachObserversToTree (impl->instance.get(), {});
}

} // namespace yup
