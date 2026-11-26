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
class AudioGuiModuleIntegrationTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        messageManager = MessageManager::getInstance();
    }

    void runDispatchLoopUntil (int millisecondsToRunFor = 50)
    {
#if YUP_MODAL_LOOPS_PERMITTED
        messageManager->runDispatchLoopUntil (millisecondsToRunFor);
#else
        ignoreUnused (millisecondsToRunFor);
#endif
    }

    /** Makes a theme of its own the global one for as long as it lives. */
    struct ScopedTheme
    {
        ScopedTheme()
            : previous (ApplicationTheme::getGlobalTheme())
            , theme (new ApplicationTheme())
        {
            ApplicationTheme::setGlobalTheme (theme);
        }

        ~ScopedTheme()
        {
            ApplicationTheme::setGlobalTheme (previous.get());
        }

        ApplicationTheme::Ptr previous;
        ApplicationTheme::Ptr theme;
    };

    MessageManager* messageManager = nullptr;
};

//==============================================================================
TEST_F (AudioGuiModuleIntegrationTests, TheAudioTypesAreRegisteredWithTheOnesADocumentIsWrittenWith)
{
    registerYuiComponentTypes();

    const auto& registry = ComponentTypeRegistry::getInstance();

    for (const auto* name : { "MidiKeyboard", "PitchWheel", "ModWheel", "SpectrumAnalyzer", "Spectrogram", "KMeter", "AudioView", "CartesianPlane" })
    {
        const auto* type = registry.findType (Identifier (name));

        ASSERT_NE (type, nullptr) << name;
        EXPECT_TRUE (type->hasFactory()) << name;
        EXPECT_EQ (type->getBaseTypeInfo(), &Component::getClassTypeInfo()) << name;
    }

    EXPECT_EQ (registry.findType (Identifier ("MidiKeyboard")), &MidiKeyboardComponent::getClassTypeInfo());
    EXPECT_EQ (registry.findType (Identifier ("KMeter")), &KMeterComponent::getClassTypeInfo());
}

TEST_F (AudioGuiModuleIntegrationTests, ADocumentBuildsTheAudioComponentsWithTheirProperties)
{
    const auto parsed = YuiParser::parse (R"yui(
        Column {
            SpectrumAnalyzer { id: analyzer; displayType: lines; windowType: blackman; minFrequency: 30; maxFrequency: 18000; }
            Spectrogram { id: spectrogram; colorMap: viridis; historyFrames: 64; }
            CartesianPlane { id: plot; title: "Response"; xMin: 20; xMax: 20000; xScaleType: logarithmic; legendVisible: false; }
            AudioView { id: waveform; channelLabelsVisible: false; }

            Row {
                KMeter { id: left; channel: 0; scaleMapping: linear; }
                ModWheel { id: mod; value: 0.25; }
                PitchWheel { id: pitch; resetOnRelease: false; }
                MidiKeyboard { id: keys; orientation: verticalFacingLeft; midiChannel: 2; lowestVisibleKey: 100; highestVisibleKey: 110; }
            }
        })yui");
    ASSERT_TRUE (parsed.wasOk()) << DataTreeDiagnostic::join (parsed.diagnostics);

    YuiBuilder builder;
    const auto result = builder.build (parsed.document);

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (builder.getDiagnostics().empty()) << DataTreeDiagnostic::join (builder.getDiagnostics());

    auto* analyzer = builder.findById<SpectrumAnalyzerComponent> ("analyzer");
    ASSERT_NE (analyzer, nullptr);
    EXPECT_EQ (analyzer->getState(), nullptr);
    EXPECT_EQ (analyzer->getDisplayType(), SpectrumAnalyzerComponent::DisplayType::lines);
    EXPECT_EQ (analyzer->getWindowType(), WindowType::blackman);
    EXPECT_FLOAT_EQ (analyzer->getMinFrequency(), 30.0f);
    EXPECT_FLOAT_EQ (analyzer->getMaxFrequency(), 18000.0f);

    auto* spectrogram = builder.findById<SpectrogramComponent> ("spectrogram");
    ASSERT_NE (spectrogram, nullptr);
    EXPECT_EQ (spectrogram->getColorMap().getType(), SpectrogramColorMap::Type::viridis);
    EXPECT_EQ (spectrogram->getNumHistoryFrames(), 64);

    // The range is set before the scale, which is logarithmic above zero only
    auto* plot = builder.findById<CartesianPlane> ("plot");
    ASSERT_NE (plot, nullptr);
    EXPECT_EQ (plot->getTitle(), "Response");
    EXPECT_DOUBLE_EQ (plot->getXRange().getStart(), 20.0);
    EXPECT_DOUBLE_EQ (plot->getXRange().getEnd(), 20000.0);
    EXPECT_EQ (plot->getXScaleType(), CartesianPlane::AxisScaleType::logarithmic);
    EXPECT_FALSE (plot->isLegendVisible());

    auto* waveform = builder.findById<AudioViewComponent> ("waveform");
    ASSERT_NE (waveform, nullptr);
    EXPECT_FALSE (waveform->isChannelLabelsVisible());

    auto* left = builder.findById<KMeterComponent> ("left");
    ASSERT_NE (left, nullptr);
    EXPECT_EQ (left->getState(), nullptr);
    EXPECT_EQ (left->getChannel(), 0);
    EXPECT_EQ (left->getScaleMapping(), KMeterComponent::ScaleMapping::linear);

    auto* mod = builder.findById<ModWheelComponent> ("mod");
    ASSERT_NE (mod, nullptr);
    EXPECT_DOUBLE_EQ (mod->getValue(), 0.25);

    auto* pitch = builder.findById<PitchWheelComponent> ("pitch");
    ASSERT_NE (pitch, nullptr);
    EXPECT_FALSE (pitch->getResetOnRelease());

    // The keys shown are written above the ones a new keyboard shows
    auto* keys = builder.findById<MidiKeyboardComponent> ("keys");
    ASSERT_NE (keys, nullptr);
    EXPECT_EQ (keys->getState(), nullptr);
    EXPECT_EQ (keys->getOrientation(), MidiKeyboardComponent::verticalKeyboardFacingLeft);
    EXPECT_EQ (keys->getMidiChannel(), 2);
    EXPECT_EQ (keys->getLowestVisibleKey(), 100);
    EXPECT_EQ (keys->getHighestVisibleKey(), 110);
}

