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

#include <gtest/gtest.h>

#include <yup_core/yup_core.h>

#include <limits>

using namespace yup;

class YAMLTests : public ::testing::Test
{
protected:
    Random random;

    String createRandomWideCharString()
    {
        yup_wchar buffer[40] = { 0 };

        for (int i = 0; i < numElementsInArray (buffer) - 1; ++i)
        {
            if (random.nextBool())
            {
                do
                {
                    buffer[i] = static_cast<yup_wchar> (1 + random.nextInt (0x10ffff - 1));
                } while (! CharPointer_UTF16::canRepresent (buffer[i]));
            }
            else
            {
                buffer[i] = static_cast<yup_wchar> (1 + random.nextInt (0xff));
            }
        }

        return CharPointer_UTF32 (buffer);
    }

    String createRandomIdentifier()
    {
        char buffer[30] = { 0 };

        for (int i = 0; i < numElementsInArray (buffer) - 1; ++i)
        {
            static const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-:";
            buffer[i] = chars[random.nextInt (sizeof (chars) - 1)];
        }

        return CharPointer_ASCII (buffer);
    }

    var createRandomDouble()
    {
        return var ((random.nextDouble() * 1000.0) + 0.1);
    }

    var createRandomVar (int depth)
    {
        switch (random.nextInt (depth > 3 ? 6 : 8))
        {
            case 0:
                return {};
            case 1:
                return random.nextInt();
            case 2:
                return random.nextInt64();
            case 3:
                return random.nextBool();
            case 4:
                return createRandomDouble();
            case 5:
                return createRandomWideCharString();

            case 6:
            {
                var v (createRandomVar (depth + 1));

                for (int i = 1 + random.nextInt (5); --i >= 0;)
                    v.append (createRandomVar (depth + 1));

                return v;
            }

            case 7:
            {
                auto o = new DynamicObject();

                for (int i = random.nextInt (5); --i >= 0;)
                    o->setProperty (createRandomIdentifier(), createRandomVar (depth + 1));

                return o;
            }

            default:
                return {};
        }
    }
};

TEST_F (YAMLTests, ParseAndGenerate)
{
    EXPECT_EQ (YAML::parse (String()), var());
    EXPECT_TRUE (YAML::parse ("{}").isObject());
    EXPECT_TRUE (YAML::parse ("[]").isArray());

    for (int i = 100; --i >= 0;)
    {
        var v = createRandomVar (0);
        bool oneLine = random.nextBool();

        String asString = YAML::toString (v, oneLine);
        var parsed = (v.isObject() || v.isArray()) ? YAML::parse (asString) : YAML::fromString (asString);
        String parsedString = YAML::toString (parsed, oneLine);

        EXPECT_FALSE (asString.isEmpty());
        //EXPECT_EQ (parsedString, asString);
        if (parsedString != asString)
        {
            EXPECT_TRUE (false);
            std::cout << "Original:\n"
                      << asString << "\nParsed:\n"
                      << parsedString << std::endl;
            break;
        }
    }
}

TEST_F (YAMLTests, ParseSimpleTypes)
{
    auto v = YAML::parse (R"(name: yup
version: 2.0
active: true
nothing: null
count: 42
ratio: 3.5)");

    EXPECT_TRUE (v.isObject());
    EXPECT_EQ ("yup", v["name"].toString());
    EXPECT_DOUBLE_EQ (2.0, static_cast<double> (v["version"]));
    EXPECT_TRUE (static_cast<bool> (v["active"]));
    EXPECT_TRUE (v["nothing"].isVoid());
    EXPECT_EQ (42, static_cast<int> (v["count"]));
    EXPECT_DOUBLE_EQ (3.5, static_cast<double> (v["ratio"]));
}

