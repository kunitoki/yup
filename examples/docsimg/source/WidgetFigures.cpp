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

#include "Figures.h"

#include <yup_audio_gui/yup_audio_gui.h>

namespace
{

//==============================================================================
/** A row or a column owning the widgets it lays out. */
struct Widgets : yup::FlexBoxComponent
{
    explicit Widgets (yup::FlexBox::Direction direction = yup::FlexBox::Direction::row)
    {
        setOpaque (false);
        setDirection (direction);
        setGap (16.0f);
    }

    /** Adds a widget of a given size along the layout, or sharing the space left when it is zero. */
    template <class Widget, class... Args>
    Widget& add (float size, Args&&... args)
    {
        auto widget = std::make_unique<Widget> (std::forward<Args> (args)...);
        auto& result = *widget;

        if (size > 0.0f)
        {
            result.setProperty (yup::Identifier (getDirection() == yup::FlexBox::Direction::row ? "preferredWidth" : "preferredHeight"), size);
        }
        else
        {
            result.setProperty (yup::Identifier ("flexGrow"), 1.0);
            result.setProperty (yup::Identifier ("flexBasis"), 0.0);
        }

        addAndMakeVisible (result);

        widgets.push_back (std::move (widget));
        return result;
    }

    std::vector<std::unique_ptr<yup::Component>> widgets;
};

//==============================================================================
/** A column of widgets wearing the palette made from some colors, or the one of the theme. */
struct PalettePanel : Widgets
{
    PalettePanel (const char* title, const char* colors)
        : Widgets (yup::FlexBox::Direction::column)
    {
        const auto mode = yup::ApplicationTheme::getGlobalTheme()->getPalette().getMode();

        if (auto palette = yup::ThemePalette::fromString (colors, mode); palette.wasOk())
            setPalette (palette.getReference());

        setPadding (yup::EdgeSpacing<float> (12.0f));
        setGap (8.0f);

        add<yup::Label> (18.0f, "title").setText (title, yup::dontSendNotification);
        add<yup::TextButton> (0.0f, "save").setButtonText ("Save");
        add<yup::ToggleButton> (0.0f, "mute").setButtonText ("Mute");
        add<yup::Slider> (0.0f, yup::Slider::LinearHorizontal).setValue (0.6);
    }

    void paint (yup::Graphics& g) override
    {
        const auto& palette = yup::ApplicationTheme::getGlobalTheme()->getPalette (*this);
        const auto bounds = getLocalBounds().reduced (0.5f);

        g.setFillColor (palette.getColor (Role::background));
        g.fillRoundedRect (bounds, 8.0f);

        g.setStrokeColor (palette.getColor (Role::outline));
        g.strokeRoundedRect (bounds, 8.0f);
    }
};

//==============================================================================
/** A list box and the model it shows, which has to live as long as the list. */
struct FruitList : yup::Component
{
    struct Model : yup::ListBoxModel
    {
        int getNumRows() override { return fruits.size(); }

        yup::String getRowText (int row) override { return fruits[row]; }

        yup::StringArray fruits { "Apple", "Banana", "Cherry", "Grape", "Lemon", "Mango" };
    };

    FruitList()
    {
        setOpaque (false);

        list.setModel (&model);
        list.setRowSize (28.0f);
        list.selectRow (1, false, yup::dontSendNotification);
        addAndMakeVisible (list);
    }

    void resized() override
    {
        list.setBounds (getLocalBounds());
    }

    Model model;
    yup::ListBox list;
};

//==============================================================================

struct NoteItem : yup::TreeViewItem
{
    explicit NoteItem (yup::String textToShow)
        : text (std::move (textToShow))
    {
    }

    yup::String getItemText() const override { return text; }

    yup::String text;
};

//==============================================================================
/** A keyboard and the state it plays into, holding a chord. */
struct Keyboard : yup::Component
{
    Keyboard()
    {
        setOpaque (false);

        keyboard.setAvailableRange (48, 84);
        addAndMakeVisible (keyboard);

        for (const auto note : { 60, 64, 67 })
            state.noteOn (1, note, 0.8f);
    }

    void resized() override
    {
        keyboard.setBounds (getLocalBounds());
    }

    yup::MidiKeyboardState state;
    yup::MidiKeyboardComponent keyboard { state, yup::MidiKeyboardComponent::horizontalKeyboard };
};

//==============================================================================
/** Two panes split by a divider, laid out as the divider says. */
struct SplitView : yup::Component
{
    struct Pane : yup::Component
    {
        explicit Pane (const char* nameToShow)
            : name (nameToShow)
        {
            setOpaque (false);
        }

