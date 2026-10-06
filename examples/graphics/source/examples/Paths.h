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

#include "paths/PathDocument.h"
#include "paths/PathSvgExport.h"
#include "paths/PathCanvas.h"
#include "paths/PathInspector.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

//==============================================================================

/** A small vector editor: layers of yup::Path held in a schema-validated DataTree, with undo and SVG export. */
class PathsExample : public yup::Component
{
public:
    PathsExample()
        : Component ("PathsExample")
        , canvas (document, selection)
        , inspector (document, selection)
    {
        setWantsKeyboardFocus (true);

        selectToolButton = &toolbar.addButton<yup::ToggleButton> ("Select (V)", [this]
        {
            setTool (Tool::select);
        });

        penToolButton = &toolbar.addButton<yup::ToggleButton> ("Pen (P)", [this]
        {
            setTool (Tool::pen);
        });

        undoButton = &toolbar.addButton ("Undo", [this]
        {
            document.undo();
        });

        redoButton = &toolbar.addButton ("Redo", [this]
        {
            document.redo();
        });

        toolbar.addButton ("Save SVG", [this]
        {
            saveSvg();
        });

        toolbar.addButton ("Copy SVG", [this]
        {
            yup::SystemClipboard::copyTextToClipboard (PathEditor::exportSvg (document.getRoot()));
        });

        addAndMakeVisible (toolbar);
        addAndMakeVisible (canvas);
        addAndMakeVisible (inspector);

        document.onChanged = [this]
        {
            documentChanged();
        };

        canvas.onSelectionChanged = [this]
        {
            selectionChanged();
        };

        inspector.onSelectionChanged = [this]
        {
            selectionChanged();
        };

        documentChanged();
        keepKeyboardFocusOnEditor (*this);
    }

    ~PathsExample() override
    {
        // The cached paint images hold GPU textures, which must go before the graphics context does
        PathEditor::clearPaintImageCache();
    }

    void resized() override
    {
        auto area = getLocalBounds();
        inspector.setBounds (area.removeFromRight (yup::jmin (300.0f, area.getWidth() * 0.45f)));
        toolbar.setBounds (area.removeFromTop (38.0f).reduced (6.0f));
        canvas.setBounds (area);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (yup::ThemePalette::Role::background));
        g.fillAll();
    }

    void visibilityChanged() override
    {
        if (isVisible() && getParentComponent() != nullptr)
            takeKeyboardFocus();
    }

    void focusLost() override
    {
        // Losing the window focus mid-drag sends no mouseUp: close the gesture so undo keeps working
        canvas.cancelDrag();
        document.endGesture();
    }

    void keyDown (const yup::KeyPress& keys, const yup::Point<float>&) override
    {
        // Every key is consumed here, so Escape and Z never reach the demo window (quit / fullscreen)
        if (document.isGestureActive())
            return;

        const auto modifiers = keys.getModifiers();
        const bool command = modifiers.isCommandDown() || modifiers.isControlDown();

        auto key = keys.getKey();
        if (key >= 'a' && key <= 'z')
            key -= 'a' - 'A';

        if (command && key == yup::KeyPress::textZKey)
            modifiers.isShiftDown() ? document.redo() : document.undo();
        else if (command && key == yup::KeyPress::textYKey)
            document.redo();
        else if (command && key == yup::KeyPress::textDKey)
            duplicateSelectedLayer();
        else if (key == yup::KeyPress::deleteKey || key == yup::KeyPress::backspaceKey)
            deleteSelection();
        else if (key == yup::KeyPress::escapeKey)
            canvas.isDrawingPath() ? canvas.finishPen() : setSelection ({}, {});
        else if (key == yup::KeyPress::enterKey)
            canvas.finishPen();
        else if (! command && key == yup::KeyPress::textVKey)
            setTool (Tool::select);
        else if (! command && key == yup::KeyPress::textPKey)
            setTool (Tool::pen);
    }

    void mouseWheel (const yup::MouseEvent& event, const yup::MouseWheelData& wheelData) override
    {
        // The focused component receives the wheel: forward it to the scrollable component under the mouse
        for (auto* target = findComponentAtForMouseEvent (event.getPosition()); target != nullptr && target != this; target = target->getParentComponent())
        {
            if (dynamic_cast<PathEditor::ScrollPanel*> (target) != nullptr || dynamic_cast<yup::ListBox*> (target) != nullptr)
            {
                target->mouseWheel (event.withPosition (target->getLocalPoint (this, event.getPosition())), wheelData);
                return;
            }
        }
    }

