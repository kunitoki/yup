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

        //======================================================================
        // Title
        titleLabel.setText ("AI Providers", yup::dontSendNotification);
        titleLabel.setFont (titleFont);
        addAndMakeVisible (titleLabel);

        //======================================================================
        // Provider selector buttons
        providerOpenAIChatButton.setButtonText ("OpenAI Chat");
        providerOpenAIChatButton.onClick = [this]
        {
            selectProvider (SelectedProvider::OpenAIChat);
        };
        addAndMakeVisible (providerOpenAIChatButton);

        providerOpenAIResponsesButton.setButtonText ("OpenAI Responses");
        providerOpenAIResponsesButton.onClick = [this]
        {
            selectProvider (SelectedProvider::OpenAIResponses);
        };
        addAndMakeVisible (providerOpenAIResponsesButton);

        providerAnthropicButton.setButtonText ("Anthropic");
        providerAnthropicButton.onClick = [this]
        {
            selectProvider (SelectedProvider::Anthropic);
        };
        addAndMakeVisible (providerAnthropicButton);

        providerGeminiButton.setButtonText ("Gemini");
        providerGeminiButton.onClick = [this]
        {
            selectProvider (SelectedProvider::Gemini);
        };
        addAndMakeVisible (providerGeminiButton);

        //======================================================================
        // Model
        modelLabel.setText ("Model", yup::dontSendNotification);
        addAndMakeVisible (modelLabel);

        modelEditor.setMultiLine (false);
        addAndMakeVisible (modelEditor);

        //======================================================================
        // Base URL
        baseUrlLabel.setText ("Base URL", yup::dontSendNotification);
        addAndMakeVisible (baseUrlLabel);

        baseUrlEditor.setMultiLine (false);
        addAndMakeVisible (baseUrlEditor);

        //======================================================================
        // API Key
        apiKeyLabel.setText ("API Key", yup::dontSendNotification);
        addAndMakeVisible (apiKeyLabel);

        apiKeyEditor.setMultiLine (false);
        apiKeyEditor.setText ("", yup::dontSendNotification);
        addAndMakeVisible (apiKeyEditor);

        //======================================================================
        // Reasoning effort (for OpenAI Responses / Gemini 2.5 — leave empty to disable)
        reasoningLabel.setText ("Reasoning (low/med/high)", yup::dontSendNotification);
        addAndMakeVisible (reasoningLabel);

        reasoningEditor.setMultiLine (false);
        reasoningEditor.setText ("", yup::dontSendNotification);
        addAndMakeVisible (reasoningEditor);

        //======================================================================
        // Prompt
        promptLabel.setText ("Prompt", yup::dontSendNotification);
        addAndMakeVisible (promptLabel);

        promptEditor.setMultiLine (true);
        promptEditor.setText ("Change this component background to dark green, then say what you changed.",
                              yup::dontSendNotification);
        addAndMakeVisible (promptEditor);

        //======================================================================
        // Action row
        askButton.setButtonText ("Ask");
        askButton.onClick = [this]
        {
            askModel();
        };
        addAndMakeVisible (askButton);

        // Tools are supported by OpenAI Chat and Gemini providers.
        toolsToggle.setButtonText ("Tools");
        toolsToggle.setToggleState (true, yup::dontSendNotification);
        addAndMakeVisible (toolsToggle);

        statusLabel.setText ("Select a provider and ask a question.", yup::dontSendNotification);
        addAndMakeVisible (statusLabel);

        //======================================================================
        // Response
        responseLabel.setText ("Response", yup::dontSendNotification);
        addAndMakeVisible (responseLabel);

        responseEditor.setMultiLine (true);
        responseEditor.setReadOnly (true);
        responseEditor.setText ("", yup::dontSendNotification);
        addAndMakeVisible (responseEditor);

        // Apply defaults for the initial provider.
        selectProvider (SelectedProvider::OpenAIChat);
    }

    ~AiDemo() override
    {
        if (requestThread != nullptr)
            requestThread->stopThread (-1);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (20);

        // Title
        titleLabel.setBounds (area.removeFromTop (40));
        area.removeFromTop (8);

        // Provider selector — four equal-width buttons.
        {
            auto row = area.removeFromTop (30);
            const int w = row.getWidth() / 4;
            providerOpenAIChatButton.setBounds (row.removeFromLeft (w));
            providerOpenAIResponsesButton.setBounds (row.removeFromLeft (w));
            providerAnthropicButton.setBounds (row.removeFromLeft (w));
            providerGeminiButton.setBounds (row);
        }
        area.removeFromTop (10);

        constexpr int columnGap = 12;
        constexpr int labelH = 20;
        constexpr int editorH = 28;
        constexpr int rowH = labelH + 4 + editorH;

        // Row 1: Model (left) | Base URL (right)
        {
            auto row = area.removeFromTop (rowH);
            auto left = row.removeFromLeft ((row.getWidth() - columnGap) / 2);
            row.removeFromLeft (columnGap);

            modelLabel.setBounds (left.removeFromTop (labelH));
            left.removeFromTop (4);
            modelEditor.setBounds (left);

            baseUrlLabel.setBounds (row.removeFromTop (labelH));
            row.removeFromTop (4);
            baseUrlEditor.setBounds (row);
        }
        area.removeFromTop (8);

        // Row 2: API Key (left) | Reasoning effort (right)
        {
            auto row = area.removeFromTop (rowH);
            auto left = row.removeFromLeft ((row.getWidth() - columnGap) / 2);
            row.removeFromLeft (columnGap);

            apiKeyLabel.setBounds (left.removeFromTop (labelH));
            left.removeFromTop (4);
            apiKeyEditor.setBounds (left);

            reasoningLabel.setBounds (row.removeFromTop (labelH));
            row.removeFromTop (4);
            reasoningEditor.setBounds (row);
        }
        area.removeFromTop (14);

        // Prompt
        promptLabel.setBounds (area.removeFromTop (labelH));
        area.removeFromTop (4);
        promptEditor.setBounds (area.removeFromTop (90));
        area.removeFromTop (12);

        // Action row
        {
            auto row = area.removeFromTop (30);
            askButton.setBounds (row.removeFromLeft (80));
            row.removeFromLeft (10);
            toolsToggle.setBounds (row.removeFromLeft (80));
            row.removeFromLeft (10);
            statusLabel.setBounds (row);
        }
        area.removeFromTop (14);

        // Response
        responseLabel.setBounds (area.removeFromTop (labelH));
        area.removeFromTop (4);
        responseEditor.setBounds (area);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (backgroundColor.value_or (
            findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray)));
        g.fillAll();

        g.setStrokeColor (yup::Colors::darkgray);
        g.setStrokeWidth (1.0f);
        g.strokeLine (20.0f, 56.0f, getWidth() - 20.0f, 56.0f); // below title
        g.strokeLine (20.0f, 96.0f, getWidth() - 20.0f, 96.0f); // below provider row
    }

