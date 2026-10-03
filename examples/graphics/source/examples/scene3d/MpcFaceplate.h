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

#include <yup_3d/yup_3d.h>

#include "NpcLogo.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

//==============================================================================
/** The printed faceplate of the MPC, painted with yup::Graphics.

    The model carries no images: the faces seen from the top use the "Faceplate" and "Knob Cap"
    materials, whose texture coordinates map the whole device straight from above. This
    component paints that texture once, then hands it to those materials.

    Buttons and knobs are found from the texture coordinates of their painted faces, so the
    labels, scales and caps follow the model. Drawing happens in design units, 1000 across the
    device, scaled to the size of the component.
*/
class MpcFaceplate final : public yup::Component
{
public:
    static constexpr float designWidth = 1000.0f;
    static constexpr float designHeight = 617.93f;

    /** Creates a faceplate rendered with the given number of pixels across the device. */
    explicit MpcFaceplate (float pixelWidth)
        : yup::Component ("mpcFaceplate")
    {
        setSize (pixelWidth, std::round (pixelWidth * designHeight / designWidth));
    }

    //==============================================================================
    /** Finds the parts of the device and the materials that show the faceplate. */
    void setDevice (const yup::EntityNode& device)
    {
        parts.clear();
        materials.clear();

        collectMaterials (device);

        for (const auto& child : device.getChildren())
        {
            collectMaterials (*child);

            auto* meshNode = child->getNode<yup::MeshNode>();
            if (meshNode == nullptr || meshNode->mesh == nullptr)
                continue;

            Part part { child->getName(), {}, yup::Colors::black };
            auto minUv = yup::Point<float> (1.0f, 1.0f);
            auto maxUv = yup::Point<float> (0.0f, 0.0f);

            for (const auto& primitive : meshNode->mesh->getPrimitives())
            {
                if (primitive.material == nullptr)
                    continue;

                if (! isPainted (*primitive.material))
                {
                    part.color = toColor (primitive.material->baseColorFactor);
                    continue;
                }

                for (const auto& vertex : primitive.vertices)
                {
                    minUv = { yup::jmin (minUv.getX(), vertex.uv.getX()), yup::jmin (minUv.getY(), vertex.uv.getY()) };
                    maxUv = { yup::jmax (maxUv.getX(), vertex.uv.getX()), yup::jmax (maxUv.getY(), vertex.uv.getY()) };
                }
            }

            if (maxUv.getX() > minUv.getX())
            {
                part.area = { minUv.getX() * designWidth,
                              minUv.getY() * designHeight,
                              (maxUv.getX() - minUv.getX()) * designWidth,
                              (maxUv.getY() - minUv.getY()) * designHeight };
            }

            parts.push_back (std::move (part));
        }

        repaint();
    }

    /** Paints the faceplate the first time it is called, and shows it on the device.

        Call it from paint(), where the context is ready to render offscreen.
    */
    void updateTexture (yup::GraphicsContext& context)
    {
        if (texture != nullptr || materials.empty())
            return;

        auto image = snapshotToImage (context);
        if (! image.isValid())
            return;

        // Mipmapped from the CPU image, so the small print doesn't shimmer when zoomed out
        yup::GpuSamplerDesc sampler (yup::GpuFilter::linear, yup::GpuWrapMode::clampToEdge);
        sampler.mipmapFilter = yup::GpuFilter::linear;
        sampler.maxAnisotropy = 16;

        texture = new yup::Texture (std::move (image), sampler, true);

        for (auto& material : materials)
            material->baseColorTexture = texture;
    }

    //==============================================================================
    void paint (yup::Graphics& g) override
    {
        g.addTransform (yup::AffineTransform::scaling (getWidth() / designWidth));

        paintBody (g);
        paintLogos (g);
        paintShadows (g);
        paintKnobScales (g);
        paintLabels (g);
        paintLeds (g);
        paintButtonCaps (g);
        paintKnobCaps (g);
    }

private:
    struct Part
    {
        yup::String name;
        yup::Rectangle<float> area;
        yup::Color color;
    };

    struct Label
    {
        const char* part;
        const char* text;
        const char* secondary;
    };

    //==============================================================================
    static bool isPainted (const yup::Material& material)
    {
        return material.name == "Faceplate" || material.name == "Knob Cap";
    }

