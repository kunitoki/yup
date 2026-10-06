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
/** Three lists side by side, driven by one options bar.

    - Text only: a thousand rows through the built-in ListBoxItem.
    - Text and icons: image icons, a header, pull-to-refresh and infinite loading.
    - Custom rows: cards with their own button, of varying heights, and every few rows a nested
      horizontal carousel. A vertical swipe on a carousel scrolls the outer list.

    With "Kinetic mouse drag" on, the mouse scrolls the lists the way a finger does, fling and
    overscroll included; with it off, a mouse drag starts dragging the selected rows instead.
*/
class ListBoxDemo : public yup::Component
{
public:
    ListBoxDemo()
        : Component ("ListBoxDemo")
    {
        setupOptionsBar();

        setupList (textList, textModel, textTitle, "Text only");

        setupList (iconList, iconModel, iconTitle, "Text and icons");
        iconList.setRowSize (44.0f);
        iconList.setHeaderComponent (std::make_unique<Banner> ("Pull down to refresh, scroll to the end to load more"));
        iconList.onRefresh = [this]
        {
            // Stands in for a network request.
            yup::Timer::callAfterDelay (1200, [this, safeThis = yup::WeakReference<yup::Component> (this)]
            {
                if (safeThis.get() == nullptr)
                    return;

                iconModel.prependContacts (3);
                iconList.rowsInserted (0, 3);
                iconList.setRefreshing (false);
            });
        };
        iconList.onEndReached = [this]
        {
            const auto startRow = iconModel.getNumRows();

            if (startRow >= 400)
                return;

            iconModel.appendContacts (25);
            iconList.rowsInserted (startRow, 25);
        };

        setupList (feedList, feedModel, feedTitle, "Custom rows");
        feedList.setRowSpacing (6.0f);
        feedList.setContentInsets (6.0f, 6.0f);

        addAndMakeVisible (status);
        status.setText ("Select, scroll, fling and pull the lists", yup::dontSendNotification);

        applyOptions();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (8.0f);

        auto bar = bounds.removeFromTop (32.0f);
        const auto cellWidth = bar.getWidth() / 4.0f;

        selectionMode.setBounds (bar.removeFromLeft (cellWidth).reduced (4.0f, 2.0f));
        mouseScrolling.setBounds (bar.removeFromLeft (cellWidth).reduced (4.0f, 2.0f));
        overscroll.setBounds (bar.removeFromLeft (cellWidth).reduced (4.0f, 2.0f));
        resistanceLabel.setBounds (bar.removeFromLeft (bar.getWidth() * 0.45f).reduced (4.0f, 2.0f));
        resistance.setBounds (bar.reduced (4.0f, 2.0f));

        status.setBounds (bounds.removeFromBottom (24.0f));
        bounds.removeFromBottom (4.0f);

        const bool sideBySide = bounds.getWidth() > bounds.getHeight();
        const auto panelSize = (sideBySide ? bounds.getWidth() : bounds.getHeight()) / 3.0f;

        for (auto [title, list] : { std::pair { &textTitle, &textList }, std::pair { &iconTitle, &iconList }, std::pair { &feedTitle, &feedList } })
        {
            auto panel = (sideBySide ? bounds.removeFromLeft (panelSize) : bounds.removeFromTop (panelSize)).reduced (4.0f);

            title->setBounds (panel.removeFromTop (24.0f));
            list->setBounds (panel);
        }
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (yup::ThemePalette::Role::background));
        g.fillAll();
    }

