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

#if YUP_AUDIO_PLUGIN_HOST_ENABLE_AUV3 && YUP_MAC

namespace yup
{

/**
    AudioPluginFormat implementation for AUv3 (macOS only).

    AUv3 plugins are App Extensions (`.appex`) discovered through
    `AVAudioUnitComponentManager` which enumerates the system
    App Extension registry.
*/
class AUv3Format : public AudioPluginFormat
{
public:
    AUv3Format();
    ~AUv3Format() override;

    AudioPluginFormatType getFormatType() const override;
    String getFormatName() const override;
    StringArray getFileExtensions() const override;

    /** Returns an empty FileSearchPath — AUv3 uses the App Extension registry. */
    FileSearchPath getDefaultSearchPaths() const override;

    /**
        Passing an invalid File triggers full AUv3 registry scan.
        Otherwise scans only the component matching the file's bundle identifier.
    */
    ResultValue<std::vector<AudioPluginDescription>> scanFile (const File& file) override;

    ResultValue<std::unique_ptr<AudioPluginInstance>> loadPlugin (
        const AudioPluginDescription& description,
        const AudioPluginHostContext& context) override;
};

} // namespace yup

#endif // YUP_AUDIO_PLUGIN_HOST_ENABLE_AUV3 && YUP_MAC