TEST_F (YAMLTests, TypeResolution)
{
    EXPECT_TRUE (YAML::fromString ("null").isVoid());
    EXPECT_TRUE (YAML::fromString ("~").isVoid());
    EXPECT_TRUE (YAML::fromString ("NULL").isVoid());

    EXPECT_TRUE (YAML::fromString ("true").isBool());
    EXPECT_TRUE (static_cast<bool> (YAML::fromString ("TRUE")));
    EXPECT_FALSE (static_cast<bool> (YAML::fromString ("false")));

    EXPECT_TRUE (YAML::fromString ("42").isInt());
    EXPECT_TRUE (YAML::fromString ("-17").isInt());
    EXPECT_TRUE (YAML::fromString ("+8").isInt());
    EXPECT_EQ (31, static_cast<int> (YAML::fromString ("0x1F")));
    EXPECT_EQ (15, static_cast<int> (YAML::fromString ("0o17")));
    EXPECT_EQ (1000, static_cast<int> (YAML::fromString ("1_000")));

    EXPECT_TRUE (YAML::fromString ("3.14").isDouble());
    EXPECT_TRUE (YAML::fromString ("1e5").isDouble());
    EXPECT_TRUE (YAML::fromString ("1.5e-3").isDouble());
    EXPECT_TRUE (YAML::fromString (".5").isDouble());
    EXPECT_TRUE (YAML::fromString ("1.").isDouble());
    EXPECT_FALSE (yup_isfinite (static_cast<double> (YAML::fromString (".inf"))));
    EXPECT_FALSE (yup_isfinite (static_cast<double> (YAML::fromString ("-.nan"))));

    EXPECT_TRUE (YAML::fromString ("hello world").isString());
    EXPECT_TRUE (YAML::fromString ("yes").isBool()); // YAML 1.1 booleans are resolved
    EXPECT_FALSE (static_cast<bool> (YAML::fromString ("no")));
    EXPECT_TRUE (static_cast<bool> (YAML::fromString ("on")));
    EXPECT_FALSE (static_cast<bool> (YAML::fromString ("off")));
    EXPECT_TRUE (YAML::fromString ("123abc").isString());
    EXPECT_TRUE (YAML::fromString ("1:30").isString());
    EXPECT_TRUE (YAML::fromString ("0x").isString());
    EXPECT_EQ ("0x", YAML::fromString ("0x").toString());
}

TEST_F (YAMLTests, NestedBlockCollections)
{
    auto v = YAML::parse (R"(person:
  name: John
  age: 30
  hobbies:
    - reading
    - hiking
address:
  city: Milan)");

    EXPECT_TRUE (v.isObject());
    EXPECT_TRUE (v["person"].isObject());
    EXPECT_EQ ("John", v["person"]["name"].toString());
    EXPECT_EQ (30, static_cast<int> (v["person"]["age"]));
    EXPECT_TRUE (v["person"]["hobbies"].isArray());
    EXPECT_EQ (2, v["person"]["hobbies"].size());
    EXPECT_EQ ("reading", v["person"]["hobbies"][0].toString());
    EXPECT_EQ ("hiking", v["person"]["hobbies"][1].toString());
    EXPECT_EQ ("Milan", v["address"]["city"].toString());
}

TEST_F (YAMLTests, NestedBlockValueAfterBlankLine)
{
    // A blank line between a key with no inline value and its nested block must
    // not be mistaken for the nested content itself.
    auto v = YAML::parse ("key:\n\n  nested: value");

    EXPECT_TRUE (v.isObject());
    EXPECT_TRUE (v["key"].isObject());
    EXPECT_EQ ("value", v["key"]["nested"].toString());
}

TEST_F (YAMLTests, SequenceOfMaps)
{
    auto v = YAML::parse (R"(- name: a
  value: 1
- name: b
  value: 2)");

    EXPECT_TRUE (v.isArray());
    EXPECT_EQ (2, v.size());
    EXPECT_TRUE (v[0].isObject());
    EXPECT_EQ ("a", v[0]["name"].toString());
    EXPECT_EQ (1, static_cast<int> (v[0]["value"]));
    EXPECT_EQ ("b", v[1]["name"].toString());
    EXPECT_EQ (2, static_cast<int> (v[1]["value"]));
}

TEST_F (YAMLTests, NestedSequences)
{
    auto v = YAML::parse (R"(- - 1
  - 2
- 3)");

    EXPECT_TRUE (v.isArray());
    EXPECT_EQ (2, v.size());
    EXPECT_TRUE (v[0].isArray());
    EXPECT_EQ (1, static_cast<int> (v[0][0]));
    EXPECT_EQ (2, static_cast<int> (v[0][1]));
    EXPECT_EQ (3, static_cast<int> (v[1]));

    EXPECT_EQ ("- - 1\n  - 2\n- 3", YAML::toString (v));
}

