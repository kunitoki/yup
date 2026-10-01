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

namespace PathEditor
{

//==============================================================================

/** Gives the keyboard focus back to the closest ancestor that wants it (the editor), after popups steal it. */
inline void focusEditorRoot (yup::Component& component)
{
    for (auto* parent = component.getParentComponent(); parent != nullptr; parent = parent->getParentComponent())
    {
        if (parent->getWantsKeyboardFocus())
        {
            parent->takeKeyboardFocus();
            return;
        }
    }
}

//==============================================================================

/** A vertical scrolling container for a single content component. */
class ScrollPanel : public yup::Component
{
public:
    static constexpr float scrollBarWidth = 10.0f;

    ScrollPanel()
        : yup::Component ("ScrollPanel")
    {
        setOpaque (false);

        scrollBar.setVisibilityMode (yup::ScrollBar::VisibilityMode::autoHide);
        scrollBar.onScrollPositionChanged = [this] (double)
        {
            updateContentBounds();
        };
        addAndMakeVisible (scrollBar);
    }

    void setContent (yup::Component& newContent)
    {
        content = &newContent;
        addAndMakeVisible (newContent, 0);
    }

    void setContentHeight (float newHeight)
    {
        contentHeight = newHeight;
        updateScrollRange();
    }

    void resized() override
    {
        scrollBar.setBounds (getLocalBounds().removeFromRight (scrollBarWidth));
        updateScrollRange();
    }

    void mouseWheel (const yup::MouseEvent&, const yup::MouseWheelData& wheelData) override
    {
        scrollBar.scrollBy (-wheelData.getDeltaY() * 60.0);
    }

private:
    void updateScrollRange()
    {
        const double visibleHeight = getHeight();
        const double start = yup::jlimit (0.0, yup::jmax (0.0, contentHeight - visibleHeight), scrollBar.getCurrentRangeStart());

        scrollBar.setRangeLimits (0.0, yup::jmax (static_cast<double> (contentHeight), visibleHeight));
        scrollBar.setCurrentRange (start, start + visibleHeight);
        updateContentBounds();
    }

    void updateContentBounds()
    {
        if (content != nullptr)
            content->setBounds (0.0f, -static_cast<float> (scrollBar.getCurrentRangeStart()), getWidth() - scrollBarWidth, contentHeight);
    }

    yup::ScrollBar scrollBar { yup::ScrollBar::Orientation::vertical };
    yup::Component* content = nullptr;
    float contentHeight = 0.0f;
};

//==============================================================================

/** A row of equally sized buttons. */
class ButtonRow : public yup::Component
{
public:
    ButtonRow()
        : yup::Component ("ButtonRow")
    {
        setOpaque (false);
    }

    template <class ButtonType = yup::TextButton>
    ButtonType& addButton (const yup::String& text, std::function<void()> onClick)
    {
        auto newButton = std::make_unique<ButtonType> (text);
        auto& button = *newButton;
        button.setButtonText (text);
        button.onClick = std::move (onClick);
        addAndMakeVisible (button);

        buttons.push_back (std::move (newButton));
        return button;
    }

    void resized() override
    {
        if (buttons.empty())
            return;

        constexpr float gap = 4.0f;
        auto area = getLocalBounds();
        const float buttonWidth = (area.getWidth() - gap * static_cast<float> (buttons.size() - 1)) / static_cast<float> (buttons.size());

        for (auto& button : buttons)
        {
            button->setBounds (area.removeFromLeft (buttonWidth));
            area.removeFromLeft (gap);
        }
    }

private:
    std::vector<std::unique_ptr<yup::Button>> buttons;
};

//==============================================================================

/** A compact color editor: swatch, hex field and H / S / V / A sliders. */
class ColorField : public yup::Component
{
public:
    static constexpr float preferredHeight = 24.0f + 4.0f + 4.0f * 22.0f;

    ColorField()
        : yup::Component ("ColorField")
    {
        setOpaque (false);

        hexEditor.onTextChange = [this]
        {
            hexChanged();
        };
        addAndMakeVisible (hexEditor);

        static constexpr const char* channelNames[] = { "H", "S", "V", "A" };

        for (std::size_t i = 0; i < sliders.size(); ++i)
        {
            labels[i].setText (channelNames[i], yup::dontSendNotification);
            addAndMakeVisible (labels[i]);

            auto& slider = sliders[i];
            slider.setRange (0.0, 1.0, 0.001);
            slider.onValueChanged = [this, i] (double value)
            {
                channels[i] = static_cast<float> (value);

                if (! updating)
                    emitChange();
            };
            slider.onDragStart = [this] (const yup::MouseEvent&)
            {
                if (onGestureStart)
                    onGestureStart();
            };
            slider.onDragEnd = [this] (const yup::MouseEvent&)
            {
                if (onGestureEnd)
                    onGestureEnd();
            };
            addAndMakeVisible (slider);
        }
    }