private:
    //==========================================================================
    enum class SelectedProvider
    {
        OpenAIChat,
        OpenAIResponses,
        Anthropic,
        Gemini
    };
    SelectedProvider currentProvider = SelectedProvider::OpenAIChat;

    //==========================================================================
    // Provider selection — updates button labels, defaults, and enabled states.
    void selectProvider (SelectedProvider p)
    {
        currentProvider = p;

        // Use a bullet marker on the active button text.
        providerOpenAIChatButton.setButtonText (p == SelectedProvider::OpenAIChat ? "• OpenAI Chat" : "OpenAI Chat");
        providerOpenAIResponsesButton.setButtonText (p == SelectedProvider::OpenAIResponses ? "• OpenAI Responses" : "OpenAI Responses");
        providerAnthropicButton.setButtonText (p == SelectedProvider::Anthropic ? "• Anthropic" : "Anthropic");
        providerGeminiButton.setButtonText (p == SelectedProvider::Gemini ? "• Gemini" : "Gemini");

        // Apply per-provider defaults (model + base URL).
        switch (p)
        {
            case SelectedProvider::OpenAIChat:
                modelEditor.setText ("gemma4", yup::dontSendNotification);
                baseUrlEditor.setText ("http://localhost:11434/v1", yup::dontSendNotification);
                statusLabel.setText ("OpenAI Chat / Ollama - supports tools, streaming, and structured output.", yup::dontSendNotification);
                break;

            case SelectedProvider::OpenAIResponses:
                modelEditor.setText ("gpt-4.1", yup::dontSendNotification);
                baseUrlEditor.setText ("https://api.openai.com/v1", yup::dontSendNotification);
                statusLabel.setText ("OpenAI Responses API - supports reasoning effort and structured output.", yup::dontSendNotification);
                break;

            case SelectedProvider::Anthropic:
                modelEditor.setText ("claude-opus-4-5", yup::dontSendNotification);
                baseUrlEditor.setText ("https://api.anthropic.com/v1", yup::dontSendNotification);
                statusLabel.setText ("Anthropic Claude - requires an API key. Prompt cached automatically.", yup::dontSendNotification);
                break;

            case SelectedProvider::Gemini:
                modelEditor.setText ("gemini-2.5-flash", yup::dontSendNotification);
                baseUrlEditor.setText ("https://generativelanguage.googleapis.com", yup::dontSendNotification);
                statusLabel.setText ("Google Gemini - supports tools and thinking budget via Reasoning field.", yup::dontSendNotification);
                break;
        }

        // Tools are supported by OpenAI Chat and Gemini.
        const bool supportsTools = (p == SelectedProvider::OpenAIChat || p == SelectedProvider::Gemini);
        toolsToggle.setEnabled (supportsTools);
        if (! supportsTools)
            toolsToggle.setToggleState (false, yup::dontSendNotification);

        // Reasoning is meaningful for OpenAI Responses and Gemini.
        reasoningEditor.setEnabled (p == SelectedProvider::OpenAIResponses || p == SelectedProvider::Gemini);
        if (p == SelectedProvider::OpenAIChat || p == SelectedProvider::Anthropic)
            reasoningEditor.setText ("", yup::dontSendNotification);
    }

    //==========================================================================
    class AiRequestThread final : public yup::Thread
    {
    public:
        AiRequestThread (AiDemo& ownerToUse,
                         yup::LLMClient::Options optionsToUse,
                         yup::String promptToUse,
                         bool useToolsToUse)
            : Thread ("AiRequest")
            , owner (ownerToUse)
            , clientOptions (std::move (optionsToUse))
            , prompt (std::move (promptToUse))
            , useTools (useToolsToUse)
            , ownerReference (&ownerToUse)
        {
        }

        void run() override
        {
            auto client = yup::LLMClientFactory::create (clientOptions);
            if (client == nullptr)
            {
                reportResult ("Error: unknown provider.");
                return;
            }

            yup::LLMClient::Request request;
            request.messages.push_back (yup::LLMMessage::user (prompt));
            request.temperature = 0.2f;

            yup::LLMToolRegistry toolRegistry;
            if (useTools)
            {
                request.systemPrompt =
                    "You are a concise assistant inside a YUP example app. "
                    "If the user asks to change the page background, call set_background_color "
                    "with a CSS color name, #RRGGBB value, rgb(...), or hsl(...). "
                    "After a tool result, briefly tell the user what changed.";

                owner.registerTools (toolRegistry, ownerReference);
                request.tools = toolRegistry.getAllTools();
                request.toolChoice = "auto";
            }

            auto response = client->runToolLoop (request, toolRegistry);

            yup::String responseText;
            if (response.failed() && response.errorMessage.has_value())
                responseText = "Error: " + *response.errorMessage;
            else if (! response.choices.empty())
                responseText = response.choices.front().message.content.trim();

            if (responseText.isEmpty())
                responseText = "No response returned. Check your connection, model name, and API key.";

            reportResult (responseText);
        }

    private:
        void reportResult (const yup::String& result)
        {
            if (threadShouldExit())
                return;

            auto ownerPtr = std::addressof (owner);
            auto weakOwner = ownerReference;

            yup::MessageManager::callAsync ([ownerPtr, weakOwner, result]
            {
                if (weakOwner.get() == nullptr)
                    return;

                ownerPtr->handleResponse (result);
            });
        }

        AiDemo& owner;
        yup::LLMClient::Options clientOptions;
        yup::String prompt;
        bool useTools;
        yup::WeakReference<yup::Component> ownerReference;
    };

    //==========================================================================
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
        const auto apiKey = apiKeyEditor.getText().trim();
        const auto reasoning = reasoningEditor.getText().trim();
        const auto prompt = promptEditor.getText().trim();
        const auto useTools = toolsToggle.getToggleState() && toolsToggle.isEnabled();

        if (model.isEmpty() || baseUrl.isEmpty() || prompt.isEmpty())
        {
            statusLabel.setText ("Model, base URL, and prompt are required.", yup::dontSendNotification);
            return;
        }

        yup::LLMClient::Options options;
        options.model = model;
        options.baseUrl = baseUrl;
        options.apiKey = apiKey;
        options.timeoutMs = 120000;
        options.maxRetries = 0;
        options.reasoningEffort = reasoning;

        switch (currentProvider)
        {
            case SelectedProvider::OpenAIChat:
                options.provider = yup::LLMClient::Provider::OpenAIChat;
                break;

            case SelectedProvider::OpenAIResponses:
                options.provider = yup::LLMClient::Provider::OpenAIResponses;
                options.noTemperature = true; // Responses API does not accept temperature
                break;

            case SelectedProvider::Anthropic:
                options.provider = yup::LLMClient::Provider::Anthropic;
                break;

            case SelectedProvider::Gemini:
                options.provider = yup::LLMClient::Provider::Gemini;
                break;
        }

        askButton.setEnabled (false);
        statusLabel.setText ("Waiting for response...", yup::dontSendNotification);
        responseEditor.setText ("", yup::dontSendNotification);

        requestThread = std::make_unique<AiRequestThread> (*this, std::move (options), prompt, useTools);

        if (! requestThread->startThread (yup::Thread::Priority::background))
        {
            requestThread.reset();
            statusLabel.setText ("Unable to start request thread.", yup::dontSendNotification);
            askButton.setEnabled (true);
        }
    }

    void handleResponse (const yup::String& responseText)
    {
        responseEditor.setText (responseText, yup::dontSendNotification);
        statusLabel.setText ("Complete.", yup::dontSendNotification);
        askButton.setEnabled (true);

        // Re-enable tools toggle for providers that support tools.
        toolsToggle.setEnabled (currentProvider == SelectedProvider::OpenAIChat
                                || currentProvider == SelectedProvider::Gemini);
    }

    void registerTools (yup::LLMToolRegistry& registry,
                        yup::WeakReference<yup::Component> ownerReference)
    {
        yup::LLMTool colorTool;
        colorTool.name = "set_background_color";
        colorTool.description = "Changes the visible background color of the current YUP example component.";

        yup::LLMTool::Parameter colorParam;
        colorParam.name = "color";
        colorParam.type = "string";
        colorParam.description = "CSS color name, #RRGGBB, rgb(...), rgba(...), hsl(...), or hsla(...) value.";
        colorParam.required = true;
        colorTool.parameters.push_back (std::move (colorParam));

        auto* ownerPtr = this;

        colorTool.setHandler ([ownerPtr, ownerReference] (const yup::var& arguments)
        {
            const auto colorText = arguments["color"].toString().trim();
            const auto colorValue = colorText.startsWithChar ('#')
                                         || colorText.startsWithIgnoreCase ("rgb")
                                         || colorText.startsWithIgnoreCase ("hsl")
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
            if (auto* obj = result.getDynamicObject())
            {
                obj->setProperty ("success", true);
                obj->setProperty ("color", colorValue);
                obj->setProperty ("message", yup::String ("Background color updated."));
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

    //==========================================================================
    // Title
    yup::Label titleLabel { "titleLabel" };
    yup::Font titleFont;

    // Provider selector
    yup::TextButton providerOpenAIChatButton { "providerOpenAIChatButton" };
    yup::TextButton providerOpenAIResponsesButton { "providerOpenAIResponsesButton" };
    yup::TextButton providerAnthropicButton { "providerAnthropicButton" };
    yup::TextButton providerGeminiButton { "providerGeminiButton" };

    // Settings fields
    yup::Label modelLabel { "modelLabel" };
    yup::TextEditor modelEditor { "modelEditor" };

    yup::Label baseUrlLabel { "baseUrlLabel" };
    yup::TextEditor baseUrlEditor { "baseUrlEditor" };

    yup::Label apiKeyLabel { "apiKeyLabel" };
    yup::TextEditor apiKeyEditor { "apiKeyEditor" };

    yup::Label reasoningLabel { "reasoningLabel" };
    yup::TextEditor reasoningEditor { "reasoningEditor" };

    // Prompt
    yup::Label promptLabel { "promptLabel" };
    yup::TextEditor promptEditor { "promptEditor" };

    // Action row
    yup::TextButton askButton { "askButton" };
    yup::ToggleButton toolsToggle { "toolsToggle" };
    yup::Label statusLabel { "statusLabel" };

    // Response
    yup::Label responseLabel { "responseLabel" };
    yup::TextEditor responseEditor { "responseEditor" };

    // State
    std::optional<yup::Color> backgroundColor;
    std::unique_ptr<AiRequestThread> requestThread;
};
