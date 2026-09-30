/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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
    Shows the standard widgets inside a panel that can be freely transformed.

    The three circles on the corners of the panel can be dragged: the affine transform that
    maps the panel corners onto them is applied to the panel, and the widgets keep painting
    and receiving input correctly through it.
*/
class WidgetsDemo : public yup::Component
{
public:
    WidgetsDemo()
    {
        addAndMakeVisible (panel);

        setupWidgets();

        for (auto* handle : { &topLeftHandle, &topRightHandle, &bottomLeftHandle })
        {
            handle->onDrag = [this]
            {
                updatePanelTransformFromHandles();
            };

            addAndMakeVisible (*handle);
        }

        resetTransformButton.onClick = [this]
        {
            setPanelTransform ({});
        };
        addAndMakeVisible (resetTransformButton);
    }

private:
    void setupWidgets()
    {
        // Text Button (uses componentID as text)
        textButton = std::make_unique<yup::TextButton> ("Text Button");
        textButton->onClick = [this]
        {
            updateStatus ("Text Button clicked!");
        };
        panel.addAndMakeVisible (textButton.get());

        // Toggle Button
        toggleButton = std::make_unique<yup::ToggleButton> ("toggleButton");
        toggleButton->setButtonText ("Toggle Button");
        toggleButton->onClick = [this]
        {
            updateStatus ("Toggle Button: " + yup::String (toggleButton->getToggleState() ? "ON" : "OFF"));
        };
        panel.addAndMakeVisible (toggleButton.get());

        // Switch Button
        switchButton = std::make_unique<yup::SwitchButton> ("switchButton");
        switchButton->onClick = [this]
        {
            updateStatus ("Switch Button: " + yup::String (switchButton->getToggleState() ? "ON" : "OFF"));
        };
        panel.addAndMakeVisible (switchButton.get());

        // Image Button whose clickable area follows the logo's opaque pixels
        imageButton = std::make_unique<ImageHitTestButton>();
        imageButton->onClick = [this]
        {
            updateStatus ("Image Button clicked on an opaque pixel!");
        };
        panel.addAndMakeVisible (imageButton.get());

        imageButtonLabel = std::make_unique<yup::Label> ("imageButtonLabel");
        imageButtonLabel->setText ("Image Button: only the logo's opaque pixels are clickable",
                                   yup::dontSendNotification);
        panel.addAndMakeVisible (imageButtonLabel.get());

        // Labels
        titleLabel = std::make_unique<yup::Label> ("titleLabel");
        titleLabel->setText ("YUP Widget Examples", yup::dontSendNotification);
        panel.addAndMakeVisible (titleLabel.get());

        statusLabel = std::make_unique<yup::Label> ("statusLabel");
        statusLabel->setText ("Click widgets to see status updates...", yup::dontSendNotification);
        panel.addAndMakeVisible (statusLabel.get());

        // ComboBox with custom callback
        comboBox = std::make_unique<CustomComboBox> ("comboBox", this);
        comboBox->addItem ("Option 1", 1);
        comboBox->addItem ("Option 2", 2);
        comboBox->addItem ("Option 3", 3);
        comboBox->setSelectedId (1);
        panel.addAndMakeVisible (comboBox.get());

        // Viewport with content
        /*
        viewport = std::make_unique<yup::Viewport> ("viewport");
        viewportContent = std::make_unique<yup::Component> ("viewportContent");
        viewportContent->setSize (yup::Size<float> (800.0f, 600.0f)); // Larger than viewport

        contentLabel = std::make_unique<yup::Label> ("contentLabel");
        contentLabel->setText ("This is scrollable content\nYou can scroll to see more text...", yup::dontSendNotification);
        contentLabel->setBounds (yup::Rectangle<float> (20.0f, 20.0f, 360.0f, 200.0f));
        viewportContent->addAndMakeVisible (contentLabel.get());

        viewport->setViewedComponent (viewportContent.release(), false);
        panel.addAndMakeVisible (viewport.get());
        */

        // Slider
        slider = std::make_unique<yup::Slider> (yup::Slider::Rotary, "slider");
        slider->setRange (yup::Range<double> (0.0, 100.0));
        slider->setValue (50.0);
        slider->onValueChanged = [this] (double value)
        {
            updateStatus ("Slider value: " + yup::String (value, 1));
        };
        panel.addAndMakeVisible (slider.get());

        // TextEditor
        textEditor = std::make_unique<yup::TextEditor> ("textEditor");
        textEditor->setText ("Type some text here...", yup::dontSendNotification);
        textEditor->setMultiLine (true);
        panel.addAndMakeVisible (textEditor.get());

        // Progress Bar (normal mode - linked to slider)
        progressBar = std::make_unique<yup::ProgressBar> ("progressBar");
        progressBar->setProgress (0.5, yup::dontSendNotification);
        progressBar->onProgressChanged = [this] (double value)
        {
            if (value >= 0.0)
                updateStatus ("Progress: " + yup::String (value * 100.0, 0) + "%");
        };
        panel.addAndMakeVisible (progressBar.get());

        // Progress Bar Label
        progressBarLabel = std::make_unique<yup::Label> ("progressBarLabel");
        progressBarLabel->setText ("Progress Bar (linked to slider):", yup::dontSendNotification);
        panel.addAndMakeVisible (progressBarLabel.get());

        // Indeterminate Progress Bar
        indeterminateProgressBar = std::make_unique<yup::ProgressBar> ("indeterminateProgressBar");
        indeterminateProgressBar->setProgress (-1.0, yup::dontSendNotification);
        panel.addAndMakeVisible (indeterminateProgressBar.get());

        // Indeterminate Progress Bar Label
        indeterminateLabel = std::make_unique<yup::Label> ("indeterminateLabel");
        indeterminateLabel->setText ("Indeterminate Progress Bar:", yup::dontSendNotification);
        panel.addAndMakeVisible (indeterminateLabel.get());

        setupTabs();

        // Update slider to control progress bar
        slider->onValueChanged = [this] (double value)
        {
            updateStatus ("Slider value: " + yup::String (value, 1));
            progressBar->setProgress (value / 100.0, yup::dontSendNotification);
        };
    }

