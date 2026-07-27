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

#include <yup_gui/yup_gui.h>

// ==============================================================================
// Mock yup::MouseListener
// ==============================================================================

class MockMouseListener : public yup::MouseListener
{
public:
    MOCK_METHOD (void, mouseEnter, (const yup::MouseEvent&), (override));
    MOCK_METHOD (void, mouseExit, (const yup::MouseEvent&), (override));
    MOCK_METHOD (void, mouseDown, (const yup::MouseEvent&), (override));
    MOCK_METHOD (void, mouseUp, (const yup::MouseEvent&), (override));
    MOCK_METHOD (void, mouseMove, (const yup::MouseEvent&), (override));
    MOCK_METHOD (void, mouseDrag, (const yup::MouseEvent&), (override));
    MOCK_METHOD (void, mouseDoubleClick, (const yup::MouseEvent&), (override));
    MOCK_METHOD (void, mouseWheel, (const yup::MouseEvent&, const yup::MouseWheelData&), (override));
};

// ==============================================================================
// Mock yup::ComponentListener
// ==============================================================================

class MockComponentListener : public yup::ComponentListener
{
public:
    MOCK_METHOD (void, componentMoved, (yup::Component&), (override));
    MOCK_METHOD (void, componentResized, (yup::Component&), (override));
    MOCK_METHOD (void, componentBeingDeleted, (yup::Component&), (override));
};

// ==============================================================================
// Mock yup::Component
//
// Provides MOCK_METHOD overrides for all Component virtual callbacks so tests
// can use EXPECT_CALL / ON_CALL instead of hand-rolled boolean tracking.
// ==============================================================================

class MockComponent : public yup::Component
{
public:
    using yup::Component::Component;

    ~MockComponent() override = default;

    // Lifecycle / hierarchy
    MOCK_METHOD (void, enablementChanged, (), (override));
    MOCK_METHOD (void, visibilityChanged, (), (override));
    MOCK_METHOD (void, moved, (), (override));
    MOCK_METHOD (void, resized, (), (override));
    MOCK_METHOD (void, displayChanged, (), (override));
    MOCK_METHOD (void, attachedToNative, (), (override));
    MOCK_METHOD (void, detachedFromNative, (), (override));
    MOCK_METHOD (void, userTriedToCloseWindow, (), (override));
    MOCK_METHOD (void, focusGained, (), (override));
    MOCK_METHOD (void, focusLost, (), (override));
    MOCK_METHOD (void, parentHierarchyChanged, (), (override));
    MOCK_METHOD (void, childrenChanged, (), (override));

    // Rendering
    MOCK_METHOD (void, paint, (yup::Graphics&), (override));
    MOCK_METHOD (void, paintOverChildren, (yup::Graphics&), (override));
    MOCK_METHOD (void, refreshDisplay, (double), (override));
    MOCK_METHOD (void, styleChanged, (), (override));

    // Input
    MOCK_METHOD (void, mouseEnter, (const yup::MouseEvent&), (override));
    MOCK_METHOD (void, mouseExit, (const yup::MouseEvent&), (override));
    MOCK_METHOD (void, mouseDown, (const yup::MouseEvent&), (override));
    MOCK_METHOD (void, mouseMove, (const yup::MouseEvent&), (override));
    MOCK_METHOD (void, mouseDrag, (const yup::MouseEvent&), (override));
    MOCK_METHOD (void, mouseUp, (const yup::MouseEvent&), (override));
    MOCK_METHOD (void, mouseDoubleClick, (const yup::MouseEvent&), (override));
    MOCK_METHOD (void, mouseWheel, (const yup::MouseEvent&, const yup::MouseWheelData&), (override));
    MOCK_METHOD (void, keyDown, (const yup::KeyPress&, const yup::Point<float>&), (override));
    MOCK_METHOD (void, keyUp, (const yup::KeyPress&, const yup::Point<float>&), (override));
    MOCK_METHOD (void, textInput, (const yup::String&), (override));

    // Display / transform
    MOCK_METHOD (void, contentScaleChanged, (float), (override));
    MOCK_METHOD (void, safeAreaChanged, (), (override));
    MOCK_METHOD (void, transformChanged, (), (override));
};

// ==============================================================================
// Mock yup::ComponentStyle
// ==============================================================================

class MockComponentStyle : public yup::ComponentStyle
{
public:
    MOCK_METHOD (void, paint, (yup::Graphics&, const yup::ApplicationTheme&, const yup::Component&), (override));
};
