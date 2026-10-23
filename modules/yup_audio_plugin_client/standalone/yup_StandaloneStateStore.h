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

#if YUP_EMSCRIPTEN
// clang-format off
EM_JS (int, yupStandaloneStateLength, (const char* key),
{
    if (typeof localStorage === 'undefined')
        return -1;

    var value = localStorage.getItem (UTF8ToString (key));
    return value === null ? -1 : lengthBytesUTF8 (value);
});

EM_JS (void, yupStandaloneStateRead, (const char* key, char* buffer, int size),
{
    var value = localStorage.getItem (UTF8ToString (key));
    stringToUTF8 (value === null ? "" : value, buffer, size);
});

EM_JS (int, yupStandaloneStateWrite, (const char* key, const char* value),
{
    if (typeof localStorage === 'undefined')
        return 0;

    try
    {
        localStorage.setItem (UTF8ToString (key), UTF8ToString (value));
        return 1;
    }
    catch (e)
    {
        console.warn ('Unable to persist the standalone state: ' + e);
        return 0;
    }
});
// clang-format on
#endif

namespace yup
{

//==============================================================================
/** Persists the audio device setup and the plugin state of a standalone plugin.

    Both live in one XML document. On desktop it is stored in a file; in the browser
    it is stored in localStorage, and loading and saving do nothing when localStorage
    is not available (for example under node).
*/
class StandaloneStateStore
{
public:
    //==============================================================================
    /** The state read back by load(). */
    struct State
    {
        /** The device setup created by AudioDeviceManager::createStateXml(), or null. */
        std::unique_ptr<XmlElement> deviceState;

        /** The plugin state created by AudioProcessor::saveStateIntoMemory(), or empty. */
        MemoryBlock processorState;
    };

    //==============================================================================
    /** Creates a store that keeps the state in a file. */
    explicit StandaloneStateStore (File stateFile)
        : file (std::move (stateFile))
    {
    }

    /** Creates the store a standalone plugin uses by default.

        On desktop this is <userPreferencesDirectory>/<vendor>/<name>/standalone.xml,
        in the browser it is the localStorage key "yup.standalone.<pluginId>".
    */
    static StandaloneStateStore createDefault ([[maybe_unused]] StringRef vendor,
                                               [[maybe_unused]] StringRef name,
                                               [[maybe_unused]] StringRef pluginId)
    {
#if YUP_EMSCRIPTEN
        StandaloneStateStore store { File() };
        store.storageKey = "yup.standalone." + String (pluginId);
        return store;
#else
        return StandaloneStateStore (File::getSpecialLocation (File::userPreferencesDirectory)
                                         .getChildFile (File::createLegalFileName (String (vendor)))
                                         .getChildFile (File::createLegalFileName (String (name)))
                                         .getChildFile ("standalone.xml"));
#endif
    }

    //==============================================================================
    /** Saves the device setup and the current plugin state.

        @param deviceState  the device setup to store, which can be null
        @param processor    the processor whose state is stored
    */
    Result save (const XmlElement* deviceState, AudioProcessor& processor) const
    {
        XmlElement root (rootTag);
        root.setAttribute (versionAttribute, currentVersion);

        if (deviceState != nullptr)
            root.addChildElement (new XmlElement (*deviceState));

        MemoryBlock processorState;
        if (processor.saveStateIntoMemory (processorState).wasOk() && ! processorState.isEmpty())
            root.setAttribute (processorAttribute, processorState.toBase64Encoding());

        return writeText (root.toString());
    }

    /** Loads the stored state, returning an empty state when nothing valid was stored. */
    State load() const
    {
        State state;

        const auto root = XmlDocument::parse (readText());
        if (root == nullptr || ! root->hasTagName (rootTag))
            return state;

        if (auto* device = root->getFirstChildElement())
            state.deviceState = std::make_unique<XmlElement> (*device);

        if (root->hasAttribute (processorAttribute)
            && ! state.processorState.fromBase64Encoding (root->getStringAttribute (processorAttribute)))
        {
            state.processorState.reset();
        }

        return state;
    }

    /** Loads a plugin state into the processor, suspending its processing meanwhile. */
    static Result restoreProcessorState (AudioProcessor& processor, const MemoryBlock& processorState)
    {
        if (processorState.isEmpty())
            return Result::ok();

        AudioProcessorBase::ScopedProcessSuspension suspension (processor);
        return processor.loadStateFromMemory (processorState);
    }

    /** Returns the file the state is kept in, which is empty when it lives in the browser. */
    const File& getFile() const noexcept { return file; }

private:
    //==============================================================================
    String readText() const
    {
#if YUP_EMSCRIPTEN
        if (storageKey.isNotEmpty())
        {
            const auto length = yupStandaloneStateLength (storageKey.toRawUTF8());
            if (length <= 0)
                return {};

            HeapBlock<char> buffer (static_cast<size_t> (length) + 1, true);
            yupStandaloneStateRead (storageKey.toRawUTF8(), buffer.getData(), length + 1);
            return String::fromUTF8 (buffer.getData(), length);
        }
#endif

        return file.existsAsFile() ? file.loadFileAsString() : String();
    }

    Result writeText (const String& text) const
    {
#if YUP_EMSCRIPTEN
        if (storageKey.isNotEmpty())
        {
            return yupStandaloneStateWrite (storageKey.toRawUTF8(), text.toRawUTF8()) != 0
                     ? Result::ok()
                     : Result::fail ("Unable to write the standalone state to localStorage");
        }
#endif

        if (const auto result = file.getParentDirectory().createDirectory(); result.failed())
            return result;

        return file.replaceWithText (text)
                 ? Result::ok()
                 : Result::fail ("Unable to write " + file.getFullPathName());
    }

    //==============================================================================
    static constexpr const char* rootTag = "STANDALONE_STATE";
    static constexpr const char* versionAttribute = "version";
    static constexpr const char* processorAttribute = "processorState";
    static constexpr int currentVersion = 1;

    File file;
    String storageKey;
};

} // namespace yup