    /** Shows a color coming from the model, without notifying. */
    void setColor (yup::Color newColor)
    {
        if (newColor != getColor())
        {
            const auto [hue, saturation, value] = newColor.toHSV();

            // Hue and saturation are undefined for greys and black: keep the previous ones
            if (value > 0.0f && saturation > 0.0f)
                channels[0] = hue;
            if (value > 0.0f)
                channels[1] = saturation;

            channels[2] = value;
            channels[3] = newColor.getAlphaFloat();
        }

        const yup::ScopedValueSetter<bool> updateScope (updating, true);

        for (std::size_t i = 0; i < sliders.size(); ++i)
            sliders[i].setValue (channels[i], yup::dontSendNotification);

        if (! hexEditor.hasKeyboardFocus())
            hexEditor.setText (getColor().toString(), yup::dontSendNotification);

        repaint();
    }

    yup::Color getColor() const
    {
        return yup::Color::fromHSV (channels[0], channels[1], channels[2], channels[3]);
    }

    std::function<void (yup::Color)> onChange;
    std::function<void()> onGestureStart;
    std::function<void()> onGestureEnd;

    void resized() override
    {
        auto area = getLocalBounds();

        auto topRow = area.removeFromTop (24.0f);
        swatchArea = topRow.removeFromLeft (40.0f);
        topRow.removeFromLeft (6.0f);
        hexEditor.setBounds (topRow);
        area.removeFromTop (4.0f);

        for (std::size_t i = 0; i < sliders.size(); ++i)
        {
            auto row = area.removeFromTop (20.0f);
            area.removeFromTop (2.0f);
            labels[i].setBounds (row.removeFromLeft (18.0f));
            sliders[i].setBounds (row);
        }
    }

    void paint (yup::Graphics& g) override
    {
        drawCheckerboard (g, swatchArea, 6.0f);
        g.setFillColor (getColor());
        g.fillRect (swatchArea);
        g.setStrokeColor (getPaletteColor (yup::ThemePalette::Role::outline));
        g.setStrokeWidth (1.0f);
        g.strokeRect (swatchArea);
    }

private:
    void emitChange()
    {
        hexEditor.setText (getColor().toString(), yup::dontSendNotification);
        repaint();

        if (onChange)
            onChange (getColor());
    }

    void hexChanged()
    {
        auto text = hexEditor.getText().trim();
        if (! text.startsWith ("#"))
            text = "#" + text;

        const int length = text.length();
        if ((length != 4 && length != 7 && length != 9) || ! text.substring (1).containsOnly ("0123456789abcdefABCDEF"))
            return;

        setColor (yup::Color::fromString (text));

        if (onChange)
            onChange (getColor());
    }

    yup::TextEditor hexEditor;
    std::array<yup::Label, 4> labels;
    std::array<yup::Slider, 4> sliders { yup::Slider (yup::Slider::LinearHorizontal), yup::Slider (yup::Slider::LinearHorizontal), yup::Slider (yup::Slider::LinearHorizontal), yup::Slider (yup::Slider::LinearHorizontal) };
    std::array<float, 4> channels { 0.0f, 0.0f, 1.0f, 1.0f };
    yup::Rectangle<float> swatchArea;
    bool updating = false;
};

//==============================================================================

/** Shows the stops of a gradient paint: click to select or add a stop, drag to move it. */
class GradientStopsBar : public yup::Component
{
public:
    explicit GradientStopsBar (PathDocument& documentToEdit)
        : yup::Component ("GradientStopsBar")
        , document (documentToEdit)
    {
        setOpaque (false);

        removeButton.setButtonText ("-");
        removeButton.onClick = [this]
        {
            removeSelectedStop();
        };
        addAndMakeVisible (removeButton);
    }

    void setPaint (const yup::DataTree& newPaintNode)
    {
        paintNode = newPaintNode;

        if (! selectedStop.isValid() || selectedStop.getParent() != paintNode)
            selectedStop = paintNode.getNumChildren() > 0 ? paintNode.getChild (0) : yup::DataTree();

        removeButton.setEnabled (paintNode.getNumChildren() > 2);
        repaint();
    }

    const yup::DataTree& getSelectedStop() const noexcept { return selectedStop; }

    std::function<void()> onStopSelected;

    void resized() override
    {
        auto area = getLocalBounds();
        removeButton.setBounds (area.removeFromRight (24.0f).withTrimmedBottom (12.0f));
        area.removeFromRight (6.0f);
        barArea = area.removeFromTop (area.getHeight() - 12.0f).reduced (6.0f, 0.0f);
    }

