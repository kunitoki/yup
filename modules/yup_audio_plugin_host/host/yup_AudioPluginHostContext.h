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
    Runtime context provided to a plugin when it is loaded.

    Pass one AudioPluginHostContext to AudioPluginFormat::loadPlugin().
    The same context can be shared across multiple instances; only
    sampleRate and maxBlockSize have per-instance meaning.
*/
struct AudioPluginHostContext
{
    /** Sample rate the plugin will be prepared at. */
    float sampleRate = 44100.0f;

    /** Maximum number of samples per processBlock() call. */
    int maxBlockSize = 512;

    /** True to prefer double-precision processing where the plugin supports it. */
    bool preferDoublePrecision = false;

    /** True when the host is preparing the plugin for offline/non-realtime rendering. */
    bool isNonRealtime = false;

    /** Host application name reported to the plugin. */
    String hostName = "YUP Audio Plugin Host";

    /** Host vendor reported to the plugin. */
    String hostVendor = "YUP";

    /** Host version string reported to the plugin. */
    String hostVersion = "1.0.0";
};

} // namespace yup
