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

/**
    Demonstrates drag and drop between components, across windows, and from the desktop.

    Two trays hold draggable tiles: a tray is a yup::DragAndDropTargetComponent and a tile is a
    yup::Component that is also a yup::DragAndDropSource, so a tile can be dragged from one tray
    onto the other. "New Window" opens a second top-level window running another copy of this demo,
    and because live tiles are registered in one process-wide list, a tile can be dragged from a
    tray in one window onto a tray in the other - which is the case that exercises the manager's
    cross-window component resolution, and that a single-window test cannot reach.

    The Inbox panel accepts files and text dragged in from outside the application and lists what
    arrived, so an OS-originated drag can be tried too. The logo in the header is the one item that
    drags *out*: its payload is the logo image, and it is the only source here asking for an external
    drag, so it can be dropped on another application - arriving there as a PNG.

    Tiles deliberately do not ask for that: dragging one between the two windows crosses out of the
    first window on the way, which the manager cannot tell apart from a drag leaving the
    application, so the export on a tile would break cross-window dragging.

    @see yup::DragAndDropSource, yup::DragAndDropTarget, yup::DragAndDropManager
*/
class DragAndDropDemo : public yup::Component
{
public:
    //==============================================================================
    /** A tile that can be dragged onto any tray, in any window.

        The payload is the tile's name rather than a pointer, so the tile can be found through the
        process-wide list of live tiles no matter which window the drop happens in.
    */
    class Tile final : public yup::Component
        , public yup::DragAndDropSource
    {
    public:
        Tile (const yup::String& newTileName, yup::Color newColor)
            : yup::Component (newTileName)
            , tileName (newTileName)
            , color (newColor)
        {
            getLiveTiles().add (this);
            setSize (72, 42);
        }

        ~Tile() override
        {
            getLiveTiles().removeFirstMatchingValue (this);
        }

        //==============================================================================
        /** Returns the name this tile is dragged under. */
        const yup::String& getTileName() const noexcept { return tileName; }

        /** Returns the live tile called @a name, or nullptr.

            A tile keeps existing when the window it started in goes away, and a drop can arrive
            from another window, so this has to be a whole-process lookup.
        */
        static Tile* findByName (const yup::String& name)
        {
            for (auto* tile : getLiveTiles())
                if (tile != nullptr && tile->getTileName() == name)
                    return tile;

            return nullptr;
        }

        //==============================================================================
        void paint (yup::Graphics& g) override
        {
            const auto bounds = getLocalBounds().to<float>().reduced (3.0f);

            g.setFillColor (color);
            g.fillRoundedRect (bounds, 6.0f);

            g.setStrokeColor (yup::Colors::black);
            g.setStrokeWidth (1.0f);
            g.strokeRoundedRect (bounds, 6.0f);
        }

        void mouseDrag (const yup::MouseEvent& event) override
        {
            if (isCurrentlyDragging())
                return;

            const auto delta = event.getPosition() - event.getLastMouseDownPosition();
            if (delta.getX() * delta.getX() + delta.getY() * delta.getY() < 64.0f)
                return;

            startDrag();
        }

        void dragOperationEnded (const yup::DragAndDropData&, yup::DragAndDropAction) override
        {
            // The manager has already taken the ghost back out of its window by the time this
            // arrives, so the replica can go.
            ghost.reset();
        }

    private:
        /** Creates the ghost replica, which must not join the list of live tiles. */
        struct GhostTag
        {
        };

        Tile (const yup::String& newTileName, yup::Color newColor, GhostTag)
            : yup::Component (newTileName)
            , tileName (newTileName)
            , color (newColor)
        {
            setSize (72, 42);
        }

        static yup::Array<Tile*>& getLiveTiles()
        {
            static yup::Array<Tile*> liveTiles;
            return liveTiles;
        }

        void startDrag()
        {
            ghost.reset (new Tile (tileName, color, GhostTag{}));

            startDragging (yup::DragAndDropSource::DragOptions{}
                               .withData (yup::DragAndDropData{}.withText (tileName))
                               .withDragImageComponent (ghost.get(), yup::Point<float> (36.0f, 21.0f))
                               .withImageOpacity (0.8f));
        }

        yup::String tileName;
        yup::Color color;
        std::unique_ptr<Tile> ghost;
    };