    void paint (yup::Graphics& g) override
    {
        if (! paintNode.isValid())
            return;

        drawCheckerboard (g, barArea, 6.0f);

        std::vector<yup::ColorGradient::ColorStop> colorStops;
        for (const auto& stop : getPaddedStops (paintNode))
            colorStops.emplace_back (stop.color, yup::Point<float> (barArea.getX() + stop.offset * barArea.getWidth(), barArea.getCenterY()), stop.offset);

        g.setFillColorGradient (yup::ColorGradient (yup::ColorGradient::Linear, std::move (colorStops)));
        g.fillRect (barArea);

        const auto accent = getPaletteColor (yup::ThemePalette::Role::accent);
        for (const auto& stop : paintNode)
        {
            const auto marker = getMarkerBounds (stop);
            g.setFillColor (PathEditor::getColor (stop, Ids::color).withAlpha (1.0f));
            g.fillRect (marker);
            g.setStrokeColor (stop == selectedStop ? accent : getPaletteColor (yup::ThemePalette::Role::outline));
            g.setStrokeWidth (stop == selectedStop ? 2.0f : 1.0f);
            g.strokeRect (marker);
        }
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        if (dragging || ! event.isLeftButtonDown() || ! paintNode.isValid())
            return;

        const auto position = event.getPosition();

        for (const auto& stop : paintNode)
        {
            if (getMarkerBounds (stop).enlarged (3.0f).contains (position))
            {
                selectStop (stop);
                document.beginGesture ("Move gradient stop");
                dragging = true;
                return;
            }
        }

        if (! barArea.contains (position))
            return;

        const float offset = getOffsetAt (position.getX());
        const auto newStop = document.createStop (offset, getColorAt (offset));

        document.edit ("Add gradient stop", [&]
        {
            document.addChild (paintNode, newStop);
        });

        selectStop (newStop);
    }

    void mouseDrag (const yup::MouseEvent& event) override
    {
        if (! dragging)
            return;

        document.apply ([&]
        {
            document.setProperty (selectedStop, Ids::offset, getOffsetAt (event.getPosition().getX()));
        });
    }

    void mouseUp (const yup::MouseEvent&) override
    {
        if (! dragging)
            return;

        dragging = false;
        document.endGesture();
    }

private:
    yup::Rectangle<float> getMarkerBounds (const yup::DataTree& stop) const
    {
        const float x = barArea.getX() + getFloat (stop, Ids::offset) * barArea.getWidth();
        return { x - 5.0f, barArea.getBottom() + 1.0f, 10.0f, 10.0f };
    }

    float getOffsetAt (float x) const
    {
        return yup::jlimit (0.0f, 1.0f, (x - barArea.getX()) / yup::jmax (1.0f, barArea.getWidth()));
    }

    yup::Color getColorAt (float offset) const
    {
        const auto stops = getPaddedStops (paintNode);

        for (std::size_t i = 1; i < stops.size(); ++i)
        {
            if (offset <= stops[i].offset)
            {
                const float span = stops[i].offset - stops[i - 1].offset;
                const float t = span > 0.0f ? (offset - stops[i - 1].offset) / span : 0.0f;
                return stops[i - 1].color.mixedWith (stops[i].color, t, yup::ColorSpace::SRGB);
            }
        }

        return stops.back().color;
    }

    void selectStop (const yup::DataTree& stop)
    {
        selectedStop = stop;
        repaint();

        if (onStopSelected)
            onStopSelected();
    }

    void removeSelectedStop()
    {
        if (paintNode.getNumChildren() <= 2 || ! selectedStop.isValid())
            return;

        document.edit ("Remove gradient stop", [&]
        {
            document.removeChild (paintNode, selectedStop);
        });

        selectStop (paintNode.getChild (0));
    }