private:
    //==============================================================================
    struct Options
    {
        yup::ListBox::SelectionMode selectionMode = yup::ListBox::SelectionMode::single;
        bool mouseDragScrolling = true;
        bool overscroll = true;
        float resistance = 0.55f;

        void applyTo (yup::ListBox& list) const
        {
            list.setMouseDragScrollingEnabled (mouseDragScrolling);
            list.setScrollOptions (yup::KineticScroller::Options()
                                       .withOverscrollEnabled (overscroll)
                                       .withOverscrollResistance (resistance));
        }
    };

    static yup::Font getFont (float height)
    {
        return yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (height);
    }

    //==============================================================================
    /** A line of text that scrolls with a list, used as its header. */
    class Banner final : public yup::Component
    {
    public:
        explicit Banner (yup::String newText)
            : text (std::move (newText))
        {
            setSize (100.0f, 36.0f);
            setWantsMouseEvents (false, false);
        }

        void paint (yup::Graphics& g) override
        {
            const auto theme = yup::ApplicationTheme::getGlobalTheme();

            g.setFillColor (theme->getPalette().getColor (yup::ThemePalette::Role::textMuted));
            g.fillFittedText (text, theme->getDefaultFont(), getLocalBounds().reduced (8.0f, 0.0f), yup::Justification::centerLeft);
        }

    private:
        yup::String text;
    };

    //==============================================================================
    /** Text only: everything comes from getRowText(), the list builds the rows. */
    class TextModel final : public yup::ListBoxModel
    {
    public:
        int getNumRows() override { return 1000; }

        yup::String getRowText (int rowIndex) override { return "Item " + yup::String (rowIndex + 1); }
    };

    //==============================================================================
    /** Text and icons: still the built-in rows, now with getRowIcon(), and a list that grows at both ends. */
    class IconModel final : public yup::ListBoxModel
    {
    public:
        IconModel()
        {
            for (int i = 0; i < 12; ++i)
                icons.push_back (makeIcon (static_cast<float> (i) / 12.0f));

            appendContacts (40);
        }

        int getNumRows() override { return static_cast<int> (contactIds.size()); }

        yup::String getRowText (int rowIndex) override
        {
            static const char* const names[] = { "Ada", "Brian", "Grace", "Linus", "Margaret", "Dennis", "Barbara", "Ken" };
            const auto id = contactIds[static_cast<size_t> (rowIndex)];

            return yup::String (names[std::abs (id) % 8]) + " #" + yup::String (id);
        }

        yup::Image getRowIcon (int rowIndex) override
        {
            return icons[static_cast<size_t> (std::abs (contactIds[static_cast<size_t> (rowIndex)])) % icons.size()];
        }

        void appendContacts (int count)
        {
            for (int i = 0; i < count; ++i)
                contactIds.push_back (nextId++);
        }

        void prependContacts (int count)
        {
            for (int i = 0; i < count; ++i)
                contactIds.insert (contactIds.begin(), previousId--);
        }

    private:
        static yup::Image makeIcon (float hue)
        {
            constexpr int size = 32;
            yup::Image image (size, size, yup::PixelFormat::RGBA);
            image.clear();

            const auto color = yup::Color::fromHSL (hue, 0.6f, 0.55f);
            const auto center = (size - 1) * 0.5f;

            for (int y = 0; y < size; ++y)
            {
                for (int x = 0; x < size; ++x)
                {
                    const auto distance = std::hypot (static_cast<float> (x) - center, static_cast<float> (y) - center);
                    const auto coverage = yup::jlimit (0.0f, 1.0f, center + 0.5f - distance);

                    if (coverage > 0.0f)
                        image.setPixelColor (x, y, (distance < center * 0.45f ? color.brighter (0.4f) : color).withAlpha (coverage));
                }
            }

            return image;
        }

        std::vector<yup::Image> icons;
        std::vector<int> contactIds;
        int nextId = 1;
        int previousId = 0;
    };

    //==============================================================================
    /** A custom row: painted by itself, with a button. Its empty areas still scroll and select the list. */
    class PostCard final : public yup::Component
    {
    public:
        PostCard()
        {
            setWantsMouseEvents (false, true);

            likeButton.onClick = [this]
            {
                liked = ! liked;
                updateButton();

                if (onLikeChanged)
                    onLikeChanged (rowIndex, liked);
            };
            addAndMakeVisible (likeButton);
        }

        void setup (int newRowIndex, bool shouldBeSelected, bool isLiked)
        {
            rowIndex = newRowIndex;
            selected = shouldBeSelected;
            liked = isLiked;

            updateButton();
            repaint();
        }

        void resized() override
        {
            likeButton.setBounds (getLocalBounds().reduced (12.0f).removeFromRight (72.0f).withSizeKeepingCenter (72.0f, 28.0f));
        }

        void paint (yup::Graphics& g) override
        {
            const auto theme = yup::ApplicationTheme::getGlobalTheme();
            const auto& palette = theme->getPalette();
            auto bounds = getLocalBounds().reduced (6.0f, 0.0f);

            g.setFillColor (palette.getColor (selected ? yup::ThemePalette::Role::accent : yup::ThemePalette::Role::surfaceRaised));
            g.fillRoundedRect (bounds, 8.0f);

            auto text = bounds.reduced (12.0f, 8.0f).withTrimmedRight (84.0f);

            g.setFillColor (palette.getColor (selected ? yup::ThemePalette::Role::onAccent : yup::ThemePalette::Role::text));
            g.fillFittedText ("Post #" + yup::String (rowIndex + 1), getFont (15.0f), text.removeFromTop (text.getHeight() * 0.5f), yup::Justification::centerLeft);

            g.setFillColor (selected ? palette.getColor (yup::ThemePalette::Role::onAccent).withAlpha (0.8f) : palette.getColor (yup::ThemePalette::Role::textMuted));
            g.fillFittedText ("Custom row with its own button", theme->getDefaultFont(), text, yup::Justification::centerLeft);
        }

        std::function<void (int, bool)> onLikeChanged;

    private:
        void updateButton()
        {
            likeButton.setButtonText (liked ? "Liked" : "Like");
        }

        yup::TextButton likeButton;
        int rowIndex = -1;
        bool selected = false;
        bool liked = false;
    };

    //==============================================================================
    /** A card inside a carousel, with a button of its own. */
    class CarouselCard final : public yup::Component
    {
    public:
        CarouselCard()
        {
            setWantsMouseEvents (false, true);

            openButton.onClick = [this]
            {
                openButton.setButtonText ("Opened");
            };
            addAndMakeVisible (openButton);
        }

        void setup (int carouselIndex, int cardIndex)
        {
            title = "Card " + yup::String (cardIndex + 1);
            color = yup::Color::fromHSL (std::fmod (carouselIndex * 0.17f + cardIndex * 0.05f, 1.0f), 0.55f, 0.5f);
            openButton.setButtonText ("Open");
            repaint();
        }

        void resized() override
        {
            openButton.setBounds (getLocalBounds().reduced (10.0f).removeFromBottom (28.0f));
        }

        void paint (yup::Graphics& g) override
        {
            g.setFillColor (color);
            g.fillRoundedRect (getLocalBounds(), 10.0f);

            g.setFillColor (yup::Colors::white);
            g.fillFittedText (title, getFont (15.0f), getLocalBounds().reduced (10.0f).removeFromTop (24.0f), yup::Justification::centerLeft);
        }

    private:
        yup::TextButton openButton;
        yup::String title;
        yup::Color color;
    };

    //==============================================================================
    /** The cards of one carousel: variable widths along a horizontal list. */
    class CarouselModel final : public yup::ListBoxModel
    {
    public:
        int getNumRows() override { return 12; }

        float getRowSize (int rowIndex) override { return 100.0f + static_cast<float> ((rowIndex * 37 + carouselIndex * 11) % 80); }

        void refreshRowComponent (int rowIndex, bool, std::unique_ptr<yup::Component>& component) override
        {
            reuseOrCreate<CarouselCard> (component).setup (carouselIndex, rowIndex);
        }

        int carouselIndex = 0;
    };

    /** A row hosting a horizontal list: swiping sideways scrolls the cards, swiping along the outer
        list hands the gesture over to it. */
    class CarouselRow final : public yup::Component
    {
    public:
        CarouselRow()
        {
            setWantsMouseEvents (false, true);

            cards.setSelectionMode (yup::ListBox::SelectionMode::none);
            cards.setRowSpacing (8.0f);
            cards.setContentInsets (8.0f, 8.0f);
            cards.setHorizontalScrollBarVisibility (yup::ScrollBar::VisibilityMode::alwaysHidden);
            cards.setOpaque (false);
            cards.setColor (yup::ListBox::Style::backgroundColorId, yup::Colors::transparentBlack);
            cards.setColor (yup::ListBox::Style::outlineColorId, yup::Colors::transparentBlack);
            cards.setModel (&model);
            addAndMakeVisible (cards);
        }

        ~CarouselRow() override
        {
            cards.setModel (nullptr);
        }

        void setup (int carouselIndex, const Options& options)
        {
            options.applyTo (cards);

            if (model.carouselIndex == carouselIndex)
                return;

            model.carouselIndex = carouselIndex;
            cards.setScrollPosition (0.0f);
            cards.updateContent();
            repaint();
        }

        void resized() override
        {
            cards.setBounds (getLocalBounds().withTrimmedTop (24.0f));
        }

        void paint (yup::Graphics& g) override
        {
            g.setFillColor (yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (yup::ThemePalette::Role::textMuted));
            g.fillFittedText ("Carousel " + yup::String (model.carouselIndex + 1) + " - swipe sideways",
                              getFont (13.0f),
                              getLocalBounds().removeFromTop (24.0f).reduced (12.0f, 0.0f),
                              yup::Justification::centerLeft);
        }

    private:
        CarouselModel model;
        yup::ListBox cards { {}, yup::ListBox::Orientation::horizontal };
    };

    //==============================================================================
    /** Custom rows of two kinds and two heights, rebuilt from reuseOrCreate() as they are recycled. */
    class FeedModel final : public yup::ListBoxModel
    {
    public:
        explicit FeedModel (ListBoxDemo& owner)
            : owner (owner)
        {
        }

        int getNumRows() override { return 120; }

        float getRowSize (int rowIndex) override
        {
            if (isCarouselRow (rowIndex))
                return 150.0f;

            return rowIndex % 3 == 0 ? 84.0f : 64.0f;
        }

        void refreshRowComponent (int rowIndex, bool isSelected, std::unique_ptr<yup::Component>& component) override
        {
            if (isCarouselRow (rowIndex))
            {
                reuseOrCreate<CarouselRow> (component).setup (rowIndex / 6, owner.options);
                return;
            }

            auto& card = reuseOrCreate<PostCard> (component);
            card.setup (rowIndex, isSelected, liked[static_cast<size_t> (rowIndex)]);
            card.onLikeChanged = [this] (int row, bool isLiked)
            {
                liked[static_cast<size_t> (row)] = isLiked;
            };
        }

    private:
        static bool isCarouselRow (int rowIndex) { return rowIndex % 6 == 3; }

        ListBoxDemo& owner;
        std::vector<bool> liked = std::vector<bool> (120, false);
    };

    //==============================================================================
    void setupOptionsBar()
    {
        selectionMode.addItem ("No selection", 1);
        selectionMode.addItem ("Single selection", 2);
        selectionMode.addItem ("Multiple selection", 3);
        selectionMode.setSelectedId (2, yup::dontSendNotification);
        selectionMode.onSelectedItemChanged = [this] { applyOptions(); };
        addAndMakeVisible (selectionMode);

        mouseScrolling.setButtonText ("Kinetic mouse drag");
        mouseScrolling.setToggleState (true, yup::dontSendNotification);
        mouseScrolling.onClick = [this] { applyOptions(); };
        addAndMakeVisible (mouseScrolling);

        overscroll.setButtonText ("Overscroll");
        overscroll.setToggleState (true, yup::dontSendNotification);
        overscroll.onClick = [this] { applyOptions(); };
        addAndMakeVisible (overscroll);

        resistanceLabel.setText ("Rubber band", yup::dontSendNotification);
        addAndMakeVisible (resistanceLabel);

        resistance.setRange (0.1, 1.0);
        resistance.setValue (0.55, yup::dontSendNotification);
        resistance.onValueChanged = [this] (double) { applyOptions(); };
        addAndMakeVisible (resistance);
    }

    void setupList (yup::ListBox& list, yup::ListBoxModel& model, yup::Label& title, const yup::String& name)
    {
        title.setText (name, yup::dontSendNotification);
        addAndMakeVisible (title);

        list.setModel (&model);
        list.onScrollStateChanged = [this, &list, name] (yup::ListBox::ScrollState)
        {
            showStatus (name, list);
        };
        list.onSelectionChanged = [this, &list, name]
        {
            showStatus (name, list);
        };
        addAndMakeVisible (list);
    }

    void applyOptions()
    {
        static constexpr yup::ListBox::SelectionMode selectionModes[] = {
            yup::ListBox::SelectionMode::none,
            yup::ListBox::SelectionMode::single,
            yup::ListBox::SelectionMode::multiple
        };

        options.selectionMode = selectionModes[yup::jlimit (0, 2, selectionMode.getSelectedItemIndex())];
        options.mouseDragScrolling = mouseScrolling.getToggleState();
        options.overscroll = overscroll.getToggleState();
        options.resistance = static_cast<float> (resistance.getValue());

        for (auto* list : { &textList, &iconList, &feedList })
        {
            options.applyTo (*list);
            list->setSelectionMode (options.selectionMode);
        }

        // Pull-to-refresh is an overscroll gesture.
        iconList.setPullToRefreshEnabled (options.overscroll);

        // Refreshes the nested carousels, which read the options in refreshRowComponent().
        feedList.updateContent();
    }

    void showStatus (const yup::String& name, const yup::ListBox& list)
    {
        static const char* const states[] = { "idle", "dragging", "settling" };

        const auto selected = list.getSelectedRows();
        auto text = name + ": ";

        if (selected.isEmpty())
            text << "nothing selected";
        else if (selected.size() == 1)
            text << "row " << (selected.getFirst() + 1) << " selected";
        else
            text << selected.size() << " rows selected";

        text << ", scroll " << states[static_cast<int> (list.getScrollState())];

        status.setText (text, yup::dontSendNotification);
    }

    //==============================================================================
    Options options;

    yup::ComboBox selectionMode;
    yup::ToggleButton mouseScrolling;
    yup::ToggleButton overscroll;
    yup::Label resistanceLabel;
    yup::Slider resistance { yup::Slider::LinearHorizontal };
    yup::Label status;

    TextModel textModel;
    IconModel iconModel;
    FeedModel feedModel { *this };

    yup::Label textTitle;
    yup::Label iconTitle;
    yup::Label feedTitle;

    yup::ListBox textList;
    yup::ListBox iconList;
    yup::ListBox feedList;
};
