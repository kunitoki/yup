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

//==============================================================================
/** The host side of a standalone plugin, as seen by the plugin editor.

    When a plugin runs as a standalone application, the wrapper draws a slim toolbar
    with a button that opens the audio and MIDI settings. An editor that prefers its
    own button can hide that toolbar and open the settings itself:

    @code
    MyEditor::MyEditor()
    {
        if (auto* host = yup::StandalonePluginHost::getInstance())
        {
            host->setToolbarVisible (false);
            settingsButton.onClick = [] { yup::StandalonePluginHost::getInstance()->showAudioSettings(); };
            addAndMakeVisible (settingsButton);
        }
    }
    @endcode

    Only the standalone wrapper creates an instance, so getInstance() returns nullptr
    whenever the plugin is loaded by a host (CLAP, VST3, AU and the other formats).
    All methods must be called on the message thread.
*/
class YUP_API StandalonePluginHost
{
public:
    //==============================================================================
    /** Destructor. */
    virtual ~StandalonePluginHost();

    //==============================================================================
    /** Returns the running standalone host, or nullptr when hosted as a plugin. */
    static StandalonePluginHost* getInstance() noexcept;

    //==============================================================================
    /** Opens the audio and MIDI device settings. */
    virtual void showAudioSettings() = 0;

    /** Shows or hides the toolbar the standalone wrapper draws above the editor. */
    virtual void setToolbarVisible (bool shouldBeVisible) = 0;

protected:
    //==============================================================================
    /** Registers this object as the running instance. Only one instance can exist at a time. */
    StandalonePluginHost();

private:
    YUP_DECLARE_NON_COPYABLE (StandalonePluginHost)
};

} // namespace yup
