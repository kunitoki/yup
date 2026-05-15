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

/**
    An AudioProcessor that wraps a loaded third-party plugin.

    Concrete subclasses are created by AudioPluginFormat::loadPlugin() — callers
    never instantiate this directly. Interact with the plugin through the
    AudioProcessor interface; format-specific behaviour stays in the subclass.

    Ownership: returned as std::unique_ptr<AudioPluginInstance> from loadPlugin().
*/
class AudioPluginInstance : public AudioProcessor
{
public:
    //==============================================================================

    /** @internal Used by format backends. */
    AudioPluginInstance (const AudioPluginDescription& description,
                         AudioBusLayout busLayout);

    ~AudioPluginInstance() override;

    //==============================================================================

    /** Returns the description that was used to load this instance. */
    const AudioPluginDescription& getDescription() const noexcept;

    //==============================================================================

    /** Returns the format type of this instance. */
    AudioPluginFormatType getFormatType() const noexcept;

protected:
    AudioPluginDescription pluginDescription;

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginInstance)
};

} // namespace yup