TEST_F (YAMLTests, FlowCollections)
{
    auto v = YAML::parse (R"({name: yup, list: [1, 2, 3], nested: {a: true}})");

    EXPECT_TRUE (v.isObject());
    EXPECT_EQ ("yup", v["name"].toString());
    EXPECT_EQ (3, v["list"].size());
    EXPECT_EQ (1, static_cast<int> (v["list"][0]));
    EXPECT_TRUE (static_cast<bool> (v["nested"]["a"]));

    auto multiLine = YAML::parse (R"({
  a: 1,
  b: [
    2,
    3
  ]
})");

    EXPECT_EQ (1, static_cast<int> (multiLine["a"]));
    EXPECT_EQ (2, multiLine["b"].size());
    EXPECT_EQ (3, static_cast<int> (multiLine["b"][1]));
}

TEST_F (YAMLTests, FlowCollectionFollowedByMoreContent)
{
    // A multi-line flow collection must not swallow the content that follows it.
    auto v = YAML::parse ("key1: {\n  a: 1\n}\nkey2: value");

    EXPECT_TRUE (v.isObject());
    EXPECT_TRUE (v["key1"].isObject());
    EXPECT_EQ (1, static_cast<int> (v["key1"]["a"]));
    EXPECT_EQ ("value", v["key2"].toString());
}

TEST_F (YAMLTests, FlowComments)
{
    auto v = YAML::parse ("{a: 1 # comment\n, b: 2} # done");
    EXPECT_EQ (1, static_cast<int> (v["a"]));
    EXPECT_EQ (2, static_cast<int> (v["b"]));
}

TEST_F (YAMLTests, FlowQuotedUnpairedHighSurrogateEscape)
{
    // Same defect as QuotedScalars' unpaired-surrogate case, but reached through a
    // quoted scalar inside a flow collection (a distinct code path: parseFlowQuoted).
    auto v = YAML::parse ("{k: \"\\uD800\\u0041\"}");

    auto chars = v["k"].toString().getCharPointer();
    EXPECT_EQ ((yup_wchar) 0xd800, chars.getAndAdvance());
    EXPECT_EQ ((yup_wchar) 'A', chars.getAndAdvance());
}

TEST_F (YAMLTests, QuotedScalars)
{
    EXPECT_EQ ("hello world", YAML::fromString ("\"hello world\"").toString());
    EXPECT_EQ ("it's", YAML::fromString ("'it''s'").toString());
    EXPECT_EQ ("line\nbreak", YAML::fromString ("\"line\\nbreak\"").toString());
    EXPECT_EQ ("tab\there", YAML::fromString ("\"tab\\there\"").toString());
    EXPECT_EQ ("quote \" inside", YAML::fromString ("\"quote \\\" inside\"").toString());
    EXPECT_EQ (String::fromUTF8 ("café"), YAML::fromString (String::fromUTF8 ("\"caf\\u00e9\"")).toString());
    EXPECT_EQ ("", YAML::fromString ("\"\"").toString());

    // A quoted number stays a string
    auto n = YAML::fromString ("\"42\"");
    EXPECT_TRUE (n.isString());
    EXPECT_EQ ("42", n.toString());

    // UTF-16 surrogate pairs combine into a single code point
    auto astral = YAML::fromString ("\"\\uD83D\\uDE00\"");
    EXPECT_TRUE (astral.isString());
    EXPECT_EQ (0x1f600, static_cast<int> (astral.toString().getCharPointer().getAndAdvance()));

    // An unpaired high surrogate followed by what looks like (but isn't) a valid low
    // surrogate escape must not swallow that second escape's decoded character.
    auto unpaired = YAML::fromString ("\"\\uD800\\u0041\"");
    EXPECT_TRUE (unpaired.isString());
    auto unpairedChars = unpaired.toString().getCharPointer();
    EXPECT_EQ ((yup_wchar) 0xd800, unpairedChars.getAndAdvance());
    EXPECT_EQ ((yup_wchar) 'A', unpairedChars.getAndAdvance());
}

