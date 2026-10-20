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
/**
    The picture behind the "start" line: a minimal YUP app in a live code editor.

    The code is the same snippet the get-started page shows, embedded from
    website/src/partials/snippets/app.cpp, in a CodeEditor themed to the site.
*/
class Start : public Demo
{
public:
    //==============================================================================
    Start()
        : Demo ("Start")
    {
        yup::CodeEditorScheme scheme;
        scheme.setColor (yup::CodeEditorScheme::ColorId::background, fieldColor);
        scheme.setColor (yup::CodeEditorScheme::ColorId::gutterBackground, fieldColor);
        scheme.setColor (yup::CodeEditorScheme::ColorId::gutterText, blueColor);
        scheme.setColor (yup::CodeEditorScheme::ColorId::text, paleColor);
        scheme.setColor (yup::CodeEditorScheme::ColorId::caret, paleColor);
        scheme.setColor (yup::CodeEditorScheme::ColorId::currentLine, blueColor.withAlpha (0.12f));
        scheme.setColor (yup::CodeEditorScheme::ColorId::selection, blueColor.withAlpha (0.35f));
        scheme.setColor (yup::SyntaxDefinition::TokenType::keyword, blueColor.brighter (0.4f));
        scheme.setColor (yup::SyntaxDefinition::TokenType::type, paleColor);
        scheme.setColor (yup::SyntaxDefinition::TokenType::string, yup::Color (0xffdbe6ff));
        scheme.setColor (yup::SyntaxDefinition::TokenType::comment, yup::Color (0xff8095c4));
        scheme.setColor (yup::SyntaxDefinition::TokenType::preprocessor, blueColor.brighter (0.2f));

        editor.setScheme (scheme);
        editor.setSyntaxDefinition ("cpp");
        editor.setReadOnly (true);
        editor.setMinimapVisible (false);
        editor.setText (yup::String::fromUTF8 (
            reinterpret_cast<const char*> (website::appSnippet_data),
            static_cast<int> (website::appSnippet_size)));
        addAndMakeVisible (editor);
    }

    void resized() override
    {
        editor.setFont (yup::ApplicationTheme::getGlobalTheme()->getDefaultMonospaceFont().withHeight (14.0f * contentScale()));
        editor.setBounds (centeredContent (620.0f, 560.0f).intersection (getLocalBounds().reduced (20.0f)));
    }

    yup::Component* getFocusTarget() override { return &editor; }

private:
    //==============================================================================
    yup::CodeDocument document;
    yup::CodeEditor editor { document };
};
