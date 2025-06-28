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

namespace yup
{

//==============================================================================
class OpaqueDemo : public Component
{
public:
    OpaqueDemo()
    {
        setOpaque (false);

        auto theme = ApplicationTheme::getGlobalTheme();
        exampleFont = theme->getDefaultFont();

        setupComponents();
    }

private:
    void setupComponents()
    {
        // Title label
        titleLabel = std::make_unique<Label> ("titleLabel");
        titleLabel->setText ("Opaque Optimization Demo", dontSendNotification);
        addAndMakeVisible (titleLabel.get());

        // Info label
        infoLabel = std::make_unique<Label> ("infoLabel");
        infoLabel->setText ("Watch console for paint calls. The green background should be skipped when repainting the blue rectangle.", dontSendNotification);
        addAndMakeVisible (infoLabel.get());

        // Demo root component
        demoRoot = std::make_unique<DemoRootComponent> ("demoRoot");
        addAndMakeVisible (demoRoot.get());

        // Button to trigger repaint
        repaintButton = std::make_unique<TextButton> ("Repaint Bottom Half");
        repaintButton->onClick = [this]
        {
            demoRoot->triggerOpaqueRepaint();
        };
        addAndMakeVisible (repaintButton.get());

        // Counter label
        counterLabel = std::make_unique<Label> ("counterLabel");
        counterLabel->setText ("Repaint count: 0", dontSendNotification);
        addAndMakeVisible (counterLabel.get());
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto margin = 20;
        int y = margin;

        // Title
        titleLabel->setBounds (Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (bounds.getWidth() - 2 * margin), 30.0f));
        y += 40;

        // Info
        infoLabel->setBounds (Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (bounds.getWidth() - 2 * margin), 40.0f));
        y += 50;

        // Demo area
        auto demoHeight = bounds.getHeight() - y - 100;
        demoRoot->setBounds (Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), static_cast<float> (bounds.getWidth() - 2 * margin), static_cast<float> (demoHeight)));
        y += demoHeight + 20;

        // Button
        repaintButton->setBounds (Rectangle<float> (static_cast<float> (margin), static_cast<float> (y), 200.0f, 30.0f));

        // Counter
        counterLabel->setBounds (Rectangle<float> (static_cast<float> (margin + 220), static_cast<float> (y), 200.0f, 30.0f));
    }

    // Root component that demonstrates the optimization
    class DemoRootComponent : public Component
    {
    public:
        DemoRootComponent (const String& componentID)
            : Component (componentID)
            , repaintCount (0)
        {
            // Create transparent child with alpha circle
            transparentChild = std::make_unique<TransparentChildComponent> ("transparentChild");
            addAndMakeVisible (transparentChild.get());

            // Create opaque sibling that covers bottom half
            opaqueSibling = std::make_unique<OpaqueSiblingComponent> ("opaqueSibling");
            addAndMakeVisible (opaqueSibling.get());
        }

        void paint (Graphics& g) override
        {
            Logger::outputDebugString ("DemoRootComponent::paint() called - drawing green background");

            // Draw green rectangle background
            g.setFillColor (Color (0, 200, 0, 255)); // Opaque green
            g.fillAll();
        }

        void paintOverChildren (Graphics& g) override
        {
            Logger::outputDebugString ("DemoRootComponent::paintOverChildren() called - drawing red diagonal");

            // Draw red diagonal line over children
            g.setStrokeColor (Color (255, 0, 0, 255)); // Red
            g.setStrokeWidth (3.0f);

            auto bounds = getLocalBounds();
            g.strokeLine (bounds.getTopLeft(), bounds.getBottomRight());
        }

        void resized() override
        {
            auto bounds = getLocalBounds();

            // Transparent child in the middle
            auto childSize = 100.0f;
            transparentChild->setBounds (Rectangle<float> (
                (bounds.getWidth() - childSize) / 2,
                (bounds.getHeight() - childSize) / 2,
                childSize,
                childSize));

            // Opaque sibling covers bottom half
            opaqueSibling->setBounds (Rectangle<float> (
                0,
                bounds.getHeight() / 2,
                bounds.getWidth(),
                bounds.getHeight() / 2));
        }

        void triggerOpaqueRepaint()
        {
            repaintCount++;
            Logger::outputDebugString ("=== TRIGGERING REPAINT #" + String (repaintCount) + " ===");

            // Repaint just the opaque sibling area - this should trigger the optimization
            opaqueSibling->repaint();

            Logger::outputDebugString ("=== END REPAINT #" + String (repaintCount) + " ===");
        }

    private:
        // Transparent child that draws alpha circle
        class TransparentChildComponent : public Component
        {
        public:
            TransparentChildComponent (const String& componentID)
                : Component (componentID)
            {
                setOpaque (false); // This child is transparent
            }

            void paint (Graphics& g) override
            {
                Logger::outputDebugString ("TransparentChildComponent::paint() called - drawing alpha circle");

                // Draw semi-transparent circle
                g.setFillColor (Color (255, 255, 0, 128)); // Semi-transparent yellow
                g.fillEllipse (getLocalBounds());
            }

            void paintOverChildren (Graphics& g) override
            {
                Logger::outputDebugString ("TransparentChildComponent::paintOverChildren() called - drawing blue diagonal");

                // Draw blue diagonal in opposite direction
                g.setStrokeColor (Color (0, 0, 255, 255)); // Blue
                g.setStrokeWidth (2.0f);

                auto bounds = getLocalBounds();
                g.strokeLine (bounds.getTopRight(), bounds.getBottomLeft());
            }
        };

        // Opaque sibling that covers bottom half
        class OpaqueSiblingComponent : public Component
        {
        public:
            OpaqueSiblingComponent (const String& componentID)
                : Component (componentID)
            {
                setOpaque (true); // This component is opaque - triggers optimization!
            }

            void paint (Graphics& g) override
            {
                Logger::outputDebugString ("OpaqueSiblingComponent::paint() called - drawing blue rectangle");

                // Draw opaque blue rectangle
                g.setFillColor (Color (0, 100, 255, 255)); // Opaque blue
                g.fillAll();

                // Add some text to show this is the opaque component
                //g.setFillColor (Colors::white);
                //g.drawText ("OPAQUE COMPONENT", getLocalBounds(), 16.0f, Justification::center);
            }
        };

        std::unique_ptr<TransparentChildComponent> transparentChild;
        std::unique_ptr<OpaqueSiblingComponent> opaqueSibling;
        int repaintCount;
    };

private:
    Font exampleFont;
    std::unique_ptr<Label> titleLabel;
    std::unique_ptr<Label> infoLabel;
    std::unique_ptr<Label> counterLabel;
    std::unique_ptr<TextButton> repaintButton;
    std::unique_ptr<DemoRootComponent> demoRoot;
};

} // namespace yup
