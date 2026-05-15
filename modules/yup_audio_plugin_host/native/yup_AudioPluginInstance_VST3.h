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

#if YUP_AUDIO_PLUGIN_HOST_ENABLE_VST3

namespace yup
{

/**
    AudioPluginFormat implementation for VST3.

    Scans .vst3 bundles/folders by loading their module factory and enumerating
    classes. Loads plugins via IComponent + IAudioProcessor setup.
*/
class VST3Format : public AudioPluginFormat
{
public:
    VST3Format();
    ~VST3Format() override;

    AudioPluginFormatType getFormatType() const override;
    String getFormatName() const override;

    FileSearchPath getDefaultSearchPaths() const override;

    ResultValue<std::vector<AudioPluginDescription>> scanFile (const File& file) override;

    ResultValue<std::unique_ptr<AudioPluginInstance>> loadPlugin (
        const AudioPluginDescription& description,
        const AudioPluginHostContext& context) override;
};

} // namespace yup

#endif // YUP_AUDIO_PLUGIN_HOST_ENABLE_VST3
