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

#pragma once

namespace yup
{

class ArtboardFile;

//==============================================================================
/** Represents a Rive ViewModel schema.

    A ViewModel is the data schema a Rive artboard is designed against: a named
    list of typed properties (boolean, number, string, color, enum, trigger,
    nested viewmodels, lists, ...). Schemas are authored in the Rive editor and
    stored inside the .riv file.

    A handle to a schema is obtained from an ArtboardFile via
    ArtboardFile::getArtboardViewModel(). Instances are read-only, refcounted
    and remain valid for as long as the handle is referenced: the handle keeps
    the owning ArtboardFile (and therefore the underlying Rive file) alive.

    @see ArtboardFile::getArtboardViewModel, ArtboardViewModelInstance
*/
class YUP_API ArtboardViewModel : public ReferenceCountedObject
{
public:
    //==============================================================================
    using Ptr = ReferenceCountedObjectPtr<ArtboardViewModel>;

    //==============================================================================
    /** The data type of a ViewModel property. */
    enum class PropertyType
    {
        /** No type (unknown property). */
        none,

        /** A boolean property. */
        boolean,

        /** A numeric property. */
        number,

        /** A string property. */
        string,

        /** A color property. */
        color,

        /** A list of nested viewmodel instances. */
        list,

        /** An enum property, whose value is one of a fixed set of options. */
        enumType,

        /** A trigger property. */
        trigger,

        /** A nested viewmodel instance property. */
        viewModel,

        /** A symbol list index property. */
        symbolListIndex,

        /** An image asset property. */
        assetImage,

        /** An artboard reference property. */
        artboard
    };

    //==============================================================================
    /** Describes a single property of a ViewModel schema. */
    struct PropertyInfo
    {
        /** Default constructor. */
        PropertyInfo() = default;

        /** Constructs a property info from its parts. */
        PropertyInfo (String propertyName,
                      PropertyType propertyType,
                      bool isInputProperty,
                      bool isOutputProperty,
                      StringArray enumOptions)
            : name (std::move (propertyName))
            , type (propertyType)
            , isInput (isInputProperty)
            , isOutput (isOutputProperty)
            , enumValues (std::move (enumOptions))
        {
        }

        /** The name of the property. */
        String name;

        /** The data type of the property. */
        PropertyType type = PropertyType::none;

        /** True if the property accepts values coming from the host (an input binding). */
        bool isInput = false;

        /** True if the property is written back to the host when the artboard changes it (an output binding). */
        bool isOutput = false;

        /** For enumType properties, the human-readable options; empty otherwise. */
        StringArray enumValues;
    };

    //==============================================================================
    /** Destructor. */
    ~ArtboardViewModel() override;

    //==============================================================================
    /** Returns the name of the ViewModel schema. */
    String getName() const;

    /** Returns the ArtboardFile this schema belongs to, or null if it has been released. */
    ArtboardFile* getArtboardFile() const noexcept;

    //==============================================================================
    /** Returns the number of properties defined by the schema. */
    int getNumProperties() const noexcept;

    /** Returns the property at the given index, or an empty PropertyInfo if out of range. */
    PropertyInfo getPropertyAt (int index) const;

    /** Returns the property with the given name, or an empty PropertyInfo if unknown. */
    PropertyInfo getProperty (StringRef name) const;

    /** Returns true if the schema defines a property with the given name. */
    bool hasProperty (StringRef name) const noexcept;

    //==============================================================================
    /** Returns the number of authored instances stored in the schema.

        .riv files may ship with pre-authored viewmodel instances (the first of
        which is the "default" instance); these can be cloned through
        ArtboardFile::createArtboardViewModelInstance() by name.
    */
    int getNumInstances() const noexcept;

    /** Returns the names of the authored instances stored in the schema. */
    StringArray getInstanceNames() const;

    //==============================================================================
    /** @internal */
    static Ptr createFromFile (const std::shared_ptr<ArtboardFile>& file, int index);

    /** @internal */
    static Ptr createFromFile (const std::shared_ptr<ArtboardFile>& file, StringRef name);

private:
    ArtboardViewModel (const std::shared_ptr<ArtboardFile>& file, void* riveViewModel);

    std::shared_ptr<ArtboardFile> file;
    void* viewModel = nullptr;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArtboardViewModel)
};

} // namespace yup
