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

ArtboardViewModel::PropertyType toPropertyType (rive::ViewModelProperty* property)
{
    if (property->is<rive::ViewModelPropertyBoolean>())
        return ArtboardViewModel::PropertyType::boolean;

    if (property->is<rive::ViewModelPropertyNumber>())
        return ArtboardViewModel::PropertyType::number;

    if (property->is<rive::ViewModelPropertyString>())
        return ArtboardViewModel::PropertyType::string;

    if (property->is<rive::ViewModelPropertyColor>())
        return ArtboardViewModel::PropertyType::color;

    if (property->is<rive::ViewModelPropertyList>())
        return ArtboardViewModel::PropertyType::list;

    if (property->is<rive::ViewModelPropertyTrigger>())
        return ArtboardViewModel::PropertyType::trigger;

    if (property->is<rive::ViewModelPropertyViewModel>())
        return ArtboardViewModel::PropertyType::viewModel;

    if (property->is<rive::ViewModelPropertySymbolListIndex>())
        return ArtboardViewModel::PropertyType::symbolListIndex;

    if (property->is<rive::ViewModelPropertyAssetImage>())
        return ArtboardViewModel::PropertyType::assetImage;

    if (property->is<rive::ViewModelPropertyArtboard>())
        return ArtboardViewModel::PropertyType::artboard;

    // Covers plain, custom and system enums (all derive from ViewModelPropertyEnum).
    if (property->is<rive::ViewModelPropertyEnum>())
        return ArtboardViewModel::PropertyType::enumType;

    return ArtboardViewModel::PropertyType::none;
}

StringArray enumOptionsOf (rive::ViewModelProperty* property)
{
    StringArray options;

    if (property != nullptr && property->is<rive::ViewModelPropertyEnum>())
        if (auto* dataEnum = property->as<rive::ViewModelPropertyEnum>()->dataEnum())
            for (auto* enumValue : dataEnum->values())
            {
                const auto& value = enumValue->value();
                options.add (value.empty() ? String (enumValue->key()) : String (value));
            }

    return options;
}

ArtboardViewModel::PropertyInfo propertyInfoOf (rive::ViewModelProperty* property)
{
    if (property == nullptr)
        return {};

    return { String (property->name()),
             toPropertyType (property),
             property->isInput(),
             property->isOutput(),
             enumOptionsOf (property) };
}

} // namespace

//==============================================================================

ArtboardViewModel::ArtboardViewModel (const std::shared_ptr<ArtboardFile>& fileToUse, rive::ViewModel* riveViewModel)
    : file (fileToUse)
    , viewModel (riveViewModel)
{
}

ArtboardViewModel::~ArtboardViewModel() = default;

//==============================================================================

String ArtboardViewModel::getName() const
{
    if (auto* vm = viewModel)
        return String (vm->name());

    return {};
}

ArtboardFile* ArtboardViewModel::getArtboardFile() const noexcept
{
    return file.get();
}

//==============================================================================

int ArtboardViewModel::getNumProperties() const noexcept
{
    if (auto* vm = viewModel)
        return static_cast<int> (vm->properties().size());

    return 0;
}

ArtboardViewModel::PropertyInfo ArtboardViewModel::getPropertyAt (int index) const
{
    if (auto* vm = viewModel)
        return propertyInfoOf (vm->property (static_cast<size_t> (index)));

    return {};
}

ArtboardViewModel::PropertyInfo ArtboardViewModel::getProperty (StringRef name) const
{
    if (auto* vm = viewModel)
        return propertyInfoOf (vm->property (String (name).toStdString()));

    return {};
}

bool ArtboardViewModel::hasProperty (StringRef name) const
{
    if (auto* vm = viewModel)
        return vm->property (String (name).toStdString()) != nullptr;

    return false;
}

//==============================================================================

int ArtboardViewModel::getNumInstances() const noexcept
{
    if (auto* vm = viewModel)
        return static_cast<int> (vm->instanceCount());

    return 0;
}

StringArray ArtboardViewModel::getInstanceNames() const
{
    StringArray names;

    if (auto* vm = viewModel)
        for (auto* instance : vm->instances())
            names.add (String (instance->name()));

    return names;
}

//==============================================================================

ArtboardViewModel::Ptr ArtboardViewModel::createFromFile (const std::shared_ptr<ArtboardFile>& fileToUse, int index)
{
    if (fileToUse == nullptr)
        return nullptr;

    auto* rivFile = fileToUse->getRiveFile();
    if (rivFile == nullptr)
        return nullptr;

    auto* vm = rivFile->viewModel (static_cast<size_t> (index));
    if (vm == nullptr)
        return nullptr;

    return Ptr (new ArtboardViewModel (fileToUse, vm));
}

ArtboardViewModel::Ptr ArtboardViewModel::createFromFile (const std::shared_ptr<ArtboardFile>& fileToUse, StringRef name)
{
    if (fileToUse == nullptr)
        return nullptr;

    auto* rivFile = fileToUse->getRiveFile();
    if (rivFile == nullptr)
        return nullptr;

    auto* vm = rivFile->viewModel (String (name).toStdString());
    if (vm == nullptr)
        return nullptr;

    return Ptr (new ArtboardViewModel (fileToUse, vm));
}

} // namespace yup
