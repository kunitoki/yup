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

namespace yup
{

namespace
{

//==============================================================================
// Each kind of block holds its own kind of members, so every writer below handles the members of one context
class YuiTextWriter
{
public:
    String write (const DataTree& tree)
    {
        if (tree.getType() != Identifier ("yui"))
        {
            writeBlock (tree);
            return out.toString();
        }

        bool previousWasImport = false;

        for (const auto& block : tree)
        {
            const auto isImport = block.getType() == Identifier ("import");

            // Blocks are separated by a blank line, consecutive imports stay together
            if (out.getDataSize() > 0 && ! (isImport && previousWasImport))
                out << "\n";

            writeBlock (block);
            previousWasImport = isImport;
        }

        return out.toString();
    }

private:
    using MemberWriter = void (YuiTextWriter::*) (const DataTree&, int);

    void writeBlock (const DataTree& block)
    {
        const auto type = block.getType().toString();

        if (type == "import")
        {
            writeLine (0, "import " + quoted (text (block, "path")) + ";");
        }
        else if (type == "palette")
        {
            writePalette (block);
        }
        else if (type == "drawing")
        {
            const auto name = text (block, "name");
            writeElement (name.isEmpty() ? type : type + " " + name, block, 0, { "name" }, &YuiTextWriter::writeBodyMember);
        }
        else if (type == "tokens")
        {
            writeElement (type, block, 0, {}, &YuiTextWriter::writeDeclaredMember);
        }
        else if (type == "style")
        {
            writeElement (type, block, 0, {}, &YuiTextWriter::writeStyleMember);
        }
        else if (type == "component")
        {
            writeComponent (block);
        }
        else
        {
            writeBodyMember (block, 0);
        }
    }

    void writePalette (const DataTree& palette)
    {
        writeLine (0, "palette " + text (palette, "name") + " {");

        for (int i = 0; i < palette.getNumProperties(); ++i)
        {
            const auto name = palette.getPropertyName (i);

            if (name != Identifier ("name"))
                writeLine (1, name.toString() + ": " + paletteValue (palette.getProperty (name).toString()) + ";");
        }

        writeLine (0, "}");
    }

    void writeComponent (const DataTree& component)
    {
        const auto base = text (component, "base");

        writeLine (0, "component " + text (component, "name") + " : " + base + " {");

        // The declarations come first, then the contents of the body, which is the element of the base type
        DataTree body;

        for (const auto& child : component)
        {
            if (child.getType().toString() == base)
                body = child;
            else
                writeDeclaredMember (child, 1);
        }

        if (body.isValid())
            writeContents (body, 1, {}, &YuiTextWriter::writeBodyMember);

        writeLine (0, "}");
    }

    // The members of drawings and component instances
    void writeBodyMember (const DataTree& element, int depth)
    {
        const auto type = element.getType().toString();

        if (type == "let")
        {
            writeLine (depth, "let " + text (element, "name") + ": " + text (element, "value") + ";");
        }
        else if (type == "property")
        {
            writePropertyDeclaration (element, depth);
        }
        else if (type == "repeat")
        {
            const auto header = "repeat " + text (element, "name") + " in " + text (element, "from") + " .. " + text (element, "to");
            writeElement (header, element, depth, { "name", "from", "to" }, &YuiTextWriter::writeBodyMember);
        }
        else if (type == "use")
        {
            writeElement ("use " + text (element, "drawing"), element, depth, { "drawing" }, &YuiTextWriter::writeBodyMember);
        }
        else if ((type == "drawing" || type == "pattern" || type == "marker") && element.hasProperty ("name"))
        {
            writeElement (type + " " + text (element, "name"), element, depth, { "name" }, &YuiTextWriter::writeBodyMember);
        }
        else if (type == "bind")
        {
            writeLine (depth, text (element, "property") + " <=> " + text (element, "target") + ";");
        }
        else if (type == "changed")
        {
            writeLine (depth, "changed " + text (element, "property") + ": " + text (element, "handler") + ";");
        }
        else
        {
            writeElement (type, element, depth, {}, &YuiTextWriter::writeBodyMember);
        }
    }

    // The members of tokens and the declarations of components
    void writeDeclaredMember (const DataTree& element, int depth)
    {
        const auto type = element.getType().toString();

        if (type == "alias")
        {
            writeLine (depth, "property alias " + text (element, "name") + " <=> " + text (element, "target") + ";");
        }
        else if (type == "signal")
        {
            StringArray arguments;

            for (const auto& argument : element)
                arguments.add (text (argument, "name") + ": " + text (argument, "type"));

            const auto list = arguments.isEmpty() ? String() : " (" + arguments.joinIntoString (", ") + ")";
            writeLine (depth, "signal " + text (element, "name") + list + ";");
        }
        else
        {
            writePropertyDeclaration (element, depth);
        }
    }

