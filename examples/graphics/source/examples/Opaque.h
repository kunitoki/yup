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

class OpaqueDemo : public yup::Component
{
public:
    OpaqueDemo()
    {
        auto theme = yup::ApplicationTheme::getGlobalTheme();
        exampleFont = theme->getDefaultFont();

        titleLabel = std::make_unique<yup::Label> ("titleLabel");
        titleLabel->setText ("Opaque Optimization Demo", yup::dontSendNotification);
        addAndMakeVisible (titleLabel.get());

        infoLabel = std::make_unique<yup::Label> ("infoLabel");
        infoLabel->setText ("Watch console for paint calls. The green background should be skipped when repainting the blue rectangle.", yup::dontSendNotification);
        addAndMakeVisible (infoLabel.get());

        demoRoot = std::make_unique<DemoRootComponent> ("demoRoot");
        addAndMakeVisible (demoRoot.get());

        repaintButton = std::make_unique<yup::TextButton> ("Repaint Bottom Half");
        repaintButton->onClick = [this]
        {
            demoRoot->triggerOpaqueRepaint();
        };
        addAndMakeVisible (repaintButton.get());

        counterLabel = std::make_unique<yup::Label> ("counterLabel");
        counterLabel->setText ("Repaint count: 0", yup::dontSendNotification);
        addAndMakeVisible (counterLabel.get());
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (yup::ThemePalette::Role::background));
        g.fillAll();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        float margin = 20.0f;
        float y = margin;

        titleLabel->setBounds (yup::Rectangle<float> (margin, y, bounds.getWidth() - 2 * margin, 30.0f));
        y += 40;

        infoLabel->setBounds (yup::Rectangle<float> (margin, y, bounds.getWidth() - 2 * margin, 40.0f));
        y += 50;

        auto demoHeight = bounds.getHeight() - y - 100;
        demoRoot->setBounds (yup::Rectangle<float> (margin, y, bounds.getWidth() - 2 * margin, demoHeight));
        y += demoHeight + 20;

        repaintButton->setBounds (yup::Rectangle<float> (margin, y, 200.0f, 30.0f));

        counterLabel->setBounds (yup::Rectangle<float> (margin + 220, y, 200.0f, 30.0f));
    }

    class DemoRootComponent : public yup::Component
    {
    public:
        DemoRootComponent (const yup::String& componentID)
            : yup::Component (componentID)
            , repaintCount (0)
        {
            transparentChild = std::make_unique<TransparentChildComponent> ("transparentChild");
            addAndMakeVisible (transparentChild.get());

            opaqueSibling = std::make_unique<OpaqueSiblingComponent> ("opaqueSibling");
            addAndMakeVisible (opaqueSibling.get());
        }

        void paint (yup::Graphics& g) override
        {
            g.setFillColor (yup::Color (255, 0, 200, 0)); // Opaque green
            g.fillAll();
        }

        void paintOverChildren (yup::Graphics& g) override
        {
            g.setStrokeColor (yup::Color (255, 255, 0, 0)); // Red
            g.setStrokeWidth (3.0f);

            auto bounds = getLocalBounds();
            g.strokeLine (bounds.getTopLeft(), bounds.getBottomRight());
        }

        void resized() override
        {
            auto bounds = getLocalBounds();

            auto childSize = 100.0f;
            transparentChild->setBounds (yup::Rectangle<float> (
                (bounds.getWidth() - childSize) / 2,
                (bounds.getHeight() - childSize) / 2,
                childSize,
                childSize));

            opaqueSibling->setBounds (yup::Rectangle<float> (
                0,
                bounds.getHeight() / 2,
                bounds.getWidth(),
                bounds.getHeight() / 2));
        }

        void triggerOpaqueRepaint()
        {
            repaintCount++;

            opaqueSibling->repaint();
        }

    private:
        class TransparentChildComponent : public yup::Component
        {
        public:
            TransparentChildComponent (const yup::String& componentID)
                : yup::Component (componentID)
            {
                setOpaque (false);
            }

            void paint (yup::Graphics& g) override
            {
                g.setFillColor (yup::Color (128, 255, 255, 0));
                g.fillEllipse (getLocalBounds());
            }

            void paintOverChildren (yup::Graphics& g) override
            {
                g.setStrokeColor (yup::Color (255, 0, 0, 255)); // Blue
                g.setStrokeWidth (2.0f);

                auto bounds = getLocalBounds();
                g.strokeLine (bounds.getTopRight(), bounds.getBottomLeft());
            }
        };

        class OpaqueSiblingComponent : public yup::Component
        {
        public:
            OpaqueSiblingComponent (const yup::String& componentID)
                : yup::Component (componentID)
            {
                setOpaque (true);
            }

            void paint (yup::Graphics& g) override
            {
                g.setFillColor (yup::Color (255, 0, 100, 255)); // Opaque blue
                g.fillAll();
            }
        };

        std::unique_ptr<TransparentChildComponent> transparentChild;
        std::unique_ptr<OpaqueSiblingComponent> opaqueSibling;
        int repaintCount;
    };

private:
    yup::Font exampleFont;
    std::unique_ptr<yup::Label> titleLabel;
    std::unique_ptr<yup::Label> infoLabel;
    std::unique_ptr<yup::Label> counterLabel;
    std::unique_ptr<yup::TextButton> repaintButton;
    std::unique_ptr<DemoRootComponent> demoRoot;
};
