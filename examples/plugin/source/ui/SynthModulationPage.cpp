/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of this software for any purpose is hereby granted, provided that the
   above copyright notice and this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  ==============================================================================
*/

#include "SynthModulationPage.h"
#include "SynthModulationRow.h"
#include "SynthTheme.h"
#include "SynthUndoHistory.h"

#include "../dsp/SynthPatch.h"

//==============================================================================
class SynthModulationPage::Content : public yup::Component
{
public:
    Content (const SynthParameters& parametersToEdit,
             SynthRoutes& routesToEdit,
             yup::Span<const yup::AudioParameter::Ptr> processorParameters)
        : routes (routesToEdit)
    {
        setOpaque (false);

        for (const auto& parameter : processorParameters)
        {
            const auto index = SynthPatch::findRouteDestination (processorParameters, parameter->getID());

            if (index >= 0)
                destinations.push_back ({ parameter->getID(), parameter->getName(), index });
        }

        for (std::size_t slot = 0; slot < rows.size(); ++slot)
        {
            rows[slot] = std::make_unique<SynthModulationRow> (static_cast<int> (slot), routesToEdit, destinations,
                                                               parametersToEdit.modulation.depths[slot]);
            addChildComponent (*rows[slot]);
        }

        syncFromRoutes();
    }

    void syncFromRoutes()
    {
        for (auto& row : rows)
            row->syncFromRoutes();

        if (const auto generation = routes.getGeneration(); generation != shownGeneration)
        {
            shownGeneration = generation;
            rebuildRows();
        }
    }

    void clearRoutes()
    {
        for (auto& row : rows)
            row->clear (true);
    }

    void setMacroNames (const std::array<yup::String, SynthExample::macroCount>& macroNames)
    {
        if (macroNames == shownMacroNames)
            return;

        shownMacroNames = macroNames;

        for (auto& row : rows)
            row->setMacroNames (macroNames);
    }

    int getListedCount() const noexcept
    {
        return static_cast<int> (listedRows.size());
    }

    yup::Rectangle<float> getEmptyRowBounds() const
    {
        if (emptyRowIndex < 0 || emptyRowIndex >= getListedCount())
            return {};

        return listedRows[static_cast<std::size_t> (emptyRowIndex)]->getBounds().to<float>();
    }

    float getContentHeight() const noexcept
    {
        return getListedCount() > 0 ? getListedCount() * (rowHeight + spacing) - spacing : 0.0f;
    }

    void setContentWidth (float width)
    {
        setSize (yup::jmax (0.0f, width), getContentHeight());
    }

    void resized() override
    {
        auto y = 0.0f;

        for (auto* row : listedRows)
        {
            row->setBounds (0.0f, y, getWidth(), rowHeight);
            y += rowHeight + spacing;
        }
    }

private:
    static constexpr float rowHeight = 34.0f;
    static constexpr float spacing = 4.0f;

    void rebuildRows()
    {
        listedRows.clear();
        emptyRowIndex = -1;
        auto offeredEmpty = false;

        for (auto& row : rows)
        {
            row->setVisible (false);

            if (row->isUsed() || ! offeredEmpty)
            {
                if (! row->isUsed())
                {
                    offeredEmpty = true;
                    emptyRowIndex = static_cast<int> (listedRows.size());
                }

                listedRows.push_back (row.get());
                row->setVisible (true);
            }
        }

        setSize (getWidth(), getContentHeight());
        resized();
    }

    std::vector<SynthRouteDestination> destinations;
    const SynthRoutes& routes;
    int shownGeneration = -1;
    int emptyRowIndex = -1;
    std::array<yup::String, SynthExample::macroCount> shownMacroNames;
    std::array<std::unique_ptr<SynthModulationRow>, SynthExample::modulationSlots> rows;
    std::vector<SynthModulationRow*> listedRows;
};

//==============================================================================
SynthModulationPage::SynthModulationPage (const SynthParameters& parametersToEdit,
                                          SynthRoutes& routesToEdit,
                                          yup::Span<const yup::AudioParameter::Ptr> processorParameters)
    : content (std::make_unique<Content> (parametersToEdit, routesToEdit, processorParameters))
{
    setOpaque (false);
    viewport.setScrollBarsShown (true, false);
    viewport.setViewedComponent (content.get());
    addAndMakeVisible (viewport);

    SynthTheme::styleRaisedButton (resetButton);
    resetButton.onClick = [this] { resetRoutes(); };
}

SynthModulationPage::~SynthModulationPage() = default;

void SynthModulationPage::syncFromParameters()
{
    const auto oldCount = content->getListedCount();
    content->syncFromRoutes();

    const auto newCount = content->getListedCount();

    if (newCount != oldCount)
        resized();

    if (newCount == oldCount + 1)
        viewport.scrollToShow (content->getEmptyRowBounds());
}

/** Removes every route and returns every depth to its default, as one undo step. */
void SynthModulationPage::resetRoutes()
{
    SynthUndoHistory::beginStep (*this);
    content->clearRoutes();
    syncFromParameters();
    viewport.setViewPosition ({ 0.0f, 0.0f });
}

void SynthModulationPage::setMacroNames (const std::array<yup::String, SynthExample::macroCount>& macroNames)
{
    content->setMacroNames (macroNames);
}

void SynthModulationPage::resized()
{
    viewport.setBounds (getLocalBounds());

    const auto needsVerticalScroll = content->getContentHeight() > viewport.getHeight();
    const auto scrollBarSpace = needsVerticalScroll ? viewport.getVerticalScrollBar().getScrollBarWidth() + 4.0f : 0.0f;
    const auto width = viewport.getWidth() - scrollBarSpace;
    content->setContentWidth (width);
}