    void writeStyleMember (const DataTree& element, int depth)
    {
        if (element.getType() == Identifier ("keyframes"))
            writeElement ("@keyframes " + text (element, "name"), element, depth, { "name" }, &YuiTextWriter::writeKeyframe);
        else
            writeElement (text (element, "selector"), element, depth, { "selector" }, &YuiTextWriter::writeRuleMember);
    }

    void writeKeyframe (const DataTree& keyframe, int depth)
    {
        writeElement (keyframePosition (keyframe.getProperty ("at")), keyframe, depth, { "at" }, &YuiTextWriter::writeRuleMember);
    }

    // The members of style rules and keyframes
    void writeRuleMember (const DataTree& element, int depth)
    {
        if (element.getType() == Identifier ("property"))
            writePropertyDeclaration (element, depth);
        else
            writeLine (depth, text (element, "name") + ": " + text (element, "value") + ";");
    }

    void writePropertyDeclaration (const DataTree& property, int depth)
    {
        const auto value = property.hasProperty ("value") ? " = " + text (property, "value") : String();
        writeLine (depth, "property " + text (property, "name") + ": " + text (property, "type") + value + ";");
    }

    //==============================================================================
    void writeElement (const String& header, const DataTree& element, int depth, const StringArray& skipped, MemberWriter writeMember)
    {
        if (! hasContents (element, skipped))
        {
            writeLine (depth, header + " {}");
            return;
        }

        writeLine (depth, header + " {");
        writeContents (element, depth + 1, skipped, writeMember);
        writeLine (depth, "}");
    }

    void writeContents (const DataTree& element, int depth, const StringArray& skipped, MemberWriter writeMember)
    {
        for (int i = 0; i < element.getNumProperties(); ++i)
        {
            const auto name = element.getPropertyName (i).toString();

            if (! skipped.contains (name))
                writeLine (depth, name + ": " + element.getProperty (name).toString() + ";");
        }

        for (const auto& child : element)
            (this->*writeMember) (child, depth);
    }

    void writeLine (int depth, const String& line)
    {
        for (int i = 0; i < depth; ++i)
            out << "    ";

        out << line << "\n";
    }

    static bool hasContents (const DataTree& element, const StringArray& skipped)
    {
        if (element.getNumChildren() > 0)
            return true;

        for (int i = 0; i < element.getNumProperties(); ++i)
        {
            if (! skipped.contains (element.getPropertyName (i).toString()))
                return true;
        }

        return false;
    }

    static String text (const DataTree& element, const Identifier& name)
    {
        return element.getProperty (name).toString();
    }

    static String quoted (const String& value)
    {
        const char quote = (value.containsChar ('"') && ! value.containsChar ('\'')) ? '\'' : '"';
        String escaped;

        for (auto ptr = value.getCharPointer(); ! ptr.isEmpty();)
        {
            const auto c = ptr.getAndAdvance();

            if (c == '\\')
                escaped << "\\\\";
            else if (c == static_cast<yup_wchar> (quote))
                escaped << '\\' << String::charToString (c);
            else if (c == '\n')
                escaped << "\\n";
            else if (c == '\r')
                escaped << "\\r";
            else if (c == '\t')
                escaped << "\\t";
            else
                escaped << String::charToString (c);
        }

        const auto delimiter = String::charToString (static_cast<yup_wchar> (quote));
        return delimiter + escaped + delimiter;
    }

    // Palette values are read verbatim with the quotes removed, so anything that can't be read back raw is quoted
    static String paletteValue (const String& value)
    {
        if (value.isEmpty()
            || value != value.trim()
            || value.containsAnyOf (";{}()[]\"'\\\r\n\t")
            || value.contains ("//")
            || value.contains ("/*"))
        {
            return quoted (value);
        }

        return value;
    }

    static String keyframePosition (double at)
    {
        if (at <= 0.0)
            return "from";

        if (at >= 1.0)
            return "to";

        return String (at * 100.0, 6).trimCharactersAtEnd ("0").trimCharactersAtEnd (".") + "%";
    }

    MemoryOutputStream out;
};

} // namespace

//==============================================================================

String YuiWriter::write (const DataTree& tree)
{
    return YuiTextWriter().write (tree);
}

} // namespace yup