    static yup::Color toColor (const std::array<float, 4>& linear)
    {
        const auto encode = [] (float c)
        {
            c = yup::jlimit (0.0f, 1.0f, c);
            const auto srgb = c <= 0.0031308f ? c * 12.92f : 1.055f * std::pow (c, 1.0f / 2.4f) - 0.055f;
            return static_cast<yup::uint8> (yup::roundToInt (srgb * 255.0f));
        };

        return { encode (linear[0]), encode (linear[1]), encode (linear[2]) };
    }

    void collectMaterials (const yup::EntityNode& entity)
    {
        auto* meshNode = entity.getNode<yup::MeshNode>();
        if (meshNode == nullptr || meshNode->mesh == nullptr)
            return;

        for (const auto& primitive : meshNode->mesh->getPrimitives())
        {
            if (primitive.material == nullptr || ! isPainted (*primitive.material))
                continue;

            if (std::find (materials.begin(), materials.end(), primitive.material) == materials.end())
                materials.push_back (primitive.material);
        }
    }

    const Part* findPart (yup::StringRef name) const
    {
        for (const auto& part : parts)
        {
            if (part.name == name)
                return &part;
        }

        return nullptr;
    }

    yup::Rectangle<float> getArea (yup::StringRef name) const
    {
        const auto* part = findPart (name);
        return part != nullptr ? part->area : yup::Rectangle<float>();
    }

    /** The radius of a whole knob: the model paints only its cap.

        The ratios mirror the proportions of the knobs built into the model: the cap is 74% of
        the skirt on the Q-Links, 80% on the main knobs and 84% on the data wheel, and the base
        flange is 7% wider than the skirt.
    */
    static float getKnobRadius (const Part& part)
    {
        const auto capRadius = part.area.getWidth() * 0.5f;

        if (part.name == "DataWheel")
            return capRadius / 0.84f;

        if (part.name.startsWith ("Knob.Q"))
            return capRadius / 0.74f * 1.07f;

        return capRadius / 0.80f * 1.07f;
    }

    static yup::Point<float> getPointOnCircle (yup::Point<float> center, float radius, float angleFromTop)
    {
        return { center.getX() + radius * std::sin (angleFromTop), center.getY() - radius * std::cos (angleFromTop) };
    }

    static yup::Font getFont (float height, float weight, float width = 100.0f)
    {
        auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (height);

        if (font.getAxisDescription ("wght").has_value())
            font = font.withAxisValue ("wght", weight);

        if (font.getAxisDescription ("wdth").has_value())
            font = font.withAxisValue ("wdth", width);

        return font;
    }

    static void paintText (yup::Graphics& g, const yup::String& text, const yup::Font& font, yup::Color color, yup::Rectangle<float> area, yup::Justification justification)
    {
        g.setFillColor (color);
        g.fillFittedText (text, font, area, justification);
    }

    //==============================================================================
    void paintBody (yup::Graphics& g)
    {
        g.setFillColorGradient (yup::ColorGradient (panelColor.brighter (0.03f), 0.0f, 0.0f, panelColor.darker (0.04f), 0.0f, frontBand.getY()));
        g.fillRect (0.0f, 0.0f, designWidth, designHeight);

        // The side wings fall away from the panel, darker towards the outside
        g.setFillColorGradient (yup::ColorGradient (wingColor.darker (0.12f), leftWing.getX(), 0.0f, wingColor, leftWing.getRight(), 0.0f));
        g.fillRect (leftWing);
        g.setFillColorGradient (yup::ColorGradient (wingColor, rightWing.getX(), 0.0f, wingColor.darker (0.12f), rightWing.getRight(), 0.0f));
        g.fillRect (rightWing);

        g.setFillColor (yup::Colors::black.withAlpha (0.18f));
        g.fillRect (leftWing.getRight() - 0.6f, 0.0f, 0.6f, frontBand.getBottom());
        g.fillRect (rightWing.getX(), 0.0f, 0.6f, frontBand.getBottom());

        // The front band slopes down to the front edge
        g.setFillColorGradient (yup::ColorGradient (bandColor.brighter (0.06f), 0.0f, frontBand.getY(), bandColor.darker (0.08f), 0.0f, frontBand.getBottom()));
        g.fillRect (frontBand);
        g.setFillColor (yup::Colors::white.withAlpha (0.35f));
        g.fillRect (frontBand.getX(), frontBand.getY(), frontBand.getWidth(), 0.8f);

        // The screen housing, the glass is a separate mesh on top of it
        g.setFillColorGradient (yup::ColorGradient (housingColor.brighter (0.04f), 0.0f, screenHousing.getY(), housingColor.darker (0.06f), 0.0f, screenHousing.getBottom()));
        g.fillRoundedRect (screenHousing, 3.0f);
        g.setStrokeColor (housingColor.darker (0.2f));
        g.setStrokeWidth (0.6f);
        g.strokeRoundedRect (screenHousing.reduced (0.3f), 3.0f);

        paintInset (g, qlinkPanel, insetColor);
        paintInset (g, transportPanel, transportColor);

        for (const auto& screw : screws)
            paintScrew (g, screw);
    }