private:
    using Tool = PathEditor::PathCanvas::Tool;

    /** Only text editors keep the keyboard focus, everything else leaves it to the editor for the shortcuts. */
    static void keepKeyboardFocusOnEditor (yup::Component& component)
    {
        for (int i = 0; i < component.getNumChildComponents(); ++i)
        {
            auto* child = component.getChildComponent (i);

            if (dynamic_cast<yup::TextEditor*> (child) == nullptr)
                child->setWantsKeyboardFocus (false);

            keepKeyboardFocusOnEditor (*child);
        }
    }

    void documentChanged()
    {
        validateSelection();
        canvas.repaint();
        inspector.refresh();
        updateToolbar();
    }

    void selectionChanged()
    {
        validateSelection();
        canvas.repaint();
        inspector.refresh();
    }

    /** Undo and redo can remove the selected nodes from the document. */
    void validateSelection()
    {
        if (! selection.layer.isAChildOf (document.getRoot()))
            selection.clear();
        else if (! selection.vertex.isAChildOf (selection.layer))
            selection.vertex = {};
    }

    void setSelection (const yup::DataTree& layer, const yup::DataTree& vertex)
    {
        selection.layer = layer;
        selection.vertex = vertex;
        selectionChanged();
    }

    void setTool (Tool tool)
    {
        canvas.setTool (tool);
        updateToolbar();
    }

    void updateToolbar()
    {
        selectToolButton->setToggleState (canvas.getTool() == Tool::select, yup::dontSendNotification);
        penToolButton->setToggleState (canvas.getTool() == Tool::pen, yup::dontSendNotification);
        undoButton->setEnabled (document.canUndo());
        redoButton->setEnabled (document.canRedo());
    }

    void deleteSelection()
    {
        if (selection.vertex.isValid())
        {
            document.deleteVertex (selection.vertex);
            setSelection (selection.layer, {});
        }
        else if (selection.layer.isValid())
        {
            document.deleteLayer (selection.layer);
            setSelection ({}, {});
        }
    }

    void duplicateSelectedLayer()
    {
        if (selection.layer.isValid())
            setSelection (document.duplicateLayer (selection.layer), {});
    }

    void saveSvg()
    {
        const auto initialFile = yup::File::getSpecialLocation (yup::File::userDocumentsDirectory).getChildFile ("paths.svg");
        fileChooser = yup::FileChooser::create ("Save SVG", initialFile, "*.svg");

        // The demo can be destroyed (switching demos) while the dialog is open
        fileChooser->browseForFileToSave ([weakThis = yup::WeakReference<yup::Component> (this)] (bool success, const yup::Array<yup::File>& results)
        {
            auto* self = dynamic_cast<PathsExample*> (weakThis.get());
            if (self == nullptr || ! success || results.isEmpty())
                return;

            auto file = results.getFirst();
            if (! file.hasFileExtension ("svg"))
                file = file.withFileExtension ("svg");

            file.replaceWithText (PathEditor::exportSvg (self->document.getRoot()), false, false, "\n");
        },
                                          true);
    }

    PathEditor::PathDocument document;
    PathEditor::Selection selection;
    PathEditor::ButtonRow toolbar;
    PathEditor::PathCanvas canvas;
    PathEditor::PathInspector inspector;
    yup::ToggleButton* selectToolButton = nullptr;
    yup::ToggleButton* penToolButton = nullptr;
    yup::TextButton* undoButton = nullptr;
    yup::TextButton* redoButton = nullptr;
    yup::FileChooser::Ptr fileChooser;
};
