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

#include <yup_audio_gui/yup_audio_gui.h>
#include <yup_gui_markup/yup_gui_markup.h>

#include <gtest/gtest.h>

using namespace yup;

//==============================================================================
class AudioParameterBindingProviderTests : public ::testing::Test
{
protected:
    struct TestProcessor final : AudioProcessor
    {
        TestProcessor()
            : AudioProcessor ("Binding Test", AudioBusLayout ({}, {}))
        {
            addParameter (AudioParameterBuilder().withID ("cutoff").withName ("Cutoff").withRange (20.0f, 20000.0f).withDefault (1000.0f).build());
            addParameter (AudioParameterBuilder().withID ("gain").withName ("Gain").withRange (0.0f, 1.0f).withDefault (0.5f).build());
            addParameter (AudioParameterBuilder().withID ("level").withName ("Level").withRange (0.0f, 1.0f).withDefault (0.0f).withReadOnly (true).build());
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

    struct BindingRecorder final : BindingProvider::Listener
    {
        void bindingValueChanged (BindingProvider&, const Identifier& name) override { changes.add (name.toString()); }

        StringArray changes;
    };

    struct HostRecorder final : AudioParameter::Listener
    {
        void parameterValueChanged (const AudioParameter::Ptr& parameter, int) override { events.add ("value " + parameter->getID()); }

        void parameterGestureBegin (const AudioParameter::Ptr& parameter, int) override { events.add ("begin " + parameter->getID()); }

        void parameterGestureEnd (const AudioParameter::Ptr& parameter, int) override { events.add ("end " + parameter->getID()); }

        StringArray events;
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

TEST_F (AudioParameterBindingProviderTests, ExposesEachParameterByIdAsANumber)
{
    AudioParameterBindingProvider provider (editor, processor.getParameters());

    const auto properties = provider.getProperties();
    ASSERT_EQ (properties.size(), 3u);
    EXPECT_EQ (properties[0].name, Identifier ("cutoff"));
    EXPECT_EQ (properties[1].name, Identifier ("gain"));
    EXPECT_EQ (properties[2].name, Identifier ("level"));

    for (const auto& property : properties)
        EXPECT_EQ (property.type, ComponentProperty::Type::number);

    EXPECT_FLOAT_EQ (static_cast<float> (provider.getValue ("cutoff")), 1000.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (provider.getValue ("gain")), 0.5f);
    EXPECT_TRUE (provider.getValue ("missing").isVoid());
    EXPECT_EQ (provider.getParameter ("gain"), parameter (1));
    EXPECT_EQ (provider.getParameter ("missing"), nullptr);
}

TEST_F (AudioParameterBindingProviderTests, WritesThroughToTheParameterAndNotifiesTheHost)
{
    AudioParameterBindingProvider provider (editor, processor.getParameters());

    BindingRecorder bindings;
    provider.addListener (&bindings);

    HostRecorder host;
    parameter (0)->addListener (&host);

    provider.setValue ("cutoff", 440.0);
    provider.setValue ("cutoff", 440.0);

    EXPECT_FLOAT_EQ (parameter (0)->getValue(), 440.0f);
    EXPECT_EQ (host.events.joinIntoString (", "), "value cutoff");
    EXPECT_EQ (bindings.changes.joinIntoString (", "), "cutoff");

    parameter (0)->removeListener (&host);
    provider.removeListener (&bindings);
}

TEST_F (AudioParameterBindingProviderTests, IgnoresWritesToReadOnlyParameters)
{
    AudioParameterBindingProvider provider (editor, processor.getParameters());

    HostRecorder host;
    parameter (2)->addListener (&host);

    provider.beginGesture ("level");
    EXPECT_FALSE (parameter (2)->isPerformingChangeGesture());

    provider.setValue ("level", 0.8);
    provider.endGesture ("level");
    provider.setValue ("missing", 0.8);

    EXPECT_FLOAT_EQ (parameter (2)->getValue(), 0.0f);
    EXPECT_TRUE (host.events.isEmpty());

    parameter (2)->removeListener (&host);
}

TEST_F (AudioParameterBindingProviderTests, ReportsChangesMadeElsewhereWhenTheEditorDispatches)
{
    AudioParameterBindingProvider provider (editor, processor.getParameters());

    BindingRecorder bindings;
    provider.addListener (&bindings);

    parameter (1)->setValue (0.75f);
    EXPECT_TRUE (bindings.changes.isEmpty());

    editor.dispatchPendingParameterChanges();
    EXPECT_EQ (bindings.changes.joinIntoString (", "), "gain");
    EXPECT_FLOAT_EQ (static_cast<float> (provider.getValue ("gain")), 0.75f);

    provider.removeListener (&bindings);
}

TEST_F (AudioParameterBindingProviderTests, WrapsGesturesInChangeGestures)
{
    AudioParameterBindingProvider provider (editor, processor.getParameters());

    HostRecorder host;
    parameter (1)->addListener (&host);

    provider.beginGesture ("gain");
    EXPECT_TRUE (parameter (1)->isPerformingChangeGesture());

    provider.setValue ("gain", 0.25);
    provider.endGesture ("gain");
    EXPECT_FALSE (parameter (1)->isPerformingChangeGesture());

    provider.beginGesture ("missing");
    provider.endGesture ("missing");

    EXPECT_EQ (host.events.joinIntoString (", "), "begin gain, value gain, end gain");

    parameter (1)->removeListener (&host);
}

TEST_F (AudioParameterBindingProviderTests, ShowsAndReadsAValueAsTheTextOfItsParameter)
{
    const auto withUnit = AudioParameterBuilder()
                              .withID ("pitch")
                              .withName ("Pitch")
                              .withRange (0.0f, 100.0f)
                              .withDefault (10.0f)
                              .withValueToString ([] (float value) { return String (roundToInt (value)) + " st"; })
                              .withStringToValue ([] (const String& text) { return text.upToFirstOccurrenceOf (" ", false, false).getFloatValue(); })
                              .build();

    const std::vector<AudioParameter::Ptr> parameters { withUnit };
    AudioParameterBindingProvider provider (editor, parameters);

    // The value given, which need not be the current one
    EXPECT_EQ (provider.getTextForValue ("pitch", 24.0).value_or ("none"), "24 st");
    EXPECT_FLOAT_EQ (static_cast<float> (provider.getValueForText ("pitch", "36 st").value_or (var())), 36.0f);
    EXPECT_FLOAT_EQ (withUnit->getValue(), 10.0f);

    // A parameter with no conversion of its own has the default one, and an unknown name has none
    AudioParameterBindingProvider plain (editor, processor.getParameters());
    EXPECT_EQ (plain.getTextForValue ("gain", 0.25).value_or ("none"), parameter (1)->convertToString (0.25f));
    EXPECT_FALSE (plain.getTextForValue ("nothing", 0.25).has_value());
    EXPECT_FALSE (plain.getValueForText ("nothing", "0.25").has_value());
}

TEST_F (AudioParameterBindingProviderTests, DrivesABuiltUserInterface)
{
    AudioParameterBindingProvider provider (editor, processor.getParameters());

    YuiBuilder builder;
    builder.setModel ("params", provider);

    const auto parsed = YuiParser::parse (R"yui(
        Component {
            Slider { id: knob; value <=> params.cutoff; maximum: 20000; minimum: 20; }
        })yui");
    ASSERT_TRUE (parsed.wasOk());

    const auto result = builder.build (parsed.document);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    auto* knob = builder.findById<Slider> ("knob");
    ASSERT_NE (knob, nullptr);
    EXPECT_FLOAT_EQ (static_cast<float> (knob->getValue()), 1000.0f);

    HostRecorder host;
    parameter (0)->addListener (&host);

    knob->setValue (440.0);
    EXPECT_FLOAT_EQ (parameter (0)->getValue(), 440.0f);
    EXPECT_EQ (host.events.joinIntoString (", "), "begin cutoff, value cutoff, end cutoff");

    parameter (0)->removeListener (&host);

    parameter (0)->setValue (5000.0f);
    editor.dispatchPendingParameterChanges();
    EXPECT_FLOAT_EQ (static_cast<float> (knob->getValue()), 5000.0f);
}