    PathDocument& document;
    yup::DataTree paintNode;
    yup::DataTree selectedStop;
    yup::TextButton removeButton;
    yup::Rectangle<float> barArea;
    bool dragging = false;
};

//==============================================================================

/** The layers of the document, topmost first, with buttons to add, duplicate, delete, reorder and hide them. */
class LayerList
    : public yup::Component
    , private yup::ListBoxModel
{
public:
    LayerList (PathDocument& documentToEdit, Selection& selectionToEdit)
        : yup::Component ("LayerList")
        , document (documentToEdit)
        , selection (selectionToEdit)
    {
        setOpaque (false);

        listBox.setModel (this);
        listBox.setSelectionMode (yup::ListBox::SelectionMode::single);
        listBox.setDragSourceEnabled (false);
        listBox.setRowSize (26.0f);
        addAndMakeVisible (listBox);

        addLayerButton = &buttons.addButton ("Add", [this]
        {
            showAddMenu();
        });

        duplicateButton = &buttons.addButton ("Dup", [this]
        {
            if (selection.layer.isValid())
                setSelectedLayer (document.duplicateLayer (selection.layer));
        });

        deleteButton = &buttons.addButton ("Del", [this]
        {
            if (! selection.layer.isValid())
                return;

            document.deleteLayer (selection.layer);
            setSelectedLayer ({});
        });

        upButton = &buttons.addButton ("Up", [this]
        {
            if (selection.layer.isValid())
                document.moveLayer (selection.layer, 1);
        });

        downButton = &buttons.addButton ("Down", [this]
        {
            if (selection.layer.isValid())
                document.moveLayer (selection.layer, -1);
        });

        visibilityButton = &buttons.addButton ("Hide", [this]
        {
            if (! selection.layer.isValid())
                return;

            const bool visible = getBool (selection.layer, Ids::visible);
            document.edit (visible ? "Hide layer" : "Show layer", [&]
            {
                document.setProperty (selection.layer, Ids::visible, ! visible);
            });
        });

        addAndMakeVisible (buttons);
    }

    std::function<void()> onSelectionChanged;

    /** Rebuilds the rows only when layers were added, removed, moved, renamed or hidden. */
    void refresh()
    {
        const auto& root = document.getRoot();

        std::vector<Row> newRows;
        for (int i = root.getNumChildren(); --i >= 0;)
        {
            const auto layer = root.getChild (i);
            newRows.push_back ({ layer, getString (layer, Ids::name) + (getBool (layer, Ids::visible) ? "" : "  (hidden)") });
        }

        const yup::ScopedValueSetter<bool> updateScope (updating, true);

        if (newRows != rows)
        {
            rows = std::move (newRows);
            listBox.updateContent();
        }

        const auto selectedRow = std::find_if (rows.begin(), rows.end(), [this] (const Row& row)
        {
            return row.layer == selection.layer;
        });

        if (selectedRow != rows.end())
            listBox.selectRow (static_cast<int> (std::distance (rows.begin(), selectedRow)), true, yup::dontSendNotification);
        else
            listBox.deselectAllRows (yup::dontSendNotification);

        const bool hasLayer = selection.layer.isValid();
        const int index = root.indexOf (selection.layer);
        duplicateButton->setEnabled (hasLayer);
        deleteButton->setEnabled (hasLayer);
        upButton->setEnabled (hasLayer && index < root.getNumChildren() - 1);
        downButton->setEnabled (hasLayer && index > 0);
        visibilityButton->setEnabled (hasLayer);
        visibilityButton->setButtonText (hasLayer && ! getBool (selection.layer, Ids::visible) ? "Show" : "Hide");
    }

    void resized() override
    {
        auto area = getLocalBounds();
        buttons.setBounds (area.removeFromBottom (26.0f));
        area.removeFromBottom (4.0f);
        listBox.setBounds (area);
    }

private:
    struct Row
    {
        yup::DataTree layer;
        yup::String text;

        bool operator== (const Row& other) const { return layer == other.layer && text == other.text; }
    };

    int getNumRows() override { return static_cast<int> (rows.size()); }

    yup::String getRowText (int rowIndex) override
    {
        return yup::isPositiveAndBelow (rowIndex, static_cast<int> (rows.size())) ? rows[static_cast<std::size_t> (rowIndex)].text : yup::String();
    }

    void selectedRowsChanged (const yup::Array<int>& selectedRows) override
    {
        if (updating)
            return;

        const int row = selectedRows.isEmpty() ? -1 : selectedRows.getFirst();
        setSelectedLayer (yup::isPositiveAndBelow (row, static_cast<int> (rows.size())) ? rows[static_cast<std::size_t> (row)].layer : yup::DataTree());
    }

    void setSelectedLayer (const yup::DataTree& layer)
    {
        if (selection.layer == layer)
            return;

        selection.layer = layer;
        selection.vertex = {};

        if (onSelectionChanged)
            onSelectionChanged();
    }

    void showAddMenu()
    {
        auto menu = yup::PopupMenu::create (yup::PopupMenu::Options {}
                                                .withParentComponent (getPopupParentComponent())
                                                .withRelativePosition (addLayerButton));

        for (int i = 0; i < static_cast<int> (std::size (presetNames)); ++i)
            menu->addItem (presetNames[i], i + 1);

        menu->show ([this] (int selectedId)
        {
            focusEditorRoot (*this);

            if (selectedId <= 0)
                return;

            const auto& root = document.getRoot();
            const auto layerColor = yup::Color::fromHSV (std::fmod (static_cast<float> (root.getNumChildren()) * 0.618034f, 1.0f), 0.55f, 0.95f);
            const auto path = createPresetPath (static_cast<Preset> (selectedId), document.getArtboardBounds());

            setSelectedLayer (document.addLayer (document.createLayer (yup::String (presetNames[selectedId - 1]) + " " + yup::String (root.getNumChildren() + 1), path, layerColor)));
        });
    }

    PathDocument& document;
    Selection& selection;
    yup::ListBox listBox;
    ButtonRow buttons;
    yup::TextButton* addLayerButton = nullptr;
    yup::TextButton* duplicateButton = nullptr;
    yup::TextButton* deleteButton = nullptr;
    yup::TextButton* upButton = nullptr;
    yup::TextButton* downButton = nullptr;
    yup::TextButton* visibilityButton = nullptr;
    std::vector<Row> rows;
    bool updating = false;
};

//==============================================================================

/** A titled block of labelled editor rows. Rows can be hidden, the height follows. */
class Section : public yup::Component
{
public:
    Section (const yup::String& title, PathDocument& documentToEdit)
        : yup::Component (title)
        , document (documentToEdit)
    {
        setOpaque (false);

        titleLabel.setText (title, yup::dontSendNotification);
        addAndMakeVisible (titleLabel);
    }