    //==============================================================================
    /** The application logo: the one thing in this demo meant to be dragged *out* of the app.

        It has to be a separate item, because a source cannot be both. An in-app drag that crosses
        between two windows leaves the first window on the way, which at the manager is
        indistinguishable from a drag leaving the application. So tiles stay in-app and carry text,
        while the logo carries an image and asks for the external drag.
    */
    class LogoItem final : public yup::Component
        , public yup::DragAndDropSource
    {
    public:
        LogoItem()
            : yup::Component ("logo")
        {
            logo = loadLogo();

            setSize (72, 72);
        }

        //==============================================================================
        void paint (yup::Graphics& g) override
        {
            if (! logo.isValid())
                return;

            const auto bounds = getLocalBounds();
            const auto scale = yup::jmin (bounds.getWidth() / static_cast<float> (logo.getWidth()),
                                          bounds.getHeight() / static_cast<float> (logo.getHeight()));
            const auto size = yup::Point<float> (logo.getWidth() * scale, logo.getHeight() * scale);
            const auto offset = (yup::Point<float> (bounds.getWidth(), bounds.getHeight()) - size) * 0.5f;

            g.drawImage (logo, yup::Rectangle<float> (offset.getX(), offset.getY(), size.getX(), size.getY()));
        }

        void mouseDrag (const yup::MouseEvent& event) override
        {
            if (isCurrentlyDragging() || ! logo.isValid())
                return;

            const auto delta = event.getPosition() - event.getLastMouseDownPosition();
            if (delta.getX() * delta.getX() + delta.getY() * delta.getY() < 64.0f)
                return;

            startDragging (yup::DragAndDropSource::DragOptions{}
                               .withData (yup::DragAndDropData{}.withImage (logo))
                               .withExternalDragAllowed (true));
        }

        void dragOperationEnded (const yup::DragAndDropData&, yup::DragAndDropAction) override
        {
        }

    private:
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
    /** A tray of tiles: a drop zone that takes a tile from any other tray, in any window. */
    class Tray final : public yup::DragAndDropTargetComponent
    {
    public:
        explicit Tray (const yup::String& trayName)
            : yup::DragAndDropTargetComponent (trayName)
        {
        }

        //==============================================================================
        bool isInterestedInDragSource (const yup::DragAndDropSourceDetails& details) override
        {
            return Tile::findByName (details.data.getText()) != nullptr;
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

            auto* tile = Tile::findByName (details.data.getText());
            auto* previousTray = (tile != nullptr) ? dynamic_cast<Tray*> (tile->getParentComponent()) : nullptr;

            if (tile == nullptr)
            {
                repaint();
                return false;
            }

            if (previousTray != this)
                addAndMakeVisible (tile); // moves it out of whichever tray, and window, held it

            resized();
            repaint();

            if (previousTray != nullptr && previousTray != this)
            {
                previousTray->resized();
                previousTray->repaint();
            }

            return true;
        }

        //==============================================================================
        void paint (yup::Graphics& g) override
        {
            const auto bounds = getLocalBounds().to<float>();

            g.setFillColor (highlighted ? yup::Color (0xff3f6d9e) : yup::Color (0xff2b2b33));
            g.fillRoundedRect (bounds, 8.0f);

            g.setStrokeColor (yup::Color (0xff55555f));
            g.setStrokeWidth (1.0f);
            g.strokeRoundedRect (bounds, 8.0f);
        }

        void resized() override
        {
            int column = 0;
            int row = 0;

            for (int i = 0; i < getNumChildComponents(); ++i)
            {
                auto* tile = dynamic_cast<Tile*> (getChildComponent (i));

                if (tile == nullptr)
                    continue;

                tile->setBounds (10 + (column * 88), 10 + (row * 56), 72, 42);

                if (++column == 4)
                {
                    column = 0;
                    ++row;
                }
            }
        }

    private:
        bool highlighted = false;
    };

    //==============================================================================
    /** Accepts files and text dragged in from outside the application. */
    class Inbox final : public yup::DragAndDropTargetComponent
    {
    public:
        Inbox()
            : yup::DragAndDropTargetComponent ("inbox")
        {
            contents = std::make_unique<yup::Label> ("inboxContents");
            contents->setText ("Drop files or text here from another application.");
            addAndMakeVisible (contents.get());
        }

        //==============================================================================
        bool isInterestedInDragSource (const yup::DragAndDropSourceDetails& details) override
        {
            return details.data.isEmpty()
                   || details.data.hasFiles()
                   || details.data.hasUris()
                   || details.data.hasText();
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

            yup::StringArray lines;

            for (const auto& file : details.data.getFiles())
                lines.add ("File: " + file.getFullPathName());

            for (const auto& uri : details.data.getUris())
                lines.add ("URI: " + uri);

            if (details.data.hasText())
                lines.add ("Text: " + details.data.getText());

            contents->setText (lines.isEmpty() ? yup::String ("Nothing arrived") : lines.joinIntoString ("\n"));

            return ! lines.isEmpty();
        }

        //==============================================================================
        void paint (yup::Graphics& g) override
        {
            const auto bounds = getLocalBounds().to<float>();

            g.setFillColor (highlighted ? yup::Color (0xff3f6d9e) : yup::Color (0xff23232a));
            g.fillRoundedRect (bounds, 8.0f);

            g.setStrokeColor (yup::Color (0xff55555f));
            g.setStrokeWidth (1.0f);
            g.strokeRoundedRect (bounds, 8.0f);
        }

        void resized() override
        {
            contents->setBounds (getLocalBounds().reduced (10));
        }

    private:
        std::unique_ptr<yup::Label> contents;
        bool highlighted = false;
    };

    //==============================================================================
    /** A second top-level window running another copy of the demo, so tiles can be dragged
        between two windows of the same process. */
    class SecondWindow final : public yup::Component
    {
    public:
        explicit SecondWindow (std::function<void()> closeCallback)
            : yup::Component ("Drag and drop - second window")
            , onClose (std::move (closeCallback))
        {
            content = std::make_unique<DragAndDropDemo>();

            closeButton = std::make_unique<yup::TextButton> ("Close");
            closeButton->onClick = [this]
            {
                // Deferred, so this window is not deleted from inside its own button callback.
                if (onClose)
                    yup::MessageManager::callAsync (onClose);
            };

            addAndMakeVisible (content.get());
            addAndMakeVisible (closeButton.get());

            addToDesktop (yup::ComponentNative::Options{}
                              .withDecoration (true)
                              .withResizableWindow (true));

            setSize (640, 480);
        }

        ~SecondWindow() override
        {
            removeFromDesktop();
        }

        void userTriedToCloseWindow() override
        {
            if (onClose)
                yup::MessageManager::callAsync (onClose);
        }

        void paint (yup::Graphics& g) override
        {
            // A window's root component is opaque, so it has to paint its own background.
            g.setFillColor (yup::Color (0xff1b1b20));
            g.fillAll();
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced (8);

            closeButton->setBounds (bounds.removeFromBottom (30).removeFromRight (110));

            bounds.removeFromBottom (8);
            content->setBounds (bounds);
        }

    private:
        std::function<void()> onClose;
        std::unique_ptr<DragAndDropDemo> content;
        std::unique_ptr<yup::TextButton> closeButton;
    };

    //==============================================================================
    /** A list whose rows can be dragged, with multiple selection enabled.

        Selecting several rows and dragging one of them carries the whole selection: the ListBox
        hands all of the selected rows to getDragSourceDescription(), and the string it returns is
        mirrored into the text MIME type, so the Inbox can show what was dropped.
    */
    class RowList final : public yup::Component
        , private yup::ListBoxModel
    {
    public:
        RowList()
        {
            rows = { "Kick", "Snare", "Hat", "Bass", "Lead", "Pad", "Arp", "Sub" };

            list = std::make_unique<yup::ListBox>();
            list->setModel (this);
            list->setSelectionMode (yup::ListBox::SelectionMode::multiple);
            addAndMakeVisible (list.get());
        }

        //==============================================================================
        int getNumRows() override { return rows.size(); }

        /** The text a row displays. The ListBox's built-in renderer takes a row's text and icon
            from here; paintListBoxItem() is not part of that path. */
        yup::String getRowText (int rowIndex) override
        {
            return rowIndex >= 0 && rowIndex < rows.size() ? rows[rowIndex] : yup::String();
        }

        yup::var getDragSourceDescription (const yup::Array<int>& selectedRows) override
        {
            if (selectedRows.isEmpty())
                return {};

            yup::StringArray names;

            for (auto rowIndex : selectedRows)
                if (rowIndex >= 0 && rowIndex < rows.size())
                    names.add (rows[rowIndex]);

            return names.joinIntoString (", ");
        }

        //==============================================================================
        void paint (yup::Graphics& g) override
        {
        }
        
        void resized() override
        {
            if (list != nullptr)
                list->setBounds (getLocalBounds());
        }

    private:
        yup::StringArray rows;
        std::unique_ptr<yup::ListBox> list;
    };

    //==============================================================================
    DragAndDropDemo()
        : yup::Component ("DragAndDropDemo")
    {
        hint = std::make_unique<yup::Label> ("hint");
        hint->setText ("Drag tiles between the trays, or the logo out of the window. Drop files or text onto the Inbox.");
        addAndMakeVisible (hint.get());

        newWindowButton = std::make_unique<yup::TextButton> ("New Window");
        newWindowButton->onClick = [this] { createSecondWindow(); };
        addAndMakeVisible (newWindowButton.get());

        logo = std::make_unique<LogoItem>();
        addAndMakeVisible (logo.get());

        leftTray = std::make_unique<Tray> ("leftTray");
        rightTray = std::make_unique<Tray> ("rightTray");
        inbox = std::make_unique<Inbox>();
        rowList = std::make_unique<RowList>();

        addAndMakeVisible (leftTray.get());
        addAndMakeVisible (rightTray.get());
        addAndMakeVisible (inbox.get());
        addAndMakeVisible (rowList.get());

        for (int i = 0; i < 6; ++i)
            (i < 4 ? leftTray : rightTray)->addAndMakeVisible (createTile (i));
    }

    //==============================================================================
    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::Color (0xff1b1b20));
        g.fillAll();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (12);

