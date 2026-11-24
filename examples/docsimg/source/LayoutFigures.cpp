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

#include <yup_gui/themes/theme_v1/yup_ThemeVersion1_Icons.h>

namespace
{

//==============================================================================
/** A named box: what the layouts place, and what the effects are applied to. */
struct Box : yup::Component
{
    explicit Box (const char* nameToShow, float marginToLeave = 0.0f)
        : name (nameToShow)
        , margin (marginToLeave)
    {
        setOpaque (false);
    }

    void paint (yup::Graphics& g) override
    {
        const auto bounds = getLocalBounds().reduced (margin + 0.5f);

        g.setFillColor (role (Role::accent));
        g.fillRoundedRect (bounds, 4.0f);

        g.setStrokeColor (role (Role::background));
        g.strokeRoundedRect (bounds, 4.0f);

        g.setFillColor (role (Role::onAccent));
        g.fillFittedText (name, font (12.0f), bounds, yup::Justification::center);
    }

    yup::String name;
    float margin;
};

//==============================================================================
/** A container whose boxes are placed by a function, as a `resized()` would do. */
struct Container : yup::Component
{
    using Layout = std::function<void (Container&, yup::Rectangle<float>)>;

    Container (std::initializer_list<const char*> names, Layout layoutToUse)
        : layout (std::move (layoutToUse))
    {
        setOpaque (false);

        for (const auto* name : names)
            addAndMakeVisible (*boxes.emplace_back (std::make_unique<Box> (name)));
    }

    void resized() override
    {
        layout (*this, getLocalBounds());
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (role (Role::surface));
        g.fillRoundedRect (getLocalBounds(), 6.0f);
    }

    Box& operator[] (int index) { return *boxes[static_cast<std::size_t> (index)]; }

