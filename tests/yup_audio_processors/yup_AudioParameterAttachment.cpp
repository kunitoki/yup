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

#include <yup_audio_processors/yup_audio_processors.h>

#include <gtest/gtest.h>

#if YUP_MODULE_AVAILABLE_yup_gui

#include <memory>
#include <thread>

using namespace yup;

//==============================================================================
class AudioParameterAttachmentTests : public ::testing::Test
{
protected:
    struct TestProcessor final : AudioProcessor
    {
        TestProcessor()
            : AudioProcessor ("Attachment Test", AudioBusLayout ({}, {}))
        {
            for (int i = 0; i < 3; ++i)
                addParameter (AudioParameterBuilder()
                                  .withID (String ("p") + String (i))
                                  .withName (String ("P") + String (i))
                                  .withRange (0.0f, 1.0f)
                                  .withDefault (0.0f)
                                  .build());
        }

        void prepareToPlay (const AudioSpec&) override {}

        void releaseResources() override {}

        void processBlock (AudioProcessContext<float>& context) override { ignoreUnused (context); }

        int getCurrentPreset() const noexcept override { return 0; }

        void setCurrentPreset (int) noexcept override {}

        int getNumPresets() const override { return 0; }

        String getPresetName (int) const override { return {}; }

        void setPresetName (int, StringRef) override {}

        Result loadStateFromMemory (const MemoryBlock&) override { return Result::ok(); }

        Result saveStateIntoMemory (MemoryBlock&) override { return Result::ok(); }

        bool hasEditor() const override { return false; }
    };

    struct TestEditor final : AudioProcessorEditor
    {
        bool isResizable() const override { return true; }

        Size<int> getPreferredSize() const override { return { 100, 100 }; }
    };

    void SetUp() override
    {
        // Bind without a timer; the tests drive dispatchPendingParameterChanges() themselves.
        editor.startParameterListening (processor, 0);
    }

    AudioParameter::Ptr parameter (int index) const { return processor.getParameters()[static_cast<std::size_t> (index)]; }

    TestProcessor processor;
    TestEditor editor;
};

TEST_F (AudioParameterAttachmentTests, CallsBackOnCreateWhenRequested)
{
    int calls = 0;
    AudioParameterAttachment attachment (editor, parameter (0), [&calls] { ++calls; }, true);

    EXPECT_EQ (1, calls);
}

TEST_F (AudioParameterAttachmentTests, DoesNotCallBackOnCreateWhenNotRequested)
{
    int calls = 0;
    AudioParameterAttachment attachment (editor, parameter (0), [&calls] { ++calls; }, false);

    EXPECT_EQ (0, calls);
}

TEST_F (AudioParameterAttachmentTests, NotifiesOnlyTheChangedParameter)
{
    int firstCalls = 0, secondCalls = 0;
    AudioParameterAttachment first (editor, parameter (0), [&firstCalls] { ++firstCalls; }, false);
    AudioParameterAttachment second (editor, parameter (1), [&secondCalls] { ++secondCalls; }, false);

    parameter (1)->setValue (0.75f);
    editor.dispatchPendingParameterChanges();

    EXPECT_EQ (0, firstCalls);
    EXPECT_EQ (1, secondCalls);
}

TEST_F (AudioParameterAttachmentTests, DoesNotNotifyWhenValueIsUnchanged)
{
    int calls = 0;
    AudioParameterAttachment attachment (editor, parameter (0), [&calls] { ++calls; }, false);

    parameter (0)->setValue (parameter (0)->getValue());
    editor.dispatchPendingParameterChanges();

    EXPECT_EQ (0, calls);
}

TEST_F (AudioParameterAttachmentTests, CoalescesRepeatedChangesIntoOneDispatch)
{
    int calls = 0;
    AudioParameterAttachment attachment (editor, parameter (0), [&calls] { ++calls; }, false);

    for (int i = 1; i <= 5; ++i)
        parameter (0)->setValue (static_cast<float> (i) * 0.1f);

    editor.dispatchPendingParameterChanges();

    EXPECT_EQ (1, calls);
}

TEST_F (AudioParameterAttachmentTests, NotifiesChangesMadeFromAnotherThread)
{
    int calls = 0;
    AudioParameterAttachment attachment (editor, parameter (0), [&calls] { ++calls; }, false);

    std::thread ([this] { parameter (0)->setValue (0.25f); }).join();
    editor.dispatchPendingParameterChanges();

    EXPECT_EQ (1, calls);
}

TEST_F (AudioParameterAttachmentTests, PreservesMutableCallbackStateBetweenDispatches)
{
    int calls = 0;
    AudioParameterAttachment attachment (editor, parameter (0), [&calls, count = 0]() mutable
    {
        calls = ++count;
    }, false);

    parameter (0)->setValue (0.5f);
    editor.dispatchPendingParameterChanges();
    parameter (0)->setValue (0.75f);
    editor.dispatchPendingParameterChanges();

    EXPECT_EQ (2, calls);
}

TEST_F (AudioParameterAttachmentTests, UnregisteredAttachmentIsNotNotified)
{
    int calls = 0;

    {
        AudioParameterAttachment attachment (editor, parameter (0), [&calls] { ++calls; }, false);
    }

    parameter (0)->setValue (0.5f);
    editor.dispatchPendingParameterChanges();

    EXPECT_EQ (0, calls);
}

