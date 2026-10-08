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

using Layout = WgslBindingLayout;

constexpr int maxWgslCallDepth = 8;

bool isWgslIdentifierChar (char c) noexcept
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

std::string_view trimWgsl (std::string_view text) noexcept
{
    constexpr std::string_view whitespace = " \t\r\n";

    const auto first = text.find_first_not_of (whitespace);
    if (first == std::string_view::npos)
        return {};

    return text.substr (first, text.find_last_not_of (whitespace) - first + 1);
}

/** Replaces line and (nested) block comments with spaces, keeping every other offset. */
std::string stripWgslComments (std::string text)
{
    for (size_t i = 0; i + 1 < text.size();)
    {
        if (text[i] == '/' && text[i + 1] == '/')
        {
            for (; i < text.size() && text[i] != '\n'; ++i)
                text[i] = ' ';
        }
        else if (text[i] == '/' && text[i + 1] == '*')
        {
            int depth = 0;
            do
            {
                if (i + 1 < text.size() && text[i] == '/' && text[i + 1] == '*')
                {
                    ++depth;
                    text[i++] = ' ';
                }
                else if (i + 1 < text.size() && text[i] == '*' && text[i + 1] == '/')
                {
                    --depth;
                    text[i++] = ' ';
                }

                if (text[i] != '\n')
                    text[i] = ' ';

                ++i;
            } while (depth > 0 && i < text.size());
        }
        else
        {
            ++i;
        }
    }

    return text;
}

/** Calls @p visit with the offset of every whole-word occurrence of @p name in @p text that is not a member access. */
template <class Visitor>
void forEachWgslOccurrence (std::string_view text, std::string_view name, Visitor&& visit)
{
    for (auto pos = text.find (name); pos != std::string_view::npos; pos = text.find (name, pos + 1))
    {
        const auto end = pos + name.size();
        if ((pos > 0 && isWgslIdentifierChar (text[pos - 1])) || (end < text.size() && isWgslIdentifierChar (text[end])))
            continue;

        const auto previous = text.find_last_not_of (" \t\r\n", pos == 0 ? 0 : pos - 1);
        if (pos > 0 && previous != std::string_view::npos && text[previous] == '.')
            continue;

        visit (pos);
    }
}

/** Returns the value N of an `@name(N)` attribute in @p attributes. */
std::optional<uint32> wgslAttributeValue (std::string_view attributes, std::string_view name)
{
    for (auto pos = attributes.find ('@'); pos != std::string_view::npos; pos = attributes.find ('@', pos + 1))
    {
        auto rest = attributes.substr (pos + 1);
        if (! rest.starts_with (name) || (rest.size() > name.size() && isWgslIdentifierChar (rest[name.size()])))
            continue;

        rest = trimWgsl (rest.substr (name.size()));
        if (! rest.starts_with ("("))
            continue;

        const auto digits = trimWgsl (rest.substr (1, rest.find (')') - 1));
        if (digits.empty() || digits.find_first_not_of ("0123456789") != std::string_view::npos)
            continue;

        uint32 value = 0;
        for (const char digit : digits)
            value = value * 10 + static_cast<uint32> (digit - '0');

        return value;
    }

    return std::nullopt;
}

/** Splits a comma separated list at its top level, ignoring commas nested in (), [] or <>. */
std::vector<std::string_view> splitWgslList (std::string_view list)
{
    std::vector<std::string_view> items;
    int depth = 0;
    size_t start = 0;

    for (size_t i = 0; i < list.size(); ++i)
    {
        const char c = list[i];
        if (c == '(' || c == '[' || c == '<')
            ++depth;
        else if (c == ')' || c == ']' || c == '>')
            --depth;
        else if (c == ',' && depth == 0)
        {
            items.push_back (trimWgsl (list.substr (start, i - start)));
            start = i + 1;
        }
    }

    if (const auto last = trimWgsl (list.substr (start)); ! last.empty())
        items.push_back (last);

    return items;
}

/** Returns the offset just past the bracket matching the one at @p open, or npos. */
size_t matchWgslBracket (std::string_view text, size_t open, char openChar, char closeChar)
{
    int depth = 0;
    for (size_t i = open; i < text.size(); ++i)
    {
        if (text[i] == openChar)
            ++depth;
        else if (text[i] == closeChar && --depth == 0)
            return i + 1;
    }

    return std::string_view::npos;
}

