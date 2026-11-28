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

#include <yup_gui_markup/yup_gui_markup.h>

//==============================================================================

// Imports an SVG as a `.yui` drawing, painted next to the SVG itself. The drawing is edited
// live, and every property it declares gets a control setting it on the component painted
class SvgDemo
    : public yup::DragAndDropTargetComponent
    , private yup::CodeDocument::Listener
    , private yup::Timer
{
public:
    SvgDemo()
        : yup::DragAndDropTargetComponent ("SvgDemo")
    {
        const auto svgDirectory = getAssetPath ("data").getChildFile ("svg");

        svgFiles = svgDirectory.findChildFiles (yup::File::findFiles, false, "*.svg");
        svgFiles.sort();

        for (int i = 0; i < svgFiles.size(); ++i)
            samples.addItem (svgFiles[i].getFileName(), i + 1);

        samples.onSelectedItemChanged = [this]
        {
            if (yup::isPositiveAndBelow (samples.getSelectedItemIndex(), svgFiles.size()))
                importSvg (svgFiles[samples.getSelectedItemIndex()]);
        };
        addAndMakeVisible (samples);

        previousButton.onClick = [this] { showSample (-1); };
        addAndMakeVisible (previousButton);

        nextButton.onClick = [this] { showSample (1); };
        addAndMakeVisible (nextButton);

        openButton.onClick = [this]
        {
            auto chooser = yup::FileChooser::create ("Open an SVG", currentFile.getParentDirectory(), "*.svg");
            chooser->browseForFileToOpen ([demo = yup::WeakReference<yup::Component> (this)] (bool success, const yup::Array<yup::File>& results)
            {
                if (demo.get() != nullptr && success && ! results.isEmpty())
                    static_cast<SvgDemo*> (demo.get())->showSvg (results[0]);
            });
        };
        addAndMakeVisible (openButton);

        colorsToggle.setButtonText ("Color properties");
        colorsToggle.setToggleState (true, yup::dontSendNotification);
        colorsToggle.onClick = [this]
        {
            if (currentFile.existsAsFile())
                importSvg (currentFile);
        };
        addAndMakeVisible (colorsToggle);

        editor.setSyntaxDefinition ("yui");
        editor.setMinimapVisible (false);
        editor.setScheme (yup::CodeEditorScheme::getBuiltIn ("oneDark"));
        editor.setOpaque (false);
        document.addListener (this);
        addAndMakeVisible (editor);

        svgCaption.setText ("SVG painted by Drawable", yup::dontSendNotification);
        addAndMakeVisible (svgCaption);
        addAndMakeVisible (svgPreview);

        drawingCaption.setText ("Drawing it is imported as", yup::dontSendNotification);
        addAndMakeVisible (drawingCaption);
        addAndMakeVisible (drawingPreview);

        propertiesView.setViewedComponent (&properties);
        propertiesView.setScrollBarsShown (true, false);
        addAndMakeVisible (propertiesView);

        status.setMultiLine (true);
        status.setReadOnly (true);
        addAndMakeVisible (status);

        divider.setThickness (20.0f);
        divider.setPosition (yup::ResizableDivider::Length::percent (45.0f));
        divider.setMinPosition (yup::ResizableDivider::Length::pixels (200.0f));
        divider.setMinTrailingSize (yup::ResizableDivider::Length::pixels (320.0f));
        divider.setDefaultPosition (yup::ResizableDivider::Length::percent (45.0f));
        divider.onPositionChanged = [this] (float) { resized(); };
        addAndMakeVisible (divider);

        setSize (1000, 600);
        setOpaque (false);

        if (! svgFiles.isEmpty())
            showSvg (svgFiles[yup::jmax (0, svgFiles.indexOf (svgDirectory.getChildFile ("python.svg")))]);
        else
            setStatus ("Drop an SVG file here, or open one.");
    }

    ~SvgDemo() override
    {
        stopTimer();
        document.removeListener (this);
    }

    //==============================================================================
    bool isInterestedInDragSource (const yup::DragAndDropSourceDetails& details) override
    {
        return details.data.isEmpty() || details.data.hasFiles();
    }

    void itemDragEnter (const yup::DragAndDropSourceDetails&) override
    {
        highlighted = true;
        repaint();
    }

    void itemDragExit (const yup::DragAndDropSourceDetails&) override
    {
        highlighted = false;
        repaint();
    }

    bool itemDropped (const yup::DragAndDropSourceDetails& details) override
    {
        highlighted = false;
        repaint();

        for (const auto& file : details.data.getFiles())
        {
            if (file.hasFileExtension ("svg"))
            {
                showSvg (file);
                return true;
            }
        }

        setStatus ("Only SVG files can be imported.");
        return false;
    }

    //==============================================================================
    void paint (yup::Graphics& g) override
    {
        const auto& palette = yup::ApplicationTheme::getGlobalTheme()->getPalette();

        g.setFillColor (palette.getColor (highlighted ? yup::ThemePalette::Role::surfaceRaised : yup::ThemePalette::Role::surface));
        g.fillRoundedRect (svgArea, 8.0f);
        g.fillRoundedRect (drawingArea, 8.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (10.0f);

        auto toolbar = bounds.removeFromTop (30.0f);
        previousButton.setBounds (toolbar.removeFromLeft (30.0f));
        toolbar.removeFromLeft (4.0f);
        samples.setBounds (toolbar.removeFromLeft (240.0f));
        toolbar.removeFromLeft (4.0f);
        nextButton.setBounds (toolbar.removeFromLeft (30.0f));
        toolbar.removeFromLeft (8.0f);
        openButton.setBounds (toolbar.removeFromLeft (90.0f));
        toolbar.removeFromLeft (8.0f);
        colorsToggle.setBounds (toolbar.removeFromLeft (180.0f));
        bounds.removeFromTop (8.0f);

        divider.layoutComponents (bounds, &leftPane, &rightPane);
        editor.setBounds (leftPane.getBounds());

        auto right = rightPane.getBounds();
        status.setBounds (right.removeFromBottom (84.0f));
        right.removeFromBottom (6.0f);

        const auto minListHeight = 3.0f * PropertyList::rowHeight;
        const auto listHeight = yup::jlimit (minListHeight, yup::jmax (minListHeight, right.getHeight() * 0.45f), properties.getContentHeight());
        propertiesView.setBounds (right.removeFromBottom (listHeight));
        properties.setSize (propertiesView.getWidth() - 14.0f, properties.getContentHeight());
        right.removeFromBottom (6.0f);

        auto captions = right.removeFromTop (22.0f);
        const auto half = (right.getWidth() - 8.0f) * 0.5f;

        svgCaption.setBounds (captions.removeFromLeft (half));
        captions.removeFromLeft (8.0f);
        drawingCaption.setBounds (captions);

        svgArea = right.removeFromLeft (half);
        right.removeFromLeft (8.0f);
        drawingArea = right;

        svgPreview.setBounds (svgArea.reduced (8.0f));
        drawingPreview.setBounds (drawingArea.reduced (8.0f));
    }

private:
    //==============================================================================
    struct SvgPreview final : yup::Component
    {
        SvgPreview()
            : Component ("svgPreview")
        {
            setOpaque (false);
        }

        void paint (yup::Graphics& g) override
        {
            drawable.paint (g, getLocalBounds(), yup::Fitting::scaleToFit, yup::Justification::center);
        }

        yup::Drawable drawable;
    };

    struct DrawingPreview final : yup::Component
    {
        DrawingPreview()
            : Component ("drawingPreview")
        {
            setOpaque (false);
        }

        void paint (yup::Graphics& g) override
        {
            if (auto style = getStyle())
                style->paint (g, *yup::ApplicationTheme::getGlobalTheme(), *this);
        }
    };

    //==============================================================================
    // The control of a property: sliders for a number or the channels of a color, a switch for a boolean
    class PropertyRow final : public yup::Component
    {
    public:
        PropertyRow (yup::Component& targetComponent, const yup::DataTree& declaration)
            : Component ("propertyRow")
            , target (targetComponent)
            , name (declaration.getProperty ("name").toString())
        {
            setOpaque (false);

            const auto typeName = declaration.getProperty ("type").toString();
            const auto value = declaration.getProperty ("value").toString().trim();

            label.setText (name.toString(), yup::dontSendNotification);
            addAndMakeVisible (label);

            switch (yup::ComponentProperty::getTypeFromName (typeName))
            {
                case yup::ComponentProperty::Type::color:
                    makeColor (value);
                    break;

                case yup::ComponentProperty::Type::number:
                    makeNumber (value.getDoubleValue(), 0.0);
                    break;

                case yup::ComponentProperty::Type::integer:
                    makeNumber (value.getDoubleValue(), 1.0);
                    break;

                case yup::ComponentProperty::Type::boolean:
                    makeSwitch (value == "true");
                    break;

                default:
                    label.setText (name.toString() + " (" + typeName + ", set from code)", yup::dontSendNotification);
                    break;
            }
        }

        const yup::Identifier& getPropertyName() const noexcept { return name; }

        void paint (yup::Graphics& g) override
        {
            if (! color)
                return;

            g.setFillColor (*color);
            g.fillRoundedRect (swatch, 4.0f);
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced (6.0f, 3.0f);

            if (sliders.empty() && toggle == nullptr)
            {
                label.setBounds (bounds);
                return;
            }

            label.setBounds (bounds.removeFromLeft (bounds.getWidth() * 0.35f));

            if (color)
            {
                swatch = bounds.removeFromLeft (bounds.getHeight());
                bounds.removeFromLeft (6.0f);
            }

            if (toggle != nullptr)
                toggle->setBounds (bounds.removeFromLeft (48.0f));

            const auto sliderWidth = bounds.getWidth() / static_cast<float> (yup::jmax<size_t> (1, sliders.size()));

            for (auto& slider : sliders)
                slider->setBounds (bounds.removeFromLeft (sliderWidth).reduced (2.0f, 0.0f));
        }

    private:
        // A color is read when it is written as `#rgb`, `#rgba`, `#rrggbb` or `#rrggbbaa`
        static yup::Color parseColor (const yup::String& text)
        {
            auto digits = text.substring (1);

            if (! text.startsWithChar ('#') || ! digits.containsOnly ("0123456789abcdefABCDEF"))
                return yup::Color (0xff808080);

            if (digits.length() <= 4)
            {
                yup::String doubled;

                for (const auto digit : digits)
                    doubled << yup::String::charToString (digit) << yup::String::charToString (digit);

                digits = doubled;
            }

            const auto value = static_cast<yup::uint32> (digits.getHexValue32());

            if (digits.length() == 8)
                return yup::Color ((value >> 8) | (value << 24));

            return digits.length() == 6 ? yup::Color (0xff000000 | value) : yup::Color (0xff808080);
        }

        yup::Slider& addSlider (double minimum, double maximum, double step, double value)
        {
            auto& slider = *sliders.emplace_back (std::make_unique<yup::Slider> (yup::Slider::LinearBarHorizontal));
            slider.setRange (minimum, maximum, step);
            slider.setValue (value, yup::dontSendNotification);
            addAndMakeVisible (slider);
            return slider;
        }

        void makeColor (const yup::String& value)
        {
            color = parseColor (value);

            for (const auto channel : { color->getRed(), color->getGreen(), color->getBlue() })
            {
                addSlider (0.0, 255.0, 1.0, channel).onValueChanged = [this] (double)
                {
                    const auto getChannel = [this] (size_t index)
                    {
                        return static_cast<yup::uint8> (yup::roundToInt (sliders[index]->getValue()));
                    };

                    color = yup::Color (color->getAlpha(), getChannel (0), getChannel (1), getChannel (2));
                    target.setProperty (name, yup::VariantConverter<yup::Color>::toVar (*color));
                    repaint();
                };
            }
        }

        // The range is a guess from the default: twice as far from zero, or up to one
        void makeNumber (double value, double step)
        {
            const auto unit = step > 0.0 ? 10.0 : 1.0;

            addSlider (yup::jmin (0.0, value * 2.0), value > 0.0 ? value * 2.0 : unit, step, value).onValueChanged = [this, step] (double newValue)
            {
                if (step > 0.0)
                {
                    label.setText (name.toString() + " = " + yup::String (yup::roundToInt (newValue)), yup::dontSendNotification);
                    target.setProperty (name, yup::roundToInt (newValue));
                }
                else
                {
                    label.setText (name.toString() + " = " + yup::String (newValue, 2), yup::dontSendNotification);
                    target.setProperty (name, newValue);
                }
            };
        }

        void makeSwitch (bool value)
        {
            toggle = std::make_unique<yup::SwitchButton> ("propertySwitch");
            toggle->setToggleState (value, yup::dontSendNotification);
            toggle->onClick = [this] { target.setProperty (name, toggle->getToggleState()); };
            addAndMakeVisible (*toggle);
        }

        yup::Component& target;
        const yup::Identifier name;
        yup::Label label { "propertyName" };
        std::vector<std::unique_ptr<yup::Slider>> sliders;
        std::unique_ptr<yup::SwitchButton> toggle;
        std::optional<yup::Color> color;
        yup::Rectangle<float> swatch;
    };

    //==============================================================================
    // The controls of the properties a drawing declares, kept while their declarations stay the same
    class PropertyList final : public yup::Component
    {
    public:
        static constexpr float rowHeight = 28.0f;

        PropertyList()
            : Component ("propertyList")
        {
            setOpaque (false);

            hint.setText ("The drawing has no properties. Declare one, for example\n"
                          "    property amount: number = 0.5;\n"
                          "and read it in an element to get a control for it.",
                          yup::dontSendNotification);
            addAndMakeVisible (hint);
        }

        void update (yup::Component& target, const yup::DataTree& drawing)
        {
            yup::String newDeclarations;

            for (const auto& child : drawing)
                if (isProperty (child))
                    newDeclarations << child.getProperty ("name").toString() << ":" << child.getProperty ("type").toString()
                                    << "=" << child.getProperty ("value").toString() << ";";

            if (newDeclarations == declarations)
                return;

            clear (target);
            declarations = newDeclarations;

            for (const auto& child : drawing)
                if (isProperty (child))
                    addAndMakeVisible (*rows.emplace_back (std::make_unique<PropertyRow> (target, child)));

            hint.setVisible (rows.empty());
            setSize (getWidth(), getContentHeight());
            resized();
        }

        // Removes the controls, and what they set on the component
        void clear (yup::Component& target)
        {
            for (const auto& row : rows)
                target.removeProperty (row->getPropertyName());

            rows.clear();
            declarations.clear();
            hint.setVisible (true);
        }

        float getContentHeight() const noexcept
        {
            return rowHeight * static_cast<float> (yup::jmax<size_t> (3, rows.size()));
        }

        void resized() override
        {
            auto bounds = getLocalBounds();
            hint.setBounds (bounds.reduced (6.0f));

            for (auto& row : rows)
                row->setBounds (bounds.removeFromTop (rowHeight));
        }

    private:
        static bool isProperty (const yup::DataTree& element)
        {
            return element.getType().toString() == "property";
        }

        yup::Label hint { "propertyHint" };
        std::vector<std::unique_ptr<PropertyRow>> rows;
        yup::String declarations;
    };

    //==============================================================================
    void codeDocumentChanged (yup::CodeDocument&, const yup::CodeDocument::Change&) override
    {
        // Typing restarts the countdown, so a burst of keystrokes rebuilds once
        if (! importing)
            startTimer (150);
    }

    void timerCallback() override
    {
        stopTimer();
        rebuild();
    }

    // Selects a file among the samples, adding it when it comes from somewhere else
    void showSvg (const yup::File& file)
    {
        auto index = svgFiles.indexOf (file);

        if (index < 0)
        {
            svgFiles.add (file);
            index = svgFiles.size() - 1;
            samples.addItem (file.getFileName(), svgFiles.size());
        }

        samples.setSelectedItemIndex (index, yup::dontSendNotification);
        importSvg (file);
    }

    // Steps through the samples, wrapping around at either end
    void showSample (int delta)
    {
        if (svgFiles.isEmpty())
            return;

        const auto count = svgFiles.size();
        const auto index = (yup::jmax (0, samples.getSelectedItemIndex()) + delta + count) % count;
        showSvg (svgFiles[index]);
    }

    // Remote images are downloaded once, a failed download included, as the request blocks
    std::optional<yup::Image> fetchHttpImage (const yup::String& href)
    {
        if (! href.startsWithIgnoreCase ("http:") && ! href.startsWithIgnoreCase ("https:"))
            return std::nullopt;

        if (! httpImages.contains (href))
            httpImages.set (href, downloadImage (href));

        const auto image = httpImages[href];
        return image.isValid() ? std::optional<yup::Image> (image) : std::nullopt;
    }

    static yup::Image downloadImage (const yup::String& href)
    {
        yup::MemoryBlock imageData;
        int statusCode = 0;

        const auto streamOptions = yup::URL::InputStreamOptions (yup::URL::ParameterHandling::inAddress)
                                       .withConnectionTimeoutMs (5000)
                                       .withNumRedirectsToFollow (5)
                                       .withStatusCode (&statusCode)
                                       .withExtraHeaders ("User-Agent: YUP SVG Demo\r\nAccept: image/*\r\n");

        const auto stream = yup::URL (href).createInputStream (streamOptions);
        if (stream == nullptr)
            return {};

        stream->readIntoMemoryBlock (imageData);

        if ((statusCode != 0 && (statusCode < 200 || statusCode >= 300)) || imageData.isEmpty())
            return {};

        auto loaded = yup::Image::loadFromData (imageData.asBytes());
        return loaded.wasOk() ? loaded.getValue() : yup::Image();
    }

    yup::Drawable::ParseOptions makeParseOptions (const yup::File& file)
    {
        yup::Drawable::ParseOptions options;
        options.baseDirectory = file.getParentDirectory();

#if ! YUP_EMSCRIPTEN // A blocking request can never complete on the browser main thread
        options.imageResolver = [this] (yup::StringRef href, const yup::File&) -> std::optional<yup::Image>
        {
            return fetchHttpImage (yup::String (href.text));
        };
#endif

        options.fontResolver = [] (yup::StringRef, float fontSize, int, bool) -> std::optional<yup::Font>
        {
            return yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (fontSize);
        };

        return options;
    }

    void importSvg (const yup::File& file)
    {
        const auto newParseOptions = makeParseOptions (file);

        yup::YuiSvgImporter::Options options;
        options.colorProperties = colorsToggle.getToggleState();

        const auto imported = yup::YuiSvgImporter::importSvg (file, options, newParseOptions);

        if (! imported.wasOk())
        {
            setStatus ("Unable to read " + file.getFileName() + " as an SVG.");
            return;
        }

        currentFile = file;
        parseOptions = newParseOptions;
        warnings = imported.warnings;

        svgPreview.drawable.clear();
        svgPreview.drawable.parseSVG (file, parseOptions);
        svgPreview.repaint();

        // Another SVG starts from its own values, even when it declares the same properties
        properties.clear (drawingPreview);

        {
            const yup::ScopedValueSetter<bool> scope (importing, true);
            editor.setText (imported.toText());
        }

        stopTimer();
        rebuild();
    }

    // A text that doesn't compile keeps the drawing painted before, its problem is marked where it is written
    void rebuild()
    {
        editor.clearErrorLines();

        const auto parsed = yup::YuiParser::parse (document.getText());

        if (! parsed.wasOk())
        {
            for (const auto& problem : parsed.diagnostics)
                if (problem.line > 0)
                    editor.setErrorLine (problem.line - 1, true);

            setStatus (yup::DataTreeDiagnostic::join (parsed.diagnostics));
            return;
        }

        const auto drawing = parsed.document.getChildWithName ("drawing");

        if (! drawing.isValid())
        {
            setStatus ("The text has no drawing.");
            return;
        }

        yup::DataTreeDiagnostic failure;
        const auto created = yup::DrawingStyle::create (drawingPreview, drawing, nullptr, &failure);

        if (created.failed())
        {
            if (const auto located = parsed.sourceMap.locate (failure); located.line > 0)
                editor.setErrorLine (located.line - 1, true);

            setStatus ("Error: " + created.getErrorMessage());
            return;
        }

        // The images of the drawing are the ones of the SVG, found the way its parser found them
        created.getReference()->setImageResolver ([options = parseOptions] (const yup::String& source)
        {
            return yup::SVGParser::loadImageFromHref (options, source);
        });

        drawingPreview.setStyle (created.getReference());
        drawingPreview.repaint();

        properties.update (drawingPreview, drawing);
        resized();

        yup::String text;
        text << currentFile.getFileName() << " as a drawing with " << static_cast<int> (created.getReference()->getProperties().size())
             << " properties. Edit it, or declare a property and read it in an element to get a control for it.";

        if (! warnings.isEmpty())
            text << "\nPainted differently from the SVG:\n- " << warnings.joinIntoString ("\n- ");

        setStatus (text);
    }

    void setStatus (const yup::String& text)
    {
        status.setText (text, yup::dontSendNotification);
    }

    //==============================================================================
    yup::Array<yup::File> svgFiles;
    yup::File currentFile;
    yup::Drawable::ParseOptions parseOptions;
    yup::HashMap<yup::String, yup::Image> httpImages;
    yup::StringArray warnings;

    yup::ComboBox samples { "samples" };
    IconButton previousButton { "previousSample", YUP_ICON_CHEVRON_LEFT };
    IconButton nextButton { "nextSample", YUP_ICON_CHEVRON_RIGHT };
    yup::TextButton openButton { "Open..." };
    yup::ToggleButton colorsToggle { "colorProperties" };

    yup::CodeDocument document;
    yup::CodeEditor editor { document };

    yup::Label svgCaption { "svgCaption" };
    yup::Label drawingCaption { "drawingCaption" };
    SvgPreview svgPreview;
    DrawingPreview drawingPreview;
    yup::Rectangle<float> svgArea, drawingArea;

    // The viewport refers to the list, and its controls to the preview
    PropertyList properties;
    yup::Viewport propertiesView { "propertiesView" };
    yup::TextEditor status { "status" };

    yup::ResizableDivider divider { yup::ResizableDivider::Orientation::vertical, "divider" };
    yup::Component leftPane, rightPane;
    bool importing = false;
    bool highlighted = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SvgDemo)
};