TEST_F (AudioParameterAttachmentTests, RebindFollowsTheNewParameter)
{
    int calls = 0;
    AudioParameterAttachment attachment (editor, parameter (0), [&calls] { ++calls; }, false);

    attachment.setParameter (parameter (2), false);

    parameter (0)->setValue (0.5f);
    parameter (2)->setValue (0.5f);
    editor.dispatchPendingParameterChanges();

    EXPECT_EQ (parameter (2).get(), attachment.getParameter().get());
    EXPECT_EQ (1, calls);
}

TEST_F (AudioParameterAttachmentTests, RemovingAnotherAttachmentDuringDispatchSkipsIt)
{
    int calls = 0;
    std::unique_ptr<AudioParameterAttachment> second;
    AudioParameterAttachment first (editor, parameter (0), [&second] { second.reset(); }, false);
    second = std::make_unique<AudioParameterAttachment> (editor, parameter (0), [&calls] { ++calls; }, false);

    parameter (0)->setValue (0.5f);
    editor.dispatchPendingParameterChanges();

    EXPECT_EQ (nullptr, second);
    EXPECT_EQ (0, calls);
}

TEST_F (AudioParameterAttachmentTests, RebindingDuringDispatchFollowsTheNewParameter)
{
    int calls = 0;
    std::unique_ptr<AudioParameterAttachment> attachment;
    attachment = std::make_unique<AudioParameterAttachment> (editor, parameter (0), [&]
    {
        ++calls;
        attachment->setParameter (parameter (2), false);
    }, false);

    parameter (0)->setValue (0.5f);
    editor.dispatchPendingParameterChanges();
    parameter (0)->setValue (0.75f);
    editor.dispatchPendingParameterChanges();
    parameter (2)->setValue (0.5f);
    editor.dispatchPendingParameterChanges();

    EXPECT_EQ (2, calls);
    EXPECT_EQ (parameter (2), attachment->getParameter());
}

TEST_F (AudioParameterAttachmentTests, AttachmentCanDestroyItselfDuringDispatch)
{
    int calls = 0;
    std::unique_ptr<AudioParameterAttachment> attachment;
    attachment = std::make_unique<AudioParameterAttachment> (editor, parameter (0), [&]
    {
        ++calls;
        attachment.reset();
    }, false);

    parameter (0)->setValue (0.5f);
    editor.dispatchPendingParameterChanges();
    parameter (0)->setValue (0.75f);
    editor.dispatchPendingParameterChanges();

    EXPECT_EQ (nullptr, attachment);
    EXPECT_EQ (1, calls);
}

TEST_F (AudioParameterAttachmentTests, EveryListeningEditorReceivesChanges)
{
    TestEditor secondEditor;
    secondEditor.startParameterListening (processor, 0);
    int firstCalls = 0, secondCalls = 0;
    AudioParameterAttachment first (editor, parameter (0), [&firstCalls] { ++firstCalls; }, false);
    AudioParameterAttachment second (secondEditor, parameter (0), [&secondCalls] { ++secondCalls; }, false);

    parameter (0)->setValue (0.5f);
    editor.dispatchPendingParameterChanges();
    secondEditor.dispatchPendingParameterChanges();

    EXPECT_EQ (1, firstCalls);
    EXPECT_EQ (1, secondCalls);

    secondEditor.stopParameterListening();
    parameter (0)->setValue (0.75f);
    editor.dispatchPendingParameterChanges();

    EXPECT_EQ (2, firstCalls);
    EXPECT_EQ (1, secondCalls);
}

TEST_F (AudioParameterAttachmentTests, ReentrantDispatchDefersChangesUntilTheNextDrain)
{
    int calls = 0;
    AudioParameterAttachment attachment (editor, parameter (0), [&]
    {
        if (++calls == 1)
        {
            parameter (0)->setValue (0.75f);
            editor.dispatchPendingParameterChanges();
        }
    }, false);

    parameter (0)->setValue (0.5f);
    editor.dispatchPendingParameterChanges();
    EXPECT_EQ (1, calls);

    editor.dispatchPendingParameterChanges();
    EXPECT_EQ (2, calls);
}

TEST_F (AudioParameterAttachmentTests, SharedParameterNotifiesEachProcessorsEditor)
{
    auto otherProcessor = std::make_unique<TestProcessor>();
    auto sharedInOther = AudioParameterBuilder().withID ("shared").withRange (0.0f, 1.0f).build();
    processor.addParameter (sharedInOther);
    otherProcessor->addParameter (AudioParameterBuilder().withID ("extra").withRange (0.0f, 1.0f).build());
    otherProcessor->addParameter (sharedInOther);

    TestEditor otherEditor;
    otherEditor.startParameterListening (*otherProcessor, 0);
    int firstCalls = 0, secondCalls = 0;
    AudioParameterAttachment first (editor, sharedInOther, [&firstCalls] { ++firstCalls; }, false);
    AudioParameterAttachment second (otherEditor, sharedInOther, [&secondCalls] { ++secondCalls; }, false);

    sharedInOther->setValue (0.5f);
    editor.dispatchPendingParameterChanges();
    otherEditor.dispatchPendingParameterChanges();

    EXPECT_EQ (1, firstCalls);
    EXPECT_EQ (1, secondCalls);

    otherEditor.stopParameterListening();
    otherProcessor.reset();
    sharedInOther->setValue (0.75f);
    editor.dispatchPendingParameterChanges();

    EXPECT_EQ (2, firstCalls);
    EXPECT_EQ (1, secondCalls);
}

#endif