TEST_F (YAMLTests, BlockScalars)
{
    auto literal = YAML::parse ("text: |\n  line1\n  line2");
    EXPECT_EQ ("line1\nline2\n", literal["text"].toString());

    auto stripped = YAML::parse ("text: |-\n  line1\n  line2");
    EXPECT_EQ ("line1\nline2", stripped["text"].toString());

    auto folded = YAML::parse ("text: >\n  folded\n  text");
    EXPECT_EQ ("folded text\n", folded["text"].toString());

    auto foldedBlank = YAML::parse ("text: >-\n  a\n\n  b");
    EXPECT_EQ ("a\nb", foldedBlank["text"].toString());

    auto sequenceItem = YAML::parse ("- |\n  hello\n- world");
    EXPECT_TRUE (sequenceItem.isArray());
    EXPECT_EQ ("hello\n", sequenceItem[0].toString());
    EXPECT_EQ ("world", sequenceItem[1].toString());

    // Literal block scalars preserve trailing whitespace on each content line
    auto trailingSpaces = YAML::parse ("key: |\n  line with trailing spaces  \n");
    EXPECT_EQ ("line with trailing spaces  \n", trailingSpaces["key"].toString());
}

TEST_F (YAMLTests, AnchorsAliasesAndMergeKeys)
{
    auto v = YAML::parse (R"(defaults: &defaults
  a: 1
  b: 2
derived:
  <<: *defaults
  b: 3
  c: 4)");

    EXPECT_TRUE (v.isObject());
    EXPECT_EQ (1, static_cast<int> (v["derived"]["a"]));
    EXPECT_EQ (3, static_cast<int> (v["derived"]["b"])); // explicit key wins
    EXPECT_EQ (4, static_cast<int> (v["derived"]["c"]));
    EXPECT_EQ (2, static_cast<int> (v["defaults"]["b"])); // anchored map unchanged

    auto aliases = YAML::parse (R"(first: &f hello
second: *f
items: &items
  - 1
  - 2
copy: *items)");

    EXPECT_EQ ("hello", aliases["second"].toString());
    EXPECT_TRUE (aliases["copy"].isArray());
    EXPECT_EQ (2, aliases["copy"].size());
    EXPECT_EQ (2, static_cast<int> (aliases["copy"][1]));

    auto flowMerge = YAML::parse (R"(base: &b {x: 1, y: 2}
derived: {<<: *b, y: 3})");

    EXPECT_EQ (1, static_cast<int> (flowMerge["derived"]["x"]));
    EXPECT_EQ (3, static_cast<int> (flowMerge["derived"]["y"]));
}

TEST_F (YAMLTests, Comments)
{
    auto v = YAML::parse (R"(# top-level comment
name: yup # inline comment
# another comment
value: 1)");

    EXPECT_EQ ("yup", v["name"].toString());
    EXPECT_EQ (1, static_cast<int> (v["value"]));
}

TEST_F (YAMLTests, EmptyContainersAndNull)
{
    auto v = YAML::parse ("empty_map: {}\nempty_list: []\nnothing: null\ncomment_only: # nothing");

    EXPECT_TRUE (v["empty_map"].isObject());
    EXPECT_EQ (0, v["empty_map"].size());
    EXPECT_TRUE (v["empty_list"].isArray());
    EXPECT_EQ (0, v["empty_list"].size());
    EXPECT_TRUE (v["nothing"].isVoid());
    EXPECT_TRUE (v["comment_only"].isVoid());
}

TEST_F (YAMLTests, QuotedKeysAndSpecialKeys)
{
    auto v = YAML::parse ("\"my key\": 1\n\"true\": 2\n\"123\": 3\nplain: 4");

    EXPECT_EQ (1, static_cast<int> (v["my key"]));
    EXPECT_EQ (2, static_cast<int> (v["true"]));
    EXPECT_EQ (3, static_cast<int> (v["123"]));
    EXPECT_EQ (4, static_cast<int> (v["plain"]));

    // A data property literally named "<<" must round-trip as a normal key
    auto o = new DynamicObject();
    o->setProperty ("<<", 1);
    var data (o);

    auto text = YAML::toString (data);
    EXPECT_EQ ("\"<<\": 1", text);

    auto parsed = YAML::parse (text);
    EXPECT_EQ (1, static_cast<int> (parsed["<<"]));
}