TEST_F (AudioGuiModuleIntegrationTests, InitializersGiveTheComponentsOfADocumentTheirStates)
{
    MidiKeyboardState keyboardState;
    SpectrumAnalyzerState inputState (1024);
    SpectrumAnalyzerState outputState (2048);
    KMeterState meterState (48000.0, 2);

    YuiDocument document;

    document.setInitializer<MidiKeyboardComponent> ([&] (MidiKeyboardComponent& keyboard) { keyboard.setState (&keyboardState); });
    document.setInitializer<ModWheelComponent> ([&] (ModWheelComponent& wheel) { wheel.setState (&keyboardState); });
    document.setInitializer<PitchWheelComponent> ([&] (PitchWheelComponent& wheel) { wheel.setState (&keyboardState); });
    document.setInitializer<KMeterComponent> ([&] (KMeterComponent& meter) { meter.setState (&meterState); });

    // The one of an id wins over the one of the type, though it is set first
    document.setInitializer<SpectrumAnalyzerComponent> ("output", [&] (SpectrumAnalyzerComponent& analyzer) { analyzer.setState (&outputState); });
    document.setInitializer<SpectrumAnalyzerComponent> ([&] (SpectrumAnalyzerComponent& analyzer) { analyzer.setState (&inputState); });

    const auto loaded = document.loadFromString (R"yui(
        Column {
            SpectrumAnalyzer { id: input; }
            SpectrumAnalyzer { id: output; }

            Row {
                KMeter { id: left; channel: 0; }
                ModWheel { id: mod; }
                PitchWheel { id: pitch; }
                MidiKeyboard { id: keys; }
            }
        })yui");

    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    auto* keys = document.findById<MidiKeyboardComponent> ("keys");
    auto* output = document.findById<SpectrumAnalyzerComponent> ("output");
    ASSERT_NE (keys, nullptr);
    ASSERT_NE (output, nullptr);

    EXPECT_EQ (keys->getState(), &keyboardState);
    EXPECT_EQ (document.findById<ModWheelComponent> ("mod")->getState(), &keyboardState);
    EXPECT_EQ (document.findById<PitchWheelComponent> ("pitch")->getState(), &keyboardState);
    EXPECT_EQ (document.findById<KMeterComponent> ("left")->getState(), &meterState);
    EXPECT_EQ (document.findById<SpectrumAnalyzerComponent> ("input")->getState(), &inputState);
    EXPECT_EQ (output->getState(), &outputState);
    EXPECT_EQ (output->getFFTSize(), 2048);

    // A note played on the keyboard of the document reaches the state of the application
    keys->handleNoteKey (KeyPress ('z'), true);
    EXPECT_TRUE (keyboardState.isNoteOn (1, 36));

    keys->handleNoteKey (KeyPress ('z'), false);
    EXPECT_FALSE (keyboardState.isNoteOn (1, 36));

    // A reload hands the state to the keyboard it creates, and leaves the one it keeps as it is
    const auto reloaded = document.loadFromString (R"yui(
        Column {
            SpectrumAnalyzer { id: input; }
            SpectrumAnalyzer { id: output; }

            Row {
                KMeter { id: left; channel: 0; }
                ModWheel { id: mod; }
                PitchWheel { id: pitch; }
                MidiKeyboard { id: keys; }
                MidiKeyboard { id: more; highestVisibleKey: 60; }
            }
        })yui");

    ASSERT_TRUE (reloaded.wasOk()) << reloaded.getErrorMessage();
    EXPECT_EQ (document.findById<MidiKeyboardComponent> ("keys"), keys);

    auto* more = document.findById<MidiKeyboardComponent> ("more");
    ASSERT_NE (more, nullptr);
    EXPECT_EQ (more->getState(), &keyboardState);
    EXPECT_EQ (more->getHighestVisibleKey(), 60);
}

