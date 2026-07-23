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

#include <gmock/gmock.h>

#include <yup_events/yup_events.h>

// ==============================================================================
// Mock yup::ChangeListener
// ==============================================================================

class MockChangeListener : public yup::ChangeListener
{
public:
    MOCK_METHOD (void, changeListenerCallback, (yup::ChangeBroadcaster*), (override));
};

// ==============================================================================
// Mock yup::ActionListener
// ==============================================================================

class MockActionListener : public yup::ActionListener
{
public:
    MOCK_METHOD (void, actionListenerCallback, (const yup::String&), (override));
};

// ==============================================================================
// Mock yup::AsyncUpdater
// ==============================================================================

class MockAsyncUpdater : public yup::AsyncUpdater
{
public:
    MOCK_METHOD (void, handleAsyncUpdate, (), (override));
};

// ==============================================================================
// Mock yup::Timer
// ==============================================================================

class MockTimer : public yup::Timer
{
public:
    MOCK_METHOD (void, timerCallback, (), (override));
};

// ==============================================================================
// Mock yup::MultiTimer
// ==============================================================================

class MockMultiTimer : public yup::MultiTimer
{
public:
    MOCK_METHOD (void, timerCallback, (int), (override));
};
