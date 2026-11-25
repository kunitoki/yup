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

std::vector<ComponentDocument*>& getComponentDocuments()
{
    static std::vector<ComponentDocument*> documents;
    return documents;
}

} // namespace

//==============================================================================

ComponentDocument::~ComponentDocument()
{
    unregisterDocument();
}

void ComponentDocument::registerDocument()
{
    getComponentDocuments().push_back (this);
}

void ComponentDocument::unregisterDocument()
{
    std::erase (getComponentDocuments(), this);
}

const std::vector<ComponentDocument*>& ComponentDocument::getRegisteredDocuments()
{
    return getComponentDocuments();
}

//==============================================================================

ComponentDocument* ComponentDocument::findFor (const Component* component)
{
    for (; component != nullptr; component = component->getParentComponent())
    {
        for (auto* document : getComponentDocuments())
        {
            if (document->getRoot() == component)
                return document;
        }
    }

    return nullptr;
}

std::optional<MemoryBlock> ComponentDocument::readAssetFor (const Component* component, const String& path)
{
    if (const auto* document = findFor (component))
        return document->readAsset (path);

    return readAssetFile (resolveAssetFile (path, {}));
}

//==============================================================================

void ComponentDocument::notifyDocumentLoaded()
{
    documentListeners.call ([this] (Listener& listener)
    {
        listener.documentLoaded (*this);
    });
}

void ComponentDocument::notifyDocumentAssetChanged (const String& path)
{
    documentListeners.call ([&] (Listener& listener)
    {
        listener.documentAssetChanged (*this, path);
    });
}

//==============================================================================

// The file of an asset is found as Image and the drawings find theirs
File ComponentDocument::resolveAssetFile (const String& path, const File& baseDirectory)
{
    if (path.isEmpty())
        return {};

    if (File::isAbsolutePath (path))
        return File (path);

    if (baseDirectory != File())
    {
        if (const auto candidate = baseDirectory.getChildFile (path); candidate.existsAsFile())
            return candidate;
    }

    return File::getCurrentWorkingDirectory().getChildFile (path);
}

std::optional<MemoryBlock> ComponentDocument::readAssetFile (const File& file)
{
    MemoryBlock bytes;

    if (! file.loadFileAsData (bytes))
        return std::nullopt;

    return bytes;
}

} // namespace yup
