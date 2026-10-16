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

#if YUP_ENABLE_SHADER_TRANSPILER

//==============================================================================
/** A Shadertoy-style fragment shader shown in the Shaders demo list, read from data/shaders/live. */
struct ShaderPreset
{
    const char* name;
    const char* fragment;
};

inline constexpr ShaderPreset shaderPresets[] = {
    { "Default", "data/shaders/live/default.shader" },
    { "Creation", "data/shaders/live/creation.shader" },
    { "Plasma", "data/shaders/live/plasma.shader" },
    { "Raymarched Spheres", "data/shaders/live/spheres.shader" },
    { "Mouse Ripples", "data/shaders/live/ripples.shader" },
    { "Noise Warp", "data/shaders/live/noisewarp.shader" },
    { "Art", "data/shaders/live/art.shader" },
    { "Curl Bands", "data/shaders/live/curlbands.shader" },
};

//==============================================================================
/**
    A Shadertoy-like live shader editor.

    Shaders are written in Shadertoy style, as a `mainImage (out vec4, in vec2)`
    fragment function using the usual inputs (iResolution, iTime, iTimeDelta,
    iFrameRate, iFrame, iMouse, iDate, iChannelTime, iChannelResolution and
    iChannel0..3). Both stages are wrapped into Vulkan-style GLSL 450 and
    compiled at runtime with GpuPipeline::compileFromGlsl(); iChannel0 is an
    RGBA noise texture, iChannel1..3 are black.

    The preset list on the left (hidden until Show Presets is pressed) loads
    the presets, the transport row rewinds, pauses and shows time, frame rate
    and size in pixels, and the editor below (which can be hidden) edits the
    vertex and fragment stages and reports compile errors. Load/Save read and
    write .ysl shader bundles.
*/
class ShadersDemo : public yup::Component
{
public:
    //==============================================================================
    ShadersDemo()
        : yup::Component ("ShadersDemo")
        , codeEditor (fragmentDocument)
    {
        presetModel.onSelectionChanged = [this] (int row)
        {
            loadPreset (row);
        };
        presetList.setModel (&presetModel);
        presetList.setRowSize (30);
        addChildComponent (presetList);

        rewindButton.onClick = [this]
        {
            rewind();
        };
        playButton.onClick = [this]
        {
            setPlaying (! playing);
        };
        loadButton.onClick = [this]
        {
            loadBundle();
        };
        saveButton.onClick = [this]
        {
            saveBundle();
        };
        editorButton.onClick = [this]
        {
            setEditorVisible (! editorVisible);
        };
        presetsButton.onClick = [this]
        {
            setPresetsVisible (! presetsVisible);
        };
        compileButton.onClick = [this]
        {
            requestCompile();
        };

        for (auto* button : { &presetsButton, &rewindButton, &playButton, &loadButton, &saveButton, &editorButton })
            addAndMakeVisible (button);

        for (auto* label : { &timeLabel, &fpsLabel, &sizeLabel })
            addAndMakeVisible (label);

        stageTabs.addTab (fragmentTabId, "Fragment");
        stageTabs.addTab (vertexTabId, "Vertex");
        stageTabs.onSelectionChanged = [this] (const yup::Identifier& tabId)
        {
            showStage (tabId == vertexTabId);
        };

        codeEditor.setSyntaxDefinition ("glsl");
        codeEditor.setScheme (yup::CodeEditorScheme::getBuiltIn ("oneDark"));
        codeEditor.setMinimapVisible (false);

        for (auto* component : std::initializer_list<yup::Component*> { &stageTabs, &codeEditor, &statusLabel, &compileButton })
            addAndMakeVisible (component);

        presetList.selectRow (0, false, yup::dontSendNotification);
        loadPreset (0);
    }

