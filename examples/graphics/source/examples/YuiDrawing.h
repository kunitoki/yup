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
        auto& document = view.getDocument();
        document.addListener (this);
        document.setModel ("params", provider);
        document.setAction ("reset", [this] (const yup::Array<yup::var>&) { resetParams(); });
        document.setAction ("undo", [this] (const yup::Array<yup::var>&) { undoManager.undo(); });
        document.setAction ("changed", [this] (const yup::Array<yup::var>& arguments)
        {
            if (arguments.size() == 2)
                setStatus (arguments[0].toString() + " changed to " + yup::String (static_cast<double> (arguments[1]) * 100.0, 0) + " %");
        });

        for (const auto* fileName : { "ui.yui", "components.yui", "knob.yui", "haloknob.yui", "flutedknob.yui", "brushedknob.yui", "palette.yui", "style.yui" })
        {
            auto& source = *sources.emplace_back (std::make_unique<Source> (fileName));
            source.editor.setSyntaxDefinition ("yui");
            source.editor.setMinimapVisible (false);
            source.editor.setScheme (yup::CodeEditorScheme::getBuiltIn ("oneDark"));
            source.document.addListener (this);
            source.editor.setOpaque (false);
            addChildComponent (source.editor);

            tabs.addTab (yup::Identifier (fileName), fileName);
        }

        tabs.setLayout (yup::TabBar::Layout::fill);
        tabs.onSelectionChanged = [this] (const yup::Identifier&) { showSelectedSource(); };
        addAndMakeVisible (tabs);

        status.setMultiLine (true);
        status.setReadOnly (true);
        addAndMakeVisible (status);

        divider.setThickness (20.0f);
        divider.setPosition (yup::ResizableDivider::Length::percent (55.0f));
        divider.setMinPosition (yup::ResizableDivider::Length::pixels (200.0f));
        divider.setMinTrailingSize (yup::ResizableDivider::Length::pixels (200.0f));
        divider.setDefaultPosition (yup::ResizableDivider::Length::percent (55.0f));
        divider.onPositionChanged = [this] (float) { resized(); };
        addAndMakeVisible (divider);

        addAndMakeVisible (view);

        // The tabs show the files after every load. On desktop the loader watches them, so
        // saving one in another editor reloads the interface in place too
        auto& loader = view.getLoader();
        loader.onLoad = [this] (const yup::Result& result)
        {
            lastLoad = result;
            showFiles();
            report();
        };
        loader.loadFile (getAssetPath ("data/yui/ui.yui"));
        loader.setWatching (true);

        tabs.setSelectedTab (yup::Identifier ("ui.yui"));

        // The first tab selects itself while being added, before any callback is attached
        showSelectedSource();

        setSize (900, 520);
        setOpaque (false);
    }

    ~YuiDrawingDemo() override
    {
        stopTimer();
        view.getDocument().removeListener (this);

        for (auto& source : sources)
            source->document.removeListener (this);
    }

    void paint (yup::Graphics& g) override
    {
        const auto* root = view.getDocument().getRoot();
        const auto* palette = root != nullptr ? root->findPalette() : nullptr;

        if (palette == nullptr)
            return;

        g.setFillColor (palette->getColor (yup::ThemePalette::Role::background));
        g.fillRoundedRect (view.getBounds(), 8.0f);
    }

    void resized() override
    {
        divider.layoutComponents (getLocalBounds().reduced (10), &leftPane, &rightPane);

        auto left = leftPane.getBounds();
        tabs.setBounds (left.removeFromTop (32.0f));

        for (auto& source : sources)
            source->editor.setBounds (left.withTrimmedTop (4.0f));

        auto right = rightPane.getBounds();
        auto footer = right.removeFromBottom (114.0f);
        status.setBounds (footer.removeFromBottom (84.0f));

        view.setBounds (right.withTrimmedBottom (6.0f));
    }

    void codeDocumentChanged (yup::CodeDocument&, const yup::CodeDocument::Change&) override
    {
        // Typing restarts the countdown, so a burst of keystrokes rebuilds once
        if (! showingFiles)
            startTimer (150);
    }

private:
    void timerCallback() override
    {
        stopTimer();
        rebuild();
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

    void showFiles()
    {
        const yup::ScopedValueSetter<bool> scope (showingFiles, true);

        for (auto& source : sources)
        {
            const auto text = view.getLoader().getLoadedText (source->fileName);

            if (text.has_value() && *text != source->document.getText())
                source->editor.setText (*text);
        }
    }

    // A tab differing from the text loaded was edited: its text overrides the file from now on,
    // so a save in another editor reloads only the files not edited here
    void rebuild()
    {
        auto& loader = view.getLoader();

        for (auto& source : sources)
        {
            const auto text = source->document.getText();

            if (loader.getLoadedText (source->fileName) != text)
                loader.setOverride (source->fileName, text);
        }

        loader.reload();
    }

    // Loads are reported by the loader, style problems met later, when a rule starts matching, are marked too
    void documentLoaded (yup::YuiDocument&, const yup::Result&) override {}

    void documentDiagnosed (yup::YuiDocument&, const yup::YuiDiagnostic&) override
    {
        report();
    }

    void report()
    {
        for (auto& source : sources)
            source->editor.clearErrorLines();

        const auto& problems = view.getDocument().getDiagnostics();

        // A failed load keeps the interface loaded before, its problems are marked where they are written
        for (const auto& problem : problems)
        {
            if (! problem.isError() || problem.line <= 0)
                continue;

            if (auto* source = findSource (problem.source.isEmpty() ? sources.front()->fileName : problem.source))
                source->editor.setErrorLine (problem.line - 1, true);
        }

        if (lastLoad.failed())
            setStatus ("Error: " + lastLoad.getErrorMessage());
        else if (! problems.empty())
            setStatus (yup::YuiDiagnostic::join (problems, "\n"));
        else
            setStatus ("Built with " + yup::String (view.getDocument().getBuilder().getBindings().getNumBindings()) + " bindings. Drag a knob, undo, or edit any tab"
                       + (view.getLoader().isWatching() ? " or file." : "."));

        repaint();
    }

    void setStatus (const yup::String& text)
    {
        status.setText (text, yup::dontSendNotification);
    }

    // The model outlives the document of the view, which listens to it
    yup::DataTree params = makeParams();
    yup::UndoManager undoManager;
    yup::DataTreeBindingProvider provider { params, &undoManager };
    yup::YuiView view { "preview" };

    std::vector<std::unique_ptr<Source>> sources;
    yup::TabBar tabs { "tabs" };
    yup::TextEditor status { "status" };
    yup::ResizableDivider divider { yup::ResizableDivider::Orientation::vertical, "divider" };
    yup::Component leftPane, rightPane;
    yup::Result lastLoad = yup::Result::ok();
    bool showingFiles = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YuiDrawingDemo)
};