TEST_F (YAMLTests, NonFiniteDoubles)
{
    EXPECT_EQ (".inf", YAML::toString (var (std::numeric_limits<double>::infinity())));
    EXPECT_EQ ("-.inf", YAML::toString (var (-std::numeric_limits<double>::infinity())));
    EXPECT_EQ (".nan", YAML::toString (var (std::numeric_limits<double>::quiet_NaN())));

    EXPECT_FALSE (yup_isfinite (static_cast<double> (YAML::fromString (".inf"))));
    EXPECT_FALSE (yup_isfinite (static_cast<double> (YAML::fromString ("-.inf"))));
    EXPECT_TRUE (static_cast<double> (YAML::fromString (".nan")) != static_cast<double> (YAML::fromString (".nan")));
}

TEST_F (YAMLTests, FromStringPrimitives)
{
    EXPECT_EQ (42, static_cast<int> (YAML::fromString ("42")));
    EXPECT_TRUE (YAML::fromString ("\"quoted\"").isString());
    EXPECT_TRUE (YAML::fromString ("[1, 2]").isArray());
    EXPECT_TRUE (YAML::fromString ("{a: 1}").isObject());
    EXPECT_TRUE (YAML::fromString ("plain scalar").isString());
    EXPECT_EQ ("hello", YAML::fromString ("hello").toString());
}

TEST_F (YAMLTests, ParseRequiresContainer)
{
    var result;

    EXPECT_TRUE (YAML::parse ("42", result).failed());
    EXPECT_TRUE (YAML::parse ("\"string\"", result).failed());
    EXPECT_TRUE (YAML::parse ("plain", result).failed());
    EXPECT_TRUE (YAML::parse ("{a: 1}", result).wasOk());
    EXPECT_TRUE (YAML::parse ("[1]", result).wasOk());
}

TEST_F (YAMLTests, ErrorCases)
{
    var result;

    EXPECT_TRUE (YAML::parse ("a: [1, 2", result).failed());
    EXPECT_TRUE (YAML::parse ("key: \"unterminated", result).failed());
    EXPECT_TRUE (YAML::parse ("a: 1\n  b: 2", result).failed());
    EXPECT_TRUE (YAML::parse ("---\na: 1", result).failed());
    EXPECT_TRUE (YAML::parse ("a: *missing", result).failed());
    EXPECT_TRUE (YAML::parse ("key: a: b", result).failed());
    EXPECT_TRUE (YAML::parse ("a: 1\nb: 2\nextra", result).failed());
    EXPECT_TRUE (YAML::parse ("\ta: 1", result).failed());

    auto error = YAML::parse ("a: *missing", result).getErrorMessage();
    EXPECT_TRUE (error.contains ("Undefined alias"));
}

TEST_F (YAMLTests, ParseFromStreamAndFile)
{
    MemoryInputStream stream ("name: from-stream\nvalue: 7");
    auto fromStream = YAML::parse (stream);
    EXPECT_EQ ("from-stream", fromStream["name"].toString());
    EXPECT_EQ (7, static_cast<int> (fromStream["value"]));

    auto file = File::createTempFile ("yaml_test");
    file.replaceWithText ("name: from-file\nvalue: 8");
    auto fromFile = YAML::parse (file);
    EXPECT_EQ ("from-file", fromFile["name"].toString());
    EXPECT_EQ (8, static_cast<int> (fromFile["value"]));
    file.deleteFile();
}

TEST_F (YAMLTests, SpacingAndFormatOptions)
{
    var v = YAML::parse ("a: 1\nb:\n  - 1\n  - 2");

    EXPECT_EQ ("a: 1\nb:\n  - 1\n  - 2", YAML::toString (v, false));
    EXPECT_EQ ("{a: 1, b: [1, 2]}", YAML::toString (v, true));
    EXPECT_EQ ("{a:1,b:[1,2]}", YAML::toString (v, YAML::FormatOptions {}.withSpacing (YAML::Spacing::none)));
    EXPECT_EQ ("{a: 1, b: [1, 2]}", YAML::toString (v, YAML::FormatOptions {}.withSpacing (YAML::Spacing::singleLine)));
}

