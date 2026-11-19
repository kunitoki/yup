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

#include "../source/yup_YuiCommands.h"
#include "../source/yup_YuiToolSupport.h"

#include <yup_gui/yup_gui.h>

#include <SDL3/SDL_hints.h>

#include <emscripten/emscripten.h>
#include <emscripten/eventloop.h>

#include <iostream>
#include <memory>
#include <vector>

using namespace yup;

namespace
{

//==============================================================================
// The files the host gives, at the paths it names them with
File getFilesFolder()
{
    return File ("/yui");
}

// The size an interface sets for its root with plain numbers, which is the one it asks to be shown at
Size<float> findDeclaredSize (const DataTree& tree)
{
    const auto rootIndex = findYuiToolRootIndex (tree);

    if (rootIndex < 0)
        return {};

    const auto read = [root = tree.getChild (rootIndex)] (const Identifier& name)
    {
        const auto text = root.getProperty (name).toString();
        return text.containsOnly ("0123456789.") ? text.getFloatValue() : 0.0f;
    };

    return { read ("width"), read ("height") };
}

//==============================================================================
/** The window the page shows in its canvas: an interface over the background of its palette. */
class PreviewWindow final : public DocumentWindow
{
public:
    PreviewWindow() = default;

    /** Shows a target of a file and tells the page what the load gave.

        The document is reloaded in place, keeping the state of its components, for as long as the
        file, the target, the models and the theme stay the same.
    */
    void show (const File& input, const YuiToolTarget& target, const File& modelsFile, bool lightTheme)
    {
        const auto modelsText = modelsFile.existsAsFile() ? modelsFile.loadFileAsString() : String();
        const auto key = StringArray { input.getFullPathName(), target.drawing, target.palette, target.hostType, modelsText, lightTheme ? "light" : "dark" }
                             .joinIntoString ("\n");

        if (session == nullptr || key != sessionKey)
        {
            session.reset();
            sessionKey = key;

            ApplicationTheme::getGlobalTheme()->setPalette (ThemePalette ({}, lightTheme ? ThemePalette::Mode::light : ThemePalette::Mode::dark));

            session = std::make_unique<Session>();

            if (modelsFile.existsAsFile() && session->models.load (modelsFile.getFullPathName()))
                session->models.attachTo (session->view.getDocument());

            session->models.onValueChanged = [] (const String& model, const String& property, const var& value)
            {
                // clang-format off
                EM_ASM ({
                    if (Module.yuiPreviewModelChanged)
                        Module.yuiPreviewModelChanged (UTF8ToString ($0), UTF8ToString ($1), JSON.parse (UTF8ToString ($2)));
                }, model.toRawUTF8(), property.toRawUTF8(), JSON::toString (value, true).toRawUTF8());
                // clang-format on
            };

            addAndMakeVisible (session->view);
            resized();
        }

        auto& document = session->view.getDocument();
        const auto shown = loadYuiToolTarget (document, session->view.getLoader(), input, target);

        Array<var> problems;

        for (const auto& problem : findYuiToolProblems (document, input, shown.result))
        {
            if (! problem.isError)
                continue;

            // The host knows the files by the paths it gave them
            auto record = problem.toJson();
            record.getDynamicObject()->setProperty ("file", problem.file.getRelativePathFrom (getFilesFolder()));
            problems.add (record);
        }

        const auto declaredSize = shown.status == YuiToolShown::Status::loaded ? findDeclaredSize (document.getTree()) : Size<float>();

        auto* report = new DynamicObject();
        report->setProperty ("status", shown.status != YuiToolShown::Status::loaded ? "nothing" : (shown.result.failed() ? "failed" : "shown"));
        report->setProperty ("message", shown.status == YuiToolShown::Status::invalidName ? String ("the name of what to show is not valid") : shown.message);
        report->setProperty ("problems", var (problems));
        report->setProperty ("hasInterface", document.getRoot() != nullptr);
        report->setProperty ("width", declaredSize.getWidth());
        report->setProperty ("height", declaredSize.getHeight());
        report->setProperty ("defaultWidth", shown.defaultSize.getWidth());
        report->setProperty ("defaultHeight", shown.defaultSize.getHeight());
        report->setProperty ("models", session->models.toJson());

        repaint();

        // clang-format off
        EM_ASM ({
            if (Module.yuiPreviewShown)
                Module.yuiPreviewShown (JSON.parse (UTF8ToString ($0)));
        }, JSON::toString (var (report)).toRawUTF8());
        // clang-format on
    }

    void paint (Graphics& g) override
    {
        const auto* root = session != nullptr ? session->view.getDocument().getRoot() : nullptr;
        const auto& theme = *ApplicationTheme::getGlobalTheme();

        g.setFillColor ((root != nullptr ? theme.getPalette (*root) : theme.getPalette (*this)).getColor (ThemePalette::Role::background));
        g.fillAll();
    }

    void resized() override
    {
        if (session != nullptr)
            session->view.setBounds (getLocalBounds());
    }

    // The page owns the window
    void userTriedToCloseWindow() override {}

    /** Changes a value of the models of the interface shown. */
    bool setModelValue (const String& model, const String& property, const var& value)
    {
        return session != nullptr && session->models.setValue (model, property, value);
    }

    /** Returns the current values of the models of the interface shown. */
    var getModels() const
    {
        return session != nullptr ? session->models.toJson() : var (new DynamicObject());
    }

private:
    // The models outlive the document they are set on
    struct Session
    {
        YuiToolModels models;
        YuiView view;
    };

