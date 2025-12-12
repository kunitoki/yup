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

#if ! YUP_MODULE_AVAILABLE_yup_data_model
#error This binding file requires adding the yup_data_model module in the project
#else
#include <yup_data_model/yup_data_model.h>
#endif

#include "yup_YupEvents_bindings.h"

#include "../utilities/yup_PyBind11Includes.h"

namespace yup::Bindings
{

//==============================================================================

void registerYupDataModelBindings (pybind11::module_& m);

//==============================================================================

struct PyDataTreeListener : public yup::DataTreeListener
{
    void propertyChanged (yup::DataTree& tree, const yup::Identifier& property) override
    {
        PYBIND11_OVERRIDE (void, yup::DataTreeListener, propertyChanged, tree, property);
    }

    void childAdded (yup::DataTree& parent, yup::DataTree& child) override
    {
        PYBIND11_OVERRIDE (void, yup::DataTreeListener, childAdded, parent, child);
    }

    void childRemoved (yup::DataTree& parent, yup::DataTree& child, int formerIndex) override
    {
        PYBIND11_OVERRIDE (void, yup::DataTreeListener, childRemoved, parent, child, formerIndex);
    }

    void childMoved (yup::DataTree& parent, yup::DataTree& child, int oldIndex, int newIndex) override
    {
        PYBIND11_OVERRIDE (void, yup::DataTreeListener, childMoved, parent, child, oldIndex, newIndex);
    }

    void treeRedirected (yup::DataTree& tree) override
    {
        PYBIND11_OVERRIDE (void, yup::DataTreeListener, treeRedirected, tree);
    }
};

//==============================================================================

struct PyUndoableAction : public yup::UndoableAction
{
    bool isValid() const override
    {
        PYBIND11_OVERRIDE_PURE (bool, yup::UndoableAction, isValid);
    }

    bool perform (yup::UndoableActionState stateToPerform) override
    {
        PYBIND11_OVERRIDE_PURE (bool, yup::UndoableAction, perform, stateToPerform);
    }
};

} // namespace yup::Bindings
