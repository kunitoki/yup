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

namespace
{

std::atomic<StandalonePluginHost*> standalonePluginHostInstance { nullptr };

} // namespace

//==============================================================================

StandalonePluginHost::StandalonePluginHost()
{
    StandalonePluginHost* expected = nullptr;
    const auto registered = standalonePluginHostInstance.compare_exchange_strong (expected, this);

    jassert (registered); // Only one standalone host can exist at a time
    ignoreUnused (registered);
}

StandalonePluginHost::~StandalonePluginHost()
{
    StandalonePluginHost* expected = this;
    standalonePluginHostInstance.compare_exchange_strong (expected, nullptr);
}

//==============================================================================

StandalonePluginHost* StandalonePluginHost::getInstance() noexcept
{
    return standalonePluginHostInstance.load();
}

} // namespace yup
