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

#include <yup_renderer/yup_renderer.h>

//==============================================================================

class Yui3DDemo
    : public yup::Component
    , public yup::CodeDocument::Listener
    , private yup::YuiDocument::Listener
    , private yup::Timer
{
public:
    Yui3DDemo()
        : Component ("Yui3DDemo")
    {
#if YUP_ENABLE_SHADER_TRANSPILER && (YUP_WASM || YUP_MOBILE)
        // The materials are compiled from their sources, around the template of yup_renderer: where the sources of the
        // module are not at runtime, it is bundled with the data - see CMakeLists.txt
        yup::MaterialShader::setSourceDirectory (getAssetPath ("data/yup_renderer/shaders"));
#endif

        for (const auto& example : getExamples())
            selector.addTab (yup::Identifier (example.files.front()), example.name);

        selector.onSelectionChanged = [this] (const yup::Identifier& selected)
        {
            for (const auto& example : getExamples())
                if (selected == yup::Identifier (example.files.front()))
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

        // The scene above its sources: a view wider than tall, whatever the shape of the window
        divider.setThickness (20.0f);
        divider.setPosition (yup::ResizableDivider::Length::percent (62.0f));
        divider.setMinPosition (yup::ResizableDivider::Length::pixels (160.0f));
        divider.setMinTrailingSize (yup::ResizableDivider::Length::pixels (120.0f));
        divider.setDefaultPosition (yup::ResizableDivider::Length::percent (62.0f));
        divider.onPositionChanged = [this] (float) { resized(); };
        addAndMakeVisible (divider);

        // The first tab selected itself while being added, before any callback was attached
        showExample (getExamples().front());

        setSize (900, 520);
        setOpaque (false);
    }

    ~Yui3DDemo() override
    {
        stopTimer();
        view->getDocument().removeListener (this);

        for (auto& source : sources)
            source->document.removeListener (this);
    }

    void paint (yup::Graphics& g) override
    {
        const auto* root = view->getDocument().getRoot();
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

        divider.layoutComponents (bounds, &upperPane, &lowerPane);
        view->setBounds (upperPane.getBounds());

        auto lower = lowerPane.getBounds();
        status.setBounds (lower.removeFromRight (yup::jmin (280.0f, lower.getWidth() * 0.4f)));
        lower.removeFromRight (6.0f);
        tabs.setBounds (lower.removeFromTop (32.0f));

        for (auto& source : sources)
            source->editor.setBounds (lower.withTrimmedTop (4.0f));
    }

    void codeDocumentChanged (yup::CodeDocument&, const yup::CodeDocument::Change&) override
    {
        // Typing restarts the countdown, so a burst of keystrokes rebuilds once
        if (! showingFiles)
            startTimer (150);
    }

private:
    // A document, then the shaders it runs: each is a tab, and what is typed in one is what the document loads
    struct Example
    {
        const char* name;
        std::vector<const char*> files;
    };

    static const std::vector<Example>& getExamples()
    {
        static const std::vector<Example> examples {
            { "Computer", { "computer.yui", "fx/plasma.frag" } },
            { "Cameras", { "cameras.yui" } },
            { "Shapes", { "shapes.yui", "fx/plasma.frag" } },
            { "Materials", { "materials.yui", "fx/iridescent.glsl", "fx/hologram.glsl", "fx/lava.glsl" } },
            { "Post effects", { "postfx.yui", "fx/crt.frag", "fx/pixelate.frag" } },
            { "Timeline", { "timeline.yui" } }
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
        stopTimer();
        rebuild();
    }

    // Each example gets a view of its own: nothing is kept of the scene shown before, and a document that can't be
    // loaded shows nothing rather than the example before it
    void showExample (const Example& example)
    {
        stopTimer();

        if (view != nullptr)
            view->getDocument().removeListener (this);

        sources.clear();
        tabs.clearTabs();
        lastAction.clear();

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

        // The documents are complete by themselves: the page gives them no model, and shows the actions they raise
        auto& document = view->getDocument();
        document.addListener (this);
        document.setFallbackAction ([this] (const yup::String& name, const yup::Array<yup::var>& arguments)
        {
            yup::StringArray values (name.quoted());

            for (const auto& argument : arguments)
                values.add (argument.isString() ? argument.toString().quoted() : argument.toString());

            lastAction = "action (" + values.joinIntoString (", ") + ")";
            report();
        });

        // The tabs show the files after every load. On desktop the loader watches them, the shader sources too, so
        // saving one in another editor loads again and updates the interface in place
        auto& loader = view->getLoader();
        loader.onLoad = [this] (const yup::Result& result)
        {
            lastLoad = result;
            showFiles();
            report();
        };
        loader.loadFile (getAssetPath ("data/yui3d").getChildFile (example.files.front()));
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

    // The text of the document as it was last loaded, and of a shader as the document reads it: from its file, or
    // from what was typed in its tab
    std::optional<yup::String> readSource (const Source& source) const
    {
        if (&source == sources.front().get())
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
    // material compiled by the frame after the load
    void documentLoaded (yup::YuiDocument&, const yup::Result&) override {}

    void documentDiagnosed (yup::YuiDocument&, const yup::YuiDiagnostic&) override
    {
        report();
    }

    void report()
    {
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
            lines.add (yup::YuiDiagnostic::join (problems, "\n"));
        else
            lines.add ("Built with " + yup::String (view->getDocument().getBuilder().getBindings().getNumBindings()) + " bindings. Edit any tab"
                       + (view->getLoader().isWatching() ? " or file." : "."));

        status.setText (lines.joinIntoString ("\n"), yup::dontSendNotification);
        repaint();
    }

    std::unique_ptr<yup::YuiView> view;

    std::vector<std::unique_ptr<Source>> sources;
    yup::TabBar selector { "selector" };
    yup::TabBar tabs { "tabs" };
    yup::TextEditor status { "status" };
    yup::ResizableDivider divider { yup::ResizableDivider::Orientation::horizontal, "divider" };
    yup::Component upperPane, lowerPane;
    yup::Result lastLoad = yup::Result::ok();
    yup::String lastAction;
    bool showingFiles = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Yui3DDemo)
};