    std::unique_ptr<Session> session;
    String sessionKey;
};

PreviewWindow* previewWindow = nullptr;
MemoryBlock pendingFile;

//==============================================================================
class PreviewApplication final : public YUPApplication
{
public:
    PreviewApplication() = default;

    String getApplicationName() override { return "yup_yui_tool"; }

    String getApplicationVersion() override { return "1.0"; }

    void initialise (const String&) override
    {
        MessageManager::callAsync ([this]
        {
            // The page has controls of its own, so the keys only go to the interface while its canvas has the focus
            SDL_SetHint (SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");

            window = std::make_unique<PreviewWindow>();
            window->centreWithSize ({ 640, 400 });
            window->setVisible (true);

            previewWindow = window.get();

            // clang-format off
            EM_ASM ({
                if (Module.yuiPreviewReady)
                    Module.yuiPreviewReady();
            });
            // clang-format on
        });
    }

    void shutdown() override
    {
        previewWindow = nullptr;
        window.reset();
    }

private:
    std::unique_ptr<PreviewWindow> window;
};

} // namespace

//==============================================================================
// Called by the host of the module, which gives it the files to work on.
extern "C"
{
    // Returns the memory the host copies the bytes of a file to, before naming it with yuiToolWriteFile
    void* EMSCRIPTEN_KEEPALIVE yuiToolAllocate (int size)
    {
        pendingFile.setSize (static_cast<std::size_t> (jmax (0, size)));
        return pendingFile.getData();
    }

    void EMSCRIPTEN_KEEPALIVE yuiToolWriteFile (const char* path)
    {
        const auto file = getFilesFolder().getChildFile (String::fromUTF8 (path));

        file.getParentDirectory().createDirectory();

        // An empty file is still a file, while replacing one with nothing deletes it
        if (pendingFile.isEmpty())
            file.create();
        else
            file.replaceWithData (pendingFile.getData(), pendingFile.getSize());

        pendingFile.reset();
    }

    // Runs a command of the tool in the folder of the files. The arguments are a JSON array of strings. A host
    // that only runs commands loads the module with noInitialRun, which leaves the application unstarted
    int EMSCRIPTEN_KEEPALIVE yuiToolRun (const char* arguments)
    {
        var parsed;
        JSON::parse (String::fromUTF8 (arguments), parsed);

        StringArray words ("yup_yui_tool");

        if (const auto* items = parsed.getArray())
            for (const auto& item : *items)
                words.add (item.toString());

        std::vector<char*> argv;

        for (auto& word : words)
            argv.push_back (const_cast<char*> (word.toRawUTF8()));

        const auto argc = static_cast<int> (argv.size());

        if (const auto exitCode = checkYuiToolCommand (argc, argv.data()))
            return *exitCode;

        // Set up once as an application is, default theme included, with no page to open a window in
        static const bool hasDriver = SDL_SetHint (SDL_HINT_VIDEO_DRIVER, "dummy");
        static const ScopedYupInitialiser_GUI gui;
        static const ScopedYupInitialiser_Windowing windowing;

        if (! hasDriver || ApplicationTheme::getGlobalTheme() == nullptr)
        {
            std::cerr << "yup_yui_tool: can't initialise the windowing system\n";
            return 2;
        }

        // A command runs to its end, so the loop the message manager started has nothing to do but keep the
        // host awake: it is stopped, and the runtime that would end with it is kept for the next commands.
        // The loop of a running application is left alone
        static const bool isLoopStopped = []
        {
            if (YUPApplicationBase::getInstance() != nullptr)
                return false;

            emscripten_runtime_keepalive_push();
            emscripten_cancel_main_loop();
            return true;
        }();

        ignoreUnused (isLoopStopped);

        getFilesFolder().createDirectory();
        getFilesFolder().setAsCurrentWorkingDirectory();

        const auto exitCode = runYuiToolCommand (argc, argv.data());

        std::cout.flush();
        std::cerr.flush();

        return exitCode;
    }

    // Shows a file in the window of the application, once Module.yuiPreviewReady was called. The names are
    // empty for what is not asked, and Module.yuiPreviewShown is called with the outcome
    void EMSCRIPTEN_KEEPALIVE yuiPreviewShow (const char* path, const char* drawing, const char* palette, const char* hostType, const char* modelsPath, int lightTheme)
    {
        if (previewWindow == nullptr)
            return;

        const auto models = String::fromUTF8 (modelsPath);

        previewWindow->show (getFilesFolder().getChildFile (String::fromUTF8 (path)),
                             { String::fromUTF8 (drawing), String::fromUTF8 (palette), String::fromUTF8 (hostType) },
                             models.isNotEmpty() ? getFilesFolder().getChildFile (models) : File(),
                             lightTheme != 0);
    }

    // Changes a value of a model of the interface shown, given as JSON. Values the interface changes itself are
    // reported to Module.yuiPreviewModelChanged (model, property, value)
    int EMSCRIPTEN_KEEPALIVE yuiPreviewSetModelValue (const char* model, const char* property, const char* valueJson)
    {
        if (previewWindow == nullptr)
            return 0;

        const auto value = JSON::fromString (String::fromUTF8 (valueJson));
        return previewWindow->setModelValue (String::fromUTF8 (model), String::fromUTF8 (property), value) ? 1 : 0;
    }

    // Returns the current values of the models as JSON, in the shape of the models file
    const char* EMSCRIPTEN_KEEPALIVE yuiPreviewModels()
    {
        static std::string text;
        text = previewWindow != nullptr ? JSON::toString (previewWindow->getModels(), true).toStdString() : std::string ("{}");
        return text.c_str();
    }
}

START_YUP_APPLICATION (PreviewApplication)