/** True when @p name, within @p scope, reaches a sampling call, directly or through the parameters of the module's functions. */
bool isWgslTextureSampled (std::string_view module, std::string_view scope, std::string_view name, int callDepth)
{
    if (callDepth > maxWgslCallDepth)
        return false;

    bool sampled = false;

    forEachWgslOccurrence (scope, name, [&] (size_t pos)
    {
        if (sampled)
            return;

        // Walk back to the call this occurrence is an argument of, counting the arguments before it
        int depth = 0;
        int argumentIndex = 0;
        size_t i = pos;
        bool inCall = false;

        while (i > 0)
        {
            const char c = scope[--i];

            if (c == ')' || c == ']')
                ++depth;
            else if (c == '[' && depth > 0)
                --depth;
            else if (c == '(' && depth > 0)
                --depth;
            else if (c == ',' && depth == 0)
                ++argumentIndex;
            else if (c == '(' && depth == 0)
            {
                inCall = true;
                break;
            }
            else if ((c == ';' || c == '{' || c == '}') && depth == 0)
                break;
        }

        if (! inCall)
            return;

        auto callee = trimWgsl (scope.substr (0, i));
        size_t calleeStart = callee.size();
        while (calleeStart > 0 && isWgslIdentifierChar (callee[calleeStart - 1]))
            --calleeStart;
        callee = callee.substr (calleeStart);

        if ((callee.starts_with ("textureSample") && argumentIndex == 0) || (callee == "textureGather" && argumentIndex == 1))
        {
            sampled = true;
            return;
        }

        if (callee.empty())
            return;

        // A function of the module, even one named like a builtin (textureBicubic): follow the argument
        // into the matching parameter of its body. Builtins such as textureLoad have no definition to follow.
        forEachWgslOccurrence (module, callee, [&] (size_t fnPos)
        {
            if (sampled)
                return;

            const auto before = trimWgsl (module.substr (0, fnPos));
            if (! before.ends_with ("fn") || (before.size() > 2 && isWgslIdentifierChar (before[before.size() - 3])))
                return;

            const auto open = module.find ('(', fnPos);
            const auto close = open == std::string_view::npos ? open : matchWgslBracket (module, open, '(', ')');
            const auto bodyOpen = close == std::string_view::npos ? close : module.find ('{', close);
            const auto bodyClose = bodyOpen == std::string_view::npos ? bodyOpen : matchWgslBracket (module, bodyOpen, '{', '}');
            if (bodyClose == std::string_view::npos)
                return;

            const auto parameters = splitWgslList (module.substr (open + 1, close - open - 2));
            if (argumentIndex >= static_cast<int> (parameters.size()))
                return;

            auto parameter = parameters[static_cast<size_t> (argumentIndex)];
            parameter = trimWgsl (parameter.substr (0, parameter.find (':')));
            size_t parameterStart = parameter.size();
            while (parameterStart > 0 && isWgslIdentifierChar (parameter[parameterStart - 1]))
                --parameterStart;

            const auto body = module.substr (bodyOpen, bodyClose - bodyOpen);
            sampled = isWgslTextureSampled (module, body, parameter.substr (parameterStart), callDepth + 1);
        });
    });

    return sampled;
}

/** True when @p name appears in @p module outside the declaration spanning [declStart, declEnd),
    other than as the name of a struct member, parameter or local (`name: type`). */
bool isWgslNameReferenced (std::string_view module, std::string_view name, size_t declStart, size_t declEnd)
{
    bool referenced = false;
    forEachWgslOccurrence (module, name, [&] (size_t pos)
    {
        const auto next = module.find_first_not_of (" \t\r\n", pos + name.size());
        const bool declaresName = next != std::string_view::npos && module[next] == ':';
        referenced = referenced || ((pos < declStart || pos >= declEnd) && ! declaresName);
    });
    return referenced;
}

std::optional<Layout::ViewDimension> toViewDimension (std::string_view dimension)
{
    if (dimension == "1d")
        return Layout::ViewDimension::texture1D;
    if (dimension == "2d")
        return Layout::ViewDimension::texture2D;
    if (dimension == "2d_array")
        return Layout::ViewDimension::array2D;
    if (dimension == "cube")
        return Layout::ViewDimension::cube;
    if (dimension == "cube_array")
        return Layout::ViewDimension::cubeArray;
    if (dimension == "3d")
        return Layout::ViewDimension::texture3D;

    return std::nullopt;
}