    static void paintInset (yup::Graphics& g, yup::Rectangle<float> area, yup::Color color)
    {
        g.setFillColor (color);
        g.fillRoundedRect (area, 2.0f);

        // Recessed: shadowed on the far edges, lit on the near ones
        g.setFillColor (yup::Colors::black.withAlpha (0.16f));
        g.fillRect (area.getX() + 1.0f, area.getY(), area.getWidth() - 2.0f, 1.0f);
        g.fillRect (area.getX(), area.getY() + 1.0f, 1.0f, area.getHeight() - 2.0f);
        g.setFillColor (yup::Colors::white.withAlpha (0.3f));
        g.fillRect (area.getX() + 1.0f, area.getBottom() - 0.8f, area.getWidth() - 2.0f, 0.8f);
        g.fillRect (area.getRight() - 0.8f, area.getY() + 1.0f, 0.8f, area.getHeight() - 2.0f);

        g.setStrokeColor (color.darker (0.25f));
        g.setStrokeWidth (0.4f);
        g.strokeRoundedRect (area, 2.0f);
    }

    static void paintScrew (yup::Graphics& g, yup::Point<float> center)
    {
        const auto head = yup::Rectangle<float> (6.4f, 6.4f).withCenter (center);

        g.setFillColorGradient (yup::ColorGradient (yup::Color (0xff6a6c6e), center.getX() - 1.5f, center.getY() - 1.5f, yup::Color (0xff2a2b2c), center.getX() + 3.2f, center.getY() + 3.2f, yup::ColorGradient::Radial));
        g.fillEllipse (head);
        g.setStrokeColor (yup::Colors::black.withAlpha (0.5f));
        g.setStrokeWidth (0.35f);
        g.strokeEllipse (head);

        g.setStrokeColor (yup::Color (0xff1a1a1a));
        g.setStrokeWidth (0.7f);
        g.strokeLine (center.getX() - 1.8f, center.getY(), center.getX() + 1.8f, center.getY());
        g.strokeLine (center.getX(), center.getY() - 1.8f, center.getX(), center.getY() + 1.8f);
    }

    //==============================================================================
    void paintLogos (yup::Graphics& g)
    {
        // YUP! professional, top left
        paintText (g, "YUP!", getFont (44.0f, 900.0f, 125.0f), logoRed, { 80.0f, 12.0f, 205.0f, 54.0f }, yup::Justification::center);

        const auto box = yup::Rectangle<float> (102.0f, 66.0f, 160.0f, 24.0f);
        g.setFillColor (logoRed);
        g.fillRoundedRect (box, 3.0f);
        paintText (g, "professional", getFont (19.0f, 700.0f, 105.0f), panelColor, box.withY (box.getY() - 1.5f), yup::Justification::center);

        // NPC Renaissance, top right, the letters drawn in outline
        g.setStrokeColor (inkColor);
        g.setStrokeWidth (1.3f);
        g.setStrokeJoin (yup::StrokeJoin::Miter);
        g.strokePath (createNpcLogoPath ({ 770.0f, 20.0f, 150.0f, 42.0f }));
        paintText (g, "RENAISSANCE", getFont (9.5f, 850.0f, 150.0f), inkColor, { 762.0f, 70.0f, 166.0f, 14.0f }, yup::Justification::center);

        // The same logo, smaller, on the front edge
        const auto frontTop = frontBand.getBottom() + 3.0f;
        paintText (g, "YUP!", getFont (13.0f, 900.0f, 125.0f), logoRed, { 76.0f, frontTop, 44.0f, 18.0f }, yup::Justification::centerLeft);

        const auto smallBox = yup::Rectangle<float> (122.0f, frontTop + 5.0f, 46.0f, 8.0f);
        g.setFillColor (logoRed);
        g.fillRoundedRect (smallBox, 1.2f);
        paintText (g, "professional", getFont (6.0f, 700.0f, 105.0f), panelColor, smallBox.withY (smallBox.getY() - 0.5f), yup::Justification::center);
    }

