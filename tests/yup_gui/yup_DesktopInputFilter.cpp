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

class DesktopInputFilterTests : public ::testing::Test
{
protected:
    class RecordingFilter : public DesktopInputFilter
    {
    public:
        bool mouseEventFiltered (InputEventType type, Component&, const MouseEvent&) override
        {
            ++mouseEvents;
            lastType = type;

            if (log != nullptr)
                log->push_back (name);

            if (removeSelfOnEvent)
                Desktop::getInstance()->removeInputFilter (this);

            return consumes;
        }

        bool keyDownFiltered (Component&, const KeyPress&, const Point<float>&) override
        {
            ++keyDowns;
            return consumes;
        }

        String name;
        bool consumes = false;
        bool removeSelfOnEvent = false;
        int mouseEvents = 0;
        int keyDowns = 0;
        InputEventType lastType = InputEventType::mouseMove;
        std::vector<String>* log = nullptr;
    };

    void TearDown() override
    {
        // Leaving a filter behind would swallow the events of every other test
        for (auto* filter : installed)
            Desktop::getInstance()->removeInputFilter (filter);

        installed.clear();
    }

    void install (RecordingFilter& filter)
    {
        Desktop::getInstance()->addInputFilter (&filter);
        installed.push_back (&filter);
    }

    bool sendMouseDown()
    {
        return Desktop::getInstance()->filterMouseEvent (DesktopInputFilter::InputEventType::mouseDown, root, event);
    }

    Component root { "filterRoot" };
    MouseEvent event;
    std::vector<DesktopInputFilter*> installed;
};

TEST_F (DesktopInputFilterTests, NoFiltersMeansNothingIsConsumed)
{
    EXPECT_FALSE (Desktop::hasInputFilters());
    EXPECT_FALSE (sendMouseDown());
}

TEST_F (DesktopInputFilterTests, AnInstalledFilterReceivesEvents)
{
    RecordingFilter filter;
    install (filter);

    EXPECT_TRUE (Desktop::hasInputFilters());
    EXPECT_FALSE (sendMouseDown());
    EXPECT_EQ (filter.mouseEvents, 1);
    EXPECT_EQ (filter.lastType, DesktopInputFilter::InputEventType::mouseDown);

    EXPECT_FALSE (Desktop::getInstance()->filterKeyDown (root, KeyPress(), {}));
    EXPECT_EQ (filter.keyDowns, 1);
}

TEST_F (DesktopInputFilterTests, ARemovedFilterStopsReceivingEvents)
{
    RecordingFilter filter;
    install (filter);

    Desktop::getInstance()->removeInputFilter (&filter);
    installed.clear();

    EXPECT_FALSE (Desktop::hasInputFilters());
    EXPECT_FALSE (sendMouseDown());
    EXPECT_EQ (filter.mouseEvents, 0);
}

TEST_F (DesktopInputFilterTests, ConsumingFilterStopsTheOnesInstalledAfterIt)
{
    std::vector<String> log;

    RecordingFilter first, second;
    first.name = "first";
    first.log = &log;
    first.consumes = true;
    second.name = "second";
    second.log = &log;

    install (first);
    install (second);

    EXPECT_TRUE (sendMouseDown());
    EXPECT_EQ (second.mouseEvents, 0);
    ASSERT_EQ (log.size(), 1u);
    EXPECT_EQ (log.front(), String ("first"));
}

TEST_F (DesktopInputFilterTests, FiltersAreVisitedInInstallationOrder)
{
    std::vector<String> log;

    RecordingFilter first, second;
    first.name = "first";
    first.log = &log;
    second.name = "second";
    second.log = &log;

    install (first);
    install (second);

    EXPECT_FALSE (sendMouseDown());
    ASSERT_EQ (log.size(), 2u);
    EXPECT_EQ (log[0], String ("first"));
    EXPECT_EQ (log[1], String ("second"));
}

TEST_F (DesktopInputFilterTests, AFilterCanRemoveItselfWhileBeingCalled)
{
    RecordingFilter first, second;
    first.removeSelfOnEvent = true;

    install (first);
    install (second);

    EXPECT_FALSE (sendMouseDown());
    EXPECT_EQ (first.mouseEvents, 1);
    EXPECT_EQ (second.mouseEvents, 1);

    EXPECT_FALSE (sendMouseDown());
    EXPECT_EQ (first.mouseEvents, 1);
    EXPECT_EQ (second.mouseEvents, 2);

    installed.erase (installed.begin());
}
