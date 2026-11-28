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

#include "yup_YupGuiMarkup_bindings.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <typeinfo>

//==============================================================================

namespace yup::Bindings
{

namespace py = pybind11;
using namespace py::literals;

void registerYupGuiMarkupBindings (py::module_& m)
{
    // clang-format off

    // ============================================================================================ yup::YuiParser

    py::class_<YuiParser> classYuiParser (m, "YuiParser");

    // The source map is not exposed: YuiDocument already locates its diagnostics with it.
    py::class_<YuiParser::ParseResult> (classYuiParser, "ParseResult")
        .def_readonly ("document", &YuiParser::ParseResult::document, "The document, invalid when the text has errors.")
        .def_readonly ("diagnostics", &YuiParser::ParseResult::diagnostics, "The problems found, empty when the text was parsed.")
        .def ("wasOk", &YuiParser::ParseResult::wasOk);

    classYuiParser
        .def_static ("parse", py::overload_cast<const String&> (&YuiParser::parse), "text"_a,
                     "Parses .yui text, stopping at the first error. Imports are kept as import elements.")
        .def_static ("parse", py::overload_cast<const String&, const YuiParser::ImportResolver&> (&YuiParser::parse), "text"_a, "resolveImport"_a,
                     "Parses .yui text along with the files it imports. resolveImport is called with each\n"
                     "imported path and returns its text, or None when it can't be found.")
        .def_static ("parseStyleSheet", [] (const String& text) -> StyleSheet::Ptr
        {
            auto sheet = YuiParser::parseStyleSheet (text);
            if (sheet.failed())
                throw py::value_error (sheet.getErrorMessage().toRawUTF8());

            return sheet.getValue();
        }, "text"_a, "Parses .yui text and builds a stylesheet from it, raising ValueError with the first problem when it can't.");

    // ============================================================================================ yup::YuiWriter

    py::class_<YuiWriter> (m, "YuiWriter")
        .def_static ("write", &YuiWriter::write, "tree"_a, "Writes a document, or a single element of one, as .yui text.");

    // ============================================================================================ yup::YuiSvgImporter

    py::class_<YuiSvgImporter> classYuiSvgImporter (m, "YuiSvgImporter");

    py::class_<YuiSvgImporter::Options> (classYuiSvgImporter, "Options")
        .def (py::init<>())
        .def_readwrite ("name", &YuiSvgImporter::Options::name, "The name of the drawing: letters, digits and _, starting with a letter.")
        .def_readwrite ("fontExpression", &YuiSvgImporter::Options::fontExpression,
                        "Called with the family, size, weight and italic flag of a text, returns the expression giving its font.")
        .def_readwrite ("colorProperties", &YuiSvgImporter::Options::colorProperties,
                        "Declares every color the SVG is painted with as a color property of the drawing.");

    py::class_<YuiSvgImporter::ImportResult> (classYuiSvgImporter, "ImportResult")
        .def_readonly ("drawing", &YuiSvgImporter::ImportResult::drawing, "The drawing element, invalid when the SVG can't be read.")
        .def_readonly ("warnings", &YuiSvgImporter::ImportResult::warnings, "What is not painted exactly as a Drawable paints it.")
        .def ("wasOk", &YuiSvgImporter::ImportResult::wasOk)
        .def ("toText", &YuiSvgImporter::ImportResult::toText, "Returns the drawing as .yui text, empty when the SVG wasn't read.");

    // The SVGDocument overload is not bound, as SVGDocument isn't either. The text is taken as a
    // StringRef because a String converts to both a StringRef and a File.
    classYuiSvgImporter
        .def_static ("importSvg", [] (StringRef svgText, const YuiSvgImporter::Options& options)
        {
            return YuiSvgImporter::importSvg (svgText, options);
        }, "svgText"_a, "options"_a = YuiSvgImporter::Options(), "Parses and converts the text of an SVG.")
        .def_static ("importSvg", [] (const File& svgFile, const YuiSvgImporter::Options& options)
        {
            return YuiSvgImporter::importSvg (svgFile, options);
        }, "svgFile"_a, "options"_a = YuiSvgImporter::Options(), "Parses and converts an SVG file.");

    // ============================================================================================ yup::YuiDocument

    // The components of a document are created in C++, and Component's polymorphic_type_hook only
    // downcasts the types registered with registerComponentType: resolving the bound class of the
    // dynamic type here is what makes a Slider written in markup reach Python as a Slider.
    const auto toBorrowedComponent = [] (Component* component) -> py::object
    {
        if (component == nullptr)
            return py::none();

        if (const auto* typeInfo = py::detail::get_type_info (typeid (*component)))
        {
            return py::reinterpret_steal<py::object> (py::detail::type_caster_generic::cast (
                dynamic_cast<const void*> (component), py::return_value_policy::reference, py::handle(), typeInfo, nullptr, nullptr));
        }

        return py::cast (component, py::return_value_policy::reference);
    };

    py::class_<YuiDocument> classYuiDocument (m, "YuiDocument");

    classYuiDocument
        .def (py::init<>())
        .def ("loadFromString", &YuiDocument::loadFromString, "text"_a, "resolveImport"_a = nullptr,
              "Parses .yui text and loads the interface it describes. resolveImport is called with each\n"
              "imported path and returns its text, or None when it can't be found.")
        .def ("load", [] (YuiDocument& self, const DataTree& document)
        {
            return self.load (document);
        }, "document"_a, "Loads the interface of a document already parsed by YuiParser.")
        .def ("clear", &YuiDocument::clear, "Destroys the interface. Actions and models are kept.")
        .def ("setBaseDirectory", &YuiDocument::setBaseDirectory, "directory"_a)
        .def ("getBaseDirectory", &YuiDocument::getBaseDirectory)

        // The document only holds a raw pointer to the undo manager, which has to outlive it
        .def ("setUndoManager", &YuiDocument::setUndoManager, "undoManagerToUse"_a, py::keep_alive<1, 2>(),
              "Records each successful load as a transaction of an undo manager, or stops when None.")
        .def ("getTree", &YuiDocument::getTree)
        .def ("applyPendingChanges", &YuiDocument::applyPendingChanges,
              "Applies the changes made to the tree of the document and to the ranges of its repeats now, rather than from the message loop.")

        // The document owns its components: Python borrows them, and keeps the document alive meanwhile
        .def ("getRoot", [toBorrowedComponent] (const YuiDocument& self)
        {
            return toBorrowedComponent (self.getRoot());
        }, py::keep_alive<0, 1>(), "Returns the root component, or None if nothing is loaded.")
        .def ("findById", [toBorrowedComponent] (const YuiDocument& self, const String& id)
        {
            return toBorrowedComponent (self.findById (id));
        }, "id"_a, py::keep_alive<0, 1>(), "Returns the component with the given id, or None.")
        .def ("findTypeInfo", &YuiDocument::findTypeInfo, "typeName"_a, py::return_value_policy::reference_internal,
              "Returns the type info of a type declared by the document or registered, or None.")
        .def ("setAction", [] (YuiDocument& self, const String& name, std::function<void (py::list)> action)
        {
            if (! action)
            {
                self.setAction (name, nullptr);
                return;
            }

            self.setAction (name, [action = std::move (action)] (const Array<var>& arguments)
            {
                py::gil_scoped_acquire gil;

                py::list values;
                for (const auto& argument : arguments)
                    values.append (argument);

                action (std::move (values));
            });
        }, "name"_a, "action"_a,
              "Registers the function run by action (\"name\", ...) handlers, called with the list of the\n"
              "values of the call, or removes it when None.")

        // The document only holds a reference to the provider, which has to outlive it
        .def ("setModel", &YuiDocument::setModel, "name"_a, "provider"_a, py::keep_alive<1, 3>(),
              "Makes the values of a provider available as name.property. Set models before loading.")
        .def ("removeModel", &YuiDocument::removeModel, "name"_a)
        .def ("getDiagnostics", &YuiDocument::getDiagnostics, py::return_value_policy::copy,
              "Returns the problems met by the last load, and by the styling or bindings of its interface since.")
        .def ("getStyleSheet", &YuiDocument::getStyleSheet,
              "Returns the stylesheet built from the document, or None when it declares no style.");

    // ============================================================================================ yup::YuiLoader

    py::class_<YuiLoader> classYuiLoader (m, "YuiLoader");

    classYuiLoader
        // The loader only holds a reference to its document, which has to outlive it
        .def (py::init<YuiDocument&>(), "documentToLoad"_a, py::keep_alive<1, 2>())
        .def ("loadFile", &YuiLoader::loadFile, "file"_a, "Loads a .yui file, whose imports are paths relative to its folder.")

        // The loader reads the bytes as soon as the finder returns, and may call it again on a later
        // reload: they are copied out of the Python object, and kept until the next lookup.
        .def ("loadResource", [] (YuiLoader& self, const String& path, std::function<std::optional<std::string> (const String&)> findResource)
        {
            if (! findResource)
                return self.loadResource (path, nullptr);

            return self.loadResource (path, [findResource = std::move (findResource), found = std::make_shared<std::string>()] (const char* resourcePath, std::size_t& size) -> const uint8*
            {
                auto bytes = findResource (String::fromUTF8 (resourcePath));
                if (! bytes.has_value())
                    return nullptr;

                *found = std::move (*bytes);
                size = found->size();

                return reinterpret_cast<const uint8*> (found->data());
            });
        }, "path"_a, "findResource"_a,
              "Loads an embedded .yui file. findResource is called with the path of the file and of each\n"
              "of its imports, and returns their bytes, or None when they can't be found.")
        .def ("reloadFromString", &YuiLoader::reloadFromString, "text"_a,
              "Loads text in place of the main text of the current source, importing from the source.")
        .def ("reload", &YuiLoader::reload, "Loads the current source again.")
        .def ("setOverride", &YuiLoader::setOverride, "path"_a, "text"_a,
              "Uses a text in place of a file of the current source, until another source is loaded.")
        .def ("clearOverrides", &YuiLoader::clearOverrides)
        .def ("getLoadedText", &YuiLoader::getLoadedText, "path"_a,
              "Returns the text a file had in the last load, or None when the last load didn't use it.")
        .def ("getFile", &YuiLoader::getFile)
        .def ("setWatching", &YuiLoader::setWatching, "shouldWatch"_a,
              "Starts or stops reloading the document when the files it was loaded from change.")
        .def ("isWatching", &YuiLoader::isWatching)
        .def_readwrite ("onLoad", &YuiLoader::onLoad, "Called with the result of each load, including the reloads started by watching.");

    // ============================================================================================ yup::YuiView

    py::class_<YuiView, Component, PyComponent<YuiView>, py::smart_holder> classYuiView (m, "YuiView");

    classYuiView
        .def (py::init<StringRef>(), "componentID"_a = StringRef())
        .def ("getDocument", &YuiView::getDocument, py::return_value_policy::reference_internal,
              "Returns the document whose interface the view shows, which lives as long as the view.")
        .def ("getLoader", &YuiView::getLoader, py::return_value_policy::reference_internal,
              "Returns the loader filling the document, which lives as long as the view.");

    // clang-format on
}

} // namespace yup::Bindings