        hint->setBounds (bounds.removeFromTop (26));

        auto headerRow = bounds.removeFromTop (48);

        newWindowButton->setBounds (headerRow.removeFromRight (140).withHeight (28));
        logo->setBounds (headerRow.removeFromRight (48).reduced (4));

        bounds.removeFromTop (10);

        auto trayArea = bounds.removeFromTop (bounds.getHeight() * 3 / 5);
        const auto halfWidth = trayArea.getWidth() / 2;

        leftTray->setBounds (trayArea.removeFromLeft (halfWidth).reduced (6));
        rightTray->setBounds (trayArea.reduced (6));

        const auto bottomHalfWidth = bounds.getWidth() / 2;

        inbox->setBounds (bounds.removeFromLeft (bottomHalfWidth).reduced (6));
        rowList->setBounds (bounds.reduced (6));
    }

private:
    //==============================================================================
    Tile* createTile (int index)
    {
        static const yup::Color colours[] = {
            yup::Color (0xff4f9de8),
            yup::Color (0xffe8734f),
            yup::Color (0xff4fe89d),
            yup::Color (0xffe8d84f),
            yup::Color (0xffb94fe8),
            yup::Color (0xff4fe8e0)
        };

        static int tileCounter = 0;

        ++tileCounter;

        ownedTiles.push_back (std::make_unique<Tile> ("Tile " + yup::String (tileCounter), colours[index % 6]));

        return ownedTiles.back().get();
    }

    /** Owns the tiles. Declared last so that it is destroyed first, while the trays that parent
        them are still alive. */
    std::vector<std::unique_ptr<Tile>> ownedTiles;

    void createSecondWindow()
    {
        if (secondWindow != nullptr)
            return;

        secondWindow = std::make_unique<SecondWindow> ([this] { secondWindow.reset(); });

        if (auto* topLevel = getTopLevelComponent())
        {
            const auto screenBounds = topLevel->getScreenBounds();
            secondWindow->setBounds (screenBounds.getX() + 40, screenBounds.getY() + 40, 640, 480);
        }

        secondWindow->setVisible (true);
    }

    //==============================================================================
    std::unique_ptr<yup::Label> hint;
    std::unique_ptr<yup::TextButton> newWindowButton;
    std::unique_ptr<Tray> leftTray;
    std::unique_ptr<Tray> rightTray;
    std::unique_ptr<Inbox> inbox;
    std::unique_ptr<RowList> rowList;
    std::unique_ptr<LogoItem> logo;

    std::unique_ptr<SecondWindow> secondWindow;
};