    float getPreferredHeight() const
    {
        float height = titleHeight + gap;
        for (const auto& row : rows)
        {
            if (row.visible)
                height += row.height + gap;
        }

        return height;
    }

    void resized() override
    {
        auto area = getLocalBounds();
        titleLabel.setBounds (area.removeFromTop (titleHeight).reduced (6.0f, 0.0f));
        area.removeFromTop (gap);
        area = area.reduced (6.0f, 0.0f);

        for (auto& row : rows)
        {
            row.label->setVisible (row.visible);
            row.editor->setVisible (row.visible);

            if (! row.visible)
                continue;

            auto rowArea = area.removeFromTop (row.height);
            area.removeFromTop (gap);

            row.label->setBounds (rowArea.removeFromLeft (labelWidth));
            row.editor->setBounds (rowArea);
        }
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (getPaletteColor (yup::ThemePalette::Role::surfaceRaised));
        g.fillRect (getLocalBounds().removeFromTop (titleHeight));
    }

protected:
    static constexpr float titleHeight = 24.0f;
    static constexpr float labelWidth = 64.0f;
    static constexpr float gap = 4.0f;

    void addRow (const yup::String& labelText, yup::Component& editor, float height = 24.0f)
    {
        auto& row = rows.emplace_back();
        row.label = std::make_unique<yup::Label>();
        row.label->setText (labelText, yup::dontSendNotification);
        row.editor = &editor;
        row.height = height;

        addAndMakeVisible (*row.label);
        addAndMakeVisible (editor);
    }

    void setRowVisible (yup::Component& editor, bool shouldBeVisible)
    {
        for (auto& row : rows)
        {
            if (row.editor == &editor && row.visible != shouldBeVisible)
            {
                row.visible = shouldBeVisible;
                resized();
            }
        }
    }

    /** Sliders: a drag is one undo step, a text entry or a double-click reset is another. */
    void bindSlider (yup::Slider& slider, const yup::String& transactionName, std::function<void (double)> write)
    {
        slider.onDragStart = [this, transactionName] (const yup::MouseEvent&)
        {
            if (! document.isGestureActive())
                document.beginGesture (transactionName);
        };

        slider.onDragEnd = [this] (const yup::MouseEvent&)
        {
            document.endGesture();
        };

        slider.onValueChanged = [this, transactionName, write = std::move (write)] (double value)
        {
            if (updating)
                return;

            document.change (transactionName, [&]
            {
                write (value);
            });
        };
    }

    void bindComboBox (yup::ComboBox& comboBox, const yup::String& transactionName, std::function<void (int)> write)
    {
        comboBox.onSelectedItemChanged = [this, &comboBox, transactionName, write = std::move (write)]
        {
            focusEditorRoot (*this);

            if (updating)
                return;

            document.edit (transactionName, [&]
            {
                write (comboBox.getSelectedId());
            });
        };
    }

    /** ToggleButton fires onClick twice per click, so only write when the state really differs. */
    void bindToggle (yup::ToggleButton& toggle, std::function<bool()> read, std::function<void (bool)> write)
    {
        toggle.onClick = [this, &toggle, read = std::move (read), write = std::move (write)]
        {
            if (updating || toggle.getToggleState() == read())
                return;

            write (toggle.getToggleState());
        };
    }

    static void setupSlider (yup::Slider& slider, double minimum, double maximum, double interval, double defaultValue)
    {
        slider.setRange (minimum, maximum, interval);
        slider.setDefaultValue (defaultValue);
        slider.setTextBoxStyle (yup::Slider::TextBoxRight, false, 56, 20);
        slider.setNumDecimalPlacesToDisplay (interval < 0.1 ? 2 : 1);
    }

