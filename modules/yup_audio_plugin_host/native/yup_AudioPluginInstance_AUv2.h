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

#if YUP_AUDIO_PLUGIN_HOST_ENABLE_AU && YUP_MAC

namespace yup
{

/**
    AudioPluginFormat implementation for AUv2 (macOS only).

    Enumerates AudioComponents via AudioComponentFindNext() and wraps each
    component via AudioComponentInstanceNew().
*/
class AUv2Format : public AudioPluginFormat
{
public:
    AUv2Format();
    ~AUv2Format() override;

    AudioPluginFormatType getFormatType() const override;
    String getFormatName() const override;

    /** Returns an empty FileSearchPath — AUv2 uses AudioComponent registry. */
    FileSearchPath getDefaultSearchPaths() const override;

    /**
        Passing an invalid File triggers full AudioComponent registry scan.
        Otherwise scans only the component matching the file's bundle identifier.
    */
    ResultValue<std::vector<AudioPluginDescription>> scanFile (const File& file) override;

    ResultValue<std::unique_ptr<AudioPluginInstance>> loadPlugin (
        const AudioPluginDescription& description,
        const AudioPluginHostContext& context) override;
};

} // namespace yup

#endif // YUP_AUDIO_PLUGIN_HOST_ENABLE_AU && YUP_MAC
