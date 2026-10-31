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

#include "PluginLogger.h"

namespace
{
const char* getPluginFormatName()
{
#if YUP_AUDIO_PLUGIN_ENABLE_AU
    return "au";
#elif YUP_AUDIO_PLUGIN_ENABLE_CLAP
    return "clap";
#elif YUP_AUDIO_PLUGIN_ENABLE_VST3
    return "vst3";
#elif YUP_AUDIO_PLUGIN_ENABLE_LV2
    return "lv2";
#elif YUP_AUDIO_PLUGIN_ENABLE_AAX
    return "aax";
#elif YUP_AUDIO_PLUGIN_ENABLE_STANDALONE
    return "standalone";
#else
    return "unknown";
#endif
}
} // namespace

PluginLogger::PluginLogger()
{
    const auto logFileName = yup::String (YupPlugin_Name) + "_" + getPluginFormatName() + ".log";
    logger.reset (new yup::FileLogger (yup::FileLogger::getSystemLogFileFolder().getChildFile (logFileName),
                                       yup::String (YupPlugin_Name) + " " + getPluginFormatName() + " log"));

    yup::Logger::setCurrentLogger (logger.get());
    yup::Logger::writeToLog ("Logger initialised: " + logger->getLogFile().getFullPathName());
}

PluginLogger::~PluginLogger()
{
    if (yup::Logger::getCurrentLogger() == logger.get())
        yup::Logger::setCurrentLogger (nullptr);
}

void PluginLogger::initialise()
{
    static PluginLogger instance;
    yup::Logger::setCurrentLogger (instance.logger.get());
}