    void setupTabs()
    {
        tabsLabel = std::make_unique<yup::Label> ("tabsLabel");
        tabsLabel->setText ("Tabs: drag to reorder, narrow the window to see the More menu", yup::dontSendNotification);
        panel.addAndMakeVisible (tabsLabel.get());

        // A segmented control of views, reorderable, that overflows into a menu
        viewTabs = std::make_unique<yup::TabBar> ("viewTabs");
        viewTabs->setReorderable (true);

        const std::pair<const char*, const char*> views[] = {
            { "Table", YUP_ICON_TABLE },
            { "Board", YUP_ICON_TABLE_COLUMNS },
            { "Chart", YUP_ICON_CHART_COLUMN },
            { "List", YUP_ICON_LIST },
            { "Timeline", YUP_ICON_TIMELINE },
            { "Calendar", YUP_ICON_CALENDAR },
            { "Gallery", YUP_ICON_IMAGE }
        };

        for (const auto& [name, glyph] : views)
            viewTabs->addTab (yup::String (name).toLowerCase(), name).setIconGlyph (glyph);

        viewTabs->onSelectionChanged = [this] (const yup::Identifier& tabId)
        {
            updateStatus ("View tab selected: " + tabId.toString());
        };

        viewTabs->onTabMoved = [this] (const yup::Identifier& tabId, int oldIndex, int newIndex)
        {
            updateStatus ("View tab " + tabId.toString() + " moved from " + yup::String (oldIndex) + " to " + yup::String (newIndex));
        };

        panel.addAndMakeVisible (viewTabs.get());

        // Browser-like tabs that can be closed and added
        documentTabs = std::make_unique<yup::TabBar> ("documentTabs");
        documentTabs->setVariant (yup::TabBar::Variant::underline);
        documentTabs->setOverflow (yup::TabBar::Overflow::scroll);
        documentTabs->setReorderable (true);

        for (int i = 0; i < 3; ++i)
            addDocumentTab();

        documentTabs->onSelectionChanged = [this] (const yup::Identifier& tabId)
        {
            if (tabId.isValid())
                updateStatus ("Document tab selected: " + tabId.toString());
        };

        documentTabs->onTabCloseRequested = [this] (const yup::Identifier& tabId)
        {
            documentTabs->removeTab (tabId);
            updateStatus ("Document tab closed: " + tabId.toString());
        };

        panel.addAndMakeVisible (documentTabs.get());

        addTabButton = std::make_unique<yup::TextButton> ("Add tab");
        addTabButton->onClick = [this]
        {
            documentTabs->setSelectedTab (addDocumentTab());
        };
        panel.addAndMakeVisible (addTabButton.get());
    }

