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

#include "yup_YupDataModel_bindings.h"

#define YUP_PYTHON_INCLUDE_PYBIND11_OPERATORS
#define YUP_PYTHON_INCLUDE_PYBIND11_FUNCTIONAL
#include "../utilities/yup_PyBind11Includes.h"

//==============================================================================

namespace yup::Bindings
{

namespace py = pybind11;
using namespace py::literals;

void registerYupDataModelBindings (py::module_& m)
{
    // clang-format off

    // ============================================================================================ yup::UndoableActionState

    py::enum_<UndoableActionState> (m, "UndoableActionState")
        .value ("Undo", UndoableActionState::Undo)
        .value ("Redo", UndoableActionState::Redo)
        .export_values();

    // ============================================================================================ yup::UndoableAction

    py::class_<UndoableAction, PyUndoableAction, ReferenceCountedObjectPtr<UndoableAction>> classUndoableAction (m, "UndoableAction");

    classUndoableAction
        .def (py::init<>())
        .def ("isValid", &UndoableAction::isValid)
        .def ("perform", &UndoableAction::perform);

    // ============================================================================================ yup::UndoManager

    py::class_<UndoManager, ReferenceCountedObjectPtr<UndoManager>> classUndoManager (m, "UndoManager");

    py::class_<UndoManager::ScopedTransaction> classUndoManagerScopedTransaction (classUndoManager, "ScopedTransaction");

    classUndoManagerScopedTransaction
        .def (py::init<UndoManager&>())
        .def (py::init<UndoManager&, StringRef>());

    classUndoManager
        .def (py::init<>())
        .def (py::init<int>())
        .def (py::init<RelativeTime>())
        .def (py::init<int, RelativeTime>())
        .def ("perform", [](UndoManager& self, UndoableAction::Ptr action) { return self.perform (std::move (action)); })
        .def ("beginNewTransaction", py::overload_cast<> (&UndoManager::beginNewTransaction))
        .def ("beginNewTransaction", py::overload_cast<StringRef> (&UndoManager::beginNewTransaction))
        .def ("getNumTransactions", &UndoManager::getNumTransactions)
        .def ("getTransactionName", &UndoManager::getTransactionName)
        .def ("getCurrentTransactionName", &UndoManager::getCurrentTransactionName)
        .def ("setCurrentTransactionName", &UndoManager::setCurrentTransactionName)
        .def ("canUndo", &UndoManager::canUndo)
        .def ("undo", &UndoManager::undo)
        .def ("canRedo", &UndoManager::canRedo)
        .def ("redo", &UndoManager::redo)
        .def ("clear", &UndoManager::clear)
        .def ("setEnabled", &UndoManager::setEnabled)
        .def ("isEnabled", &UndoManager::isEnabled);

    // ============================================================================================ yup::DataTreeListener

    py::class_<DataTreeListener, PyDataTreeListener> classDataTreeListener (m, "DataTreeListener");

    classDataTreeListener
        .def (py::init<>())
        .def ("propertyChanged", &DataTreeListener::propertyChanged)
        .def ("childAdded", &DataTreeListener::childAdded)
        .def ("childRemoved", &DataTreeListener::childRemoved)
        .def ("childMoved", &DataTreeListener::childMoved)
        .def ("treeRedirected", &DataTreeListener::treeRedirected);

    // ============================================================================================ yup::DataTree

    py::class_<DataTree> classDataTree (m, "DataTree");

    py::class_<DataTree::Iterator> classDataTreeIterator (classDataTree, "Iterator");

    classDataTreeIterator
        .def (py::init<>())
        .def ("__iter__", [] (DataTree::Iterator& self) { return self; })
        .def ("__next__", [] (DataTree::Iterator& self)
        {
            // We need to manually implement the iteration logic
            // This is a simplified version - a proper implementation would track the end
            auto value = *self;
            ++self;
            return value;
        });

    py::class_<DataTree::Transaction> classDataTreeTransaction (classDataTree, "Transaction");

    classDataTreeTransaction
        .def ("commit", &DataTree::Transaction::commit)
        .def ("abort", &DataTree::Transaction::abort)
        .def ("isActive", &DataTree::Transaction::isActive)
        .def ("setProperty", &DataTree::Transaction::setProperty)
        .def ("removeProperty", &DataTree::Transaction::removeProperty)
        .def ("removeAllProperties", &DataTree::Transaction::removeAllProperties)
        .def ("addChild", &DataTree::Transaction::addChild, "child"_a, "index"_a = -1)
        .def ("removeChild", py::overload_cast<const DataTree&> (&DataTree::Transaction::removeChild))
        .def ("removeChild", py::overload_cast<int> (&DataTree::Transaction::removeChild))
        .def ("removeAllChildren", &DataTree::Transaction::removeAllChildren)
        .def ("moveChild", &DataTree::Transaction::moveChild)
        .def ("getEffectiveChildCount", &DataTree::Transaction::getEffectiveChildCount);

    py::class_<DataTree::ValidatedTransaction> classDataTreeValidatedTransaction (classDataTree, "ValidatedTransaction");

    classDataTreeValidatedTransaction
        .def ("setProperty", &DataTree::ValidatedTransaction::setProperty)
        .def ("removeProperty", &DataTree::ValidatedTransaction::removeProperty)
        .def ("addChild", &DataTree::ValidatedTransaction::addChild, "child"_a, "index"_a = -1)
        .def ("createAndAddChild", &DataTree::ValidatedTransaction::createAndAddChild, "childType"_a, "index"_a = -1)
        .def ("removeChild", &DataTree::ValidatedTransaction::removeChild)
        .def ("commit", &DataTree::ValidatedTransaction::commit)
        .def ("abort", &DataTree::ValidatedTransaction::abort)
        .def ("isActive", &DataTree::ValidatedTransaction::isActive)
        .def ("getTransaction", &DataTree::ValidatedTransaction::getTransaction, py::return_value_policy::reference);

    classDataTree
        .def (py::init<>())
        .def (py::init<const Identifier&>())
        .def (py::init<const Identifier&, const std::initializer_list<std::pair<Identifier, var>>&>())
        .def (py::init<const Identifier&, const std::initializer_list<DataTree>&>())
        .def (py::init<const Identifier&, const std::initializer_list<std::pair<Identifier, var>>&, const std::initializer_list<DataTree>&>())
        .def (py::init<const DataTree&>())
        .def ("isValid", &DataTree::isValid)
        .def ("__bool__", &DataTree::isValid)
        .def ("getType", &DataTree::getType)
        .def ("clone", &DataTree::clone)
        .def ("getNumProperties", &DataTree::getNumProperties)
        .def ("getPropertyName", &DataTree::getPropertyName)
        .def ("hasProperty", &DataTree::hasProperty)
        .def ("getProperty", &DataTree::getProperty, "name"_a, "defaultValue"_a = var())
        .def ("getNumChildren", &DataTree::getNumChildren)
        .def ("getChild", &DataTree::getChild)
        .def ("getChildWithName", &DataTree::getChildWithName)
        .def ("indexOf", &DataTree::indexOf)
        .def ("getParent", &DataTree::getParent)
        .def ("getRoot", &DataTree::getRoot)
        .def ("isAChildOf", &DataTree::isAChildOf)
        .def ("getDepth", &DataTree::getDepth)
        .def ("__iter__", [] (const DataTree& self)
        {
            return py::make_iterator (self.begin(), self.end());
        }, py::keep_alive<0, 1>())
        .def ("forEachChild", [] (const DataTree& self, py::function callback)
        {
            self.forEachChild ([&callback] (const DataTree& child)
            {
                py::gil_scoped_acquire acquire;
                auto result = callback (child);
                if (py::isinstance<py::bool_> (result))
                    return result.cast<bool>();
                return false;
            });
        })
        .def ("forEachDescendant", [] (const DataTree& self, py::function callback)
        {
            self.forEachDescendant ([&callback] (const DataTree& child)
            {
                py::gil_scoped_acquire acquire;
                auto result = callback (child);
                if (py::isinstance<py::bool_> (result))
                    return result.cast<bool>();
                return false;
            });
        })
        .def ("findChildren", [] (const DataTree& self, py::function predicate)
        {
            std::vector<DataTree> results;
            self.findChildren (results, [&predicate] (const DataTree& child)
            {
                py::gil_scoped_acquire acquire;
                return predicate (child).cast<bool>();
            });
            return results;
        })
        .def ("findChild", [] (const DataTree& self, py::function predicate)
        {
            return self.findChild ([&predicate] (const DataTree& child)
            {
                py::gil_scoped_acquire acquire;
                return predicate (child).cast<bool>();
            });
        })
        .def ("findDescendants", [] (const DataTree& self, py::function predicate)
        {
            std::vector<DataTree> results;
            self.findDescendants (results, [&predicate] (const DataTree& child)
            {
                py::gil_scoped_acquire acquire;
                return predicate (child).cast<bool>();
            });
            return results;
        })
        .def ("findDescendant", [] (const DataTree& self, py::function predicate)
        {
            return self.findDescendant ([&predicate] (const DataTree& child)
            {
                py::gil_scoped_acquire acquire;
                return predicate (child).cast<bool>();
            });
        })
        .def ("createXml", &DataTree::createXml)
        .def_static ("fromXml", py::overload_cast<const XmlElement&> (&DataTree::fromXml))
        .def ("writeToBinaryStream", &DataTree::writeToBinaryStream)
        .def_static ("readFromBinaryStream", &DataTree::readFromBinaryStream)
        .def ("createJson", &DataTree::createJson)
        .def_static ("fromJson", &DataTree::fromJson)
        .def ("addListener", &DataTree::addListener, py::keep_alive<1, 2>())
        .def ("removeListener", &DataTree::removeListener)
        .def ("removeAllListeners", &DataTree::removeAllListeners)
        .def (py::self == py::self)
        .def (py::self != py::self)
        .def ("isEquivalentTo", &DataTree::isEquivalentTo)
        .def ("beginTransaction", py::overload_cast<UndoManager*> (&DataTree::beginTransaction), "undoManager"_a = nullptr)
        .def ("__repr__", [] (const DataTree& self)
        {
            String result;
            result
                << "<" << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (DataTree).name(), 1)
                << " object at " << String::formatted ("%p", std::addressof (self))
                << " type=\"" << self.getType().toString() << "\">";
            return result;
        });

    // clang-format on
}

} // namespace yup::Bindings
