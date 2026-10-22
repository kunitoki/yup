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

#include <yup_core/yup_core.h>

#include <memory>

//==============================================================================
/** Writes the plugin's log to a file named after the plugin and its format.

    One logger is shared by every instance of the plugin in the process.
*/
class PluginLogger final
{
public:
    /** Creates the shared logger on first use and makes it the current one. */
    static void initialise();

    ~PluginLogger();

private:
    PluginLogger();

    std::unique_ptr<yup::FileLogger> logger;
};
