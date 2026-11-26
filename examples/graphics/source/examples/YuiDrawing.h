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

#include <yup_audio_gui/yup_audio_gui.h>
#include <yup_gui_markup/yup_gui_markup.h>

//==============================================================================

class YuiDrawingDemo
    : public yup::Component
    , public yup::CodeDocument::Listener
    , private yup::YuiDocument::Listener
    , private yup::Timer
{
public:
    YuiDrawingDemo()
        : Component ("YuiDrawingDemo")
    {
        for (const auto& example : getExamples())
            selector.addTab (yup::Identifier (example.name), example.name);

        selector.setLayout (yup::TabBar::Layout::fill);
        selector.setOverflow (yup::TabBar::Overflow::shrink);
        selector.onSelectionChanged = [this] (const yup::Identifier& selected)
        {
            for (const auto& example : getExamples())
                if (selected == yup::Identifier (example.name))
                    showExample (example);
        };
        addAndMakeVisible (selector);

        tabs.setLayout (yup::TabBar::Layout::fill);
        tabs.setVariant (yup::TabBar::Variant::underline);
        tabs.onSelectionChanged = [this] (const yup::Identifier&) { showSelectedSource(); };
        addAndMakeVisible (tabs);

        status.setMultiLine (true);
        status.setReadOnly (true);
        addAndMakeVisible (status);

        divider.setThickness (20.0f);
        divider.setPosition (yup::ResizableDivider::Length::percent (45.0f));
        divider.setMinPosition (yup::ResizableDivider::Length::pixels (200.0f));
        divider.setMinTrailingSize (yup::ResizableDivider::Length::pixels (200.0f));
        divider.setDefaultPosition (yup::ResizableDivider::Length::percent (45.0f));
        divider.onPositionChanged = [this] (float) { resized(); };
        addAndMakeVisible (divider);

        // The first tab selected itself while being added, before any callback was attached
        showExample (getExamples().front());

        setSize (900, 520);
        setOpaque (false);

        startTimerHz (ticksPerSecond);
    }

    ~YuiDrawingDemo() override
    {
        stopTimer();

        if (view != nullptr)
            view->getDocument().removeListener (this);

        for (auto& source : sources)
            source->document.removeListener (this);
    }

    void paint (yup::Graphics& g) override
    {
        const auto* root = view != nullptr ? view->getDocument().getRoot() : nullptr;
        const auto* palette = root != nullptr ? root->findPalette() : nullptr;

        if (palette == nullptr)
            return;

        g.setFillColor (palette->getColor (yup::ThemePalette::Role::background));
        g.fillRoundedRect (view->getBounds(), 8.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (10);
        selector.setBounds (bounds.removeFromTop (32.0f));
        bounds.removeFromTop (6.0f);

        divider.layoutComponents (bounds, &leftPane, &rightPane);

        auto left = leftPane.getBounds();
        tabs.setBounds (left.removeFromTop (32.0f));

        for (auto& source : sources)
            source->editor.setBounds (left.withTrimmedTop (4.0f));

        auto right = rightPane.getBounds();
        status.setBounds (right.removeFromBottom (72.0f));

        if (view != nullptr)
            view->setBounds (right.withTrimmedBottom (6.0f));
    }

    void codeDocumentChanged (yup::CodeDocument&, const yup::CodeDocument::Change&) override
    {
        // Typing restarts the countdown, so a burst of keystrokes rebuilds once
        if (! showingFiles)
            ticksUntilRebuild = ticksBeforeRebuild;
    }

private:
    // A document, then what it imports and the shaders it runs, as it names them: each is a tab, and what is typed
    // in one is what the document loads. The files are in a folder of data/yui, or in data/yui itself
    struct Example
    {
        const char* name;
        const char* folder;
        std::vector<const char*> files;
        bool usesParams = false;
    };

    static const std::vector<Example>& getExamples()
    {
        static const std::vector<Example> examples {
            { "Filter", "filter", { "ui.yui", "components.yui", "scope.yui", "knob.yui", "haloknob.yui", "flutedknob.yui", "brushedknob.yui", "palette.yui", "style.yui" }, true },
            { "Widgets", "", { "widgets.yui" } },
            { "Lists", "", { "lists.yui" } },
            { "Layout", "", { "layout.yui" } },
            { "Styles", "", { "styles.yui" } },
            { "Bindings", "", { "bindings.yui" } },
            { "Drawing", "", { "drawing.yui" } },
            { "Series", "", { "series.yui" } },
            { "Parts", "", { "parts.yui" } },
            { "Controls", "", { "controls.yui" } },
            { "Audio", "", { "audio.yui" } },
            { "Shaders", "", { "shaders.yui", "fx/spectrum.frag" } }
        };

        return examples;
    }

    struct Source
    {
        explicit Source (const char* name)
            : fileName (name)
            , editor (document)
        {
        }

        yup::String fileName;
        yup::CodeDocument document;
        yup::CodeEditor editor;
    };

    void timerCallback() override
    {
        feedAudio();

        if (ticksUntilRebuild > 0 && --ticksUntilRebuild == 0)
            rebuild();
    }

    // What the audio widgets of a document show: a chord swelling and fading with a partial sweeping over it for the
    // analyzers and the meters, and an arpeggio for the keyboards
    void feedAudio()
    {
        static constexpr int numSamples = static_cast<int> (sampleRate) / ticksPerSecond;
        static constexpr int notes[] = { 48, 52, 55, 60, 64, 67, 72, 67, 64, 60, 55, 52 };

        std::array<float, numSamples> left {};
        std::array<float, numSamples> right {};

        const auto advance = [] (double& phase, double frequency)
        {
            phase += yup::MathConstants<double>::twoPi * frequency / sampleRate;

            if (phase > yup::MathConstants<double>::twoPi)
                phase -= yup::MathConstants<double>::twoPi;

            return std::sin (phase);
        };

        for (int i = 0; i < numSamples; ++i)
        {
            const auto swell = 0.5 + 0.5 * advance (swellPhase, 0.35);
            const auto sweep = 2400.0 + 2000.0 * advance (sweepPhase, 0.11);

            const auto sample = swell * (0.45 * advance (lowPhase, 220.0) + 0.3 * advance (midPhase, 880.0))
                              + 0.2 * advance (highPhase, sweep);

            left[static_cast<std::size_t> (i)] = static_cast<float> (sample);
            right[static_cast<std::size_t> (i)] = static_cast<float> (sample * 0.5);
        }

        for (auto& state : analyzerStates)
            state->pushSamples (left.data(), numSamples);

        const float* channels[2] = { left.data(), right.data() };
        meterState.pushSamples (channels, 2, numSamples);
        meterState.processPendingAudio();

        if (++ticksSinceNote >= ticksPerNote)
        {
            ticksSinceNote = 0;

            if (noteIsOn)
                keyboardState.noteOff (1, notes[noteIndex], 0.0f);

            noteIndex = (noteIndex + 1) % std::size (notes);
            keyboardState.noteOn (1, notes[noteIndex], 0.8f);
            noteIsOn = true;
        }
    }

    // Each example gets a view of its own: nothing is kept of the one shown before, and a document that can't be
    // loaded shows nothing rather than the example before it
    void showExample (const Example& example)
    {
        ticksUntilRebuild = 0;

        if (view != nullptr)
            view->getDocument().removeListener (this);

        sources.clear();
        tabs.clearTabs();
        lastAction.clear();

        // The states are let go once the components showing them are
        view.reset();
        analyzerStates.clear();

        view = std::make_unique<yup::YuiView> ("preview");
        addAndMakeVisible (*view);

        for (const auto* fileName : example.files)
        {
            auto& source = *sources.emplace_back (std::make_unique<Source> (fileName));
            source.editor.setSyntaxDefinitionForExtension (source.fileName.fromLastOccurrenceOf (".", false, false));
            source.editor.setMinimapVisible (false);
            source.editor.setScheme (yup::CodeEditorScheme::getBuiltIn ("oneDark"));
            source.document.addListener (this);
            source.editor.setOpaque (false);
            addChildComponent (source.editor);

            tabs.addTab (yup::Identifier (fileName), fileName);
        }

        auto& document = view->getDocument();
        document.addListener (this);

        // The Filter is wired to a model owned by the page, so its values outlive a rebuild and every change is one
        // undo step. The other documents are complete by themselves
        if (example.usesParams)
        {
            document.setModel ("params", provider);
            document.setAction ("reset", [this] (const yup::Array<yup::var>&) { resetParams(); });
            document.setAction ("undo", [this] (const yup::Array<yup::var>&) { undoManager.undo(); });
        }

        // The actions a document raises are shown as they are called
        document.setFallbackAction ([this] (const yup::String& name, const yup::Array<yup::var>& arguments)
        {
            yup::StringArray values (name.quoted());

            for (const auto& argument : arguments)
                values.add (argument.isString() ? argument.toString().quoted() : argument.toString());

            lastAction = "action (" + values.joinIntoString (", ") + ")";
            report();
        });

        // What a document can't write is given to the components it creates: the states the audio widgets show, an
        // analyzer having one of its own
        document.setInitializer<yup::SpectrumAnalyzerComponent> ([this] (yup::SpectrumAnalyzerComponent& analyzer)
        {
            analyzer.setState (analyzerStates.emplace_back (std::make_unique<yup::SpectrumAnalyzerState> (2048, 2048)).get());
            analyzer.setSampleRate (sampleRate);
        });

        document.setInitializer<yup::KMeterComponent> ([this] (yup::KMeterComponent& meter) { meter.setState (&meterState); });
        document.setInitializer<yup::MidiKeyboardComponent> ([this] (yup::MidiKeyboardComponent& keyboard) { keyboard.setState (&keyboardState); });
        document.setInitializer<yup::PitchWheelComponent> ([this] (yup::PitchWheelComponent& wheel) { wheel.setState (&keyboardState); });
        document.setInitializer<yup::ModWheelComponent> ([this] (yup::ModWheelComponent& wheel) { wheel.setState (&keyboardState); });

        // The tabs show the files after every load. On desktop the loader watches them, the shader sources too, so
        // saving one in another editor loads again and updates the interface in place
        auto& loader = view->getLoader();
        loader.onLoad = [this] (const yup::Result& result)
        {
            lastLoad = result;
            showFiles();
            report();
        };

        auto folder = getAssetPath ("data/yui");

        if (! yup::String (example.folder).isEmpty())
            folder = folder.getChildFile (example.folder);

        loader.loadFile (folder.getChildFile (example.files.front()));
        loader.setWatching (true);

        showSelectedSource();
        resized();
    }

    Source* findSource (const yup::String& fileName) const
    {
        for (auto& source : sources)
            if (source->fileName == fileName)
                return source.get();

        return nullptr;
    }

    void showSelectedSource()
    {
        const auto selected = tabs.getSelectedTabId().toString();

        for (auto& source : sources)
            source->editor.setVisible (source->fileName == selected);
    }

    static yup::DataTree makeParams()
    {
        yup::DataTree tree ("params");

        auto transaction = tree.beginTransaction();
        transaction.setProperty ("mode", 0);
        transaction.setProperty ("cutoff", 0.65);
        transaction.setProperty ("resonance", 0.3);
        transaction.setProperty ("drive", 0.2);
        transaction.setProperty ("mix", 0.5);

        return tree;
    }

    void resetParams()
    {
        undoManager.beginNewTransaction ("Reset");

        const auto defaults = makeParams();
        auto transaction = params.beginTransaction (&undoManager);

        for (int i = 0; i < defaults.getNumProperties(); ++i)
        {
            const auto name = defaults.getPropertyName (i);
            transaction.setProperty (name, defaults.getProperty (name));
        }
    }

    // The text of a document or of what it imports as it was last loaded, and of a shader as the document reads
    // it: from its file, or from what was typed in its tab
    std::optional<yup::String> readSource (const Source& source) const
    {
        if (source.fileName.endsWithIgnoreCase (".yui"))
            return view->getLoader().getLoadedText (source.fileName);

        if (const auto bytes = view->getDocument().readAsset (source.fileName))
            return bytes->toString();

        return std::nullopt;
    }

    void showFiles()
    {
        const yup::ScopedValueSetter<bool> scope (showingFiles, true);

        for (auto& source : sources)
        {
            const auto text = readSource (*source);

            if (text.has_value() && *text != source->document.getText())
                source->editor.setText (*text);
        }
    }

    // A tab differing from the text loaded was edited: its text overrides the file from now on, so a save in another
    // editor reloads only the files not edited here. The source of a shader goes the same way as the document: the
    // reload tells what compiled it that it changed
    void rebuild()
    {
        if (view == nullptr)
            return;

        auto& loader = view->getLoader();

        for (auto& source : sources)
        {
            const auto text = source->document.getText();

            if (readSource (*source) != text)
                loader.setOverride (source->fileName, text);
        }

        loader.reload();
    }

    // Loads are reported by the loader, the problems met later are marked too: a rule that starts matching, or a
    // shader compiled when it is first used
    void documentLoaded (yup::YuiDocument&, const yup::Result&) override {}

    void documentDiagnosed (yup::YuiDocument&, const yup::DataTreeDiagnostic&) override
    {
        report();
    }

    void report()
    {
        if (view == nullptr)
            return;

        for (auto& source : sources)
            source->editor.clearErrorLines();

        const auto& problems = view->getDocument().getDiagnostics();

        // A failed load keeps the interface loaded before, its problems are marked where they are written
        for (const auto& problem : problems)
        {
            if (! problem.isError() || problem.line <= 0)
                continue;

            if (auto* source = findSource (problem.source.isEmpty() ? sources.front()->fileName : problem.source))
                source->editor.setErrorLine (problem.line - 1, true);
        }

        yup::StringArray lines;

        if (lastAction.isNotEmpty())
            lines.add (lastAction);

        if (lastLoad.failed())
            lines.add ("Error: " + lastLoad.getErrorMessage());
        else if (! problems.empty())
            lines.add (yup::DataTreeDiagnostic::join (problems, "\n"));
        else
            lines.add ("Built with " + yup::String (view->getDocument().getBuilder().getBindings().getNumBindings()) + " bindings. Edit any tab"
                       + (view->getLoader().isWatching() ? " or file." : "."));

        status.setText (lines.joinIntoString ("\n"), yup::dontSendNotification);
        repaint();
    }

    static constexpr double sampleRate = 48000.0;
    static constexpr int ticksPerSecond = 30;
    static constexpr int ticksBeforeRebuild = 5;
    static constexpr int ticksPerNote = 8;

    // The model of the Filter outlives the documents, which listen to it
    yup::DataTree params = makeParams();
    yup::UndoManager undoManager;
    yup::DataTreeBindingProvider provider { params, &undoManager };

    // What the audio widgets show outlives the view holding them
    yup::MidiKeyboardState keyboardState;
    yup::KMeterState meterState { sampleRate, 2 };
    std::vector<std::unique_ptr<yup::SpectrumAnalyzerState>> analyzerStates;
    double lowPhase = 0.0, midPhase = 0.0, highPhase = 0.0, sweepPhase = 0.0, swellPhase = 0.0;
    std::size_t noteIndex = 0;
    int ticksSinceNote = 0;
    bool noteIsOn = false;

    std::unique_ptr<yup::YuiView> view;

    std::vector<std::unique_ptr<Source>> sources;
    yup::TabBar selector { "selector" };
    yup::TabBar tabs { "tabs" };
    yup::TextEditor status { "status" };
    yup::ResizableDivider divider { yup::ResizableDivider::Orientation::vertical, "divider" };
    yup::Component leftPane, rightPane;
    yup::Result lastLoad = yup::Result::ok();
    yup::String lastAction;
    int ticksUntilRebuild = 0;
    bool showingFiles = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YuiDrawingDemo)
};
