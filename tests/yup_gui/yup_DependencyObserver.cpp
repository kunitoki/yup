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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

class DependencyObserverTests : public ::testing::Test
{
protected:
};

TEST_F (DependencyObserverTests, ReferenceCountsComponentListenersWithoutDuplicateCallbacks)
{
    Slider slider (Slider::LinearHorizontal);
    slider.setRange (0.0, 10.0, 0.0);

    int changeNotifications = 0;
    Identifier lastProperty;

    DependencyObserver observer;
    observer.onChanged = [&] (Component& changed, const Identifier& property)
    {
        if (&changed == &slider && property == Identifier ("value"))
        {
            ++changeNotifications;
            lastProperty = property;
        }
    };

    EXPECT_TRUE (observer.isEmpty());

    // Observing the same component twice registers only a single ComponentListener
    observer.observe (slider);
    observer.observe (slider);
    EXPECT_FALSE (observer.isEmpty());

    slider.setValue (3.0, dontSendNotification);
    EXPECT_EQ (changeNotifications, 1);
    EXPECT_EQ (lastProperty, Identifier ("value"));

    // First unobserve decrements count from 2 to 1; listener remains active
    observer.unobserve (slider);
    EXPECT_FALSE (observer.isEmpty());
    slider.setValue (6.0, dontSendNotification);
    EXPECT_EQ (changeNotifications, 2);

    // Final unobserve removes the listener
    observer.unobserve (slider);
    EXPECT_TRUE (observer.isEmpty());
    slider.setValue (9.0, dontSendNotification);
    EXPECT_EQ (changeNotifications, 2);
}

TEST_F (DependencyObserverTests, CleansUpAutomaticallyWhenObservedComponentIsDeleted)
{
    DependencyObserver observer;
    int deletedCount = 0;

    observer.onDeleted = [&] (Component&)
    {
        ++deletedCount;
    };

    {
        Component transient ("transient");
        const std::vector<DrawingScope::Dependency> deps {
            { &transient, Identifier ("x") },
            { &transient, Identifier ("y") }
        };
        observer.observe (deps);
        EXPECT_FALSE (observer.isEmpty());
    }

    EXPECT_EQ (deletedCount, 1);
    EXPECT_TRUE (observer.isEmpty());
}