TEST_F (YAMLTests, FlowMapKeyWithEmbeddedColonRoundTrips)
{
    // A flow-map key containing a bare ':' must be quoted on write, since the
    // reader's flow key scanner has no lookahead and would otherwise mis-split it.
    auto o = new DynamicObject();
    o->setProperty ("a:b", 1);
    var data (o);

    auto text = YAML::toString (data, YAML::FormatOptions {}.withSpacing (YAML::Spacing::singleLine));
    EXPECT_EQ ("{\"a:b\": 1}", text);

    auto parsed = YAML::parse (text);
    EXPECT_TRUE (parsed.isObject());
    EXPECT_EQ (1, static_cast<int> (parsed["a:b"]));

    // The common no-space JSON-style case must still parse as a single key.
    auto noSpace = YAML::fromString ("{a:1}");
    EXPECT_EQ (1, static_cast<int> (noSpace["a"]));
}

TEST_F (YAMLTests, JsonCompatibility)
{
    auto v = YAML::parse (R"({"name": "yup", "tags": ["core", "yaml"], "nested": {"a": 1}})");

    EXPECT_EQ ("yup", v["name"].toString());
    EXPECT_EQ ("core", v["tags"][0].toString());
    EXPECT_EQ ("yaml", v["tags"][1].toString());
    EXPECT_EQ (1, static_cast<int> (v["nested"]["a"]));
}

TEST_F (YAMLTests, EscapeStringAndParseQuotedString)
{
    EXPECT_EQ ("a\\nb", YAML::escapeString ("a\nb"));
    EXPECT_EQ ("quote \\\" here", YAML::escapeString ("quote \" here"));

    String input = "\"hello\\nworld\"";
    String::CharPointerType t = input.getCharPointer();
    var result;

    EXPECT_TRUE (YAML::parseQuotedString (t, result).wasOk());
    EXPECT_EQ ("hello\nworld", result.toString());

    String singleQuoted = "'it''s'";
    t = singleQuoted.getCharPointer();
    EXPECT_TRUE (YAML::parseQuotedString (t, result).wasOk());
    EXPECT_EQ ("it's", result.toString());

    String notQuoted = "hello";
    t = notQuoted.getCharPointer();
    EXPECT_TRUE (YAML::parseQuotedString (t, result).failed());

    // Same unpaired-surrogate defect as QuotedScalars/FlowQuotedUnpairedHighSurrogateEscape,
    // but through the public CharPointerType-based overload (a third, independent code path).
    String surrogate = "\"\\uD800\\u0041\"";
    t = surrogate.getCharPointer();
    EXPECT_TRUE (YAML::parseQuotedString (t, result).wasOk());
    auto surrogateChars = result.toString().getCharPointer();
    EXPECT_EQ ((yup_wchar) 0xd800, surrogateChars.getAndAdvance());
    EXPECT_EQ ((yup_wchar) 'A', surrogateChars.getAndAdvance());
}

TEST_F (YAMLTests, Yaml11Booleans)
{
    EXPECT_TRUE (YAML::fromString ("y").isBool());
    EXPECT_TRUE (static_cast<bool> (YAML::fromString ("y")));
    EXPECT_TRUE (YAML::fromString ("Y").isBool());
    EXPECT_TRUE (static_cast<bool> (YAML::fromString ("Y")));

    EXPECT_TRUE (YAML::fromString ("n").isBool());
    EXPECT_FALSE (static_cast<bool> (YAML::fromString ("n")));
    EXPECT_TRUE (YAML::fromString ("N").isBool());
    EXPECT_FALSE (static_cast<bool> (YAML::fromString ("N")));

    // YES/NO uppercase variants
    EXPECT_TRUE (static_cast<bool> (YAML::fromString ("YES")));
    EXPECT_FALSE (static_cast<bool> (YAML::fromString ("NO")));

    // ON/OFF uppercase variants
    EXPECT_TRUE (static_cast<bool> (YAML::fromString ("ON")));
    EXPECT_FALSE (static_cast<bool> (YAML::fromString ("OFF")));
}

