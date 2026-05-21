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

#include <memory>
#include <optional>
#include <utility>

//==============================================================================

class AiDemo : public yup::Component
{
public:
    AiDemo()
        : Component ("AiDemo")
    {
        auto theme = yup::ApplicationTheme::getGlobalTheme();
        titleFont = theme->getDefaultFont();

        titleLabel.setText ("Ollama Tools", yup::dontSendNotification);
        titleLabel.setFont (titleFont);
        addAndMakeVisible (titleLabel);

        modelLabel.setText ("Model", yup::dontSendNotification);
        addAndMakeVisible (modelLabel);

        modelEditor.setText ("llama3.2", yup::dontSendNotification);
        modelEditor.setMultiLine (false);
        addAndMakeVisible (modelEditor);

        baseUrlLabel.setText ("Base URL", yup::dontSendNotification);
        addAndMakeVisible (baseUrlLabel);

        baseUrlEditor.setText ("http://localhost:11434/v1", yup::dontSendNotification);
        baseUrlEditor.setMultiLine (false);
        addAndMakeVisible (baseUrlEditor);

        promptLabel.setText ("Prompt", yup::dontSendNotification);
        addAndMakeVisible (promptLabel);

        promptEditor.setText ("Change this component background to dark green, then say what you changed.", yup::dontSendNotification);
        promptEditor.setMultiLine (true);
        addAndMakeVisible (promptEditor);

        askButton.setButtonText ("Ask");
        askButton.onClick = [this]
        {
            askModel();
        };
        addAndMakeVisible (askButton);

        statusLabel.setText ("Ollama can call set_background_color for this page.", yup::dontSendNotification);
        addAndMakeVisible (statusLabel);

        responseLabel.setText ("Response", yup::dontSendNotification);
        addAndMakeVisible (responseLabel);

        responseEditor.setMultiLine (true);
        responseEditor.setReadOnly (true);
        responseEditor.setText ("", yup::dontSendNotification);
        addAndMakeVisible (responseEditor);
    }

    ~AiDemo() override
    {
        if (requestThread != nullptr)
            requestThread->stopThread (-1);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (20);

        titleLabel.setBounds (area.removeFromTop (40));
        area.removeFromTop (10);

        auto settings = area.removeFromTop (58);
        auto columnWidth = (settings.getWidth() - 12) / 2;

        auto modelColumn = settings.removeFromLeft (columnWidth);
        settings.removeFromLeft (12);
        auto baseUrlColumn = settings;

        modelLabel.setBounds (modelColumn.removeFromTop (22));
        modelEditor.setBounds (modelColumn.removeFromTop (30));

        baseUrlLabel.setBounds (baseUrlColumn.removeFromTop (22));
        baseUrlEditor.setBounds (baseUrlColumn.removeFromTop (30));

        area.removeFromTop (14);

        promptLabel.setBounds (area.removeFromTop (22));
        promptEditor.setBounds (area.removeFromTop (120));

        area.removeFromTop (12);

        auto actionRow = area.removeFromTop (34);
        askButton.setBounds (actionRow.removeFromLeft (96));
        actionRow.removeFromLeft (12);
        statusLabel.setBounds (actionRow);

        area.removeFromTop (18);

        responseLabel.setBounds (area.removeFromTop (22));
        responseEditor.setBounds (area);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (backgroundColor.value_or (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray)));
        g.fillAll();

        g.setStrokeColor (yup::Colors::darkgray);
        g.setStrokeWidth (1.0f);
        g.strokeLine (20.0f, 66.0f, getWidth() - 20.0f, 66.0f);
    }