    //==============================================================================
    void refreshDisplay (double lastFrameTimeSeconds) override
    {
        frameDelta = (float) lastFrameTimeSeconds;

        if (playing)
        {
            playbackTime += lastFrameTimeSeconds;
            ++playbackFrame;
        }

        if (auto* native = getNativeComponent())
            frameRate = native->getCurrentFrameRate();

        timeLabel.setText (yup::String (playbackTime, 2) + "s", yup::dontSendNotification);
        fpsLabel.setText (yup::String (frameRate, 1) + " fps", yup::dontSendNotification);
        sizeLabel.setText (yup::String (pixelWidth) + " x " + yup::String (pixelHeight) + " px", yup::dontSendNotification);

        repaint (viewportArea);
    }

    //==============================================================================
    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (yup::ThemePalette::Role::background));
        g.fillAll();

        auto& context = g.getGraphicsContext();
        if (! context.isGpuAvailable())
        {
            if (! compilePending)
                return;

            compilePending = false;
            postCompileResult ("GPU unavailable", 0.0);
            return;
        }

        contextScale = g.getContextScale();
        pixelWidth = yup::roundToInt (viewportArea.getWidth() * contextScale);
        pixelHeight = yup::roundToInt (viewportArea.getHeight() * contextScale);

        if (pixelWidth < 2 || pixelHeight < 2)
            return;

        auto device = context.getGpuDevice();

        if (channelSampler == nullptr)
            createChannels (device);

        if (compilePending)
            compile (device);

        if (pipeline == nullptr)
            return;

        if (target == nullptr || target->getWidth() != pixelWidth || target->getHeight() != pixelHeight)
            target = yup::GpuTarget::create (device, pixelWidth, pixelHeight);

        if (target == nullptr)
            return;

        const auto inputs = makeInputs();

        auto frame = yup::GpuFrame::begin (device);
        auto pass = target->beginRenderPass (frame, { true, yup::GpuColor::transparentBlack() });
        pass.setPipeline (pipeline);
        pass.setUniformBuffer (0, 0, &inputs, sizeof (inputs));

        for (int i = 0; i < numChannels; ++i)
        {
            pass.setTexture (0, 1 + i, channelTextures[(size_t) i]);
            pass.setSampler (0, 1 + numChannels + i, channelSampler);
        }

        pass.draw (3);
        pass.finish();
        frame.submit();

        g.drawTexture (target->asTexture(), viewportArea);
    }

    //==============================================================================
    void resized() override
    {
        auto bounds = getLocalBounds().to<float>().reduced (10.0f);

        presetList.setVisible (presetsVisible);

        if (presetsVisible)
        {
            presetList.setBounds (bounds.removeFromLeft (200.0f));
            bounds.removeFromLeft (10.0f);
        }

        for (auto* component : std::initializer_list<yup::Component*> { &stageTabs, &codeEditor, &statusLabel, &compileButton })
            component->setVisible (editorVisible);

        if (editorVisible)
        {
            auto editorArea = bounds.removeFromBottom (bounds.getHeight() * 0.4f);
            bounds.removeFromBottom (6.0f);

            auto statusRow = editorArea.removeFromBottom (28.0f);
            compileButton.setBounds (statusRow.removeFromRight (100.0f));
            statusRow.removeFromRight (6.0f);
            statusLabel.setBounds (statusRow);
            editorArea.removeFromBottom (4.0f);

            stageTabs.setBounds (editorArea.removeFromTop (32.0f));
            codeEditor.setBounds (editorArea);
        }

        auto transportRow = bounds.removeFromBottom (28.0f);
        bounds.removeFromBottom (6.0f);
        viewportArea = bounds;

        presetsButton.setBounds (transportRow.removeFromLeft (110.0f));
        transportRow.removeFromLeft (10.0f);
        rewindButton.setBounds (transportRow.removeFromLeft (70.0f));
        transportRow.removeFromLeft (4.0f);
        playButton.setBounds (transportRow.removeFromLeft (70.0f));
        transportRow.removeFromLeft (10.0f);

        editorButton.setBounds (transportRow.removeFromRight (100.0f));
        transportRow.removeFromRight (4.0f);
        saveButton.setBounds (transportRow.removeFromRight (80.0f));
        transportRow.removeFromRight (4.0f);
        loadButton.setBounds (transportRow.removeFromRight (80.0f));
        transportRow.removeFromRight (10.0f);

        timeLabel.setBounds (transportRow.removeFromLeft (70.0f));
        fpsLabel.setBounds (transportRow.removeFromLeft (70.0f));
        sizeLabel.setBounds (transportRow);
    }

    //==============================================================================
    void mouseDown (const yup::MouseEvent& event) override
    {
        if (! event.isLeftButtonDown() || ! viewportArea.contains (event.getPosition()))
            return;

        mouseDragging = true;

        const auto position = toShaderPixels (event.getPosition());
        mouse = { position.getX(), position.getY(), position.getX(), position.getY() };
    }

    void mouseDrag (const yup::MouseEvent& event) override
    {
        if (! mouseDragging)
            return;

        const auto position = toShaderPixels (event.getPosition());
        mouse[0] = position.getX();
        mouse[1] = position.getY();
    }

    void mouseUp (const yup::MouseEvent&) override
    {
        if (! mouseDragging)
            return;

        mouseDragging = false;
        mouse[2] = -std::abs (mouse[2]);
        mouse[3] = -std::abs (mouse[3]);
    }