    //==============================================================================
    void paintShadows (yup::Graphics& g)
    {
        for (const auto& part : parts)
        {
            if (part.area.isEmpty())
                continue;

            if (part.name.startsWith ("Knob.") || part.name == "DataWheel")
            {
                // Ambient occlusion around the base: centered, the cast shadows come from the renderer
                const auto center = part.area.getCenter();
                const auto radius = getKnobRadius (part);
                const auto blur = radius * 0.45f;
                const auto occlusion = yup::Colors::black.withAlpha (0.35f);

                g.setFillColorGradient (yup::ColorGradient (yup::ColorGradient::Radial, {
                    yup::ColorGradient::ColorStop (occlusion, center, 0.0f),
                    yup::ColorGradient::ColorStop (occlusion, center, radius / (radius + blur)),
                    yup::ColorGradient::ColorStop (occlusion.withAlpha (0.0f), center.getX() + radius + blur, center.getY(), 1.0f) }));
                g.fillEllipse (yup::Rectangle<float> ((radius + blur) * 2.0f, (radius + blur) * 2.0f).withCenter (center));
            }
            else if (part.name.startsWith ("Button."))
            {
                g.setFillColor (yup::Colors::black.withAlpha (0.06f));

                for (int i = 1; i <= 4; ++i)
                    g.fillRoundedRect (part.area.enlarged (0.5f * static_cast<float> (i)), 1.5f + 0.5f * static_cast<float> (i));
            }
        }
    }

    void paintKnobScales (yup::Graphics& g)
    {
        constexpr auto sweep = yup::MathConstants<float>::pi * 0.75f;

        g.setStrokeCap (yup::StrokeCap::Round);

        for (const auto& part : parts)
        {
            if (! part.name.startsWith ("Knob.") || part.area.isEmpty())
                continue;

            const auto center = part.area.getCenter();
            const auto radius = getKnobRadius (part);

            if (part.name.startsWith ("Knob.Q"))
            {
                // Q-Link: a ring of ticks, the longer ones at the ends
                g.setStrokeColor (scaleColor);
                g.setStrokeWidth (0.9f);

                for (int i = 0; i <= 10; ++i)
                {
                    const auto angle = -sweep + sweep * 2.0f * static_cast<float> (i) / 10.0f;
                    const auto length = (i == 0 || i == 10) ? 4.5f : 3.2f;
                    g.strokeLine (getPointOnCircle (center, radius + 2.2f, angle), getPointOnCircle (center, radius + 2.2f + length, angle));
                }

                paintText (g, part.name.fromFirstOccurrenceOf (".", false, false), getFont (4.4f, 650.0f), inkColor, { center.getX() - 15.0f, center.getY() + radius + 3.5f, 30.0f, 6.0f }, yup::Justification::centerTop);
                continue;
            }

            // Main knobs: a dotted scale with its end marks
            g.setFillColor (scaleColor);

            for (int i = 0; i <= 10; ++i)
            {
                const auto angle = -sweep + sweep * 2.0f * static_cast<float> (i) / 10.0f;
                g.fillEllipse (yup::Rectangle<float> (1.2f, 1.2f).withCenter (getPointOnCircle (center, radius + 3.5f, angle)));
            }

            const auto isDirectMon = part.name == "Knob.DirectMon";
            const auto endFont = getFont (3.1f, 650.0f);
            const auto minPoint = getPointOnCircle (center, radius + 7.0f, -sweep * 1.05f);
            const auto maxPoint = getPointOnCircle (center, radius + 7.0f, sweep * 1.05f);
            paintText (g, isDirectMon ? "INPUT" : "MIN", endFont, inkColor, yup::Rectangle<float> (14.0f, 4.0f).withCenter (minPoint), yup::Justification::center);
            paintText (g, isDirectMon ? "COMP" : "MAX", endFont, inkColor, yup::Rectangle<float> (14.0f, 4.0f).withCenter (maxPoint), yup::Justification::center);

            const auto* name = part.name == "Knob.RecGain" ? "REC GAIN" : isDirectMon ? "DIRECT MON" : "MAIN VOLUME";
            paintText (g, name, getFont (4.3f, 700.0f), inkColor, { center.getX() - 25.0f, center.getY() + radius + 2.0f, 50.0f, 6.0f }, yup::Justification::centerTop);
        }

        // The input level meter beside the record gain
        const auto recGain = getArea ("Knob.RecGain");
        if (! recGain.isEmpty())
        {
            static constexpr std::array<const char*, 4> levels { "0dB", "-3dB", "-6dB", "-20dB" };

            for (size_t i = 0; i < levels.size(); ++i)
            {
                const auto y = recGain.getCenterY() - 26.0f + 8.0f * static_cast<float> (i);
                const auto led = yup::Point<float> (recGain.getX() - 30.0f, y);

                g.setFillColor (i == 0 ? yup::Color (0xff5c1414) : yup::Color (0xff1f4a18));
                g.fillEllipse (yup::Rectangle<float> (2.6f, 2.6f).withCenter (led));
                paintText (g, levels[i], getFont (3.3f, 650.0f), inkColor, { led.getX() - 20.0f, y - 2.5f, 17.0f, 5.0f }, yup::Justification::centerRight);
            }
        }
    }

