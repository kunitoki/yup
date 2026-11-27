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

ComponentDocument::~ComponentDocument()
{
    detachFromRoot();
}

void ComponentDocument::attachToRoot()
{
    auto* newRoot = getRoot();

    if (attachedRoot.get() == newRoot)
        return;

    detachFromRoot();

    attachedRoot = newRoot;

    if (newRoot != nullptr)
        newRoot->componentDocument = this;
}

void ComponentDocument::detachFromRoot()
{
    // A root given to another document since is left to it
    if (auto* root = attachedRoot.get(); root != nullptr && root->componentDocument == this)
        root->componentDocument = nullptr;

    attachedRoot = nullptr;
}

//==============================================================================

ComponentDocument* ComponentDocument::findFor (const Component* component)
{
    for (; component != nullptr; component = component->getParentComponent())
    {
        if (component->componentDocument != nullptr)
            return component->componentDocument;
    }

    return nullptr;
}

std::optional<MemoryBlock> ComponentDocument::readAssetFor (const Component* component, const String& path)
{
    if (const auto* document = findFor (component))
        return document->readAsset (path);

    return readAssetFile (resolveAssetFile (path, {}));
}

File ComponentDocument::getAssetFileFor (const Component* component, const String& path)
{
    if (const auto* document = findFor (component))
        return document->getAssetFile (path);

    return resolveAssetFile (path, {});
}

//==============================================================================

ComponentDocument::Listener::~Listener()
{
    follow (nullptr);
}

bool ComponentDocument::Listener::follow (ComponentDocument* document)
{
    if (followed.get() == document)
        return false;

    if (auto* previous = followed.get())
        previous->removeListener (this);

    followed = document;

    if (document != nullptr)
        document->addListener (this);

    return true;
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
