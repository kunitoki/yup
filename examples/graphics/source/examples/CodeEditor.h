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

class CodeEditorDemo : public yup::Component
{
public:
    CodeEditorDemo()
        : Component ("CodeEditorDemo")
        , editor (document)
    {
        languageCombo.addItem ("C++", languageCpp);
        languageCombo.addItem ("GLSL", languageGlsl);
        languageCombo.addItem ("Python", languagePython);
        languageCombo.addItem ("XML", languageXml);
        languageCombo.setSelectedId (languageCpp);
        languageCombo.onSelectedItemChanged = [this]
        {
            applyLanguage (languageCombo.getSelectedId());
        };
        addAndMakeVisible (languageCombo);

        applyLanguage (languageCpp);
        addAndMakeVisible (editor);

        setSize (640, 480);
        setOpaque (false);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (10);
        languageCombo.setBounds (bounds.removeFromTop (24));
        editor.setBounds (bounds.reduced (0, 6));
    }

private:
    enum
    {
        languageCpp = 1,
        languageGlsl,
        languagePython,
        languageXml
    };

    void applyLanguage (int languageId)
    {
        switch (languageId)
        {
            case languageCpp:
                editor.setSyntaxDefinition ("cpp");
                editor.setText (cppSample);
                break;

            case languageGlsl:
                editor.setSyntaxDefinition ("glsl");
                editor.setText (glslSample);
                break;

            case languagePython:
                editor.setSyntaxDefinition ("python");
                editor.setText (pythonSample);
                break;

            case languageXml:
                editor.setSyntaxDefinition ("xml");
                editor.setText (xmlSample);
                break;

            default:
                break;
        }
    }

    static constexpr const char* cppSample = R"(// Syntax-highlighted C++ code
#include <vector>
#include <string>

class Example
{
public:
    int sum (const std::vector<int>& values) const
    {
        int total = 0;
        for (const auto& value : values)
            total += value;  // accumulate everything
        return total;
    }

private:
    std::string name = "yup";
};

/* Block comments span
   multiple lines. */
int main()
{
    Example example;
    std::vector<int> values = { 1, 2, 3, 4, 5 };
    return example.sum (values);
}
)";

    static constexpr const char* glslSample = R"(// Syntax-highlighted GLSL shader
#version 330 core

uniform mat4 viewProjection;
uniform vec3 lightDirection;

in vec3 position;
in vec2 texCoord;
out vec2 vTexCoord;

void main()
{
    gl_Position = viewProjection * vec4 (position, 1.0);
    vTexCoord = texCoord;
}
)";

    static constexpr const char* pythonSample = R"(# A tiny Python example
import math

def circle_area (radius):
    """Compute the area of a circle."""
    return math.pi * radius ** 2

class Shape:
    def __init__ (self, name):
        self.name = name

for i in range (3):
    print (f'area[{i}] = {circle_area (i + 1):.2f}')
)";

    static constexpr const char* xmlSample = R"(<?xml version="1.0" encoding="UTF-8"?>
<!-- A small SVG document -->
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg xmlns="http://www.w3.org/2000/svg" width="640" height="480">
    <rect x="10" y="10" width="100" height="100" fill="#ff0000"/>
    <circle cx="200" cy="200" r="50" fill="none" stroke="#00ff00" stroke-width="4"/>
    <text x="50" y="50" font-size="24">Hello, XML!</text>
</svg>
)";

    yup::ComboBox languageCombo { "LanguageSelector" };
    yup::CodeDocument document;
    yup::CodeEditor editor;
};