    //==============================================================================
    void paintLabels (yup::Graphics& g)
    {
        static constexpr std::array<Label, 47> labels { {
            { "Button.VintageMode", "VINTAGE MODE", nullptr },
            { "Button.PadBankA", "A", "E" },
            { "Button.PadBankB", "B", "F" },
            { "Button.PadBankC", "C", "G" },
            { "Button.FullLevel", "FULL LEVEL", "HALF LEVEL" },
            { "Button.SixteenLevel", "16 LEVEL", nullptr },
            { "Button.StepSeq", "STEP SEQ", nullptr },
            { "Button.NextSeq", "NEXT SEQ", nullptr },
            { "Button.PadAssign", "PAD ASSIGN", "PAD COPY" },
            { "Button.F1", "F1", nullptr },
            { "Button.F2", "F2", nullptr },
            { "Button.F3", "F3", nullptr },
            { "Button.F4", "F4", nullptr },
            { "Button.F5", "F5", nullptr },
            { "Button.F6", "F6", nullptr },
            { "Button.Window", "WINDOW", "FULL SCREEN" },
            { "Button.ProgEdit", "PROG EDIT", "Q-LINK" },
            { "Button.ProgMix", "PROG MIX", "TRACK MIX" },
            { "Button.SeqEdit", "SEQ EDIT", "EFFECTS" },
            { "Button.SampleEdit", "SAMPLE EDIT", "SAMPLE REC" },
            { "Button.Project", "PROJECT", "FOLDER 1" },
            { "Button.Sequence", "SEQUENCE", "FOLDER 2" },
            { "Button.Program", "PROGRAM", "FOLDER 3" },
            { "Button.Sample", "SAMPLE", "FOLDER 4" },
            { "Button.Main", "MAIN", "TRACK" },
            { "Button.Browser", "BROWSER", "SAVE" },
            { "Button.Undo", "UNDO", "REDO" },
            { "Button.Shift", nullptr, "SHIFT" },
            { "Button.TapTempo", "TAP TEMPO", nullptr },
            { "Button.Num1", "1", nullptr },
            { "Button.Num2", "2", nullptr },
            { "Button.Num3", "3", nullptr },
            { "Button.Num4", "4", nullptr },
            { "Button.Num5", "5", nullptr },
            { "Button.Num6", "6", nullptr },
            { "Button.Num7", "7", nullptr },
            { "Button.Num8", "8", nullptr },
            { "Button.Num9", "9", nullptr },
            { "Button.Sign", "-/+", nullptr },
            { "Button.Num0", "0", nullptr },
            { "Button.Enter", "ENTER", nullptr },
            { "Button.Minus", "-", nullptr },
            { "Button.Plus", "+", nullptr },
            { "Button.Cursor", "CURSOR", nullptr },
            { "Button.QLinkTrigger", "Q-LINK TRIGGER", nullptr },
            { "Button.Erase", "ERASE", nullptr },
            { "Button.NoteRepeat", "NOTE REPEAT", "LATCH" },
        } };

        const auto labelFont = getFont (4.2f, 650.0f);
        const auto secondaryFont = getFont (3.8f, 650.0f);

        for (const auto& label : labels)
        {
            const auto area = getArea (label.part);
            if (area.isEmpty())
                continue;

            if (label.text != nullptr)
                paintText (g, label.text, labelFont, inkColor, { area.getCenterX() - 30.0f, area.getY() - 7.0f, 60.0f, 6.0f }, yup::Justification::centerBottom);

            if (label.secondary != nullptr)
                paintText (g, label.secondary, secondaryFont, secondaryColor, { area.getCenterX() - 30.0f, area.getBottom() + 0.8f, 60.0f, 6.0f }, yup::Justification::centerTop);
        }

        g.setStrokeCap (yup::StrokeCap::Butt);

        paintHeader (g, "PAD BANK", { "Button.PadBankA", "Button.PadBankC" }, inkColor);
        paintHeader (g, "MODE", { "Button.ProgEdit", "Button.SampleEdit" }, inkColor);
        paintHeader (g, "DATA SELECT", { "Button.Project", "Button.Sample" }, inkColor);
        paintHeader (g, "EVENT", { "Button.PrevEvent", "Button.NextEvent" }, transportInkColor);
        paintHeader (g, "BAR", { "Button.PrevBar", "Button.NextBar" }, transportInkColor);

        // Vintage mode selects one of the emulations along its line
        const auto vintage = getArea ("Button.VintageMode");
        if (! vintage.isEmpty())
        {
            g.setStrokeColor (inkColor);
            g.setStrokeWidth (0.4f);
            g.strokeLine (vintage.getRight() + 1.5f, vintageLeds[0].getY(), vintageLeds[2].getX(), vintageLeds[2].getY());

            static constexpr std::array<const char*, 3> names { "NPC3000", "NPC60", "OTHER" };
            for (size_t i = 0; i < names.size(); ++i)
                paintText (g, names[i], getFont (3.6f, 650.0f), inkColor, yup::Rectangle<float> (28.0f, 5.0f).withCenter (vintageLeds[i].translated (0.0f, -7.0f)), yup::Justification::center);
        }
    }