private:
    //==============================================================================
    class PresetListModel : public yup::ListBoxModel
    {
    public:
        int getNumRows() override
        {
            return (int) std::size (shaderPresets);
        }

        yup::String getRowText (int rowIndex) override
        {
            return yup::isPositiveAndBelow (rowIndex, getNumRows()) ? yup::String (shaderPresets[rowIndex].name) : yup::String();
        }

        void selectedRowsChanged (const yup::Array<int>& selectedRows) override
        {
            if (onSelectionChanged && ! selectedRows.isEmpty())
                onSelectionChanged (selectedRows[0]);
        }

        std::function<void (int)> onSelectionChanged;
    };

    //==============================================================================
    /** Mirrors the std140 ShaderToyInputs block of the prelude. */
    struct alignas (16) ShaderToyInputs
    {
        float resolution[3];
        float time;
        float timeDelta;
        float frameRate;
        int32_t frame;
        float pad;
        float mouse[4];
        float date[4];
        float channelTime[4][4]; // std140 pads each array element to 16 bytes
        float channelResolution[4][4];
    };

    static_assert (sizeof (ShaderToyInputs) == 192);
    static_assert (offsetof (ShaderToyInputs, mouse) == 32);
    static_assert (offsetof (ShaderToyInputs, channelTime) == 64);
    static_assert (offsetof (ShaderToyInputs, channelResolution) == 128);

    static constexpr int numChannels = 4;

    // Prepended to both stages: the Shadertoy inputs, and the channels as separate texture + sampler pairs
    static constexpr const char* sourcePrelude = R"glsl(#version 450
layout(set = 0, binding = 0, std140) uniform ShaderToyInputs
{
    vec3 iResolution;
    float iTime;
    float iTimeDelta;
    float iFrameRate;
    int iFrame;
    vec4 iMouse;
    vec4 iDate;
    float iChannelTime[4];
    vec3 iChannelResolution[4];
};
layout(set = 0, binding = 1) uniform texture2D yupChannelTexture0;
layout(set = 0, binding = 2) uniform texture2D yupChannelTexture1;
layout(set = 0, binding = 3) uniform texture2D yupChannelTexture2;
layout(set = 0, binding = 4) uniform texture2D yupChannelTexture3;
layout(set = 0, binding = 5) uniform sampler yupChannelSampler0;
layout(set = 0, binding = 6) uniform sampler yupChannelSampler1;
layout(set = 0, binding = 7) uniform sampler yupChannelSampler2;
layout(set = 0, binding = 8) uniform sampler yupChannelSampler3;
#define iChannel0 sampler2D(yupChannelTexture0, yupChannelSampler0)
#define iChannel1 sampler2D(yupChannelTexture1, yupChannelSampler1)
#define iChannel2 sampler2D(yupChannelTexture2, yupChannelSampler2)
#define iChannel3 sampler2D(yupChannelTexture3, yupChannelSampler3)
)glsl";

    // The markers let a saved bundle be stripped back to the user code, and #line keeps error lines matching the editor
    static constexpr const char* userCodeBegin = "// <yup-user-code>\n#line 1\n";
    static constexpr const char* userCodeEnd = "\n// </yup-user-code>\n";

    // Appended to the fragment stage: Shadertoy expects a bottom-left origin, and the alpha would blend with the clear
    static constexpr const char* fragmentMain = R"glsl(layout(location = 0) out vec4 yupFragColor;
