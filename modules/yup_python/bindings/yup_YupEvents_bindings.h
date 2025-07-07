/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#if !YUP_MODULE_AVAILABLE_yup_events
 #error This binding file requires adding the yup_events module in the project
#else
 #include <yup_events/yup_events.h>
#endif

#include "../utilities/yup_PyBind11Includes.h"

#include <variant>

namespace yup::Bindings {

// =================================================================================================

void registerYupEventsBindings (pybind11::module_& m);

// =================================================================================================

struct PyActionListener : public yup::ActionListener
{
    void actionListenerCallback (const yup::String& message) override
    {
        PYBIND11_OVERRIDE_PURE (void, yup::ActionListener, actionListenerCallback, message);
    }
};

// =================================================================================================

struct PyAsyncUpdater : public yup::AsyncUpdater
{
    void handleAsyncUpdate() override
    {
        PYBIND11_OVERRIDE_PURE (void, yup::AsyncUpdater, handleAsyncUpdate);
    }
};

// =================================================================================================

struct PyChangeListener : public yup::ChangeListener
{
    void changeListenerCallback (yup::ChangeBroadcaster* source) override
    {
        PYBIND11_OVERRIDE_PURE (void, yup::ChangeListener, changeListenerCallback, source);
    }
};

// =================================================================================================

template <class Base = yup::MessageManager::MessageBase>
struct PyMessageBase : public Base
{
    using Base::Base;

    void messageCallback() override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, messageCallback);
    }
};

// =================================================================================================

template <class Base = yup::CallbackMessage>
struct PyCallbackMessage : public PyMessageBase<Base>
{
    using PyMessageBase<Base>::PyMessageBase;

    void messageCallback() override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, messageCallback);
    }
};

// =================================================================================================

struct PyMessageListener : public yup::MessageListener
{
    void handleMessage (const yup::Message& message) override
    {
        PYBIND11_OVERRIDE_PURE (void, yup::MessageListener, handleMessage, message);
    }
};

// =================================================================================================

struct PyMessageManagerLock
{
    explicit PyMessageManagerLock (yup::Thread* thread)
        : thread (thread)
    {
    }

    explicit PyMessageManagerLock (yup::ThreadPoolJob* threadPoolJob)
        : threadPoolJob (threadPoolJob)
    {
    }

    yup::Thread* thread = nullptr;
    yup::ThreadPoolJob* threadPoolJob = nullptr;
    std::variant<std::monostate, yup::MessageManagerLock> state;
};

// =================================================================================================

struct PyTimer : public yup::Timer
{
    using yup::Timer::Timer;

    void timerCallback() override
    {
        PYBIND11_OVERRIDE_PURE (void, yup::Timer, timerCallback);
    }
};

// =================================================================================================

struct PyMultiTimer : public yup::MultiTimer
{
    using yup::MultiTimer::MultiTimer;

    void timerCallback (int timerID) override
    {
        PYBIND11_OVERRIDE_PURE (void, yup::MultiTimer, timerCallback, timerID);
    }
};

} // namespace yup::Bindings