        void paint (yup::Graphics& g) override
        {
            g.setFillColor (role (Role::surface));
            g.fillRoundedRect (getLocalBounds(), 6.0f);

            g.setFillColor (role (Role::textMuted));
            g.fillFittedText (name, font (12.0f), getLocalBounds(), yup::Justification::center);
        }

        const char* name;
    };

    SplitView()
    {
        setOpaque (false);

        divider.setMinPosition (yup::ResizableDivider::Length::pixels (120.0f));
        divider.setPosition (yup::ResizableDivider::Length::pixels (180.0f), yup::dontSendNotification);

        addAndMakeVisible (sidebar);
        addAndMakeVisible (content);
        addAndMakeVisible (divider);
    }

    void resized() override
    {
        divider.layoutComponents (getLocalBounds(), &sidebar, &content);
    }

    Pane sidebar { "sidebar" }, content { "content" };
    yup::ResizableDivider divider;
};

} // namespace

//==============================================================================

void addWidgetFigures (Figures& f)
{
    using namespace yup;

    //==============================================================================
    // docs/ui/component-styling.md

    f.component ("ui/component-styling_colors", { 480.0f, 96.0f }, []() -> std::unique_ptr<Component>
    {
        auto row = std::make_unique<Widgets>();

        row->add<TextButton> (0.0f, "theme").setButtonText ("Theme colors");

        auto& custom = row->add<TextButton> (0.0f, "custom");
        custom.setButtonText ("setColor()");
        custom.setColor (TextButton::Style::backgroundColorId, Colors::darkslateblue);
        custom.setColor (TextButton::Style::textColorId, Colors::white);

        return row;
    });

    f.component ("ui/component-styling_component-palettes", { 560.0f, 188.0f }, []() -> std::unique_ptr<Component>
    {
        auto row = std::make_unique<Widgets>();

        row->add<PalettePanel> (0.0f, "Theme palette", "");
        row->add<PalettePanel> (0.0f, "Component palette", "https://coolors.co/0b132b-1c2541-3a506b-5bc0be-6fffe9");
        row->add<PalettePanel> (0.0f, "Component palette", "https://coolors.co/100b00-85cb33-efffc8-a5cbc3-3b341f");

        return row;
    });

    //==============================================================================
    // docs/ui/list-box.md, tree-view.md, tabs.md, midi-keyboard.md and resizable-divider.md

    f.component ("ui/list-box_rows", { 320.0f, 216.0f }, []
    {
        return std::make_unique<FruitList>();
    }).settleMs = 100;

    f.component ("ui/tree-view_tree", { 320.0f, 216.0f }, []() -> std::unique_ptr<Component>
    {
        auto root = std::make_unique<NoteItem> ("Notes");

        auto& ideas = root->addSubItem (std::make_unique<NoteItem> ("Ideas"));
        ideas.addSubItem (std::make_unique<NoteItem> ("A tree view"));
        ideas.addSubItem (std::make_unique<NoteItem> ("A list box"));

        auto& drafts = root->addSubItem (std::make_unique<NoteItem> ("Drafts"));
        drafts.addSubItem (std::make_unique<NoteItem> ("Release notes"));

        root->addSubItem (std::make_unique<NoteItem> ("Archive"));

        auto tree = std::make_unique<TreeView>();
        tree->setExpandAnimationTime (0.0);
        tree->setRootItem (std::move (root));
        tree->setRootItemVisible (false);

        ideas.setOpen (true);
        ideas.setSelected (true);

        return tree;
    }).settleMs = 100;

    // A pill bar as long as its tabs, one filling its width, and an underlined one
    f.component ("ui/tabs_variants", { 480.0f, 196.0f }, []() -> std::unique_ptr<Component>
    {
        auto column = std::make_unique<Widgets> (FlexBox::Direction::column);

        for (int index = 0; index < 3; ++index)
        {
            auto& bar = column->add<TabBar> (0.0f);
            bar.setAnimationDuration (0.0);
            bar.setVariant (index == 2 ? TabBar::Variant::underline : TabBar::Variant::pill);
            bar.setLayout (index == 0 ? TabBar::Layout::natural : TabBar::Layout::fill);

            for (const auto* name : { "Mix", "Effects", "Settings" })
                bar.addTab (name, name);

            bar.setSelectedTab ("Effects", dontSendNotification);
        }

        return column;
    }).settleMs = 100;

    f.component ("ui/midi-keyboard_keyboard", { 560.0f, 130.0f }, []
    {
        return std::make_unique<Keyboard>();
    });

    f.component ("ui/resizable-divider_split", { 480.0f, 160.0f }, []
    {
        return std::make_unique<SplitView>();
    });
}