    template <std::size_t N>
    static void fillComboBox (yup::ComboBox& comboBox, const char* const (&names)[N])
    {
        for (std::size_t i = 0; i < N; ++i)
            comboBox.addItem (names[i], static_cast<int> (i) + 1);
    }

    PathDocument& document;
    bool updating = false;

private:
    struct Row
    {
        std::unique_ptr<yup::Label> label;
        yup::Component* editor = nullptr;
        float height = 24.0f;
        bool visible = true;
    };

    yup::Label titleLabel;
    std::vector<Row> rows;
};

//==============================================================================

class LayerSection : public Section
{
public:
    explicit LayerSection (PathDocument& documentToEdit)
        : Section ("Layer", documentToEdit)
    {
        nameEditor.onTextChange = [this]
        {
            if (updating)
                return;

            document.edit ("Rename layer", [&]
            {
                document.setProperty (layer, Ids::name, nameEditor.getText());
            });
        };
        addRow ("Name", nameEditor);

        visibleToggle.setButtonText ("Visible");
        bindToggle (
            visibleToggle, [this]
            {
                return getBool (layer, Ids::visible);
            },
            [this] (bool visible)
            {
                document.edit ("Toggle visibility", [&]
                {
                    document.setProperty (layer, Ids::visible, visible);
                });
            });
        addRow ("", visibleToggle);

        setupSlider (opacitySlider, 0.0, 1.0, 0.01, 1.0);
        bindSlider (opacitySlider, "Layer opacity", [this] (double value)
        {
            document.setProperty (layer, Ids::opacity, value);
        });
        addRow ("Opacity", opacitySlider);

        fillComboBox (blendCombo, blendModeNames);
        bindComboBox (blendCombo, "Blend mode", [this] (int id)
        {
            document.setProperty (layer, Ids::blendMode, blendModeNames[id - 1]);
        });
        addRow ("Blend", blendCombo);

        setupSlider (featherSlider, 0.0, 100.0, 0.5, 0.0);
        bindSlider (featherSlider, "Layer feather", [this] (double value)
        {
            document.setProperty (layer, Ids::feather, value);
        });
        addRow ("Feather", featherSlider);

        fillComboBox (fillRuleCombo, fillRuleNames);
        bindComboBox (fillRuleCombo, "Fill rule", [this] (int id)
        {
            document.setProperty (layer, Ids::fillRule, fillRuleNames[id - 1]);
        });
        addRow ("Fill rule", fillRuleCombo);
    }

    void setLayer (const yup::DataTree& newLayer)
    {
        layer = newLayer;

        const yup::ScopedValueSetter<bool> updateScope (updating, true);

        if (! nameEditor.hasKeyboardFocus())
            nameEditor.setText (getString (layer, Ids::name), yup::dontSendNotification);

        visibleToggle.setToggleState (getBool (layer, Ids::visible), yup::dontSendNotification);
        opacitySlider.setValue (getFloat (layer, Ids::opacity), yup::dontSendNotification);
        blendCombo.setSelectedId (indexOfName (blendModeNames, getString (layer, Ids::blendMode)) + 1, yup::dontSendNotification);
        featherSlider.setValue (getFloat (layer, Ids::feather), yup::dontSendNotification);
        fillRuleCombo.setSelectedId (indexOfName (fillRuleNames, getString (layer, Ids::fillRule)) + 1, yup::dontSendNotification);
    }

private:
    yup::DataTree layer;
    yup::TextEditor nameEditor;
    yup::ToggleButton visibleToggle;
    yup::Slider opacitySlider { yup::Slider::LinearHorizontal };
    yup::ComboBox blendCombo;
    yup::Slider featherSlider { yup::Slider::LinearHorizontal };
    yup::ComboBox fillRuleCombo;
};

//==============================================================================

/** Edits the Fill or the Stroke of a layer: kind, solid color or gradient stops, and the stroke style. */
class PaintSection : public Section
{
public:
    PaintSection (PathDocument& documentToEdit, bool isStrokeSection)
        : Section (isStrokeSection ? "Stroke" : "Fill", documentToEdit)
        , isStroke (isStrokeSection)
        , stopsBar (documentToEdit)
    {
        fillComboBox (kindCombo, paintKindNames);
        bindComboBox (kindCombo, isStroke ? "Stroke type" : "Fill type", [this] (int id)
        {
            document.setPaintKind (paintNode, paintKindNames[id - 1], buildPath (layer).getBounds());
        });
        addRow ("Type", kindCombo);

        stopsBar.onStopSelected = [this]
        {
            refreshColorField();
        };
        addRow ("Stops", stopsBar, 36.0f);

        colorField.onChange = [this] (yup::Color newColor)
        {
            auto target = isGradientKind (getString (paintNode, Ids::kind)) ? stopsBar.getSelectedStop() : paintNode;
            if (! target.isValid())
                return;

            document.change ("Change color", [&]
            {
                document.setProperty (target, Ids::color, newColor.toString());
            });
        };
        colorField.onGestureStart = [this]
        {
            if (! document.isGestureActive())
                document.beginGesture ("Change color");
        };
        colorField.onGestureEnd = [this]
        {
            document.endGesture();
        };
        addRow ("Color", colorField, ColorField::preferredHeight);

        if (! isStroke)
            return;

        setupSlider (widthSlider, 0.0, 100.0, 0.5, 2.0);
        bindSlider (widthSlider, "Stroke width", [this] (double value)
        {
            document.setProperty (paintNode, Ids::width, value);
        });
        addRow ("Width", widthSlider);

        fillComboBox (joinCombo, strokeJoinNames);
        bindComboBox (joinCombo, "Stroke join", [this] (int id)
        {
            document.setProperty (paintNode, Ids::join, strokeJoinNames[id - 1]);
        });
        addRow ("Join", joinCombo);

        fillComboBox (capCombo, strokeCapNames);
        bindComboBox (capCombo, "Stroke cap", [this] (int id)
        {
            document.setProperty (paintNode, Ids::cap, strokeCapNames[id - 1]);
        });
        addRow ("Cap", capCombo);
    }