    yup::Identifier addDocumentTab()
    {
        ++numDocumentsCreated;

        const auto tabId = yup::Identifier ("document" + yup::String (numDocumentsCreated));
        auto& tab = documentTabs->addTab (tabId, "Document " + yup::String (numDocumentsCreated));
        tab.setIconGlyph (YUP_ICON_FILE_LINES);
        tab.setClosable (true);

        return tabId;
    }

    void updateStatus (const yup::String& message)
    {
        statusLabel->setText (message, yup::dontSendNotification);
    }

    void resized() override
    {
        resetTransformButton.setBounds (getWidth() - 130.0f, 4.0f, 120.0f, 24.0f);

        panel.setBounds (getLocalBounds().reduced (panelMargin));
        layoutWidgets();

        setPanelTransform (panel.getTransform());
    }

    /** Applies a transform to the panel and moves the handles onto its corners. */
    void setPanelTransform (const yup::AffineTransform& transform)
    {
        panel.setTransform (transform);

        const auto toParent = transform.translated (panel.getPosition());
        const auto size = panel.getSize();

        topLeftHandle.setCenter (yup::Point<float> (0.0f, 0.0f).transformed (toParent));
        topRightHandle.setCenter (yup::Point<float> (size.getWidth(), 0.0f).transformed (toParent));
        bottomLeftHandle.setCenter (yup::Point<float> (0.0f, size.getHeight()).transformed (toParent));

        repaint();
    }

    /** Builds the transform that maps the panel corners onto the handles. */
    void updatePanelTransformFromHandles()
    {
        const auto origin = panel.getPosition();
        const auto topLeft = topLeftHandle.getCenter() - origin;
        const auto xAxis = (topRightHandle.getCenter() - topLeftHandle.getCenter()) / panel.getWidth();
        const auto yAxis = (bottomLeftHandle.getCenter() - topLeftHandle.getCenter()) / panel.getHeight();

        const yup::AffineTransform transform (xAxis.getX(), yAxis.getX(), topLeft.getX(),
                                              xAxis.getY(), yAxis.getY(), topLeft.getY());

        // A degenerate transform would collapse the panel and make it unreachable
        if (std::abs (transform.getDeterminant()) < 0.01f)
            return setPanelTransform (panel.getTransform());

        panel.setTransform (transform);
        repaint();
    }

    /** Lays the widgets out inside the panel bounds, so they fit whatever size the panel gets.

        Rows keep a fixed height and only the text editor and the slider/logo row absorb the
        space that is left, which keeps every widget inside the panel (and so inside the
        transformed shape the handles control).
    */
    void layoutWidgets()
    {
        auto area = panel.getLocalBounds().reduced (contentMargin);

        titleLabel->setBounds (area.removeFromTop (32.0f));
        statusLabel->setBounds (area.removeFromTop (rowHeight));
        area.removeFromTop (sectionSpacing);

        // Full width, so the view tabs only overflow into the More menu when the panel is narrow
        layoutTabs (area);
        area.removeFromTop (sectionSpacing);

        // Two columns side by side, stacked instead when the remaining area is taller than wide
        if (area.getWidth() >= area.getHeight())
        {
            layoutInputs (area.removeFromLeft ((area.getWidth() - sectionSpacing) * 0.5f));
            area.removeFromLeft (sectionSpacing);
        }
        else
        {
            layoutInputs (area.removeFromTop ((area.getHeight() - sectionSpacing) * 0.5f));
            area.removeFromTop (sectionSpacing);
        }

        layoutValues (area);
    }

    void layoutTabs (yup::Rectangle<float>& area)
    {
        constexpr auto addTabButtonWidth = 90.0f;

        tabsLabel->setBounds (area.removeFromTop (labelHeight));
        viewTabs->setBounds (area.removeFromTop (tabBarHeight));
        area.removeFromTop (spacing);

        auto documentRow = area.removeFromTop (tabBarHeight);
        addTabButton->setBounds (documentRow.removeFromRight (addTabButtonWidth).withSizeKeepingCenter (addTabButtonWidth, rowHeight));
        documentRow.removeFromRight (spacing);
        documentTabs->setBounds (documentRow);
    }

    void layoutInputs (yup::Rectangle<float> area)
    {
        constexpr auto switchWidth = 60.0f;

        auto buttonsRow = area.removeFromTop (rowHeight);
        switchButton->setBounds (buttonsRow.removeFromRight (switchWidth));
        buttonsRow.removeFromRight (spacing);
        textButton->setBounds (buttonsRow.removeFromLeft ((buttonsRow.getWidth() - spacing) * 0.5f));
        buttonsRow.removeFromLeft (spacing);
        toggleButton->setBounds (buttonsRow);
        area.removeFromTop (spacing);

        comboBox->setBounds (area.removeFromTop (rowHeight));
        area.removeFromTop (spacing);

        textEditor->setBounds (area);
    }