    /** A title over a group of buttons, on a line spanning the group. */
    void paintHeader (yup::Graphics& g, const yup::String& text, std::array<const char*, 2> span, yup::Color color)
    {
        const auto area = getArea (span[0]).unionWith (getArea (span[1]));
        if (getArea (span[0]).isEmpty() || getArea (span[1]).isEmpty())
            return;

        const auto font = getFont (3.8f, 750.0f);
        const auto y = area.getY() - 9.0f;
        const auto halfText = static_cast<float> (text.length()) * 1.4f + 2.5f;

        g.setStrokeColor (color);
        g.setStrokeWidth (0.4f);
        g.strokeLine (area.getX(), y, area.getCenterX() - halfText, y);
        g.strokeLine (area.getCenterX() + halfText, y, area.getRight(), y);
        g.strokeLine (area.getX(), y, area.getX(), y + 1.5f);
        g.strokeLine (area.getRight(), y, area.getRight(), y + 1.5f);

        paintText (g, text, font, color, { area.getCenterX() - 30.0f, y - 3.0f, 60.0f, 6.0f }, yup::Justification::center);
    }

    void paintLeds (yup::Graphics& g)
    {
        // Bezels around the LED domes, which are separate meshes
        for (const auto& led : leds)
        {
            g.setFillColor (yup::Color (0xff2a2a2a));
            g.fillEllipse (yup::Rectangle<float> (6.4f, 6.4f).withCenter (led));
            g.setFillColor (yup::Colors::white.withAlpha (0.25f));
            g.fillEllipse (yup::Rectangle<float> (5.2f, 5.2f).withCenter (led.translated (0.3f, 0.3f)));
        }
    }

    //==============================================================================
    void paintButtonCaps (yup::Graphics& g)
    {
        for (const auto& part : parts)
        {
            if (! part.name.startsWith ("Button.") || part.area.isEmpty())
                continue;

            const auto& area = part.area;
            const auto color = part.color;

            g.setFillColorGradient (yup::ColorGradient (color.brighter (0.07f), 0.0f, area.getY(), color.darker (0.07f), 0.0f, area.getBottom()));
            g.fillRoundedRect (area, 1.2f);

            g.setFillColor (yup::Colors::white.withAlpha (0.25f));
            g.fillRoundedRect (area.reduced (0.8f).withHeight (0.6f), 0.3f);

            g.setStrokeColor (color.darker (0.3f));
            g.setStrokeWidth (0.35f);
            g.strokeRoundedRect (area.reduced (0.2f), 1.2f);

            paintCapLegend (g, part);
        }
    }