    void setLayer (const yup::DataTree& newLayer)
    {
        layer = newLayer;
        paintNode = isStroke ? getStroke (layer) : getFill (layer);

        const yup::ScopedValueSetter<bool> updateScope (updating, true);

        const auto kind = getString (paintNode, Ids::kind);
        const bool visible = kind != "none";
        const bool gradient = isGradientKind (kind);

        kindCombo.setSelectedId (indexOfName (paintKindNames, kind) + 1, yup::dontSendNotification);

        setRowVisible (stopsBar, gradient);
        setRowVisible (colorField, visible);

        if (gradient)
            stopsBar.setPaint (paintNode);

        refreshColorField();

        if (! isStroke)
            return;

        setRowVisible (widthSlider, visible);
        setRowVisible (joinCombo, visible);
        setRowVisible (capCombo, visible);

        widthSlider.setValue (getFloat (paintNode, Ids::width), yup::dontSendNotification);
        joinCombo.setSelectedId (indexOfName (strokeJoinNames, getString (paintNode, Ids::join)) + 1, yup::dontSendNotification);
        capCombo.setSelectedId (indexOfName (strokeCapNames, getString (paintNode, Ids::cap)) + 1, yup::dontSendNotification);
    }

private:
    void refreshColorField()
    {
        const auto source = isGradientKind (getString (paintNode, Ids::kind)) ? stopsBar.getSelectedStop() : paintNode;
        if (source.isValid())
            colorField.setColor (PathEditor::getColor (source, Ids::color));
    }

    const bool isStroke;
    yup::DataTree layer;
    yup::DataTree paintNode;
    yup::ComboBox kindCombo;
    GradientStopsBar stopsBar;
    ColorField colorField;
    yup::Slider widthSlider { yup::Slider::LinearHorizontal };
    yup::ComboBox joinCombo;
    yup::ComboBox capCombo;
};

//==============================================================================

class VertexSection : public Section
{
public:
    VertexSection (PathDocument& documentToEdit, Selection& selectionToEdit)
        : Section ("Vertex", documentToEdit)
        , selection (selectionToEdit)
    {
        setupSlider (xSlider, -1000.0, 4000.0, 0.5, 0.0);
        bindSlider (xSlider, "Move vertex", [this] (double value)
        {
            document.setProperty (selection.vertex, Ids::x, value);
        });
        addRow ("X", xSlider);

        setupSlider (ySlider, -1000.0, 4000.0, 0.5, 0.0);
        bindSlider (ySlider, "Move vertex", [this] (double value)
        {
            document.setProperty (selection.vertex, Ids::y, value);
        });
        addRow ("Y", ySlider);

        smoothToggle.setButtonText ("Smooth");
        bindToggle (
            smoothToggle, [this]
            {
                return getBool (selection.vertex, Ids::smooth);
            },
            [this] (bool)
            {
                document.edit ("Toggle smooth", [&]
                {
                    document.toggleSmooth (selection.vertex);
                });
            });
        addRow ("", smoothToggle);

        closedToggle.setButtonText ("Closed contour");
        bindToggle (
            closedToggle, [this]
            {
                return getBool (selection.vertex.getParent(), Ids::closed);
            },
            [this] (bool closed)
            {
                document.edit (closed ? "Close contour" : "Open contour", [&]
                {
                    document.setProperty (selection.vertex.getParent(), Ids::closed, closed);
                });
            });
        addRow ("", closedToggle);

        buttons.addButton ("Insert vertex", [this]
        {
            if (auto newVertex = document.insertVertexAfter (selection.vertex); newVertex.isValid())
                setSelectedVertex (newVertex);
        });
        buttons.addButton ("Delete vertex", [this]
        {
            document.deleteVertex (selection.vertex);
            setSelectedVertex ({});
        });
        addRow ("", buttons, 26.0f);
    }