    void layoutValues (yup::Rectangle<float> area)
    {
        indeterminateProgressBar->setBounds (area.removeFromBottom (rowHeight));
        indeterminateLabel->setBounds (area.removeFromBottom (labelHeight));
        area.removeFromBottom (spacing);

        progressBar->setBounds (area.removeFromBottom (rowHeight));
        progressBarLabel->setBounds (area.removeFromBottom (labelHeight));
        area.removeFromBottom (spacing);

        imageButtonLabel->setBounds (area.removeFromBottom (labelHeight));

        // The rotary slider and the logo are square: take the largest pair of squares that fits
        const auto squareSize = yup::jmax (0.0f, yup::jmin (area.getHeight(), (area.getWidth() - spacing) * 0.5f));
        auto squaresRow = area.withSizeKeepingCenter (squareSize * 2.0f + spacing, squareSize);

        slider->setBounds (squaresRow.removeFromLeft (squareSize));
        imageButton->setBounds (squaresRow.removeFromRight (squareSize));
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (yup::ThemePalette::Role::background));
        g.fillAll();
    }

    //==============================================================================
    /** A button whose clickable area is the logo's opaque pixels rather than its bounds.

        hitTest() is what decides which component a mouse event reaches, so sampling the
        image's alpha there makes the transparent parts of the PNG fall through to whatever
        is behind the button. The hover highlight goes through the same test, so dragging
        the pointer across a transparent region inside the button's bounds drops it - which
        is the easiest way to see the irregular hit area.
    */
    class ImageHitTestButton final : public yup::Button
    {
    public:
        ImageHitTestButton()
            : yup::Button ("imageButton")
        {
            logo = loadLogo();
        }

        bool hitTest (float x, float y) override
        {
            if (! logo.isValid())
                return yup::Button::hitTest (x, y);

            const auto pixel = componentToImage ({ x, y });
            const auto pixelX = static_cast<int> (pixel.getX());
            const auto pixelY = static_cast<int> (pixel.getY());

            // Letterboxing leaves bands of the component the image does not cover, and
            // getPixelColor() throws rather than clamping, so the bounds test has to come first.
            if (! yup::isPositiveAndBelow (pixelX, logo.getWidth())
                || ! yup::isPositiveAndBelow (pixelY, logo.getHeight()))
                return false;

            return logo.getPixelColor (pixelX, pixelY).getAlpha() >= minimumHitAlpha;
        }

        void paintButton (yup::Graphics& g) override
        {
            const auto imageArea = getImageArea();

            if (isButtonOver())
            {
                g.setFillColor (yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (yup::ThemePalette::Role::text).withAlpha (isButtonDown() ? 0.35f : 0.15f));
                g.fillRoundedRect (imageArea, 8.0f);
            }

            if (logo.isValid())
                g.drawImage (logo, imageArea);
        }

    private:
        // The logo is anti-aliased, so a bare test for full transparency would leave a fringe
        // of barely visible pixels clickable.
        static constexpr yup::uint8 minimumHitAlpha = 128;

        yup::Rectangle<float> getImageArea() const
        {
            if (! logo.isValid())
                return getLocalBounds();

            const auto bounds = getLocalBounds();
            const auto scale = yup::jmin (bounds.getWidth() / static_cast<float> (logo.getWidth()),
                                          bounds.getHeight() / static_cast<float> (logo.getHeight()));

            return bounds.withSizeKeepingCenter (logo.getWidth() * scale, logo.getHeight() * scale);
        }

        yup::Point<float> componentToImage (const yup::Point<float>& localPoint) const
        {
            const auto imageArea = getImageArea();
            const auto scale = imageArea.getWidth() / static_cast<float> (logo.getWidth());

            return (localPoint - imageArea.getPosition()) / scale;
        }

        static yup::Image loadLogo()
        {
            const auto file = getAssetPath ("data/logo.png");

            if (! file.existsAsFile())
                return {};

            yup::ImageFormatManager formatManager;
            formatManager.registerDefaultFormats();

            auto reader = formatManager.createReaderFor (file);

            return reader != nullptr ? reader->readImage() : yup::Image();
        }

        yup::Image logo;
    };

    //==============================================================================
    /** The panel holding the widgets, painted so its transformed shape is visible. */
    class WidgetsPanel final : public yup::Component
    {
    public:
        WidgetsPanel()
            : yup::Component ("widgetsPanel")
        {
        }

        void paint (yup::Graphics& g) override
        {
            g.setFillColor (yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (yup::ThemePalette::Role::surface));
            g.fillAll();
        }

        void paintOverChildren (yup::Graphics& g) override
        {
            // Painted in local coordinates: the panel transform maps it onto the handles
            g.setStrokeColor (yup::Colors::orange.withAlpha (0.8f));
            g.setStrokeWidth (1.5f);
            g.strokeRect (getLocalBounds().reduced (0.75f));
        }
    };

    //==============================================================================
    /** A circle that can be dragged around its parent. */
    class CornerHandle final : public yup::Component
    {
    public:
        CornerHandle()
        {
            setSize (handleSize, handleSize);
            setMouseCursor (yup::MouseCursor::Hand);
            setOpaque (false);
        }

        void paint (yup::Graphics& g) override
        {
            const auto circle = getLocalBounds().reduced (1.0f);

            g.setFillColor (isDragging ? yup::Colors::orange : yup::Colors::white);
            g.fillEllipse (circle);

            g.setStrokeColor (yup::Colors::orange);
            g.setStrokeWidth (2.0f);
            g.strokeEllipse (circle);
        }

        void mouseDown (const yup::MouseEvent& event) override
        {
            isDragging = true;
            dragOffset = event.getPosition();
            repaint();
        }

        void mouseDrag (const yup::MouseEvent& event) override
        {
            setTopLeft (getTopLeft() + event.getPosition() - dragOffset);

            if (onDrag)
                onDrag();
        }

        void mouseUp (const yup::MouseEvent&) override
        {
            isDragging = false;
            repaint();
        }

        std::function<void()> onDrag;

    private:
        static constexpr float handleSize = 16.0f;

        yup::Point<float> dragOffset;
        bool isDragging = false;
    };

    //==============================================================================
    // Custom ComboBox to handle selection changes
    class CustomComboBox : public yup::ComboBox
    {
    public:
        CustomComboBox (const yup::String& componentID, WidgetsDemo* parent)
            : yup::ComboBox (componentID)
            , parentWidget (parent)
        {
        }

        void selectedItemChanged() override
        {
            if (parentWidget)
                parentWidget->updateStatus ("ComboBox selected: " + getItemText (getSelectedItemIndex()));
        }

    private:
        WidgetsDemo* parentWidget;
    };