void main()
{
    vec4 yupColor = vec4(0.0);
    mainImage(yupColor, vec2(gl_FragCoord.x, iResolution.y - gl_FragCoord.y));
    yupFragColor = vec4(yupColor.rgb, 1.0);
}
)glsl";

    static constexpr const char* defaultVertexSource = R"glsl(// Fullscreen triangle generated from the vertex index, without vertex buffers.
void main()
{
    float x = float((gl_VertexIndex & 1u) << 2u) - 1.0;
    float y = float((gl_VertexIndex & 2u) << 1u) - 1.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
}
)glsl";

    inline static const yup::Identifier fragmentTabId { "fragment" };
    inline static const yup::Identifier vertexTabId { "vertex" };

    //==============================================================================
    static yup::String wrapUserCode (const yup::String& code, bool fragment)
    {
        return yup::String (sourcePrelude) + userCodeBegin + code + userCodeEnd + (fragment ? fragmentMain : "");
    }

    static std::optional<yup::String> unwrapUserCode (const yup::String& source)
    {
        const yup::String begin (userCodeBegin);
        const int start = source.indexOf (begin);
        const int end = source.lastIndexOf (userCodeEnd);

        if (start < 0 || end < start + begin.length())
            return std::nullopt;

        return source.substring (start + begin.length(), end);
    }

    //==============================================================================
    void loadPreset (int row)
    {
        if (! yup::isPositiveAndBelow (row, (int) std::size (shaderPresets)))
            return;

        const auto file = getAssetPath (shaderPresets[row].fragment);
        if (! file.existsAsFile())
        {
            setStatus ("Missing preset " + file.getFullPathName(), yup::Colors::tomato);
            return;
        }

        fragmentDocument.setText (file.loadFileAsString());
        vertexDocument.setText (defaultVertexSource);
        showStage (false);
        resetEditor();

        rewind();
        requestCompile();
    }

    void showStage (bool vertex)
    {
        stageTabs.setSelectedTab (vertex ? vertexTabId : fragmentTabId, yup::dontSendNotification);
        codeEditor.setDocument (vertex ? vertexDocument : fragmentDocument);

        // The highlighter keeps the lines of the previous document until its syntax is set again
        codeEditor.setSyntaxDefinition ("glsl");

        // setDocument() drops the error lines, so put back the ones belonging to this stage
        if (vertex == errorsInVertex)
        {
            for (const int line : errorLines)
                codeEditor.setErrorLine (line, true);
        }
    }

    // setDocument() only resets the editor for a different document, not for new text in the shown one
    void resetEditor()
    {
        errorLines.clear();
        codeEditor.clearErrorLines();
        codeEditor.setCaretPosition (0); // also collapses the selection
        codeEditor.setScrollOffset ({});
    }

    void setPresetsVisible (bool shouldBeVisible)
    {
        presetsVisible = shouldBeVisible;
        presetsButton.setButtonText (presetsVisible ? "Hide Presets" : "Show Presets");
        resized();
        repaint();
    }

    void setEditorVisible (bool shouldBeVisible)
    {
        editorVisible = shouldBeVisible;
        editorButton.setButtonText (editorVisible ? "Hide Editor" : "Show Editor");
        resized();
        repaint();
    }

    void rewind()
    {
        playbackTime = 0.0;
        playbackFrame = 0;
    }

    void setPlaying (bool shouldPlay)
    {
        playing = shouldPlay;
        playButton.setButtonText (playing ? "Pause" : "Play");
    }

    void setStatus (const yup::String& text, std::optional<yup::Color> color = std::nullopt)
    {
        statusLabel.setText (text, yup::dontSendNotification);
        statusLabel.setColor (yup::Label::Style::textFillColorId, color);
    }

    //==============================================================================
    void requestCompile()
    {
        pendingVertexSource = wrapUserCode (vertexDocument.getText(), false);
        pendingFragmentSource = wrapUserCode (fragmentDocument.getText(), true);
        compilePending = true;

        errorLines.clear();
        codeEditor.clearErrorLines();

        setStatus ("Compiling...");
        repaint();
    }

    /** Runs on the render thread, keeping the last working pipeline when compilation fails. */
    void compile (yup::GpuDevice::Ptr device)
    {
        compilePending = false;

        const auto startMs = yup::Time::getMillisecondCounterHiRes();
        auto result = yup::GpuPipeline::compileFromGlsl (std::move (device), pendingVertexSource, pendingFragmentSource);
        const auto elapsedMs = yup::Time::getMillisecondCounterHiRes() - startMs;

        if (result.failed())
        {
            postCompileResult (result.getErrorMessage(), elapsedMs);
            return;
        }

        pipeline = result.getValue();
        postCompileResult ({}, elapsedMs);
    }

    /** Hands a compile result from the render thread over to the message thread. */
    void postCompileResult (const yup::String& error, double elapsedMs)
    {
        yup::MessageManager::callAsync ([this, weakThis = yup::WeakReference<yup::Component> (this), error, elapsedMs]
        {
            if (weakThis.get() != nullptr)
                showCompileResult (error, elapsedMs);
        });
    }

    void showCompileResult (const yup::String& error, double elapsedMs)
    {
        if (error.isEmpty())
        {
            setStatus ("Compiled OK (" + yup::String (yup::roundToInt (elapsedMs)) + " ms)", yup::Colors::limegreen);
            return;
        }

        // glslang reports "ERROR: <string>:<line>: <message>" lines
        std::vector<int> lines;
        yup::String firstMessage;

        for (const auto& line : yup::StringArray::fromLines (error))
        {
            const auto entry = line.fromFirstOccurrenceOf ("ERROR: ", false, false);
            const auto afterSource = entry.fromFirstOccurrenceOf (":", false, false);
            const auto sourceIndex = entry.upToFirstOccurrenceOf (":", false, false);
            const auto lineNumber = afterSource.upToFirstOccurrenceOf (":", false, false);

            if (sourceIndex.isEmpty() || lineNumber.isEmpty() || ! sourceIndex.containsOnly ("0123456789") || ! lineNumber.containsOnly ("0123456789"))
                continue;

            if (lines.empty())
                firstMessage = afterSource.fromFirstOccurrenceOf (":", false, false).trim();

            lines.push_back (yup::jmax (0, lineNumber.getIntValue() - 1));
        }

        if (lines.empty())
        {
            auto text = error;
            for (const char* prefix : { "Vertex shader compile failed: ", "Fragment shader compile failed: ", "ShaderBundleCompiler: " })
            {
                if (text.startsWith (prefix))
                    text = text.substring (yup::String (prefix).length());
            }

            setStatus (text.upToFirstOccurrenceOf ("\n", false, false), yup::Colors::tomato);
            return;
        }

        const bool vertexFailed = error.startsWith ("Vertex");

        auto status = yup::String (vertexFailed ? "Vertex" : "Fragment") + " line " + yup::String (lines.front() + 1) + ": " + firstMessage;
        if (lines.size() > 1)
            status << " (+" << (int) (lines.size() - 1) << " more)";

        setStatus (status, yup::Colors::tomato);

        errorLines = std::move (lines);
        errorsInVertex = vertexFailed;

        if (! editorVisible)
            setEditorVisible (true);

        showStage (vertexFailed);

        auto& document = vertexFailed ? vertexDocument : fragmentDocument;
        codeEditor.setCaretPosition (document.getLineStartOffset (errorLines.front()));
        codeEditor.scrollToLine (errorLines.front());
        codeEditor.takeKeyboardFocus();
    }

    //==============================================================================
    void loadBundle()
    {
        auto chooser = yup::FileChooser::create ("Load Shader Bundle", yup::File(), "*.ysl");
        chooser->browseForFileToOpen ([this] (bool success, const yup::Array<yup::File>& results)
        {
            if (! success || results.isEmpty())
                return;

            auto loaded = yup::ShaderBundle::loadFromFile (results[0]);
            if (loaded.failed())
            {
                setStatus ("Failed to load .ysl: " + loaded.getErrorMessage(), yup::Colors::tomato);
                return;
            }

            const auto& bundle = loaded.getReference();
            const auto* vertexShader = bundle.findShader (yup::ShaderStage::vertex, yup::ShaderLanguage::glsl);
            const auto* fragmentShader = bundle.findShader (yup::ShaderStage::fragment, yup::ShaderLanguage::glsl);

            const auto vertexCode = vertexShader != nullptr ? unwrapUserCode (vertexShader->inputSource) : std::nullopt;
            const auto fragmentCode = fragmentShader != nullptr ? unwrapUserCode (fragmentShader->inputSource) : std::nullopt;

            if (! vertexCode.has_value() || ! fragmentCode.has_value())
            {
                setStatus ("Bundle was not saved by the Shaders demo", yup::Colors::tomato);
                return;
            }

            vertexDocument.setText (*vertexCode);
            fragmentDocument.setText (*fragmentCode);
            showStage (false);
            resetEditor();

            rewind();
            requestCompile();
        });
    }

    void saveBundle()
    {
        const auto vertexSource = wrapUserCode (vertexDocument.getText(), false);
        const auto fragmentSource = wrapUserCode (fragmentDocument.getText(), true);

        auto chooser = yup::FileChooser::create ("Save Shader Bundle", yup::File(), "*.ysl");
        chooser->browseForFileToSave ([this, vertexSource, fragmentSource] (bool success, const yup::Array<yup::File>& results)
        {
            if (! success || results.isEmpty())
                return;

            const auto result = saveGlslShaderBundle (results[0], vertexSource, fragmentSource);
            if (result.failed())
            {
                setStatus ("Save .ysl: " + result.getErrorMessage(), yup::Colors::tomato);
                return;
            }

            setStatus ("Bundle saved.");
        },
                                      true);
    }

    //==============================================================================
    /** Creates the channel inputs: an RGBA noise texture on iChannel0, black on the others. */
    void createChannels (yup::GpuDevice::Ptr device)
    {
        auto makeTexture = [&device] (int size, const uint8_t* pixels) -> yup::GpuTexture::Ptr
        {
            auto texture = yup::GpuTexture::create (device, yup::GpuTextureDesc ((uint32_t) size, (uint32_t) size, yup::GpuTextureFormat::rgba8unorm));
            if (texture == nullptr)
                return nullptr;

            yup::GpuTextureDataDesc data;
            data.data = pixels;
            return texture->upload (data) ? texture : nullptr;
        };

        constexpr int noiseSize = 256;
        std::vector<uint8_t> noise ((size_t) noiseSize * noiseSize * 4);

        yup::Random random (1234);
        for (auto& value : noise)
            value = (uint8_t) random.nextInt (256);

        const uint8_t black[4] = { 0, 0, 0, 255 };
        const auto blackTexture = makeTexture (1, black);

        channelTextures = { makeTexture (noiseSize, noise.data()), blackTexture, blackTexture, blackTexture };
        channelSampler = yup::GpuSampler::create (device, yup::GpuSamplerDesc (yup::GpuFilter::linear, yup::GpuWrapMode::repeat));
    }

    ShaderToyInputs makeInputs() const
    {
        ShaderToyInputs inputs {};
        inputs.resolution[0] = (float) pixelWidth;
        inputs.resolution[1] = (float) pixelHeight;
        inputs.resolution[2] = 1.0f;
        inputs.time = (float) playbackTime;
        inputs.timeDelta = frameDelta;
        inputs.frameRate = frameRate;
        inputs.frame = playbackFrame;
        std::copy (mouse.begin(), mouse.end(), inputs.mouse);

        const auto now = yup::Time::getCurrentTime();
        inputs.date[0] = (float) now.getYear();
        inputs.date[1] = (float) now.getMonth();
        inputs.date[2] = (float) now.getDayOfMonth();
        inputs.date[3] = (float) (now.getHours() * 3600 + now.getMinutes() * 60 + now.getSeconds()) + (float) now.getMilliseconds() / 1000.0f;

        for (int i = 0; i < numChannels; ++i)
        {
            if (const auto& texture = channelTextures[(size_t) i]; texture != nullptr)
            {
                inputs.channelResolution[i][0] = (float) texture->getWidth();
                inputs.channelResolution[i][1] = (float) texture->getHeight();
                inputs.channelResolution[i][2] = 1.0f;
            }
        }

        return inputs;
    }

    /** Converts a component position to shader pixels, with Shadertoy's bottom-left origin. */
    yup::Point<float> toShaderPixels (yup::Point<float> position) const
    {
        const auto local = position - viewportArea.getTopLeft();
        const float x = yup::jlimit (0.0f, (float) pixelWidth, local.getX() * contextScale);
        const float y = yup::jlimit (0.0f, (float) pixelHeight, local.getY() * contextScale);
        return { x, (float) pixelHeight - y };
    }

    //==============================================================================
    PresetListModel presetModel;
    yup::ListBox presetList;

    yup::TextButton presetsButton { "Show Presets" };
    yup::TextButton rewindButton { "Rewind" };
    yup::TextButton playButton { "Pause" };
    yup::TextButton loadButton { "Load .ysl" };
    yup::TextButton saveButton { "Save .ysl" };
    yup::TextButton editorButton { "Hide Editor" };
    yup::Label timeLabel { "time" };
    yup::Label fpsLabel { "fps" };
    yup::Label sizeLabel { "size" };

    yup::TabBar stageTabs { "stageTabs" };
    yup::CodeDocument fragmentDocument;
    yup::CodeDocument vertexDocument;
    yup::CodeEditor codeEditor;
    yup::Label statusLabel { "status" };
    yup::TextButton compileButton { "Compile" };

    yup::Rectangle<float> viewportArea;
    bool presetsVisible = false;
    bool editorVisible = true;

    // Playback
    bool playing = true;
    double playbackTime = 0.0;
    int playbackFrame = 0;
    float frameDelta = 0.0f;
    float frameRate = 0.0f;
    std::array<float, 4> mouse {};
    bool mouseDragging = false;

    // Render thread state, sized in pixels on the last paint
    int pixelWidth = 0;
    int pixelHeight = 0;
    float contextScale = 1.0f;

    yup::String pendingVertexSource;
    yup::String pendingFragmentSource;
    bool compilePending = false;

    // Lines (0-based) of the last failed compile, in the stage that failed
    std::vector<int> errorLines;
    bool errorsInVertex = false;

    yup::GpuPipeline::Ptr pipeline;
    yup::GpuTarget::Ptr target;
    std::array<yup::GpuTexture::Ptr, numChannels> channelTextures;
    yup::GpuSampler::Ptr channelSampler;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShadersDemo)
};

#endif