    std::function<void()> onSelectionChanged;

    void setVertex (const yup::DataTree& vertex)
    {
        const yup::ScopedValueSetter<bool> updateScope (updating, true);

        xSlider.setValue (getFloat (vertex, Ids::x), yup::dontSendNotification);
        ySlider.setValue (getFloat (vertex, Ids::y), yup::dontSendNotification);
        smoothToggle.setToggleState (getBool (vertex, Ids::smooth), yup::dontSendNotification);
        closedToggle.setToggleState (getBool (vertex.getParent(), Ids::closed), yup::dontSendNotification);
    }

private:
    void setSelectedVertex (const yup::DataTree& vertex)
    {
        selection.vertex = vertex;

        if (onSelectionChanged)
            onSelectionChanged();
    }

    Selection& selection;
    yup::Slider xSlider { yup::Slider::LinearHorizontal };
    yup::Slider ySlider { yup::Slider::LinearHorizontal };
    yup::ToggleButton smoothToggle;
    yup::ToggleButton closedToggle;
    ButtonRow buttons;
};

//==============================================================================

/** The right-hand panel: the layer list on top, the property sections of the selection below. */
class PathInspector : public yup::Component
{
public:
    PathInspector (PathDocument& documentToEdit, Selection& selectionToEdit)
        : yup::Component ("PathInspector")
        , selection (selectionToEdit)
        , layerList (documentToEdit, selectionToEdit)
        , layerSection (documentToEdit)
        , fillSection (documentToEdit, false)
        , strokeSection (documentToEdit, true)
        , vertexSection (documentToEdit, selectionToEdit)
    {
        layerList.onSelectionChanged = [this]
        {
            notifySelectionChanged();
        };
        addAndMakeVisible (layerList);

        vertexSection.onSelectionChanged = [this]
        {
            notifySelectionChanged();
        };

        for (auto* section : getSections())
            sectionsContent.addChildComponent (*section);

        sectionsContent.setOpaque (false);
        scrollPanel.setContent (sectionsContent);
        addAndMakeVisible (scrollPanel);
    }

    std::function<void()> onSelectionChanged;

    /** Updates every widget in place from the document and the selection. */
    void refresh()
    {
        layerList.refresh();

        const bool hasLayer = selection.layer.isValid();
        const bool hasVertex = selection.vertex.isValid();

        layerSection.setVisible (hasLayer);
        fillSection.setVisible (hasLayer);
        strokeSection.setVisible (hasLayer);
        vertexSection.setVisible (hasVertex);

        if (hasLayer)
        {
            layerSection.setLayer (selection.layer);
            fillSection.setLayer (selection.layer);
            strokeSection.setLayer (selection.layer);
        }

        if (hasVertex)
            vertexSection.setVertex (selection.vertex);

        layoutSections();
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (6.0f);
        layerList.setBounds (area.removeFromTop (std::floor (area.getHeight() * 0.35f)));
        area.removeFromTop (6.0f);
        scrollPanel.setBounds (area);
        layoutSections();
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (getPaletteColor (yup::ThemePalette::Role::background));
        g.fillAll();
        g.setFillColor (getPaletteColor (yup::ThemePalette::Role::outline));
        g.fillRect (getLocalBounds().removeFromLeft (1.0f));
    }

private:
    std::array<Section*, 4> getSections() { return { &layerSection, &fillSection, &strokeSection, &vertexSection }; }

    void layoutSections()
    {
        const float width = scrollPanel.getWidth() - ScrollPanel::scrollBarWidth;
        float y = 0.0f;

        for (auto* section : getSections())
        {
            if (! section->isVisible())
                continue;

            const float height = section->getPreferredHeight();
            section->setBounds (0.0f, y, width, height);
            y += height + 8.0f;
        }

        scrollPanel.setContentHeight (y);
    }

    void notifySelectionChanged()
    {
        if (onSelectionChanged)
            onSelectionChanged();
    }

    Selection& selection;
    LayerList layerList;
    ScrollPanel scrollPanel;
    yup::Component sectionsContent;
    LayerSection layerSection;
    PaintSection fillSection;
    PaintSection strokeSection;
    VertexSection vertexSection;
};

} // namespace PathEditor