    /** The legends printed on some of the caps. */
    static void paintCapLegend (yup::Graphics& g, const Part& part)
    {
        const auto& area = part.area;
        const auto center = area.getCenter();
        const auto key = part.name.fromFirstOccurrenceOf (".", false, false);
        const auto legendColor = key == "Rec" || key == "OverDub" ? yup::Color (0xfff4ece6) : yup::Color (0xff2b2b2b);

        if (key == "Rec" || key == "OverDub" || key == "Stop" || key == "Play")
        {
            const auto* text = key == "OverDub" ? "OVER DUB" : key == "Rec" ? "REC" : key == "Stop" ? "STOP" : "PLAY";
            paintText (g, text, getFont (6.0f, 750.0f), legendColor, area, yup::Justification::center);
            return;
        }

        const auto chevron = [&] (float x, float direction)
        {
            yup::Path path;
            path.moveTo (x - 2.0f * direction, center.getY() - 4.0f)
                .lineTo (x + 2.0f * direction, center.getY())
                .lineTo (x - 2.0f * direction, center.getY() + 4.0f);

            g.setStrokeColor (legendColor);
            g.setStrokeWidth (1.3f);
            g.setStrokeJoin (yup::StrokeJoin::Miter);
            g.setStrokeCap (yup::StrokeCap::Butt);
            g.strokePath (path);
        };

        if (key == "PrevEvent")
            chevron (center.getX(), -1.0f);
        else if (key == "NextEvent")
            chevron (center.getX(), 1.0f);
        else if (key == "PrevBar")
        {
            chevron (center.getX() - 2.5f, -1.0f);
            chevron (center.getX() + 2.5f, -1.0f);
        }
        else if (key == "NextBar")
        {
            chevron (center.getX() - 2.5f, 1.0f);
            chevron (center.getX() + 2.5f, 1.0f);
        }
        else if (key == "Cursor")
        {
            const auto arrowAt = [&] (float angle)
            {
                yup::Path path;
                path.moveTo (getPointOnCircle (center, area.getWidth() * 0.38f, angle))
                    .lineTo (getPointOnCircle (center, area.getWidth() * 0.26f, angle - 0.35f))
                    .lineTo (getPointOnCircle (center, area.getWidth() * 0.26f, angle + 0.35f))
                    .close();

                g.setFillColor (legendColor);
                g.fillPath (path);
            };

            for (int i = 0; i < 4; ++i)
                arrowAt (yup::MathConstants<float>::halfPi * static_cast<float> (i));
        }
    }

    void paintKnobCaps (yup::Graphics& g)
    {
        for (const auto& part : parts)
        {
            if (part.area.isEmpty())
                continue;

            const auto center = part.area.getCenter();
            const auto radius = part.area.getWidth() * 0.5f;

            if (part.name == "DataWheel")
            {
                paintMetalCap (g, center, radius);

                // The finger dimple
                const auto dimple = getPointOnCircle (center, radius * 0.55f, -0.7f);
                const auto dimpleRadius = radius * 0.17f;
                g.setFillColorGradient (yup::ColorGradient (yup::Color (0xffc9cbcd), dimple.getX() + dimpleRadius * 0.3f, dimple.getY() + dimpleRadius * 0.3f, yup::Color (0xff76797c), dimple.getX() + dimpleRadius, dimple.getY(), yup::ColorGradient::Radial));
                g.fillEllipse (yup::Rectangle<float> (dimpleRadius * 2.0f, dimpleRadius * 2.0f).withCenter (dimple));
            }
            else if (part.name.startsWith ("Knob.Q"))
            {
                paintMetalCap (g, center, radius);
            }
            else if (part.name.startsWith ("Knob."))
            {
                // Main knobs: a colored top with a white pointer
                const auto color = part.name == "Knob.RecGain" ? yup::Color (0xffb02530) : yup::Color (0xff1c1c1c);

                g.setFillColorGradient (yup::ColorGradient (color.brighter (0.15f), center.getX() - radius * 0.3f, center.getY() - radius * 0.3f, color.darker (0.1f), center.getX() + radius, center.getY(), yup::ColorGradient::Radial));
                g.fillEllipse (yup::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCenter (center));

                g.setStrokeColor (yup::Color (0xfff2f0ea));
                g.setStrokeWidth (radius * 0.12f);
                g.setStrokeCap (yup::StrokeCap::Round);
                g.strokeLine (getPointOnCircle (center, radius * 0.25f, 0.0f), getPointOnCircle (center, radius * 0.9f, 0.0f));
            }
        }
    }

