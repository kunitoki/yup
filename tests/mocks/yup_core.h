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

#include <yup_core/yup_core.h>

// ==============================================================================
// Mock yup::Logger
// ==============================================================================

class MockLogger : public yup::Logger
{
public:
    MOCK_METHOD (void, logMessage, (const yup::String&), (override));
};

// ==============================================================================
// Mock yup::InputStream
// ==============================================================================

class MockInputStream : public yup::InputStream
{
public:
    MOCK_METHOD (yup::int64, getTotalLength, (), (override));
    MOCK_METHOD (bool, isExhausted, (), (override));
    MOCK_METHOD (int, read, (void*, int), (override));
    MOCK_METHOD (yup::int64, getPosition, (), (override));
    MOCK_METHOD (bool, setPosition, (yup::int64), (override));
};

// ==============================================================================
// Mock yup::OutputStream
// ==============================================================================

class MockOutputStream : public yup::OutputStream
{
public:
    MOCK_METHOD (void, flush, (), (override));
    MOCK_METHOD (yup::int64, getPosition, (), (override));
    MOCK_METHOD (bool, setPosition, (yup::int64), (override));
    MOCK_METHOD (bool, write, (const void*, size_t), (override));
};

// ==============================================================================
// Mock yup::InputSource
// ==============================================================================

class MockInputSource : public yup::InputSource
{
public:
    MOCK_METHOD (yup::InputStream*, createInputStream, (), (override));
    MOCK_METHOD (yup::InputStream*, createInputStreamFor, (const yup::String&), (override));
    MOCK_METHOD (yup::int64, hashCode, (), (const, override));
};

// ==============================================================================
// Mock yup::Thread::Listener
// ==============================================================================

class MockThreadListener : public yup::Thread::Listener
{
public:
    MOCK_METHOD (void, exitSignalSent, (), (override));
};

// ==============================================================================
// Mock yup::Thread
// ==============================================================================

class MockThread : public yup::Thread
{
public:
    MOCK_METHOD (void, run, (), (override));
};

// ==============================================================================
// Mock yup::ThreadPoolJob
// ==============================================================================

class MockThreadPoolJob : public yup::ThreadPoolJob
{
public:
    using yup::ThreadPoolJob::ThreadPoolJob;

    MOCK_METHOD (yup::ThreadPoolJob::JobStatus, runJob, (), (override));
};

// ==============================================================================
// Mock yup::HighResolutionTimer
// ==============================================================================

class MockHighResolutionTimer : public yup::HighResolutionTimer
{
public:
    MOCK_METHOD (void, hiResTimerCallback, (), (override));
};

// ==============================================================================
// Mock yup::TimeSliceClient
// ==============================================================================

class MockTimeSliceClient : public yup::TimeSliceClient
{
public:
    MOCK_METHOD (int, useTimeSlice, (), (override));
};