/** Fills the type-dependent fields of @p entry from its WGSL type, or returns false for a type a bind group cannot hold. */
bool parseWgslResourceType (std::string_view module, std::string_view type, std::string_view name, Layout::Entry& entry)
{
    if (type == "sampler" || type == "sampler_comparison")
    {
        entry.kind = Layout::ResourceKind::sampler;
        entry.samplerType = type == "sampler" ? Layout::SamplerType::filtering : Layout::SamplerType::comparison;
        return true;
    }

    if (! type.starts_with ("texture_"))
        return false;

    auto shape = type.substr (8);
    std::vector<std::string_view> parameters;

    if (const auto templateStart = shape.find ('<'); templateStart != std::string_view::npos)
    {
        parameters = splitWgslList (shape.substr (templateStart + 1, shape.rfind ('>') - templateStart - 1));
        shape = trimWgsl (shape.substr (0, templateStart));
    }

    const bool depth = shape.starts_with ("depth_");
    if (depth)
        shape.remove_prefix (6);

    if (shape.starts_with ("storage_"))
    {
        const auto dimension = toViewDimension (shape.substr (8));
        if (! dimension || parameters.size() != 2)
            return false;

        entry.kind = Layout::ResourceKind::storageTexture;
        entry.viewDimension = *dimension;
        entry.storageFormat = String (std::string (parameters[0]));

        if (parameters[1] == "read")
            entry.storageAccess = Layout::StorageAccess::readOnly;
        else if (parameters[1] == "write")
            entry.storageAccess = Layout::StorageAccess::writeOnly;
        else if (parameters[1] == "read_write")
            entry.storageAccess = Layout::StorageAccess::readWrite;
        else
            return false;

        return true;
    }

    entry.kind = Layout::ResourceKind::texture;
    entry.multisampled = shape.starts_with ("multisampled_");
    if (entry.multisampled)
        shape.remove_prefix (13);

    const auto dimension = toViewDimension (shape);
    if (! dimension)
        return false;

    entry.viewDimension = *dimension;

    if (depth)
        entry.sampleType = Layout::SampleType::depth;
    else if (parameters.size() == 1 && parameters[0] == "i32")
        entry.sampleType = Layout::SampleType::sint;
    else if (parameters.size() == 1 && parameters[0] == "u32")
        entry.sampleType = Layout::SampleType::uint;
    else if (parameters.size() == 1 && parameters[0] == "f32")
        entry.sampleType = ! entry.multisampled && isWgslTextureSampled (module, module, name, 0) ? Layout::SampleType::floatFilterable
                                                                                                : Layout::SampleType::floatUnfilterable;
    else
        return false;

    return true;
}

} // namespace

//==============================================================================

ResultValue<std::vector<WgslBindingLayout::Entry>> WgslBindingLayout::parse (const String& wgsl)
{
    const auto source = stripWgslComments (wgsl.toStdString());
    const std::string_view module = source;

    std::vector<Entry> declarations;
    std::vector<std::pair<size_t, size_t>> spans;
    String unsupported;

    forEachWgslOccurrence (module, "var", [&] (size_t varPos)
    {
        if (unsupported.isNotEmpty())
            return;

        auto attributesStart = module.find_last_of (";{}", varPos);
        attributesStart = attributesStart == std::string_view::npos ? 0 : attributesStart + 1;

        const auto attributes = module.substr (attributesStart, varPos - attributesStart);
        const auto group = wgslAttributeValue (attributes, "group");
        const auto binding = wgslAttributeValue (attributes, "binding");
        const auto end = module.find (';', varPos);
        if (! group || ! binding || end == std::string_view::npos)
            return;

        auto rest = trimWgsl (module.substr (varPos + 3, end - varPos - 3));
        std::string_view addressSpace;

        if (rest.starts_with ("<"))
        {
            const auto close = rest.find ('>');
            addressSpace = rest.substr (1, close - 1);
            rest = trimWgsl (rest.substr (close + 1));
        }

        const auto colon = rest.find (':');
        if (colon == std::string_view::npos)
            return;

        const auto name = trimWgsl (rest.substr (0, colon));
        const auto type = trimWgsl (rest.substr (colon + 1));

        Entry entry;
        entry.name = String (std::string (name));
        entry.group = *group;
        entry.binding = *binding;

        bool supported = true;

        if (addressSpace.empty())
        {
            supported = parseWgslResourceType (module, type, name, entry);
        }
        else
        {
            const auto parts = splitWgslList (addressSpace);
            const auto access = parts.size() > 1 ? parts[1] : std::string_view ("read");

            entry.kind = ResourceKind::buffer;

            if (parts.empty())
                supported = false;
            else if (parts[0] == "uniform")
                entry.bufferType = BufferType::uniform;
            else if (parts[0] == "storage")
                entry.bufferType = access == "read" ? BufferType::readOnlyStorage : BufferType::storage;
            else
                supported = false;
        }

        if (! supported)
        {
            unsupported = entry.name;
            return;
        }

        declarations.push_back (std::move (entry));
        spans.emplace_back (attributesStart, end);
    });

    if (unsupported.isNotEmpty())
        return makeResultValueFail ("The WGSL resource '" + unsupported + "' has a type a bind group cannot hold");

    std::vector<Entry> result;

    for (size_t i = 0; i < declarations.size(); ++i)
    {
        auto& entry = declarations[i];

        auto existing = std::find_if (result.begin(), result.end(), [&] (const auto& other)
        {
            return other.group == entry.group && other.binding == entry.binding;
        });

        if (existing == result.end())
            result.push_back (std::move (entry));
        else if (isWgslNameReferenced (module, entry.name.toStdString(), spans[i].first, spans[i].second))
            *existing = std::move (entry);
    }

    return makeResultValueOk (std::move (result));
}

} // namespace yup