TEST_F (AudioGuiModuleIntegrationTests, AWheelFollowsTheStateItIsGivenAfterItsConstruction)
{
    MidiKeyboardState first;
    MidiKeyboardState second;
    ModWheelComponent mod;
    PitchWheelComponent pitch;

    EXPECT_EQ (mod.getState(), nullptr);
    EXPECT_EQ (pitch.getState(), nullptr);

    // Without a state a wheel is moved by the user alone
    mod.setValue (0.5);
    mod.handleAsyncUpdate();
    EXPECT_DOUBLE_EQ (mod.getValue(), 0.5);

    // A wheel keeps its value until its state reports another one
    mod.setState (&first);
    pitch.setState (&first);
    EXPECT_DOUBLE_EQ (mod.getValue(), 0.5);
    EXPECT_DOUBLE_EQ (pitch.getValue(), 0.0);

    first.processNextMidiEvent (MidiMessage::controllerEvent (1, 1, 127));
    first.processNextMidiEvent (MidiMessage::pitchWheel (1, 16383));
    mod.handleAsyncUpdate();
    pitch.handleAsyncUpdate();
    EXPECT_DOUBLE_EQ (mod.getValue(), 1.0);
    EXPECT_DOUBLE_EQ (pitch.getValue(), 1.0);

    // What the previous state reported and isn't shown yet is not read from the new one, whose wheel never moved
    pitch.setState (&second);
    runDispatchLoopUntil();
    EXPECT_EQ (pitch.getState(), &second);
    EXPECT_DOUBLE_EQ (pitch.getValue(), 1.0);

    // Only the state it has moves it
    first.processNextMidiEvent (MidiMessage::pitchWheel (1, 0));
    runDispatchLoopUntil();
    EXPECT_DOUBLE_EQ (pitch.getValue(), 1.0);

    second.processNextMidiEvent (MidiMessage::pitchWheel (1, 8192));
    pitch.handleAsyncUpdate();
    EXPECT_DOUBLE_EQ (pitch.getValue(), 0.0);

    mod.setState (nullptr);
    pitch.setState (nullptr);
    EXPECT_EQ (mod.getState(), nullptr);
    EXPECT_EQ (pitch.getState(), nullptr);
}