    std::vector<std::unique_ptr<Box>> boxes;
    Layout layout;
};

std::unique_ptr<Container> container (std::initializer_list<const char*> names, Container::Layout layout)
{
    return std::make_unique<Container> (names, std::move (layout));
}

//==============================================================================
// docs/ui/component-layout.md

void addLayoutPageFigures (Figures& f)
{
    using namespace yup;

    f.component ("ui/component-layout_flex-row", { 560.0f, 104.0f }, []
    {
        return container ({ "sidebar", "content" }, [] (Container& c, Rectangle<float> bounds)
        {
            FlexBox box;
            box.flexDirection = FlexBox::Direction::row;
            box.gap = 8.0f;

            box.items.add (FlexItem (c[0]).withWidth (200.0f));
            box.items.add (FlexItem (c[1]).withFlex (1).withFlexBasis (0));

            box.performLayout (bounds);
        });
    });

    f.component ("ui/component-layout_justify", { 560.0f, 230.0f }, []() -> std::unique_ptr<Component>
    {
        const char* const names[] = { "flexStart", "flexEnd", "center", "spaceBetween", "spaceAround", "spaceEvenly" };
        auto gallery = std::make_unique<Gallery> (2);

        for (int index = 0; index < 6; ++index)
        {
            gallery->add (names[index], container ({ "1", "2", "3" }, [index] (Container& c, Rectangle<float> bounds)
            {
                FlexBox box;
                box.justifyContent = static_cast<FlexBox::JustifyContent> (index);
                box.setPadding (6.0f);
                box.gap = 6.0f;

                for (int i = 0; i < 3; ++i)
                    box.items.add (FlexItem (c[i]).withWidth (44.0f));

                box.performLayout (bounds);
            }));
        }

        return gallery;
    });

    f.component ("ui/component-layout_align", { 560.0f, 150.0f }, []() -> std::unique_ptr<Component>
    {
        const char* const names[] = { "flexStart", "flexEnd", "center", "stretch" };
        auto gallery = std::make_unique<Gallery> (4);

        for (int index = 0; index < 4; ++index)
        {
            gallery->add (names[index], container ({ "1", "2", "3" }, [index] (Container& c, Rectangle<float> bounds)
            {
                FlexBox box;
                box.alignItems = static_cast<FlexBox::AlignItems> (index);
                box.setPadding (6.0f);
                box.gap = 6.0f;

                // An item with a height of its own keeps it, so the stretched ones have none
                for (int i = 0; i < 3; ++i)
                {
                    const auto item = FlexItem (c[i]).withWidth (30.0f);
                    box.items.add (index == 3 ? item : item.withHeight (24.0f + 16.0f * static_cast<float> (i)));
                }

                box.performLayout (bounds);
            }));
        }

        return gallery;
    });

    f.component ("ui/component-layout_auto-margins", { 560.0f, 80.0f }, []
    {
        return container ({ "File", "Edit", "Settings" }, [] (Container& c, Rectangle<float> bounds)
        {
            FlexBox box;
            box.setPadding (6.0f);
            box.gap = 6.0f;

            box.items.add (FlexItem (c[0]).withWidth (64.0f));
            box.items.add (FlexItem (c[1]).withWidth (64.0f));
            box.items.add (FlexItem (c[2]).withWidth (84.0f).withAutoMargins (true, false, false, false));

            box.performLayout (bounds);
        });
    });

    f.component ("ui/component-layout_grid", { 560.0f, 182.0f }, []
    {
        return container ({ "header", "body" }, [] (Container& c, Rectangle<float> bounds)
        {
            Grid grid;
            grid.templateColumns.add (Grid::TrackInfo::px (200));
            grid.templateColumns.add (Grid::TrackInfo::fr (1));
            grid.templateRows.addArray (Grid::repeat (3, Grid::TrackInfo::px (48)));
            grid.gap = 8.0f;

            grid.items.add (GridItem (c[0]).withColumn (0).withRow (0).withColumnSpan (2));
            grid.items.add (GridItem (c[1]).withColumn (1).withRow (1));

            grid.performLayout (bounds);
        });
    });

    f.component ("ui/component-layout_grid-tracks", { 560.0f, 84.0f }, []
    {
        return container ({ "px (120)", "fr (1)", "fr (2)", "percent (20)" }, [] (Container& c, Rectangle<float> bounds)
        {
            Grid grid;
            grid.templateColumns.add (Grid::TrackInfo::px (120));
            grid.templateColumns.add (Grid::TrackInfo::fr (1));
            grid.templateColumns.add (Grid::TrackInfo::fr (2));
            grid.templateColumns.add (Grid::TrackInfo::percent (20));
            grid.templateRows.add (Grid::TrackInfo::fr (1));
            grid.gap = 8.0f;

            for (int i = 0; i < 4; ++i)
                grid.items.add (GridItem (c[i]));

            grid.performLayout (bounds);
        });
    });

    f.component ("ui/component-layout_grid-areas", { 480.0f, 220.0f }, []
    {
        return container ({ "header", "side", "main", "footer" }, [] (Container& c, Rectangle<float> bounds)
        {
            Grid grid;
            grid.templateColumns.add (Grid::TrackInfo::px (120));
            grid.templateColumns.add (Grid::TrackInfo::fr (1));
            grid.templateRows.add (Grid::TrackInfo::px (36));
            grid.templateRows.add (Grid::TrackInfo::fr (1));
            grid.templateRows.add (Grid::TrackInfo::px (36));
            grid.gap = 8.0f;

            if (grid.setTemplateAreas ({ "header header", "side   main", "side   footer" }).failed())
                return;

            for (int i = 0; i < 4; ++i)
                grid.items.add (GridItem (c[i]).withArea (c[i].name));

            grid.performLayout (bounds);
        });
    });
}

//==============================================================================
// docs/ui/component-basics.md

void addComponentPageFigures (Figures& f)
{
    using namespace yup;

    f.component ("ui/component-basics_transform", { 560.0f, 150.0f }, []() -> std::unique_ptr<Component>
    {
        const char* const names[] = { "rotation (0.4)", "scaling (1.5)", "translation (30, 12)" };
        auto gallery = std::make_unique<Gallery> (3);

        for (int index = 0; index < 3; ++index)
        {
            gallery->add (names[index], container ({ "as placed", "transformed" }, [index] (Container& c, Rectangle<float> bounds)
            {
                const AffineTransform transforms[] = { AffineTransform::rotation (0.4f), AffineTransform::scaling (1.5f), AffineTransform::translation (30.0f, 12.0f) };

                for (int i = 0; i < 2; ++i)
                    c[i].setBounds (bounds.withSize (84.0f, 30.0f).translated (24.0f, 14.0f));

                c[0].setOpacity (0.35f);
                c[1].setTransform (transforms[index]);
            }));
        }

        return gallery;
    });

    f.component ("ui/component-basics_z-order", { 560.0f, 150.0f }, []() -> std::unique_ptr<Component>
    {
        const char* const names[] = { "as added", "A.toFront (false)", "C.toBack()" };
        auto gallery = std::make_unique<Gallery> (3);

        for (int index = 0; index < 3; ++index)
        {
            auto& stack = gallery->add (names[index], container ({ "A", "B", "C" }, [] (Container& c, Rectangle<float> bounds)
            {
                for (int i = 0; i < 3; ++i)
                    c[i].setBounds (bounds.withSizeKeepingCenter (64.0f, 44.0f).translated (26.0f * static_cast<float> (i - 1), 14.0f * static_cast<float> (i - 1)));
            }));

            if (index == 1)
                stack[0].toFront (false);
            else if (index == 2)
                stack[2].toBack();
        }

        return gallery;
    });
}

//==============================================================================
// docs/ui/component-styling.md and docs/ui/component-effects.md

void addStylingPageFigures (Figures& f)
{
    using namespace yup;

    f.paint ("ui/component-styling_palette-roles", { 560.0f, 166.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        labelled (g, area, { "background", "surface", "surfaceRaised", "outline", "text", "textMuted", "accent", "onAccent" }, 4, [&g] (int index, Rectangle<float> cell)
        {
            const auto swatch = cell.reduced (0.5f);

            g.setFillColor (role (static_cast<Role> (index)));
            g.fillRoundedRect (swatch, 8.0f);

            g.setStrokeColor (role (Role::outline));
            g.strokeRoundedRect (swatch, 8.0f);
        });
    });

    f.paint ("ui/component-styling_icons", { 560.0f, 96.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        const char* const icons[] = { YUP_ICON_GEAR, YUP_ICON_PLAY, YUP_ICON_PAUSE, YUP_ICON_STOP, YUP_ICON_FOLDER, YUP_ICON_MUSIC, YUP_ICON_HEART, YUP_ICON_STAR };
        const auto iconFont = ApplicationTheme::getGlobalTheme()->getDefaultIconFont().withHeight (28.0f);

        labelled (g, area, { "GEAR", "PLAY", "PAUSE", "STOP", "FOLDER", "MUSIC", "HEART", "STAR" }, 8, [&] (int index, Rectangle<float> cell)
        {
            g.setFillColor (role (Role::text));
            g.fillFittedText (String::fromUTF8 (icons[index]), iconFont, cell, Justification::center);
        });
    });

    f.component ("ui/component-effects_built-in", { 560.0f, 130.0f }, []() -> std::unique_ptr<Component>
    {
        auto gallery = std::make_unique<Gallery> (5, 4.0f);
        const auto card = []
        {
            return std::make_unique<Box> ("YUP", 12.0f);
        };

        gallery->add ("no effect", card());
        gallery->add ("BlurEffect", card()).setComponentEffect (new BlurEffect (3.0f));
        gallery->add ("DropShadowEffect", card()).setComponentEffect (new DropShadowEffect (Color (0xaa000000), 5.0f, { 0.0f, 5.0f }));
        gallery->add ("GlowEffect", card()).setComponentEffect (new GlowEffect (6.0f, 0.9f));
        gallery->add ("ColorMatrixEffect", card()).setComponentEffect (ColorMatrixEffect::grayscale());

        return gallery;
    });

    f.component ("ui/component-effects_paint-margin", { 400.0f, 150.0f }, []() -> std::unique_ptr<Component>
    {
        auto gallery = std::make_unique<Gallery> (2, 20.0f);

        for (const auto margin : { 0.0f, 20.0f })
        {
            auto& card = gallery->add (margin > 0.0f ? "setPaintMargin (20)" : "no paint margin", std::make_unique<Box> ("YUP"));
            card.setComponentEffect (new DropShadowEffect (Color (0xaa000000), 5.0f, { 0.0f, 5.0f }));
            card.setPaintMargin (margin);
        }

        return gallery;
    });
}

} // namespace

//==============================================================================

void addLayoutFigures (Figures& f)
{
    addComponentPageFigures (f);
    addLayoutPageFigures (f);
    addStylingPageFigures (f);
}