TEST_F (YAMLTests, Int64Min)
{
    auto v = YAML::fromString ("-9223372036854775808");
    EXPECT_TRUE (v.isInt64());
    EXPECT_EQ (std::numeric_limits<int64>::min(), static_cast<int64> (v));

    // INT64_MAX should still be int64 (not fitting in int)
    auto maxV = YAML::fromString ("9223372036854775807");
    EXPECT_TRUE (maxV.isInt64());
    EXPECT_EQ (std::numeric_limits<int64>::max(), static_cast<int64> (maxV));
}

#if ! YUP_WASM
TEST_F (YAMLTests, DepthLimitExceeded)
{
    var result;

    // Build a deeply nested array that exceeds the max depth
    String deep;
    for (int i = 0; i < 600; ++i)
        deep << "[";
    for (int i = 0; i < 600; ++i)
        deep << "]";

    EXPECT_TRUE (YAML::parse (deep, result).failed());

    // Deeply nested map
    String deepMap;
    for (int i = 0; i < 600; ++i)
        deepMap << "k" << String (i) << ": {";
    deepMap << "v: 1";
    for (int i = 0; i < 600; ++i)
        deepMap << "}";

    EXPECT_TRUE (YAML::parse (deepMap, result).failed());
}
#endif

TEST_F (YAMLTests, DuplicateAnchor)
{
    var result;

    // Same anchor name defined twice
    EXPECT_TRUE (YAML::parse ("a: &dup 1\nb: &dup 2", result).failed());

    // Also test in flow context
    EXPECT_TRUE (YAML::parse ("{a: &dup 1, b: &dup 2}", result).failed());
}

TEST_F (YAMLTests, CyclicAlias)
{
    var result;

    // Direct self-reference cycle
    EXPECT_TRUE (YAML::parse ("&a [*a]", result).failed());

    // Indirect cycle through a map
    EXPECT_TRUE (YAML::parse ("&a {b: *a}", result).failed());
}

TEST_F (YAMLTests, ExtendedEscapeSequences)
{
    // \0 null character
    EXPECT_EQ (String::fromUTF8 ("a\0b", 3), YAML::fromString ("\"a\\0b\"").toString());

    // \v vertical tab
    EXPECT_EQ ("a\vb", YAML::fromString ("\"a\\vb\"").toString());

    // \e escape
    EXPECT_EQ (String::charToString (0x1b), YAML::fromString ("\"\\e\"").toString());

    // \xNN hex escape
    EXPECT_EQ ("A", YAML::fromString ("\"\\x41\"").toString());

    // \N next line (0x85)
    EXPECT_EQ (String::charToString (0x85), YAML::fromString ("\"\\N\"").toString());

    // \_ non-breaking space (0xa0)
    EXPECT_EQ (String::charToString (0xa0), YAML::fromString ("\"\\_\"").toString());

    // \L line separator (0x2028)
    EXPECT_EQ (String::charToString (0x2028), YAML::fromString ("\"\\L\"").toString());

    // \P paragraph separator (0x2029)
    EXPECT_EQ (String::charToString (0x2029), YAML::fromString ("\"\\P\"").toString());

    // \U0001F600 astral character via 8-digit hex
    auto astral8 = YAML::fromString ("\"\\U0001F600\"");
    EXPECT_EQ (0x1f600, static_cast<int> (astral8.toString().getCharPointer().getAndAdvance()));

    // escaped space
    EXPECT_EQ ("a b", YAML::fromString ("\"a\\ b\"").toString());

    // escaped forward slash
    EXPECT_EQ ("a/b", YAML::fromString ("\"a\\/b\"").toString());
}