private:
    static constexpr float panelMargin = 30.0f;
    static constexpr float contentMargin = 20.0f;
    static constexpr float spacing = 10.0f;
    static constexpr float sectionSpacing = 16.0f;
    static constexpr float rowHeight = 30.0f;
    static constexpr float labelHeight = 24.0f;
    static constexpr float tabBarHeight = 36.0f;

    WidgetsPanel panel;
    CornerHandle topLeftHandle;
    CornerHandle topRightHandle;
    CornerHandle bottomLeftHandle;
    yup::TextButton resetTransformButton { "Reset Transform" };
    std::unique_ptr<yup::TextButton> textButton;
    std::unique_ptr<yup::ToggleButton> toggleButton;
    std::unique_ptr<yup::SwitchButton> switchButton;
    std::unique_ptr<ImageHitTestButton> imageButton;
    std::unique_ptr<yup::Label> imageButtonLabel;
    std::unique_ptr<yup::Label> titleLabel;
    std::unique_ptr<yup::Label> statusLabel;
    std::unique_ptr<CustomComboBox> comboBox;
    /*
    std::unique_ptr<yup::Viewport> viewport;
    std::unique_ptr<yup::Component> viewportContent;
    */
    std::unique_ptr<yup::Label> contentLabel;
    std::unique_ptr<yup::Slider> slider;
    std::unique_ptr<yup::TextEditor> textEditor;
    std::unique_ptr<yup::ProgressBar> progressBar;
    std::unique_ptr<yup::Label> progressBarLabel;
    std::unique_ptr<yup::ProgressBar> indeterminateProgressBar;
    std::unique_ptr<yup::Label> indeterminateLabel;
    std::unique_ptr<yup::Label> tabsLabel;
    std::unique_ptr<yup::TabBar> viewTabs;
    std::unique_ptr<yup::TabBar> documentTabs;
    std::unique_ptr<yup::TextButton> addTabButton;
    int numDocumentsCreated = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WidgetsDemo)
};