    /** Brushed aluminum: a radial sheen, fine concentric rings and two reflections across. */
    static void paintMetalCap (yup::Graphics& g, yup::Point<float> center, float radius)
    {
        const auto disc = yup::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCenter (center);

        g.setFillColorGradient (yup::ColorGradient (yup::Color (0xffdfe1e3), center.getX(), center.getY(), yup::Color (0xff8f9296), center.getX() + radius, center.getY(), yup::ColorGradient::Radial));
        g.fillEllipse (disc);

        g.setStrokeWidth (0.15f);
        int ring = 0;
        for (float r = 0.5f; r < radius; r += 0.4f)
        {
            g.setStrokeColor ((ring++ % 2) == 0 ? yup::Colors::white.withAlpha (0.12f) : yup::Colors::black.withAlpha (0.08f));
            g.strokeEllipse (yup::Rectangle<float> (r * 2.0f, r * 2.0f).withCenter (center));
        }

        for (const auto angle : { -0.8f, -0.8f + yup::MathConstants<float>::pi })
        {
            yup::Path wedge;
            wedge.moveTo (center)
                .lineTo (getPointOnCircle (center, radius, angle - 0.3f))
                .lineTo (getPointOnCircle (center, radius, angle))
                .lineTo (getPointOnCircle (center, radius, angle + 0.3f))
                .close();

            g.setFillColor (yup::Colors::white.withAlpha (0.22f));
            g.fillPath (wedge);
        }

        g.setStrokeColor (yup::Colors::black.withAlpha (0.45f));
        g.setStrokeWidth (0.4f);
        g.strokeEllipse (disc.reduced (0.2f));
    }

    //==============================================================================
    // Areas of the body, in design units: the parts are found from the model
    static constexpr yup::Rectangle<float> leftWing { 0.0f, 0.0f, 62.94f, 606.84f };
    static constexpr yup::Rectangle<float> rightWing { 934.53f, 0.0f, 65.47f, 606.84f };
    static constexpr yup::Rectangle<float> frontBand { 62.94f, 506.03f, 871.59f, 85.03f };
    static constexpr yup::Rectangle<float> qlinkPanel { 74.89f, 251.12f, 212.32f, 209.08f };
    static constexpr yup::Rectangle<float> transportPanel { 711.37f, 369.45f, 199.99f, 114.68f };
    static constexpr yup::Rectangle<float> screenHousing { 299.17f, 37.81f, 389.04f, 105.75f };

    // The LED domes have no painted faces to find them from
    static constexpr std::array<yup::Point<float>, 3> vintageLeds { { { 148.5f, 125.8f }, { 180.5f, 125.8f }, { 207.9f, 125.8f } } };
    static constexpr std::array<yup::Point<float>, 5> leds { { { 148.5f, 125.8f }, { 180.5f, 125.8f }, { 207.9f, 125.8f }, { 736.9f, 426.9f }, { 768.9f, 426.9f } } };

    static constexpr std::array<yup::Point<float>, 8> screws { { { 72.0f, 14.0f }, { 290.0f, 14.0f }, { 708.0f, 14.0f }, { 926.0f, 14.0f }, { 72.0f, 497.0f }, { 292.0f, 497.0f }, { 702.0f, 497.0f }, { 926.0f, 497.0f } } };

    static constexpr yup::Color panelColor { 0xffe6e3da };
    static constexpr yup::Color wingColor { 0xff8a8e91 };
    static constexpr yup::Color bandColor { 0xff858a8d };
    static constexpr yup::Color housingColor { 0xffd7d7d3 };
    static constexpr yup::Color insetColor { 0xffd3d2cc };
    static constexpr yup::Color transportColor { 0xff8c9093 };
    static constexpr yup::Color inkColor { 0xff2b2b2b };
    static constexpr yup::Color transportInkColor { 0xffeeeeea };
    static constexpr yup::Color secondaryColor { 0xffd9632b };
    static constexpr yup::Color scaleColor { 0xff3a3a3a };
    static constexpr yup::Color logoRed { 0xffc5283a };

    std::vector<Part> parts;
    std::vector<yup::Material::Ptr> materials;
    yup::Texture::Ptr texture;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MpcFaceplate)
};