TEST_F (AudioGuiModuleIntegrationTests, DrawingsPaintTheAudioComponentsFromTheSeriesTheyTell)
{
    const auto figure = File (__FILE__)
                            .getParentDirectory()
                            .getParentDirectory()
                            .getParentDirectory()
                            .getChildFile ("examples/docsimg/figures/ui/yui/widgets_audio.yui");

    if (! figure.existsAsFile())
        GTEST_SKIP() << "The figures of the documentation are not available";

    const ScopedTheme scopedTheme;

    {
        MidiKeyboardState keyboardState;
        YuiDocument document;

        const auto loaded = document.loadFromString (figure.loadFileAsString());
        ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
        ASSERT_NE (document.getRoot(), nullptr);

        document.getRoot()->setBounds (0.0f, 0.0f, 560.0f, 190.0f);
        document.getRoot()->getStyleContext()->restyleNow();
        EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

        auto* analyzer = document.findById<SpectrumAnalyzerComponent> ("analyzer");
        auto* meter = document.findById<KMeterComponent> ("meter");
        auto* keys = document.findById<MidiKeyboardComponent> ("keys");
        ASSERT_NE (analyzer, nullptr);
        ASSERT_NE (meter, nullptr);
        ASSERT_NE (keys, nullptr);

        auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
        auto renderer = context->makeRenderer (560, 190);
        Graphics g (*context, *renderer);

        const auto measure = [] (Component& component, const String& id)
        {
            auto* style = dynamic_cast<DrawingStyle*> (component.getStyle().get());
            EXPECT_NE (style, nullptr);

            if (style != nullptr)
                for (const auto& element : style->measureElements (component))
                    if (element.id == id)
                        return element;

            return DrawingStyle::ElementBounds();
        };

        // The curve of the analyzer has one vertex for each unit of its width, reading the levels at rest
        const auto curve = measure (*analyzer, "curve");
        EXPECT_EQ (curve.type, String ("vertex"));
        EXPECT_GT (analyzer->getWidth(), 1.0f);
        EXPECT_EQ (curve.numRows, static_cast<int> (analyzer->getWidth()));

        // The keys are the ones the keyboard shows, each where the keyboard puts it
        const auto numKeys = keys->getHighestVisibleKey() - keys->getLowestVisibleKey() + 1;
        EXPECT_EQ (measure (*keys, "white").numRows, numKeys);
        EXPECT_EQ (measure (*keys, "black").numRows, numKeys);

        const auto keyBounds = keys->getProperty (Identifier ("keyBounds"));
        ASSERT_EQ (keyBounds.size(), numKeys);
        EXPECT_EQ (VariantConverter<Rectangle<float>>::fromVar (keyBounds[0]), keys->getRectangleForKey (keys->getLowestVisibleKey()));

        // A note played on the state the keyboard is given is a key told as pressed
        keys->setState (&keyboardState);
        keyboardState.noteOn (1, keys->getLowestVisibleKey() + 2, 1.0f);

        const auto pressed = keys->getProperty (Identifier ("keyPressed"));
        ASSERT_EQ (pressed.size(), numKeys);
        EXPECT_FALSE (static_cast<bool> (pressed[0]));
        EXPECT_TRUE (static_cast<bool> (pressed[2]));

        // The drawings paint all three, the meter at rest
        for (auto* component : { static_cast<Component*> (analyzer), static_cast<Component*> (meter), static_cast<Component*> (keys) })
        {
            ASSERT_NE (component->getStyle().get(), nullptr);
            component->getStyle()->paint (g, *scopedTheme.theme, *component);
        }

        EXPECT_FLOAT_EQ (measure (*meter, "level").bounds.getHeight(), 0.0f);

        keys->setState (nullptr);
    }
}

TEST_F (AudioGuiModuleIntegrationTests, AnAliasOfASeriesIsAValueOfTheComposite)
{
    YuiDocument document;

    const auto loaded = document.loadFromString (R"yui(
        component Scope : Column {
            property alias levels <=> analyzer.levels;
            SpectrumAnalyzer { id: analyzer; }
        }

        Scope { })yui");
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    const auto* type = document.findTypeInfo ("Scope");
    ASSERT_NE (type, nullptr);

    const auto* levels = type->findProperty (Identifier ("levels"));
    ASSERT_NE (levels, nullptr);
    EXPECT_EQ (levels->type, ComponentProperty::Type::array);

    // The levels are in the memory of the analyzer: the composite has their name, and no such memory to be read in place
    EXPECT_FALSE (levels->isSeries());
    EXPECT_TRUE (levels->arrayGetter == nullptr);
}