TEST_F (YAMLTests, FormatterEscapeOutput)
{
    // escapeString for various control characters
    EXPECT_EQ ("\\n", YAML::escapeString ("\n"));
    EXPECT_EQ ("\\t", YAML::escapeString ("\t"));
    EXPECT_EQ ("\\r", YAML::escapeString ("\r"));
    EXPECT_EQ ("\\\\", YAML::escapeString ("\\"));
    EXPECT_EQ ("\\\"", YAML::escapeString ("\""));
    EXPECT_EQ ("\\a", YAML::escapeString ("\a"));
    EXPECT_EQ ("\\b", YAML::escapeString ("\b"));
    EXPECT_EQ ("\\f", YAML::escapeString ("\f"));
    EXPECT_EQ ("\\v", YAML::escapeString ("\v"));
    EXPECT_EQ ("\\e", YAML::escapeString (String::charToString (0x1b)));

    // Non-ASCII characters should be unicode-escaped
    auto escaped = YAML::escapeString (String::fromUTF8 ("café"));
    EXPECT_TRUE (escaped.contains ("\\u00e9") || escaped.contains ("\\u00E9"));

    // toString with spacing none
    var v = YAML::parse ("a: 1\nb:\n  - 1\n  - 2");
    EXPECT_EQ ("{a:1,b:[1,2]}", YAML::toString (v, YAML::FormatOptions {}.withSpacing (YAML::Spacing::none)));
}

TEST_F (YAMLTests, WriteToStream)
{
    MemoryOutputStream mo;
    var data = YAML::parse ("a: 1\nb: 2");
    YAML::writeToStream (mo, data, false);
    EXPECT_EQ ("a: 1\nb: 2", mo.toUTF8());

    MemoryOutputStream mo2;
    YAML::writeToStream (mo2, data, YAML::FormatOptions {}.withSpacing (YAML::Spacing::singleLine));
    EXPECT_EQ ("{a: 1, b: 2}", mo2.toUTF8());
}

TEST_F (YAMLTests, FlowSequenceOfMaps)
{
    auto v = YAML::parse (R"(- name: a
  value: 1
- name: b
  value: 2)");

    EXPECT_TRUE (v.isArray());
    EXPECT_EQ (2, v.size());
    EXPECT_TRUE (v[0].isObject());
    EXPECT_EQ ("a", v[0]["name"].toString());
    EXPECT_EQ (1, static_cast<int> (v[0]["value"]));
}

TEST_F (YAMLTests, KeepChompingBlockScalar)
{
    // + chomping indicator keeps all trailing newlines
    auto v = YAML::parse ("text: |+\n  line1\n  line2\n");
    EXPECT_EQ ("line1\nline2\n", v["text"].toString());
}

TEST_F (YAMLTests, ExplicitIndentBlockScalar)
{
    // Explicit indent indicator on block scalar
    auto v = YAML::parse ("text: |2\n   indented\n   content");
    EXPECT_EQ (" indented\n content\n", v["text"].toString());
}

TEST_F (YAMLTests, SequenceItemWithAnchor)
{
    auto v = YAML::parse ("- &anchor value\n- *anchor");
    EXPECT_TRUE (v.isArray());
    EXPECT_EQ (2, v.size());
    EXPECT_EQ ("value", v[0].toString());
    EXPECT_EQ ("value", v[1].toString());
}

TEST_F (YAMLTests, MultilineFlowSequence)
{
    auto v = YAML::parse (R"([
  1,
  2,
  3
])");
    EXPECT_TRUE (v.isArray());
    EXPECT_EQ (3, v.size());
    EXPECT_EQ (1, static_cast<int> (v[0]));
    EXPECT_EQ (2, static_cast<int> (v[1]));
    EXPECT_EQ (3, static_cast<int> (v[2]));
}

TEST_F (YAMLTests, InvalidEscapeSequence)
{
    var result;
    // \q is not a valid escape
    EXPECT_TRUE (YAML::parse ("\"\\q\"", result).failed());
}

TEST_F (YAMLTests, UnexpectedContentAfterFlowCollection)
{
    var result;
    // Extra content after a flow collection on the same line
    EXPECT_TRUE (YAML::parse ("{a: 1} extra", result).failed());
}

TEST_F (YAMLTests, UnterminatedFlowCollection)
{
    var result;
    EXPECT_TRUE (YAML::parse ("{a: 1", result).failed());
}

TEST_F (YAMLTests, AliasAnchorAsMapKey)
{
    var result;
    // Aliases and anchors are not allowed as map keys
    EXPECT_TRUE (YAML::parse ("*alias: value", result).failed());
    EXPECT_TRUE (YAML::parse ("&anchor: value", result).failed());
}