private:
    class OllamaRequestThread final : public yup::Thread
    {
    public:
        OllamaRequestThread (AiDemo& ownerToUse, yup::String modelToUse, yup::String baseUrlToUse, yup::String promptToUse)
            : Thread ("OllamaRequest")
            , owner (ownerToUse)
            , model (std::move (modelToUse))
            , baseUrl (std::move (baseUrlToUse))
            , prompt (std::move (promptToUse))
            , ownerReference (&ownerToUse)
        {
        }

        void run() override
        {
            yup::LLMClient::Options options;
            options.model = model;
            options.baseUrl = baseUrl;
            options.timeoutMs = 120000;
            options.maxRetries = 0;

            yup::LLMHttpClient client (std::move (options));

            yup::LLMClient::Request request;
            request.systemPrompt = "You are a concise assistant inside a YUP example app. "
                                   "If the user asks to change the page background, call set_background_color with a CSS color name, #RRGGBB value, rgb(...), or hsl(...). "
                                   "After a tool result, briefly tell the user what changed.";
            request.messages.push_back (yup::LLMMessage::user (prompt));
            request.temperature = 0.2f;

            yup::LLMToolRegistry toolRegistry;
            owner.registerTools (toolRegistry, ownerReference);
            request.tools = toolRegistry.getAllTools();
            request.toolChoice = "auto";

            auto response = client.runToolLoop (request, toolRegistry);

            yup::String responseText;
            if (! response.choices.empty())
                responseText = response.choices.front().message.content.trim();

            if (responseText.isEmpty())
                responseText = "No response was returned. Check that Ollama is running, the model is pulled, and the base URL is reachable.";

            if (threadShouldExit())
                return;

            auto ownerPtr = std::addressof (owner);
            auto weakOwner = ownerReference;

            yup::MessageManager::callAsync ([ownerPtr, weakOwner, responseText]
            {
                if (weakOwner.get() == nullptr)
                    return;

                ownerPtr->handleResponse (responseText);
            });
        }

    private:
        AiDemo& owner;
        yup::String model;
        yup::String baseUrl;
        yup::String prompt;
        yup::WeakReference<yup::Component> ownerReference;
    };

    void askModel()
    {
        if (requestThread != nullptr && requestThread->isThreadRunning())
        {
            statusLabel.setText ("A request is already running.", yup::dontSendNotification);
            return;
        }

        requestThread.reset();

        const auto model = modelEditor.getText().trim();
        const auto baseUrl = baseUrlEditor.getText().trim();
        const auto prompt = promptEditor.getText().trim();

        if (model.isEmpty() || baseUrl.isEmpty() || prompt.isEmpty())
        {
            statusLabel.setText ("Model, base URL, and prompt are required.", yup::dontSendNotification);
            return;
        }

        askButton.setEnabled (false);
        statusLabel.setText ("Waiting for Ollama...", yup::dontSendNotification);
        responseEditor.setText ("", yup::dontSendNotification);

        requestThread = std::make_unique<OllamaRequestThread> (*this, model, baseUrl, prompt);
        if (! requestThread->startThread (yup::Thread::Priority::background))
        {
            requestThread.reset();
            responseEditor.setText ("", yup::dontSendNotification);
            statusLabel.setText ("Unable to start background request thread.", yup::dontSendNotification);
            askButton.setEnabled (true);
        }
    }

    void handleResponse (const yup::String& responseText)
    {
        responseEditor.setText (responseText, yup::dontSendNotification);
        statusLabel.setText ("Complete", yup::dontSendNotification);
        askButton.setEnabled (true);
    }

    void registerTools (yup::LLMToolRegistry& registry, yup::WeakReference<yup::Component> ownerReference)
    {
        yup::LLMTool colorTool;
        colorTool.name = "set_background_color";
        colorTool.description = "Changes the visible background color of the current YUP example component.";

        yup::LLMTool::Parameter colorParameter;
        colorParameter.name = "color";
        colorParameter.type = "string";
        colorParameter.description = "CSS color name, #RRGGBB, rgb(...), rgba(...), hsl(...), or hsla(...) value.";
        colorParameter.required = true;
        colorTool.parameters.push_back (std::move (colorParameter));

        auto ownerPtr = this;

        colorTool.setHandler ([ownerPtr, ownerReference] (const yup::var& arguments)
        {
            const auto colorText = arguments["color"].toString().trim();
            const auto colorValue = colorText.startsWithChar ('#') || colorText.startsWithIgnoreCase ("rgb") || colorText.startsWithIgnoreCase ("hsl")
                                      ? colorText
                                      : colorText.removeCharacters (" ");
            const auto color = yup::Color::fromString (colorValue);

            yup::MessageManager::callAsync ([ownerPtr, ownerReference, color]
            {
                if (ownerReference.get() == nullptr)
                    return;

                ownerPtr->setBackgroundColor (color);
            });

            auto result = yup::var (std::make_unique<yup::DynamicObject>());
            if (auto* object = result.getDynamicObject())
            {
                object->setProperty ("success", true);
                object->setProperty ("color", colorValue);
                object->setProperty ("message", "Background color updated.");
            }

            return result;
        });

        registry.registerTool (std::move (colorTool));
    }

    void setBackgroundColor (yup::Color color)
    {
        backgroundColor = color;
        repaint();
    }

    yup::Label titleLabel { "titleLabel" };
    yup::Label modelLabel { "modelLabel" };
    yup::Label baseUrlLabel { "baseUrlLabel" };
    yup::Label promptLabel { "promptLabel" };
    yup::Label statusLabel { "statusLabel" };
    yup::Label responseLabel { "responseLabel" };

    yup::TextEditor modelEditor { "modelEditor" };
    yup::TextEditor baseUrlEditor { "baseUrlEditor" };
    yup::TextEditor promptEditor { "promptEditor" };
    yup::TextEditor responseEditor { "responseEditor" };
    yup::TextButton askButton { "askButton" };

    yup::Font titleFont;
    std::optional<yup::Color> backgroundColor;
    std::unique_ptr<OllamaRequestThread> requestThread;
};
